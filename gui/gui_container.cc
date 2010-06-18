/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include "gui_container.h"
#include "../simwin.h"

gui_container_t::gui_container_t() : gui_komponente_t(), komp_focus(NULL)
{
	list_dirty = false;
}


/**
 * Fügt eine Komponente zum Container hinzu.
 * @author Hj. Malthaner
 */
void gui_container_t::add_komponente(gui_komponente_t *komp)
{
	/* insert builds the diologe from bottom to top:
	 * Essential for comobo-boxes, so they overlap lower elements
	 */
	komponenten.insert(komp);
	list_dirty = true;
}


/**
 * Entfernt eine Komponente aus dem Container.
 * @author Hj. Malthaner
 */
void gui_container_t::remove_komponente(gui_komponente_t *komp)
{
	/* since we can remove a subcomponent,
	 * that actually contains the element with focus
	 */
	if(  komp_focus == komp->get_focus()  ) {
		komp_focus = NULL;
	}
	komponenten.remove(komp);
	list_dirty = true;
}


/**
 * Entfernt alle Komponente aus dem Container.
 * @author Markus Weber
 */
void gui_container_t::remove_all()
{
	// clear also focus
	while(  !komponenten.empty()  ) {
		remove_komponente( komponenten.remove_first() );
	}
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_container_t::infowin_event(const event_t *ev)
{
	bool swallowed = false;
	gui_komponente_t *new_focus = komp_focus;

	// need to change focus?
	if(  ev->ev_class==EVENT_KEYBOARD  ) {

		if(  komp_focus  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -komp_focus->get_pos().x, -komp_focus->get_pos().y);
			swallowed = komp_focus->infowin_event(&ev2);
		}
		if(  !swallowed  ) {
			if(  ev->ev_code==9  ) {
				// TAB: find new focus
				slist_iterator_tpl<gui_komponente_t *> iter (komponenten);
				new_focus = NULL;
				if(  (ev->ev_key_mod&1)==0  ) {
					// find next textinput field
					while(  iter.next()  &&  (komp_focus==NULL  ||  iter.get_current()->get_focus()!=komp_focus)  ) {
						if(  iter.get_current()->get_focus()  ) {
							new_focus = iter.get_current();
						}
					}
				}
				else {
					// or previous input field
					bool valid = komp_focus==NULL;
					while(  iter.next()  ) {
						if(  valid  &&  iter.get_current()->get_focus()  ) {
							new_focus = iter.get_current();
							break;
						}
						if(  iter.get_current()->get_focus()==komp_focus  ) {
							valid = true;
						}
					}
				}
				swallowed = komp_focus!=new_focus;
			}
			else if(  ev->ev_code==13  ||  ev->ev_code==27  ) {
				new_focus = NULL;
				if(  ev->ev_code==27  ) {
					// no untop message even!
					komp_focus = NULL;
				}
				swallowed = komp_focus!=new_focus;
			}
		}
	}
	else {
		// CASE : not a keyboard event
		const int x = ev->ev_class==EVENT_MOVE ? ev->mx : ev->cx;
		const int y = ev->ev_class==EVENT_MOVE ? ev->my : ev->cy;

		slist_iterator_tpl<gui_komponente_t *> iter (komponenten);
		slist_tpl<gui_komponente_t *>handle_mouseover;
		while(  !list_dirty  &&  iter.next()  ) {
			gui_komponente_t *komp = iter.get_current();

			// Hajo: deliver events if
			// a) The mouse or click coordinates are inside the component
			// b) The event affects all components, this are WINDOW events
			if(  komp  ) {
				if( DOES_WINDOW_CHILDREN_NEED( ev ) ) { // (Mathew Hounsell)
					// Hajo: no need to translate the event, it has no valid coordinates either
					komp->infowin_event(ev);
				}
				else if(  komp->is_visible()  ) {
					if(  komp->getroffen(x, y)  ) {
						handle_mouseover.insert( komp );
					}
				}

			} // if(komp)
		} // while()

		/* since the last drawn are overlaid over all others
		 * the event-handling must go reverse too
		 */
		slist_iterator_tpl<gui_komponente_t *> iter_mouseover (handle_mouseover);
		while(  !list_dirty  &&  iter_mouseover.next()  ) {
			gui_komponente_t *komp = iter_mouseover.get_current();

			// Hajo: if componet hit, translate coordinates and deliver event
			event_t ev2 = *ev;
			translate_event(&ev2, -komp->get_pos().x, -komp->get_pos().y);

			// Hajo: infowin_event() can delete the component
			// -> thus we need to ask first
			gui_komponente_t *focus = komp->get_focus();

			swallowed = komp->infowin_event(&ev2);

			// set focus for komponente, if komponente allows focus
			if(  focus  &&  IS_LEFTRELEASE(ev)  &&  komp->getroffen(ev->cx, ev->cy)  ) {
				/* the focus swallow all following events;
				 * due to the activation action
				 */
				new_focus = focus;
			}
			// stop here, if event swallowed or focus received
			if(  swallowed  ||  komp==new_focus  ) {
				break;
			}
		}
	}

	list_dirty = false;

	// handle unfocus/next focus stuff
	if(  new_focus!=komp_focus  ) {
		gui_komponente_t *old_focus = komp_focus;
		komp_focus = new_focus;
		if(  old_focus  ) {
			// release focus
			event_t ev2 = *ev;
			translate_event(&ev2, -old_focus->get_pos().x, -old_focus->get_pos().y);
			ev2.ev_class = INFOWIN;
			ev2.ev_code = WIN_UNTOP;
			old_focus->infowin_event(&ev2);
		}
	}

	return swallowed;
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_container_t::zeichnen(koord offset)
{
	const koord screen_pos = pos + offset;

	slist_iterator_tpl<gui_komponente_t *> iter (komponenten);

	while(iter.next()) {
		if (iter.get_current()->is_visible()) {
			// @author hsiegeln; check if component is hidden or displayed
			iter.get_current()->zeichnen(screen_pos);
		}
	}
}


void gui_container_t::set_focus( gui_komponente_t *k )
{
	if(  komponenten.is_contained(k)  ||  k==NULL  ) {
		komp_focus = k;
	}
}


/**
 * returns element that has the focus
 * that is: go down the hierarchy as much as possible
 */
gui_komponente_t *gui_container_t::get_focus() const
{
	if(komp_focus) {
		// if the komp_focus-element has another focused element
		// .. return this element instead
		return komp_focus->get_focus();
	}
	return NULL;
}
