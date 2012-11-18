/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 *
 * All moving stuff (vehikel_basis_t) and all player vehicle (derived from vehikel_t)
 *
 * 01.11.99  getrennt von simdings.cc
 *
 * Hansjoerg Malthaner, Nov. 1999
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../boden/grund.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/strasse.h"

#include "../bauer/warenbauer.h"

#include "../simworld.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simunits.h"

#include "../player/simplay.h"
#include "../simfab.h"
#include "../simware.h"
#include "../simhalt.h"
#include "../simsound.h"

#include "../simimg.h"
#include "../simmesg.h"
#include "../simcolor.h"
#include "../simgraph.h"

#include "../simline.h"

#include "../simintr.h"

#include "../dings/wolke.h"
#include "../dings/signal.h"
#include "../dings/roadsign.h"
#include "../dings/crossing.h"
#include "../dings/zeiger.h"

#include "../gui/karte.h"

#include "../besch/stadtauto_besch.h"
#include "../besch/ware_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/roadsign_besch.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"


#include "../bauer/vehikelbauer.h"

#include "simvehikel.h"
#include "simverkehr.h"



/* get dx and dy from dir (just to remind you)
 * any vehikel (including city cars and pedestrians)
 * will go this distance per sync step.
 * (however, the real dirs are only calculated during display, these are the old ones)
 */
sint8 vehikel_basis_t::dxdy[ 8*2 ] = {
	-2, 1,	// s
	-2, -1,	// w
	-4, 0,	// sw
	0, 2,	//se
	2, -1,	// n
	2, 1,	// e
	4, 0,	// ne
	0, -2	// nw
};


// Constants
uint8 vehikel_basis_t::old_diagonal_vehicle_steps_per_tile = 128;
uint8 vehikel_basis_t::diagonal_vehicle_steps_per_tile = 181;
uint16 vehikel_basis_t::diagonal_multiplier = 724;


// set only once, before loading!
void vehikel_basis_t::set_diagonal_multiplier( uint32 multiplier, uint32 old_diagonal_multiplier )
{
	diagonal_multiplier = (uint16)multiplier;
	diagonal_vehicle_steps_per_tile = (uint8)(130560u/diagonal_multiplier) + 1;
	old_diagonal_vehicle_steps_per_tile = (uint8)(130560u/old_diagonal_multiplier) + 1;
}


// if true, convoi, must restart!
bool vehikel_basis_t::need_realignment() const
{
	return old_diagonal_vehicle_steps_per_tile!=diagonal_vehicle_steps_per_tile  &&  ribi_t::ist_kurve(fahrtrichtung);
}

// [0]=xoff [1]=yoff
static sint8 driveleft_base_offsets[8][2] =
{
	{ 12, 6 },
	{ -12, 6 },
	{ 0, 6 },
	{ 12, 0 },
	{ -12, -6 },
	{ 12, -6 },
	{ 0, -6 },
	{ -12, 0 }
//	{ 12, -12, 0, 12, -12, 12, 0 -12 },
//	{ 6, 6, 6, 0, -6, -6, -6, 0 }
};

// [0]=xoff [1]=yoff
sint8 vehikel_basis_t::overtaking_base_offsets[8][2];

// recalc offsets for overtaking
void vehikel_basis_t::set_overtaking_offsets( bool driving_on_the_left )
{
	sint8 sign = driving_on_the_left ? -1 : 1;
	// a tile has the internal size of
	const sint8 XOFF=12;
	const sint8 YOFF=6;

	overtaking_base_offsets[0][0] = sign * XOFF;
	overtaking_base_offsets[1][0] = -sign * XOFF;
	overtaking_base_offsets[2][0] = 0;
	overtaking_base_offsets[3][0] = sign * XOFF;
	overtaking_base_offsets[4][0] = -sign * XOFF;
	overtaking_base_offsets[5][0] = sign * XOFF;
	overtaking_base_offsets[6][0] = 0;
	overtaking_base_offsets[7][0] = sign * (-XOFF-YOFF);

	overtaking_base_offsets[0][1] = sign * YOFF;
	overtaking_base_offsets[1][1] = sign * YOFF;
	overtaking_base_offsets[2][1] = sign * YOFF;
	overtaking_base_offsets[3][1] = 0;
	overtaking_base_offsets[4][1] = -sign * YOFF;
	overtaking_base_offsets[5][1] = -sign * YOFF;
	overtaking_base_offsets[6][1] = -sign * YOFF;
	overtaking_base_offsets[7][1] = 0;
}


/**
 * Checks if this vehicle must change the square upon next move
 * THIS IS ONLY THERE FOR LOADING OLD SAVES!
 * @author Hj. Malthaner
 */
bool vehikel_basis_t::is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const
{
	const sint8 y_off_2 = 2*neu_yoff;
	const sint8 c_plus  = y_off_2 + neu_xoff;
	const sint8 c_minus = y_off_2 - neu_xoff;

	return ! (c_plus < OBJECT_OFFSET_STEPS*2  &&  c_minus < OBJECT_OFFSET_STEPS*2  &&  c_plus > -OBJECT_OFFSET_STEPS*2  &&  c_minus > -OBJECT_OFFSET_STEPS*2);
}


vehikel_basis_t::vehikel_basis_t(karte_t *welt):
	ding_t(welt)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	drives_on_left = false;
	dx = 0;
	dy = 0;
	hoff = 0;
}


vehikel_basis_t::vehikel_basis_t(karte_t *welt, koord3d pos):
	ding_t(welt, pos)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
	pos_next = pos;
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	drives_on_left = false;
	dx = 0;
	dy = 0;
	hoff = 0;
}


void vehikel_basis_t::rotate90()
{
	koord3d pos_cur = get_pos();
	pos_cur.rotate90( welt->get_groesse_y()-1 );
	set_pos( pos_cur );
	// directions are counterclockwise to ribis!
	fahrtrichtung = ribi_t::rotate90( fahrtrichtung );
	pos_next.rotate90( welt->get_groesse_y()-1 );
	// new offsets: very tricky ...
	sint8 new_dx = -dy*2;
	dy = dx/2;
	dx = new_dx;
	// new pos + step offsets (only possible, since we know the height!
	sint8 neu_yoff = get_xoff()/2;
	set_xoff( -get_yoff()*2 );
	set_yoff( neu_yoff );
}


void vehikel_basis_t::verlasse_feld()
{
	// first: release crossing
	grund_t *gr = welt->lookup(get_pos());
	if(gr  &&  gr->ist_uebergang()) {
		crossing_t *cr = gr->find<crossing_t>(2);
		grund_t *gr2 = welt->lookup(pos_next);
		if(gr2==NULL  ||  gr2==gr  ||  !gr2->ist_uebergang()  ||  cr->get_logic()!=gr2->find<crossing_t>(2)->get_logic()) {
			cr->release_crossing(this);
		}
	}

	// then remove from ground (or search whole map, if failed)
	if(!get_flag(not_on_map)  &&  (gr==NULL  ||  !gr->obj_remove(this)) ) {

		// was not removed (not found?)
		dbg->error("vehikel_basis_t::verlasse_feld()","'typ %i' %p could not be removed from %d %d", get_typ(), this, get_pos().x, get_pos().y);
		DBG_MESSAGE("vehikel_basis_t::verlasse_feld()","checking all plan squares");

		// check, whether it is on another height ...
		if(welt->ist_in_kartengrenzen( get_pos().get_2d() )) {
			gr = welt->lookup( get_pos().get_2d() )->get_boden_von_obj(this);
			if(gr) {
				gr->obj_remove(this);
				dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",get_typ(), this, get_pos().x, get_pos().y);
			}
			return;
		}

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->get_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->get_groesse_x(); k.x++) {
				grund_t *gr = welt->lookup( k )->get_boden_von_obj(this);
				if(gr && gr->obj_remove(this)) {
					dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",get_name(), this, k.x, k.y);
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehikel_basis_t::verlasse_feld()","'%s' %p was not found on any map sqaure!",get_name(), this);
		}
	}
}


void vehikel_basis_t::betrete_feld()
{
	grund_t *gr=welt->lookup(get_pos());
	if(!gr) {
		dbg->error("vehikel_basis_t::betrete_feld()","'%s' new position (%i,%i,%i)!",get_name(), get_pos().x, get_pos().y, get_pos().z );
		gr = welt->lookup_kartenboden(get_pos().get_2d());
		set_pos( gr->get_pos() );
	}
	gr->obj_add(this);
}


/* THE routine for moving vehicles
 * it will drive on as log as it can
 * @return the distance actually travelled
 */
uint32 vehikel_basis_t::fahre_basis(uint32 distance)
{
	koord3d pos_prev;
	uint32 distance_travelled; // Return value

	uint32 steps_to_do = distance >> YARDS_PER_VEHICLE_STEP_SHIFT;

	if(  steps_to_do == 0  ) {
		// ok, we will not move in this steps
		return 0;
	}
	// ok, so moving ...
	if(  !get_flag(ding_t::dirty)  ) {
		mark_image_dirty(get_bild(),hoff);
		set_flag(ding_t::dirty);
	}
	uint32 steps_target = steps_to_do + steps;

	if(  steps_target > steps_next  ) {
		// We are going far enough to hop.

		// We'll be adding steps_next+1 for each hop, as if we
		// started at the beginning of this tile, so for an accurate
		// count of steps done we must subtract the location we started with.
		sint32 steps_done = -steps;
		bool has_hopped = false;

		// Hop as many times as possible.
		while(  steps_target > steps_next  &&  hop_check()  ) {
			steps_target -= steps_next+1;
			steps_done += steps_next+1;
			pos_prev = get_pos();
			hop();
			use_calc_height = true;
			has_hopped = true;
		}

		if(  steps_next == 0  ) {
			// only needed for aircrafts, which can turn on the same tile
			// the indicate the turn with this here
			steps_next = VEHICLE_STEPS_PER_TILE - 1;
			steps_target = VEHICLE_STEPS_PER_TILE - 1;
			steps_done -= VEHICLE_STEPS_PER_TILE - 1;
		}

		if(  steps_target > steps_next  ) {
			// could not go as far as we wanted (hop_check failed) => stop at end of tile
			steps_target = steps_next;
		}
		// Update internal status, how far we got within the tile.
		steps = steps_target;

		steps_done += steps;
		distance_travelled = steps_done << YARDS_PER_VEHICLE_STEP_SHIFT;

		if(  has_hopped  ) {
			set_xoff( (dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
			set_yoff( (dy<0) ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );
			if(dx*dy==0) {
				if(dx==0) {
					if(dy>0) {
						set_xoff( pos_prev.x!=get_pos().x ? -OBJECT_OFFSET_STEPS : OBJECT_OFFSET_STEPS );
					}
					else {
						set_xoff( pos_prev.x!=get_pos().x ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
					}
				}
				else {
					if(dx>0) {
						set_yoff( pos_prev.y!=get_pos().y ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );
					}
					else {
						set_yoff( pos_prev.y!=get_pos().y ? -OBJECT_OFFSET_STEPS/2 : OBJECT_OFFSET_STEPS/2 );
					}
				}
			}
		}
	}
	else {
		// Just travel to target, it's on same tile
		steps = steps_target;
		distance_travelled = distance & YARDS_VEHICLE_STEP_MASK; // round down to nearest step
	}

	if(use_calc_height) {
		hoff = calc_height();
	}
	// remaining steps
	return distance_travelled;
}


// to make smaller steps than the tile granularity, we have to use this trick
void vehikel_basis_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)steps*(uint16)raster_width;
	if(dx*dy) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}
	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (hoff*raster_width)/(4*16);

	if(  drives_on_left  ) {
		const int drive_left_dir = ribi_t::get_dir(get_fahrtrichtung());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}
}


// calcs new direction and applies it to the vehicles
ribi_t::ribi vehikel_basis_t::calc_set_richtung(koord start, koord ende)
{
	ribi_t::ribi richtung = ribi_t::keine;

	const sint8 di = ende.x - start.x;
	const sint8 dj = ende.y - start.y;

	if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
		dx = 2;
		dy = -1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
		dx = -2;
		dy = 1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
		dx = -2;
		dy = -1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
		dx = 2;
		dy = 1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
		dx = 0;
		dy = 2;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
		dx = 0;
		dy = -2;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
		dx = 4;
		dy = 0;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else {
		richtung = ribi_t::suedwest;
		dx = -4;
		dy = 0;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	}
	// we could artificially make diagonals shorter: but this would break existing game behaviour
	return richtung;
}


ribi_t::ribi vehikel_basis_t::calc_richtung(koord start, koord ende)
{
#if 0
	// may be faster on some architectures
	static ribi_t::ribi didj_richtung[9] =
	{
		ribi_t::nordwest, ribi_t::nord, ribi_t::nordost,	// dy<0
		ribi_t::west, ribi_t::keine, ribi_t::ost,	// dy==0
		ribi_t::suedwest, ribi_t::sued, ribi_t::suedost	// dy>0
	};

	uint8 di = 0x80 & (ende.x - start.x);
	uint8 dj = 0x80 & (ende.y - start.y);
	di ++ ;	// 0=(di<0), 1=(di==0), 2=(di>0)
	dj ++ ;
	return richtung[di+(3*dj)];
#endif
	ribi_t::ribi richtung;
	const sint8 di = (ende.x - start.x);
	const sint8 dj = (ende.y - start.y);
	if(dj == 0 && di == 0) {
		richtung = ribi_t::keine;
	} else if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
	} else {
		richtung = ribi_t::suedwest;
	}
	return richtung;

}


// this routine calculates the new height
// beware of bridges, tunnels, slopes, ...
sint8 vehikel_basis_t::calc_height()
{
	sint8 hoff = 0;
	use_calc_height = false;	// assume, we are only needed after next hop

	grund_t *gr = welt->lookup(get_pos());
	if(gr==NULL) {
		// slope changed below a moving thing?!?
		return 0;
	}
	else if(  gr->ist_tunnel()  &&  gr->ist_karten_boden()  ) {
		use_calc_height = true; // to avoid errors if undergroundmode is switched
		if(  grund_t::underground_mode == grund_t::ugm_none  ||
			(grund_t::underground_mode == grund_t::ugm_level  &&  gr->get_hoehe() < grund_t::underground_level)
		) {
			// need hiding? One of the few uses of XOR: not half driven XOR exiting => not hide!
			ribi_t::ribi hang_ribi = ribi_typ( gr->get_grund_hang() );
			if((steps<(steps_next/2))  ^  ((hang_ribi&fahrtrichtung)!=0)  ) {
				set_bild(IMG_LEER);
			}
			else {
				calc_bild();
			}
		}
	}
	else if(  !gr->is_visible()  ) {
		set_bild(IMG_LEER);
	}
	else {
		// will not work great with ways, but is very short!
		hang_t::typ hang = gr->get_weg_hang();
		if(hang) {
			ribi_t::ribi hang_ribi = ribi_typ(hang);
			if(  ribi_t::doppelt(hang_ribi)  ==  ribi_t::doppelt(fahrtrichtung)) {
				sint16 h_end = hang_ribi & ribi_t::rueckwaerts(fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				sint16 h_start = (hang_ribi & fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				hoff = (h_start*(sint16)(uint16)steps + h_end*(256-steps)) >> 8;
			}
			else {
				// only for shadows and movingobjs ...
				sint16 h_end = hang_ribi & ribi_t::rueckwaerts(fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				sint16 h_start = (hang_ribi & fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				hoff = ((h_start*(sint16)(uint16)steps + h_end*(256-steps)) >> 9) - TILE_HEIGHT_STEP/2;
			}
			use_calc_height = true;	// we need to recalc again next time
		}
		else {
			hoff = -gr->get_weg_yoff();
		}
	}
	return hoff;
}


/* true, if one could pass through this field
 * also used for citycars, thus defined here
 */
vehikel_basis_t *vehikel_basis_t::no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_fahrtrichtung, const uint8 next_fahrtrichtung, const uint8 next_90fahrtrichtung )
{
	// suche vehikel
	for(  uint8 pos=1;  pos<(volatile uint8)gr->get_top();  pos++ ) {
		if (vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(pos))) {
			if(  v->get_typ()==ding_t::fussgaenger  ) {
				continue;
			}

			// check for car
			uint8 other_fahrtrichtung=255;
			bool other_moving = false;
			if (automobil_t const* const at = ding_cast<automobil_t>(v)) {
				// ignore ourself
				if(cnv==at->get_convoi()) {
					continue;
				}
				other_fahrtrichtung = at->get_fahrtrichtung();
				other_moving = at->get_convoi()->get_akt_speed() > kmh_to_speed(1);
			}
			// check for city car
			else if(  v->get_waytype()==road_wt  ) {
				other_fahrtrichtung = v->get_fahrtrichtung();
				if(stadtauto_t const* const sa = ding_cast<stadtauto_t>(v)){
					other_moving = sa->get_current_speed() > 1;
				}
			}

			// ok, there is another car ...
			if(other_fahrtrichtung!=255) {
				const ribi_t::ribi other_90fahrtrichtung = (gr->get_pos().get_2d() == v->get_pos_next().get_2d()) ? other_fahrtrichtung : calc_richtung(gr->get_pos().get_2d(),v->get_pos_next().get_2d());
				if(  other_90fahrtrichtung==next_90fahrtrichtung  ) {
					// Want to exit in same as other   ~50% of the time
					return v;
				}

				const bool across = next_fahrtrichtung == (drives_on_left ? ribi_t::rotate45l(next_90fahrtrichtung) : ribi_t::rotate45(next_90fahrtrichtung)); // turning across the opposite directions lane
				const bool other_across = other_fahrtrichtung == (drives_on_left ? ribi_t::rotate45l(other_90fahrtrichtung) : ribi_t::rotate45(other_90fahrtrichtung)); // other is turning across the opposite directions lane
				if(  other_fahrtrichtung==next_fahrtrichtung  && !(other_across || across)  ) {
					// entering same straight waypoint as other   ~18%
					return v;
				}

				const bool straight = next_fahrtrichtung == next_90fahrtrichtung; // driving straight
				const ribi_t::ribi current_90fahrtrichtung = straight ? ribi_t::rueckwaerts(next_90fahrtrichtung) : (~(next_fahrtrichtung|ribi_t::rueckwaerts(next_90fahrtrichtung)))&0x0F;
				const bool other_straight = other_fahrtrichtung == other_90fahrtrichtung; // other is driving straight
				const bool other_exit_same_side = current_90fahrtrichtung == other_90fahrtrichtung; // other is exiting same side as we're entering
				const bool other_exit_opposite_side = ribi_t::rueckwaerts(current_90fahrtrichtung) == other_90fahrtrichtung; // other is exiting side across from where we're entering
				if(  across  &&  ((ribi_t::ist_orthogonal(current_90fahrtrichtung,other_fahrtrichtung) && other_moving) || (other_across && other_exit_opposite_side) || ((other_across||other_straight) && other_exit_same_side && other_moving) ) )  {
					// other turning across in front of us from orth entry dir'n   ~4%
					return v;
				}

				const bool headon = ribi_t::rueckwaerts(current_fahrtrichtung) == other_fahrtrichtung; // we're meeting the other headon
				const bool other_exit_across = (drives_on_left ? ribi_t::rotate90l(next_90fahrtrichtung) : ribi_t::rotate90(next_90fahrtrichtung)) == other_90fahrtrichtung; // other is exiting by turning across the opposite directions lane
				if( straight && (ribi_t::ist_orthogonal(current_90fahrtrichtung,other_fahrtrichtung) || (other_across && other_moving && (other_exit_across || (other_exit_same_side && !headon))) ) ) {
					// other turning across in front of us, but allow if other is stopped - duplicating historic behaviour   ~2%
					return v;
				}
				else if(  other_fahrtrichtung==current_fahrtrichtung && current_90fahrtrichtung==ribi_t::keine  ) {
					// entering same diagonal waypoint as other   ~1%
					return v;
				}

				// else other car is not blocking   ~25%
			}
		}
	}

	// way is free
	return NULL;
}


void vehikel_t::rotate90()
{
	vehikel_basis_t::rotate90();
	alte_fahrtrichtung = ribi_t::rotate90( alte_fahrtrichtung );
	pos_prev.rotate90( welt->get_groesse_y()-1 );
	last_stop_pos.rotate90( welt->get_groesse_y()-1 );
}


void vehikel_t::rotate90_freight_destinations(const sint16 y_size)
{
	// now rotate the freight
	FOR(slist_tpl<ware_t>, & tmp, fracht) {
		tmp.rotate90(welt, y_size );
	}
}


void vehikel_t::set_convoi(convoi_t *c)
{
	/* cnv can have three values:
	 * NULL: not previously assigned
	 * 1 (only during loading): convoi wants to reserve the whole route
	 * other: previous convoi (in this case, currently always c==cnv)
	 *
	 * if c is NULL, then the vehicle is removed from the convoi
	 * (the waggon_t::set_convoi etc. routines must then remove a
	 *  possibly pending reservation of stops/tracks)
	 */
	assert(  c==NULL  ||  cnv==NULL  ||  cnv==(convoi_t *)1  ||  c==cnv);
	cnv = c;
	if(cnv) {
		// we need to reestablish the finish flag after loading
		if(ist_erstes) {
			route_t const& r = *cnv->get_route();
			check_for_finish = r.empty() || route_index >= r.get_count() || get_pos() == r.position_bei(route_index);
		}
		// some convois were saved with broken coordinates
		if(  !welt->lookup(pos_prev)  ) {
			if(  pos_prev!=koord3d::invalid  ) {
				dbg->error("vehikel_t::set_convoi()","pos_prev is illegal of convoi %i at %s", cnv->self.get_id(), get_pos().get_str() );
			}
			if(  grund_t *gr = welt->lookup_kartenboden(pos_prev.get_2d())  ) {
				pos_prev = gr->get_pos();
			}
			else {
				pos_prev = get_pos();
			}
		}
		if(  pos_next != koord3d::invalid  ) {
			route_t const& r = *cnv->get_route();
			if (!r.empty() && route_index < r.get_count() - 1) {
				grund_t const* const gr = welt->lookup(pos_next);
				if (!gr || !gr->get_weg(get_waytype())) {
					if (!(water_wt == get_waytype()  &&  gr->ist_wasser())) { // ships on the open sea are valid
						pos_next = r.position_bei(route_index + 1U);
					}
				}
			}
		}
		// just correct freight deistinations
		FOR(slist_tpl<ware_t>, & c, fracht) {
			c.laden_abschliessen(welt, get_besitzer());
		}
	}
}


/**
 * Unload freight to halt
 * @return sum of unloaded goods
 * @author Hj. Malthaner
 */
uint16 vehikel_t::unload_freight(halthandle_t halt)
{
	uint16 sum_menge = 0;

	if(halt->is_enabled( get_fracht_typ() )) {
		if (!fracht.empty()) {

			for(  slist_tpl<ware_t>::iterator i = fracht.begin(), end = fracht.end();  i != end;  ) {
				const ware_t& tmp = *i;

				halthandle_t end_halt = tmp.get_ziel();
				halthandle_t via_halt = tmp.get_zwischenziel();

				// probleme mit fehlerhafter ware
				// vielleicht wurde zwischendurch die
				// Zielhaltestelle entfernt ?
				if(!end_halt.is_bound() || !via_halt.is_bound()) {
					DBG_MESSAGE("vehikel_t::unload_freight()", "destination of %d %s is no longer reachable",tmp.menge,translator::translate(tmp.get_name()));
					total_freight -= tmp.menge;
					i = fracht.erase( i );
				}
				else if(end_halt==halt || via_halt==halt) {

					//		    printf("Liefere %d %s nach %s via %s an %s\n",
					//                           tmp->menge,
					//			   tmp->name(),
					//			   end_halt->get_name(),
					//			   via_halt->get_name(),
					//			   halt->get_name());

					// hier sollte nur ordentliche ware verabeitet werden
					int menge = halt->liefere_an(tmp);
					sum_menge += menge;
					total_freight -= menge;

					// book delivered goods to destination
					if(end_halt==halt) {
						// pax is alway index 1
						const int categorie = tmp.get_index()>1 ? 2 : tmp.get_index();
						get_besitzer()->buche( menge, (player_cost)(COST_TRANSPORTED_PAS+categorie) );
					}

					i = fracht.erase( i );
				}
				else {
					++i;
				}
			}
		}
		INT_CHECK("vehikel_t::unload_freight");
	}

	return sum_menge;
}


/**
 * Load freight from halt
 * @return loading successful?
 * @author Hj. Malthaner
 */
bool vehikel_t::load_freight(halthandle_t halt)
{
	if (!halt->gibt_ab(besch->get_ware()))
		return false;

	if (total_freight < besch->get_zuladung()) {
		const uint16 hinein = besch->get_zuladung() - total_freight;

		slist_tpl<ware_t> zuladung;
		halt->hole_ab(zuladung, besch->get_ware(), hinein, cnv->get_schedule(), cnv->get_besitzer());

		if(zuladung.empty()) {
			// now empty, but usually, we can get it here ...
			return true;
		}

		for (slist_tpl<ware_t>::iterator iter_z = zuladung.begin(); iter_z != zuladung.end();) {
			ware_t &ware = *iter_z;

			total_freight += ware.menge;

			// could this be joined with existing freight?
			FOR(slist_tpl<ware_t>, & tmp, fracht) {
				// for pax: join according next stop
				// for all others we *must* use target coordinates
				if(ware.same_destination(tmp)) {
					tmp.menge += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not join it to existing => load it
			if(ware.menge != 0) {
				++iter_z;
				// we add list directly
			}
			else {
				iter_z = zuladung.erase(iter_z);
			}
		}

		if(!zuladung.empty()) {
			fracht.append_list(zuladung);
		}

		INT_CHECK("simvehikel 876");
	}
	return true;
}


/**
 * Remove freight that no longer can reach it's destination
 * i.e. becuase of a changed schedule
 * @author Hj. Malthaner
 */
void vehikel_t::remove_stale_freight()
{
	DBG_DEBUG("vehikel_t::remove_stale_freight()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	slist_tpl<ware_t> kill_queue;
	total_freight = 0;

	if (!fracht.empty()) {
		FOR(slist_tpl<ware_t>, & tmp, fracht) {
			bool found = false;

			FOR(minivec_tpl<linieneintrag_t>, const& i, cnv->get_schedule()->eintrag) {
				if (haltestelle_t::get_halt( welt, i.pos, cnv->get_besitzer()) == tmp.get_zwischenziel()) {
					found = true;
					break;
				}
			}

			if (!found) {
				kill_queue.insert(tmp);
			}
			else {
				// since we need to point at factory (0,0), we recheck this too
				koord k = tmp.get_zielpos();
				fabrik_t *fab = fabrik_t::get_fab( welt, k );
				tmp.set_zielpos( fab ? fab->get_pos().get_2d() : k );

				total_freight += tmp.menge;
			}
		}

		FOR(slist_tpl<ware_t>, const& c, kill_queue) {
			fracht.remove(c);
		}
	}
}


void vehikel_t::play_sound() const
{
	if(  besch->get_sound() >= 0  &&  !welt->is_fast_forward()  ) {
		welt->play_sound_area_clipped(get_pos().get_2d(), besch->get_sound());
	}
}


/**
 * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
 * der Convoi eine neue Route ermittelt
 * @author Hj. Malthaner
 */
void vehikel_t::neue_fahrt(uint16 start_route_index, bool recalc)
{
	route_index = start_route_index+1;
	check_for_finish = false;
	use_calc_height = true;

	if(welt->ist_in_kartengrenzen(get_pos().get_2d())) {
		// mark the region after the image as dirty
		// better not try to twist your brain to follow the retransformation ...
		mark_image_dirty( get_bild(), hoff );
	}

	route_t const& r = *cnv->get_route();
	if(!recalc) {
		// always set pos_next
		// otherwise a convoi with somehow a wrong pos_next will next continue
		pos_next = r.position_bei(route_index);
		assert(get_pos() == r.position_bei(start_route_index));
	}
	else {

		// recalc directions
		pos_next = r.position_bei(route_index);
		pos_prev = get_pos();
		set_pos(r.position_bei(start_route_index));

		alte_fahrtrichtung = fahrtrichtung;
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		hoff = 0;
		steps = 0;

		set_xoff( (dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
		set_yoff( (dy<0) ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );

		calc_bild();
	}
	if ( ribi_t::ist_einfach(fahrtrichtung) ) {
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else {
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	}
}


vehikel_t::vehikel_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp) :
	vehikel_basis_t(sp->get_welt(), pos)
{
	this->besch = besch;

	set_besitzer( sp );
	insta_zeit = welt->get_current_month();
	cnv = NULL;
	speed_limit = SPEED_UNLIMITED;

	route_index = 1;

	rauchen = true;
	fahrtrichtung = ribi_t::keine;
	pos_prev = koord3d::invalid;

	current_friction = 4;
	total_freight = 0;
	sum_weight = besch->get_gewicht();

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;
	has_driven = false;

	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
	target_halt = halthandle_t();
}


vehikel_t::vehikel_t(karte_t *welt) :
	vehikel_basis_t(welt)
{
	rauchen = true;

	besch = NULL;
	cnv = NULL;

	route_index = 1;
	current_friction = 4;
	sum_weight = 10;
	total_freight = 0;

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;

	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
}


bool vehikel_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	return route->calc_route(welt, start, ziel, this, max_speed, 0 );
}


bool vehikel_t::hop_check()
{
	// the leading vehicle will do all the checks
	if(ist_erstes) {
		if(check_for_finish) {
			// so we are there yet?
			cnv->ziel_erreicht();
			if(cnv->get_state()==convoi_t::INITIAL) {
				// to avoid crashes with airplanes
				use_calc_height = false;
			}
			return false;
		}

		// now check, if we can go here
		const grund_t *bd = welt->lookup(pos_next);
		if(bd==NULL  ||  !ist_befahrbar(bd)  ||  cnv->get_route()->empty()) {
			// weg not existent (likely destroyed) or no route ...
			cnv->suche_neue_route();
			return false;
		}

		// check for oneway sign etc.
		if(air_wt!=get_waytype()  &&  route_index<cnv->get_route()->get_count()-1) {
			uint8 dir = get_ribi(bd);
			koord3d nextnext_pos = cnv->get_route()->position_bei(route_index+1);
			uint8 new_dir = ribi_typ(nextnext_pos.get_2d()-pos_next.get_2d());
			if((dir&new_dir)==0) {
				// new one way sign here?
				cnv->suche_neue_route();
				return false;
			}
		}

		int restart_speed = -1;
		// ist_weg_frei() berechnet auch die Geschwindigkeit
		// mit der spaeter weitergefahren wird
		if(!ist_weg_frei(restart_speed,false)) {

			// convoi anhalten, wenn strecke nicht frei
			cnv->warten_bis_weg_frei(restart_speed);

			// nicht weiterfahren
			return false;
		}
	}
	else {
		// this is needed since in convoi_t::vorfahren the flag ist_erstes is set to null
		if(check_for_finish) {
			return false;
		}
	}
	return true;
}


void vehikel_t::verlasse_feld()
{
	vehikel_basis_t::verlasse_feld();
#ifndef DEBUG_ROUTES
	if(ist_letztes  &&  reliefkarte_t::is_visible) {
			reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}
#endif
}


/* this routine add a vehicle to a tile and will insert it in the correct sort order to prevent overlaps
 * @author prissi
 */
void vehikel_t::betrete_feld()
{
	vehikel_basis_t::betrete_feld();
	if(ist_erstes  &&  reliefkarte_t::is_visible  ) {
		reliefkarte_t::get_karte()->calc_map_pixel( get_pos().get_2d() );
	}
}


void vehikel_t::hop()
{
	verlasse_feld();

	pos_prev = get_pos();
	set_pos( pos_next );  // naechstes Feld
	if(route_index<cnv->get_route()->get_count()-1) {
		route_index ++;
		pos_next = cnv->get_route()->position_bei(route_index);
	}
	else {
		route_index ++;
		check_for_finish = true;
	}
	alte_fahrtrichtung = fahrtrichtung;

	// this is a required hack for aircrafts! Aircrafts can turn on a single square, and this confuses the previous calculation!
	// author: hsiegeln
	if(!check_for_finish  &&  pos_prev.get_2d()==pos_next.get_2d()) {
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		steps_next = 0;
	}
	else {
		if(pos_next!=get_pos()) {
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::ist_gerade(welt->lookup(pos_next)->get_weg_ribi_unmasked(get_waytype()))  )  ||  welt->lookup(pos_next)->is_halt()) {
			// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
	}
	calc_bild();

	betrete_feld();
	grund_t *gr = welt->lookup(get_pos());
	const weg_t *weg = gr->get_weg(get_waytype());
	if(  weg  ) {
		speed_limit = kmh_to_speed( weg->get_max_speed() );
		if(  weg->is_crossing()  ) {
			gr->find<crossing_t>(2)->add_to_crossing(this);
		}
	}
	else {
		speed_limit = SPEED_UNLIMITED;
	}

	if(  ist_erstes  ) {
		cnv->add_running_cost( weg );
		if(  check_for_finish  &&  (fahrtrichtung==ribi_t::nord  ||  fahrtrichtung==ribi_t::west)  ) {
			steps_next = (steps_next/2)+1;
		}
		cnv->must_recalc_data_front();
	}

	// friction factors and speedlimit may have changed
	cnv->must_recalc_data();

	calc_akt_speed(gr);
}


/* calculates the current friction coefficient based on the curent track
 * flat, slope, curve ...
 * @author prissi, HJ, Dwachs
 */
void vehikel_t::calc_akt_speed(const grund_t *gr)
{

	// assume straight flat track
	current_friction = 1;

	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) {
		current_friction = 8;
	}

	// or a hill?
	const hang_t::typ hang = gr->get_weg_hang();
	if(hang!=hang_t::flach) {
		if(ribi_typ(hang)==fahrtrichtung) {
			// hill up, since height offsets are negative: heavy deccelerate
			current_friction += 23;
		}
		else {
			// hill down: accelerate
			current_friction += -13;
		}
	}
}


void vehikel_t::rauche() const
{
	// raucht ueberhaupt ?
	if(rauchen  &&  besch->get_rauch()) {

		bool smoke = besch->get_engine_type()==vehikel_besch_t::steam;

		if(!smoke) {
			// Hajo: only produce smoke when heavily accelerating
			//       or steam engine
			sint32 akt_speed = kmh_to_speed(besch->get_geschw());
			if(akt_speed > speed_limit) {
				akt_speed = speed_limit;
			}

			smoke = (cnv->get_akt_speed() < (sint32)((akt_speed*7u)>>3));
		}

		if(smoke) {
			grund_t * gr = welt->lookup( get_pos() );
			if(gr) {
				wolke_t *abgas =  new wolke_t(welt, get_pos(), get_xoff()+((dx*(sint16)((uint16)steps*OBJECT_OFFSET_STEPS))>>8), get_yoff()+((dy*(sint16)((uint16)steps*OBJECT_OFFSET_STEPS))>>8)+hoff, besch->get_rauch() );
				if(  !gr->obj_add(abgas)  ) {
					abgas->set_flag(ding_t::not_on_map);
					delete abgas;
				}
				else {
					welt->sync_way_eyecandy_add( abgas );
				}
			}
		}
	}
}


/**
 * Payment is done per hop. It iterates all goods and calculates
 * the income for the last hop. This method must be called upon
 * every stop.
 * @return income total for last hop
 * @author Hj. Malthaner
 */
sint64 vehikel_t::calc_gewinn(koord start, koord end) const
{
	// may happen when waiting in station
	if (start == end || fracht.empty()) {
		return 0;
	}

	//const long dist = abs(end.x - start.x) + abs(end.y - start.y);
	const sint32 ref_kmh = welt->get_average_speed( get_besch()->get_waytype() );

	// kmh_base = lesser of min_top_speed, power limited top speed, and average way speed limits on trip, except aircraft which are not power limited and don't have speed limits
	sint32 cnv_kmh = cnv->get_speedbonus_kmh();
	const sint32 kmh_base = (100 * cnv_kmh) / ref_kmh - 100;

	sint64 value = 0;

	if(  welt->get_settings().get_pay_for_total_distance_mode() == settings_t::TO_DESTINATION  ) {
		// pay only the distance, we get closer to our destination
		FOR(slist_tpl<ware_t>, const& ware, fracht) {
			if(  ware.menge==0  ) {
				continue;
			}

			// now only use the real gain in difference for the revenue (may as well be negative!)
			const koord &zwpos = ware.get_zielpos();
			// cast of koord_distance to sint32 is necessary otherwise the r-value would be interpreted as unsigned, leading to overflows
			const sint32 dist = (sint32)koord_distance( zwpos, start ) - (sint32)koord_distance( end, zwpos );

			const sint32 grundwert128 = ware.get_besch()->get_preis() * welt->get_settings().get_bonus_basefactor();	// bonus price will be always at least this
			const sint32 grundwert_bonus = (ware.get_besch()->get_preis()*(1000+kmh_base*ware.get_besch()->get_speed_bonus()));
			const sint64 price = (sint64)(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus) * (sint64)dist * (sint64)ware.menge;

			// sum up new price
			value += price;
		}
	} else if (welt->get_settings().get_pay_for_total_distance_mode() == settings_t::TO_TRANSFER) {
		// pay distance traveled to next trasnfer stop
		FOR(slist_tpl<ware_t>, const& ware, fracht) {
			if(ware.menge==0  ||  !ware.get_zwischenziel().is_bound()) {
				continue;
			}

			// now only use the real gain in difference for the revenue (may as well be negative!)
			const koord &zwpos = ware.get_zwischenziel()->get_basis_pos();
			// cast of koord_distance to sint32 is necessary otherwise the r-value would be interpreted as unsigned, leading to overflows
			const sint32 dist = (sint32)koord_distance( zwpos, start ) - (sint32)koord_distance( end, zwpos );

			const sint32 grundwert128 = ware.get_besch()->get_preis() * welt->get_settings().get_bonus_basefactor();	// bonus price will be always at least this
			const sint32 grundwert_bonus = (ware.get_besch()->get_preis()*(1000+kmh_base*ware.get_besch()->get_speed_bonus()));
			const sint64 price = (sint64)(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus) * (sint64)dist * (sint64)ware.menge;

			// sum up new price
			value += price;
		}
	}
	else {
		// pay distance traveled
		const long dist = koord_distance( start, end );
		FOR(slist_tpl<ware_t>, const& ware, fracht) {
			if(ware.menge==0  ||  !ware.get_zwischenziel().is_bound()) {
				continue;
			}

			// now only use the real gain in difference for the revenue (may as well be negative!)
			const sint32 grundwert128 = ware.get_besch()->get_preis() * welt->get_settings().get_bonus_basefactor();	// bonus price will be always at least this
			const sint32 grundwert_bonus = (ware.get_besch()->get_preis()*(1000+kmh_base*ware.get_besch()->get_speed_bonus()));
			const sint64 price = (sint64)(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus) * (sint64)dist * (sint64)ware.menge;

			// sum up new price
			value += price;
		}
	}

	// Hajo: Rounded value, in cents
	// prissi: Why on earth 1/3???
	return (value+1500ll)/3000ll;
}


const char *vehikel_t::get_fracht_mass() const
{
	return get_fracht_typ()->get_mass();
}


/**
 * Berechnet Gesamtgewicht der transportierten Fracht in KG
 * @author Hj. Malthaner
 */
uint32 vehikel_t::get_fracht_gewicht() const
{
	uint32 weight = 0;

	FOR(slist_tpl<ware_t>, const& c, fracht) {
		weight += c.menge * c.get_besch()->get_weight_per_unit();
	}
	return weight;
}


const char *vehikel_t::get_fracht_name() const
{
	return get_fracht_typ()->get_name();
}


void vehikel_t::get_fracht_info(cbuffer_t & buf) const
{
	if (fracht.empty()) {
		buf.append("  ");
		buf.append(translator::translate("leer"));
		buf.append("\n");
	} else {
		FOR(slist_tpl<ware_t>, const& ware, fracht) {
			const char * name = "Error in Routing";

			halthandle_t halt = ware.get_ziel();
			if(halt.is_bound()) {
				name = halt->get_name();
			}

			buf.printf("   %u%s %s > %s\n", ware.menge, translator::translate(ware.get_mass()), translator::translate(ware.get_name()), name);
		}
	}
}


void vehikel_t::loesche_fracht()
{
	FOR(  slist_tpl<ware_t>, w, fracht ) {
		fabrik_t::update_transit( &w, false );
	}
	fracht.clear();
}


bool vehikel_t::beladen(halthandle_t halt)
{
	bool ok = true;
	if(halt.is_bound()) {
		ok = load_freight(halt);
	}
	sum_weight =  get_fracht_gewicht() + besch->get_gewicht();
	calc_bild();
	return ok;
}


/**
 * fahrzeug an haltestelle entladen
 * @author Hj. Malthaner
 */
bool vehikel_t::entladen(halthandle_t halt)
{
	uint16 menge = unload_freight(halt);
	if(menge>0) {
		// add delivered goods to statistics
		cnv->book(menge, convoi_t::CONVOI_TRANSPORTED_GOODS);
		// add delivered goods to halt's statistics
		halt->book(menge, HALT_ARRIVED);
		return true;
	}
	return false;
}


/**
 * Ermittelt fahrtrichtung
 * @author Hj. Malthaner
 */
ribi_t::ribi vehikel_t::richtung() const
{
	ribi_t::ribi neu = calc_richtung(pos_prev.get_2d(), pos_next.get_2d());
	// nothing => use old direct further on
	return (neu == ribi_t::keine) ? fahrtrichtung : neu;
}


void vehikel_t::calc_bild()
{
	image_id old_bild=get_bild();
	if (fracht.empty()) {
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung()),NULL));
	}
	else {
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung()), fracht.front().get_besch()));
	}
	if(old_bild!=get_bild()) {
		set_flag(ding_t::dirty);
	}
}


void vehikel_t::rdwr(loadsave_t *file)
{
	// this is only called from dingliste => we save nothing ...
	assert(  file->is_saving()  );
}


void vehikel_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t r( file, "vehikel_t" );

	sint32 fracht_count = 0;

	if(file->is_saving()) {
		fracht_count = fracht.get_count();
		// we try to have one freight count to guess the right freight
		// when no besch is given
		if(fracht_count==0  &&  besch->get_ware()!=warenbauer_t::nichts  &&  besch->get_zuladung()>0) {
			fracht_count = 1;
		}
	}

	ding_t::rdwr(file);

	// since ding_t does no longer save positions
	if(  file->get_version()>=101000  ) {
		koord3d pos = get_pos();
		pos.rdwr(file);
		set_pos(pos);
	}

	if(file->get_version()<86006) {
		// parameter werden in der deklarierten reihenfolge gespeichert
		sint32 l;
		file->rdwr_long(insta_zeit);
		file->rdwr_long(l);
		dx = (sint8)l;
		file->rdwr_long(l);
		dy = (sint8)l;
		file->rdwr_long(l);
		hoff = (sint8)(l*TILE_HEIGHT_STEP/16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(fahrtrichtung);
		file->rdwr_enum(alte_fahrtrichtung);
		file->rdwr_long(fracht_count);
		file->rdwr_long(l);
		route_index = (uint16)l;
		insta_zeit = (insta_zeit >> welt->ticks_per_world_month_shift) + welt->get_settings().get_starting_year();
DBG_MESSAGE("vehicle_t::rdwr_from_convoi()","bought at %i/%i.",(insta_zeit%12)+1,insta_zeit/12);
	}
	else {
		// prissi: changed several data types to save runtime memory
		file->rdwr_long(insta_zeit);
		if(file->get_version()<99018) {
			file->rdwr_byte(dx);
			file->rdwr_byte(dy);
		}
		else {
			file->rdwr_byte(steps);
			file->rdwr_byte(steps_next);
			if(steps_next==old_diagonal_vehicle_steps_per_tile - 1  &&  file->is_loading()) {
				// reset diagonal length (convoi will be resetted anyway, if game diagonal is different)
				steps_next = diagonal_vehicle_steps_per_tile - 1;
			}
		}
		sint16 dummy16 = ((16*(sint16)hoff)/TILE_HEIGHT_STEP);
		file->rdwr_short(dummy16);
		hoff = (sint8)((TILE_HEIGHT_STEP*(sint16)dummy16)/16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(fahrtrichtung);
		file->rdwr_enum(alte_fahrtrichtung);
		file->rdwr_long(fracht_count);
		file->rdwr_short(route_index);
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];
	}

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx=get_xoff(), ddy=get_yoff()-hoff;
		sint8 i=1;
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];

		while(  !is_about_to_hop(ddx+dx*i,ddy+dy*i )  &&  i<16 ) {
			i++;
		}
		i--;
		set_xoff( ddx-(16-i)*dx );
		set_yoff( ddy-(16-i)*dy );
		if(file->is_loading()) {
			if(dx*dy) {
				steps = min( VEHICLE_STEPS_PER_TILE - 1, VEHICLE_STEPS_PER_TILE - 1-(i*16) );
				steps_next = VEHICLE_STEPS_PER_TILE - 1;
			}
			else {
				// will be corrected anyway, if in a convoi
				steps = min( diagonal_vehicle_steps_per_tile - 1, diagonal_vehicle_steps_per_tile - 1-(uint8)(((uint16)i*(uint16)(diagonal_vehicle_steps_per_tile - 1))/8) );
				steps_next = diagonal_vehicle_steps_per_tile - 1;
			}
		}
	}

	// information about the target halt
	if(file->get_version()>=88007) {
		bool target_info;
		if(file->is_loading()) {
			file->rdwr_bool(target_info);
			cnv = (convoi_t *)target_info;	// will be checked during convoi reassignement
		}
		else {
			target_info = target_halt.is_bound();
			file->rdwr_bool(target_info);
		}
	}
	else {
		if(file->is_loading()) {
			cnv = NULL;	// no reservation too
		}
	}
	pos_prev.rdwr(file);

	if(file->get_version()<=99004) {
		koord3d dummy;
		dummy.rdwr(file);	// current pos (is already saved as ding => ignore)
	}
	pos_next.rdwr(file);

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		besch = vehikelbauer_t::get_info(s);
		if(besch==NULL) {
			besch = vehikelbauer_t::get_info(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			welt->add_missing_paks( s, karte_t::MISSING_VEHICLE );
			dbg->warning("vehikel_t::rdwr_from_convoi()","no vehicle pak for '%s' search for something similar", s);
		}
	}

	if(file->is_saving()) {
		if (fracht.empty()  &&  fracht_count>0) {
			// create dummy freight for savegame compatibility
			ware_t ware( besch->get_ware() );
			ware.menge = 0;
			ware.set_ziel( halthandle_t() );
			ware.set_zwischenziel( halthandle_t() );
			ware.set_zielpos( get_pos().get_2d() );
			ware.rdwr(welt,file);
		}
		else {
			FOR(slist_tpl<ware_t>, ware, fracht) {
				ware.rdwr(welt,file);
			}
		}
	}
	else {
		for(int i=0; i<fracht_count; i++) {
			ware_t ware(welt,file);
			if(  (besch==NULL  ||  ware.menge>0)  &&  welt->ist_in_kartengrenzen(ware.get_zielpos())  ) {	// also add, of the besch is unknown to find matching replacement
				fracht.insert(ware);
				if(  file->get_version() <= 112000  ) {
					// restore intransit information
					fabrik_t::update_transit( &ware, true );
				}
			}
			else if(  ware.menge>0  ) {
				dbg->error( "vehikel_t::rdwr_from_convoi()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
			}
		}
	}

	// skip first last info (the convoi will know this better than we!)
	if(file->get_version()<88007) {
		bool dummy = 0;
		file->rdwr_bool(dummy);
		file->rdwr_bool(dummy);
	}

	// koordinate of the last stop
	if(file->get_version()>=99015) {
		last_stop_pos.rdwr(file);
	}

	if(file->is_loading()) {
		ist_erstes = ist_letztes = false;	// dummy, will be setz by convoi afterwards
		if(besch) {
			calc_bild();
			// full weight after loading
			sum_weight =  get_fracht_gewicht() + besch->get_gewicht();
		}
		// recalc total freight
		total_freight = 0;
		FOR(slist_tpl<ware_t>, const& c, fracht) {
			total_freight += c.menge;
		}
	}

	if(  file->get_version()>=110000  ) {
		bool hd = has_driven;
		file->rdwr_bool( hd );
		has_driven = hd;
	}
	else {
		if (file->is_loading()) {
			has_driven = false;
		}
	}
}


uint32 vehikel_t::calc_restwert() const
{
	// if already used, there is a general price reduction
	double value = (double)besch->get_preis();
	if(  has_driven  ) {
		value *= (1000 - welt->get_settings().get_used_vehicle_reduction()) / 1000.0;
	}
	// after 20 year, it has only half value
	return (uint32)( value * pow(0.997, (int)(welt->get_current_month() - get_insta_zeit())));
}


void vehikel_t::zeige_info()
{
	if(  cnv != NULL  ) {
		cnv->zeige_info();
	} else {
		dbg->warning("vehikel_t::zeige_info()","cnv is null, can't open convoi window!");
	}
}


void vehikel_t::info(cbuffer_t & buf) const
{
	if(cnv) {
		cnv->info(buf);
	}
}


const char *vehikel_t::ist_entfernbar(const spieler_t *)
{
	return "Fahrzeuge koennen so nicht entfernt werden";
}


vehikel_t::~vehikel_t()
{
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		// remove vehicle's marker from the reliefmap
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}
}


// this routine will display a tooltip for lost, on depot order and stucked vehicles
void vehikel_t::display_after(int xpos, int ypos, bool is_gobal) const
{
	if(is_gobal  &&  cnv  &&  ist_erstes) {

		COLOR_VAL color = COL_GREEN; // not used, but stop compiler warning about uninitialized
		char tooltip_text[1024];
		tooltip_text[0] = 0;
		uint8 state = umgebung_t::show_vehicle_states;
		if(  state==1  ) {
			// only show when mouse over vehicle
			if(  welt->get_zeiger()->get_pos()==get_pos()  ) {
				state = 2;
			}
			else {
				state = 0;
			}
		}

		// now find out what has happend
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::WAITING_FOR_CLEARANCE:
			case convoi_t::CAN_START:
			case convoi_t::CAN_START_ONE_MONTH:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Waiting for clearance!"), lengthof(tooltip_text) );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::LOADING:
				if(  state>=1  ) {
					sprintf( tooltip_text, translator::translate("Loading (%i->%i%%)!"), cnv->get_loading_level(), cnv->get_loading_limit() );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::FAHRPLANEINGABE:
//			case convoi_t::ROUTING_1:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Schedule changing!"), lengthof(tooltip_text) );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::DRIVING:
				if(  state>=1  ) {
					grund_t const* const gr = welt->lookup(cnv->get_route()->back());
					if(  gr  &&  gr->get_depot()  ) {
						tstrncpy( tooltip_text, translator::translate("go home"), lengthof(tooltip_text) );
						color = COL_GREEN;
					}
					else if(  cnv->get_no_load()  ) {
						tstrncpy( tooltip_text, translator::translate("no load"), lengthof(tooltip_text) );
						color = COL_GREEN;
					}
				}
				break;

			case convoi_t::LEAVING_DEPOT:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Leaving depot!"), lengthof(tooltip_text) );
					color = COL_GREEN;
				}
				break;

			case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			case convoi_t::CAN_START_TWO_MONTHS:
				tstrncpy( tooltip_text, translator::translate("clf_chk_stucked"), lengthof(tooltip_text) );
				color = COL_ORANGE;
				break;

			case convoi_t::NO_ROUTE:
				tstrncpy( tooltip_text, translator::translate("clf_chk_noroute"), lengthof(tooltip_text) );
				color = COL_RED;
				break;
		}

		// something to show?
		if(  tooltip_text[0]  ) {
			const int width = proportional_string_width(tooltip_text)+7;
			const int raster_width = get_current_tile_raster_width();
			get_screen_offset( xpos, ypos, raster_width );
			xpos += tile_raster_scale_x(get_xoff(), raster_width);
			ypos += tile_raster_scale_y(get_yoff(), raster_width)+14;
			if(ypos>LINESPACE+32  &&  ypos+LINESPACE<display_get_clip_wh().yy) {
				display_ddd_proportional_clip( xpos, ypos, width, 0, color, COL_BLACK, tooltip_text, true);
			}
		}
	}
}


/*--------------------------- Fahrdings ------------------------------*/


automobil_t::automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
	drives_on_left = welt->get_settings().is_drive_left();
}


automobil_t::automobil_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			const ware_besch_t* w = (!fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere);
			dbg->warning("automobil_t::automobil_t()","try to find a fitting vehicle for %s.",  w->get_name() );
			besch = vehikelbauer_t::get_best_matching(road_wt, 0, (fracht.empty() ? 0 : 50), is_first?50:0, speed_to_kmh(speed_limit), w, true, last_besch, is_last );
			if(besch) {
				DBG_MESSAGE("automobil_t::automobil_t()","replaced by %s",besch->get_name());
				// still wrong load ...
				calc_bild();
			}
			if(!fracht.empty()  &&  fracht.front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht.remove_first();
			}
		}
		if(  besch  ) {
			last_besch = besch;
		}
	}
	drives_on_left = welt->get_settings().is_drive_left();
}


// need to reset halt reservation (if there was one)
bool automobil_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	assert(cnv);
	// free target reservation
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
	if(ist_erstes   &&  alte_fahrtrichtung!=ribi_t::keine  &&  cnv  &&  target_halt.is_bound() ) {
		// now reserve our choice (beware: might be longer than one tile!)
		for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<cnv->get_route()->get_count();  length++  ) {
			target_halt->unreserve_position( welt->lookup( cnv->get_route()->position_bei( cnv->get_route()->get_count()-length-1) ), cnv->self );
		}
	}
	target_halt = halthandle_t();	// no block reserved
	route_t::route_result_t r = route->calc_route(welt, start, ziel, this, max_speed, cnv->get_tile_length() );
	if(  r == route_t::valid_route_halt_too_short  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("Vehicle %s cannot choose because stop too short!"), cnv->get_name());
		welt->get_message()->add_message( (const char *)buf, ziel.get_2d(), message_t::traffic_jams, PLAYER_FLAG | cnv->get_besitzer()->get_player_nr(), cnv->front()->get_basis_bild() );
	}
	return r;
}


bool automobil_t::ist_befahrbar(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->get_weg(road_wt);
	if(str==NULL  ||  str->get_max_speed()==0) {
		return false;
	}
	bool electric = cnv!=NULL  ?  cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric;
	if(electric  &&  !str->is_electrified()) {
		return false;
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			if(  rs->get_besch()->get_min_speed()>0  &&  rs->get_besch()->get_min_speed()>kmh_to_speed(get_besch()->get_geschw())  ) {
				return false;
			}
			if(  rs->get_besch()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// prvate road
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound()  &&  cnv->is_waiting()  &&  rs->get_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA) {
				return false;
			}
		}
	}
	return true;
}


// how expensive to go here (for way search)
// author prissi
int automobil_t::get_kosten(const grund_t *gr, const sint32 max_speed, koord from_pos) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(road_wt);
	if(!w) {
		return 0xFFFF;
	}

	// max_speed?
	sint32 max_tile_speed = w->get_max_speed();

	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 : 4-(3*max_tile_speed)/max_speed;

	// assume all traffic is not good ... (otherwise even smoke counts ... )
	costs += (w->get_statistics(WAY_STAT_CONVOIS)  >  ( 2 << (welt->get_settings().get_bits_per_month()-16) )  );

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		if(  hang_t::is_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y )  ) {
			costs += 15;
		}
	}
	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool automobil_t::ist_ziel(const grund_t *gr, const grund_t *prev_gr) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
		// now we must check the precessor => try to advance as much as possible
		if(prev_gr!=NULL) {
			const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
			ribi_t::ribi ribi = ribi_typ(dir);
			if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ) {
				// one way sign wrong direction
				return false;
			}
			grund_t *to;
			if(  !gr->get_neighbour(to,road_wt,ribi)  ||  !(to->get_halt()==target_halt)  ||  (gr->get_weg(get_waytype())->get_ribi_maske() & ribi_typ(dir))!=0  ||  !target_halt->is_reservable(to,cnv->self)  ) {
				// end of stop: Is it long enough?
				uint16 tiles = cnv->get_tile_length();
				while(  tiles>1  ) {
					if(  !gr->get_neighbour(to,get_waytype(),ribi_t::rueckwaerts(ribi))  ||  !(to->get_halt()==target_halt)  ||  !target_halt->is_reservable(to,cnv->self)  ) {
						return false;
					}
					gr = to;
					tiles --;
				}
				return true;
			}
			// can advance more
			return false;
		}
//DBG_MESSAGE("is_target()","success at %i,%i",gr->get_pos().x,gr->get_pos().y);
//		return true;
	}
	return false;
}


// to make smaller steps than the tile granularity, we have to use this trick
void automobil_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	vehikel_basis_t::get_screen_offset( xoff, yoff, raster_width );

	// eventually shift position to take care of overtaking
	if(cnv) {
		if(  cnv->is_overtaking()  ) {
			xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width);
			yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width);
		}
		else if(  cnv->is_overtaken()  ) {
			xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width)/5;
			yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width)/5;
		}
	}
}


// chooses a route at a choose sign; returns true on success
bool automobil_t::choose_route( int &restart_speed, ribi_t::dir richtung, uint16 index )
{
	route_t *rt = cnv->access_route();
	// is our target occupied?
	target_halt = haltestelle_t::get_halt( welt, rt->back(), get_besitzer() );
	if(  target_halt.is_bound()  ) {

		// since convois can long than one tile, check is more difficult
		bool can_go_there = true;
		bool original_route = (rt->back() == cnv->get_schedule()->get_current_eintrag().pos);
		for(  uint32 length=0;  can_go_there  &&  length<cnv->get_tile_length()  &&  length+1<rt->get_count();  length++  ) {
			if(  grund_t *gr = welt->lookup( rt->position_bei( rt->get_count()-length-1) )  ) {
				if (gr->get_halt().is_bound()) {
					can_go_there &= target_halt->is_reservable( gr, cnv->self );
				}
				else {
					// if this is the original stop, it is too short!
					can_go_there |= original_route;
				}
			}
		}
		if(  can_go_there  ) {
			// then reserve it ...
			for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<rt->get_count();  length++  ) {
				target_halt->reserve_position( welt->lookup( rt->position_bei( rt->get_count()-length-1) ), cnv->self );
			}
		}
		else {
			// cannot go there => need slot search

			// if we fail, we will wait in a step, much more simulation friendly
			if(!cnv->is_waiting()) {
				restart_speed = -1;
				target_halt = halthandle_t();
				return false;
			}

			// check if there is a free position
			// this is much faster than waysearch
			if(  !target_halt->find_free_position(road_wt,cnv->self,ding_t::automobil  )) {
				restart_speed = 0;
				target_halt = halthandle_t();
				return false;
			}

			// now it make sense to search a route
			route_t target_rt;
			koord3d next3d = rt->position_bei(index);
			if(  !target_rt.find_route( welt, next3d, this, speed_to_kmh(cnv->get_min_top_speed()), richtung, 33 )  ) {
				// nothing empty or not route with less than 33 tiles
				target_halt = halthandle_t();
				restart_speed = 0;
				return false;
			}

			// now reserve our choice (beware: might be longer than one tile!)
			for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<target_rt.get_count();  length++  ) {
				target_halt->reserve_position( welt->lookup( target_rt.position_bei( target_rt.get_count()-length-1) ), cnv->self );
			}
			rt->remove_koord_from( index );
			rt->append( &target_rt );
		}
	}
	return true;
}


bool automobil_t::ist_weg_frei(int &restart_speed, bool second_check)
{
	// check for traffic lights (only relevant for the first car in a convoi)
	if(  ist_erstes  ) {
		// no further check, when already entered a crossing (to allow leaving it)
		const grund_t *gr = welt->lookup(get_pos());
		if(  gr  &&  gr->ist_uebergang()  &&  !second_check  ) {
			return true;
		}

		gr = welt->lookup(pos_next);
		if(  !gr  ) {
			// weg not existent (likely destroyed)
			if(  !second_check  ) {
				cnv->suche_neue_route();
			}
			return false;
		}

		// pruefe auf Schienenkreuzung
		const strasse_t *str = (strasse_t *)gr->get_weg(road_wt);
		if(  !str  ||  gr->get_top() > 250  ) {
			// too many cars here or no street
			return false;
		}

		// first: check roadsigns
		const roadsign_t *rs = NULL;
		if(str->has_sign()) {
			rs = gr->find<roadsign_t>();
			if(rs) {
				// since at the corner, our direction may be diagonal, we make it straight
				const uint8 richtung = ribi_typ(get_pos().get_2d(),pos_next.get_2d());

				if(rs->get_besch()->is_traffic_light()  &&  (rs->get_dir()&richtung)==0) {
					// wait here
					restart_speed = 16;
					return false;
				}
				// check, if we reached a choose point
				else if(  rs->is_free_route(richtung)  &&  !target_halt.is_bound()  ) {
					if(  second_check  ) {
						return false;
					}
					if(  !choose_route( restart_speed, richtung, route_index )  ) {
						return false;
					}
				}
			}
		}

		vehikel_basis_t *dt = NULL;
		uint32 test_index = route_index + 1u;

		// way should be clear for overtaking: we checked previously
		if(  !cnv->is_overtaking()  ) {
			// calculate new direction
			route_t const& r = *cnv->get_route();
			koord next = (route_index < r.get_count() - 1u ? r.position_bei(route_index + 1u) : pos_next).get_2d();
			ribi_t::ribi curr_fahrtrichtung   = get_fahrtrichtung();
			ribi_t::ribi curr_90fahrtrichtung = calc_richtung(get_pos().get_2d(), pos_next.get_2d());
			ribi_t::ribi next_fahrtrichtung   = calc_richtung(get_pos().get_2d(), next);
			ribi_t::ribi next_90fahrtrichtung = calc_richtung(pos_next.get_2d(), next);
			dt = no_cars_blocking( gr, cnv, curr_fahrtrichtung, next_fahrtrichtung, next_90fahrtrichtung );

			// do not block intersections
			bool int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90fahrtrichtung) : ribi_t::rotate90(curr_90fahrtrichtung)) & str->get_ribi_unmasked())  ||  curr_90fahrtrichtung != next_90fahrtrichtung  ||  (rs  &&  rs->get_besch()->is_traffic_light()));

			// check exit from crossings and intersections, allow to proceed after 4 consecutive
			while(  !dt   &&  (str->is_crossing()  ||  int_block)  &&  test_index < r.get_count()  &&  test_index < route_index + 4u  ) {
				if(  str->is_crossing()  ) {
					crossing_t* cr = gr->find<crossing_t>(2);
					if(  !cr->request_crossing(this)  ) {
						restart_speed = 0;
						return false;
					}
				}

				// test next position
				gr = welt->lookup(r.position_bei(test_index));
				if(  !gr  ) {
					// weg not existent (likely destroyed)
					if(  !second_check  ) {
						cnv->suche_neue_route();
					}
					return false;
				}

				str = (strasse_t *)gr->get_weg(road_wt);
				if(  !str  ||  gr->get_top() > 250  ) {
					// too many cars here or no street
					return false;
				}

				// check cars
				curr_fahrtrichtung   = next_fahrtrichtung;
				curr_90fahrtrichtung = next_90fahrtrichtung;
				if(  test_index + 1u < r.get_count()  ) {
					next                 = r.position_bei(test_index + 1u).get_2d();
					next_fahrtrichtung   = calc_richtung(r.position_bei(test_index - 1u).get_2d(), next);
					next_90fahrtrichtung = calc_richtung(r.position_bei(test_index).get_2d(),     next);
					dt = no_cars_blocking( gr, cnv, curr_fahrtrichtung, next_fahrtrichtung, next_90fahrtrichtung );
				}
				else {
					next                 = r.position_bei(test_index).get_2d();
					next_90fahrtrichtung = calc_richtung(r.position_bei(test_index - 1u).get_2d(), next);
					if(  curr_fahrtrichtung == next_90fahrtrichtung  ||  !gr->is_halt()  ) {
						// check cars but allow to enter intersecion if we are turning even when a car is blocking the halt on the last tile of our route
						// preserves old bus terminal behaviour
						dt = no_cars_blocking( gr, cnv, curr_fahrtrichtung, next_90fahrtrichtung, ribi_t::keine );
					}
				}

				// check roadsigns
				if(  str->has_sign()  ) {
					rs = gr->find<roadsign_t>();
					if(  rs  ) {
						// since at the corner, our direction may be diagonal, we make it straight
						if(  rs->get_besch()->is_traffic_light()  &&  (rs->get_dir() & curr_90fahrtrichtung)==0  ) {
							// wait here
							restart_speed = 16;
							return false;
						}
						// check, if we reached a choose point

						else if(  rs->is_free_route(curr_90fahrtrichtung)  &&  !target_halt.is_bound()  ) {
							if(  second_check  ) {
								return false;
							}
							if(  !choose_route( restart_speed, curr_90fahrtrichtung, test_index )  ) {
								return false;
							}
						}
					}
				}
				else {
					rs = NULL;
				}

				// check for blocking intersection
				int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90fahrtrichtung) : ribi_t::rotate90(curr_90fahrtrichtung)) & str->get_ribi_unmasked())  ||  curr_90fahrtrichtung != next_90fahrtrichtung  ||  (rs  &&  rs->get_besch()->is_traffic_light()));

				test_index++;
			}

			if(  dt  &&  test_index > route_index + 1u  &&  !str->is_crossing()  &&  !int_block  ) {
				// found a car blocking us after checking at least 1 intersection or crossing
				// and the car is in a place we could stop. So if it can move, assume it will, so we will too.
				if(  automobil_t const* const car = ding_cast<automobil_t>(dt)  ) {
					const convoi_t* const ocnv = car->get_convoi();
					int dummy;
					if(  ocnv->front()->get_route_index() < ocnv->get_route()->get_count()  &&  ocnv->front()->ist_weg_frei(dummy, true)  ) {
						return true;
					}
				}
			}
		}

		// stuck message ...
		if(  dt  &&  !second_check  ) {
			if(  dt->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehikel is stuck too
				restart_speed = 0;
				cnv->set_tiles_overtaking(0);
				cnv->reset_waiting();
			}
			else {
				if(  test_index == route_index + 1u  ) {
					// no intersections or crossings, we might be able to overtake this one ...
					overtaker_t *over = dt->get_overtaker();
					if(  over  &&  !over->is_overtaken()  ) {
						if(  over->is_overtaking()  ) {
							// otherwise we would stop every time being overtaken
							return true;
						}
						// not overtaking/being overtake: we need to make a more thourough test!
						if(  automobil_t const* const car = ding_cast<automobil_t>(dt)  ) {
							convoi_t* const ocnv = car->get_convoi();
							if(  cnv->can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
								return true;
							}
						}
						else if(  stadtauto_t* const caut = ding_cast<stadtauto_t>(dt)  ) {
							if(  cnv->can_overtake(caut, caut->get_besch()->get_geschw(), VEHICLE_STEPS_PER_TILE)  ) {
								return true;
							}
						}
					}
				}
				// we have to wait ...
				restart_speed = (cnv->get_akt_speed()*3)/4;
				cnv->set_tiles_overtaking(0);
			}
		}

		return dt==NULL;
	}

	return true;
}


void automobil_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	const int cargo = get_fracht_menge();
	weg_t *str = welt->lookup( get_pos() )->get_weg(road_wt);
	str->book(cargo, WAY_STAT_GOODS);
	if (ist_erstes)  {
		str->book(1, WAY_STAT_CONVOIS);
		cnv->update_tiles_overtaking();
	}
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
}


schedule_t * automobil_t::erzeuge_neuen_fahrplan() const
{
  return new autofahrplan_t();
}


void automobil_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("automobil_t::set_convoi()","%p",c);
	if(c!=NULL) {
		bool target=(bool)cnv;	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
		vehikel_t::set_convoi(c);
		if(target  &&  ist_erstes  &&  c->get_route()->empty()) {
			// reintitialize the target halt
			const route_t *rt = cnv->get_route();
			target_halt = haltestelle_t::get_halt( welt, rt->back(), get_besitzer() );
			if(  target_halt.is_bound()  ) {
				for(  uint32 i=0;  i<c->get_tile_length()  &&  i+1<rt->get_count();  i++  ) {
					target_halt->reserve_position( welt->lookup( rt->position_bei(rt->get_count()-i-1) ), cnv->self );
				}
			}
		}
	}
	else {
		if(  cnv  &&  ist_erstes  &&  target_halt.is_bound()  ) {
			// now reserve our choice (beware: might be longer than one tile!)
			for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<cnv->get_route()->get_count();  length++  ) {
				target_halt->unreserve_position( welt->lookup( cnv->get_route()->position_bei( cnv->get_route()->get_count()-length-1) ), cnv->self );
			}
			target_halt = halthandle_t();
		}
		cnv = NULL;
	}
}


/* from now on rail vehicles (and other vehicles using blocks) */
waggon_t::waggon_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	vehikel_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			int power = (is_first || fracht.empty() || fracht.front() == warenbauer_t::nichts) ? 500 : 0;
			const ware_besch_t* w = fracht.empty() ? warenbauer_t::nichts : fracht.front().get_besch();
			dbg->warning("waggon_t::waggon_t()","try to find a fitting vehicle for %s.", power>0 ? "engine": w->get_name() );
			if(last_besch!=NULL  &&  last_besch->can_follow(last_besch)  &&  last_besch->get_ware()==w  &&  (!is_last  ||  last_besch->get_nachfolger(0)==NULL)) {
				// same as previously ...
				besch = last_besch;
			}
			else {
				// we have to search
				besch = vehikelbauer_t::get_best_matching(get_waytype(), 0, w!=warenbauer_t::nichts?5000:0, power, speed_to_kmh(speed_limit), w, false, last_besch, is_last );
			}
			if(besch) {
DBG_MESSAGE("waggon_t::waggon_t()","replaced by %s",besch->get_name());
				calc_bild();
			}
			else {
				dbg->error("waggon_t::waggon_t()","no matching besch found for %s!",w->get_name());
			}
			if (!fracht.empty() && fracht.front().menge == 0) {
				// this was only there to find a matchin vehicle
				fracht.remove_first();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}


waggon_t::waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
    cnv = cn;
}


waggon_t::~waggon_t()
{
	if (cnv && ist_erstes) {
		route_t const& r = *cnv->get_route();
		if (!r.empty() && route_index < r.get_count()) {
			// free all reserved blocks
			uint16 dummy;
			block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, target_halt.is_bound() ? 100000 : 1, false, false);
		}
	}
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		schiene_t * sch = (schiene_t *)gr->get_weg(get_waytype());
		if(sch) {
			sch->unreserve(this);
		}
	}
}


void waggon_t::set_convoi(convoi_t *c)
{
	if(c!=cnv) {
		DBG_MESSAGE("waggon_t::set_convoi()","new=%p old=%p",c,cnv);
		if(ist_erstes) {
			if(cnv!=NULL  &&  cnv!=(convoi_t *)1) {
				// free route from old convoi
				route_t const& r = *cnv->get_route();
				if(  !r.empty()  &&  route_index + 1U < r.get_count() - 1  ) {
					uint16 dummy;
					block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, 100000, false, false);
					target_halt = halthandle_t();
				}
			}
			else if(  c->get_next_reservation_index()==0  ) {
				assert(c!=NULL);
				// eventually search new route
				route_t const& r = *c->get_route();
				if(  (r.get_count()<=route_index  ||  r.empty()  ||  get_pos()==r.back())  &&  c->get_state()!=convoi_t::INITIAL  &&  c->get_state()!=convoi_t::LOADING  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  ) {
					check_for_finish = true;
					dbg->warning("waggon_t::set_convoi()", "convoi %i had a too high route index! (%i of max %i)", c->self.get_id(), route_index, r.get_count() - 1);
				}
				// set default next stop indext
				c->set_next_stop_index( max(route_index,1)-1 );
				// need to reserve new route?
				if(  !check_for_finish  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  &&  (c->get_state()==convoi_t::DRIVING  ||  c->get_state()>=convoi_t::LEAVING_DEPOT)  ) {
					long num_index = cnv==(convoi_t *)1 ? 1001 : 0; 	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
					uint16 next_signal, next_crossing;
					cnv = c;
					if(  block_reserver(&r, max(route_index,1)-1, next_signal, next_crossing, num_index, true, false)  ) {
						c->set_next_stop_index( next_signal>next_crossing ? next_crossing : next_signal );
					}
				}
			}
		}
		vehikel_t::set_convoi(c);
	}
}


// need to reset halt reservation (if there was one)
bool waggon_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	if(ist_erstes  &&  route_index<cnv->get_route()->get_count()) {
		// free all reserved blocks
		uint16 dummy;
		block_reserver(cnv->get_route(), cnv->back()->get_route_index(), dummy, dummy, target_halt.is_bound() ? 100000 : 1, false, true);
	}
	cnv->set_next_reservation_index( 0 );	// nothing to reserve
	target_halt = halthandle_t();	// no block reserved
	// use length 8888 tiles to advance to the end of all stations
	return route->calc_route(welt, start, ziel, this, max_speed, 8888 /*cnv->get_tile_length()*/ );
}


bool waggon_t::ist_befahrbar(const grund_t *bd) const
{
	schiene_t const* const sch = ding_cast<schiene_t>(bd->get_weg(get_waytype()));
	if(  !sch  ) {
		return false;
	}

	// Hajo: diesel and steam engines can use electrifed track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric);
	if(  (!needs_no_electric  &&  !sch->is_electrified())  ||  sch->get_max_speed() == 0  ) {
		return false;
	}

	if (depot_t *depot = bd->get_depot()) {
		if (depot->get_waytype() != besch->get_waytype()  ||  depot->get_besitzer() != get_besitzer()) {
			return false;
		}
	}
	// now check for special signs
	if(sch->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(  rs->get_besch()->get_wtyp()==get_waytype()  ) {
			if(  rs->get_besch()->get_min_speed() > 0  &&  rs->get_besch()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(  rs->get_besch()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// prvate road
				return false;
			}
		}
	}

	if(  target_halt.is_bound()  &&  cnv->is_waiting()  ) {
		// we are searching a stop here:
		// ok, we can go where we already are ...
		if(bd->get_pos()==get_pos()) {
			return true;
		}
		// we cannot pass an end of choose area
		if(sch->has_sign()) {
			const roadsign_t* rs = bd->find<roadsign_t>();
			if(  rs->get_besch()->get_wtyp()==get_waytype()  ) {
				if(  rs->get_besch()->get_flags() & roadsign_besch_t::END_OF_CHOOSE_AREA  ) {
					return false;
				}
			}
		}
		// but we can only use empty blocks ...
		// now check, if we could enter here
		return sch->can_reserve(cnv->self);
	}

	return true;
}


// how expensive to go here (for way search)
// author prissi
int waggon_t::get_kosten(const grund_t *gr, const sint32 max_speed, koord from_pos) const
{
	// first favor faster ways
	const weg_t *w = gr->get_weg(get_waytype());
	if(  w==NULL  ) {
		// only occurs when deletion during waysearch
		return 999;
	}

	// add cost for going (with maximum speed, cost is 1)
	const sint32 max_tile_speed = w->get_max_speed();
	int costs = (max_speed<=max_tile_speed) ? 1 : 4-(3*max_tile_speed)/max_speed;

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		if(  hang_t::is_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y )  ) {
			costs += 25;
		}
	}

	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool waggon_t::ist_ziel(const grund_t *gr,const grund_t *prev_gr) const
{
	const schiene_t * sch1 = (const schiene_t *) gr->get_weg(get_waytype());
	// first check blocks, if we can go there
	if(  sch1->can_reserve(cnv->self)  ) {
		//  just check, if we reached a free stop position of this halt
		if(  gr->is_halt()  &&  gr->get_halt()==target_halt  ) {
			// now we must check the precessor ...
			if(  prev_gr!=NULL  ) {
				const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
				const ribi_t::ribi ribi = ribi_typ(dir);
				if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ) {
					// signal/one way sign wrong direction
					return false;
				}
				grund_t *to;
				if(  !gr->get_neighbour(to,get_waytype(),ribi)  ||  !(to->get_halt()==target_halt)  ||  (to->get_weg(get_waytype())->get_ribi_maske() & ribi_typ(dir))!=0  ) {
					// end of stop: Is it long enough?
					// end of stop could be also signal!
					uint16 tiles = cnv->get_tile_length();
					while(  tiles>1  ) {
						if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ||  !gr->get_neighbour(to,get_waytype(),ribi_t::rueckwaerts(ribi))  ||  !(to->get_halt()==target_halt)  ) {
							return false;
						}
						gr = to;
						tiles --;
					}
					return true;
				}
			}
		}
	}
	return false;
}


bool waggon_t::is_weg_frei_longblock_signal( signal_t *sig, uint16 next_block, int &restart_speed )
{
	// longblock signal: first check, wether there is a signal coming up on the route => just like normal signal
	uint16 next_signal, next_crossing;
	if(  !block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		// not even the "Normal" signal route part is free => no bother checking further on
		sig->set_zustand( roadsign_t::rot );
		restart_speed = 0;
		return false;
	}

	if(  next_signal != INVALID_INDEX  ) {
		// success, and there is a signal before end of route => finished
		sig->set_zustand( roadsign_t::gruen );
		cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		return true;
	}

	// no signal before end_of_route => need to do routesearch in a step
	if(  !cnv->is_waiting()  ) {
		restart_speed = -1;
		return false;
	}

	// now we can use the route seach array
	// (route until end is already reserved at this point!)
	uint8 fahrplan_index = cnv->get_schedule()->get_aktuell()+1;
	route_t target_rt;
	koord3d cur_pos = cnv->get_route()->back();
	uint16 dummy, next_next_signal;
	bool success = true;
	if(fahrplan_index >= cnv->get_schedule()->get_count()) {
		fahrplan_index = 0;
	}
	while(  fahrplan_index != cnv->get_schedule()->get_aktuell()  ) {
		// now search
		// search for route
		success = target_rt.calc_route( welt, cur_pos, cnv->get_schedule()->eintrag[fahrplan_index].pos, this, speed_to_kmh(cnv->get_min_top_speed()), 8888 /*cnv->get_tile_length()*/ );
		if(  success  ) {
			success = block_reserver( &target_rt, 1, next_next_signal, dummy, 0, true, false );
			block_reserver( &target_rt, 1, dummy, dummy, 0, false, false );
		}

		if(  success  ) {
			// ok, would be free
			if(  next_next_signal<target_rt.get_count()  ) {
				// and here is a signal => finished
				// (however, if it is this signal, we need to renew reservation ...
				if(  target_rt.position_bei(next_next_signal) == cnv->get_route()->position_bei( next_block )  ) {
					block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false );
				}
				sig->set_zustand( roadsign_t::gruen );
				cnv->set_next_stop_index( min( next_crossing, next_signal ) );
				return true;
			}
		}

		if(  !success  ) {
			block_reserver( cnv->get_route(), next_block+1, next_next_signal, dummy, 0, false, false );
			sig->set_zustand( roadsign_t::rot );
			restart_speed = 0;
			return false;
		}
		// prepare for next leg of schedule
		cur_pos = target_rt.back();
		fahrplan_index ++;
		if(fahrplan_index >= cnv->get_schedule()->get_count()) {
			fahrplan_index = 0;
		}
	}
	return true;
}


bool waggon_t::is_weg_frei_choose_signal( signal_t *sig, const uint16 start_block, int &restart_speed )
{
	uint16 next_signal, next_crossing;
	grund_t const* const target = welt->lookup(cnv->get_route()->back());
	if(  target==NULL  ) {
		cnv->suche_neue_route();
		return false;
	}

	// first check, if we are not heading to a waypoint
	bool choose_ok = target->get_halt().is_bound();
	target_halt = halthandle_t();

	// check, if there is another choose signal or end_of_choose on the route
	for(  uint32 idx=start_block+1;  choose_ok  &&  idx<cnv->get_route()->get_count();  idx++  ) {
		grund_t *gr = welt->lookup(cnv->get_route()->position_bei(idx));
		if(  gr==0  ) {
			choose_ok = false;
			break;
		}
		if(  gr->get_halt()==target->get_halt()  ) {
			target_halt = gr->get_halt();
			break;
		}
		weg_t *way = gr->get_weg(get_waytype());
		if(  way==0  ) {
			choose_ok = false;
			break;
		}
		if(  way->has_sign()  ) {
			roadsign_t *rs = gr->find<roadsign_t>(1);
			if(  rs  &&  rs->get_besch()->get_wtyp()==get_waytype()  ) {
				if(  (rs->get_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA)!=0  ) {
					// end of choose on route => not choosing here
					choose_ok = false;
				}
			}
		}
		if(  way->has_signal()  ) {
			signal_t *sig = gr->find<signal_t>(1);
			if(  sig  &&  sig->get_besch()->is_choose_sign()  ) {
				// second choose signal on route => not choosing here
				choose_ok = false;
			}
		}
	}

	if(  !choose_ok  ) {
		// just act as normal signal
		if(  block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 0, true, false )  ) {
			sig->set_zustand(  roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_zustand(  roadsign_t::rot );
		restart_speed = 0;
		return false;
	}

	target_halt = target->get_halt();
	if(  !block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 100000, true, false )  ) {
		// no free route to target!
		// note: any old reservations should be invalid after the block reserver call.
		// => We can now start freshly all over

		if(!cnv->is_waiting()) {
			restart_speed = -1;
			target_halt = halthandle_t();
			return false;
		}
		// now we are in a step and can use the route search array

		// now it we are in a step and can use the route search
		route_t target_rt;
		const int richtung = ribi_typ(get_pos().get_2d(),pos_next.get_2d());	// to avoid confusion at diagonals
#ifdef MAX_CHOOSE_BLOCK_TILES
		if(  !target_rt.find_route( welt, cnv->get_route()->position_bei(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, MAX_CHOOSE_BLOCK_TILES )  ) {
#else
		if(  !target_rt.find_route( welt, cnv->get_route()->position_bei(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, welt->get_groesse_x()+welt->get_groesse_y() )  ) {
#endif
			// nothing empty or not route with less than MAX_CHOOSE_BLOCK_TILES tiles
			target_halt = halthandle_t();
			sig->set_zustand(  roadsign_t::rot );
			restart_speed = 0;
			return false;
		}
		else {
			// try to alloc the whole route
			cnv->access_route()->remove_koord_from(start_block);
			cnv->access_route()->append( &target_rt );
			if(  !block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 100000, true, false )  ) {
				dbg->error( "waggon_t::is_weg_frei_choose_signal()", "could not reserved route after find_route!" );
				target_halt = halthandle_t();
				sig->set_zustand(  roadsign_t::rot );
				restart_speed = 0;
				return false;
			}
		}
		// reserved route to target
	}
	sig->set_zustand(  roadsign_t::gruen );
	cnv->set_next_stop_index( min( next_crossing, next_signal ) );
	return true;
}


bool waggon_t::is_weg_frei_pre_signal( signal_t *sig, uint16 next_block, int &restart_speed )
{
	// parse to next signal; if needed recurse, since we allow cascading
	uint16 next_signal, next_crossing;
	if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		if(  next_signal == INVALID_INDEX  ||  cnv->get_route()->position_bei(next_signal) == cnv->get_route()->back()  ||  is_weg_frei_signal( next_signal, restart_speed )  ) {
			// ok, end of route => we can go
			sig->set_zustand( roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_signal, next_crossing ) );
			return true;
		}
		// when we reached here, the way is aparently not free => relase reservation and set state to next free
		sig->set_zustand( roadsign_t::naechste_rot );
		block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, false, false );
		restart_speed = 0;
		return false;
	}
	// if we end up here, there was not even the next block free
	sig->set_zustand( roadsign_t::rot );
	restart_speed = 0;
	return false;
}


bool waggon_t::is_weg_frei_signal( uint16 next_block, int &restart_speed )
{
	// called, when there is a signal; will call other signal routines if needed
	grund_t *gr_next_block = welt->lookup(cnv->get_route()->position_bei(next_block));
	signal_t *sig = gr_next_block->find<signal_t>();
	if(  sig==NULL  ) {
		dbg->error( "waggon_t::is_weg_frei_signal()", "called at %s without a signal!", cnv->get_route()->position_bei(next_block).get_str() );
		return true;
	}

	// action depend on the next signal
	const roadsign_besch_t *sig_besch=sig->get_besch();
	uint16 next_signal, next_crossing;

	// simple signal: drive on, if next block is free
	if(  !sig_besch->is_longblock_signal()  &&  !sig_besch->is_choose_sign()  &&  !sig_besch->is_pre_signal()  ) {
		if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
			sig->set_zustand(  roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_zustand(  roadsign_t::rot );
		restart_speed = 0;
		return false;
	}

	if(  sig_besch->is_pre_signal()  ) {
		return is_weg_frei_pre_signal( sig, next_block, restart_speed );
	}

	if(  sig_besch->is_longblock_signal()  ) {
		return is_weg_frei_longblock_signal( sig, next_block, restart_speed );
	}

	if(  sig_besch->is_choose_sign()  ) {
		return is_weg_frei_choose_signal( sig, next_block, restart_speed );
	}

	dbg->error( "waggon_t::is_weg_frei_signal()", "falled through at signal at %s", cnv->get_route()->position_bei(next_block).get_str() );
	return false;
}


bool waggon_t::ist_weg_frei(int & restart_speed,bool)
{
	assert(ist_erstes);
	uint16 next_signal, next_crossing;
	if(  cnv->get_state()==convoi_t::CAN_START  ||  cnv->get_state()==convoi_t::CAN_START_ONE_MONTH  ||  cnv->get_state()==convoi_t::CAN_START_TWO_MONTHS  ) {
		// reserve first block at the start until the next signal
		grund_t *gr = welt->lookup( get_pos() );
		weg_t *w = gr ? gr->get_weg(get_waytype()) : NULL;
		if(  w==NULL  ||  !(w->has_signal()  ||  w->is_crossing())  ) {
			// free track => reserve up to next signal
			if(  !block_reserver(cnv->get_route(), max(route_index,1)-1, next_signal, next_crossing, 0, true, false )  ) {
				restart_speed = 0;
				return false;
			}
			cnv->set_next_stop_index( next_crossing<next_signal ? next_crossing : next_signal );
			return true;
		}
		cnv->set_next_stop_index( max(route_index,1)-1 );
		if(  steps<steps_next  ) {
			// not yet at tile border => can drive to signal safely
			return true;
		}
		// we start with a signal/crossing => use stuff below ...
	}

	const grund_t *gr = welt->lookup(pos_next);
	if(gr==NULL) {
		// weg not existent (likely destroyed)
		cnv->suche_neue_route();
		return false;
	}
	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	schiene_t *w = (schiene_t *)gr->get_weg(get_waytype());
	if(w==NULL) {
		return false;
	}

	/* this should happen only before signals ...
	 * but if it is already reserved, we can save lots of other checks later
	 */
	if(  !w->can_reserve(cnv->self)  ) {
		restart_speed = 0;
		return false;
	}

	// is there any signal/crossing to be resrved?
	uint16 next_block = cnv->get_next_stop_index()-1;
	if(  next_block >= cnv->get_route()->get_count()  ) {
		// no obstacle in the way => drive on ...
		return true;
	}

	// it happened in the past that a convoi has a next signal stored way after the current position!
	// this only happens in vorfahren, but we can cure this easily
	if(  next_block+1<route_index  ) {
		bool ok = block_reserver( cnv->get_route(), route_index, next_signal, next_crossing, 0, true, false );
		cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		return ok;
	}

	if(  next_block <= route_index+3  ) {
		koord3d block_pos=cnv->get_route()->position_bei(next_block);
		grund_t *gr_next_block = welt->lookup(block_pos);
		const schiene_t *sch1 = gr_next_block ? (const schiene_t *)gr_next_block->get_weg(get_waytype()) : NULL;
		if(sch1==NULL) {
			// weg not existent (likely destroyed)
			cnv->suche_neue_route();
			return false;
		}

		// Is a crossing?
		// note: crossing and signal might exist on same tile
		// so first check crossing
		if(  sch1->is_crossing()  ) {
			if(  crossing_t* cr = gr_next_block->find<crossing_t>(2)  ) {
				// ok, here is a draw/turnbridge ...
				bool ok = cr->request_crossing(this);
				if(!ok) {
					// cannot cross => wait here
					restart_speed = 0;
					return cnv->get_next_stop_index()>route_index+1;
				}
				else if(  !sch1->has_signal()  ) {
					// can reserve: find next place to do something and drive on
					if(  block_pos == cnv->get_route()->back()  ) {
						// is also last tile => go on ...
						cnv->set_next_stop_index( INVALID_INDEX );
						return true;
					}
					else if(  !block_reserver( cnv->get_route(), cnv->get_next_stop_index(), next_signal, next_crossing, 0, true, false )  ) {
						dbg->error( "waggon_t::ist_weg_frei()", "block not free but was reserved!" );
						return false;
					}
					cnv->set_next_stop_index( next_crossing<next_signal ? next_crossing : next_signal );
				}
			}
		}

		// next check for signal
		if(  sch1->has_signal()  ) {
			if(  !is_weg_frei_signal( next_block, restart_speed )  ) {
				// only return false, if we are directly in front of the signal
				return cnv->get_next_stop_index()>route_index;
			}
		}
	}
	return true;
}


/*
 * reserves or unreserves all blocks and returns the handle to the next block (if there)
 * if count is larger than 1, (and defined) maximum MAX_CHOOSE_BLOCK_TILES tiles will be checked
 * (freeing or reserving a choose signal path)
 * if (!reserve && force_unreserve) then unreserve everything till the end of the route
 * return the last checked block
 * @author prissi
 */
bool waggon_t::block_reserver(const route_t *route, uint16 start_index, uint16 &next_signal_index, uint16 &next_crossing_index, int count, bool reserve, bool force_unreserve  ) const
{
	bool success=true;
#ifdef MAX_CHOOSE_BLOCK_TILES
	int max_tiles=2*MAX_CHOOSE_BLOCK_TILES; // max tiles to check for choosesignals
#endif
	slist_tpl<grund_t *> signs;	// switch all signals on their way too ...

	if(start_index>=route->get_count()) {
		cnv->set_next_reservation_index( max(route->get_count(),1)-1 );
		return 0;
	}

	if(route->position_bei(start_index)==get_pos()  &&  reserve) {
		start_index++;
	}

	if(  !reserve  ) {
		cnv->set_next_reservation_index( start_index );
	}

	// find next blocksegment enroute
	uint16 i=start_index;
	uint16 skip_index=INVALID_INDEX;
	next_signal_index=INVALID_INDEX;
	next_crossing_index=INVALID_INDEX;
	bool unreserve_now = false;
	for ( ; success  &&  count>=0  &&  i<route->get_count(); i++) {

		koord3d pos = route->position_bei(i);
		grund_t *gr = welt->lookup(pos);
		schiene_t * sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;
		if(sch1==NULL  &&  reserve) {
			// reserve until the end of track
			break;
		}
		// we unreserve also nonexisting tiles! (may happen during deletion)

#ifdef MAX_CHOOSE_BLOCK_TILES
		max_tiles--;
		if(max_tiles<0  &&   count>1) {
			break;
		}
#endif
		if(reserve) {
			if(sch1->has_signal()) {
				if(count) {
					signs.append(gr);
				}
				count --;
				next_signal_index = i;
			}
			if(  !sch1->reserve( cnv->self, ribi_typ( route->position_bei(max(1u,i)-1u), route->position_bei(min(route->get_count()-1u,i+1u)) ) )  ) {
				success = false;
			}
			if(next_crossing_index==INVALID_INDEX  &&  sch1->is_crossing()) {
				next_crossing_index = i;
			}
		}
		else if(sch1) {
			if(!sch1->unreserve(cnv->self)) {
				if(unreserve_now) {
					// reached an reserved or free track => finished
					return false;
				}
			}
			else {
				// unreserve from here (used during sale, since there might be reserved tiles not freed)
				unreserve_now = !force_unreserve;
			}
			if(sch1->has_signal()) {
				signal_t* signal = gr->find<signal_t>();
				if(signal) {
					signal->set_zustand(roadsign_t::rot);
				}
			}
			if(sch1->is_crossing()) {
				gr->find<crossing_t>()->release_crossing(this);
			}
		}
	}

	if(!reserve) {
		return false;
	}
	// here we go only with reserve

//DBG_MESSAGE("block_reserver()","signals at %i, sucess=%d",next_signal_index,success);

	// free, in case of unreserve or no sucess in reservation
	if(!success) {
		// free reservation
		for ( int j=start_index; j<i; j++) {
			if(i!=skip_index) {
				schiene_t * sch1 = (schiene_t *)welt->lookup( route->position_bei(j))->get_weg(get_waytype());
				sch1->unreserve(cnv->self);
			}
		}
		cnv->set_next_reservation_index( start_index );
		return false;
	}

	// ok, switch everything green ...
	FOR(slist_tpl<grund_t*>, const g, signs) {
		if (signal_t* const signal = g->find<signal_t>()) {
			signal->set_zustand(roadsign_t::gruen);
		}
	}
	cnv->set_next_reservation_index( i );

	return true;
}


/* beware: we must unreserve railblocks ... */
void waggon_t::verlasse_feld()
{
	vehikel_t::verlasse_feld();
	// fix counters
	if(ist_letztes) {
		grund_t *gr = welt->lookup( get_pos() );
		if(gr) {
			schiene_t *sch0 = (schiene_t *) gr->get_weg(get_waytype());
			if(sch0) {
				sch0->unreserve(this);
				// tell next signal?
				// and swith to red
				if(sch0->has_signal()) {
					signal_t* sig = welt->lookup(get_pos())->find<signal_t>();
					if(sig) {
						sig->set_zustand(roadsign_t::rot);
					}
				}
			}
		}
	}
}


void waggon_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	if(  schiene_t *sch0 = (schiene_t *) welt->lookup(get_pos())->get_weg(get_waytype())  ) {
		// way statistics
		const int cargo = get_fracht_menge();
		sch0->book(cargo, WAY_STAT_GOODS);
		if(ist_erstes) {
			sch0->book(1, WAY_STAT_CONVOIS);
			sch0->reserve( cnv->self, get_fahrtrichtung() );
		}
	}
}


schedule_t * waggon_t::erzeuge_neuen_fahrplan() const
{
	return besch->get_waytype()==tram_wt ? new tramfahrplan_t() : new zugfahrplan_t();
}


schedule_t * monorail_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new monorailfahrplan_t();
}


schedule_t * maglev_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new maglevfahrplan_t();
}


schedule_t * narrowgauge_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new narrowgaugefahrplan_t();
}


schiff_t::schiff_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
}


schiff_t::schiff_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	vehikel_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			dbg->warning("schiff_t::schiff_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passagiere");
			besch = vehikelbauer_t::get_best_matching(water_wt, 0, fracht.empty() ? 0 : 30, 100, 40, !fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere, true, last_besch, is_last );
			if(besch) {
				calc_bild();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}


bool schiff_t::ist_befahrbar(const grund_t *bd) const
{
	if(  bd->ist_wasser()  ) {
		return true;
	}
	// channel can have more stuff to check
	const weg_t *w = bd->get_weg(water_wt);
#if ENABLE_WATERWAY_SIGNS
	if(  w  &&  w->has_sign()  ) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(  rs->get_besch()->get_wtyp()==get_waytype()  ) {
			if(  rs->get_besch()->get_min_speed() > 0  &&  rs->get_besch()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(  rs->get_besch()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// prvate road
				return false;
			}
		}
	}
#endif
	return (w  &&  w->get_max_speed()>0);
}


/* Since slopes are handled different for ships
 * @author prissi
 */
void schiff_t::calc_akt_speed(const grund_t *gr)
{
	// or a hill?
	if(gr->get_weg_hang()) {
		// hill up or down => in lock => deccelarte
		current_friction = 16;
	}
	else {
		// flat track
		current_friction = 1;
	}

	if(alte_fahrtrichtung != fahrtrichtung) {
		// curve: higher friction
		current_friction *= 2;
	}
}


bool schiff_t::ist_weg_frei(int &restart_speed,bool)
{
	restart_speed = -1;

	if(ist_erstes) {
		const grund_t *gr = welt->lookup( pos_next );
		if(gr==NULL) {
			// weg not existent (likely destroyed)
			cnv->suche_neue_route();
			return false;
		}
		weg_t *w = gr->get_weg(water_wt);
		if(w  &&  w->is_crossing()) {
			// ok, here is a draw/turnbridge ...
			crossing_t* cr = gr->find<crossing_t>();
			if(!cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}


schedule_t * schiff_t::erzeuge_neuen_fahrplan() const
{
  return new schifffahrplan_t();
}


/**** from here on planes ***/


// for flying thingies, everywhere is good ...
// another function only called during route searching
ribi_t::ribi aircraft_t::get_ribi(const grund_t *gr) const
{
	switch(state) {
		case taxiing:
		case looking_for_parking:
			return gr->get_weg_ribi(air_wt);

		case taxiing_to_halt:
		{
			// we must invert all one way signs here, since we start from the target position here!
			weg_t *w = gr->get_weg(air_wt);
			if(w) {
				ribi_t::ribi r = w->get_ribi_unmasked();
				if(  ribi_t::ribi mask = w->get_ribi_maske()  ) {
					r &= mask;
				}
				return r;
			}
			return ribi_t::keine;
		}

		case landing:
		case departing:
		{
			ribi_t::ribi dir = gr->get_weg_ribi(air_wt);
			if(dir==0) {
				return ribi_t::alle;
			}
			return dir;
		}

		case flying:
		case circling:
			return ribi_t::alle;
	}
	return ribi_t::keine;
}


// how expensive to go here (for way search)
// author prissi
int aircraft_t::get_kosten(const grund_t *gr, const sint32, koord) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(air_wt);
	int costs = 0;

	if(state==flying) {
		if(w==NULL) {
			costs += 1;
		}
		else {
			if(w->get_besch()->get_styp()==0) {
				costs += 25;
			}
		}
	}
	else {
		// only, if not flying ...
		assert(w);

		if(w->get_besch()->get_styp()==0) {
			costs += 3;
		}
		else {
			costs += 2;
		}
	}

	return costs;
}


// whether the ground is drivable or not depends on the current state of the airplane
bool aircraft_t::ist_befahrbar(const grund_t *bd) const
{
	switch (state) {
		case taxiing:
		case taxiing_to_halt:
		case looking_for_parking:
//DBG_MESSAGE("ist_befahrbar()","at %i,%i",bd->get_pos().x,bd->get_pos().y);
			return (bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_max_speed()>0);

		case landing:
		case departing:
		case flying:
		case circling:
		{
//DBG_MESSAGE("aircraft_t::ist_befahrbar()","(cnv %i) in idx %i",cnv->self.get_id(),route_index );
			// prissi: here a height check could avoid too height montains
			return true;
		}
	}
	return false;
}


// this routine is called by find_route, to determined if we reached a destination
bool aircraft_t::ist_ziel(const grund_t *gr,const grund_t *) const
{
	if(state!=looking_for_parking  ||  !target_halt.is_bound()) {
		// search for the end of the runway
		const weg_t *w=gr->get_weg(air_wt);
		if(w  &&  w->get_besch()->get_styp()==1) {
			// ok here is a runway
			ribi_t::ribi ribi= w->get_ribi_unmasked();
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&approach_dir)!=0) {
				// pointing in our direction
				// here we should check for length, but we assume everything is ok
				return true;
			}
		}
	}
	else {
		// otherwise we just check, if we reached a free stop position of this halt
		if(gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
			return true;
		}
	}
	return false;
}


/* finds a free stop, calculates a route and reserve the position
 * else return false
 */
bool aircraft_t::find_route_to_stop_position()
{
	if(target_halt.is_bound()) {
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","bound! (cnv %i)",cnv->self.get_id());
		return true;	// already searched with success
	}

	// check for skipping circle
	route_t *rt=cnv->access_route();

//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","can approach? (cnv %i)",cnv->self.get_id());

	grund_t const* const last = welt->lookup(rt->back());
	target_halt = last ? last->get_halt() : halthandle_t();
	if(!target_halt.is_bound()) {
		return true;	// no halt to search
	}

	// then: check if the search point is still on a runway (otherwise just proceed)
	grund_t const* const target = welt->lookup(rt->position_bei(suchen));
	if(target==NULL  ||  !target->hat_weg(air_wt)) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no runway found at (%s)",rt->position_bei(suchen).get_str());
		return true;	// no runway any more ...
	}

	// is our target occupied?
//	DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","state %i",state);
	if(!target_halt->find_free_position(air_wt,cnv->self,ding_t::aircraft)  ) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no free prosition found!");
		return false;
	}
	else {
		// calculate route to free position:

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not reentrant!
		if(!cnv->is_waiting()) {
			target_halt = halthandle_t();
			return false;
		}

		// now search a route
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","some free: find route from index %i",suchen);
		route_t target_rt;
		flight_state prev_state = state;
		state = looking_for_parking;
		if(!target_rt.find_route( welt, rt->position_bei(suchen), this, 500, ribi_t::alle, 100 )) {
DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","found no route to free one");
			// circle slowly another round ...
			target_halt = halthandle_t();
			state = prev_state;
			return false;
		}
		state = prev_state;

		// now reserve our choice ...
		target_halt->reserve_position(welt->lookup(target_rt.back()), cnv->self);
		//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()", "found free stop near %i,%i,%i", target_rt.back().x, target_rt.back().y, target_rt.back().z);
		rt->remove_koord_from(suchen);
		rt->append( &target_rt );
		return true;
	}
}


// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
bool aircraft_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
//DBG_MESSAGE("aircraft_t::calc_route()","search route from %i,%i,%i to %i,%i,%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	if(ist_erstes  &&  cnv) {
		// free target reservation
		if(  target_halt.is_bound() ) {
			if (grund_t* const target = welt->lookup(cnv->get_route()->back())) {
				target_halt->unreserve_position(target,cnv->self);
			}
		}
		// free runway reservation
		if(route_index>=takeoff  &&  route_index<touchdown-21  &&  state!=flying) {
			block_reserver( takeoff, takeoff+100, false );
		}
		else if(route_index>=touchdown-1  &&  state!=taxiing) {
			block_reserver( touchdown, suchen+1, false );
		}
	}
	target_halt = halthandle_t();	// no block reserved

	const weg_t *w=welt->lookup(start)->get_weg(air_wt);
	bool start_in_the_air = (w==NULL);
	bool end_in_air=false;

	suchen = takeoff = touchdown = 0x7ffffffful;
	if(!start_in_the_air) {

		// see, if we find a direct route: We are finished
		state = taxiing;
		if(route->calc_route( welt, start, ziel, this, max_speed, 0 )) {
			// ok, we can taxi to our location
			return true;
		}
	}

	if(start_in_the_air  ||  (w->get_besch()->get_styp()==1  &&  ribi_t::ist_einfach(w->get_ribi())) ) {
		// we start here, if we are in the air or at the end of a runway
		search_start = start;
		start_in_the_air = true;
		route->clear();
//DBG_MESSAGE("aircraft_t::calc_route()","start in air at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}
	else {
		// not found and we are not on the takeoff tile (where the route search will fail too) => we try to calculate a complete route, starting with the way to the runway

		// second: find start runway end
		state = taxiing;
#ifdef USE_DIFFERENT_WIND
		approach_dir = get_approach_ribi( ziel, start );	// reverse
//DBG_MESSAGE("aircraft_t::calc_route()","search runway start near %i,%i,%i with corner in %x",start.x,start.y,start.z, approach_dir);
#else
		approach_dir = ribi_t::nordost;	// reverse
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near (%s)",start.get_str());
#endif
		if(!route->find_route( welt, start, this, max_speed, ribi_t::alle, 100 )) {
			DBG_MESSAGE("aircraft_t::calc_route()","failed");
			return false;
		}
		// save the route
		search_start = route->back();
		//DBG_MESSAGE("aircraft_t::calc_route()","start at ground (%s)",search_start.get_str());
	}

	// second: find target runway end

	state = taxiing_to_halt;	// only used for search

#ifdef USE_DIFFERENT_WIND
	approach_dir = get_approach_ribi( start, ziel );	// reverse
	//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z,approach_dir);
#else
	approach_dir = ribi_t::suedwest;	// reverse
	//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
#endif
	route_t end_route;

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::alle, 100 )) {
		// well, probably this is a waypoint
		if(  grund_t *target = welt->lookup(ziel)  ) {
			if(  !target->get_weg(air_wt)  ) {
				end_in_air = true;
				search_end = ziel;
			}
			else {
				// we have a taxiway/illegal runway here we cannot reach
				return false;	// no route!
			}
		}
		else {
			// illegal coordinates?
			return false;
		}
	}
	else {
		// save target route
		search_end = end_route.back();
	}
	//DBG_MESSAGE("aircraft_t::calc_route()","end at ground (%s)",search_end.get_str());

	// create target route
	if(!start_in_the_air) {
		takeoff = route->get_count()-1;
		koord start_dir(welt->lookup(search_start)->get_weg_ribi(air_wt));
		if(start_dir!=koord(0,0)) {
			// add the start
			ribi_t::ribi start_ribi = ribi_t::rueckwaerts(ribi_typ(start_dir));
			const grund_t *gr=NULL;
			// add the start
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->ist_in_kartengrenzen(search_start.get_2d()+(start_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_start.get_2d()+(start_dir*endi));
				if(over<3  ||  (gr->get_weg_ribi(air_wt)&start_ribi)==0) {
					over --;
				}
				endi ++;
				route->append(gr->get_pos());
			} while(  over>0  );
			// out of map
			if(gr==NULL) {
				dbg->error("aircraft_t::calc_route()","out of map!");
				return false;
			}
			// need some extra step to avoid 180 deg turns
			if( start_dir.x!=0  &&  sgn(start_dir.x)!=sgn(search_end.x-search_start.x)  ) {
				route->append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 1 : -1 ) )->get_pos() );
				route->append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->get_pos() );
			}
			else if( start_dir.y!=0  &&  sgn(start_dir.y)!=sgn(search_end.y-search_start.y)  ) {
				route->append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 1 : -1 ,0) )->get_pos() );
				route->append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 2 : -2 ,0) )->get_pos() );
			}
		}
		else {
			// init with startpos
			dbg->error("aircraft_t::calc_route()","Invalid route calculation: start is on a single direction field ...");
		}
		state = taxiing;
		flughoehe = 0;
		target_height = (sint16)get_pos().z*TILE_HEIGHT_STEP;
	}
	else {
		// init with current pos (in air ... )
		route->clear();
		route->append( get_pos() );
		state = flying;
		if(flughoehe==0) {
			flughoehe = 3*TILE_HEIGHT_STEP;
		}
		takeoff = 0;
		target_height = ((sint16)get_pos().z+3)*TILE_HEIGHT_STEP;
	}

//DBG_MESSAGE("aircraft_t::calc_route()","take off ok");

	koord3d landing_start=search_end;
	if(!end_in_air) {
		// now find way to start of landing pos
		ribi_t::ribi end_ribi = welt->lookup(search_end)->get_weg_ribi(air_wt);
		koord end_dir(end_ribi);
		end_ribi = ribi_t::rueckwaerts(end_ribi);
		if(end_dir!=koord(0,0)) {
			// add the start
			const grund_t *gr;
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->ist_in_kartengrenzen(search_end.get_2d()+(end_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_end.get_2d()+(end_dir*endi));
				if(over<3  ||  (gr->get_weg_ribi(air_wt)&end_ribi)==0) {
					over --;
				}
				endi ++;
				landing_start = gr->get_pos();
			} while(  over>0  );
		}
	}
	else {
		suchen = touchdown = 0x7FFFFFFFul;
	}

	// just some straight routes ...
	if(!route->append_straight_route(welt,landing_start)) {
		// should never fail ...
		dbg->error( "aircraft_t::calc_route()", "No straight route found!" );
		return false;
	}

	if(!end_in_air) {

		// find starting direction
		int offset = 0;
		switch(welt->lookup(search_end)->get_weg_ribi(air_wt)) {
			case ribi_t::nord: offset = 0; break;
			case ribi_t::west: offset = 4; break;
			case ribi_t::sued: offset = 8; break;
			case ribi_t::ost: offset = 12; break;
		}

		// now make a curve
		koord circlepos=landing_start.get_2d();
		static const koord circle_koord[16]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		for(  int  i=0;  i<16;  i++  ) {
			circlepos += circle_koord[(offset+i+16)%16];
			if(welt->ist_in_kartengrenzen(circlepos)) {
				route->append( welt->lookup_kartenboden(circlepos)->get_pos() );
			}
			else {
				// could only happen during loading old savegames;
				// in new versions it should not possible to build a runway here
				route->clear();
				dbg->error("aircraft_t::calc_route()","airport too close to the edge! (Cannot go to %i,%i!)",circlepos.x,circlepos.y);
				return false;
			}
		}

		touchdown = route->get_count()+2;
		route->append_straight_route(welt,search_end);

		// now the route rearch point (+1, since it will check before entering the tile ...)
		suchen = route->get_count()-1;

		// now we just append the rest
		for( int i=end_route.get_count()-2;  i>=0;  i--  ) {
			route->append(end_route.position_bei(i));
		}
	}

//DBG_MESSAGE("aircraft_t::calc_route()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route->get_count()-1, state );
	return true;
}


/* reserves runways (reserve true) or removes the reservation
 * finishes when reaching end tile or leaving the ground (end of runway)
 * @return true if the reservation is successfull
 */
bool aircraft_t::block_reserver( uint32 start, uint32 end, bool reserve ) const
{
	bool start_now = false;
	bool success = true;
	uint32 i;

	const route_t *route = cnv->get_route();
	if(route->empty()) {
		return false;
	}

	for(  uint32 i=start;  success  &&  i<end  &&  i<route->get_count();  i++) {

		grund_t *gr = welt->lookup(route->position_bei(i));
		runway_t * sch1 = gr ? (runway_t *)gr->get_weg(air_wt) : NULL;
		if(sch1==NULL) {
			if(reserve) {
				if(!start_now) {
					// touched down here
					start = i;
				}
				else {
					// most likely left the ground here ...
					end = i;
					break;
				}
			}
		}
		else {
			// we unreserve also nonexisting tiles! (may happen during deletion)
			if(reserve) {
				start_now = true;
				if(!sch1->reserve(cnv->self,ribi_t::keine)) {
					// unsuccessful => must unreserve all
					success = false;
					end = i;
					break;
				}
				// end of runway?
				if(i>start  &&  ribi_t::ist_einfach(sch1->get_ribi_unmasked())  ) {
					return true;
				}
			}
			else if(!sch1->unreserve(cnv->self)) {
				if(start_now) {
					// reached an reserved or free track => finished
					return true;
				}
			}
			else {
				// unreserve from here (only used during sale, since there might be reserved tiles not freed)
				start_now = true;
			}
		}
	}

	// unreserve if not successful
	if(!success  &&  reserve) {
		for(  i=start;  i<end;  i++  ) {
			grund_t *gr = welt->lookup(route->position_bei(i));
			if (gr) {
				runway_t* sch1 = (runway_t *)gr->get_weg(air_wt);
				if (sch1) {
					sch1->unreserve(cnv->self);
				}
			}
		}
	}
	return success;
}


// handles all the decisions on the ground an in the air
bool aircraft_t::ist_weg_frei( int & restart_speed, bool )
{
	restart_speed = -1;

	grund_t *gr = welt->lookup( pos_next );
	if(gr==NULL) {
		// weg not existent (likely destroyed)
		cnv->suche_neue_route();
		return false;
	}

	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	if(route_index<takeoff  &&  route_index>1  &&  takeoff<cnv->get_route()->get_count()-1) {
		// check, if tile occupied by a plane on ground
		if(  route_index > 1  ) {
			for(  uint8 i = 1;  i<gr->get_top();  i++  ) {
				ding_t *d = gr->obj_bei(i);
				if(  d->get_typ()==ding_t::aircraft  &&  ((aircraft_t *)d)->is_on_ground()  ) {
					restart_speed = 0;
					return false;
				}
			}
		}
		// need to reserve runway?
		runway_t *rw = (runway_t *)gr->get_weg(air_wt);
		if(rw==NULL) {
			cnv->suche_neue_route();
			return false;
		}
		// next tile a runway => then reserve
		if(rw->get_besch()->get_styp()==1) {
			// try to reserve the runway
			if(!block_reserver(takeoff,takeoff+100,true)) {
				// runway already blocked ...
				restart_speed = 0;
				return false;
			}
		}
		return true;
	}

	if(route_index==takeoff  &&  state==taxiing) {
		// try to reserve the runway if not already done
		if(route_index==2  &&  !block_reserver(takeoff,takeoff+100,true)) {
			// runway blocked, wait at start of runway
			restart_speed = 0;
			return false;
		}
		// stop shortly at the end of the runway
		state = departing;
		restart_speed = 0;
		return false;
	}

//DBG_MESSAGE("aircraft_t::ist_weg_frei()","index %i<>%i",route_index,touchdown);

	// check for another circle ...
	if(  route_index==(touchdown-3)  ) {
		if(  !block_reserver( touchdown, suchen+1, true )  ) {
			route_index -= 16;
			return true;
		}
		state = landing;
		return true;
	}

	if(  route_index==touchdown-16-3  &&  state!=circling  ) {
		// just check, if the end of runway ist free; we will wait there
		if(  block_reserver( touchdown, suchen+1, true )  ) {
			route_index += 16;
			// can land => set landing height
			state = landing;
		}
		else {
			// circle slowly next round
			state = circling;
			cnv->must_recalc_data();
			if(  ist_erstes  ) {
				cnv->must_recalc_data_front();
			}
		}
	}

	if(route_index==suchen  &&  state==landing  &&  !target_halt.is_bound()) {

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not reentrant!
		if(!cnv->is_waiting()) {
			return false;
		}

		// nothing free here?
		if(find_route_to_stop_position()) {
			// stop reservation successful
			block_reserver( touchdown, suchen+1, false );
			state = taxiing;
			return true;
		}
		restart_speed = 0;
		return false;
	}

	if(state==looking_for_parking) {
		state = taxiing;
	}

	if(state == taxiing  &&  gr->is_halt()  &&  gr->find<aircraft_t>()) {
		// the next step is a parking position. We do not enter, if occupied!
		restart_speed = 0;
		return false;
	}

	return true;
}


// this must also change the internal modes for the calculation
void aircraft_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	if(  this->is_on_ground()  ) {
		runway_t *w=(runway_t *)welt->lookup(get_pos())->get_weg(air_wt);
		if(w) {
			const int cargo = get_fracht_menge();
			w->book(cargo, WAY_STAT_GOODS);
			if (ist_erstes) {
				w->book(1, WAY_STAT_CONVOIS);
			}
		}
	}
}


aircraft_t::aircraft_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	rdwr_from_convoi(file);
	old_x = old_y = -1;
	old_bild = IMG_LEER;

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			dbg->warning("aircraft_t::aircraft_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passagiere");
			besch = vehikelbauer_t::get_best_matching(air_wt, 0, 101, 1000, 800, !fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere, true, last_besch, is_last );
			if(besch) {
				calc_bild();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}


aircraft_t::aircraft_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
	state = taxiing;
	old_x = old_y = -1;
	old_bild = IMG_LEER;
	flughoehe = 0;
	target_height = pos.z;
}


aircraft_t::~aircraft_t()
{
	// mark aircraft (after_image) dirty, since we have no "real" image
	mark_image_dirty( bild, -flughoehe-hoff-2 );
	mark_image_dirty( bild, 0 );
}


void aircraft_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("aircraft_t::set_convoi()","%p",c);
	if(ist_erstes  &&  (unsigned long)cnv > 1) {
		// free stop reservation
		route_t const& r = *cnv->get_route();
		if(target_halt.is_bound()) {
			target_halt->unreserve_position(welt->lookup(r.back()), cnv->self);
			target_halt = halthandle_t();
		}
		if (!r.empty()) {
			// free runway reservation
			if(route_index>=takeoff  &&  route_index<touchdown-4  &&  state!=flying) {
				block_reserver( takeoff, takeoff+100, false );
			}
			else if(route_index>=touchdown-1  &&  state!=taxiing) {
				block_reserver( touchdown, suchen+1, false );
			}
		}
	}
	// maybe need to restore state?
	if(c!=NULL) {
		bool target=(bool)cnv;
		vehikel_t::set_convoi(c);
		if(ist_erstes) {
			if(target) {
				// reintitialize the target halt
				grund_t* const target=welt->lookup(cnv->get_route()->back());
				target_halt = target->get_halt();
				if(target_halt.is_bound()) {
					target_halt->reserve_position(target,cnv->self);
				}
			}
			// restore reservation
			if(  grund_t *gr = welt->lookup(get_pos())  ) {
				if(  weg_t *weg = gr->get_weg(air_wt)  ) {
					if(  weg->get_besch()->get_styp()==1  ) {
						// but only if we are on a runway ...
						if(  route_index>=takeoff  &&  route_index<touchdown-21  &&  state!=flying  ) {
							block_reserver( takeoff, takeoff+100, true );
						}
						else if(  route_index>=touchdown-1  &&  state!=taxiing  ) {
							block_reserver( touchdown, suchen+1, true );
						}
					}
				}
			}
		}
	}
	else {
		vehikel_t::set_convoi(NULL);
	}
}


schedule_t *aircraft_t::erzeuge_neuen_fahrplan() const
{
	return new airfahrplan_t();
}


void aircraft_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t t( file, "aircraft_t" );

	// initialize as vehikel_t::rdwr_from_convoi calls get_bild()
	if (file->is_loading()) {
		state = taxiing;
		flughoehe = 0;
	}
	vehikel_t::rdwr_from_convoi(file);

	file->rdwr_enum(state);
	file->rdwr_short(flughoehe);
	flughoehe &= ~(TILE_HEIGHT_STEP-1);
	file->rdwr_short(target_height);
	file->rdwr_long(suchen);
	file->rdwr_long(touchdown);
	file->rdwr_long(takeoff);
}


#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two diffrent directions for the runway search
uint8 aircraft_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_typ( (koord)((ziel-start).get_2d()) );	// reverse
	// make sure, there are at last two directions to choose, or you might en up with not route
	if(ribi_t::ist_einfach(dir)) {
		dir |= (dir<<1);
		if(dir>16) {
			dir += 1;
		}
	}
	return dir&0x0F;
}
#endif


void aircraft_t::hop()
{
	if(  !get_flag(ding_t::dirty)  ) {
		mark_image_dirty( bild, get_yoff()-flughoehe-hoff-2 );
	}

	sint32 new_speed_limit = SPEED_UNLIMITED;
	sint32 new_friction = 0;

	// take care of inflight height ...
	const sint16 h_cur = height_scaling((sint16)get_pos().z)*TILE_HEIGHT_STEP;
	const sint16 h_next = height_scaling((sint16)pos_next.z)*TILE_HEIGHT_STEP;

	switch(state) {
		case departing: {
			flughoehe = 0;
			target_height = h_cur;
			new_friction = max( 1, 28/(1+(route_index-takeoff)*2) ); // 9 5 4 3 2 2 1 1...

			// take off, when a) end of runway or b) last tile of runway or c) fast enough
			weg_t *weg=welt->lookup(get_pos())->get_weg(air_wt);
			if(  (weg==NULL  ||  // end of runway (broken runway)
				 weg->get_besch()->get_styp()!=1  ||  // end of runway (grass now ... )
				 (route_index>takeoff+1  &&  ribi_t::ist_einfach(weg->get_ribi_unmasked())) )  ||  // single ribi at end of runway
				 cnv->get_akt_speed()>kmh_to_speed(besch->get_geschw())/3 // fast enough
			) {
				state = flying;
				new_friction = 1;
				block_reserver( takeoff, takeoff+100, false );
				flughoehe = h_cur - h_next;
				target_height = h_cur+TILE_HEIGHT_STEP*3;
			}
			break;
		}
		case circling: {
			new_speed_limit = kmh_to_speed(besch->get_geschw())/3;
			new_friction = 4;
			// do not change height any more while circling
			flughoehe += h_cur;
			flughoehe -= h_next;
			break;
		}
		case flying: {
			// since we are at a tile border, round up to the nearest value
			flughoehe += h_cur;
			if(  flughoehe < target_height  ) {
				flughoehe = (flughoehe+TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
			}
			else if(  flughoehe > target_height  ) {
				flughoehe = (flughoehe-TILE_HEIGHT_STEP);
			}
			flughoehe -= h_next;
			// did we have to change our flight height?
			if(  target_height-h_next > TILE_HEIGHT_STEP*5  ) {
				// sinken
				target_height -= TILE_HEIGHT_STEP*2;
			}
			else if(  target_height-h_next < TILE_HEIGHT_STEP*2  ) {
				// steigen
				target_height += TILE_HEIGHT_STEP*2;
			}
			break;
		}
		case landing: {
			flughoehe += h_cur;
			new_speed_limit = kmh_to_speed(besch->get_geschw())/3; // ==approach speed
			new_friction = 8;
			if(  flughoehe > target_height  ) {
				// still decenting
				flughoehe = (flughoehe-TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
				flughoehe -= h_next;
			}
			else if(  flughoehe == h_cur  ) {
				// touchdown!
				flughoehe = 0;
				target_height = h_next;
				const sint32 taxi_speed = kmh_to_speed( min( 60, besch->get_geschw()/4 ) );
				if(  cnv->get_akt_speed() <= taxi_speed  ) {
					new_speed_limit = taxi_speed;
					new_friction = 16;
				}
				else {
					const sint32 runway_left = suchen - route_index;
					new_speed_limit = min( new_speed_limit, runway_left*runway_left*taxi_speed ); // ...approach 540 240 60 60
					const sint32 runway_left_fr = max( 0, 6-runway_left );
					new_friction = max( new_friction, min( besch->get_geschw()/12, 4 + 4*(runway_left_fr*runway_left_fr+1) )); // ...8 8 12 24 44 72 108 152
				}
			}
			else {
				const sint16 landehoehe = height_scaling(cnv->get_route()->position_bei(touchdown).z) + (touchdown-route_index)*TILE_HEIGHT_STEP;
				if(landehoehe<=flughoehe) {
					target_height = height_scaling((sint16)cnv->get_route()->position_bei(touchdown).z)*TILE_HEIGHT_STEP;
				}
				flughoehe -= h_next;
			}
			break;
		}
		default: {
			new_speed_limit = kmh_to_speed( min( 60, besch->get_geschw()/4 ) );
			new_friction = 16;
			flughoehe = 0;
			target_height = h_next;
			break;
		}
	}

	// hop to next tile
	vehikel_t::hop();

	speed_limit = new_speed_limit;
	current_friction = new_friction;
}


// this routine will display the aircraft (if in flight)
void aircraft_t::display_after(int xpos_org, int ypos_org, bool is_global) const
{
	if(bild != IMG_LEER  &&  !is_on_ground()) {
		int xpos = xpos_org, ypos = ypos_org;

		const int raster_width = get_current_tile_raster_width();
		const sint16 z = get_pos().z;
		if (z + flughoehe/TILE_HEIGHT_STEP - 1 > grund_t::underground_level) {
			return;
		}
		const sint16 target = target_height - ((sint16)z*TILE_HEIGHT_STEP);
		sint16 current_flughohe = flughoehe;
		if(  current_flughohe < target  ) {
			current_flughohe += (steps*TILE_HEIGHT_STEP) >> 8;
		}
		else if(  current_flughohe > target  ) {
			current_flughohe -= (steps*TILE_HEIGHT_STEP) >> 8;
		}

		ypos += tile_raster_scale_y(get_yoff()-current_flughohe-hoff-2, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		get_screen_offset( xpos, ypos, raster_width );

		// will be dirty
		// the aircraft!!!
		display_color(bild, xpos, ypos, get_player_nr(), true, true/*get_flag(ding_t::dirty)*/ );

		vehikel_t::display_after( xpos_org, ypos_org-tile_raster_scale_y(current_flughohe-hoff-2, raster_width), is_global );
	}
	else if(  is_on_ground()  ) {
		// show loading toolstips on ground
		vehikel_t::display_after( xpos_org, ypos_org, is_global );
	}
}


const char * aircraft_t::ist_entfernbar(const spieler_t *sp)
{
	if (is_on_ground()) {
		return vehikel_t::ist_entfernbar(sp);
	}
	return NULL;
}
