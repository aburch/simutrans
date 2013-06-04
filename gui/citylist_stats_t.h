/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where the citylist status are calculated (for graphs and statistics)
 */

#ifndef CITYLIST_STATS_T_H
#define CITYLIST_STATS_T_H

#include "components/gui_komponente.h"
#include "../tpl/vector_tpl.h"


class karte_t;
class stadt_t;


namespace citylist {
	enum sort_mode_t { by_name=0, by_size, by_growth, SORT_MODES };
};


// City list stats display
class citylist_stats_t : public gui_komponente_t
{
private:
	karte_t *welt;
	vector_tpl<stadt_t*> city_list;
	uint32 line_selected;

	citylist::sort_mode_t sortby;
	bool sortreverse;

public:
	static char total_bev_string[128];

	citylist_stats_t(karte_t* welt, citylist::sort_mode_t sortby, bool sortreverse);

	void sort(citylist::sort_mode_t sortby, bool sortreverse);

	bool infowin_event(event_t const*) OVERRIDE;

	// Recalc the current size required to display everything, and set component size
	void recalc_size();

	// Draw the component
	void zeichnen(koord offset);
};

#endif
