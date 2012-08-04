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

class settings_t;


/**
 * set the climate borders
 * @author prissi
 */
class climate_gui_t  : public gui_frame_t, private action_listener_t
{
private:
	settings_t* sets;

	// since decrease/increase buttons always pair these ...
	gui_numberinput_t water_level, mountain_height, mountain_roughness;

	gui_numberinput_t snowline_winter;

	gui_numberinput_t climate_borders_ui[rocky_climate];

	button_t no_tree; // without tree

	gui_numberinput_t river_n, river_min, river_max;

public:
	climate_gui_t(settings_t*);

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

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// called when river number changed due to increasing map size
	void update_river_number( sint16 new_river_number );
};

#endif
