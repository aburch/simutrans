/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "display_settings.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../obj/baum.h"
#include "../obj/zeiger.h"
#include "../display/simgraph.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"

#include "gui_theme.h"
#include "themeselector.h"
#include "loadfont_frame.h"
#include "simwin.h"

#include "../path_explorer.h"
#include "components/gui_image.h"

enum {
	IDBTN_SCROLL_INVERSE,
	IDBTN_IGNORE_NUMLOCK,
	IDBTN_PEDESTRIANS_AT_STOPS,
	IDBTN_PEDESTRIANS_IN_TOWNS,
	IDBTN_DAY_NIGHT_CHANGE,
	IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN,
	IDBTN_HIDE_TREES,
	IDBTN_TRANSPARENT_STATION_COVERAGE,
	IDBTN_SHOW_STATION_COVERAGE,
	IDBTN_UNDERGROUND_VIEW,
	IDBTN_SHOW_GRID,
	IDBTN_SHOW_STATION_NAMES_ARROW,
	IDBTN_SHOW_WAITING_BARS,
	IDBTN_SHOW_SLICE_MAP_VIEW,
	IDBTN_HIDE_BUILDINGS,
	IDBTN_SHOW_SCHEDULES_STOP,
	IDBTN_SHOW_THEMEMANAGER,
	IDBTN_SIMPLE_DRAWING,
	IDBTN_CHANGE_FONT,
	IDBTN_LEFT_TO_RIGHT_GRAPHS,
	IDBTN_SHOW_SIGNALBOX_COVERAGE,
	IDBTN_CLASSES_WAITING_BAR,
	COLORS_MAX_BUTTONS,
};

static button_t buttons[COLORS_MAX_BUTTONS];

/**
* class to visualize station names
*/
class gui_label_stationname_t : public gui_label_t
{
	karte_ptr_t welt;
public:
	gui_label_stationname_t(const char* text) : gui_label_t(text) {}

	void draw(scr_coord offset) OVERRIDE
	{
		scr_coord p = pos + offset;

		FLAGGED_PIXVAL pc = welt->get_active_player() ? color_idx_to_rgb(welt->get_active_player()->get_player_color1()+3) : color_idx_to_rgb(COL_ORANGE);
		const char *text = get_text_pointer();
		switch( env_t::show_names>>2 ) {
		case 0:
			if(  env_t::show_names & 1  ) {
				display_ddd_proportional_clip( p.x, p.y + get_size().h/2, proportional_string_width(text)+7, 0, pc, color_idx_to_rgb(COL_BLACK), text, 1 );
			}
			break;
		case 1:
			display_outline_proportional_rgb( p.x, p.y+LINESPACE/4, pc+1, color_idx_to_rgb(COL_BLACK), text, 1 );
			break;
		case 2:
			display_outline_proportional_rgb( p.x + LINESPACE + D_H_SPACE, p.y+LINESPACE/4, color_idx_to_rgb(COL_YELLOW), color_idx_to_rgb(COL_BLACK), text, 1 );
			display_ddd_box_clip_rgb(         p.x,                     p.y,   LINESPACE,   LINESPACE,   pc-2, pc+2 );
			display_fillbox_wh_clip_rgb(      p.x+1,                   p.y+1, LINESPACE-2, LINESPACE-2, pc,   true);
			break;
		}
	}

	scr_size get_min_size() const OVERRIDE
	{
		return gui_label_t::get_min_size() + scr_size(LINESPACE + D_H_SPACE, 4);
	}
};



gui_settings_t::gui_settings_t()
{
	set_table_layout( 1, 0 );
	buttons[ IDBTN_LEFT_TO_RIGHT_GRAPHS ].init(button_t::square_state, "Inverse graphs");
	buttons[ IDBTN_LEFT_TO_RIGHT_GRAPHS ].pressed = !env_t::left_to_right_graphs;
	buttons[ IDBTN_LEFT_TO_RIGHT_GRAPHS ].set_tooltip("Graphs right to left instead of left to right");
	add_component( buttons + IDBTN_LEFT_TO_RIGHT_GRAPHS );

	// Show thememanager
	buttons[ IDBTN_SHOW_THEMEMANAGER ].init( button_t::roundbox_state | button_t::flexible, "Select a theme for display" );
	add_component( buttons + IDBTN_SHOW_THEMEMANAGER );

	// Change font
	buttons[ IDBTN_CHANGE_FONT ].init( button_t::roundbox_state | button_t::flexible, "Select display font" );
	add_component( buttons + IDBTN_CHANGE_FONT );

	// add controls to info container
	add_table(2,4);
	{
		set_alignment(ALIGN_LEFT);
		// Frame time label
		new_component<gui_label_t>("Frame time:");
		frame_time_value_label.buf().printf(" 9999 ms");
		frame_time_value_label.update();
		add_component(&frame_time_value_label);
		// Idle time label
		new_component<gui_label_t>("Idle:");
		idle_time_value_label.buf().printf(" 9999 ms");
		idle_time_value_label.update();
		add_component(&idle_time_value_label);
		// FPS label
		new_component<gui_label_t>("FPS:");
		fps_value_label.buf().printf(" 99.9 fps");
		fps_value_label.update();
		add_component(&fps_value_label);
		// Simloops label
		new_component<gui_label_t>("Sim:");
		simloops_value_label.buf().printf(" 999.9");
		simloops_value_label.update();
		add_component(&simloops_value_label);
	}
	end_table();

	new_component<gui_divider_t>();

	add_table(2, 0);
	{
		set_alignment(ALIGN_LEFT);
		new_component<gui_label_t>("Rebuild connexions:");
		rebuild_connexion_label.buf().printf("-");
		rebuild_connexion_label.set_color(SYSCOL_TEXT_TITLE);
		rebuild_connexion_label.update();
		add_component(&rebuild_connexion_label);

		new_component<gui_label_t>("Filter eligible halts:");
		eligible_halts_label.buf().printf("-");
		eligible_halts_label.set_color(SYSCOL_TEXT_TITLE);
		eligible_halts_label.update();
		add_component(&eligible_halts_label);

		new_component<gui_label_t>("Fill path matrix:");
		fill_path_matrix_label.buf().printf("-");
		fill_path_matrix_label.set_color(SYSCOL_TEXT_TITLE);
		fill_path_matrix_label.update();
		add_component(&fill_path_matrix_label);

		new_component<gui_label_t>("Explore paths:");
		explore_path_label.buf().printf("-");
		explore_path_label.set_color(SYSCOL_TEXT_TITLE);
		explore_path_label.update();
		add_component(&explore_path_label);

		new_component<gui_label_t>("Re-route goods:");
		reroute_goods_label.buf().printf("-");
		reroute_goods_label.set_color(SYSCOL_TEXT_TITLE);
		reroute_goods_label.update();
		add_component(&reroute_goods_label);

		new_component<gui_label_t>("Status:");
		status_label.buf().printf("stand-by");
		status_label.set_color(SYSCOL_TEXT_TITLE);
		status_label.update();
		add_component(&status_label);
	}
	end_table();
}

void gui_settings_t::draw(scr_coord offset)
{
	// Update label buffers
	frame_time_value_label.buf().printf(" %d ms", get_frame_time() );
	frame_time_value_label.update();
	idle_time_value_label.buf().printf(" %d ms", world()->get_idle_time() );
	idle_time_value_label.update();

	// fps_label
	uint32 target_fps = world()->is_fast_forward() ? env_t::ff_fps : env_t::fps;
	uint32 loops = world()->get_realFPS();
	PIXVAL color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops < (target_fps*16*3)/4  ) {
		color = color_idx_to_rgb(( loops <= target_fps*16/2 ) ? COL_RED : COL_YELLOW);
	}
	fps_value_label.set_color(color);
	fps_value_label.buf().printf(" %d fps", loops/16 );
#if MSG_LEVEL >= 3
	if(  env_t::simple_drawing  ) {
		fps_value_label.buf().append( "*" );
	}
#endif
	fps_value_label.update();

	//simloops_label
	loops = world()->get_simloops();
	color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops <= 30  ) {
		color = color_idx_to_rgb((loops<=20) ? COL_RED : COL_YELLOW);
	}
	simloops_value_label.set_color(color);
	simloops_value_label.buf().printf(" %d%c%d", loops/10, get_fraction_sep(), loops%10 );
	simloops_value_label.update();

	if ( path_explorer_t::is_processing() )
	{
		status_label.buf().printf("%s (%s) / %s", translator::translate(path_explorer_t::get_current_category_name()), path_explorer_t::get_current_class_name(), path_explorer_t::get_current_phase_name());
	}
	else {
		status_label.buf().printf("%s", "stand-by");
	}
	status_label.update();

	rebuild_connexion_label.buf().printf("%lu", path_explorer_t::get_limit_rebuild_connexions());
	rebuild_connexion_label.update();

	eligible_halts_label.buf().printf("%lu", path_explorer_t::get_limit_filter_eligible());
	eligible_halts_label.update();

	fill_path_matrix_label.buf().printf("%lu", path_explorer_t::get_limit_fill_matrix());
	fill_path_matrix_label.update();

	explore_path_label.buf().printf("%lu", (long)path_explorer_t::get_limit_explore_paths());
	explore_path_label.update();

	reroute_goods_label.buf().printf("%lu", path_explorer_t::get_limit_reroute_goods());
	reroute_goods_label.update();

	// All components are updated, now draw them...
	gui_aligned_container_t::draw(offset);
}


map_settings_t::map_settings_t()
{
	set_table_layout( 1, 0 );
	new_component<gui_label_t>("Grid");
	add_table(3,0);
	{
		// Show grid checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SHOW_GRID].init(button_t::square_state, "show grid");
		buttons[IDBTN_SHOW_GRID].set_tooltip("Shows the borderlines of each tile in the main game window. Can be useful for construction. Toggle with the # key.");
		add_component(buttons + IDBTN_SHOW_GRID, 2);

		// Underground view checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_UNDERGROUND_VIEW].init(button_t::square_state, "underground mode");
		buttons[IDBTN_UNDERGROUND_VIEW].set_tooltip("See under the ground, to build tunnels and underground railways/metros. Toggle with SHIFT + U");
		add_component(buttons + IDBTN_UNDERGROUND_VIEW, 2);

		// Show slice map view checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SHOW_SLICE_MAP_VIEW].init(button_t::square_state, "sliced underground mode");
		buttons[IDBTN_SHOW_SLICE_MAP_VIEW].set_tooltip("See under the ground, one layer at a time. Toggle with CTRL + U. Move up/down in layers with HOME and END.");
		add_component(buttons + IDBTN_SHOW_SLICE_MAP_VIEW);
		// underground slice edit
		inp_underground_level.set_value(grund_t::underground_mode == grund_t::ugm_level ? grund_t::underground_level : world()->get_zeiger()->get_pos().z);
		inp_underground_level.set_limits(world()->get_groundwater() - 10, 32);
		inp_underground_level.add_listener(this);
		add_component(&inp_underground_level);

		// Toggle simple drawing for debugging
#ifdef DEBUG
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SIMPLE_DRAWING].init(button_t::square_state, "Simple drawing");
		add_component(buttons + IDBTN_SIMPLE_DRAWING, 2);
#endif
	}
	end_table();

	new_component<gui_label_t>("Brightness");
	add_table(3, 0);
	{
		// Day/night change checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_DAY_NIGHT_CHANGE].init(button_t::square_state, "8WORLD_CHOOSE");
		buttons[IDBTN_DAY_NIGHT_CHANGE].set_tooltip("Whether the lighting in the main game window simulates a periodic transition between day and night.");
		add_component(buttons + IDBTN_DAY_NIGHT_CHANGE, 2);

		// Brightness label
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("1LIGHT_CHOOSE");
		// brightness edit
		brightness.set_value(env_t::daynight_level);
		brightness.set_limits(0, 9);
		brightness.add_listener(this);
		add_component(&brightness);
	}
	end_table();

	new_component<gui_label_t>("Map scroll");
	add_table(3, 0);
	{
		// Scroll inverse checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SCROLL_INVERSE].init(button_t::square_state, "4LIGHT_CHOOSE");
		buttons[IDBTN_SCROLL_INVERSE].set_tooltip("The main game window can be scrolled by right-clicking and dragging the ground.");
		add_component(buttons + IDBTN_SCROLL_INVERSE, 2);

		// Numpad key
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_IGNORE_NUMLOCK].init(button_t::square_state, "Num pad keys always move map");
		buttons[IDBTN_IGNORE_NUMLOCK].pressed = env_t::numpad_always_moves_map;
		add_component(buttons + IDBTN_IGNORE_NUMLOCK, 2);

		// Scroll speed label
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("3LIGHT_CHOOSE");
		// Scroll speed edit
		scrollspeed.set_value(abs(env_t::scroll_multi));
		scrollspeed.set_limits(1, 9);
		scrollspeed.add_listener(this);
		add_component(&scrollspeed);
	}
	end_table();
	new_component<gui_divider_t>();

	new_component<gui_label_t>("transparencies");
	add_table(3, 0);
	{
		// Transparent instead of hidden checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN].init(button_t::square_state, "hide transparent");
		buttons[IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN].set_tooltip("All hidden items (such as trees and buildings) will appear as transparent.");
		add_component(buttons + IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN, 2);

		// Hide trees checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_HIDE_TREES].init(button_t::square_state, "hide trees");
		buttons[IDBTN_HIDE_TREES].set_tooltip("Trees will be miniaturised or made transparent in the main game window.");
		add_component(buttons + IDBTN_HIDE_TREES, 2);

		// Hide buildings
		new_component<gui_margin_t>(LINESPACE/2);
		hide_buildings.set_focusable(false);
		hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("no buildings hidden"), SYSCOL_TEXT);
		hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("hide city building"), SYSCOL_TEXT);
		hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("hide all building"), SYSCOL_TEXT);
		hide_buildings.set_selection(env_t::hide_buildings);
		add_component(&hide_buildings, 2);
		hide_buildings.add_listener(this);

		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_HIDE_BUILDINGS].init(button_t::square_state, "Smart hide objects");
		buttons[IDBTN_HIDE_BUILDINGS].set_tooltip("hide objects under cursor");
		add_component(buttons + IDBTN_HIDE_BUILDINGS);
		// Smart hide objects edit
		cursor_hide_range.set_value(env_t::cursor_hide_range);
		cursor_hide_range.set_limits(0, 10);
		cursor_hide_range.add_listener(this);
		add_component(&cursor_hide_range);
	}
	end_table();
}

bool map_settings_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	// Brightness edit
	if( &brightness == comp ) {
		env_t::daynight_level = (sint8)v.i;
	}
	// Scroll speed edit
	if( &scrollspeed == comp ) {
		env_t::scroll_multi = (sint16)(buttons[ IDBTN_SCROLL_INVERSE ].pressed ? -v.i : v.i);
	}
	// underground slice edit
	if( comp == &inp_underground_level ) {
		if( grund_t::underground_mode == grund_t::ugm_level ) {
			grund_t::underground_level = inp_underground_level.get_value();

			// calc new images
			world()->update_underground();
		}
	}
	// Smart hide objects edit
	if( &cursor_hide_range == comp ) {
		env_t::cursor_hide_range = cursor_hide_range.get_value();
	}
	// Hide building
	if( &hide_buildings == comp ) {
		env_t::hide_buildings = v.i;
		world()->set_dirty();
	}
	return true;
}


void map_settings_t::draw( scr_coord offset )
{
	hide_buildings.set_selection( env_t::hide_buildings );

	gui_aligned_container_t::draw(offset);
}


label_settings_t::label_settings_t()
{
	set_table_layout(1,0);

	// Show signalbox coverage
	buttons[IDBTN_SHOW_SIGNALBOX_COVERAGE].init(button_t::square_state, "show signalbox coverage");
	buttons[IDBTN_SHOW_SIGNALBOX_COVERAGE].pressed = env_t::signalbox_coverage_show;
	buttons[IDBTN_SHOW_SIGNALBOX_COVERAGE].set_tooltip("Show coverage radius of the signalbox.");
	add_component(buttons + IDBTN_SHOW_SIGNALBOX_COVERAGE);
	new_component<gui_divider_t>();

	new_component<gui_label_t>("Station display");
	add_table(5, 0);
	{
		// Transparent station coverage
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_TRANSPARENT_STATION_COVERAGE].init(button_t::square_state, "transparent station coverage");
		buttons[IDBTN_TRANSPARENT_STATION_COVERAGE].pressed = env_t::use_transparency_station_coverage;
		buttons[IDBTN_TRANSPARENT_STATION_COVERAGE].set_tooltip("The display of the station coverage can either be a transparent rectangle or a series of boxes.");
		add_component(buttons + IDBTN_TRANSPARENT_STATION_COVERAGE, 4);

		// Show station coverage
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SHOW_STATION_COVERAGE].init(button_t::square_state, "show station coverage");
		buttons[IDBTN_SHOW_STATION_COVERAGE].set_tooltip("Show from how far that passengers or goods will come to use your stops. Toggle with the v key.");
		add_component(buttons + IDBTN_SHOW_STATION_COVERAGE, 4);

		// Show waiting bars checkbox
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SHOW_WAITING_BARS].init(button_t::square_state, "show waiting bars");
		buttons[IDBTN_SHOW_WAITING_BARS].pressed = env_t::show_names & 2;
		buttons[IDBTN_SHOW_WAITING_BARS].set_tooltip("Shows a bar graph representing the number of passengers/mail/goods waiting at stops.");
		add_component(buttons + IDBTN_SHOW_WAITING_BARS, 4);

		// waiting bar option for passenger and mail classes
		bool pakset_has_pass_classes = (goods_manager_t::passengers->get_number_of_classes() > 1);
		bool pakset_has_mail_classes = (goods_manager_t::mail->get_number_of_classes() > 1);
		if (pakset_has_pass_classes || pakset_has_mail_classes) {
			new_component<gui_margin_t>(LINESPACE/2);
			new_component<gui_margin_t>(LINESPACE/2);
			new_component<gui_image_t>()->set_image(pakset_has_pass_classes ? skinverwaltung_t::passengers->get_image_id(0) : IMG_EMPTY, true);
			new_component<gui_image_t>()->set_image(pakset_has_mail_classes ? skinverwaltung_t::mail->get_image_id(0) : IMG_EMPTY, true);
			buttons[IDBTN_CLASSES_WAITING_BAR].init(button_t::square_state, "Divided by class");
			buttons[IDBTN_CLASSES_WAITING_BAR].pressed = env_t::classes_waiting_bar;
			buttons[IDBTN_CLASSES_WAITING_BAR].enable(env_t::show_names & 2);
			buttons[IDBTN_CLASSES_WAITING_BAR].set_tooltip("Waiting bars are displayed separately for each class.");
			add_component(buttons + IDBTN_CLASSES_WAITING_BAR);
		}

		// waiting bar option for freight
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_image_t>()->set_image(skinverwaltung_t::goods->get_image_id(0), true);
		new_component<gui_empty_t>();
		freight_waiting_bar.set_focusable(false);
		freight_waiting_bar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Unified"), SYSCOL_TEXT);
		freight_waiting_bar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Divided by category"), SYSCOL_TEXT);
		freight_waiting_bar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Divided by goods"), SYSCOL_TEXT);
		freight_waiting_bar.set_selection(env_t::freight_waiting_bar_level);
		freight_waiting_bar.enable(env_t::show_names & 2);
		add_component(&freight_waiting_bar);
		freight_waiting_bar.add_listener(this);
	}
	end_table();

	// Show station names arrow
	add_table(3,1);
	{
		new_component<gui_margin_t>(LINESPACE/2);
		buttons[IDBTN_SHOW_STATION_NAMES_ARROW].set_typ(button_t::arrowright);
		buttons[IDBTN_SHOW_STATION_NAMES_ARROW].set_tooltip("Shows the names of the individual stations in the main game window.");
		add_component(buttons + IDBTN_SHOW_STATION_NAMES_ARROW);
		new_component<gui_label_stationname_t>("show station names");
	}
	end_table();

	new_component<gui_divider_t>();

	new_component<gui_label_t>("Convoy tooltips");
	add_table(3,0);
	{
		// Convoy nameplate
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("Nameplates")->set_tooltip(translator::translate("The line name or convoy name is displayed above the convoy."));
		convoy_nameplate.set_focusable(false);
		convoy_nameplate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("never show"), SYSCOL_TEXT);
		convoy_nameplate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("mouseover"), SYSCOL_TEXT);
		convoy_nameplate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("only active player's"), SYSCOL_TEXT);
		convoy_nameplate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("always show all"), SYSCOL_TEXT);
		convoy_nameplate.set_selection(env_t::show_cnv_nameplates);
		add_component(&convoy_nameplate);
		convoy_nameplate.add_listener(this);

		// Convoy loading bar
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("Loading bar")->set_tooltip(translator::translate("A loading rate bar is displayed above the convoy."));
		convoy_loadingbar.set_focusable(false);
		convoy_loadingbar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("never show"), SYSCOL_TEXT);
		convoy_loadingbar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("mouseover"), SYSCOL_TEXT);
		convoy_loadingbar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("only active player's"), SYSCOL_TEXT);
		convoy_loadingbar.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("always show all"), SYSCOL_TEXT);
		convoy_loadingbar.set_selection(env_t::show_cnv_loadingbar);
		add_component(&convoy_loadingbar);
		convoy_loadingbar.add_listener(this);

		// Convoy tooltip
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("Tooltip")->set_tooltip(translator::translate("Toggle vehicle tooltips"));
		convoy_tooltip.set_focusable(false);
		convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("mouseover"), SYSCOL_TEXT);
		convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("only error convoys"), SYSCOL_TEXT);
		convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("only active player's"), SYSCOL_TEXT);
		convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("always show all"), SYSCOL_TEXT);
		convoy_tooltip.set_selection(env_t::show_vehicle_states);
		add_component(&convoy_tooltip);
		convoy_tooltip.add_listener(this);

		// convoi booking message options
		new_component<gui_margin_t>(LINESPACE/2);
		new_component<gui_label_t>("Money message")->set_tooltip(translator::translate("Income display displayed when convoy arrives at the stop."));
		money_booking.set_focusable(false);
		money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("always show all"), SYSCOL_TEXT);
		money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("only active player's"), SYSCOL_TEXT);
		money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("never show"), SYSCOL_TEXT);
		money_booking.set_selection(env_t::show_money_message);
		add_component(&money_booking, 2);
		money_booking.add_listener(this);
	}
	end_table();

}

bool label_settings_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	// Convoy tooltip
	if (&convoy_tooltip == comp) {
		env_t::show_vehicle_states = v.i;
	}
	// Convoy nameplate
	if (&convoy_nameplate == comp) {
		env_t::show_cnv_nameplates = v.i;
	}
	// Convoy loading bar
	if (&convoy_loadingbar == comp) {
		env_t::show_cnv_loadingbar = v.i;
	}
	if (&money_booking == comp) {
		env_t::show_money_message = v.i;
	}
	// freight waiting bar detail level
	if (&freight_waiting_bar == comp) {
		env_t::freight_waiting_bar_level = v.i;
	}
	return true;
}

void label_settings_t::draw(scr_coord offset)
{
	convoy_nameplate.set_selection(env_t::show_cnv_nameplates);
	convoy_loadingbar.set_selection(env_t::show_cnv_loadingbar);
	convoy_tooltip.set_selection(env_t::show_vehicle_states);
	freight_waiting_bar.set_selection(env_t::freight_waiting_bar_level);
	freight_waiting_bar.enable(env_t::show_names & 2);

	gui_aligned_container_t::draw(offset);
}

traffic_settings_t::traffic_settings_t()
{
	set_table_layout( 1, 0 );
	add_table( 2, 0 );

	// Pedestrians in towns checkbox
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].init(button_t::square_state, "6LIGHT_CHOOSE");
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].set_tooltip("Pedestrians will appear randomly in towns.");
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].pressed = world()->get_settings().get_random_pedestrians();
	add_component(buttons+IDBTN_PEDESTRIANS_IN_TOWNS, 2);

	// Pedestrians at stops checkbox
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].init(button_t::square_state, "5LIGHT_CHOOSE");
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].set_tooltip("Pedestrians will appear near stops whenver a passenger vehicle unloads there.");
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].pressed = world()->get_settings().get_show_pax();
	add_component(buttons+IDBTN_PEDESTRIANS_AT_STOPS, 2);

	// Traffic density label
	new_component<gui_label_t>("6WORLD_CHOOSE");

	// Traffic density edit
	traffic_density.set_value(world()->get_settings().get_traffic_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);
	add_component(&traffic_density);

	// Convoy follow mode
	new_component<gui_label_t>("Convoi following mode")->set_tooltip(translator::translate("Select the behavior of the camera when the following convoy enters the tunnel."));

	follow_mode.set_focusable(false);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("do nothing"), SYSCOL_TEXT);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("underground mode"), SYSCOL_TEXT);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("sliced underground mode"), SYSCOL_TEXT);
	follow_mode.set_selection(env_t::follow_convoi_underground);
	add_component(&follow_mode);
	follow_mode.add_listener(this);

	// Show schedule's stop checkbox
	buttons[IDBTN_SHOW_SCHEDULES_STOP].init( button_t::square_state ,  "Highlite schedule" );
	buttons[IDBTN_SHOW_SCHEDULES_STOP].set_tooltip("Highlight the locations of stops on the current schedule");
	add_component(buttons+IDBTN_SHOW_SCHEDULES_STOP, 2);

	end_table();
}

bool traffic_settings_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	// Traffic density edit
	if( &traffic_density == comp ) {
		if( !env_t::networkmode || world()->get_active_player_nr() == 1 ) {
			static char level[ 16 ];
			sprintf( level, "%li", v.i );
			tool_t::simple_tool[ TOOL_TRAFFIC_LEVEL & 0xFFF ]->set_default_param( level );
			world()->set_tool( tool_t::simple_tool[ TOOL_TRAFFIC_LEVEL & 0xFFF ], world()->get_active_player() );
		}
		else {
			traffic_density.set_value( world()->get_settings().get_traffic_level() );
		}
	}

	if( &follow_mode == comp ) {
		env_t::follow_convoi_underground = v.i;
	}
	return true;
}



color_gui_t::color_gui_t() :
	gui_frame_t( translator::translate( "Helligk. u. Farben" ) ),
	scrolly_gui(&gui_settings),
	scrolly_map(&map_settings),
	scrolly_station(&station_settings),
	scrolly_traffic(&traffic_settings)
{
	set_table_layout( 1, 0 );

	scrolly_gui.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_map.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_station.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_traffic.set_scroll_amount_y(D_BUTTON_HEIGHT/2);

	tabs.add_tab(&scrolly_gui, translator::translate("GUI settings"));
	tabs.add_tab(&scrolly_map, translator::translate("map view"));
	tabs.add_tab(&scrolly_station, translator::translate("tooltip/coverage"));
	tabs.add_tab(&scrolly_traffic, translator::translate("traffic settings"));
	add_component(&tabs);

	for( int i = 0; i < COLORS_MAX_BUTTONS; i++ ) {
		buttons[ i ].add_listener( this );
	}

	set_resizemode(diagonal_resize);
	set_min_windowsize( scr_size(D_DEFAULT_WIDTH, max(get_min_windowsize().h, traffic_settings.get_size().h)) );
	// It is assumed that the map view tab is the tab with the most lines.
	set_windowsize( scr_size(D_DEFAULT_WIDTH, map_settings.get_size().h+D_TAB_HEADER_HEIGHT+D_TITLEBAR_HEIGHT+D_MARGINS_Y) );
	resize( scr_coord( 0, 0 ) );
}

bool color_gui_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	int i;
	for(  i=0;  i<COLORS_MAX_BUTTONS  &&  comp!=buttons+i;  i++  ) { }

	switch( i )
	{
	case IDBTN_IGNORE_NUMLOCK:
		env_t::numpad_always_moves_map = !env_t::numpad_always_moves_map;
		buttons[IDBTN_IGNORE_NUMLOCK].pressed = env_t::numpad_always_moves_map;
		break;
	case IDBTN_SCROLL_INVERSE:
		env_t::scroll_multi = -env_t::scroll_multi;
		break;
	case IDBTN_PEDESTRIANS_AT_STOPS:
		if( !env_t::networkmode || welt->get_active_player_nr() == 1 ) {
			welt->set_tool( tool_t::simple_tool[ TOOL_TOOGLE_PAX & 0xFFF ], welt->get_active_player() );
		}
		break;
	case IDBTN_PEDESTRIANS_IN_TOWNS:
		if( !env_t::networkmode || welt->get_active_player_nr() == 1 ) {
			welt->set_tool( tool_t::simple_tool[ TOOL_TOOGLE_PEDESTRIANS & 0xFFF ], welt->get_active_player() );
		}
		break;
	case IDBTN_HIDE_TREES:
		env_t::hide_trees = !env_t::hide_trees;
		baum_t::recalc_outline_color();
		break;
	case IDBTN_DAY_NIGHT_CHANGE:
		env_t::night_shift = !env_t::night_shift;
		break;
	case IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN:
		env_t::hide_with_transparency = !env_t::hide_with_transparency;
		baum_t::recalc_outline_color();
		break;
	case IDBTN_TRANSPARENT_STATION_COVERAGE:
		env_t::use_transparency_station_coverage = !env_t::use_transparency_station_coverage;
		break;
	case IDBTN_SHOW_STATION_COVERAGE:
		env_t::station_coverage_show = env_t::station_coverage_show == 0 ? 0xFF : 0;
		break;
	case IDBTN_UNDERGROUND_VIEW:
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode( buttons[ IDBTN_UNDERGROUND_VIEW ].pressed ? grund_t::ugm_none : grund_t::ugm_all, map_settings.inp_underground_level.get_value() );

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_SHOW_GRID:
		grund_t::toggle_grid();
		break;
	case IDBTN_SHOW_STATION_NAMES_ARROW:
		if( env_t::show_names & 1 ) {
			if( (env_t::show_names >> 2) == 2 ) {
				env_t::show_names &= 2;
			}
			else {
				env_t::show_names += 4;
			}
		}
		else {
			env_t::show_names &= 2;
			env_t::show_names |= 1;
		}
		break;
	case IDBTN_SHOW_WAITING_BARS:
		env_t::show_names ^= 2;
		buttons[IDBTN_CLASSES_WAITING_BAR].enable(env_t::show_names & 2);
		break;
	case IDBTN_CLASSES_WAITING_BAR:
		env_t::classes_waiting_bar = !env_t::classes_waiting_bar;
		buttons[IDBTN_CLASSES_WAITING_BAR].pressed ^= 1;
		break;
	case IDBTN_SHOW_SLICE_MAP_VIEW:
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode( buttons[ IDBTN_SHOW_SLICE_MAP_VIEW ].pressed ? grund_t::ugm_none : grund_t::ugm_level, map_settings.inp_underground_level.get_value() );

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_HIDE_BUILDINGS:
		// see simtool.cc::tool_hide_under_cursor_t::init
		env_t::hide_under_cursor = !env_t::hide_under_cursor  &&  env_t::cursor_hide_range > 0;

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_SHOW_SCHEDULES_STOP:
		env_t::visualize_schedule = !env_t::visualize_schedule;
		break;
	case IDBTN_SHOW_THEMEMANAGER:
		create_win( new themeselector_t(), w_info, magic_themes );
		break;
	case IDBTN_SIMPLE_DRAWING:
		env_t::simple_drawing = !env_t::simple_drawing;
		break;
	case IDBTN_CHANGE_FONT:
		create_win( new loadfont_frame_t(), w_info, magic_font );
		break;
	case IDBTN_LEFT_TO_RIGHT_GRAPHS:
		env_t::left_to_right_graphs = !env_t::left_to_right_graphs;
		buttons[IDBTN_LEFT_TO_RIGHT_GRAPHS].pressed ^= 1;
		break;
	case IDBTN_SHOW_SIGNALBOX_COVERAGE:
		env_t::signalbox_coverage_show = env_t::signalbox_coverage_show == 0 ? 0xFF : 0;
		break;
	default:
		assert( 0 );
	}

	welt->set_dirty();

	return true;
}


void color_gui_t::draw(scr_coord pos, scr_size size)
{
	// Update button states that was changed with keyboard ...
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].pressed = welt->get_settings().get_show_pax();
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].pressed = welt->get_settings().get_random_pedestrians();
	buttons[IDBTN_HIDE_TREES].pressed = env_t::hide_trees;
	buttons[IDBTN_HIDE_BUILDINGS].pressed = env_t::hide_under_cursor;
	buttons[IDBTN_SHOW_STATION_COVERAGE].pressed = env_t::station_coverage_show;
	buttons[IDBTN_UNDERGROUND_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[IDBTN_SHOW_GRID].pressed = grund_t::show_grid;
	buttons[IDBTN_SHOW_WAITING_BARS].pressed = (env_t::show_names&2)!=0;
	buttons[IDBTN_SHOW_SLICE_MAP_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[IDBTN_SHOW_SCHEDULES_STOP].pressed = env_t::visualize_schedule;
	buttons[IDBTN_SIMPLE_DRAWING].pressed = env_t::simple_drawing;
	buttons[IDBTN_SIMPLE_DRAWING].enable(welt->is_paused());
	buttons[IDBTN_SCROLL_INVERSE].pressed = env_t::scroll_multi < 0;
	buttons[IDBTN_DAY_NIGHT_CHANGE].pressed = env_t::night_shift;
	buttons[IDBTN_SHOW_SLICE_MAP_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[IDBTN_UNDERGROUND_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[IDBTN_TRANSPARENT_STATION_COVERAGE].pressed = env_t::use_transparency_station_coverage;
	buttons[IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN].pressed = env_t::hide_with_transparency;


	buttons[IDBTN_SHOW_SIGNALBOX_COVERAGE].pressed = env_t::signalbox_coverage_show;

	// All components are updated, now draw them...
	gui_frame_t::draw(pos, size);
}
