/*
 * Menu with display settings
 * @author Hj. Malthaner
 */

#ifndef _display_settings_h_
#define _display_settings_h_

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"

#define COLORS_MAX_BUTTONS (23)

class karte_t;

class color_gui_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;
	button_t buttons[COLORS_MAX_BUTTONS];
	gui_numberinput_t brightness, scrollspeed, traffic_density, inp_underground_level, cursor_hide_range;

public:
	color_gui_t(karte_t *welt);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "display.txt"; }

	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void set_fenstergroesse(koord groesse);
};

#endif
