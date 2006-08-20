/*
 * colors.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "colors.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
// #include "../utils/cbuffer_t.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simdisplay.h"


color_gui_t::color_gui_t(karte_t *welt)
 : infowin_t(welt), buttons(10)
{
  // init buttons
  button_t button_def;

  button_def.setze_pos( koord(95,26) );
  button_def.setze_typ(button_t::arrowleft);
  buttons.append(button_def);

  button_def.setze_pos( koord(121,26) );
  button_def.setze_typ(button_t::arrowright);
  buttons.append(button_def);

  button_def.setze_pos( koord(95,38) );
  button_def.setze_typ(button_t::arrowleft);
  buttons.append(button_def);

  button_def.setze_pos( koord(121,38) );
  button_def.setze_typ(button_t::arrowright);
  buttons.append(button_def);

  button_def.setze_pos( koord(95,50) );
  button_def.setze_typ(button_t::arrowleft);
  buttons.append(button_def);

  button_def.setze_pos( koord(121,50) );
  button_def.setze_typ(button_t::arrowright);
  buttons.append(button_def);

  button_def.setze_pos( koord(10,74) );
  button_def.setze_typ(button_t::square);
  button_def.text = translator::translate("4LIGHT_CHOOSE");
  buttons.append(button_def);

  button_def.setze_pos( koord(10,89) );
  button_def.setze_typ(button_t::square);
  button_def.text = translator::translate("5LIGHT_CHOOSE");
  buttons.append(button_def);

  button_def.setze_pos( koord(10,104) );
  button_def.setze_typ(button_t::square);
  button_def.text = translator::translate("8WORLD_CHOOSE");
  buttons.append(button_def);
}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char * color_gui_t::gib_hilfe_datei() const
{
  return "display.txt";
}



const char *
color_gui_t::gib_name() const
{
    return translator::translate("Helligk. u. Farben");
}


int color_gui_t::bild() const
{
    return IMG_LEER;
}


void color_gui_t::info(cbuffer_t &) const
{
}


koord color_gui_t::gib_fenstergroesse() const
{
    return koord(170, 180);
}


void color_gui_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev) || IS_LEFTREPEAT(ev)) {

	const einstellungen_t * sets = welt->gib_einstellungen();

	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	    display_set_light(display_get_light()-1);
	    welt->setze_dirty();

	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	  if(display_get_light() < 0) {
	    display_set_light(display_get_light()+1);
	  }
	  welt->setze_dirty();

	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	  if(display_get_light() > -8) {
	    display_set_color(display_get_color()-1);
	  }
	  welt->setze_dirty();

	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	    display_set_color(display_get_color()+1);
	    welt->setze_dirty();

	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_scroll_multi() > 1) {
		welt->setze_scroll_multi(sets->gib_scroll_multi()-1);
	    }
	    if(sets->gib_scroll_multi() < -1) {
		welt->setze_scroll_multi(sets->gib_scroll_multi()+1);
	    }

	} else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_scroll_multi() >= 1) {
		welt->setze_scroll_multi(sets->gib_scroll_multi()+1);
	    }
	    if(sets->gib_scroll_multi() <= -1) {
		welt->setze_scroll_multi(sets->gib_scroll_multi()-1);
	    }

	} else if(buttons.at(6).getroffen(ev->mx, ev->my)) {
	  if (IS_LEFTCLICK(ev)) {
	    welt->setze_scroll_multi(- sets->gib_scroll_multi());
	  }

	} else if(buttons.at(7).getroffen(ev->mx, ev->my)) {
	  if (IS_LEFTCLICK(ev)) {
	    welt->gib_einstellungen()->setze_show_pax( !welt->gib_einstellungen()->gib_show_pax() );
	  }

	} else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {
            umgebung_t::night_shift = !umgebung_t::night_shift;
	}
    }
}

static char *ntos(int number, const char *format)
{
  static char tempstring[64];
  int r;

  if (format) {
    r = sprintf(tempstring, format, number);
  } else {
    r = sprintf(tempstring, "%d", number);
  }
  assert(r<32);

  return tempstring;
}


/**
 *
 * @author Hj. Malthaner
 * @return eine NULL-Terminierte Liste von Buttons fuer das
 * Beobachtungsfenster
 */
vector_tpl<button_t>*
color_gui_t::gib_fensterbuttons()
{
  // berechne variablen anteil der buttons
  buttons.at(6).text = translator::translate("4LIGHT_CHOOSE");
  buttons.at(6).pressed = welt->gib_einstellungen()->gib_scroll_multi() < 0;

  buttons.at(7).text = translator::translate("5LIGHT_CHOOSE");
  buttons.at(7).pressed = welt->gib_einstellungen()->gib_show_pax();

  buttons.at(8).text = translator::translate("8WORLD_CHOOSE");
  buttons.at(8).pressed = umgebung_t::night_shift;

  return &buttons;
}

void color_gui_t::zeichnen(koord pos, koord gr)
{
  const int x = pos.x;
  const int y = pos.y;
  const einstellungen_t * sets = welt->gib_einstellungen();
  char buf[128];


  infowin_t::zeichnen(pos, gr);

  display_divider(x+10, y+122, 149);

  display_proportional(x+10, y+22, translator::translate("1LIGHT_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+113, y+22, ntos(display_get_light(), 0),
		       ALIGN_MIDDLE, SCHWARZ, true);
  display_proportional(x+10, y+34, translator::translate("2LIGHT_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+113, y+34, ntos(display_get_color(), 0),
		       ALIGN_MIDDLE, SCHWARZ, true);
  display_proportional(x+10, y+46, translator::translate("3LIGHT_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+113, y+46, ntos(abs(sets->gib_scroll_multi()), 0),
		       ALIGN_MIDDLE, SCHWARZ, true);

  display_proportional(x+10, y+130, "Frame time:",
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+10, y+140, "Idle:",
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+10, y+150, "FPS:",
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional(x+10, y+160, "Sim:",
		       ALIGN_LEFT, SCHWARZ, true);

  sprintf(buf,"%ld/%ld/%ld ms", get_actual_frame_time(), get_average_frame_time(), get_frame_time());

  display_proportional(x+77, y+130, buf,
		       ALIGN_LEFT, WEISS, true);
  display_proportional(x+37, y+140, ntos(welt->gib_schlaf_zeit(), "%d µs"),
		       ALIGN_LEFT, WEISS, true);
  display_proportional(x+37, y+150, ntos(welt->gib_FPS(), "%d fps"),
		       ALIGN_LEFT, WEISS, true);
  display_proportional(x+37, y+160, ntos(welt->gib_simloops(), "%d loops"),
		       ALIGN_LEFT, WEISS, true);


}
