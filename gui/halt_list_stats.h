/*
 * halt_list_stats.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef halt_list_stats_h
#define halt_list_stats_h

#include "../dataobj/koord.h"
#include "../ifc/gui_komponente.h"
#include "../simhalt.h"
#include "../halthandle_t.h"


/**
 * Komponenten von Fenstern sollten von dieser Klassse abgeleitet werden.
 *
 * @autor Hj. Malthaner
 */
class halt_list_stats_t : public gui_komponente_t
{
private:
	/**
	 * Handle des anzuzeigenden Convois.
	 * @author Hj. Malthaner
	 */
	halthandle_t halt;

	/**
	 * Nummer des anzuzeigenden Convois.
	 * @author Hj. Malthaner
	 */
	int nummer;

public:
	/**
	 * Konstruktor.
	 * @param cnv das Handle für den anzuzeigenden Convoi.
	 * @author Hj. Malthaner
	 */
	halt_list_stats_t(halthandle_t halt, int n) { this->halt = halt; nummer = n; }

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
