/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "gui_container.h"
#include "../gui_theme.h"

// DEBUG: shows outline of all elements
//#define SHOW_BBOX

gui_container_t::gui_container_t() : gui_component_t(), comp_focus(NULL)
{
	list_dirty = false;
	inside_infowin_event = false;
}


/**
 * Add component to the container
 */
void gui_container_t::add_component(gui_component_t *comp)
{
	/* Inserts/builds the dialog from bottom to top:
	 * Essential for combo-boxes, so they overlap lower elements
	 */
	components.append(comp);
	list_dirty = true;
}


/**
 * Remove/destroy component from container
 */
void gui_container_t::remove_component(gui_component_t *comp)
{
	/* since we can remove a subcomponent,
	 * that actually contains the element with focus
	 */
	if(  comp_focus == comp  ||  comp_focus == comp->get_focus()  ) {
		comp_focus = NULL;
	}
	components.remove(comp);
	list_dirty = true;
}


/**
 * Remove all components from container
 */
void gui_container_t::remove_all()
{
	// clear also focus
	while(  !components.empty()  ) {
		remove_component( components.pop_back() );
	}
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool gui_container_t::infowin_event(const event_t *ev)
{
	inside_infowin_event = true;

	bool swallowed = false;
	gui_component_t *new_focus = comp_focus;

	// unfocus invisible focused component
	if (comp_focus  &&  !comp_focus->is_visible()) {
		new_focus = NULL;
	}

	// need to change focus?
	if(  ev->ev_class==EVENT_KEYBOARD  ) {

		if(  comp_focus  &&  comp_focus->is_visible()  ) {
			event_t ev2 = *ev;
			ev2.move_origin(comp_focus->get_pos());
			swallowed = comp_focus->infowin_event(&ev2);
		}

		// either event not swallowed, or inner container has no focused child component after TAB event
		if(  !swallowed  ||  (ev->ev_code==SIM_KEY_TAB  &&  comp_focus  &&  comp_focus->get_focus()==NULL)  ) {
			if(  ev->ev_code==SIM_KEY_TAB  ) {
				// TAB: find new focus
				new_focus = NULL;
				if(  IS_SHIFT_PRESSED(ev)  ) {
					// find previous textinput field
					FOR(vector_tpl<gui_component_t*>, const c, components) {
						if (c == comp_focus) break;
						if (c->is_focusable()) {
							new_focus = c;
						}
					}
				}
				else {
					// or next input field
					bool valid = comp_focus==NULL;
					FOR(vector_tpl<gui_component_t*>, const c, components) {
						if (valid && c->is_focusable()) {
							new_focus = c;
							break;
						}
						if (c == comp_focus) {
							valid = true;
						}
					}
				}

				// inner containers with focusable components may not have a focused component yet
				// ==> give the inner container a chance to activate the first focusable component
				if(  new_focus  &&  new_focus->get_focus()==NULL  ) {
					event_t ev2 = *ev;
					ev2.move_origin(new_focus->get_pos());
					new_focus->infowin_event(&ev2);
				}

				swallowed = comp_focus!=new_focus;
			}
			else if(  ev->ev_code==SIM_KEY_ENTER  ||  ev->ev_code==SIM_KEY_ESCAPE  ) {
				new_focus = NULL;
				if(  ev->ev_code==SIM_KEY_ESCAPE  ) {
					// no untop message even!
					comp_focus = NULL;
				}
				swallowed = comp_focus!=new_focus;
			}
		}
	}
	else {
		// CASE : not a keyboard event
		const int x = ev->ev_class==EVENT_MOVE ? ev->mx : ev->cx;
		const int y = ev->ev_class==EVENT_MOVE ? ev->my : ev->cy;

		// Handle the focus!
		if(  comp_focus  &&  comp_focus->is_visible()  ) {
			gui_component_t *const comp = comp_focus;
			event_t ev2 = *ev;
			ev2.move_origin(comp->get_pos());
			swallowed = comp->infowin_event(&ev2);

			// set focus for component, if component allows focus
			gui_component_t *const focus = comp->get_focus() ? comp : NULL;
			if(  focus  &&  IS_LEFTCLICK(ev)  &&  comp->getroffen(ev->cx, ev->cy)  ) {
				/* the focus swallow all following events;
				 * due to the activation action
				 */
				new_focus = focus;
			}
		}

		if(  !swallowed  ) {

			vector_tpl<gui_component_t *>handle_mouseover;
			FOR(  vector_tpl<gui_component_t*>,  const comp,  components  ) {

				if(  list_dirty  ) {
					break;
				}

				if(  comp == comp_focus  ) {
					// do not handle focus objects twice
					continue;
				}

				// deliver events if
				// a) The mouse or click coordinates are inside the component
				// b) The event affects all components, this are WINDOW events
				if(  comp  ) {
					if(  DOES_WINDOW_CHILDREN_NEED( ev )  ) {
						// no need to translate the event, it has no valid coordinates either
						comp->infowin_event(ev);
					}
					else if(  comp->is_visible()  ) {
						if(  comp->getroffen(x, y)  ) {
							handle_mouseover.append( comp );
						}
					}

				} // if(comp)
			}

			/* since the last drawn are overlaid over all others
			 * the event-handling must go reverse too
			 */
			FOR(  vector_tpl<gui_component_t*>,  const comp,  handle_mouseover  ) {

				if (list_dirty) {
					break;
				}

				if(  comp == comp_focus  ) {
					// do not handle focus objects twice
					continue;
				}

				// if component hit, translate coordinates and deliver event
				event_t ev2 = *ev;
				ev2.move_origin(comp->get_pos());

				// CAUTION : call to infowin_event() should not delete the component itself!
				swallowed = comp->infowin_event(&ev2);

				// set focus for component, if component allows focus
				gui_component_t *focus = comp->get_focus() ? comp : NULL;

				// set focus for component, if component allows focus
				if(  focus  &&  IS_LEFTRELEASE(ev)  &&  comp->getroffen(ev->cx, ev->cy)  ) {
					/* the focus swallow all following events;
					 * due to the activation action
					 */
					new_focus = focus;
				}

				// stop here, if event swallowed or focus received
				if(  swallowed  ||  comp==new_focus  ) {
					break;
				}
			}
		}
	}

	list_dirty = false;

	// handle unfocus/next focus stuff, but do no unfocus twice
	if(  new_focus!=comp_focus  &&  !(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_UNTOP)  ) {
		gui_component_t *old_focus = comp_focus;
		comp_focus = new_focus;
		if(  old_focus  ) {
			// release focus
			event_t ev2 = *ev;
			ev2.move_origin(old_focus->get_pos());
			ev2.ev_class = INFOWIN;
			ev2.ev_code = WIN_UNTOP;
			old_focus->infowin_event(&ev2);
		}
	}

	inside_infowin_event = false;

	return swallowed;
}


/**
 * Draw the component
 */
void gui_container_t::draw(scr_coord offset)
{
	const scr_coord screen_pos = pos + offset;
	bool redraw_focus = false;

	clip_dimension cd = display_get_clip_wh();
	scr_rect clip_rect(cd.x, cd.y, cd.w, cd.h);

	// For debug purpose, draw the container's boundary
#ifdef SHOW_BBOX
#define shorten(d) clamp(d, 0, 0x4fff)
	display_ddd_box_clip_rgb(shorten(screen_pos.x), shorten(screen_pos.y), shorten(get_size().w), shorten(get_size().h), color_idx_to_rgb(COL_RED), color_idx_to_rgb(COL_RED));
#endif
	// iterate backwards
	for(  uint32 iter = components.get_count(); iter > 0; iter--) {
		gui_component_t*const c = components[iter-1];
		if (c->is_visible()) {

			// check if component is in the drawing region
			// also fixes integer overflow as KOORDVAL in simgraph is 16bit, while scr_coord is 32bit
			if (!clip_rect.is_overlapping( scr_rect(screen_pos + c->get_pos(), c->get_size()) ) ) {
				continue;
			}

			if(  c == comp_focus  ) {
				redraw_focus = true;
				continue;
			}
#ifdef SHOW_BBOX
			if (dynamic_cast<gui_container_t*>(c) == NULL) {
				scr_coord c_pos = screen_pos + c->get_pos();
				int color = c->is_marginless() ? COL_BLUE : COL_YELLOW;
				display_ddd_box_clip_rgb(shorten(c_pos.x), shorten(c_pos.y), shorten(c->get_size().w), shorten(c->get_size().h), color_idx_to_rgb(color),color_idx_to_rgb(color));
			}
#endif
			c->draw(screen_pos);
		}
	}
	// this allows focussed (selected) components to overlay other components; but focus may subcomponent!
	if(  redraw_focus  ) {
		comp_focus->draw(screen_pos);
	}
}


bool gui_container_t::is_focusable()
{
	FOR( vector_tpl<gui_component_t*>, const c, components ) {
		if(  c->is_focusable()  ) {
			return true;
		}
	}
	return false;
}


void gui_container_t::set_focus( gui_component_t *c )
{
	if(  inside_infowin_event  ) {
		assert(false);
		dbg->error("gui_container_t::set_focus", "called from inside infowin_event, will have no effect");
	}
	if(  components.is_contained(c)  ||  c==NULL  ) {
		comp_focus = c;
	}
}


/**
 * returns element that has the focus
 * that is: go down the hierarchy as much as possible
 */
gui_component_t *gui_container_t::get_focus()
{
	// if the comp_focus-element has another focused element
	// .. return this element instead
	return comp_focus ? comp_focus->get_focus() : NULL;
}
