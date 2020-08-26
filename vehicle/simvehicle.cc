/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <limits.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

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
#include "../simunits.h"

#include "../player/simplay.h"
#include "../player/finance.h"
#include "../simfab.h"
#include "../simware.h"
#include "../simhalt.h"
#include "../simsound.h"
#include "../simcity.h"
#include "../simsignalbox.h"

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
#include "../dataobj/way_constraints.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../utils/simrandom.h"
#include "../descriptor/goods_desc.h"

#include "../convoy.h"

#include "../bauer/vehikelbauer.h"

#include "simvehicle.h"
#include "simroadtraffic.h"

#include "../path_explorer.h"
#include "../freight_list_sorter.h"

#define LOADINGBAR_HEIGHT 6
#define WAITINGBAR_HEIGHT 4
#define LOADINGBAR_WIDTH 100

void traffic_vehicle_t::flush_travel_times(strasse_t* str)
{
	if(get_max_speed() && str->get_max_speed() && dist_travelled_since_last_hop > (128 << YARDS_PER_VEHICLE_STEP_SHIFT))
	{
		str->update_travel_times(world()->get_ticks() - time_at_last_hop, dist_travelled_since_last_hop / min(get_max_speed(), kmh_to_speed(str->get_max_speed())));
	}
	reset_measurements();
}

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
	if(old_diagonal_multiplier == 0)
	{
		old_diagonal_vehicle_steps_per_tile = diagonal_vehicle_steps_per_tile;
	}
	else
	{
		old_diagonal_vehicle_steps_per_tile = (uint8)(130560u/old_diagonal_multiplier) + 1;
	}
}


// if true, convoi, must restart!
bool vehicle_base_t::need_realignment() const
{
	return old_diagonal_vehicle_steps_per_tile!=diagonal_vehicle_steps_per_tile  &&  ribi_t::is_bend(direction);
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


#ifdef INLINE_OBJ_TYPE
vehicle_base_t::vehicle_base_t(typ type):
	obj_t(type)
#else
vehicle_base_t::vehicle_base_t():
	obj_t()
#endif
{
	image = IMG_EMPTY;
	set_flag( obj_t::is_vehicle );
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	drives_on_left = false;
	dx = 0;
	dy = 0;
	zoff_start = zoff_end = 0;
	next_lane = 0;
	gr = NULL;
	weg = NULL;
	disp_lane = 2;
}


#ifdef INLINE_OBJ_TYPE
vehicle_base_t::vehicle_base_t(typ type, koord3d pos):
	obj_t(type, pos)
#else
vehicle_base_t::vehicle_base_t(koord3d pos):
	obj_t(pos)
#endif
{
	image = IMG_EMPTY;
	set_flag( obj_t::is_vehicle );
	pos_next = pos;
	steps = 0;
	steps_next = VEHICLE_STEPS_PER_TILE - 1;
	use_calc_height = true;
	drives_on_left = false;
	dx = 0;
	dy = 0;
	zoff_start = zoff_end = 0;
	next_lane = 0;
	gr = NULL;
	weg = NULL;
	disp_lane = 2;
}






void vehicle_base_t::rotate90()
{
	obj_t::rotate90();
	// directions are counterclockwise to ribis!
	direction = ribi_t::rotate90( direction );
	pos_next.rotate90( welt->get_size().y-1 );
	// new offsets
	sint8 new_dx = -dy*2;
	dy = dx/2;
	dx = new_dx;
}



void vehicle_base_t::leave_tile()
{
	// first: release crossing
	// This needs to be refreshed here even though this value is cached:
	// otherwise, it can sometimes be incorrect, which could lead to vehicles failing to release crossings.
	gr = welt->lookup(get_pos());
	weg = NULL;
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
			gr = NULL;
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
	gr = NULL;
}


void vehicle_base_t::enter_tile(grund_t* gr)
{
	if(!gr) {
		dbg->error("vehicle_base_t::enter_tile()","'%s' new position (%i,%i,%i)!",get_name(), get_pos().x, get_pos().y, get_pos().z );
		gr = welt->lookup_kartenboden(get_pos().get_2d());
		set_pos( gr->get_pos() );
	}
	if(!gr->obj_add(this))
	{
		dbg->error("vehicle_base_t::enter_tile()","'%s failed to be added to the object list", get_name());
	}
	weg = gr->get_weg(get_waytype());
}



/* THE routine for moving vehicles
 * it will drive on as long as it can
 * @return the distance actually travelled
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

	uint32 distance_travelled; // Return value

	grund_t *gr = NULL; // if hopped, then this is new position

	uint32 steps_target = steps_to_do + (uint32)steps;

	if(  steps_target > (uint32)steps_next  ) {
		// We are going far enough to hop.

		// We'll be adding steps_next+1 for each hop, as if we
		// started at the beginning of this tile, so for an accurate
		// count of steps done we must subtract the location we started with.
		sint32 steps_done = -steps;
		bool has_hopped = false;
		koord3d pos_prev;

		// Hop as many times as possible.
		while( steps_target > steps_next && (gr = hop_check()) ) {
			// now do the update for hopping
			steps_target -= steps_next+1;
			steps_done += steps_next+1;
			pos_prev = get_pos();
			hop(gr);
			use_calc_height = true;
			has_hopped = true;
		}

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
				else
				{
					set_yoff( pos_prev.y != get_pos().y ? -OBJECT_OFFSET_STEPS/2 : OBJECT_OFFSET_STEPS/2 );
				}
			}
			else
			{
				set_yoff( dy < 0 ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );
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
	update_bookkeeping(distance_travelled  >> YARDS_PER_VEHICLE_STEP_SHIFT );
	return distance_travelled;
}

// For reversing: length-8*(paksize/16)

// to make smaller steps than the tile granularity, we have to use this trick
void vehicle_base_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	sint32 adjusted_steps = steps;
	const vehicle_t* veh = obj_cast<vehicle_t>(this);
	const int dir = ribi_t::get_dir(get_direction());
	if (veh  &&  veh->is_reversed())
	{
		adjusted_steps += (VEHICLE_STEPS_PER_TILE / 2 - veh->get_desc()->get_length_in_steps());
		//adjusted_steps += (sint32)(env_t::reverse_base_offsets[dir][2] * VEHICLE_STEPS_PER_TILE) / veh->get_desc()->get_length_in_steps();
		adjusted_steps += env_t::reverse_base_offsets[dir][2];
	}

	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)adjusted_steps*(uint16)raster_width;

	if(dx && dy) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}

	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (get_hoff(raster_width)) / (4 * 16);

	if (veh && veh->is_reversed())
	{
		xoff += tile_raster_scale_x(env_t::reverse_base_offsets[dir][0], raster_width);
		yoff += tile_raster_scale_y(env_t::reverse_base_offsets[dir][1], raster_width);
	}

	if(  drives_on_left  ) {
		xoff += tile_raster_scale_x( driveleft_base_offsets[dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[dir][1], raster_width );
	}
}


uint16 vehicle_base_t::get_tile_steps(const koord &start, const koord &ende, /*out*/ ribi_t::ribi &direction) const
{
	static const ribi_t::ribi ribis[3][3] = {
			{ribi_t::northwest, ribi_t::north,  ribi_t::northeast},
			{ribi_t::west,     ribi_t::none, ribi_t::east},
			{ribi_t::southwest, ribi_t::south,  ribi_t::southeast}
	};
	const sint8 di = sgn(ende.x - start.x);
	const sint8 dj = sgn(ende.y - start.y);
	direction = ribis[dj + 1][di + 1];
	return di == 0 || dj == 0 ? VEHICLE_STEPS_PER_TILE : diagonal_vehicle_steps_per_tile;
}


// calcs new direction and applies it to the vehicles
ribi_t::ribi vehicle_base_t::calc_set_direction(const koord3d& start, const koord3d& ende)
{
	ribi_t::ribi direction = ribi_t::none; //"direction" = direction (Google); "keine" = none (Google)

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
	zoff_start = zoff_end = 0; // assume flat way

	if(gr==NULL) {
		gr = welt->lookup(get_pos());
	}
	if(gr==NULL) {
		// slope changed below a moving thing?!?
		return;
	}
	else if (gr->ist_tunnel() && gr->ist_karten_boden() && !is_flying()) {
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
	else if(  !gr->is_visible()  ) {
		set_image(IMG_EMPTY);
	}
	else {
		// force a valid image above ground, with special handling of tunnel entraces
		if (get_image() == IMG_EMPTY) {
			if (!gr->ist_tunnel() && gr->ist_karten_boden()) {
				calc_image();
			}
		}
		// will not work great with ways, but is very short!
		slope_t::type hang = gr->get_weg_hang();
		if(  hang  ) {
			const uint slope_height = is_one_high(hang) ? 1 : 2;
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
	return ((h_start*steps + h_end*(256 - steps))*raster_width) >> 9;
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
		cnv_overtaking = true; //treat as convoi is overtaking.
		break;
		case -1:
		cnv_overtaking = false; //treat as convoi is not overtaking.
		break;
	}
	// Search vehicle
	for(  uint8 pos=1;  pos < gr->get_top();  pos++  ) {
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

				const ribi_t::ribi other_90direction = (gr->get_pos().get_2d() == v->get_pos_next().get_2d()) ? other_direction : calc_direction(gr->get_pos(),v->get_pos_next());
				if(  other_90direction == next_90direction  &&  cnv_overtaking == other_overtaking  ) {
					// Want to exit in same as other   ~50% of the time
					return v;
				}

				const bool across = next_direction == (drives_on_left ? ribi_t::rotate45l(next_90direction) : ribi_t::rotate45(next_90direction)); // turning across the opposite directions lane
				const bool other_across = other_direction == (drives_on_left ? ribi_t::rotate45l(other_90direction) : ribi_t::rotate45(other_90direction)); // other is turning across the opposite directions lane
				if(  other_direction == next_direction  &&  !(other_across || across)  &&  cnv_overtaking == other_overtaking  ) {
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
	bool on_left = !(is_overtaking==welt->get_settings().is_drive_left());
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


sint16 vehicle_t::compare_directions(sint16 first_direction, sint16 second_direction) const
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

void vehicle_t::rotate90()
{
	vehicle_base_t::rotate90();
	previous_direction = ribi_t::rotate90( previous_direction );
	last_stop_pos.rotate90( welt->get_size().y-1 );
}


void vehicle_t::rotate90_freight_destinations(const sint16 y_size)
{
	// now rotate the freight
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		FOR(slist_tpl<ware_t>, &tmp, fracht[i])
		{
			tmp.rotate90(y_size);
		}
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
					if (!(water_wt == get_waytype()  && gr && gr->is_water())) { // ships on the open sea are valid
						pos_next = r.at(route_index + 1U);
					}
				}
			}
		}
		// just correct freight destinations
		for (uint8 i = 0; i < number_of_classes; i++)
		{
			FOR(slist_tpl<ware_t>, &c, fracht[i])
			{
				c.finish_rd(welt);
			}
		}
	}
}

/**
 * Unload freight to halt
 * @return sum of unloaded goods
 * @author Hj. Malthaner
 */
uint16 vehicle_t::unload_cargo(halthandle_t halt, sint64 & revenue_from_unloading, array_tpl<sint64> & apportioned_revenues)
{
	uint16 sum_menge = 0, sum_delivered = 0, index = 0;

	revenue_from_unloading = 0;

	if(  !halt.is_bound()  ) {
		return 0;
	}

	if(halt->is_enabled(get_cargo_type()))
	{
		for (uint8 j = 0; j < number_of_classes; j++)
		{
			if (!fracht[j].empty())
			{
				for (slist_tpl<ware_t>::iterator i = fracht[j].begin(), end = fracht[j].end(); i != end; )
				{
					const ware_t& tmp = *i;

					halthandle_t end_halt = tmp.get_ziel();
					halthandle_t via_halt = tmp.get_zwischenziel();

					// check if destination or transfer is still valid
					if (!end_halt.is_bound() || !via_halt.is_bound())
					{
						DBG_MESSAGE("vehicle_t::unload_cargo()", "destination of %d %s is no longer reachable", tmp.menge, translator::translate(tmp.get_name()));
						total_freight -= tmp.menge;
						sum_weight -= tmp.menge * tmp.get_desc()->get_weight_per_unit();
						i = fracht[j].erase(i);
					}
					else if (end_halt == halt || via_halt == halt)
					{
						// here, only ordinary goods should be processed

						if (halt != end_halt && welt->get_settings().is_avoid_overcrowding() && tmp.is_passenger() && !halt->is_within_walking_distance_of(via_halt) && halt->is_overcrowded(tmp.get_desc()->get_index()))
						{
							// The avoid_overcrowding setting is activated
							// Halt overcrowded - discard passengers, and collect no revenue.
							// Experimetal 7.2 - also calculate a refund.

							if (tmp.get_origin().is_bound() && get_owner()->get_finance()->get_account_balance() > 0)
							{
								// Cannot refund unless we know the origin.
								// Also, ought not refund unless the player is solvent.
								// Players ought not be put out of business by refunds, as this makes gameplay too unpredictable,
								// especially in online games, where joining one player's network to another might lead to a large
								// influx of passengers which one of the networks cannot cope with.
								const uint16 distance = shortest_distance(halt->get_basis_pos(), tmp.get_origin()->get_basis_pos());
								const uint32 distance_meters = (uint32)distance * welt->get_settings().get_meters_per_tile();
								// Refund is approximation.
								const sint64 refund_amount = (tmp.menge * tmp.get_desc()->get_refund(distance_meters) + 2048ll) / 4096ll;

								revenue_from_unloading -= refund_amount;

								// Book refund to the convoi & line
								// (the revenue accounting in simconvoi.cc will take care of the profit booking)
								cnv->book(-refund_amount, convoi_t::CONVOI_REFUNDS);
							}

							// Add passengers to unhappy (due to overcrowding) passengers.
							halt->add_pax_unhappy(tmp.menge);

							total_freight -= tmp.menge;
						}
						else
						{
							assert(halt.is_bound());

							const uint32 menge = tmp.menge;
							halt->liefere_an(tmp); //"supply" (Babelfish)
							sum_menge += menge;
							index = tmp.get_index(); // Note that there is only one freight type per vehicle
							total_freight -= tmp.menge;
							cnv->invalidate_weight_summary();

							// Calculate the revenue for each packet.
							// Also, add to the "apportioned revenues" for way tolls.

							// Don't charge a higher class that the passenger is willing to pay, in case the accommodation was reassigned enroute.
							const uint8 class_of_accommodation = min(class_reassignments[j], tmp.get_class());

							revenue_from_unloading += menge > 0 ? cnv->calc_revenue(tmp, apportioned_revenues, class_of_accommodation) : 0;

							// book delivered goods to destination
							if (end_halt == halt)
							{
								sum_delivered += menge;
								if (tmp.is_passenger())
								{
									// New for Extended 7.2 - add happy passengers
									// to the origin station and transported passengers/mail
									// to the origin city only *after* they arrive at their
									// destinations.
									if (tmp.get_origin().is_bound())
									{
										// Check required because Simutrans-Standard saved games
										// do not have origins. Also, the start halt might not
										// be in (or be fully in) a city.
										tmp.get_origin()->add_pax_happy(menge);
										koord origin_pos = tmp.get_origin()->get_basis_pos();
										const planquadrat_t* tile = welt->access(origin_pos);
										stadt_t* origin_city = tile ? tile->get_city() : NULL;
										if (!origin_city)
										{
											// The origin stop is not within a city.
											// If the stop is located outside the city, but the passengers
											// come from a city, they will not record as transported.
											origin_pos = tmp.get_origin()->get_init_pos();
											tile = welt->access(origin_pos);
											origin_city = tile ? tile->get_city() : NULL;
										}

										if (!origin_city)
										{
											for (uint8 i = 0; i < 16; i++)
											{
												koord pos(origin_pos + origin_pos.second_neighbours[i]);
												const planquadrat_t* tile = welt->access(pos);
												origin_city = tile ? tile->get_city() : NULL;
												if (origin_city)
												{
													break;
												}
											}
										}

										if (origin_city)
										{
											origin_city->add_transported_passengers(menge);
										}
									}
								}
								else if (tmp.is_mail())
								{
									if (tmp.get_origin().is_bound())
									{
										// Check required because Simutrans-Standard saved games
										// do not have origins. Also, the start halt might not
										// be in (or be fully in) a city.
										tmp.get_origin()->add_mail_delivered(menge);
										koord origin_pos = tmp.get_origin()->get_basis_pos();
										const planquadrat_t* tile = welt->access(origin_pos);
										stadt_t* origin_city = tile ? tile->get_city() : NULL;
										if (!origin_city)
										{
											// The origin stop is not within a city.
											// If the stop is located outside the city, but the passengers
											// come from a city, they will not record as transported.
											origin_pos = tmp.get_origin()->get_init_pos();
											tile = welt->access(origin_pos);
											origin_city = tile ? tile->get_city() : NULL;
										}

										if (!origin_city)
										{
											for (uint8 i = 0; i < 16; i++)
											{
												koord pos(origin_pos + origin_pos.second_neighbours[i]);
												const planquadrat_t* tile = welt->access(pos);
												origin_city = tile ? tile->get_city() : NULL;
												if (origin_city)
												{
													break;
												}
											}
										}
										if (origin_city)
										{
											origin_city->add_transported_mail(menge);
										}
									}
								}
							}
						}
						i = fracht[j].erase(i);
					}
					else {
						++i;
					}
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
 * @return true if still space for more cargo
 * @author Hj. Malthaner
 */
bool vehicle_t::load_freight_internal(halthandle_t halt, bool overcrowd, bool *skip_vehicles, bool use_lower_classes)
{
	const uint16 total_capacity = desc->get_total_capacity() + (overcrowd ? desc->get_overcrowded_capacity() : 0);
	bool other_classes_available = false;
	uint8 goods_restriction = 0;
	if (total_freight < total_capacity)
	{
		schedule_t *schedule = cnv->get_schedule();
		slist_tpl<ware_t> freight_add;
		uint16 capacity_this_class;
		uint32 freight_this_class;
		uint8 lowest_class_with_nonzero_capacity = 255;

		*skip_vehicles = true;
		if (!fracht[0].empty() && desc->get_mixed_load_prohibition()) {
			FOR(slist_tpl<ware_t>, const& w, fracht[0])
			{
				goods_restriction = w.index;
				break;
			}
		}
		for (uint8 i = 0; i < number_of_classes; i++)
		{
			capacity_this_class = get_accommodation_capacity(i);
			if (capacity_this_class == 0)
			{
				continue;
			}

			if (lowest_class_with_nonzero_capacity == 255)
			{
				lowest_class_with_nonzero_capacity = i;
				if (overcrowd)
				{
					capacity_this_class += desc->get_overcrowded_capacity();
				}
			}

			if (desc->get_freight_type() == goods_manager_t::passengers || desc->get_freight_type() == goods_manager_t::mail)
			{
				freight_this_class = 0;
				if (!fracht[i].empty())
				{
					FOR(slist_tpl<ware_t>, const& w, fracht[i])
					{
						freight_this_class += w.menge;
					}
				}
			}
			else
			{
				// Things other than passengers and mail need not worry about class.
				freight_this_class = total_freight;
			}

			const sint32 capacity_left_this_class = capacity_this_class - freight_this_class;

			// use_lower_classes as passed to this method indicates whether the higher class accommodation is full, hence
			// the need for higher class passengers/mail to use lower class accommodation.
			if (capacity_left_this_class >= 0)
			{
				*skip_vehicles &= halt->fetch_goods(freight_add, desc->get_freight_type(), capacity_left_this_class, schedule, cnv->get_owner(), cnv, overcrowd, class_reassignments[i], use_lower_classes, other_classes_available, desc->get_mixed_load_prohibition(), goods_restriction);
				if (!freight_add.empty())
				{
					cnv->invalidate_weight_summary();
					for (slist_tpl<ware_t>::iterator iter_z = freight_add.begin(); iter_z != freight_add.end(); )
					{
						ware_t &ware = *iter_z;
						total_freight += ware.menge;

						// could this be joined with existing freight?
						FOR(slist_tpl<ware_t>, &tmp, fracht[i])
						{
							// New system: only merges if origins are alike.
							// @author: jamespetts
							if (ware.can_merge_with(tmp))
							{
								tmp.menge += ware.menge;
								ware.menge = 0;
								break;
							}
						}

						// if != 0 we could not join it to existing => load it
						if (ware.menge != 0)
						{
							++iter_z;
							// we add list directly
						}
						else
						{
							iter_z = freight_add.erase(iter_z);
						}
					}

					if (!freight_add.empty())
					{
						// We now DON'T have to unpick which class was reassigned to i.
						// i is the accommodation class.
						fracht[i].append_list(freight_add);
					}
				}
			}
		}
	}
	return (total_freight < total_capacity && !other_classes_available);
}

void vehicle_t::fix_class_accommodations()
{
	if (!desc || desc->get_total_capacity() == 0)
	{
		// Vehicle ought to be empty - perhaps we should check this.
		return;
	}
	// Any wares in invalid accommodation will be moved to the highest
	// available accommodation. If all accommodation is full, the lowest
	// class of accommodation will be overloaded (as with normal
	//overcrowding).
	vector_tpl<ware_t> wares_to_move;
	sint32 pos = -1;
	sint32 lowest_available_class = -1;

	array_tpl<sint32> load(number_of_classes);
	array_tpl<sint32> capacity(number_of_classes);

	for (uint32 i = 0; i < number_of_classes; i++)
	{
		capacity[i] = get_accommodation_capacity(i);
		load[i] = 0;
		if (capacity[i] > 0 && lowest_available_class == -1)
		{
			lowest_available_class = i;
			continue;
		}
		FOR(slist_tpl<ware_t>, &tmp, fracht[i])
		{
			load[i] += tmp.menge;
			if (load[i] > capacity[i])
			{
				ware_t moved_ware = tmp;
				moved_ware.menge = load[i] - capacity[i];
				tmp.menge -= load[i] - capacity[i];
				load[i] = capacity[i];
				wares_to_move.append(moved_ware);
				pos++;
			}
		}
	}
	sint32 i = number_of_classes - 1;
	while (pos >= 0)
	{
		if (load[i] >= capacity[i] && i != lowest_available_class)
		{
			i--;
			continue;
		}
		load[i] += wares_to_move[pos].menge;
		if (load[i] > capacity[i] && i != lowest_available_class)
		{
			ware_t ware_copy = wares_to_move[pos];
			ware_copy.menge = wares_to_move[pos].menge + capacity[i] - load[i];
			wares_to_move[pos].menge = load[i] - capacity[i];
			fracht[i].append(ware_copy);
			load[i] = capacity[i];
			i--;
		}
		else
		{
			fracht[i].append(wares_to_move[pos]);
			pos--;
		}
	}
}


/**
 * Remove freight that no longer can reach its destination
 * e.g. because of a changed schedule
 * @author Hj. Malthaner
 */
void vehicle_t::remove_stale_cargo()
{
	DBG_DEBUG("vehicle_t::remove_stale_cargo()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	//slist_tpl<ware_t> kill_queue;
	vector_tpl<ware_t> kill_queue;
	total_freight = 0;

	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (!fracht[i].empty())
		{
			FOR(slist_tpl<ware_t>, &tmp, fracht[i])
			{

				bool found = false;

				if (tmp.get_zwischenziel().is_bound()) {
					// the original halt exists, but does we still go there?
					FOR(minivec_tpl<schedule_entry_t>, const& i, cnv->get_schedule()->entries) {
						if (haltestelle_t::get_halt(i.pos, cnv->get_owner()) == tmp.get_zwischenziel()) {
							found = true;
							break;
						}
					}
				}
				if (!found) {
					// the target halt may have been joined or there is a closer one now, thus our original target is no longer valid
					const sint32 offset = cnv->get_schedule()->get_current_stop();
					const sint32 max_count = cnv->get_schedule()->entries.get_count();
					for (sint32 j = 0; j < max_count; j++) {
						// try to unload on next stop
						halthandle_t halt = haltestelle_t::get_halt(cnv->get_schedule()->entries[(i + offset) % max_count].pos, cnv->get_owner());
						if (halt.is_bound()) {
							if (halt->is_enabled(tmp.get_index())) {
								// ok, lets change here, since goods are accepted here
								tmp.access_zwischenziel() = halt;
								if (!tmp.get_ziel().is_bound()) {
									// set target, to prevent that unload_cargo drops cargo
									tmp.set_ziel(halt);
								}
								found = true;
								break;
							}
						}
					}
				}

				if (!found)
				{
					kill_queue.append(tmp);
				}
				else {
					// since we need to point at factory (0,0), we recheck this too
					koord k = tmp.get_zielpos();
					fabrik_t *fab = fabrik_t::get_fab(k);
					tmp.set_zielpos(fab ? fab->get_pos().get_2d() : k);

					total_freight += tmp.menge;
				}
			}

			FOR(vector_tpl<ware_t>, const& c, kill_queue) {
				fabrik_t::update_transit(c, false);
				fracht[i].remove(c);
				cnv->invalidate_weight_summary();
			}
		}
	}
	sum_weight =  get_cargo_weight() + desc->get_weight();
}


void vehicle_t::play_sound() const
{
	if(desc->get_sound() >= 0 && !welt->is_fast_forward())
	{
		welt->play_sound_area_clipped(get_pos().get_2d(), desc->get_sound(), get_waytype());
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
	route_index = start_route_index + 1;
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

		if(previous_direction != direction)
		{
			pre_corner_direction.clear();
		}
	}

	if ( ribi_t::is_single(direction) ) {
		steps_next = VEHICLE_STEPS_PER_TILE - 1;
	}
	else {
		steps_next = diagonal_vehicle_steps_per_tile - 1;
	}
}


#ifdef INLINE_OBJ_TYPE
vehicle_t::vehicle_t(typ type, koord3d pos, const vehicle_desc_t* desc, player_t* player) :
	vehicle_base_t(type, pos)
#else
vehicle_t::vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player) :
	vehicle_base_t(pos)
#endif
{
	this->desc = desc;

	set_owner( player );
	purchase_time = welt->get_current_month();
	cnv = NULL;
	speed_limit = speed_unlimited();

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

	//@author: jamespetts
	direction_steps = 16;
	is_overweight = false;
	reversed = false;
	hop_count = 0;
	base_costs = 0;
	diagonal_costs = 0;
	hill_up = 0;
	hill_down = 0;
	current_livery = "default";
	number_of_classes = desc->get_number_of_classes();

	fracht = new slist_tpl<ware_t>[number_of_classes];
	class_reassignments = new uint8[number_of_classes];
	for (uint32 i = 0; i < number_of_classes; i++)
	{
		// Initialise these with default values.
		class_reassignments[i] = i;
	}

	last_stopped_tile = koord3d::invalid;

}

#ifdef INLINE_OBJ_TYPE
vehicle_t::vehicle_t(typ type) :
	vehicle_base_t(type)
#else
vehicle_t::vehicle_t() :
	vehicle_base_t()
#endif
{
	smoke = true;

	desc = NULL;
	cnv = NULL;

	route_index = 1;
	current_friction = 4;
	sum_weight = 10000UL;
	total_freight = 0;

	leading = last = false;
	check_for_finish = false;
	use_calc_height = true;

	previous_direction = direction = ribi_t::none;

	//@author: jamespetts
	direction_steps = 16;
	is_overweight = false;
	reversed = false;
	hop_count = 0;
	base_costs = 0;
	diagonal_costs = 0;
	hill_up = 0;
	hill_down = 0;
	current_livery = "default";

	// These cannot be set substantively here
	// because we do not know the number of
	// classes yet, which is set in desc.
	number_of_classes = 0;
	fracht = NULL;
	class_reassignments = NULL;

	last_stopped_tile = koord3d::invalid;
}

void vehicle_t::set_desc(const vehicle_desc_t* value)
{
	// Used when upgrading vehicles.

	// Empty the vehicle (though it should already be empty).
	// We would otherwise have to check passengers occupied valid accommodation.
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (!fracht[i].empty())
		{
			FOR(slist_tpl<ware_t>, &tmp, fracht[i]) {
				fabrik_t::update_transit(tmp, false);
			}
			fracht[i].clear();
		}
	}

	desc = value;
}


route_t::route_result_t vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route)
{
	return route->calc_route(welt, start, ziel, this, max_speed, cnv != NULL ? cnv->get_highest_axle_load() : ((get_sum_weight() + 499) / 1000), is_tall, 0, SINT64_MAX_VALUE, cnv != NULL ? cnv->get_weight_summary().weight / 1000 : get_total_weight());
}

route_t::route_result_t vehicle_t::reroute(const uint16 reroute_index, const koord3d &ziel)
{
	route_t xroute;    // new scheduled route from position at reroute_index to ziel
	route_t *route = cnv ? cnv->get_route() : NULL;
	const bool live = route == NULL;

	route_t::route_result_t done = route ? calc_route(route->at(reroute_index), ziel, speed_to_kmh(cnv->get_min_top_speed()), cnv->has_tall_vehicles(), &xroute) : route_t::no_route;
	if(done == route_t::valid_route && live)
	{
		// convoy replaces existing route starting at reroute_index with found route.
		cnv->update_route(reroute_index, xroute);
	}
	return done;
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
			uint8 dir;
			const weg_t* way = bd->get_weg(wt);
			if(way && way->has_signal() && (wt == track_wt || wt == tram_wt || wt == narrowgauge_wt || wt == maglev_wt || wt == monorail_wt))
			{
				dir = bd->get_weg_ribi_unmasked(wt);
			}
			else
			{
				dir = get_ribi(bd);
			}
			koord3d nextnext_pos = cnv->get_route()->at(route_index+1);
			uint8 new_dir = ribi_type(nextnext_pos-pos_next);
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
		// can_enter_tile() berechnet auch die Geschwindigkeit
		// mit der spaeter weitergefahren wird
		// "can_enter_tile() calculates the speed later continued with the" (Babelfish)
		if(cnv->get_checked_tile_this_step() != get_pos() && !can_enter_tile(bd, restart_speed, 0))
		{
			// stop convoi, when the way is not free
			cnv->warten_bis_weg_frei(restart_speed);

			// don't continue
			return NULL;
		}
		// we cache it here, hop() will use it to save calls to karte_t::lookup
		return bd;
	}
	else {
		// this is needed since in convoi_t::vorfahren the flag leading is set to null
		if(check_for_finish) {
			return NULL;
		}
	}
	return welt->lookup(pos_next);
}

bool vehicle_t::can_enter_tile(sint32 &restart_speed, uint8 second_check_count)
{
	cnv->set_checked_tile_this_step(get_pos());
	grund_t *gr = welt->lookup(pos_next);
	if (gr)
	{
		bool ok = can_enter_tile( gr, restart_speed, second_check_count );
		if (!ok)
		{
			cnv->set_checked_tile_this_step(koord3d::invalid);
		}
		return ok;
	}
	else {
		if(  !second_check_count  ) {
			cnv->suche_neue_route();
		}
		cnv->set_checked_tile_this_step(koord3d::invalid);
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
		reliefkarte_t::get_karte()->calc_map_pixel( get_pos().get_2d() );  //"Set relief colour" (Babelfish)
	}
}


void vehicle_t::hop(grund_t* gr)
{
	//const grund_t *gr_prev = get_grund();
	const weg_t * weg_prev = get_weg();

	leave_tile();

	pos_prev = get_pos();
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
//	if(  leading  &&  get_pos()==cnv->get_schedule_target()  ) { // leading turned off in vorfahren when reversing
	if(  get_pos()==cnv->get_schedule_target()  ) {
		if(  route_index+1 >= cnv->get_route()->get_count()  ) {
			// we end up here after loading a game or when a waypoint is reached which crosses next itself
			cnv->set_schedule_target( koord3d::invalid );
		}
		else if(welt->lookup(cnv->get_route()->at(route_index))->get_halt() != welt->lookup(cnv->get_route()->at(cnv->get_route()->get_count() - 1))->get_halt())
		{
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
//		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::is_straight(welt->lookup(pos_next)->get_weg_ribi_unmasked(get_waytype()))  )  ||  welt->lookup(pos_next)->is_halt())
		else
		{
			grund_t* gr_next = welt->lookup(pos_next);
			if ( gr_next && ( ( check_for_finish && ribi_t::is_straight(gr_next->get_weg_ribi_unmasked(get_waytype())) ) || gr_next->is_halt()) )
				// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
				direction = calc_set_direction( pos_prev, pos_next );
		}
	}

	// change image if direction changes
	if (previous_direction != direction) {
		calc_image();
	}

	enter_tile(gr); //"Enter field" (Google)
	weg_t *weg = get_weg();
	if(  weg  )	{
		//const grund_t *gr_prev = welt->lookup(pos_prev);
		//const weg_t * weg_prev = gr_prev != NULL ? gr_prev->get_weg(get_waytype()) : NULL;

		// Calculating the speed limit on the fly has the advantage of using up to date data - but there is no algorithm for taking into account the number of tiles of a bridge and thus the bridge weight limit using this mehtod.
		//speed_limit = calc_speed_limit(weg, weg_prev, &pre_corner_direction, direction, previous_direction);
		if (get_route_index() < cnv->get_route_infos().get_count())
		{
			speed_limit = cnv->get_route_infos().get_element(get_route_index()).speed_limit;
		}
		else
		{
			// This does not take into account bridge length (assuming any bridge to be infinitely long)
			speed_limit = calc_speed_limit(weg, weg_prev, &pre_corner_direction, cnv->get_tile_length(), direction, previous_direction);
		}

		// Weight limit needed for GUI flag
		const uint32 max_axle_load = weg->get_max_axle_load() > 0 ? weg->get_max_axle_load() : 1;
		const uint32 bridge_weight_limit = weg->get_bridge_weight_limit() > 0 ? weg->get_bridge_weight_limit() : 1;
		// Necessary to prevent division by zero exceptions if
		// weight limit is set to 0 in the file.

		// This is just used for the GUI display, so only set to true if the weight limit is set to enforce by speed restriction.
		is_overweight = ((cnv->get_highest_axle_load() > max_axle_load || cnv->get_weight_summary().weight / 1000 > bridge_weight_limit) && (welt->get_settings().get_enforce_weight_limits() == 1 || welt->get_settings().get_enforce_weight_limits() == 3));
		if (is_overweight && bridge_weight_limit)
		{
			// This may be triggered incorrectly if the convoy is longer than the bridge and the part of the convoy that can fit onto the bridge at any one time is not overweight. Check for this.
			const sint32 calced_speed_limit = calc_speed_limit(weg, weg_prev, &pre_corner_direction, cnv->get_tile_length(), direction, previous_direction);
			if (speed_limit > calced_speed_limit || calced_speed_limit > calc_speed_limit(weg, weg_prev, &pre_corner_direction, 1, direction, previous_direction))
			{
				is_overweight = false;
			}
		}

		if(weg->is_crossing())
		{
			crossing_t *crossing = gr->find<crossing_t>(2);
			if (crossing)
				crossing->add_to_crossing(this);
		}
	}
	else
	{
		speed_limit = speed_unlimited();
	}

	if(  check_for_finish  &  leading && (direction==ribi_t::north  || direction==ribi_t::west))
	{
		steps_next = (steps_next/2)+1;
	}

	// speedlimit may have changed
	cnv->must_recalc_data();

	calc_drag_coefficient(gr);
	if(weg)
	{
		weg->wear_way(desc->get_way_wear_factor());
	}
	hop_count ++;
	gr->set_all_obj_dirty();
}

/* Calculates the modified speed limit of the current way,
 * taking into account the curve and weight limit.
 * @author: jamespetts/Bernd Gabriel
 */
sint32 vehicle_t::calc_speed_limit(const weg_t *w, const weg_t *weg_previous, fixed_list_tpl<sint16, 192>* cornering_data, uint32 bridge_tiles, ribi_t::ribi current_direction, ribi_t::ribi previous_direction)
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
	const bool is_slope = welt->lookup(w->get_pos())->get_weg_hang() != slope_t::flat;
	const bool slope_specific_speed = w->get_desc()->get_topspeed_gradient_1() < w->get_desc()->get_topspeed() || w->get_desc()->get_topspeed_gradient_2() < w->get_desc()->get_topspeed();

	const bool is_tilting = desc->get_tilting();
	const sint32 base_limit = desc->get_override_way_speed() && !(slope_specific_speed && is_slope) ? SINT32_MAX_VALUE : kmh_to_speed(w->get_max_speed());
	const uint32 max_axle_load = w->get_max_axle_load();
	const uint32 bridge_weight_limit = w->get_bridge_weight_limit();
	const sint32 total_weight = cnv->get_weight_summary().weight / 1000;
	sint32 overweight_speed_limit = base_limit;
	sint32 corner_speed_limit = base_limit;
	sint32 new_limit = base_limit;
	const uint32 highest_axle_load = cnv->get_highest_axle_load();

	// Reduce speed for overweight vehicles

	uint32 adjusted_convoy_weight = cnv->get_tile_length() == 0 ? total_weight : (total_weight * max(bridge_tiles - 2, 1)) / cnv->get_tile_length();
	adjusted_convoy_weight = min(adjusted_convoy_weight, total_weight);

	if((highest_axle_load > max_axle_load || adjusted_convoy_weight > bridge_weight_limit) && (welt->get_settings().get_enforce_weight_limits() == 1 || welt->get_settings().get_enforce_weight_limits() == 3))
	{
		if((max_axle_load != 0 && (highest_axle_load * 100) / max_axle_load <= 110) && (bridge_weight_limit != 0 && (adjusted_convoy_weight * 100) / bridge_weight_limit <= 110))
		{
			// Overweight by up to 10% - reduce speed limit to a third.
			overweight_speed_limit = base_limit / 3;
		}
		else if((max_axle_load == 0 || (highest_axle_load * 100) / max_axle_load > 110) || (bridge_weight_limit == 0 || (adjusted_convoy_weight * 100) / bridge_weight_limit > 110))
		{
			// Overweight by more than 10% - reduce speed limit by a factor of 10.
			overweight_speed_limit = base_limit / 10;
		}
	}

	waytype_t waytype = cnv->get_schedule()->get_waytype();

	if(!is_corner || direction_steps == 0 || waytype == air_wt)
	{
		// If we are not counting corners, do not attempt to calculate their speed limit.
		return overweight_speed_limit;
	}
	else
	{
		// Curve radius computed: only by the time that a 90 or
		// greter degree bend, or a pair of 45 degree bends is
		// found can an accurate radius be computed.

		sint16 direction_difference = 0;
		const sint16 direction = get_direction_degrees(ribi_t::get_dir(current_direction));
		uint16 tmp;

		sint32 counter = 0;
		sint32 steps_to_second_45 = 0;
		sint32 steps_to_90 = 0;
		sint32 steps_to_135 = 0;
		sint32 steps_to_180 = 0;
		const uint16 meters_per_tile = welt->get_settings().get_meters_per_tile();
		int direction_changes = 0;
		sint16 previous_direction = direction;
		for(int i = cornering_data->get_count() - 1; i >= 0 && counter <= direction_steps; i --)
		{
			counter ++;
			if(previous_direction != cornering_data->get_element(i))
			{
				// For detecting pairs of 45 degree corners
				direction_changes ++;
				if(direction_changes == 2)
				{
					steps_to_second_45 = counter;
				}
			}
			previous_direction = cornering_data->get_element(i);
			tmp = vehicle_t::compare_directions(direction, cornering_data->get_element(i));
			if(tmp > direction_difference)
			{
				direction_difference = tmp;
				switch(direction_difference)
				{
					case 90:
						steps_to_90 = counter;
						break;
					case 135:
						steps_to_135 = counter;
						break;
					case 180:
						steps_to_180 = counter;
						break;
					default:
						break;
				}
			}
		}

		// Calculate the radius on the basis of the most severe curve detected.
		int radius = 0;
		sint32 corner_limit_kmh = SINT32_MAX_VALUE;

		// This is the maximum lateral force, denominated in fractions of G.
		// If the number is 10, the maximum allowed lateral force is 1/10th or 0.1G.
		const sint32 corner_force_divider = welt->get_settings().get_corner_force_divider(waytype);

		if(steps_to_180)
		{
			// A 180 degree curve can be made in a minimum of 4 tiles and will have a minimum radius of half the meters_per_tile value
			// The steps_to_x values are the *manhattan* distance, which is exactly twice the actual radius. Thus, halve this here.
			steps_to_180 = max(1, steps_to_180 / 2);

			radius = (steps_to_180 * meters_per_tile) / 2;
			// See here for formula: https://books.google.co.uk/books?id=NbYqQSQcE2MC&pg=PA30&lpg=PA30&dq=curve+radius+speed+limit+formula+rail&source=imglist&ots=mbfC3lCnX4&sig=qClyuNSarnvL-zgOj4HlTVgYOr8&hl=en&sa=X&ei=sBGwVOSGHMyBU4mHgNAC&ved=0CCYQ6AEwATgK#v=onepage&q=curve%20radius%20speed%20limit%20formula%20rail&f=false
			corner_limit_kmh = sqrt_i32((87 * radius) / corner_force_divider);
		}

		if(steps_to_135)
		{
			// A 135 degree curve can be made in a minimum of 4 tiles and will have a minimum radius of 2/3rds the meters_per_tile value
			// The steps_to_x values are the *manhattan* distance, which is exactly twice the actual radius.
			// This was formerly halved, but then multiplied by 2 again, which was redundant, so remove both instead.

			radius = (steps_to_135 * meters_per_tile) / 3;
			corner_limit_kmh = min(corner_limit_kmh, sqrt_i32((87 * radius) / corner_force_divider));
		}

		if(steps_to_90)
		{
			// A 90 degree curve can be made in a minimum of 3 tiles and will have a minimum radius of the meters_per_tile value
			// The steps_to_x values are the *manhattan* distance, which is exactly twice the actual radius. Thus, halve this here.
			radius = (steps_to_90 * meters_per_tile) / 2;

			corner_limit_kmh = min(corner_limit_kmh, sqrt_i32((87 * radius) / corner_force_divider));
		}

		if(steps_to_second_45 && !steps_to_90)
		{
			// Go here only if this is a pair of *self-correcting* 45 degree turns, not a pair
			// that between them add up to 90 degrees, the algorithm for which is above.
			uint32 assumed_radius = welt->get_settings().get_assumed_curve_radius_45_degrees();

			// If assumed_radius == 0, then do not impose any speed limit for 45 degree turns alone.
			if(assumed_radius > 0)
			{
				// A pair of self-correcting 45 degree corners can be made in a minimum of 4 tiles and will have a minimum radius of twice the meters per tile value
				// However, this is too harsh for most uses, so set the assumed radius as the minimum here.
				// There is no need to divide steos_to_second_45 by 2 only to multiply it by 2 again.
				radius = max(assumed_radius, (steps_to_second_45 * meters_per_tile));

				corner_limit_kmh = min(corner_limit_kmh, sqrt_i32((87 * radius) / corner_force_divider));
			}
		}

		if(radius > 0)
		{
			const int tilting_min_radius_effect = welt->get_settings().get_tilting_min_radius_effect();

			sint32 corner_limit = kmh_to_speed(corner_limit_kmh);

			// Adjust for tilting.
			// Tilting only makes a difference to reasonably wide corners.
			if(is_tilting && radius > tilting_min_radius_effect)
			{
				// Tilting trains can take corners faster
				corner_limit = (corner_limit * 130) / 100;
			}

			// Now apply the adjusted corner limit
			if (direction_difference > 0)
			{
				corner_speed_limit = min(base_limit, corner_limit);
			}
		}
	}

	// Overweight penalty not to be made cumulative to cornering penalty
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
	return max(kmh_to_speed(1), new_limit);
}

/** gets the waytype specific friction on straight flat way.
 * extracted from vehicle_t::calc_drag_coefficient()
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


/* calculates the current friction coefficient based on the current track
 * flat, slope, (curve)...
 * @author prissi, HJ, Dwachs
 */
void vehicle_t::calc_drag_coefficient(const grund_t *gr) //,const int h_alt, const int h_neu)
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
	if(previous_direction != direction)
	{
		//The level (if any) of additional friction to apply around corners.
		const uint8 curve_friction_factor = welt->get_settings().get_curve_friction_factor(waytype);
		current_friction += curve_friction_factor;
	}

	// or a hill?
	// Cumulative drag for hills: @author: jamespetts
	// See here for an explanation of the additional resistance
	// from hills: https://en.wikibooks.org/wiki/Fundamentals_of_Transportation/Grade
	const slope_t::type hang = gr->get_weg_hang();
	if(hang != slope_t::flat)
	{
		// Bernd Gabriel, Nov, 30 2009: at least 1 partial direction must match for uphill (op '&'), but not the
		// complete direction. The hill might begin in a curve and then '==' accidently accelerates the vehicle.
		const uint slope_height = is_one_high(hang) ? 1 : 2;
		if(ribi_type(hang) & direction)
		{
			// Uphill

			//  HACK: For some reason yet to be discerned, some road vehicles struggled with hills to the extent of
			// stalling (reverting to the minimum speed of 1km/h). Until we find out where the problem ultimately lies,
			// assume that hills are less teep for road vehicles as for anything else.
			const sint16 abf_1 = get_waytype() == road_wt ? 18 : 40;
			const sint16 abf_2 = get_waytype() == road_wt ? 35 : 80;

			const sint16 acf_1 = get_waytype() == road_wt ? 10 : 23;
			const sint16 acf_2 = get_waytype() == road_wt ? 19 : 47;

			const sint16 additional_base_friction = slope_height == 1 ? abf_1 : abf_2;
			const sint16 additional_current_friction = slope_height == 1 ? acf_1 : acf_2;
			current_friction = min(base_friction + additional_base_friction, current_friction + additional_current_friction);
		}
		else
		{
			// Downhill
			const sint16 subtractional_base_friction = slope_height == 1 ? 24 : 48;
			const sint16 subtractional_current_friction = slope_height == 1 ? 14 : 28;
			current_friction = max(base_friction - subtractional_base_friction, current_friction - subtractional_current_friction);
		}
	}
	else
	{
		current_friction = max(base_friction, current_friction - 13);
	}
}

vehicle_t::direction_degrees vehicle_t::get_direction_degrees(ribi_t::dir direction) const
{
	switch(direction)
	{
	case ribi_t::dir_north :
		return vehicle_t::North;
	case ribi_t::dir_northeast :
		return vehicle_t::Northeast;
	case ribi_t::dir_east :
		return vehicle_t::East;
	case ribi_t::dir_southeast :
		return vehicle_t::Southeast;
	case ribi_t::dir_south :
		return vehicle_t::South;
	case ribi_t::dir_southwest :
		return vehicle_t::Southwest;
	case ribi_t::dir_west :
		return vehicle_t::West;
	case ribi_t::dir_northwest :
		return vehicle_t::Northwest;
	default:
		return vehicle_t::North;
	};
	return vehicle_t::North;
}

void vehicle_t::make_smoke() const
{
	// does it smoke at all?
	if(  smoke  &&  desc->get_smoke()  ) {
		// Hajo: only produce smoke when heavily accelerating or steam engine
		if(  (cnv->get_akt_speed() < (sint32)((cnv->get_vehicle_summary().max_sim_speed * 7u) >> 3) && (route_index < cnv->get_route_infos().get_count() - 4)) ||  desc->get_engine_type() == vehicle_desc_t::steam  ) {
			grund_t* const gr = welt->lookup( get_pos() );
			if(  gr  ) {
				wolke_t* const abgas = new wolke_t( get_pos(), get_xoff() + ((dx * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> 8), get_yoff() + ((dy * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> 8) + get_hoff(), desc->get_smoke() );
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
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		FOR(slist_tpl<ware_t>, const& c, fracht[i])
		{
			weight += c.menge * c.get_desc()->get_weight_per_unit();
		}
	}
	return weight;
}


const char *vehicle_t::get_cargo_name() const
{
	return get_cargo_type()->get_name();
}


void vehicle_t::get_cargo_info(cbuffer_t & buf) const
{
	vector_tpl<vector_tpl<ware_t>> fracht_array(number_of_classes);
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		vector_tpl<ware_t> this_iteration_vector(fracht->get_count());
		FOR(slist_tpl<ware_t>, w, fracht[i])
		{
			this_iteration_vector.append(w);
		}
		fracht_array.append(this_iteration_vector);
	}

	INT_CHECK("simconvoi 2643");

	buf.clear();

	if (get_cargo_type() == goods_manager_t::passengers || get_cargo_type() == goods_manager_t::mail)
	{
		uint8 freight_info_order = 9; // = by_accommodation_detail
		ware_t cargo_type = get_cargo_type();
		for (uint8 i = 0; i < number_of_classes; i++)
		{
			freight_list_sorter_t::sort_freight(fracht_array[i], buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, NULL, "loaded", get_reassigned_class(i), get_accommodation_capacity(i), &cargo_type, true);
		}
	}
	else
	{
		uint8 freight_info_order = 6; // = by_destination_detail
		slist_tpl <ware_t>capacity;
		ware_t ware = get_cargo_type();
		ware.menge = desc->get_total_capacity();
		capacity.insert(ware);
		freight_list_sorter_t::sort_freight(fracht_array[0], buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, &capacity, "loaded", 0, 0, NULL, true);
	}
}

/**
 * Delete all vehicle load
 * @author Hj. Malthaner
 */
void vehicle_t::discard_cargo()
{
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		FOR(slist_tpl<ware_t>, w, fracht[i])
		{
			fabrik_t::update_transit(w, false);
		}
		fracht[i].clear();
	}
	sum_weight =  desc->get_weight();
}

uint16 vehicle_t::load_cargo(halthandle_t halt, bool overcrowd, bool *skip_convois, bool *skip_vehicles, bool use_lower_classes)
{
	const uint16 start_freight = total_freight;
	if(halt.is_bound() && halt->gibt_ab(desc->get_freight_type()))
	{
		*skip_convois = load_freight_internal(halt, overcrowd, skip_vehicles, use_lower_classes);
	}
	else if(desc->get_total_capacity() > 0)
	{
		*skip_convois = true; // don't try to load anymore from a stop that can't supply
	}
	sum_weight = get_cargo_weight() + desc->get_weight();
	calc_image();
	return total_freight - start_freight;
}

void vehicle_t::calc_image()
{
	const goods_desc_t* gd;
	bool empty = true;
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (!fracht[i].empty())
		{
			gd = fracht[i].front().get_desc();
			empty = false;
			break;
		}
	}

	image_id old_image=get_image();
	if (empty)
	{
		set_image(desc->get_image_id(ribi_t::get_dir(get_direction_of_travel()), NULL, current_livery.c_str()));
	}
	else
	{
		set_image(desc->get_image_id(ribi_t::get_dir(get_direction_of_travel()), gd, current_livery.c_str()));
	}
	if(old_image!=get_image()) {
		set_flag(obj_t::dirty);
	}
}

image_id vehicle_t::get_loaded_image() const
{
	const goods_desc_t* gd;
	bool empty = true;
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (!fracht[i].empty())
		{
			gd = fracht[i].front().get_desc();
			empty = false;
			break;
		}
	}
	if (reversed)
	{
		return desc->get_image_id(ribi_t::dir_north, empty ? goods_manager_t::none : gd, current_livery.c_str());
	}
	else
	{
		return desc->get_image_id(ribi_t::dir_south, empty ? goods_manager_t::none : gd, current_livery.c_str());
	}
}


// true, if this vehicle did not moved for some time
bool vehicle_t::is_stuck()
{
	return cnv==NULL  ||  cnv->is_waiting();
}

void vehicle_t::update_bookkeeping(uint32 steps)
{
	   // Only the first vehicle in a convoy does this,
	   // or else there is double counting.
	   // NOTE: As of 9.0, increment_odometer() also adds running costs for *all* vehicles in the convoy.
		if (leading) cnv->increment_odometer(steps);
}

ribi_t::ribi vehicle_t::get_direction_of_travel() const
{
	ribi_t::ribi dir = get_direction();
	if(reversed)
	{
		switch(dir)
		{
		case ribi_t::north:
			dir = ribi_t::south;
			break;

		case ribi_t::east:
			dir = ribi_t::west;
			break;

		case ribi_t::northeast:
			dir = ribi_t::southwest;
			break;

		case ribi_t::south:
			dir = ribi_t::north;
			break;

		case ribi_t::southeast:
			dir = ribi_t::northwest;
			break;

		case ribi_t::west:
			dir = ribi_t::east;
			break;

		case ribi_t::northwest:
			dir = ribi_t::southeast;
			break;

		case ribi_t::southwest:
			dir = ribi_t::northeast;
			break;
		};
	}
	return dir;
}

void vehicle_t::set_reversed(bool value)
{
	if(desc->is_bidirectional())
	{
		reversed = value;
	}
}

uint16 vehicle_t::get_total_cargo_by_class(uint8 g_class) const
{
	uint16 carried = 0;
	if (g_class >= number_of_classes)
	{
		return 0;
	}

	FOR(slist_tpl<ware_t>, const& ware, fracht[g_class])
	{
		carried += ware.menge;
	}

	return carried;
}

uint16 vehicle_t::get_reassigned_class(uint8 g_class) const
{
	uint16 reassigned_class = class_reassignments[g_class];
	return reassigned_class;
}


uint8 vehicle_t::get_number_of_accommodation_classes() const
{
	uint8 accommodation_classes = 0;
	for (uint8 i = 0; i < number_of_classes; i++) {
		if (get_fare_capacity(i)) {
			accommodation_classes++;
		}
	}
	if (!accommodation_classes && desc->get_overcrowded_capacity()) {
		accommodation_classes++;
	}
	return accommodation_classes;
}


uint16 vehicle_t::get_overcrowded_capacity(uint8 g_class) const
{
	if (g_class >= number_of_classes)
	{
		return 0;
	}
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (desc->get_capacity(i) > 0)
		{
			return i == g_class ? desc->get_overcrowded_capacity() : 0;
		}
	}
	return 0;
}

uint16 vehicle_t::get_overcrowding(uint8 g_class) const
{
	const uint16 carried = get_total_cargo_by_class(g_class);
	const uint16 capacity = get_accommodation_capacity(g_class); // Do not count a vehicle as overcrowded if the higher class passengers can travel in lower class accommodation and still get a seat.

	return carried - capacity > 0 ? carried - capacity : 0;
}

uint16 vehicle_t::get_accommodation_capacity(uint8 g_class, bool include_lower_classes) const
{
	if (!include_lower_classes) {
		return desc->get_capacity(g_class);
	}

	uint16 cap = 0;
	for (uint i=0; i <= g_class; i++)
	{
		cap += desc->get_capacity(i);
	}
	return cap;
}

uint16 vehicle_t::get_fare_capacity(uint8 g_class, bool include_lower_classes) const
{
	// Take into account class reassignments.
	uint16 cap = 0;
	for (uint8 i = 0; i < desc->get_number_of_classes(); i++)
	{
		if (class_reassignments[i] == g_class || (include_lower_classes && class_reassignments[i] < g_class))
		{
			cap += desc->get_capacity(i);
		}
	}

	return cap;
}

uint8 vehicle_t::get_comfort(uint8 catering_level, uint8 g_class) const
{
	// If we have more than one class of accommodation that has been reassigned
	// to carry the same class of passengers, take an average of the various types
	// weighted by capacity.
	uint32 comfort_sum = 0;
	uint32 capacity_this_class = 0;

	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (class_reassignments[i] == g_class)
		{
			comfort_sum += desc->get_adjusted_comfort(catering_level, i) * get_accommodation_capacity(i);
			capacity_this_class += get_accommodation_capacity(i);
		}
	}

	uint8 base_comfort;

	if (comfort_sum == 0)
	{
		return 0;
	}


	base_comfort = (uint8)(comfort_sum / capacity_this_class);

	if(base_comfort == 0)
	{
		return 0;
	}
	else if(total_freight <= capacity_this_class)
	{
		// Not overcrowded - return base level
		return base_comfort;
	}

	// Else
	// Overcrowded - adjust comfort. Standing passengers
	// are very uncomfortable (no more than 10).
	const uint8 standing_comfort = (base_comfort < 20) ? (base_comfort / 2) : 10;
	uint16 passenger_count = 0;
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (class_reassignments[i] == g_class)
		{
			FOR(slist_tpl<ware_t>, const& ware, fracht[i])
			{
				if(ware.is_passenger())
				{
					passenger_count += ware.menge;
				}
			}
		}
	}
	//assert(passenger_count <= total_freight); // FIXME: This assert is often triggered after commit 6500dcc. It is not clear why.

	const uint16 total_seated_passengers = passenger_count < capacity_this_class ? passenger_count : capacity_this_class;
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



void vehicle_t::rdwr(loadsave_t *file)
{
	// this is only called from objlist => we save nothing ...
	assert(  file->is_saving()  );
	(void)file;
}


void vehicle_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t r( file, "vehicle_t" );

	obj_t::rdwr(file);
	uint8 saved_number_of_classes = number_of_classes;

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
	{
		// We needed to save this in case "desc" could not be found on re-loading because
		// of some pakset changes. Now we save this for compatibility, and also in
		// case the number of classes in the pakset changes.

		file->rdwr_byte(saved_number_of_classes);
	}
	else if (file->is_loading())
	{
		saved_number_of_classes = 1;
	}

	sint32 total_fracht_count = 0;
	sint32* fracht_count = new sint32[saved_number_of_classes];
	bool create_dummy_ware = false;

	if (file->is_saving())
	{
		for (uint8 i = 0; i < saved_number_of_classes; i++)
		{
			fracht_count[i] = fracht[i].get_count();
			total_fracht_count += fracht_count[i];
		}

		// we try to have one freight count in class zero to guess the right freight
		// when no desc is given
		if (total_fracht_count == 0 && (desc->get_freight_type() != goods_manager_t::none) && desc->get_total_capacity() > 0)
		{
			total_fracht_count = 1;
			fracht_count[0] = 1;
			create_dummy_ware = true;
		}
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
	{
		file->rdwr_bool(create_dummy_ware);
	}

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
		hoff = (sint8)(l*TILE_HEIGHT_STEP / 16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(direction);
		file->rdwr_enum(previous_direction);
		file->rdwr_long(total_fracht_count);
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
		sint16 dummy16 = ((16 * (sint16)hoff) / TILE_HEIGHT_STEP);
		file->rdwr_short(dummy16);
		hoff = (sint8)((TILE_HEIGHT_STEP*(sint16)dummy16) / 16);
		file->rdwr_long(speed_limit);
		file->rdwr_enum(direction);
		file->rdwr_enum(previous_direction);
		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
		{
			for (uint8 i = 0; i < saved_number_of_classes; i++)
			{
				file->rdwr_long(fracht_count[i]);
			}
		}
		else
		{
			file->rdwr_long(total_fracht_count);
		}
		file->rdwr_short(route_index);
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(direction)*2];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1];
	}

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx = get_xoff(), ddy = get_yoff() - hoff;
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
	if((file->get_extended_version()==0 && file->get_version()<=112008) || file->get_extended_version()<14) {
		// Standard version number was increased in Extended without porting this change
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

	if(file->is_saving())
	{
		if (create_dummy_ware)
		{
			// create dummy freight for savegame compatibility
			ware_t ware( desc->get_freight_type() );
			ware.menge = 0;
			ware.set_ziel( halthandle_t() );
			ware.set_zwischenziel( halthandle_t() );
			ware.set_zielpos( get_pos().get_2d() );
			ware.rdwr(file);
		}

		for (uint8 i = 0; i < saved_number_of_classes; i++)
		{
			FOR(slist_tpl<ware_t>, ware, fracht[i])
			{
				ware.rdwr(file);
			}
		}

	}
	else // Loading
	{
		slist_tpl<ware_t> *temp_fracht = new slist_tpl<ware_t>[saved_number_of_classes];

		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
		{
			for (uint8 i = 0; i < saved_number_of_classes; i++)
			{
				for (uint32 j = 0; j < fracht_count[i]; j++)
				{
					ware_t ware(file);
					if ((desc == NULL || ware.menge > 0) && welt->is_within_limits(ware.get_zielpos()) && ware.get_desc())
					{
						// also add, of the desc is unknown to find matching replacement
						temp_fracht[i].insert(ware);
							// restore in-transit information
							fabrik_t::update_transit(ware, true);
					}
					else if (ware.menge > 0)
					{
						if (ware.get_desc())
						{
							dbg->error("vehicle_t::rdwr_from_convoi()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str());
						}
						else
						{
							dbg->error("vehicle_t::rdwr_from_convoi()", "%i of unknown to %s ignored!", ware.menge, ware.get_zielpos().get_str());
						}
					}
				}
			}
		}
		else // Older, pre-classes version
		{
			// We must initialise fracht[] here, as only now do we
			// know the correct number of classes when loading an
			// older saved game: for those games, the number of
			// clases defaults to 1 and we have to read this from
			// desc.

			for (int i = 0; i < total_fracht_count; i++)
			{
				// From an older version with no classes, so put all the ware
				// packets in class zero.
				ware_t ware(file);
				if ((desc == NULL || ware.menge > 0) && welt->is_within_limits(ware.get_zielpos()) && ware.get_desc())
				{
					// also add, of the desc is unknown to find matching replacement
					temp_fracht[0].insert(ware);
						// restore in-transit information
						fabrik_t::update_transit(ware, true);
				}
				else if (ware.menge > 0) {
					if (ware.get_desc()) {
						dbg->error("vehicle_t::rdwr_from_convoi()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str());
					}
					else {
						dbg->error("vehicle_t::rdwr_from_convoi()", "%i of unknown to %s ignored!", ware.menge, ware.get_zielpos().get_str());
					}
				}
			}
		}

		// Work out the number of classes we should have, based
		// on desc (if known) else contained goods (if known),
		// else assume passengers. This must be consistent with the
		// algorithm used to replace missing vehicles.
		if (desc == NULL)
		{
			const goods_desc_t* gd = NULL;
			ware_t ware;
			for (uint8 i = 0; i < saved_number_of_classes; i++)
			{
				if (!temp_fracht[i].empty())
				{
					ware = temp_fracht[i].front();
					gd = ware.get_desc();
					break;
				}
			}
			number_of_classes = gd ? gd->get_number_of_classes() : goods_manager_t::passengers->get_number_of_classes();
		}
		else
		{
			number_of_classes = desc->get_number_of_classes();
		}

		fracht = new slist_tpl<ware_t>[number_of_classes];

		for (uint8 i = 0; i < saved_number_of_classes; i++)
		{
			fracht[min(i, number_of_classes-1)].append_list(temp_fracht[i]);
		}
		delete[]temp_fracht;
	}

	delete[]fracht_count;

	// skip first last info (the convoi will know this better than we!)
	if(file->get_version()<88007) {
		bool dummy = 0;
		file->rdwr_bool(dummy);
		file->rdwr_bool(dummy);
	}

	// koordinate of the last stop
	if(file->get_version()>=99015) {
		// This used to be 2d, now it's 3d.
		if(file->get_extended_version() < 12) {
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

	if(file->get_extended_version() >= 1)
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

	if(file->get_extended_version() >=9 && file->get_version() >= 110000)
	{
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_short(direction_steps);
		// "Current revenue" is obsolete, but was used in this file version
		sint64 current_revenue = 0;
		file->rdwr_longlong(current_revenue);

		if(file->is_saving())
		{
			uint8 count = pre_corner_direction.get_count();
			file->rdwr_byte(count);
			sint16 dir;
			ITERATE(pre_corner_direction, n)
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

	if(file->get_extended_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_string(current_livery);
	}
	else if(file->is_loading())
	{
		current_livery = "default";
	}

	if (file->is_loading())
	{
		// Set sensible defaults.
		class_reassignments = new uint8[number_of_classes];
		for (uint8 i = 0; i < number_of_classes; i++)
		{
			class_reassignments[i] = i;
		}
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
	{
		for (uint8 i = 0; i < saved_number_of_classes; i++)
		{
			uint8 cr = class_reassignments[i];
			file->rdwr_byte(cr);
			if (i < number_of_classes)
			{
				class_reassignments[i] = min(cr, saved_number_of_classes - 1);
			}
		}
	}

	if (file->get_extended_version() >= 15 || (file->get_extended_version() == 14 && file->get_extended_revision() >= 19))
	{
		last_stopped_tile.rdwr(file);
	}
	else
	{
		last_stopped_tile = koord3d::invalid;
	}

	if (file->is_loading())
	{
		leading = last = false;	// dummy, will be set by convoi afterwards
		if (desc) {
			calc_image();

			// full weight after loading
			sum_weight = get_cargo_weight() + desc->get_weight();
		}
		// recalc total freight
		total_freight = 0;

		for (uint8 i = 0; i < number_of_classes; i++)
		{
			FOR(slist_tpl<ware_t>, const& c, fracht[i])
			{
				total_freight += c.menge;
			}
		}
	}
}


uint32 vehicle_t::calc_sale_value() const
{
	// Use float32e8 for reliable and accurate computation
	float32e8_t value ( desc->get_value() );
	if(has_driven)
	{
		// "Drive it off the lot" loss: expressed in mills
		value *= (1000 - welt->get_settings().get_used_vehicle_reduction());
		value /= 1000;
	}
	// General depreciation
	// after 20 years, it has only half value
	// Multiply by .997**number of months
	// Make sure to use OUR version of pow().
	float32e8_t age_in_months = welt->get_current_month() - get_purchase_time();
	static const float32e8_t base_of_exponent(997, 1000);
	value *= pow(base_of_exponent, age_in_months);

	// Convert back to integer
	return (uint32) value;
}

void
vehicle_t::show_info()
{
	if(  cnv != NULL  ) {
		cnv->show_info();
	} else {
		dbg->warning("vehicle_t::show_info()","cnv is null, can't open convoi window!");
	}
}

#if 0
void vehicle_t::info(cbuffer_t & buf) const
{
	if(cnv) {
		cnv->info(buf);
	}
}
#endif

const char *vehicle_t:: is_deletable(const player_t *)
{
	return "Vehicles cannot be removed";
}

ribi_t::ribi vehicle_t::get_next_90direction() const {
	const route_t* route = cnv->get_route();
	if(  route  &&  route_index < route->get_count() - 1u  ) {
		const koord3d pos_next2 = route->at(route_index + 1u);
		return calc_direction(pos_next, pos_next2);
	}
	return ribi_t::none;
}

bool vehicle_t::check_way_constraints(const weg_t &way) const
{
	return missing_way_constraints_t(desc->get_way_constraints(), way.get_way_constraints()).check_next_tile();
}

bool vehicle_t::check_access(const weg_t* way) const
{
	if(get_owner() && get_owner()->is_public_service())
	{
		// The public player can always connect to ways.
		return true;
	}
	const grund_t* const gr = welt->lookup(get_pos());
	const weg_t* const current_way = gr ? gr->get_weg(get_waytype()) : nullptr;
	if(current_way == nullptr)
	{
		return true;
	}
	if (desc->get_engine_type() == vehicle_desc_t::MAX_TRACTION_TYPE && desc->get_topspeed() == 8888)
	{
		// This is a wayobj checker - only allow on ways actually owned by the player or wholly unowned.
		return way && (way->get_owner() == nullptr || (way->get_owner() == get_owner()));
	}
	return way && (way->is_public_right_of_way() || way->get_owner() == nullptr || way->get_owner() == get_owner() || get_owner() == nullptr || way->get_owner() == current_way->get_owner() || way->get_owner()->allows_access_to(get_owner()->get_player_nr()));
}


vehicle_t::~vehicle_t()
{
	if(!welt->is_destroying()) {
		// remove vehicle's marker from the relief map
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}

	delete[] class_reassignments;
	delete[] fracht;
}

void vehicle_t::set_class_reassignment(uint8 original_class, uint8 new_class)
{
	if (original_class >= number_of_classes || new_class >= number_of_classes)
	{
		dbg->error("vehicle_t::set_class_reassignment()", "Attempt to set class out of range");
		return;
	}
	const bool different = class_reassignments[original_class] != new_class;
	class_reassignments[original_class] = new_class;
	if (different)
	{
		for (uint8 i = min(original_class, new_class); i < max(original_class, new_class); i++)
		{
			path_explorer_t::refresh_class_category(get_desc()->get_freight_type()->get_catg(), i);
		}
	}
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
		COLOR_VAL color = COL_GREEN; // not used, but stop compiler warning about uninitialized
		char tooltip_text[1024];
		tooltip_text[0] = 0;
		uint8 tooltip_display_level = env_t::show_vehicle_states;
		// only show when mouse over vehicle
		if(  welt->get_zeiger()->get_pos()==get_pos()  ) {
			tooltip_display_level = 4;
		}else if(tooltip_display_level ==2 && cnv->get_owner() == welt->get_active_player()) {
			tooltip_display_level = 3;
		}

		// now find out what has happened
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::WAITING_FOR_CLEARANCE:
			case convoi_t::CAN_START:
			case convoi_t::CAN_START_ONE_MONTH:
				if(tooltip_display_level >=3  ) {
					tstrncpy( tooltip_text, translator::translate("Waiting for clearance!"), lengthof(tooltip_text) );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::EMERGENCY_STOP:

				if (tooltip_display_level > 0) {
					char emergency_stop_time[64];
					cnv->snprintf_remaining_emergency_stop_time(emergency_stop_time, sizeof(emergency_stop_time));
					sprintf(tooltip_text, translator::translate("emergency_stop %s left"), emergency_stop_time/*, lengthof(tooltip_text) */);
					color = COL_RED;
				}
				break;

			case convoi_t::LOADING:
				if( tooltip_display_level >=3 )
				{
					char waiting_time[64];
					cnv->snprintf_remaining_loading_time(waiting_time, sizeof(waiting_time));
					if(cnv->get_schedule()->get_current_entry().wait_for_time)
					{
						sprintf( tooltip_text, translator::translate("Waiting for schedule. %s left"), waiting_time);
					}
					else if(cnv->get_loading_limit())
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
			case convoi_t::WAITING_FOR_LOADING_THREE_MONTHS:
			case convoi_t::WAITING_FOR_LOADING_FOUR_MONTHS:
				if (tooltip_display_level > 0) {
					sprintf(tooltip_text, translator::translate("Loading (%i->%i%%) Long Time"), cnv->get_loading_level(), cnv->get_loading_limit());
					color = COL_ORANGE;
				}
				break;

			case convoi_t::EDIT_SCHEDULE:
			case convoi_t::ROUTING_2:
			case convoi_t::ROUTE_JUST_FOUND:
				if( tooltip_display_level >=3 ) {
					tstrncpy( tooltip_text, translator::translate("Schedule changing!"), lengthof(tooltip_text) );
					color = COL_LIGHT_YELLOW;
				}
				break;

			case convoi_t::DRIVING:
				if( tooltip_display_level >=3 ) {
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
				if( tooltip_display_level >=3 ) {
					tstrncpy( tooltip_text, translator::translate("Leaving depot!"), lengthof(tooltip_text) );
					color = COL_GREEN;
				}
				break;

				case convoi_t::REVERSING:
				if( tooltip_display_level >=3 )
				{
					char reversing_time[64];
					cnv->snprintf_remaining_reversing_time(reversing_time, sizeof(reversing_time));
					switch (cnv->get_terminal_shunt_mode()) {
						case convoi_t::rearrange:
						case convoi_t::shunting_loco:
							sprintf(tooltip_text, translator::translate("Shunting. %s left"), reversing_time);
							break;
						case convoi_t::change_direction:
							sprintf(tooltip_text, translator::translate("Changing direction. %s left"), reversing_time);
							break;
						default:
							sprintf(tooltip_text, translator::translate("Reversing. %s left"), reversing_time);
							break;
					}
					color = COL_YELLOW;
				}
				break;

			case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			case convoi_t::CAN_START_TWO_MONTHS:
				if (tooltip_display_level > 0) {
					tstrncpy(tooltip_text, translator::translate("clf_chk_stucked"), lengthof(tooltip_text));
					color = COL_ORANGE;
				}
				break;

			case convoi_t::NO_ROUTE:
				if (tooltip_display_level > 0) {
					tstrncpy(tooltip_text, translator::translate("clf_chk_noroute"), lengthof(tooltip_text));
					color = COL_RED;
				}
				break;

			case convoi_t::NO_ROUTE_TOO_COMPLEX:
				if (tooltip_display_level > 0) {
					tstrncpy(tooltip_text, translator::translate("clf_chk_noroute_too_complex"), lengthof(tooltip_text));
					color = COL_RED;
				}
				break;

			case convoi_t::OUT_OF_RANGE:
				if (tooltip_display_level > 0) {
					tstrncpy(tooltip_text, translator::translate("out of range"), lengthof(tooltip_text));
					color = COL_RED;
				}
				break;
		}
		if(is_overweight)
		{
			if (tooltip_display_level > 0) {
				sprintf(tooltip_text, translator::translate("Too heavy"), cnv->get_name());
				color = COL_ORANGE;
			}
		}

		const air_vehicle_t* air = (const air_vehicle_t*)this;
		if(get_waytype() == air_wt && air->runway_too_short)
		{
			if (tooltip_display_level > 0) {
				sprintf(tooltip_text, translator::translate("Runway too short, require %dm"), desc->get_minimum_runway_length());
				color = COL_ORANGE;
			}
		}

		if(get_waytype() == air_wt && air->airport_too_close_to_the_edge)
		{
			if (tooltip_display_level > 0) {
				sprintf(tooltip_text, "%s", translator::translate("Airport too close to the edge"));
				color = COL_ORANGE;
			}
		}

		// something to show?
		if (!tooltip_text[0] && !env_t::show_cnv_nameplates && !env_t::show_cnv_loadingbar) {
			return;
		}
		const int raster_width = get_current_tile_raster_width();
		get_screen_offset(xpos, ypos, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		ypos += tile_raster_scale_y(get_yoff(), raster_width) + 14;

		// convoy(line) nameplate
		if (cnv && (env_t::show_cnv_nameplates == 3 || (env_t::show_cnv_nameplates == 2 && cnv->get_owner() == welt->get_active_player())
			|| ((env_t::show_cnv_nameplates == 1 || env_t::show_cnv_nameplates == 2) && welt->get_zeiger()->get_pos() == get_pos()) ))
		{
			char nameplate_text[1024];
			// show the line name, including when the convoy is coupled.
			linehandle_t lh = cnv->get_line();
			if (lh.is_bound()) {
				// line name
				tstrncpy(nameplate_text, lh->get_name(), lengthof(nameplate_text));
			}
			else {
				// the convoy belongs to no line -> show convoy name
				tstrncpy(nameplate_text, cnv->get_name(), lengthof(nameplate_text));
			}
			const COLOR_VAL pcolor = lh.is_bound() ? cnv->get_owner()->get_player_color1()+3 : cnv->get_owner()->get_player_color1()+1;

			const int width = proportional_string_width(nameplate_text) + 7;
			if (ypos > LINESPACE + 32 && ypos + LINESPACE < display_get_clip_wh().yy) {
				display_ddd_proportional_clip(xpos, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT - LINESPACE/2-2, width, 0, pcolor, COL_WHITE, nameplate_text, true);
				// (*)display_ddd_proportional_clip's height is LINESPACE/2+1+1
			}
		}

		// loading bar
		int extra_y = 0; // yoffset from the default tooltip position
		sint64 waiting_time_per_month = 0;
		COLOR_VAL waiting_bar_col = COL_APRICOT;
		if (cnv && (env_t::show_cnv_loadingbar == 3 || (env_t::show_cnv_loadingbar == 2 && cnv->get_owner() == welt->get_active_player())
			|| ((env_t::show_cnv_loadingbar == 1 || env_t::show_cnv_loadingbar == 2) && welt->get_zeiger()->get_pos() == get_pos()) ))
		{
			switch (cnv->get_state()) {
				case convoi_t::LOADING:
					waiting_time_per_month = int(0.9 + cnv->calc_remaining_loading_time()*200 / welt->ticks_per_world_month);
					break;
				case convoi_t::REVERSING:
					waiting_time_per_month = int(0.9 + cnv->get_wait_lock()*200 / welt->ticks_per_world_month);
					break;
				case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
				case convoi_t::CAN_START_TWO_MONTHS:
					waiting_time_per_month = 100;
					waiting_bar_col = COL_ORANGE;
					if (skinverwaltung_t::alerts) {
						display_color_img(skinverwaltung_t::alerts->get_image_id(3), xpos - 15, ypos - LINESPACE, 0, false, true);
					}
					break;
				case convoi_t::NO_ROUTE:
				case convoi_t::NO_ROUTE_TOO_COMPLEX:
				case convoi_t::OUT_OF_RANGE:
					waiting_time_per_month = 100;
					waiting_bar_col = COL_RED+1;
					if (skinverwaltung_t::pax_evaluation_icons) {
						display_color_img(skinverwaltung_t::pax_evaluation_icons->get_image_id(4), xpos - 15, ypos - LINESPACE, 0, false, true);
					}
					break;
				default:
					break;
			}

			display_ddd_box_clip(xpos-2, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y, LOADINGBAR_WIDTH+2, LOADINGBAR_HEIGHT, MN_GREY2, MN_GREY0);
			sint32 colored_width = cnv->get_loading_level() > 100 ? 100 : cnv->get_loading_level();
			if (cnv->get_loading_limit() && cnv->get_state() == convoi_t::LOADING) {
				display_fillbox_wh_clip(xpos - 1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, cnv->get_loading_limit(), LOADINGBAR_HEIGHT - 2, COL_YELLOW, true);
			}
			else if (cnv->get_loading_limit()) {
				display_blend_wh(xpos-1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, cnv->get_loading_limit(), LOADINGBAR_HEIGHT - 2, COL_YELLOW, 60);
			}
			else {
				display_blend_wh(xpos-1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, LOADINGBAR_WIDTH, LOADINGBAR_HEIGHT - 2, MN_GREY2, colored_width ? 65 : 40);
			}
			display_cylinderbar_wh_clip(xpos-1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, colored_width, LOADINGBAR_HEIGHT-2, COL_GREEN-1, true);

			// overcrowding
			if (cnv->get_overcrowded() && skinverwaltung_t::pax_evaluation_icons) {
				display_color_img(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), xpos + LOADINGBAR_WIDTH + 5, ypos - LINESPACE, 0, false, true);
			}

			extra_y += LOADINGBAR_HEIGHT;

			// winting gauge
			if (waiting_time_per_month) {
				colored_width = waiting_time_per_month > 100 ? 100 : waiting_time_per_month;
				display_ddd_box_clip(xpos - 2, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y, colored_width + 2, WAITINGBAR_HEIGHT, MN_GREY2, MN_GREY0);
				display_cylinderbar_wh_clip(xpos - 1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, colored_width, WAITINGBAR_HEIGHT - 2, waiting_bar_col, true);
				if (waiting_time_per_month > 100) {
					colored_width = waiting_time_per_month-100;
					display_cylinderbar_wh_clip(xpos - 1, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + extra_y + 1, colored_width, WAITINGBAR_HEIGHT - 2, waiting_bar_col-2, true);
				}
				extra_y += WAITINGBAR_HEIGHT;
			}
		}

		// normal tooltip
		if (tooltip_text[0]) {
			const int width = proportional_string_width(tooltip_text) + 7;
			if (ypos > LINESPACE + 32 && ypos + LINESPACE < display_get_clip_wh().yy) {
				display_ddd_proportional_clip(xpos, ypos - LOADINGBAR_HEIGHT - WAITINGBAR_HEIGHT + LINESPACE/2+2 + extra_y, width, 0, color, COL_BLACK, tooltip_text, true);
				// (*)display_ddd_proportional_clip's height is LINESPACE/2+1+1
			}
		}
	}
}

// BG, 06.06.2009: added
void vehicle_t::finish_rd()
{
}

// BG, 06.06.2009: added
void vehicle_t::before_delete()
{
}

uint32 road_vehicle_t::get_max_speed() { return cnv->get_min_top_speed(); }

road_vehicle_t::road_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::road_vehicle, pos, desc, player)
#else
    vehicle_t(pos, desc, player)
#endif
{
	cnv = cn;
	is_checker = false;
	drives_on_left = welt->get_settings().is_drive_left();
	reset_measurements();
}

road_vehicle_t::road_vehicle_t() :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::road_vehicle)
#else
    vehicle_t()
#endif
{
	// This is blank - just used for the automatic road finder.
	cnv = NULL;
	is_checker = true;
}

road_vehicle_t::road_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::road_vehicle)
#else
    vehicle_t()
#endif
{
	rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_leading) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL)
		{
			bool empty = true;
			const goods_desc_t* gd = NULL;
			ware_t ware;
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				if (!fracht[i].empty())
				{
					ware = fracht[i].front();
					gd = ware.get_desc();
					empty = false;
					break;
				}
			}
			if (!gd)
			{
				// Important: This must be the same default ware as used in rdwr_from_convoi.
				gd = goods_manager_t::passengers;
			}

			dbg->warning("road_vehicle_t::road_vehicle_t()","try to find a fitting vehicle for %s.", gd->get_name() );
			desc = vehicle_builder_t::get_best_matching(road_wt, 0, (empty ? 0 : 50), is_leading?50:0, speed_to_kmh(speed_limit), gd, true, last_desc, is_last );
			if(desc) {
				DBG_MESSAGE("road_vehicle_t::road_vehicle_t()","replaced by %s",desc->get_name());
				// still wrong load ...
				calc_image();
			}
			if(!empty  &&  fracht[0].front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht[0].remove_first();
			}
		}
		if(  desc  ) {
			last_desc = desc;
		}
	}
	fix_class_accommodations();
	is_checker = false;
	drives_on_left = welt->get_settings().is_drive_left();
	reset_measurements();
}

void road_vehicle_t::rotate90()
{
	vehicle_t::rotate90();
	calc_disp_lane();
}


void road_vehicle_t::calc_disp_lane()
{
	// driving in the back or the front
	ribi_t::ribi test_dir = welt->get_settings().is_drive_left() ? ribi_t::northeast : ribi_t::southwest;
	disp_lane = get_direction() & test_dir ? 1 : 3;
}


// need to reset halt reservation (if there was one)
route_t::route_result_t road_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route)
{
	assert(cnv);
	// free target reservation
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
	if(leading   &&  previous_direction!=ribi_t::none  &&  cnv  &&  target_halt.is_bound() ) {
		// now reserve our choice (beware: might be longer than one tile!)
		for(  uint32 length=0;  length<cnv->get_tile_length()  &&  length+1<cnv->get_route()->get_count();  length++  ) {
			target_halt->unreserve_position(welt->lookup( cnv->get_route()->at( cnv->get_route()->get_count()-length-1) ), cnv->self );
		}
	}
	target_halt = halthandle_t();	// no block reserved
	const uint32 routing_weight = cnv != NULL ? cnv->get_highest_axle_load() : ((get_sum_weight() + 499) / 1000);
	route_t::route_result_t r = route->calc_route(welt, start, ziel, this, max_speed, routing_weight, is_tall, cnv->get_tile_length(), SINT64_MAX_VALUE, cnv->get_weight_summary().weight / 1000 );
	if(  r == route_t::valid_route_halt_too_short  )
	{
		cbuffer_t buf;
		buf.printf( translator::translate("Vehicle %s cannot choose because stop too short!"), cnv->get_name());
		welt->get_message()->add_message( (const char *)buf, ziel.get_2d(), message_t::traffic_jams, PLAYER_FLAG | cnv->get_owner()->get_player_nr(), cnv->front()->get_base_image() );
	}
	return r;
}



bool road_vehicle_t::check_next_tile(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->get_weg(road_wt);
	if(str==NULL || ((str->get_max_speed()==0 && speed_limit < INT_MAX)))
	{
		return false;
	}
	if(!is_checker)
	{
		bool electric = cnv != NULL ? cnv->needs_electrification() : desc->get_engine_type() == vehicle_desc_t::electric;
		if(electric  &&  !str->is_electrified()) {
			return false;
		}
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			const roadsign_desc_t* rs_desc = rs->get_desc();
			if(get_desc() && rs_desc->get_min_speed()>0  &&  rs_desc->get_min_speed() > kmh_to_speed(get_desc()->get_topspeed())  )
			{
				return false;
			}
			if(  rs_desc->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// private road
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound() && cnv && cnv->is_waiting() && (rs_desc->get_flags() & roadsign_desc_t::END_OF_CHOOSE_AREA)) {
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
int road_vehicle_t::get_cost(const grund_t *gr, const sint32 max_speed, koord from_pos)
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(road_wt);
	if(!w) {
		return 0xFFFF;
	}

	// max_speed?
	sint32 max_tile_speed = w->get_max_speed();

	// add cost for going (with maximum speed, cost is 1)
	sint32 costs = (max_speed <= max_tile_speed) ? 10 : 40 - (30 * max_tile_speed) / max_speed;

	if (desc->get_override_way_speed())
	{
		costs = 1;
	}

	// Take traffic congestion into account in determining the cost. Use the same formula as for private cars.
	const uint32 congestion_percentage = w->get_congestion_percentage();
	if (congestion_percentage)
	{
		costs += (costs * congestion_percentage) / 200;
	}

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		// 75 hardcoded, see get_cost_upslope()
		costs += 75 * slope_t::get_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y );
	}

	// It is now difficult to calculate here whether the vehicle is overweight, so do this in the route finder instead.

	if(w->is_diagonal())
	{
		// Diagonals are a *shorter* distance.
		costs = (costs * 5) / 7; // was: costs /= 1.4
	}

	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool road_vehicle_t:: is_target(const grund_t *gr, const grund_t *prev_gr)
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->get_halt()==target_halt  &&  target_halt->get_empty_lane(gr,cnv->self)!=0) {
		// now we must check the predecessor => try to advance as much as possible
		if(prev_gr!=NULL) {
			const koord3d dir=gr->get_pos()-prev_gr->get_pos();
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

	// eventually shift position to take care of overtaking
	if(cnv) {
		sint8 tiles_overtaking = prev_based ? cnv->get_prev_tiles_overtaking() : cnv->get_tiles_overtaking();
		if(  tiles_overtaking>0  ) {
			/* This means the convoy is overtaking other vehicles. */
			xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
			yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
		}
		else if(  tiles_overtaking<0  ) {
			/* This means the convoy is overtaken by other vehicles. */
			xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width)/5;
			yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width)/5;
		}
	}
}


// chooses a route at a choose sign; returns true on success
bool road_vehicle_t::choose_route(sint32 &restart_speed, ribi_t::ribi start_direction, uint16 index)
{
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
			if(  !target_rt.find_route(welt, next3d, this, speed_to_kmh(cnv->get_min_top_speed()), start_direction, cnv->get_highest_axle_load(), cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, 33, cnv->has_tall_vehicles())  ) {
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
		if (!second_check_count) {
			const grund_t *gr_current = welt->lookup(get_pos());
			if(  gr_current  &&  gr_current->ist_uebergang()  ) {
				return true;
			}
		}

		strasse_t *str = (strasse_t *)gr->get_weg(road_wt);
		if(  !str  ||  gr->get_top() > 250  ) {
			// too many cars here or no street
			if(  !second_check_count  &&  !str) {
				cnv->suche_neue_route();
			}
			return false;
		}

		if (gr->is_height_restricted() && cnv->has_tall_vehicles())
		{
			cnv->suche_neue_route();
			return false;
		}

		route_t const& r = *cnv->get_route();
		const bool route_index_beyond_end_of_route = route_index >= r.get_count();

		// first: check roadsigns
		const roadsign_t *rs = NULL;
		if (str->has_sign()) {
			rs = gr->find<roadsign_t>();

			if (rs && (!route_index_beyond_end_of_route)) {
				// since at the corner, our direction may be diagonal, we make it straight
				uint8 direction90 = ribi_type(get_pos(), pos_next);

				if (rs->get_desc()->is_traffic_light() && (rs->get_dir()&direction90) == 0) {
					// wait here
					restart_speed = 16;
					return false;
				}
				// check, if we reached a choose point
				else if(route_index + 1u < r.get_count())
				{
					// route position after road sign
					const koord3d pos_next_next = r.at(route_index + 1u);
					// since at the corner, our direction may be diagonal, we make it straight
					direction90 = ribi_type(pos_next, pos_next_next);

					if (rs->is_free_route(direction90) && !target_halt.is_bound()) {
						if (second_check_count) {
							return false;
						}
						if (!choose_route(restart_speed, direction90, route_index)) {
							return false;
						}
					}
				}
			}
		}

		// At a crossing, decide whether the convoi should go on passing lane.
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
		if( current_str && current_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) {
			next_lane = -1;
		}

		vehicle_base_t *obj = NULL;
		uint32 test_index = route_index;

		// way should be clear for overtaking: we checked previously
		// calculate new direction
		koord3d next = (route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next);
		ribi_t::ribi curr_direction   = get_direction();
		ribi_t::ribi curr_90direction = calc_direction(get_pos(), pos_next);
		ribi_t::ribi next_direction   = calc_direction(get_pos(), next);
		ribi_t::ribi next_90direction = calc_direction(pos_next, next);
		obj = no_cars_blocking( gr, cnv, curr_direction, next_direction, next_90direction, NULL, next_lane );

		// do not block intersections
		const bool drives_on_left = welt->get_settings().is_drive_left();
		bool int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));

		//If this convoi is overtaking, the convoi must avoid a head-on crash.
		if(  cnv->is_overtaking()  ){
			while(  test_index < route_index + 2u && test_index < r.get_count()  ){
				grund_t *grn = welt->lookup(r.at(test_index));
				if(  !grn  ) {
					cnv->suche_neue_route();
					return false;
				}
				for(  uint8 pos=1;  pos < grn->get_top();  pos++  ){
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
							ribi_t::ribi this_direction = 0;
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
				// ground not existent (probably destroyed)
				if(  !second_check_count  ) {
					cnv->suche_neue_route();
				}
				return false;
			}

			str = (strasse_t *)gr->get_weg(road_wt);

			if(  !str  ) {
				// road not existent (probably destroyed)
				if(  !second_check_count  ) {
					cnv->suche_neue_route();
				}
				return false;
			}

			if(  gr->get_top() > 250  ) {
				// too many cars here or no street
				return false;
			}

			if(  mode_of_start_point<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) lane_of_the_tile = -1;

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

			// first: check roadsigns
			const roadsign_t *rs = NULL;
			if (str->has_sign()) {
				rs = gr->find<roadsign_t>();
				const ribi_t::ribi dir = rs->get_dir();
				const bool is_traffic_light = rs->get_desc()->is_traffic_light();
				route_t const& r = *cnv->get_route();

				if (is_traffic_light || gr->get_weg(get_waytype())->get_ribi_maske() & dir)
				{
					if (rs && (!route_index_beyond_end_of_route)) {
						// since at the corner, our direction may be diagonal, we make it straight

						uint8 direction90 = ribi_type(get_pos(), pos_next);
						if (is_traffic_light && (dir & direction90) == 0) {
							// wait here
							restart_speed = 16;
							return false;
						}
						// Check whether if we reached a choose point
						else if (rs->get_desc()->is_choose_sign())
						{
							// route position after road sign
							const koord3d pos_next_next = r.at(route_index + 1u);
							// since at the corner, our direction may be diagonal, we make it straight
							direction90 = ribi_type(pos_next, pos_next_next);

							if (rs->is_free_route(direction90) && !target_halt.is_bound()) {
								if (second_check_count) {
									return false;
								}
								if (!choose_route(restart_speed, direction90, route_index)) {
									return false;
								}
							}

							const route_t *rt = cnv->get_route();
							// is our target occupied?
							target_halt = haltestelle_t::get_halt(rt->back(), get_owner());
							if (target_halt.is_bound()) {
								// since convois can long than one tile, check is more difficult
								bool can_go_there = true;
								for (uint32 length = 0; can_go_there && length < cnv->get_tile_length() && length + 1 < rt->get_count(); length++) {
									can_go_there &= target_halt->is_reservable(welt->lookup(rt->at(rt->get_count() - length - 1)), cnv->self);
								}
								if (can_go_there) {
									// then reserve it ...
									for (uint32 length = 0; length < cnv->get_tile_length() && length + 1 < rt->get_count(); length++) {
										target_halt->reserve_position(welt->lookup(rt->at(rt->get_count() - length - 1)), cnv->self);
									}
								}
								else {
									// cannot go there => need slot search

									// if we fail, we will wait in a step, much more simulation friendly
									if (!cnv->is_waiting()) {
										restart_speed = -1;
										target_halt = halthandle_t();
										return false;
									}

									// check if there is a free position
									// this is much faster than waysearch
									if (!target_halt->find_free_position(road_wt, cnv->self, obj_t::road_vehicle)) {
										restart_speed = 0;
										target_halt = halthandle_t();
										//DBG_MESSAGE("road_vehicle_t::can_enter_tile()","cnv=%d nothing free found!",cnv->self.get_id());
										return false;
									}

									// now it make sense to search a route
									route_t target_rt;
									koord3d next3d = r.at(test_index);
									if (!target_rt.find_route(welt, next3d, this, speed_to_kmh(cnv->get_min_top_speed()), curr_90direction, cnv->get_highest_axle_load(), cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, 33, cnv->has_tall_vehicles(), route_t::choose_signal)) {
										// nothing empty or not route with less than 33 tiles
										target_halt = halthandle_t();
										restart_speed = 0;
										return false;
									}

									// now reserve our choice (beware: might be longer than one tile!)
									for (uint32 length = 0; length < cnv->get_tile_length() && length + 1 < target_rt.get_count(); length++) {
										target_halt->reserve_position(welt->lookup(target_rt.at(target_rt.get_count() - length - 1)), cnv->self);
									}
									cnv->update_route(test_index, target_rt);
								}
							}
						}
					}
				}
			}
			else {
				rs = NULL;
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
				convoi_t* const ocnv = car->get_convoi();
				sint32 dummy;
				if(  ocnv->front()->get_route_index() < ocnv->get_route()->get_count()  &&  ocnv->front()->can_enter_tile(dummy, second_check_count + 1 )  ) {
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
				// The overtaking judge method itself works only when test_index==route_index+1, that means the front tile is not an intersection.
				// However, with halt_mode we want to simulate a bus terminal. Overtaking in a intersection is essential. So we make a exception to the test_index==route_index+1 condition, although it is not clear that this exception is safe or not!
				if(  (test_index == route_index + 1u) || (test_index == route_index + 2u  &&  overtaking_mode == halt_mode)  ) {
					// no intersections or crossings, we might be able to overtake this one ...
					if(  obj->get_overtaker()  ) {
						// not overtaking/being overtake: we need to make a more thought test!
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
							convoi_t* const ocnv = car->get_convoi();
							// yielding vehicle should not be overtaken by the vehicle whose maximum speed is same.
							bool yielding_factor = true;
							if(  ocnv->get_yielding_quit_index() != -1  &&  this->get_speed_limit() - ocnv->get_min_top_speed() < kmh_to_speed(10)  ) {
								yielding_factor = false;
							}
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking()  &&  !other_lane_blocked()  &&  yielding_factor  &&  cnv->can_overtake( ocnv, (ocnv->is_loading() ? 0 : ocnv->get_akt_speed()), ocnv->get_length_in_steps()+ocnv->front()->get_steps())  ) {
								return true;
							}
							strasse_t *str=(strasse_t *)gr->get_weg(road_wt);
							sint32 cnv_max_speed;
							sint32 other_max_speed;
							if (desc->get_override_way_speed())
							{
								cnv_max_speed = cnv->get_min_top_speed() * kmh_to_speed(1);
							}
							else
							{
								cnv_max_speed = (int)fmin(cnv->get_min_top_speed(), str->get_max_speed() * kmh_to_speed(1));
							}
							if (ocnv->front()->get_desc()->get_override_way_speed())
							{
								other_max_speed = ocnv->get_min_top_speed() * kmh_to_speed(1);
							}
							else
							{
								other_max_speed = (int)fmin(ocnv->get_min_top_speed(), str->get_max_speed() * kmh_to_speed(1));
							}
							if(  cnv->is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
								// If the convoi is on passing lane and there is slower convoi in front of this, this convoi request the slower to go to traffic lane.
								ocnv->set_requested_change_lane(true);
							}
							//For the case that the faster convoi is on traffic lane.
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
								if(  vehicle_base_t* const br = car->other_lane_blocked()  ) {
									if(  road_vehicle_t const* const blk = obj_cast<road_vehicle_t>(br)  ) {
										if(  car->get_direction() == blk->get_direction() && abs(car->get_convoi()->get_min_top_speed() - blk->get_convoi()->get_min_top_speed()) < kmh_to_speed(5)  ){
											//same direction && (almost) same speed vehicle exists.
											ocnv->yield_lane_space();
										}
									}
								}
							}
						}
						else if(  private_car_t* const caut = obj_cast<private_car_t>(obj)  ) {
							if(  cnv->get_lane_affinity() != -1  &&  next_lane<1  &&  !cnv->is_overtaking()  &&  !other_lane_blocked()  &&  cnv->can_overtake(caut, caut->get_current_speed(), VEHICLE_STEPS_PER_TILE)  ) {
								return true;
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
			else if(  overtaking_mode <= twoway_mode) {
				// road is two-way and overtaking is allowed on the stricter condition.
				if(  obj->is_stuck()  ) {
					// end of traffic jam, but no stuck message, because previous vehicle is stuck too
					// Not giving stuck messages here is a problem as there might be a circular jam, which is
					// actually a common case. Do not reset waiting here.
					restart_speed = 0;
					//cnv->set_tiles_overtaking(0);
					//cnv->reset_waiting();
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
							if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
								convoi_t* const ocnv = car->get_convoi();
								if(  cnv->can_overtake( ocnv, (ocnv->is_loading() ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->front()->get_steps())  ) {
									return true;
								}
							}
							else if(  private_car_t* const caut = obj_cast<private_car_t>(obj)  ) {
								if(  cnv->can_overtake(caut, caut->get_desc()->get_topspeed(), VEHICLE_STEPS_PER_TILE)  ) {
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
		if( !route_index_beyond_end_of_route && str->get_pos()!=r.at(route_index)  ){
			for(uint32 test_index = route_index+1u; test_index<r.get_count(); test_index++){
				offset += 1;
				if(  str->get_pos()==r.at(test_index)  ) break;
			}
		}
		// If this vehicle is on passing lane and the next tile prohibites overtaking, this vehicle must wait until traffic lane become safe.
		// When condition changes, overtaking should be quitted once.
		if(  (cnv->is_overtaking()  &&  str->get_overtaking_mode()==prohibited_mode)  ||  (cnv->is_overtaking()  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode()<=prohibited_mode  &&  static_cast<strasse_t*>(welt->lookup(get_pos())->get_weg(road_wt))->get_overtaking_mode()<=oneway_mode)  ) {
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
		// If the next tile is a intersection, lane crossing must be checked before entering.
		// The other vehicle is ignored if it is stopping to avoid stuck.
		const grund_t *gr = route_index_beyond_end_of_route ? NULL : welt->lookup(r.at(route_index));
		const strasse_t *stre = gr ? (strasse_t *)gr->get_weg(road_wt) : NULL;
		const ribi_t::ribi way_ribi = stre ? stre->get_ribi_unmasked() : ribi_t::none;
		if(  stre  &&  stre->get_overtaking_mode() <= oneway_mode  &&  (way_ribi == ribi_t::all  ||  ribi_t::is_threeway(way_ribi))  ) {
			if(  const vehicle_base_t* v = other_lane_blocked(true)  ) {
				if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
					if(  at->get_convoi()->get_akt_speed()!=0  &&  judge_lane_crossing(calc_direction(get_pos(),pos_next), calc_direction(pos_next,pos_next2), at->get_90direction(), cnv->is_overtaking(), false)  ) {
						// vehicle must stop.
						restart_speed = 0;
						cnv->reset_waiting();
						cnv->set_next_cross_lane(true);
						return false;
					}
				}
			}
		}
		// For the case that this vehicle is fixed to passing lane and is on traffic lane.
		if(  str->get_overtaking_mode() <= oneway_mode  &&  cnv->get_lane_affinity() == 1  &&  !cnv->is_overtaking()  ) {
			if(  vehicle_base_t* v = other_lane_blocked()  ) {
				if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
					convoi_t* ocnv = car->get_convoi();
					if(  ocnv  &&  abs(cnv->get_min_top_speed() - ocnv->get_min_top_speed()) < kmh_to_speed(5)  ) {
						cnv->yield_lane_space();
					}
				}
				// citycars do not have the yielding mechanism.
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
		}

		return obj==NULL;
	}

	return true;
}

overtaker_t* road_vehicle_t::get_overtaker()
{
	return cnv;
}

//return overtaker in "convoi_t"
convoi_t* road_vehicle_t::get_overtaker_cv()
{
	return cnv;
}

vehicle_base_t* road_vehicle_t::other_lane_blocked(const bool only_search_top, sint8 offset) const{
	// This function calculate whether the convoi can change lane.
	// only_search_top == false: check whether there's no car in -1 ~ +1 section.
	// only_search_top == true: check whether there's no car in front of this vehicle. (Not the same lane.)
	if(  leading  ){
		route_t const& r = *cnv->get_route();
		bool can_reach_tail = false;
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
		for(sint32 test_index = route_index+offset < (sint32)r.get_count() ? route_index+offset : r.get_count() - 1u;; test_index--){
			grund_t *gr = welt->lookup(r.at(test_index));
			if(  !gr  ) {
				cnv->suche_neue_route();
				return NULL;
			}
			// this function cannot process vehicles on twoway and related mode road.
			const strasse_t* str = (strasse_t *)gr->get_weg(road_wt);
			if(  !str  ||  (str->get_overtaking_mode()>=twoway_mode  &&  str->get_overtaking_mode() <= prohibited_mode)  ) {
				break;
			}

			for(  uint8 pos=1;  pos < gr->get_top();  pos++  ) {
				if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(pos))  ) {
					if(  v->get_typ()==obj_t::pedestrian  ) {
						continue;
					}
					if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
						// ignore ourself
						if(  cnv == at->get_convoi()  ) {
							continue;
						}
						if(  cnv->is_overtaking() && at->get_convoi()->is_overtaking()  ){
							continue;
						}
						if(  !cnv->is_overtaking() && !(at->get_convoi()->is_overtaking())  ){
							//Prohibit going on passing lane when facing traffic exists.
							ribi_t::ribi other_direction = at->get_direction();
							route_t const& r = *cnv->get_route();
							koord3d next = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
							if(  calc_direction(next,get_pos()) == other_direction  ) {
								return v;
							}
							continue;
						}
						// the logic of other_lane_blocked cannot be applied to facing traffic.
						if(test_index>0) {
							const ribi_t::ribi this_prev_dir = calc_direction(r.at(test_index-1u), r.at(test_index));
							if(ribi_t::backward(this_prev_dir)&at->get_previous_direction()) {
								continue;
							}
						}
						// Ignore stopping convoi on the tile behind this convoi to change lane in traffic jam.
						if(  can_reach_tail  &&  at->get_convoi()->get_akt_speed() == 0  ) {
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
						if(  cnv->is_overtaking() && caut->is_overtaking()  ){
							continue;
						}
						if(  !cnv->is_overtaking() && !(caut->is_overtaking())  ){
							//Prohibit going on passing lane when facing traffic exists.
							ribi_t::ribi other_direction = caut->get_direction();
							route_t const& r = *cnv->get_route();
							koord3d next = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
							if(  calc_direction(next,get_pos()) == other_direction  ) {
								return v;
							}
							continue;
						}
						// Ignore stopping convoi on the tile behind this convoi to change lane in traffic jam.
						if(  can_reach_tail  &&  caut->get_current_speed() == 0  ) {
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
			if(  test_index==tail_index-1+offset  ){
				// we finished the examination of the tile behind the tail of convoy.
				break;
			}
			if(  r.at(test_index)==cnv->back()->get_pos()  ) {
				can_reach_tail = true;
			}
			if(  test_index == 0  ||  only_search_top  ) {
				break;
			}
		}
	}
	return NULL;
}

uint32 road_vehicle_t::do_drive(uint32 distance)
{
	uint32 distance_travelled = vehicle_base_t::do_drive(distance);
	if(leading)
	{
		add_distance(distance_travelled);
	}
	return distance_travelled;
}

void road_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);
	const int cargo = get_total_cargo();
	strasse_t *str = (strasse_t*)gr->get_weg(road_wt);
	if(str == NULL)
	{
		return;
	}
	str->book(cargo, WAY_STAT_GOODS);
	if (  leading  )  {
		str->book(1, WAY_STAT_CONVOIS);
		cnv->update_tiles_overtaking();
		if(  next_lane==1  ) {
			cnv->set_tiles_overtaking(3);
			next_lane = 0;
		}
		//decide if overtaking convoi should go back to the traffic lane.
		if(  cnv->get_tiles_overtaking() == 1  &&  str->get_overtaking_mode() <= oneway_mode  ){
			vehicle_base_t* v = NULL;
			if(  cnv->get_lane_affinity() == 1  ||  (v = other_lane_blocked())!=NULL  ){
				//lane change denied
				cnv->set_tiles_overtaking(3);
				if(  cnv->is_requested_change_lane()  ||  cnv->get_lane_affinity() == -1  ) {
					//request the blocking convoi to reduce speed.
					if(  v  ) {
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
							if(  abs(cnv->get_min_top_speed() - car->get_convoi()->get_min_top_speed()) < kmh_to_speed(5)  ) {
								car->get_convoi()->yield_lane_space();
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
				if(  (prev_str  &&  (prev_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode() <= prohibited_mode))  ||  str->get_overtaking_mode()==prohibited_mode  ){
					cnv->set_tiles_overtaking(0);
				}
			}
		}
	}
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
}

void road_vehicle_t::hop(grund_t* gr_to) {
	if(leading)
	{
		grund_t* gr = get_grund();
		if(gr)
		{
			strasse_t* str = (strasse_t*)gr->get_weg(road_wt);
			if(str)
			{
				if(get_last_stop_pos() != get_pos() && get_pos_next() != get_pos_prev())
				{
					flush_travel_times(str);
				}
				else {
					reset_measurements();
				}
			}
		}
	}
	vehicle_t::hop(gr_to);
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



/* from now on rail vehicles (and other vehicles using blocks) */



#ifdef INLINE_OBJ_TYPE
rail_vehicle_t::rail_vehicle_t(typ type, loadsave_t *file, bool is_leading, bool is_last) :
    vehicle_t(type)
{
	init(file, is_leading, is_last);
}

rail_vehicle_t::rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) :
    vehicle_t(obj_t::rail_vehicle)
{
	init(file, is_leading, is_last);
}

void rail_vehicle_t::init(loadsave_t *file, bool is_leading, bool is_last)
#else
rail_vehicle_t::rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) :
    vehicle_t()
#endif
{
	rail_vehicle_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_leading) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL)
		{
			bool empty = true;
			const goods_desc_t* gd = NULL;
			ware_t ware;
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				if (!fracht[i].empty())
				{
					ware = fracht[i].front();
					gd = ware.get_desc();
					empty = false;
					break;
				}
			}
			if (!gd)
			{
				// Important: This must be the same default ware as used in rdwr_from_convoi.
				gd = goods_manager_t::passengers;
			}

			int power = (is_leading || empty || gd == goods_manager_t::none) ? 500 : 0;
			const goods_desc_t* w = empty ? goods_manager_t::none : gd;
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
			if (!empty && !fracht->empty() && fracht[0].front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht[0].remove_first();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
	fix_class_accommodations();
}

#ifdef INLINE_OBJ_TYPE
rail_vehicle_t::rail_vehicle_t(typ type, koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
    vehicle_t(type, pos, desc, player)
{
    cnv = cn;
	working_method = drive_by_sight;
}
#endif

rail_vehicle_t::rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::rail_vehicle, pos, desc, player)
#else
	vehicle_t(pos, desc, player)
#endif
{
    cnv = cn;
	working_method = drive_by_sight;
}


rail_vehicle_t::~rail_vehicle_t()
{
	if (cnv && leading) {
		route_t & r = *cnv->get_route();
		if (!r.empty() && route_index < r.get_count()) {
			// free all reserved blocks
			uint16 dummy = 0;
			block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, target_halt.is_bound() ? 100000 : 1, false, false);
		}
	}
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		schiene_t * sch = (schiene_t *)gr->get_weg(get_waytype());
		if(sch && !get_flag(obj_t::not_on_map)) {
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

				route_t& r = *cnv->get_route();
				if(  !r.empty()  &&  route_index + 1U < r.get_count() - 1  ) {

					uint16 dummy = 0;
					block_reserver(&r, cnv->back()->get_route_index(), dummy, dummy, 100000, false, false);
					target_halt = halthandle_t();
				}
			}
			else if(c->get_next_reservation_index() == 0 && c->get_next_stop_index() == 0 && c->get_state() != convoi_t::REVERSING)
			{
				assert(c!=NULL);
				// eventually search new route
				route_t& r = *c->get_route();
				if(  (r.get_count()<=route_index  ||  r.empty()  ||  get_pos()==r.back())  &&  c->get_state()!=convoi_t::INITIAL  &&  !c->is_loading()  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  ) {
					check_for_finish = true;
					dbg->warning("rail_vehicle_t::set_convoi()", "convoi %i had a too high route index! (%i of max %i)", c->self.get_id(), route_index, r.get_count() - 1);
				}
				// set default next stop index
				c->set_next_stop_index( max(route_index,1)-1 );
				// need to reserve new route?
				if(  !check_for_finish  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  &&  (c->get_state()==convoi_t::DRIVING  ||  c->get_state()>=convoi_t::LEAVING_DEPOT)  ) {
					sint32 num_index = cnv==(convoi_t *)1 ? 1001 : 0; 	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
					uint16 next_signal;
					cnv = c;
					if(  block_reserver(&r, max(route_index,1)-1, welt->get_settings().get_sighting_distance_tiles(), next_signal, num_index, true, false)  ) {
						c->set_next_stop_index(next_signal);
					}
				}
			}
		}
		vehicle_t::set_convoi(c);
	}
}

// need to reset halt reservation (if there was one)
route_t::route_result_t rail_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route)
{
	if(last && route_index < cnv->get_route()->get_count())
	{
		// free all reserved blocks
		if (cnv->get_allow_clear_reservation() && (working_method == one_train_staff || working_method == token_block))
		{
			// These cannot sensibly be resumed inside a section
			set_working_method(drive_by_sight);
			cnv->unreserve_route();
			cnv->reserve_own_tiles();
		}
		else
		{
			uint16 dummy = 0;
			block_reserver(cnv->get_route(), cnv->back()->get_route_index(), dummy, dummy, target_halt.is_bound() ? 100000 : 1, false, true);
		}
	}

	cnv->set_next_reservation_index( 0 );	// nothing to reserve
	target_halt = halthandle_t();	// no block reserved
	// use length > 8888 tiles to advance to the end of terminus stations
	const sint16 tile_length = (cnv->get_schedule()->get_current_entry().reverse == 1 ? 8888 : 0) + cnv->get_true_tile_length();
	route_t::route_result_t r = route->calc_route(welt, start, ziel, this, max_speed, cnv != NULL ? cnv->get_highest_axle_load() : ((get_sum_weight() + 499) / 1000), is_tall, tile_length, SINT64_MAX_VALUE, cnv ? cnv->get_weight_summary().weight / 1000 : get_total_weight());
	cnv->set_next_stop_index(0);
 	if(r == route_t::valid_route_halt_too_short)
	{
		cbuffer_t buf;
		buf.printf( translator::translate("Vehicle %s cannot choose because stop too short!"), cnv ? cnv->get_name() : "Invalid convoy");
		welt->get_message()->add_message( (const char *)buf, ziel.get_2d(), message_t::warnings, PLAYER_FLAG | cnv->get_owner()->get_player_nr(), cnv->front()->get_base_image() );
	}
	return r;
}



bool rail_vehicle_t::check_next_tile(const grund_t *bd) const
{
	if(!bd) return false;
	schiene_t const* const sch = obj_cast<schiene_t>(bd->get_weg(get_waytype()));
	if(  !sch  ) {
		return false;
	}

	// Hajo: diesel and steam engines can use electrified track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : desc->get_engine_type() == vehicle_desc_t::electric);

	if((!needs_no_electric && !sch->is_electrified()) || (sch->get_max_speed() == 0 && speed_limit < INT_MAX) || (cnv ? !cnv->check_way_constraints_of_all_vehicles(*sch) : !check_way_constraints(*sch)))
	{
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
		if(rs && rs->get_desc()->get_wtyp()==get_waytype()  ) {
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

	bool check_reservation = true;

	const koord dir = bd->get_pos().get_2d() - get_pos().get_2d();
	const ribi_t::ribi ribi = ribi_type(dir);

	if(target_halt.is_bound() && cnv && cnv->is_waiting())
	{
		// we are searching a stop here:
		// ok, we can go where we already are ...
		if(bd->get_pos() == get_pos())
		{
			return true;
		}
		// but we can only use empty blocks ...
		// now check, if we could enter here
		check_reservation = sch->can_reserve(cnv->self, ribi);
	}

	if(cnv && cnv->get_is_choosing())
	{
		check_reservation = sch->can_reserve(cnv->self, ribi);
	}

	return check_reservation && check_access(sch);
}


// how expensive to go here (for way search)
// author prissi
int rail_vehicle_t::get_cost(const grund_t *gr, const sint32 max_speed, koord from_pos)
{
	// first favor faster ways
	const weg_t *w = gr->get_weg(get_waytype());
	if(  w==NULL  ) {
		// only occurs when deletion during waysearch
		return 9999;
	}

	// add cost for going (with maximum speed, cost is 10)
	const sint32 max_tile_speed = w->get_max_speed();
	int costs = (max_speed <= max_tile_speed) ? 10 : 40 - (30 * max_tile_speed) / max_speed;
	if (desc->get_override_way_speed())
	{
		costs = 1;
	}

	// effect of slope
	if(  gr->get_weg_hang()!=0  ) {
		// Knightly : check if the slope is upwards, relative to the previous tile
		from_pos -= gr->get_pos().get_2d();
		// 125 hardcoded, see get_cost_upslope()
		costs += 125 * slope_t::get_sloping_upwards( gr->get_weg_hang(), from_pos.x, from_pos.y );
	}

	//@author: jamespetts
	// Strongly prefer routes for which the vehicle is not overweight.
	uint32 max_axle_load = w->get_max_axle_load();
	uint32 bridge_weight_limit = w->get_bridge_weight_limit();
	if(welt->get_settings().get_enforce_weight_limits() == 3 || (welt->get_settings().get_enforce_weight_limits() == 1 && cnv && (cnv->get_highest_axle_load() > max_axle_load || (cnv->get_weight_summary().weight / 1000) > bridge_weight_limit) ) )
	{
		costs += 400;
	}

	if(w->is_diagonal())
	{
		// Diagonals are a *shorter* distance.
		costs = (costs * 5) / 7; // was: costs /= 1.4
	}

	return costs;
}

// this routine is called by find_route, to determined if we reached a destination
bool rail_vehicle_t::is_target(const grund_t *gr,const grund_t *prev_gr)
{
	const schiene_t * sch1 = (const schiene_t *) gr->get_weg(get_waytype());
	ribi_t::ribi ribi_maybe;
	if(prev_gr)
	{
		ribi_maybe = ribi_type(gr->get_pos(), prev_gr->get_pos());
	}
	else
	{
		ribi_maybe = ribi_t::all;
	}
	// first check blocks, if we can go there
	if(  sch1->can_reserve(cnv->self, ribi_maybe)  ) {
		//  just check, if we reached a free stop position of this halt
		if(  gr->is_halt()  &&  gr->get_halt()==target_halt  ) {
			// now we must check the predecessor ...
			if(  prev_gr!=NULL  ) {
				const koord3d dir=gr->get_pos()-prev_gr->get_pos();
				const ribi_t::ribi ribi = ribi_type(dir);
				signal_t* sig = gr->find<signal_t>();
				if(!sig && gr->get_weg(get_waytype())->get_ribi_maske() & ribi)
				{
					// Unidirectional signals allow routing in both directions but only act in one direction. Check whether this is one of those.
					// one way sign wrong direction
					return false;
				}
				grund_t *to;
				if(  !gr->get_neighbour(to,get_waytype(),ribi)  ||  !(to->get_halt()==target_halt)  ||  (!sig && to->get_weg(get_waytype())->get_ribi_maske() & ribi_type(dir))!=0  ) {
					// end of stop: Is it long enough?
					// end of stop could be also signal!
					uint16 tiles = cnv->get_tile_length();
					while(  tiles>1  ) {
						if(  (!sig && gr->get_weg(get_waytype())->get_ribi_maske() & ribi)  ||  !gr->get_neighbour(to,get_waytype(),ribi_t::backward(ribi))  ||  !(to->get_halt()==target_halt)  ) {
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

sint32 rail_vehicle_t::activate_choose_signal(const uint16 start_block, uint16 &next_signal_index, uint32 brake_steps, uint16 modified_sighting_distance_tiles, route_t* route, sint32 modified_route_index)
{
	const schedule_t* schedule = cnv->get_schedule();
	grund_t const* target = welt->lookup(schedule->get_current_entry().pos);

	if(target == NULL)
	{
		cnv->suche_neue_route();
		return 0;
	}

	bool choose_ok = true;

	// TODO: Add option in the convoy's schedule to skip choose signals, and implement this here.

	// check whether there is another choose signal or end_of_choose on the route
	uint32 break_index = start_block + 1;
	for(uint32 idx = break_index; choose_ok && idx < route->get_count(); idx++)
	{
		grund_t *gr = welt->lookup(route->at(idx));
		if(!gr)
		{
			choose_ok = false;
			break_index = idx;
			break;
		}
		if(gr->get_halt() == target->get_halt())
		{
			break_index = idx;
			break;
		}
		weg_t *way = gr->get_weg(get_waytype());
		if(!way)
		{
			choose_ok = false;
			break_index = idx;
			break;
		}
		if(way->has_sign())
		{
			roadsign_t *rs = gr->find<roadsign_t>(1);
			if(rs && rs->get_desc()->get_wtyp() == get_waytype())
			{
				if(rs->get_desc()->is_end_choose_signal())
				{
					if (!(gr->is_halt() && gr->get_halt() != target->get_halt()))
					{
						// Ignore end of choose signals on platforms: these make no sense
						// and cause problems.
						target = gr;
						break_index = idx;
						break;
					}
				}
			}
		}
		if(way->has_signal())
		{
			signal_t *sig = gr->find<signal_t>(1);
 			ribi_t::ribi ribi = ribi_type(route->at(max(1u, modified_route_index) - 1u));
			if(!(gr->get_weg(get_waytype())->get_ribi_maske() & ribi) && gr->get_weg(get_waytype())->get_ribi_maske() != ribi_t::backward(ribi)) // Check that the signal is facing in the right direction.
			{
				if(sig && sig->get_desc()->is_choose_sign())
				{
					// second choose signal on route => not choosing here
					choose_ok = false;
				}
			}
		}
	}

	if(!choose_ok)
	{
		// No need to check the block reserver here as in the old is_choose_signal_clear() method,
		// as this is now called from the block reserver.
		return 0;
	}

	// Choose logic hereafter
	target_halt = target->get_halt();

	// The standard route is not available (which we can assume from the method having been called): try a new route.

	// We are in a step and can use the route search
	route_t target_rt;
	const uint16 first_block = start_block == 0 ? start_block : start_block - 1;
	const uint16 second_block = start_block == 0 ? start_block + 1 : start_block;
	uint16 third_block = start_block == 0 ? start_block + 2 : start_block + 1;
	third_block = min(third_block, route->get_count() - 1);
	const koord3d first_tile = route->at(first_block);
	const koord3d second_tile = route->at(second_block);
	const koord3d third_tile = route->at(third_block);
	const grund_t* third_ground = welt->lookup(third_tile);
	uint8 direction = ribi_type(first_tile, second_tile);

	if (third_ground)
	{
		const schiene_t* railway = (schiene_t*)third_ground->get_weg(get_waytype());
		if (railway && railway->is_junction())
		{
			ribi_t::ribi old_direction = direction;
			direction |= ribi_t::all;
			direction ^= ribi_t::backward(old_direction);
		}
	}
	direction |= ribi_type(second_tile, third_tile);
	direction |= welt->lookup(second_tile)->get_weg(get_waytype())->get_ribi_unmasked();
	cnv->set_is_choosing(true);
	bool can_find_route;

	if(target_halt.is_bound())
	{
		// The target is a stop.
		uint32 route_depth;
		const sint32 route_steps = welt->get_settings().get_max_choose_route_steps();
		if(route_steps)
		{
			route_depth = route_steps;
		}
		else
		{
			route_depth = (welt->get_size().x + welt->get_size().y) * 1000;
		}

		can_find_route = target_rt.find_route(welt, route->at(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), direction, cnv->get_highest_axle_load(), cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, route_depth, cnv->has_tall_vehicles(), route_t::choose_signal);
	}
	else
	{
		// The target is an end of choose sign along the route.
		can_find_route = target_rt.calc_route(welt, route->at(start_block), target->get_pos(), this, speed_to_kmh(cnv->get_min_top_speed()), cnv->get_highest_axle_load(), cnv->has_tall_vehicles(), cnv->get_tile_length(), SINT64_MAX_VALUE, cnv->get_weight_summary().weight / 1000, koord3d::invalid, direction) == route_t::valid_route;
		// This route only takes us to the end of choose sign, so we must calculate the route again beyond that point to the actual destination then concatenate them.
		if(can_find_route)
		{
			// Merge this part route with the remaining route
			route_t intermediate_route;
			intermediate_route.append(route);
			intermediate_route.remove_koord_to(break_index);
			target_rt.append(&intermediate_route);
		}
	}
	sint32 blocks;
	if(!can_find_route)
	{
		// nothing empty or not route with less than welt->get_settings().get_max_choose_route_steps() tiles (if applicable)
		target_halt = halthandle_t();
		cnv->set_is_choosing(false);
		return 0;
	}
	else
	{
		// try to reserve the whole route
		cnv->update_route(start_block, target_rt);
		blocks = block_reserver(route, start_block, modified_sighting_distance_tiles, next_signal_index, 100000, true, false, true, false, false, false, brake_steps);
		if(!blocks)
		{
			target_halt = halthandle_t();
		}
	}
	cnv->set_is_choosing(false);
	return blocks;
}


bool rail_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	assert(leading);
	cnv = get_convoi(); // This should not be necessary, but for some unfathomable reason, cnv is sometimes not equal to get_convoi(), even though get_convoi() just returns cnv (!?!)
	uint16 next_signal = INVALID_INDEX;
	schedule_entry_t destination = cnv->get_schedule()->get_current_entry();

	bool destination_is_nonreversing_waypoint = !destination.reverse && !haltestelle_t::get_halt(destination.pos, get_owner()).is_bound() && (!welt->lookup(destination.pos) || !welt->lookup(destination.pos)->get_depot());

	bool exiting_one_train_staff = false;

	schiene_t *w = (schiene_t*)gr->get_weg(get_waytype());
	if(w == NULL)
	{
		return false;
	}

	// We need gr_current because gr is pos_next
	grund_t *gr_current = welt->lookup(get_pos());
	schiene_t *w_current = gr_current ?  (schiene_t*)gr_current->get_weg(get_waytype()) : NULL;

	if(w_current == NULL)
	{
		return false;
	}

	ribi_t::ribi ribi = ribi_type(cnv->get_route()->at(max(1u, min(cnv->get_route()->get_count() - 1u, route_index)) - 1u), cnv->get_route()->at(min(cnv->get_route()->get_count() - 1u, route_index + 1u)));
	bool nonadjacent_one_train_staff_cabinet = false;

	if(working_method == one_train_staff && cnv->get_state() != convoi_t::LEAVING_DEPOT)
	{
		signal_t* signal = w->get_signal(ribi);
		if(signal && signal->get_desc()->get_working_method() == one_train_staff)
		{
			// Ignore cabinets distant from the triggering cabinet
			const koord3d first_pos = cnv->get_last_signal_pos();
			if (shortest_distance(get_pos().get_2d(), first_pos.get_2d()) < 3)
			{
				signal->set_state(roadsign_t::call_on); // Do not use the same cabinet to switch back to drive by sight immediately after releasing.
				clear_token_reservation(signal, this, w);
				set_working_method(drive_by_sight);
				exiting_one_train_staff = true;
			}
			else if(first_pos != koord3d::invalid)
			{
				nonadjacent_one_train_staff_cabinet = true;
			}
		}
	}

	signal_t* signal_current = NULL;
	if (!nonadjacent_one_train_staff_cabinet)
	{
		signal_current = w_current->get_signal(ribi);
		if (signal_current && signal_current->get_desc()->get_working_method() == one_train_staff)
		{
			// Ignore cabinets distant from the triggering cabinet
			const koord3d first_pos = cnv->get_last_signal_pos();
			if (first_pos != koord3d::invalid && shortest_distance(get_pos().get_2d(), first_pos.get_2d()) >= 3)
			{
				nonadjacent_one_train_staff_cabinet = true;
				signal_current = NULL;
			}
		}
	}

	nonadjacent_one_train_staff_cabinet == false ? w_current->get_signal(ribi) : NULL;

	if(cnv->get_state() == convoi_t::CAN_START)
	{
		if(working_method != token_block && working_method != one_train_staff)
		{
			set_working_method(drive_by_sight);
		}

		if(signal_current && working_method != token_block)
		{
			if(working_method != one_train_staff && (signal_current->get_desc()->get_working_method() != one_train_staff || signal_current->get_pos() != cnv->get_last_signal_pos()))
			{
				set_working_method(signal_current->get_desc()->get_working_method());
			}
		}
	}

	halthandle_t this_halt = haltestelle_t::get_halt(get_pos(), get_owner());
	bool this_halt_has_station_signals = this_halt.is_bound() && this_halt->get_station_signals_count();

	const bool starting_from_stand = cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH
		|| cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS
		|| cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE
		|| cnv->get_state() == convoi_t::CAN_START
		|| cnv->get_state() == convoi_t::CAN_START_ONE_MONTH
		|| cnv->get_state() == convoi_t::CAN_START_TWO_MONTHS
		|| cnv->get_state() == convoi_t::REVERSING;

	// Check for starter signals at the station.
	if (!signal_current && starting_from_stand && this_halt.is_bound() && !this_halt_has_station_signals)
	{
		const uint32 route_count = cnv->get_route()->get_count();
		for (uint32 i = route_index; i < route_count; i ++)
		{
			const koord3d tile_to_check_ahead = cnv->get_route()->at(min(route_count - 1u, i));
			grund_t *gr_ahead = welt->lookup(tile_to_check_ahead);
			weg_t *way = gr_ahead ? gr_ahead->get_weg(get_waytype()) : NULL;
			if (!way)
			{
				// This may happen if a way has been removed since the route was calculated. Must recalculate the route.
				break;
			}
			uint16 modified_route_index = min(route_index + i, route_count - 1u);
			ribi_t::ribi ribi = ribi_type(cnv->get_route()->at(max(1u, modified_route_index) - 1u), cnv->get_route()->at(min(cnv->get_route()->get_count() - 1u, modified_route_index + 1u)));
			signal_current = way->get_signal(ribi);
			if (signal_current)
			{
				// Check for non-adjacent one train staff cabinets - these should be ignored.
				const koord3d first_pos = cnv->get_last_signal_pos();
				if (shortest_distance(get_pos().get_2d(), first_pos.get_2d()) < 3)
				{
					break;
				}
				else if (first_pos != koord3d::invalid)
				{
					nonadjacent_one_train_staff_cabinet = true;
				}
			}
			if (gr_ahead->get_halt() != this_halt)
			{
				break;
			}
		}
	}

	if(signal_current && (signal_current->get_desc()->get_working_method() == time_interval || signal_current->get_desc()->get_working_method() == time_interval_with_telegraph) && signal_current->get_state() == roadsign_t::danger && !signal_current->get_desc()->is_choose_sign() && signal_current->get_no_junctions_to_next_signal() && (signal_current->get_desc()->get_working_method() != one_train_staff || !starting_from_stand))
	{
		restart_speed = 0;
		return false;
	}

	if (signal_current && signal_current->get_desc()->get_working_method() == one_train_staff && cnv->get_state() == convoi_t::DRIVING && signal_current->get_state() != signal_t::call_on)
	{
		// This should only be encountered when a train comes upon a one train staff cabinet having previously stopped at a double block signal or having started from a station.
		if (working_method == drive_by_sight)
		{
			cnv->set_last_signal_pos(koord3d::invalid);
			const bool ok = block_reserver(cnv->get_route(), max(route_index, 1), welt->get_settings().get_sighting_distance_tiles(), next_signal, 0, true, false);
			if (!ok)
			{
				restart_speed = 0;
				return false;
			}
		}
		else
		{
			set_working_method(one_train_staff);
		}
	}

	if((destination_is_nonreversing_waypoint || starting_from_stand) && working_method != one_train_staff && (signal_current || this_halt_has_station_signals) && (this_halt_has_station_signals || !signal_current->get_desc()->get_permissive() || signal_current->get_no_junctions_to_next_signal() == false))
	{
		bool allow_block_reserver = true;
		if (this_halt_has_station_signals)
		{
			// We need to check whether this is a station signal that does not protect any junctions: if so, just check its state, do not call the block reserver.
			enum station_signal_status { none, forward, inverse };
			station_signal_status station_signal = station_signal_status::none;
			signal_t* sig = NULL;
			for(uint32 k = 0; k < this_halt->get_station_signals_count(); k ++)
			{
				grund_t* gr_check = welt->lookup(this_halt->get_station_signal(k));
				if(gr_check)
				{
					weg_t* way_check = gr_check->get_weg(get_waytype());
					sig = way_check ? way_check->get_signal(ribi) : NULL;
					if(sig)
					{
						station_signal = station_signal_status::forward;
						break;
					}
					else
					{
						// Check the opposite direction as station signals work in the exact opposite direction
						sig = gr_check->get_weg(get_waytype())->get_signal(ribi_t::backward(ribi));
						if(sig)
						{
							station_signal = station_signal_status::inverse;
							break;
						}
					}
				}
			}

			if (sig)
			{
				if ((sig->get_desc()->get_working_method() == working_method_t::time_interval || sig->get_desc()->get_working_method() == working_method_t::time_interval_with_telegraph) && sig->get_no_junctions_to_next_signal() && !sig->get_desc()->is_choose_sign())
				{
					// A time interval signal on plain track - do not engage the block reserver
					allow_block_reserver = false;
					const sint64 last_passed = this_halt->get_train_last_departed(ribi);

					const sint64 caution_interval_ticks = welt->get_seconds_to_ticks(welt->get_settings().get_time_interval_seconds_to_caution());
					const sint64 clear_interval_ticks =  welt->get_seconds_to_ticks(welt->get_settings().get_time_interval_seconds_to_clear());
					const sint64 ticks = welt->get_ticks();

					set_working_method(sig->get_desc()->get_working_method());

					if(last_passed + caution_interval_ticks > ticks)
					{
						// Danger
						sig->set_state(signal_t::danger);
						restart_speed = 0;
						return false;
					}
					else if(last_passed + clear_interval_ticks > ticks)
					{
						// Caution
						cnv->set_maximum_signal_speed(min(kmh_to_speed(w_current->get_max_speed()) / 2, sig->get_desc()->get_max_speed() / 2));
						sig->set_state(station_signal == station_signal_status::inverse ? roadsign_t::caution : roadsign_t::caution_no_choose);
						find_next_signal(cnv->get_route(),  max(route_index, 1) - 1, next_signal);
						if (next_signal <= route_index)
						{
							find_next_signal(cnv->get_route(),  max(route_index, 1), next_signal);
						}
					}
					else
					{
						// Clear
						cnv->set_maximum_signal_speed(kmh_to_speed(sig->get_desc()->get_max_speed()));
						sig->set_state(station_signal == station_signal_status::inverse ? roadsign_t::clear : roadsign_t::clear_no_choose);
						find_next_signal(cnv->get_route(),  max(route_index, 1) - 1, next_signal);
						if (next_signal <= route_index)
						{
							find_next_signal(cnv->get_route(),  max(route_index, 1), next_signal);
						}
					}
				}
			}
			else
			{
				// There are no station signals facing in the correct direction
				this_halt_has_station_signals = false;
			}

		}

		if (allow_block_reserver && !block_reserver(cnv->get_route(), max(route_index, 1) - 1, welt->get_settings().get_sighting_distance_tiles(), next_signal, 0, true, false))
		{
			restart_speed = 0;
			return false;
		}
		if (allow_block_reserver && next_signal <= route_index)
		{
			const bool ok = block_reserver(cnv->get_route(), max(route_index, 1), welt->get_settings().get_sighting_distance_tiles(), next_signal, 0, true, false);
			if (!ok)
			{
				restart_speed = 0;
				return false;
			}
		}
		if(!destination_is_nonreversing_waypoint || (cnv->get_state() != convoi_t::CAN_START && cnv->get_state() != convoi_t::CAN_START_ONE_MONTH && cnv->get_state() != convoi_t::CAN_START_TWO_MONTHS))
		{
			cnv->set_next_stop_index(next_signal);
		}
		if (working_method != one_train_staff && signal_current && (signal_current->get_desc()->get_working_method() != one_train_staff || (signal_current->get_pos() != cnv->get_last_signal_pos() && signal_current->get_pos() == get_pos())))
		{
			if (working_method == token_block && signal_current->get_desc()->get_working_method() != token_block)
			{
				clear_token_reservation(w_current->get_signal(ribi), this, w_current);
			}
			set_working_method(signal_current->get_desc()->get_working_method());
		}
		return true;
	}

	if(starting_from_stand && working_method != one_train_staff)
 	{
		cnv->set_next_stop_index(max(route_index, 1) - 1);
		if(steps < steps_next && !this_halt.is_bound())
		{
			// not yet at tile border => can drive to signal safely
 			return true;
 		}
	}

	if(gr->get_top() > 250)
	{
		// too many objects here
		return false;
	}

	//const signal_t* signal_next_tile = welt->lookup(cnv->get_route()->at(min(cnv->get_route()->get_count() - 1u, route_index)))->find<signal_t>();

	if(starting_from_stand && cnv->get_next_stop_index() <= route_index /*&& !signal_next_tile*/ && !signal_current && working_method != drive_by_sight && working_method != moving_block)
	{
		// If we are starting from stand, have no reservation beyond here and there is no signal, assume that it has been deleted and revert to drive by sight.
		// This might also occur when a train in the time interval working method has had a collision from the rear and has gone into an emergency stop.
		if (working_method == token_block || working_method == one_train_staff)
		{
			cnv->set_next_stop_index(INVALID_INDEX);
		}
		else
		{
			set_working_method(drive_by_sight);
		}
	}

	const koord3d dir = gr->get_pos() - get_pos();
	ribi = ribi_type(dir);
	if(!w->can_reserve(cnv->self, ribi))
	{
		restart_speed = 0;
		if(((working_method == time_interval || working_method == time_interval_with_telegraph) && cnv->get_state() == convoi_t::DRIVING && !(signal_current && signal_current->get_state() == roadsign_t::danger)))
		{
			const sint32 emergency_stop_duration = welt->get_seconds_to_ticks(welt->get_settings().get_time_interval_seconds_to_caution() / 2);
			convoihandle_t c = w->get_reserved_convoi();
			const koord3d ground_pos = gr->get_pos();
			for(sint32 i = 0; i < c->get_vehicle_count(); i ++)
			{
				if(c->get_vehicle(i)->get_pos() == ground_pos)
				{
					cnv->set_state(convoi_t::EMERGENCY_STOP);
					cnv->set_wait_lock(emergency_stop_duration + 2000); // We add 2,000ms to what we assume is the rear train to ensure that the front train starts first.
					set_working_method(drive_by_sight);
					cnv->unreserve_route();
					cnv->reserve_own_tiles();
					if(c.is_bound() && !this_halt_has_station_signals)
					{
						c->set_state(convoi_t::EMERGENCY_STOP);
						c->set_wait_lock(emergency_stop_duration);
						/*c->unreserve_route();
						c->reserve_own_tiles();*/
					}
					break;
				}
			}
		}
		else if (working_method == drive_by_sight)
		{
			// Check for head-on collisions in drive by sight mode so as to fix deadlocks automatically.
			convoihandle_t c = w->get_reserved_convoi();
			if (c.is_bound() && (c->get_state() == convoi_t::DRIVING || c->get_state() == convoi_t::WAITING_FOR_CLEARANCE|| c->get_state() == convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH || c->get_state() == convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS) &&
				(cnv->get_state() == convoi_t::DRIVING || cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE|| cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH || cnv->get_state() == convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS) )
			{
				ribi_t::ribi other_convoy_direction = c->front()->get_direction();
				const depot_t* dep = gr->get_depot();
				if (c->front()->get_pos_next() == get_pos() && get_pos_next() == c->front()->get_pos() && !dep && other_convoy_direction != get_direction())
				{
					// Opposite directions detected
					// We must decide whether this or the other convoy should reverse.
					convoihandle_t lowest_numbered_convoy;
					convoihandle_t highest_numbered_convoy;
					if (cnv->self.get_id() < c->self.get_id())
					{
						lowest_numbered_convoy = cnv->self;
						highest_numbered_convoy = c->self;
					}
					else
					{
						lowest_numbered_convoy = c->self;
						highest_numbered_convoy = cnv->self;
					}

					const koord this_pos = lowest_numbered_convoy->get_pos().get_2d();
					const koord lowest_numbered_last_stop_pos = lowest_numbered_convoy->get_schedule()->get_prev_halt(lowest_numbered_convoy->get_owner()).is_bound() ? lowest_numbered_convoy->get_schedule()->get_prev_halt(lowest_numbered_convoy->get_owner())->get_next_pos(this_pos) : this_pos;
					const koord highest_numbered_last_stop_pos = highest_numbered_convoy->get_schedule()->get_prev_halt(highest_numbered_convoy->get_owner()).is_bound() ? highest_numbered_convoy->get_schedule()->get_prev_halt(highest_numbered_convoy->get_owner())->get_next_pos(this_pos) : this_pos;

					const uint16 lowest_numbered_convoy_distance_to_stop = shortest_distance(this_pos, lowest_numbered_last_stop_pos);
					const uint16 highest_numbered_convoy_distance_to_stop = shortest_distance(this_pos, highest_numbered_last_stop_pos);

					if (lowest_numbered_convoy_distance_to_stop <= highest_numbered_convoy_distance_to_stop)
					{
						// The lowest numbered convoy reverses.
						if (lowest_numbered_convoy == cnv->self)
						{
							cnv->is_reversed() ? cnv->get_schedule()->advance() : cnv->get_schedule()->advance_reverse();
							cnv->suche_neue_route();
						}
					}
					else
					{
						// The highest numbered convoy reverses
						if (highest_numbered_convoy == cnv->self)
						{
							cnv->is_reversed() ? cnv->get_schedule()->advance() : cnv->get_schedule()->advance_reverse();
							cnv->suche_neue_route();
						}
					}
				}
			}
		}
		return false;
	}

	convoi_t &convoy = *cnv;
	sint32 brake_steps = convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), cnv->get_akt_speed());
	route_t &route = *cnv->get_route();
	convoi_t::route_infos_t &route_infos = cnv->get_route_infos();

	const uint16 sighting_distance_tiles = welt->get_settings().get_sighting_distance_tiles();
	uint16 modified_sighting_distance_tiles = sighting_distance_tiles;

	sint32 last_index = max (0, route_index - 1);
	for(uint32 i = route_index; i <= min(route_index + sighting_distance_tiles, route.get_count() - 1); i++)
	{
		koord3d i_pos = route.at(i);
		ribi_t::ribi old_dir = calc_direction(route.at(route_index), route.at(min(route.get_count() - 1, route_index + 1)));
		ribi_t::ribi new_dir = calc_direction(route.at(last_index), i_pos);

		const grund_t* gr_new = welt->lookup(i_pos);
		grund_t* gr_bridge = welt->lookup(koord3d(i_pos.x, i_pos.y, i_pos.z + 1));
		if(!gr_bridge)
		{
			gr_bridge = welt->lookup(koord3d(i_pos.x, i_pos.y, i_pos.z + 2));
		}

		slope_t::type old_hang = gr->get_weg_hang();
		slope_t::type new_hang = gr_new ? gr_new->get_weg_hang() : old_hang;

		bool corner			= !(old_dir & new_dir);
		bool different_hill = old_hang != new_hang;
		bool overbridge		= gr_bridge && gr_bridge->ist_bruecke();

		if(w_current->is_diagonal())
		{
			const grund_t* gr_diagonal = welt->lookup(i_pos);
			const schiene_t* sch2 = gr_diagonal ? (schiene_t*)gr_diagonal->get_weg(get_waytype()) : NULL;
			if(sch2 && sch2->is_diagonal())
			{
				ribi_t::ribi adv_dir = calc_direction(route.at(i), route.at(min(i + 1, route.get_count() - 1)));
				ribi_t::ribi mix_dir = adv_dir | new_dir;
				corner = !(mix_dir & old_dir);
			}
			else
			{
				corner = true;
			}
		}

		if(corner || different_hill || overbridge)
		{
			modified_sighting_distance_tiles = max(i - route_index, 1);
			break;
		}
		last_index = i;
	}

	// is there any signal/crossing to be reserved?
	sint32 next_block = (sint32)cnv->get_next_stop_index() - 1;
	last_index = route.get_count() - 1;

	if(next_block > last_index && !exiting_one_train_staff && !(working_method == one_train_staff && next_block >= INVALID_INDEX)) // last_index is a waypoint and we need to keep routing.
	{
		const sint32 route_steps = route_infos.get_element(last_index).steps_from_start - (route_index < route_infos.get_count() ? route_infos.get_element(route_index).steps_from_start : 0);
		bool way_is_free = route_steps >= brake_steps || brake_steps <= 0 || route_steps == 0; // If brake_steps <= 0 and way_is_free == false, weird excess block reservations can occur that cause blockages.
		if(!way_is_free)
		{
			// We need a longer route to decide whether we shall have to start braking
			route_t target_rt;
			schedule_t *schedule = cnv->get_schedule();
			const koord3d start_pos = route.at(last_index);
			uint8 index = schedule->get_current_stop();
			bool rev = cnv->get_reverse_schedule();
			schedule->increment_index(&index, &rev);
			const koord3d next_ziel = schedule->entries[index].pos;

			way_is_free = target_rt.calc_route(welt, start_pos, next_ziel, this, speed_to_kmh(cnv->get_min_top_speed()), cnv->get_highest_axle_load(), cnv->has_tall_vehicles(), cnv->get_tile_length(), welt->get_settings().get_max_route_steps(), cnv->get_weight_summary().weight / 1000) != route_t::valid_route;
			if(!way_is_free)
			{
				if(schedule->entries[schedule->get_current_stop()].reverse == -1)
				{
					schedule->entries[schedule->get_current_stop()].reverse = cnv->check_destination_reverse(NULL, &target_rt);
					linehandle_t line = cnv->get_line();
					if(line.is_bound())
					{
						simlinemgmt_t::update_line(line);
					}
				}
				if(schedule->entries[schedule->get_current_stop()].reverse == 0 && haltestelle_t::get_halt(schedule->entries[schedule->get_current_stop()].pos, get_owner()).is_bound() == false)
				{
					// Extending the route if the convoy needs to reverse would interfere with tile reservations.
					// This convoy can pass a waypoint without reversing/stopping. Append route to the next stop/waypoint.

					// This is still needed after the new (November 2015) system as there are some (possibly
					// transitional) cases in which next_block is still ahead of the calculated route.

					if(rev)
					{
						schedule->advance_reverse();
					}
					else
					{
						schedule->advance();
					}
					cnv->update_route(last_index, target_rt);
					if(working_method == cab_signalling)
					{
						// With cab signalling, the train can always detect the aspect of the next stop signal without a distant (pre-signal).
						way_is_free = block_reserver( &route, last_index, modified_sighting_distance_tiles, next_signal, 0, true, false );
					}
					else if(working_method == absolute_block || working_method == track_circuit_block)
					{
						// With absolute or track circuit block signalling, it is necessary to be within the sighting distance or use a distant or MAS.
						if(next_signal - route_index <= modified_sighting_distance_tiles)
						{
							way_is_free = block_reserver( &route, last_index, modified_sighting_distance_tiles, next_signal, 0, true, false );
						}
					}

					last_index = route.get_count() - 1;
					cnv->set_next_stop_index(next_signal);
					next_block = cnv->get_next_stop_index() - 1;
				}
			}
		}
		// no obstacle in the way => drive on ...
		if (way_is_free || next_block > last_index)
			return true;
	}

	bool do_not_set_one_train_staff = false;
	bool modify_check_tile = false;
	if((next_block < route_index && (working_method == one_train_staff || next_block < route_index - 1)))
	{
		modify_check_tile = true;
		do_not_set_one_train_staff = working_method == one_train_staff;
		set_working_method(working_method != one_train_staff ? drive_by_sight : one_train_staff);
	}

	if(working_method == absolute_block || working_method == track_circuit_block || working_method == drive_by_sight || working_method == token_block || working_method == time_interval || working_method == time_interval_with_telegraph)
	{
		// Check for signals at restrictive aspects within the sighting distance to see whether they can now clear whereas they could not before.
		for(uint16 tiles_to_check = 1; tiles_to_check <= modified_sighting_distance_tiles; tiles_to_check++)
		{
			const uint32 route_count = route.get_count() - 1u;
			const uint32 check_ahead_tile = route_index + tiles_to_check;
			const koord3d tile_to_check_ahead = cnv->get_route()->at(std::min(route_count, check_ahead_tile));
			grund_t *gr_ahead = welt->lookup(tile_to_check_ahead);
			weg_t *way = gr_ahead ? gr_ahead->get_weg(get_waytype()) : NULL;
			if(!way)
			{
				// This may happen if a way has been removed since the route was calculated. Must recalculate the route.
				cnv->suche_neue_route();
				return false;
			}
			uint16 modified_route_index = std::min(check_ahead_tile, cnv->get_route()->get_count() - 1u);
			ribi_t::ribi ribi = ribi_type(cnv->get_route()->at(max(1u,modified_route_index)-1u), cnv->get_route()->at(std::min(cnv->get_route()->get_count()-1u,modified_route_index+1u)));
			signal_t* signal = way->get_signal(ribi);

			// There might be a station signal here, which might be operative despite facing in the opposite direction.
			if(!signal && haltestelle_t::get_halt(tile_to_check_ahead, NULL).is_bound())
			{
				signal = way->get_signal(ribi_t::backward(ribi));
				if(signal && !signal->get_desc()->is_longblock_signal())
				{
					signal = NULL;
				}
			}

			if(signal && (signal->get_state() == signal_t::caution
				|| signal->get_state() == signal_t::preliminary_caution
				|| signal->get_state() == signal_t::advance_caution
				|| ((working_method == time_interval || working_method == time_interval_with_telegraph || working_method == track_circuit_block) && signal->get_state() == signal_t::clear)
				|| ((working_method == time_interval || working_method == time_interval_with_telegraph || working_method == track_circuit_block) && signal->get_state() == signal_t::clear_no_choose)
				|| signal->get_state() == signal_t::caution_no_choose
				|| signal->get_state() == signal_t::preliminary_caution_no_choose
				|| signal->get_state() == signal_t::advance_caution_no_choose
				|| (working_method == token_block && signal->get_state() == signal_t::danger)))
			{
				if (!starting_from_stand && (signal->get_desc()->get_working_method() == token_block || signal->get_desc()->get_working_method() == one_train_staff))
				{
					// These signals should only clear when a train is starting from them.
					break;
				}

				// We come accross a signal at caution: try (again) to free the block ahead.
				const bool ok = block_reserver(cnv->get_route(), route_index, modified_sighting_distance_tiles - (tiles_to_check - 1), next_signal, 0, true, false);
				cnv->set_next_stop_index(next_signal);
				if(!ok && working_method != drive_by_sight && starting_from_stand && this_halt.is_bound())
				{
					return false;
				}
				break;
				// Setting the aspect of the signal is done inside the block reserver
			}
		}
	}

	if(working_method == token_block && w_current->has_sign())
	{
		roadsign_t* rs = gr->find<roadsign_t>();
		if(rs && rs->get_desc()->is_single_way())
		{
			// If we come upon a single way sign, clear the reservation so far, as we know that we are now on unidirectional track again.
			clear_token_reservation(NULL, this, w_current);
		}
	}

	sint32 max_element = cnv->get_route_infos().get_count() - 1u;

	if(w_current->has_signal())
	{
		// With cab signalling, even if we need do nothing else at this juncture, we may need to change the working method.
		const uint16 check_route_index = next_block <= 0 ? 0 : next_block - 1u;
		const uint32 cnv_route_count = cnv->get_route()->get_count() - 1u;
		ribi_t::ribi ribi = next_block < INVALID_INDEX ? ribi_type(cnv->get_route()->at(max(1u, std::min(cnv_route_count, (uint32)check_route_index)) - 1u), cnv->get_route()->at(std::min((uint32)max_element, min(cnv_route_count - 1u, ((uint32)check_route_index + 1u))))) : ribi_t::all;
		signal_t* signal = w_current->get_signal(ribi);

		if (signal && working_method == one_train_staff && starting_from_stand)
		{
			const bool ok = block_reserver(cnv->get_route(), route_index - 1, modified_sighting_distance_tiles, next_signal, 0, true, false);
			cnv->set_next_stop_index(next_signal);
			if (!ok)
			{
				return false;
			}
		}

		else if(signal && (working_method == cab_signalling || working_method == moving_block || (signal->get_desc()->get_working_method() == cab_signalling && working_method != drive_by_sight) || signal->get_desc()->get_working_method() == moving_block))
		{
			if ((working_method == cab_signalling || working_method == moving_block) && signal->get_desc()->get_working_method() != cab_signalling && signal->get_desc()->get_working_method() != moving_block)
			{
				// Transitioning out of a signalling system in which the stopping distance is not based on the position of the signals: run the block reserver again in case it was not run on this signal previously.
				block_reserver(cnv->get_route(), next_block, modified_sighting_distance_tiles, next_signal, 0, true, false, cnv->get_is_choosing());
			}
			set_working_method(signal->get_desc()->get_working_method());
		}

	}

	if(next_block > max_element)
	{
		next_block = cnv->get_next_stop_index() - 1;
	}

	bool call_on = false;

	const sint32 route_steps = brake_steps > 0 && route_index <= route_infos.get_count() - 1 ? cnv->get_route_infos().get_element((next_block > 0 ? std::min(next_block - 1, max_element) : 0)).steps_from_start - cnv->get_route_infos().get_element(route_index).steps_from_start : -1;
	if(route_steps <= brake_steps || brake_steps < 0)
	{
		sint32 check_tile = modify_check_tile ? route_index + 1 : next_block; // This might otherwise end up as -1 without the ? block, which would be an attempt to check the *previous* tile, which would be silly.
		if((check_tile > 0) && check_tile >= cnv->get_route()->get_count()) // Checking if check_tile > 0 is necessary because get_count() is a uint32
		{
			check_tile = cnv->get_route()->get_count() - 1;
		}
		koord3d block_pos = cnv->get_route()->at(check_tile);

		grund_t *gr_next_block = welt->lookup(block_pos);
		const schiene_t *sch1 = gr_next_block ? (const schiene_t *)gr_next_block->get_weg(get_waytype()) : NULL;
		if(w_current == NULL)
		{
			// way (weg) not existent (likely destroyed)
			cnv->suche_neue_route();
			return false;
		}

		// next check for signal
		if(sch1 && sch1->has_signal())
		{
			const uint16 check_route_index = next_block <= 0 ? 0 : next_block - 1u;
			max_element = std::min((uint32)max_element, cnv->get_route()->get_count() - 1u);
			ribi_t::ribi ribi = ribi_type(cnv->get_route()->at(max(1u, (std::min((uint32)max_element, (uint32)check_route_index))) - 1u), cnv->get_route()->at(std::min((uint32)max_element, check_route_index + 1u)));
			signal_t* signal = sch1->get_signal(ribi);

			if(signal && ((signal->get_desc()->get_working_method() == cab_signalling) || (check_tile - route_index <= sighting_distance_tiles)))
			{
				working_method_t old_working_method = working_method;
				if(working_method != token_block && working_method != one_train_staff)
				{
					if(signal->get_desc()->get_working_method() != one_train_staff || (!do_not_set_one_train_staff && (signal->get_pos() == get_pos()) && (signal->get_state() != roadsign_t::call_on)))
					{
						set_working_method(signal->get_desc()->get_working_method());
						if (signal->get_desc()->get_working_method() == one_train_staff)
						{
							cnv->set_last_signal_pos(signal->get_pos());
						}
					}
				}

				if(working_method == cab_signalling
					|| signal->get_desc()->is_pre_signal()
					|| ((working_method == token_block
					|| working_method == time_interval
					|| working_method == time_interval_with_telegraph
					|| working_method == track_circuit_block
					|| working_method == absolute_block) && next_block - route_index <= max(modified_sighting_distance_tiles - 1, 1))
					|| (working_method == one_train_staff && shortest_distance(get_pos().get_2d(), signal->get_pos().get_2d()) < 2))
 				{
					// Brake for the signal unless we can see it somehow. -1 because this is checked on entering the tile.
					const bool allow_block_reserver = ((working_method != time_interval && working_method != time_interval_with_telegraph) || !signal->get_no_junctions_to_next_signal() || signal->get_desc()->is_choose_sign() || signal->get_state() != roadsign_t::danger) && ((signal->get_desc()->get_working_method() != one_train_staff && signal->get_desc()->get_working_method() != token_block) || route_index == next_block);
					if(allow_block_reserver && !block_reserver(cnv->get_route(), next_block, modified_sighting_distance_tiles, next_signal, 0, true, false, cnv->get_is_choosing()))
					{
						restart_speed = 0;
						signal->set_state(roadsign_t::danger);
						if (old_working_method == drive_by_sight)
						{
							set_working_method(old_working_method);
						}
						if(starting_from_stand && (working_method == absolute_block || working_method == track_circuit_block || working_method == cab_signalling))
						{
							// Check for permissive working. Only check here, as the convoy must be brought to a stand before a call on.
							if(signal->get_desc()->get_permissive() && signal->get_no_junctions_to_next_signal())
							{
								// Permissive working allowed: call on.
								signal->set_state(roadsign_t::call_on);
								set_working_method(drive_by_sight);
								call_on = true;
							}
						}
						cnv->set_next_stop_index(next_signal == INVALID_INDEX ? next_block : next_signal);
						if(signal->get_state() != roadsign_t::call_on)
						{
							cnv->set_is_choosing(false);
							return cnv->get_next_stop_index() > route_index;
						}
					}
					else
					{
						if(allow_block_reserver)
						{
							if ((next_signal == next_block) && (next_signal == route_index - 1))
							{
								return false;
							}
							else
							{
								cnv->set_next_stop_index(next_signal);
							}
						}
						else
						{
							return cnv->get_next_stop_index() > route_index;
						}
					}
					if(!signal->get_desc()->is_choose_sign())
					{
						cnv->set_is_choosing(false);
					}
				}
			}
		}
		else if(working_method == one_train_staff)
		{
			// Check for non-adjacent one train staff cabinets - these should be ignored.
			const koord3d first_pos = cnv->get_last_signal_pos();
			if (shortest_distance(get_pos().get_2d(), first_pos.get_2d()) < 3)
			{
				cnv->set_next_stop_index(next_signal);
			}
			else if(first_pos != koord3d::invalid)
			{
				nonadjacent_one_train_staff_cabinet = true;
			}
		}
	}

	if(w_current->has_signal())
	{
		if(working_method != one_train_staff || (welt->lookup(w_current->get_pos())->find<signal_t>()->get_desc()->get_working_method() == one_train_staff && !nonadjacent_one_train_staff_cabinet))
		{
			cnv->set_last_signal_pos(w_current->get_pos());
		}
	}

	// This is necessary if a convoy is passing through a passing loop without stopping so as to set token block working
	// properly on the token block signal at the exit of the loop.
	if(welt->lookup(get_pos())->get_weg(get_waytype())->has_signal())
	{
		signal_t* signal = get_weg()->get_signal(ribi);

		if(signal && signal->get_desc()->get_working_method() == token_block)
		{
			set_working_method(token_block);
		}
	}
	if(working_method == drive_by_sight || working_method == moving_block)
	{
		bool ok = block_reserver(cnv->get_route(), route_index, modified_sighting_distance_tiles, next_signal, 0, true, false, false, false, false, false, brake_steps, (uint16)65530U, call_on);
		ok |= route_index == route.get_count() || next_signal > route_index;
		if (!ok && working_method == one_train_staff)
		{
			set_working_method(drive_by_sight);
		}
		cnv->set_next_stop_index(next_signal);
		if (exiting_one_train_staff && get_working_method() == one_train_staff)
		{
			set_working_method(drive_by_sight);
		}
		return ok;
	}

	return true;
}

void rail_vehicle_t::set_working_method(working_method_t value)
{
	if (working_method == one_train_staff && value != one_train_staff)
	{
		unreserve_in_rear();
		cnv->reserve_own_tiles();
	}

	working_method = value;
}

void rail_vehicle_t::find_next_signal(route_t* route, uint16 start_index, uint16 &next_signal_index)
{
	if(start_index >= route->get_count())
	{
		// Cannot start reserving beyond the end of the route.
		cnv->set_next_reservation_index(max(route->get_count(), 1) - 1);
		return;
	}

	sint32 count = 0;
	uint32 i = start_index;
	next_signal_index = INVALID_INDEX;
	signal_t* signal = NULL;
	koord3d pos = route->at(start_index);
	for (; count >= 0 && i < route->get_count(); i++)
	{
		pos = route->at(i);
		grund_t *gr = welt->lookup(pos);
		schiene_t* sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;

		if (sch1 && sch1->has_signal())
		{
			ribi_t::ribi ribi = ribi_type(route->at(max(1u, i) - 1u), route->at(min(route->get_count() - 1u, i + 1u)));
			signal = gr ? gr->get_weg(get_waytype())->get_signal(ribi) : NULL;
			if (signal && !signal->get_desc()->is_pre_signal())
			{
				next_signal_index = i;
				return;
			}
		}
	}
}

/*
 * reserves or un-reserves all blocks and returns whether it succeeded.
 * if count is larger than 1, (and defined) maximum welt->get_settings().get_max_choose_route_steps() tiles will be checked
 * (freeing or reserving a choose signal path)
 * if (!reserve && force_unreserve) then un-reserve everything till the end of the route
 * @author prissi
 */
sint32 rail_vehicle_t::block_reserver(route_t *route, uint16 start_index, uint16 modified_sighting_distance_tiles, uint16 &next_signal_index, int count, bool reserve, bool force_unreserve, bool is_choosing, bool is_from_token, bool is_from_starter, bool is_from_directional, uint32 brake_steps, uint16 first_one_train_staff_index, bool from_call_on, bool *break_loop)
{
	bool success = true;
	sint32 max_tiles = 2 * welt->get_settings().get_max_choose_route_steps(); // max tiles to check for choosesignals

	slist_tpl<grund_t *> signs;	// switch all signals on the route
	slist_tpl<signal_t*> pre_signals;
	slist_tpl<signal_t*> combined_signals;

	if(start_index >= route->get_count())
	{
		// Cannot start reserving beyond the end of the route.
		cnv->set_next_reservation_index(max(route->get_count(), 1) - 1);
		return 0;
	}

	bool starting_at_signal = false;
	if(route->at(start_index) == get_pos() && reserve && start_index < route->get_count() - 1)
	{
		starting_at_signal = true;
	}

	// perhaps find an early platform to stop at
	uint16 platform_size_needed = 0;
	uint16 platform_size_found = 0;
	ribi_t::ribi ribi_last = ribi_t::none;
	halthandle_t dest_halt = halthandle_t();
	uint16 early_platform_index = INVALID_INDEX;
	uint16 first_stop_signal_index = INVALID_INDEX;
	const schedule_t* schedule = NULL;
	if(cnv != NULL)
	{
		schedule = cnv->get_schedule();
	}
	else
	{
		return 0;
	}
	bool do_early_platform_search =	schedule != NULL
		&& (schedule->is_mirrored() || schedule->is_bidirectional())
		&& schedule->get_current_entry().minimum_loading == 0;

	if(do_early_platform_search)
	{
		platform_size_needed = cnv->get_tile_length();
		dest_halt = haltestelle_t::get_halt(cnv->get_schedule()->get_current_entry().pos, cnv->get_owner());
	}
	if(!reserve)
	{
		cnv->set_next_reservation_index(start_index);
	}

	// find next block segment enroute
	uint32 i = start_index - (count == 100001 ? 1 : 0);
	next_signal_index = INVALID_INDEX;
	bool unreserve_now = false;

	enum set_by_distant
	{
		uninitialised, not_set, set
	};

	koord3d pos = route->at(start_index);
	koord3d last_pos = start_index > 0 ? route->at(start_index - 1) : pos;
	const halthandle_t this_halt = haltestelle_t::get_halt(pos, get_owner());
	halthandle_t last_step_halt;
	uint16 this_stop_signal_index = INVALID_INDEX;
	uint16 last_pre_signal_index = INVALID_INDEX;
	uint16 last_stop_signal_index = INVALID_INDEX;
	uint16 restore_last_stop_signal_index = INVALID_INDEX;
	koord3d last_stop_signal_pos = koord3d::invalid;
	uint16 last_longblock_signal_index = INVALID_INDEX;
	uint16 last_combined_signal_index = INVALID_INDEX;
	uint16 last_choose_signal_index = INVALID_INDEX;
	uint16 last_token_block_signal_index = INVALID_INDEX;
	uint16 last_bidirectional_signal_index = INVALID_INDEX;
	uint16 last_stop_signal_before_first_bidirectional_signal_index = INVALID_INDEX;
	uint16 last_non_directional_index = start_index;
	uint16 first_oneway_sign_index = INVALID_INDEX;
	uint16 last_oneway_sign_index = INVALID_INDEX;
	uint16 last_station_tile = INVALID_INDEX;
	uint16 first_double_block_signal_index = INVALID_INDEX;
	uint32 stop_signals_since_last_double_block_signal = 0;
	halthandle_t stop_at_station_signal;
	const halthandle_t destination_halt = haltestelle_t::get_halt(route->at(route->get_count() - 1), get_owner());
	bool do_not_clear_distant = false;
	working_method_t next_signal_working_method = working_method;
	working_method_t old_working_method = working_method;
	set_by_distant working_method_set_by_distant_only = uninitialised;
	bool next_signal_protects_no_junctions = false;
	koord3d signalbox_last_distant_signal = koord3d::invalid;
	bool last_distant_signal_was_intermediate_block = false;
	sint32 remaining_aspects = -1;
	sint32 choose_return = 0;
	bool reached_end_of_loop = false;
	bool no_junctions_to_next_signal = true;
	signal_t* previous_signal = NULL;
	bool end_of_block = false;
	bool not_entirely_free = false;
	bool directional_only = is_from_directional;
	bool previous_telegraph_directional = false;
	bool directional_reservation_succeeded = true;
	bool one_train_staff_loop_complete = false;
	bool this_signal_is_nonadjacent_one_train_staff_cabinet = false;
	bool time_interval_reservation = false;
	bool reserving_beyond_a_train = false;
	enum ternery_uncertainty {
		is_true, is_false, is_uncertain
	};
	ternery_uncertainty previous_time_interval_reservation = is_uncertain;
	bool set_first_station_signal = false;
	bool time_interval_after_double_block = false;
	const uint32 station_signals = this_halt.is_bound() ? this_halt->get_station_signals_count() : 0;
	signal_t* first_station_signal = NULL;
	enum station_signal_status { none, forward, inverse };
	station_signal_status station_signal = station_signal_status::none;
	uint32 steps_so_far = 0;
	roadsign_t::signal_aspects next_time_interval_state = roadsign_t::danger;
	roadsign_t::signal_aspects first_time_interval_state = roadsign_t::advance_caution; // A time interval signal will never be in advance caution, so this is a placeholder to indicate that this value has not been set.
	signal_t* station_signal_to_clear_for_entry = NULL;

	if(working_method == drive_by_sight)
	{
		const sint32 max_speed_drive_by_sight = welt->get_settings().get_max_speed_drive_by_sight();
		if(max_speed_drive_by_sight && get_desc()->get_waytype() != tram_wt)
		{
			cnv->set_maximum_signal_speed(max_speed_drive_by_sight);
		}
	}

	for( ; success && count >= 0 && i < route->get_count(); i++)
	{
		station_signal = none;
		this_stop_signal_index = INVALID_INDEX;
		last_pos = pos;
		pos = route->at(i);
		if(!directional_only)
		{
			last_non_directional_index = i;
		}
		grund_t *gr = welt->lookup(pos);
		schiene_t* sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;

		if (sch1 == NULL && reserve)
		{
			if (i < route->get_count() - 1)
			{
				// A way tile has been deleted; the route needs recalculating.
				cnv->suche_neue_route();
			}
			break;
		}
		// we un-reserve also nonexistent tiles! (may happen during deletion)

		if (reserve && sch1->is_diagonal())
		{
			steps_so_far += diagonal_vehicle_steps_per_tile;
		}
		else
		{
			steps_so_far += VEHICLE_STEPS_PER_TILE;
		}

		if((working_method == drive_by_sight && (i - (start_index - 1)) > modified_sighting_distance_tiles) && (!this_halt.is_bound() || (haltestelle_t::get_halt(pos, get_owner())) != this_halt))
		{
			// In drive by sight mode, do not reserve further than can be seen (taking account of obstacles);
			// but treat signals at the end of the platform as a signal at which the train is now standing.
			next_signal_index = i;
			break;
		}

		if(working_method == moving_block && !directional_only && last_choose_signal_index >= INVALID_INDEX && !is_choosing)
		{
			next_signal_index = i;

			// Do not reserve further than the braking distance in moving block mode.
			if(steps_so_far > brake_steps + (VEHICLE_STEPS_PER_TILE * 2))
			{
				break;
			}
		}

		if(is_choosing && welt->get_settings().get_max_choose_route_steps())
		{
			max_tiles--;
			if(max_tiles < 0)
			{
				break;
			}
		}

		if(reserve)
		{
			if(sch1->is_junction())
			{
				no_junctions_to_next_signal = false;
			}
			if(sch1->is_crossing() && !directional_only)
			{
				crossing_t* cr = gr->find<crossing_t>(2);
				if(cr)
				{
					// Only reserve if the crossing is clear.
					if(i < first_stop_signal_index)
					{
						success = cr->request_crossing(this, true) || next_signal_working_method == one_train_staff;
					}
					else
					{
						not_entirely_free = !cr->request_crossing(this, true) && next_signal_working_method != one_train_staff;
						if(not_entirely_free && (working_method != time_interval || ((i - (start_index - 1)) < modified_sighting_distance_tiles)))
						{
							count --;
							next_signal_index = last_stop_signal_index;
						}
					}
				}
			}

			roadsign_t* rs = gr->find<roadsign_t>();
			ribi_t::ribi ribi = ribi_type(route->at(max(1u,i)-1u), route->at(min(route->get_count()-1u,i+1u)));

			if(working_method == moving_block)
			{
				// Continue in moving block if in range of the most recent moving block signal.
				const grund_t* gr_last_signal = welt->lookup(cnv->get_last_signal_pos());
				const signal_t* sg = gr_last_signal ? gr_last_signal->find<signal_t>() : NULL;
				if(!sg || sg->get_desc()->get_max_distance_to_signalbox() < shortest_distance(pos.get_2d(), cnv->get_last_signal_pos().get_2d()))
				{
					// Out of range of the moving block beacon/signal; revert to drive by sight
					set_working_method(drive_by_sight);
					break;
				}
			}

			halthandle_t check_halt = haltestelle_t::get_halt(pos, NULL);

			if(check_halt.is_bound() && (check_halt == this_halt))
			{
				last_station_tile = i;
			}

			if (rs && rs->get_desc()->is_single_way())
			{
				if (first_oneway_sign_index >= INVALID_INDEX)
				{
					if (directional_only)
					{
						// Stop a directional reservation when a one way sign is encountered
						count--;
						if (is_from_directional)
						{
							reached_end_of_loop = true;
							if (break_loop)
							{
								*break_loop = true;
							}
						}
						next_signal_index = last_stop_signal_index;
					}
					if (last_bidirectional_signal_index < INVALID_INDEX || last_longblock_signal_index < INVALID_INDEX)
					{
						first_oneway_sign_index = i;
						last_oneway_sign_index = i;
					}
				}
				else
				{
					last_oneway_sign_index = i;
				}
			}

			const uint32 station_signals_ahead = check_halt.is_bound() ? check_halt->get_station_signals_count() : 0;
			bool station_signals_in_advance = false;
			const bool signal_here = sch1->has_signal() || station_signals;
			bool station_signal_to_clear_only = false;

			if(!signal_here && station_signals_ahead)
			{
				halthandle_t destination_halt = haltestelle_t::get_halt(cnv->get_schedule()->get_current_entry().pos, get_owner());
				station_signals_in_advance = check_halt != this_halt && check_halt != destination_halt;
				if (check_halt != this_halt && check_halt == destination_halt)
				{
					station_signal_to_clear_only = true;
				}
			}

			if(signal_here || station_signals_in_advance || station_signal_to_clear_only)
			{
				signal_t* signal = NULL;
				halthandle_t station_signal_halt;
				uint32 check_station_signals = 0;

				if(station_signals && last_station_tile >= i)
				{
					// This finds a station signal on the station at which the train is currently standing
					check_station_signals = station_signals;
					station_signal_halt = this_halt;
				}
				else if(station_signals_ahead || station_signal_to_clear_only)
				{
					// This finds a station signal in a station in advance
					check_station_signals = station_signals_ahead;
					station_signal_halt = check_halt;
				}

				if(check_station_signals)
				{
					// This is a signal that applies to the whole station for the relevant direction. Find it if it is there.

					for(uint32 k = 0; k < check_station_signals; k ++)
					{
						grund_t* gr_check = welt->lookup(station_signal_halt->get_station_signal(k));
						if(gr_check)
						{
							weg_t* way_check = gr_check->get_weg(get_waytype());
							signal = way_check ? way_check->get_signal(ribi) : NULL;
							if(signal)
							{
								station_signal = forward;
								break;
							}
							else
							{
								// Check the opposite direction as station signals work in the exact opposite direction
								ribi_t::ribi ribi_backwards = ribi_t::backward(ribi);
								signal = gr_check->get_weg(get_waytype())->get_signal(ribi_backwards);
								if(signal)
								{
									station_signal = inverse;
									break;
								}
							}
						}
					}
					if (station_signal_to_clear_only)
					{
						station_signal_to_clear_for_entry = signal;
						goto station_signal_to_clear_only_point;
					}
				}
				else
				{
					// An ordinary signal on plain track
					signal = gr->get_weg(get_waytype())->get_signal(ribi);
					// But, do not select a station signal here, as this will lead to capricious behaviour depending on whether the train passes the track on which the station signal happens to be situated or not.
					if(signal && check_halt.is_bound() && signal->get_desc()->is_station_signal())
					{
						signal = NULL;
					}
				}

				if(signal && (signal != first_station_signal || !set_first_station_signal))
				{
					if(station_signal)
					{
						first_station_signal = signal;
						set_first_station_signal = true;
					}

					if(!directional_only && (signal->get_desc()->is_longblock_signal() || signal->is_bidirectional()))
					{
						last_bidirectional_signal_index = i;
					}

					if(previous_signal && !previous_signal->get_desc()->is_choose_sign())
					{
						previous_signal->set_no_junctions_to_next_signal(no_junctions_to_next_signal);
					}

					previous_signal = signal;
					if(working_method != time_interval_with_telegraph || last_longblock_signal_index >= INVALID_INDEX || signal->get_desc()->get_working_method() != time_interval)
					{
						// In time interval with telegraph method, do not change the working method to time interval when reserving beyond a longblock signal in the time interval with telegraph method.
						working_method_t nwm = signal->get_desc()->get_working_method();
						if (first_stop_signal_index < INVALID_INDEX && (nwm == track_circuit_block || nwm == cab_signalling) && next_signal_working_method != cab_signalling && next_signal_working_method != track_circuit_block && remaining_aspects < 0)
						{
							// Set remaining_aspects correctly when transitioning to track circuit block or cab signalling from another working method
							remaining_aspects = signal->get_desc()->get_aspects();
						}

						next_signal_working_method = nwm;
					}

					next_signal_protects_no_junctions = signal->get_no_junctions_to_next_signal();

					if (destination_halt == check_halt)
					{
						next_signal_protects_no_junctions = false;
					}
					if(working_method == drive_by_sight && sch1->can_reserve(cnv->self, ribi) && (signal->get_pos() != cnv->get_last_signal_pos() || signal->get_desc()->get_working_method() != one_train_staff))
					{
						if (signal->get_desc()->get_working_method() == one_train_staff && pos != get_pos())
						{
							// Do not try to reserve beyond a one train staff cabinet unless the train is at the cabinet.
							next_signal_index = i;
							count --;
						}
						else
						{
							set_working_method(next_signal_working_method);
							if (signal->get_desc()->is_pre_signal() && working_method_set_by_distant_only == uninitialised)
							{
								working_method_set_by_distant_only = set;
							}
							else if (!signal->get_desc()->is_pre_signal())
							{
								working_method_set_by_distant_only = not_set;
							}
						}
					}

					if(next_signal_working_method == one_train_staff && first_one_train_staff_index == INVALID_INDEX)
					{
						// Do not set this when already in the one train staff working method unless this is an adjacent cabinet
						const koord3d first_pos = cnv->get_last_signal_pos();
						if (working_method != one_train_staff || shortest_distance(pos.get_2d(), first_pos.get_2d()) < 3)
						{
							first_one_train_staff_index = i;
						}
						else if (working_method == one_train_staff && first_pos != koord3d::invalid)
						{
							this_signal_is_nonadjacent_one_train_staff_cabinet = true;
						}
					}

					if(next_signal_working_method == one_train_staff && (first_one_train_staff_index != i || (is_from_token && first_one_train_staff_index < INVALID_INDEX)))
					{
						// A second one train staff cabinet. Is this the same as or a neighbour of the first?
						const koord3d first_pos = cnv->get_last_signal_pos();
						if(shortest_distance(pos.get_2d(), first_pos.get_2d()) < 3)
						{
							one_train_staff_loop_complete = true;
						}
						else if(first_pos != koord3d::invalid)
						{
							// Any non-adjacent one train staff cabinets should be ignored,
							// or else the one train staff method effectively becomes token block.
							this_signal_is_nonadjacent_one_train_staff_cabinet = true;
						}
					}

					if((next_signal_working_method == drive_by_sight && working_method != drive_by_sight) || one_train_staff_loop_complete || (((working_method != one_train_staff && (first_one_train_staff_index < INVALID_INDEX && first_one_train_staff_index > start_index))) && (first_double_block_signal_index == INVALID_INDEX || first_double_block_signal_index != last_stop_signal_index)))
					{
						// Do not reserve through end of signalling signs (unless already in drive by side mode) or one train staff cabinets
						next_signal_index = i;
						count --;
					}

					if(time_interval_after_double_block)
					{
						next_signal_index = i;
						count --;
					}

					if(!signal->get_desc()->is_pre_signal() && !this_signal_is_nonadjacent_one_train_staff_cabinet) // Stop signal or multiple aspect signal
					{
						if (signal->get_desc()->get_double_block() == true && first_double_block_signal_index == INVALID_INDEX)
						{
							first_double_block_signal_index = i;
							stop_signals_since_last_double_block_signal = 0;
						}
						else if(first_double_block_signal_index < INVALID_INDEX)
						{
							stop_signals_since_last_double_block_signal++;
						}

						if(signal->get_desc()->get_working_method() == time_interval || signal->get_desc()->get_working_method() == time_interval_with_telegraph)
						{
							// Note that this gives the pure time interval state, ignoring whether the signal protects any junctions.
							sint64 last_passed;
							if(station_signal && station_signal_halt.is_bound())
							{
								last_passed = station_signal_halt->get_train_last_departed(ribi);
							}
							else
							{
								last_passed = signal->get_train_last_passed();
							}
							const sint64 caution_interval_ticks = welt->get_seconds_to_ticks(welt->get_settings().get_time_interval_seconds_to_caution());
							const sint64 clear_interval_ticks =  welt->get_seconds_to_ticks(welt->get_settings().get_time_interval_seconds_to_clear());
							const sint64 ticks = welt->get_ticks();

							if(last_passed + caution_interval_ticks > ticks)
							{
								next_time_interval_state = roadsign_t::danger;
							}
							else if(last_passed + clear_interval_ticks > ticks)
							{
								next_time_interval_state = station_signal == inverse ? roadsign_t::caution : roadsign_t::caution_no_choose;
							}
							else
							{
								next_time_interval_state = station_signal == inverse ? roadsign_t::clear : roadsign_t::clear_no_choose;
							}

							if(first_time_interval_state == roadsign_t::advance_caution)
							{
								first_time_interval_state = next_time_interval_state;
							}

							// Sometimes the signal's state might be incorrect (e.g. if it was formerly incorrectly registered as protecting a junction),
							// so set it here to correct in so far as necessary (but only if it is on plain track, otherwise this will need to be set
							// separately).
							if (signal->get_no_junctions_to_next_signal())
							{
								signal->set_state(next_time_interval_state);
							}

							if (first_double_block_signal_index < INVALID_INDEX && first_double_block_signal_index == last_stop_signal_index)
							{
								// We are checking here whether to clear the double block stop signal before a time interval signal.
								if (next_time_interval_state == roadsign_t::danger)
								{
									next_signal_index = first_double_block_signal_index;
									count--;
									not_entirely_free = true;
									success = false;
								}
								else
								{
									time_interval_after_double_block = true;
								}
							}
						}

						if(last_bidirectional_signal_index >= INVALID_INDEX)
						{
							last_stop_signal_before_first_bidirectional_signal_index = i;
						}
						if((count || pre_signals.get_count() || next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling || ((first_stop_signal_index >= INVALID_INDEX && next_signal_working_method != time_interval && next_signal_working_method != time_interval_with_telegraph) || !next_signal_protects_no_junctions)) && !directional_only)
						{
							signs.append(gr);
						}
						if (pre_signals.get_count() || combined_signals.get_count())
						{
							// Do not reserve after a stop signal not covered by a distant or combined signal
							// (or multiple aspect signals with the requisite number of aspects).
							if(next_signal_working_method == absolute_block || next_signal_working_method == token_block)
							{
								if(last_combined_signal_index < INVALID_INDEX && pre_signals.empty())
								{
									// Treat the last combined signal as a distant signal.
									// Check whether it can be used from the current box.
									signal_t* last_combined_signal = combined_signals.back();
									const signalbox_t* sb = NULL;
									const grund_t* gr_signalbox = welt->lookup(signal->get_signalbox());
									if(gr_signalbox)
									{
										const gebaeude_t* gb = gr_signalbox->get_building();
										if(gb && gb->get_tile()->get_desc()->is_signalbox())
										{
											sb = static_cast<const signalbox_t *>(gb);
										}
									}
									if(sb && sb->can_add_signal(signal) && !directional_only)
									{
										// This is compatible: treat it as a distant signal.
										pre_signals.append(last_combined_signal);
										last_pre_signal_index = i;
										signalbox_last_distant_signal = signal->get_signalbox();
										last_distant_signal_was_intermediate_block = signal->get_desc()->get_intermediate_block();
										signs.append_unique(gr);
									}
									else if (first_double_block_signal_index != last_stop_signal_index)
									{
										// The last combined signal is not compatible with this signal's signalbox:
										// do not treat it as a distant signal.
										count --;
										end_of_block = true;
										if(!is_from_token)
										{
											next_signal_index = last_stop_signal_index;
										}
									}
								}
								else if(signalbox_last_distant_signal != signal->get_signalbox() ||
									// XOR - allow intermediate block in advance or in rear, but do not allow them to be chained.
									((signal->get_desc()->get_intermediate_block() ^ last_distant_signal_was_intermediate_block) && first_double_block_signal_index != last_stop_signal_index))
								{
									count --;
									end_of_block = true;
								}
							}
						}

						if(!directional_only && (next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling) && remaining_aspects <= 2 && remaining_aspects >= 0)
						{
							if((last_bidirectional_signal_index < INVALID_INDEX || last_longblock_signal_index < INVALID_INDEX) && first_oneway_sign_index >= INVALID_INDEX)
							{
								directional_only = true;
								last_stop_signal_index = i;
								last_stop_signal_pos = pos;
							}
							else if(first_double_block_signal_index != last_stop_signal_index && !(last_bidirectional_signal_index < INVALID_INDEX && last_oneway_sign_index < last_bidirectional_signal_index))
							{
								count --;
							}

							if(first_double_block_signal_index != last_stop_signal_index)
							{
								end_of_block = true;
							}
						}

						if(stop_at_station_signal.is_bound() && stop_at_station_signal != check_halt)
						{
							// We have gone beyond the end of the station where we need to be stopping for the station signal.
							next_signal_index = i - 1;
							count --;
							last_stop_signal_index = i - 1;
							last_stop_signal_pos = route->at(i - 1);
							end_of_block = true;
						}

						if(!directional_only && (next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph) && last_longblock_signal_index < INVALID_INDEX && last_longblock_signal_index < this_stop_signal_index)
						{
							if(next_signal_working_method == time_interval_with_telegraph)
							{
								directional_only = true;
								next_signal_index = i;
							}
							else if(first_double_block_signal_index != last_stop_signal_index)
							{
								next_signal_index = i;
								count --;
							}
							last_stop_signal_index = i;
							last_stop_signal_pos = pos;
							end_of_block = true;
						}

						if(!directional_only && next_signal_working_method == time_interval_with_telegraph && signal->get_no_junctions_to_next_signal() && signal->get_desc()->is_longblock_signal() && (!check_halt.is_bound() || check_halt->get_station_signals_count() == 0))
						{
							directional_only = true;
						}

						if((!is_from_token || first_stop_signal_index >= INVALID_INDEX) && !directional_only)
						{
							next_signal_index = i;
						}

						if(signal->get_desc()->is_combined_signal())
						{
							// We cannot know yet whether it ought to be treated as a distant
							// without knowing whether it is close enough to the signalbox
							// controlling the next stop signal on this route.
							combined_signals.append(signal);
							last_combined_signal_index = i;
						}
						else if((!directional_only && (next_signal_working_method == moving_block && working_method != moving_block)) || ((next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling) && remaining_aspects >= 0 && remaining_aspects <= 2))
						{
							// If there are no more caution aspects, or this is a transition to moving block signalling do not reserve any further at this juncture.
							if(((last_bidirectional_signal_index < INVALID_INDEX || last_longblock_signal_index < INVALID_INDEX) && (first_oneway_sign_index >= INVALID_INDEX || (!directional_only && (last_bidirectional_signal_index < INVALID_INDEX && last_oneway_sign_index < last_bidirectional_signal_index)))) || (next_signal_working_method == moving_block && working_method != moving_block))
							{
								directional_only = true;
								restore_last_stop_signal_index = last_stop_signal_index;
								last_stop_signal_index = i;
							}
							else if(first_double_block_signal_index != last_stop_signal_index)
							{
								count --;
							}
						}

						if(working_method == moving_block && ((last_bidirectional_signal_index < INVALID_INDEX || last_longblock_signal_index < INVALID_INDEX) && first_oneway_sign_index >= INVALID_INDEX))
						{
							directional_only = true;
						}

						if((working_method == time_interval || working_method == time_interval_with_telegraph) && last_pre_signal_index < INVALID_INDEX && !next_signal_protects_no_junctions)
						{
							const koord3d last_pre_signal_pos = route->at(last_pre_signal_index);
							grund_t *gr_last_pre_signal = welt->lookup(last_pre_signal_pos);
							signal_t* last_pre_signal = gr_last_pre_signal->find<signal_t>();
							if(last_pre_signal)
							{
								pre_signals.append(last_pre_signal);
							}
						}

						if(signal->get_desc()->is_choose_sign())
						{
							last_choose_signal_index = i;
							if(directional_only && last_bidirectional_signal_index < i && first_double_block_signal_index != last_stop_signal_index)
							{
								// If a choose signal is encountered, assume that there is no need to check whether this is free any further.
								count --;
								directional_reservation_succeeded = true;
							}
						}


						if(first_stop_signal_index >= INVALID_INDEX)
						{
							first_stop_signal_index = i;
							if(next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph)
							{
								if(next_signal_protects_no_junctions)
								{
									if(signal->get_state() == roadsign_t::danger)
									{
										count --;
									}
									else if(signal->get_state() == roadsign_t::caution || signal->get_state() == roadsign_t::caution_no_choose)
									{
										// Half line speed allowed for caution indications on time interval stop signals on straight track
										cnv->set_maximum_signal_speed(min(kmh_to_speed(sch1->get_max_speed()) / 2, signal->get_desc()->get_max_speed() / 2));
									}
									else if(signal->get_state() == roadsign_t::clear || signal->get_state() == roadsign_t::clear_no_choose)
									{
										cnv->set_maximum_signal_speed(signal->get_desc()->get_max_speed());
									}
								}
								else // Junction signal
								{
									// Must proceed with great caution in basic time interval and some caution even with a telegraph (see LT&S Signalling, p. 5)
									if(signal->get_desc()->is_longblock_signal() && (station_signal || next_signal_working_method == time_interval_with_telegraph))
									{
										if(next_time_interval_state == roadsign_t::danger || (last_longblock_signal_index < INVALID_INDEX && i > first_stop_signal_index))
										{
											next_signal_index = i;
											count --;
										}
										else if(next_time_interval_state == roadsign_t::caution || next_time_interval_state == roadsign_t::caution_no_choose)
										{
											cnv->set_maximum_signal_speed(min(kmh_to_speed(sch1->get_max_speed()) / 2, signal->get_desc()->get_max_speed() / 2));
										}
										else if(next_time_interval_state == roadsign_t::clear || next_time_interval_state == roadsign_t::clear_no_choose)
										{
											cnv->set_maximum_signal_speed(signal->get_desc()->get_max_speed());
										}
									}
									else if(next_signal_working_method == time_interval)
									{
										cnv->set_maximum_signal_speed(min(kmh_to_speed(sch1->get_max_speed()) / 2, signal->get_desc()->get_max_speed() / 2));
									}
									else // With telegraph
									{
										// The same as for caution
										cnv->set_maximum_signal_speed(min(kmh_to_speed(sch1->get_max_speed()) / 2, signal->get_desc()->get_max_speed() / 2));
									}
								}
							}
							else
							{
								cnv->set_maximum_signal_speed(signal->get_desc()->get_max_speed());
							}
							if(next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling)
							{
								remaining_aspects = signal->get_desc()->get_aspects();
							}
							if(is_from_token && (working_method == token_block || working_method == absolute_block || next_signal_working_method == token_block || next_signal_working_method == absolute_block) && first_double_block_signal_index != last_stop_signal_index)
							{
								// Do not try to reserve beyond the first signal if this is called recursively from a token block signal.
								count --;
							}
						}
						else if((next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling) && !directional_only)
						{
							remaining_aspects = min(remaining_aspects - 1, signal->get_desc()->get_aspects());
						}
						else if(next_signal_working_method == absolute_block && pre_signals.empty() && first_double_block_signal_index != last_stop_signal_index)
						{
							// No distant signals: just reserve one block beyond the first stop signal and no more (unless the last signal is a double block signal).
							count --;
						}
						else if(next_signal_working_method == time_interval && (last_longblock_signal_index >= INVALID_INDEX || !next_signal_protects_no_junctions) && signal->get_state() == roadsign_t::danger && next_signal_protects_no_junctions && pre_signals.empty())
						{
							next_time_interval_state = signal->get_state();
							if(station_signal)
							{
								stop_at_station_signal = check_halt;
							}
							else
							{
								next_signal_index = i;
								count --;
							}
						}
						else if(next_signal_working_method == time_interval_with_telegraph || next_signal_working_method == time_interval)
						{
							if(signal->get_desc()->is_longblock_signal() && (last_longblock_signal_index < INVALID_INDEX || working_method == time_interval_with_telegraph))
							{

								if(next_time_interval_state == roadsign_t::danger || (last_longblock_signal_index < INVALID_INDEX && i > first_stop_signal_index))
								{
									if(station_signal)
									{
										stop_at_station_signal = check_halt;
									}
									else
									{
										next_signal_index = i;
										count --;
									}
								}
							}
							else
							{
								if((working_method == time_interval_with_telegraph && i > first_stop_signal_index && (last_longblock_signal_index >= INVALID_INDEX)) || !next_signal_protects_no_junctions)
								{
									if(station_signal)
									{
										stop_at_station_signal = check_halt;
									}
									else if((!directional_only || signal->get_desc()->get_working_method() != time_interval) && !signal->get_no_junctions_to_next_signal())
									{
										next_signal_index = i;
										count --;
									}
								}
								else if(working_method == time_interval && i > first_stop_signal_index)
								{
									if(station_signal)
									{
										stop_at_station_signal = check_halt;
									}
									else
									{
										next_signal_index = i;
										count --;
									}
								}
							}
						}
						else if((working_method == time_interval || working_method == time_interval_with_telegraph) && (next_signal_working_method == absolute_block || next_signal_working_method == token_block || next_signal_working_method == track_circuit_block || next_signal_working_method == moving_block || next_signal_working_method == cab_signalling))
						{
							// When transitioning out of the time interval working method, do not attempt to reserve beyond the first stop signal of a different working method until the last time interval signal before the first signal of the other method has been passed.
							next_signal_index = i;
							count --;
						}

						if(signal->get_desc()->get_working_method() == token_block)
						{
							last_token_block_signal_index = i;
							const bool platform_starter = (this_halt.is_bound() && (haltestelle_t::get_halt(signal->get_pos(), get_owner())) == this_halt)
								&& (haltestelle_t::get_halt(get_pos(), get_owner()) == this_halt) && (cnv->get_akt_speed() == 0);

							// Do not reserve through a token block signal: the train must stop to take the token.
							if(((last_token_block_signal_index > first_stop_signal_index || (!starting_at_signal && !platform_starter)) && (first_double_block_signal_index != last_stop_signal_index)) ||
								(first_double_block_signal_index == INVALID_INDEX && last_stop_signal_index == INVALID_INDEX && !starting_at_signal && !platform_starter))
							{
								count --;
								end_of_block = true;
								do_not_clear_distant = true;
							}
						}

						if(!directional_only)
						{
							this_stop_signal_index = i;
						}
						if(signal->get_desc()->is_longblock_signal() || (station_signal && last_longblock_signal_index == INVALID_INDEX))
						{
							last_longblock_signal_index = i;
						}

						// Any junctions previously found no longer apply to the next signal, unless this is a pre-signal
						no_junctions_to_next_signal = true;

					}
					else if(!directional_only) // Distant signal or repeater
					{
						if(next_signal_working_method == absolute_block || next_signal_working_method == token_block)
						{
							const grund_t* gr_last_signal = welt->lookup(cnv->get_last_signal_pos());
							signal_t* last_signal = gr_last_signal ? gr_last_signal->find<signal_t>() : NULL;
							koord3d last_signalbox_pos = last_signal ? last_signal->get_signalbox() : koord3d::invalid;

							if(signalbox_last_distant_signal == koord3d::invalid
								&& i - start_index <= modified_sighting_distance_tiles
								&& (last_signalbox_pos == koord3d::invalid
								 || last_signalbox_pos != signal->get_signalbox()
								 || (!last_signal || !signal->get_desc()->get_intermediate_block())
								 || (signal->get_desc()->get_intermediate_block() ^ last_signal->get_desc()->get_intermediate_block())) // Cannot be two intermediate blocks in a row.
								&& (pre_signals.empty() || first_stop_signal_index == INVALID_INDEX))
							{
								pre_signals.append(signal);
								last_pre_signal_index = i;
								signalbox_last_distant_signal = signal->get_signalbox();
								last_distant_signal_was_intermediate_block = signal->get_desc()->get_intermediate_block();
							}
						}
						else if(next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling)
						{
							// In this mode, distant signals are regarded as mere repeaters.
							if(remaining_aspects < 2 && pre_signals.empty())
							{
								remaining_aspects = 3;
							}
							if(first_stop_signal_index == INVALID_INDEX)
							{
								pre_signals.append(signal);
								last_pre_signal_index = i;
							}
						}
						else if((next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph) && last_pre_signal_index >= INVALID_INDEX)
						{
							last_pre_signal_index = i;
						}
					}
				}
			}

			station_signal_to_clear_only_point:

			if(next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph)
			{
				uint16 time_interval_starting_point;
				if(next_signal_working_method == time_interval && last_station_tile < INVALID_INDEX && set_first_station_signal && (last_station_tile > last_stop_signal_index || last_station_tile > this_stop_signal_index))
				{
					time_interval_starting_point = last_station_tile;
				}
				else if(last_stop_signal_index < INVALID_INDEX)
				{
					time_interval_starting_point = last_stop_signal_index;
				}
				else
				{
					time_interval_starting_point = i;
				}
				time_interval_reservation =
					   (!next_signal_protects_no_junctions || first_double_block_signal_index < INVALID_INDEX)
					&& ((this_stop_signal_index < INVALID_INDEX || last_stop_signal_index < INVALID_INDEX) || last_choose_signal_index < INVALID_INDEX) && (count >= 0 || i == start_index)
					&& ((i - time_interval_starting_point <= welt->get_settings().get_sighting_distance_tiles()) || next_signal_working_method == time_interval_with_telegraph
						|| (check_halt.is_bound() && check_halt == last_step_halt && (next_signal_working_method == time_interval_with_telegraph || previous_time_interval_reservation == is_true || previous_time_interval_reservation == is_uncertain))); // Time interval with telegraph signals have no distance limit for reserving.
			}

			const bool telegraph_directional = (time_interval_reservation && previous_telegraph_directional) || (next_signal_working_method == time_interval_with_telegraph && (((last_longblock_signal_index == last_stop_signal_index && first_stop_signal_index == last_longblock_signal_index) && last_longblock_signal_index < INVALID_INDEX) || (first_double_block_signal_index < INVALID_INDEX && next_signal_protects_no_junctions)));
			const schiene_t::reservation_type rt = directional_only || (telegraph_directional && i > modified_sighting_distance_tiles) ? schiene_t::directional : schiene_t::block;
			bool transitioning_from_time_interval = (working_method == time_interval || working_method == time_interval_with_telegraph) && (next_signal_working_method == absolute_block || next_signal_working_method == track_circuit_block || next_signal_working_method == token_block);
			bool attempt_reservation = directional_only || time_interval_reservation || previous_telegraph_directional || ((next_signal_working_method != time_interval && next_signal_working_method != time_interval_with_telegraph && ((next_signal_working_method != drive_by_sight && !transitioning_from_time_interval) || i < start_index + modified_sighting_distance_tiles + 1)) && (!stop_at_station_signal.is_bound() || stop_at_station_signal == check_halt));
			previous_telegraph_directional = telegraph_directional;
			previous_time_interval_reservation = time_interval_reservation ? is_true : is_false;
			if(!reserving_beyond_a_train && attempt_reservation && !sch1->reserve(cnv->self, ribi_type(route->at(max(1u,i)-1u), route->at(min(route->get_count()-1u,i+1u))), rt, (working_method == time_interval || working_method == time_interval_with_telegraph)))
			{
				not_entirely_free = true;
				if (from_call_on)
				{
					next_signal_working_method = drive_by_sight;
				}

				if (directional_only && restore_last_stop_signal_index < INVALID_INDEX)
				{
					last_stop_signal_index = next_signal_index = restore_last_stop_signal_index;
				}

				if (first_double_block_signal_index < INVALID_INDEX && stop_signals_since_last_double_block_signal < 2)
				{
					// If the last but one signal is a double block signal, do not allow the train to pass beyond that signal
					// even if the route to the next signal is free
					last_stop_signal_index = first_double_block_signal_index;
					last_stop_signal_pos = route->at(first_double_block_signal_index);
					if (next_signal_index > last_stop_signal_index)
					{
						next_signal_index = last_stop_signal_index;
					}
				}

				if(next_signal_index == i && (next_signal_working_method == absolute_block || next_signal_working_method == token_block || next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling))
				{
					// If the very last tile of a block is not free, do not enter the block.
					next_signal_index = last_stop_signal_index < INVALID_INDEX || this_stop_signal_index == INVALID_INDEX ? last_stop_signal_index : this_stop_signal_index;
				}
				if((next_signal_working_method == drive_by_sight || next_signal_working_method == moving_block) && !directional_only && last_choose_signal_index >= INVALID_INDEX && !is_choosing)
				{
					next_signal_index = i - 1;
					break;
				}

				if(((next_signal_working_method == absolute_block || next_signal_working_method == token_block || next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph) && first_stop_signal_index < i) && !directional_only)
				{
					// Because the distant signal applies to all signals controlled by the same signalbox, the driver cannot know that the route
					// will be clear beyond the *first* stop signal after the distant.
					do_not_clear_distant = true;
					if(next_signal_working_method == absolute_block || next_signal_working_method == token_block)
					{
						if((pre_signals.empty() && first_stop_signal_index > start_index + modified_sighting_distance_tiles))
						{
							next_signal_index = first_stop_signal_index;
						}
						if(next_signal_index == start_index && !is_choosing)
						{
							success = false;
							directional_reservation_succeeded = false;
						}
						break;
					}
				}

				if((next_signal_working_method == track_circuit_block
					|| next_signal_working_method == cab_signalling)
					&& last_stop_signal_index < INVALID_INDEX
					&& !directional_only
					&& ((next_signal_index <= last_stop_signal_before_first_bidirectional_signal_index
					&& last_stop_signal_before_first_bidirectional_signal_index < INVALID_INDEX)
					|| sch1->can_reserve(cnv->self, ribi_type(route->at(max(1u,i)-1u), route->at(min(route->get_count()-1u,i+1u))), schiene_t::directional)))
				{
					next_signal_index = last_stop_signal_index;
					break;
				}
				if((directional_only || next_signal_index > last_stop_signal_before_first_bidirectional_signal_index) && i < first_oneway_sign_index)
				{
					// Do not allow the train to proceed along a section of bidirectionally signalled line unless
					// a directional reservation can be secured the whole way.
					next_signal_index = last_stop_signal_before_first_bidirectional_signal_index;
				}
				success = false;
				directional_reservation_succeeded = false;
				if(next_signal_index > first_stop_signal_index && last_stop_signal_before_first_bidirectional_signal_index >= INVALID_INDEX && i < first_oneway_sign_index)
				{
					next_signal_index = first_stop_signal_index;
					break;
				}
			}
			else if(attempt_reservation ||
				(next_signal_working_method == time_interval || (next_signal_working_method == time_interval_with_telegraph
				&& !next_signal_protects_no_junctions && this_stop_signal_index == i && !directional_only)))
			{
				if(this_stop_signal_index != INVALID_INDEX && !directional_only)
				{
					last_stop_signal_index = this_stop_signal_index;
					last_stop_signal_pos = route->at(this_stop_signal_index);
				}

				if(attempt_reservation && !directional_only)
				{
					if(sch1->is_crossing())
					{
						crossing_t* cr = gr->find<crossing_t>(2);
						if(cr)
						{
							// Actually reserve the crossing
							cr->request_crossing(this);
						}
					}
				}

				if(end_of_block && !is_from_token && !directional_only)
				{
					next_signal_index = last_stop_signal_index;
					break;
				}

			}

			last_step_halt = check_halt;

			// Do not attempt to reserve beyond a train ahead.
			if(reserving_beyond_a_train || ((next_signal_working_method != time_interval_with_telegraph || next_time_interval_state == roadsign_t::danger || attempt_reservation) && !directional_only && sch1->get_reserved_convoi().is_bound() && sch1->get_reserved_convoi() != cnv->self && sch1->get_reserved_convoi()->get_pos() == pos))
			{
				if (attempt_reservation)
				{
					success = false;
					if (stop_at_station_signal == check_halt)
					{
						next_signal_index = first_stop_signal_index;
					}
					break;
				}
				else
				{
					// This is necessary in case attempt reservation is enabled in a subsequent step.
					reserving_beyond_a_train = true;
				}
			}

			// check if there is an early platform available to stop at
			if(do_early_platform_search)
			{
				if(early_platform_index == INVALID_INDEX)
				{
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
							while(current_pos != cnv->get_schedule()->get_current_entry().pos && haltestelle_t::get_halt(current_pos, cnv->get_owner()) == dest_halt && route_ahead_check < route->get_count())
							{
								current_pos = route->at(route_ahead_check);
								route_ahead_check++;
								platform_size_found++;
							}
							if(current_pos != cnv->get_schedule()->get_current_entry().pos)
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
					if(platform_size_needed < platform_size_found && !schedule->get_current_entry().reverse)
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
					route->remove_koord_from(early_platform_index);
					if(next_signal_index > early_platform_index && !is_from_token && next_signal_working_method != one_train_staff)
					{
						next_signal_index = INVALID_INDEX;
					}
					success = true;
					reached_end_of_loop = true;
					break;
				}
				ribi_last = ribi;
			} // Early platform search
		} //if(reserve)

		else if(sch1) // Unreserve
		{
			if(!sch1->unreserve(cnv->self))
			{
				if(unreserve_now)
				{
					// reached an reserved or free track => finished
					return 0;
				}
			}
			else
			{
				// un-reserve from here (used during sale, since there might be reserved tiles not freed)
				unreserve_now = !force_unreserve;
			}
			if(sch1->has_signal())
			{
				ribi_t::ribi direction_of_travel = ribi_type(route->at(max(1u,i)-1u), route->at(min(route->get_count()-1u,i+1u)));
				signal_t* signal = sch1->get_signal(direction_of_travel);
				if(signal && !signal->get_no_junctions_to_next_signal())
				{
					if(signal->get_desc()->is_pre_signal())
					{
						signal->set_state(roadsign_t::caution);
					}
					else
					{
						signal->set_state(roadsign_t::danger);
					}
				}
			}
			if(sch1->is_crossing())
			{
				gr->find<crossing_t>()->release_crossing(this);
			}
		} // Unreserve

		if(i >= route->get_count() - 1)
		{
			reached_end_of_loop = true;
			if(!is_from_token)
			{
				next_signal_index = INVALID_INDEX;
			}
		}
	} // For loop

	if(!reserve)
	{
		return 0;
	}
	// here we go only with reserve

	koord3d signal_pos;
	if (i >= route->get_count())
	{
		if (next_signal_index == i)
		{
			// This deals with a very specific problem in which the route is truncated
			signal_pos = pos;
		}
		else
		{
			signal_pos = koord3d::invalid;
		}
	}
	else
	{
		signal_pos = next_signal_index < INVALID_INDEX ? route->at(next_signal_index) : koord3d::invalid;
	}
	bool platform_starter = (this_halt.is_bound() && (haltestelle_t::get_halt(signal_pos, get_owner())) == this_halt) && (haltestelle_t::get_halt(get_pos(), get_owner()) == this_halt) && first_stop_signal_index == last_stop_signal_index;

	// If we are in token block or one train staff mode, one train staff mode or making directional reservations, reserve to the end of the route if there is not a prior signal.
	// However, do not call this if we are in the block reserver already called from this method to prevent infinite recursion.
	const bool bidirectional_reservation = (working_method == track_circuit_block || working_method == cab_signalling || working_method == moving_block)
		&& last_bidirectional_signal_index < INVALID_INDEX && (last_oneway_sign_index >= INVALID_INDEX || last_oneway_sign_index < last_bidirectional_signal_index);
	const bool one_train_staff_onward_reservation = working_method == one_train_staff || start_index == first_one_train_staff_index;
	if(!is_from_token && !is_from_directional && (((working_method == token_block || (first_double_block_signal_index < INVALID_INDEX && stop_signals_since_last_double_block_signal)) && last_token_block_signal_index < INVALID_INDEX) || bidirectional_reservation || one_train_staff_onward_reservation) && next_signal_index == INVALID_INDEX)
	{
		route_t target_rt;
		schedule_t *schedule = cnv->get_schedule();
		uint8 schedule_index = schedule->get_current_stop();
		bool rev = cnv->get_reverse_schedule();
		bool no_reverse = schedule->entries[schedule_index].reverse == 0;
		schedule->increment_index(&schedule_index, &rev);
		koord3d cur_pos = route->back();
		uint16 next_next_signal;
		bool route_success;
		sint32 token_block_blocks = 0;
		if(no_reverse || one_train_staff_onward_reservation)
		{
			bool break_loop_recursive = false;
			if (one_train_staff_onward_reservation && working_method != one_train_staff)
			{
				set_working_method(one_train_staff);
			}
			do
			{
				// Search for route until the next signal is found.
				route_success = target_rt.calc_route(welt, cur_pos, cnv->get_schedule()->entries[schedule_index].pos, this, speed_to_kmh(cnv->get_min_top_speed()), cnv->get_highest_axle_load(), cnv->has_tall_vehicles(), 8888 + cnv->get_tile_length(), SINT64_MAX_VALUE, cnv->get_weight_summary().weight / 1000) == route_t::valid_route;

				if(route_success)
				{
					if (one_train_staff_onward_reservation && first_one_train_staff_index < INVALID_INDEX && !this_signal_is_nonadjacent_one_train_staff_cabinet)
					{
						cnv->set_last_signal_pos(route->at(first_one_train_staff_index));
					}

					token_block_blocks = block_reserver(&target_rt, 1, modified_sighting_distance_tiles, next_next_signal, 0, true, false, false, !bidirectional_reservation, false, bidirectional_reservation, brake_steps, one_train_staff_onward_reservation ? first_one_train_staff_index : INVALID_INDEX, false, &break_loop_recursive);
				}

				if(token_block_blocks && next_next_signal < INVALID_INDEX)
				{
					// There is a signal in a later part of the route to which we can reserve now.
					if(bidirectional_reservation)
					{
						if(signal_t* sg = welt->lookup(target_rt.at(next_next_signal))->get_weg(get_waytype())->get_signal(ribi_type((target_rt.at(next_next_signal - 1)), target_rt.at(next_next_signal))))
						{
							if(!sg->is_bidirectional() || sg->get_desc()->is_longblock_signal())
							{
								break;
							}
						}
					}
					else
					{
						if(!one_train_staff_onward_reservation)
						{
							cnv->set_next_stop_index(cnv->get_route()->get_count() - 1u);
						}
						break;
					}
				}

				no_reverse = schedule->entries[schedule_index].reverse != 1;

				if(token_block_blocks)
				{
					// prepare for next leg of schedule
					cur_pos = target_rt.back();
					schedule->increment_index(&schedule_index, &rev);
				}
				else
				{
					success = false;
				}
			} while((schedule_index != cnv->get_schedule()->get_current_stop()) && token_block_blocks && no_reverse && !break_loop_recursive);
		}



		if(token_block_blocks && !bidirectional_reservation)
		{
			if(cnv->get_next_stop_index() - 1 <= route_index)
			{
				if (one_train_staff_onward_reservation && next_signal_index >= INVALID_INDEX)
				{
					cnv->set_next_stop_index(next_next_signal);
				}
				else
				{
					cnv->set_next_stop_index(cnv->get_route()->get_count() - 1);
				}
			}
		}
	}

	if(last_stop_signal_index < INVALID_INDEX && last_bidirectional_signal_index < INVALID_INDEX && first_oneway_sign_index >= INVALID_INDEX && directional_reservation_succeeded && end_of_block)
	{
		 //TODO: Consider whether last_stop_signal_index should be assigned to last_bidirectional_signal_index
		 next_signal_index = last_stop_signal_index;
		 platform_starter = (this_halt.is_bound() && i < route->get_count() && (haltestelle_t::get_halt(route->at(last_stop_signal_index), get_owner())) == this_halt) && (haltestelle_t::get_halt(get_pos(), get_owner()) == this_halt);
	}


	/*if (no_junctions_to_last_signal && no_junctions_to_next_signal && reached_end_of_loop && success && last_stop_signal_index < INVALID_INDEX && i > (last_stop_signal_index + 1))
	{
		const grund_t* gr_signal = welt->lookup(last_stop_signal_pos);
		signal_t* signal = gr_signal->find<signal_t>();
		if (signal && !signal->get_desc()->is_choose_sign())
		{
			signal->set_no_junctions_to_next_signal(true);
		}
	}*/

	bool choose_route_identical_to_main_route = false;

	// free, in case of un-reserve or no success in reservation
	// or alternatively free that section reserved beyond the last signal to which reservation can take place
	if(!success || !directional_reservation_succeeded || ((next_signal_index < INVALID_INDEX) && (next_signal_working_method == absolute_block || next_signal_working_method == token_block || next_signal_working_method == track_circuit_block || next_signal_working_method == cab_signalling || ((next_signal_working_method == time_interval || next_signal_working_method == time_interval_with_telegraph) && !next_signal_protects_no_junctions))))
	{
		const bool will_choose = (last_choose_signal_index < INVALID_INDEX) && !is_choosing && not_entirely_free && (last_choose_signal_index == first_stop_signal_index) && !is_from_token;
		// free reservation
		uint16 curtailment_index;
		bool do_not_increment_curtailment_index_directional = false;
		if(!success)
		{
			const uint16 next_stop_index = cnv->get_next_stop_index();

			if (working_method_set_by_distant_only == set)
			{
				working_method = old_working_method;
			}
			if (next_stop_index == start_index + 1)
			{
				curtailment_index = start_index;
			}
			else if (!directional_reservation_succeeded && next_signal_working_method == time_interval_with_telegraph && next_signal_index < INVALID_INDEX)
			{
				curtailment_index = next_signal_index;
			}
			else
			{
				curtailment_index = max(start_index, cnv->get_next_stop_index());
			}
		}
		else if(!directional_reservation_succeeded)
		{
			curtailment_index = last_non_directional_index;
			do_not_increment_curtailment_index_directional = true;
		}
		else if(will_choose && first_double_block_signal_index > next_signal_index)
		{
			curtailment_index = last_choose_signal_index;
		}
		else
		{
			curtailment_index = next_signal_index;
		}

		if(!directional_only && !do_not_increment_curtailment_index_directional)
		{
			curtailment_index ++;
		}

		if(next_signal_index < INVALID_INDEX && (next_signal_index == start_index || platform_starter) && !is_from_token && !is_choosing)
		{
			// Cannot go anywhere either because this train is already on the tile of the last signal to which it can go, or is in the same station as it.
			success = false;
		}

		if(route->get_count() <= start_index)
		{
			// Occasionally, the route may be recalculated here when there is a combination of a choose signal and misplaced bidirectional or longblock signal without
			// the start index changing. This can cause errors in the next line.
			start_index = 0;
		}

		if((working_method == token_block || working_method == one_train_staff) && success == false)
		{
			cnv->unreserve_route();
			cnv->reserve_own_tiles();
		}
		else if(curtailment_index < i)
		{
			const halthandle_t halt_current = haltestelle_t::get_halt(get_pos(), get_owner());
			for(uint32 j = curtailment_index; j < route->get_count(); j++)
			{
				grund_t* gr_this = welt->lookup(route->at(j));
				schiene_t * sch1 = gr_this ? (schiene_t *)gr_this->get_weg(get_waytype()) : NULL;
				const halthandle_t halt_this = haltestelle_t::get_halt(route->at(j), get_owner());
				if(sch1 && (sch1->is_reserved(schiene_t::block)
					|| (!directional_reservation_succeeded
					&& sch1->is_reserved(schiene_t::directional)))
					&& sch1->get_reserved_convoi() == cnv->self
					&& sch1->get_pos() != get_pos()
					&& (!halt_current.is_bound() || halt_current != halt_this))
				{
					sch1->unreserve(cnv->self);
					if(sch1->has_signal())
					{
						signs.remove(gr_this);
					}
					if(sch1->is_crossing())
					{
						gr_this->find<crossing_t>()->release_crossing(this);
					}
				}
			}
		}

		if(will_choose)
		{
			// This will call the block reserver afresh from the last choose signal with choose logic enabled.
			sint32 modified_route_index;
			const uint32 route_count = route->get_count();
			if(is_from_token || is_from_directional)
			{
				modified_route_index = min(route_count, (route_index - route_count));
			}
			else
			{
				modified_route_index = min(route_index, route_count);
			}

			const koord3d check_tile_mid = route->at(route_count / 2u);
			const koord3d check_tile_end = route->back();

			choose_return = activate_choose_signal(last_choose_signal_index, next_signal_index, brake_steps, modified_sighting_distance_tiles, route, modified_route_index);

			if (success && choose_return == 0)
			{
				// Restore the curtailed route above: the original route works, but the choose route does not.
				bool re_reserve_succeeded = true;
				const uint32 route_count = route->get_count();
				uint32 limit = min(next_signal_index, route_count);
				for (uint32 n = curtailment_index; n < limit; n++)
				{
					grund_t* gr_this = welt->lookup(route->at(n));
					schiene_t * sch1 = gr_this ? (schiene_t *)gr_this->get_weg(get_waytype()) : NULL;
					re_reserve_succeeded &= sch1->reserve(cnv->self, ribi_t::all);
				}

				if (re_reserve_succeeded)
				{
					const koord3d last_choose_pos = route->at(last_choose_signal_index);
					grund_t* const signal_ground = welt->lookup(last_choose_pos);
					if (signal_ground)
					{
						signs.append_unique(signal_ground);
					}
				}
			}

			if (route_count == route->get_count())
			{
				// Check whether the choose signal route really is a new route.
				choose_route_identical_to_main_route = check_tile_mid == route->at(route_count / 2u) && check_tile_end == route->back();
			}
		}

		if(!success && !choose_return)
		{
			cnv->set_next_reservation_index(curtailment_index);
			if (last_stop_signal_index > start_index && last_pre_signal_index < last_stop_signal_index)
			{
				if (working_method != token_block && working_method != one_train_staff)
				{
					set_working_method(drive_by_sight);
				}
				return 1;
			}
			return 0;
		}
	}

	// Clear signals on the route.
	if(!is_from_token && !is_from_directional)
	{
		// Clear any station signals
		if(first_station_signal)
		{
			if(first_station_signal->get_desc()->get_working_method() == absolute_block && success)
			{
				first_station_signal->set_state(station_signal == inverse ? roadsign_t::clear : roadsign_t::clear_no_choose);
			}
			else
			{
				first_station_signal->set_state(first_time_interval_state);
			}
		}
		if (station_signal_to_clear_for_entry)
		{
			// Clear the station signal when a train is arriving.
			// TODO: Consider whether to make this optional on a setting in the signal's .dat file
			if (station_signal_to_clear_for_entry->get_desc()->get_working_method() != absolute_block && station_signal_to_clear_for_entry->get_desc()->get_aspects() > 2)
			{
				station_signal_to_clear_for_entry->set_state(station_signal == inverse ? roadsign_t::caution : roadsign_t::caution_no_choose);
			}
			else
			{
				station_signal_to_clear_for_entry->set_state(station_signal == inverse ? roadsign_t::clear : roadsign_t::clear_no_choose);
			}
		}

		sint32 counter = (signs.get_count() - 1) + choose_return;
		bool time_interval_junction_signal = false;
		if(next_signal_working_method == time_interval && last_stop_signal_index > start_index + modified_sighting_distance_tiles && success && !not_entirely_free)
		{
			// A signal in the plain time interval method protecting a junction should clear even if the route all the way to the next signal is not clear, provided that the route within the sighting distance is clear.
			counter ++;
			time_interval_junction_signal = true;
		}

		bool last_signal_was_track_circuit_block = false;

		FOR(slist_tpl<grund_t*>, const g, signs)
		{
			if(signal_t* const signal = g->find<signal_t>())
			{
				if(((counter -- > 0 || (pre_signals.empty() && (!starting_at_signal || signs.get_count() == 1)) || (reached_end_of_loop && (early_platform_index == INVALID_INDEX || last_stop_signal_index < early_platform_index))) && (signal->get_desc()->get_working_method() != token_block || starting_at_signal || ((start_index == first_stop_signal_index) && (first_stop_signal_index == last_stop_signal_index))) && ((route->at(route->get_count() - 1) != signal->get_pos()) || signal->get_desc()->get_working_method() == token_block)))
				{
					const bool use_no_choose_aspect = choose_route_identical_to_main_route || (signal->get_desc()->is_choose_sign() && !is_choosing && choose_return == 0);

					if(signal->get_desc()->get_working_method() == absolute_block || signal->get_desc()->get_working_method() == token_block)
					{
						// There is no need to set a combined signal to clear here, as this is set below in the pre-signals clearing loop.
						if (!last_signal_was_track_circuit_block || count >= 0 || next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
						{
							signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::clear ? roadsign_t::clear_no_choose : signal->get_desc()->is_combined_signal() ? roadsign_t::caution : roadsign_t::clear);
						}
					}

					if((signal->get_desc()->get_working_method() == time_interval || signal->get_desc()->get_working_method() == time_interval_with_telegraph) && (end_of_block || i >= last_stop_signal_index + 1 || time_interval_junction_signal))
					{
						if((signal->get_desc()->get_working_method() == time_interval_with_telegraph || signal->get_desc()->get_working_method() == time_interval) && signal->get_desc()->is_longblock_signal())
						{
							// Longblock signals in time interval can clear fully.
							// We also assume that these will not also be choose signals.
							signal->set_state(next_time_interval_state);
						}
						else
						{
							signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::caution ? roadsign_t::caution_no_choose : roadsign_t::caution);
						}
					}

					if(signal->get_desc()->get_working_method() == track_circuit_block || signal->get_desc()->get_working_method() == cab_signalling)
					{
						if(count >= 0 || next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
						{
							// Do not clear the last signal in the route, as nothing is reserved beyond it, unless there are no more signals beyond at all (count == 0)
							sint32 add_value = 0;
							if (reached_end_of_loop && (signs.get_count() == 1 || (signs.get_count() > 0 && this_stop_signal_index >= INVALID_INDEX)))
							{
								add_value = 1;
							}

							if (choose_route_identical_to_main_route)
							{
								add_value -= choose_return;
							}

							switch(signal->get_desc()->get_aspects())
							{
							case 2:
							default:
								if(next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
								{
									signal->set_state(use_no_choose_aspect ? roadsign_t::clear_no_choose : roadsign_t::clear);
								}
								break;
							case 3:
								if(counter + add_value >= 1)
								{
									signal->set_state((use_no_choose_aspect && signal->get_state() != roadsign_t::clear) || signal->get_state() == roadsign_t::caution ? roadsign_t::clear_no_choose : roadsign_t::clear);
								}
								else if(next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
								{
									if (signal->get_state() == roadsign_t::danger || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::caution))
									{
										signal->set_state(use_no_choose_aspect ? roadsign_t::caution_no_choose : roadsign_t::caution);
									}
								}
								break;
							case 4:
								if(counter + add_value >= 2)
								{
									signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::clear && signal->get_state() != roadsign_t::caution && signal->get_state() != roadsign_t::preliminary_caution ? roadsign_t::clear_no_choose : roadsign_t::clear);
								}
								else if(counter + add_value >= 1)
								{
									if(signal->get_state() == roadsign_t::danger || signal->get_state() == roadsign_t::caution || signal->get_state() == roadsign_t::caution_no_choose || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::preliminary_caution))
									{
										signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::caution && signal->get_state() != roadsign_t::preliminary_caution ? roadsign_t::preliminary_caution_no_choose : roadsign_t::preliminary_caution);
									}
								}
								else if(next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
								{
									if (signal->get_state() == roadsign_t::danger || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::caution))
									{
										signal->set_state(use_no_choose_aspect ? roadsign_t::caution_no_choose : roadsign_t::caution);
									}
								}
								break;
							case 5:
								if(counter + add_value >= 3)
								{
									signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::caution && signal->get_state() != roadsign_t::clear && signal->get_state() != roadsign_t::preliminary_caution && signal->get_state() != roadsign_t::advance_caution ? roadsign_t::clear_no_choose : roadsign_t::clear);
								}
								else if(counter + add_value >= 2)
								{
									if(signal->get_state() == roadsign_t::danger || signal->get_state() == roadsign_t::caution || signal->get_state() == roadsign_t::preliminary_caution || signal->get_state() == roadsign_t::caution_no_choose || signal->get_state() == roadsign_t::preliminary_caution_no_choose || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::advance_caution))
									{
										signal->set_state(use_no_choose_aspect && signal->get_state() != roadsign_t::caution && signal->get_state() != roadsign_t::clear && signal->get_state() != roadsign_t::preliminary_caution && signal->get_state() != roadsign_t::advance_caution  ? roadsign_t::advance_caution_no_choose : roadsign_t::advance_caution);
									}
								}
								else if(counter + add_value >= 1)
								{
									if (signal->get_state() == roadsign_t::danger || signal->get_state() == roadsign_t::caution || signal->get_state() == roadsign_t::caution_no_choose || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::preliminary_caution))
									{
										signal->set_state((use_no_choose_aspect && signal->get_state() != roadsign_t::caution && signal->get_state() != roadsign_t::clear && signal->get_state() != roadsign_t::preliminary_caution && signal->get_state() != roadsign_t::advance_caution)  || signal->get_state() == roadsign_t::caution ? roadsign_t::preliminary_caution_no_choose : roadsign_t::preliminary_caution);
									}
								}
								else if(next_signal_index > route->get_count() - 1 || signal->get_pos() != route->at(next_signal_index))
								{
									if (signal->get_state() == roadsign_t::danger || (choose_route_identical_to_main_route && choose_return && signal->get_state() == roadsign_t::caution))
									{
										signal->set_state(use_no_choose_aspect ? roadsign_t::caution_no_choose : roadsign_t::caution);
									}
								}
								break;
							}
						}
					}
				}
				last_signal_was_track_circuit_block = signal->get_desc()->get_working_method() == track_circuit_block;
			}
			else
			{
				counter --;
			}
		}

		FOR(slist_tpl<signal_t*>, const signal, pre_signals)
		{
			if(!do_not_clear_distant && !(not_entirely_free && next_signal_index == first_stop_signal_index))
			{
				signal->set_state(roadsign_t::clear);
			}
		}
	}

	if(next_signal_index != INVALID_INDEX && (next_signal_working_method == track_circuit_block || next_signal_working_method == absolute_block || next_signal_working_method == cab_signalling || next_signal_working_method == token_block))
	{
		if(platform_starter && !is_from_token && !is_from_starter && !is_choosing)
		{
			// This is a platform starter signal for this station: do not move until it clears.
			/*const koord3d signal_dir = signal_pos - (route->at(min(next_signal_index + 1u, route->get_count() - 1u)));
			const ribi_t::ribi direction_of_travel = ribi_type(signal_dir);
			const signal_t* sig = welt->lookup(signal_pos)->get_weg(get_waytype())->get_signal(direction_of_travel); */
			signal_t* sig = welt->lookup(signal_pos)->find<signal_t>();
			if(sig && sig->get_state() == signal_t::danger)
			{
				sint32 success_2 = block_reserver(cnv->get_route(), modified_sighting_distance_tiles, next_signal_index, next_signal_index, 0, true, false, false, false, true);
				next_signal_index = cnv->get_next_stop_index() - 1;
				return success_2;
			}
		}
	}

	if(cnv && !is_from_token && !is_from_directional)
	{
		cnv->set_next_reservation_index(last_non_directional_index);
	}

	// Trim route to the early platform index.
	if(early_platform_index < INVALID_INDEX)
	{
		route->remove_koord_from(early_platform_index);
	}

	return reached_end_of_loop || working_method != track_circuit_block ? (!combined_signals.empty() && !pre_signals.empty() ? 2 : 1) + choose_return : directional_only && success ? 1 : (sint32)signs.get_count() + choose_return;
}

void rail_vehicle_t::clear_token_reservation(signal_t* sig, rail_vehicle_t* w, schiene_t* sch)
{
	route_t* route = cnv ? cnv->get_route() : NULL;
	if(sig && (sig->get_desc()->get_working_method() != token_block && sig->get_desc()->get_working_method() != one_train_staff) && cnv->get_state() != convoi_t::REVERSING)
	{
		w->set_working_method(sig->get_desc()->get_working_method());
	}
	if(cnv && cnv->get_needs_full_route_flush())
	{
		// The route has been recalculated since token block mode was entered, so delete all the reservations.
		// Do not unreserve tiles ahead in the route (i.e., those not marked stale), however.
		const waytype_t waytype = sch->get_waytype();
		FOR(vector_tpl<weg_t*>, const way, weg_t::get_alle_wege())
		{
			if(way->get_waytype() == waytype)
			{
				schiene_t* const sch = obj_cast<schiene_t>(way);
				if(sch->get_reserved_convoi() == cnv->self && sch->is_stale())
				{
					sch->unreserve(w);
				}
			}
		}
		cnv->set_needs_full_route_flush(false);
	}
	else
	{
		uint32 break_now = 0;
		const bool is_one_train_staff = sig && sig->get_desc()->get_working_method() == one_train_staff;
		bool last_tile_was_break = false;
		for(int i = route_index - 1; i >= 0; i--)
		{
			grund_t* gr_route = route ? welt->lookup(route->at(i)) : NULL;
			schiene_t* sch_route = gr_route ? (schiene_t *)gr_route->get_weg(get_waytype()) : NULL;
			if ((!is_one_train_staff && (sch_route && !sch_route->is_reserved())) || (sch_route && sch_route->get_reserved_convoi() != cnv->self))
			{
				if (!last_tile_was_break)
				{
					break_now++;
				}
				last_tile_was_break = true;
			}
			else
			{
				last_tile_was_break = false;
			}
			if(sch_route && (!cnv || cnv->get_state() != convoi_t::REVERSING))
			{
				sch_route->unreserve(cnv->self);
			}
			if (break_now > 1)
			{
				break;
			}
		}
	}
}

void rail_vehicle_t::unreserve_in_rear()
{
	route_t* route = cnv ? cnv->get_route() : NULL;

	route_index = min(route_index, route ? route->get_count() - 1 : route_index);

	for (int i = route_index - 1; i >= 0; i--)
	{
		const koord3d this_pos = route->at(i);
		grund_t* gr_route = welt->lookup(this_pos);
		schiene_t* sch_route = gr_route ? (schiene_t *)gr_route->get_weg(get_waytype()) : NULL;
		if (sch_route)
		{
			sch_route->unreserve(cnv->self);
		}
	}
}

void rail_vehicle_t::unreserve_station()
{
	grund_t* gr = welt->lookup(get_pos());
	if (!gr)
	{
		return;
	}
	bool in_station = gr->get_halt().is_bound();

	route_t* route = cnv ? cnv->get_route() : NULL;
	route_index = min(route_index, route ? route->get_count() - 1 : route_index);
	if (route->get_count() < route_index || route->empty())
	{
		// The route has been recalculated, so we cannot
		// unreserve this in the usual way.
		// Instead, brute-force the unreservation.
		cnv->unreserve_route();
		cnv->reserve_own_tiles();
		return;
	}
	const koord3d this_pos = route->at(route_index);
	const koord3d last_pos = route->at(route_index - 1);
	const koord3d dir = this_pos - last_pos;
	const ribi_t::ribi direction_of_travel = ribi_type(dir);
	const ribi_t::ribi reverse_direction = ribi_t::backward(direction_of_travel);
	bool is_previous;

	grund_t* gr_prev = gr;
	while (in_station)
	{
		is_previous = gr_prev->get_neighbour(gr_prev, get_waytype(), reverse_direction);
		if (is_previous)
		{
			in_station = gr_prev->get_halt().is_bound();
			schiene_t* sch = (schiene_t*)gr_prev->get_weg(get_waytype());
			sch->unreserve(cnv->self);
		}
		else
		{
			// Sometimes, where a train leaves a station in
			// a different direction to that in which it entered
			// it (e.g., if the next tile after the end of the
			// station is a junction and the train is at the very
			// end of the station when it moves), the code cannot
			// search backwards. In this case, just use the old
			// brute force method, which is inefficient but works.
			cnv->unreserve_route();
			cnv->reserve_own_tiles();
			break;
		}
	}
}


/* beware: we must un-reserve rail blocks... */
void rail_vehicle_t::leave_tile()
{
	grund_t *gr = get_grund();
	schiene_t *sch0 = (schiene_t *) get_weg();
	vehicle_t::leave_tile();
	// fix counters
	if(last)
	{
		if(gr)
		{
			if(sch0)
			{
				rail_vehicle_t* w = cnv ? (rail_vehicle_t*)cnv->front() : NULL;

				const halthandle_t this_halt = gr->get_halt();
				const halthandle_t dest_halt = haltestelle_t::get_halt((cnv && cnv->get_schedule() ? cnv->get_schedule()->get_current_entry().pos : koord3d::invalid), get_owner());
				const bool at_reversing_destination = dest_halt.is_bound() && this_halt == dest_halt && cnv->get_schedule() && cnv->get_schedule()->get_current_entry().reverse == 1;

				route_t* route = cnv ? cnv->get_route() : NULL;
				koord3d this_tile;
				koord3d previous_tile;
				if (route)
				{
					// It can very occasionally happen that a train due to reverse at a station
					// passes through another, unconnected part of that station before it stops.
					// When this happens, an island of reservation can remain which can block
					// other trains.
					// Detect this case when it has just begun to occur and rectify it.
					if (route_index > route->get_count() - 1)
					{
						route_index = route->get_count() - 1;
					}
					const uint16 ri = min(route_index, route->get_count() - 1u);
					this_tile = cnv->get_route()->at(ri);
					previous_tile = cnv->get_route()->at(max(1u, ri) - 1u);

					if (cnv->get_schedule() && cnv->get_schedule()->get_current_entry().reverse)
					{
						grund_t* gr_previous_tile = welt->lookup(previous_tile);
						grund_t* gr_this_tile = welt->lookup(this_tile);
						if (gr_previous_tile && gr_this_tile)
						{
							if (gr_previous_tile->get_halt().is_bound() && gr_previous_tile->get_halt() == dest_halt && !gr_this_tile->get_halt().is_bound())
							{
								unreserve_in_rear();
							}
						}
					}
				}

				if((!cnv || cnv->get_state() != convoi_t::REVERSING) && !at_reversing_destination)
				{
					if (w && (w->get_working_method() == token_block || (w->get_working_method() == one_train_staff)))
					{
						// We mark these for later unreservation.
						sch0->set_stale();
					}
					else
					{
						sch0->unreserve(this);
					}
				}

				// The end of the train is passing a signal. Check whether to re-set its aspect
				// or, in the case of a distant signal in absolute or token block working, reset
				// it only when the tail of the train has passed the next home signal.
				if(sch0->has_signal())
				{
					signal_t* sig;
					if(route)
					{
						const koord3d dir = this_tile - previous_tile;
						ribi_t::ribi direction_of_travel = ribi_type(dir);
						sig = sch0->get_signal(direction_of_travel);
					}
					else
					{
						sig = gr->find<signal_t>();
					}

					// Ignore non-adjacent one train staff cabinets
					if (sig && cnv && sig->get_desc()->get_working_method() == one_train_staff)
					{
						const koord3d first_pos = cnv->get_last_signal_pos();
						if (shortest_distance(get_pos().get_2d(), first_pos.get_2d()) >= 3)
						{
							sig = NULL;
						}
					}

					if(sig)
					{
						sig->set_train_last_passed(welt->get_ticks());
						if(sig->get_no_junctions_to_next_signal() && !sig->get_desc()->is_pre_signal() && (sig->get_desc()->get_working_method() == time_interval || sig->get_desc()->get_working_method() == time_interval_with_telegraph))
						{
							welt->add_time_interval_signal_to_check(sig);
						}
						if(!route)
						{
							if(sig->get_desc()->is_pre_signal())
							{
								sig->set_state(roadsign_t::caution);
							}
							else
							{
								sig->set_state(roadsign_t::danger);
							}
							return;
						}
						else if(w &&
							(w->get_working_method() == token_block
							|| (w->get_working_method() == one_train_staff
							&& sig->get_desc()->get_working_method() == one_train_staff))
							&& !sig->get_desc()->is_pre_signal()
							&& cnv->get_state() != convoi_t::REVERSING)
						{
							// On passing a signal, clear all the so far uncleared reservation when in token block mode.
							// If the signal is not a token block signal, clear token block mode. This assumes that token
							// block signals will be placed at the entrance and stop signals at the exit of single line
							// sections.
							clear_token_reservation(sig, w, sch0);

							if (sig->get_desc()->get_working_method() == drive_by_sight)
							{
								set_working_method(drive_by_sight);
								cnv->set_next_stop_index(route_index + 1);
							}
						}
						else if((sig->get_desc()->get_working_method() == track_circuit_block
							|| sig->get_desc()->get_working_method() == cab_signalling
							|| (w && (w->get_working_method() == track_circuit_block
							|| w->get_working_method() == cab_signalling)))
							&& !sig->get_desc()->is_pre_signal())
						{
							// Must reset all "automatic" signals behind this convoy to less restrictive states unless they are of the normal danger type.
							koord3d last_pos = get_pos();
							uint32 signals_count = 0;
							for(int i = route_index - 1; i >= 0; i--)
							{
								const koord3d this_pos = route->at(i);
								const koord3d dir = last_pos - this_pos;
								ribi_t::ribi ribi_route = ribi_type(dir);
								grund_t* gr_route = welt->lookup(this_pos);
								schiene_t* sch_route = gr_route ? (schiene_t *)gr_route->get_weg(get_waytype()) : NULL;
								if(!sch_route || (!sch_route->can_reserve(cnv->self, ribi_route) && gr_route->get_convoi_vehicle()))
								{
									// Cannot go further back than this in any event
									break;
								}
								if(!cnv || cnv->get_state() != convoi_t::REVERSING)
								{
									signal_t* signal_route = sch_route->get_signal(ribi_route);
									if(!signal_route || signal_route == sig)
									{
										continue;
									}
									if(!signal_route->get_desc()->is_pre_signal() && (!signal_route->get_no_junctions_to_next_signal() || signal_route->get_desc()->is_choose_sign() || signal_route->get_desc()->get_normal_danger()))
									{
										break;
									}

									working_method_t swm = signal_route->get_desc()->get_working_method();
									if (swm != track_circuit_block && swm != cab_signalling)
									{
										// Only re-set track circuit block type signals to permissive aspects here.
										break;
									}

									switch(signals_count)
									{
									case 0:
										if(signal_route->get_desc()->get_aspects() > 2 || signal_route->get_desc()->is_pre_signal())
										{
											signal_route->set_state(roadsign_t::caution);
										}
										else
										{
											signal_route->set_state(roadsign_t::clear);
										}
										break;
									case 1:
										if(signal_route->get_desc()->get_aspects() > 3)
										{
											signal_route->set_state(roadsign_t::preliminary_caution);
										}
										else
										{
											signal_route->set_state(roadsign_t::clear);
										}
									break;
									case 2:
										if(signal_route->get_desc()->get_aspects() > 4)
										{
											signal_route->set_state(roadsign_t::advance_caution);
										}
										else
										{
											signal_route->set_state(roadsign_t::clear);
										}
										break;
									default:
										signal_route->set_state(roadsign_t::clear);
									};
									signals_count++;
								}
							}
						}

						if(!sig->get_desc()->is_pre_signal())
						{
							sig->set_state(roadsign_t::danger);
						}

						if(route && !sig->get_desc()->is_pre_signal() && (w && (w->get_working_method() == absolute_block || w->get_working_method() == token_block || w->get_working_method() == time_interval || w->get_working_method() == time_interval_with_telegraph)))
						{
							// Set distant signals in the rear to caution only after the train has passed the stop signal.
							int count = 0;
							for(int i = min(route_index, route->get_count() - 1); i > 0; i--)
							{
								const koord3d current_pos = route->at(i);
								grund_t* gr_route = welt->lookup(current_pos);
								ribi_t::ribi ribi;
								if(i == 0)
								{
									ribi = ribi_t::all;
								}
								else
								{
									ribi = ribi_type(route->at(i - 1), current_pos);
								}
								const weg_t* wg = gr_route ? gr_route->get_weg(get_waytype()) : NULL;
								signal_t* sig_route = wg ? wg->get_signal(ribi) : NULL;
								//signal_t* sig_route = gr_route->find<signal_t>();
								if(sig_route)
								{
									count++;
								}
								if(sig_route && sig_route->get_desc()->is_pre_signal())
								{
									sig_route->set_state(roadsign_t::caution);
									if(sig_route->get_desc()->get_working_method() == time_interval || sig_route->get_desc()->get_working_method() == time_interval_with_telegraph)
									{
										sig_route->set_train_last_passed(welt->get_ticks());
										if(sig->get_no_junctions_to_next_signal())
										{
											// Junction signals reserve within sighting distance in ordinary time interval mode, or to the next signal in time interval with telegraph mode.
											welt->add_time_interval_signal_to_check(sig_route);
										}
									}
									break;
								}
								if(count > 1)
								{
									break;
								}
							}
						}
					}
				}

				// Alternatively, there might be a station signal.
				// Set the timings only on leaving the stop.
				halthandle_t last_tile_halt = cnv && route_index - 2 < cnv->get_route()->get_count() ? haltestelle_t::get_halt(cnv->get_route()->at(route_index - 2), get_owner()) : halthandle_t();
				const uint32 station_signals_count = last_tile_halt.is_bound() ? last_tile_halt->get_station_signals_count() : 0;
				if(station_signals_count && !this_halt.is_bound())
				{
					ribi_t::ribi direction = ribi_type(cnv->get_route()->at(0u), cnv->get_route()->at(1u));
					last_tile_halt->set_train_last_departed(welt->get_ticks(), direction);
					for(uint j = 0; j < station_signals_count; j ++)
					{
						koord3d station_signal_pos = last_tile_halt->get_station_signal(j);
						grund_t* gr_ssp = welt->lookup(station_signal_pos);
						if(gr_ssp)
						{
							weg_t* station_signal_way = gr_ssp->get_weg(get_waytype());
							if(station_signal_way)
							{
								signal_t* station_signal = station_signal_way->get_signal(direction);
								if(!station_signal)
								{
									// There might be a signal in the inverse direction.
									ribi_t::ribi backwards = ribi_t::backward(direction);
									station_signal = station_signal_way->get_signal(backwards);
								}
								if(station_signal)
								{
									station_signal->set_state(roadsign_t::danger);
								}
							}
						}
					}
				}
			}
		}
	}
}


void rail_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);

	if(  schiene_t *sch0 = (schiene_t *) get_weg()  ) {
		// way statistics
		const int cargo = get_total_cargo();
		sch0->book(cargo, WAY_STAT_GOODS);
		if(leading) {
			sch0->book(1, WAY_STAT_CONVOIS);
			if(cnv->get_state() != convoi_t::REVERSING)
			{
				sch0->reserve( cnv->self, get_direction() );
			}
		}
	}
}



schedule_t * rail_vehicle_t::generate_new_schedule() const
{
	return desc->get_waytype()==tram_wt ? new tram_schedule_t() : new train_schedule_t();
}


schedule_t * monorail_rail_vehicle_t::generate_new_schedule() const
{
	return new monorail_schedule_t();
}


schedule_t * maglev_rail_vehicle_t::generate_new_schedule() const
{
	return new maglev_schedule_t();
}


schedule_t * narrowgauge_rail_vehicle_t::generate_new_schedule() const
{
	return new narrowgauge_schedule_t();
}


water_vehicle_t::water_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::water_vehicle, pos, desc, player)
#else
	vehicle_t(pos, desc, player)
#endif
{
	cnv = cn;
}

water_vehicle_t::water_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::water_vehicle)
#else
    vehicle_t()
#endif
{
	vehicle_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_leading) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL)
		{
			bool empty = true;
			const goods_desc_t* gd = NULL;
			ware_t w;
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				if (!fracht[i].empty())
				{
					w = fracht[i].front();
					gd = w.get_desc();
					empty = false;
					break;
				}
			}
			if (!gd)
			{
				// Important: This must be the same default ware as used in rdwr_from_convoi.
				gd = goods_manager_t::passengers;
			}

			dbg->warning("water_vehicle_t::water_vehicle_t()", "try to find a fitting vehicle for %s.", gd->get_name());
			desc = vehicle_builder_t::get_best_matching(water_wt, 0, empty ? 0 : 30, 100, 40, gd, true, last_desc, is_last );
			if(desc) {
				calc_image();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
	fix_class_accommodations();
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
	const weg_t *w = bd->get_weg(water_wt);
	if (cnv && cnv->has_tall_vehicles())
	{
		if (w)
		{
			if (bd->is_height_restricted())
			{
				cnv->suche_neue_route();
				return false;
			}
		}
		else
		{
			// Check for low bridges on open water
			const grund_t* gr_above = world()->lookup(get_pos() + koord3d(0, 0, 1));
			if (env_t::pak_height_conversion_factor == 2 && gr_above && gr_above->get_weg_nr(0))
			{
				return false;
			}
		}
	}

	if(bd->is_water() || !w)
	{
		// If there are permissive constraints, this vehicle cannot
		// traverse open seas, but it may use lakes.
		if(bd->get_hoehe() > welt->get_groundwater())
		{
			return true;
		}
		else
		{
			return desc->get_way_constraints().get_permissive() == 0;
		}
	}
	// channel can have more stuff to check
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
	const uint8 convoy_vehicle_count = cnv ? cnv->get_vehicle_count() : 1;
	bool can_clear_way_constraints = true;
	if(convoy_vehicle_count < 2)
	{
		return(check_access(w) && w->get_max_speed() > 0 && check_way_constraints(*w));
	}
	else
	{
		// We must check the way constraints of each vehicle in the convoy
		if(check_access(w) && w->get_max_speed() > 0)
		{
			for(sint32 i = 0; i < convoy_vehicle_count; i++)
			{
				if(!cnv->get_vehicle(i)->check_way_constraints(*w))
				{
					can_clear_way_constraints = false;
					break;
				}
			}
		}
		else
		{
			return false;
		}
	}
	return can_clear_way_constraints;
}



/* Since slopes are handled different for ships
 * @author prissi
 */
void water_vehicle_t::calc_drag_coefficient(const grund_t *gr)
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

	if(previous_direction != direction) {
		// curve: higher friction
		current_friction *= 2;
	}
}


bool water_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	restart_speed = -1;

	if (leading)
	{
		assert(gr);

		if (!check_tile_occupancy(gr))
		{

			return false;
		}

		const weg_t* w = gr->get_weg(water_wt);

		if (w && w->is_crossing()) {
			// ok, here is a draw/turn-bridge ...
			crossing_t* cr = gr->find<crossing_t>();
			if (!cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}

bool water_vehicle_t::check_tile_occupancy(const grund_t* gr)
{
	if (gr->get_top() > 200)
	{
		return false;
	}

	const uint8 base_max_vehicles_on_tile = 127;
	const weg_t *w = gr->get_weg(water_wt);
	uint8 max_water_vehicles_on_tile = w ? w->get_desc()->get_max_vehicles_on_tile() : base_max_vehicles_on_tile;
	uint8 water_vehicles_on_tile = gr->get_top();

	if(water_vehicles_on_tile > max_water_vehicles_on_tile)
	{
		int relevant_water_vehicles_on_tile = 0;
		if(max_water_vehicles_on_tile < base_max_vehicles_on_tile && water_vehicles_on_tile < base_max_vehicles_on_tile)
		{
			for(sint32 n = gr->get_top(); n-- != 0;)
			{
				const obj_t *obj = gr->obj_bei(n);
				if(obj && obj->get_typ() == obj_t::water_vehicle)
				{
					const vehicle_t* water_craft = (const vehicle_t*)obj;
					if(water_craft->get_convoi()->is_loading())
					{
						continue;
					}
					const bool has_superior_loading_level = cnv->get_loading_level() > water_craft->get_convoi()->get_loading_level();
					const bool has_inferior_id = water_craft->get_convoi()->self.get_id() < get_convoi()->self.get_id();
					if(!has_superior_loading_level && has_inferior_id)
					{
						relevant_water_vehicles_on_tile ++;
						if(relevant_water_vehicles_on_tile >= max_water_vehicles_on_tile)
						{
							// Too many water vehicles already here.
							return false;
						}
					}
					else if(water_craft->get_convoi()->get_state() == convoi_t::DRIVING)
					{
						water_craft->get_convoi()->set_state(convoi_t::WAITING_FOR_CLEARANCE);
					}
				}
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

// this routine is called by find_route, to determined if we reached a destination
bool air_vehicle_t:: is_target(const grund_t *gr,const grund_t *)
{
	if(state!=looking_for_parking)
	{
		// search for the end of the runway
		const weg_t *w=gr->get_weg(air_wt);
		if(w  &&  w->get_desc()->get_styp()==type_runway)
		{
			// ok here is a runway
			ribi_t::ribi ribi= w->get_ribi_unmasked();
			//int success = 1;
			if(ribi_t::is_single(ribi)  &&  (ribi&approach_dir)!=0)
			{
				// pointing in our direction

				// Check for length
				const uint16 min_runway_length_meters = desc->get_minimum_runway_length();
				uint16 min_runway_length_tiles = ignore_runway_length ? 0 : min_runway_length_meters / welt->get_settings().get_meters_per_tile();
				if (!ignore_runway_length && min_runway_length_meters % welt->get_settings().get_meters_per_tile())
				{
					// Without this, this will be rounded down incorrectly.
					min_runway_length_tiles ++;
				}
				uint32 runway_length_tiles;

				bool runway_36_18 = false;
				bool runway_09_27 = false;

				switch (ribi)
				{
				case 1: // north
				case 4: // south
				case 5: // north-south
				case 7: // north-south-east
				case 13: // north-south-west
					runway_36_18 = true;
					break;
				case 2: // east
				case 8: // west
				case 10: // east-west
				case 11: // east-west-north
				case 14: // east-west-south
					runway_09_27 = true;
					break;
				case 15: // all
					runway_36_18 = true;
					runway_09_27 = true;
					break;
				default:
					runway_36_18 = false;
					runway_09_27 = false;
					break;
				}

				if (!runway_36_18 && !runway_09_27)
				{
					dbg->warning("bool air_vehicle_t:: is_target(const grund_t *gr,const grund_t *)", "Invalid runway directions for %u at %u, %u", cnv->self.get_id(), w->get_pos().x, w->get_pos().y);
					return false;
				}

				runway_length_tiles = w->get_runway_length(runway_36_18);

				return runway_length_tiles >= min_runway_length_tiles;
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


// for flying things, everywhere is good ...
// another function only called during route searching
ribi_t::ribi
air_vehicle_t::get_ribi(const grund_t *gr) const
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
int air_vehicle_t::get_cost(const grund_t *gr, const sint32, koord)
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(air_wt);
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
		assert(w);

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
bool
air_vehicle_t::check_next_tile(const grund_t *bd) const
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



/* finds a free stop, calculates a route and reserve the position
 * else return false
 */
bool air_vehicle_t::find_route_to_stop_position()
{
	// check for skipping circle
	route_t *rt=cnv->access_route();

//DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","can approach? (cnv %i)",cnv->self.get_id());

	grund_t const* const last = welt->lookup(rt->back());
	target_halt = last ? last->get_halt() : halthandle_t();
	if(!target_halt.is_bound()) {
		return true;	// no halt to search
	}

	// then: check if the search point is still on a runway (otherwise just proceed)
	grund_t const* const target = welt->lookup(rt->at(search_for_stop));
	if(target==NULL  ||  !target->hat_weg(air_wt)) {
		target_halt = halthandle_t();
		DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","no runway found at (%s)",rt->at(search_for_stop).get_str());
		return true;	// no runway any more ...
	}

	// is our target occupied?
//	DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","state %i",state);
	if(!target_halt->find_free_position(air_wt,cnv->self,obj_t::air_vehicle)  ) {
		target_halt = halthandle_t();
		DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","no free position found!");
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
//DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","some free: find route from index %i",search_for_stop);
		route_t target_rt;
		flight_state prev_state = state;
		state = looking_for_parking;
		if(!target_rt.find_route( welt, rt->at(search_for_stop), this, 500, ribi_t::all, cnv->get_highest_axle_load(), cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, 100, cnv->has_tall_vehicles())) {
DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()","found no route to free one");
			// circle slowly another round ...
			target_halt = halthandle_t();
			state = prev_state;
			return false;
		}
		state = prev_state;

		// now reserve our choice ...
		target_halt->reserve_position(welt->lookup(target_rt.back()), cnv->self);
		//DBG_MESSAGE("air_vehicle_t::find_route_to_stop_position()", "found free stop near %i,%i,%i", target_rt.back().x, target_rt.back().y, target_rt.back().z);
		cnv->update_route(search_for_stop, target_rt);
		cnv->set_next_stop_index(INVALID_INDEX);
		return true;
	}
}


// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
route_t::route_result_t air_vehicle_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool /*is_tall*/, route_t* route)
{
	if(leading) {
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
			block_reserver( touchdown - landing_distance, search_for_stop+1, false );
		}
	}
	target_halt = halthandle_t();	// no block reserved

	takeoff = touchdown = search_for_stop = INVALID_INDEX;
	const route_t::route_result_t result = calc_route_internal(welt, start, ziel, max_speed, cnv->get_highest_axle_load(), state, flying_height, target_height, runway_too_short, airport_too_close_to_the_edge, takeoff, touchdown, search_for_stop, *route);
	cnv->set_next_stop_index(INVALID_INDEX);
	return result;
}


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
route_t::route_result_t air_vehicle_t::calc_route_internal(
	karte_t *welt,
	const koord3d &start,            // input: start of (partial) route to calculate
	const koord3d &ziel,             // input: end of (partial) route to calculate
	sint32 max_speed,                // input: in the air
	uint32 weight,                   // input: gross weight of aircraft in kg (typical aircrafts don't have trailers)
	air_vehicle_t::flight_state &state, // input/output: at start
	sint16 &flying_height,               // input/output: at start
	sint16 &target_height,           // output: at end of takeoff
	bool &runway_too_short,          // output: either departure or arrival runway
	bool &airport_too_close_to_the_edge, // output: either airport locates close to the edge or not
	uint32 &takeoff,                 // output: route index to takeoff tile at departure airport
	uint32 &touchdown,               // output: scheduled route index to touchdown tile at arrival airport
	uint32 &search_for_stop,                  // output: scheduled route index to end of (required length of) arrival runway.
	route_t &route)                  // output: scheduled route from start to ziel
{
	//DBG_MESSAGE("air_vehicle_t::calc_route_internal()","search route from %i,%i,%i to %i,%i,%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	const bool vtol = get_desc()->get_minimum_runway_length() == 0;
	runway_too_short = false;
	airport_too_close_to_the_edge = false;
	search_for_stop = takeoff = touchdown = INVALID_INDEX;
	if(vtol)
	{
		takeoff = 0;
	}

	const grund_t* gr_start = welt->lookup(start);
	const weg_t *w_start = gr_start ? gr_start->get_weg(air_wt) : NULL;
	bool start_in_air = flying_height || w_start == NULL;

	const weg_t *w_ziel = welt->lookup(ziel)->get_weg(air_wt);
	bool end_in_air = w_ziel == NULL;

	if(!start_in_air)
	{
		// see, if we find a direct route: We are finished
		state = air_vehicle_t::taxiing;
		calc_altitude_level( get_desc()->get_topspeed() );
		route_t::route_result_t success = route.calc_route(welt, start, ziel, this, max_speed, weight, false, 0);
		if(success == route_t::valid_route)
		{
			// ok, we can taxi to our location
			return route_t::valid_route;
		}
	}

	koord3d search_start, search_end;
	if(start_in_air || vtol || (w_start && w_start->get_desc()->get_styp()==type_runway && ribi_t::is_single(w_start->get_ribi())))
	{
		// we start here, if we are in the air or at the end of a runway
		search_start = start;
		start_in_air = true;
		route.clear();
		//DBG_MESSAGE("air_vehicle_t::calc_route()","start in air at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}
	else
	{
		// not found and we are not on the takeoff tile (where the route search will fail too) => we try to calculate a complete route, starting with the way to the runway

		// second: find start runway end
		state = taxiing;
#ifdef USE_DIFFERENT_WIND
		approach_dir = get_approach_ribi( ziel, start );	// reverse
		//DBG_MESSAGE("air_vehicle_t::calc_route()","search runway start near %i,%i,%i with corner in %x",start.x,start.y,start.z, approach_dir);
#else
		approach_dir = ribi_t::northeast;	// reverse
		DBG_MESSAGE("air_vehicle_t::calc_route()","search runway start near (%s)",start.get_str());
#endif
		if (!route.find_route(welt, start, this, max_speed, ribi_t::all, weight, cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, welt->get_settings().get_max_route_steps(), cnv->has_tall_vehicles()))
		{
			// There might be no route at all, or the runway might be too short. Test this case.
			ignore_runway_length = true;
			if (!route.find_route(welt, start, this, max_speed, ribi_t::all, weight, cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, welt->get_settings().get_max_route_steps(), cnv->has_tall_vehicles()))
			{
				DBG_MESSAGE("air_vehicle_t::calc_route()", "failed");
				ignore_runway_length = false;
				return route_t::no_route;
			}
			else
			{
				ignore_runway_length = false;
				runway_too_short = true;
			}
		}
		// save the route
		search_start = route.back();
		//DBG_MESSAGE("air_vehicle_t::calc_route()","start at ground (%s)",search_start.get_str());
	}

	// second: find target runway end
	state = taxiing_to_halt;	// only used for search
#ifdef USE_DIFFERENT_WIND
	approach_dir = get_approach_ribi( start, ziel );	// reverse
	//DBG_MESSAGE("air_vehicle_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z,approach_dir);
#else
	approach_dir = ribi_t::southwest;	// reverse
	//DBG_MESSAGE("air_vehicle_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
#endif
	route_t end_route;

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::all, weight, cnv->get_tile_length(), cnv->get_weight_summary().weight / 1000, welt->get_settings().get_max_route_steps(), cnv->has_tall_vehicles())) {
		// well, probably this is a waypoint
		end_in_air = true;
		search_end = ziel;
	}
	else {
		// save target route
		search_end = end_route.back();
	}
	//DBG_MESSAGE("air_vehicle_t::calc_route()","end at ground (%s)",search_end.get_str());

	// create target route
	if(!start_in_air)
	{
		takeoff = route.get_count()-1;
		koord start_dir(welt->lookup(search_start)->get_weg_ribi(air_wt));
		if(start_dir!=koord(0,0)) {
			// add the start
			ribi_t::ribi start_ribi = ribi_t::backward(ribi_type(start_dir));
			const grund_t *gr=NULL;
			// add the start
			int endi = 1;
			int over = landing_distance/3;
			// now add all runway + 3 ...
			do {
				if(!welt->is_within_limits(search_start.get_2d()+(start_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_start.get_2d()+(start_dir*endi));
				if(over<landing_distance/3  ||  (gr->get_weg_ribi(air_wt)&start_ribi)==0) {
					over --;
				}
				endi ++;
				route.append(gr->get_pos());
			} while(  over>0  );
			// out of map
			if(gr==NULL) {
				dbg->error("air_vehicle_t::calc_route()","out of map!");
				return route_t::no_route;
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
			dbg->error("air_vehicle_t::calc_route()","Invalid route calculation: start is on a single direction field ...");
		}
		state = taxiing;
		flying_height = 0;
		target_height = (sint16)start.z*TILE_HEIGHT_STEP;
	}
	else {
		// init with current pos (in air ... )
		route.clear();
		route.append( start );
		state = flying;
		calc_altitude_level( desc->get_topspeed() ); // added for AFHP
		if(flying_height==0) {
			flying_height = 3*TILE_HEIGHT_STEP;
		}
		takeoff = 0;
		//		target_height = ((sint16)start.z+3)*TILE_HEIGHT_STEP;
		target_height = ((sint16)start.z+altitude_level)*TILE_HEIGHT_STEP;
	}
//DBG_MESSAGE("air_vehicle_t::calc_route()","take off ok");

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
			//			int over = landing_distance;
			int over = landing_distance;
			do {
				if(!welt->is_within_limits(search_end.get_2d()+(end_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_end.get_2d()+(end_dir*endi));
				if(over< landing_distance  ||  (gr->get_weg_ribi(air_wt)&end_ribi)==0) {
					over --;
				}
				endi ++;
				landing_start = gr->get_pos();
			} while(  over>0  );
		}
	}
	else {
		search_for_stop = touchdown = INVALID_INDEX;
	}

	// just some straight routes ...
	if(!route.append_straight_route(welt,landing_start)) {
		// should never fail ...
		dbg->error( "air_vehicle_t::calc_route()", "No straight route found!" );
		return route_t::no_route;
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
		static const koord circle_koord[HOLDING_PATTERN_LENGTH]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		for(  int  i=0;  i < HOLDING_PATTERN_LENGTH;  i++  ) {
			circlepos += circle_koord[(offset + i + HOLDING_PATTERN_LENGTH) % HOLDING_PATTERN_LENGTH];
			if(welt->is_within_limits(circlepos)) {
				route.append( welt->lookup_kartenboden(circlepos)->get_pos() );
			}
			else {
				// could only happen during loading old savegames;
				// in new versions it should not possible to build a runway here
				route.clear();
				dbg->error("air_vehicle_t::calc_route()","airport too close to the edge! (Cannot go to %i,%i!)",circlepos.x,circlepos.y);
				airport_too_close_to_the_edge = true;
				return route_t::no_route;
			}
		}

		//touchdown = route.get_count() + 2; //2 means HOLDING_PATTERN_OFFSET-1
		touchdown = route.get_count() + landing_distance - 1;
		route.append_straight_route(welt,search_end);


		// now the route to ziel search point (+1, since it will check before entering the tile ...)
		search_for_stop = route.get_count()-1;

		// define the endpoint on the runway
		uint32 runway_tiles = search_for_stop - touchdown;
		uint32 min_runway_tiles = desc->get_minimum_runway_length() / welt->get_settings().get_meters_per_tile() + 1;
		sint32 excess_of_tiles = runway_tiles - min_runway_tiles;
		search_for_stop -= excess_of_tiles;
		// now we just append the rest
		for( int i=end_route.get_count()-2;  i>=0;  i--  ) {
			route.append(end_route.at(i));
		}
	}

	//DBG_MESSAGE("air_vehicle_t::calc_route_internal()","departing=%i  touchdown=%i   search_for_stop=%i   total=%i  state=%i",takeoff, touchdown, search_for_stop, route.get_count()-1, state );
	return route_t::valid_route;
}


// BG, 08.08.2012: extracted from can_enter_tile()
route_t::route_result_t air_vehicle_t::reroute(const uint16 reroute_index, const koord3d &ziel)
{
	// new aircraft state after successful routing:
	air_vehicle_t::flight_state xstate = state;
	sint16 xflughoehe = flying_height;
	sint16 xtarget_height;
	bool xrunway_too_short;
	bool xairport_too_close_to_the_edge;
	uint32 xtakeoff;   // new route index to takeoff tile at departure airport
	uint32 xtouchdown; // new scheduled route index to touchdown tile at arrival airport
	uint32 xsuchen;    // new scheduled route index to end of (required length of) arrival runway.
	route_t xroute;    // new scheduled route from position at reroute_index to ziel

	route_t &route = *cnv->get_route();
	route_t::route_result_t done = calc_route_internal(welt, route.at(reroute_index), ziel,
																	speed_to_kmh(cnv->get_min_top_speed()), cnv->get_highest_axle_load(),
																	xstate, xflughoehe, xtarget_height,
																	xrunway_too_short, xairport_too_close_to_the_edge,
																	xtakeoff, xtouchdown, xsuchen, xroute);
	if (done)
	{
		// convoy replaces existing route starting at reroute_index with found route.
		cnv->update_route(reroute_index, xroute);
		cnv->set_next_stop_index(INVALID_INDEX);
		if (reroute_index == route_index)
		{
			state = xstate;
			flying_height = xflughoehe;
			target_height = xtarget_height;
			runway_too_short = xrunway_too_short;
		}
		if (takeoff >= reroute_index)
			takeoff = xtakeoff != INVALID_INDEX ? reroute_index + xtakeoff : INVALID_INDEX;
		if (touchdown >= reroute_index)
			touchdown = xtouchdown != INVALID_INDEX ? reroute_index + xtouchdown : INVALID_INDEX;
		if (search_for_stop >= reroute_index)
			search_for_stop = xsuchen != INVALID_INDEX ? reroute_index + xsuchen : INVALID_INDEX;
	}
	return done;
}


/* reserves runways (reserve true) or removes the reservation
 * finishes when reaching end tile or leaving the ground (end of runway)
 * @return true if the reservation is successful
 */
int air_vehicle_t::block_reserver( uint32 start, uint32 end, bool reserve ) const
{
	const route_t *route = cnv->get_route();
	if (route->empty()) {
		return 0;
	}

	bool start_now = false;

	uint16 runway_tiles = end - start;
	uint16 runway_meters = runway_tiles * welt->get_settings().get_meters_per_tile();
	const uint16 min_runway_length_meters = desc->get_minimum_runway_length();

	int success = runway_meters >= min_runway_length_meters ? 1 : 2;

	for(  uint32 i=start;  success  &&  i<end  &&  i<route->get_count();  i++) {
		grund_t *gr = welt->lookup(route->at(i));
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
			// we un-reserve also nonexistent tiles! (may happen during deletion)
			if(reserve) {
				start_now = true;
				if(!sch1->reserve(cnv->self,ribi_t::none)) {
					// unsuccessful => must unreserve all
					success = 0;
					end = i;
					break;
				}
				// reserve to the minimum runway length...
				uint16 current_runway_length_meters = ((i+1)-start)*welt->get_settings().get_meters_per_tile();
				if(i>start && current_runway_length_meters>min_runway_length_meters){
					//					std::cout << "reached minimum runway length? min = "<<min_runway_length_meters <<", len = "<<runway_meters << ", i="<<i<<std::endl;
					success = success == 0 ? 0 : runway_meters >= min_runway_length_meters ? 1 : 2;
					return success;
				}

				// end of runway? <- this will not be executed.
				if(i>start  &&  ribi_t::is_single(sch1->get_ribi_unmasked())  )
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
				// un-reserve from here (only used during sale, since there might be reserved tiles not freed)
				start_now = true;
			}
		}
	}

	// un-reserve if not successful
	if(!success  &&  reserve) {
		for(uint32 i=start;  i<end;  i++  ) {
			grund_t *gr = welt->lookup(route->at(i));
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
bool air_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8 )
{
	restart_speed = -1;

	assert(gr);

	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	route_t &route = *cnv->get_route();
	convoi_t::route_infos_t &route_infos = cnv->get_route_infos();

	uint16 next_block = cnv->get_next_stop_index() - 1;
	uint16 last_index = route.get_count() - 1;
	if(next_block > 65000)
	{
		convoi_t &convoy = *cnv;
		const sint32 brake_steps = convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), cnv->get_akt_speed());
		const sint32 route_steps = route_infos.calc_steps(route_infos.get_element(route_index).steps_from_start, route_infos.get_element(last_index).steps_from_start);
		const grund_t* gr = welt->lookup(cnv->get_schedule()->get_current_entry().pos);
		if(route_steps <= brake_steps && (!gr || !gr->get_depot())) // Do not recalculate a route if the route ends in a depot.
		{
			// we need a longer route to decide, whether we will have to throttle:
			schedule_t *schedule = cnv->get_schedule();
			uint8 index = schedule->get_current_stop();
			bool reversed = cnv->get_reverse_schedule();
			schedule->increment_index(&index, &reversed);
			if (reroute(last_index, schedule->entries[index].pos))
			{
				if (reversed)
					schedule->advance_reverse();
				else
					schedule->advance();
				cnv->set_next_stop_index(INVALID_INDEX);
				last_index = route.get_count() - 1;
			}
		}
	}

	if(  route_index < takeoff  &&  route_index > 1  &&  takeoff < last_index  ) {
		// check, if tile occupied by a plane on ground
		for(  uint8 i = 1;  i<gr->get_top();  i++  ) {
			obj_t *obj = gr->obj_bei(i);
			if(  obj->get_typ()==obj_t::air_vehicle  &&  ((air_vehicle_t *)obj)->is_on_ground()  ) {
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
		if(rw->get_desc()->get_styp()==type_elevated) {
			// try to reserve the runway
			//const uint16 runway_length_tiles = ((search_for_stop+1) - touchdown) - takeoff;
			const int runway_state = block_reserver(takeoff,takeoff+100,true);
			if(runway_state != 1)
			{
				// runway already blocked or too short
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

	if(  state == taxiing  ) {
		// enforce on ground for taxiing
		flying_height = 0;
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

//DBG_MESSAGE("air_vehicle_t::can_enter_tile()","index %i<>%i",route_index,touchdown);

	// check for another circle ...
	//	if(  route_index == touchdown - HOLDING_PATTERN_OFFSET )
	//circling now!
	if(  route_index == touchdown - landing_distance){
		const int runway_state = block_reserver( touchdown, search_for_stop+1 , true );
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

	if(  route_index == touchdown - HOLDING_PATTERN_LENGTH - landing_distance &&  state != circling  )
	{
		// just check, if the end of runway is free; we will wait there
		//		std::cout << "reserve 2: "<<state<<" "<< touchdown <<" "<<search_for_stop+1<< std::endl;
		const int runway_state = block_reserver( touchdown , search_for_stop+1 , true );
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

	if(route_index==search_for_stop &&  state==landing) {

		// we will wait in a step, much more simulation friendly
		// and the route finder is not re-entrant!
		if(!cnv->is_waiting()) {
			return false;
		}

		// nothing free here?
		if(find_route_to_stop_position()) {
			// stop reservation successful
			// unreserve when the aircraft reached the end of runway
			block_reserver( touchdown, search_for_stop+1, false );
			//			std::cout << "unreserve 3: "<< state <<" "<< touchdown << std::endl;
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


air_vehicle_t::air_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) :
#ifdef INLINE_OBJ_TYPE
	vehicle_t(obj_t::air_vehicle)
#else
    vehicle_t()
#endif
{
	rdwr_from_convoi(file);
	calc_altitude_level( desc->get_topspeed() );
	runway_too_short = false;
	airport_too_close_to_the_edge = false;

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_leading) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL)
		{
			const goods_desc_t* gd = NULL;
			ware_t w;
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				if (!fracht[i].empty())
				{
					w = fracht[i].front();
					gd = w.get_desc();
					break;
				}
			}
			if (!gd)
			{
				// Important: This must be the same default ware as used in rdwr_from_convoi.
				gd = goods_manager_t::passengers;
			}

			dbg->warning("air_vehicle_t::air_vehicle_t()", "try to find a fitting vehicle for %s.", gd->get_name());
			desc = vehicle_builder_t::get_best_matching(air_wt, 0, 101, 1000, 800, gd, true, last_desc, is_last );
			if(desc) {
				calc_image();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
	fix_class_accommodations();
}


air_vehicle_t::air_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
#ifdef INLINE_OBJ_TYPE
    vehicle_t(obj_t::air_vehicle, pos, desc, player)
#else
	vehicle_t(pos, desc, player)
#endif
{
	cnv = cn;
	state = taxiing;
	flying_height = 0;
	target_height = pos.z;
	runway_too_short = false;
	airport_too_close_to_the_edge = false;
	calc_altitude_level( desc->get_topspeed() );
}



air_vehicle_t::~air_vehicle_t()
{
	// mark aircraft (after_image) dirty, since we have no "real" image
	const int raster_width = get_current_tile_raster_width();
	sint16 yoff = tile_raster_scale_y(-flying_height - get_hoff() - 2, raster_width);

	mark_image_dirty( image, yoff);
	mark_image_dirty( image, 0 );
}



void air_vehicle_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("air_vehicle_t::set_convoi()","%p",c);
	if(leading  &&  (uint64)cnv > 1ll) {
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
				//				block_reserver( touchdown, search_for_stop+1, false );
				//				std::cout << "unreserve 4: "<<state<<" "<< touchdown << std::endl;
				block_reserver( touchdown, search_for_stop+1, false );
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
							//							std::cout << "reserve 5: "<<state<<" "<< touchdown << std::endl;
							block_reserver( touchdown, search_for_stop+1, true );
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




schedule_t * air_vehicle_t::generate_new_schedule() const
{
	return new airplane_schedule_();
}

void rail_vehicle_t::rdwr_from_convoi(loadsave_t* file)
{
	xml_tag_t t( file, "rail_vehicle_t" );

	vehicle_t::rdwr_from_convoi(file);
#ifdef SPECIAL_RESCUE_12_5
	if(file->get_extended_version() >= 12 && file->is_saving())
#else
	if(file->get_extended_version() >= 12)
#endif
	{
		uint8 wm = (uint8)working_method;
		file->rdwr_byte(wm);
		working_method = (working_method_t)wm;
	}
}


void air_vehicle_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t t( file, "air_vehicle_t" );

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
	file->rdwr_long(search_for_stop);
	file->rdwr_long(touchdown);
	file->rdwr_long(takeoff);
}



#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two different directions for the runway search
uint8 air_vehicle_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_type(ziel-start);	// reverse
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
	if(  !get_flag(obj_t::dirty)  ) {
		mark_image_dirty( image, 0 );
		set_flag( obj_t::dirty );
	}

	sint32 new_speed_limit = speed_unlimited();
	sint32 new_friction = 0;

	// take care of in-flight height ...
	const sint16 h_cur = (sint16)get_pos().z*TILE_HEIGHT_STEP;
	const sint16 h_next = (sint16)pos_next.z*TILE_HEIGHT_STEP;

	switch(state) {
		case departing: {
			flying_height = 0;
			target_height = h_cur;
			new_friction = max( 1, 28/(1+(route_index-takeoff)*2) ); // 9 5 4 3 2 2 1 1...

			// take off, when a) end of runway or b) last tile of runway or c) has reached minimum runway length
			weg_t *weg=welt->lookup(get_pos())->get_weg(air_wt);
			const sint16 runway_tiles_so_far = route_index - takeoff;
			const sint16 runway_meters_so_far = runway_tiles_so_far * welt->get_settings().get_meters_per_tile();
			const uint16 min_runway_length_meters = desc->get_minimum_runway_length();

			if(  (weg==NULL  ||  // end of runway (broken runway)
				 weg->get_desc()->get_styp()!=type_runway  ||  // end of runway (grass now ... )
				 (route_index>takeoff+1  &&  ribi_t::is_single(weg->get_ribi_unmasked())) )  ||  // single ribi at end of runway
				 (min_runway_length_meters && runway_meters_so_far >= min_runway_length_meters)   //  has reached minimum runway length
			) {
				state = flying;
				play_sound();
				new_friction = 1;
				block_reserver( takeoff, takeoff+100, false );
				calc_altitude_level( desc->get_topspeed() );
				flying_height = h_cur - h_next;
				target_height = h_cur+TILE_HEIGHT_STEP*(altitude_level+(sint16)get_pos().z);//modified
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
			if(  target_height-h_next > TILE_HEIGHT_STEP*altitude_level*4/3 + (sint16)get_pos().z  ) {
				// Move down
				target_height -= TILE_HEIGHT_STEP*2;
			}
			else if(  target_height-h_next < TILE_HEIGHT_STEP*altitude_level*2/3 + (sint16)get_pos().z   ) {
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

			if (route_index >= touchdown ) {
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
						const sint32 runway_left = search_for_stop - route_index;
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
		ypos += tile_raster_scale_y(get_yoff() - current_flughohe - hoff - 2, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		get_screen_offset( xpos, ypos, raster_width );

		display_swap_clip_wh(CLIP_NUM_VAR);

		// will be dirty
		// the aircraft!!!
		display_color( image, xpos, ypos, get_player_nr(), true, true/*get_flag(obj_t::dirty)*/  CLIP_NUM_PAR);
#ifndef MULTI_THREAD
		vehicle_t::display_after(xpos_org, ypos_org - tile_raster_scale_y(current_flughohe - hoff - 2, raster_width), is_global);
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

		vehicle_t::display_overlay(xpos_org, ypos_org - tile_raster_scale_y(current_flughohe - get_hoff() - 2, raster_width));
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


const char *air_vehicle_t:: is_deletable(const player_t *player)
{
	if (is_on_ground()) {
		return vehicle_t:: is_deletable(player);
	}
	return NULL;
}
