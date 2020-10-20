/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CLIMATES_H
#define GUI_CLIMATES_H


#include "gui_frame.h"
#include "components/gui_aligned_container.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"

class settings_t;


/**
 * set the climate borders
 */
class climate_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	settings_t* sets;

	gui_numberinput_t
		water_level,
		mountain_height,
		mountain_roughness,
		climate_borders_ui[MAX_CLIMATES][2],
		// this is only for height based climates
		patch_size,
		// this is only for humidity based climates
		snowline_winter,
		snowline_summer,
		moistering,
		moistering_water,
		humidities[2],
		temperatures[5];

	gui_combobox_t
		tree, wind_dir;

	gui_numberinput_t
		lake; // lake height

	gui_numberinput_t
		river_n,
		river_min,
		river_max;

	gui_tab_panel_t
		climate_generator;


	gui_aligned_container_t height_climate;
	gui_aligned_container_t humidity_climate;

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
