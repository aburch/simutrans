/*
 * Dialog to configure the generation of a new map
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#ifndef welt_gui_h
#define welt_gui_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_textinput.h"
#include "components/gui_numberinput.h"
#include "components/gui_divider.h"
#include "components/gui_map_preview.h"

#include "../tpl/array2d_tpl.h"

class settings_t;

/**
 * The dialog for new world generation
 *
 * @author Hj. Malthaner, Niels Roest
 */
class welt_gui_t  :
	public  gui_frame_t,
	private action_listener_t
{
	private:
		settings_t* sets;

		/**
		* Mini Map-Preview
		* @author Hj. Malthaner
		*/
		array2d_tpl<uint8>   map;
		scr_size            map_size;

		bool load_heightfield;
		bool loaded_heightfield;
		bool load;
		bool start;
		bool close;
		bool scenario;
		bool quit;

		double city_density;
		double industry_density;
		double attraction_density;
		double river_density;

		gui_map_preview_t
			map_preview;

		gui_numberinput_t
			inp_map_number,
			inp_x_size, inp_y_size;

		button_t
			random_map,
			load_map;

		gui_numberinput_t
			inp_number_of_towns,
			inp_town_size,
			inp_intercity_road_len,
			inp_other_industries,
			inp_tourist_attractions,
			inp_intro_date;

		gui_label_t
			world_title_label,
			map_number_label,
			size_label,
			cities_label,
			median_label,
			intercity_label,
			factories_label,
			tourist_label;

		button_t
			use_intro_dates,
			use_beginner_mode,
			open_climate_gui,
			open_setting_gui,
			load_game,
			load_scenario,
			start_game,
			quit_game;

		gui_divider_t
			divider_1,
			divider_2,
			divider_3;

	/**
	* Calculates preview from height map
	* @param filename name of heightfield file
	* @author Hajo/prissi
	*/
	bool update_from_heightfield(const char *filename);

	void resize_preview();

	void update_densities();

	public:
		welt_gui_t(settings_t*);

		/**
		* Berechnet Preview-Karte neu. Inititialisiert RNG neu!
		* public, because also the climate dialog need it
		* @author Hj. Malthaner
		*/
		void update_preview();
		void clear_loaded_heightfield() { loaded_heightfield =0; }
		bool get_loaded_heightfield() const { return loaded_heightfield; }

		/**
		* Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 * @author Hj. Malthaner
		 */
		const char * get_hilfe_datei() const {return "new_world.txt";}

		settings_t* get_sets() const { return sets; }

		// does not work during new world dialog
		bool has_sticky() const { return false; }

		bool infowin_event(event_t const*) OVERRIDE;

		/**
		 * Draw new component. The values to be passed refer to the window
		 * i.e. It's the screen coordinates of the window where the
		 * component is displayed.
		 * @author Hj. Malthaner
		 */
		void draw(scr_coord pos, scr_size size);

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
