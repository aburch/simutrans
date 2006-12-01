/*
 * citylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef CITYLIST_STATS_T_H
#define CITYLIST_STATS_T_H

#include "../ifc/gui_komponente.h"
#include "../simcity.h"


class karte_t;


namespace citylist {
	enum sort_mode_t { by_name=0, by_size, by_growth, SORT_MODES };
};


/** City list stats display */
class citylist_stats_t : public gui_komponente_t
{
	public:
		static char total_bev_string[128];

		citylist_stats_t(karte_t* welt, citylist::sort_mode_t sortby, bool sortreverse);

		void sort(citylist::sort_mode_t sortby, bool sortreverse);

		/** Events werden hiermit an die GUI-Komponenten gemeldet */
		void infowin_event(const event_t*);

		/** Zeichnet die Komponente */
		void zeichnen(koord offset);

	private:
		karte_t * welt;
		vector_tpl<stadt_t*> city_list;
};

#endif
