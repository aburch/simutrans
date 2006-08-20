/*
 * halt_list_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 by Markus Weber
 * filtering added by Volker Meyer
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "gui_container.h"
#include "halt_list_item.h"
#include "halt_list_frame.h"
#include "halt_list_filter_frame.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simwin.h"
#include "../simcolor.h"
#include "../dataobj/translator.h"
#include "../simplay.h"

#include "../bauer/warenbauer.h"

#include "components/list_button.h"

/**
 * All filter and sort settings are static, so the old settings are
 * used when the window is reopened.
 */

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
halt_list_frame_t::sort_mode_t halt_list_frame_t::sortby = nach_name;

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool halt_list_frame_t::sortreverse = false;

/**
 * Default filter: keine Ölbohrinseln!
 */
int halt_list_frame_t::filter_flags = any_filter|typ_filter|frachthof_filter|bushalt_filter|bahnhof_filter;

char halt_list_frame_t::name_filter_value[64] = "";

slist_tpl<const ware_besch_t *> halt_list_frame_t::waren_filter_ab;
slist_tpl<const ware_besch_t *> halt_list_frame_t::waren_filter_an;

const char *halt_list_frame_t::sort_text[SORT_MODES] = {
    "gl_btn_unsort",
    "hl_btn_sort_name",
    "hl_btn_sort_waiting",
    "hl_btn_sort_type"
};


/**
* This function is called by the quick-sort function which is used
* to sort the station list. This function compares two stations
*
* @param *p1 and *p2; see manpage of qsort
* @return  1 if station p1 > p2
*         -1 if station p1 < p2
*          0 if station p1 = p2
* @author Markus Weber
*/
int halt_list_frame_t::compare_halts(const void *p1, const void *p2)
{
    halthandle_t halt1 =*(const halthandle_t *)p1;
    halthandle_t halt2 =*(const halthandle_t *)p2;
    int order;

    /***********************************
    * Compare station 1 and station 2
    ***********************************/
    switch (sortby) {
    case 0: // sort by station number
	order = halt1->index_of() - halt2->index_of();
	break;
    default:
    case 1: // sort by station name
	order = 0;
	break;
    case 2: // sort by waiting goods
	order = halt1->sum_all_waiting_goods() - halt2->sum_all_waiting_goods();
	break;
    case 3: // sort by station type
	order = halt1->get_station_type() - halt2->get_station_type();
	break;
    }
    /**
     * use name as an additional sort, to make sort more stable.
     */
    if(order == 0) {
	order = strcmp(halt1->gib_name(), halt2->gib_name());
    }
   /***********************************
    * Beruecksichtige die
    * Sortierreihenfolge
    ***********************************/
    return sortreverse ? -order : order;
}



bool halt_list_frame_t::passes_filter(halthandle_t halt)
{
    bool ok;
    uint32 i, j;

    if(!gib_filter(any_filter)) {
	return true;
    }
    if(gib_filter(name_filter) && !strstr(halt->gib_name(), name_filter_value)) {
	return false;
    }
    if(gib_filter(typ_filter)) {
	int t = halt->get_station_type() ;

	if(!(gib_filter(frachthof_filter) && (t & haltestelle_t::loadingbay)) &&
	   !(gib_filter(bahnhof_filter) && (t & haltestelle_t::railstation)) &&
	   !(gib_filter(bushalt_filter) && (t & haltestelle_t::busstop)) &&
	   !(gib_filter(airport_filter) && (t & haltestelle_t::airstop)) &&
	   !(gib_filter(dock_filter) && (t & haltestelle_t::dock)))
	{
	    return false;
	}
    }
    if(gib_filter(spezial_filter)) {
	ok = false;

	if(gib_filter(ueberfuellt_filter)) {
		const int farbe=halt->gib_status_farbe();
		ok = (farbe==COL_RED  &&  farbe==COL_ORANGE);
	}
	if(!ok && gib_filter(ohneverb_filter)) {
	    const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele();
	    ok = (ziele->count() == 0); //only display stations with NO connection
	}
	if(!ok) {
	    return false;
	}
    }
    if(gib_filter(ware_ab_filter)) {
	/*
	 * Die Unterkriterien werden gebildet aus:
	 * - die Ware wird produziert (pax/post_enabled bzw. fabrik vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].is_empty())
	 */
	ok = false;

	// const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele();
	// Hajo: todo: check if there is a destination for the good (?)

	for(i = 0; !ok && i < warenbauer_t::gib_waren_anzahl(); i++) {
	    const ware_besch_t *ware = warenbauer_t::gib_info(i);

	    if(gib_ware_filter_ab(ware)) {
		if(ware == warenbauer_t::passagiere) {
		    ok = halt->get_pax_enabled();
		}
		else if(ware == warenbauer_t::post) {
		    ok = halt->get_post_enabled();
		}
		else if(ware != warenbauer_t::nichts) {

		  // Oh Mann - eine doppelte Schleife und das noch pro Haltestelle
		  // Zum Glück ist die Anzahl der Fabriken und die ihrer Ausgänge
		  // begrenzt (Normal 1-2 Fabriken mit je 0-1 Ausgang) -  V. Meyer
		    slist_iterator_tpl<fabrik_t *> fab_iter(halt->gib_fab_list());
		    while(!ok && fab_iter.next()) {
			const vector_tpl<ware_t> *ausgang = fab_iter.get_current()->gib_ausgang();
			for(j = 0; !ok && j < ausgang->get_count(); j++) {
			    ok = ausgang->get(j).gib_typ() == ware;
			}
		    }
		}
	    }
	}

	if(!ok) {
	    return false;
	}
    }
    if(gib_filter(ware_an_filter)) {
	/*
	 * Die Unterkriterien werden gebildet aus:
	 * - die Ware wird verbraucht (pax/post_enabled bzw. fabrik vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].is_empty())
	 */

	ok = false;
	// const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele();
	// Hajo: todo: check if there is a destination for the good (?)

	for(i = 0; !ok && i < warenbauer_t::gib_waren_anzahl(); i++) {
	    const ware_besch_t *ware = warenbauer_t::gib_info(i);

	    if(gib_ware_filter_an(ware)) {
		if(ware == warenbauer_t::passagiere) {
		    ok = halt->get_pax_enabled();
		}
		else if(ware == warenbauer_t::post) {
		    ok = halt->get_post_enabled();
		}
		else if(ware != warenbauer_t::nichts) {

		     // Oh Mann - eine doppelte Schleife und das noch pro Haltestelle
    		     // Zum Glück ist die Anzahl der Fabriken und die ihrer Ausgänge
		     // begrenzt (Normal 1-2 Fabriken mit je 0-1 Ausgang) -  V. Meyer

		    slist_iterator_tpl<fabrik_t *> fab_iter(halt->gib_fab_list());
		    while(!ok && fab_iter.next()) {
			const vector_tpl<ware_t> * eingang = fab_iter.get_current()->gib_eingang();
			for(j = 0; !ok && j < eingang->get_count(); j++) {
			    ok = eingang->get(j).gib_typ() == ware;
			}
		    }
		}
	    }
	}

	if(!ok) {
	    return false;
	}
    }
    return true;
}

/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
 * @author Markus Weber
 */
halt_list_frame_t::halt_list_frame_t(spieler_t *sp) :
    gui_frame_t("hl_title", sp),
    scrolly(&cont),
    sort_label(translator::translate("hl_txt_sort")),
    filter_label(translator::translate("hl_txt_filter"))
{
    m_sp = sp;
    filter_frame = NULL;

    sort_label.setze_pos(koord(BUTTON1_X, 4));
    add_komponente(&sort_label);
    sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
    sortedby.add_listener(this);
    add_komponente(&sortedby);

    sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
    sorteddir.add_listener(this);
    add_komponente(&sorteddir);

    filter_label.setze_pos(koord(BUTTON3_X, 4));
    add_komponente(&filter_label);

    filter_on.init(button_t::roundbox, translator::translate(gib_filter(any_filter) ? "hl_btn_filter_enable" : "hl_btn_filter_disable"), koord(BUTTON3_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
    filter_on.add_listener(this);
    add_komponente(&filter_on);

    filter_details.init(button_t::roundbox, translator::translate("hl_btn_filter_settings"), koord(BUTTON4_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
    filter_details.add_listener(this);
    add_komponente(&filter_details);

    scrolly.setze_pos(koord(1, 30));
    setze_opaque(true);
    setze_fenstergroesse(koord(BUTTON4_X+BUTTON_WIDTH+2, 191+16+16));

	// use gui-resize
	set_min_windowsize(koord(BUTTON4_X+BUTTON_WIDTH+2,191+16+16));
	set_resizemode(diagonal_resize);

    display_list();
    add_komponente(&scrolly);

    resize (koord(0,0));
}


/**
 * Destruktor.
 * @author Markus Weber
 */
halt_list_frame_t::~halt_list_frame_t()
{
}


/**
* This function refreshs the station-list
* @author Markus Weber/Volker Meyer
*/
void halt_list_frame_t::display_list(void)
{
    slist_iterator_tpl<halthandle_t > halt_iter (haltestelle_t::gib_alle_haltestellen());	// iteration with haltestellen (stations)
    const int count = haltestelle_t::gib_alle_haltestellen().count();				// count of stations
    int ypos = 0;

#ifdef _MSC_VER
    halthandle_t *a = new halthandle_t[count];
#else
    // V.Meyer: Sorry this is not standard C/C++ - too heavy for VC
    halthandle_t a[count]; // contains a sorted station list later
#endif
    int n = 0; // temporary variable
    int i;


    /**************************
     * Sort the station list
     *************************/

    // create a unsorted station list
    while(halt_iter.next()) {
	halthandle_t halt = halt_iter.get_current();
	if(halt->gib_besitzer() == m_sp && passes_filter(halt)) {
    	    a[n++] = halt;

	}
    }
    // sort the station list
    qsort((void *)a, n, sizeof (halthandle_t ), compare_halts);

    sortedby.setze_text(translator::translate(sort_text[gib_sortierung()]));
    sorteddir.setze_text(translator::translate(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc"));

    /****************************
     * Display the station list
     ***************************/

    cont.remove_all();   // clear the list

    // display stations
    for (i = 0; i < n; i++) {
	halt_list_item_t *cinfo = new halt_list_item_t(a[i], a[i]->index_of()+1);

	cinfo->setze_pos(koord(0, ypos));
	cinfo->setze_groesse(koord(500, 28));
	cont.add_komponente(cinfo);

        ypos += 28;
    }
    cont.setze_groesse(koord(500, ypos));
#ifdef _MSC_VER
    delete [] a;
#endif
}



void halt_list_frame_t::infowin_event(const event_t *ev)
{
    if(ev->ev_class == INFOWIN &&
       ev->ev_code == WIN_CLOSE) {
	if(filter_frame) {
	    filter_frame->infowin_event(ev);
	}
    }
    gui_frame_t::infowin_event(ev);
}

/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool halt_list_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
    if(komp == &filter_on) {
		setze_filter(any_filter, !gib_filter(any_filter));
		filter_on.setze_text(translator::translate(gib_filter(any_filter) ? "hl_btn_filter_enable" : "hl_btn_filter_disable"));
		display_list();
    }
	else if(komp == &sortedby) {
    	setze_sortierung((sort_mode_t)((gib_sortierung() + 1) % SORT_MODES));
		display_list();
    }
	else if(komp == &sorteddir) {
    	setze_reverse(!gib_reverse());
		display_list();
    }
	else if(komp == &filter_details) {
		if (filter_frame) {
	    	destroy_win(filter_frame);
		}
		else {
	    	filter_frame = new halt_list_filter_frame_t(m_sp, this);
	    	create_win(filter_frame, w_autodelete, -1);
		}
    }
    return true;
}


/**
 * Resize the window
 * @author Markus Weber
 */
void halt_list_frame_t::resize(const koord size_change)
{
	gui_frame_t::resize(size_change);
	koord groesse = gib_fenstergroesse()-koord(0,47);
	scrolly.setze_groesse(groesse);
}

void halt_list_frame_t::zeichnen(koord pos, koord gr)
{
    filter_details.pressed = filter_frame != NULL;
    gui_frame_t::zeichnen(pos, gr);
}

void halt_list_frame_t::setze_ware_filter_ab(const ware_besch_t *ware, int mode)
{
    if(ware != warenbauer_t::nichts) {
	if(gib_ware_filter_ab(ware)) {
	    if(mode != 1) {
		waren_filter_ab.remove(ware);
	    }
	} else {
	    if(mode != 0) {
		waren_filter_ab.append(ware);
	    }
	}
    }
}

void halt_list_frame_t::setze_ware_filter_an(const ware_besch_t *ware, int mode)
{
    if(ware != warenbauer_t::nichts) {
	if(gib_ware_filter_an(ware)) {
	    if(mode != 1) {
		waren_filter_an.remove(ware);
	    }
	} else {
	    if(mode != 0) {
		waren_filter_an.append(ware);
	    }
	}
    }
}

void halt_list_frame_t::setze_alle_ware_filter_ab(int mode)
{
    if(mode == 0) {
	waren_filter_ab.clear();
    }
    else {
	for(unsigned int i = 0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	    setze_ware_filter_ab(warenbauer_t::gib_info(i), mode);
	}
    }
}


void halt_list_frame_t::setze_alle_ware_filter_an(int mode)
{
    if(mode == 0) {
	waren_filter_an.clear();
    }
    else {
	for(unsigned int i = 0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	    setze_ware_filter_an(warenbauer_t::gib_info(i), mode);
	}
    }
}
