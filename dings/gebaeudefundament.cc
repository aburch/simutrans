/*
 * gebaeudefundament.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../mm/mempool.h"
#include "../besch/grund_besch.h"

#include "gebaeudefundament.h"


mempool_t * gebaeudefundament_t::mempool = new mempool_t(sizeof(gebaeudefundament_t) );


gebaeudefundament_t::gebaeudefundament_t(karte_t *welt, loadsave_t *file) :
    ding_t(welt)
{
  rdwr(file);
  step_frequency = 0;
}

gebaeudefundament_t::gebaeudefundament_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    ding_t(welt, pos)
{
    setze_besitzer( sp );

    setze_bild(0, grund_besch_t::fundament->gib_bild(welt->get_slope(gib_pos().gib_2d())));

  step_frequency = 0;
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void gebaeudefundament_t::laden_abschliessen()
{
  setze_bild(0, grund_besch_t::fundament->gib_bild(welt->get_slope(gib_pos().gib_2d())));
}


void * gebaeudefundament_t::operator new(size_t /*s*/)
{
    return mempool->alloc();
}


void gebaeudefundament_t::operator delete(void *p)
{
    mempool->free( p );
}
