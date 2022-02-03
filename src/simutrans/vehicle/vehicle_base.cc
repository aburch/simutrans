/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "vehicle_base.h"

#include "road_vehicle.h"
#include "simroadtraffic.h"
#include "vehicle.h"

#include "../dataobj/environment.h"
#include "../simunits.h"
#include "../world/simworld.h"
#include "../obj/crossing.h"
#include "../simconvoi.h"


/* get dx and dy from dir (just to remind you)
 * any vehicle (including city cars and pedestrians)
 * will go this distance per sync step.
 * (however, the real dirs are only calculated during display, these are the old ones)
 */
sint8 vehicle_base_t::dxdy[ 8*2 ] = {
	-2,  1, // s
	-2, -1, // w
	-4,  0, // sw
	 0,  2, // se
	 2, -1, // n
	 2,  1, // e
	 4,  0, // ne
	 0, -2  // nw
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
	old_diagonal_vehicle_steps_per_tile = (uint8)(130560u/max(old_diagonal_multiplier, 1u)) + 1;
}


// if true, convoi, must restart!
bool vehicle_base_t::need_realignment() const
{
	return old_diagonal_vehicle_steps_per_tile!=diagonal_vehicle_steps_per_tile  &&  ribi_t::is_bend(direction);
}

// [0]=xoff [1]=yoff
sint8 vehicle_base_t::driveleft_base_offsets[8][2] =
{
	{  12,  6 },
	{ -12,  6 },
	{   0,  6 },
	{  12,  0 },
	{ -12, -6 },
	{  12, -6 },
	{   0, -6 },
	{ -12,  0 }
};

// [0]=xoff [1]=yoff
sint8 vehicle_base_t::overtaking_base_offsets[8][2];

// recalc offsets for overtaking
void vehicle_base_t::set_overtaking_offsets( bool driving_on_the_left )
{
	const sint8 sign = driving_on_the_left ? -1 : 1;
	// a tile has the internal size of
	const sint8 XOFF = 12;
	const sint8 YOFF = 6;

	overtaking_base_offsets[0][0] =  sign * XOFF;
	overtaking_base_offsets[1][0] = -sign * XOFF;
	overtaking_base_offsets[2][0] = 0;
	overtaking_base_offsets[3][0] =  sign * XOFF;
	overtaking_base_offsets[4][0] = -sign * XOFF;
	overtaking_base_offsets[5][0] =  sign * XOFF;
	overtaking_base_offsets[6][0] = 0;
	overtaking_base_offsets[7][0] =  sign * (-XOFF-YOFF);

	overtaking_base_offsets[0][1] =  sign * YOFF;
	overtaking_base_offsets[1][1] =  sign * YOFF;
	overtaking_base_offsets[2][1] =  sign * YOFF;
	overtaking_base_offsets[3][1] = 0;
	overtaking_base_offsets[4][1] = -sign * YOFF;
	overtaking_base_offsets[5][1] = -sign * YOFF;
	overtaking_base_offsets[6][1] = -sign * YOFF;
	overtaking_base_offsets[7][1] = 0;
}


/**
 * Checks if this vehicle must change the square upon next move
 * THIS IS ONLY THERE FOR LOADING OLD SAVES!
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
		dbg->error("vehicle_base_t::leave_tile()", "%s %p could not be removed from (%s)", get_name(), (void *)this, get_pos().get_str());
		DBG_MESSAGE("vehicle_base_t::leave_tile()", "checking all plan squares");

		// check, whether it is on another height ...
		const planquadrat_t *pl = welt->access( get_pos().get_2d() );
		if(  pl  ) {
			gr = pl->get_boden_von_obj(this);
			if(  gr  ) {
				gr->obj_remove(this);
				dbg->warning("vehicle_base_t::leave_tile()", "Removed %s %p from (%s)", get_name(), (void *)this, get_pos().get_str());
			}
			return;
		}

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->get_size().y; k.y++) {
			for(k.x=0; k.x<welt->get_size().x; k.x++) {
				grund_t *gr = welt->access( k )->get_boden_von_obj(this);
				if(gr && gr->obj_remove(this)) {
					dbg->warning("vehicle_base_t::leave_tile()", "Removed %s %p from (%s)", get_name(), (void *)this, k.get_str());
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehicle_base_t::leave_tile()", "%s %p was not found on any map square!", get_name(), (void *)this);
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
	use_calc_height = false;   // assume, we are only needed after next hop
	zoff_start = zoff_end = 0; // assume flat way

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
	return ((h_start*steps + h_end*(256-steps))*raster_width) >> 9;
}


/* true, if one could pass through this field
 * also used for citycars, thus defined here
 */
vehicle_base_t *vehicle_base_t::no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_direction, const uint8 next_direction, const uint8 next_90direction )
{
	// Search vehicle
	for(  uint8 pos=1;  pos<(uint8)gr->get_top();  pos++  ) {
		if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(pos))  ) {
			if(  v->get_typ()==obj_t::pedestrian  ) {
				continue;
			}

			// check for car
			uint8 other_direction=255;
			bool other_moving = false;
			if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
				// ignore ourself
				if(  cnv == at->get_convoi()  ) {
					continue;
				}
				other_direction = at->get_direction();
				other_moving = at->get_convoi()->get_akt_speed() > kmh_to_speed(1);
			}
			// check for city car
			else if(  v->get_waytype() == road_wt  ) {
				other_direction = v->get_direction();
				if(  private_car_t const* const sa = obj_cast<private_car_t>(v)  ){
					other_moving = sa->get_current_speed() > 1;
				}
			}

			// ok, there is another car ...
			if(  other_direction != 255  ) {
				if(  next_direction == other_direction  &&  !ribi_t::is_threeway(gr->get_weg_ribi(road_wt))  ) {
					// cars going in the same direction and no crossing => that mean blocking ...
					return v;
				}

				const ribi_t::ribi other_90direction = (gr->get_pos().get_2d() == v->get_pos_next().get_2d()) ? other_direction : calc_direction(gr->get_pos(), v->get_pos_next());
				if(  other_90direction == next_90direction  ) {
					// Want to exit in same as other   ~50% of the time
					return v;
				}

				const bool drives_on_left = welt->get_settings().is_drive_left();
				const bool across = next_direction == (drives_on_left ? ribi_t::rotate45l(next_90direction) : ribi_t::rotate45(next_90direction)); // turning across the opposite directions lane
				const bool other_across = other_direction == (drives_on_left ? ribi_t::rotate45l(other_90direction) : ribi_t::rotate45(other_90direction)); // other is turning across the opposite directions lane
				if(  other_direction == next_direction  &&  !(other_across || across)  ) {
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
				else if(  other_direction == current_direction  &&  current_90direction == ribi_t::none  ) {
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
