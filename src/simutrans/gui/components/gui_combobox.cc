/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Defines a drop-down list with left/right arrows
 */

#include <string.h>

#include "../../macros.h"
#include "../../simdebug.h"
#include "../gui_frame.h"
#include "gui_combobox.h"
#include "../../simevent.h"
#include "../../display/simgraph.h"
#include "../../display/scr_coord.h"
#include "../../simcolor.h"
#include "../simwin.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/simstring.h"


gui_combobox_t::gui_combobox_t(gui_scrolled_list_t::item_compare_func cmp) :
	gui_component_t(true),
	droplist(gui_scrolled_list_t::listskin, cmp)
{
	bt_prev.set_typ(button_t::arrowleft);
	bt_prev.set_pos( scr_coord(0,2) );

	bt_next.set_typ(button_t::arrowright);

	set_focusable( true ); // needed, otherwise fails on closing when clicking elsewhere!

	editstr[0] = 0;
	old_editstr[0] = 0;
	textinp.add_listener(this);

	first_call = true;
	finish = false;
	wrapping = true;
	droplist.set_visible(false);
	droplist.add_listener(this);
	closed_size = get_size();
	max_size = scr_size(0,10*LINESPACE);
	opened_above = false;
	last_draw_offset = scr_coord(0,0);
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool gui_combobox_t::infowin_event(const event_t *ev)
{
	if( !(bt_next.enabled() || bt_prev.enabled()) ) {
		// no infowind events if disabled
		if(  droplist.is_visible()  ) {
			close_box();
		}
		return false;
	}

	if(  !droplist.is_visible()  ) {
DBG_MESSAGE("event","%d,%d",ev->cx, ev->cy);
		if(  bt_prev.getroffen(ev->cx, ev->cy)  ) {
DBG_MESSAGE("event","HOWDY!");
			bt_prev.pressed = IS_LEFT_BUTTON_PRESSED(ev);
			if(IS_LEFTRELEASE(ev)) {
				value_t p;
				bt_prev.pressed = false;
				set_selection( droplist.get_selection() > 0 ? droplist.get_selection() - 1 : wrapping ? droplist.get_count() - 1 : 0 );
				p.i = droplist.get_selection();
				call_listeners( p );
			}
			return true;
		}
		else if(  bt_next.getroffen(ev->cx, ev->cy)  ) {
			bt_next.pressed = IS_LEFT_BUTTON_PRESSED(ev);
			if(IS_LEFTRELEASE(ev)) {
				bt_next.pressed = false;
				value_t p;
				set_selection( droplist.get_selection() < droplist.get_count() - 1 ? droplist.get_selection() + 1 : wrapping ? 0 : droplist.get_count() - 1 );
				p.i = droplist.get_selection();
				call_listeners(p);
			}
			return true;
		}
	}
	else if(  (IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))  &&  droplist.getroffen(ev->mx + get_pos().x, ev->my + get_pos().y)  ) {
		// scroll the list
		droplist.infowin_event(ev);
		return true;
	}

	// goto next/previous choice
	if(  ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code == SIM_KEY_UP  ||  ev->ev_code == SIM_KEY_DOWN)  ) {
		int sel = droplist.get_selection();
		if(  ev->ev_code == SIM_KEY_UP  ) {
			set_selection(  sel > 0 ? sel-1 : (wrapping ? droplist.get_count()-1 : 0) );
		}
		else {
			set_selection( sel < droplist.get_count()-1 ? sel+1 : (wrapping ? 0 : droplist.get_count()-1) );
		}
		value_t p;
		p.i = droplist.get_selection();
		call_listeners( p );
		return true;
	}

	if(  ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code == SIM_KEY_ENTER  ||  ev->ev_code == SIM_KEY_TAB)  &&  droplist.is_visible()  ) {
		// close with enter/tab
		close_box();
		return ev->ev_code == SIM_KEY_ENTER;
	}

	if(  IS_LEFTCLICK(ev)  ||  IS_LEFTDRAG(ev)  ||  IS_LEFTRELEASE(ev)  ||  IS_LEFTDRAG(ev)  ) {

		if(first_call) {

			// ignore clicks outside if closed
			scr_rect this_comp( get_size() );
			if(  !droplist.is_visible()  &&  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
				// not us, just in old focus from previous selection or tab
				return false;
			}

			// swallow the first mouse click
			if(  !IS_LEFTRELEASE(ev)  ) {
				return false;
			}
			first_call = false;

			// else prepare for selection (after a left mbutton release event!)
			droplist.set_visible(true);

			// determine possible size of droplist and whether open below/above input field
			scr_coord_val win_height = win_get_top()->get_windowsize().h - D_TITLEBAR_HEIGHT;
			scr_coord_val last_draw_y = last_draw_offset.y  + get_pos().y - win_get_pos(win_get_top()).y- D_TITLEBAR_HEIGHT;
			scr_coord_val height_above = last_draw_y - D_V_SPACE;
			scr_coord_val height_below = win_height - (last_draw_y + textinp.get_size().h + D_V_SPACE);
			scr_coord_val request_height = min(max_size.h - closed_size.h, droplist.get_max_size().h);

			// request size of droplist, should stay inside window
			// call returns actual height, might be smaller than request_height
			request_height = droplist.request_size(scr_size(this->size.w, min(request_height, max(height_above, height_below))) ).h;

			// open below if enough space or more space than above
			opened_above = request_height > height_below  &&  height_below < height_above;
			set_pos(get_pos());

			int sel = droplist.get_selection();
			if((uint32)sel>=(uint32)droplist.get_count()  ||  !droplist.get_element(sel)->is_valid()) {
				sel = 0;
			}
			droplist.show_selection(sel);
		}
		else if (droplist.is_visible()) {
			event_t ev2 = *ev;
			const scr_coord diff = droplist.get_pos() - gui_component_t::get_pos();
			ev2.move_origin(diff);

			if( droplist.getroffen(ev->cx + pos.x, ev->cy + pos.y)  ) {
				int old_selection = droplist.get_selection();
				if(  droplist.infowin_event(&ev2)  ) {
					if(  droplist.get_selection() !=  old_selection  ) {
						// close box will anyway call
						if( !IS_LEFTRELEASE( ev ) ) {
							// in case of LEFTRELEASE, close_box will call it again
							call_listeners( droplist.get_selection() );
						}
						finish = true;
					}
					// we selected something?
					if(finish  &&  IS_LEFTRELEASE(ev)) {
						close_box();
					}
				}
				return true;
			}
			else {
				// acting on "release" is better than checking for "new selection"
				if(  IS_LEFTRELEASE(ev)  ) {
					close_box();
					return false;
				}
			}
		}
		return true;
	}
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_CLOSE  ||  ev->ev_code==WIN_UNTOP)  ) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
		textinp.infowin_event(ev);
		return true;
	}
	else {
		// finally handle textinput
		gui_scrolled_list_t::scrollitem_t *item = droplist.get_selected_item();
		if(  item==NULL  ||  item->is_editable()  ) {
			event_t ev2 = *ev;
			ev2.move_origin(textinp.get_pos());
			return textinp.infowin_event(ev);
		}
	}
	return false;
}


/* selection now handled via callback */
bool gui_combobox_t::action_triggered( gui_action_creator_t *comp,value_t p)
{
	if (  comp == &droplist  ) {
		const int selection = (int)p.i;
DBG_MESSAGE("gui_combobox_t::infowin_event()", "scroll selected %i", selection);
		finish = true;
		set_selection(selection);
	}
	else if (  comp == &textinp  ) {
		rename_selected_item();
	}
	return false;
}


/**
 * Draw the component
 */
void gui_combobox_t::draw(scr_coord offset)
{
	last_draw_offset = offset;
	// text changed? Then update it
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_selected_item();
	if(  item  &&  item->is_valid()  &&  item->is_editable()  &&  strncmp( item->get_text(), old_editstr, 127 )  ) {
		reset_selected_item_name();
	}

	bool with_focus = (win_get_focus()==this)  &&  (item==NULL  ||  item->is_editable());
	textinp.display_with_focus( offset, with_focus);

	if(  droplist.is_visible()  ) {
		droplist.draw(offset);
	}
	else {
		offset += pos;
		bt_prev.draw(offset);
		bt_next.draw(offset);
	}
}


void gui_combobox_t::enable()
{
	set_focusable(true);
	bt_next.enable();
	bt_prev.enable();
	textinp.set_color(SYSCOL_EDIT_TEXT);
}

void gui_combobox_t::disable()
{
	set_focusable(false);
	bt_next.disable();
	bt_prev.disable();
	textinp.set_color(SYSCOL_EDIT_TEXT_DISABLED);
}

/**
 * sets the selection
 */
void gui_combobox_t::set_selection(int s)
{
	// try to finish renaming first
	rename_selected_item();

	if (droplist.is_visible()  &&  !finish) {
		// visible and not closing
		// change also offset of scrollbar
		droplist.show_selection( s );
	}
	else {
		// just set it
		droplist.set_selection(s);
	}
	// edit the text
	reset_selected_item_name();
}


/**
 * Check whether we should rename selected item
 */
void gui_combobox_t::rename_selected_item()
{
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_selected_item();
	// if name was not changed in the meantime, we can rename it
	if(  item  &&  item->is_valid()  &&  item->is_editable()  ) {
		const char *current_str = ((gui_scrolled_list_t::const_text_scrollitem_t *)item)->get_text();
		if(  strncmp( current_str, old_editstr, 127 )==0  &&  strncmp( current_str, editstr, 127 )!=0  ) {
			item->set_text(editstr);
		}
	}
}


void gui_combobox_t::reset_selected_item_name()
{
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_selected_item();
	if(  item==NULL  ) {
		editstr[0] = 0;
		textinp.set_text( editstr, 0  );
		droplist.set_selection(-1);
	}
	else if(  item->is_valid()  ) {
		const char *current_str = ((gui_scrolled_list_t::const_text_scrollitem_t *)item)->get_text();
		if(  strncmp( current_str, old_editstr, 127 )!=0  ) {
			tstrncpy( editstr, current_str, lengthof(editstr) );
			textinp.set_text( editstr, lengthof(editstr) );
		}
	}
	tstrncpy( old_editstr, editstr, lengthof(old_editstr) );
}


/**
* Release the focus if we had it
*/
void gui_combobox_t::close_box()
{
	if(finish) {
		value_t p;
		p.i = droplist.get_selection();
		call_listeners(p);
		finish = false;
	}
	droplist.set_visible(false);
	first_call = true;
}


void gui_combobox_t::set_pos(scr_coord pos_par)
{
	gui_component_t::set_pos( pos_par );

	if(  opened_above  ) {
		droplist.set_pos( gui_component_t::get_pos() + scr_size( (get_size().w -droplist.get_size().w)/2, D_V_SPACE/4 - droplist.get_size().h) );
	}
	else {
		droplist.set_pos( gui_component_t::get_pos() + scr_size( (get_size().w -droplist.get_size().w)/2, textinp.get_size().h) );
	}
}


void gui_combobox_t::set_size(scr_size size)
{
	closed_size = size;
	gui_component_t::set_size( size );

	droplist.request_size(scr_size(this->size.w, droplist.get_size().h));

	textinp.set_size( scr_size( size.w - bt_prev.get_size().w - bt_next.get_size().w - D_H_SPACE, closed_size.h ) );
	set_pos(get_pos());

	bt_prev.set_pos( scr_coord(0,(size.h-D_ARROW_LEFT_HEIGHT)/2) );
	textinp.align_to( &bt_prev, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord( pos.x + D_H_SPACE / 2, pos.y ) );
	bt_next.align_to( &textinp, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord( -pos.x + D_H_SPACE / 2, -pos.y ) );
}


/**
* set maximum size for control
*/
void gui_combobox_t::set_max_size(scr_size max)
{
	max_size = max;
	droplist.request_size( scr_size( size.w, max_size.h - closed_size.h ) );
}


scr_size gui_combobox_t::get_min_size() const
{
	scr_size bl = bt_prev.get_min_size();
	scr_size ti = textinp.get_min_size();
	scr_size sl = droplist.get_min_size();
	scr_size br = bt_next.get_min_size();

	if (sl.w == scr_size::inf.w) {
		// contains editable items, use min-width of input
		return scr_size(bl.w + ti.w + br.w + D_H_SPACE, max(max(bl.h, ti.h), br.h));
	}
	else {
		return scr_size(bl.w + br.w + sl.w - D_H_SPACE, max(max(bl.h, ti.h), br.h));
	}
}


scr_size gui_combobox_t::get_max_size() const
{
	scr_size msize = get_min_size();
	msize.w = droplist.get_max_size().w;

	return msize;
}


void gui_combobox_t::rdwr( loadsave_t *file )
{
	sint32 i = get_selection();
	file->rdwr_long(i);
	set_selection(i);
}
