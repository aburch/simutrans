/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
* signal infowindow buttons //Ves
 */

#include "signal_info.h"
#include "../obj/signal.h" // The rest of the dialog
#include "../obj/gebaeude.h"

#include "../simmenu.h"
#include "../simworld.h"
#include "../display/viewport.h"
#include "../simsignalbox.h"
#include "../utils/simstring.h"


signal_info_t::signal_info_t(signal_t* const s) :
	obj_infowin_t(s),
	sig(s)
{
	koord3d sb = sig->get_signalbox();

	new_component<gui_label_t>("Controlled from");
	add_table(3,3);
	{
		bt_goto_signalbox.init(button_t::posbutton, NULL);
		bt_goto_signalbox.set_visible(false);
		bt_goto_signalbox.set_tooltip(translator::translate("goto_signalbox"));
		bt_goto_signalbox.set_rigid(true);
		add_component(&bt_goto_signalbox);
		bt_goto_signalbox.add_listener(this);

		lb_sb_name.buf().clear();
		if (sb == koord3d::invalid) {
			lb_sb_name.buf().append(translator::translate("keine"));
		}
		else {
			const grund_t* gr = welt->lookup(sb);
			if (gr) {
				const gebaeude_t* gb = gr->get_building();
				if (gb) {
					char sb_name[1024] = { '\0' };
					int max_width = 250;
					int max_lines = 5; // Set a limit
					sprintf(sb_name, "%s", translator::translate(gb->get_name()));

					//sprintf(sb_name,"This is a very very long signal box name which is so long that no one remembers what it was actually called before the super long name of the signalbox got changed to its current slightly longer name which is still too long to display in only one line therefore splitting this very long signalbox name into several lines although maximum five lines which should suffice more than enough to guard against silly long signal box names");
					int next_char_index = 0;

					for (int l = 0; l < max_lines; l++) {
						char temp_name[1024] = { '\0' };
						next_char_index = display_fit_proportional(sb_name, max_width, 0);

						if (sb_name[next_char_index] == '\0') {
							lb_sb_name.buf().append(sb_name);
							break;
						}
						else {
							for (int i = 0; i < next_char_index; i++) {
								temp_name[i] = sb_name[i];
							}
							lb_sb_name.buf().append(temp_name);
							if (l + 1 == max_lines) {
								lb_sb_name.buf().append("...");
							}

							for (int i = 0; sb_name[i] != '\0'; i++) {
								sb_name[i] = sb_name[i + next_char_index];
							}
						}
					}

					const grund_t *ground = welt->lookup_kartenboden(sb.x, sb.y);
					bool sb_underground = ground->get_hoehe() > sb.z;

					char sb_coordinates[20];
					sprintf(sb_coordinates, "<%i,%i>", sb.x, sb.y);
					lb_sb_name.buf().printf("  %s", sb_coordinates);
					if (sb_underground) {
						lb_sb_name.buf().printf(" (%s)", translator::translate("underground"));
					}

					// Show the distance between the signal and its signalbox, along with the signals maximum range
					const uint32 tiles_to_signalbox = shortest_distance(s->get_pos().get_2d(), sb.get_2d());
					const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
					const double km_to_signalbox = (double)tiles_to_signalbox * km_per_tile;

					if (km_to_signalbox < 1)
					{
						float m_to_signalbox = km_to_signalbox * 1000;
						lb_sb_distance.buf().append(m_to_signalbox);
						lb_sb_distance.buf().append("m");
					}
					else {
						uint n_actual;
						if (km_to_signalbox < 20) {
							n_actual = 1;
						}
						else {
							n_actual = 0;
						}
						char number_actual[10];
						number_to_string(number_actual, km_to_signalbox, n_actual);
						lb_sb_distance.buf().append(number_actual);
						lb_sb_distance.buf().append("km");
					}

					if (s->get_desc()->get_working_method() != moving_block)
					{
						lb_sb_distance.buf().append(" (");

						uint32 mdt_sb = s->get_desc()->get_max_distance_to_signalbox();

						if (mdt_sb == 0) {
							lb_sb_distance.buf().append(translator::translate("infinite_range"));
						}
						else {
							if (mdt_sb < 1000) {
								lb_sb_distance.buf().printf("%s: ", translator::translate("max"));
								lb_sb_distance.buf().append(mdt_sb);
								lb_sb_distance.buf().append("m");
							}
							else {
								uint n_max;
								const double max_dist = (double)mdt_sb / 1000;
								if (max_dist < 20) {
									n_max = 1;
								}
								else {
									n_max = 0;
								}
								char number_max[10];
								number_to_string(number_max, max_dist, n_max);
								lb_sb_distance.buf().printf("%s: ", translator::translate("max"));
								lb_sb_distance.buf().append(number_max);
								lb_sb_distance.buf().append("km");
							}
						}
						lb_sb_distance.buf().append(")");
					}
				}
				else {
					lb_sb_name.buf().append(translator::translate("keine"));
					dbg->warning("signal_t::info()", "Signalbox could not be found from a signal on valid ground");
				}
			}
			else {
				lb_sb_name.buf().append(translator::translate("keine"));
				dbg->warning("signal_t::info()", "Signalbox could not be found from a signal on valid ground");
			}
		}
		lb_sb_name.update();
		add_component(&lb_sb_name);

		bt_info_signalbox.init(button_t::roundbox, "Details");
		bt_info_signalbox.set_size(scr_size(D_BUTTON_WIDTH*2/3, D_BUTTON_HEIGHT));
		bt_info_signalbox.set_visible(false);
		bt_info_signalbox.set_tooltip(translator::translate("open_signalbox_info"));
		bt_info_signalbox.add_listener(this);
		bt_info_signalbox.set_rigid(true);
		add_component(&bt_info_signalbox);

		new_component<gui_empty_t>();
		lb_sb_distance.update();
		add_component(&lb_sb_distance, 2);

		new_component<gui_margin_t>(D_H_SPACE,D_V_SPACE);
	}
	end_table();

	if (sb == koord3d::invalid) {
		// No signalbox
	}
	else
	{
		const grund_t* gr = welt->lookup(sb);
		if (gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if (gb) {
				bt_goto_signalbox.set_visible(true);
				bt_info_signalbox.set_visible(true);
			}
		}
	}

	// show author below the settings
	if (char const* const maker = sig->get_desc()->get_copyright()) {
		gui_label_buf_t* lb = new_component<gui_label_buf_t>();
		lb->buf().printf(translator::translate("Constructed by %s"), maker);
		lb->update();
	}

	recalc_size();
}


/**
 * This method is called if an action is triggered
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 */
bool signal_info_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &bt_goto_signalbox)
	{
		koord3d sb = sig->get_signalbox();
		welt->get_viewport()->change_world_position(koord3d(sb));
	}
	if (comp == &bt_info_signalbox)
	{
		koord3d sb = sig->get_signalbox();
		if (sb == koord3d::invalid)
		{
			// No signalbox
		}
		else
		{
			const grund_t* gr = welt->lookup(sb);
			if (gr)
			{
				gebaeude_t* gb = gr->get_building();
				if (gb)
				{
					gr->get_building()->show_info();
					if (sig->get_player_nr() == welt->get_active_player()->get_player_nr())
					{
						welt->get_active_player()->set_selected_signalbox(static_cast<signalbox_t *>(gb));
					}
				}
			}
			return true;
		}
	}

	return false;
}
