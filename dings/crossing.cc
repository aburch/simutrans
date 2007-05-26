/*
 * signal.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simplay.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"

#include "../besch/kreuzung_besch.h"

#include "../boden/wege/strasse.h"
#include "../boden/grund.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "crossing.h"



crossing_t::crossing_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
}



crossing_t::crossing_t(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t ns, waytype_t ew) :  ding_t(welt, pos)
{
	besch = get_crossing( ns, ew );
	zustand = false;
	phase = 0;
	timer = 0;
}



crossing_t::~crossing_t()
{
	welt->sync_remove( this );
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void crossing_t::info(cbuffer_t & buf) const
{
}



// coulb be still better aligned for drive_left settings ...
void crossing_t::calc_bild()
{
}




// only used for traffic light: change the current state
bool
crossing_t::sync_step(long delta_t)
{
	// change every 24 hours in normal speed = (1<<18)/24
	return true;
}



void
crossing_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	file->rdwr_byte(zustand, " ");
	file->rdwr_byte(phase, " ");
	file->rdwr_long(timer, " ");
}




void
crossing_t::entferne(spieler_t *sp)
{
}



/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void crossing_t::laden_abschliessen()
{
	grund_t *gr=welt->lookup(gib_pos());
	if(gr==NULL  ||  !gr->hat_weg(besch->get_waytype(0))  ||  !gr->hat_weg(besch->get_waytype(1))) {
		dbg->error("crossing_t::laden_abschliessen","way/ground missing at %i,%i => ignore", gib_pos().x, gib_pos().y );
	}
	else {
		// after loading restore speedlimits
		weg_t *w1=gr->gib_weg(besch->get_waytype(0));
		if(w1->gib_max_speed()>besch->gib_maxspeed(0)) {
			w1->setze_max_speed( besch->gib_maxspeed(0) );
		}
		w1->count_sign();
		weg_t *w2=gr->gib_weg(besch->get_waytype(1));
		if(w2->gib_max_speed()>besch->gib_maxspeed(1)) {
			w2->setze_max_speed( besch->gib_maxspeed(1) );
		}
		w2->count_sign();
		welt->sync_add( this );
	}
}


/* static stuff from here on ... */


// nothing can cross airways, so waytype 0..7 is enough
kreuzung_besch_t *crossing_t::can_cross_array[16][16] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};



bool crossing_t::register_besch(kreuzung_besch_t *besch)
{
	// mark if crossing possible
	if(besch->get_waytype(0)<16  &&  besch->get_waytype(1)<16) {
		can_cross_array[besch->get_waytype(0)][besch->get_waytype(1)] = besch;
		can_cross_array[besch->get_waytype(1)][besch->get_waytype(0)] = besch;
	}
DBG_DEBUG( "crossing_t::register_besch()","%s", besch->gib_name() );
	return true;
}
