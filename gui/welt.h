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

class einstellungen_t;


/**
 * Ein Dialog mit Einstellungen fuer eine neue Karte
 *
 * @author Hj. Malthaner, Niels Roest
 */
class welt_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	einstellungen_t * sets;

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

	// since decrease/increase buttons always pair these ...
	button_t map_number[2];
	gui_textinput_t inp_map_number;	// direct map number entering
	button_t x_size[2];
	button_t y_size[2];
	button_t random_map, load_map;

	button_t number_of_towns[2];
	button_t town_size[2];
	button_t intercity_road_len[2];
	button_t traffic_desity[2];

	button_t other_industries[2];
	button_t electric_producer[2];
	button_t tourist_attractions[2];

	button_t use_intro_dates;
	button_t intro_date[2];
	button_t allow_player_change;
	button_t use_beginner_mode;

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
	void  update_preview();
	void clear_loaded_heightfield() { loaded_heightfield =0; }
	bool get_loaded_heightfield() const { return loaded_heightfield; }

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * gib_hilfe_datei() const {return "new_world.txt";}

	bool gib_load_heightfield() const {return load_heightfield;}
	bool gib_scenario() const {return scenario;}
	bool gib_load() const {return load;}
	bool gib_start() const {return start;}
	bool gib_close() const {return close;}
	bool gib_quit() const {return quit;}

	einstellungen_t* gib_sets() const { return sets; }

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
	bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
