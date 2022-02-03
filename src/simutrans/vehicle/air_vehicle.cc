/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "air_vehicle.h"

#include "../obj/way/runway.h"
#include "../simhalt.h"
#include "../simconvoi.h"
#include "../world/simworld.h"
#include "../dataobj/environment.h"
#include "../simware.h"
#include "../builder/vehikelbauer.h"
#include "../dataobj/schedule.h"


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
			costs += rw->get_reservation_count()*9; // encourage detours even during take off
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
			// here a height check could avoid too high mountains
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
		return true; // already searched with success
	}

	// check for skipping circle
	route_t *rt=cnv->access_route();

//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","can approach? (cnv %i)",cnv->self.get_id());

	grund_t const* const last = welt->lookup(rt->back());
	target_halt = last ? last->get_halt() : halthandle_t();
	if(!target_halt.is_bound()) {
		return true; // no halt to search
	}

	// then: check if the search point is still on a runway (otherwise just proceed)
	grund_t const* const target = welt->lookup(rt->at(search_for_stop));
	if(target==NULL  ||  !target->hat_weg(air_wt)) {
		target_halt = halthandle_t();
		block_reserver( search_for_stop, 0xFFFFu, false ); // unreserve all tiles
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no runway found at (%s)",rt->at(search_for_stop).get_str());
		return true; // no runway any more ...
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
		if(!target_rt.find_route( welt, rt->at(search_for_stop), this, 500, ribi_t::all, welt->get_settings().get_max_choose_route_steps() )) {
DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","found no route to free one");

			// just make sure, that there is a route at all, otherwise start route search again
			for(  uint32 i=search_for_stop;  i<rt->get_count();  i++  ) {
				grund_t const* const target = welt->lookup( rt->at( i ) );
				if(  target == NULL  ||  !target->hat_weg( air_wt )  ) {
					DBG_MESSAGE( "aircraft_t::find_route_to_stop_position()", "no runway found at (%s)", rt->at( search_for_stop ).get_str() );
					get_convoi()->set_state(convoi_t::ROUTING_1);
					block_reserver( search_for_stop, 0xFFFFu, false ); // unreserve all tiles
					return false; // find new route
				}
			}

			// circle slowly another round ...
			target_halt = halthandle_t();
			state = prev_state;
			return false;
		}
		state = prev_state;

		// now reserve our choice ...
		target_halt->reserve_position(welt->lookup(target_rt.back()), cnv->self);
		//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()", "found free stop near %i,%i,%i", target_rt.back().x, target_rt.back().y, target_rt.back().z);
		rt->remove_koord_from(search_for_stop);
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
	target_halt = halthandle_t(); // no block reserved

	const weg_t *w=welt->lookup(start)->get_weg(air_wt);
	bool start_in_the_air = (w==NULL);
	bool end_in_air=false;

	search_for_stop = takeoff = touchdown = 0x7ffffffful;
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
		approach_dir = welt->get_settings().get_approach_dir(); // reverse
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near (%s)",start.get_str());

		if(!route->find_route( welt, start, this, max_speed, ribi_t::all, 100 )) {
			DBG_MESSAGE("aircraft_t::calc_route()","failed");
			return false;
		}
		// save the route
		search_start = route->back();
		//DBG_MESSAGE("aircraft_t::calc_route()","start at ground (%s)",search_start.get_str());
	}

	// second: find target runway end

	state = taxiing_to_halt; // only used for search

	approach_dir =  ~welt->get_settings().get_approach_dir(); // reverse
	//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
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
				return false; // no route!
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
		search_for_stop = touchdown = 0x7FFFFFFFul;
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
		search_for_stop = route->get_count()-1;

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
				block_reserver( touchdown, search_for_stop+1, false );
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

//DBG_MESSAGE("aircraft_t::can_enter_tile()","index %i<>%i",route_index,touchdown);

	// check for another circle ...
	if(  route_index==(touchdown-3)  ) {
		if(  !block_reserver( touchdown, search_for_stop+1, true )  ) {
			route_index -= 16;
			return true;
		}
		state = landing;
		return true;
	}

	if(  route_index==touchdown-16-3  &&  state!=circling  ) {
		// just check, if the end of runway is free; we will wait there
		if(  block_reserver( touchdown, search_for_stop+1, true )  ) {
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

	if(route_index==search_for_stop  &&  state==landing  &&  !target_halt.is_bound()) {

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


air_vehicle_t::air_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
	vehicle_t(pos, desc, player)
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
	if(leading  &&  (uintptr_t)cnv > 1) {
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
	file->rdwr_long(search_for_stop);
	file->rdwr_long(touchdown);
	file->rdwr_long(takeoff);
}



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

		sint16 hoff = get_hoff();
		ypos += tile_raster_scale_y(get_yoff()-current_flughohe-hoff-2, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		get_screen_offset( xpos, ypos, raster_width );

		display_swap_clip_wh(CLIP_NUM_VAR);
		// will be dirty
		// the aircraft!!!
		display_color( image, xpos, ypos, get_owner_nr(), true, true/*get_flag(obj_t::dirty)*/  CLIP_NUM_PAR);
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
