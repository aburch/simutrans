/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../display/simimg.h"

#include "../besch/kreuzung_besch.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"

#include "../utils/cbuffer_t.h"

#include "crossing.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t crossing_logic_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif


crossing_t::crossing_t(loadsave_t* const file) : obj_no_info_t()
{
	bild = after_bild = IMG_LEER;
	logic = NULL;
	rdwr(file);
}


crossing_t::crossing_t(spieler_t* const sp, koord3d const pos, kreuzung_besch_t const* const besch, uint8 const ns) : obj_no_info_t(pos)
{
	this->ns = ns;
	this->besch = besch;
	logic = NULL;
	zustand = crossing_logic_t::CROSSING_INVALID;
	bild = after_bild = IMG_LEER;
	set_besitzer( sp );
}


crossing_t::~crossing_t()
{
	assert(logic==NULL);
}


void crossing_t::entferne(spieler_t *)
{
	if(logic) {
		crossing_logic_t *old_logic = logic;
		logic = NULL;
		old_logic->remove(this);
	}
}


void crossing_t::rotate90()
{
	obj_t::rotate90();
	ns ^= 1;
}


// changed state: mark dirty
void crossing_t::state_changed()
{
	mark_image_dirty( bild, 0 );
	mark_image_dirty( after_bild, 0 );
	calc_bild();
}


/**
 * Dient zur Neuberechnung des Bildes
 * @author Hj. Malthaner
 */
void crossing_t::calc_bild()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &crossing_logic_mutex );
#endif
	if(  logic  ) {
		zustand = logic->get_state();
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &crossing_logic_mutex );
#endif
	const bool snow_image = get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;
	// recalc bild each step ...
	const bild_besch_t *a = besch->get_bild_after( ns, zustand!=crossing_logic_t::CROSSING_CLOSED, snow_image );
	if(  a==NULL  &&  snow_image  ) {
		// no snow image? take normal one
		a = besch->get_bild_after( ns, zustand!=crossing_logic_t::CROSSING_CLOSED, 0);
	}
	after_bild = a ? a->get_nummer() : IMG_LEER;
	const bild_besch_t *b = besch->get_bild( ns, zustand!=crossing_logic_t::CROSSING_CLOSED, snow_image );
	if (b==NULL  &&  snow_image) {
		// no snow image? take normal one
		b = besch->get_bild( ns, zustand!=crossing_logic_t::CROSSING_CLOSED, 0);
	}
	bild = b ? b->get_nummer() : IMG_LEER;
}


void crossing_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "crossing_t" );

	obj_t::rdwr(file);

	// variables ... attention, logic now in crossing_logic_t
	zustand = logic==NULL ? crossing_logic_t::CROSSING_INVALID : logic->get_state();
	file->rdwr_byte(zustand);
	file->rdwr_byte(ns);
	if(file->get_version()<99016) {
		uint32 ldummy=0;
		uint8 bdummy=0;
		file->rdwr_byte(bdummy);
		file->rdwr_long(ldummy);
		dbg->fatal("crossing_t::rdwr()","I should be never force to load old style crossings!" );
	}
	// which waytypes?
	uint8 w1, w2;
	sint32 speedlimit0 = 999;
	sint32 speedlimit1 = 999;
	if(file->is_saving()) {
		w1 = besch->get_waytype(0);
		w2 = besch->get_waytype(1);
		speedlimit0 = besch->get_maxspeed(0);
		speedlimit1 = besch->get_maxspeed(1);
	}

	file->rdwr_byte(w1);
	file->rdwr_byte(w2);
	if(  file->get_version()>=110000  ) {
		file->rdwr_long( speedlimit0 );
	}
	if(  file->get_version()>=110001  ) {
		file->rdwr_long( speedlimit1 );
	}

	if(  file->is_loading()  ) {
		besch = crossing_logic_t::get_crossing( (waytype_t)w1, (waytype_t)w2, speedlimit0, speedlimit1, welt->get_timeline_year_month());
		if(besch==NULL) {
			dbg->warning("crossing_t::rdwr()","requested for waytypes %i and %i not available, try to load object without timeline", w1, w2 );
			besch = crossing_logic_t::get_crossing( (waytype_t)w1, (waytype_t)w2, speedlimit0, speedlimit1, 0);
		}
		if(besch==NULL) {
			dbg->fatal("crossing_t::rdwr()","requested for waytypes %i and %i but nothing defined!", w1, w2 );
		}
		crossing_logic_t::add( this, static_cast<crossing_logic_t::crossing_state_t>(zustand) );
	}
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void crossing_t::laden_abschliessen()
{
	grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL  ||  !gr->hat_weg(besch->get_waytype(0))  ||  !gr->hat_weg(besch->get_waytype(1))) {
		dbg->error("crossing_t::laden_abschliessen","way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// try to find crossing that matches way max speed
		weg_t *w1=gr->get_weg(besch->get_waytype(0));
		weg_t *w2=gr->get_weg(besch->get_waytype(1));
		const kreuzung_besch_t *test = crossing_logic_t::get_crossing( besch->get_waytype(0), besch->get_waytype(1), w1->get_besch()->get_topspeed(), w2->get_besch()->get_topspeed(), welt->get_timeline_year_month());
		if (test  &&  test!=besch) {
			besch = test;
		}
		// after loading restore speedlimits
		w1->count_sign();
		w2->count_sign();
		ns = ribi_t::ist_gerade_ns(w2->get_ribi_unmasked());
#ifdef MULTI_THREAD
		pthread_mutex_lock( &crossing_logic_mutex );
#endif
		crossing_logic_t::add( this, static_cast<crossing_logic_t::crossing_state_t>(zustand) );
		logic->recalc_state();
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &crossing_logic_mutex );
#endif
	}
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *crossing_t::ist_entfernbar(const spieler_t *sp)
{
	if (get_player_nr()==1) {
		return NULL;
	}
	else {
		return obj_t::ist_entfernbar(sp);
	}
}
