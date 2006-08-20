/*
 * tab_panel.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "../simdebug.h"
#include "tab_panel.h"
#include "../dataobj/translator.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simcolor.h"


tab_panel_t::tab_panel_t()
{
    active_tab = 0;
}


void tab_panel_t::add_tab(gui_komponente_t *c, const char *name)
{
    tabs.append(c);
    namen.append(name);

    c->setze_groesse(gib_groesse()-koord(0,HEADER_VSIZE));
}

gui_komponente_t *
tab_panel_t::gib_aktives_tab() const
{
    return tabs.at(active_tab);
}

void tab_panel_t::setze_groesse(koord groesse)
{
    gui_komponente_t::setze_groesse(groesse);

    this->groesse = groesse;

    slist_iterator_tpl<gui_komponente_t *> iter(tabs);
    while(iter.next()) {
	iter.get_current()->setze_groesse(gib_groesse()-koord(0,HEADER_VSIZE));
    }
}


void tab_panel_t::infowin_event(const event_t *ev)
{
//    printf("tab_panel_t::infowin_event()\n");


    if(ev->my >= HEADER_VSIZE || ev->cy >= HEADER_VSIZE) {
	// Komponente getroffen
	event_t ev2 = *ev;
	translate_event(&ev2, 0, -HEADER_VSIZE);
	tabs.at(active_tab)->infowin_event(&ev2);

    } else if(IS_LEFTCLICK(ev)) {
	if(ev->cy > 0 && ev->cy < HEADER_VSIZE-1) {
	    // Reiter getroffen

	    slist_iterator_tpl<const char*> iter (namen);

	    int text_x = 4;
	    int i = 0;

	    while(iter.next()) {
		const char *text = translator::translate(iter.get_current());
		const int width = proportional_string_width( text );

		if(text_x < ev->cx && text_x+width+8 > ev->cx) {
		    active_tab = i;
			call_listeners();
		    break;
		}

		text_x += width + 8;

		i++;
	    }
	}
    }
}



void tab_panel_t::zeichnen(koord parent_pos) const
{
    // Position am Bildschirm/Fenster
    const int xpos = parent_pos.x + pos.x;
    const int ypos = parent_pos.y + pos.y;

    int text_x = xpos+8;
    int i = 0;

    display_fillbox_wh(xpos, ypos+HEADER_VSIZE-1, 4, 1, WEISS, true);

    slist_iterator_tpl<const char*> iter (namen);

    while(iter.next()) {
	const char *text = translator::translate(iter.get_current());

	const int width = proportional_string_width( text );

	if(i != active_tab) {
	    display_fillbox_wh(text_x-4, ypos+HEADER_VSIZE-1, width+8, 1, MN_GREY4, true);
	    display_fillbox_wh(text_x-3, ypos+4, width+5, 1, MN_GREY4, true);

	    display_vline_wh(text_x-4, ypos+5, HEADER_VSIZE-6, MN_GREY4, true);
	    display_vline_wh(text_x+width+3, ypos+5, HEADER_VSIZE-6, MN_GREY0, true);

	    display_proportional(text_x, ypos+7, text, ALIGN_LEFT, SCHWARZ, true);
	} else {
	    display_fillbox_wh(text_x-3, ypos+3, width+5, 1, MN_GREY4, true);

	    display_vline_wh(text_x-4, ypos+4, 13, MN_GREY4, true);
	    display_vline_wh(text_x+width+3, ypos+4, 13, MN_GREY0, true);

	    display_proportional(text_x, ypos+7, text, ALIGN_LEFT, SCHWARZ, true);

	    tabs.at(i)->zeichnen(koord(xpos+0, ypos+HEADER_VSIZE));
	}

	text_x += width + 8;

	i++;
    }
    display_fillbox_wh(text_x-4, ypos+HEADER_VSIZE-1, groesse.x-(text_x-xpos)+4, 1, MN_GREY4, true);
}

/**
 * Inform all listeners that an action was triggered.
 * @author Hj. Malthaner
 */
void tab_panel_t::call_listeners()
{
    // printf("Message: button_t::call_listeners()\n");

    slist_iterator_tpl<action_listener_t *> iter (listeners);
    while(iter.next() && !iter.get_current()->action_triggered(this));
}