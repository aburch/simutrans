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
#include "../simvehikel.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simsound.h"

#include "../besch/kreuzung_besch.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "crossing.h"



crossing_t::crossing_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
}



crossing_t::crossing_t(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t w1, waytype_t w2) :  ding_t(welt, pos)
{
	besch = get_crossing( w1, w2 );
	if(besch==NULL) {
		dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", w1, w2 );
	}
	zustand = CROSSING_INVALID;
	phase = 0;
	timer = 0;
	set_state( CROSSING_OPEN );
	setze_besitzer( sp );
}



crossing_t::~crossing_t()
{
//	welt->sync_remove( this );
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void crossing_t::info(cbuffer_t & buf) const
{
	buf.append("way1 reserved by ");
	buf.append(on_way1.get_count());
	buf.append("cars.\nway2 reserved by ");
	buf.append(on_way2.get_count());
	buf.append("cars.\nstate ");
	buf.append(zustand);
}


// request permission to pass crossing
bool
crossing_t::request_crossing( const vehikel_basis_t *v )
{
	if(v->gib_waytype()==besch->get_waytype(0)) {
		if(on_way2.get_count()==0  &&  zustand==CROSSING_OPEN) {
			// way2 is empty ...
			return true;
		}
		// passage denied, since there are vehicle on way2
		// which has priority
		// => ok only if I am already crossing
		return on_way1.is_contained(v);
	}
	else {
		// vehikel from way2 arrives
		if(on_way1.get_count()) {
			// sorry, still things on the crossing, but we will prepare
			set_state( CROSSING_REQUEST_CLOSE );
			return false;
		}
		else {
			set_state( CROSSING_CLOSED );
			return true;
		}
	}
}



// request permission to pass crossing
void
crossing_t::add_to_crossing( const vehikel_basis_t *v )
{
	if(v->gib_waytype()==besch->get_waytype(0)) {
		on_way1.append_unique(v);
	}
	else {
		// add it and close crossing
		on_way2.append_unique(v);
		set_state( CROSSING_CLOSED );
	}
}



// called after passing of the last vehicle (in a convoi)
// or of a city car; releases the crossing which may switch state
void
crossing_t::release_crossing( const vehikel_basis_t *v )
{
	if(v->gib_waytype()==besch->get_waytype(0)) {
		on_way1.remove(v);
		if(zustand==CROSSING_REQUEST_CLOSE  &&  on_way1.get_count()==0) {
			set_state( CROSSING_CLOSED );
		}
	}
	else {
		on_way2.remove(v);
		if(on_way2.get_count()==0) {
			set_state( CROSSING_OPEN );
		}
	}
}



// change state; mark dirty and plays sound
void
crossing_t::set_state( uint8 new_state )
{
	if(new_state!=zustand) {
		mark_image_dirty( bild, 0 );
		mark_image_dirty( after_bild, 0 );
		set_flag(ding_t::dirty);
		// play sound (if there and closing)
		if(new_state==CROSSING_CLOSED  &&  besch->gib_sound()>=0) {
			struct sound_info info;
			info.index = besch->gib_sound();
			info.volume = 255;
			info.pri = 0;
			welt->play_sound_area_clipped(gib_pos().gib_2d(), info);
		}
		zustand = new_state;
		calc_bild();
	}
}



/**
 * Dient zur Neuberechnung des Bildes
 * @author Hj. Malthaner
 */
void
crossing_t::calc_bild()
{
	// recalc bild each step ...
	const bild_besch_t *a = besch->gib_bild_after( ns, zustand!=CROSSING_CLOSED, 0 );
	after_bild = a ? a->gib_nummer() : IMG_LEER;
	const bild_besch_t *b = besch->gib_bild( ns, zustand!=CROSSING_CLOSED, 0 );
	bild = b ? b->gib_nummer() : IMG_LEER;
}



void
crossing_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	uint8 dummy = zustand;
	file->rdwr_byte(dummy, " ");
	zustand = dummy;
	dummy = ns;
	file->rdwr_byte(dummy, " ");
	ns = dummy;
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
		if(besch==NULL) {
			dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", w1, w2 );
		}
		uint new_state = zustand;
		zustand = CROSSING_INVALID;
		set_state( new_state==CROSSING_INVALID ? CROSSING_OPEN : new_state );
	}
}




void
crossing_t::entferne(spieler_t *sp)
{
/*
	grund_t *gr=welt->lookup(gib_pos());
	if(gr==NULL) {
		dbg->error("crossing_t::entferne","ground missing at %i,%i => ignore", gib_pos().x, gib_pos().y );
	}
	else {
		// after loading restore speedlimits
		weg_t *w1=gr->gib_weg(besch->get_waytype(0));
		if(w1  &&  w1->gib_besch()->gib_topspeed()>besch->gib_maxspeed(0)) {
			w1->setze_besch( w1->gib_besch() );
		}
		weg_t *w2=gr->gib_weg(besch->get_waytype(1));
		if(w2  &&  w2->gib_besch()->gib_topspeed()>besch->gib_maxspeed(1)) {
			w2->setze_besch( w2->gib_besch() );
		}
	}
*/
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
		w1->count_sign();
		weg_t *w2=gr->gib_weg(besch->get_waytype(1));
		w2->count_sign();
		ns = ribi_t::ist_gerade_ns(w2->gib_ribi_unmasked());
//		welt->sync_add( this );
	}
}


/* static stuff from here on ... */


// nothing can cross airways, so waytype 0..7 is enough
kreuzung_besch_t* crossing_t::can_cross_array[8][8] =
{
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
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
