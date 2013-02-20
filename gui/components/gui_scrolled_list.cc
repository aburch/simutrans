/*
 * Scrolled List.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between
 */

#include <stdio.h>

#include "gui_scrollbar.h"
#include "gui_scrolled_list.h"

#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"


int gui_scrolled_list_t::total_vertical_size() const
{
	return item_list.get_count() * LINESPACE + 2;
}


gui_scrolled_list_t::gui_scrolled_list_t(enum type type) :
	gui_komponente_t(true),
	sb(scrollbar_t::vertical)
{
	this->type = type;
	selection = -1; // nothing
	groesse = koord(0,0);
	pos = koord(0,0);
	offset = 0;
	border = 0;
	if (type==select) {
		border = 2;
	}
	else if (type==list) {
		border = 4;
	}
	sb.add_listener(this);
	sb.set_knob_offset(0);

	clear_elements();
}


bool gui_scrolled_list_t::action_triggered( gui_action_creator_t * /* comp */, value_t extra)
{
	// search/replace all offsets with sb.get_offset() is also an option
	offset = extra.i;
	return true;
}


// set the scrollbar offset, so that the selected itm is visible
void gui_scrolled_list_t::show_selection(int s)
{
	if((unsigned)s<item_list.get_count()) {
		selection = s;
DBG_MESSAGE("gui_scrolled_list_t::show_selection()","sel=%d, offset=%d, groesse.y=%d",s,offset,groesse.y);
		s *= LINESPACE;
		if(s<offset  ||  (s+LINESPACE)>offset+groesse.y) {
			// outside range => reposition
			sb.set_knob_offset( max(0,s-(groesse.y/2) ) );
			offset = sb.get_knob_offset();
		}
	}
	else {
		selection = -1;
	}
}


void gui_scrolled_list_t::clear_elements()
{
	while(  !item_list.empty()  ) {
		delete item_list.remove_first();
	}
	adjust_scrollbar();
}


void gui_scrolled_list_t::append_element( scrollitem_t *item )
{
	item_list.append( item );
	adjust_scrollbar();
}

// minimum vertical size
// no less than 3, must be room for scrollbuttons
#define YMIN ((LINESPACE*3)+2)

// sets size: if requested is too large, then the size is adjusted
// use this only for variable sized lists
koord gui_scrolled_list_t::request_groesse(koord request)
{
	koord groesse = get_groesse();

	groesse.x = request.x;
	int y = request.y;
	int vz = total_vertical_size();

	if (y > vz) {
		y = vz;
	}

	if (y < YMIN) {
		y = YMIN;
	}

	groesse.y = y + border;

	set_groesse( groesse );

	return groesse;
}


void gui_scrolled_list_t::set_groesse(koord groesse)
{
	gui_komponente_t::set_groesse(groesse);
	adjust_scrollbar();
}


/* resizes scrollbar */
void gui_scrolled_list_t::adjust_scrollbar()
{
	sb.set_pos(koord(groesse.x-scrollbar_t::BAR_SIZE,0));

	int vz = total_vertical_size();
	// need scrollbar?
	if ( groesse.y-border < vz) {
		sb.set_visible(true);
		sb.set_groesse(koord(scrollbar_t::BAR_SIZE, (int)groesse.y+border-1));
		sb.set_knob(groesse.y-border, vz);
	}
	else {
		sb.set_visible(false);
	}
}


bool gui_scrolled_list_t::infowin_event(const event_t *ev)
{
	const int x = ev->cx;
	const int y = ev->cy;

	// size without scrollbar
	const int w = groesse.x - scrollbar_t::BAR_SIZE+2;
	const int h = groesse.y;
	if(x <= w) { // inside list
		switch(type) {
			case list:
				break;
			case select:
				if(  IS_LEFTCLICK(ev)  &&  x>=(border/2) && x<(w-border/2) &&  y>=(border/2) && y<(h-border/2)) {
					int new_selection = (y-(border/2)-2+offset);
					if(new_selection>=0) {
						new_selection/=LINESPACE;
						if((unsigned)new_selection>=item_list.get_count()) {
							new_selection = -1;
						}
						DBG_MESSAGE("gui_scrolled_list_t::infowin_event()","selected %i",selection);
					}
					selection = new_selection;
					call_listeners((long)new_selection);
				}
				break;
		}
	}

	// goto next/previous choice
	if(  ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code==SIM_KEY_UP  ||  ev->ev_code==SIM_KEY_DOWN)  ) {
		int new_selection = (ev->ev_code==SIM_KEY_DOWN) ? min(item_list.get_count()-1, selection+1) : max(0, selection-1);
		selection = new_selection;
		show_selection(selection);
		call_listeners((long)new_selection);
		return true;
	}

	if(  sb.is_visible()  &&  (sb.getroffen(x, y)  ||  IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))  ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -sb.get_pos().x, -sb.get_pos().y);
		return sb.infowin_event(&ev2);
	}

	return false;
}


void gui_scrolled_list_t::zeichnen(koord pos)
{
	pos += this->pos;

	const koord gr = get_groesse();

	const int x = pos.x;
	const int y = pos.y;
	const int w = gr.x-scrollbar_t::BAR_SIZE;
	const int h = gr.y;

	switch(type) {
		case list:
			break;
		case select:
			display_vline_wh(x, y+1, h-1, MN_GREY0, true);
			display_fillbox_wh(x,y,w,1, MN_GREY0, true);
			display_vline_wh(x+w-1, y+1, h-2, MN_GREY4, true);
			display_fillbox_wh(x+1,y+h-1,w-1,1, MN_GREY4, true);
			display_fillbox_wh(x+1,y+1,w-2,h-2, MN_GREY3, true);
			break;
	}

	display_fillbox_wh(x,y,w,h, MN_GREY3, true);
	display_ddd_box(x,y-1,w,h+2, COL_BLACK, COL_WHITE);

	PUSH_CLIP(x+1,y+1,w-2,h-2);
	int ycum = y+2-offset; // y cumulative
	int i=0;
	for (slist_tpl<scrollitem_t*>::iterator iter = item_list.begin(), end = item_list.end(); iter != end;) {
		scrollitem_t* const item = *iter;
		if(  !item->is_valid()  ) {
			iter = item_list.erase(iter);
			delete item;
			if(i == selection) {
				selection = -1;
			}
			else if(  i<selection  ) {
				selection --;
			}
		}
		else {
			if(i == selection) {
				// the selection is grey on color
				display_fillbox_wh_clip(x+3, ycum-1, w-5, LINESPACE, highlight_color, true);
				display_proportional_clip(x+7, ycum, item->get_text(), ALIGN_LEFT, (win_get_focus()==this ? COL_WHITE : MN_GREY3), true);
			}
			else {
				// normal text
				display_proportional_clip(x+7, ycum, item->get_text(), ALIGN_LEFT, item->get_color(), true);
			}
			ycum += LINESPACE;
			++iter;
			i++;
		}
	}
	POP_CLIP();

	if (sb.is_visible()) {
		sb.zeichnen(pos);
	}
}
