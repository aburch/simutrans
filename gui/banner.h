/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef banner_h
#define banner_h

#include "../ifc/gui_fenster.h"


/**
 * Eine Klasse, die ein Fenster zur Auswahl von bis zu acht
 * Parametern für ein Werkzeug per Icon darstellt.
 *
 * @author Hj. Malthaner
 */
class banner_t : public gui_fenster_t
{
private:
	sint32 last_ms;
	int line;
	sint16 xoff, yoff;

public:
		banner_t();

	/**
	* Fenstertitel
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return ""; }

	/**
	* gibt farbinformationen fuer Fenstertitel, -ränder und -körper
	* zurück
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL get_titelcolor() const {return WIN_TITEL; }

	/**
	* @return gibt wunschgroesse für das beobachtungsfenster zurueck
	*/
	koord get_fenstergroesse() const { return koord(display_get_width(),display_get_height()+48); }

	/* returns true, if inside window area ...
	* @author Hj. Malthaner
	*/
	bool getroffen(int , int ) { return true; }

	/* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	bool infowin_event(const event_t *ev);

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);
};

#endif
