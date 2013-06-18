/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * A container for other gui_komponenten. Is itself
 * a gui_komponente, and can therefor be nested.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include "gui_container.h"

gui_container_t::gui_container_t() : gui_komponente_t(), komp_focus(NULL)
{
	list_dirty = false;
	inside_infowin_event = false;
}


/**
 * Add component to the container
 * @author Hj. Malthaner
 */
void gui_container_t::add_komponente(gui_komponente_t *komp)
{
	/* Inserts/builds the dialog from bottom to top:
	 * Essential for combo-boxes, so they overlap lower elements
	 */
	komponenten.insert(komp);
	list_dirty = true;
}


/**
 * Remove/destroy component from container
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
 * Remove all components from container
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
	inside_infowin_event = true;

	bool swallowed = false;
	gui_komponente_t *new_focus = komp_focus;

	// need to change focus?
	if(  ev->ev_class==EVENT_KEYBOARD  ) {

		if(  komp_focus  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -komp_focus->get_pos().x, -komp_focus->get_pos().y);
			swallowed = komp_focus->infowin_event(&ev2);
		}

		// Knightly : either event not swallowed, or inner container has no focused child component after TAB event
		if(  !swallowed  ||  (ev->ev_code==SIM_KEY_TAB  &&  komp_focus  &&  komp_focus->get_focus()==NULL)  ) {
			if(  ev->ev_code==SIM_KEY_TAB  ) {
				// TAB: find new focus
				new_focus = NULL;
				if(  !IS_SHIFT_PRESSED(ev)  ) {
					// find next textinput field
					FOR(slist_tpl<gui_komponente_t*>, const c, komponenten) {
						if (c == komp_focus) break;
						if (c->is_focusable()) {
							new_focus = c;
						}
					}
				}
				else {
					// or previous input field
					bool valid = komp_focus==NULL;
					FOR(slist_tpl<gui_komponente_t*>, const c, komponenten) {
						if (valid && c->is_focusable()) {
							new_focus = c;
							break;
						}
						if (c == komp_focus) {
							valid = true;
						}
					}
				}

				// Knightly :	inner containers with focusable components may not have a focused component yet
				//				==> give the inner container a chance to activate the first focusable component
				if(  new_focus  &&  new_focus->get_focus()==NULL  ) {
					event_t ev2 = *ev;
					translate_event(&ev2, -new_focus->get_pos().x, -new_focus->get_pos().y);
					new_focus->infowin_event(&ev2);
				}

				swallowed = komp_focus!=new_focus;
			}
			else if(  ev->ev_code==SIM_KEY_ENTER  ||  ev->ev_code==SIM_KEY_ESCAPE  ) {
				new_focus = NULL;
				if(  ev->ev_code==SIM_KEY_ESCAPE  ) {
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

		slist_tpl<gui_komponente_t *>handle_mouseover;
		FOR(  slist_tpl<gui_komponente_t*>,  const komp,  komponenten  ) {
			if(  list_dirty  ) {
				break;
			}

			// Hajo: deliver events if
			// a) The mouse or click coordinates are inside the component
			// b) The event affects all components, this are WINDOW events
			if(  komp  ) {
				if(  DOES_WINDOW_CHILDREN_NEED( ev )  ) { // (Mathew Hounsell)
					// Hajo: no need to translate the event, it has no valid coordinates either
					komp->infowin_event(ev);
				}
				else if(  komp->is_visible()  ) {
					if(  komp->getroffen(x, y)  ) {
						handle_mouseover.insert( komp );
					}
				}

			} // if(komp)
		}

		/* since the last drawn are overlaid over all others
		 * the event-handling must go reverse too
		 */
		FOR(  slist_tpl<gui_komponente_t*>,  const komp,  handle_mouseover  ) {
			if (list_dirty) {
				break;
			}

			// Hajo: if component hit, translate coordinates and deliver event
			event_t ev2 = *ev;
			translate_event(&ev2, -komp->get_pos().x, -komp->get_pos().y);

			// CAUTION : call to infowin_event() should not delete the component itself!
			swallowed = komp->infowin_event(&ev2);

			// focused component of this container can only be one of its immediate children
			gui_komponente_t *focus = komp->get_focus() ? komp : NULL;

			// set focus for komponente, if komponente allows focus
			if(  focus  &&  IS_LEFTCLICK(ev)  &&  komp->getroffen(ev->cx, ev->cy)  ) {
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

	inside_infowin_event = false;

	return swallowed;
}


/* Draw the component
 * @author Hj. Malthaner
 */
void gui_container_t::zeichnen(koord offset)
{
	const koord screen_pos = pos + offset;
	FOR(slist_tpl<gui_komponente_t*>, const c, komponenten) {
		if (c->is_visible()) {
			// @author hsiegeln; check if component is hidden or displayed
			c->zeichnen(screen_pos);
		}
	}
}


bool gui_container_t::is_focusable()
{
	FOR( slist_tpl<gui_komponente_t*>, const c, komponenten ) {
		if(  c->is_focusable()  ) {
			return true;
		}
	}
	return false;
}


void gui_container_t::set_focus( gui_komponente_t *k )
{
	if(  inside_infowin_event  ) {
		dbg->error("gui_container_t::set_focus", "called from inside infowin_event, will have no effect");
	}
	if(  komponenten.is_contained(k)  ||  k==NULL  ) {
		komp_focus = k;
	}
}


/**
 * returns element that has the focus
 * that is: go down the hierarchy as much as possible
 */
gui_komponente_t *gui_container_t::get_focus()
{
	// if the komp_focus-element has another focused element
	// .. return this element instead
	return komp_focus ? komp_focus->get_focus() : NULL;
}
