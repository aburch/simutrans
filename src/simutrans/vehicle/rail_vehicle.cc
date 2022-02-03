/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "rail_vehicle.h"

#include "../builder/goods_manager.h"
#include "../builder/vehikelbauer.h"
#include "../simware.h"
#include "../simconvoi.h"
#include "../world/simworld.h"
#include "../obj/way/schiene.h"
#include "../obj/depot.h"
#include "../obj/roadsign.h"
#include "../obj/signal.h"
#include "../dataobj/schedule.h"
#include "../obj/crossing.h"


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


rail_vehicle_t::rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
	vehicle_t(pos, desc, player)
{
	cnv = cn;
}


rail_vehicle_t::~rail_vehicle_t()
{
	if (cnv && leading) {
		route_t const& r = *cnv->get_route();
		if (!r.empty() && route_index < r.get_count()) {
			// free all reserved blocks
			route_t::index_t dummy;
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
					route_t::index_t dummy;
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
					dbg->warning("rail_vehicle_t::set_convoi()", "convoi %i had a too high route index! (%i of max %i)", c->self.get_id(), route_index, r.get_count() - 1);
				}
				// set default next stop index
				c->set_next_stop_index( max(route_index,1)-1 );
				// need to reserve new route?
				if(  !check_for_finish  &&  c->get_state()!=convoi_t::SELF_DESTRUCT  &&  (c->get_state()==convoi_t::DRIVING  ||  c->get_state()>=convoi_t::LEAVING_DEPOT)  ) {
					sint32 num_index = cnv==(convoi_t *)1 ? 1001 : 0; // only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
					route_t::index_t next_signal, next_crossing;
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
	if(leading  &&  route_index<cnv->get_route()->get_count()) {
		// free all reserved blocks
		route_t::index_t dummy;
		block_reserver(cnv->get_route(), cnv->back()->get_route_index(), dummy, dummy, target_halt.is_bound() ? 100000 : 1, false, true);
	}
	cnv->set_next_reservation_index( 0 ); // nothing to reserve
	target_halt = halthandle_t(); // no block reserved
	// use length 8888 tiles to advance to the end of all stations
	const uint16 convoy_length = world()->get_settings().get_stop_halt_as_scheduled() ? cnv->get_tile_length() : 8888;
	return route->calc_route(welt, start, ziel, this, max_speed, convoy_length );
}


bool rail_vehicle_t::check_next_tile(const grund_t *bd) const
{
	schiene_t const* const sch = obj_cast<schiene_t>(bd->get_weg(get_waytype()));
	if(  !sch  ) {
		return false;
	}

	// diesel and steam engines can use electrified track as well.
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
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_owner_nr()) ) == 0  ) {
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
		// check if the slope is upwards, relative to the previous tile
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


bool rail_vehicle_t::is_longblock_signal_clear(signal_t *sig, route_t::index_t next_block, sint32 &restart_speed)
{
	// longblock signal: first check, whether there is a signal coming up on the route => just like normal signal
	route_t::index_t next_signal, next_crossing;
	if(  !block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		// not even the "Normal" signal route part is free => no bother checking further on
		sig->set_state( roadsign_t::STATE_RED );
		restart_speed = 0;
		return false;
	}

	if(  next_signal != route_t::INVALID_INDEX  ) {
		// success, and there is a signal before end of route => finished
		sig->set_state( roadsign_t::STATE_GREEN );
		cnv->set_next_stop_index( min( next_crossing, next_signal ) );
		return true;
	}

	// no signal before end_of_route => need to do route search in a step
	if(  !cnv->is_waiting()  ) {
		restart_speed = -1;
		return false;
	}

	// now we can use the route search array, since we are in step mode
	// (route until end is already reserved at this point!)
	uint8 schedule_index = cnv->get_schedule()->get_current_stop()+1;
	route_t target_rt;
	koord3d cur_pos = cnv->get_route()->back();
	route_t::index_t dummy;
	route_t::index_t next_next_signal = route_t::INVALID_INDEX;

	if(schedule_index >= cnv->get_schedule()->get_count()) {
		schedule_index = 0;
	}
	while(  schedule_index != cnv->get_schedule()->get_current_stop()  ) {
		// now search
		// search for route
		bool success = target_rt.calc_route( welt, cur_pos, cnv->get_schedule()->entries[schedule_index].pos, this, speed_to_kmh(cnv->get_min_top_speed()), 8888 /*cnv->get_tile_length()*/ );
		if(  target_rt.is_contained(get_pos())  ) {
			// do not reserve route going through my current stop&
			break;
		}
		if(  success  ) {
			success = block_reserver( &target_rt, 1, next_next_signal, dummy, 0, true, false );
			block_reserver( &target_rt, 1, dummy, dummy, 0, false, false );
		}

		if(  success  ) {
			// ok, would be free
			if(  next_next_signal<target_rt.get_count()  ) {
				// and here is a signal => finished
				// (however, if it is this signal, we need to renew reservation ...
				if(  target_rt.at(next_next_signal) == cnv->get_route()->at( next_block )  ) {
					block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false );
				}
				sig->set_state( roadsign_t::STATE_GREEN );
				cnv->set_next_stop_index( min( min( next_crossing, next_signal ), cnv->get_route()->get_count()-1-1 ) );
				return true;
			}
		}

		if(  !success  ) {
			block_reserver( cnv->get_route(), next_block+1, next_next_signal, dummy, 0, false, false );
			sig->set_state( roadsign_t::STATE_RED );
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


bool rail_vehicle_t::is_choose_signal_clear(signal_t *sig, const route_t::index_t start_block, sint32 &restart_speed)
{
	bool choose_ok = false;
	target_halt = halthandle_t();

	route_t::index_t next_signal, next_crossing;
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
			sig->set_state(  roadsign_t::STATE_GREEN );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_state(  roadsign_t::STATE_RED );
		restart_speed = 0;
		return false;
	}

	target_halt = target->get_halt();
	if(  !block_reserver( cnv->get_route(), start_block+1, next_signal, next_crossing, 100000, true, false )  ) {
		// no free route to target!
		// note: any old reservations should be invalid after the block reserver call.
		// => We can now start freshly all over

		if(!cnv->is_waiting()) {
			// we are driving and cannot search yet
			restart_speed = -1;
			target_halt = halthandle_t();
			return false;
		}

		// now we are in a step and can use the route search array
		route_t target_rt;
		const int richtung = ribi_type(get_pos(), pos_next); // to avoid confusion at diagonals
		if(  !target_rt.find_route( welt, cnv->get_route()->at(start_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, welt->get_settings().get_max_choose_route_steps() )  ) {
			// nothing empty or not route with less than get_max_choose_route_steps() tiles
			target_halt = halthandle_t();
			sig->set_state(  roadsign_t::STATE_RED );
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
				sig->set_state(  roadsign_t::STATE_RED );
				restart_speed = 0;
				return false;
			}
		}
		// reserved route to target
	}
	sig->set_state(  roadsign_t::STATE_GREEN );
	cnv->set_next_stop_index( min( next_crossing, next_signal ) );
	return true;
}


bool rail_vehicle_t::is_pre_signal_clear(signal_t *sig, route_t::index_t next_block, sint32 &restart_speed)
{
	// parse to next signal; if needed recurse, since we allow cascading
	route_t::index_t next_signal, next_crossing;
	if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		if(  next_signal == route_t::INVALID_INDEX  ||  cnv->get_route()->at(next_signal) == cnv->get_route()->back()  ||  is_signal_clear( next_signal, restart_speed )  ) {
			// ok, end of route => we can go
			sig->set_state( roadsign_t::STATE_GREEN );
			cnv->set_next_stop_index( min( next_signal, next_crossing ) );
			return true;
		}
		// when we reached here, the way is apparently not free => release reservation and set state to next free
		sig->set_state( roadsign_t::STATE_YELLOW );
		block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, false, false );
		restart_speed = 0;
		return false;
	}

	// if we end up here, there was not even the next block free
	sig->set_state( roadsign_t::STATE_RED );
	restart_speed = 0;
	return false;
}



bool rail_vehicle_t::is_priority_signal_clear(signal_t *sig, route_t::index_t next_block, sint32 &restart_speed)
{
	// parse to next signal; if needed recurse, since we allow cascading
	route_t::index_t next_signal, next_crossing;

	if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
		if(  next_signal == route_t::INVALID_INDEX  ||  cnv->get_route()->at(next_signal) == cnv->get_route()->back()  ||  is_signal_clear( next_signal, restart_speed )  ) {
			// ok, end of route => we can go
			sig->set_state( roadsign_t::STATE_GREEN );
			cnv->set_next_stop_index( min( next_signal, next_crossing ) );

			return true;
		}

		// when we reached here, the way after the last signal is not free though the way before is => we can still go
		if(  cnv->get_next_stop_index()<=next_signal+1  ) {
			// only show third aspect on last signal of cascade
			sig->set_state( roadsign_t::STATE_YELLOW );
		}
		else {
			sig->set_state( roadsign_t::STATE_GREEN );
		}
		cnv->set_next_stop_index( min( next_signal, next_crossing ) );

		return false;
	}

	// if we end up here, there was not even the next block free
	sig->set_state( roadsign_t::STATE_RED );
	restart_speed = 0;

	return false;
}


bool rail_vehicle_t::is_signal_clear(route_t::index_t next_block, sint32 &restart_speed)
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

	// simple signal: fail, if next block is not free
	if(  sig_desc->is_simple_signal()  ) {

		route_t::index_t next_signal, next_crossing;
		if(  block_reserver( cnv->get_route(), next_block+1, next_signal, next_crossing, 0, true, false )  ) {
			sig->set_state(  roadsign_t::STATE_GREEN );
			cnv->set_next_stop_index( min( next_crossing, next_signal ) );
			return true;
		}
		// not free => wait here if directly in front
		sig->set_state(  roadsign_t::STATE_RED );
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
	route_t::index_t next_signal, next_crossing;

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
	route_t::index_t next_block = cnv->get_next_stop_index()-1;
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
						cnv->set_next_stop_index( route_t::INVALID_INDEX );
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


/**
 * reserves or un-reserves all blocks and returns the handle to the next block (if there)
 * if count is larger than 1, (and defined) maximum MAX_CHOOSE_BLOCK_TILES tiles will be checked
 * (freeing or reserving a choose signal path)
 * if (!reserve && force_unreserve) then un-reserve everything till the end of the route
 * return the last checked block
 */
bool rail_vehicle_t::block_reserver(const route_t *route, route_t::index_t start_index, route_t::index_t &next_signal_index, route_t::index_t &next_crossing_index, int count, bool reserve, bool force_unreserve  ) const
{
	bool success=true;
#ifdef MAX_CHOOSE_BLOCK_TILES
	int max_tiles=2*MAX_CHOOSE_BLOCK_TILES; // max tiles to check for choosesignals
#endif
	slist_tpl<grund_t *> signs; // switch all signals on their way too ...

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
	route_t::index_t i = start_index;
	next_signal_index = route_t::INVALID_INDEX;
	next_crossing_index = route_t::INVALID_INDEX;
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
			if(next_crossing_index==route_t::INVALID_INDEX  &&  sch1->is_crossing()) {
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
			}
			if(sch1->has_signal()) {
				signal_t* signal = gr->find<signal_t>();
				if(signal) {
					signal->set_state(roadsign_t::STATE_RED);
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
		for ( route_t::index_t j=start_index; j<i; j++) {
			schiene_t * sch1 = (schiene_t *)welt->lookup( route->at(j))->get_weg(get_waytype());
			sch1->unreserve(cnv->self);
		}
		cnv->set_next_reservation_index( start_index );
		return false;
	}

	// ok, switch everything green ...
	for(grund_t* const g : signs) {
		if (signal_t* const signal = g->find<signal_t>()) {
			signal->set_state(roadsign_t::STATE_GREEN);
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
				// tell next signal?
				// and switch to red
				if(sch0->has_signal()) {
					signal_t* sig = gr->find<signal_t>();
					if(sig) {
						sig->set_state(roadsign_t::STATE_RED);
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
