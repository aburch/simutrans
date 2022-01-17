/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "road_vehicle.h"

#include "simroadtraffic.h"

#include "../bauer/goods_manager.h"
#include "../bauer/vehikelbauer.h"
#include "../boden/wege/strasse.h"
#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../descriptor/citycar_desc.h"
#include "../obj/crossing.h"
#include "../obj/roadsign.h"
#include "../player/simplay.h"
#include "../simware.h"
#include "../simworld.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simmesg.h"
#include "../utils/cbuffer_t.h"


road_vehicle_t::road_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
	vehicle_t(pos, desc, player)
{
	cnv = cn;
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
		calc_disp_lane();
	}
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
	target_halt = halthandle_t(); // no block reserved
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
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_owner_nr()) ) == 0  ) {
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
		// check if the slope is upwards, relative to the previous tile
		// 15 hardcoded, see get_cost_upslope()
		costs += 15 * get_sloping_upwards( gr->get_weg_hang(), from );
	}
	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool road_vehicle_t::is_target(const grund_t *gr, const grund_t *prev_gr) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
		// now we must check the predecessor => try to advance as much as possible
		if(prev_gr!=NULL) {
			const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
			ribi_t::ribi ribi = ribi_type(dir);
			if(  gr->get_weg(get_waytype())->get_ribi_maske() & ribi  ) {
				// one way sign wrong direction
				return false;
			}
			grund_t *to;
			if(  !gr->get_neighbour(to,road_wt,ribi)  ||  !(to->get_halt()==target_halt)  ||  (gr->get_weg(get_waytype())->get_ribi_maske() & ribi_type(dir))!=0  ||  !target_halt->is_reservable(to,cnv->self)  ) {
				// end of stop: Is it long enough?
				uint16 tiles = cnv->get_tile_length();
				while(  tiles>1  ) {
					if(  !gr->get_neighbour(to,get_waytype(),ribi_t::backward(ribi))  ||  !(to->get_halt()==target_halt)  ||  !target_halt->is_reservable(to,cnv->self)  ) {
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
void road_vehicle_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	vehicle_base_t::get_screen_offset( xoff, yoff, raster_width );

	if(  welt->get_settings().is_drive_left()  ) {
		const int drive_left_dir = ribi_t::get_dir(get_direction());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}

	// eventually shift position to take care of overtaking
	if(cnv) {
		if(  cnv->is_overtaking()  ) {
			xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
			yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
		}
		else if(  cnv->is_overtaken()  ) {
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
			if(  const grund_t *gr_current = welt->lookup(get_pos())  ) {
				if(  gr_current  &&  gr_current->ist_uebergang()  ) {
					return true;
				}
			}
			// always allow to leave traffic lights (avoid vehicles stuck on crossings directly after though)
			if(  const grund_t *gr_current = welt->lookup(get_pos())  ) {
				if(  const roadsign_t *rs = gr_current->find<roadsign_t>()  ) {
					if(  rs  &&  rs->get_desc()->is_traffic_light()  &&  !gr->ist_uebergang()  ) {
						return true;
					}
				}
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

		vehicle_base_t *obj = NULL;
		uint32 test_index = route_index + 1u;

		// way should be clear for overtaking: we checked previously
		if(  !cnv->is_overtaking()  ) {
			// calculate new direction
			route_t const& r = *cnv->get_route();
			koord3d next = route_index < r.get_count() - 1u ? r.at(route_index + 1u) : pos_next;
			ribi_t::ribi curr_direction   = get_direction();
			ribi_t::ribi curr_90direction = calc_direction(get_pos(), pos_next);
			ribi_t::ribi next_direction   = calc_direction(get_pos(), next);
			ribi_t::ribi next_90direction = calc_direction(pos_next, next);
			obj = no_cars_blocking( gr, cnv, curr_direction, next_direction, next_90direction );

			// do not block intersections
			const bool drives_on_left = welt->get_settings().is_drive_left();
			bool int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));

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

				// check cars
				curr_direction   = next_direction;
				curr_90direction = next_90direction;
				if(  test_index + 1u < r.get_count()  ) {
					next                 = r.at(test_index + 1u);
					next_direction   = calc_direction(r.at(test_index - 1u), next);
					next_90direction = calc_direction(r.at(test_index),      next);
					obj = no_cars_blocking( gr, cnv, curr_direction, next_direction, next_90direction );
				}
				else {
					next                 = r.at(test_index);
					next_90direction = calc_direction(r.at(test_index - 1u), next);
					if(  curr_direction == next_90direction  ||  !gr->is_halt()  ) {
						// check cars but allow to enter intersection if we are turning even when a car is blocking the halt on the last tile of our route
						// preserves old bus terminal behaviour
						obj = no_cars_blocking( gr, cnv, curr_direction, next_90direction, ribi_t::none );
					}
				}

				// check roadsigns
				if(  str->has_sign()  ) {
					rs = gr->find<roadsign_t>();
					if(  rs  ) {
						// check, if we reached a choose point
						if(  rs->is_free_route(curr_90direction)  &&  !target_halt.is_bound()  ) {
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
		}

		// stuck message ...
		if(  obj  &&  !second_check_count  ) {
			if(  obj->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehicle is stuck too
				restart_speed = 0;
				cnv->set_tiles_overtaking(0);
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
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
							convoi_t* const ocnv = car->get_convoi();
							if(  cnv->can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehicle(0)->get_steps())  ) {
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
				cnv->set_tiles_overtaking(0);
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


void road_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);
	calc_disp_lane();

	const int cargo = get_total_cargo();
	weg_t *str = gr->get_weg(road_wt);
	if (str) {
		str->book(cargo, WAY_STAT_GOODS);
		if (leading)  {
			str->book(1, WAY_STAT_CONVOIS);
		}
	}
	if (leading)  {
		cnv->update_tiles_overtaking();
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
		bool target=(bool)cnv; // only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
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
