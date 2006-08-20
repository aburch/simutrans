/*
 * goods_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "goods_frame_t.h"
#include "goods_stats_t.h"
#include "gui_scrollpane.h"



goods_frame_t::goods_frame_t() : gui_frame_t("Goods list")
{
  goods_stats_t * goods_stats = new goods_stats_t();

  scrolly = new gui_scrollpane_t(goods_stats);


  scrolly->setze_pos(koord(0, 0));

  add_komponente(scrolly);

  setze_fenstergroesse(koord(400, 240));

  set_resizemode(diagonal_resize);
  resize(koord(0,0));

  setze_opaque(true);
}


goods_frame_t::~goods_frame_t()
{
  delete scrolly;
  scrolly = 0;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);

  koord groesse = gib_fenstergroesse()-koord(0,16);

  scrolly->setze_groesse(groesse);
}
