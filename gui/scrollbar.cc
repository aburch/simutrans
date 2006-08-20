/* scrollbar.cc
 *
 * Scrollbar class
 * Niel Roest
 */

#include "../simdebug.h"

#include "scrollbar.h"
#include "ifc/scrollbar_listener.h"

#include "../simcolor.h"
#include "../simgraph.h"


scrollbar_t::scrollbar_t(enum type type)
{
  this->type = type;
  pos = koord(0,0);
  opaque = false;
  knob_offset = 0;
  knob_size = 10;
  knob_area = 20;
  knob_scroll_amount = 11; // equals one line
  callback_list.clear();

  if (type == vertical) {
    groesse = koord(10,40);
    button_def[0].setze_typ(button_t::arrowup);
    button_def[1].setze_typ(button_t::arrowdown);
  } else { // horizontal
    groesse = koord(40,10);
    button_def[0].setze_typ(button_t::arrowleft);
    button_def[1].setze_typ(button_t::arrowright);
  }
  button_def[0].setze_pos(koord(0,0));
  button_def[1].setze_pos(koord(0,0));
  button_def[2].setze_typ(button_t::scrollbar);
  reposition_buttons();

  int i;
  for(i=0; i<3; i++) { buttons[i] = &button_def[i]; }
  buttons[3] = 0;
}


void scrollbar_t::call_callback()
{
    int range = knob_area - knob_size;
    if (range < 0) {
	range = 0;
    }

    slist_iterator_tpl<scrollbar_listener_t *> iter( callback_list );
    while( iter.next() ) {
	iter.get_current()->scrollbar_moved(this, range, knob_offset);
    }
}


void scrollbar_t::setze_groesse(koord groesse)
{
    this->groesse = groesse;
    reposition_buttons();
}


void scrollbar_t::setze_knob(int size, int area)
{
    knob_size = size;
    knob_area = area;
    reposition_buttons();
}


// reset variable position and size values of the three buttons
void scrollbar_t::reposition_buttons()
{
  int area; // area will be actual area knob can move in
  if (type == vertical) { area = groesse.y; }
  else /* horizontal */ { area = groesse.x; }
  area -= 24;

  // check if scrollbar is too low
  if (knob_size + knob_offset > knob_area) {
    knob_offset = knob_area - knob_size;
    if (knob_offset < 0) knob_offset = 0;
    call_callback();
  }

  float ratio = (float)area / (float)knob_area;
  if (knob_area < knob_size) { ratio = (float)area / (float)knob_size; }

  int offset = (int)( (float)knob_offset * ratio +.5 );
  int size   = (int)( (float)knob_size   * ratio +.5 );
  //if (knob_area < knob_size) { offset = 0; }

  if (type == vertical) {
    button_def[1].pos.y = groesse.y-10;
    button_def[2].setze_pos(koord(0,12+offset));
    button_def[2].setze_groesse(koord(10,size));
  } else { // horizontal
    button_def[1].pos.x = groesse.x-10;
    button_def[2].setze_pos(koord(12+offset,0));
    button_def[2].setze_groesse(koord(size,10));
  }
}

// signals slider drag. If slider hits end, returned amount is smaller.
int scrollbar_t::slider_drag(int amount)
{
  int area; // area will be actual area knob can move in
  if (type == vertical) area = groesse.y;
  else /* horizontal */ area = groesse.x;
  area -= 24;

  //printf("am %d\n", amount);

  float ratio = (float)area / (float)knob_area;
  amount = (int)( (float)amount / ratio );
  int proposed_offset = knob_offset + amount;

  int maximum = knob_area - knob_size;
  if (maximum<0) { maximum = 0; } // possible if content is smaller than window

  if (proposed_offset<0) { proposed_offset = 0; }
  if (proposed_offset>maximum) { proposed_offset = maximum; }

  if (proposed_offset != knob_offset) {
    int o;
    knob_offset = proposed_offset;
    call_callback();
    o = real_knob_position();
    reposition_buttons();

    //printf("am2 %d\n", real_knob_position() - o);
    return real_knob_position() - o;
  }
  //printf("am3 0\n");
  return 0;
}
// either arrow buttons is just pressed (or long enough for a repeat event)
void scrollbar_t::button_press(int number)
{
  // the offset can range from 0 to maximum
  int maximum = knob_area - knob_size;
  if (maximum<0) { maximum = 0; } // possible if content is smaller than window

  if (number == 0) {
    knob_offset -= knob_scroll_amount;
    if (knob_offset<0) { knob_offset = 0; }
  } else { // number == 1
    knob_offset += knob_scroll_amount;
    if (knob_offset>maximum) { knob_offset = maximum; }
  }

  call_callback();
  reposition_buttons();
}

void scrollbar_t::space_press(int updown) // 0: scroll up/left, 1: scroll down/right
{
  int maximum = knob_area - knob_size;
  if (maximum<0) { maximum = 0; } // possible if content is smaller than window

  if (updown == 0) {
    knob_offset -= knob_size;
    if (knob_offset<0) { knob_offset = 0; }
  } else { // number == 1
    knob_offset += knob_size;
    if (knob_offset>maximum) { knob_offset = maximum; }
  }

  call_callback();
  reposition_buttons();
}


void scrollbar_t::infowin_event(const event_t *ev)
{
//    printf("Scrollbar_t::infowin_event() at %d,%d\n", ev->cx, ev->cy);


  const int x = ev->cx;
  const int y = ev->cy;
  int i;
  bool b_button_hit = false;

  // 2003-11-04 hsiegeln added wheelsupport
  if (IS_WHEELUP(ev) && (type == vertical)) {
  	DBG_MESSAGE("scrollbar_t::infowin_event", "mousewheelup");
  }
  else if (IS_WHEELDOWN(ev) && (type == vertical)) {
  	DBG_MESSAGE("scrollbar_t::infowin_event", "mousewheeldown");
  }
  else if (IS_LEFTCLICK(ev)) {
    for (i=0;i<3;i++) {
      if (button_def[i].getroffen(x, y)) {
		button_def[i].pressed = true;
		if (i<2) { button_press(i); }
		b_button_hit = true;
      }
    }

	if (!b_button_hit) {
		if (type == vertical) {
			if (y < real_knob_position()+12) {
				space_press(0);
			} else {
				space_press(1);
			}
		} else {
			if (x < real_knob_position()+12) {
				space_press(0);
			} else {
				space_press(1);
			}
		}
	}

  } else if (IS_LEFTREPEAT(ev)) {
    if (button_def[0].getroffen(x, y)) { button_press(0); }
    if (button_def[1].getroffen(x, y)) { button_press(1); }
  } else if (IS_LEFTDRAG(ev)) {
    if (button_def[2].getroffen(x,y)) {
      int delta;

      // Hajo: added vertical/horizontal check
      if(type == vertical) {
		delta = ev->my - ev->cy;
      } else {
      	delta = ev->mx - ev->cx;
      }

      delta = slider_drag(delta);

      // Hajo: added vertical/horizontal check
      if(type == vertical) {
	change_drag_start(0, delta);
      } else {
	change_drag_start(delta, 0);
      }
    }
  } else if (IS_LEFTRELEASE(ev)) {
    for (i=0;i<3;i++) {
      if (button_def[i].getroffen(x, y)) {
	button_def[i].pressed = false;
      }
    }
  }
}


void scrollbar_t::zeichnen(koord pos, int button_farbe) const
{
    pos += this->pos;

    if(opaque) {
    	// if opaque style, display GREY sliding bar backgrounds
    	if (type == vertical) {
			display_fillbox_wh(pos.x, pos.y+12, 10, groesse.y-24, MN_GREY1, true);
		} else {
			display_fillbox_wh(pos.x+12, pos.y, groesse.x-24, 10, MN_GREY1, true);
		}
    }

    int i=0;
	    while(buttons[i] != NULL) {
		buttons[i]->zeichnen(pos, button_farbe);
		i++;
    }
}

void scrollbar_t::zeichnen(koord pos) const
{
    zeichnen(pos, SCHWARZ);
}
