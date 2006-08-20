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
gui_combobox_t::gui_combobox_t()
{
	bt_prev.setze_typ(button_t::arrowleft);
	bt_prev.setze_pos( koord(0,2) );
	bt_prev.setze_groesse( koord(10,10) );

	bt_next.setze_typ(button_t::arrowright);
	bt_next.setze_groesse( koord(10,10) );

//	textinp.add_listener(this);

	first_call = true;
	finish = false;
	droplist = new gui_scrolled_list_t(gui_scrolled_list_t::select);
	droplist->set_visible(false);
	droplist->add_listener(this);
	setze_groesse(gib_groesse());
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
	release_focus(this);
	delete droplist;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_combobox_t::infowin_event(const event_t *ev)
{
	textinp.infowin_event(ev);

	if(!droplist->is_visible()) {

DBG_MESSAGE("event","%d,%d",ev->cx, ev->cy);
		if(bt_prev.getroffen(ev->cx, ev->cy)) {
DBG_MESSAGE("event","HOWDY!");
			bt_prev.pressed = IS_LEFT_BUTTON_PRESSED(ev);
			if(IS_LEFTRELEASE(ev)) {
				bt_prev.pressed = false;
				if(droplist->gib_selection()>=-1) {
					value_t p;
					p.i = droplist->gib_selection()-1;
					set_selection( p.i );
					call_listeners(p);
				}
			}
			return;
		}
		else if(bt_next.getroffen(ev->cx, ev->cy)) {
			bt_next.pressed = IS_LEFT_BUTTON_PRESSED(ev);
			if(IS_LEFTRELEASE(ev)) {
				bt_next.pressed = false;
				if(droplist->gib_selection()<droplist->get_count()-1) {
					value_t p;
					p.i = droplist->gib_selection()+1;
					set_selection( p.i );
					call_listeners(p);
				}
			}
			return;
		}
	}

	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev) || IS_LEFTRELEASE(ev)) {

		if(first_call) {
			// prepare for selection

			// swallow the first mouseclick
			if(IS_LEFTRELEASE(ev)) {
				first_call = false;
			}

			droplist->set_visible(true);

			droplist->setze_groesse(koord(this->groesse.x, max_size.y-16));
			droplist->setze_pos(koord(this->pos.x, this->pos.y+16));
			droplist->request_groesse(droplist->gib_groesse());
			setze_groesse( droplist->gib_groesse()+koord(0,16) );
			droplist->show_selection( droplist->gib_selection() );
		}
		else {
			if(droplist->is_visible()) {
				event_t ev2 = *ev;
				translate_event(&ev2, 0, -16);

				if (droplist->getroffen(ev->cx+pos.x, ev->cy+pos.y)) {
					droplist->infowin_event(&ev2);
					// we selected something?
					if(finish  && IS_LEFTRELEASE(ev)) {
						close_box();
					}
				}
				else {
					// acting on "release" is better than checking for "new selection"
					if (IS_LEFTRELEASE(ev)) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
						close_box();
					}
				}
			}
		}
	} else if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_CLOSE) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
		close_box();
	} else if (ev->ev_class == EVENT_KEYBOARD) {
		if(ev->ev_code==13) {
			//return key
			droplist->set_visible(false);
			close_box();
		}
	}
	// update "mouse-click-catch-area"
	droplist->is_visible() ? setze_groesse(koord(groesse.x, max_size.y)) : setze_groesse(koord(groesse.x, 14));
}



/* selction now handled via callback */
bool gui_combobox_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp==droplist) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","scroll selected %i",get_selection());
		finish = true;
		textinp.setze_text( (char *)droplist->get_element(droplist->gib_selection()),128);
	}
	return false;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_combobox_t::zeichnen(koord offset) const
{
	textinp.zeichnen(offset);

	if (droplist->is_visible()) {
		droplist->zeichnen(offset);
	}
	else {
		offset += pos;
		bt_prev.zeichnen(offset);
		bt_next.zeichnen(offset);
	}
}



/**
 * sets the selection
 * @author hsiegeln
 */
void
gui_combobox_t::set_selection(int s)
{
	if(droplist->is_visible()) {
		// visible? change also offset of scrollbar
		droplist->show_selection( s );
	}
	else {
		// just set it
		droplist->setze_selection( s );
	}
	textinp.setze_text( (char *)droplist->get_element(s),128);
}



/**
* Release the focus if we had it
*/
void
gui_combobox_t::close_box()
{
	if(finish) {
//DBG_MESSAGE("gui_combobox_t::infowin_event()","prepare selected %i for %d listerners",get_selection(),listeners.count());
		value_t p;
		p.i = droplist->gib_selection();
		call_listeners(p);
		finish = false;
	}
	release_focus(this);
	release_focus(&textinp);
	droplist->set_visible(false);
	setze_groesse(koord(groesse.x, 14));
	first_call = true;
};



/**
* Release the focus if we had it
*/
void gui_combobox_t::setze_groesse(koord gr)
{
	textinp.setze_pos( pos+koord(12,0) );
	textinp.setze_groesse( koord(gr.x-26,14) );
	bt_next.setze_pos( koord(gr.x-12,2) );
	groesse = gr;
}
