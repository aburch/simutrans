/*
 * citylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "citylist_stats_t.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../gui/stadt_info.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include <string.h>

static const char *total_bev_translation=NULL;
char citylist_stats_t::total_bev_string[128];

citylist_stats_t::citylist_stats_t(karte_t * w,const citylist::sort_mode_t& sortby,const bool& sortreverse) :
	city_list(0)
{
    welt = w;

    setze_groesse(koord(210, welt->gib_staedte()->get_count()*(LINESPACE+1)-10));
    total_bev_translation = translator::translate("Total inhabitants:");
    sort(sortby,sortreverse);
}



void citylist_stats_t::sort(const citylist::sort_mode_t& sortby,const bool& sortreverse)
{
    const weighted_vector_tpl<stadt_t *> *cities = welt->gib_staedte();

    city_list.clear();
    city_list.resize(cities->get_count());

    if (cities->get_count() == 0) {
	return;
	}

    city_list.append(cities->get(0),1);

    for(unsigned int i=1; i<cities->get_count(); i++) {
	stadt_t *city = cities->get(i);
	bool append = true;

	for (unsigned int j=0; j<city_list.get_count(); j++) {
	    const stadt_t *check_city = city_list.at(j);


	    switch (sortby) {
		case citylist::by_name:
		    if (sortreverse)
			append = strcmp(city->gib_name(),check_city->gib_name())<0;
		    else
			append = strcmp(city->gib_name(),check_city->gib_name())>=0;
		    break;
		case citylist::by_size:
		    if (sortreverse)
			append = (city->gib_einwohner()<=check_city->gib_einwohner());
		    else
			append = (city->gib_einwohner()>=check_city->gib_einwohner());
		    break;
		case citylist::by_growth:
		    if (sortreverse)
			append = (city->gib_wachstum()<=check_city->gib_wachstum());
		    else
			append = (city->gib_wachstum()>=check_city->gib_wachstum());
		    break;
	    }
	    if(!append) {
		city_list.insert_at(j,city,1);
		break;
	    }
	} // end of for (unsigned int j=0; j<city_list.get_count(); j++)
	if(append) {
	    city_list.append(city,1);
	}
    }
} // end of function citylist_stats_t::sort()



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void citylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = ev->cy / (LINESPACE+1);
	if(line>=city_list.get_count()) {
		return;
	}

	stadt_t *stadt = city_list.get(line);
	if(!stadt) {
		return;
	}

	if(IS_LEFTRELEASE(ev)  &&  ev->cy>0) {
		create_win(320, 0, -1, stadt->gib_stadt_info(), w_info, magic_none);/* with magic!=none only one dialog is allowed */
	}
	else if (IS_RIGHTRELEASE(ev) && ev->cy>0) {
		const koord pos = stadt->gib_pos();
		welt->setze_ij_off(koord3d(pos,welt->min_hgt(pos)));
	}
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void citylist_stats_t::zeichnen(koord offset)
{
	static cbuffer_t buf (256);
	sint32 total_bev = 0, total_growth=0;

	for(unsigned int i=0; i<city_list.get_count(); i++) {
		const stadt_t * stadt = city_list.get(i);
		buf.clear();
		stadt->get_short_info(buf);
		display_proportional_clip(offset.x+4, offset.y+i*(LINESPACE+1), buf, ALIGN_LEFT,COL_BLACK,true);

		total_bev += stadt->gib_einwohner();
		total_growth += stadt->gib_wachstum();
	}
	// some cities there?
	if(total_bev>0) {
		sprintf(total_bev_string,"%s %d (%+d)", total_bev_translation,total_bev,total_growth);
	}
	else {
		total_bev_string[0] = 0;
	}
}
