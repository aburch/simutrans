/*
 * banner.h
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef banner_h
#define banner_h

#include "../ifc/gui_fenster.h"

class karte_t;
struct event_t;
class skin_besch_t;



/**
 * Eine Klasse, die ein Fenster zur Auswahl von bis zu acht
 * Parametern für ein Werkzeug per Icon darstellt.
 *
 * @author Hj. Malthaner
 */
class banner_t : public gui_fenster_t
{
private:
	/**
	* The tools will be applied to this map
	* @author Hj. Malthaner
	*/
	karte_t *welt;

	int last_ms;
	int line;
	int xoff, yoff;

public:
    banner_t(karte_t *welt);

    ~banner_t() {}

	/**
	* Fenstertitel
	* @author Hj. Malthaner
	*/
	const char *gib_name() const {return ""; }

	/**
	* gibt farbinformationen fuer Fenstertitel, -ränder und -körper
	* zurück
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL get_titelcolor() const {return WIN_TITEL; }

	/**
	* @return gibt wunschgroesse für das beobachtungsfenster zurueck
	*/
	koord gib_fenstergroesse() const;

	/* returns true, if inside window area ...
	* @author Hj. Malthaner
	*/
	bool getroffen(int x, int y) { return true; }

	/* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	void infowin_event(const event_t *ev);

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);
};

#endif
