/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi details window
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
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/gui_chart.h"



karte_t *convoi_detail_t::welt = NULL;



convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: gui_frame_t( cnv->get_name(), cnv->get_besitzer() ),
  scrolly(&veh_info),
  veh_info(cnv)
{
	this->cnv = cnv;
	welt = cnv->get_welt();

	sale_button.init(button_t::roundbox, "Verkauf", koord(BUTTON4_X, 0), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sale_button.set_tooltip("Remove vehicle from map. Use with care!");
	sale_button.add_listener(this);
	add_komponente(&sale_button);

	withdraw_button.init(button_t::roundbox, "withdraw", koord(BUTTON3_X, 0), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
	withdraw_button.add_listener(this);
	add_komponente(&withdraw_button);

	scrolly.set_pos(koord(0, 2+16+5*LINESPACE));
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+17*(LINESPACE+1)+button_t::gui_scrollbar_size.y-6));
	set_min_windowsize(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+3*(LINESPACE+1)+button_t::gui_scrollbar_size.y-3));

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
		}
		else {
			sale_button.disable();
			withdraw_button.disable();
		}
		withdraw_button.pressed = cnv->get_withdraw();

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

		number_to_string( number, (double)cnv->get_total_distance_traveled(), 0 );
		buf.clear();
		buf.printf( translator::translate("Odometer: %s km"), number );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;

		buf.clear();
		buf.printf("%s %i", translator::translate("Station tiles:"), cnv->get_tile_length() );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;

		money_to_string( number, cnv->calc_restwert() / 100.0 );
		buf.clear();
		buf.printf("%s %s", translator::translate("Restwert:"), number );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;

		money_to_string( number, cnv->calc_restwert() / 100.0 );
		buf.clear();
		buf.printf(translator::translate("Bonusspeed: %i km/h"), cnv->get_speedbonus_kmh() );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, COL_BLACK, true );
		offset_y += LINESPACE;
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
	cnv = convoihandle_t();
}


void convoi_detail_t::rdwr(loadsave_t *file)
{
	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		koord3d cnv_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		cnv_pos.rdwr( file );
		file->rdwr_str( name, lengthof(name) );
	}
	else {
		// handle
		convoi_t::rdwr_convoihandle_t(file, cnv);
	}
	// window size, scroll position
	koord gr = get_fenstergroesse();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();

	gr.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );

	if(  file->is_loading()  ) {
		// convoy vanished
		if(  !cnv.is_bound()  ) {
			dbg->error( "convoi_detail_t::rdwr()", "Could not restore convoi detail window of (%d)", cnv.get_id() );
			destroy_win( this );
			return;
		}

		// now we can open the window ...
		koord const& pos = win_get_pos(this);
		convoi_detail_t *w = new convoi_detail_t(cnv);
		create_win(pos.x, pos.y, w, w_info, magic_convoi_detail + cnv.get_id());
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



/*
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_vehicleinfo_t::zeichnen(koord offset)
{
	// keep previous maximum width
	int x_size = get_groesse().x-51-pos.x;

	int total_height = 0;
	if(cnv.is_bound()) {
		char number[64];
		cbuffer_t buf;

		// for bonus stuff
		const sint32 ref_kmh = cnv->get_welt()->get_average_speed( cnv->front()->get_waytype() );
		const sint32 cnv_kmh = (cnv->front()->get_waytype() == air_wt) ? speed_to_kmh(cnv->get_min_top_speed()) : cnv->get_speedbonus_kmh();
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
			display_base_img(bild,11-x+pos.x+offset.x,pos.y+offset.y+total_height-y+2,cnv->get_besitzer()->get_player_nr(),false,true);
			w = max(40,w+4)+11;

			// now add the other info
			int extra_y=0;

			// name of this
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate(v->get_besch()->get_name()), ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;

			// age
			buf.clear();
			{
				const sint32 month = v->get_insta_zeit();
				buf.printf( "%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			}
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_BLACK, true );
			extra_y += LINESPACE;

			// value
			money_to_string( number, v->calc_restwert() / 100.0 );
			buf.clear();
			buf.printf( "%s %s", translator::translate("Restwert:"), number );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			// power
			if(v->get_besch()->get_leistung()>0) {
				buf.clear();
				buf.printf( "%s %i kW, %s %.2f", translator::translate("Power:"), v->get_besch()->get_leistung(), translator::translate("Gear:"), v->get_besch()->get_gear()/64.0 );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
				extra_y += LINESPACE;
			}

			// friction
			buf.clear();
			buf.printf( "%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			if(v->get_fracht_max() > 0) {

				// bonus stuff
				int len = 5+display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, COL_BLACK, true );
				const sint32 grundwert128 = v->get_fracht_typ()->get_preis() * v->get_besitzer()->get_welt()->get_settings().get_bonus_basefactor();	// bonus price will be always at least this
				const sint32 grundwert_bonus = v->get_fracht_typ()->get_preis()*(1000l+kmh_base*v->get_fracht_typ()->get_speed_bonus());
				const sint32 price = (v->get_fracht_max()*(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus))/3000 - v->get_betriebskosten();
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
			else {
				// bonus stuff
				int len = 5+display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, COL_BLACK, true );
				money_to_string( number, v->get_betriebskosten()/(-100.0) );
				display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, v->get_betriebskosten()<=0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;

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
