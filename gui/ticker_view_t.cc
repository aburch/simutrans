/*
 * gui/ticker_view_t.cc
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "ticker_view_t.h"
#include "../simdebug.h"
#include "../simcolor.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simticker.h"
#include "../simworld.h"


static int news_x = 0;


ticker_view_t::ticker_view_t(karte_t * welt)
{
  this->welt = welt;
}


/**
 * in top-level fenstern wird der Name in der Titelzeile dargestellt
 * @return den nicht uebersetzten Namen der Komponente
 * @author Hj. Malthaner
 */
const char * ticker_view_t::gib_name() const
{
  return "News ticker:";
}


/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 * @author Hj. Malthaner
 */
fensterfarben ticker_view_t::gib_fensterfarben() const
{
  struct fensterfarben farben;

  farben.titel  = MN_GREY2;
  farben.hell   = MN_GREY4;
  farben.mittel = MN_GREY2;
  farben.dunkel = MN_GREY0;

  return farben;
}


/**
 * @return gibt wunschgroesse für das Darstellungsfenster zurueck
 * @author Hj. Malthaner
 */
koord ticker_view_t::gib_fenstergroesse() const
{
  return koord(display_get_width(), 16);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void ticker_view_t::infowin_event(const event_t *ev)
{
  /*
  dbg->message("ticker_view_t::infowin_event()",
	       "event: class=%d, code=%d, cx=%d, cy=%d",
	       ev->ev_class, ev->ev_code, ev->cx, ev->cy);
  */

  if(IS_LEFTCLICK(ev)) {
    ticker_t *ticker = ticker_t::get_instance();
    const ticker_t::node * n = ticker->first();


    // Hajo: display news ticker, but only if there are news
    if(n) {
      int pos = 0;

      do {
	const int w = proportional_string_width(n->msg);
	pos = n->xpos-news_x;

	if(pos+w < 0) {
	  // gar nicht zu sehen ?
	} else {
	  if(pos < display_get_width()) {
	    if(ev->cx >= pos && ev->cx <= pos+w) {
	      dbg->message("ticker_view_t::infowin_event()",
			   "clicked message '%s'  with pos=%d,%d",
			   n->msg,
			   n->pos.x,
			   n->pos.y);


	      if(welt->ist_in_kartengrenzen(n->pos)) {
		welt->zentriere_auf(n->pos);
	      }
	    }
	  } else {
	    // nicht mehr zu sehen
	    break;
	  }

	}
	n = ticker->next();
      } while(n);
    }
  }
}


/**
 * Fenster neu zeichnen.
 * @author Hj. Malthaner
 */
void ticker_view_t::zeichnen(koord wpos, koord gr)
{
  ticker_t *ticker = ticker_t::get_instance();
  const ticker_t::node * n = ticker->first();


  // Hajo: display news ticker, but only if there are news
  if(n) {
    display_fillbox_wh(wpos.x, wpos.y, gr.x, 1, SCHWARZ, true);
    display_fillbox_wh(wpos.x, wpos.y+1, gr.x, 15, MN_GREY2, true);

    int pop = 0;
    int pos = 0;

    //    printf("News0: %d %d %s\n", news_x, curr_x, msg);

    do {
      const int w = proportional_string_width(n->msg);
      pos = n->xpos-news_x;

      //	printf("News1: %d %d %s\n", news_x, curr_x, msg);

      if(pos+w < 0) {
	// gar nicht zu sehen ?
	pop ++;
      } else {
	if(pos < gr.x) {
	  display_proportional_clip(pos, wpos.y+4, n->msg,
				    ALIGN_LEFT, n->color, true);
	} else {
	  // nicht mehr zu sehen
	  break;
	}

      }
      n = ticker->next();
    } while(n);


    // Nachrichten aufräumen

    for(int i=0; i<pop; i++) {
      // printf("News: pop()\n");
      ticker->pop();
    }

    if(ticker->count() > 0) {
      news_x += 2;

      if(pos < gr.x) {
	ticker->add_next_pos(2);
      }
    } else {
      //	printf("News: reset\n");
      news_x = 0;
      ticker->set_next_pos(gr.x);
    }
  }
}
