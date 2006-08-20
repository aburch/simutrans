/*
 * map_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>
#include <cmath>

#include "map_frame.h"
#include "../ifc/karte_modell.h"

#include "../simworld.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/translator.h"
#include "../besch/fabrik_besch.h"
#include "../tpl/minivec_tpl.h"

#ifndef MAX
#define MAX(a,b)            ((a) > (b) ? (a) : (b))
#define MIN(a,b)            ((a) < (b) ? (a) : (b))
#endif


koord map_frame_t::size;

// Hajo: we track our position onscreen
koord screenpos;

static minivec_tpl <cstring_t> legend_names (32);
static minivec_tpl <int> legend_colors (32);


// @author hsiegeln
const char map_frame_t::map_type[MAX_MAP_TYPE][64] =
{
  "Passagiere",
    "Post",
    "Fracht",
    "Status",
    "Service",
    "Traffic",
    "Origin",
    "Destination",
    "Waiting",
    "Tracks",
    "Speedlimit",
    "Powerlines",
    "Tourists",
};

const int map_frame_t::map_type_color[MAX_MAP_TYPE] =
{
  7, 11, 15, 132, 23, 27, 31, 35, 241, 7, 11, 71, 57
};

/**
 * Calculate button positions
 * @author Hj. Malthaner
 */
void map_frame_t::calc_button_positions()
{
  const int button_columns = (gib_fenstergroesse().x - 20) / 80;
  for (int type=0; type<MAX_MAP_TYPE; type++) {

    filter_buttons[type].init(button_t::box,
			     translator::translate(map_type[type]),
			     koord(10 + 80*(type % button_columns),
				   gib_fenstergroesse().y - legend_height + button_start + 15 * ((int)type/button_columns-1)),
			     koord(79, 14));
  }
}



/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
 * @author Hj. Malthaner
 */
map_frame_t::map_frame_t(const karte_modell_t *welt) : gui_frame_t("Reliefkarte"), scrolly(reliefkarte_t::gib_karte())
{
    reliefkarte_t *karte = reliefkarte_t::gib_karte();
    karte->setze_pos(koord(0, 0));
    scrolly.setze_pos(koord(0, 0));

    is_dragging = false;

    const koord gr = karte->gib_groesse();


    legend_names.clear();
    legend_colors.clear();

    const stringhashtable_tpl<const fabrik_besch_t *> & fabesch =
      fabrikbauer_t::gib_fabesch();

    stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

    while( iter.next() ) {

      cstring_t label (translator::translate(iter.get_current_value()->gib_name()));
//      if(  small_proportional_width(label)>60  ) {//label.len() > 14) {
      if(  label.len() > 14) {
		label.set_at(12, '.');
		label.set_at(13, '.');
		label.set_at(14, '\0');
      }

      legend_names.append(label);
      legend_colors.append(iter.get_current_value()->gib_kennfarbe());
    }


    legend_height = (((legend_names.count() + 3) >> 2) << 3) + 8 + (int)(MAX_MAP_TYPE / 2) * 15;
    button_start = (((legend_names.count() + 3) >> 2) << 3) + 12;



    set_min_windowsize(koord(256+20,240));

    // Hajo: Hack: use static size if set by a former object
    if(size != koord(0,0)) {
      setze_fenstergroesse(size);
      scrolly.setze_groesse(size-koord(0,16));
    }
    else {
      if(gr.x <= 256) {
	setze_fenstergroesse(gr + koord(12, 12+16+30));
	scrolly.setze_groesse(gr + koord(12, 12));
      }
      else {
	setze_fenstergroesse(koord(250, 250+12+16+30));
	scrolly.setze_groesse(koord(250, 250));
      }
    }

    add_komponente(&scrolly);

    const koord ij = welt->gib_ij_off();

    // Clipping geändert - max. 250 war zu knapp für grosse Karten - V.Meyer
    scrolly.setze_scroll_position(MAX(0, MIN(gr.x - 240,(ij.x+8) * 2-128)),
				  MAX(0, MIN(gr.x - 240,(ij.y+8) * 2-128)) );



    // Hajo: Trigger layouting
    set_resizemode(diagonal_resize);
    resize(koord(0,0));


    // Hajo: place buttons
    calc_button_positions();


    // add filter buttons
    // @author hsiegeln
    for (int type=0; type<MAX_MAP_TYPE; type++) {

      filter_buttons[type].add_listener(this);
      filter_buttons[type].background = map_type_color[type];
      is_filter_active[type] = karte->get_mode() == type;
      add_komponente(filter_buttons + type);
    }
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void map_frame_t::infowin_event(const event_t *ev)
{
    // %DB0 printf( "\nMessage: map_frame_t::infowin_event( event_t const * ev ) : Fenster|Window %p : Event %d", (void*)this, ev->ev_class );

  if(!is_dragging) {
    gui_frame_t::infowin_event(ev);
  }

  // Hajo: hack: relief map can resize upon right click
  // we track this here, and adjust size.

  if(IS_RIGHTCLICK(ev)) {
    is_dragging = false;
    reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(false);
  }

  if(IS_RIGHTRELEASE(ev)) {
    if(!is_dragging) {
      resize( koord(0,0) );
    }

    is_dragging = false;
    reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(false);
  }


  const koord groesse = gib_fenstergroesse();

  // Hajo: check if click was inside window
  const bool inside_window =
    screenpos.x <= ev->cx &&
    screenpos.y <= ev->cy &&
    screenpos.x+groesse.x >= ev->cx &&
    screenpos.y+groesse.y >= ev->cy;

  if(inside_window && IS_RIGHTDRAG(ev)) {
    int x = scrolly.get_scroll_x();
    int y = scrolly.get_scroll_y();

    x += (ev->mx - ev->cx)*2;
    y += (ev->my - ev->cy)*2;

    scrolly.setze_scroll_position(MAX(0, MIN(groesse.x, x)),
				  MAX(0, MIN(groesse.y, y)) );

    // Hajo: re-center mouse pointer
    display_move_pointer(screenpos.x+ev->cx, screenpos.y+ev->cy);

    // Hajo: use this for BeOS
    // change_drag_start(ev->mx - ev->cx, ev->my - ev->cy);

    is_dragging = true;
    reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(true);
  }
}


/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_frame_t::setze_fenstergroesse(koord groesse)
{
  gui_frame_t::setze_fenstergroesse( groesse );

  map_frame_t::size = gib_fenstergroesse();   // Hajo: remember size

  scrolly.setze_groesse( gib_fenstergroesse() - koord(0,16+legend_height) ) ;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);

  const koord groesse = gib_fenstergroesse()-koord(0,16);

  // Hajo: Hack: save statically
  size = groesse+koord(0,16);

  setze_fenstergroesse(size);

  calc_button_positions();
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void map_frame_t::zeichnen(koord pos, koord gr)
{
  screenpos = pos;

  // scrollbar "opaqueness" mechanism has changed. So we must draw grey background here
	display_fillbox_wh(pos.x+1,pos.y+16,gr.x-2,gr.y-legend_height, MN_GREY2, true);

  display_fillbox_wh(pos.x+1, pos.y+gr.y-legend_height, gr.x-2, legend_height-1, MN_GREY2, true);
  display_fillbox_wh(pos.x, pos.y+gr.y-1, gr.x, 1, MN_GREY0, true);

  display_fillbox_wh(pos.x, pos.y+gr.y-legend_height-16, 1, legend_height+16, MN_GREY4, true);
  display_fillbox_wh(pos.x+gr.x-1, pos.y+gr.y-legend_height, 1, legend_height, MN_GREY0, true);


  for(uint32 i=0; i<legend_names.count(); i++) {

    const int xpos = pos.x + (i & 3) * 65 + 4;
    const int ypos = pos.y+gr.y-legend_height+4+(i>>2)*8;
    const int color = legend_colors.at(i);

    display_fillbox_wh(xpos, ypos+1, 4, 4, color, false);

	/* changed for unicode display
	 * @author: prissi
	 */
	display_small_proportional( xpos+5, ypos, legend_names.get(i), ALIGN_LEFT, SCHWARZ, false);
  }

  for (int i = 0;i<MAX_MAP_TYPE;i++) {
    filter_buttons[i].pressed = is_filter_active[i];
  }

  // color bar
  for (int i = 0; i<12; i++) {
    display_fillbox_wh(pos.x + size.x - 10, pos.y+gr.y-legend_height + 10 + i*8, 4, 8, reliefkarte_t::severity_color[11-i], false);
  }

  display_small_proportional(pos.x + size.x, pos.y+gr.y-legend_height - 3, translator::translate("max"), ALIGN_RIGHT, SCHWARZ, false);
  display_small_proportional(pos.x + size.x, pos.y+gr.y-legend_height + 107, translator::translate("min"), ALIGN_RIGHT, SCHWARZ, false);

  gui_frame_t::zeichnen(pos, gr);
}


bool
map_frame_t::action_triggered(gui_komponente_t *komp)
{
  reliefkarte_t::gib_karte()->set_mode(-1);
  for (int i=0;i<MAX_MAP_TYPE;i++) {
    if (komp == &filter_buttons[i]) {
      if (is_filter_active[i] == true) {
	is_filter_active[i] = false;
      } else {
	reliefkarte_t::gib_karte()->set_mode(i);
	is_filter_active[i] = true;
      }
    } else {
      is_filter_active[i] = false;
    }
  }

  return true;
}
