/*
 * gui_combobox.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "../../simdebug.h"
#include "gui_combobox.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"



/**
 * Konstruktor
 *
 * @author Hj. Malthaner
 */
gui_combobox_t::gui_combobox_t() : gui_textinput_t()
{
	max = 0;
	first_call = true;
	finish = false;
	droplist = new scrolled_list_gui_t(scrolled_list_gui_t::select);
	droplist->set_visible(false);
	droplist->add_listener(this);
	max_size = koord(0,100);
	set_highlight_color(0);
	set_read_only(false);
}



/**
 * Destruktor
 *
 * @author Hj. Malthaner
 */
gui_combobox_t::~gui_combobox_t()
{
    if(has_focus()) {
		release_focus();
    }
    delete droplist;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_combobox_t::infowin_event(const event_t *ev)
{
	gui_textinput_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev) || IS_LEFTRELEASE(ev)) {

		if(first_call) {
			// prepare for selection

			// swallow the first mouseclick
			if(IS_LEFTRELEASE(ev)) {
				first_call = false;
			}

			droplist->set_visible(true);

			if(!has_focus()) {
				request_focus();
				return;
			}
		}
		else {
			if(!has_focus()) {
				request_focus();
				return;
			}

			if(droplist->is_visible()) {
				event_t ev2 = *ev;
				translate_event(&ev2, 0, -16);

				if (droplist->getroffen(ev->cx+pos.x, ev->cy+pos.y)) {
					droplist->infowin_event(&ev2);
					// we selected something?
					if(finish  && IS_LEFTRELEASE(ev)) {
						release_focus();
					}
				}
				else {
					// acting on "release" is better than checking for "new selection"
					if (IS_LEFTRELEASE(ev)) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
						release_focus();
					}
				}
			}
		}
	} else if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_CLOSE  &&  has_focus()) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
		release_focus();
	} else if (ev->ev_class == EVENT_KEYBOARD) {
		if(ev->ev_code==13) {
			//return key
			droplist->set_visible(false);
			release_focus();
		}
	}
	// update "mouse-click-catch-area"
	droplist->is_visible() ? setze_groesse(koord(groesse.x, max_size.y)) : setze_groesse(koord(groesse.x, 14));
}



/* selction now handled via callback */
bool gui_combobox_t::action_triggered(gui_komponente_t *komp)
{
	if(komp==droplist) {
		finish = true;
	}
	return false;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_combobox_t::zeichnen(koord offset) const
{
	gui_textinput_t::zeichnen(offset);

	droplist->setze_groesse(koord(this->groesse.x, max_size.y-16));
	droplist->setze_pos(koord(this->pos.x, this->pos.y+16));
	droplist->request_groesse(droplist->gib_groesse());

	if (droplist->is_visible()) {
		droplist->zeichnen(offset);
	}
}



/**
* Release the focus if we had it
*/
bool gui_combobox_t::release_focus()
{
	if(finish) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","prepare selected %i",get_selection());
		call_listeners();
		finish = false;
	}
	gui_textinput_t::release_focus();
	droplist->set_visible(false);
	setze_groesse(koord(groesse.x, 14));
	first_call = true;
	return !has_focus();
};
