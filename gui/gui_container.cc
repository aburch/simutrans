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
	if(  komp_focus == komp  ) {
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
void gui_container_t::infowin_event(const event_t *ev)
{
	slist_iterator_tpl<gui_komponente_t *> iter (komponenten);
	// try to deliver event to gui komponente which has focus
	if(komp_focus != NULL) {
		if(ev->ev_class==EVENT_KEYBOARD  ||  (komp_focus->getroffen(ev->mx, ev->my) || komp_focus->getroffen(ev->cx, ev->cy))  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -komp_focus->get_pos().x, -komp_focus->get_pos().y);
			komp_focus->infowin_event(&ev2);
			if(  ev->ev_code==13  ||  ev->ev_code==27  ) {
				// release focus
				// TAB for next field would be possible too ...
				komp_focus = NULL;
			}
			return;
		}
	}

	while(!list_dirty && iter.next()) {
		gui_komponente_t *komp = iter.get_current();

		// Hajo: deliver events if
		// a) The mouse or click coordinates are inside the component
		// b) The event affects all components, this are WINDOW events
		if(komp) {
			if(komp->getroffen(ev->mx, ev->my) || komp->getroffen(ev->cx, ev->cy)) {

				// Hajo: if componet hit, translate coordinates and deliver event
				event_t ev2 = *ev;
				translate_event(&ev2, -komp->get_pos().x, -komp->get_pos().y);

				// Hajo: infowon_event() can delete the component
				// -> thus we need to ask first
				const bool allow_focus = komp->get_allow_focus();

				komp->infowin_event(&ev2);
				// set focus for komponente, if komponente allows focus
				if(allow_focus) {
					komp_focus = komp;
					break;
				}
			} else if( DOES_WINDOW_CHILDREN_NEED( ev ) ) { // (Mathew Hounsell)
				// Hajo: no need to translate the event, it has no valid coordinates either
				komp->infowin_event(ev);
			}
		} // if(komp)
	}

	list_dirty = false;
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
		gui_komponente_t *focus = komp_focus->get_focus();
		if (focus) {
			return focus;
		}
	}
	return komp_focus;
}
