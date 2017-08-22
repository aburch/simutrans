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
#include "../simmenu.h"
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/gui_chart.h"

#include "../obj/roadsign.h"



#define SCL_HEIGHT (15*LINESPACE)

vehicle_class_manager_t::vehicle_class_manager_t(convoihandle_t cnv)
	: gui_frame_t(translator::translate("class_management"), cnv->get_owner()),
	scrolly(&veh_info),
	veh_info(cnv)
{
	this->cnv = cnv;

	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();

	// First, create the lists of the names of classes
	for (int i = 0; i < pass_classes; i++)
	{
		class_name = new (nothrow) char[32];
		sprintf(class_name, "p_class[%u]", i);
		pass_class_name_untranslated[i] = class_name;
	}

	for (int i = 0; i < mail_classes; i++)
	{
		class_name = new (nothrow) char[32];
		sprintf(class_name, "m_class[%u]", i);
		mail_class_name_untranslated[i] = class_name;
	}
		
	for (int i = 0; i < pass_classes; i++)
	{
		gui_combobox_t *class_selector = new (nothrow) gui_combobox_t();
		if (class_selector != nullptr)
		{
			add_component(class_selector);
			class_selector->add_listener(this);
			class_selector->set_focusable(false);

			pass_class_sel.append(class_selector);
		}
	}
	
	for (int i = 0; i < mail_classes; i++)
	{
		gui_combobox_t *class_selector = new (nothrow) gui_combobox_t();
		if (class_selector != nullptr)
		{
			add_component(class_selector);
			class_selector->add_listener(this);
			class_selector->set_focusable(false);

			mail_class_sel.append(class_selector);
		}
	}

	reset_all_classes_button.set_typ(button_t::roundbox);
	reset_all_classes_button.set_text("reset_all_classes");
	reset_all_classes_button.add_listener(this);
	reset_all_classes_button.set_tooltip("resets_all_classes_to_their_defaults");
	add_component(&reset_all_classes_button);

	add_component(&scrolly);
	scrolly.set_show_scroll_x(true);

	set_resizemode(diagonal_resize);
	resize(scr_coord(0, 0));
	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT + 50 + 17 * (LINESPACE + 1) + D_SCROLLBAR_HEIGHT - 6));


	layout();
	build_class_entries();



}



void vehicle_class_manager_t::build_class_entries()
{
	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();

	for (int i = 0; i < pass_classes; i++) // i = the class this combobox represents
	{
		bool veh_all_same_class = true;
		pass_class_sel.at(i)->clear_elements();
		for (int j = 0; j < pass_classes; j++) // j = the entries of this combobox
		{
			pass_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(pass_class_name_untranslated[j]), SYSCOL_TEXT));
			pass_class_sel.at(i)->set_selection(i);
		}

		// Below an attempt to make a new entry if a vehicle has its class changed individually

		//for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
		//{
		//	vehicle_t* v = cnv->get_vehicle(veh);
		//	if (v->get_capacity(i) != v->get_desc()->get_capacity(i))
		//	{
		//		veh_all_same_class = false;
		//	}
		//}
		//if (veh_all_same_class == false) // If a class is changed in a single vehicle, change the text in the combobox to the following
		//{
		//	//pass_class_sel.at(i)->clear_elements();
		//	pass_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("individual_pr_car"), SYSCOL_TEXT));
		//	pass_class_sel.at(i)->set_selection(pass_classes+1);
		//}
		//else
		//{
		//	pass_class_sel.at(i)->set_selection(i);
		//}
	}
	for (int i = 0; i < mail_classes; i++)
	{
		for (int j = 0; j < mail_classes; j++)
		{
			mail_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(mail_class_name_untranslated[j]), SYSCOL_TEXT));
			mail_class_sel.at(i)->set_selection(i);
		}
	}
}




void vehicle_class_manager_t::layout()
{
	int pass_class_desc_capacity[255] = { 0 };
	int mail_class_desc_capacity[255] = { 0 };
	bool any_pass = false;
	bool any_mail = false;

	for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
	{
		vehicle_t* v = cnv->get_vehicle(veh);
		uint8 classes_amount = v->get_desc()->get_number_of_classes();
		if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
		{
			for (int i = 0; i < classes_amount; i++)
			{
				pass_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
				any_pass = true;
			}
		}
		else if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
		{
			for (int i = 0; i < classes_amount; i++)
			{
				mail_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
				any_mail = true;
			}
		}
	}
	sint16 y = LINESPACE;
	sint16 button_width = 190;
	const scr_coord_val column_2 = longest_class_name + 30;

	for (int i = 0; i < pass_class_sel.get_count(); i++)
	{
		pass_class_sel.at(i)->set_visible(false);
		if (pass_class_desc_capacity[i] > 0)
		{
			pass_class_sel.at(i)->set_visible(true);
			pass_class_sel.at(i)->set_pos(scr_coord(column_2, y));
			pass_class_sel.at(i)->set_highlight_color(1);
			pass_class_sel.at(i)->set_size(scr_size(button_width, D_BUTTON_HEIGHT));
			pass_class_sel.at(i)->set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
			y += LINESPACE;

		}
	}
	if (any_pass && any_mail)
	{
		y += LINESPACE;
	}
	for (int i = 0; i < mail_class_sel.get_count(); i++)
	{
		mail_class_sel.at(i)->set_visible(false);
		if (mail_class_desc_capacity[i] > 0)
		{
			mail_class_sel.at(i)->set_visible(true);
			mail_class_sel.at(i)->set_pos(scr_coord(column_2, y));
			mail_class_sel.at(i)->set_highlight_color(1);
			mail_class_sel.at(i)->set_size(scr_size(button_width, D_BUTTON_HEIGHT));
			mail_class_sel.at(i)->set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
			y += LINESPACE;
		}
	}
	y += LINESPACE;
	reset_all_classes_button.set_pos(scr_coord(column_2, y - 2));
	reset_all_classes_button.set_size(scr_size(button_width, D_BUTTON_HEIGHT));
	y += LINESPACE;

	if (overcrowded_capacity>0)
	{
		y += LINESPACE;
	}

	//build_class_entries();

	header_height = y + (current_number_of_classes * LINESPACE);
	int actual_width = column_2 + button_width + 10;
	int old_window_h = get_windowsize().h;
	int default_window_h = D_TITLEBAR_HEIGHT + 50 + 17 * (LINESPACE + 1) + D_SCROLLBAR_HEIGHT - 6;

	scrolly.set_pos(scr_coord(0, header_height));
	set_min_windowsize(scr_size(max(D_DEFAULT_WIDTH, actual_width), D_TITLEBAR_HEIGHT + header_height));
	set_windowsize(scr_size(max(D_DEFAULT_WIDTH, actual_width), max(default_window_h, old_window_h)));

}

void vehicle_class_manager_t::draw(scr_coord pos, scr_size size)
{
	if (!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		int offset_y = pos.y + 2 + 16;
		header_height = 0;

		// current value
		if (cnv.is_bound())
		{

			current_number_of_vehicles = 0;
			current_number_of_compartments = 0;
			current_number_of_classes = 0;
			overcrowded_capacity = 0;
			highest_catering = 0;

			cbuffer_t buf;

			// This is the different compartments in the train.
			// Each compartment have a combobox associated to it to change the class to another.
			int pass_class_desc_capacity[255] = { 0 };
			int mail_class_desc_capacity[255] = { 0 };
			uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
			uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();
			bool any_pass = false;
			bool any_mail = false;
			int len = 0;
			int column_1 = 10;
			int column_2;
			int compartment_height;
			bool is_tpo = false;

			char const*  const  pass_name = translator::translate(goods_manager_t::passengers->get_name());
			char const*  const  mail_name = translator::translate(goods_manager_t::mail->get_name());

			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				current_number_of_vehicles++;
				vehicle_t* v = cnv->get_vehicle(veh);
				uint8 classes_amount = v->get_desc()->get_number_of_classes();
				if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
				{
					for (int i = 0; i < classes_amount; i++)
					{
						pass_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
					}
					if (v->get_desc()->get_catering_level() > highest_catering)
					{
						highest_catering = v->get_desc()->get_catering_level();
					}
				}
			}
			for (int i = 0; i < pass_classes; i++)
			{
				if (pass_class_desc_capacity[i] > 0)
				{
					if (!any_pass)
					{
						buf.clear();
						buf.printf("%s (%s):", translator::translate("compartments"), pass_name);
						display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						offset_y += LINESPACE;
					}
					any_pass = true;

					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "p_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i", class_name, pass_class_desc_capacity[i]);
					len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					if (len > longest_class_name)
					{
						longest_class_name = len;
					}
					offset_y += LINESPACE;
					current_number_of_compartments++;
				}
			}
			if (any_pass && any_mail)
			{
				offset_y += LINESPACE;
			}

			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				uint8 classes_amount = v->get_desc()->get_number_of_classes();
				if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					for (int i = 0; i < classes_amount; i++)
					{
						mail_class_desc_capacity[i] += v->get_desc()->get_capacity(i);
					}
					if (v->get_desc()->get_catering_level() > 0)
					{
						is_tpo = true;
					}
				}
			}
			for (int i = 0; i < mail_classes; i++)
			{
				if (mail_class_desc_capacity[i] > 0)
				{
					if (!any_mail)
					{
						buf.clear();
						buf.printf("%s (%s):", translator::translate("compartments"), mail_name);
						display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						offset_y += LINESPACE;
					}
					any_mail = true;

					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "m_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i", class_name, mail_class_desc_capacity[i]);
					len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					if (len > longest_class_name)
					{
						longest_class_name = len;
					}
					offset_y += LINESPACE;
					current_number_of_compartments++;
				}
			}

			offset_y += LINESPACE;

			// This section shows the reassigned classes after they have been modified.
			// If nothing is modified, they will show the same as the above section.
			int pass_class_capacity[255] = { 0 };
			int mail_class_capacity[255] = { 0 };
			buf.clear();
			buf.printf("%s:", translator::translate("capacity_per_class"));
			len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			if (len > longest_class_name)
			{
				longest_class_name = len;
			}
			offset_y += LINESPACE;
			compartment_height = offset_y;



			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
				{
					overcrowded_capacity += v->get_desc()->get_overcrowded_capacity();
					for (int i = 0; i < pass_classes; i++)
					{
						pass_class_capacity[i] += v->get_capacity(i);
					}
				}
			}
			for (int i = 0; i < pass_classes; i++)
			{
				if (pass_class_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "p_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i %s", class_name, pass_class_capacity[i], pass_name);
					len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);			if (len > longest_class_name)
					{
						longest_class_name = len;
					}
					offset_y += LINESPACE;
					current_number_of_classes++;
				}
			}
			if (overcrowded_capacity > 0)
			{
				buf.clear();
				buf.printf("%s: %i %s", translator::translate("overcrowded_capacity"), overcrowded_capacity, pass_name);
				len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				if (len > longest_class_name)
				{
					longest_class_name = len;
				}
				offset_y += LINESPACE;
				current_number_of_classes++;
			}

			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					for (int i = 0; i < mail_classes; i++)
					{
						mail_class_capacity[i] += v->get_capacity(i);
					}
				}
			}
			for (int i = 0; i < mail_classes; i++)
			{
				if (mail_class_capacity[i] > 0)
				{
					buf.clear();
					char class_name_untranslated[32];
					sprintf(class_name_untranslated, "m_class[%u]", i);
					const char* class_name = translator::translate(class_name_untranslated);
					buf.printf("%s: %i %s", class_name, mail_class_capacity[i], mail_name);
					len = display_proportional_clip(pos.x + column_1, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					if (len > longest_class_name)
					{
						longest_class_name = len;
					}
					offset_y += LINESPACE;
					current_number_of_classes++;
				}
			}
			column_2 = longest_class_name + 30;



			if (highest_catering > 0)
			{
				buf.clear();
				buf.printf(translator::translate("Catering level: %i"), highest_catering);
				display_proportional_clip(pos.x + column_2, compartment_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				compartment_height += LINESPACE;
			}
			if (is_tpo)
			{
				buf.clear();
				buf.printf("%s", translator::translate("traveling_post_office"));
				display_proportional_clip(pos.x + column_2, compartment_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				compartment_height += LINESPACE;
			}



			if (old_number_of_compartments != current_number_of_compartments)
			{
				old_number_of_compartments = current_number_of_compartments;
				layout();
				header_height = offset_y;
			}
			if (old_number_of_vehicles != current_number_of_vehicles)
			{
				old_number_of_vehicles = current_number_of_vehicles;
				layout();
				header_height = offset_y;
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
	for (int i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		if (comp == pass_class_sel.at(i))
		{
			sint32 new_class = pass_class_sel.at(i)->get_selection();
			//sint32 new_class = goods_manager_t::passengers->get_number_of_classes() - pass_class_sel[i].get_selection() - 1;
			if (new_class < 0)
			{
				pass_class_sel.at(i)->set_selection(0);
				new_class = 0;
			}
			int good_type = 0; // 0 = Passenger, 1 = Mail, 2 = both
			cbuffer_t buf;
			buf.printf("%i,%i,%i", i, new_class, good_type);
			cnv->call_convoi_tool('c', buf);

			if (i < 0)
			{
				break;
			}
			return false;
		}
	}
	for (int i = 0; i < goods_manager_t::mail->get_number_of_classes(); i++)
	{
		if (comp == mail_class_sel.at(i))
		{
			sint32 new_class = mail_class_sel.at(i)->get_selection();
			//sint32 new_class = goods_manager_t::mail->get_number_of_classes() - mail_class_sel[i].get_selection() - 1;
			if (new_class < 0)
			{
				mail_class_sel.at(i)->set_selection(0);
				new_class = 0;
			}
			int good_type = 1; // 0 = Passenger, 1 = Mail, 2 = both
			cbuffer_t buf;
			buf.printf("%i,%i,%i", i, new_class, good_type);
			cnv->call_convoi_tool('c', buf);

			if (i < 0)
			{
				break;
			}
			return false;
		}
	}
	if (comp == &reset_all_classes_button)
	{
		int good_type = 2; // 0 = Passenger, 1 = Mail, 2 = both
		cbuffer_t buf;
		buf.printf("%i,%i,%i", 0, 0, good_type);
		cnv->call_convoi_tool('c', buf);

		for (int i = 0; i < pass_class_sel.get_count(); i++)
		{
			pass_class_sel.at(i)->set_selection(i);
		}
		for (int i = 0; i < mail_class_sel.get_count(); i++)
		{
			mail_class_sel.at(i)->set_selection(i);
		}

		layout();
		return true;

	}
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

	//int pass_class_capacity[255] = { 0 };
	//int mail_class_capacity[255] = { 0 };
	//uint16 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	//uint16 mail_classes = goods_manager_t::mail->get_number_of_classes();

	//for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
	//{
	//	vehicle_t* v = cnv->get_vehicle(veh);
	//	if (v->get_cargo_type()->get_catg_index() == 0)
	//	{
	//		for (int i = 0; i < pass_classes; i++)
	//		{
	//			if (v->get_desc()->get_capacity(i) > 0)
	//			{
	//				pass_class_veh_sel[i].set_highlight_color(1);
	//				pass_class_veh_sel[i].clear_elements();

	//				char pass_class_name_untranslated[32][1020];
	//				for (int j = 0; j < pass_classes; j++)
	//				{
	//					sprintf(pass_class_name_untranslated[j], "p_class[%u]", j /*- 1*/);
	//					pass_class_veh_sel[i].append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(pass_class_name_untranslated[j]), SYSCOL_TEXT));
	//					//class_indices.append(j);
	//				}
	//				add_component(pass_class_veh_sel + i);
	//				pass_class_veh_sel[i].add_listener(this);
	//				pass_class_veh_sel[i].set_focusable(false);

	//			}
	//		}
	//	}
	//}
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
		uint8 higest_catering = 0;
		uint8 higest_tpo = 0;
		uint32 passenger_count = 0;
		uint32 mail_count = 0;


		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++) // Run this check first to find the amount of passengers and mail along with any catering or TPO's
		{
			vehicle_t *v = cnv->get_vehicle(veh);
			bool pass_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS;
			bool mail_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL;

			if (pass_veh)
			{
				passenger_count += v->get_desc()->get_total_capacity() - v->get_desc()->get_overcrowded_capacity();
				if (v->get_desc()->get_catering_level() > higest_catering)
				{
					higest_catering = v->get_desc()->get_catering_level();
				}
			}
			else // is mail vehicle
			{
				mail_count += v->get_desc()->get_total_capacity();
				if (v->get_desc()->get_catering_level() > higest_catering)
				{
					higest_tpo = v->get_desc()->get_catering_level();
				}
			}
		}

		for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) 
		{
			vehicle_t *v=cnv->get_vehicle(veh);
			bool pass_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS;
			bool mail_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL;

			if ((pass_veh || mail_veh) && v->get_desc()->get_total_capacity() > 0)
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
				int extra_w = 10;
				int reassigned_w = 0;
				bool reassigned = false;
				sint64 total_income = 0;
				

				// name of this
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;

				goods_desc_t const& g = *v->get_cargo_type();
				char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());
				uint8 base_comfort = 0;
				uint8 additional_comfort = 0;

				for (uint8 i = 0; i < v->get_desc()->get_number_of_classes(); i++)
				{
					reassigned = false;
					reassigned_w = 0;
					if (v->get_desc()->get_capacity(i) > 0)
					{
						if (v->get_reassigned_class(i) != i)
						{
							reassigned = true;
						}
						buf.clear();
						char class_name_untranslated[32];
						sprintf(class_name_untranslated, pass_veh ? "p_class[%u]" : "m_class[%u]", reassigned ? v->get_reassigned_class(i) : i);
						const char* class_name = translator::translate(class_name_untranslated);
						buf.printf("%s: ", class_name);
						reassigned_w = display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT_HIGHLIGHT, true);

						if (reassigned)
						{
							buf.clear();
							char reassigned_class_name_untranslated[32];
							sprintf(reassigned_class_name_untranslated, pass_veh ? "p_class[%u]" : "m_class[%u]", i);
							const char* reassigned_class_name = translator::translate(reassigned_class_name_untranslated);
							buf.printf("(%s)", reassigned_class_name);
							display_proportional_clip(pos.x + w + offset.x + reassigned_w, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_EDIT_TEXT_DISABLED, true);

						}
						extra_y += LINESPACE + 2;

						buf.clear();
						char capacity[32];
						sprintf(capacity, v->get_overcrowding(i) > 0 ? "%i (%i)" : "%i", v->get_desc()->get_capacity(i), v->get_overcrowding(i));
						buf.printf(translator::translate("capacity: %s %s"), capacity, name);
						display_proportional_clip(pos.x + w + offset.x + extra_w, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;
						if (pass_veh)
						{
							buf.clear();
							buf.printf(translator::translate("comfort: %i"), v->get_comfort(0, v->get_reassigned_class(i)));
							int len = display_proportional_clip(pos.x + w + offset.x + extra_w, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
							if (v->get_reassigned_class(i) >= higest_catering && higest_catering > 0)
							{
								base_comfort = v->get_comfort(0, v->get_reassigned_class(i));
								additional_comfort = v->get_comfort(higest_catering, v->get_reassigned_class(i)) - base_comfort;
								buf.clear();
								buf.printf(translator::translate(" + %i"), additional_comfort);
								display_proportional_clip(pos.x + w + offset.x + extra_w + len, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
							}
							extra_y += LINESPACE;
						}
						// Compartment revenue
						int len = 5 + display_proportional_clip(pos.x + w + offset.x + extra_w, pos.y + offset.y + total_height + extra_y, translator::translate("income_pr_km_(when_full):"), ALIGN_LEFT, SYSCOL_TEXT, true);
						// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
						sint64 fare = v->get_cargo_type()->get_total_fare(1000, 0, base_comfort + additional_comfort, v->get_desc()->get_catering_level(), v->get_reassigned_class(i));
						sint64 profit = ((v->get_capacity(v->get_reassigned_class(i)) + v->get_overcrowding(v->get_reassigned_class(i)))*fare + 2048ll) / 4096ll;
						money_to_string(number, profit / 100.0);
						display_proportional_clip(pos.x + w + offset.x + len + extra_w, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, profit > 0 ? MONEY_PLUS : MONEY_MINUS, true);
							
						extra_y += LINESPACE + 2;						
						total_income += fare;
					}
					extra_y += 2;
				}
				//Catering or postoffice?
				if (v->get_desc()->get_catering_level() > 0)
				{
					if (pass_veh)
					{
						buf.clear();
						buf.printf(translator::translate("Catering level: %i"), v->get_desc()->get_catering_level());
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;
					}
					else
					{
						//Catering vehicles that carry mail are treated as TPOs.
						buf.clear();
						buf.printf("%s", translator::translate("This is a travelling post office"));
						display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE;
					}
				}
				if (v->get_desc()->get_overcrowded_capacity() > 0)
				{
					buf.clear();
					buf.printf("%s: %i %s", translator::translate("overcrowded_capacity"), v->get_desc()->get_overcrowded_capacity(), name);
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}
				char min_loading_time_as_clock[32];
				char max_loading_time_as_clock[32];
				welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), v->get_desc()->get_min_loading_time());
				welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), v->get_desc()->get_max_loading_time());
				buf.clear();
				buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;

				// Revenue
				int len = 5 + display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate("Base profit per km (when full):"), ALIGN_LEFT, SYSCOL_TEXT, true);
				// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
				// Excludes TPO/catering revenue, class and comfort effects.  FIXME --neroden
				sint64 overcrowded_fare = v->get_cargo_type()->get_total_fare(1000); // Class needs to be added here (Ves?)
																		 // Multiply by capacity, convert to simcents, subtract running costs
				sint64 profit = (v->get_cargo_max()*total_income + 2048ll) / 4096ll - v->get_running_cost(welt);
				money_to_string(number, profit / 100.0);
				display_proportional_clip(pos.x + w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, profit>0 ? MONEY_PLUS : MONEY_MINUS, true);

				if (v->get_desc()->get_catering_level() > 0)
				{
					uint32 unit_count = 0;
					char catering_service[32];
					if (mail_veh)
					{
						sprintf(catering_service, "tpo_income_pr_km_(full_convoy):");
						unit_count = mail_count;
					}
					else
					{
						sprintf(catering_service, "catering_income_pr_km_(full_convoy):");
						unit_count = passenger_count;
					}
					extra_y += LINESPACE;
					int len = 5 + display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, catering_service, ALIGN_LEFT, SYSCOL_TEXT, true);
					// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
					sint64 fare = v->get_cargo_type()->get_total_fare(1000, 0, 0, v->get_desc()->get_catering_level());
					sint64 profit = (unit_count*fare + 2048ll) / 4096ll;
					money_to_string(number, profit / 100.0);
					display_proportional_clip(pos.x + w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, profit > 0 ? MONEY_PLUS : MONEY_MINUS, true);
				}
				extra_y += LINESPACE + 2;

				//skip at least five lines
				total_height += max(extra_y + LINESPACE, 5 * LINESPACE);
			}
		}
	}

	total_height += LINESPACE;

	scr_size size(max(x_size+pos.x,get_size().w),total_height);
	if(  size!=get_size()  ) {
		set_size(size);
	}
}
/**
* This method is called if an action is triggered
* @author Markus Weber
*/
//bool gui_class_vehicleinfo_t::action_triggered(gui_action_creator_t *comp, value_t p)
//{
//	return false;
//}

