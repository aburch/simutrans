/* scrolled_list.cc
 *
 * Scrolled List.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between
 *
 */

#include <stdio.h>

#include "gui_scrollbar.h"
#include "gui_scrolled_list.h"

#include "../../simworld.h"
#include "../../simgraph.h"
#include "../../simcolor.h"

#include "../../tpl/slist_tpl.h"


int gui_scrolled_list_t::total_vertical_size() const
{
  return item_list.count() * LINESPACE + 2;
}


gui_scrolled_list_t::gui_scrolled_list_t(enum type type) :
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
	sb.setze_opaque(true);

	clear_elements();
}



bool
gui_scrolled_list_t::action_triggered(gui_komponente_t * /* comp */, value_t extra)
{
    // search/replace all offsets with sb.gib_offset() is also an option
    offset = extra.i;
    return true;
}



// set the scrollbar offset, so that the selected itm is visible
void
gui_scrolled_list_t::show_selection(int s)
{
	if((unsigned)s<item_list.count()) {
		selection = s;
//		if(groesse.y>0) {
DBG_MESSAGE("gui_scrolled_list_t::show_selection()","sel=%d, offset=%d, groesse.y=%d",s,offset,groesse.y);
			s *= LINESPACE;
			if(s<offset  ||  (s+LINESPACE)>offset+groesse.y) {
				// outside range => reposition
				sb.setze_knob_offset( max(0,s-(groesse.y/2) ) );
				offset = sb.gib_knob_offset();
			}
//		}
	}
	else {
		selection = -1;
	}
}


void gui_scrolled_list_t::clear_elements()
{
    item_list.clear();
    sb.setze_knob(groesse.y-border, total_vertical_size());
}


void gui_scrolled_list_t::insert_element(const char *string, const int pos /*= 0*/, const uint8 color /*COL_BLACK*/)
{
	item it={ string, color };
	item_list.insert(it, pos);
	sb.setze_knob(groesse.y-border, total_vertical_size());
}


void gui_scrolled_list_t::append_element(const char *string,const uint8 color)
{
	item it={ string, color };
	item_list.append(it);
	sb.setze_knob(groesse.y-border, total_vertical_size());
}


void gui_scrolled_list_t::remove_element(const int pos)
{
	item_list.remove_at(pos);
	sb.setze_knob(groesse.y-border, total_vertical_size());
}


// no less than 3, must be room for scrollbuttons
#define YMIN ((LINESPACE*3)+2)
koord gui_scrolled_list_t::request_groesse(koord request)
{
	koord groesse = gib_groesse();

	groesse.x = request.x;
	int y = request.y;
	int vz = total_vertical_size();
	// if y is too large, but smaller than current, accept it.
	if (y > vz && y > (groesse.y - border)) {
		if (vz > groesse.y - border) {
			y = vz;
		}
		else {
			y = groesse.y - border;
		}
	}

	if (y < YMIN) {
		y = YMIN;
	}

	groesse.y = y + border;

	setze_groesse( groesse );

	sb.setze_pos(koord(groesse.x-12,0));
	sb.setze_groesse(koord(10, (int)groesse.y+border));
	sb.setze_knob(groesse.y-border, total_vertical_size());

	return groesse;
}


void
gui_scrolled_list_t::infowin_event(const event_t *ev)
{
	const int x = ev->cx;
	const int y = ev->cy;

	// size without scrollbar
	const int w = groesse.x - 13;
	const int h = groesse.y;
	if(x <= w) { // inside list
		switch(type) {
			case list:
				break;
			case select:
				if(  IS_LEFTCLICK(ev)  &&  x>=(border/2) && x<(w-border/2) &&  y>=(border/2) && y<(h-border/2)) {
					selection = (y-(border/2)-2+offset);
					if(selection>=0) {
						selection/=LINESPACE;
						if((unsigned)selection>=item_list.count()) {
							selection = -1;
						}
						DBG_MESSAGE("gui_scrolled_list_t::infowin_event()","selected %i",selection);
					}
					call_listeners((long)selection);
				}
				break;
		}
	}

    if (sb.getroffen(x, y)) {
		event_t ev2 = *ev;
		translate_event(&ev2, -sb.gib_pos().x, -sb.gib_pos().y);
		sb.infowin_event(&ev2);
    }
}

void gui_scrolled_list_t::zeichnen(koord pos) const
{
	pos += this->pos;

	const koord gr = gib_groesse();

	const int x = pos.x;
	const int y = pos.y;
	const int w = gr.x-13;
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
	int ycum = y+4-offset; // y cumulative
	int i=0;
	slist_iterator_tpl<item>iter( item_list );
	while( iter.next() ) {
		if (iter.get_current().text) {
			if (i == selection) {
				// the selection is grey on color
				display_fillbox_wh_clip(x+3, ycum-1, w-5, 11, highlight_color, true);
				display_proportional_clip(x+7, ycum, iter.get_current().text, ALIGN_LEFT, MN_GREY3, true);
			}
			else {
				// normal text
				display_proportional_clip(x+7, ycum, iter.get_current().text, ALIGN_LEFT, iter.get_current().color, true);
			}
			ycum += 11;
			i++;
		}
	}
	POP_CLIP();

	sb.zeichnen(pos);
}
