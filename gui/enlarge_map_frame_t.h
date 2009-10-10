/*
 * Dialogue to increase the map size.
 */

#ifndef bigger_map_gui_h
#define bigger_map_gui_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

class einstellungen_t;


class enlarge_map_frame_t  : public gui_frame_t, private action_listener_t
{
private:
	// local settings of the new world ...
	einstellungen_t * sets;

	enum { preview_size = 64 };

	/**
	* Mini Karten-Preview
	* @author Hj. Malthaner
	*/
	unsigned char karte[preview_size*preview_size];

	char map_number_s[16];

	bool changed_number_of_towns;
	int old_lang;

	gui_numberinput_t inp_x_size, inp_y_size, inp_number_of_towns, inp_town_size;

	button_t start_button;

	gui_label_t memory;// memory requirement
	char memory_str[256];

	karte_t *welt;

public:
	static inline koord koord_from_rotation( einstellungen_t *, sint16 y, sint16 x, sint16 w, sint16 h );

	enlarge_map_frame_t(spieler_t *spieler,karte_t *welt);

	/**
	* Berechnet Preview-Karte neu. Inititialisiert RNG neu!
	* public, because also the climate dialog need it
	* @author Hj. Malthaner
	*/
	void update_preview();

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);


	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "enlarge_map.txt";}

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
