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
#include "map_legend.h"
#include "../ifc/karte_modell.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/translator.h"
#include "../besch/fabrik_besch.h"


koord map_frame_t::size;

// Hajo: we track our position onscreen
koord map_frame_t::screenpos;

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

	const koord ij = welt->gib_ij_off();
	const koord gr = karte->gib_groesse();

	set_min_windowsize(koord(64+36,64+16));

	// Hajo: Hack: use static size if set by a former object
	if(size != koord(0,0)) {
		setze_fenstergroesse(size);
		scrolly.setze_groesse(size-koord(0,16));
	}
	else {
		setze_fenstergroesse(koord(gr.x+12,gr.y+12+14));
	}
	add_komponente(&scrolly);


	// Clipping geändert - max. 250 war zu knapp für grosse Karten - V.Meyer
	scrolly.setze_scroll_position(max(0, min(gr.x - (64+16),(ij.x+8) * 2-128)),
	max(0, min(gr.x - (64+16),(ij.y+8) * 2-128)) );

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	int x = CLIP(gib_maus_x() - size.x/2 - 260, 0, display_get_width()-size.x);
	int y = CLIP(gib_maus_y() - size.y-32, 0, display_get_height()-size.y);
	create_win(x, y, -1, new map_legend_t(welt), w_info, magic_map_legend);
	gui_fenster_t *f=win_get_magic(magic_map_legend);
	x = win_get_posx(f)+f->gib_fenstergroesse().x;
	y = win_get_posy(f);
	// change own psoition ...
	f=win_get_magic(magic_reliefmap);
	win_set_pos(f , x, y );
}


map_frame_t::~map_frame_t()
{
	// close the map window
	gui_fenster_t *f=win_get_magic(magic_map_legend);
	if(f) {
DBG_MESSAGE("map_frame_t::~map_frame_t()","close legend");
		destroy_win(f);
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

    scrolly.setze_scroll_position(max(0, min(groesse.x, x)),
				  max(0, min(groesse.y, y)) );

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
  scrolly.setze_groesse( gib_fenstergroesse() - koord(0,16) ) ;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);

  const koord groesse = gib_fenstergroesse();

  // Hajo: Hack: save statically
  size = groesse;
  setze_fenstergroesse(size);
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
  display_fillbox_wh(pos.x+gr.x-14,pos.y+16,14, gr.y-16, MN_GREY2, true);
  display_fillbox_wh(pos.x,pos.y+gr.y-12,gr.x,12, MN_GREY2, true);
  display_fillbox_wh(pos.x+gr.x, pos.y+16, 1, gr.y-16, MN_GREY0, true);
  display_fillbox_wh(pos.x, pos.y+gr.y, gr.x, 1, MN_GREY0, true);

  gui_frame_t::zeichnen(pos, gr);
}
