/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_convoiinfo_h
#define gui_convoiinfo_h

#include "../dataobj/koord.h"
#include "gui_container.h"
#include "components/gui_speedbar.h"
#include "../convoihandle_t.h"


/**
 * One element of the vehicle list display
 *
 * @autor Hj. Malthaner
 */
class gui_convoiinfo_t : public gui_komponente_t
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
	* @param cnv das Handle für den anzuzeigenden Convoi.
	* @author Hj. Malthaner
	*/
	gui_convoiinfo_t(convoihandle_t cnv, int n);

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	void infowin_event(const event_t *);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
