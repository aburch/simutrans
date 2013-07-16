/*
 * Scrollable list.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between
 * Does ONLY cater for vertical offset (yet).
 * two possible types:
 * -list.      simply lists some items.
 * -selection. is a list, but additionally, one item can be selected.
 * @author Niels Roest, additions by Hj. Malthaner
 */

#include <stdio.h>

#include "gui_scrollbar.h"
#include "gui_scrolled_list.h"

#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"



// draws a single line of text
KOORD_VAL gui_scrolled_list_t::const_text_scrollitem_t::zeichnen( koord pos, KOORD_VAL w, bool selected, bool focus )
{
	if(selected) {
		// the selection is grey on color
		display_fillbox_wh_clip( pos.x+3, pos.y-1, w-5, LINESPACE, COL_BLUE, true);
		return display_proportional_clip( pos.x+7, pos.y, get_text(), ALIGN_LEFT, (focus ? COL_WHITE : MN_GREY3), true);
	}
	else {
		// normal text
		return display_proportional_clip( pos.x+7, pos.y, get_text(), ALIGN_LEFT, get_color(), true);
	}
}


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


// set the scrollbar offset, so that the selected item is visible
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
	for(  uint32 i=0;  i<item_list.get_count();  i++  ) {
		delete item_list[i];
	}
	item_list.clear();
	/*
	while(  !item_list.empty()  ) {
		delete item_list.remove_first();
	}
	*/
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
	sb.set_pos(koord(groesse.x-button_t::gui_scrollbar_size.x,0));

	int vz = total_vertical_size();
	// need scrollbar?
	if ( groesse.y-border < vz) {
		sb.set_visible(true);
		sb.set_groesse(koord(button_t::gui_scrollbar_size.x, (int)groesse.y+border-1));
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
	const int w = groesse.x - button_t::gui_scrollbar_size.x+2;
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
	const int w = gr.x-button_t::gui_scrollbar_size.x;
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
	display_ddd_box(x,y-1,w,h+2, COL_BLACK, COL_WHITE, true);

	PUSH_CLIP(x+1,y+1,w-2,h-2);
	int ycum = y+2-offset; // y cumulative
	int i=0;
	const bool focus = win_get_focus()==this;
	KOORD_VAL max_w = 0;
	for(  vector_tpl<scrollitem_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ) {
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
			KOORD_VAL this_w = item->zeichnen( koord( x, ycum), w, i == selection, focus );
			if(  this_w > max_w  ) {
				max_w = this_w;
			}
			ycum += item->get_h();
			++iter;
			i++;
		}
	}
	POP_CLIP();

	if (sb.is_visible()) {
		sb.zeichnen(pos);
	}
}
