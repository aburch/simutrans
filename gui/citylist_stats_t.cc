/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include "citylist_stats_t.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../gui/stadt_info.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include <string.h>

static const char* total_bev_translation = NULL;
char citylist_stats_t::total_bev_string[128];


citylist_stats_t::citylist_stats_t(karte_t* w, citylist::sort_mode_t sortby, bool sortreverse) :
	welt(w)
{
	setze_groesse(koord(210, welt->gib_staedte().get_count() * (LINESPACE + 1) - 10));
	total_bev_translation = translator::translate("Total inhabitants:");
	sort(sortby, sortreverse);
}


class compare_cities
{
	public:
		compare_cities(citylist::sort_mode_t sortby_, bool reverse_) :
			sortby(sortby_),
			reverse(reverse_)
		{}

		bool operator ()(const stadt_t* a, const stadt_t* b)
		{
			int cmp;
			switch (sortby) {
				default: NOT_REACHED
				case citylist::by_name:   cmp = strcmp(a->gib_name(), b->gib_name());    break;
				case citylist::by_size:   cmp = a->gib_einwohner() - b->gib_einwohner(); break;
				case citylist::by_growth: cmp = a->gib_wachstum()  - b->gib_wachstum();  break;
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		citylist::sort_mode_t sortby;
		bool reverse;
};


void citylist_stats_t::sort(citylist::sort_mode_t sortby, bool sortreverse)
{
	const weighted_vector_tpl<stadt_t*>& cities = welt->gib_staedte();

	city_list.clear();
	city_list.resize(cities.get_count());

	for (weighted_vector_tpl<stadt_t*>::const_iterator i = cities.begin(), end = cities.end(); i != end; ++i) {
		city_list.push_back(*i);
	}
	std::sort(city_list.begin(), city_list.end(), compare_cities(sortby, sortreverse));
}


void citylist_stats_t::infowin_event(const event_t * ev)
{
	const uint line = ev->cy / (LINESPACE + 1);
	if (line >= city_list.get_count()) return;

	stadt_t* stadt = city_list[line];

	if (IS_LEFTRELEASE(ev) && ev->cy > 0) {
		create_win(320, 0, -1, stadt->gib_stadt_info(), w_info, magic_none); // with magic!=none only one dialog is allowed
	} else if (IS_RIGHTRELEASE(ev) && ev->cy > 0) {
		const koord pos = stadt->gib_pos();
		welt->setze_ij_off(koord3d(pos, welt->min_hgt(pos)));
	}
}


void citylist_stats_t::zeichnen(koord offset)
{
	static cbuffer_t buf(256);
	sint32 total_bev = 0;
	sint32 total_growth = 0;

	for (uint i = 0; i < city_list.get_count(); i++) {
		const stadt_t* stadt = city_list[i];
		buf.clear();
		buf.append(stadt->gib_name());
		buf.append(": ");
		buf.append(stadt->gib_einwohner());
		buf.append(" (+");
		buf.append(stadt->gib_wachstum());
		buf.append(")");

		display_proportional_clip(offset.x + 4, offset.y + i * (LINESPACE + 1), buf, ALIGN_LEFT, COL_BLACK, true);

		total_bev    += stadt->gib_einwohner();
		total_growth += stadt->gib_wachstum();
	}
	// some cities there?
	if (total_bev > 0) {
		sprintf(total_bev_string,"%s %d (%+d)", total_bev_translation, total_bev, total_growth);
	} else {
		total_bev_string[0] = 0;
	}
}
