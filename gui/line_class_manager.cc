/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Vehicle class manager
 */

#include <stdio.h>

#include "line_class_manager.h"

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

line_class_manager_t::line_class_manager_t(linehandle_t line)
	: gui_frame_t(translator::translate("class_manager"), line->get_owner())
{
	this->line = line;

	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();

	// Create the list of comboboxes, as well as the names of the classes
	for (int i = 0; i < pass_classes; i++)
	{
		gui_combobox_t *class_selector = new gui_combobox_t();
		if (class_selector != nullptr)
		{
			add_component(class_selector);
			class_selector->add_listener(this);
			class_selector->set_focusable(false);
			pass_class_sel.append(class_selector);
		}

		char *class_name = new char[32]();
		if (class_name != nullptr)
		{
			sprintf(class_name, "p_class[%u]", i);
			pass_class_name_untranslated[i] = class_name;
		}
	}

	for (int i = 0; i < mail_classes; i++)
	{
		gui_combobox_t *class_selector = new gui_combobox_t();
		if (class_selector != nullptr)
		{
			add_component(class_selector);
			class_selector->add_listener(this);
			class_selector->set_focusable(false);
			mail_class_sel.append(class_selector);
		}

		char *class_name = new char[32]();
		if (class_name != nullptr)
		{
			sprintf(class_name, "m_class[%u]", i);
			mail_class_name_untranslated[i] = class_name;
		}
	}

	reset_all_pass_button.set_typ(button_t::roundbox);
	reset_all_pass_button.set_text("reset_all_pass_classes");
	reset_all_pass_button.add_listener(this);
	reset_all_pass_button.set_tooltip("resets_all_passenger_classes_to_their_defaults");
	add_component(&reset_all_pass_button);

	reset_all_mail_button.set_typ(button_t::roundbox);
	reset_all_mail_button.set_text("reset_all_mail_classes");
	reset_all_mail_button.add_listener(this);
	reset_all_mail_button.set_tooltip("resets_all_mail_classes_to_their_defaults");
	add_component(&reset_all_mail_button);

	set_resizemode(diagonal_resize);
	resize(scr_coord(0, 0));

	layout();
	//build_class_entries();

}



void line_class_manager_t::build_class_entries()
{
	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();

	for (int i = 0; i < pass_classes; i++) // i = the class this combobox represents
	{
		pass_class_sel.at(i)->clear_elements();
		if (pass_capacity_at_accommodation[i] > 0)
		{
			for (int j = 0; j < pass_classes; j++) // j = the entries of this combobox
			{
				pass_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(pass_class_name_untranslated[j]), SYSCOL_TEXT));
			}
		}

		// Below will preset the selection to the most appropriate entry
		bool multiple_classes = false;
		int old_reassigned_class = -1;
		uint8 display_class = i;

		for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
		{
			convoihandle_t cnv = line->get_convoy(convoy);
			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_accommodation_capacity(i) > 0 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
				{
					if (old_reassigned_class != v->get_reassigned_class(i))
					{
						if (old_reassigned_class == -1)
						{
							old_reassigned_class = v->get_reassigned_class(i);
							display_class = old_reassigned_class;
						}
						else
						{
							multiple_classes = true;
						}
					}
				}
			}
		}
		if (multiple_classes)
		{
			pass_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("reassigned_to_multiple"), SYSCOL_TEXT));
			display_class = pass_classes;
		}
		pass_class_sel.at(i)->set_selection(display_class);
	}


	for (int i = 0; i < mail_classes; i++)
	{
		mail_class_sel.at(i)->clear_elements();
		if (mail_capacity_at_accommodation[i] > 0)
		{
			for (int j = 0; j < mail_classes; j++)
			{
				mail_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(mail_class_name_untranslated[j]), SYSCOL_TEXT));
			}
		}

		// Below will preset the selection to the most appropriate entry
		bool multiple_classes = false;
		int old_reassigned_class = -1;
		uint8 display_class = i;

		for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
		{
			convoihandle_t cnv = line->get_convoy(convoy);
			for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_accommodation_capacity(i) > 0 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					if (old_reassigned_class != v->get_reassigned_class(i))
					{
						if (old_reassigned_class == -1)
						{
							old_reassigned_class = v->get_reassigned_class(i);
							display_class = old_reassigned_class;
						}
						else
						{
							multiple_classes = true;
						}
					}
				}
			}
		}
		if (multiple_classes)
		{
			mail_class_sel.at(i)->append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("reassigned_to_multiple"), SYSCOL_TEXT));
			display_class = mail_classes;
		}
		mail_class_sel.at(i)->set_selection(display_class);
	}
}




void line_class_manager_t::layout()
{

	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();


	// First a clean up:
	show_pass = false;
	show_mail = false;
	highest_catering = 0;
	lowest_catering = 255;
	catering_cars_amount = 0;
	tpo_amount = 0;
	is_tpo = false;
	reset_all_pass_button.set_visible(false);
	reset_all_mail_button.set_visible(false);

	for (int i = 0; i < pass_classes; i++)
	{
		pass_capacity_at_accommodation[i] = 0;
		pass_class_sel.at(i)->set_visible(false);
	}
	for (int i = 0; i < mail_classes; i++)
	{
		mail_capacity_at_accommodation[i] = 0;
		mail_class_sel.at(i)->set_visible(false);
	}

	// for each vehicle in each convoy, we need some details
	for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
	{
		convoihandle_t cnv = line->get_convoy(convoy);

		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
		{
			vehicle_t* v = cnv->get_vehicle(veh);

			// First what accommodations there is, along with the catering calculations
			uint8 classes_amount = v->get_desc()->get_number_of_classes();
			if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
			{
				for (int i = 0; i < classes_amount; i++)
				{
					pass_capacity_at_accommodation[i] += v->get_accommodation_capacity(i);
				}
				if (v->get_desc()->get_catering_level() > 0)
				{
					catering_cars_amount++;
					if (v->get_desc()->get_catering_level() > highest_catering)
					{
						highest_catering = v->get_desc()->get_catering_level();
					}
					if (v->get_desc()->get_catering_level() < lowest_catering)
					{
						lowest_catering = v->get_desc()->get_catering_level();
					}
				}
				show_pass = true;
			}
			else if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
			{
				for (int i = 0; i < classes_amount; i++)
				{
					mail_capacity_at_accommodation[i] += v->get_accommodation_capacity(i);
				}
				if (v->get_desc()->get_catering_level() > 0)
				{
					is_tpo = true;
					tpo_amount++;
				}
				show_mail = true;
			}
		}

	}

	// Now, put up all the comboboxes
	sint16 y = 2;
	cbuffer_t buf;
	int assumed_longest_class_name = 5 * 32;
	const scr_coord_val column_2 = assumed_longest_class_name + 35;
	y += LINESPACE;

	if (show_pass)
	{
		for (uint32 i = 0; i < pass_class_sel.get_count(); i++)
		{
			pass_class_sel.at(i)->set_visible(true);
			pass_class_sel.at(i)->set_pos(scr_coord(column_2, y));
			pass_class_sel.at(i)->set_highlight_color(1);
			pass_class_sel.at(i)->set_size(scr_size(button_width, D_BUTTON_HEIGHT));
			pass_class_sel.at(i)->set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
			y += LINESPACE;
		}
		y += LINESPACE - 4;
		reset_all_pass_button.set_pos(scr_coord(column_2, y));
		reset_all_pass_button.set_size(scr_size(button_width, D_BUTTON_HEIGHT));
		reset_all_pass_button.set_visible(true);
	}

	y = 2 + LINESPACE*pass_entries; // To get it to match up with the text from draw(...)
	if (show_mail)
	{
		for (uint32 i = 0; i < mail_class_sel.get_count(); i++)
		{

			mail_class_sel.at(i)->set_visible(true);
			mail_class_sel.at(i)->set_pos(scr_coord(column_2, y));
			mail_class_sel.at(i)->set_highlight_color(1);
			mail_class_sel.at(i)->set_size(scr_size(button_width, D_BUTTON_HEIGHT));
			mail_class_sel.at(i)->set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 3 + 2 + 16));
			y += LINESPACE;
		}
		y += LINESPACE - 4;
		reset_all_mail_button.set_pos(scr_coord(column_2, y));
		reset_all_mail_button.set_size(scr_size(button_width, D_BUTTON_HEIGHT));
		reset_all_mail_button.set_visible(true);
	}


	build_class_entries();

	int default_window_h = text_height;
	int old_window_h = min(get_windowsize().h, default_window_h);

	set_min_windowsize(scr_size(max(D_DEFAULT_WIDTH, column_2), default_window_h));
	set_windowsize(scr_size(max(D_DEFAULT_WIDTH, column_2), max(default_window_h, old_window_h)));

}

void line_class_manager_t::draw(scr_coord pos, scr_size size)
{
	if (!line.is_bound()) {
		destroy_win(this);
	}
	else {
		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		int offset_y = pos.y + 2 + 16;

		// current value
		if (line.is_bound())
		{
			current_pass_entries = 1;

			cbuffer_t buf;
			uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
			uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();
			char const*  const  pass_name = translator::translate(goods_manager_t::passengers->get_name());
			char const*  const  mail_name = translator::translate(goods_manager_t::mail->get_name());
			bool pass_header = true;
			bool mail_header = true;
			int column_1 = 10;
			int column_1andahalf = column_1 + (5 * 12);
			int column_2 = (5 * 32) + 35 + (button_width / 2);
			PIXVAL text_color = SYSCOL_TEXT;


			// We need to keep a constant tracking of what classes we have
			overcrowded_capacity = 0;
			vehicle_count = 0;
			for (int i = 0; i < pass_classes; i++)
			{
				pass_capacity_at_class[i] = 0;
			}
			for (int i = 0; i < mail_classes; i++)
			{
				mail_capacity_at_class[i] = 0;
			}
			for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
			{
				convoihandle_t cnv = line->get_convoy(convoy);
				for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
				{
					vehicle_t* v = cnv->get_vehicle(veh);
					vehicle_count++;
					if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
					{
						overcrowded_capacity += v->get_desc()->get_overcrowded_capacity();
						for (int i = 0; i < pass_classes; i++)
						{
							pass_capacity_at_class[i] += v->get_fare_capacity(i);
						}
					}
					if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
					{
						for (int i = 0; i < mail_classes; i++)
						{
							mail_capacity_at_class[i] += v->get_fare_capacity(i);
						}
					}
				}
			}

			// This is the different accommodation in the train.
			// Each accommodation have a combobox associated to it to change the class to another.
			// We start with the text description for the comboboxes on the right side of the window:
			if (show_pass)
			{
				buf.clear();
				buf.printf("%s:", translator::translate("reassign_classes"));
				display_proportional_clip_rgb(pos.x + column_2, offset_y, buf, ALIGN_CENTER_H, text_color, true);
			}
			else
			{
				text_color = SYSCOL_EDIT_TEXT_DISABLED;
			}
			// Then the actual accommodations:
			for (int i = 0; i < pass_classes; i++)
			{
				if (pass_header)
				{
					buf.clear();
					buf.printf("%s (%s):", translator::translate("accommodation"), pass_name);
					display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
					offset_y += LINESPACE;
					current_pass_entries++;
				}
				pass_header = false;
				text_color = SYSCOL_TEXT;
				if (pass_capacity_at_accommodation[i] == 0)
				{
					text_color = SYSCOL_EDIT_TEXT_DISABLED;
				}

				buf.clear();
				buf.printf("%s:", translator::translate(pass_class_name_untranslated[i]));
				display_proportional_clip_rgb(pos.x + column_1andahalf, offset_y, buf, ALIGN_RIGHT, text_color, true);
				buf.clear();
				buf.printf("%i %s", pass_capacity_at_accommodation[i], pass_name);
				display_proportional_clip_rgb(pos.x + column_1andahalf + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
				offset_y += LINESPACE;
				current_pass_entries++;
			}
			offset_y += LINESPACE;
			current_pass_entries++;

			// Only show what classes we have, if we have any
			text_color = SYSCOL_TEXT;
			if (show_pass)
			{
				buf.clear();
				buf.printf("%s:", translator::translate("capacity_per_class"));
				display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
				offset_y += LINESPACE;
				current_pass_entries++;
				for (int i = 0; i < pass_classes; i++)
				{
					if (pass_capacity_at_class[i] > 0)
					{
						buf.clear();
						buf.printf("%s:", translator::translate(pass_class_name_untranslated[i]));
						display_proportional_clip_rgb(pos.x + column_1andahalf, offset_y, buf, ALIGN_RIGHT, text_color, true);
						buf.clear();
						buf.printf("%i %s", pass_capacity_at_class[i], pass_name);
						display_proportional_clip_rgb(pos.x + column_1andahalf + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
						offset_y += LINESPACE;
						current_pass_entries++;
					}
				}

				if (overcrowded_capacity == 0)
				{
					text_color = SYSCOL_EDIT_TEXT_DISABLED;
				}
				buf.clear();
				buf.printf("%s:", translator::translate("overcrowded_capacity"));
				display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
				buf.clear();
				offset_y += LINESPACE;
				current_pass_entries++;
				buf.printf("%i %s", overcrowded_capacity, pass_name);
				display_proportional_clip_rgb(pos.x + column_1andahalf + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
				offset_y += LINESPACE;
				current_pass_entries++;

				if (show_pass)
				{
					/* Catering entry:
						"Catering cars: 5"
						"Catering levels: 2 - 3"
					*/
					if (highest_catering > 0)
					{
						text_color = SYSCOL_TEXT;
						buf.clear();
						buf.printf(translator::translate("catering_cars: %i"), catering_cars_amount);
						int len = display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);

						char catering[10];
						sprintf(catering, "%i", highest_catering);
						if (lowest_catering != highest_catering)
						{
							sprintf(catering, "%i - %i", lowest_catering, highest_catering);
						}
						buf.clear();
						buf.printf(translator::translate("(catering_levels: %s)"), catering);
						display_proportional_clip_rgb(pos.x + column_1 + len + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
						offset_y += LINESPACE;
						current_pass_entries++;
					}
				}
				offset_y += LINESPACE;
				current_pass_entries++;
			}

			text_color = SYSCOL_TEXT;
			if (show_mail)
			{
				buf.clear();
				buf.printf("%s:", translator::translate("reassign_classes"));
				display_proportional_clip_rgb(pos.x + column_2, offset_y, buf, ALIGN_CENTER_H, text_color, true);
			}
			else
			{
				text_color = SYSCOL_EDIT_TEXT_DISABLED;
			}

			for (int i = 0; i < mail_classes; i++)
			{
				if (mail_header)
				{
					buf.clear();
					buf.printf("%s (%s):", translator::translate("accommodation"), mail_name);
					display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
					offset_y += LINESPACE;
				}
				mail_header = false;
				text_color = SYSCOL_TEXT;
				if (mail_capacity_at_accommodation[i] == 0)
				{
					text_color = SYSCOL_EDIT_TEXT_DISABLED;
				}
				buf.clear();
				buf.printf("%s:", translator::translate(mail_class_name_untranslated[i]));
				display_proportional_clip_rgb(pos.x + column_1andahalf, offset_y, buf, ALIGN_RIGHT, text_color, true);
				buf.clear();
				buf.printf("%i %s", mail_capacity_at_accommodation[i], mail_name);
				display_proportional_clip_rgb(pos.x + column_1andahalf + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
				offset_y += LINESPACE;

			}
			offset_y += LINESPACE;

			text_color = SYSCOL_TEXT;
			if (show_mail)
			{
				buf.clear();
				buf.printf("%s:", translator::translate("capacity_per_class"));
				display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
				offset_y += LINESPACE;
				for (int i = 0; i < mail_classes; i++)
				{
					if (mail_capacity_at_class[i] > 0)
					{
						buf.clear();
						buf.printf("%s:", translator::translate(mail_class_name_untranslated[i]));
						display_proportional_clip_rgb(pos.x + column_1andahalf, offset_y, buf, ALIGN_RIGHT, text_color, true);
						buf.clear();
						buf.printf("%i %s", mail_capacity_at_class[i], mail_name);
						display_proportional_clip_rgb(pos.x + column_1andahalf + 5, offset_y, buf, ALIGN_LEFT, text_color, true);
						offset_y += LINESPACE;
					}
				}

				/* TPO entry:
					"TPO cars: 5"
				*/
				if (is_tpo)
				{
					text_color = SYSCOL_TEXT;
					buf.clear();
					buf.printf(translator::translate("tpo_cars: %i"), tpo_amount);
					display_proportional_clip_rgb(pos.x + column_1, offset_y, buf, ALIGN_LEFT, text_color, true);
					offset_y += LINESPACE;
				}
			}
		}
		offset_y += LINESPACE;

		// If anything vital has changed, we need to contact the layout department.
		if (old_vehicle_count != vehicle_count || pass_entries != current_pass_entries)
		{
			pass_entries = current_pass_entries;
			old_vehicle_count = vehicle_count;
			text_height = offset_y;
			layout();
		}
		if (need_to_update_comboboxes)
		{
			build_class_entries();
			need_to_update_comboboxes = false;
		}
	}

}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool line_class_manager_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	for (int i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		if (comp == pass_class_sel.at(i))
		{
			sint32 new_class = pass_class_sel.at(i)->get_selection();
			if (new_class < 0 )
			{
				pass_class_sel.at(i)->set_selection(0);
				new_class = 0;
			}
			int good_type = 0; // 0 = Passenger, 1 = Mail,
			int reset = 0; // 0 = reset only single class, 1 = reset all classes
			cbuffer_t buf;
			buf.printf("%i,%i,%i,%i", i, new_class, good_type, reset);
			for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
			{
				convoihandle_t cnv = line->get_convoy(convoy);
				cnv->call_convoi_tool('c', buf);
			}
			layout();
			if (i < 0)
			{
				break;
			}
			if (pass_class_sel.at(i)->count_elements() > goods_manager_t::passengers->get_number_of_classes())
			{
				need_to_update_comboboxes = true;
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
			int good_type = 1; // 0 = Passenger, 1 = Mail,
			int reset = 0; // 0 = reset only single class, 1 = reset all classes
			cbuffer_t buf;
			buf.printf("%i,%i,%i,%i", i, new_class, good_type, reset);
			for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
			{
				convoihandle_t cnv = line->get_convoy(convoy);
				cnv->call_convoi_tool('c', buf);
			}
			layout();
			if (i < 0)
			{
				break;
			}
			if (mail_class_sel.at(i)->count_elements() > goods_manager_t::mail->get_number_of_classes())
			{
				need_to_update_comboboxes = true;
			}
			return false;
		}
	}
	if (comp == &reset_all_pass_button)
	{
		int good_type = 0; // 0 = Passenger, 1 = Mail,
		int reset = 1; // 0 = reset only single class, 1 = reset all classes
		cbuffer_t buf;
		buf.printf("%i,%i,%i,%i", 0, 0, good_type, reset);
		for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
		{
			convoihandle_t cnv = line->get_convoy(convoy);
			cnv->call_convoi_tool('c', buf);
		}

		for (uint32 i = 0; i < pass_class_sel.get_count(); i++)
		{
			pass_class_sel.at(i)->set_selection(i);
			if (pass_class_sel.at(i)->count_elements() > goods_manager_t::passengers->get_number_of_classes())
			{
				need_to_update_comboboxes = true;
			}
		}
		layout();
		return true;

	}
	if (comp == &reset_all_mail_button)
	{
		int good_type = 1; // 0 = Passenger, 1 = Mail,
		int reset = 1; // 0 = reset only single class, 1 = reset all classes
		cbuffer_t buf;
		buf.printf("%i,%i,%i,%i", 0, 0, good_type, reset);
		for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
		{
			convoihandle_t cnv = line->get_convoy(convoy);
			cnv->call_convoi_tool('c', buf);
		}

		for (uint32 i = 0; i < mail_class_sel.get_count(); i++)
		{
			mail_class_sel.at(i)->set_selection(i);
			if (mail_class_sel.at(i)->count_elements() > goods_manager_t::mail->get_number_of_classes())
			{
				need_to_update_comboboxes = true;
			}
		}
		layout();
		return true;

	}
	return false;
}



/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void line_class_manager_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
}


// dummy for loading
line_class_manager_t::line_class_manager_t()
: gui_frame_t("", NULL )
{
	line = linehandle_t();
}

// destruction!
line_class_manager_t::~line_class_manager_t()
{
	for (uint32 i = 0; i < pass_class_sel.get_count(); i++)
	{
		if (pass_class_sel.at(i))
		{
			delete pass_class_sel.at(i);
		}
	}
	for (uint32 i = 0; i < mail_class_sel.get_count(); i++)
	{
		if (mail_class_sel.at(i))
		{
			delete mail_class_sel.at(i);
		}
	}
	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();
	for (int i = 0; i < pass_classes; i++)
	{
		if (pass_class_name_untranslated[i] != nullptr)
		{
			delete[] pass_class_name_untranslated[i];
		}
	}
	for (int i = 0; i < mail_classes; i++)
	{
		if (mail_class_name_untranslated[i])
		{
			delete[] mail_class_name_untranslated[i];
		}
	}
}


void line_class_manager_t::rdwr(loadsave_t *file)
{
	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		koord3d line_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		line_pos.rdwr( file );
		file->rdwr_str( name, lengthof(name) );
	}
	else {
		// handle
		simline_t::rdwr_linehandle_t(file, line);
	}
	// window size, scroll position
	scr_size size = get_windowsize();
	sint32 xoff = size.w;
	sint32 yoff = size.h;

	size.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );

	if(  file->is_loading()  ) {
		// convoy vanished
		if(  !line.is_bound()  ) {
			dbg->error( "line_class_manager_t::rdwr()", "Could not restore line class manager window of (%d)", line.get_id() );
			destroy_win( this );
			return;
		}

		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		line_class_manager_t *w = new line_class_manager_t(line);
		create_win(pos.x, pos.y, w, w_info, magic_line_class_manager + line.get_id());
		w->set_windowsize( size );
		// we must invalidate halthandle
		line = linehandle_t();
		destroy_win( this );
	}
}

