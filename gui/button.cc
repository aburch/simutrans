/*
 * gui_komponente.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "button.h"

#include "../simdebug.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simevent.h"
#include "../simwin.h"
#include "../dataobj/translator.h"

#include "../simskin.h"
#include "../besch/skin_besch.h"

#include "ifc/action_listener.h"

#include "../tpl/slist_tpl.h"


/*
 * Hajo: image numbers of button skins
 */
static int square_button_pushed = -1;
static int square_button_normal = -1;
static int arrow_left_pushed = -1;
static int arrow_left_normal = -1;
static int arrow_right_pushed = -1;
static int arrow_right_normal = -1;
static int arrow_up_pushed = -1;
static int arrow_up_normal = -1;
static int arrow_down_pushed = -1;
static int arrow_down_normal = -1;


static int b_cap_left = -1;
static int b_body = -1;
static int b_cap_right = -1;

static int b_cap_left_p = -1;
static int b_body_p = -1;
static int b_cap_right_p = -1;


/**
 * Lazy button image number init
 * @author Hj. Malthaner
 */
static void init_button_images()
{
  if(square_button_pushed == -1) {

    square_button_normal =
      skinverwaltung_t::window_skin->gib_bild(6)->gib_nummer();
    square_button_pushed =
      skinverwaltung_t::window_skin->gib_bild(7)->gib_nummer();

    arrow_left_normal =
      skinverwaltung_t::window_skin->gib_bild(8)->gib_nummer();
    arrow_left_pushed =
      skinverwaltung_t::window_skin->gib_bild(9)->gib_nummer();

    arrow_right_normal =
      skinverwaltung_t::window_skin->gib_bild(10)->gib_nummer();
    arrow_right_pushed =
      skinverwaltung_t::window_skin->gib_bild(11)->gib_nummer();

    b_cap_left =
      skinverwaltung_t::window_skin->gib_bild(12)->gib_nummer();
    b_cap_right =
      skinverwaltung_t::window_skin->gib_bild(13)->gib_nummer();
    b_body =
      skinverwaltung_t::window_skin->gib_bild(14)->gib_nummer();

    b_cap_left_p =
      skinverwaltung_t::window_skin->gib_bild(15)->gib_nummer();
    b_cap_right_p =
      skinverwaltung_t::window_skin->gib_bild(16)->gib_nummer();
    b_body_p =
      skinverwaltung_t::window_skin->gib_bild(17)->gib_nummer();


    arrow_up_normal =
      skinverwaltung_t::window_skin->gib_bild(18)->gib_nummer();
    arrow_up_pushed =
      skinverwaltung_t::window_skin->gib_bild(19)->gib_nummer();

    arrow_down_normal =
      skinverwaltung_t::window_skin->gib_bild(20)->gib_nummer();
    arrow_down_pushed =
      skinverwaltung_t::window_skin->gib_bild(21)->gib_nummer();
  }
}


/**
 * Displays the different button types
 * @author Hj. Malthaner
 */
static void display_button_image(int x, int y, int number, bool pushed)
{
  int button = 0;

  switch (number) {
  case SQUARE_BUTTON:
    if (pushed)
      button = square_button_pushed;
    else
      button = square_button_normal;
    break;
  case ARROW_LEFT:
    if (pushed)
      button = arrow_left_pushed;
    else
      button = arrow_left_normal;
    break;
  case ARROW_RIGHT:
    if (pushed)
      button = arrow_right_pushed;
    else
      button = arrow_right_normal;
    break;
  case ARROW_UP:
    if (pushed)
      button = arrow_up_pushed;
    else
      button = arrow_up_normal;
    break;
  case ARROW_DOWN:
    if (pushed)
      button = arrow_down_pushed;
    else
      button = arrow_down_normal;
    break;
  }

  display_color_img(button, x, y, 0, false, false);
}


/**
 * Add a new listener to this button.
 * @author Hj. Malthaner
 */
void button_t::add_listener(action_listener_t * l) {
  listeners->insert( l );
}


/**
 * Inform all listeners that an action was triggered.
 * @author Hj. Malthaner
 */
void button_t::call_listeners()
{
  // printf("Message: button_t::call_listeners()\n");

  slist_iterator_tpl<action_listener_t *> iter (listeners);

  while(iter.next() && !iter.get_current()->action_triggered(this))
    ;
}


button_t::button_t()
{
  text = 0;
  pressed = false;
  type = box;
  kennfarbe = 0;
  tooltip = 0;
  background = MN_GREY3;
	b_enabled = true;

  listeners = new slist_tpl < action_listener_t * >;

  init_button_images();
}


button_t::button_t(const button_t & other) : gui_komponente_t(other)
{
  *this = other;
}


button_t::~button_t()
{
  delete listeners;
  listeners = 0;
}


void button_t::init(enum type typ, const char *text, koord pos, koord size)
{
  setze_typ(typ);
  setze_text(translator::translate(text));
  setze_pos(pos);
  if(size != koord::invalid) {
    setze_groesse(size);
  }
}


// set type. this includes size for specified buttons.
void button_t::setze_typ(enum type t)
{
  type = t;
  switch (type) {
  case square:
  case arrowleft:
  case arrowright:
  case arrowup:
  case arrowdown:
    groesse.x = 10;
    groesse.y = 10;
    break;
  case roundbox:
    groesse.y = 14;
    break;
  default:
    break;
  }
}


/**
 * Sets the tooltip of this button
 * @author Hj. Malthaner
 */
void button_t::set_tooltip(const char * t)
{
  tooltip = t;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void button_t::infowin_event(const event_t *ev)
{
  // printf("Message: button_t::infowin_event(): class=%d buttons=%x\n", ev->ev_class, ev->button_state);

  // Hajo: we ignore resize events, they shouldn't make us
  // pressed or upressed

  if(IS_WINDOW_RESIZE(ev)) {
    return;
  }

  // Hajo: check button state, if we should look depressed

  pressed = (ev->button_state == 1) && enabled();

  if(IS_LEFTRELEASE(ev)) {
    call_listeners();
  }
}


// draw button. x,y is top left of window.
void button_t::zeichnen(koord offset, int button_color) const
{
  int bx = offset.x + pos.x;
  int by = offset.y + pos.y;

  int bw = groesse.x;
  int bh = groesse.y;

  switch (type) {
   case box: // old, 4-line box
    if (pressed) {
      display_ddd_box_clip(bx, by, bw, bh, MN_GREY0, MN_GREY4);
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, background, false);
    } else {
      display_ddd_box_clip(bx, by, bw, bh, GRAU6, GRAU3);
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, background, false);
    }
    display_proportional_clip(bx+4,by+(bh-large_font_height)/2, text, ALIGN_LEFT, b_enabled ? button_color : GRAU4, true);
    break;
   case roundbox: // new box with round corners
#if 1
#if 1
	if (pressed) {
		display_fillbox_wh_clip(bx, by, bw, 1, MN_GREY1, true);
		display_fillbox_wh_clip(bx+1, by+1, bw-2, 1, SCHWARZ, true);
		display_fillbox_wh_clip(bx+2, by+2, bw-2, bh-4, MN_GREY1, true);
		display_fillbox_wh_clip(bx, by+bh-2, bw, 1, MN_GREY3, true);
		display_fillbox_wh_clip(bx, by+bh-1, bw, 1, WEISS, true);
		display_vline_wh_clip(bx+bw-2, by+1, bh-2, MN_GREY4, true);
		display_vline_wh_clip(bx+bw-1, by+1, bh-1, WEISS, true);
		display_vline_wh_clip(bx, by, bh, MN_GREY1, true);
		display_vline_wh_clip(bx+1, by+1, bh-2, SCHWARZ, true);
	}
	else {
		display_fillbox_wh_clip(bx, by, bw, 1, WEISS, true);
		display_fillbox_wh_clip(bx+1, by+1, bw-2, 1, MN_GREY4, true);
		display_fillbox_wh_clip(bx+2, by+2, bw-2, bh-4, MN_GREY3, true);
		display_fillbox_wh_clip(bx, by+bh-2, bw, 1, MN_GREY1, true);
		display_fillbox_wh_clip(bx, by+bh-1, bw, 1, SCHWARZ, true);
		display_vline_wh_clip(bx+bw-2, by+1, bh-2, MN_GREY1, true);
		display_vline_wh_clip(bx+bw-1, by+1, bh-1, SCHWARZ, true);
		display_vline_wh_clip(bx, by, bh, WEISS, true);
		display_vline_wh_clip(bx+1, by+1, bh-2, MN_GREY4, true);
	}
#else
    // not exactly round, but heck. new 3d-look.
    PUSH_CLIP(bx, by, bw, bh+1);
    mark_rect_dirty_wc(bx, by, bx+bw-1, by+bh-1);
    if (pressed) {

      display_color_img(b_cap_left_p, 0, bx, by, false, false);
      for(int x=4; x<bw-4; x+=64) {
	display_color_img(b_body_p, bx+x, by, 0, false, false);
      }
      display_color_img(b_cap_right_p, bx+bw-4, by, 0, false, false);

    } else {

      display_color_img(b_cap_left, bx, by, 0, false, false);
      for(int x=4; x<bw-4; x+=64) {
	display_color_img(b_body, bx+x, by, 0, false, false);
      }
      display_color_img(b_cap_right, bx+bw-4, by, 0, false, false);

    }
    POP_CLIP();
#endif
#else
    // original roundbox
//    PUSH_CLIP(bx, by, bw, bh);
    if (pressed) {
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, MN_GREY1, false);

      display_fillbox_wh_clip(bx+2, by,      bw-4, 1, MN_GREY0, true);
      display_fillbox_wh_clip(bx+2, by+bh-1, bw-4, 1, MN_GREY4, true);
      display_vline_wh_clip(bx,      by+2, bh-4, MN_GREY0, true);
      display_vline_wh_clip(bx+bw-1, by+2, bh-4, MN_GREY4, true);
      display_pixel(bx+1,    by+1,    MN_GREY0);
      display_pixel(bx+1,    by+bh-2, MN_GREY0);
      display_pixel(bx+bw-2, by+1,    MN_GREY0);
      display_pixel(bx+bw-2, by+bh-2, MN_GREY4);
    } else {
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, MN_GREY3, false);

      display_fillbox_wh_clip(bx+2, by,      bw-4, 1, MN_GREY4, true);
      display_fillbox_wh_clip(bx+2, by+bh-1, bw-4, 1, MN_GREY0, true);
      display_vline_wh_clip(bx,      by+2, bh-4, MN_GREY4, true);
      display_vline_wh_clip(bx+bw-1, by+2, bh-4, MN_GREY0, true);
      display_pixel(bx+1,    by+1,    MN_GREY4);
      display_pixel(bx+1,    by+bh-2, MN_GREY1);
      display_pixel(bx+bw-2, by+1,    MN_GREY1);
      display_pixel(bx+bw-2, by+bh-2, MN_GREY1);
    }
    //POP_CLIP();
#endif
    display_proportional_clip(bx+(bw>>1),by+(bh-large_font_height)/2, text, ALIGN_MIDDLE, b_enabled ? button_color : GRAU4, true);
    break;

   case square: // little square in front of text
    display_button_image(bx, by, SQUARE_BUTTON, pressed);
    display_proportional_clip(bx+16,by+(12-large_font_height)/2, text, ALIGN_LEFT, b_enabled ? button_color : GRAU4, false);
    break;

   case arrowleft:
    display_button_image(bx, by, ARROW_LEFT, pressed);
    break;
   case arrowright:
    display_button_image(bx, by, ARROW_RIGHT, pressed);
    break;

   case arrowup:
    display_button_image(bx, by, ARROW_UP, pressed);
    break;
   case arrowdown:
    display_button_image(bx, by, ARROW_DOWN, pressed);
    break;

   case scrollbar:
#if 1
    // new 3d-look scrollbar knob
//    PUSH_CLIP(bx, by, bw, bh);
    mark_rect_dirty_wc(bx, by, bx+bw-1, by+bh-1);
    if (pressed) {
      display_fillbox_wh_clip(bx+2, by+2, bw-3, bh-3, MN_GREY1, false);
      display_vline_wh_clip  (bx+2, by+3, 2,   MN_GREY0, false);
      display_fillbox_wh_clip(bx+2, by+2, 3,1, MN_GREY0, false);
      display_vline_wh_clip  (bx+1, by+2, bh-3,   BLACK, false);
      display_fillbox_wh_clip(bx+1, by+1, bw-2,1, BLACK, false);
      display_vline_wh_clip  (bx+bw-2, by+3, bh-5,   MN_GREY2, false);
      display_fillbox_wh_clip(bx+3, by+bh-2, bw-4,1, MN_GREY2, false);
      display_vline_wh_clip  (bx, by+1, bh-2, MN_GREY0, false);
      display_fillbox_wh_clip(bx, by, bw-1,1, MN_GREY0, false);
      display_vline_wh_clip  (bx+bw-1, by, bh,   WHITE, false);
      display_fillbox_wh_clip(bx, by+bh-1, bw,1, WHITE, false);
    } else {
      display_fillbox_wh_clip(bx+1, by+1, bw-3, bh-3, MN_GREY3, false);
      display_vline_wh_clip  (bx+bw-3, by+bh-5, 2,   MN_GREY1, false);
      display_fillbox_wh_clip(bx+bw-5, by+bh-3, 3,1, MN_GREY1, false);
      display_vline_wh_clip  (bx+1, by+2, bh-5,   MN_GREY4, false);
      display_fillbox_wh_clip(bx+1, by+1, bw-4,1, MN_GREY4, false);
      display_vline_wh_clip  (bx+bw-2, by+1, bh-3,   MN_GREY0, false);
      display_fillbox_wh_clip(bx+1, by+bh-2, bw-2,1, MN_GREY0, false);
      display_vline_wh_clip  (bx, by+1, bh-2, WHITE, false);
      display_fillbox_wh_clip(bx, by, bw-1,1, WHITE, false);
      display_vline_wh_clip  (bx+bw-1, by, bh,   BLACK, false);
      display_fillbox_wh_clip(bx, by+bh-1, bw,1, BLACK, false);
    }
//    POP_CLIP();
#else
    // original scrollbar knob
    if (pressed) {
      display_fillbox_wh_clip(bx, by, bw, 1, MN_GREY0, false);
      display_vline_wh_clip(bx, by+1, bh-2, MN_GREY0, true);
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, MN_GREY1, false);
      display_fillbox_wh_clip(bx, by+bh-1, bw, 1, MN_GREY4, false);
      display_vline_wh(bx+bw-1, by, bh-1, MN_GREY4, true);
    } else {
      display_fillbox_wh_clip(bx, by, bw, 1, MN_GREY4, false);
      display_vline_wh_clip(bx, by+1, bh-2, MN_GREY4, true);
      display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, MN_GREY3, false);
      display_fillbox_wh_clip(bx, by+bh-1, bw, 1, MN_GREY1, false);
      display_vline_wh_clip(bx+bw-1, by, bh-1, MN_GREY1, true);
    }
#endif
    break;
  }
}


void button_t::zeichnen(koord offset) const
{
  zeichnen(offset, SCHWARZ);

  if(tooltip &&
     gib_maus_x() >= offset.x + pos.x &&
     gib_maus_y() >= offset.y + pos.y &&
     gib_maus_x() <  offset.x + pos.x + groesse.x &&
     gib_maus_y() <  offset.y + pos.y + groesse.y)
  {

    win_set_tooltip(offset.x + pos.x + 16,
		    offset.y + pos.y - 16,
		    tooltip
		    );
  }
}


void button_t::operator= (const button_t & other)
{
  set_visible(other.is_visible());
  pos = other.pos;
  groesse = other.groesse;

  text = other.text;
  pressed = other.pressed;
  type = other.type;
  kennfarbe = other.kennfarbe;
  tooltip = other.tooltip;
  background = other.background;

  listeners = new slist_tpl < action_listener_t * >;

  slist_iterator_tpl<action_listener_t *> iter (other.listeners);

  while( iter.next() ) {
    listeners->append(iter.get_current());
  }

  init_button_images();
}
