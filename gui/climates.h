/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CLIMATES_H
#define GUI_CLIMATES_H


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "components/action_listener.h"
#include "components/gui_textinput.h"

class settings_t;


/**
 * set the climate borders
 */
class climate_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	settings_t* sets;

	enum { MAX_CLIMATE_LABEL=15 };
	gui_numberinput_t
		water_level,
		mountain_height,
		mountain_roughness,
		snowline_winter,
		climate_borders_ui[rocky_climate];

	gui_label_buf_t
		summer_snowline;

	button_t
		no_tree; // without tree

	button_t
		lake; // lake

	gui_numberinput_t
		river_n,
		river_min,
		river_max;

	// Gives a hilly landscape
	// @author: jamespetts
	button_t hilly;

	// If this is selected, cities will not prefer
	// lower grounds as they do by default.
	// @ author: jamespetts
	button_t cities_ignore_height;

	gui_numberinput_t cities_like_water;
public:
	climate_gui_t(settings_t*);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "climates.txt";}

	// does not work during new world dialog
	bool has_sticky() const OVERRIDE { return false; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// called when river number changed due to increasing map size
	void update_river_number( sint16 new_river_number );
};

#endif
