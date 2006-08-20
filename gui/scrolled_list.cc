/* scrolled_list.cc
 *
 * Scrolled List.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between
 *
 */

#include <stdio.h>

#include "scrollbar.h"
#include "scrolled_list.h"

#include "../simworld.h"
#include "../simgraph.h"
#include "../simcolor.h"

#include "../tpl/slist_tpl.h"


/**
 * Adds a selection_listener_t to this component
 * @author Hj. Malthaner
 */
void scrolled_list_gui_t::add_listener(selection_listener_t *c) {
  listeners->insert(c);
};


/**
 * Removes a selection_listener_t from this component
 * @author Hj. Malthaner
 */
void scrolled_list_gui_t::remove_listener(selection_listener_t *c) {
  listeners->remove(c);
};


/**
 * Informs (calls) all listeners about a new selection
 * @author Hj. Malthaner
 */
void scrolled_list_gui_t::call_listeners()
{
    slist_iterator_tpl<selection_listener_t *> iter( listeners );
    while( iter.next() ) {
	iter.get_current()->eintrag_gewaehlt(this, selection);
    }
}


int scrolled_list_gui_t::total_vertical_size() const
{
  return item_list->count() * 11 + 4;
}


scrolled_list_gui_t::scrolled_list_gui_t(enum type type)
{
  listeners = new slist_tpl <selection_listener_t *>;
  item_list = new slist_tpl <const char *>;

    this->type = type;
    selection = -1; // nothing
    groesse = koord(0,0);
    pos = koord(0,0);
    offset = 0;
    border = 0;
    if (type==select) {
	border = 2;
    } else if (type==list) {
	border = 4;
    }
    sb = new scrollbar_t(scrollbar_t::vertical);
    sb->add_listener(this);
    sb->setze_opaque(true);

    clear_elements();
}

scrolled_list_gui_t::~scrolled_list_gui_t()
{
  delete listeners;
  listeners = 0;

  delete item_list;
  item_list = 0;
}


void scrolled_list_gui_t::scrollbar_moved(class scrollbar_t *,
					  int, int value)
{
    // search/replace all offsets with sb->gib_offset() is also an option
    offset = value;
}


void scrolled_list_gui_t::clear_elements()
{
    item_list->clear();
    sb->setze_knob(groesse.y-border, total_vertical_size());
}


void scrolled_list_gui_t::insert_element(const char *string, const int pos /*= 0*/)
{
    item_list->insert(string, pos);
    sb->setze_knob(groesse.y-border, total_vertical_size());
}


void scrolled_list_gui_t::append_element(const char *string)
{
    item_list->append(string);
    sb->setze_knob(groesse.y-border, total_vertical_size());
}


void scrolled_list_gui_t::remove_element(const int pos)
{
    item_list->remove_at(pos);
    sb->setze_knob(groesse.y-border, total_vertical_size());
}


// no less than 3, must be room for scrollbuttons
#define YMIN ((11*3)+4)
koord scrolled_list_gui_t::request_groesse(koord request)
{
    koord groesse = gib_groesse();

    groesse.x = request.x;
    int y = request.y;
    int vz = total_vertical_size();
    // if y is too large, but smaller than current, accept it.
    if (y > vz && y > (groesse.y - border)) {
	if (vz > groesse.y - border) {
	    y = vz;
	} else {
	    y = groesse.y - border;
	}
    }

    if (y < YMIN) {
	y = YMIN;
    }

    groesse.y = y + border;

    setze_groesse( groesse );

    sb->setze_pos(koord(groesse.x-10,0));
    sb->setze_groesse(koord(10, (int)groesse.y));
    sb->setze_knob(groesse.y-border, total_vertical_size());

    return groesse;
}


void
scrolled_list_gui_t::infowin_event(const event_t *ev)
{
    const int x = ev->cx;
    const int y = ev->cy;

    // size without scrollbar
    const int w = groesse.x - 13;
    const int h = groesse.y;
    if (x <= w) { // inside list
	switch(type) {
	case list:
	    break;
	case select:
	    if (IS_LEFTCLICK(ev) &&
		x>=(border/2) && x<(w-border/2) &&
		y>=(border/2) && y<(h-border/2)) {
		selection = (y-(border/2)-2+offset);
		if (selection>=0) {
		    selection/=11;
		}

		call_listeners();
	    }
	    break;
	}
    }

    if (sb->getroffen(x, y)) {
	event_t ev2 = *ev;
	translate_event(&ev2, -sb->gib_pos().x, -sb->gib_pos().y);
	sb->infowin_event(&ev2);
    }
}

void scrolled_list_gui_t::zeichnen(koord pos) const
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
  slist_iterator_tpl<const char *> iter( item_list );
  while( iter.next() ) {
    if (iter.get_current()) {
      if (i == selection) { // the selection is grey on color
	display_fillbox_wh_clip(x+3, ycum-1, w-5, 11,
			        highlight_color, true);
	display_proportional_clip(x+7, ycum, iter.get_current(),
				  ALIGN_LEFT, MN_GREY3, true);
      } else { // normal text is just black
	display_proportional_clip(x+7, ycum, iter.get_current(),
				  ALIGN_LEFT, COL_BLACK, true);
      }
      ycum += 11; i++;
    }
  }
  POP_CLIP();


  sb->zeichnen(pos);
}
