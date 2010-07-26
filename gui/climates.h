/*
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#ifndef climate_gui_h
#define climate_gui_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "components/action_listener.h"
#include "components/gui_textinput.h"

class einstellungen_t;


/**
 * set the climate borders
 * @author prissi
 */
class climate_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	einstellungen_t * sets;

	// since decrease/increase buttons always pair these ...
	gui_numberinput_t water_level, mountain_height, mountain_roughness;

	gui_numberinput_t snowline_winter;

	gui_numberinput_t climate_borders_ui[rocky_climate];

	button_t no_tree; // without tree

	gui_numberinput_t river_n, river_min, river_max;

	// Gives a hilly landscape
	// @author: jamespetts
	button_t hilly;

	// If this is selected, cities will not prefer
	// lower grounds as they do by default.
	// @ author: jamespetts
	button_t cities_ignore_height;

	gui_numberinput_t cities_like_water;
public:
	climate_gui_t(einstellungen_t* sets);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "climates.txt";}

	// does not work during new world dialoge
	virtual bool has_sticky() const { return false; }

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
