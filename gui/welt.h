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

class einstellungen_t;


/**
 * Ein Dialog mit Einstellungen fuer eine neue Karte
 *
 * @author Hj. Malthaner, Niels Roest
 */
class welt_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	einstellungen_t *sets;

	enum { preview_size = 64 };

	/**
	* Mini Karten-Preview
	* @author Hj. Malthaner
	*/
	unsigned char karte[preview_size*preview_size];

	char map_number_s[16];
	bool load_heightfield, loaded_heightfield;
	bool load;
	bool start;
	bool close;
	bool scenario;
	bool quit;

	int old_lang;

	gui_numberinput_t inp_map_number, inp_x_size, inp_y_size;

	button_t random_map, load_map;

	gui_numberinput_t inp_number_of_towns,
		inp_town_size,
		inp_intercity_road_len,
		inp_traffic_density,
		inp_other_industries,

		inp_electric_producer,
		inp_tourist_attractions,
		inp_intro_date;

	button_t use_intro_dates;
	button_t allow_player_change;

	button_t open_climate_gui, open_sprach_gui;

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


public:
	welt_gui_t(karte_t *welt, einstellungen_t *sets);

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

	bool get_load_heightfield() const {return load_heightfield;}
	bool get_scenario() const {return scenario;}
	bool get_load() const {return load;}
	bool get_start() const {return start;}
	bool get_close() const {return close;}
	bool get_quit() const {return quit;}

	einstellungen_t* get_sets() const { return sets; }

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *ev);

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 *
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
