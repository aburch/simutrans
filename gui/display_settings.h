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

#define COLORS_MAX_BUTTONS (23)
#define BUF_MAXLEN_MS_FORMAT (10)

class karte_t;

/**
 * Display settings dialog
 * @author Hj. Malthaner
 */
class color_gui_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;

	button_t buttons[COLORS_MAX_BUTTONS];

	gui_numberinput_t
		brightness,
		scrollspeed,
		traffic_density,
		inp_underground_level,
		cursor_hide_range;

	gui_label_t
		brightness_label,
		scrollspeed_label,
		hide_buildings_label,
		traffic_density_label,
		convoy_tooltip_label,
		frame_time_label,
		frame_time_value_label,
		idle_time_label,
		idle_time_value_label,
		fps_label,
		fps_value_label,
		simloops_label,
		simloops_value_label;

	gui_divider_t
		divider1,
		divider2,
		divider3,
		divider4;

	// Non translated text buffers for label values
	char frame_time_buf[BUF_MAXLEN_MS_FORMAT];
	char idle_time_buf[BUF_MAXLEN_MS_FORMAT];
	char fps_buf[BUF_MAXLEN_MS_FORMAT];
	char simloops_buf[BUF_MAXLEN_MS_FORMAT];

public:
	color_gui_t(karte_t *welt);

	/**
	 * Some windows have associated help text.
	 * @return The help file name or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "display.txt"; }

	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void set_fenstergroesse(koord groesse);
};

#endif
