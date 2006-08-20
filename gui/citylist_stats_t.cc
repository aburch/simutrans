/*
 * citylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "citylist_stats_t.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../gui/stadt_info.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"

static const char *total_bev_string;


citylist_stats_t::citylist_stats_t(karte_t * w)
{
  welt = w;

  setze_groesse(koord(600, welt->gib_staedte()->get_count()*14 + 19));
  total_bev_string = translator::translate("Total inhabitants:");
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void citylist_stats_t::infowin_event(const event_t * ev)
{
	if(IS_LEFTRELEASE(ev)  &&  ev->cy>15) {

		const unsigned int line = (ev->cy-15) / 14;
		const weighted_vector_tpl<stadt_t *> * cities = welt->gib_staedte();
		if(line < cities->get_count()) {

			stadt_t * stadt = cities->get(line);
			if(stadt) {
				create_win(320, 0, -1, stadt->gib_stadt_info(), w_info, magic_none);	/* with magic!=none only one dialog is allowed */

				const koord pos = stadt->gib_pos();
				if(welt->ist_in_kartengrenzen(pos)) {
					welt->setze_ij_off(pos + koord(-5,-5));
				}
			}
		}
	}
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void citylist_stats_t::zeichnen(koord offset) const
{
  static cbuffer_t buf (1024);
  long total_bev=0;
  long total_growth=0;

  const weighted_vector_tpl<stadt_t *> * cities = welt->gib_staedte();

  for(unsigned int i=0; i<cities->get_count(); i++) {
    const stadt_t * stadt = cities->get(i);

    if(stadt) {
      buf.clear();

      buf.append(i+1);
      buf.append(".) ");

      total_bev += stadt->gib_einwohner();
      total_growth += stadt->gib_wachstum();
      stadt->get_short_info(buf);
      display_multiline_text(offset.x+10, 19+ offset.y+i*14, buf, COL_BLACK);
    }
  }
  buf.clear();
  buf.append(total_bev_string);
  buf.append(" ");
  buf.append(total_bev);
  buf.append(" (+");
  buf.append(total_growth);
  buf.append(")");
  display_multiline_text(offset.x+10, offset.y+4, buf, COL_BLACK);
}
