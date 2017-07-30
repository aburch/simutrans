/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Vehicle class manager
 */

#include <stdio.h>

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
	: gui_frame_t(cnv->get_name(), cnv->get_owner()),
	scrolly(&veh_info),
	veh_info(cnv)
{
	this->cnv = cnv;
	int offset_y = LINESPACE;
	int pass_class_capacity[255] = { 0 };
	for (int i = goods_manager_t::passengers->get_number_of_classes(); i >= 0; i--)
	{
		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
		{
			vehicle_t* v = cnv->get_vehicle(veh);
			if (v->get_cargo_type()->get_catg_index() == 0)
			{
				pass_class_capacity[i] += v->get_desc()->get_capacity(i);
			}
		}
		if (pass_class_capacity[i] > 0)
		{
			pass_class_sel[i].set_pos(scr_coord(100, offset_y));
			pass_class_sel[i].set_size(scr_size(185, D_BUTTON_HEIGHT));
			pass_class_sel[i].set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
			pass_class_sel[i].set_highlight_color(1);
			pass_class_sel[i].clear_elements();

			class_indices.clear();

			for (int j = goods_manager_t::passengers->get_number_of_classes(); j > 0; j--)
			{
				char pass_class_name_untranslated[32][510];
				sprintf(pass_class_name_untranslated[j], "p_class[%u]", j - 1);
				pass_class_sel[i].append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(pass_class_name_untranslated[j]), SYSCOL_TEXT));
				class_indices.append(j);
			}
			add_component(pass_class_sel +i);
			pass_class_sel[i].add_listener(this);
			pass_class_sel[i].set_focusable(false);
			offset_y += LINESPACE;
		}
	}


	scrolly.set_pos(scr_coord(0, 2 + 16 + 5 * LINESPACE));
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
		
		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		int offset_y = pos.y + 2 + 16;

		// current value
		if (cnv.is_bound())
		{

			char number[64];
			cbuffer_t buf;

			// This is the different compartments in the train. Each compartment have a combobox associated to it to change the class to another.
			int pass_class_desc_capacity[255] = { 0 };
			int mail_class_desc_capacity[255] = { 0 };
			uint32 overcrowded_capacity = 0;

			buf.clear();
			buf.printf("%s:", translator::translate("compartments"));
			display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			offset_y += LINESPACE;

			for (int i = goods_manager_t::passengers->get_number_of_classes(); i >= 0; i--)
			{
				for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
				{		
					vehicle_t* v = cnv->get_vehicle(veh);
					if (v->get_cargo_type()->get_catg_index() == 0)
					{
						pass_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
						overcrowded_capacity += v->get_desc()->get_overcrowded_capacity();
					}
				}
				if (pass_class_desc_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "p_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i", class_name, pass_class_desc_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;

					// Did not get this to work properly. Now it is set in vehicle_class_manager_t::vehicle_class_manager_t(convoihandle_t cnv)
					//pass_class_sel[i].set_pos(scr_coord(100, offset_y));
					//pass_class_sel[i].set_size(scr_size(185, D_BUTTON_HEIGHT));
					//pass_class_sel[i].set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
									
				}
			}
			if (overcrowded_capacity > 0)
			{
				buf.clear();
				buf.printf("%s:  %i",translator::translate("overcrowded_capacity"), overcrowded_capacity);
				display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				offset_y += LINESPACE;
			}
			for (int i = goods_manager_t::mail->get_number_of_classes(); i >= 0; i--)
			{
				for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
				{
					vehicle_t* v = cnv->get_vehicle(veh);
					if (v->get_cargo_type()->get_catg_index() == 1)
					{
						mail_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
					}
				}
				if (mail_class_desc_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "m_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i", class_name, mail_class_desc_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;
				}
			}

			offset_y += LINESPACE;
			
			// This section shows the reassigned classes after they have been modified. If nothing is modified, they will show the same as the above section.
			int pass_class_capacity[255] = { 0 };
			int mail_class_capacity[255] = { 0 };
			buf.clear();
			buf.printf("%s:", translator::translate("capacity_per_class"));
			display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			offset_y += LINESPACE;

			for (int i = goods_manager_t::passengers->get_number_of_classes(); i >= 0; i--)
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
					buf.printf("%s: %i", class_name, pass_class_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;
				}
			}

			for (int i = goods_manager_t::mail->get_number_of_classes(); i >= 0; i--)
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
					buf.printf("%s: %i", class_name, mail_class_capacity[i]);
					display_proportional_clip(pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					offset_y += LINESPACE;
				}
			}
		}
	}
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool vehicle_class_manager_t::action_triggered(gui_action_creator_t *comp, value_t p)
{
	for (int i = 0; i< goods_manager_t::passengers->get_number_of_classes(); i++)
	{
	if (comp == pass_class_sel +i)
	{
			sint32 new_class = goods_manager_t::passengers->get_number_of_classes() - pass_class_sel[i].get_selection()-1;
				if (new_class < 0)
				{
					pass_class_sel[i].set_selection(0);
					new_class = 0;
				}
			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_cargo_type()->get_catg_index() == 0)
				{
					v->set_class_reassignment(i, new_class);
				}
			}
			if (i < 0)
			{
				break;
			}
		}
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
			dbg->error( "vehicle_class_manager_t::rdwr()", "Could not restore vehicle class manager window of (%d)", cnv.get_id() );
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
		for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) 
		{
			vehicle_t *v=cnv->get_vehicle(veh);
			if (v->get_cargo_type()->get_catg_index() == 0 || v->get_cargo_type()->get_catg_index() == 1)
			{
				int returns = 0;
				freight_info.clear();

				// first image
				scr_coord_val x, y, w, h;
				const image_id image = v->get_loaded_image();
				display_get_base_image_offset(image, &x, &y, &w, &h);
				display_base_img(image, 11 - x + pos.x + offset.x, pos.y + offset.y + total_height - y + 2, cnv->get_owner()->get_player_nr(), false, true);
				w = max(40, w + 4) + 11;

				// now add the other info
				int extra_y = 0;

				// name of this
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;

				//Catering
				if (v->get_desc()->get_catering_level() > 0)
				{
					buf.clear();
					if (v->get_desc()->get_freight_type()->get_catg_index() == 1)
					{
						//Catering vehicles that carry mail are treated as TPOs.
						buf.printf("%s", translator::translate("This is a travelling post office\n"));
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;
					}
					else
					{
						buf.printf(translator::translate("Catering level: %i\n"), v->get_desc()->get_catering_level());
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;
					}
				}



				if (v->get_desc()->get_total_capacity() > 0)
				{
					char min_loading_time_as_clock[32];
					char max_loading_time_as_clock[32];
					welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), v->get_desc()->get_min_loading_time());
					welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), v->get_desc()->get_max_loading_time());
					buf.clear();
					buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}

				goods_desc_t const& g = *v->get_cargo_type();
				char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());

				if (v->get_cargo_type()->get_catg_index() == 0)
				{
					buf.clear();
					buf.printf(translator::translate("accommodations:"));
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
					for (uint8 i = 0; i < v->get_desc()->get_number_of_classes(); i++)
					{
						if (v->get_desc()->get_capacity(i) > 0)
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
	


							buf.clear();
							char capacity[32];
							sprintf(capacity, v->get_overcrowding(i) > 0 ? "%i (%i)" : "%i", v->get_desc()->get_capacity(i), v->get_overcrowding(i));
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
							sint64 fare = v->get_cargo_type()->get_total_fare(1000, 0, v->get_comfort(0, i), 0, v->get_reassigned_class(i));
							// Multiply by capacity, convert to simcents, subtract running costs
							sint64 profit = ((v->get_capacity(i) + v->get_overcrowding(i))*fare + 2048ll) / 4096ll;
							money_to_string(number, profit / 100.0);
							display_proportional_clip(pos.x + w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, profit > 0 ? MONEY_PLUS : MONEY_MINUS, true);
							extra_y += LINESPACE;
						}
					}
				}
				//skip at least five lines
				total_height += max(extra_y + LINESPACE, 5 * LINESPACE);
			}
		}
	}

	scr_size size(max(x_size+pos.x,get_size().w),total_height);
	if(  size!=get_size()  ) {
		set_size(size);
	}
}

