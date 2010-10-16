/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_halt_info_h
#define gui_halt_info_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_button.h"
#include "components/location_view_t.h"
#include "components/action_listener.h"
#include "components/gui_chart.h"

#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"
#include "../simwin.h"


/**
 * Dies stellt ein Fenster mit den Zielinformationen
 * fuer eine Haltestelle dar.
 *
 * @author Hj. Malthaner
 */

class halt_info_t : public gui_frame_t, private action_listener_t
{
private:
	static karte_t *welt;
	gui_scrollpane_t scrolly;
	gui_textarea_t text;
	gui_textinput_t input;
	gui_chart_t chart;
	gui_label_t sort_label;
	location_view_t view;
	button_t button;
	button_t sort_button;     // @author hsiegeln
	button_t filterButtons[7];
	button_t toggler;

	halthandle_t halt;
	char edit_name[256];

	/**
	* Buffer for freight info text string.
	* @author Hj. Malthaner
	*/
	cbuffer_t freight_info;
	cbuffer_t info_buf;

	void show_hide_statistics( bool show );

public:
	halt_info_t(karte_t *welt, halthandle_t halt);

	virtual ~halt_info_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "station.txt";}

	/**
	 * Komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 */
	void resize(const koord delta);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	void map_rotate90( sint16 );

	// this contructor is only used during loading
	halt_info_t(karte_t *welt);

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_halt_info; }
};

#endif
