/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../world/simworld.h"
#include "simobj.h"
#include "../display/simimg.h"

#include "../descriptor/crossing_desc.h"

#include "../ground/grund.h"
#include "../obj/way/weg.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer.h"

#include "../player/simplay.h"

#include "crossing.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t crossing_logic_mutex;
static recursive_mutex_maker_t crossing_lm_maker(crossing_logic_mutex);
#endif


crossing_t::crossing_t(loadsave_t* const file) : obj_no_info_t()
{
	image = foreground_image = IMG_EMPTY;
	logic = NULL;
	rdwr(file);
}


crossing_t::crossing_t(player_t* const player, koord3d const pos, crossing_desc_t const* const desc, uint8 const ns) : obj_no_info_t(pos)
{
	this->ns = ns;
	this->desc = desc;
	logic = NULL;
	state = crossing_logic_t::CROSSING_INVALID;
	image = foreground_image = IMG_EMPTY;
	set_owner( player );
}


crossing_t::~crossing_t()
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
	mark_image_dirty( image, 0 );
	mark_image_dirty( foreground_image, 0 );
	calc_image();
}


/**
 * Dient zur Neuberechnung des Bildes
 */
void crossing_t::calc_image()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &crossing_logic_mutex );
#endif
	if(  logic  ) {
		state = logic->get_state();
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &crossing_logic_mutex );
#endif
	const bool snow_image = get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;
	// recalc image each step ...
	const image_t *a = desc->get_foreground( ns, state!=crossing_logic_t::CROSSING_CLOSED, snow_image );
	if(  a==NULL  &&  snow_image  ) {
		// no snow image? take normal one
		a = desc->get_foreground( ns, state!=crossing_logic_t::CROSSING_CLOSED, 0);
	}
	foreground_image = a ? a->get_id() : IMG_EMPTY;
	const image_t *b = desc->get_background( ns, state!=crossing_logic_t::CROSSING_CLOSED, snow_image );
	if (b==NULL  &&  snow_image) {
		// no snow image? take normal one
		b = desc->get_background( ns, state!=crossing_logic_t::CROSSING_CLOSED, 0);
	}
	image = b ? b->get_id() : IMG_EMPTY;
}


void crossing_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "crossing_t" );

	obj_t::rdwr(file);

	// variables ... attention, logic now in crossing_logic_t
	state = logic==NULL ? crossing_logic_t::CROSSING_INVALID : logic->get_state();
	file->rdwr_byte(state);
	state = clamp<uint8>(state, 0, crossing_logic_t::NUM_CROSSING_STATES-1);
	file->rdwr_byte(ns);

	if(file->is_version_less(99, 16)) {
		uint32 ldummy=0;
		uint8 bdummy=0;
		file->rdwr_byte(bdummy);
		file->rdwr_long(ldummy);
		dbg->fatal("crossing_t::rdwr()","I should be never forced to load old style crossings!" );
	}
	// which waytypes?
	uint8 w1, w2;
	sint32 speedlimit0 = 999;
	sint32 speedlimit1 = 999;
	if(file->is_saving()) {
		w1 = desc->get_waytype(0);
		w2 = desc->get_waytype(1);
		speedlimit0 = desc->get_maxspeed(0);
		speedlimit1 = desc->get_maxspeed(1);
	}

	file->rdwr_byte(w1);
	file->rdwr_byte(w2);
	if(  file->is_version_atleast(110, 0)  ) {
		file->rdwr_long( speedlimit0 );
	}
	if(  file->is_version_atleast(110, 1)  ) {
		file->rdwr_long( speedlimit1 );
	}

	if(  file->is_loading()  ) {
		desc = crossing_logic_t::get_crossing( (waytype_t)w1, (waytype_t)w2, speedlimit0, speedlimit1, welt->get_timeline_year_month());
		if(desc==NULL) {
			dbg->warning("crossing_t::rdwr()","requested for waytypes %i and %i not available, try to load object without timeline", w1, w2 );
			desc = crossing_logic_t::get_crossing( (waytype_t)w1, (waytype_t)w2, 0, 0, 0);
		}
		if(desc==NULL) {
			dbg->fatal("crossing_t::rdwr()","requested for waytypes %i and %i but nothing defined!", w1, w2 );
		}
		crossing_logic_t::add( this, static_cast<crossing_logic_t::crossing_state_t>(state) );
	}
}


void crossing_t::finish_rd()
{
	grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL  ||  !gr->hat_weg(desc->get_waytype(0))  ||  !gr->hat_weg(desc->get_waytype(1))) {
		dbg->error("crossing_t::finish_rd","way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// try to find crossing that matches way max speed
		weg_t *w1=gr->get_weg(desc->get_waytype(0));
		weg_t *w2=gr->get_weg(desc->get_waytype(1));
		const crossing_desc_t *test = crossing_logic_t::get_crossing( desc->get_waytype(0), desc->get_waytype(1), w1->get_desc()->get_topspeed(), w2->get_desc()->get_topspeed(), welt->get_timeline_year_month());
		if (test  &&  test!=desc) {
			desc = test;
		}
		// after loading restore speedlimits
		w1->count_sign();
		w2->count_sign();
		ns = ribi_t::is_straight_ns(w2->get_ribi_unmasked());
#ifdef MULTI_THREAD
		pthread_mutex_lock( &crossing_logic_mutex );
#endif
		crossing_logic_t::add( this, static_cast<crossing_logic_t::crossing_state_t>(state) );
		logic->recalc_state();
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &crossing_logic_mutex );
#endif
	}
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *crossing_t::is_deletable(const player_t *player)
{
	if (get_owner_nr()==PUBLIC_PLAYER_NR) {
		return NULL;
	}
	else {
		return obj_t::is_deletable(player);
	}
}


void crossing_t::info(cbuffer_t & buf) const
{
	buf.append(translator::translate(get_name()));
	buf.append("\n");
	logic->info(buf);
	buf.append("\n");

	if (char const* const maker = get_desc()->get_copyright()) {
		buf.printf(translator::translate("Constructed by %s"), maker);
		buf.append("\n\n");
	}
}
