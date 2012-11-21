/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../../macros.h"
#include "../../simdebug.h"
#include "gui_combobox.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"
#include "../../utils/simstring.h"


gui_combobox_t::gui_combobox_t() :
	gui_komponente_t(true),
	droplist(gui_scrolled_list_t::select)
{
	bt_prev.set_typ(button_t::arrowleft);
	bt_prev.set_pos( koord(0,2) );
	bt_prev.set_groesse( koord(10,10) );

	bt_next.set_typ(button_t::arrowright);
	bt_next.set_groesse( koord(10,10) );

	editstr[0] = 0;
	old_editstr[0] = 0;
	textinp.add_listener(this);

	first_call = true;
	finish = false;
	wrapping = true;
	droplist.set_visible(false);
	droplist.add_listener(this);
	set_groesse(get_groesse());
	max_size = koord(0,100);
	set_highlight_color(0);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_combobox_t::infowin_event(const event_t *ev)
{
	if (!droplist.is_visible()) {
DBG_MESSAGE("event","%d,%d",ev->cx, ev->cy);
		if(bt_prev.getroffen(ev->cx, ev->cy)) {
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
		else if(bt_next.getroffen(ev->cx, ev->cy)) {
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
	else if(  IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev)  ) {
		// scroll the list
		droplist.infowin_event(ev);
		return true;
	}

	// goto next/previous choice
	if(  ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code == SIM_KEY_UP  ||  ev->ev_code == SIM_KEY_DOWN)  ) {
		if(  ev->ev_code == SIM_KEY_UP  ) {
			set_selection( droplist.get_selection() > 0 ? droplist.get_selection() - 1 : droplist.get_count() - 1);
		}
		else {
			set_selection( droplist.get_selection() < droplist.get_count() - 1 ? droplist.get_selection() + 1 : 0);
		}
		value_t p;
		p.i = droplist.get_selection();
		call_listeners( p );
		return true;
	}

	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev) || IS_LEFTRELEASE(ev)  ) {

		if(first_call) {
			// prepare for selection

			// swallow the first mouseclick
			if(IS_LEFTRELEASE(ev)) {
				first_call = false;
			}

			droplist.set_visible(true);
			droplist.set_pos(koord(this->pos.x, this->pos.y + 16));
			droplist.request_groesse(koord(this->groesse.x, max_size.y - 16));
			set_groesse(droplist.get_groesse() + koord(0, 16));
			int sel = droplist.get_selection();
			if((uint32)sel>=(uint32)droplist.get_count()  ||  !droplist.get_element(sel)->is_valid()) {
				sel = 0;
			}
			droplist.show_selection(sel);
		}
		else if (droplist.is_visible()) {
			event_t ev2 = *ev;
			translate_event(&ev2, 0, -16);

			if(droplist.getroffen(ev->cx + pos.x, ev->cy + pos.y)  ||  IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev)) {
				droplist.infowin_event(&ev2);
				// we selected something?
				if(finish  &&  IS_LEFTRELEASE(ev)) {
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
	} else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_CLOSE  ||  ev->ev_code==WIN_UNTOP)  ) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","close");
		textinp.infowin_event(ev);
		droplist.set_visible(false);
		close_box();
		// update "mouse-click-catch-area"
		set_groesse(koord(groesse.x, droplist.is_visible() ? max_size.y : 14));
	}
	else {
		// finally handle textinput
		gui_scrolled_list_t::scrollitem_t *item = droplist.get_element(droplist.get_selection());
		if(  item==NULL  ||  item->is_editable()) {
			event_t ev2 = *ev;
			translate_event(&ev2, -textinp.get_pos().x, -textinp.get_pos().y);
			return textinp.infowin_event(ev);
		}
	}
	return true;
}



/* selction now handled via callback */
bool gui_combobox_t::action_triggered( gui_action_creator_t *komp,value_t p)
{
	if (komp == &droplist) {
DBG_MESSAGE("gui_combobox_t::infowin_event()","scroll selected %i",p.i);
		finish = true;
		set_selection(p.i);
	}
	else if (komp == &textinp) {
		rename_selected_item();
	}
	return false;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_combobox_t::zeichnen(koord offset)
{
	// text changed? Then update it
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_element( droplist.get_selection() );
	if(  item  &&  item->is_valid()  &&  strncmp(item->get_text(),old_editstr,127)!=0  ) {
		reset_selected_item_name();
	}

	bool with_focus = (win_get_focus()==this)  &&  (item==NULL  ||  item->is_editable());

	textinp.display_with_focus( offset, with_focus);

	if (droplist.is_visible()) {
		droplist.zeichnen(offset);
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
void gui_combobox_t::set_selection(int s)
{
	// try to finish renaming first
	rename_selected_item();

	if (droplist.is_visible()) {
		// visible? change also offset of scrollbar
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
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_element(droplist.get_selection());
	// if name was not changed in the meantime, we can rename it
	if(  item  &&  item->is_valid() &&  item->is_editable() &&  strncmp(item->get_text(),old_editstr,127)==0  &&  strncmp(item->get_text(),editstr,127)) {
		item->set_text(editstr);
	}
}

void gui_combobox_t::reset_selected_item_name()
{
	gui_scrolled_list_t::scrollitem_t *item = droplist.get_element(droplist.get_selection());
	if(  item==NULL  ) {
		editstr[0] = 0;
		textinp.set_text( editstr, 0  );
		droplist.set_selection(-1);
	}
	else if(  item->is_valid()  &&  strncmp(editstr,item->get_text(),127)!=0  ) {
		tstrncpy(editstr, item->get_text(), lengthof(editstr));
		textinp.set_text( editstr, sizeof(editstr));
	}
	tstrncpy(old_editstr, editstr, sizeof(old_editstr));
}



/**
* Release the focus if we had it
*/
void gui_combobox_t::close_box()
{
	if(finish) {
//DBG_MESSAGE("gui_combobox_t::infowin_event()","prepare selected %i for %d listerners",get_selection(),listeners.get_count());
		value_t p;
		p.i = droplist.get_selection();
		call_listeners(p);
		finish = false;
	}
	droplist.set_visible(false);
	set_groesse(koord(groesse.x, 14));
	first_call = true;
}




void gui_combobox_t::set_groesse(koord gr)
{
	textinp.set_pos( pos+koord(12,0) );
	textinp.set_groesse( koord(gr.x-26,14) );
	bt_next.set_pos( koord(gr.x-12,2) );
	gui_komponente_t::groesse = gr;
}

/**
* set maximum size for control
* @author hsiegeln, Dwachs
*/
void gui_combobox_t::set_max_size(koord max)
{
	max_size = max;
	droplist.request_groesse(koord(this->groesse.x, max_size.y - 16));
	if (droplist.is_visible()) {
		set_groesse(droplist.get_groesse() + koord(0, 16));
	}
}
