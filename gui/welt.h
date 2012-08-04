/*
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
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

#include "../tpl/array2d_tpl.h"

class settings_t;
class karte_t;

/**
 * Ein Dialog mit Einstellungen fuer eine neue Karte
 *
 * @author Hj. Malthaner, Niels Roest
 */
class welt_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	settings_t* sets;

	/**
	* Mini Karten-Preview
	* @author Hj. Malthaner
	*/
	array2d_tpl<uint8> karte;
	koord karte_size;

	bool load_heightfield, loaded_heightfield;
	bool load;
	bool start;
	bool close;
	bool scenario;
	bool quit;

	double city_density;
	double industry_density;
	double attraction_density;
	double river_density;

	int old_lang;

	gui_numberinput_t inp_map_number, inp_x_size, inp_y_size;

	button_t random_map, load_map;

	gui_numberinput_t
		inp_number_of_towns,
		inp_town_size,
		inp_intercity_road_len,
		inp_other_industries,
		inp_tourist_attractions,
		inp_intro_date;

	button_t use_intro_dates, use_beginner_mode;

	button_t open_climate_gui, open_setting_gui;

	button_t load_game;
	button_t load_scenario;
	button_t start_game;
	button_t quit_game;

	karte_t *welt;

	/**
	* Calculates preview from height map
	* @param filename name of heightfield file
	* @author Hajo/prissi
	*/
	bool update_from_heightfield(const char *filename);

	void resize_preview();

	void update_densities();

public:
	welt_gui_t(karte_t*, settings_t*);

	/**
	* Berechnet Preview-Karte neu. Inititialisiert RNG neu!
	* public, because also the climate dialog need it
	* @author Hj. Malthaner
	*/
	void update_preview();
	void clear_loaded_heightfield() { loaded_heightfield =0; }
	bool get_loaded_heightfield() const { return loaded_heightfield; }

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "new_world.txt";}

	settings_t* get_sets() const { return sets; }

	// does not work during new world dialoge
	virtual bool has_sticky() const { return false; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 *
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
