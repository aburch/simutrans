/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "convoi_detail_t.h"

#include "../simconvoi.h"
#include "../simdepot.h"
#include "../vehicle/simvehikel.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../simwin.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "fahrplan_gui.h"
// @author hsiegeln
#include "../simlinemgmt.h"
#include "../simline.h"
#include "../boden/grund.h"
#include "messagebox.h"

#include "../utils/simstring.h"

#include "components/gui_chart.h"
#include "components/list_button.h"


convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: gui_frame_t(cnv->get_name(), cnv->get_besitzer()),
  scrolly(&veh_info),
  veh_info(cnv)
{
	this->cnv = cnv;

	sale_button.init(button_t::roundbox, "verkaufen", koord(BUTTON4_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sale_button.add_listener(this);
	sale_button.set_tooltip("Remove vehicle from map. Use with care!");
	add_komponente(&sale_button);

	withdraw_button.set_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	withdraw_button.set_pos(koord(BUTTON3_X,14));
	withdraw_button.set_text("withdraw");
	withdraw_button.set_typ(button_t::roundbox);
	withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
	add_komponente(&withdraw_button);
	withdraw_button.add_listener(this);

	scrolly.set_pos(koord(0, 64));
	add_komponente(&scrolly);

	set_fenstergroesse(koord(TOTAL_WIDTH, 278));

	set_min_windowsize(koord(TOTAL_WIDTH, 194));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
convoi_detail_t::zeichnen(koord pos, koord gr)
{
	if(!cnv.is_bound()) {
		destroy_win(dynamic_cast <gui_fenster_t *>(this));
	}
	else {
		if(cnv->get_besitzer()==cnv->get_welt()->get_active_player()) {
			withdraw_button.enable();
			sale_button.enable();
		}
		else {
			sale_button.disable();
			withdraw_button.disable();
		}
		withdraw_button.pressed = cnv->get_withdraw();

		// all gui stuff set => display it
		veh_info.set_groesse(koord(1,1));
		gui_frame_t::zeichnen(pos, gr);
		int offset_y = pos.y+14+16;

		// current value
		char tmp[256];

		// current power
		sprintf( tmp, translator::translate("Leistung: %d kW"), cnv->get_sum_leistung() );
		display_proportional_clip( pos.x+10, offset_y, tmp, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;

		char buf[32];
		money_to_string( buf, cnv->calc_restwert()/100.0 );
		sprintf( tmp, "%s %s", translator::translate("Restwert:"), buf );
		display_proportional_clip( pos.x+10, offset_y, tmp, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;
	}
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool
convoi_detail_t::action_triggered(gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if(cnv.is_bound()) {
		if(komp==&sale_button) {
			cnv->self_destruct();
			return true;
		}
		else if(komp==&withdraw_button) {
			cnv->set_withdraw(!cnv->get_withdraw());
			cnv->set_no_load(cnv->get_withdraw());
			return true;
		}
	}
	return false;
}



/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void convoi_detail_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
}



gui_vehicleinfo_t::gui_vehicleinfo_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_vehicleinfo_t::zeichnen(koord offset)
{
	int total_height=5;
	if(cnv.is_bound()) {
		char buf[256], tmp[256];

		// for bonus stuff
		const sint32 ref_speed = cnv->get_welt()->get_average_speed( cnv->get_vehikel(0)->get_waytype() );
		const sint32 speed_base = (100*speed_to_kmh(cnv->get_min_top_speed()))/ref_speed-100;

		static cbuffer_t freight_info(1024);
		for(unsigned veh=0;  veh<cnv->get_vehikel_anzahl(); veh++ ) {
			vehikel_t *v=cnv->get_vehikel(veh);
			int returns = 0;
			freight_info.clear();

			// first image
			KOORD_VAL x, y, w, h;
			const image_id bild=v->get_basis_bild();
			display_get_base_image_offset(bild, &x, &y, &w, &h );
			display_base_img(bild,11-x+pos.x+offset.x,pos.y+offset.y+total_height-y,cnv->get_besitzer()->get_player_nr(),false,true);
			w = max(40,w+4)+11;

			// now add the other info
			int extra_y=0;

			// name of this
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate(v->get_besch()->get_name()), ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;

			// age
			sint32 month = v->get_insta_zeit();
			sprintf( buf, "%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12  );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;

			// value
			sint32 current = v->calc_restwert();
			money_to_string( tmp, current/100.0 );
			sprintf( buf, "%s %s", translator::translate("Restwert:"), tmp );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			// power
			if(v->get_besch()->get_leistung()>0) {
				sprintf( buf, "%s %i kW, %s %.2f", translator::translate("Power:"), v->get_besch()->get_leistung(), translator::translate("Gear:"), v->get_besch()->get_gear()/64.0 );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
				extra_y += LINESPACE;
			}

			//Catering
			if(v->get_besch()->get_catering_level() > 0)
			{
				if(v->get_besch()->get_ware()->get_catg_index() == 1)
				{
					//Catering vehicles that carry mail are treated as TPOs.
					sprintf(buf , translator::translate("This is a travelling post office\n"));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
				else
				{
					sprintf(buf, translator::translate("Catering level: %i\n"), v->get_besch()->get_catering_level());
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}

			//Tilting
			if(v->get_besch()->get_tilting())
			{
				sprintf(buf, translator::translate("This is a tilting vehicle\n"));
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
				extra_y += LINESPACE;
			}

			// friction
			sprintf( buf, "%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			if(v->get_fracht_max() > 0) {

				// bonus stuff
				int len = 5+display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, COL_BLACK, true );
				const sint32 grundwert128 = v->get_fracht_typ()->get_preis()<<7;
				const sint32 grundwert_bonus = v->get_fracht_typ()->get_preis()*(1000l+speed_base*v->get_fracht_typ()->get_speed_bonus());
				const sint32 price = (v->get_fracht_max()*(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus))/30 - v->get_betriebskosten(cnv->get_welt());
				money_to_string( tmp, price/100.0 );
				display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, tmp, ALIGN_LEFT, price>0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;

				freight_info.append(v->get_fracht_menge());
				freight_info.append("/");
				freight_info.append(v->get_fracht_max());
				freight_info.append(translator::translate(v->get_fracht_mass()));
				freight_info.append(" ");
				freight_info.append(v->get_fracht_typ()->get_catg() == 0 ? translator::translate(v->get_fracht_typ()->get_name()) : translator::translate(v->get_fracht_typ()->get_catg_name()));
				freight_info.append("\n");
				v->get_fracht_info(freight_info);
				// show it
				display_multiline_text( pos.x+offset.x+w, pos.y+offset.y+total_height+extra_y, freight_info, COL_BLACK );
				// count returns
				const char *p=freight_info;
				for(int i=0; i<freight_info.len(); i++ ) {
					if(p[i]=='\n') {
						returns ++;
					}
				}
				extra_y += returns*LINESPACE;
			}
			
			// Permissive way constraints
			// (If vehicle has, way must have)
			// @author: jamespetts
			for(uint8 i = 0; i < 8; i++)
			{
				if(v->get_besch()->permissive_way_constraint_set(i))
				{
					char tmpbuf1[13];
					sprintf(tmpbuf1, "\nMUST USE: ");
					char tmpbuf[14];
					sprintf(tmpbuf, "Permissive %i", i);
					sprintf(buf, "%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}
			
			// Prohibitive way constraints
			// (If way has, vehicle must have)
			// @author: jamespetts
			for(uint8 i = 0; i < 8; i++)
			{
				if(v->get_besch()->prohibitive_way_constraint_set(i))
				{
					char tmpbuf1[13];
					sprintf(tmpbuf1, "\nMAY USE: ");
					char tmpbuf[14];
					sprintf(tmpbuf, "Prohibitive %i", i);
					sprintf(buf, "%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}
			
			//skip at least five lines
			total_height += max(extra_y,4*LINESPACE)+5;
		}
	}

	// the size will change as soon something is loaded ...
	set_groesse( get_groesse()+koord(0,total_height) );
}
