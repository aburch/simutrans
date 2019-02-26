/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 *
 * All moving stuff (vehicle_base_t) and all player vehicle (derived from vehicle_t)
 *
 * 01.11.99  Moved from simobj.cc
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

#include "../bauer/goods_manager.h"

#include "../simworld.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simconvoi.h"
#include "../simunits.h"

#include "../player/simplay.h"
#include "../simfab.h"
#include "../simware.h"
#include "../simhalt.h"
#include "../simsound.h"

#include "../display/simimg.h"
#include "../simmesg.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"

#include "../simline.h"

#include "../simintr.h"

#include "../obj/wolke.h"
#include "../obj/signal.h"
#include "../obj/roadsign.h"
#include "../obj/crossing.h"
#include "../obj/zeiger.h"

#include "../gui/karte.h"

#include "../descriptor/citycar_desc.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/roadsign_desc.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../bauer/vehikelbauer.h"

#include "simvehicle.h"
#include "simroadtraffic.h"


/* get dx and dy from dir (just to remind you)
 * any vehicle (including city cars and pedestrians)
 * will go this distance per sync step.
 * (however, the real dirs are only calculated during display, these are the old ones)
 */
sint8 vehicle_base_t::dxdy[ 8*2 ] = {
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
uint8 vehicle_base_t::old_diagonal_vehicle_steps_per_tile = 128;
uint8 vehicle_base_t::diagonal_vehicle_steps_per_tile = 181;
uint16 vehicle_base_t::diagonal_multiplier = 724;


// set only once, before loading!
void vehicle_base_t::set_diagonal_multiplier( uint32 multiplier, uint32 old_diagonal_multiplier )
{
	diagonal_multiplier = (uint16)multiplier;
	diagonal_vehicle_steps_per_tile = (uint8)(130560u/diagonal_multiplier) + 1;
	old_diagonal_vehicle_steps_per_tile = (uint8)(130560u/old_diagonal_multiplier) + 1;
}


// if true, convoi, must restart!
bool vehicle_base_t::need_realignment() const
{
	return old_diagonal_vehicle_steps_per_tile!=diagonal_vehicle_steps_per_tile  &&  ribi_t::is_bend(direction);
}

// [0]=xoff [1]=yoff
sint8 vehicle_base_t::driveleft_base_offsets[8][2] =
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
sint8 vehicle_base_t::overtaking_base_offsets[8][2];

// recalc offsets for overtaking
void vehicle_base_t::set_overtaking_offsets( bool driving_on_the_left )
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
bool vehicle_base_t::is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const
{
	const sint8 y_off_2 = 2*neu_yoff;
	const sint8 c_plus  = y_off_2 + neu_xoff;
	const sint8 c_minus = y_off_2 - neu_xoff;

	return ! (c_plus < OBJECT_OFFSET_STEPS*2  &&  c_minus < OBJECT_OFFSET_STEPS*2  &&  c_plus > -OBJECT_OFFSET_STEPS*2  &&  c_minus > -OBJECT_OFFSET_STEPS*2);
}


vehicle_base_t::vehicle_base_t():
	obj_t()
{
	image = IMG_EMPTY;
	set_flag( obj_t::is_vehicle );
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	dx = 0;
	dy = 0;
	zoff_start = zoff_end = 0;
	next_lane = 0;
	disp_lane = 2;
}


vehicle_base_t::vehicle_base_t(koord3d pos):
	obj_t(pos)
{
	image = IMG_EMPTY;
	set_flag( obj_t::is_vehicle );
	pos_next = pos;
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	dx = 0;
	dy = 0;
	zoff_start = zoff_end = 0;
	next_lane = 0;
	disp_lane = 2;
}


void vehicle_base_t::rotate90()
{
	koord3d pos_cur = get_pos();
	pos_cur.rotate90( welt->get_size().y-1 );
	set_pos( pos_cur );
	// directions are counterclockwise to ribis!
	direction = ribi_t::rotate90( direction );
	pos_next.rotate90( welt->get_size().y-1 );
	// new offsets: very tricky ...
	sint8 new_dx = -dy*2;
	dy = dx/2;
	dx = new_dx;
	// new pos + step offsets (only possible, since we know the height!
	sint8 neu_yoff = get_xoff()/2;
	set_xoff( -get_yoff()*2 );
	set_yoff( neu_yoff );
	// adjust disp_lane individually
}


void vehicle_base_t::leave_tile()
{
	// first: release crossing
	grund_t *gr = welt->lookup(get_pos());
	if(  gr  &&  gr->ist_uebergang()  ) {
		crossing_t *cr = gr->find<crossing_t>(2);
		grund_t *gr2 = welt->lookup(pos_next);
		if(  gr2==NULL  ||  gr2==gr  ||  !gr2->ist_uebergang()  ||  cr->get_logic()!=gr2->find<crossing_t>(2)->get_logic()  ) {
			cr->release_crossing(this);
		}
	}

	// then remove from ground (or search whole map, if failed)
	if(!get_flag(not_on_map)  &&  (gr==NULL  ||  !gr->obj_remove(this)) ) {

		// was not removed (not found?)
		dbg->error("vehicle_base_t::leave_tile()","'typ %i' %p could not be removed from %d %d", get_typ(), this, get_pos().x, get_pos().y);
		DBG_MESSAGE("vehicle_base_t::leave_tile()","checking all plan squares");

		// check, whether it is on another height ...
		const planquadrat_t *pl = welt->access( get_pos().get_2d() );
		if(  pl  ) {
			gr = pl->get_boden_von_obj(this);
			if(  gr  ) {
				gr->obj_remove(this);
				dbg->warning("vehicle_base_t::leave_tile()","removed vehicle typ %i (%p) from %d %d",get_typ(), this, get_pos().x, get_pos().y);
			}
			return;
		}

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->get_size().y; k.y++) {
			for(k.x=0; k.x<welt->get_size().x; k.x++) {
				grund_t *gr = welt->access( k )->get_boden_von_obj(this);
				if(gr && gr->obj_remove(this)) {
					dbg->warning("vehicle_base_t::leave_tile()","removed vehicle typ %i (%p) from %d %d",get_name(), this, k.x, k.y);
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehicle_base_t::leave_tile()","'%s' %p was not found on any map square!",get_name(), this);
		}
	}
}


void vehicle_base_t::enter_tile(grund_t* gr)
{
	if(!gr) {
		dbg->error("vehicle_base_t::enter_tile()","'%s' new position (%i,%i,%i)!",get_name(), get_pos().x, get_pos().y, get_pos().z );
		gr = welt->lookup_kartenboden(get_pos().get_2d());
		set_pos( gr->get_pos() );
	}
	gr->obj_add(this);
}


/* THE routine for moving vehicles
 * it will drive on as log as it can
 * @return the distance actually traveled
 */
uint32 vehicle_base_t::do_drive(uint32 distance)
{

	uint32 steps_to_do = distance >> YARDS_PER_VEHICLE_STEP_SHIFT;

	if(  steps_to_do == 0  ) {
		// ok, we will not move in this steps
		return 0;
	}
	// ok, so moving ...
	if(  !get_flag(obj_t::dirty)  ) {
		mark_image_dirty( image, 0 );
		set_flag( obj_t::dirty );
	}

	grund_t *gr = NULL; // if hopped, then this is new position

	uint32 steps_target = steps_to_do + steps;

	uint32 distance_travelled; // Return value

	if(  steps_target > steps_next  ) {
		// We are going far enough to hop.

		// We'll be adding steps_next+1 for each hop, as if we
		// started at the beginning of this tile, so for an accurate
		// count of steps done we must subtract the location we started with.
		sint32 steps_done = -steps;

		// Hop as many times as possible.
		while(  steps_target > steps_next  &&  (gr = hop_check())  ) {
			// now do the update for hopping
			steps_target -= steps_next+1;
			steps_done += steps_next+1;
			koord pos_prev(get_pos().get_2d());
			hop(gr);
			use_calc_height = true;

			// set offsets
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

		if(  steps_next == 0  ) {
			// only needed for aircrafts, which can turn on the same tile
			// the indicate the turn with this here
			steps_next = VEHICLE_STEPS_PER_TILE - 1;
			steps_target = VEHICLE_STEPS_PER_TILE - 1;
			steps_done -= VEHICLE_STEPS_PER_TILE - 1;
		}

		// Update internal status, how far we got within the tile.
		if(  steps_target <= steps_next  ) {
			steps = steps_target;
		}
		else {
			// could not go as far as we wanted (hop_check failed) => stop at end of tile
			steps = steps_next;
		}

		steps_done += steps;
		distance_travelled = steps_done << YARDS_PER_VEHICLE_STEP_SHIFT;

	}
	else {
		// Just travel to target, it's on same tile
		steps = steps_target;
		distance_travelled = distance & YARDS_VEHICLE_STEP_MASK; // round down to nearest step
	}

	if(use_calc_height) {
		calc_height(gr);
	}
	// remaining steps
	return distance_travelled;
}


// to make smaller steps than the tile granularity, we have to use this trick
void vehicle_base_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)steps*(uint16)raster_width;
	if(dx && dy) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}
	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (get_hoff(raster_width))/(4*16);
}


// calcs new direction and applies it to the vehicles
ribi_t::ribi vehicle_base_t::calc_set_direction(const koord3d& start, const koord3d& ende)
{
	ribi_t::ribi direction = ribi_t::none;

	const sint8 di = ende.x - start.x;
	const sint8 dj = ende.y - start.y;

	if(dj < 0 && di == 0) {
		direction = ribi_t::north;
		dx = 2;
		dy = -1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(dj > 0 && di == 0) {
		direction = ribi_t::south;
		dx = -2;
		dy = 1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di < 0 && dj == 0) {
		direction = ribi_t::west;
		dx = -2;
		dy = -1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di >0 && dj == 0) {
		direction = ribi_t::east;
		dx = 2;
		dy = 1;
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	} else if(di > 0 && dj > 0) {
		direction = ribi_t::southeast;
		dx = 0;
		dy = 2;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else if(di < 0 && dj < 0) {
		direction = ribi_t::northwest;
		dx = 0;
		dy = -2;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else if(di > 0 && dj < 0) {
		direction = ribi_t::northeast;
		dx = 4;
		dy = 0;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	} else {
		direction = ribi_t::southwest;
		dx = -4;
		dy = 0;
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	}
	// we could artificially make diagonals shorter: but this would break existing game behaviour
	return direction;
}


// this routine calculates the new height
// beware of bridges, tunnels, slopes, ...
void vehicle_base_t::calc_height(grund_t *gr)
{
	use_calc_height = false;	// assume, we are only needed after next hop
	zoff_start = zoff_end = 0;    // assume flat way

	if(gr==NULL) {
		gr = welt->lookup(get_pos());
	}
	if(gr==NULL) {
		// slope changed below a moving thing?!?
		return;
	}
	else if(  gr->ist_tunnel()  &&  gr->ist_karten_boden()  &&  !is_flying() ) {
		use_calc_height = true; // to avoid errors if underground mode is switched
		if(  grund_t::underground_mode == grund_t::ugm_none  ||
			(grund_t::underground_mode == grund_t::ugm_level  &&  gr->get_hoehe() < grund_t::underground_level)
		) {
			// need hiding? One of the few uses of XOR: not half driven XOR exiting => not hide!
			ribi_t::ribi hang_ribi = ribi_type( gr->get_grund_hang() );
			if((steps<(steps_next/2))  ^  ((hang_ribi&direction)!=0)  ) {
				set_image(IMG_EMPTY);
			}
			else {
				calc_image();
			}
		}
	}
	else {
		// force a valid image above ground, with special handling of tunnel entraces
		if(  get_image()==IMG_EMPTY  ) {
			if(  !gr->ist_tunnel()  &&  gr->ist_karten_boden()  ) {
				calc_image();
			}
		}

		// will not work great with ways, but is very short!
		slope_t::type hang = gr->get_weg_hang();
		if(  hang  ) {
			const uint slope_height = (hang & 7) ? 1 : 2;
			ribi_t::ribi hang_ribi = ribi_type(hang);
			if(  ribi_t::doubles(hang_ribi)  ==  ribi_t::doubles(direction)) {
				zoff_start = hang_ribi & direction                      ? 2*slope_height : 0;  // 0 .. 4
				zoff_end   = hang_ribi & ribi_t::backward(direction) ? 2*slope_height : 0;  // 0 .. 4
			}
			else {
				// only for shadows and movingobjs ...
				zoff_start = hang_ribi & direction                      ? slope_height+1 : 0;  // 0 .. 3
				zoff_end   = hang_ribi & ribi_t::backward(direction) ? slope_height+1 : 0;  // 0 .. 3
			}
		}
		else {
			zoff_start = (gr->get_weg_yoff() * 2) / TILE_HEIGHT_STEP;
			zoff_end = zoff_start;
		}
	}
}


sint16 vehicle_base_t::get_hoff(const sint16 raster_width) const
{
	sint16 h_start = -(sint8)TILE_HEIGHT_STEP * (sint8)zoff_start;
	sint16 h_end   = -(sint8)TILE_HEIGHT_STEP * (sint8)zoff_end;
	return ((h_start*steps + h_end*(256-steps))*raster_width) >> 9;
}


/* true, if one could pass through this field
 * also used for citycars, thus defined here
 */
vehicle_base_t *vehicle_base_t::no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_direction, const uint8 next_direction, const uint8 next_90direction, const private_car_t *pcar, sint8 lane_on_the_tile )
{
	bool cnv_overtaking = false; //whether this convoi is on passing lane.
	if(  cnv  ) {
		cnv_overtaking = cnv -> is_overtaking();
	}
	if(  pcar  ) {
		cnv_overtaking = pcar -> is_overtaking();
	}
	switch (lane_on_the_tile) {
		case 1:
		cnv_overtaking = true; //treated as convoi is overtaking.
		break;
		case -1:
		cnv_overtaking = false; //treated as convoi is not
		break;
	}
	// Search vehicle
	for(  uint8 pos=1;  pos<(uint8)gr->get_top();  pos++  ) {
		if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(pos))  ) {
			if(  v->get_typ()==obj_t::pedestrian  ) {
				continue;
			}

			// check for car
			uint8 other_direction=255;
			bool other_moving = false;
			bool other_overtaking = false; //whether the other convoi is on passing lane.
			if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
				// ignore ourself
				if(  cnv == at->get_convoi()  ) {
					continue;
				}
				other_direction = at->get_direction();
				other_moving = at->get_convoi()->get_akt_speed() > kmh_to_speed(1);
				other_overtaking = at->get_convoi()->is_overtaking();
			}
			// check for city car
			else if(  v->get_waytype() == road_wt  ) {
				other_direction = v->get_direction();
				if(  private_car_t const* const sa = obj_cast<private_car_t>(v)  ){
					if(  pcar == sa  ) {
						continue; // ignore ourself
					}
					other_moving = sa->get_current_speed() > 1;
					other_overtaking = sa->is_overtaking();
				}
			}

			// ok, there is another car ...
			if(  other_direction != 255  ) {
				if(  next_direction == other_direction  &&  !ribi_t::is_threeway(gr->get_weg_ribi(road_wt))  &&  cnv_overtaking == other_overtaking  ) {
					// only consider cars on same lane.
					// cars going in the same direction and no crossing => that mean blocking ...
					return v;
				}

				const ribi_t::ribi other_90direction = (gr->get_pos().get_2d() == v->get_pos_next().get_2d()) ? other_direction : calc_direction(gr->get_pos(), v->get_pos_next());
				if(  other_90direction == next_90direction   &&  cnv_overtaking == other_overtaking  ) {
					// Want to exit in same as other   ~50% of the time
					return v;
				}

				const bool drives_on_left = welt->get_settings().is_drive_left();
				const bool across = next_direction == (drives_on_left ? ribi_t::rotate45l(next_90direction) : ribi_t::rotate45(next_90direction)); // turning across the opposite directions lane
				const bool other_across = other_direction == (drives_on_left ? ribi_t::rotate45l(other_90direction) : ribi_t::rotate45(other_90direction)); // other is turning across the opposite directions lane
				if(  other_direction == next_direction  &&  !(other_across || across)  &&  cnv_overtaking == other_overtaking) {
					// only consider cars on same lane.
					// entering same straight waypoint as other ~18%
					return v;
				}

				const bool straight = next_direction == next_90direction; // driving straight
				const ribi_t::ribi current_90direction = straight ? ribi_t::backward(next_90direction) : (~(next_direction|ribi_t::backward(next_90direction)))&0x0F;
				const bool other_straight = other_direction == other_90direction; // other is driving straight
				const bool other_exit_same_side = current_90direction == other_90direction; // other is exiting same side as we're entering
				const bool other_exit_opposite_side = ribi_t::backward(current_90direction) == other_90direction; // other is exiting side across from where we're entering
				if(  across  &&  ((ribi_t::is_perpendicular(current_90direction,other_direction)  &&  other_moving)  ||  (other_across  &&  other_exit_opposite_side)  ||  ((other_across  ||  other_straight)  &&  other_exit_same_side  &&  other_moving) ) )  {
					// other turning across in front of us from orth entry dir'n   ~4%
					return v;
				}

				const bool headon = ribi_t::backward(current_direction) == other_direction; // we're meeting the other headon
				const bool other_exit_across = (drives_on_left ? ribi_t::rotate90l(next_90direction) : ribi_t::rotate90(next_90direction)) == other_90direction; // other is exiting by turning across the opposite directions lane
				if(  straight  &&  (ribi_t::is_perpendicular(current_90direction,other_direction)  ||  (other_across  &&  other_moving  &&  (other_exit_across  ||  (other_exit_same_side  &&  !headon))) ) ) {
					// other turning across in front of us, but allow if other is stopped - duplicating historic behaviour   ~2%
					return v;
				}
				else if(  other_direction == current_direction  &&  current_90direction == ribi_t::none  &&  cnv_overtaking == other_overtaking  ) {
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

bool vehicle_base_t::judge_lane_crossing( const uint8 current_direction, const uint8 next_direction, const uint8 other_next_direction, const bool is_overtaking, const bool forced_to_change_lane ) const
{
	bool on_left = false;
	const bool drives_on_left = welt->get_settings().is_drive_left();
	if(  is_overtaking  &&  !drives_on_left  ) {
		on_left = true;
	}
	if(  !is_overtaking  &&  drives_on_left  ) {
		on_left = true;
	}
	// go straight = 0, turn right = -1, turn left = 1.
	sint8 this_turn;
	if(  next_direction == ribi_t::rotate90(current_direction)  ) {
		this_turn = -1;
	}
	else if(  next_direction == ribi_t::rotate90l(current_direction)  ) {
		this_turn = 1;
	}
	else {
		// go straight?
		this_turn = 0;
	}
	sint8 other_turn;
	if(  other_next_direction == ribi_t::rotate90(current_direction)  ) {
		other_turn = -1;
	}
	else if(  other_next_direction == ribi_t::rotate90l(current_direction)  ) {
		other_turn = 1;
	}
	else {
		// go straight?
		other_turn = 0;
	}
	if(  on_left  ) {
		if(  forced_to_change_lane  &&  other_turn - this_turn >= 0  ) {
			return true;
		}
		else if(  other_turn - this_turn > 0  ) {
			return true;
		}
	}
	else {
		if(  forced_to_change_lane  &&  other_turn - this_turn <= 0  ) {
			return true;
		}
		else if(  other_turn - this_turn < 0  ) {
			return true;
		}
	}
	return false;
}

void vehicle_t::rotate90()
{
	vehicle_base_t::rotate90();
	previous_direction = ribi_t::rotate90( previous_direction );
	last_stop_pos.rotate90( welt->get_size().y-1 );
}


void vehicle_t::rotate90_freight_destinations(const sint16 y_size)
{
	// now rotate the freight
	FOR(slist_tpl<ware_t>, & tmp, fracht) {
		tmp.rotate90(y_size );
	}
}


void vehicle_t::set_convoi(convoi_t *c)
{
	/* cnv can have three values:
	 * NULL: not previously assigned
	 * 1 (only during loading): convoi wants to reserve the whole route
	 * other: previous convoi (in this case, currently always c==cnv)
	 *
	 * if c is NULL, then the vehicle is removed from the convoi
	 * (the rail_vehicle_t::set_convoi etc. routines must then remove a
	 *  possibly pending reservation of stops/tracks)
	 */
	assert(  c==NULL  ||  cnv==NULL  ||  cnv==(convoi_t *)1  ||  c==cnv);
	cnv = c;
	if(cnv) {
		// we need to re-establish the finish flag after loading
		if(leading) {
			route_t const& r = *cnv->get_route();
			check_for_finish = r.empty() || route_index >= r.get_count() || get_pos() == r.at(route_index);
		}
		if(  pos_next != koord3d::invalid  ) {
			route_t const& r = *cnv->get_route();
			if (!r.empty() && route_index < r.get_count() - 1) {
				grund_t const* const gr = welt->lookup(pos_next);
				if (!gr || !gr->get_weg(get_waytype())) {
					if (!(water_wt == get_waytype()  &&  gr->is_water())) { // ships on the open sea are valid
						pos_next = r.at(route_index + 1U);
					}
				}
			}
		}
		// just correct freight destinations
		FOR(slist_tpl<ware_t>, & c, fracht) {
			c.finish_rd(welt);
		}
	}
}


/**
 * Unload freight to halt
 * @return sum of unloaded goods
 * @author Hj. Malthaner
 */
uint16 vehicle_t::unload_cargo(halthandle_t halt, bool unload_all )
{
	uint16 sum_menge = 0, sum_delivered = 0, index = 0;
	if(  !halt.is_bound()  ) {
		return 0;
	}

	if(  halt->is_enabled( get_cargo_type() )  ) {
		if(  !fracht.empty()  ) {

			for(  slist_tpl<ware_t>::iterator i = fracht.begin(), end = fracht.end();  i != end;  ) {
				const ware_t& tmp = *i;

				halthandle_t end_halt = tmp.get_ziel();
				halthandle_t via_halt = tmp.get_zwischenziel();

				// check if destination or transfer is still valid
				if(  !end_halt.is_bound() || !via_halt.is_bound()  ) {
					// target halt no longer there => delete and remove from fab in transit
					fabrik_t::update_transit( &tmp, false );
					DBG_MESSAGE("vehicle_t::unload_freight()", "destination of %d %s is no longer reachable",tmp.menge,translator::translate(tmp.get_name()));
					total_freight -= tmp.menge;
					sum_weight -= tmp.menge * tmp.get_desc()->get_weight_per_unit();
					i = fracht.erase( i );
				}
				else if(  end_halt == halt || via_halt == halt  ||  unload_all  ) {

					//		    printf("Liefere %d %s nach %s via %s an %s\n",
					//                           tmp->menge,
					//			   tmp->name(),
					//			   end_halt->get_name(),
					//			   via_halt->get_name(),
					//			   halt->get_name());

					// here, only ordinary goods should be processed
					int menge = halt->liefere_an(tmp);
					sum_menge += menge;
					total_freight -= menge;
					sum_weight -= tmp.menge * tmp.get_desc()->get_weight_per_unit();

					index = tmp.get_index();

					if(end_halt==halt) {
						sum_delivered += menge;
					}

					i = fracht.erase( i );
				}
				else {
					++i;
				}
			}
		}
	}

	if(  sum_menge  ) {
		// book transported goods
		get_owner()->book_transported( sum_menge, get_desc()->get_waytype(), index );

		if(  sum_delivered  ) {
			// book delivered goods to destination
			get_owner()->book_delivered( sum_delivered, get_desc()->get_waytype(), index );
		}

		// add delivered goods to statistics
		cnv->book( sum_menge, convoi_t::CONVOI_TRANSPORTED_GOODS );

		// add delivered goods to halt's statistics
		halt->book( sum_menge, HALT_ARRIVED );
	}
	return sum_menge;
}


/**
 * Load freight from halt
 * @return amount loaded
 * @author Hj. Malthaner
 */
uint16 vehicle_t::load_cargo(halthandle_t halt, const vector_tpl<halthandle_t>& destination_halts)
{
	if(  !halt.is_bound()  ||  !halt->gibt_ab(desc->get_freight_type())  ) {
		return 0;
	}

	const uint16 total_freight_start = total_freight;
	const uint16 capacity_left = desc->get_capacity() - total_freight;
	if (capacity_left > 0) {

		slist_tpl<ware_t> freight_add;
		halt->fetch_goods( freight_add, desc->get_freight_type(), capacity_left, destination_halts);

		if(  freight_add.empty()  ) {
			// now empty, but usually, we can get it here ...
			return 0;
		}

		for(  slist_tpl<ware_t>::iterator iter_z = freight_add.begin();  iter_z != freight_add.end();  ) {
			ware_t &ware = *iter_z;

			total_freight += ware.menge;
			sum_weight += ware.menge * ware.get_desc()->get_weight_per_unit();

			// could this be joined with existing freight?
			FOR( slist_tpl<ware_t>, & tmp, fracht ) {
				// for pax: join according next stop
				// for all others we *must* use target coordinates
				if(  ware.same_destination(tmp)  ) {
					tmp.menge += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not join it to existing => load it
			if(  ware.menge != 0  ) {
				++iter_z;
				// we add list directly
			}
			else {
				iter_z = freight_add.erase(iter_z);
			}
		}

		if(  !freight_add.empty()  ) {
			fracht.append_list(freight_add);
		}
	}
	return total_freight - total_freight_start;
}


/**
 * Remove freight that no longer can reach it's destination
 * i.e. because of a changed schedule
 * @author Hj. Malthaner
 */
void vehicle_t::remove_stale_cargo()
{
	DBG_DEBUG("vehicle_t::remove_stale_freight()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	slist_tpl<ware_t> kill_queue;
	total_freight = 0;

	if (!fracht.empty()) {
		FOR(slist_tpl<ware_t>, & tmp, fracht) {
			bool found = false;

			if(  tmp.get_zwischenziel().is_bound()  ) {
				// the original halt exists, but does we still go there?
				FOR(minivec_tpl<schedule_entry_t>, const& i, cnv->get_schedule()->entries) {
					if(  haltestelle_t::get_halt( i.pos, cnv->get_owner()) == tmp.get_zwischenziel()  ) {
						found = true;
						break;
					}
				}
			}
			if(  !found  ) {
				// the target halt may have been joined or there is a closer one now, thus our original target is no longer valid
				const int offset = cnv->get_schedule()->get_current_stop();
				const int max_count = cnv->get_schedule()->entries.get_count();
				for(  int i=0;  i<max_count;  i++  ) {
					// try to unload on next stop
					halthandle_t halt = haltestelle_t::get_halt( cnv->get_schedule()->entries[ (i+offset)%max_count ].pos, cnv->get_owner() );
					if(  halt.is_bound()  ) {
						if(  halt->is_enabled(tmp.get_index())  ) {
							// ok, lets change here, since goods are accepted here
							tmp.access_zwischenziel() = halt;
							if (!tmp.get_ziel().is_bound()) {
								// set target, to prevent that unload_freight drops cargo
								tmp.set_ziel( halt );
							}
							found = true;
							break;
						}
					}
				}
			}

			if(  !found  ) {
				kill_queue.insert(tmp);
			}
			else {
				// since we need to point at factory (0,0), we recheck this too
				koord k = tmp.get_zielpos();
				fabrik_t *fab = fabrik_t::get_fab( k );
				tmp.set_zielpos( fab ? fab->get_pos().get_2d() : k );

				total_freight += tmp.menge;
			}
		}

		FOR(slist_tpl<ware_t>, const& c, kill_queue) {
			fabrik_t::update_transit( &c, false );
			fracht.remove(c);
		}
	}
	sum_weight =  get_cargo_weight() + desc->get_weight();
}


void vehicle_t::play_sound() const
{
	if(  desc->get_sound() >= 0  &&  !welt->is_fast_forward()  ) {
		welt->play_sound_area_clipped(get_pos().get_2d(), desc->get_sound());
	}
}


/**
 * Prepare vehicle for new ride.
 * Sets route_index, pos_next, steps_next.
 * If @p recalc is true this sets position and recalculates/resets movement parameters.
 * @author Hj. Malthaner
 */
void vehicle_t::initialise_journey(uint16 start_route_index, bool recalc)
{
	route_index = start_route_index+1;
	check_for_finish = false;
	use_calc_height = true;

	if(welt->is_within_limits(get_pos().get_2d())) {
		mark_image_dirty( get_image(), 0 );
	}

	route_t const& r = *cnv->get_route();
	if(!recalc) {
		// always set pos_next
		pos_next = r.at(route_index);
		assert(get_pos() == r.at(start_route_index));
	}
	else {
		// set pos_next
		if (route_index < r.get_count()) {
			pos_next = r.at(route_index);
		}
		else {
			// already at end of route
			check_for_finish = true;
		}
		set_pos(r.at(start_route_index));

		// recalc directions
		previous_direction = direction;
		direction = calc_set_direction( get_pos(), pos_next );

		zoff_start = zoff_end = 0;
		steps = 0;

		// reset lane yielding
		cnv->quit_yielding_lane();

		set_xoff( (dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
		set_yoff( (dy<0) ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );

		calc_image();
	}
	if ( ribi_t::is_single(direction) ) {
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	}
	else {
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	}
}


vehicle_t::vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player_) :
	vehicle_base_t(pos)
{
	this->desc = desc;

	set_owner( player_ );
	purchase_time = welt->get_current_month();
	cnv = NULL;
	speed_limit = SPEED_UNLIMITED;

	route_index = 1;

	smoke = true;
	direction = ribi_t::none;

	current_friction = 4;
	total_freight = 0;
	sum_weight = desc->get_weight();

	leading = last = false;
	check_for_finish = false;
	use_calc_height = true;
	has_driven = false;

	previous_direction = direction = ribi_t::none;
	target_halt = halthandle_t();
}


vehicle_t::vehicle_t() :
	vehicle_base_t()
{
	smoke = true;

	desc = NULL;
	cnv = NULL;

	route_index = 1;
	current_friction = 4;
	sum_weight = 10;
	total_freight = 0;

	leading = last = false;
	check_for_finish = false;
	use_calc_height = true;

	previous_direction = direction = ribi_t::none;
}


bool vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	return route->calc_route(welt, start, ziel, this, max_speed, 0 );
}


grund_t* vehicle_t::hop_check()
{
	// the leading vehicle will do all the checks
	if(leading) {
		if(check_for_finish) {
			// so we are there yet?
			cnv->ziel_erreicht();
			if(cnv->get_state()==convoi_t::INITIAL) {
				// to avoid crashes with airplanes
				use_calc_height = false;
			}
			return NULL;
		}

		// now check, if we can go here
		grund_t *bd = welt->lookup(pos_next);
		if(bd==NULL  ||  !check_next_tile(bd)  ||  cnv->get_route()->empty()) {
			// way (weg) not existent (likely destroyed) or no route ...
			cnv->suche_neue_route();
			return NULL;
		}

		// check for one-way sign etc.
		const waytype_t wt = get_waytype();
		if(  air_wt != wt  &&  route_index < cnv->get_route()->get_count()-1  ) {
			uint8 dir = get_ribi(bd);
			koord3d nextnext_pos = cnv->get_route()->at(route_index+1);
			if ( nextnext_pos == get_pos() ) {
				dbg->error("vehicle_t::hop_check", "route contains point (%s) twice for %s", nextnext_pos.get_str(), cnv->get_name());
			}
			uint8 new_dir = ribi_type(nextnext_pos - pos_next);
			if((dir&new_dir)==0) {
				// new one way sign here?
				cnv->suche_neue_route();
				return NULL;
			}
			// check for recently built bridges/tunnels or reverse branches (really slows down the game, so we do this only on slopes)
			if(  bd->get_weg_hang()  ) {
				grund_t *from;
				if(  !bd->get_neighbour( from, get_waytype(), ribi_type( get_pos(), pos_next ) )  ) {
					// way likely destroyed or altered => reroute
					cnv->suche_neue_route();
					return NULL;
				}
			}
		}

		sint32 restart_speed = -1;
		// ist_weg_frei() berechnet auch die Geschwindigkeit
		// mit der spaeter weitergefahren wird
		if(  !can_enter_tile( bd, restart_speed, 0 )  ) {
			// stop convoi, when the way is not free
			cnv->warten_bis_weg_frei(restart_speed);
			// If this is a road vehicle, this must unreserve tiles.
			if(  road_vehicle_t* r = obj_cast<road_vehicle_t>(this)  ) {
				r->unreserve_all_tiles();
			}

			// don't continue
			return NULL;
		}
		// we cache it here, hop() will use it to save calls to karte_t::lookup
		return bd;
	}
	else {
		// this is needed since in convoi_t::vorfahren the flag 'leading' is set to null
		if(check_for_finish) {
			return NULL;
		}
	}
	return welt->lookup(pos_next);
}


bool vehicle_t::can_enter_tile(sint32 &restart_speed, uint8 second_check_count)
{
	grund_t *gr = welt->lookup( pos_next );
	if(  gr  ) {
		if(  can_enter_tile( gr, restart_speed, second_check_count )  ) {
			// the vehicle can enter tile.
			return true;
		} else {
			// If this is a road vehicle, this must unreserve tiles.
			if(  road_vehicle_t* r = obj_cast<road_vehicle_t>(this)  ) {
				r->unreserve_all_tiles();
			}
			return false;
		}
	}
	else {
		if(  !second_check_count  ) {
			cnv->suche_neue_route();
		}
		return false;
	}
}


void vehicle_t::leave_tile()
{
	vehicle_base_t::leave_tile();
#ifndef DEBUG_ROUTES
	if(last  &&  reliefkarte_t::is_visible) {
			reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}
#endif
}


/* this routine add a vehicle to a tile and will insert it in the correct sort order to prevent overlaps
 * @author prissi
 */
void vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_base_t::enter_tile(gr);

	if(leading  &&  reliefkarte_t::is_visible  ) {
		reliefkarte_t::get_karte()->calc_map_pixel( get_pos().get_2d() );
	}
}


void vehicle_t::hop(grund_t* gr)
{
	leave_tile();

	koord3d pos_prev = get_pos();
	set_pos( pos_next );  // next field
	if(route_index<cnv->get_route()->get_count()-1) {
		route_index ++;
		pos_next = cnv->get_route()->at(route_index);
	}
	else {
		route_index ++;
		check_for_finish = true;
	}
	previous_direction = direction;

	// check if arrived at waypoint, and update schedule to next destination
	// route search through the waypoint is already complete
	if(  get_pos()==cnv->get_schedule_target()  ) {
		if(  route_index >= cnv->get_route()->get_count()  ) {
			// we end up here after loading a game or when a waypoint is reached which crosses next itself
			cnv->set_schedule_target( koord3d::invalid );
		}
		else {
			cnv->get_schedule()->advance();
			const koord3d ziel = cnv->get_schedule()->get_current_entry().pos;
			cnv->set_schedule_target( cnv->is_waypoint(ziel) ? ziel : koord3d::invalid );
		}
	}

	// this is a required hack for aircrafts! Aircrafts can turn on a single square, and this confuses the previous calculation!
	// author: hsiegeln
	if(!check_for_finish  &&  pos_prev==pos_next) {
		direction = calc_set_direction( get_pos(), pos_next);
		steps_next = 0;
	}
	else {
		if(  pos_next!=get_pos()  ) {
			direction = calc_set_direction( pos_prev, pos_next );
		}
		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::is_straight(welt->lookup(pos_next)->get_weg_ribi_unmasked(get_waytype()))  )  ||  welt->lookup(pos_next)->is_halt()) {
			// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
			direction = calc_set_direction( pos_prev, pos_next );
		}
	}

	// change image if direction changes
	if (previous_direction != direction) {
		calc_image();
	}
	sint32 old_speed_limit = speed_limit;

	enter_tile(gr);
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

	if(  leading  ) {
		if(  check_for_finish  &&  (direction==ribi_t::north  ||  direction==ribi_t::west)  ) {
			steps_next = (steps_next/2)+1;
		}
		cnv->add_running_cost( weg );
		cnv->must_recalc_data_front();
	}

	// update friction and friction weight of convoy
	sint16 old_friction = current_friction;
	calc_friction(gr);

	if (old_friction != current_friction) {
		cnv->update_friction_weight( (current_friction-old_friction) * (sint64)sum_weight);
	}

	// if speed limit changed, then cnv must recalc
	if (speed_limit != old_speed_limit) {
		if (speed_limit < old_speed_limit) {
			if (speed_limit < cnv->get_speed_limit()) {
				// update
				cnv->set_speed_limit(speed_limit);
			}
		}
		else {
			if (old_speed_limit == cnv->get_speed_limit()) {
				// convoy's speed limit may be larger now
				cnv->must_recalc_speed_limit();
			}
		}
	}
}


/* calculates the current friction coefficient based on the current track
 * flat, slope, curve ...
 * @author prissi, HJ, Dwachs
 */
void vehicle_t::calc_friction(const grund_t *gr)
{

	// assume straight flat track
	current_friction = 1;

	// curve: higher friction
	if(previous_direction != direction) {
		current_friction = 8;
	}

	// or a hill?
	const slope_t::type hang = gr->get_weg_hang();
	if(  hang != slope_t::flat  ) {
		const uint slope_height = (hang & 7) ? 1 : 2;
		if(  ribi_type(hang) == direction  ) {
			// hill up, since height offsets are negative: heavy decelerate
			current_friction += 15 * slope_height * slope_height;
		}
		else {
			// hill down: accelerate
			current_friction += -7 * slope_height * slope_height;
		}
	}
}


void vehicle_t::make_smoke() const
{
	// does it smoke at all?
	if(  smoke  &&  desc->get_smoke()  ) {
		// Hajo: only produce smoke when heavily accelerating or steam engine
		if(  cnv->get_akt_speed() < (sint32)((cnv->get_speed_limit() * 7u) >> 3)  ||  desc->get_engine_type() == vehicle_desc_t::steam  ) {
			grund_t* const gr = welt->lookup( get_pos() );
			if(  gr  ) {
				wolke_t* const abgas =  new wolke_t( get_pos(), get_xoff() + ((dx * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> 8), get_yoff() + ((dy * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> 8) + get_hoff(), desc->get_smoke() );
				if(  !gr->obj_add( abgas )  ) {
					abgas->set_flag( obj_t::not_on_map );
					delete abgas;
				}
				else {
					welt->sync_way_eyecandy.add( abgas );
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
sint64 vehicle_t::calc_revenue(const koord3d& start, const koord3d& end) const
{
	// may happen when waiting in station
	if (start == end || fracht.empty()) {
		return 0;
	}

	// cnv_kmh = lesser of min_top_speed, power limited top speed, and average way speed limits on trip, except aircraft which are not power limited and don't have speed limits
	sint32 cnv_kmh = cnv->get_speedbonus_kmh();

	sint64 value = 0;

	// cache speedbonus price
	const goods_desc_t* last_freight = NULL;
	sint64 freight_revenue = 0;

	sint32 dist = 0;
	if(  welt->get_settings().get_pay_for_total_distance_mode() == settings_t::TO_PREVIOUS  ) {
		// pay distance traveled
		dist = koord_distance( start, end );
	}

	FOR(slist_tpl<ware_t>, const& ware, fracht) {
		if(  ware.menge==0  ) {
			continue;
		}
		// which distance will be paid?
		switch(welt->get_settings().get_pay_for_total_distance_mode()) {
			case settings_t::TO_TRANSFER: {
				// pay distance traveled to next transfer stop

				// now only use the real gain in difference for the revenue (may as well be negative!)
				if (ware.get_zwischenziel().is_bound()) {
					const koord &zwpos = ware.get_zwischenziel()->get_basis_pos();
					// cast of koord_distance to sint32 is necessary otherwise the r-value would be interpreted as unsigned, leading to overflows
					dist = (sint32)koord_distance( zwpos, start ) - (sint32)koord_distance( end, zwpos );
				}
				else {
					dist = koord_distance( end, start );
				}
				break;
			}
			case settings_t::TO_DESTINATION: {
				// pay only the distance, we get closer to our destination

				// now only use the real gain in difference for the revenue (may as well be negative!)
				const koord &zwpos = ware.get_zielpos();
				// cast of koord_distance to sint32 is necessary otherwise the r-value would be interpreted as unsigned, leading to overflows
				dist = (sint32)koord_distance( zwpos, start ) - (sint32)koord_distance( end, zwpos );
				break;
			}
			default: ; // no need to recompute
		}

		// calculate freight revenue incl. speed-bonus
		if (ware.get_desc() != last_freight) {
			freight_revenue = ware_t::calc_revenue(ware.get_desc(), get_desc()->get_waytype(), cnv_kmh);
			last_freight = ware.get_desc();
		}
		const sint64 price = freight_revenue * (sint64)dist * (sint64)ware.menge;

		// sum up new price
		value += price;
	}

	// Hajo: Rounded value, in cents
	return (value+1500ll)/3000ll;
}


const char *vehicle_t::get_cargo_mass() const
{
	return get_cargo_type()->get_mass();
}


/**
 * Calculate transported cargo total weight in KG
 * @author Hj. Malthaner
 */
uint32 vehicle_t::get_cargo_weight() const
{
	uint32 weight = 0;

	FOR(slist_tpl<ware_t>, const& c, fracht) {
		weight += c.menge * c.get_desc()->get_weight_per_unit();
	}
	return weight;
}


void vehicle_t::get_cargo_info(cbuffer_t & buf) const
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


/**
 * Delete all vehicle load
 * @author Hj. Malthaner
 */
void vehicle_t::discard_cargo()
{
	FOR(  slist_tpl<ware_t>, w, fracht ) {
		fabrik_t::update_transit( &w, false );
	}
	fracht.clear();
	sum_weight =  desc->get_weight();
}


void vehicle_t::calc_image()
{
	image_id old_image=get_image();
	if (fracht.empty()) {
		set_image(desc->get_image_id(ribi_t::get_dir(get_direction()),NULL));
	}
	else {
		set_image(desc->get_image_id(ribi_t::get_dir(get_direction()), fracht.front().get_desc()));
	}
	if(old_image!=get_image()) {
		set_flag(obj_t::dirty);
	}
}


image_id vehicle_t::get_loaded_image() const
{
	return desc->get_image_id(ribi_t::dir_south, fracht.empty() ?  NULL  : fracht.front().get_desc());
}


// true, if this vehicle did not moved for some time
bool vehicle_t::is_stuck()
{
	return cnv==NULL  ||  cnv->is_waiting();
}


void vehicle_t::rdwr(loadsave_t *file)
{
	// this is only called from objlist => we save nothing ...
	assert(  file->is_saving()  ); (void)file;
}


void vehicle_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t r( file, "vehikel_t" );

	sint32 fracht_count = 0;

	if(file->is_saving()) {
		fracht_count = fracht.get_count();
		// we try to have one freight count to guess the right freight
		// when no desc is given
		if(fracht_count==0  &&  desc->get_freight_type()!=goods_manager_t::none  &&  desc->get_capacity()>0) {
			fracht_count = 1;
		}
	}

	obj_t::rdwr(file);

	// since obj_t does no longer save positions
	if(  file->get_version()>=101000  ) {
		koord3d pos = get_pos();
		pos.rdwr(file);
		set_pos(pos);
	}


	sint8 hoff = file->is_saving() ? get_hoff() : 0;

	if(file->get_version()<86006) {
		sint32 l;
		file->rdwr_long(purchase_time);
		file->rdwr_long(l);
		dx = (sint8)l;
		file->rdwr_long(l);
		dy = (sint8)l;
		file->rdwr_long(l);
		hoff = (sint8)(l*TILE_HEIGHT_STEP/16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(direction);
		file->rdwr_enum(previous_direction);
		file->rdwr_long(fracht_count);
		file->rdwr_long(l);
		route_index = (uint16)l;
		purchase_time = (purchase_time >> welt->ticks_per_world_month_shift) + welt->get_settings().get_starting_year();
DBG_MESSAGE("vehicle_t::rdwr_from_convoi()","bought at %i/%i.",(purchase_time%12)+1,purchase_time/12);
	}
	else {
		// prissi: changed several data types to save runtime memory
		file->rdwr_long(purchase_time);
		if(file->get_version()<99018) {
			file->rdwr_byte(dx);
			file->rdwr_byte(dy);
		}
		else {
			file->rdwr_byte(steps);
			file->rdwr_byte(steps_next);
			if(steps_next==old_diagonal_vehicle_steps_per_tile - 1  &&  file->is_loading()) {
				// reset diagonal length (convoi will be reset anyway, if game diagonal is different)
				steps_next = diagonal_vehicle_steps_per_tile - 1;
			}
		}
		sint16 dummy16 = ((16*(sint16)hoff)/TILE_HEIGHT_STEP);
		file->rdwr_short(dummy16);
		hoff = (sint8)((TILE_HEIGHT_STEP*(sint16)dummy16)/16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(direction);
		file->rdwr_enum(previous_direction);
		file->rdwr_long(fracht_count);
		file->rdwr_short(route_index);
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(direction)*2];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1];
	}

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx=get_xoff(), ddy=get_yoff()-hoff;
		sint8 i=1;
		dx = dxdy[ ribi_t::get_dir(direction)*2];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1];

		while(  !is_about_to_hop(ddx+dx*i,ddy+dy*i )  &&  i<16 ) {
			i++;
		}
		i--;
		set_xoff( ddx-(16-i)*dx );
		set_yoff( ddy-(16-i)*dy );
		if(file->is_loading()) {
			if(dx && dy) {
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
			cnv = (convoi_t *)target_info;	// will be checked during convoi reassignment
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
	if(file->get_version()<=112008) {
		koord3d pos_prev(koord3d::invalid);
		pos_prev.rdwr(file);
	}

	if(file->get_version()<=99004) {
		koord3d dummy;
		dummy.rdwr(file);	// current pos (is already saved as ding => ignore)
	}
	pos_next.rdwr(file);

	if(file->is_saving()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		desc = vehicle_builder_t::get_info(s);
		if(desc==NULL) {
			desc = vehicle_builder_t::get_info(translator::compatibility_name(s));
		}
		if(desc==NULL) {
			welt->add_missing_paks( s, karte_t::MISSING_VEHICLE );
			dbg->warning("vehicle_t::rdwr_from_convoi()","no vehicle pak for '%s' search for something similar", s);
		}
	}

	if(file->is_saving()) {
		if (fracht.empty()  &&  fracht_count>0) {
			// create dummy freight for savegame compatibility
			ware_t ware( desc->get_freight_type() );
			ware.menge = 0;
			ware.set_ziel( halthandle_t() );
			ware.set_zwischenziel( halthandle_t() );
			ware.set_zielpos( get_pos().get_2d() );
			ware.rdwr(file);
		}
		else {
			FOR(slist_tpl<ware_t>, ware, fracht) {
				ware.rdwr(file);
			}
		}
	}
	else {
		for(int i=0; i<fracht_count; i++) {
			ware_t ware(file);
			if(  (desc==NULL  ||  ware.menge>0)  &&  welt->is_within_limits(ware.get_zielpos())  &&  ware.get_desc()  ) {
				// also add, of the desc is unknown to find matching replacement
				fracht.insert(ware);
#ifdef CACHE_TRANSIT
				if(  file->get_version() <= 112000  )
#endif
					// restore in-transit information
					fabrik_t::update_transit( &ware, true );
			}
			else if(  ware.menge>0  ) {
				if(  ware.get_desc()  ) {
					dbg->error( "vehicle_t::rdwr_from_convoi()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
				}
				else {
					dbg->error( "vehicle_t::rdwr_from_convoi()", "%i of unknown to %s ignored!", ware.menge, ware.get_zielpos().get_str() );
				}
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
		// This used to be 2d, now it's 3d.
		if(file->get_version() < 112008) {
			if(file->is_saving()) {
				koord last_stop_pos_2d = last_stop_pos.get_2d();
				last_stop_pos_2d.rdwr(file);
			}
			else {
				// loading.  Assume ground level stop (could be wrong, but how would we know?)
				koord last_stop_pos_2d = koord::invalid;
				last_stop_pos_2d.rdwr(file);
				const grund_t* gr = welt->lookup_kartenboden(last_stop_pos_2d);
				if (gr) {
					last_stop_pos = koord3d(last_stop_pos_2d, gr->get_hoehe());
				}
				else {
					// no ground?!?
					last_stop_pos = koord3d::invalid;
				}
			}
		}
		else {
			// current version, 3d
			last_stop_pos.rdwr(file);
		}
	}

	if(file->is_loading()) {
		leading = last = false;	// dummy, will be set by convoi afterwards
		if(desc) {
			calc_image();

			// full weight after loading
			sum_weight =  get_cargo_weight() + desc->get_weight();
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


uint32 vehicle_t::calc_sale_value() const
{
	// if already used, there is a general price reduction
	double value = (double)desc->get_price();
	if(  has_driven  ) {
		value *= (1000 - welt->get_settings().get_used_vehicle_reduction()) / 1000.0;
	}
	// after 20 year, it has only half value
	return (uint32)( value * pow(0.997, (int)(welt->get_current_month() - get_purchase_time())));
}


void vehicle_t::show_info()
{
	if(  cnv != NULL  ) {
		cnv->open_info_window();
	} else {
		dbg->warning("vehicle_t::show_info()","cnv is null, can't open convoi window!");
	}
}


void vehicle_t::info(cbuffer_t & buf) const
{
	if(cnv) {
		cnv->info(buf);
	}
}


const char *vehicle_t::is_deletable(const player_t *)
{
	return "Fahrzeuge koennen so nicht entfernt werden";
}

ribi_t::ribi vehicle_t::get_next_90direction() const {
	const route_t* route = cnv->get_route();
	if(  route  &&  route_index < route->get_count() - 1u  ) {
		const koord3d pos_next2 = route->at(route_index + 1u);
		return calc_direction(pos_next,pos_next2);
	}
	return ribi_t::none;
}

vehicle_t::~vehicle_t()
{
	// remove vehicle's marker from the relief map
	reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
}


#ifdef MULTI_THREAD
void vehicle_t::display_overlay(int xpos, int ypos) const
{
	if(  cnv  &&  leading  ) {
#else
void vehicle_t::display_after(int xpos, int ypos, bool is_gobal) const
{
	if(  is_gobal  &&  cnv  &&  leading  ) {
#endif
		PIXVAL color = 0; // not used, but stop compiler warning about uninitialized
		char tooltip_text[1024];
		tooltip_text[0] = 0;
		uint8 state = env_t::show_vehicle_states;
		if(  state==1  ) {
			// only show when mouse over vehicle
			if(  welt->get_zeiger()->get_pos()==get_pos()  ) {
				state = 2;
			}
			else {
				state = 0;
			}
		}

		// now find out what has happened
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::WAITING_FOR_CLEARANCE:
			case convoi_t::CAN_START:
			case convoi_t::CAN_START_ONE_MONTH:
				if(  state>=2  ) {
					snprintf( tooltip_text, lengthof(tooltip_text), "%s (%s)", translator::translate("Waiting for clearance!"), cnv->get_schedule()->get_current_entry().pos.get_str() );
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::LOADING:
				if(  state>=1  ) {
					sprintf( tooltip_text, translator::translate("Loading (%i->%i%%)!"), cnv->get_loading_level(), cnv->get_loading_limit() );
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::EDIT_SCHEDULE:
//			case convoi_t::ROUTING_1:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Schedule changing!"), lengthof(tooltip_text) );
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::DRIVING:
				if(  state>=1  ) {
					grund_t const* const gr = welt->lookup(cnv->get_route()->back());
					if(  gr  &&  gr->get_depot()  ) {
						tstrncpy( tooltip_text, translator::translate("go home"), lengthof(tooltip_text) );
						color = color_idx_to_rgb(COL_GREEN);
					}
					else if(  cnv->get_no_load()  ) {
						tstrncpy( tooltip_text, translator::translate("no load"), lengthof(tooltip_text) );
						color = color_idx_to_rgb(COL_GREEN);
					}
				}
				break;

			case convoi_t::LEAVING_DEPOT:
				if(  state>=2  ) {
					tstrncpy( tooltip_text, translator::translate("Leaving depot!"), lengthof(tooltip_text) );
					color = color_idx_to_rgb(COL_GREEN);
				}
				break;

			case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			case convoi_t::CAN_START_TWO_MONTHS:
				snprintf( tooltip_text, lengthof(tooltip_text), "%s (%s)", translator::translate("clf_chk_stucked"), cnv->get_schedule()->get_current_entry().pos.get_str() );
				color = color_idx_to_rgb(COL_ORANGE);
				break;

			case convoi_t::NO_ROUTE:
				tstrncpy( tooltip_text, translator::translate("clf_chk_noroute"), lengthof(tooltip_text) );
				color = color_idx_to_rgb(COL_RED);
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
				display_ddd_proportional_clip( xpos, ypos, width, 0, color, color_idx_to_rgb(COL_BLACK), tooltip_text, true );
			}
		}
	}
}



road_vehicle_t::road_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player_, convoi_t* cn) :
	vehicle_t(pos, desc, player_)
{
	cnv = cn;
	pos_prev = koord3d::invalid;
	last_stop_for_intersection = koord3d::invalid;
}


road_vehicle_t::road_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : vehicle_t()
{
	rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_first) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL) {
			const goods_desc_t* w = (!fracht.empty() ? fracht.front().get_desc() : goods_manager_t::passengers);
			dbg->warning("road_vehicle_t::road_vehicle_t()","try to find a fitting vehicle for %s.",  w->get_name() );
			desc = vehicle_builder_t::get_best_matching(road_wt, 0, (fracht.empty() ? 0 : 50), is_first?50:0, speed_to_kmh(speed_limit), w, true, last_desc, is_last );
			if(desc) {
				DBG_MESSAGE("road_vehicle_t::road_vehicle_t()","replaced by %s",desc->get_name());
				// still wrong load ...
				calc_image();
			}
			if(!fracht.empty()  &&  fracht.front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht.remove_first();
			}
		}
		if(  desc  ) {
			last_desc = desc;
		}
		last_stop_for_intersection = koord3d::invalid;
		calc_disp_lane();
	}
}

void road_vehicle_t::rdwr_from_convoi(loadsave_t *file){
	vehicle_t::rdwr_from_convoi(file);
	if(  (env_t::previous_OTRP_data  &&  file->get_version() >= 120006)  ||  file->get_OTRP_version() >= 14  ) {
		koord3d pos_prev_rdwr = this->pos_prev;
		pos_prev_rdwr.rdwr(file);
		this->pos_prev = pos_prev_rdwr;
	} else {
		pos_prev = koord3d::invalid;
	}
}


void road_vehicle_t::rotate90()
{
	vehicle_t::rotate90();
	last_stop_for_intersection.rotate90(welt->get_size().y-1);
	pos_prev.rotate90(welt->get_size().y-1);
	for(uint16 i=0; i<reserving_tiles.get_count(); i++) {
		reserving_tiles[i].rotate90(welt->get_size().y-1);
	}
	calc_disp_lane();
}


void road_vehicle_t::calc_disp_lane()
{
	// driving in the back or the front
	ribi_t::ribi test_dir = welt->get_settings().is_drive_left() ? ribi_t::northeast : ribi_t::southwest;
	disp_lane = get_direction() & test_dir ? 1 : 3;
}

// need to reset halt reservation (if there was one)
bool road_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	assert(cnv);
	// free target reservation
	if(leading   &&  previous_direction!=ribi_t::none  &&  cnv  &&  target_halt.is_bound() ) {
		// now reserve our choice (beware: might be longer than one tile!)
		for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<cnv->get_route()->get_count();  length++  ) {
			target_halt->unreserve_position( welt->lookup( cnv->get_route()->at( cnv->get_route()->get_count()-length-1) ), cnv->self );
		}
	}
	target_halt = halthandle_t();	// no block reserved
	route_t::route_result_t r = route->calc_route(welt, start, ziel, this, max_speed, cnv->get_tile_length() );
	if(  r == route_t::valid_route_halt_too_short  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("Vehicle %s cannot choose because stop too short!"), cnv->get_name());
		welt->get_message()->add_message( (const char *)buf, ziel.get_2d(), message_t::traffic_jams, PLAYER_FLAG | cnv->get_owner()->get_player_nr(), cnv->front()->get_base_image() );
	}
	return r;
}


bool road_vehicle_t::check_next_tile(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->get_weg(road_wt);
	if(str==NULL  ||  str->get_max_speed()==0) {
		return false;
	}
	bool electric = cnv!=NULL  ?  cnv->needs_electrification() : desc->get_engine_type()==vehicle_desc_t::electric;
	if(electric  &&  !str->is_electrified()) {
		return false;
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			if(  rs->get_desc()->get_min_speed()>0  &&  rs->get_desc()->get_min_speed()>kmh_to_speed(get_desc()->get_topspeed())  ) {
				return false;
			}
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// private road
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound()  &&  cnv->is_waiting()  &&  rs->get_desc()->get_flags()&roadsign_desc_t::END_OF_CHOOSE_AREA) {
				return false;
			}
		}
	}
	return true;
}


// how expensive to go here (for way search)
// author prissi
int road_vehicle_t::get_cost(const grund_t *gr, const weg_t *w, const sint32 max_speed, ribi_t::ribi from) const
{
	// first favor faster ways
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
		// 15 hardcoded, see get_cost_upslope()
		costs += 15 * get_sloping_upwards( gr->get_weg_hang(), from );
	}
	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool road_vehicle_t::is_target(const grund_t *gr, const grund_t *prev_gr) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->get_halt()==target_halt  &&  target_halt->get_empty_lane(gr,cnv->self)!=0) {
		// now we must check the predecessor => try to advance as much as possible
		if(prev_gr!=NULL) {
			const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
			ribi_t::ribi ribi = ribi_type(dir);
			if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ) {
				// one way sign wrong direction
				return false;
			}
			grund_t *to;
			if(  !gr->get_neighbour(to,road_wt,ribi)  ||  !(to->get_halt()==target_halt)  ||  (gr->get_weg(get_waytype())->get_ribi_maske() & ribi_type(dir))!=0  ||  target_halt->get_empty_lane(to,cnv->self)==0  ) {
				// end of stop: Is it long enough?
				uint16 tiles = cnv->get_tile_length();
				uint8 empty_lane = 3;
				while(  tiles>1  ) {
					if(  !gr->get_neighbour(to,get_waytype(),ribi_t::backward(ribi))  ||  !(to->get_halt()==target_halt)  ||  (empty_lane &= target_halt->get_empty_lane(to,cnv->self))==0  ) {
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
void road_vehicle_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width, bool prev_based ) const
{
	vehicle_base_t::get_screen_offset( xoff, yoff, raster_width );

	if(  welt->get_settings().is_drive_left()  ) {
		const int drive_left_dir = ribi_t::get_dir(get_direction());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}

	// eventually shift position to take care of overtaking
	if(cnv) {
		sint8 tiles_overtaking = prev_based ? cnv->get_prev_tiles_overtaking() : cnv->get_tiles_overtaking();
		if(  tiles_overtaking>0  ) { /* This means the convoy is overtaking other vehicles. */
			xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
			yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
		}
		else if(  tiles_overtaking<0  ) { /* This means the convoy is overtaken by other vehicles. */
			xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width)/5;
			yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width)/5;
		}
	}
}


// chooses a route at a choose sign; returns true on success
bool road_vehicle_t::choose_route(sint32 &restart_speed, ribi_t::ribi start_direction, uint16 index)
{
	if(  cnv->get_schedule_target()!=koord3d::invalid  ) {
		// destination is a waypoint!
		return true;
	}

	// are we heading to a target?
	route_t *rt = cnv->access_route();
	target_halt = haltestelle_t::get_halt( rt->back(), get_owner() );
	if(  target_halt.is_bound()  ) {

		// since convois can long than one tile, check is more difficult
		bool can_go_there = true;
		bool original_route = (rt->back() == cnv->get_schedule()->get_current_entry().pos);
		for(  uint32 length=0;  can_go_there  &&  length<cnv->get_tile_length()  &&  length+1<rt->get_count();  length++  ) {
			if(  grund_t *gr = welt->lookup( rt->at( rt->get_count()-length-1) )  ) {
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
				target_halt->reserve_position( welt->lookup( rt->at( rt->get_count()-length-1) ), cnv->self );
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
			if(  !target_halt->find_free_position(road_wt,cnv->self,obj_t::road_vehicle  )) {
				restart_speed = 0;
				target_halt = halthandle_t();
				return false;
			}

			// now it make sense to search a route
			route_t target_rt;
			koord3d next3d = rt->at(index);
			if(  !target_rt.find_route( welt, next3d, this, speed_to_kmh(cnv->get_min_top_speed()), start_direction, welt->get_settings().get_max_choose_route_steps() )  ) {
				// nothing empty or not route with less than 33 tiles
				target_halt = halthandle_t();
				restart_speed = 0;
				return false;
			}

			// now reserve our choice (beware: might be longer than one tile!)
			for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<target_rt.get_count();  length++  ) {
				target_halt->reserve_position( welt->lookup( target_rt.at( target_rt.get_count()-length-1) ), cnv->self );
			}
			rt->remove_koord_from( index );
			rt->append( &target_rt );
		}
	}
	return true;
}


bool road_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8 second_check_count)
{
	// check for traffic lights (only relevant for the first car in a convoi)
	if(  leading  ) {
		// no further check, when already entered a crossing (to allow leaving it)
		if(  !second_check_count  ) {
			const grund_t *gr_current = welt->lookup(get_pos());
			if(  gr_current  &&  gr_current->ist_uebergang()  ) {
				return true;
			}
		}

		assert(gr);

		const strasse_t *str = (strasse_t *)gr->get_weg(road_wt);
		if(  !str  ||  gr->get_top() > 250  ) {
			// too many cars here or no street
			return false;
		}

		// first: check roadsigns
		const roadsign_t *rs = NULL;
		if(  str->has_sign()  ) {
			rs = gr->find<roadsign_t>();
			route_t const& r = *cnv->get_route();

			if(  rs  &&  (route_index + 1u < r.get_count())  ) {
				// since at the corner, our direction may be diagonal, we make it straight
				uint8 direction90 = ribi_type(get_pos(), pos_next);

				if(  rs->get_desc()->is_traffic_light()  &&  (rs->get_dir()&direction90) == 0  ) {
					// wait here
					restart_speed = 16;
					return false;
				}
				// check, if we reached a choose point
				else {
					// route position after road sign
					const koord pos_next_next = r.at(route_index + 1u).get_2d();
					// since at the corner, our direction may be diagonal, we make it straight
					direction90 = ribi_type( pos_next, pos_next_next );

					if(  rs->is_free_route(direction90)  &&  !target_halt.is_bound()  ) {
						if(  second_check_count  ) {
							return false;
						}
						if(  !choose_route( restart_speed, direction90, route_index )  ) {
							return false;
						}
					}
				}
			}
		}

		route_t const& r = *cnv->get_route();

		// At an intersection, decide whether the convoi should go on passing lane.
		// side road -> main road from passing lane side: vehicle should enter passing lane on main road.
		next_lane = 0;
		if(  (str->get_ribi_unmasked() == ribi_t::all  ||  ribi_t::is_threeway(str->get_ribi_unmasked()))  &&  str->get_overtaking_mode() <= oneway_mode  ) {
			const strasse_t* str_prev = route_index == 0 ? NULL : (strasse_t *)welt->lookup(r.at(route_index - 1u))->get_weg(road_wt);
			const grund_t* gr_next = route_index < r.get_count() - 1u ? welt->lookup(r.at(route_index + 1u)) : NULL;
			const strasse_t* str_next = gr_next ? (strasse_t*)gr_next -> get_weg(road_wt) : NULL;
			if(str_prev && str_next && str_prev->get_overtaking_mode() > oneway_mode  && str_next->get_overtaking_mode() <= oneway_mode) {
				const koord3d pos_next2 = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
				if(  (!welt->get_settings().is_drive_left()  &&  ribi_t::rotate90l(get_90direction()) == calc_direction(pos_next,pos_next2))  ||  (welt->get_settings().is_drive_left()  &&  ribi_t::rotate90(get_90direction()) == calc_direction(pos_next,pos_next2))  ) {
					// next: enter passing lane.
					next_lane = 1;
				}
			}
		}

		// When overtaking_mode changes from inverted_mode to others, no cars blocking must work as the convoi is on traffic lane. Otherwise, no_cars_blocking cannot recognize vehicles on the traffic lane of the next tile.
		//next_lane = -1 does NOT mean that the vehicle must go traffic lane on the next tile.
		const strasse_t* current_str = (strasse_t*)(welt->lookup(get_pos())->get_weg(road_wt));
		if(  current_str  &&  current_str->get_overtaking_mode()==inverted_mode  ) {
			if(  str->get_overtaking_mode()<inverted_mode  ) {
				next_lane = -1;
			}
		}

		if(  current_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) {
			next_lane = -1;
		}

		vehicle_base_t *obj = NULL;
		uint32 test_index = route_index;

		// way should be clear for overtaking: we checked previously
		// Now we have to consider even if convoi is overtaking!
		// calculate new direction
		koord3d next = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
		ribi_t::ribi curr_direction   = get_direction();
		ribi_t::ribi curr_90direction = calc_direction(get_pos(), pos_next);
		ribi_t::ribi next_direction   = calc_direction(get_pos(), next);
		ribi_t::ribi next_90direction = calc_direction(pos_next, next);
		
		obj = no_cars_blocking( gr, cnv, curr_direction, next_direction, next_90direction, NULL, next_lane );
		
		// If the next tile is an intersection, we have to refer the reservation.
		// However, if we are already in an intersection, we ignore it to avoid stuck.
		if(  str->get_overtaking_mode()<=oneway_mode  &&  ribi_t::is_threeway(str->get_ribi_unmasked())  ) {
			// try to reserve tiles
			bool overtaking_on_tile = cnv->is_overtaking();
			if(  next_lane==1  ) {
				overtaking_on_tile = true;
			} else if(  next_lane==-1  ) {
				overtaking_on_tile = false;
			}
			// since reserve function modifies variables of the instance...
			strasse_t* s = (strasse_t *)gr->get_weg(road_wt);
			if(  !s  ||  !s->reserve(this, overtaking_on_tile, get_pos(), next)  ) {
				if(  obj  &&  obj->is_stuck()  ) {
					// because the blocking vehicle is stuck too...
					restart_speed = 0;
					cnv->reset_waiting();
				} else {
					restart_speed =  cnv->get_akt_speed()*3/4;
				}
				return false;
			}
			// now we succeeded in reserving the road. register it.
			reserving_tiles.append(gr->get_pos());
		}

		// do not block intersections
		const bool drives_on_left = welt->get_settings().is_drive_left();
		bool int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));

		// do we have to stop before entering the intersection?
		// we consider prior direction of the intersection.
		if(  !welt->get_settings().get_stop_at_intersection_without_traffic_light()  ) {
			// stopping is not requested by the setting.
		} else if(  !ribi_t::is_threeway(str->get_ribi_unmasked())  ) {
			// this tile is not an intersection.
		} else if(  (curr_90direction&str->get_prior_direction())!=0  ) {
			// we are in the prior directions.
		} else if(  (drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction))==next_90direction  ) {
			// we make an inside turn. This tile might be a merging point, so we don't stop.
		} else if(  last_stop_for_intersection==gr->get_pos()  ) {
			// we already stopped for this intersection.
		} else if(  ribi_t::is_single(str->get_ribi())  ) {
			// this tile is a merging point. we don't stop.
		} else if(  (curr_90direction!=next_90direction)  &&  (ribi_t::backward(next_90direction)&str->get_ribi())==0  ) {
			// we make an outside turn, but crossing street is oneway.
		} else {
			// check for traffic light. If there is a traffic light, we don't have to stop here.
			bool traffic_light = false;
			if(  str->has_sign()  ) {
				rs = gr->find<roadsign_t>();
				if(  rs  &&  rs->get_desc()->is_traffic_light()  ) {
					traffic_light = true;
				}
			}
			// crossing traffic has priority. we have to stop.
			if(  !traffic_light  &&  cnv->get_akt_speed()>kmh_to_speed(5)  ) {
				restart_speed = 0;
				last_stop_for_intersection = gr->get_pos();
				return false;
			}
		}

		//If this convoi is overtaking, the convoi must avoid a head-on crash.
		if(  cnv->is_overtaking()  &&  current_str->get_overtaking_mode()!=inverted_mode  ){
			while(  test_index < route_index + 2u && test_index < r.get_count()  ){
				grund_t *grn = welt->lookup(r.at(test_index));
				if(  !grn  ) {
					cnv->suche_neue_route();
					return false;
				}
				for(  uint8 pos=1;  pos<(volatile uint8)grn->get_top();  pos++  ){
					if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(grn->obj_bei(pos))  ){
						if(  v->get_typ()==obj_t::pedestrian  ) {
							continue;
						}
						ribi_t::ribi other_direction=255;
						if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
							//ignore ourself
							if(  cnv == at->get_convoi()  ||  at->get_convoi()->is_overtaking()  ){
								continue;
							}
							other_direction = at->get_90direction();
						}
						//check for city car
						else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
							if(  caut->is_overtaking()  ) {
								continue;
							}
							other_direction = v->get_90direction();
						}
						if(  other_direction != 255  ){
							//There is another car. We have to check if this convoi is facing or not.
							ribi_t::ribi this_direction;
							if(  test_index-route_index==0  ) this_direction = get_90direction();
							if(  test_index-route_index==1  ) this_direction = get_next_90direction();
							if(  ribi_t::reverse_single(this_direction) == other_direction  ) {
								//printf("%s: crash avoid. (%d,%d)\n", cnv->get_name(), get_pos().x, get_pos().y);
								cnv->set_tiles_overtaking(0);
							}
						}
					}
				}
				test_index++;
			}
		}

		test_index = route_index + 1u; //reset test_index
		// we have to assume the lane that this vehicle goes in the intersection.
		sint8 lane_of_the_tile = next_lane;
		overtaking_mode_t mode_of_start_point = str->get_overtaking_mode();
		bool enter_passing_lane_from_side_road = false;
		// check exit from crossings and intersections, allow to proceed after 4 consecutive
		while(  !obj   &&  (str->is_crossing()  ||  int_block)  &&  test_index < r.get_count()  &&  test_index < route_index + 4u  ) {
			if(  str->is_crossing()  ) {
				crossing_t* cr = gr->find<crossing_t>(2);
				if(  !cr->request_crossing(this)  ) {
					restart_speed = 0;
					return false;
				}
			}

			// test next position
			gr = welt->lookup(r.at(test_index));
			if(  !gr  ) {
				// way (weg) not existent (likely destroyed)
				if(  !second_check_count  ) {
					cnv->suche_neue_route();
				}
				return false;
			}

			str = (strasse_t *)gr->get_weg(road_wt);
			if(  !str  ||  gr->get_top() > 250  ) {
				// too many cars here or no street
				if(  !second_check_count  &&  !str) {
					cnv->suche_neue_route();
				}
				return false;
			}

			if(  mode_of_start_point<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) {
				lane_of_the_tile = -1;
			}
			else if(  str->get_overtaking_mode()==inverted_mode  ) {
				lane_of_the_tile = 1;
			}
			else if(  str->get_overtaking_mode()<=oneway_mode  &&  enter_passing_lane_from_side_road  ) {
				lane_of_the_tile = 1;
			}
			
			// Decide whether the convoi should go on passing lane.
			// side road -> main road from passing lane side: vehicle should enter passing lane on main road.
			if(   ribi_t::is_threeway(str->get_ribi_unmasked())  &&  str->get_overtaking_mode() <= oneway_mode  ) {
				const strasse_t* str_prev = (strasse_t *)welt->lookup(r.at(test_index - 1u))->get_weg(road_wt);
				if(  str_prev  &&  str_prev->get_overtaking_mode() > oneway_mode  &&  test_index + 1u < r.get_count()  ) {
					ribi_t::ribi dir_1 = calc_direction(r.at(test_index - 1u), r.at(test_index));
					ribi_t::ribi dir_2 = calc_direction(r.at(test_index), r.at(test_index + 1u));
					if(  (!welt->get_settings().is_drive_left()  &&  ribi_t::rotate90l(dir_1) == dir_2)  ||  (welt->get_settings().is_drive_left()  &&  ribi_t::rotate90(dir_1) == dir_2)  ) {
						// next: enter passing lane.
						lane_of_the_tile = 1;
						enter_passing_lane_from_side_road = true;
					}
				}
			}

			// check cars
			curr_direction   = next_direction;
			curr_90direction = next_90direction;
			if(  test_index + 1u < r.get_count()  ) {
				next                 = r.at(test_index + 1u);
				next_direction   = calc_direction(r.at(test_index - 1u), next);
				next_90direction = calc_direction(r.at(test_index),      next);
				obj = no_cars_blocking( gr, cnv, curr_direction, next_direction, next_90direction, NULL, lane_of_the_tile );
			}
			else {
				next                 = r.at(test_index);
				next_90direction = calc_direction(r.at(test_index - 1u), next);
				if(  curr_direction == next_90direction  ||  !gr->is_halt()  ) {
					// check cars but allow to enter intersection if we are turning even when a car is blocking the halt on the last tile of our route
					// preserves old bus terminal behaviour
					obj = no_cars_blocking( gr, cnv, curr_direction, next_90direction, ribi_t::none, NULL, lane_of_the_tile );
				}
			}

			// check roadsigns
			if(  str->has_sign()  ) {
				rs = gr->find<roadsign_t>();
				if(  rs  ) {
					// since at the corner, our direction may be diagonal, we make it straight
					if(  rs->get_desc()->is_traffic_light()  &&  (rs->get_dir() & curr_90direction)==0  ) {
						// wait here
						restart_speed = 16;
						return false;
					}
					// check, if we reached a choose point

					else if(  rs->is_free_route(curr_90direction)  &&  !target_halt.is_bound()  ) {
						if(  second_check_count  ) {
							return false;
						}
						if(  !choose_route( restart_speed, curr_90direction, test_index )  ) {
							return false;
						}
					}
				}
			}
			else {
				rs = NULL;
			}
			
			
			if(  str->get_overtaking_mode()<=oneway_mode  &&  ribi_t::is_threeway(str->get_ribi_unmasked())  ) {
				// try to reserve tiles
				bool overtaking_on_tile = cnv->is_overtaking();
				if(  lane_of_the_tile==1  ) {
					overtaking_on_tile = true;
				} else if(  lane_of_the_tile==-1  ) {
					overtaking_on_tile = false;
				}
				// since reserve function modifies variables of the instance...
				strasse_t* s = (strasse_t *)gr->get_weg(road_wt);
				if(  !s  ||  !s->reserve(this, overtaking_on_tile, r.at(test_index - 1u), next)  ) {
					if(  obj  &&  obj->is_stuck()  ) {
						// because the blocking vehicle is stuck too...
						restart_speed = 0;
						cnv->reset_waiting();
					} else {
						restart_speed =  cnv->get_akt_speed()*3/4;
					}
					return false;
				}
				// now we succeeded in reserving the road. register it.
				reserving_tiles.append(gr->get_pos());
			}

			// check for blocking intersection
			int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));

			test_index++;
		}

		if(  obj  &&  test_index > route_index + 1u  &&  !str->is_crossing()  &&  !int_block  ) {
			// found a car blocking us after checking at least 1 intersection or crossing
			// and the car is in a place we could stop. So if it can move, assume it will, so we will too.
			// but check only upto 8 cars ahead to prevent infinite recursion on roundabouts.
			if(  second_check_count >= 8  ) {
				return false;
			}
			if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
				const convoi_t* const ocnv = car->get_convoi();
				sint32 dummy;
				if(  ocnv->front()->get_route_index() < ocnv->get_route()->get_count()  &&  ocnv->front()->can_enter_tile( dummy, second_check_count + 1 )  ) {
					return true;
				}
			}
		}

		// stuck message ...
		if(  obj  &&  !second_check_count  ) {
			// Process is different whether the road is for one-way or two-way
			sint8 overtaking_mode = str->get_overtaking_mode();
			if(  overtaking_mode <= oneway_mode  ) {
				// road is one-way.
				bool can_judge_overtaking = (test_index == route_index + 1u);
				// The overtaking judge method itself works only when test_index==route_index+1, that means the front tile is not an intersection.
				// However, with halt_mode we want to simulate a bus terminal. Overtaking in a intersection is essential. So we make a exception to the test_index==route_index+1 condition, although it is not clear that this exception is safe or not!
				if(  !can_judge_overtaking  &&  test_index == route_index + 2u  &&  overtaking_mode == halt_mode  ) {
					can_judge_overtaking = true;
				}
				if(  can_judge_overtaking  ) {
					// no intersections or crossings, we might be able to overtake this one ...
					overtaker_t *over = obj->get_overtaker();
					if(  over  ) {
						// not overtaking/being overtake: we need to make a more thought test!
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
							convoi_t* const ocnv = car->get_convoi();
							// yielding vehicle should not be overtaken by the vehicle whose maximum speed is same.
							bool yielding_factor = true;
							if(  ocnv->get_yielding_quit_index() != -1  &&  this->get_speed_limit() - ocnv->get_speed_limit() < kmh_to_speed(10)  ) {
								yielding_factor = false;
							}
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking()  &&  !other_lane_blocked()  &&  yielding_factor  &&  cnv->can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : ocnv->get_akt_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
								// this vehicle changes lane. we have to unreserve tiles.
								unreserve_all_tiles();
								return true;
							}
							strasse_t *str=(strasse_t *)gr->get_weg(road_wt);
							sint32 cnv_max_speed = (int)fmin(cnv->get_speed_limit(), str->get_max_speed()*kmh_to_speed(1));
							sint32 other_max_speed = (int)fmin(ocnv->get_speed_limit(), str->get_max_speed()*kmh_to_speed(1));
							if(  cnv->is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
								//If the convoi is on passing lane and there is slower convoi in front of this, this convoi request the slower to go to traffic lane.
								ocnv->set_requested_change_lane(true);
							}
							//For the case that the faster convoi is on traffic lane.
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
								if(  vehicle_base_t* const br = car->other_lane_blocked()  ) {
									if(  road_vehicle_t const* const blk = obj_cast<road_vehicle_t>(br)  ) {
										if(  car->get_direction() == blk->get_direction() && abs(car->get_convoi()->get_speed_limit() - blk->get_convoi()->get_speed_limit()) < kmh_to_speed(5)  ){
											//same direction && (almost) same speed vehicle exists.
											ocnv->yield_lane_space();
										}
									}
									else if(  private_car_t* pbr = dynamic_cast<private_car_t*>(br)  ) {
										if(  car->get_direction() == pbr->get_direction() && abs(car->get_speed_limit() - pbr->get_speed_limit()) < kmh_to_speed(5)  ){
											//same direction && (almost) same speed vehicle exists.
											ocnv->yield_lane_space();
										}
									}
								}
							}
						}
						else if(  private_car_t* const caut = obj_cast<private_car_t>(obj)  ) {
							// yielding vehicle should not be overtaken by the vehicle whose maximum speed is same.
							bool yielding_factor = true;
							if(  caut->get_yielding_quit_index() != -1  &&  this->get_speed_limit() - caut->get_speed_limit() < kmh_to_speed(10)  ) {
								yielding_factor = false;
							}
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking()  &&  yielding_factor  &&  !other_lane_blocked(false)  &&  cnv->can_overtake(caut, caut->get_current_speed(), VEHICLE_STEPS_PER_TILE)  ) {
								// this vehicle changes lane. we have to unreserve tiles.
								unreserve_all_tiles();
								return true;
							}
							sint32 other_max_speed = caut->get_speed_limit();
							if(  cnv->is_overtaking() && kmh_to_speed(10) <  get_speed_limit() - other_max_speed  ) {
								//If the convoi is on passing lane and there is slower convoi in front of this, this convoi request the slower to go to traffic lane.
								caut->set_requested_change_lane(true);
							}
							//For the case that the faster car is on traffic lane.
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking() && kmh_to_speed(10) <  get_speed_limit() - other_max_speed  ) {
								if(  vehicle_base_t* const br = caut->other_lane_blocked(false)  ) {
									if(  road_vehicle_t const* const blk = dynamic_cast<road_vehicle_t*>(br)  ) {
										if(  caut->get_direction() == blk->get_direction() && abs(caut->get_speed_limit() - blk->get_convoi()->get_speed_limit()) < kmh_to_speed(5)  ){
											//same direction && (almost) same speed vehicle exists.
											caut->yield_lane_space();
										}
									}
									else if(  private_car_t* pbr = dynamic_cast<private_car_t*>(br)  ) {
										if(  caut->get_direction() == pbr->get_direction() && abs(caut->get_speed_limit() - pbr->get_speed_limit()) < kmh_to_speed(5)  ){
											//same direction && (almost) same speed vehicle exists.
											caut->yield_lane_space();
										}
									}
								}
							}
						}
					}
				}
				// we have to wait ...
				if(  obj->is_stuck()  ) {
					// end of traffic jam, but no stuck message, because previous vehicle is stuck too
					restart_speed = 0;
					cnv->reset_waiting();
					if(  cnv->is_overtaking()  &&  other_lane_blocked() == NULL  ) {
						cnv->set_tiles_overtaking(0);
					}
				}
				else {
					restart_speed = (cnv->get_akt_speed()*3)/4;
				}
			}
			else if(  overtaking_mode <= loading_only_mode  ) {
				// road is two-way and overtaking is allowed on the stricter condition.
				if(  obj->is_stuck()  ) {
					// end of traffic jam, but no stuck message, because previous vehicle is stuck too
					restart_speed = 0;
					//cnv->set_tiles_overtaking(0);
					cnv->reset_waiting();
				}
				else {
					if(  test_index == route_index + 1u  ) {
						// no intersections or crossings, we might be able to overtake this one ...
						overtaker_t *over = obj->get_overtaker();
						if(  over  &&  !over->is_overtaken()  ) {
							if(  over->is_overtaking()  ) {
								// otherwise we would stop every time being overtaken
								return true;
							}
							// not overtaking/being overtake: we need to make a more thought test!
							if(  road_vehicle_t const* const car = dynamic_cast <road_vehicle_t*>(obj)  ) {
								convoi_t* const ocnv = car->get_convoi();
								if(  cnv->can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
									// this vehicle changes lane. we have to unreserve tiles.
									unreserve_all_tiles();
									return true;
								}
							}
							else if(  private_car_t* const caut = dynamic_cast<private_car_t*>(obj)  ) {
								if(  cnv->can_overtake(caut, caut->get_desc()->get_topspeed(), VEHICLE_STEPS_PER_TILE)  ) {
									// this vehicle changes lane. we have to unreserve tiles.
									unreserve_all_tiles();
									return true;
								}
							}
						}
					}
					// we have to wait ...
					restart_speed = (cnv->get_akt_speed()*3)/4;
					//cnv->set_tiles_overtaking(0);
				}
			}
			else {
				// lane change is prohibited.
				if(  obj->is_stuck()  ) {
					// end of traffic jam, but no stuck message, because previous vehicle is stuck too
					restart_speed = 0;
					//cnv->set_tiles_overtaking(0);
					cnv->reset_waiting();
				}
				else {
					// we have to wait ...
					restart_speed = (cnv->get_akt_speed()*3)/4;
					//cnv->set_tiles_overtaking(0);
				}
			}
		}

		const koord3d pos_next2 = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
		// We have to calculate offset.
		uint32 offset = 0;
		if(  route_index<r.get_count()  &&  str->get_pos()!=r.at(route_index)  ) {
			for(uint32 test_index = route_index+1u; test_index<r.get_count(); test_index++){
				offset += 1;
				if(  str->get_pos()==r.at(test_index)  ) {
					break;
				}
			}
		}
		// If this vehicle is on passing lane and the next tile prohibites overtaking, this vehicle must wait until traffic lane become safe.
		// When condition changes, overtaking should be quitted once.
		if(  (cnv->is_overtaking()  &&  str->get_overtaking_mode()==prohibited_mode)  ||  (cnv->is_overtaking()  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode()<inverted_mode  &&  static_cast<strasse_t*>(welt->lookup(get_pos())->get_weg(road_wt))->get_overtaking_mode()<=oneway_mode)  ) {
			if(  vehicle_base_t* v = other_lane_blocked(false, offset)  ) {
				if(  v->get_waytype() == road_wt  ) {
					restart_speed = 0;
					cnv->reset_waiting();
					cnv->set_next_cross_lane(true);
					return false;
				}
			}
			// There is no vehicle on traffic lane.
			// cnv->set_tiles_overtaking(0); is done in enter_tile()
		}
		// If the next tile is our destination and we are on passing lane of oneway mode road, we have to wait until traffic lane become safe.
		if(  cnv->is_overtaking()  &&  str->get_overtaking_mode()==oneway_mode  &&  route_index == r.get_count() - 1u  ) {
			halthandle_t halt = haltestelle_t::get_halt(welt->lookup(r.at(route_index))->get_pos(),cnv->get_owner());
			vehicle_base_t* v = other_lane_blocked(false, offset);
			if(  halt.is_bound()  &&  gr->get_weg_ribi(get_waytype())!=0  &&  v  &&  v->get_waytype() == road_wt  ) {
				restart_speed = 0;
				cnv->reset_waiting();
				cnv->set_next_cross_lane(true);
				return false;
			}
			// There is no vehicle on traffic lane.
		}
		// If this vehicle is on traffic lane and the next tile forces to go passing lane, this vehicle must wait until passing lane become safe.
		if(  !cnv->is_overtaking()  &&  str->get_overtaking_mode() == inverted_mode  ) {
			if(  vehicle_base_t* v = other_lane_blocked(false, offset)  ) {
				if(  v->get_waytype() == road_wt  ) {
					restart_speed = 0;
					cnv->reset_waiting();
					cnv->set_next_cross_lane(true);
					return false;
				}
			}
			// There is no vehicle on passing lane.
			next_lane = 1;
			return true;
		}
		// If the next tile is a intersection, lane crossing must be checked before entering.
		// The other vehicle is ignored if it is stopping to avoid stuck.
		const grund_t *gr = route_index<r.get_count() ? welt->lookup(r.at(route_index)) : NULL;
		const strasse_t *stre= gr ? (strasse_t *)gr->get_weg(road_wt) : NULL;
		const ribi_t::ribi way_ribi = stre ? stre->get_ribi_unmasked() : ribi_t::none;
		if(  stre  &&  stre->get_overtaking_mode() <= oneway_mode  &&  (way_ribi == ribi_t::all  ||  ribi_t::is_threeway(way_ribi))  ) {
			if(  const vehicle_base_t* v = other_lane_blocked(true)  ) {
				if(  road_vehicle_t const* const at = dynamic_cast<const road_vehicle_t*>(v)  ) {
					if(  at->get_convoi()->get_akt_speed()>kmh_to_speed(1)  &&  judge_lane_crossing(calc_direction(get_pos(),pos_next), calc_direction(pos_next,pos_next2), at->get_90direction(), cnv->is_overtaking(), false)  ) {
						// vehicle must stop.
						restart_speed = 0;
						cnv->reset_waiting();
						cnv->set_next_cross_lane(true);
						return false;
					}
				}
			}
			else if(  private_car_t const* const pcar = dynamic_cast<const private_car_t*>(v)  ) {
					if(  pcar->get_current_speed()>kmh_to_speed(1)  &&  judge_lane_crossing(calc_direction(get_pos(),pos_next), calc_direction(pos_next,pos_next2), pcar->get_90direction(), cnv->is_overtaking(), false)  ) {
						// vehicle must stop.
						restart_speed = 0;
						cnv->reset_waiting();
						cnv->set_next_cross_lane(true);
						return false;
					}
				}
		}
		// For the case that this vehicle is fixed to passing lane and is on traffic lane.
		if(  str->get_overtaking_mode() <= oneway_mode  &&  cnv->get_lane_affinity() == 1  &&  !cnv->is_overtaking()  ) {
			if(  vehicle_base_t* v = other_lane_blocked()  ) {
				if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
					convoi_t* ocnv = car->get_convoi();
					if(  ocnv  &&  abs(cnv->get_speed_limit() - ocnv->get_speed_limit()) < kmh_to_speed(5)  ) {
						cnv->yield_lane_space();
					}
				}
				else if(  private_car_t const* const pcar = dynamic_cast<private_car_t*>(v)  ) {
					if(  abs(get_speed_limit()-pcar->get_speed_limit())<kmh_to_speed(5)  ) {
						cnv->yield_lane_space();
					}
				}
			}
			else {
				// go on passing lane.
				cnv->set_tiles_overtaking(3);
			}
		}
		// If there is a vehicle that requests lane crossing, this vehicle must stop to yield space.
		if(  vehicle_base_t* v = other_lane_blocked(true)  ) {
			if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
				if(  at->get_convoi()->get_next_cross_lane()  &&  at==at->get_convoi()->back()  ) {
					// vehicle must stop.
					restart_speed = 0;
					cnv->reset_waiting();
					return false;
				}
			}
			else if(  private_car_t* const pcar = obj_cast<private_car_t>(v)  ) {
				if(  pcar->get_next_cross_lane()  ) {
					// vehicle must stop.
					restart_speed = 0;
					cnv->reset_waiting();
					return false;
				}
			}
		}

		return obj==NULL;
	}

	return true;
}


overtaker_t* road_vehicle_t::get_overtaker()
{
	return cnv;
}

vehicle_base_t* road_vehicle_t::other_lane_blocked(const bool only_search_top, sint8 offset) const{
	// This function calculate whether the convoi can change lane.
	// only_search_top == false: check whether there's no car in -1 ~ +1 section.
	// only_search_top == true: check whether there's no car in front of this vehicle. (Not the same lane.)
	if(  leading  ){
		route_t const& r = *cnv->get_route();
		// we have to know the index of the tail of convoy.
		sint32 tail_index = -1;
		if(  cnv->get_vehicle_count()==1  ){
			tail_index = route_index==0 ? 0 : route_index-1;
		}else{
			for(uint32 test_index = 0; test_index < r.get_count(); test_index++){
				koord3d test_pos = r.at(test_index);
				if(test_pos==cnv->back()->get_pos()){
					tail_index = test_index;
					break;
				}
				if(test_pos==cnv->front()->get_pos()) break;
			}
		}
		
		// tile inspection
		const sint32 start_index = route_index+offset < (sint32)r.get_count() ? route_index+offset : r.get_count() - 1u;
		sint32 end_index;
		if(  only_search_top  ) {
			// only the front tile is inspected.
			end_index = start_index;
		} else {
			// we finish the inspection of the tile behind the tail of convoy.
			end_index = max(0, tail_index-1+offset);
		}
		for(sint32 test_index = start_index; test_index >= end_index; test_index--){
			grund_t *gr = welt->lookup(r.at(test_index));
			if(  !gr  ) {
				cnv->suche_neue_route();
				return NULL;
			}

			// this function cannot process vehicles on twoway and related mode road.
			const strasse_t* str = (strasse_t *)gr->get_weg(road_wt);
			if(  !str  ||  (str->get_overtaking_mode()>=twoway_mode  &&  str->get_overtaking_mode()<inverted_mode)  ) {
				continue;
			}
			
			bool ignore_stucked = !only_search_top  &&  test_index==end_index;

			for(  uint8 pos=1;  pos<(volatile uint8)gr->get_top();  pos++  ) {
				if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(pos))  ) {
					if(  v->get_typ()==obj_t::pedestrian  ) {
						continue;
					}
					if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
						// ignore ourself
						if(  cnv == at->get_convoi()  ) {
							continue;
						}
						if(  cnv->is_overtaking() == at->get_convoi()->is_overtaking()  ){
							continue;
						}
						// Ignore stopping convoi on the tile behind this convoi to change lane in traffic jam.
						if(  ignore_stucked  &&  at->get_convoi()->get_akt_speed() == 0  ) {
							continue;
						}
						if(  test_index==tail_index-1+offset  ||  test_index==tail_index+offset  ){
							uint8 tail_offset = 0;
							if(  test_index==tail_index-1+offset  ) tail_offset = 1;
							if(  test_index+tail_offset>=1  &&  test_index+tail_offset<(sint32)r.get_count()-1  &&   judge_lane_crossing(calc_direction(r.at(test_index-1u+tail_offset),r.at(test_index+tail_offset)), calc_direction(r.at(test_index+tail_offset),r.at(test_index+1u+tail_offset)),  v->get_90direction(), cnv->is_overtaking(), true)  ){
								return v;
							}
							continue;
						}
						return v;
					}
					else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
						if(  cnv->is_overtaking() == caut->is_overtaking()  ){
							continue;
						}
						// Ignore stopping convoi on the tile behind this convoi to change lane in traffic jam.
						if(  ignore_stucked  &&  caut->get_current_speed() == 0  ) {
							continue;
						}
						if(  test_index==tail_index-1+offset  ||  test_index==tail_index+offset  ){
							uint8 tail_offset = 0;
							if(  test_index==tail_index-1+offset  ) tail_offset = 1;
							if(  test_index+tail_offset>=1  &&  test_index+tail_offset<(sint32)r.get_count()-1  &&   judge_lane_crossing(calc_direction(r.at(test_index-1u+tail_offset),r.at(test_index+tail_offset)), calc_direction(r.at(test_index+tail_offset),r.at(test_index+1u+tail_offset)),  v->get_90direction(), cnv->is_overtaking(), true)  ){
								return v;
							}
							continue;
						}
						return v;
					}
				}
			}
			if(  only_search_top  ) {
				break;
			}
		}
	}
	return NULL;
}

void road_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);
	calc_disp_lane();

	const int cargo = get_total_cargo();
	strasse_t *str = (strasse_t*)gr->get_weg(road_wt);
	ribi_t::ribi enter_direction = calc_direction(pos_prev, gr->get_pos());
	str->book(cargo, WAY_STAT_GOODS, enter_direction);
	if (  leading  )  {
		str->book(1, WAY_STAT_CONVOIS, enter_direction);
		cnv->update_tiles_overtaking();
		if(  next_lane==1  ) {
			cnv->set_tiles_overtaking(3);
			next_lane = 0;
		}
		//decide if overtaking convoi should go back to the traffic lane.
		if(  cnv->get_tiles_overtaking() == 1  &&  str->get_overtaking_mode() <= oneway_mode  ){
			vehicle_base_t* v = NULL;
			if(  cnv->get_lane_affinity() == 1  ||  (v = other_lane_blocked())!=NULL  ||  str->is_reserved_by_others(this, false, pos_prev, pos_next)  ){
				//lane change denied
				cnv->set_tiles_overtaking(3);
				if(  cnv->is_requested_change_lane()  ||  cnv->get_lane_affinity() == -1  ) {
					//request the blocking convoi to reduce speed.
					if(  v  ) {
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
							if(  abs(cnv->get_speed_limit() - car->get_convoi()->get_speed_limit()) < kmh_to_speed(5)  ) {
								car->get_convoi()->yield_lane_space();
							}
						}
						else if(  private_car_t* pcar = dynamic_cast<private_car_t*>(v)  ) {
							if(  abs(get_speed_limit() - pcar->get_speed_limit()) < kmh_to_speed(5)  ) {
								pcar->yield_lane_space();
							}
						}
					}
					else {
						// perhaps this vehicle is in lane fixing.
						cnv->set_requested_change_lane(false);
					}
				}
			}
			else {
				//lane change accepted
				cnv->set_requested_change_lane(false);
			}
		}
		if(  cnv->get_yielding_quit_index() <= route_index  ) {
			cnv->quit_yielding_lane();
		}
		if(  str->get_overtaking_mode() == inverted_mode  ) {
			cnv->set_tiles_overtaking(1);
		}
		cnv->set_next_cross_lane(false); // since this convoi moved...
		// If there is one-way sign, calc lane_affinity. This should not be calculated in can_enter_tile().
		if(  roadsign_t* rs = gr->find<roadsign_t>()  ) {
			if(  rs->get_desc()->is_single_way()  ) {
				if(  cnv->calc_lane_affinity(rs->get_lane_affinity())  ) {
					// write debug code here.
				}
			}
		}
		if(  cnv->get_lane_affinity_end_index() == route_index  ) {
			cnv->reset_lane_affinity();
		}
		// If this tile is two-way ~ prohibited and the previous tile is oneway, the convoy have to move on traffic lane. Safety is confirmed in can_enter_tile().
		if(  pos_prev!=koord3d::invalid  ) {
			grund_t* prev_gr = welt->lookup(pos_prev);
			if(  prev_gr  ){
				strasse_t* prev_str = (strasse_t*)prev_gr->get_weg(road_wt);
				if(  (prev_str  &&  (prev_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode()<inverted_mode))  ||  str->get_overtaking_mode()==prohibited_mode  ){
					cnv->set_tiles_overtaking(0);
				}
			}
		}
		pos_prev = gr->get_pos();
	}
}

void road_vehicle_t::leave_tile() {
	vehicle_t::leave_tile();
	// unreserve the tile
	grund_t* gr = welt->lookup(get_pos());
	strasse_t* str = gr ? (strasse_t*)(gr->get_weg(road_wt)) : NULL;
	if(  str  ) {
		str->unreserve(this);
		reserving_tiles.remove(gr->get_pos());
	}
}


schedule_t * road_vehicle_t::generate_new_schedule() const
{
  return new truck_schedule_t();
}


void road_vehicle_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("road_vehicle_t::set_convoi()","%p",c);
	if(c!=NULL) {
		bool target=(bool)cnv;	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
		vehicle_t::set_convoi(c);
		if(target  &&  leading  &&  c->get_route()->empty()) {
			// reinitialize the target halt
			const route_t *rt = cnv->get_route();
			target_halt = haltestelle_t::get_halt( rt->back(), get_owner() );
			if(  target_halt.is_bound()  ) {
				for(  uint32 i=0;  i<c->get_tile_length()  &&  i+1<rt->get_count();  i++  ) {
					target_halt->reserve_position( welt->lookup( rt->at(rt->get_count()-i-1) ), cnv->self );
				}
			}
		}
	}
	else {
		if(  cnv  &&  leading  &&  target_halt.is_bound()  ) {
			// now reserve our choice (beware: might be longer than one tile!)
			for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<cnv->get_route()->get_count();  length++  ) {
				target_halt->unreserve_position( welt->lookup( cnv->get_route()->at( cnv->get_route()->get_count()-length-1) ), cnv->self );
			}
			target_halt = halthandle_t();
		}
		cnv = NULL;
	}
}

// To prevent glitch
void road_vehicle_t::refresh() {
	int xpos=0, ypos=0;
	get_screen_offset( xpos, ypos, get_tile_raster_width(), true );
	viewport_t *vp = welt->get_viewport();
	scr_coord scr_pos = vp->get_screen_coord(get_pos(), koord(get_xoff(), get_yoff()));
	display_mark_img_dirty( image, scr_pos.x + xpos, scr_pos.y + ypos);
	if(  !get_flag(obj_t::dirty)  ) {
		set_flag( obj_t::dirty );
	}
}

void road_vehicle_t::unreserve_all_tiles() {
	for(uint32 i=0; i<reserving_tiles.get_count(); i++) {
		grund_t* gr = welt->lookup(reserving_tiles[i]);
		if(  gr  ) {
			strasse_t* str = (strasse_t*) (gr->get_weg(road_wt));
			if(  str  ) {
				str->unreserve(this);
			}
		}
	}
	reserving_tiles.clear();
}

road_vehicle_t::~road_vehicle_t() {
	// unreserve tiles before this vehicle is disposed.
	unreserve_all_tiles();
}


/* from now on rail vehicles (and other vehicles using blocks) */
rail_vehicle_t::rail_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : vehicle_t()
{
	vehicle_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_first) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL) {
			int power = (is_first || fracht.empty() || fracht.front() == goods_manager_t::none) ? 500 : 0;
			const goods_desc_t* w = fracht.empty() ? goods_manager_t::none : fracht.front().get_desc();
			dbg->warning("rail_vehicle_t::rail_vehicle_t()","try to find a fitting vehicle for %s.", power>0 ? "engine": w->get_name() );
			if(last_desc!=NULL  &&  last_desc->can_follow(last_desc)  &&  last_desc->get_freight_type()==w  &&  (!is_last  ||  last_desc->get_trailer(0)==NULL)) {
				// same as previously ...
				desc = last_desc;
			}
			else {
				// we have to search
				desc = vehicle_builder_t::get_best_matching(get_waytype(), 0, w!=goods_manager_t::none?5000:0, power, speed_to_kmh(speed_limit), w, false, last_desc, is_last );
			}
			if(desc) {
DBG_MESSAGE("rail_vehicle_t::rail_vehicle_t()","replaced by %s",desc->get_name());
				calc_image();
			}
			else {
				dbg->error("rail_vehicle_t::rail_vehicle_t()","no matching desc found for %s!",w->get_name());
			}
			if (!fracht.empty() && fracht.front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht.remove_first();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
}


rail_vehicle_t::rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player_, convoi_t* cn) :
	vehicle_t(pos, desc, player_)
{
    cnv = cn;
}


rail_vehicle_t::~rail_vehicle_t()
{
	if (cnv && leading) {
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


void rail_vehicle_t::set_convoi(convoi_t *c)
{
	if(c!=cnv) {
		DBG_MESSAGE("rail_vehicle_t::set_convoi()","new=%p old=%p",c,cnv);
		if(leading) {
			if(cnv!=NULL  &&  cnv!=(convoi_t *)1) {
				// free route from old convoi
				route_t const& r = *cnv->get_route();
				if(  !r.empty()  &&  route_index + 1U < r.get_count() - 1  ) {
					uint16 dummy;
					block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, 100000, false, false);
					target_halt = halthandle_t();
				}
			}
			else if(  c->get_next_reservation_index()==0  &&  c->is_reservation_empty()  ) {
				assert(c!=NULL);
				// eventually search new route
				route_t const& r = *c->get_route();
				if(  (r.get_count()<=route_index  ||  r.empty()  ||  get_pos()==r.back())  &&  c->get_state()!=convoi_t::INITIAL  &&  c->get_state()!=convoi_t::LOADING  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  ) {
					check_for_finish = true;
					dbg->warning("rail_vehicle_t::set_convoi()", "convoi %i had a too high route index! (%i of max %i)", c->self.get_id(), route_index, r.get_count() - 1);
				}
				// set default next stop index
				c->set_next_stop_index( max(route_index,1)-1 );
				// need to reserve new route?
				if(  !check_for_finish  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  &&  (c->get_state()==convoi_t::DRIVING  ||  c->get_state()>=convoi_t::LEAVING_DEPOT)  ) {
					sint32 num_index = cnv==(convoi_t *)1 ? 1001 : 0; 	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
					uint16 next_signal, next_crossing;
					cnv = c;
					if(  block_reserver(&r, max(route_index,1)-1, next_signal, next_crossing, num_index, true, false)  ) {
						c->set_next_stop_index( next_signal>next_crossing ? next_crossing : next_signal );
					}
				}
			}
		}
		vehicle_t::set_convoi(c);
	}
}


// need to reset halt reservation (if there was one)
bool rail_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
	cnv->set_next_reservation_index( 0 );	// nothing to reserve
	target_halt = halthandle_t();	// no block reserved
	// use length 8888 tiles to advance to the end of all stations
	uint16 len = welt->get_settings().get_advance_to_end() ? 8888 : cnv->get_tile_length();
	return route->calc_route(welt, start, ziel, this, max_speed, len);
}


bool rail_vehicle_t::check_next_tile(const grund_t *bd) const
{
	schiene_t const* const sch = obj_cast<schiene_t>(bd->get_weg(get_waytype()));
	if(  !sch  ) {
		return false;
	}

	// Hajo: diesel and steam engines can use electrified track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : desc->get_engine_type()==vehicle_desc_t::electric);
	if(  (!needs_no_electric  &&  !sch->is_electrified())  ||  sch->get_max_speed() == 0  ) {
		return false;
	}

	if (depot_t *depot = bd->get_depot()) {
		if (depot->get_waytype() != desc->get_waytype()  ||  depot->get_owner() != get_owner()) {
			return false;
		}
	}
	// now check for special signs
	if(sch->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(  rs->get_desc()->get_wtyp()==get_waytype()  ) {
			if(  cnv != NULL  &&  rs->get_desc()->get_min_speed() > 0  &&  rs->get_desc()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// private road
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
			if(  rs->get_desc()->get_wtyp()==get_waytype()  ) {
				if(  rs->get_desc()->get_flags() & roadsign_desc_t::END_OF_CHOOSE_AREA  ) {
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
int rail_vehicle_t::get_cost(const grund_t *gr, const weg_t *w, const sint32 max_speed, ribi_t::ribi from) const
{
	// first favor faster ways
	if(  w==NULL  ) {
		// only occurs when deletion during way search
		return 999;
	}

	// add cost for going (with maximum speed, cost is 1)
	const sint32 max_tile_speed = w->get_max_speed();
	int costs = (max_speed<=max_tile_speed) ? 1 : 4-(3*max_tile_speed)/max_speed;

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		// 25 hardcoded, see get_cost_upslope()
		costs += 25 * get_sloping_upwards( gr->get_weg_hang(), from );
	}

	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool rail_vehicle_t::is_target(const grund_t *gr,const grund_t *prev_gr) const
{
	const schiene_t * sch1 = (const schiene_t *) gr->get_weg(get_waytype());
	// first check blocks, if we can go there
	if(  sch1->can_reserve(cnv->self)  ) {
		//  just check, if we reached a free stop position of this halt
		if(  gr->is_halt()  &&  gr->get_halt()==target_halt  ) {
			// now we must check the predecessor ...
			if(  prev_gr!=NULL  ) {
				const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
				const ribi_t::ribi ribi = ribi_type(dir);
				if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ) {
					// signal/one way sign wrong direction
					return false;
				}
				grund_t *to;
				if(  !gr->get_neighbour(to,get_waytype(),ribi)  ||  !(to->get_halt()==target_halt)  ||  (to->get_weg(get_waytype())->get_ribi_maske() & ribi_type(dir))!=0  ) {
					// end of stop: Is it long enough?
					// end of stop could be also signal!
					uint16 tiles = cnv->get_tile_length();
					while(  tiles>1  ) {
						if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ||  !gr->get_neighbour(to,get_waytype(),ribi_t::backward(ribi))  ||  !(to->get_halt()==target_halt)  ) {
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

bool rail_vehicle_t::check_longblock_signal(signal_t *sig, uint16 next_block, sint32 &restart_speed) {
	// longblock signal: first check, whether there is a signal coming up on the route => just like normal signal
	uint16 next_signal, next_crossing;
	if(  !block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false, true )  ) {
		// not even the "Normal" signal route part is free => no bother checking further on
		sig->set_state( roadsign_t::rot );
		restart_speed = 0;
		return false;
	}

	if(  next_signal != INVALID_INDEX  ) {
		// success, and there is a signal before end of route => finished
		sig->set_state( roadsign_t::gruen );
		cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		return true;
	}
	
	// now we have to maintain reservation with reserved_tiles, that is slower than using next_reservation_index
	// copy all tiles that are already reserved
	bool add_pos = false;
	vector_tpl<koord3d> tiles_convoy_on;
	for(  uint16 i=0;  i<cnv->get_vehicle_count();  i++  ) {
		tiles_convoy_on.append_unique(cnv->get_vehikel(i)->get_pos());
	}
	for(  uint16 i=0;  i<next_block+1  &&  i<cnv->get_route()->get_count();  i++  ) {
		if(  !add_pos  &&  tiles_convoy_on.is_contained(cnv->get_route()->at(i))  ) {
			add_pos = true;
		}
		if(  add_pos  ) {
			cnv->reserve_pos(cnv->get_route()->at(i));
		}
	}

	// now we can use the route search array
	// (route until end is already reserved at this point!)
	uint8 schedule_index = cnv->get_schedule()->get_current_stop()+1;
	route_t target_rt;
	koord3d cur_pos = cnv->get_route()->back();
	uint16 dummy, next_next_signal;
	if(schedule_index >= cnv->get_schedule()->get_count()) {
		schedule_index = 0;
	}
	while(  schedule_index != cnv->get_schedule()->get_current_stop()  ) {
		// now search
		// search for route
		uint16 len = welt->get_settings().get_advance_to_end() ? 8888 : cnv->get_tile_length();
		bool success = target_rt.calc_route( welt, cur_pos, cnv->get_schedule()->entries[schedule_index].pos, this, speed_to_kmh(cnv->get_min_top_speed()), len );
		if(  target_rt.is_contained(get_pos())  ) {
			// do not reserve route going through my current stop&
			break;
		}
		if(  success  ) {
			success = block_reserver( &target_rt, 1, next_next_signal, dummy, 0, true, false, true );
		}

		if(  success  ) {
			// ok, would be free
			if(  next_next_signal<target_rt.get_count()  ) {
				// and here is a signal => finished
				sig->set_state( roadsign_t::gruen );
				// we stop at the end of the route.
				cnv->set_next_stop_index( min( min( next_crossing, next_signal ), cnv->get_route()->get_count()-1 ) );
				return true;
			}
		}

		if(  !success  ) {
			// unreserve tiles which we reserved in this routine.
			// find the index from which we unreserve the tiles.
			sint32 start_idx;
			for(  start_idx=0;  start_idx<(sint32)cnv->get_reserved_tiles().get_count()  &&  cnv->get_reserved_tiles()[start_idx]!=cnv->get_route()->at(next_block+1);  start_idx++  );
			// tiles on which this convoy is must not be unreserved.
			vector_tpl<koord3d> tiles_convoy_on;
			for(  uint16 i=0;  i<cnv->get_vehicle_count();  i++  ) {
				tiles_convoy_on.append_unique(cnv->get_vehikel(i)->get_pos());
			}
			// now we unreserve the tiles
			for(  sint32 i=cnv->get_reserved_tiles().get_count()-1;  i>=start_idx;  i--  ) {
				grund_t* gr = welt->lookup(cnv->get_reserved_tiles()[i]);
				schiene_t* sch1 = gr ? (schiene_t*)gr->get_weg(get_waytype()) : NULL;
				if(  sch1  &&  !tiles_convoy_on.is_contained(gr->get_pos())  ) {
					sch1->unreserve(cnv->self);
				}
				cnv->get_reserved_tiles().remove_at(i);
			}
			sig->set_state( roadsign_t::rot );
			restart_speed = 0;
			return false;
		}
		// prepare for next leg of schedule
		cur_pos = target_rt.back();
		schedule_index ++;
		if(schedule_index >= cnv->get_schedule()->get_count()) {
			schedule_index = 0;
		}
	}
	if(  cnv->get_next_stop_index()-1 <= route_index  ) {
		cnv->set_next_stop_index( cnv->get_route()->get_count()-1 );
	}
	return true;
}

bool rail_vehicle_t::is_longblock_signal_clear(signal_t *sig, uint16 next_block, sint32 &restart_speed)
{
	if(  cnv->is_waiting()  ) {
		// we are in a step. do that.
		const bool res = check_longblock_signal(sig, next_block, restart_speed);
		cnv->set_longblock_signal_judge_request_invalid();
		return res;
	}
	else {
		// we are in a sync_step. request to do this in a step.
		cnv->request_longblock_signal_judge(sig, next_block);
		restart_speed = 0; 
		return false;
	}
}


bool rail_vehicle_t::is_choose_signal_clear(signal_t *sig, const uint16 start_block, sint32 &restart_speed)
{
	bool choose_ok = false;
	target_halt = halthandle_t();

	uint16 next_signal, next_crossing;
	grund_t const* const target = welt->lookup(cnv->get_route()->back());

	if(  cnv->get_schedule_target()!=koord3d::invalid  ) {
		// destination is a waypoint!
		goto skip_choose;
	}

	if(  target==NULL  ) {
		cnv->suche_neue_route();
		return false;
	}

	// first check, if we are not heading to a waypoint
	if(  !target->get_halt().is_bound()  ) {
		goto skip_choose;
	}

	// now we might choose something at least
	choose_ok = true;

	// check, if there is another choose signal or end_of_choose on the route
	for(  uint32 idx=start_block+1;  choose_ok  &&  idx<cnv->get_route()->get_count();  idx++  ) {
		grund_t *gr = welt->lookup(cnv->get_route()->at(idx));
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
			if(  rs  &&  rs->get_desc()->get_wtyp()==get_waytype()  ) {
				if(  rs->get_desc()->get_flags() & roadsign_desc_t::END_OF_CHOOSE_AREA  ) {
					// end of choose on route => not choosing here
					choose_ok = false;
				}
			}
		}
		if(  way->has_signal()  ) {
			signal_t *sig = gr->find<signal_t>(1);
			if(  sig  &&  sig->get_desc()->is_choose_sign()  ) {
				// second choose signal on route => not choosing here
				choose_ok = false;
			}
		}
	}

skip_choose:
	if(  !choose_ok  ) {
		// just act as normal signal
		if(  block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 0, true, false )  ) {
			sig->set_state(  roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_state(  roadsign_t::rot );
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
		const int richtung = ribi_type(get_pos(), pos_next);	// to avoid confusion at diagonals
		if(  !target_rt.find_route( welt, cnv->get_route()->at(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, welt->get_settings().get_max_choose_route_steps() )  ) {
			// nothing empty or not route with less than get_max_choose_route_steps() tiles
			target_halt = halthandle_t();
			sig->set_state(  roadsign_t::rot );
			restart_speed = 0;
			return false;
		}
		else {
			// try to alloc the whole route
			cnv->access_route()->remove_koord_from(start_block);
			cnv->access_route()->append( &target_rt );
			if(  !block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 100000, true, false )  ) {
				dbg->error( "rail_vehicle_t::is_choose_signal_clear()", "could not reserved route after find_route!" );
				target_halt = halthandle_t();
				sig->set_state(  roadsign_t::rot );
				restart_speed = 0;
				return false;
			}
		}
		// reserved route to target
	}
	sig->set_state(  roadsign_t::gruen );
	cnv->set_next_stop_index( min( next_crossing, next_signal ) );
	return true;
}


bool rail_vehicle_t::is_pre_signal_clear(signal_t *sig, uint16 next_block, sint32 &restart_speed)
{
	// parse to next signal; if needed recurse, since we allow cascading
	uint16 next_signal, next_crossing;
	if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		if(next_signal == INVALID_INDEX ||
           cnv->get_route()->at(next_signal) == cnv->get_route()->back() ||
           is_signal_clear( next_signal, restart_speed )) {
			// ok, end of route => we can go
			sig->set_state( roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_signal, next_crossing ) );
			return true;
		}
		// when we reached here, the way is apparently not free => release reservation and set state to next free
		sig->set_state( roadsign_t::naechste_rot );
		block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, false, false );
		restart_speed = 0;
		return false;
	}
	// if we end up here, there was not even the next block free
	sig->set_state( roadsign_t::rot );
	restart_speed = 0;
	return false;
}



bool rail_vehicle_t::is_priority_signal_clear(signal_t *sig, uint16 next_block, sint32 &restart_speed)
{
	// parse to next signal; if needed recurse, since we allow cascading
	uint16 next_signal, next_crossing;

	if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		if(  next_signal == INVALID_INDEX  ||  cnv->get_route()->at(next_signal) == cnv->get_route()->back()  ||  is_signal_clear( next_signal, restart_speed )  ) {
			// ok, end of route => we can go
			sig->set_state( roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_signal, next_crossing ) );

			return true;
		}

		// when we reached here, the way after the last signal is not free though the way before is => we can still go
		if(  cnv->get_next_stop_index()<=next_signal+1  ) {
			// only show third aspect on last signal of cascade
			sig->set_state( roadsign_t::naechste_rot );
		}
		else {
			sig->set_state( roadsign_t::gruen );
		}
		cnv->set_next_stop_index( min( next_signal, next_crossing ) );

		return false;
	}

	// if we end up here, there was not even the next block free
	sig->set_state( roadsign_t::rot );
	restart_speed = 0;

	return false;
}


bool rail_vehicle_t::is_signal_clear(uint16 next_block, sint32 &restart_speed)
{
	// called, when there is a signal; will call other signal routines if needed
	grund_t *gr_next_block = welt->lookup(cnv->get_route()->at(next_block));
	signal_t *sig = gr_next_block->find<signal_t>();
	if(  sig==NULL  ) {
		dbg->error( "rail_vehicle_t::is_signal_clear()", "called at %s without a signal!", cnv->get_route()->at(next_block).get_str() );
		return true;
	}

	// action depend on the next signal
	const roadsign_desc_t *sig_desc=sig->get_desc();

	// If the next signal is not a longblock signal, reservation must be handled by next_reservation_index.
	if(  !sig_desc->is_longblock_signal()  ) {
		cnv->clear_reserved_tiles();
	}

	// simple signal: drive on, if next block is free
	if(  !sig_desc->is_longblock_signal() &&
      !sig_desc->is_choose_sign() &&
      !sig_desc->is_pre_signal() &&
      !sig_desc->is_priority_signal()) {

		uint16 next_signal, next_crossing;
		if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
			sig->set_state(  roadsign_t::gruen );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_state(  roadsign_t::rot );
		restart_speed = 0;
		return false;
	}

	if(  sig_desc->is_pre_signal()  ) {
		return is_pre_signal_clear( sig, next_block, restart_speed );
	}

	if (  sig_desc->is_priority_signal()  ) {
 	return is_priority_signal_clear( sig, next_block, restart_speed );
 }

	if(  sig_desc->is_longblock_signal()  ) {
		return is_longblock_signal_clear( sig, next_block, restart_speed );
	}

	if(  sig_desc->is_choose_sign()  ) {
		return is_choose_signal_clear( sig, next_block, restart_speed );
	}

	dbg->error( "rail_vehicle_t::is_signal_clear()", "felt through at signal at %s", cnv->get_route()->at(next_block).get_str() );
	return false;
}


bool rail_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	assert(leading);
	uint16 next_signal, next_crossing;
	if(  cnv->get_state()==convoi_t::CAN_START  ||  cnv->get_state()==convoi_t::CAN_START_ONE_MONTH  ||  cnv->get_state()==convoi_t::CAN_START_TWO_MONTHS  ) {
		// reserve first block at the start until the next signal
		grund_t *gr_current = welt->lookup( get_pos() );
		weg_t *w = gr_current ? gr_current->get_weg(get_waytype()) : NULL;
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

	assert(gr);
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

	// is there any signal/crossing to be reserved?
	uint16 next_block = cnv->get_next_stop_index()-1;
	if(  next_block >= cnv->get_route()->get_count()  ) {
		// no obstacle in the way => drive on ...
		return true;
	}

	// signal disappeared, train passes the tile of former signal
	if(  next_block+1 < route_index  ) {
		// we need to reserve the next block even if there is no signal present anymore
		bool ok = block_reserver( cnv->get_route(), route_index, next_signal, next_crossing, 0, true, false );
		if (ok) {
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		}
		return ok;
		// if reservation was not possible the train will wait on the track until block is free
	}

	// reserve crossing in advance
	vector_tpl<std::pair< uint16, uint16> > crossing_reservation_index = cnv->get_crossing_reservation_index();
	for(  sint16 i=crossing_reservation_index.get_count()-1;  i>=0;  i--  ) {
		// check whether target index is valid
		if(  crossing_reservation_index[i].second >= cnv->get_route()->get_count()  ||  crossing_reservation_index[i].second <= route_index  ) {
			// this reservation index is invalid or already obsolete.
			cnv->remove_crossing_reservation_at(i);
			continue;
		}
		else if(  crossing_reservation_index[i].first <= route_index  &&  crossing_reservation_index[i].second > route_index  ) {
			grund_t* cr_gr = welt->lookup(cnv->get_route()->at(crossing_reservation_index[i].second));
			crossing_t* cr = cr_gr ? cr_gr->find<crossing_t>(2) : NULL;
			if(  cr  ) {
				cr->request_crossing(this);
			}
			cnv->remove_crossing_reservation_at(i);
		}
	}

	if(  next_block <= route_index+3  ) {
		koord3d block_pos=cnv->get_route()->at(next_block);
		grund_t *gr_next_block = welt->lookup(block_pos);
		const schiene_t *sch1 = gr_next_block ? (const schiene_t *)gr_next_block->get_weg(get_waytype()) : NULL;
		if(sch1==NULL) {
			// way (weg) not existent (likely destroyed)
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
						dbg->error( "rail_vehicle_t::can_enter_tile()", "block not free but was reserved!" );
						return false;
					}
					cnv->set_next_stop_index( next_crossing<next_signal ? next_crossing : next_signal );
				}
			}
		}

		// next check for signal
		if(  sch1->has_signal()  ) {
			if(  !is_signal_clear( next_block, restart_speed )  ) {
				// only return false, if we are directly in front of the signal
				return cnv->get_next_stop_index()>route_index;
			}
		}
	}
	return true;
}


/*
 * reserves or un-reserves all blocks and returns the handle to the next block (if there)
 * if count is larger than 1, (and defined) maximum MAX_CHOOSE_BLOCK_TILES tiles will be checked
 * (freeing or reserving a choose signal path)
 * if (!reserve && force_unreserve) then un-reserve everything till the end of the route
 * return the last checked block
 * @author prissi
 */
bool rail_vehicle_t::block_reserver(const route_t *route, uint16 start_index, uint16 &next_signal_index, uint16 &next_crossing_index, int count, bool reserve, bool force_unreserve, bool use_vector  ) const
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

	if(route->at(start_index)==get_pos()  &&  reserve) {
		start_index++;
	}

	if(  !reserve  ) {
		cnv->set_next_reservation_index( start_index );
	}

	// find next block segment en route
	uint16 i=start_index;
	next_signal_index=INVALID_INDEX;
	next_crossing_index=INVALID_INDEX;
	bool unreserve_now = false;
	for ( ; success  &&  count>=0  &&  i<route->get_count(); i++) {

		koord3d pos = route->at(i);
		grund_t *gr = welt->lookup(pos);
		schiene_t * sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;
		if(sch1==NULL  &&  reserve) {
			// reserve until the end of track
			break;
		}
		// we un-reserve also nonexistent tiles! (may happen during deletion)

#ifdef MAX_CHOOSE_BLOCK_TILES
		max_tiles--;
		if(max_tiles<0  &&   count>1) {
			break;
		}
#endif
		if(reserve) {
			if(  sch1->has_signal()  &&  i<route->get_count()-1  ) {
				if(count) {
					signs.append(gr);
				}
				count --;
				next_signal_index = i;
			}
			if(  !sch1->reserve( cnv->self, ribi_type( route->at(max(1u,i)-1u), route->at(min(route->get_count()-1u,i+1u)) ) )  ) {
				success = false;
			}
			else if(  use_vector  ){
				// use reserved_tiles instead of next_reservation_index to hold reservations.
				cnv->reserve_pos(pos);
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
				// un-reserve from here (used during sale, since there might be reserved tiles not freed)
				unreserve_now = !force_unreserve;
				// If reservation is controlled by next_reservation_index, this does nothing.
				cnv->unreserve_pos(pos);
			}
			if(sch1->has_signal()) {
				signal_t* signal = gr->find<signal_t>();
				if(signal) {
					signal->set_state(roadsign_t::rot);
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

//DBG_MESSAGE("block_reserver()","signals at %i, success=%d",next_signal_index,success);

	// free, in case of un-reserve or no success in reservation
	if(!success) {
		// free reservation
		for ( int j=start_index; j<i; j++) {
			schiene_t * sch1 = (schiene_t *)welt->lookup( route->at(j))->get_weg(get_waytype());
			sch1->unreserve(cnv->self);
			cnv->unreserve_pos(route->at(j));
		}
		cnv->set_next_reservation_index( start_index );
		return false;
	}

	// ok, switch everything green ...
	FOR(slist_tpl<grund_t*>, const g, signs) {
		if (signal_t* const signal = g->find<signal_t>()) {
			signal->set_state(roadsign_t::gruen);
		}
	}
	cnv->set_next_reservation_index( i );

	return true;
}


/* beware: we must un-reserve rail blocks... */
void rail_vehicle_t::leave_tile()
{
	vehicle_t::leave_tile();
	// fix counters
	if(last) {
		grund_t *gr = welt->lookup( get_pos() );
		if(gr) {
			schiene_t *sch0 = (schiene_t *) gr->get_weg(get_waytype());
			if(sch0) {
				sch0->unreserve(this);
				if(  cnv  ) {
					// If reservation is controlled by next_reservation_index, this does nothing.
					cnv->unreserve_pos(get_pos());
				}
				// tell next signal?
				// and switch to red
				if(sch0->has_signal()) {
					signal_t* sig = gr->find<signal_t>();
					if(sig) {
						sig->set_state(roadsign_t::rot);
					}
				}
			}
		}
	}
}


void rail_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);

	if(  schiene_t *sch0 = (schiene_t *) gr->get_weg(get_waytype())  ) {
		// way statistics
		const int cargo = get_total_cargo();
		sch0->book(cargo, WAY_STAT_GOODS);
		if(leading) {
			sch0->book(1, WAY_STAT_CONVOIS);
			sch0->reserve( cnv->self, get_direction() );
		}
	}
}


schedule_t * rail_vehicle_t::generate_new_schedule() const
{
	return desc->get_waytype()==tram_wt ? new tram_schedule_t() : new train_schedule_t();
}


schedule_t * monorail_vehicle_t::generate_new_schedule() const
{
	return new monorail_schedule_t();
}


schedule_t * maglev_vehicle_t::generate_new_schedule() const
{
	return new maglev_schedule_t();
}


schedule_t * narrowgauge_vehicle_t::generate_new_schedule() const
{
	return new narrowgauge_schedule_t();
}


water_vehicle_t::water_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player_, convoi_t* cn) :
	vehicle_t(pos, desc, player_)
{
	cnv = cn;
}


water_vehicle_t::water_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : vehicle_t()
{
	vehicle_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_first) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL) {
			dbg->warning("water_behicle_t::water_behicle_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passengers");
			desc = vehicle_builder_t::get_best_matching(water_wt, 0, fracht.empty() ? 0 : 30, 100, 40, !fracht.empty() ? fracht.front().get_desc() : goods_manager_t::passengers, true, last_desc, is_last );
			if(desc) {
				calc_image();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
}


void water_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);

	if(  weg_t *ch = gr->get_weg(water_wt)  ) {
		// we are in a channel, so book statistics
		ch->book(get_total_cargo(), WAY_STAT_GOODS);
		if (leading)  {
			ch->book(1, WAY_STAT_CONVOIS);
		}
	}
}


bool water_vehicle_t::check_next_tile(const grund_t *bd) const
{
	if(  bd->is_water()  ) {
		return true;
	}
	// channel can have more stuff to check
	const weg_t *w = bd->get_weg(water_wt);
#if ENABLE_WATERWAY_SIGNS
	if(  w  &&  w->has_sign()  ) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(  rs->get_desc()->get_wtyp()==get_waytype()  ) {
			if(  cnv !=NULL  &&  rs->get_desc()->get_min_speed() > 0  &&  rs->get_desc()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// private road
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
void water_vehicle_t::calc_friction(const grund_t *gr)
{
	// or a hill?
	if(gr->get_weg_hang()) {
		// hill up or down => in lock => decelerate
		current_friction = 16;
	}
	else {
		// flat track
		current_friction = 1;
	}

	if(previous_direction != direction) {
		// curve: higher friction
		current_friction *= 2;
	}
}


bool water_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	restart_speed = -1;

	if(leading) {

		assert(gr);
		if(  gr->get_top()>251  ) {
			// too many ships already here ..
			return false;
		}
		weg_t *w = gr->get_weg(water_wt);
		if(w  &&  w->is_crossing()) {
			// ok, here is a draw/turn-bridge ...
			crossing_t* cr = gr->find<crossing_t>();
			if(!cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}


schedule_t * water_vehicle_t::generate_new_schedule() const
{
  return new ship_schedule_t();
}


/**** from here on planes ***/


// for flying things, everywhere is good ...
// another function only called during route searching
ribi_t::ribi air_vehicle_t::get_ribi(const grund_t *gr) const
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
			return ribi_t::none;
		}

		case landing:
		case departing:
		{
			ribi_t::ribi dir = gr->get_weg_ribi(air_wt);
			if(dir==0) {
				return ribi_t::all;
			}
			return dir;
		}

		case flying:
		case circling:
			return ribi_t::all;
	}
	return ribi_t::none;
}


// how expensive to go here (for way search)
// author prissi
int air_vehicle_t::get_cost(const grund_t *, const weg_t *w, const sint32, ribi_t::ribi) const
{
	// first favor faster ways
	int costs = 0;

	if(state==flying) {
		if(w==NULL) {
			costs += 1;
		}
		else {
			if(w->get_desc()->get_styp()==type_flat) {
				costs += 25;
			}
		}
	}
	else {
		// only, if not flying ...
		const runway_t *rw =(const runway_t *)w;
		// if we are on a runway, then take into account how many convois are already going there
		if(  rw->get_desc()->get_styp()==1  ) {
			costs += rw->get_reservation_count()*9;	// encourage detours even during take off
		}
		if(w->get_desc()->get_styp()==type_flat) {
			costs += 3;
		}
		else {
			costs += 2;
		}
	}

	return costs;
}


// whether the ground is drivable or not depends on the current state of the airplane
bool air_vehicle_t::check_next_tile(const grund_t *bd) const
{
	switch (state) {
		case taxiing:
		case taxiing_to_halt:
		case looking_for_parking:
//DBG_MESSAGE("check_next_tile()","at %i,%i",bd->get_pos().x,bd->get_pos().y);
			return (bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_max_speed()>0);

		case landing:
		case departing:
		case flying:
		case circling:
		{
//DBG_MESSAGE("air_vehicle_t::check_next_tile()","(cnv %i) in idx %i",cnv->self.get_id(),route_index );
			// prissi: here a height check could avoid too high mountains
			return true;
		}
	}
	return false;
}


// this routine is called by find_route, to determined if we reached a destination
bool air_vehicle_t::is_target(const grund_t *gr,const grund_t *) const
{
	if(state!=looking_for_parking  ||  !target_halt.is_bound()) {
		// search for the end of the runway
		const weg_t *w=gr->get_weg(air_wt);
		if(w  &&  w->get_desc()->get_styp()==type_runway) {
			// ok here is a runway
			ribi_t::ribi ribi= w->get_ribi_unmasked();
			if(ribi_t::is_single(ribi)  &&  (ribi&approach_dir)!=0) {
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
bool air_vehicle_t::find_route_to_stop_position()
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
	grund_t const* const target = welt->lookup(rt->at(searchforstop));
	if(target==NULL  ||  !target->hat_weg(air_wt)) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no runway found at (%s)",rt->at(searchforstop).get_str());
		return true;	// no runway any more ...
	}

	// is our target occupied?
//	DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","state %i",state);
	if(!target_halt->find_free_position(air_wt,cnv->self,obj_t::air_vehicle)  ) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no free position found!");
		return false;
	}
	else {
		// calculate route to free position:

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not re-entrant!
		if(!cnv->is_waiting()) {
			target_halt = halthandle_t();
			return false;
		}

		// now search a route
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","some free: find route from index %i",suchen);
		route_t target_rt;
		flight_state prev_state = state;
		state = looking_for_parking;
		if(!target_rt.find_route( welt, rt->at(searchforstop), this, 500, ribi_t::all, welt->get_settings().get_max_choose_route_steps() )) {
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
		rt->remove_koord_from(searchforstop);
		rt->append( &target_rt );
		return true;
	}
}


// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
bool air_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route)
{
//DBG_MESSAGE("aircraft_t::calc_route()","search route from %i,%i,%i to %i,%i,%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	if(leading  &&  cnv) {
		// free target reservation
		if(  target_halt.is_bound() ) {
			if (grund_t* const target = welt->lookup(cnv->get_route()->back())) {
				target_halt->unreserve_position(target,cnv->self);
			}
		}
		// free runway reservation
		block_reserver( route_index, route->get_count(), false );
	}
	target_halt = halthandle_t();	// no block reserved

	const weg_t *w=welt->lookup(start)->get_weg(air_wt);
	bool start_in_the_air = (w==NULL);
	bool end_in_air=false;

	searchforstop = takeoff = touchdown = 0x7ffffffful;
	if(!start_in_the_air) {

		// see, if we find a direct route: We are finished
		state = taxiing;
		if(route->calc_route( welt, start, ziel, this, max_speed, 0 )) {
			// ok, we can taxi to our location
			return true;
		}
	}

	if(start_in_the_air  ||  (w->get_desc()->get_styp()==type_runway  &&  ribi_t::is_single(w->get_ribi())) ) {
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
		approach_dir = ribi_t::northeast;	// reverse
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near (%s)",start.get_str());
#endif
		if(!route->find_route( welt, start, this, max_speed, ribi_t::all, 100 )) {
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
	approach_dir = ribi_t::southwest;	// reverse
	//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
#endif
	route_t end_route;

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::all, welt->get_settings().get_max_choose_route_steps() )) {
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
			ribi_t::ribi start_ribi = ribi_t::backward(ribi_type(start_dir));
			const grund_t *gr=NULL;
			// add the start
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->is_within_limits(search_start.get_2d()+(start_dir*endi)) ) {
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
		flying_height = 0;
		target_height = (sint16)get_pos().z*TILE_HEIGHT_STEP;
	}
	else {
		// init with current pos (in air ... )
		route->clear();
		route->append( start );
		state = flying;
		if(flying_height==0) {
			flying_height = 3*TILE_HEIGHT_STEP;
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
		end_ribi = ribi_t::backward(end_ribi);
		if(end_dir!=koord(0,0)) {
			// add the start
			const grund_t *gr;
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->is_within_limits(search_end.get_2d()+(end_dir*endi)) ) {
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
		searchforstop = touchdown = 0x7FFFFFFFul;
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
			case ribi_t::north: offset = 0; break;
			case ribi_t::west: offset = 4; break;
			case ribi_t::south: offset = 8; break;
			case ribi_t::east: offset = 12; break;
		}

		// now make a curve
		koord circlepos=landing_start.get_2d();
		static const koord circle_koord[16]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		for(  int  i=0;  i<16;  i++  ) {
			circlepos += circle_koord[(offset+i+16)%16];
			if(welt->is_within_limits(circlepos)) {
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

		// now the route reach point (+1, since it will check before entering the tile ...)
		searchforstop = route->get_count()-1;

		// now we just append the rest
		for( int i=end_route.get_count()-2;  i>=0;  i--  ) {
			route->append(end_route.at(i));
		}
	}

//DBG_MESSAGE("aircraft_t::calc_route()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route->get_count()-1, state );
	return true;
}


/* reserves runways (reserve true) or removes the reservation
 * finishes when reaching end tile or leaving the ground (end of runway)
 * @return true if the reservation is successful
 */
bool air_vehicle_t::block_reserver( uint32 start, uint32 end, bool reserve ) const
{
	bool start_now = false;
	bool success = true;

	const route_t *route = cnv->get_route();
	if(route->empty()) {
		return false;
	}

	for(  uint32 i=start;  success  &&  i<end  &&  i<route->get_count();  i++) {

		grund_t *gr = welt->lookup(route->at(i));
		runway_t *sch1 = gr ? (runway_t *)gr->get_weg(air_wt) : NULL;
		if(  !sch1  ) {
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
			// we un-reserve also nonexistent tiles! (may happen during deletion)
			if(reserve) {
				start_now = true;
				sch1->add_convoi_reservation(cnv->self);
				if(  !sch1->reserve(cnv->self,ribi_t::none)  ) {
					// unsuccessful => must un-reserve all
					success = false;
					end = i;
					break;
				}
				// end of runway?
				if(  i > start  &&  (ribi_t::is_single( sch1->get_ribi_unmasked() )  ||  sch1->get_desc()->get_styp() != type_runway)   ) {
					end = i;
					break;
				}
			}
			else {
				// we always unreserve everything
				sch1->unreserve(cnv->self);
			}
		}
	}

	// un-reserve if not successful
	if(  !success  &&  reserve  ) {
		for(  uint32 i=start;  i<end;  i++  ) {
			grund_t *gr = welt->lookup(route->at(i));
			if (gr) {
				runway_t* sch1 = (runway_t *)gr->get_weg(air_wt);
				if (sch1) {
					sch1->unreserve(cnv->self);
				}
			}
		}
		return false;
	}

	if(  reserve  &&  end<touchdown  ) {
		// reserve runway for landing for load balancing
		for(  uint32 i=touchdown;  i<route->get_count();  i++  ) {
			if(  grund_t *gr = welt->lookup(route->at(i))  ) {
				if(  runway_t* sch1 = (runway_t *)gr->get_weg(air_wt)  ) {
					if(  sch1->get_desc()->get_styp()!=type_runway  ) {
						break;
					}
					sch1->add_convoi_reservation( cnv->self );
				}
			}
		}
	}

	return success;
}


// handles all the decisions on the ground an in the air
bool air_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	restart_speed = -1;

	assert(gr);
	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	if(  route_index < takeoff  &&  route_index > 1  &&  takeoff<cnv->get_route()->get_count()-1  ) {
		// check, if tile occupied by a plane on ground
		if(  route_index > 1  ) {
			for(  uint8 i = 1;  i<gr->get_top();  i++  ) {
				obj_t *obj = gr->obj_bei(i);
				if(  obj->get_typ()==obj_t::air_vehicle  &&  ((air_vehicle_t *)obj)->is_on_ground()  ) {
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
		if(rw->get_desc()->get_styp()==type_runway) {
			// try to reserve the runway
			if(!block_reserver(takeoff,takeoff+100,true)) {
				// runway already blocked ...
				restart_speed = 0;
				return false;
			}
		}
		return true;
	}

	if(  state == taxiing  ) {
		// enforce on ground for taxiing
		flying_height = 0;
		// we may need to unreserve the runway after leaving it
		if(  route_index >= touchdown  ) {
			runway_t *rw = (runway_t *)gr->get_weg(air_wt);
			// next tile a not runway => then unreserve
			if(  rw == NULL  ||  rw->get_desc()->get_styp() != type_runway  ||  gr->is_halt()  ) {
				block_reserver( touchdown, searchforstop+1, false );
			}
		}
	}

	if(  route_index == takeoff  &&  state == taxiing  ) {
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
		if(  !block_reserver( touchdown, searchforstop+1, true )  ) {
			route_index -= 16;
			return true;
		}
		state = landing;
		return true;
	}

	if(  route_index==touchdown-16-3  &&  state!=circling  ) {
		// just check, if the end of runway is free; we will wait there
		if(  block_reserver( touchdown, searchforstop+1, true )  ) {
			route_index += 16;
			// can land => set landing height
			state = landing;
		}
		else {
			// circle slowly next round
			state = circling;
			cnv->must_recalc_data();
			if(  leading  ) {
				cnv->must_recalc_data_front();
			}
		}
	}

	if(route_index==searchforstop  &&  state==landing  &&  !target_halt.is_bound()) {

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not re-entrant!
		if(!cnv->is_waiting()) {
			return false;
		}

		// nothing free here?
		if(find_route_to_stop_position()) {
			// stop reservation successful
			state = taxiing;
			return true;
		}
		restart_speed = 0;
		return false;
	}

	if(state==looking_for_parking) {
		state = taxiing;
	}

	if(state == taxiing  &&  gr->is_halt()  &&  gr->find<air_vehicle_t>()) {
		// the next step is a parking position. We do not enter, if occupied!
		restart_speed = 0;
		return false;
	}

	return true;
}


// this must also change the internal modes for the calculation
void air_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);

	if(  this->is_on_ground()  ) {
		runway_t *w=(runway_t *)gr->get_weg(air_wt);
		if(w) {
			const int cargo = get_total_cargo();
			w->book(cargo, WAY_STAT_GOODS);
			if (leading) {
				w->book(1, WAY_STAT_CONVOIS);
			}
		}
	}
}


air_vehicle_t::air_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : vehicle_t()
{
	rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_first) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL) {
			dbg->warning("aircraft_t::aircraft_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passengers");
			desc = vehicle_builder_t::get_best_matching(air_wt, 0, 101, 1000, 800, !fracht.empty() ? fracht.front().get_desc() : goods_manager_t::passengers, true, last_desc, is_last );
			if(desc) {
				calc_image();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
}


air_vehicle_t::air_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player_, convoi_t* cn) :
	vehicle_t(pos, desc, player_)
{
	cnv = cn;
	state = taxiing;
	flying_height = 0;
	target_height = pos.z;
}


air_vehicle_t::~air_vehicle_t()
{
	// mark aircraft (after_image) dirty, since we have no "real" image
	const int raster_width = get_current_tile_raster_width();
	sint16 yoff = tile_raster_scale_y(-flying_height-get_hoff()-2, raster_width);

	mark_image_dirty( image, yoff);
	mark_image_dirty( image, 0 );
}


void air_vehicle_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("aircraft_t::set_convoi()","%p",c);
	if(leading  &&  (unsigned long)cnv > 1) {
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
				block_reserver( touchdown, searchforstop+1, false );
			}
		}
	}
	// maybe need to restore state?
	if(c!=NULL) {
		bool target=(bool)cnv;
		vehicle_t::set_convoi(c);
		if(leading) {
			if(target) {
				// reinitialize the target halt
				grund_t* const target=welt->lookup(cnv->get_route()->back());
				target_halt = target->get_halt();
				if(target_halt.is_bound()) {
					target_halt->reserve_position(target,cnv->self);
				}
			}
			// restore reservation
			if(  grund_t *gr = welt->lookup(get_pos())  ) {
				if(  weg_t *weg = gr->get_weg(air_wt)  ) {
					if(  weg->get_desc()->get_styp()==type_runway  ) {
						// but only if we are on a runway ...
						if(  route_index>=takeoff  &&  route_index<touchdown-21  &&  state!=flying  ) {
							block_reserver( takeoff, takeoff+100, true );
						}
						else if(  route_index>=touchdown-1  &&  state!=taxiing  ) {
							block_reserver( touchdown, searchforstop+1, true );
						}
					}
				}
			}
		}
	}
	else {
		vehicle_t::set_convoi(NULL);
	}
}


schedule_t *air_vehicle_t::generate_new_schedule() const
{
	return new airplane_schedule_t();
}


void air_vehicle_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t t( file, "aircraft_t" );

	// initialize as vehicle_t::rdwr_from_convoi calls get_image()
	if (file->is_loading()) {
		state = taxiing;
		flying_height = 0;
	}
	vehicle_t::rdwr_from_convoi(file);

	file->rdwr_enum(state);
	file->rdwr_short(flying_height);
	flying_height &= ~(TILE_HEIGHT_STEP-1);
	file->rdwr_short(target_height);
	file->rdwr_long(searchforstop);
	file->rdwr_long(touchdown);
	file->rdwr_long(takeoff);
}


#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two different directions for the runway search
uint8 air_vehicle_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_type(start, ziel);	// reverse
	// make sure, there are at last two directions to choose, or you might en up with not route
	if(ribi_t::is_single(dir)) {
		dir |= (dir<<1);
		if(dir>16) {
			dir += 1;
		}
	}
	return dir&0x0F;
}
#endif


void air_vehicle_t::hop(grund_t* gr)
{
	sint32 new_speed_limit = SPEED_UNLIMITED;
	sint32 new_friction = 0;

	// take care of in-flight height ...
	const sint16 h_cur = (sint16)get_pos().z*TILE_HEIGHT_STEP;
	const sint16 h_next = (sint16)pos_next.z*TILE_HEIGHT_STEP;

	switch(state) {
		case departing: {
			flying_height = 0;
			target_height = h_cur;
			new_friction = max( 1, 28/(1+(route_index-takeoff)*2) ); // 9 5 4 3 2 2 1 1...

			// take off, when a) end of runway or b) last tile of runway or c) fast enough
			weg_t *weg=welt->lookup(get_pos())->get_weg(air_wt);
			if(  (weg==NULL  ||  // end of runway (broken runway)
				 weg->get_desc()->get_styp()!=type_runway  ||  // end of runway (grass now ... )
				 (route_index>takeoff+1  &&  ribi_t::is_single(weg->get_ribi_unmasked())) )  ||  // single ribi at end of runway
				 cnv->get_akt_speed()>kmh_to_speed(desc->get_topspeed())/3 // fast enough
			) {
				state = flying;
				new_friction = 1;
				block_reserver( takeoff, touchdown-1, false );
				flying_height = h_cur - h_next;
				target_height = h_cur+TILE_HEIGHT_STEP*3;
			}
			break;
		}
		case circling: {
			new_speed_limit = kmh_to_speed(desc->get_topspeed())/3;
			new_friction = 4;
			// do not change height any more while circling
			flying_height += h_cur;
			flying_height -= h_next;
			break;
		}
		case flying: {
			// since we are at a tile border, round up to the nearest value
			flying_height += h_cur;
			if(  flying_height < target_height  ) {
				flying_height = (flying_height+TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
			}
			else if(  flying_height > target_height  ) {
				flying_height = (flying_height-TILE_HEIGHT_STEP);
			}
			flying_height -= h_next;
			// did we have to change our flight height?
			if(  target_height-h_next > TILE_HEIGHT_STEP*5  ) {
				// Move down
				target_height -= TILE_HEIGHT_STEP*2;
			}
			else if(  target_height-h_next < TILE_HEIGHT_STEP*2  ) {
				// Move up
				target_height += TILE_HEIGHT_STEP*2;
			}
			break;
		}
		case landing: {
			new_speed_limit = kmh_to_speed(desc->get_topspeed())/3; // ==approach speed
			new_friction = 8;
			flying_height += h_cur;
			if(  flying_height < target_height  ) {
				flying_height = (flying_height+TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
			}
			else if(  flying_height > target_height  ) {
				flying_height = (flying_height-TILE_HEIGHT_STEP);
			}

			if (route_index >= touchdown)  {
				// come down, now!
				target_height = h_next;

				// touchdown!
				if (flying_height==h_next) {
					const sint32 taxi_speed = kmh_to_speed( min( 60, desc->get_topspeed()/4 ) );
					if(  cnv->get_akt_speed() <= taxi_speed  ) {
						new_speed_limit = taxi_speed;
						new_friction = 16;
					}
					else {
						const sint32 runway_left = searchforstop - route_index;
						new_speed_limit = min( new_speed_limit, runway_left*runway_left*taxi_speed ); // ...approach 540 240 60 60
						const sint32 runway_left_fr = max( 0, 6-runway_left );
						new_friction = max( new_friction, min( desc->get_topspeed()/12, 4 + 4*(runway_left_fr*runway_left_fr+1) )); // ...8 8 12 24 44 72 108 152
					}
				}
			}
			else {
				// runway is on this height
				const sint16 runway_height = cnv->get_route()->at(touchdown).z*TILE_HEIGHT_STEP;

				// we are too low, ascent asap
				if (flying_height < runway_height + TILE_HEIGHT_STEP) {
					target_height = runway_height + TILE_HEIGHT_STEP;
				}
				// too high, descent
				else if (flying_height + h_next - h_cur > runway_height + (sint16)(touchdown-route_index-1)*TILE_HEIGHT_STEP) {
					target_height = runway_height +  (touchdown-route_index-1)*TILE_HEIGHT_STEP;
				}
			}
			flying_height -= h_next;
			break;
		}
		default: {
			new_speed_limit = kmh_to_speed( min( 60, desc->get_topspeed()/4 ) );
			new_friction = 16;
			flying_height = 0;
			target_height = h_next;
			break;
		}
	}

	// hop to next tile
	vehicle_t::hop(gr);

	speed_limit = new_speed_limit;
	current_friction = new_friction;

	// friction factors and speed limit may have changed
	// TODO use the same logic as in vehicle_t::hop
	cnv->must_recalc_data();
}


// this routine will display the aircraft (if in flight)
#ifdef MULTI_THREAD
void air_vehicle_t::display_after(int xpos_org, int ypos_org, const sint8 clip_num) const
#else
void air_vehicle_t::display_after(int xpos_org, int ypos_org, bool is_global) const
#endif
{
	if(  image != IMG_EMPTY  &&  !is_on_ground()  ) {
		int xpos = xpos_org, ypos = ypos_org;

		const int raster_width = get_current_tile_raster_width();
		const sint16 z = get_pos().z;
		if(  z + flying_height/TILE_HEIGHT_STEP - 1 > grund_t::underground_level  ) {
			return;
		}
		const sint16 target = target_height - ((sint16)z*TILE_HEIGHT_STEP);
		sint16 current_flughohe = flying_height;
		if(  current_flughohe < target  ) {
			current_flughohe += (steps*TILE_HEIGHT_STEP) >> 8;
		}
		else if(  current_flughohe > target  ) {
			current_flughohe -= (steps*TILE_HEIGHT_STEP) >> 8;
		}

		sint8 hoff = get_hoff();
		ypos += tile_raster_scale_y(get_yoff()-current_flughohe-hoff-2, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		get_screen_offset( xpos, ypos, raster_width );

		display_swap_clip_wh(CLIP_NUM_VAR);
		// will be dirty
		// the aircraft!!!
		display_color( image, xpos, ypos, get_player_nr(), true, true/*get_flag(obj_t::dirty)*/  CLIP_NUM_PAR);
#ifndef MULTI_THREAD
		vehicle_t::display_after( xpos_org, ypos_org - tile_raster_scale_y( current_flughohe - hoff - 2, raster_width ), is_global );
#endif
		display_swap_clip_wh(CLIP_NUM_VAR);
	}
#ifdef MULTI_THREAD
}
void air_vehicle_t::display_overlay(int xpos_org, int ypos_org) const
{
	if(  image != IMG_EMPTY  &&  !is_on_ground()  ) {
		const int raster_width = get_current_tile_raster_width();
		const sint16 z = get_pos().z;
		if(  z + flying_height/TILE_HEIGHT_STEP - 1 > grund_t::underground_level  ) {
			return;
		}
		const sint16 target = target_height - ((sint16)z*TILE_HEIGHT_STEP);
		sint16 current_flughohe = flying_height;
		if(  current_flughohe < target  ) {
			current_flughohe += (steps*TILE_HEIGHT_STEP) >> 8;
		}
		else if(  current_flughohe > target  ) {
			current_flughohe -= (steps*TILE_HEIGHT_STEP) >> 8;
		}

		vehicle_t::display_overlay( xpos_org, ypos_org - tile_raster_scale_y( current_flughohe - get_hoff() - 2, raster_width ) );
	}
#endif
	else if(  is_on_ground()  ) {
		// show loading tooltips on ground
#ifdef MULTI_THREAD
		vehicle_t::display_overlay( xpos_org, ypos_org );
#else
		vehicle_t::display_after( xpos_org, ypos_org, is_global );
#endif
	}
}


const char *air_vehicle_t::is_deletable(const player_t *player)
{
	if (is_on_ground()) {
		return vehicle_t::is_deletable(player);
	}
	return NULL;
}
