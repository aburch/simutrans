/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef message_stats_t_h
#define message_stats_t_h

#include "../ifc/gui_komponente.h"
#include "../simimg.h"

class karte_t;
class message_t;

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class message_stats_t : public gui_komponente_t
{
private:
	message_t *msg;
	karte_t *welt;
	unsigned last_count;
	sint32 message_selected;

public:
	message_stats_t(karte_t *welt);

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
