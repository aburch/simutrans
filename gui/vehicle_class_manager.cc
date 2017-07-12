/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Vehicle class manager
 */

#include <stdio.h>
#include <array>

#include "vehicle_class_manager.h"

#include "../simunits.h"
#include "../simconvoi.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../simworld.h"
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

#include "../obj/roadsign.h"



#define SCL_HEIGHT (15*LINESPACE)

vehicle_class_manager_t::vehicle_class_manager_t(convoihandle_t cnv)
: gui_frame_t( cnv->get_name(), cnv->get_owner() ),
  scrolly(&veh_info),
  veh_info(cnv)
{
	this->cnv = cnv;

	scrolly.set_pos(scr_coord(0, 2+16+5*LINESPACE));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+17*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-6));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+3*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-3));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


void vehicle_class_manager_t::draw(scr_coord pos, scr_size size)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		if (cnv->get_owner() == welt->get_active_player() && !welt->get_active_player()->is_locked()) {
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
		gui_frame_t::draw(pos, size);
		int offset_y = pos.y + 2 + 16;

		// current value
		if (cnv.is_bound())
		{
			char number[64];
			cbuffer_t buf;
			int pass_class_capacity[255] = { 0 };
			int mail_class_capacity[255] = { 0 };

			for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
			{
				for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
				{		
					vehicle_t* v = cnv->get_vehicle(veh);
					if (v->get_cargo_type()->get_catg_index() == 0)
					{
						pass_class_capacity[i] += v->get_capacity(i);
					}
				}
				if (pass_class_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "p_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s:  %i", class_name, pass_class_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;
				}
			}
			for (uint8 i = 0; i < goods_manager_t::mail->get_number_of_classes(); i++)
			{
				for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
				{
					vehicle_t* v = cnv->get_vehicle(veh);
					if (v->get_cargo_type()->get_catg_index() == 1)
					{
						mail_class_capacity[i] += v->get_capacity(i);
					}
				}
				if (mail_class_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "m_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s:  %i", class_name, mail_class_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;
				}
			}
			
				
		






			// current resale value
			money_to_string(number, cnv->calc_sale_value() / 100.0);
			buf.clear();
			buf.printf("%s %s", translator::translate("Restwert:"), number);
			display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, MONEY_PLUS, true);
			offset_y += LINESPACE;

		}
	}
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool vehicle_class_manager_t::action_triggered(gui_action_creator_t *comp,value_t v/* */)           // 28-Dec-01    Markus Weber    Added
{
	if(cnv.is_bound()) {
		if(comp==&sale_button) {
			cnv->call_convoi_tool( 'x', NULL );
			return true;
		}
		else if(comp==&withdraw_button) {
			cnv->call_convoi_tool( 'w', NULL );
			return true;
		}
		else if(comp==&retire_button) {
			cnv->call_convoi_tool( 'T', NULL );
			return true;
		}
		/*else if (v.i&~1) {
			koord k = *(const koord *)v.p;
			uint16 j = k.y;
			if (j < class_selectors.get_count())
			int	class_selection = class_selector->get_selection();
			if (class_selection < 0)
			{
				class_selector.set_selection(0);
				class_selection = 0;
			}
			
		}*/
	}
	return false;
}



/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void vehicle_class_manager_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}


// dummy for loading
vehicle_class_manager_t::vehicle_class_manager_t()
: gui_frame_t("", NULL ),
  scrolly(&veh_info),
  veh_info(convoihandle_t())
{
	cnv = convoihandle_t();
}


void vehicle_class_manager_t::rdwr(loadsave_t *file)
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
			dbg->error( "vehicle_class_manager_t::rdwr()", "Could not restore convoi detail window of (%d)", cnv.get_id() );
			destroy_win( this );
			return;
		}

		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		vehicle_class_manager_t *w = new vehicle_class_manager_t(cnv);
		create_win(pos.x, pos.y, w, w_info, magic_convoi_detail + cnv.get_id());
		w->set_windowsize( size );
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		cnv = convoihandle_t();
		destroy_win( this );
	}
}


// component for vehicle display
gui_class_vehicleinfo_t::gui_class_vehicleinfo_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}



/*
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_class_vehicleinfo_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w-51-pos.x;
	karte_t *welt = world();
	offset.y += LINESPACE;

	int total_height = 0;
	if(cnv.is_bound()) {
		char number[64];
		cbuffer_t buf;

		static cbuffer_t freight_info;
		for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) {
			vehicle_t *v=cnv->get_vehicle(veh);
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
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;

			// age
			buf.clear();
			{
				const sint32 month = v->get_purchase_time();
				buf.printf( "%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			}
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;
			
			// Bernd Gabriel, 16.06.2009: current average obsolescence increase percentage
			uint32 percentage = v->get_desc()->calc_running_cost(welt, 100) - 100;
			if (percentage > 0)
			{
				buf.clear();
				buf.printf("%s: %d%%", translator::translate("Obsolescence increase"), percentage);
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, COL_DARK_BLUE, true );
				extra_y += LINESPACE;
			}

			// value
			money_to_string( number, v->calc_sale_value() / 100.0 );
			buf.clear();
			buf.printf( "%s %s", translator::translate("Restwert:"), number );
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
			extra_y += LINESPACE;

			// power, tractive force, gear
			if(v->get_desc()->get_power()>0) {
				buf.clear();
				buf.printf( "%s %i kW, %s %i kN, %s %.2f",
					translator::translate("Power:"), v->get_desc()->get_power(),
					translator::translate("Tractive Force:"), v->get_desc()->get_tractive_effort(),
					translator::translate("Gear:"), v->get_desc()->get_gear()/64.0 );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
				extra_y += LINESPACE;
			}

			// weight
			buf.clear();
			buf.printf( "%s %dt", translator::translate("Weight:"), v->get_sum_weight());
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;

			// Axle load
			buf.clear();
			buf.printf("%s %dt", translator::translate("Axle load:"), v->get_desc()->get_axle_load());
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// Brake force
			buf.clear();
			buf.printf("%s %u kN", translator::translate("Max. brake force:"), v->get_desc()->get_brake_force());
			display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
			extra_y += LINESPACE;
			
			//Catering
			if(v->get_desc()->get_catering_level() > 0)
			{
				buf.clear();
				if(v->get_desc()->get_freight_type()->get_catg_index() == 1)
				{
					//Catering vehicles that carry mail are treated as TPOs.
					buf.printf( "%s", translator::translate("This is a travelling post office\n"));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
					extra_y += LINESPACE;
				}
				else
				{
					buf.printf(translator::translate("Catering level: %i\n"), v->get_desc()->get_catering_level());
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
					extra_y += LINESPACE;
				}
			}

			//Tilting
			if(v->get_desc()->get_tilting())
			{
				buf.clear();
				buf.printf("%s", translator::translate("This is a tilting vehicle\n"));
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
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

			if(v->get_desc()->get_total_capacity() > 0)
			{
				char min_loading_time_as_clock[32];
				char max_loading_time_as_clock[32];
				welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), v->get_desc()->get_min_loading_time());
				welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), v->get_desc()->get_max_loading_time());
				buf.clear();
				buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock );
				display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
				extra_y += LINESPACE;
			}

			goods_desc_t const& g = *v->get_cargo_type();
			char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());
			
			if(v->get_cargo_type()->get_catg_index() == 0)
			{
				buf.clear();
				buf.printf(translator::translate("accommodations:"));
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;
				for (uint8 i = 0; i < v->get_desc()->get_number_of_classes(); i++)
				{
					if (v->get_capacity(i) > 0)
					{
						buf.clear();
						char class_name_untranslated[32];
						sprintf(class_name_untranslated, "p_class[%u]", i);
						const char* class_name = translator::translate(class_name_untranslated);
						buf.printf(" %s:", class_name);
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;

						buf.clear();
						char reassigned_class_name_untranslated[32];
						sprintf(reassigned_class_name_untranslated, "p_class[%u]", v->get_reassigned_class(i));
						const char* reassigned_class_name = translator::translate(reassigned_class_name_untranslated);
						buf.printf(" %s:", reassigned_class_name);
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;


						// This commented out section is an attempt to add a selector where player can choose new class. I will revisit this later.
/*						buf.clear();
						gui_combobox_t *class_selector = new gui_combobox_t();

						class_selector->set_pos(scr_coord(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y));
						class_selector->set_size(scr_size(185, D_BUTTON_HEIGHT));
						class_selector->set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
						class_selector->set_highlight_color(1);
						class_selector->clear_elements();
				
						class_indices.clear();
						for (uint8 j = 0; j < v->get_desc()->get_number_of_classes(); j++)
						{
							char class_selector_name_untranslated[32];
							sprintf(class_selector_name_untranslated, "p_class[%u]", j);
							const char* class_selector_name = translator::translate(class_selector_name_untranslated);
							class_selector->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(class_selector_name), SYSCOL_TEXT));
							class_indices.append(j);
						}

						cont.add_component(class_selector);
						class_selector->add_listener(this);
						class_selector->set_focusable(false);
						extra_y += LINESPACE;

						//pb->add_listener(this);
*/
			
						
						buf.clear();
						char capacity[32];
						sprintf(capacity, v->get_overcrowding(i) > 0 ? "%i (%i)" : "%i", v->get_capacity(i), v->get_overcrowding(i));
						buf.printf(translator::translate("  capacity: %s %s"), capacity, name);
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;

						buf.clear();
						buf.printf(translator::translate("  comfort: %i"), v->get_comfort(0, i));
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;

						// Compartment revenue
						int len = 5 + display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate("  income_pr_km_(when_full):"), ALIGN_LEFT, SYSCOL_TEXT, true);
						// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
						// Excludes TPO/catering revenue, class and comfort effects.  FIXME --neroden
						sint64 fare = v->get_cargo_type()->get_total_fare(1000,0, v->get_comfort(0,i),0,v->get_reassigned_class(i));
																				 // Multiply by capacity, convert to simcents, subtract running costs
						sint64 profit = ((v->get_capacity(i)+v->get_overcrowding(i))*fare + 2048ll) / 4096ll;
						money_to_string(number, profit / 100.0);
						display_proportional_clip(pos.x + w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, profit>0 ? MONEY_PLUS : MONEY_MINUS, true);
						extra_y += LINESPACE;
					}
				}
			}

			buf.clear();
			buf.printf(translator::translate("possible_amount_of_classes: %i"), v->get_desc()->get_number_of_classes());
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;
			
			if(v->get_cargo_max() > 0) {

				// Revenue
				int len = 5+display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Base profit per km (when full):"), ALIGN_LEFT, SYSCOL_TEXT, true );
				// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
				// Excludes TPO/catering revenue, class and comfort effects.  FIXME --neroden
				sint64 fare = v->get_cargo_type()->get_total_fare(1000); // Class needs to be added here (Ves?)
				// Multiply by capacity, convert to simcents, subtract running costs
				sint64 profit = (v->get_cargo_max()*fare + 2048ll) / 4096ll/* - v->get_running_cost(welt)*/;
				money_to_string( number, profit/100.0 );
				display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, profit>0?MONEY_PLUS:MONEY_MINUS, true );
				extra_y += LINESPACE;

				if(  sint64 cost = welt->scale_with_month_length(v->get_desc()->get_maintenance())  ) {
					KOORD_VAL len = display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, translator::translate("Maintenance"), ALIGN_LEFT, SYSCOL_TEXT, true );
					len += display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, ": ", ALIGN_LEFT, SYSCOL_TEXT, true );
					money_to_string( number, cost/(100.0) );
					display_proportional_clip( pos.x+w+offset.x+len, pos.y+offset.y+total_height+extra_y, number, ALIGN_LEFT, MONEY_MINUS, true );
					extra_y += LINESPACE;
				}


				freight_info.printf("%u/%u%s %s\n", v->get_total_cargo(), v->get_cargo_max(), translator::translate(v->get_cargo_mass()), name); // TODO: Consider whether to differentiate classes here (Ves?)
				v->get_cargo_info(freight_info);
				// show it
				const int px_len = display_multiline_text( pos.x+offset.x+w, pos.y+offset.y+total_height+extra_y, freight_info, SYSCOL_TEXT );
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
			
			const way_constraints_t &way_constraints = v->get_desc()->get_way_constraints();
			// Permissive way constraints
			// (If vehicle has, way must have)
			// @author: jamespetts
			//for(uint8 i = 0; i < 8; i++)
			for(uint8 i = 0; i < way_constraints.get_count(); i++)
			{
				//if(v->get_desc()->permissive_way_constraint_set(i))
				if(way_constraints.get_permissive(i))
				{
					buf.clear();
					char tmpbuf1[13];
					sprintf(tmpbuf1, "\nMUST USE: ");
					char tmpbuf[14];
					sprintf(tmpbuf, "Permissive %i", i);
					buf.printf("%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
					extra_y += LINESPACE;
				}
			}
			
			// Prohibitive way constraints
			// (If way has, vehicle must have)
			// @author: jamespetts
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
					display_proportional_clip( pos.x+w+offset.x, pos.y+offset.y+total_height+extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
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

