/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 * This class defines all toolbar dialoges, i.e. the part the user will see
 */

#ifndef gui_werkzeug_waehler_h
#define gui_werkzeug_waehler_h

#include "../ifc/gui_fenster.h"
#include "../tpl/vector_tpl.h"
//#include "../simmenu.h"

class karte_t;
class werkzeug_t;


class werkzeug_waehler_t : public gui_fenster_t
{
private:
	koord icon;	// size of symbols here

	karte_t *welt;

	/**
	* The tool definitions
	* @author Hj. Malthaner
	*/
	vector_tpl<werkzeug_t *> tools;

	/**
	 * window width in toolboxes
	 * @author Hj. Malthaner
	 */
	int tool_icon_width;

	koord groesse;

	/**
	 * Fenstertitel
	 * @author Hj. Malthaner
	 */
	const char *titel;

	/**
	 * Name of the help file
	 * @author Hj. Malthaner
	 */
	const char *hilfe_datei;

	// needs dirty redraw (only when changed)
	bool dirty;

	bool allow_break;

public:
	werkzeug_waehler_t(karte_t *welt, const char *titel, const char *helpfile, koord size, bool allow_break=true );

	/**
	 * Add a new tool with values and tooltip text.
	 * @author Hj. Malthaner
	 */
	void add_werkzeug(werkzeug_t *w);

	// purges toolbar
	void reset_tools();

	const char *get_name() const {return titel;}

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return hilfe_datei;}

	koord get_fenstergroesse() const { return groesse; }

	PLAYER_COLOR_VAL get_titelcolor() const { return WIN_TITEL; }

	bool has_sticky() const { return true; }

	/* returns true, if inside window area ...
	 * @author Hj. Malthaner
	 */
	bool getroffen(int x, int y);

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
