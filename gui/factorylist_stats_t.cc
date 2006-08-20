/*
 * factorylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "factorylist_stats_t.h"

#include "../simgraph.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simwin.h"
#include "../simworld.h"

#include "components/list_button.h"

#include "../bauer/warenbauer.h"
#include "../utils/cbuffer_t.h"

factorylist_stats_t::factorylist_stats_t(karte_t * w,
					 const factorylist::sort_mode_t& sortby,
					 const bool& sortreverse) :
    fab_list(0)
{
	welt = w;
	setze_groesse(koord(210, welt->gib_fab_list().count()*(LINESPACE+1)-10));
	sort(sortby,sortreverse);
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void factorylist_stats_t::infowin_event(const event_t * ev)
{
    const unsigned int line = (ev->cy) / (LINESPACE+1);
    if (line >= fab_list.get_count())
	return;

    fabrik_t * fab = fab_list.at(line);
    if (!fab)
	return;

    const koord3d pos = fab->gib_pos();

    if (IS_LEFTRELEASE(ev)) {
	grund_t *gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
	if (gr) {
	    gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
	    if (gb)
		create_win(320, 0,-1,new fabrik_info_t(fab,gb,welt),w_info,magic_none);
	}
    }
    else if (IS_RIGHTRELEASE(ev)) {
	if(welt->ist_in_kartengrenzen(pos.gib_2d()))
	    welt->setze_ij_off(pos.gib_2d() + koord(-5,-5));
    }
} // end of function factorylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void factorylist_stats_t::zeichnen(koord offset) const
{
    //DBG_DEBUG("factorylist_stats_t()","zeichnen()");
    const struct clip_dimension cd = display_gib_clip_wh();
    const int start = cd.y-LINESPACE-1;
    const int end = cd.yy+LINESPACE+1;

    static cbuffer_t buf(256);
    int xoff = offset.x;
    int yoff = offset.y;

    for (uint32 i=0; i<fab_list.get_count()  &&  yoff<end; i++) {

	// skip invisible lines
	if(yoff<start) {
	    yoff += LINESPACE+1;
	    continue;
	}

	const fabrik_t *fab = fab_list.at(i);
	if(fab) {

	    //DBG_DEBUG("factorylist_stats_t()","zeichnen() factory %i",i);
	    unsigned long input, output;
	    unsigned indikatorfarbe = fabrik_t::status_to_color[ fab->calc_factory_status( &input, &output ) ];

	    buf.clear();
	    //		buf.append(i+1);
	    //		buf.append(".) ");
	    buf.append(fab_list.at(i)->gib_name());
	    buf.append(" (");

	    if (fab->gib_eingang()->get_count() > 0) {
		buf.append(input);
	    }
	    else {
		buf.append("-");
	    }
	    buf.append(", ");

	    if (fab->gib_ausgang()->get_count() > 0) {
		buf.append(output);
	    }
	    else {
		buf.append("-");
	    }
	    buf.append(", ");

	    buf.append(fab->max_produktion());
	    buf.append(") ");


	    //display_ddd_box_clip(xoff+7, yoff+2, 8, 8, MN_GREY0, MN_GREY4);
	    display_fillbox_wh_clip(xoff+2, yoff+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indikatorfarbe, true);

	    if (fab->get_prodfaktor() > 16) {
		display_color_img(skinverwaltung_t::electricity->gib_bild_nr(0), xoff+4+INDICATOR_WIDTH, yoff, 0, false, false);
	    }

	    // show text
	    display_proportional_clip(xoff+INDICATOR_WIDTH+6+10,yoff,buf,ALIGN_LEFT,COL_BLACK,true);
	}
	yoff += LINESPACE+1;
    }
    //DBG_DEBUG("factorylist_stats_t()","zeichnen() ende");
}



void factorylist_stats_t::sort(const factorylist::sort_mode_t& sortby,const bool& sortreverse)
{
    slist_iterator_tpl<fabrik_t *> fab_iter (welt->gib_fab_list());

    fab_list.clear();
    fab_list.resize(welt->gib_fab_list().count());

    //DBG_DEBUG("factorylist_stats_t()","fabriken.count() == %d",fabriken->count());

    if(!fab_iter.next()) {
	return;
    }
    // ok, we have at least one factory
    fab_list.append(fab_iter.get_current());

    while(fab_iter.next()) {
	fabrik_t *fab = fab_iter.get_current();

	bool append = true;
	for (unsigned int j=0; j<fab_list.get_count(); j++) {
	    const fabrik_t *check_fab=fab_list.at(j);

	    unsigned long input, output, check_input, check_output;
	    switch (sortby) {
		case factorylist::by_name:
		    if (sortreverse)
			append = strcmp(fab->gib_name(),check_fab->gib_name())<0;
		    else
			append = strcmp(fab->gib_name(),check_fab->gib_name())>=0;
		    break;
		case factorylist::by_input:
		    fab->calc_factory_status( &input, &output );
		    check_fab->calc_factory_status( &check_input, &check_output );
		    if (sortreverse)
			append = (input<=check_input &&  check_fab->gib_eingang()->get_count()!=0);
		    else
			append = (input>=check_input);//  &&  check_fab->gib_eingang()->get_count()!=0);
		    break;
		case factorylist::by_output:
		    fab->calc_factory_status( &input, &output );
		    check_fab->calc_factory_status( &check_input, &check_output );
		    if (sortreverse)
			append = (output<=check_output  &&  check_fab->gib_ausgang()->get_count()!=0);
		    else
			append = (output>=check_output);//  &&  check_fab->gib_ausgang()->get_count()!=0);
		    break;
		case factorylist::by_maxprod:
		    if (sortreverse)
			append =  (fab->max_produktion() < check_fab->max_produktion());
		    else
			append =  (fab->max_produktion() >= check_fab->max_produktion());
		    break;
		case factorylist::by_status:
		    if (sortreverse)
			append = fab->calc_factory_status( NULL, NULL )<check_fab->calc_factory_status( NULL, NULL );
		    else
			append = fab->calc_factory_status( NULL, NULL )>=check_fab->calc_factory_status( NULL, NULL );
		    break;
		case factorylist::by_power:
		    if (sortreverse)
			append = (fab->get_prodfaktor() < check_fab->get_prodfaktor());
		    else
			append = (fab->get_prodfaktor() >= check_fab->get_prodfaktor());
	    }
	    if(!append) {
		fab_list.insert_at(j,fab);
		break;
	    }
	} // end of for (unsigned int j=0; j<fab_list.get_count(); j++)
	if(append) {
	    fab_list.append(fab);
	}

    } // end of iterator

} // end of function factorylist_stats_t::sort()
