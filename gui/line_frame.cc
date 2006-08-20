/*
 * schedule_list.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * filtering added by Volker Meyer
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_convoiinfo.h"
#include "../simvehikel.h"
#include "../simconvoi.h"
#include "../simplay.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simdepot.h"
#include "../tpl/slist_tpl.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

#include "schedule_list.h"

/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
 * @author Hj. Malthaner
 */
line_frame_t::line_frame_t(spieler_t *sp, karte_t *welt) :
	gui_frame_t("Lines", sp->kennfarbe),
	scrolly(&cont)
{
    owner = sp;
    this->welt = welt;

    //Resize button
    vresize.setze_pos(koord(1,200));
    vresize.setze_groesse(koord(318, 6));
    vresize.set_type(gui_resizer_t::vertical_resize);

    scrolly.setze_pos(koord(1, 30));
    setze_opaque(true);
    setze_fenstergroesse(koord(320, 191+16+16));

    display_list();
    add_komponente(&scrolly);

    add_komponente(&vresize);
    vresize.add_listener(this);

    resize (koord(0,0));
}


/**
 * Destruktor.
 * @author Hj. Malthaner
 */
line_frame_t::~line_frame_t()
{
}

void line_frame_t::infowin_event(const event_t *ev)
{
    gui_frame_t::infowin_event(ev);
}

/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool line_frame_t::action_triggered(gui_komponente_t *komp)           // 28-Dec-01    Markus Weber    Added
{
    if(komp == &vresize) {
	resize (koord(vresize.get_hresize(), vresize.get_vresize()));
    }
    return true;
}


void line_frame_t::resize(koord size_change)                          // 28-Dec-01    Markus Weber    Added
{

    koord new_windowsize = gib_fenstergroesse() + size_change;
    int vresize_top = vresize.gib_pos().y + size_change.y;


    if (new_windowsize.y < 100 )
    {
        new_windowsize.y = 100;
        vresize_top=100-23 ;
        vresize.cancelresize();
    }

    setze_fenstergroesse (new_windowsize);

    scrolly.setze_groesse(koord(318, gib_fenstergroesse().y - 1 - 16 - 16 - 20));
    vresize.setze_pos(koord(1,  vresize_top));
}



void line_frame_t::zeichnen(koord pos, koord gr)
{
    gui_frame_t::zeichnen(pos, gr);
}

void line_frame_t::display_list()
{
    int ypos = 0;
    int i;

    slist_iterator_tpl<simline_t *> iter (simlinemgmt_t::all_managed_lines);

    const int count = welt->simlinemgmt->count_lines();
    for (i = 0; i < count; i++) {
    	ypos += 40;
    }
    cont.setze_groesse(koord(500, ypos));
    scrolly.setze_groesse(koord(318, gib_fenstergroesse().y - 1 - 16 - 16 - 20));
}
