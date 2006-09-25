/*
 * gui_convoiinfo.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_convoiinfo_h
#define gui_convoiinfo_h

#include "../dataobj/koord.h"
#include "gui_container.h"
#include "components/gui_speedbar.h"
#include "../simconvoi.h"
#include "../convoihandle_t.h"


struct event_t;


/**
 * One element of the vehicle list display
 *
 * @autor Hj. Malthaner
 */
class gui_convoiinfo_t : public gui_container_t
{
private:
	/**
	* Handle des anzuzeigenden Convois.
	* @author Hj. Malthaner
	*/
	convoihandle_t cnv;

	/**
	* Nummer des anzuzeigenden Convois.
	* @author Hj. Malthaner
	*/
	int nummer;

	gui_speedbar_t filled_bar;

public:
	/**
	* Konstruktor.
	* @param cnv das Handle für den anzuzeigenden Convoi.
	* @author Hj. Malthaner
	*/
	gui_convoiinfo_t(convoihandle_t cnv, int n);

	/**
	* Virtueller Destruktor, damit Klassen sauber abgeleitet werden können
	* @author Hj. Malthaner
	*/
	virtual ~gui_convoiinfo_t() {}

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	virtual void infowin_event(const event_t *);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
