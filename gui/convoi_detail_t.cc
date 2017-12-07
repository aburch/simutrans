/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
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
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../simworld.h"
#include "../simware.h"
#include "../gui/simwin.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
// @author hsiegeln
#include "../simline.h"
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/gui_chart.h"






convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: gui_frame_t( cnv->get_name(), cnv->get_owner() ),
  scrolly(&veh_info),
  veh_info(cnv)
{
	this->cnv = cnv;

	sale_button.init(button_t::roundbox, "Verkauf", scr_coord(BUTTON4_X, 0));
	sale_button.set_tooltip("Remove vehicle from map. Use with care!");
	sale_button.add_listener(this);
	add_component(&sale_button);

	withdraw_button.init(button_t::roundbox, "withdraw", scr_coord(BUTTON3_X, 0));
	withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
	withdraw_button.add_listener(this);
	add_component(&withdraw_button);

	scrolly.set_pos(scr_coord(0, 2+16+5*LINESPACE));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+17*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-6));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+3*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-3));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


void convoi_detail_t::draw(scr_coord pos, scr_size size)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {
			withdraw_button.enable();
			sale_button.enable();
		}
		else {
			sale_button.disable();
			withdraw_button.disable();
		}
		withdraw_button.pressed = cnv->get_withdraw();

		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		int offset_y = pos.y+2+16;

		// current value
		char number[64];
		cbuffer_t buf;

		// current power
		buf.printf( translator::translate("Leistung: %d kW"), cnv->get_sum_power() );
		display_proportional_clip_rgb( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		offset_y += LINESPACE;

		number_to_string( number, (double)cnv->get_total_distance_traveled(), 0 );
		buf.clear();
		buf.printf( translator::translate("Odometer: %s km"), number );
		display_proportional_clip_rgb( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		offset_y += LINESPACE;

		buf.clear();
		buf.printf("%s %i %s %i", translator::translate("Vehicle count:"), cnv->get_vehicle_count(), translator::translate("Station tiles:"), cnv->get_tile_length());
		display_proportional_clip_rgb( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		offset_y += LINESPACE;

		money_to_string( number, cnv->calc_restwert() / 100.0 );
		buf.clear();
		buf.printf("%s %s", translator::translate("Restwert:"), number );
		display_proportional_clip_rgb( pos.x+10, offset_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;

		buf.clear();
		buf.printf(translator::translate("Bonusspeed: %i km/h"), cnv->get_speedbonus_kmh() );
		display_proportional_clip_rgb( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
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
void convoi_detail_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}


// dummy for loading
convoi_detail_t::convoi_detail_t()
: gui_frame_t("", NULL ),
  scrolly(&veh_info),
  veh_info(convoihandle_t())
{
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
	scr_size size = get_windowsize();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();

	size.rdwr( file );
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
		scr_coord const& pos = win_get_pos(this);
		convoi_detail_t *w = new convoi_detail_t(cnv);
		create_win(pos.x, pos.y, w, w_info, magic_convoi_detail + cnv.get_id());
		w->set_windowsize( size );
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
void gui_vehicleinfo_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w-51-pos.x;
	world_t *welt = world();

	int total_height = 0;
	if(cnv.is_bound()) {
		char number[64];
		cbuffer_t buf;

		// for bonus stuff
		const sint32 cnv_kmh = (cnv->front()->get_waytype() == air_wt) ? speed_to_kmh(cnv->get_min_top_speed()) : cnv->get_speedbonus_kmh();

		static cbuffer_t freight_info;
		for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) {
			vehicle_t *v=cnv->get_vehikel(veh);
			int returns = 0;
			freight_info.clear();

			// first image
			scr_coord_val x, y, w, h;
			const image_id image=v->get_loaded_image();
			display_get_base_image_offset(image, &x, &y, &w, &h );
			display_base_img(image,11-x+pos.x+offset.x,pos.y+offset.y+total_height-y+2,cnv->get_owner()->get_player_nr(),false,true);
			w = max(40,w+4)+11;

			// now add the other info
			int extra_y=0;

			// name of this
			display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;

			// age
			buf.clear();
			{
				const sint32 month = v->get_purchase_time();
				buf.printf( "%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			}
			display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;

			// value
			money_to_string( number, v->calc_sale_value() / 100.0 );
			buf.clear();
			buf.printf( "%s %s", translator::translate("Restwert:"), number );
			display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			// power
			if(v->get_desc()->get_power()>0) {
				buf.clear();
				buf.printf( "%s %i kW, %s %.2f", translator::translate("Power:"), v->get_desc()->get_power(), translator::translate("Gear:"), v->get_desc()->get_gear()/64.0 );
				display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
				extra_y += LINESPACE;
			}

			// friction
			buf.clear();
			buf.printf( "%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
			display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			if(v->get_cargo_max() > 0) {

				// bonus stuff
				int len = 5+display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, SYSCOL_TEXT, true );
				const sint32 price = (v->get_cargo_max()* ware_t::calc_revenue(v->get_cargo_type(), cnv->front()->get_waytype(), cnv_kmh) )/3000 - v->get_operating_cost();
				money_to_string( number, price/100.0 );
				display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, price>0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;

				if(  sint64 cost = welt->scale_with_month_length(v->get_desc()->get_maintenance())  ) {
					KOORD_VAL len = display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Maintenance"), ALIGN_LEFT, SYSCOL_TEXT, true );
					len += display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, ": ", ALIGN_LEFT, SYSCOL_TEXT, true );
					money_to_string( number, cost/(100.0) );
					display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, MONEY_MINUS, true );
					extra_y += LINESPACE;
				}

				goods_desc_t const& g    = *v->get_cargo_type();
				char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());
				freight_info.printf("%u/%u%s %s\n", v->get_total_cargo(), v->get_cargo_max(), translator::translate(v->get_cargo_mass()), name);
				v->get_cargo_info(freight_info);
				// show it
				const int px_len = display_multiline_text_rgb( pos.x+offset.x+w, pos.y+offset.y+total_height+extra_y, freight_info, SYSCOL_TEXT );
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
				// Non-freight (engine)
				int cost = -v->get_operating_cost();
				int len = display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Max income:"), ALIGN_LEFT, SYSCOL_TEXT, true );
				money_to_string( number, cost/(100.0) );
				display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, cost>=0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;
				// Fixed costs
				if(  sint64 cost = welt->scale_with_month_length(v->get_desc()->get_maintenance())  ) {
					KOORD_VAL len = display_proportional_clip_rgb( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Maintenance"), ALIGN_LEFT, SYSCOL_TEXT, true );
					len += display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, ": ", ALIGN_LEFT, SYSCOL_TEXT, true );
					money_to_string( number, cost/(100.0) );
					display_proportional_clip_rgb( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, MONEY_MINUS, true );
					extra_y += LINESPACE;
				}

			}

			//skip at least five lines
			total_height += max(extra_y+LINESPACE,5*LINESPACE);
		}
	}

	scr_size size(max(x_size+pos.x,get_size().w),total_height);
	if(  size!=get_size()  ) {
		set_size(size);
	}
}
