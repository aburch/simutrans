/*
 * welt.h
 *
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
 *
 * Hj. Malthaner
 *
 * April 2000
 */


#ifndef welt_gui_h
#define welt_gui_h

#include "gui_frame.h"
#include "button.h"
#include "gui_label.h"
#include "ifc/action_listener.h"

class einstellungen_t;
struct event_t;

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

    bool load_heightfield, loaded_heightfield;
    bool load;
    bool start;
    bool close;

	int old_lang;

	// since decrease/increase buttons always pair these ...
	button_t map_number[2];
	button_t x_size[2];
	button_t y_size[2];
	button_t water_level[2];
	button_t mountain_height[2];
	button_t mountain_roughness[2];
	button_t random_map, load_map;

	button_t number_of_towns[2];
	button_t town_size[2];
	button_t intercity_road_len[2];
	button_t traffic_desity[2];

	button_t other_industries[2];
	button_t town_industries[2];
	button_t tourist_attractions[2];

	button_t use_intro_dates;
	button_t intro_date[2];
	button_t allow_player_change;

	button_t load_game;
	button_t start_game;

	karte_t *welt;

	/**
	* Calculates preview from height map
	* @param filename name of heightfield file
	* @author Hajo/prissi
	*/
	bool update_from_heightfield(const char *filename);

	/**
	 * Berechnet Preview-Karte neu. Inititialisiert RNG neu!
	 * @author Hj. Malthaner
	 */
	void  update_preview();

public:
    welt_gui_t(karte_t *welt, einstellungen_t *sets);
    ~welt_gui_t() {}


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "new_world.txt";};

    bool gib_load_heightfield() const {return load_heightfield;}
    bool gib_load() const {return load;}
    bool gib_start() const {return start;}
    bool gib_close() const {return close;}
    bool gib_quit() const {return false;}

    einstellungen_t *gib_sets() const {return sets;};

    /**
     *
     * @author Hj. Malthaner
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    koord gib_fenstergroesse() const;

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *);

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
};

#endif
