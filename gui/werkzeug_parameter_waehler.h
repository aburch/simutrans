/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_werkzeug_parameter_waehler_h
#define gui_werkzeug_parameter_waehler_h

#include "../ifc/gui_fenster.h"
#include "../tpl/vector_tpl.h"
#include "../utils/cstring_t.h"

class spieler_t;
class karte_t;


/**
 * Eine Klasse, die ein Fenster zur Auswahl von bis zu acht
 * Parametern für ein Werkzeug per Icon darstellt.
 *
 * @author Hj. Malthaner
 */
class werkzeug_parameter_waehler_t : public gui_fenster_t
{
private:

	struct werkzeug_t {
		/**
		* Does this tool have an parameter or not?
		* @author Hj. Malthaner
		*/
		bool has_param;

		int  (* wzwp)(spieler_t *, karte_t *, koord pos, value_t param);
		int  (* wzwop)(spieler_t *, karte_t *, koord pos);

		value_t param;
		int  versatz;

		int  sound_ok;
		int  sound_ko;

		int  icon;
		int  cursor;

		cstring_t tooltip;
	};


	/**
	* The tools will be applied to this map
	* @author Hj. Malthaner
	*/
	karte_t *welt;


	/**
	* The tool definitions
	* @author Hj. Malthaner
	*/
	vector_tpl<werkzeug_t> tools;

	/**
	* window width in toolboxes
	* @author Hj. Malthaner
	*/
	int tool_icon_width;

	/**
	* Fenstertitel
	* @author Hj. Malthaner
	*/
	const char * titel;


	/**
	* Name of the help file
	* @author Hj. Malthaner
	*/
	const char * hilfe_datei;

	// needs dirty redraw (only when changed)
	bool dirty;

public:
    werkzeug_parameter_waehler_t(karte_t *welt, const char * titel);

    /**
     * Add a new tool with values and tooltip text.
     * Text must already be translated.
     * @author Hj. Malthaner
     */
    void add_tool(int (* wz1)(spieler_t *, karte_t *, koord),
		int versatz,
		int sound_ok,
		int sound_ko,
		int icon,
		int cursor,
		cstring_t tooltip);

    /**
     * Add a new tool with parameter values and tooltip text.
     * Text must already be translated.
     * @author Hj. Malthaner
     */
    void add_param_tool(int (* wz1)(spieler_t *, karte_t *, koord, value_t),
		value_t param,
		int versatz,
		int sound_ok,
		int sound_ko,
		int icon,
		int cursor,
		cstring_t tooltip);

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char * gib_hilfe_datei() const {return hilfe_datei;}

	void setze_hilfe_datei(const char *datei) {hilfe_datei = datei;}

	/**
	* manche Fenster kanns nur einmal geben, dazu die magic number
	* @return -1 wenn fenster schon vorhanden, sonst >= 0
	* @author Hj. Malthaner
	*/
	int zeige_info(int magic);

	/**
	* Fenstertitel
	* @author Hj. Malthaner
	*/
	const char *gib_name() const;

	/**
	* @return gibt wunschgroesse für das beobachtungsfenster zurueck
	*/
	koord gib_fenstergroesse() const;

	/**
	* gibt farbinformationen fuer Fenstertitel, -ränder und -körper
	* zurück
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL get_titelcolor() const;

	/* returns true, if inside window area ...
	* @author Hj. Malthaner
	*/
	bool getroffen(int x, int y);

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
