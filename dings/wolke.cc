/*
 * wolke.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "wolke.h"
#include "../dataobj/loadsave.h"

#include "../mm/mempool.h"


wolke_t::wolke_t(karte_t *welt) : ding_t (welt)
{
}


wolke_t::wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, int bild) :
    ding_t(welt, pos)
{
    setze_bild( 0, bild );
    setze_yoff( y_off-8 );
    setze_xoff( x_off );
    insta_zeit = welt->gib_zeit_ms();
}

void
wolke_t::zeige_info()
{
    // wolken sollten keine Infofenster erzeugen
}

void
wolke_t::rdwr(loadsave_t *file)
{
    ding_t::rdwr( file );
    file->rdwr_long(insta_zeit, "\n");
}


mempool_t * sync_wolke_t::mempool = new mempool_t(sizeof(sync_wolke_t) );


sync_wolke_t::sync_wolke_t(karte_t *welt, loadsave_t *file) : wolke_t(welt)
{
    welt->sync_add( this );
    rdwr(file);
}


sync_wolke_t::sync_wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, int bild)
 : wolke_t(welt, pos, x_off, y_off, bild)

{
    base_y_off = y_off-8;
    base_image = bild;
}

void
sync_wolke_t::rdwr(loadsave_t *file)
{
    wolke_t::rdwr( file );
    file->rdwr_short(base_y_off, "\n");
    file->rdwr_short(base_image, "\n");
}


bool
sync_wolke_t::sync_step(long /*delta_t*/)
{
    // DBG_MESSAGE("sync_wolke_t::sync_step()", "%p called", this);

    const uint32 delta = welt->gib_zeit_ms() - gib_insta_zeit();

    setze_yoff(base_y_off - (delta >> 7));
    setze_bild(0, base_image+(delta >> 9));

    // aus sync liste entfernen
    return delta < 2500;
}


/**
 * Wird aufgerufen, wenn wolke entfernt wird. Entfernt wolke aus
 * der Verwaltuing synchroner Objekte.
 * @author Hj. Malthaner
 */
void sync_wolke_t::entferne(spieler_t *)
{
    welt->sync_remove(this);
}


void *
sync_wolke_t::operator new(size_t /*s*/)
{
//    printf("new wolke\n");

    return mempool->alloc();
}


void
sync_wolke_t::operator delete(void *p)
{
//    printf("delete wolke\n");

    mempool->free( p );
}


async_wolke_t::async_wolke_t(karte_t *welt, loadsave_t *file) : wolke_t(welt)
{
    rdwr(file);
}


async_wolke_t::async_wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, int bild) :
   wolke_t(welt, pos, x_off, y_off, bild)
{
}

bool
async_wolke_t::step(long /*delta_t*/)
{
    const int yoff = gib_yoff();

    if(yoff > -120 &&
       welt->gib_zeit_ms() - gib_insta_zeit() < 15000 ) {
	setze_yoff( yoff - 2 );

	return true;
    }

    // remove cloud
    return false;
}
