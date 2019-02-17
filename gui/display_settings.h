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

#define COLORS_MAX_BUTTONS (27)


/**
 * Display settings dialog
 * @author Hj. Malthaner
 */
class color_gui_t : public gui_frame_t, private action_listener_t
{
private:

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
