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
#include "../simunits.h"

#include "../player/simplay.h"
#include "../simfab.h"
#include "../simware.h"
#include "../simhalt.h"
#include "../simsound.h"
#include "../simcity.h"

#include "../simimg.h"
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
#include "../dataobj/way_constraints.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../simtools.h"
#include "../besch/ware_besch.h"

#include "../convoy.h"

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



void
vehikel_basis_t::verlasse_feld() //"leave field" (Google)
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
 * it will drive on as long as it can
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

		if	(has_hopped)
		{
			if (dx == 0)
			{
				if (dy > 0)
				{
					set_xoff( pos_prev.x != get_pos().x ? -OBJECT_OFFSET_STEPS : OBJECT_OFFSET_STEPS );
				}
				else
				{
					set_xoff( pos_prev.x != get_pos().x ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
				}
			}
			else
			{
				set_xoff( dx < 0 ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
			}

			if (dy == 0)
			{
				if (dx > 0)
				{
					set_yoff( pos_prev.y != get_pos().y ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );
				}
				else {
					set_yoff( pos_prev.y != get_pos().y ? -OBJECT_OFFSET_STEPS/2 : OBJECT_OFFSET_STEPS/2 );
				}
			}
			else
			{
				set_yoff( dy < 0 ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );
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
	update_bookkeeping(distance_travelled  >> YARDS_PER_VEHICLE_STEP_SHIFT );
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
	yoff += ((display_steps*dy) >> 10) + ((hoff*raster_width) >> 6);

	if(  drives_on_left  ) {
		const int drive_left_dir = ribi_t::get_dir(get_fahrtrichtung());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}
}


uint16 vehikel_basis_t::get_tile_steps(const koord &start, const koord &ende, /*out*/ ribi_t::ribi &richtung) const
{
	static const ribi_t::ribi ribis[3][3] = {
			{ribi_t::nordwest, ribi_t::nord,  ribi_t::nordost},
			{ribi_t::west,     ribi_t::keine, ribi_t::ost},
			{ribi_t::suedwest, ribi_t::sued,  ribi_t::suedost}
	};
	const sint8 di = sgn(ende.x - start.x);
	const sint8 dj = sgn(ende.y - start.y);
	richtung = ribis[dj + 1][di + 1];
	return di == 0 || dj == 0 ? VEHICLE_STEPS_PER_TILE : diagonal_vehicle_steps_per_tile;
}


// calcs new direction and applies it to the vehicles
ribi_t::ribi
vehikel_basis_t::calc_set_richtung(koord start, koord ende)
{
	ribi_t::ribi richtung = ribi_t::keine; //"richtung" = direction (Google); "keine" = none (Google)

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

ribi_t::ribi
vehikel_basis_t::calc_richtung(koord start, koord ende)
{/*
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
*/
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
	const uint8 top = gr->get_top();
	for(  uint8 pos=1;  pos<top;  pos++ ) {
		if (vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(pos))) {
			uint8 other_fahrtrichtung=255;
			bool other_moving = false;

			// check for car
			if (automobil_t const* const at = ding_cast<automobil_t>(v)) {
				// ignore ourself
				if(cnv==at->get_convoi()) {
					continue;
				}
				other_fahrtrichtung = at->get_fahrtrichtung();
				other_moving = at->get_convoi()->get_akt_speed() > kmh_to_speed(1);
			}
			// check for city car
			else if(v->get_waytype()==road_wt  &&  v->get_typ()!=ding_t::fussgaenger) {
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
				if( across && ((ribi_t::ist_orthogonal(current_90fahrtrichtung,other_fahrtrichtung) && other_moving) || (other_across && other_exit_opposite_side) || ((other_across||other_straight) && other_exit_same_side && other_moving) ) )  {
					// other turning across in front of us from orth entry dir'n   ~4%
					return v;
				}

				const bool headon = ribi_t::rueckwaerts(current_fahrtrichtung) == other_fahrtrichtung; // we're meeting the other headon
				const bool other_exit_across = (drives_on_left ? ribi_t::rotate90l(next_90fahrtrichtung) : ribi_t::rotate90(next_90fahrtrichtung)) == other_90fahrtrichtung; // other is exiting by turning across the opposite directions lane
				if( straight && (ribi_t::ist_orthogonal(current_90fahrtrichtung,other_fahrtrichtung) || (other_across && other_moving && (other_exit_across || (other_exit_same_side && !headon))) ) ) {
					// other turning across in front of us, but allow if other is stopped - duplicating historic behaviour   ~2%
					return v;
				} else if(  other_fahrtrichtung==current_fahrtrichtung && current_90fahrtrichtung==ribi_t::keine  ) {
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


sint16 vehikel_t::compare_directions(sint16 first_direction, sint16 second_direction) const
{
	//Returns difference between two directions in degrees
	sint16 difference = 0;
	if(first_direction > 360 || second_direction > 360 || first_direction < 0 || second_direction < 0) return 0; 
	//If directions > 360, this means that they are not supposed to be checked.
	if(first_direction > 180 && second_direction < 180)
	{
		sint16 tmp = first_direction - second_direction;
		if(tmp > 180) difference = 360 - tmp;
		else difference = tmp;
	}
	else if(first_direction < 180 && second_direction > 180)
	{
		sint16 tmp = second_direction - first_direction;
		if(tmp > 180) difference = 360 - tmp;
		else difference = tmp;
	}
	else if(first_direction > second_direction)
	{
		difference = first_direction - second_direction;
	}
	else
	{
		difference = second_direction - first_direction;
	}
	return difference;
}

void vehikel_t::rotate90()
{
	vehikel_basis_t::rotate90();
	alte_fahrtrichtung = ribi_t::rotate90( alte_fahrtrichtung );
	pos_prev.rotate90( welt->get_groesse_y()-1 );
	last_stop_pos.rotate90( welt->get_groesse_y()-1 );
	// now rotate the freight
	FOR(slist_tpl<ware_t>, & tmp, fracht) {
		koord k = tmp.get_zielpos();
		k.rotate90( welt->get_groesse_y()-1 );
		tmp.set_zielpos( k );
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
uint16
vehikel_t::unload_freight(halthandle_t halt)
{
	uint16 sum_menge = 0;

	if(halt->is_enabled(get_fracht_typ())) 
	{
		if (!fracht.empty())
		{
			halthandle_t end_halt;
			halthandle_t via_halt;
			vector_tpl<ware_t> kill_queue;
			uint8 count = 0;
			FOR(slist_tpl<ware_t>, & tmp, fracht) 
			{
				if(&tmp == NULL)
				{
					continue;
				}

				if(++count == 255)
				{
					INT_CHECK("simvehikel 793");
				}

				end_halt = tmp.get_ziel();
				via_halt = tmp.get_zwischenziel();
				
				// probleme mit fehlerhafter ware
				// vielleicht wurde zwischendurch die
				// Zielhaltestelle entfernt ?
				if(!end_halt.is_bound() || !via_halt.is_bound()) 
				{
					DBG_MESSAGE("vehikel_t::entladen()", "destination of %d %s is no longer reachable",tmp.menge,translator::translate(tmp.get_name()));
					kill_queue.append(tmp);
				} 

				else if(halt == end_halt || halt == via_halt) 
				{

					//		    printf("Liefere %d %s nach %s via %s an %s\n",
					//                           tmp->menge,
					//			   tmp->name(),
					//			   end_halt->get_name(),
					//			   via_halt->get_name(),
					//			   halt->get_name());

					// hier sollte nur ordentliche ware verabeitet werden
					// "here only tidy commodity should be processed" (Babelfish) 
					
					if(halt != end_halt && welt->get_settings().is_avoid_overcrowding() && tmp.is_passenger() && !halt->is_within_walking_distance_of(via_halt) && halt->is_overcrowded(tmp.get_besch()->get_catg_index()))
					{
						// Halt overcrowded - discard goods/passengers, and collect no revenue.
						// Experimetal 7.2 - also calculate a refund.

						if(tmp.get_origin().is_bound() && get_besitzer()->get_player_cash_int() > 0)
						{
							// Cannot refund unless we know the origin.
							// Also, ought not refund unless the player is solvent. 
							// Players ought not be put out of business by refunds, as this makes gameplay too unpredictable, 
							// especially in online games, where joining one player's network to another might lead to a large
							// influx of passengers which one of the networks cannot cope with.
							const uint16 distance = shortest_distance(halt->get_basis_pos(), tmp.get_origin()->get_basis_pos());

							// Refund is approximation: 2x distance at standard rate with no adjustments. 
							const sint64 refund_amount = ((tmp.menge * tmp.get_fare(distance) * 2000ll) + 1500ll) / 3000ll;

							current_revenue -= refund_amount;
							cnv->book(-refund_amount, convoi_t::CONVOI_PROFIT);
							cnv->book(-refund_amount, convoi_t::CONVOI_REFUNDS);
							get_besitzer()->buche(-refund_amount, COST_VEHICLE_RUN);
							if(cnv->get_line().is_bound())
							{
								cnv->get_line()->book(-refund_amount, LINE_REFUNDS);
								cnv->get_line()->book(-refund_amount, LINE_PROFIT);
								get_besitzer()->buche(-refund_amount, COST_VEHICLE_RUN);
							}
						}

						// Add passengers to unhappy passengers.
						if(tmp.is_passenger())
						{
							halt->add_pax_unhappy(tmp.menge);
						}
					}
					
					else
					{
						const uint32 menge = halt->liefere_an(tmp); //"supply" (Babelfish)
						sum_menge += menge;
						halt->unload_repeat_counter = 0;

						// Calculates the revenue for each packet. 
						// @author: jamespetts
						current_revenue += cnv->calc_revenue(tmp);

						// book delivered goods to destination
						if(end_halt == halt) 
						{
							// pax is always index 1
							const int categorie = tmp.get_index()>1 ? 2 : tmp.get_index();
							get_besitzer()->buche( menge, (player_cost)(COST_TRANSPORTED_PAS+categorie) );
							if(tmp.is_passenger())
							{
								// New for Experimental 7.2 - add happy passengers
								// to the origin station and transported passengers/mail
								// to the origin city only *after* they arrive at their 
								// destinations.
								if(tmp.get_origin().is_bound())
								{
									// Check required because Simutrans-Standard saved games
									// do not have origins. Also, the start halt might not
									// be in (or be fully in) a city.
									tmp.get_origin()->add_pax_happy(menge);
									koord origin_pos = tmp.get_origin()->get_basis_pos();
									stadt_t* origin_city = welt->get_city(origin_pos);
									if(!origin_city)
									{
										// The origin stop is not within a city. 
										// If the stop is located outside the city, but the passengers
										// come from a city, they will not record as transported.
										origin_pos = tmp.get_origin()->get_init_pos();
										origin_city = welt->get_city(origin_pos);
									}
									
									if(!origin_city)
									{
										for(uint8 i = 0; i < 16; i ++)
										{
											koord pos(origin_pos + origin_pos.second_neighbours[i]);
											origin_city = welt->get_city(pos);
											if(origin_city)
											{
												break;
											}
										}
									}
								
									if(origin_city)
									{
										origin_city->add_transported_passengers(menge);
									}
								}
							}
							else if(tmp.is_mail())
							{
								if(tmp.get_origin().is_bound())
								{
									// Check required because Simutrans-Standard saved games
									// do not have origins. Also, the start halt might not
									// be in (or be fully in) a city.
									koord origin_pos = tmp.get_origin()->get_basis_pos();
									stadt_t* origin_city = welt->get_city(origin_pos);
									if(!origin_city)
									{
										// The origin stop is not within a city. 
										// If the stop is located outside the city, but the passengers
										// come from a city, they will not record as transported.
										origin_pos = tmp.get_origin()->get_init_pos();
										origin_city = welt->get_city(origin_pos);
									}
									
									if(!origin_city)
									{
										for(uint8 i = 0; i < 16; i ++)
										{
											koord pos(origin_pos + origin_pos.second_neighbours[i]);
											origin_city = welt->get_city(pos);
											if(origin_city)
											{
												break;
											}
										}
									}
									if(origin_city)
									{
										origin_city->add_transported_mail(menge);
									}
								}
							}
						}
					}				
					kill_queue.append(tmp);

					INT_CHECK("simvehikel 955");
				}
			}
			
			ITERATE(kill_queue, i)
			{
				total_freight -= kill_queue[i].menge;
				const bool ok = fracht.remove(kill_queue[i]);
				assert(ok);
			}
		}
	}

	return sum_menge;
}


/**
 * Load freight from halt
 * @return loading successful?
 * @author Hj. Malthaner
 */
bool vehikel_t::load_freight(halthandle_t halt, bool overcrowd)
{
	const bool ok = halt->gibt_ab(besch->get_ware());
	schedule_t *fpl = cnv->get_schedule();
	if( ok ) 
	{
		uint16 total_capacity = besch->get_zuladung() + (overcrowd ? besch->get_overcrowded_capacity() : 0);
		DBG_DEBUG4("vehikel_t::load_freight", "total_freight %d < total_capacity %d", total_freight, total_capacity);
		while(total_freight < total_capacity) //"Payload" (Google)
		{
			// Modified to allow overcrowding.
			// @author: jamespetts
			const uint16 hinein = total_capacity - total_freight; 
			//hinein = inside (Google)

			ware_t ware = halt->hole_ab(besch->get_ware(), hinein, fpl, cnv->get_besitzer(), cnv, overcrowd);
					
			if(ware.menge == 0) 
			{
				// now empty, but usually, we can get it here ...
				return ok;
			}

			uint16 count = 0;

			for (slist_tpl<ware_t>::iterator iter_z = fracht.begin(); iter_z != fracht.end();) 
			{
				if(count++ > fracht.get_count())
				{
					break;
				}
				ware_t &tmp = *iter_z;
				
				// New system: only merges if origins are alike.
				// @author: jamespetts

				if(ware.can_merge_with(tmp))
				{
					tmp.menge += ware.menge;
					total_freight += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not join it to existing => load it
			if(ware.menge != 0) 
			{
				fracht.insert(ware);
				total_freight += ware.menge;
			}

			INT_CHECK("simvehikel 876");
		}
		DBG_DEBUG4("vehikel_t::load_freight", "total_freight %d of %d loaded.", total_freight, total_capacity);
	}
	return true;
}


/**
 * Remove freight that no longer can reach its destination
 * i.e. becuase of a changed schedule
 * @author Hj. Malthaner
 */
void vehikel_t::remove_stale_freight()
{
	DBG_DEBUG("vehikel_t::remove_stale_freight()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	//slist_tpl<ware_t> kill_queue;
	vector_tpl<ware_t> kill_queue;
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

			if (!found) 
			{
				kill_queue.append(tmp);
			}
			else {
				// since we need to point at factory (0,0), we recheck this too
				koord k = tmp.get_zielpos();
				fabrik_t *fab = fabrik_t::get_fab( welt, k );
				tmp.set_zielpos( fab ? fab->get_pos().get_2d() : k );

				total_freight += tmp.menge;
			}
		}

		FOR(vector_tpl<ware_t>, const& c, kill_queue) {
			fracht.remove(c);
		}
	}
}


void
vehikel_t::play_sound() const
{
	if(  besch->get_sound() >= 0  &&  !welt->is_fast_forward() && sound_ticks < welt->get_zeit_ms() ) 
	{
		if(welt->play_sound_area_clipped(get_pos().get_2d(), besch->get_sound()))
		{
			// Only reset the counter if the sound can be heard.
			const sint64 sound_offset = sim_async_rand(30000) + 5000;
			sound_ticks = welt->get_zeit_ms() + sound_offset;
		}
	}
}


/**
 * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
 * der Convoi eine neue Route ermittelt
 *
 * Fahrzeiug prepares for new journey before - will be called 
 * if the Convoi a new route determined
 * @author Hj. Malthaner
 */
void vehikel_t::neue_fahrt(uint16 start_route_index, bool recalc)
{
	route_index = start_route_index + 1;
	check_for_finish = false;
	use_calc_height = true;

	if(welt->ist_in_kartengrenzen(get_pos().get_2d())) { //"Is in map border" (Babelfish)
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

		alte_fahrtrichtung = fahrtrichtung; //1
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		hoff = 0;
		steps = 0;

		set_xoff( (dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
		set_yoff( (dy<0) ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );

		calc_bild();
		
		if(alte_fahrtrichtung != fahrtrichtung)
		{
			pre_corner_direction.clear();
		}
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
	speed_limit = speed_unlimited();

	route_index = 1;

	rauchen = true;
	fahrtrichtung = ribi_t::keine;
	pos_prev = koord3d::invalid;

	current_friction = 4;
	total_freight = 0;
	sum_weight = besch->get_gewicht() * 1000UL;

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;
	has_driven = false;

	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
	target_halt = halthandle_t();

	//@author: jamespetts 
#ifdef debug_corners	 
	current_corner = 0;
#endif
	direction_steps = 4;
	is_overweight = false;
	reversed = false;
	current_revenue = 0;
	hop_count = 0;
	base_costs = 0;
	diagonal_costs = 0;
    hill_up = 0;
    hill_down = 0;
	current_livery = "default";
}

sint64 vehikel_t::sound_ticks = 0;

vehikel_t::vehikel_t(karte_t *welt) :
	vehikel_basis_t(welt)
{
	rauchen = true;

	besch = NULL;
	cnv = NULL;

	route_index = 1;
	current_friction = 4;
	sum_weight = 10000UL;
	total_freight = 0;

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;

	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;

	//@author: jamespetts 
#ifdef debug_corners 
	current_corner = 0;
#endif
	direction_steps = 4;
	is_overweight = false;
	reversed = false;
	current_revenue = 0;
	hop_count = 0;
	base_costs = 0;
	diagonal_costs = 0;
    hill_up = 0;
    hill_down = 0;
	current_livery = "default";
}


bool vehikel_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	return route->calc_route(welt, start, ziel, this, max_speed, cnv != NULL ? cnv->get_heaviest_vehicle() : get_sum_weight(), 0);
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



void
vehikel_t::verlasse_feld()
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
		reliefkarte_t::get_karte()->calc_map_pixel( get_pos().get_2d() );  //"Set relief colour" (Babelfish)
	}
}


void vehikel_t::hop()
{	
	verlasse_feld(); //"Verlasse" = "leave" (Babelfish)

	pos_prev = get_pos();
	set_pos( pos_next );  // naechstes Feld ("next field" - Babelfish)
	if(route_index<cnv->get_route()->get_count()-1) {
		route_index ++;
		pos_next = cnv->get_route()->position_bei(route_index);
	}
	else {
		route_index ++;
		check_for_finish = true;
	}
	alte_fahrtrichtung = fahrtrichtung;

	// this is a required hack for aircraft: aircraft can turn on a single square, and this confuses the previous calculation.
	// author: hsiegeln
	if(!check_for_finish  &&  pos_prev.get_2d()==pos_next.get_2d()) 
	{
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		steps_next = 0;
	}
	else 
	{
		if(pos_next!=get_pos()) 
		{
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::ist_gerade(welt->lookup(pos_next)->get_weg_ribi_unmasked(get_waytype()))  )  ||  welt->lookup(pos_next)->is_halt()) 
		{
			// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
	}
	calc_bild(); //Calculate image

	betrete_feld(); //"Enter field" (Google)

	const grund_t *gr_prev = welt->lookup(pos_prev);
	const weg_t * weg_prev = gr_prev != NULL ? gr_prev->get_weg(get_waytype()) : NULL;

	const grund_t *gr = welt->lookup(get_pos());
	const weg_t *weg = gr != NULL ? gr->get_weg(get_waytype()) : NULL;
	if(weg)
	{
		speed_limit = calc_speed_limit(weg, weg_prev, &pre_corner_direction, fahrtrichtung, alte_fahrtrichtung);

		// Weight limit needed for GUI flag
		const uint32 weight_limit = (weg->get_max_weight()) > 0 ? weg->get_max_weight() : 1;
		// Necessary to prevent division by zero exceptions if
		// weight limit is set to 0 in the file.

		// This is just used for the GUI display, so only set to true if the weight limit is set to enforce by speed restriction.
		is_overweight = (cnv->get_heaviest_vehicle() > weight_limit && welt->get_settings().get_enforce_weight_limits() == 1); 

		if(weg->is_crossing() && gr->find<crossing_t>(2)) 
		{
			gr->find<crossing_t>(2)->add_to_crossing(this);
		}
	}
	else
	{
		speed_limit = speed_unlimited();
	}

	if(  check_for_finish  &  ist_erstes && (fahrtrichtung==ribi_t::nord  || fahrtrichtung==ribi_t::west))
	{
		steps_next = (steps_next/2)+1;
	}

	// speedlimit may have changed
	cnv->must_recalc_data();
	
	calc_drag_coefficient(gr);

	hop_count ++;
}

/* Calculates the modified speed limit of the current way,
 * taking into account the curve and weight limit.
 * @author: jamespetts/Bernd Gabriel
 */
sint32 vehikel_t::calc_speed_limit(const weg_t *w, const weg_t *weg_previous, fixed_list_tpl<sint16, 16>* cornering_data, ribi_t::ribi current_direction, ribi_t::ribi previous_direction)
{	
	if (weg_previous)
	{
		cornering_data->add_to_tail(get_direction_degrees(ribi_t::get_dir(previous_direction)));
	}
	if(w == NULL)
	{
		return speed_limit;
	}
	const bool is_corner = current_direction != previous_direction;

	const bool is_tilting = besch->get_tilting();
	const sint32 base_limit = kmh_to_speed(w->get_max_speed());
	const uint16 weight_limit = w->get_max_weight();
	sint32 overweight_speed_limit = base_limit;
	sint32 corner_speed_limit = base_limit;
	sint32 new_limit = base_limit;
	const uint32 heaviest_vehicle = cnv->get_heaviest_vehicle();

	//Reduce speed for overweight vehicles

	if(heaviest_vehicle > weight_limit && welt->get_settings().get_enforce_weight_limits() == 1)
	{
		if((heaviest_vehicle * 100) / weight_limit <= 110)
		{
			//Overweight by up to 10% - reduce speed limit to a third.
			overweight_speed_limit = base_limit / 3;
		}
		else if((heaviest_vehicle * 100) / weight_limit > 110)
		{
			//Overweight by more than 10% - reduce speed limit by a factor of 10.
			overweight_speed_limit = base_limit / 10;
		}
	}

	waytype_t waytype = cnv->get_schedule()->get_waytype();

	// Cornering settings. Vehicles must slow to take corners.
	const sint32 max_corner_limit = welt->get_settings().get_max_corner_limit(waytype);
	const sint32 min_corner_limit = welt->get_settings().get_min_corner_limit(waytype);
	const uint16 max_corner_adjustment_factor = welt->get_settings().get_max_corner_adjustment_factor(waytype);
	const uint16 min_corner_adjustment_factor = welt->get_settings().get_min_corner_adjustment_factor(waytype);
	const uint8 min_direction_steps = welt->get_settings().get_min_direction_steps(waytype);
	const uint8 max_direction_steps = welt->get_settings().get_max_direction_steps(waytype);
	
#ifndef debug_corners	
	if(is_corner && max_direction_steps > 0)
	{
#endif
 		sint16 direction_difference = 0;
		const sint16 direction = get_direction_degrees(ribi_t::get_dir(current_direction));
		const uint16 modified_route_index = min(route_index - 1, cnv->get_route()->get_count() - 1);
		const koord3d *previous_tile = &cnv->get_route()->position_bei(modified_route_index);
	
		uint16 limit_adjustment_percentage = 100;
		
		if(base_limit > max_corner_limit)
		{
			limit_adjustment_percentage = max_corner_adjustment_factor;
			direction_steps = max_direction_steps;
		}
		else if(base_limit < min_corner_limit)
		{
			limit_adjustment_percentage = min_corner_adjustment_factor;
			direction_steps = min_direction_steps;
		}
		else
		{
			//Smooth the difference
			const uint32 tmp1 = base_limit - min_corner_limit;
			const uint32 tmp2 = max_corner_limit - min_corner_limit;
			const uint32 percentage = (tmp1 * 100) / tmp2;
			limit_adjustment_percentage = min_corner_adjustment_factor - (((min_corner_adjustment_factor - max_corner_adjustment_factor) * percentage) / 100);
			direction_steps = (sint16)(((max_direction_steps - min_direction_steps) * percentage) / 100) + min_direction_steps; 
		}
		
		if(direction_steps == 0)
		{
			// If we are not counting corners, do not attempt to calculate their speed limit.
			return overweight_speed_limit;
		}

		uint16 tmp;

		int counter = 0;
		int steps_to_90 = 0;
		int steps_to_135 = 0;
		int steps_to_180 = 0;
		int smoothing_percentage = 0;
		const uint16 meters_per_tile = welt->get_settings().get_meters_per_tile();
		int meters = 0;
		for(int i = cornering_data->get_count() - 1; i >= 0 && counter <= direction_steps; i --)
		{
			counter ++;
			meters = meters_per_tile * counter;
			tmp = vehikel_t::compare_directions(direction, cornering_data->get_element(i));
			if(tmp > direction_difference)
			{
				direction_difference = tmp;
				switch(direction_difference)
				{
					case 90:
						steps_to_90 = counter;
						if(meters > 500)
						{
							smoothing_percentage = 100;
						}
						break;
					case 135:
						steps_to_135 = counter;
						if(meters > 750)
						{
							smoothing_percentage = 100;
						}
						break;
					case 180:
						steps_to_180 = counter;
						if(meters > 1000)
						{
							smoothing_percentage = 100;
						}
						break;
					default:
						break;
				}

			}
		}

#ifdef debug_corners
		current_corner = direction_difference;
#endif
		const sint16 previous_direction_degrees = get_direction_degrees(ribi_t::get_dir(previous_direction));
		if(direction_difference == 0 && current_direction != previous_direction)
		{
			//Fallback code in case the checking the histories did not work properly (for example, if the histories were cleared recently) 
			direction_difference = compare_directions(direction, previous_direction_degrees);
		}

		// Maximum speeds for sharper corners no matter what the base limit of the way.	
		sint32 max_speed_135;
		sint32 max_speed_180;
		sint32 max_speed_90;
		
		sint32 limit_adjustment_percentage_90 = limit_adjustment_percentage / 2;
		sint32 limit_adjustment_percentage_135 = limit_adjustment_percentage / 3;
		sint32 limit_adjustment_percentage_180 = limit_adjustment_percentage / 4;

		switch(waytype)
		{
			case track_wt:
			case narrowgauge_wt:
			case monorail_wt:
			case maglev_wt:
				max_speed_90 = kmh_to_speed(30);
				max_speed_135 = kmh_to_speed(20);
				max_speed_180 = kmh_to_speed(4);
				break;
				
			case tram_wt:
				max_speed_90 = kmh_to_speed(42);
				max_speed_135 = kmh_to_speed(35);
				max_speed_180 = kmh_to_speed(20);
				break;

			default:
			case road_wt:
				max_speed_90 = kmh_to_speed(45);
				max_speed_135 = kmh_to_speed(40);
				max_speed_180 = kmh_to_speed(35);
				break;
		}

		//Smoothing code: smoothed corners benefit.	
		if(smoothing_percentage > 0)
		{
			smoothing_percentage = (steps_to_90 * 100) / (direction_steps + 1);
			max_speed_90 += (base_limit - max_speed_90) * smoothing_percentage / 100;
			limit_adjustment_percentage_90 += (limit_adjustment_percentage - limit_adjustment_percentage_90) * smoothing_percentage / 100;
			if(direction_difference > 90)
			{
				const int smoothing_percentage_135 = (steps_to_135 * 100) / (direction_steps + 1);
				max_speed_135 += (base_limit - max_speed_135) * smoothing_percentage_135 / 100;
				limit_adjustment_percentage_135 += (limit_adjustment_percentage - limit_adjustment_percentage_135) * smoothing_percentage / 100;
				smoothing_percentage = min(smoothing_percentage, smoothing_percentage_135);
				if(direction_difference >= 180)
				{
					const int smoothing_percentage_180 = (steps_to_180 * 100) / (direction_steps + 1);
					max_speed_180 += (base_limit - max_speed_180) * smoothing_percentage_180 / 100;
					limit_adjustment_percentage_180 += (limit_adjustment_percentage - limit_adjustment_percentage_180) * smoothing_percentage / 100;
					smoothing_percentage = min(smoothing_percentage, smoothing_percentage_180);
				}
			}
		}

		// Find the maximum constraint on the corner
		sint32 hard_limit;
		switch(direction_difference)
		{
		case 0:
		case 45:
		default:
			hard_limit = base_limit;
			break; 
		case 90:
			limit_adjustment_percentage = limit_adjustment_percentage_90;
			hard_limit = max_speed_90;
			break;
		case 135:
			limit_adjustment_percentage = max(limit_adjustment_percentage_90, limit_adjustment_percentage_135);
			hard_limit = min(max_speed_90, max_speed_135);
			break;
		case 180:
		case 270:
			const sint32 tmp_percentage = max(limit_adjustment_percentage_90, limit_adjustment_percentage_135);
			limit_adjustment_percentage = max(tmp_percentage, limit_adjustment_percentage_180);
			const sint32 tmp_hard = min(max_speed_90, max_speed_135);
			hard_limit = min(tmp_hard, max_speed_180);
		}

		// Adjust for tilting.
		// Tilting only makes a difference on faster track and on well smoothed corners.
		if(is_tilting && base_limit > kmh_to_speed(120) && (smoothing_percentage > 50 || direction_difference <= 45))
		{	
			// Tilting trains can take corners faster
			limit_adjustment_percentage += 30;
			if(limit_adjustment_percentage > 100)
			{
				// But cannot go faster on a corner than on the straight!
				limit_adjustment_percentage = 100;
			}

			hard_limit = (hard_limit * 130) / 100;
		}
		
		// Now apply the adjusted corner limit
		corner_speed_limit = min((base_limit * limit_adjustment_percentage) / 100, hard_limit);

#ifndef debug_corners
	}
#endif
	
	//Overweight penalty not to be made cumulative to cornering penalty
	if(corner_speed_limit < overweight_speed_limit)
	{
		new_limit = corner_speed_limit;
	}
	else
	{
		new_limit = overweight_speed_limit;
	}
	
	sint8 trim_size = cornering_data->get_count() - direction_steps;
	cornering_data->trim_from_head((trim_size >= 0) ? trim_size : 0);

	//Speed limit must never be 0.
	if(new_limit > 0)
	{
		return new_limit;
	}
	else
	{
		return kmh_to_speed(10);
	}
}

/** gets the waytype specific friction on straight flat way.
 * extracted from vehikel_t::calc_drag_coefficient()
 * @author Bernd Gabriel, Nov, 05 2009
 */
sint16 get_friction_of_waytype(waytype_t waytype)
{
	switch(waytype)
	{
		case road_wt:	
			return 4;
		default:
			return 1;
	}
}


/* calculates the current friction coefficient based on the curent track
 * flat, slope, (curve)...
 * @author prissi, HJ, Dwachs
 */
void vehikel_t::calc_drag_coefficient(const grund_t *gr) //,const int h_alt, const int h_neu)
{
	if(gr == NULL)
	{
		return;
	}

	const waytype_t waytype = get_waytype();
	const sint16 base_friction = get_friction_of_waytype(waytype);
	//current_friction = base_friction;
	
	// Old method - not realistic. Now uses modified speed limit. Preserved optionally.
	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) //"Old direction != direction"	
	{
		//The level (if any) of additional friction to apply around corners.
		const uint8 curve_friction_factor =welt->get_settings().get_curve_friction_factor(waytype);
		current_friction += curve_friction_factor;
	}

	// or a hill?
	// Cumulative drag for hills: @author: jamespetts
	const hang_t::typ hang = gr->get_weg_hang();
	if(hang!=hang_t::flach) 
	{
		// Bernd Gabriel, Nov, 30 2009: at least 1 partial direction must match for uphill (op '&'), but not the 
		// complete direction. The hill might begin in a curve and then '==' accidently accelerates the vehicle.
		if(ribi_typ(hang) & fahrtrichtung)
		{
			//Uphill
			//current_friction += 45;
			current_friction = min(base_friction + 75, current_friction + 42);
		}
		else
		{
			//Downhill
			//current_friction -= 45;
			current_friction = max(base_friction - 40, current_friction - 22);
		}
	}
	else
	{
		current_friction = max(base_friction, current_friction - 13);
	}
}

vehikel_t::direction_degrees vehikel_t::get_direction_degrees(ribi_t::dir direction) const
{
	switch(direction)
	{
	case ribi_t::dir_nord :
		return vehikel_t::North;
	case ribi_t::dir_nordost :
		return vehikel_t::Northeast;
	case ribi_t::dir_ost :
		return vehikel_t::East;
	case ribi_t::dir_suedost :
		return vehikel_t::Southeast;
	case ribi_t::dir_sued :
		return vehikel_t::South;
	case ribi_t::dir_suedwest :
		return vehikel_t::Southwest;
	case ribi_t::dir_west :
		return vehikel_t::West;
	case ribi_t::dir_nordwest :
		return vehikel_t::Northwest;
	default:
		return vehikel_t::North;
	};
	return vehikel_t::North;
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
					welt->sync_add( abgas );
				}
			}
		}
	}
}


const char *vehikel_t::get_fracht_mass() const
{
	return get_fracht_typ()->get_mass();
}


/**
 * Berechnet Gesamtgewicht der transportierten Fracht in KG
 * "Calculated total weight of cargo transported in KG"  (Translated by Google)
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
	fracht.clear();
}


uint16 vehikel_t::beladen(halthandle_t halt, bool overcrowd)
{
	bool ok = true;
	uint16 load_charge = total_freight;
	if(halt.is_bound()) 
	{
		ok = load_freight(halt, overcrowd);
	}
	sum_weight = get_fracht_gewicht() + besch->get_gewicht() * 1000UL;
	calc_bild();
	if(ok)
	{
		return total_freight - load_charge;
	}
	else
	{
		return 0;
	}
}


/**
 * fahrzeug an haltestelle entladen
 * "Vehicle to stop discharged" (translated by Google)
 * @author Hj. Malthaner
 */
uint16 vehikel_t::entladen(halthandle_t halt)
{
	uint16 menge = unload_freight(halt);
	if(menge > 0) 
	{
		// add delivered goods to statistics
		cnv->book(menge, convoi_t::CONVOI_TRANSPORTED_GOODS);
		// add delivered goods to halt's statistics
		halt->book(menge, HALT_ARRIVED);
	}
	return menge;
}


/**
 * Ermittelt fahrtrichtung
 * "Determined Direction" (translated by Google)
 * @author Hj. Malthaner
 */
ribi_t::ribi vehikel_t::richtung() const
{
	ribi_t::ribi neu = calc_richtung(pos_prev.get_2d(), pos_next.get_2d());
	// nothing => use old direct further on
	return (neu == ribi_t::keine) ? fahrtrichtung : neu;
}


void
vehikel_t::calc_bild() //"Bild" = "picture" (Google)
{
	image_id old_bild=get_bild();
	if (fracht.empty()) 
	{
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_direction_of_travel()), NULL, current_livery.c_str())); 
	}
	else 
	{
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_direction_of_travel()), fracht.front().get_besch(), current_livery.c_str()));
	}
	if(old_bild!=get_bild()) {
		set_flag(ding_t::dirty);
	}
}

ribi_t::ribi
vehikel_t::get_direction_of_travel()
{
	ribi_t::ribi dir = get_fahrtrichtung();
	if(reversed)
	{
		switch(dir)
		{
		case ribi_t::nord:
			dir = ribi_t::sued;
			break;

		case ribi_t::ost:
			dir = ribi_t::west;
			break;

		case ribi_t::nordost:
			dir = ribi_t::suedwest;
			break;

		case ribi_t::sued:
			dir = ribi_t::nord;
			break;

		case ribi_t::suedost:
			dir = ribi_t::nordwest;
			break;

		case ribi_t::west:
			dir = ribi_t::ost;
			break;

		case ribi_t::nordwest:
			dir = ribi_t::suedost;
			break;

		case ribi_t::suedwest:
			dir = ribi_t::nordost;
			break;
		};
	}
	return dir;
}

void vehikel_t::set_reversed(bool value)
{
	if(besch->is_bidirectional() || (cnv != NULL && cnv->get_reversable()))
	{
		reversed = value;
	}
}

uint16 vehikel_t::get_overcrowding() const
{
	return total_freight - besch->get_zuladung() > 0 ? total_freight - besch->get_zuladung() : 0;
}

uint8 vehikel_t::get_comfort() const
{
	const uint8 base_comfort = besch->get_comfort();
	if(base_comfort == 0)
	{
		return 0;
	}
	else if(total_freight <= get_fracht_max())
	{
		// Not overcrowded - return base level
		return base_comfort;
	}

	// Else
	// Overcrowded - adjust comfort. Standing passengers
	// are very uncomfortable (no more than 10).
	const uint8 standing_comfort = (base_comfort < 20) ? (base_comfort / 2) : 10;
	uint16 passenger_count = 0;
	FOR(slist_tpl<ware_t>, const& ware, fracht) 
	{
		if(ware.is_passenger())
		{
			passenger_count += ware.menge;
		}
	}
	assert(passenger_count <= total_freight);
	const uint16 total_seated_passengers = passenger_count < get_fracht_max() ? passenger_count : get_fracht_max();
	const uint16 total_standing_passengers = passenger_count > total_seated_passengers ? passenger_count - total_seated_passengers : 0;
	// Avoid division if we can
	if(total_standing_passengers == 0)
	{
		return base_comfort;
	}
	// Else
	// Average comfort of seated and standing
	return ((total_seated_passengers * base_comfort) + (total_standing_passengers * standing_comfort)) / passenger_count;
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
		// "Parameters are declared in the order saved" (Translated by Google)
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
			if(  (besch==NULL  ||  ware.menge>0)  &&  welt->ist_in_kartengrenzen(ware.get_zielpos()) ) {	// also add, of the besch is unknown to find matching replacement
				fracht.insert(ware);
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
			sum_weight = get_fracht_gewicht() + besch->get_gewicht() * 1000UL;
		}
		// recalc total freight
		total_freight = 0;
		FOR(slist_tpl<ware_t>, const& c, fracht) {
			total_freight += c.menge;
		}
	}

	if(file->get_experimental_version() >= 1)
	{
		file->rdwr_bool(reversed);
	}
	else
	{
		reversed = false;
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

	if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_short(direction_steps);
		file->rdwr_longlong(current_revenue);
		
		if(file->is_saving())
		{
			uint8 count = pre_corner_direction.get_count();
			file->rdwr_byte(count);
			sint16 dir;
			ITERATE(pre_corner_direction,n)
			{
				dir = pre_corner_direction[n];
				file->rdwr_short(dir);
			}
		}
		else
		{
			pre_corner_direction.clear();
			uint8 count;
			file->rdwr_byte(count);
			for(uint8 n = 0; n < count; n++)
			{
				sint16 dir;
				file->rdwr_short(dir);
				pre_corner_direction.add_to_tail(dir);
			}
		}
	}

	if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_string(current_livery);
	}
	else if(file->is_loading())
	{
		current_livery = "default";
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




void
vehikel_t::zeige_info()
{
	if(  cnv != NULL  ) {
		cnv->zeige_info();
	} else {
		dbg->warning("vehikel_t::zeige_info()","cnv is null, can't open convoi window!");
	}
}

#if 0
void vehikel_t::info(cbuffer_t & buf) const
{
	if(cnv) {
		cnv->info(buf);
	}
}
#endif

const char *vehikel_t::ist_entfernbar(const spieler_t *)
{
	return "Vehicles cannot be removed";
}

bool vehikel_t::check_way_constraints(const weg_t &way) const
{
	return missing_way_constraints_t(besch->get_way_constraints(), way.get_way_constraints()).ist_befahrbar();
}

bool vehikel_t::check_access(const weg_t* way) const
{
	if(get_besitzer() && get_besitzer()->get_player_nr() == 1)
	{
		// The public player can always connect to ways. It has no vehicles.
		return true;
	}
	const grund_t* const gr = welt->lookup(get_pos());
	const weg_t* const current_way = gr ? gr->get_weg(get_waytype()) : NULL;
	if(current_way == NULL)
	{
		return true;
	}
	return way && (way->get_besitzer() == NULL || way->get_besitzer() == get_besitzer() || get_besitzer() == NULL || way->get_besitzer() == current_way->get_besitzer() || way->get_besitzer()->allows_access_to(get_besitzer()->get_player_nr()) || (welt->get_city(way->get_pos().get_2d()) && way->get_waytype() == road_wt));
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
				if(  state>=1  ) 
				{
					char waiting_time[64];
					cnv->snprintf_remaining_loading_time(waiting_time, sizeof(waiting_time));
					if (cnv->get_loading_limit()) 
					{
						if(!cnv->is_wait_infinite() && strcmp(waiting_time, "0:00")) 
						{
							sprintf( tooltip_text, translator::translate("Loading (%i->%i%%), %s left!"), cnv->get_loading_level(), cnv->get_loading_limit(), waiting_time);
						}
						else 
						{
							sprintf( tooltip_text, translator::translate("Loading (%i->%i%%)!"), cnv->get_loading_level(), cnv->get_loading_limit());
						}
					} 
					else 
					{
						sprintf( tooltip_text, translator::translate("Loading. %s left!"), waiting_time);
					}
					color = COL_YELLOW;
				}
				break;

			case convoi_t::FAHRPLANEINGABE:
//			case convoi_t::ROUTING_1:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Schedule changing!"), lengthof(tooltip_text) );
					color = COL_LIGHT_YELLOW;
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

				case convoi_t::REVERSING:
				if(  state>=2  ) 
				{
					char reversing_time[64];
					cnv->snprintf_remaining_reversing_time(reversing_time, sizeof(reversing_time));
					sprintf( tooltip_text, translator::translate("Reversing. %s left"), reversing_time);
					color = COL_YELLOW;
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
		if(is_overweight)
		{
			sprintf(tooltip_text, translator::translate("Too heavy"), cnv->get_name());
			color = COL_ORANGE;
		}

		const aircraft_t* air = (const aircraft_t*)this;
		if(get_waytype() == air_wt && air->runway_too_short)
		{
			sprintf(tooltip_text, translator::translate("Runway too short"), cnv->get_name());
			color = COL_ORANGE;
		}
#ifdef debug_corners
			sprintf(tooltip_text, translator::translate("CORNER: %i"), current_corner);
			color = COL_GREEN;
#endif
		

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

// BG, 06.06.2009: added
void vehikel_t::laden_abschliessen()
{
	spieler_t *sp = get_besitzer();
	if (sp) {
		// BG, 06.06.2009: fixed maintenance for loaded vehicles, which are located on the map
		sp->add_maintenance((sint32)get_besch()->get_fixed_cost(welt), spieler_t::MAINT_VEHICLE);
	}
}

// BG, 06.06.2009: added
void vehikel_t::before_delete()
{
	spieler_t *sp = get_besitzer();
	if (sp) {
		// BG, 06.06.2009: withdraw fixed maintenance for deleted vehicles
		sp->add_maintenance(-(sint32)get_besch()->get_fixed_cost(welt), spieler_t::MAINT_VEHICLE);
	}
}

/*--------------------------- Fahrdings ------------------------------*/
//(Translated by Babelfish as "driving thing")


automobil_t::automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
	is_checker = false;
	drives_on_left = welt->get_settings().is_drive_left();
}

automobil_t::automobil_t(karte_t *welt) : vehikel_t(welt)
{
	// This is blank - just used for the automatic road finder.
	cnv = NULL;
	is_checker = true;
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
	is_checker = false;
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
	const uint32 routing_weight = cnv != NULL ? cnv->get_heaviest_vehicle() : get_sum_weight();
	return route->calc_route(welt, start, ziel, this, max_speed, routing_weight, cnv->get_tile_length());
}



bool automobil_t::ist_befahrbar(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->get_weg(road_wt);
	if(str==NULL  ||  str->get_max_speed()==0) {
		return false;
	}
	if(!is_checker)
	{
		bool electric = cnv!=NULL  ?  cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric;
		if(electric  &&  !str->is_electrified()) {
			return false;
		}
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			if(get_besch() && rs->get_besch()->get_min_speed()>0  &&  rs->get_besch()->get_min_speed()>kmh_to_speed(get_besch()->get_geschw())  ) 
			{
				return false;
			}
			if(get_besch() &&  rs->get_besch()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) 
			{
				// prvate road
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound()  &&  cnv->is_waiting()  &&  (rs->get_besch()->get_flags() & roadsign_besch_t::END_OF_CHOOSE_AREA)) {
				return false;
			}
		}
	}
	if(!is_checker)
	{
		return check_access(str) && check_way_constraints(*str);
	}
	return check_access(str);
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
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	// assume all traffic (and even road signs etc.) is not good ...
	costs += gr->get_top();

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		if(  hang_t::is_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y )  ) {
			costs += 15;
		}
	}

	//@author: jamespetts
	// Strongly prefer routes for which the vehicle is not overweight.
	uint16 weight_limit = w->get_max_weight();
	if(vehikel_t::get_sum_weight() > weight_limit &&welt->get_settings().get_enforce_weight_limits() == 1)
	{
		costs += 40;
	}

	if(w->is_diagonal())
	{
		// Diagonals are a *shorter* distance.
		costs = (costs * 5) / 7; // was: costs /= 1.4
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

					const route_t *rt = cnv->get_route();
					// is our target occupied?
					target_halt = haltestelle_t::get_halt( welt, rt->back(), get_besitzer() );
					if(  target_halt.is_bound()  ) {
						// since convois can long than one tile, check is more difficult
						bool can_go_there = true;
						for(  uint32 length=0;  can_go_there  &&  length<cnv->get_tile_length()  &&  length+1<rt->get_count();  length++  ) {
							can_go_there &= target_halt->is_reservable( welt->lookup( rt->position_bei( rt->get_count()-length-1) ), cnv->self );
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
							if(!target_halt->find_free_position(road_wt,cnv->self,ding_t::automobil)) {
								restart_speed = 0;
								target_halt = halthandle_t();
								//DBG_MESSAGE("automobil_t::ist_weg_frei()","cnv=%d nothing free found!",cnv->self.get_id());
								return false;
							}

							// now it make sense to search a route
							route_t target_rt;
							if(  !target_rt.find_route( welt, pos_next, this, speed_to_kmh(cnv->get_min_top_speed()), richtung, cnv->get_heaviest_vehicle(), 33 )  ) {
								// nothing empty or not route with less than 33 tiles
								target_halt = halthandle_t();
								restart_speed = 0;
								return false;
							}

							// now reserve our choice (beware: might be longer than one tile!)
							for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<target_rt.get_count();  length++  ) {
								target_halt->reserve_position( welt->lookup( target_rt.position_bei( target_rt.get_count()-length-1) ), cnv->self );
							}
							cnv->update_route(route_index, target_rt);
						}
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

							const route_t *rt = cnv->get_route();
							// is our target occupied?
							target_halt = haltestelle_t::get_halt( welt, rt->back(), get_besitzer() );
							if(  target_halt.is_bound()  ) {
								// since convois can long than one tile, check is more difficult
								bool can_go_there = true;
								for(  uint32 length=0;  can_go_there  &&  length<cnv->get_tile_length()  &&  length+1<rt->get_count();  length++  ) {
									can_go_there &= target_halt->is_reservable( welt->lookup( rt->position_bei( rt->get_count()-length-1) ), cnv->self );
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
									if(!target_halt->find_free_position(road_wt,cnv->self,ding_t::automobil)) {
										restart_speed = 0;
										target_halt = halthandle_t();
										//DBG_MESSAGE("automobil_t::ist_weg_frei()","cnv=%d nothing free found!",cnv->self.get_id());
										return false;
									}

									// now it make sense to search a route
									route_t target_rt;
									koord3d next3d = r.position_bei(test_index);
									if(  !target_rt.find_route( welt, next3d, this, speed_to_kmh(cnv->get_min_top_speed()), curr_90fahrtrichtung, cnv->get_heaviest_vehicle(), 33 )  ) {
										// nothing empty or not route with less than 33 tiles
										target_halt = halthandle_t();
										restart_speed = 0;
										return false;
									}

									// now reserve our choice (beware: might be longer than one tile!)
									for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<target_rt.get_count();  length++  ) {
										target_halt->reserve_position( welt->lookup( target_rt.position_bei( target_rt.get_count()-length-1) ), cnv->self );
									}
									cnv->update_route(test_index, target_rt);
								}
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
					convoi_t* const ocnv = car->get_convoi();
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
					if(over  &&  !over->is_overtaken()) {
						if(  over->is_overtaking()  ) {
							// otherwise we would stop every time being overtaken
							return true;
						}
						// not overtaking/being overtake: we need to make a more thourough test!
						if(  automobil_t const* const car = ding_cast<automobil_t>(dt)  ) {
							convoi_t* const ocnv = car->get_convoi();
							if(  cnv->can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 :  ocnv->get_akt_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
								return true;
							}
						} else if (stadtauto_t* const caut = ding_cast<stadtauto_t>(dt)) {
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
	if(str == NULL)
	{
		return;
	}
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
		// try to find a matching vehicle
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
		route_t & r = *cnv->get_route();
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

				route_t& r = *cnv->get_route();
				if(  !r.empty()  &&  route_index + 1U < r.get_count() - 1  ) {

					uint16 dummy;
					block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, 100000, false, false);
					target_halt = halthandle_t();
				}
			}
			else {
				assert(c!=NULL);
				// eventually search new route
				route_t& r = *c->get_route();
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
	if (ist_erstes && route_index < cnv->get_route()->get_count())
	{
		// free all reserved blocks
		uint16 dummy;
		block_reserver(cnv->get_route(), cnv->back()->get_route_index(), dummy,
				dummy, target_halt.is_bound() ? 100000 : 1, false, true);
	}
	target_halt = halthandle_t(); // no block reserved
	const uint16 tile_length = cnv->get_schedule()->get_current_eintrag().reverse ? 8888 : cnv->get_tile_length();
	return route->calc_route(welt, start, ziel, this, max_speed, cnv != NULL ? cnv->get_heaviest_vehicle() : get_sum_weight(), tile_length);
}



bool waggon_t::ist_befahrbar(const grund_t *bd) const
{
	if(!bd) return false;
	schiene_t const* const sch = ding_cast<schiene_t>(bd->get_weg(get_waytype()));
	if(  !sch  ) {
		return false;
	}

	// Hajo: diesel and steam engines can use electrifed track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : besch->get_engine_type() == vehikel_besch_t::electric);
	
	if((!needs_no_electric  &&  !sch->is_electrified())  ||  sch->get_max_speed() == 0 || !check_way_constraints(*sch)) 
	{
		return false;
	}

	// now check for special signs
	if(sch->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs && rs->get_besch()->get_wtyp()==get_waytype()  ) {
			if(rs->get_besch()->get_min_speed() > 0  &&  rs->get_besch()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(rs && rs->get_besch()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// prvate road
				return false;
			}
		}
	}

	if(  target_halt.is_bound()  &&  cnv->is_waiting()  ) 
	{
		// we are searching a stop here:
		// ok, we can go where we already are ...
		if(bd->get_pos() == get_pos()) 
		{
			return check_access(sch);
		}
		// we cannot pass an end of choose area
		if(sch->has_sign()) 
		{
			const roadsign_t* rs = bd->find<roadsign_t>();
			if(  rs->get_besch()->get_wtyp()==get_waytype()  ) 
			{
				if(  rs->get_besch()->get_flags() & roadsign_besch_t::END_OF_CHOOSE_AREA  ) 
				{
					return false;
				}
			}
		}
		// but we can only use empty blocks ...
		// now check, if we could enter here
		return check_access(sch) && sch->can_reserve(cnv->self);
	}

	return check_access(sch);
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

	sint32 max_tile_speed = w->get_max_speed();
	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed <= max_tile_speed) ? 1 : (max_speed*4)/(max_tile_speed*4);

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		if(  hang_t::is_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y )  ) {
			costs += 25;
		}
	}

	//@author: jamespetts
	// Strongly prefer routes for which the vehicle is not overweight.
	uint16 weight_limit = w->get_max_weight();
	if(vehikel_t::get_sum_weight() > weight_limit &&welt->get_settings().get_enforce_weight_limits() == 1)
	{
		costs += 40;
	}

	if(w->is_diagonal())
	{
		// Diagonals are a *shorter* distance.
		costs = (costs * 5) / 7; // was: costs /= 1.4
	}

	return costs;
}


//signal_t *waggon_t::ist_blockwechsel(koord3d k2) const
//{
//	const grund_t* gr = welt->lookup(k2);
//	if(gr == NULL)
//	{
//		// Possible fix for rotation bug
//		// @author: jamespetts
//		return NULL;
//	}
//	
//	const schiene_t * sch1 = (const schiene_t *) gr->get_weg(get_waytype());
//	if(sch1  &&  sch1->has_signal()) 
//	{
//		// a signal for us
//		
//		return gr->find<signal_t>();
//	}
//	return NULL;
//}



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
		sig->set_zustand( roadsign_t::gruen ); // "gruen" = "green" (Google); "zustand" = "condition" (Google)
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
		success = target_rt.calc_route( welt, cur_pos, cnv->get_schedule()->eintrag[fahrplan_index].pos, this, get_convoi()->get_heaviest_vehicle(), speed_to_kmh(cnv->get_min_top_speed()), 8888 /*cnv->get_tile_length()*/ );
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
		if(  !target_rt.find_route( welt, cnv->get_route()->position_bei(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, cnv->get_heaviest_vehicle(), MAX_CHOOSE_BLOCK_TILES )  ) {
#else
		if(  !target_rt.find_route( welt, cnv->get_route()->position_bei(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, cnv->get_heaviest_vehicle(), welt->get_groesse_x()+welt->get_groesse_y() )  ) {
#endif
			// nothing empty or not route with less than MAX_CHOOSE_BLOCK_TILES tiles
			target_halt = halthandle_t();
			sig->set_zustand(  roadsign_t::rot );
			restart_speed = 0;
			return false;
		}
		else {
			// try to alloc the whole route
			cnv->update_route(start_block, target_rt);
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
		sig->set_zustand(  roadsign_t::rot ); // "rot" = "red" (Google)
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
	if(  cnv->get_state()==convoi_t::CAN_START  ||  cnv->get_state()==convoi_t::CAN_START_ONE_MONTH  ||  cnv->get_state()==convoi_t::CAN_START_TWO_MONTHS || cnv->get_state()==convoi_t::REVERSING )
	{
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

	existing_convoy_t convoy(*cnv);
	sint32 brake_steps = convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), cnv->get_akt_speed());
	route_t &route = *cnv->get_route();
	convoi_t::route_infos_t &route_infos = cnv->get_route_infos();

	// is there any signal/crossing to be reserved?
	uint16 next_block = cnv->get_next_stop_index() - 1;
	uint16 last_index = route.get_count() - 1;
	if(next_block > last_index) 
	{
		const sint32 route_steps = route_infos.get_element(last_index).steps_from_start - route_index <= route_infos.get_count() - 1 ? route_infos.get_element(route_index).steps_from_start : 0;
		bool weg_frei = route_steps >= brake_steps || brake_steps == 0 || route_steps == 0; // If brake_steps == 0 and weg_frei == false, weird excess block reservations can occur that cause blockages.
		if (!weg_frei)
		{ 	
			// we need a longer route to decide, whether we will have to start braking:
			route_t target_rt;
			schedule_t *fpl = cnv->get_schedule();
			const koord3d start_pos = route.position_bei(last_index);
			uint8 index = fpl->get_aktuell();
			bool reversed = cnv->get_reverse_schedule();
			fpl->increment_index(&index, &reversed);
			const koord3d next_ziel = fpl->eintrag[index].pos;

			weg_frei = !target_rt.calc_route( welt, start_pos, next_ziel, this, speed_to_kmh(cnv->get_min_top_speed()), cnv->get_heaviest_vehicle(), welt->get_settings().get_max_route_steps());
			if (!weg_frei)
			{
				ribi_t::ribi old_dir = calc_richtung(route.position_bei(last_index - 1).get_2d(), route.position_bei(last_index).get_2d());
				ribi_t::ribi new_dir = calc_richtung(target_rt.position_bei(0).get_2d(), target_rt.position_bei(1).get_2d());
				if (old_dir & ribi_t::rueckwaerts(new_dir))
				{
					// convoy must reverse and thus stop at the waypoint. No need to extend the route now.
					// Extending the route now would confuse tile reservation.
					fpl->eintrag[fpl->get_aktuell()].reverse = true;
				}
				else
				{
					// convoy can pass waypoint without reversing/stopping. Append route to next stop/waypoint
					fpl->advance();
					cnv->update_route(last_index, target_rt);
					weg_frei = block_reserver( &route, last_index, next_signal, next_crossing, 0, true, false );
					last_index = route.get_count() - 1;
					cnv->set_next_stop_index( min( next_crossing, next_signal ) );
					next_block = cnv->get_next_stop_index() - 1;
				}
			}
		}
		// no obstacle in the way => drive on ...
		if (weg_frei || next_block > last_index)
			return true;
	}

	// it happened in the past that a convoi has a next signal stored way after the current position!
	// this only happens in vorfahren, but we can cure this easily
	if(  next_block+1<route_index  ) {
		bool ok = block_reserver( cnv->get_route(), route_index, next_signal, next_crossing, 0, true, false );
		cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		return ok;
	}

	const sint32 route_steps = brake_steps > 0 && route_index <= route_infos.get_count() - 1 ? cnv->get_route_infos().get_element((next_block > 0 ? next_block - 1 : 0)).steps_from_start - cnv->get_route_infos().get_element(route_index).steps_from_start : -1;
	if (route_steps <= brake_steps) 
	{ 	
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
			crossing_t* cr = gr_next_block->find<crossing_t>(2);
			if (cr) {
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
bool waggon_t::block_reserver(route_t *route, uint16 start_index, uint16 &next_signal_index, uint16 &next_crossing_index, int count, bool reserve, bool force_unreserve) const
{
	bool success=true;
#ifdef MAX_CHOOSE_BLOCK_TILES
	int max_tiles=2*MAX_CHOOSE_BLOCK_TILES; // max tiles to check for choosesignals
#endif
	slist_tpl<grund_t *> signs;	// switch all signals on their way too ...

	if(start_index>=route->get_count()) {
		return 0;
	}

	if(route->position_bei(start_index)==get_pos()  &&  reserve) {
		start_index++;
	}

	// perhaps find an early platform to stop at
	uint16 platform_size_needed = 0;
	uint16 platform_size_found = 0;
	ribi_t::ribi ribi_last = ribi_t::keine;
	ribi_t::ribi ribi = ribi_t::keine;
	halthandle_t dest_halt = halthandle_t();
	uint16 early_platform_index = INVALID_INDEX;
	const schedule_t* fpl = NULL;
	if(cnv != NULL)
	{
		fpl = cnv->get_schedule();
	}
	bool do_early_platform_search =	fpl != NULL
		&& (fpl->is_mirrored() || fpl->is_bidirectional())
		&& fpl->get_current_eintrag().ladegrad == 0;

	if(do_early_platform_search) 
	{
		platform_size_needed = cnv->get_tile_length();
		dest_halt = haltestelle_t::get_halt(welt, cnv->get_schedule()->get_current_eintrag().pos, cnv->get_besitzer());
	}

	// find next block segment enroute
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
		if(reserve) 
		{
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
			// check if there is an early platform available to stop at
			if (do_early_platform_search)
			{
				if(early_platform_index == INVALID_INDEX)
				{
					/*const char* TEST_this_halt = gr->get_halt().is_bound() ? gr->get_halt()->get_name() : "NULL";
					const char* TEST_dest_halt = dest_halt->get_name();*/
					if(gr->get_halt().is_bound() && gr->get_halt() == dest_halt) 
					{
						if(ribi == ribi_last)
						{
							platform_size_found++;
						} 
						else
						{
							platform_size_found = 1;
						}

						if(platform_size_found >= platform_size_needed)
						{
							// Now check to make sure that the actual destination tile is not just a few tiles down the same platform
							uint16 route_ahead_check = i;
							koord3d current_pos = pos;
							while(current_pos != cnv->get_schedule()->get_current_eintrag().pos && haltestelle_t::get_halt(welt, current_pos, cnv->get_besitzer()) == dest_halt && route_ahead_check < route->get_count())
							{
								current_pos = route->position_bei(route_ahead_check);
								route_ahead_check++;
								platform_size_found++;
							}
							if(current_pos != cnv->get_schedule()->get_current_eintrag().pos)
							{
								early_platform_index = i;
							}
							else
							{
								platform_size_found = 0;
								do_early_platform_search = false;
							}
						}
					} 
					else 
					{
						platform_size_found = 0;
					}
				} 
				else if(ribi_last == ribi && gr->get_halt().is_bound() && gr->get_halt() == dest_halt)
				{
					// A platform was found, but it continues so go on to its end
					early_platform_index = i;
				} 
				else 
				{
					// A platform was found, and has ended - check where this convoy should stop.
					if(platform_size_needed < platform_size_found && !fpl->get_current_eintrag().reverse)
					{
						// Do not go to the end, but stop part way along the platform.
						const uint16 difference = platform_size_found - platform_size_needed;
						early_platform_index -= (difference / 2u);
						if(difference > 2)
						{
							early_platform_index ++;
						}
					}
					sch1->unreserve(cnv->self);
					success = true;
					break;
				}
				ribi_last = ribi;
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
		return false;
	}

	// ok, switch everything green ...
	FOR(slist_tpl<grund_t*>, const g, signs) {
		if (signal_t* const signal = g->find<signal_t>()) {
			signal->set_zustand(roadsign_t::gruen);
		}
	}
	if (do_early_platform_search) {
		// if an early platform was found, stop there
		if(early_platform_index!=INVALID_INDEX) {
			next_signal_index = early_platform_index;
			// directly modify the route
			route->truncate_from(early_platform_index);
		}
	}
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
	if(  bd->ist_wasser()  ) 
	{
		// If there are permissive constraints, this vehicle cannot use the open water.
		return besch->get_way_constraints().get_permissive() == 0;
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
				// private road
				return false;
			}
		}
	}
#endif
	return (check_access(w)  &&  w->get_max_speed()>0 && check_way_constraints(*w));
}



/* Since slopes are handled different for ships
 * @author prissi
 */
void
schiff_t::calc_drag_coefficient(const grund_t *gr)
{
	if(gr == NULL)
	{
		return;
	}
	
	// flat water
	current_friction = get_friction_of_waytype(water_wt);
	if(gr->get_weg_hang()) {
		// hill up or down => in lock => deccelarte
		current_friction += 15;
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


// this routine is called by find_route, to determined if we reached a destination
bool aircraft_t::ist_ziel(const grund_t *gr,const grund_t *) const
{
	if(state!=looking_for_parking) 
	{
		// search for the end of the runway
		const weg_t *w=gr->get_weg(air_wt);
		if(w  &&  w->get_besch()->get_styp()==1) 
		{
			// ok here is a runway
			ribi_t::ribi ribi= w->get_ribi_unmasked();
			int success = 1;
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&approach_dir)!=0)
			{
				// pointing in our direction
				return true;
				/* BELOW CODE NOT WORKING
				// Check for length
				const uint16 min_runway_length_meters = besch->get_minimum_runway_length();
				const uint16 min_runway_length_tiles = min_runway_length_meters / welt->get_settings().get_meters_per_tile();
				for(uint16 i = 0; i <= min_runway_length_tiles; i ++) 
				{
					if (const ribi_t::ribi dir = ribi & approach_dir & ribi_t::nsow[i]) 
					{
						const grund_t* gr2 = welt->lookup_kartenboden(gr->get_pos().get_2d() + koord(dir));
						if(gr2)
						{
							const weg_t* w2 = gr2->get_weg(air_wt);
							if(
								w2 && 
								w2->get_besch()->get_styp() == 1 && 
								ribi_t::ist_einfach(w2->get_ribi_unmasked()) &&
								(w2->get_ribi_unmasked() & approach_dir) != 0
								)
							{
								// All is well - there is runway here.
								continue;
							}
							else
							{
								goto bad_runway;
							}
						}
						else
						{
							goto bad_runway;
						}
					}
					
					else
					{
bad_runway:
						// Reached end of runway before iteration for total number of minimum runway tiles exhausted:
						// runway too short.
						success = 0;
						break;
					}
				}
							
				//return true;
				return success;*/
			}
		}
	}
	else {
		// otherwise we just check, if we reached a free stop position of this halt
		if(gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
			return 1;
		}
	}
	return 0;
}



// for flying thingies, everywhere is good ...
// another function only called during route searching
ribi_t::ribi
aircraft_t::get_ribi(const grund_t *gr) const
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
				ribi_t::ribi mask = w->get_ribi_maske();
				return (mask) ? (r & ~ribi_t::rueckwaerts(mask)) : r;
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
bool
aircraft_t::ist_befahrbar(const grund_t *bd) const
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
		if(!target_rt.find_route( welt, rt->position_bei(suchen), this, 500, ribi_t::alle, cnv->get_heaviest_vehicle(), 100 )) {
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
		cnv->update_route(suchen, target_rt);
		cnv->set_next_stop_index(INVALID_INDEX);
		return true;
	}
}


/* reserves runways (reserve true) or removes the reservation
 * finishes when reaching end tile or leaving the ground (end of runway)
 * @return true if the reservation is successful
 */
int aircraft_t::block_reserver( uint32 start, uint32 end, bool reserve ) const
{
	const route_t *route = cnv->get_route();
	if (route->empty()) {
		return 0;
	}

	bool start_now = false;

	uint16 runway_tiles = end - start;
	uint16 runway_meters = runway_tiles * welt->get_settings().get_meters_per_tile();
	const uint16 min_runway_length_meters = besch->get_minimum_runway_length();
	int success = runway_meters >= min_runway_length_meters ? 1 : 2;

	for(  uint32 i=start;  success  &&  i<end  &&  i<route->get_count();  i++) {

		grund_t *gr = welt->lookup(route->position_bei(i));
		runway_t * sch1 = gr ? (runway_t *)gr->get_weg(air_wt) : NULL;
		if(sch1==NULL) {
			if(reserve) {
				if(!start_now) 
				{
					// touched down here
					start = i;
				}
				else 
				{
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
					success = 0;
					end = i;
					break;
				}
				// end of runway?
				if(i>start  &&  ribi_t::ist_einfach(sch1->get_ribi_unmasked())  ) 
				{
					runway_tiles = (i + 1) - start;
					runway_meters = runway_tiles * welt->get_settings().get_meters_per_tile();
					success = success == 0 ? 0 : runway_meters >= min_runway_length_meters ? 1 : 2;
					return success;
				}
			}
			else if(!sch1->unreserve(cnv->self)) {
				if(start_now) {
					// reached a reserved or free track => finished
					return success;
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
		for(uint32 i=start;  i<end;  i++  ) {
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

// handles all the decisions on the ground and in the air
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

	route_t &route = *cnv->get_route();
	convoi_t::route_infos_t &route_infos = cnv->get_route_infos();

	uint16 next_block = cnv->get_next_stop_index() - 1;
	uint16 last_index = route.get_count() - 1;
	if (next_block > 65000) 
	{
		existing_convoy_t convoy(*cnv);
		const sint32 brake_steps = convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), cnv->get_akt_speed());
		const sint32 route_steps = route_infos.calc_steps(route_infos.get_element(route_index).steps_from_start, route_infos.get_element(last_index).steps_from_start);
		if (route_steps <= brake_steps)
		{ 	
			// we need a longer route to decide, whether we will have to throttle:
			schedule_t *fpl = cnv->get_schedule();
			uint8 index = fpl->get_aktuell();
			bool reversed = cnv->get_reverse_schedule();
			fpl->increment_index(&index, &reversed);
			if (reroute(last_index, fpl->eintrag[index].pos))
			{
				fpl->advance();
				cnv->set_next_stop_index(INVALID_INDEX);
				last_index = route.get_count() - 1;
			}
		}
	}

	if(route_index<takeoff  &&  route_index>1  &&  takeoff<last_index) {
		// check, if tile occupied by a plane on ground
		for(  uint8 i = 1;  i<gr->get_top();  i++  ) {
			ding_t *d = gr->obj_bei(i);
			if(  d->get_typ()==ding_t::aircraft  &&  ((aircraft_t *)d)->is_on_ground()  ) {
				restart_speed = 0;
				return false;
			}
		}
		// need to reserve runway?
		runway_t *rw = (runway_t *)gr->get_weg(air_wt);
		if(rw==NULL) {
			cnv->suche_neue_route();
			return false;
		}
		// next tile a runway => then reserve
		if(rw->get_besch()->get_styp()==weg_t::type_elevated) {
			// try to reserve the runway
			const uint16 runway_length_tiles = ((suchen+1) - touchdown) - takeoff;
			const int runway_state = block_reserver(takeoff,takeoff+100,true);
			if(runway_state != 1)
			{
				// runway already blocked ...
				restart_speed = 0;

				if(runway_state == 2)
				{
					// Runway too short - explain to player
					runway_too_short = true;
				}
				else
				{
					runway_too_short = false;
				}

				return false;
			}
		}
		runway_too_short = false;
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
	if(  route_index == touchdown - HOLDING_PATTERN_OFFSET  ) 
	{
		const int runway_state = block_reserver( touchdown, suchen+1, true );
		if( runway_state != 1 ) 
		{
			
			if(runway_state == 2)
			{
				// Runway too short - explain to player
				runway_too_short = true;
				// TODO: Find some way of recalculating the runway
				// length here. Recalculating the route produces
				// crashes.
			}
			else
			{
				runway_too_short = false;
			}

			route_index -= HOLDING_PATTERN_LENGTH;
			return true;
		}
		state = landing;
		return true;
		runway_too_short = false;
	}

	if(  route_index == touchdown - HOLDING_PATTERN_LENGTH - HOLDING_PATTERN_OFFSET  &&  state != circling  ) 
	{
		// just check, if the end of runway is free; we will wait there
		const int runway_state = block_reserver( touchdown, suchen+1, true );
		if(runway_state == 1) 
		{
			route_index += HOLDING_PATTERN_LENGTH;
			// can land => set landing height
			state = landing;
			runway_too_short = false;
		}
		else 
		{
			if(runway_state == 2)
			{
				// Runway too short - explain to player
				runway_too_short = true;
			}
			else
			{
				runway_too_short = false;
			}
			
			// circle slowly next round
			state = circling;
			cnv->must_recalc_data();
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
void
aircraft_t::betrete_feld()
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
	runway_too_short = false;

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
	runway_too_short = false;
}



aircraft_t::~aircraft_t()
{
	// mark aircraft (after_image) dirty, since we have no "real" image
	mark_image_dirty( bild, -flughoehe-hoff-2 );
	mark_image_dirty( bild, 0 );
}



void
aircraft_t::set_convoi(convoi_t *c)
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




schedule_t * aircraft_t::erzeuge_neuen_fahrplan() const
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
uint8
aircraft_t::get_approach_ribi( koord3d start, koord3d ziel )
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


// BG, 07.08.2012: calculates a potential route without modifying any aircraft data.
/* 
Allows partial routing for route extending or re-routing e.g. if runway not available 
as well as calculating a complete route from gate to gate.

There are several flight phases in a "normal" complete route: 
1) taxiing from start gate to runway
2) starting on the runway
3) flying to the destination runway
4) circling in the holding pattern
5) landing on destination runway
6) taxiing to destination gate
Both start and end point might be somewhere in one of these states. 
Start state <= end state, of course.

As coordinates are koord3d, we are able to detect being in the air or on ground, aren't we?.
Try to handle the transition tiles like (virtual) waypoints and the partial (ground) routes as blocks: 
- start of departure runway, 
- takeoff, 
- holding "switch", 
- touchdown, 
- end of required length of arrival runway
*/
bool aircraft_t::calc_route_internal(
	karte_t *welt, 
	const koord3d &start,            // input: start of (partial) route to calculate
	const koord3d &ziel,             // input: end of (partial) route to calculate
	sint32 max_speed,                // input: in the air 
	uint32 weight,                   // input: gross weight of aircraft in kg (typical aircrafts don't have trailers)
	aircraft_t::flight_state &state, // input/output: at start
	sint16 &flughoehe,               // input/output: at start
	sint16 &target_height,           // output: at end of takeoff
	bool &runway_too_short,          // output: either departure or arrival runway
	uint32 &takeoff,                 // output: route index to takeoff tile at departure airport
	uint32 &touchdown,               // output: scheduled route index to touchdown tile at arrival airport
	uint32 &suchen,                  // output: scheduled route index to end of (required length of) arrival runway. 
	route_t &route)                  // output: scheduled route from start to ziel
{
	//DBG_MESSAGE("aircraft_t::calc_route_internal()","search route from %i,%i,%i to %i,%i,%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	runway_too_short = false;
	suchen = takeoff = touchdown = INVALID_INDEX;

	const weg_t *w_start = welt->lookup(start)->get_weg(air_wt);
	bool start_in_air = w_start == NULL;

	const weg_t *w_ziel = welt->lookup(ziel)->get_weg(air_wt);
	bool end_in_air = w_ziel == NULL;

	if(!start_in_air) 
	{
		// see, if we find a direct route: We are finished
		state = aircraft_t::taxiing;
		if(route.calc_route( welt, start, ziel, this, max_speed, weight, 0)) 
		{
			// ok, we can taxi to our location
			return true;
		}
	}

	koord3d search_start, search_end;
	if(start_in_air  ||  (w_start->get_besch()->get_styp()==1  &&  ribi_t::ist_einfach(w_start->get_ribi())) ) {
		// we start here, if we are in the air or at the end of a runway
		search_start = start;
		start_in_air = true;
		route.clear();
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
		if(!route.find_route( welt, start, this, max_speed, ribi_t::alle, weight, welt->get_settings().get_max_route_steps())) {
			DBG_MESSAGE("aircraft_t::calc_route()","failed");
			return false;
		}
		// save the route
		search_start = route.back();
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

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::alle, weight, welt->get_settings().get_max_route_steps())) {
		// well, probably this is a waypoint
		end_in_air = true;
		search_end = ziel;
	}
	else {
		// save target route
		search_end = end_route.back();
	}
	//DBG_MESSAGE("aircraft_t::calc_route()","end at ground (%s)",search_end.get_str());

	// create target route
	if(!start_in_air) 
	{
		takeoff = route.get_count()-1;
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
				route.append(gr->get_pos());
			} while(  over>0  );
			// out of map
			if(gr==NULL) {
				dbg->error("aircraft_t::calc_route()","out of map!");
				return false;
			}
			// need some extra step to avoid 180 deg turns
			if( start_dir.x!=0  &&  sgn(start_dir.x)!=sgn(search_end.x-search_start.x)  ) {
				route.append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 1 : -1 ) )->get_pos() );
				route.append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->get_pos() );
			}
			else if( start_dir.y!=0  &&  sgn(start_dir.y)!=sgn(search_end.y-search_start.y)  ) {
				route.append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 1 : -1 ,0) )->get_pos() );
				route.append( welt->lookup_kartenboden(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 2 : -2 ,0) )->get_pos() );
			}
		}
		else {
			// init with startpos
			dbg->error("aircraft_t::calc_route()","Invalid route calculation: start is on a single direction field ...");
		}
		state = taxiing;
		flughoehe = 0;
		target_height = ((sint16)start.z*TILE_HEIGHT_STEP)/Z_TILE_STEP;
	}
	else 
	{
		// init with current pos (in air ... )
		route.clear();
		route.append( start );
		state = flying;
		if(flughoehe==0) {
			flughoehe = 3*TILE_HEIGHT_STEP;
		}
		takeoff = 0;
		target_height = (((sint16)start.z+3)*TILE_HEIGHT_STEP)/Z_TILE_STEP;
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
		suchen = touchdown = INVALID_INDEX;
	}

	// just some straight routes ...
	if(!route.append_straight_route(welt,landing_start)) {
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
		static const koord circle_koord[HOLDING_PATTERN_LENGTH]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		for(  int  i=0;  i < HOLDING_PATTERN_LENGTH;  i++  ) {
			circlepos += circle_koord[(offset + i + HOLDING_PATTERN_LENGTH) % HOLDING_PATTERN_LENGTH];
			if(welt->ist_in_kartengrenzen(circlepos)) {
				route.append( welt->lookup_kartenboden(circlepos)->get_pos() );
			}
			else {
				// could only happen during loading old savegames;
				// in new versions it should not possible to build a runway here
				route.clear();
				dbg->error("aircraft_t::calc_route()","airport too close to the edge! (Cannot go to %i,%i!)",circlepos.x,circlepos.y);
				return false;
			}
		}

		touchdown = route.get_count()+2;
		route.append_straight_route(welt,search_end);

		// now the route to ziel search point (+1, since it will check before entering the tile ...)
		suchen = route.get_count()-1;

		// now we just append the rest
		for( int i=end_route.get_count()-2;  i>=0;  i--  ) {
			route.append(end_route.position_bei(i));
		}
	}

	//DBG_MESSAGE("aircraft_t::calc_route_internal()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route.get_count()-1, state );
	return true;
}


// BG, 08.08.2012: extracted from ist_weg_frei()
bool aircraft_t::reroute(const uint16 route_index, const koord3d &ziel)
{
	// new aircraft state after successful routing:
	aircraft_t::flight_state xstate = state; 
	sint16 xflughoehe = flughoehe;               
	sint16 xtarget_height;
	bool xrunway_too_short;
	uint32 xtakeoff;   // new route index to takeoff tile at departure airport
	uint32 xtouchdown; // new scheduled route index to touchdown tile at arrival airport
	uint32 xsuchen;    // new scheduled route index to end of (required length of) arrival runway. 
	route_t xroute;    // new scheduled route from start to ziel

	route_t &route = *cnv->get_route();
	bool done = calc_route_internal(welt, route.position_bei(route_index), ziel, 
		speed_to_kmh(cnv->get_min_top_speed()), cnv->get_heaviest_vehicle(), 	
		xstate, xflughoehe, xtarget_height, xrunway_too_short, xtakeoff, xtouchdown, xsuchen, xroute);
	if (done)
	{
		// convoy replaces existing route starting at route_index with found route. 
		cnv->update_route(route_index, xroute);
		cnv->set_next_stop_index(INVALID_INDEX);
		state = xstate;
		flughoehe = xflughoehe;
		target_height = xtarget_height;
		runway_too_short = xrunway_too_short;
		if (takeoff >= route_index)
			takeoff = xtakeoff != INVALID_INDEX ? route_index + xtakeoff : INVALID_INDEX;
		if (touchdown >= route_index)
			touchdown = xtouchdown != INVALID_INDEX ? route_index + xtouchdown : INVALID_INDEX;
		if (suchen >= route_index)
			suchen = xsuchen != INVALID_INDEX ? route_index + xsuchen : INVALID_INDEX;
	}
	return done;
}


// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
bool aircraft_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
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

	takeoff = touchdown = suchen = INVALID_INDEX;
	bool result = calc_route_internal(welt, start, ziel, max_speed, cnv->get_heaviest_vehicle(), state, flughoehe, target_height, runway_too_short, takeoff, touchdown, suchen, *route);
	cnv->set_next_stop_index(INVALID_INDEX);	
	return result;
}


void aircraft_t::hop()
{
	if(  !get_flag(ding_t::dirty)  ) {
		mark_image_dirty( bild, get_yoff()-flughoehe-hoff-2 );
	}

	sint32 new_speed_limit = speed_unlimited();

	sint32 new_friction = 0;

	// take care of inflight height ...
	const sint16 h_cur = height_scaling((sint16)get_pos().z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
	const sint16 h_next = height_scaling((sint16)pos_next.z)*TILE_HEIGHT_STEP/Z_TILE_STEP;

	switch(state) {
		case departing: {
			flughoehe = 0;
			target_height = h_cur;
			new_friction = max( 1, 28/(1+(route_index-takeoff)*2) ); // 9 5 4 3 2 2 1 1...

			// take off, when a) end of runway or b) last tile of runway or c) has reached minimum runway length
			weg_t *weg=welt->lookup(get_pos())->get_weg(air_wt);
			const sint16 runway_tiles_so_far = route_index - takeoff;
			const sint16 runway_meters_so_far = runway_tiles_so_far * welt->get_settings().get_meters_per_tile();
			const uint16 min_runway_length_meters = besch->get_minimum_runway_length();

			if(  (weg==NULL  ||  // end of runway (broken runway)
				 weg->get_besch()->get_styp()!=1  ||  // end of runway (grass now ... )
				 (route_index>takeoff+1  &&  ribi_t::ist_einfach(weg->get_ribi_unmasked())) )  ||  // single ribi at end of runway
				 (min_runway_length_meters && runway_meters_so_far >= min_runway_length_meters)   //  has reached minimum runway length
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
				// still descending
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
					target_height = height_scaling((sint16)cnv->get_route()->position_bei(touchdown).z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
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
		const sint16 target = target_height - ((sint16)z*TILE_HEIGHT_STEP)/Z_TILE_STEP;
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
