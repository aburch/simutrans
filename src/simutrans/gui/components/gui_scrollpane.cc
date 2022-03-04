/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simdebug.h"

#include "../gui_frame.h"
#include "gui_scrollpane.h"
#include "gui_scrollbar.h"
#include "gui_button.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/environment.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"


/**
 * @param comp the scrolling component
 */
gui_scrollpane_t::gui_scrollpane_t(gui_component_t *comp, bool b_scroll_x, bool b_scroll_y) :
	scroll_x(scrollbar_t::horizontal),
	scroll_y(scrollbar_t::vertical)
{
	this->comp = comp;

	max_width = D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT;

	b_show_scroll_x = b_scroll_x;
	b_show_scroll_y = b_scroll_y;
	b_has_size_corner = true;
	b_can_drag = true;
	b_is_dragging = false;

	old_comp_size = scr_size::invalid;
	maximize = false;
}


scr_size gui_scrollpane_t::get_min_size() const
{
	// request width of largest element, but leave enough space for scrollbars
	scr_size csize = comp->get_min_size();
	csize.w = max( csize.w, scroll_x.get_min_size().w );
	csize.w = min( csize.w, max_width );
	csize.h = min( csize.h, scroll_y.get_min_size().h );
	return csize;
}

scr_size gui_scrollpane_t::get_max_size() const
{
	return comp->get_max_size();
}


/**
 * recalc the scroll bar sizes
 */
void gui_scrollpane_t::recalc_sliders(scr_size size)
{
	scroll_x.set_pos( scr_coord(0, size.h-D_SCROLLBAR_HEIGHT) );
	scroll_y.set_pos( scr_coord(size.w-D_SCROLLBAR_WIDTH, 0) );
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		scroll_x.set_size( size-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( size.w-D_SCROLLBAR_WIDTH, comp->get_size().w + comp->get_pos().x );
	}
	else if(  b_has_size_corner  ) {
		scroll_x.set_size( size-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( size.w, comp->get_size().w + comp->get_pos().x );
	}
	else {
		scroll_x.set_size( size-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( size.w, comp->get_size().w + comp->get_pos().x );
	}

	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		scroll_y.set_size( size-D_SCROLLBAR_SIZE );
		scroll_y.set_knob( size.h-D_SCROLLBAR_HEIGHT, comp->get_size().h + comp->get_pos().y );
	}
	else if(  b_has_size_corner  ) {
		scroll_y.set_size( size-D_SCROLLBAR_SIZE );
		scroll_y.set_knob( size.h, comp->get_size().h + comp->get_pos().y );
	}
	else {
		scroll_y.set_size( size-scr_coord(D_SCROLLBAR_WIDTH,0) );
		scroll_y.set_knob( size.h, comp->get_size().h + comp->get_pos().y );
	}

	old_comp_size = comp->get_size();
}


/**
 * Scrollpanes _must_ be used in this method to set the size
 */
void gui_scrollpane_t::set_size(scr_size size)
{
	gui_component_t::set_size(size);
	// automatically increase/decrease slider area
	scr_coord k = comp->get_size()+comp->get_pos();
	scroll_x.set_visible( (k.x > size.w)  &&  b_show_scroll_x  );
	scroll_y.set_visible(  (k.y > size.h)  &&  b_show_scroll_y  );

	scr_size c_size = size - comp->get_pos();
	// resize scrolled component
	if (scroll_x.is_visible()) {
		c_size.h -= scroll_x.get_size().h;
	}
	if (scroll_y.is_visible()) {
		c_size.w -= scroll_y.get_size().w;
	}

	c_size.clip_lefttop( comp->get_min_size() );
	c_size.clip_rightbottom( comp->get_max_size() );
	comp->set_size(c_size);

	recalc_sliders(size);
	show_focused();
}


scr_size gui_scrollpane_t::request_size(scr_size request)
{
	// do not enlarge past max size of comp
	scr_size cmax = comp->get_max_size();
	if (cmax.w  < request.w - comp->get_pos().x  &&  cmax.h < request.h - comp->get_pos().y) {
		request = cmax;
	}
	set_size(request);
	return get_size();
}


/**
 * Events werden hiermit an die GUI-components gemeldet
 */
bool gui_scrollpane_t::infowin_event(const event_t *ev)
{
	bool swallow = false;
	if(   (b_show_scroll_y  &&  scroll_y.is_visible())  &&  ev->ev_class!=EVENT_KEYBOARD  &&  (scroll_y.getroffen(ev->mx, ev->my) || scroll_y.getroffen(ev->cx, ev->cy)) ) {
		event_t ev2 = *ev;
		ev2.move_origin(scroll_y.get_pos());
		b_is_dragging = false;
		return scroll_y.infowin_event(&ev2);
	}
	else if(  (b_show_scroll_x  &&  scroll_x.is_visible())  &&  ev->ev_class!=EVENT_KEYBOARD  &&  (scroll_x.getroffen(ev->mx, ev->my) || scroll_x.getroffen(ev->cx, ev->cy))) {
		event_t ev2 = *ev;
		ev2.move_origin(scroll_x.get_pos());
		b_is_dragging = false;
		return scroll_x.infowin_event(&ev2);
	}
	else if(  ev->ev_class<EVENT_CLICK  ||  (ev->mx>=0 &&  ev->my>=0  &&  ev->mx<=size.w  &&  ev->my<=size.h)  ||  b_is_dragging  ) {

		// since we get can grab the focus to get keyboard events, we must make sure to handle mouse events only if we are hit
		if(  ev->ev_class < EVENT_CLICK  ||  IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev)  ) {
			b_is_dragging = false;
		}

		// we will handle dragging ourselves inf not prevented
		if(  b_is_dragging  &&  ev->ev_class < INFOWIN  ) {
			// now drag: scrollbars are not in pixel, but we will scroll one unit per pixels ...
			scroll_x.set_knob_offset(scroll_x.get_knob_offset() - (ev->mx - origin.x));
			scroll_y.set_knob_offset(scroll_y.get_knob_offset() - (ev->my - origin.y));
			origin = scr_coord(ev->mx, ev->my);
			// and finally end dragging on release of any button
			if(  ev->ev_class == EVENT_RELEASE  ) {
				b_is_dragging = false;
				if(  abs(ev->mx - ev->cx) >= 5  || abs(ev->cx-ev->mx)+abs(ev->cy-ev->my) >= env_t::scroll_threshold  ) {
					// dragged a lot => swallow click
					return true;
				}
			}
			else {
				// continue dragging, swallow other events
				return true;
			}
		}

		// translate according to scrolled position
		event_t ev2 = *ev;
		const scr_coord offset(scroll_x.get_knob_offset(), scroll_y.get_knob_offset());
		ev2.move_origin(comp->get_pos() - offset);

		gui_component_t *focused = get_focus();
		// hand event to component
		swallow = comp->infowin_event(&ev2);

		// now process wheel-events that are not swallowed by component, scroll the pane
		if(!swallow) {
			if(  (IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))
				 &&  (((b_show_scroll_y  &&  scroll_y.is_visible())  &&  !IS_SHIFT_PRESSED(ev))  ||  ((b_show_scroll_x  &&  scroll_x.is_visible())  &&  IS_SHIFT_PRESSED(ev)))  ) {
				// otherwise these events are only registered where directly over the scroll region
				// (and sometime even not then ... )
				return IS_SHIFT_PRESSED(ev) ? scroll_x.infowin_event(ev) : scroll_y.infowin_event(ev);
			}
		}

		if(  !swallow  &&  b_can_drag  &&  (ev->ev_class == EVENT_CLICK || ev->ev_class == EVENT_DRAG)  ) {
			// init dragging? (Android SDL starts dragging without preceeding click!)
			if(!b_is_dragging) {
				origin = scr_coord(ev->mx, ev->my);
				b_is_dragging = true;
				return true;
			}
		}

		// check if we need to scroll to the focused component
		gui_component_t *new_focus = get_focus();
		if(new_focus &&  focused != new_focus) {
			show_focused();
		}

		// hack: component could have changed size
		// this recalculates the scrollbars
		if(  old_comp_size!=comp->get_size()  ) {
			recalc_sliders(get_size());
		}

	}
	return swallow;
}


void gui_scrollpane_t::show_focused()
{
	const gui_component_t *const focused_comp = comp->get_focus();
	if(  focused_comp  ) {
		const scr_size comp_size = focused_comp->get_size();
		const scr_coord relative_pos = comp->get_focus_pos();
		if(  b_show_scroll_x  ) {
			const sint32 knob_offset_x = scroll_x.get_knob_offset();
			const sint32 view_width = size.w-D_SCROLLBAR_WIDTH;
			if(  relative_pos.x+comp_size.w<knob_offset_x  ) {
				scroll_x.set_knob_offset(relative_pos.x);
			}
			if(  relative_pos.x>knob_offset_x+view_width  ) {
				scroll_x.set_knob_offset(relative_pos.x+comp_size.w-view_width);
			}
		}
		if(  b_show_scroll_y  ) {
			const sint32 knob_offset_y = scroll_y.get_knob_offset();
			const sint32 view_height = (b_has_size_corner || b_show_scroll_x) ? size.h-D_SCROLLBAR_HEIGHT : size.h;
			if(  relative_pos.y+comp_size.h<knob_offset_y  ) {
				scroll_y.set_knob_offset(relative_pos.y);
			}
			if(  relative_pos.y>knob_offset_y+view_height  ) {
				scroll_y.set_knob_offset(relative_pos.y+comp_size.h-view_height);
			}
		}
	}
}


/**
 * Set the position of the Scrollbars
 */
void gui_scrollpane_t::set_scroll_position(int x, int y)
{
	scroll_x.set_knob_offset(x);
	scroll_y.set_knob_offset(y);
}


int gui_scrollpane_t::get_scroll_x() const
{
	return scroll_x.get_knob_offset();
}


int gui_scrollpane_t::get_scroll_y() const
{
	return scroll_y.get_knob_offset();
}


scr_rect gui_scrollpane_t::get_client( void )
{
	scr_rect client( pos, pos+size );
	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		client.h -= D_SCROLLBAR_HEIGHT;
	}
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		client.w -= D_SCROLLBAR_WIDTH;
	}
	return client;
}


/**
 * Draw the component
 */
void gui_scrollpane_t::draw(scr_coord pos)
{
	// check, if we need to recalc slider size
	if(  old_comp_size  !=  comp->get_size()  ) {
		recalc_sliders( size );
	}

	// get client area (scroll panel - scrollbars)
	scr_rect client = get_client() + pos;

	PUSH_CLIP_FIT( client.x, client.y, client.w, client.h )
		comp->draw( client.get_pos()-scr_coord(scroll_x.get_knob_offset(), scroll_y.get_knob_offset()) );
	POP_CLIP()

	// sliding bar background color is now handled by the scrollbar!
	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		scroll_x.draw( pos+get_pos() );
	}
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		scroll_y.draw( pos+get_pos() );
	}
}

void gui_scrollpane_t::rdwr( loadsave_t *file )
{
	sint32 x = get_scroll_x();
	sint32 y = get_scroll_y();
	file->rdwr_long(x);
	file->rdwr_long(y);
	if (file->is_loading()) {
		set_scroll_position(x, y);
	}
}
