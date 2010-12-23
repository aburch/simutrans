/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef labellist_stats_t_h
#define labellist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "components/gui_komponente.h"
#include "components/gui_button.h"

class karte_t;

namespace labellist {
    enum sort_mode_t { by_name=0, by_koord, by_player, SORT_MODES };
};

/**
 * Curiosity list stats display
 * @author Hj. Malthaner
 */
class labellist_stats_t : public gui_komponente_t
{
private:
	karte_t * welt;
	vector_tpl<koord> labels;
	uint32 line_selected;

public:
	labellist_stats_t(karte_t* welt, labellist::sort_mode_t sortby, bool sortreverse, bool filter);

	void get_unique_labels(labellist::sort_mode_t sortby, bool reverse, bool filter);

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	bool infowin_event(const event_t *);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
