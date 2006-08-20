/*
 * werkzeug_parameter_waehler.cc
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdlib.h>

#include "../simcolor.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simwerkz.h"
#include "../besch/skin_besch.h"

#include "../tpl/minivec_tpl.h"

#include "werkzeug_parameter_waehler.h"


werkzeug_parameter_waehler_t::werkzeug_parameter_waehler_t(karte_t *welt,
							   const char *titel)

{
    this->welt = welt;
    this->titel  = titel;
    this->hilfe_datei = NULL;

    tools = new minivec_tpl <struct werkzeug_t> (32);
}


werkzeug_parameter_waehler_t::~werkzeug_parameter_waehler_t()
{
  delete tools;
}



/**
 * Add a new tool with values and tooltip text.
 * Text must already be translated.
 * @author Hj. Malthaner
 */
void werkzeug_parameter_waehler_t::add_tool(int (* wz1)(spieler_t *, karte_t *, koord),
	      int versatz,
	      int sound_ok,
	      int sound_ko,
	      int icon,
	      int cursor,
	      cstring_t tooltip)
{
  struct werkzeug_t tool;

  tool.has_param = false;
  tool.wzwp = 0;
  tool.wzwop = wz1;
  tool.param.i = 0;
  tool.versatz = versatz;
  tool.sound_ok = sound_ok;
  tool.sound_ko = sound_ko;
  tool.icon = icon;
  tool.cursor = cursor;
  tool.tooltip = tooltip;

  tools->append(tool);
}


/**
 * Add a new tool with parameter values and tooltip text.
 * Text must already be translated.
 * @author Hj. Malthaner
 */
void werkzeug_parameter_waehler_t::add_param_tool(int (* wz1)(spieler_t *, karte_t *, koord, value_t),
		    value_t param,
		    int versatz,
		    int sound_ok,
		    int sound_ko,
		    int icon,
		    int cursor,
		    cstring_t tooltip)
{
  struct werkzeug_t tool;

  tool.has_param = true;
  tool.wzwp = wz1;
  tool.wzwop = 0;
  tool.param = param;
  tool.versatz = versatz;
  tool.sound_ok = sound_ok;
  tool.sound_ko = sound_ko;
  tool.icon = icon;
  tool.cursor = cursor;
  tool.tooltip = tooltip;

  tools->append(tool);
}


int werkzeug_parameter_waehler_t::zeige_info(int magic)
{
  // an mauskoordinate anzeigen
  return create_win(-1, -1, -1, this, w_autodelete, magic);
}


const char *werkzeug_parameter_waehler_t::gib_name() const
{
  return titel;
}


/**
 * @return gibt wunschgroesse für das beobachtungsfenster zurueck
 */
koord werkzeug_parameter_waehler_t::gib_fenstergroesse() const
{
  return koord(tools->count()*32, 32+16);
}


/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 */
fensterfarben werkzeug_parameter_waehler_t::gib_fensterfarben() const
{
  fensterfarben f;

  f.titel  = WIN_TITEL;
  f.hell   = GRAU6;
  f.mittel = GRAU5;
  f.dunkel = GRAU3;

  return f;
}


void werkzeug_parameter_waehler_t::infowin_event(const event_t *ev)
{
  if(IS_LEFTCLICK(ev)) {

    const int x = ev->mx/32;
    const int wz_idx = x;

    //    printf("%d\n", wz_idx);

    if(wz_idx >= 0 && wz_idx < tools->count()) {
      const struct werkzeug_t & tool = tools->at(wz_idx);

      if(tool.has_param) {
	welt->setze_maus_funktion(tool.wzwp,
				  tool.cursor,
				  tool.versatz,
				  tool.param,
				  tool.sound_ok,
				  tool.sound_ko);
      } else {
	welt->setze_maus_funktion(tool.wzwop,
				  tool.cursor,
				  tool.versatz,
				  tool.sound_ok,
				  tool.sound_ko);
      }


    }
  } else if(ev->ev_class == INFOWIN &&
	    ev->ev_code == WIN_CLOSE) {
    welt->setze_maus_funktion(wkz_abfrage,
			      skinverwaltung_t::fragezeiger->gib_bild_nr(0),
			      karte_t::Z_PLAN,
			      0,
			      0);

  }
}


void werkzeug_parameter_waehler_t::zeichnen(koord pos, koord)
{
  for(unsigned int i=0; i<tools->count(); i++) {
    const int icon = tools->at(i).icon;

    if(icon == -1) {
      // Hajo: no icon image available, draw a blank
      // DDD box as replacement

      // top
      display_fillbox_wh(pos.x + i*32,
			 pos.y + 16,
			 32, 1,
			 MN_GREY4,
			 true);
      // body
      display_fillbox_wh(pos.x + i*32+1,
			 pos.y + 16+1,
			 30, 30,
			 MN_GREY2,
			 true);
      // bottom
      display_fillbox_wh(pos.x + i*32,
			 pos.y + 16+31,
			 32, 1,
			 MN_GREY0,
			 true);
      // Left
      display_fillbox_wh(pos.x + i*32,
			 pos.y + 16,
			 1, 32,
			 MN_GREY4,
			 true);
      // Right
      display_fillbox_wh(pos.x + i*32+31,
			 pos.y + 16,
			 1, 32,
			 MN_GREY0,
			 true);
    } else {
      display_color_img(tools->at(i).icon,
			pos.x + i*32,
			pos.y + 16,
			0,
			false,
			true);
    }
  }

  const int xdiff = (gib_maus_x() - pos.x) >> 5;
  const int ydiff = (gib_maus_y() - pos.y - 16) >> 5;

  // print("%d %d\n", xdiff, ydiff);

  if(xdiff >= 0 && xdiff <tools->count() && ydiff >= 0 && ydiff < 1) {
    const int tipnr = xdiff;

    win_set_tooltip(gib_maus_x() + 16,
		    gib_maus_y() - 16,
		    tools->at(tipnr).tooltip);
  }
}
