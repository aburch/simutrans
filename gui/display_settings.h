/*
 * Menu with display settings
 * @author Hj. Malthaner
 */

#ifndef _display_settings_h_
#define _display_settings_h_

#include "gui_frame.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"
#include "components/gui_combobox.h"



/**
 * Display settings dialog
 * @author Hj. Malthaner
 */
class color_gui_t : public gui_frame_t, private action_listener_t
{
private:

enum {
	IDBTN_SCROLL_INVERSE,
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
	COLORS_MAX_BUTTONS,
};

	button_t buttons[COLORS_MAX_BUTTONS];

	gui_numberinput_t
		brightness,
		scrollspeed,
		traffic_density,
		inp_underground_level,
		cursor_hide_range;

	gui_label_buf_t
		frame_time_value_label,
		idle_time_value_label,
		fps_value_label,
		simloops_value_label;

	gui_combobox_t
		convoy_tooltip,
		hide_buildings,
		money_booking;

	gui_aligned_container_t* container_bottom;

	void update_labels();

public:
	color_gui_t();

	/**
	 * Some windows have associated help text.
	 * @return The help file name or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE { return "display.txt"; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
