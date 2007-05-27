/*
 * crossing.cc
 *
 * Copyright (c) 2007 prissi
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

#include "../boden/wege/schiene.h"
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



// only used for traffic light: change the current state
bool
crossing_t::sync_step(long delta_t)
{
	// recalc bild each step ...
	grund_t *gr = welt->lookup(gib_pos());
	weg_t *w = gr->gib_weg(besch->get_waytype(1));
	uint8 ns = ribi_t::ist_gerade_ns(w->gib_ribi_unmasked());
	bool open = (besch->get_waytype(1)==road_wt ||  besch->get_waytype(1)==water_wt) ? gr->obj_bei(gr->gib_top()-1)->is_moving() : !((schiene_t *)w)->is_reserved();
	const bild_besch_t *a = besch->gib_bild_after( ns, open, 0 );
	after_bild = a ? a->gib_nummer() : IMG_LEER;
	const bild_besch_t *b = besch->gib_bild( ns, open, 0 );
	bild = b ? b->gib_nummer() : IMG_LEER;

	return true;
}



void
crossing_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	file->rdwr_byte(zustand, " ");
	file->rdwr_byte(phase, " ");
	file->rdwr_long(timer, " ");
	if(file->is_saving()) {
		uint8 wt = besch->get_waytype(0);
		file->rdwr_byte(wt,"w");
		wt = besch->get_waytype(1);
		file->rdwr_byte(wt,"w");
	}
	else {
		uint8 w1, w2;
		file->rdwr_byte(w1,"w");
		file->rdwr_byte(w2,"w");
		besch = get_crossing( (waytype_t)w1, (waytype_t)w2 );
	}
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
kreuzung_besch_t *crossing_t::can_cross_array[8][8] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};



bool crossing_t::register_besch(kreuzung_besch_t *besch)
{
	// mark if crossing possible
	if(besch->get_waytype(0)<8  &&  besch->get_waytype(1)<8) {
		can_cross_array[besch->get_waytype(0)][besch->get_waytype(1)] = besch;
		can_cross_array[besch->get_waytype(1)][besch->get_waytype(0)] = besch;
	}
DBG_DEBUG( "crossing_t::register_besch()","%s", besch->gib_name() );
	return true;
}
