/*
 * citylist_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "citylist_frame_t.h"
#include "citylist_stats_t.h"
#include "../gui/gui_scrollpane.h"



citylist_frame_t::citylist_frame_t(karte_t * welt) : gui_frame_t("City list")
{
  stats = new citylist_stats_t(welt);
  scrolly = new gui_scrollpane_t(stats);
  scrolly->setze_pos(koord(0, 0));
  add_komponente(scrolly);

  setze_fenstergroesse(koord(320, 240));
  // a min-size for the window
  set_min_windowsize(koord(320, 240));

  set_resizemode(diagonal_resize);
  resize(koord(0,0));

  setze_opaque(true);
}


citylist_frame_t::~citylist_frame_t()
{
  delete scrolly;
  scrolly = 0;
  delete stats;
  stats = 0;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void citylist_frame_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);
  koord groesse = gib_fenstergroesse()-koord(0,16);
  scrolly->setze_groesse(groesse);
}
