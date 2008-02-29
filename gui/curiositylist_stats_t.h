/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef curiositylist_stats_t_h
#define curiositylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "../ifc/gui_komponente.h"
#include "components/gui_button.h"

class karte_t;
class gebaeude_t;

namespace curiositylist {
    enum sort_mode_t { by_name=0, by_paxlevel/*, by_maillevel*/, SORT_MODES };
};

/**
 * Curiosity list stats display
 * @author Hj. Malthaner
 */
class curiositylist_stats_t : public gui_komponente_t
{
private:
	karte_t * welt;
	vector_tpl<gebaeude_t*> attractions;
	uint32 line_selected;

public:
	curiositylist_stats_t(karte_t* welt, curiositylist::sort_mode_t sortby, bool sortreverse);

	void get_unique_attractions(curiositylist::sort_mode_t sortby, bool reverse);

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
