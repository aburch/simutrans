/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "convoi_detail_t.h"

#include "../simunits.h"
#include "../simconvoi.h"
#include "../vehicle/simvehikel.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../simwin.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
// @author hsiegeln
#include "../simline.h"
#include "../boden/grund.h"
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/gui_chart.h"
#include "components/list_button.h"

karte_t *convoi_detail_t::welt = NULL;


convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: gui_frame_t( cnv->get_name(), cnv->get_besitzer() ),
  scrolly(&veh_info),
  veh_info(cnv)
{
	this->cnv = cnv;
	welt = cnv->get_welt();

	sale_button.init(button_t::roundbox, "verkaufen", koord(BUTTON4_X, 0), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sale_button.set_tooltip("Remove vehicle from map. Use with care!");
	sale_button.add_listener(this);
	add_komponente(&sale_button);

	withdraw_button.init(button_t::roundbox, "withdraw", koord(BUTTON3_X, 0), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
	withdraw_button.add_listener(this);
	add_komponente(&withdraw_button);

	retire_button.init(button_t::roundbox, "Retire", koord(BUTTON3_X, 16), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	retire_button.set_tooltip("Convoi is sent to depot when all wagons are empty.");
	add_komponente(&retire_button);
	retire_button.add_listener(this);

	// From new Standard (111.1) - probably associated with new features logged for convoys not used in Standard.
	//scrolly.set_pos(koord(0, 2+16+5*LINESPACE));
	scrolly.set_pos(koord(0, 50));
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);

	set_fenstergroesse(koord(TOTAL_WIDTH, TITLEBAR_HEIGHT+50+17*(LINESPACE+1)+scrollbar_t::BAR_SIZE-6));
	set_min_windowsize(koord(TOTAL_WIDTH, TITLEBAR_HEIGHT+50+3*(LINESPACE+1)+scrollbar_t::BAR_SIZE-3));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}


void convoi_detail_t::zeichnen(koord pos, koord gr)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		if(cnv->get_besitzer()==cnv->get_welt()->get_active_player()) {
			withdraw_button.enable();
			sale_button.enable();
			retire_button.enable();
		}
		else {
			withdraw_button.disable();
			sale_button.disable();
			retire_button.disable();
		}
		withdraw_button.pressed = cnv->get_withdraw();
		retire_button.pressed = cnv->get_depot_when_empty();

		// all gui stuff set => display it
		gui_frame_t::zeichnen(pos, gr);
		int offset_y = pos.y+2+16;

		// current value
		char number[64];
		cbuffer_t buf;

		// current power
		buf.printf( translator::translate("Leistung: %d kW"), cnv->get_sum_leistung() );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;

		number_to_string( number, cnv->get_total_distance_traveled(), 0 );
		buf.clear();
		buf.printf( translator::translate("Odometer: %s km"), number );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;

		buf.clear();
		buf.printf("%s %i", translator::translate("Station tiles:"), cnv->get_tile_length() );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;

		// current resale value
		money_to_string( number, cnv->calc_restwert()/100.0 );
		buf.clear();
		buf.printf("%s %s", translator::translate("Restwert:"), number );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;
		
		// Bernd Gabriel, 16.06.2009: current average obsolescence increase percentage
		uint16 count = cnv->get_vehikel_anzahl();
		if (count > 0)
		{

		/* Bernd Gabriel, 17.06.2009:
		The average percentage tells nothing about the real cost increase: If a cost-intensive 
		loco is very old and at max increase (1 * 400% * 1000 cr/month, but 15 low-cost cars are 
		brand new (15 * 100% * 100 cr/month), an average percentage of 
		(1 * 400% + 15 * 100%) / 16 = 118.75% does not tell the truth. Actually we pay
		(1 * 400% * 1000 + 15 * 100% * 100) / (1 * 1000 + 15 * 100) = 220% twice as much as in the
		early years of the loco.

			uint32 percentage = 0;
			karte_t *welt = cnv->get_welt();
			for (uint16 i = 0; i < count; i++) {
				percentage += cnv->get_vehikel(i)->get_besch()->calc_running_cost(welt, 10000);
			}
			percentage = percentage / (count * 100) - 100;
			if (percentage > 0)
			{
				sprintf( tmp, "%s: %d%%", translator::translate("Obsolescence increase"), percentage);
				display_proportional_clip( pos.x+10, offset_y, tmp, ALIGN_LEFT, COL_DARK_BLUE, true );
				offset_y += LINESPACE;
			}
		On the other hand: a single effective percentage does not tell the truth as well. Supposed we 
		calculate the percentage from the costs per km, the relations for the month costs can widely differ.
		Therefore I show different values for running and monthly costs:
		*/
			uint32 run_actual = 0, run_nominal = 0, run_percent = 0;
			uint32 mon_actual = 0, mon_nominal = 0, mon_percent = 0;
			karte_t *welt = cnv->get_welt();
			for (uint16 i = 0; i < count; i++) {
				const vehikel_besch_t *besch = cnv->get_vehikel(i)->get_besch();
				run_nominal += besch->get_betriebskosten();
				run_actual  += besch->get_betriebskosten(welt);
				mon_nominal += besch->get_fixed_maintenance();
				mon_actual  += besch->get_fixed_maintenance(welt);
			}
			buf.clear();
			if (run_nominal) run_percent = ((run_actual - run_nominal) * 100) / run_nominal;
			if (mon_nominal) mon_percent = ((mon_actual - mon_nominal) * 100) / mon_nominal;
			if (run_percent)
			{
				if (mon_percent)
				{
					buf.printf("%s: %d%%/km %d%%/mon", translator::translate("Obsolescence increase"), run_percent, mon_percent);
				}
				else
				{
					buf.printf("%s: %d%%/km", translator::translate("Obsolescence increase"), run_percent);
				}
			}
			else
			{
				if (mon_percent)
				{
					buf.printf("%s: %d%%/mon", translator::translate("Obsolescence increase"), mon_percent);
				}
			}
			if (buf.len() > 0)
			{
				display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_DARK_BLUE, true );
				offset_y += LINESPACE;
			}

		}
	}
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool convoi_detail_t::action_triggered(gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if(cnv.is_bound()) {
		if(komp==&sale_button) {
			cnv->call_convoi_tool( 'x', NULL );
			return true;
		}
		else if(komp==&withdraw_button) {
			cnv->call_convoi_tool( 'w', NULL );
			return true;
		}
		else if(komp==&retire_button) {
			cnv->call_convoi_tool( 'T', NULL );
			return true;
		}
	}
	return false;
}



/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void convoi_detail_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);
	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
}


// dummy for loading
convoi_detail_t::convoi_detail_t(karte_t *w)
: gui_frame_t("", NULL ),
  scrolly(&veh_info),
  veh_info(convoihandle_t())
{
	welt = w;
}


void convoi_detail_t::rdwr(loadsave_t *file)
{
	koord3d cnv_pos;
	char name[128];
	koord gr = get_fenstergroesse();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		cnv_pos = cnv->front()->get_pos();
		tstrncpy(name, cnv->get_name(), lengthof(name));
	}
	cnv_pos.rdwr( file );
	file->rdwr_str( name, lengthof(name) );
	gr.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		// find convoi by name and position
		if(  grund_t *gr = welt->lookup(cnv_pos)  ) {
			for(  uint8 i=0;  i<gr->get_top();  i++  ) {
				if(  gr->obj_bei(i)->is_moving()  ) {
					vehikel_t const* const v = dynamic_cast<vehikel_t *>(gr->obj_bei(i));
					if(  v  &&  v->get_convoi()  ) {
						if(  strcmp(v->get_convoi()->get_name(),name)==0  ) {
							cnv = v->get_convoi()->self;
							break;
						}
					}
				}
			}
		}
		// we might be unlucky, then search all convois for a convoi with this name
		if(  !cnv.is_bound()  ) {
			for(  vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end();  i != end;  ++i  ) {
				if(  strcmp( (*i)->get_name(),name)==0  ) {
					cnv = *i;
					break;
				}
			}
		}
		// still not found?
		if(  !cnv.is_bound()  ) {
			dbg->error( "convoi_detail_t::rdwr()", "Could not restore convoi detail window of %s", name );
			destroy_win( this );
			return;
		}
		// now we can open the window ...
		KOORD_VAL xpos = win_get_posx( this );
		KOORD_VAL ypos = win_get_posy( this );
		convoi_detail_t *w = new convoi_detail_t(cnv);
		create_win( xpos, ypos, w, w_info, magic_convoi_detail+cnv.get_id() );
		w->set_fenstergroesse( gr );
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		cnv = convoihandle_t();
		destroy_win( this );
	}
}


// component for vehicle display
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
	// keep previous maximum width
	int x_size = get_groesse().x-51-pos.x;

	int total_height = LINESPACE;
	if(cnv.is_bound()) {
		char number[64];
		cbuffer_t buf;

		// for bonus stuff
		const sint32 ref_kmh = cnv->get_welt()->get_average_speed( cnv->front()->get_waytype() );
		const sint32 cnv_kmh = cnv->get_line().is_bound() ? cnv->get_line()->get_finance_history(1, LINE_AVERAGE_SPEED): cnv->get_finance_history(1, CONVOI_AVERAGE_SPEED);
		const sint32 kmh_base = (100 * cnv_kmh) / ref_kmh - 100;

		static cbuffer_t freight_info;
		for(unsigned veh=0;  veh<cnv->get_vehikel_anzahl(); veh++ ) {
			vehikel_t *v=cnv->get_vehikel(veh);
			int returns = 0;
			freight_info.clear();

			// first image
			KOORD_VAL x, y, w, h;
			const image_id bild=v->get_basis_bild();
			display_get_base_image_offset(bild, &x, &y, &w, &h );
			display_base_img(bild,11-x+pos.x+offset.x,pos.y+offset.y+total_height-y-LINESPACE+2,cnv->get_besitzer()->get_player_nr(),false,true);
			w = max(40,w+4)+11;

			// now add the other info
			int extra_y=0;

			// name of this
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate(v->get_besch()->get_name()), ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;

			// age
			sint32 month = v->get_insta_zeit();
			buf.clear();
			buf.printf( "%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;
			
			// Bernd Gabriel, 16.06.2009: current average obsolescence increase percentage
			uint32 percentage = v->get_besch()->calc_running_cost(v->get_welt(), 100) - 100;
			if (percentage > 0)
			{
				buf.clear();
				buf.printf("%s: %d%%", translator::translate("Obsolescence increase"), percentage);
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_DARK_BLUE, true );
				extra_y += LINESPACE;
			}

			// power
			if(v->get_besch()->get_leistung()>0) {
				buf.clear();
				buf.printf( "%s %i kW, %s %.2f", translator::translate("Power:"), v->get_besch()->get_leistung(), translator::translate("Gear:"), v->get_besch()->get_gear()/64.0 );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
				extra_y += LINESPACE;
			}

			// weight
			buf.clear();
			buf.printf( "%s %dt", translator::translate("Weight:"), v->get_sum_weight());
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			//Catering
			if(v->get_besch()->get_catering_level() > 0)
			{
				buf.clear();
				if(v->get_besch()->get_ware()->get_catg_index() == 1)
				{
					//Catering vehicles that carry mail are treated as TPOs.
					buf.printf( "%s", translator::translate("This is a travelling post office\n"));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
				else
				{
					buf.printf(translator::translate("Catering level: %i\n"), v->get_besch()->get_catering_level());
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}

			//Tilting
			if(v->get_besch()->get_tilting())
			{
				buf.clear();
				buf.printf("%s", translator::translate("This is a tilting vehicle\n"));
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
				extra_y += LINESPACE;
			}

			// friction
			if (v->get_frictionfactor() != 1)
			{
				buf.clear();
				buf.printf("%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
				extra_y += LINESPACE;
			}

			if(v->get_besch()->get_zuladung() > 0)
			{
				char min_loading_time_as_clock[32];
				char max_loading_time_as_clock[32];
				v->get_welt()->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), v->get_besch()->get_min_loading_time());
				v->get_welt()->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), v->get_besch()->get_max_loading_time());
				buf.clear();
				buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
				extra_y += LINESPACE;
			}
			
			if(v->get_fracht_typ()->get_catg_index() == 0)
			{
				buf.clear();
				buf.printf("%s %i", translator::translate("Comfort:"), v->get_comfort() );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
				extra_y += LINESPACE;
			}
			
			if(v->get_fracht_max() > 0) {

				// bonus stuff
				int len = 5+display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, COL_BLACK, true );
				const sint32 grundwert128 = v->get_fracht_typ()->get_preis()<<7;
				const sint32 grundwert_bonus = v->get_fracht_typ()->get_preis()*(1000l+kmh_base*v->get_fracht_typ()->get_speed_bonus());
				const sint32 price = (v->get_fracht_max()*(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus))/30 - v->get_betriebskosten(cnv->get_welt());
				money_to_string( number, price/100.0 );
				display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, price>0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;

				ware_besch_t const& g    = *v->get_fracht_typ();
				char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());
				freight_info.printf("%u/%u%s %s\n", v->get_fracht_menge(), v->get_fracht_max(), translator::translate(v->get_fracht_mass()), name);
				v->get_fracht_info(freight_info);
				// show it
				const int px_len = display_multiline_text( pos.x+offset.x+w, pos.y+offset.y+total_height+extra_y, freight_info, COL_BLACK );
				if(px_len+w>x_size) {
					x_size = px_len+w;
				}

				// count returns
				const char *p=freight_info;
				for(int i=0; i<freight_info.len(); i++ ) {
					if(p[i]=='\n') {
						returns ++;
					}
				}
				extra_y += returns*LINESPACE;
			}
			
			const way_constraints_t &way_constraints = v->get_besch()->get_way_constraints();
			// Permissive way constraints
			// (If vehicle has, way must have)
			// @author: jamespetts
			//for(uint8 i = 0; i < 8; i++)
			for(uint8 i = 0; i < way_constraints.get_count(); i++)
			{
				//if(v->get_besch()->permissive_way_constraint_set(i))
				if(way_constraints.get_permissive(i))
				{
					buf.clear();
					char tmpbuf1[13];
					sprintf(tmpbuf1, "\nMUST USE: ");
					char tmpbuf[14];
					sprintf(tmpbuf, "Permissive %i", i);
					buf.printf("%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}
			
			// Prohibitive way constraints
			// (If way has, vehicle must have)
			// @author: jamespetts
			//for(uint8 i = 0; i < 8; i++)
			for(uint8 i = 0; i < way_constraints.get_count(); i++)
			{
				if(way_constraints.get_prohibitive(i))
				{
					buf.clear();
					char tmpbuf1[13];
					sprintf(tmpbuf1, "\nMAY USE: ");
					char tmpbuf[14];
					sprintf(tmpbuf, "Prohibitive %i", i);
					buf.printf("%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
					extra_y += LINESPACE;
				}
			}
			
			//skip at least five lines
			total_height += max(extra_y+LINESPACE,5*LINESPACE);
		}
	}

	koord gr(max(x_size+pos.x,get_groesse().x),total_height);
	if(  gr!=get_groesse()  ) {
		set_groesse(gr);
	}
}




