/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "vehicle.h"

#include "../simintr.h"
#include "../world/simworld.h"
#include "../simware.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../dataobj/translator.h"
#include "../simfab.h"
#include "../player/simplay.h"
#include "../dataobj/schedule.h"
#include "../gui/minimap.h"
#include "../obj/crossing.h"
#include "../obj/wolke.h"
#include "../utils/cbuffer.h"
#include "../dataobj/environment.h"
#include "../dataobj/pakset_manager.h"
#include "../builder/vehikelbauer.h"
#include "../obj/zeiger.h"
#include "../utils/simstring.h"

#include <cmath>


void vehicle_t::rotate90()
{
	vehicle_base_t::rotate90();
	previous_direction = ribi_t::rotate90( previous_direction );
	last_stop_pos.rotate90( welt->get_size().y-1 );
}


void vehicle_t::rotate90_freight_destinations(const sint16 y_size)
{
	// now rotate the freight
	for(ware_t & tmp : fracht) {
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
					if (!(water_wt == get_waytype()  &&  gr  &&  gr->is_water())) { // ships on the open sea are valid
						pos_next = r.at(route_index + 1U);
					}
				}
			}
		}
		// just correct freight destinations
		for(ware_t & c : fracht) {
			c.finish_rd(welt);
		}
	}
}


/**
 * Unload freight to halt
 * @return sum of unloaded goods
 */
uint16 vehicle_t::unload_cargo(halthandle_t halt, bool unload_all, uint16 max_amount )
{
	uint16 sum_menge = 0, sum_delivered = 0, index = 0;
	if(  !halt.is_bound()  ) {
		return 0;
	}

	if(  halt->is_enabled( get_cargo_type() )  ) {
		if(  !fracht.empty()  ) {

			for(  slist_tpl<ware_t>::iterator i = fracht.begin(), end = fracht.end();  i != end  &&  max_amount > 0;  ) {
				ware_t& tmp = *i;

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
				else if(  end_halt == halt  ||  via_halt == halt  ||  unload_all  ) {

					// here, only ordinary goods should be processed
					uint32 org_menge = tmp.menge;
					if (tmp.menge > max_amount) {
						tmp.menge = max_amount;
					}
					// since the max capacity of a vehicle is an uint16
					uint16 menge = (uint16)halt->liefere_an(tmp);
					max_amount -= menge;
					sum_menge += menge;
					total_freight -= menge;
					sum_weight -= tmp.menge * tmp.get_desc()->get_weight_per_unit();

					index = tmp.get_index();

					if(end_halt==halt) {
						sum_delivered += menge;
					}

					// in case of partial unlaoding
					tmp.menge = org_menge-tmp.menge;
					if (tmp.menge == 0) {
						i = fracht.erase(i);
					}
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
 */
uint16 vehicle_t::load_cargo(halthandle_t halt, const vector_tpl<halthandle_t>& destination_halts, uint16 max_amount )
{
	if(  !halt.is_bound()  ||  !halt->gibt_ab(desc->get_freight_type())  ) {
		return 0;
	}

	const uint16 total_freight_start = total_freight;
	const uint16 capacity_left = min(desc->get_capacity() - total_freight, max_amount);
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
			for(ware_t & tmp : fracht ) {
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
 */
void vehicle_t::remove_stale_cargo()
{
	DBG_DEBUG("vehicle_t::remove_stale_cargo()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	slist_tpl<ware_t> kill_queue;
	total_freight = 0;

	if (!fracht.empty()) {
		for(ware_t & tmp : fracht) {
			bool found = false;

			if(  tmp.get_zwischenziel().is_bound()  ) {
				// the original halt exists, but does we still go there?
				for(schedule_entry_t const& i : cnv->get_schedule()->entries) {
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

		for(ware_t const& c : kill_queue) {
			fabrik_t::update_transit( &c, false );
			fracht.remove(c);
		}
	}
	sum_weight =  get_cargo_weight() + desc->get_weight();
}


void vehicle_t::play_sound() const
{
	if(  desc->get_sound() >= 0  &&  !welt->is_fast_forward()  ) {
		welt->play_sound_area_clipped(get_pos().get_2d(), desc->get_sound(), TRAFFIC_SOUND );
	}
}


/**
 * Prepare vehicle for new ride.
 * Sets route_index, pos_next, steps_next.
 * If @p recalc is true this sets position and recalculates/resets movement parameters.
 */
void vehicle_t::initialise_journey(route_t::index_t start_route_index, bool recalc)
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


vehicle_t::vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player) :
	vehicle_base_t(pos)
{
	this->desc = desc;

	set_owner( player );
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
		// can_enter_tile() wll compute restart_speed
		if(  !can_enter_tile( bd, restart_speed, 0 )  ) {
			// stop convoi, when the way is not free
			cnv->warten_bis_weg_frei(restart_speed);

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
		return can_enter_tile( gr, restart_speed, second_check_count );
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
	if(last  &&  minimap_t::is_visible) {
			minimap_t::get_instance()->calc_map_pixel(get_pos().get_2d());
	}
#endif
}


/** this routine add a vehicle to a tile and will insert it in the correct sort order to prevent overlaps
 */
void vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_base_t::enter_tile(gr);

	if(leading  &&  minimap_t::is_visible  ) {
		minimap_t::get_instance()->calc_map_pixel( get_pos().get_2d() );
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


/** calculates the current friction coefficient based on the current track
 * flat, slope, curve ...
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
		const uint slope_height = is_one_high(hang) ? 1 : 2;
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
		// only produce smoke when heavily accelerating or steam engine
		if(  cnv->get_akt_speed() < (sint32)((cnv->get_speed_limit() * 7u) >> 3)  ||  desc->get_engine_type() == vehicle_desc_t::steam  ) {
			grund_t* const gr = welt->lookup( get_pos() );
			if(  gr  ) {
				wolke_t* const abgas = new wolke_t( get_pos(),
					get_xoff() + ((dx * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> VEHICLE_STEPS_PER_TILE_SHIFT),
					get_yoff() + ((dy * (sint16)((uint16)steps * OBJECT_OFFSET_STEPS)) >> VEHICLE_STEPS_PER_TILE_SHIFT),
					get_hoff() - LEGACY_SMOKE_YOFFSET, DEFAULT_EXHAUSTSMOKE_TIME, DEFAULT_SMOKE_UPLIFT, desc->get_smoke() );

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

	for(ware_t const& ware : fracht) {
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

	// Rounded value, in cents
	return (value+1500ll)/3000ll;
}


const char *vehicle_t::get_cargo_mass() const
{
	return get_cargo_type()->get_mass();
}


/**
 * Calculate transported cargo total weight in KG
 */
uint32 vehicle_t::get_cargo_weight() const
{
	uint32 weight = 0;

	for(ware_t const& c : fracht) {
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
		for(ware_t const& ware : fracht) {
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
 */
void vehicle_t::discard_cargo()
{
	for(ware_t w : fracht ) {
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
	if(  file->is_version_atleast(101, 0)  ) {
		koord3d pos = get_pos();
		pos.rdwr(file);
		set_pos(pos);
	}


	sint8 hoff = file->is_saving() ? get_hoff() : 0;

	if(file->is_version_less(86, 6)) {
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
		// changed several data types to save runtime memory
		file->rdwr_long(purchase_time);
		if(file->is_version_less(99, 18)) {
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
	if(file->is_version_less(99, 18)) {
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
	if(file->is_version_atleast(88, 7)) {
		bool target_info;
		if(file->is_loading()) {
			file->rdwr_bool(target_info);
			cnv = (convoi_t *)target_info; // will be checked during convoi reassignment
		}
		else {
			target_info = target_halt.is_bound();
			file->rdwr_bool(target_info);
		}
	}
	else {
		if(file->is_loading()) {
			cnv = NULL; // no reservation too
		}
	}
	if(file->is_version_less(112, 9)) {
		koord3d pos_prev(koord3d::invalid);
		pos_prev.rdwr(file);
	}

	if(file->is_version_less(99, 5)) {
		koord3d dummy;
		dummy.rdwr(file); // current pos (is already saved as ding => ignore)
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
			pakset_manager_t::add_missing_paks( s, MISSING_VEHICLE );
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
			for(ware_t ware : fracht) {
				ware.rdwr(file);
			}
		}
	}
	else {
		for(int i=0; i<fracht_count; i++) {
			ware_t ware(file);
			if(  (desc==NULL  ||  ware.menge>0)  &&  welt->is_within_limits(ware.get_zielpos())  &&  ware.get_desc()  ) {
				// also add, of the desc is unknown to find matching replacement
				fracht.append(ware);
#ifdef CACHE_TRANSIT
				if(  file->is_version_less(112, 1)  )
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
	if(file->is_version_less(88, 7)) {
		bool dummy = 0;
		file->rdwr_bool(dummy);
		file->rdwr_bool(dummy);
	}

	// koordinate of the last stop
	if(file->is_version_atleast(99, 15)) {
		// This used to be 2d, now it's 3d.
		if(file->is_version_less(112, 8)) {
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
		leading = last = false; // dummy, will be set by convoi afterwards
		if(desc) {
			calc_image();

			// full weight after loading
			sum_weight =  get_cargo_weight() + desc->get_weight();
		}
		// recalc total freight
		total_freight = 0;
		for(ware_t const& c : fracht) {
			total_freight += c.menge;
		}
	}

	if(  file->is_version_atleast(110, 0)  ) {
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


vehicle_t::~vehicle_t()
{
	// remove vehicle's marker from the minimap
	minimap_t::get_instance()->calc_map_pixel(get_pos().get_2d());
}


#ifdef MULTI_THREAD
void vehicle_t::display_overlay(int xpos, int ypos) const
{
	if(  cnv  &&  leading  ) {
#else
void vehicle_t::display_after(int xpos, int ypos, bool is_global) const
{
	if(  is_global  &&  cnv  &&  leading  ) {
#endif
		PIXVAL color = 0; // not used, but stop compiler warning about uninitialized
		char tooltip_text[1024];
		tooltip_text[0] = 0;
		uint8 state = env_t::show_vehicle_states;
		if(  state==1  ||  state==2  ) {
			// mouse over check
			bool mo_this_convoy = false;
			const koord3d mouse_pos = world()->get_zeiger()->get_pos();
			if(  mouse_pos == get_pos()  ) {
				mo_this_convoy = true;
			}
			else if(  grund_t* mo_gr = world()->lookup(mouse_pos)  ) {
				if(  vehicle_t* mo_veh = (vehicle_t *)mo_gr->get_convoi_vehicle()  ) {
					mo_this_convoy = mo_veh->get_convoi() == get_convoi();
				}
			}
			// only show when mouse over vehicle
			if(  mo_this_convoy  ) {
				state = 3;
			}
			else {
				state = 0;
			}
		}
		if(  state != 3  ) {
			// nothing to show
			return;
		}

		// now find out what has happened
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::WAITING_FOR_CLEARANCE:
			case convoi_t::CAN_START:
			case convoi_t::CAN_START_ONE_MONTH:
				if(  state>=3  ) {
					snprintf( tooltip_text, lengthof(tooltip_text), "%s (%s)", translator::translate("Waiting for clearance!"), cnv->get_schedule()->get_current_entry().pos.get_str() );
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::LOADING:
				if(  state>=3  ) {
					if(  cnv->is_unloading()  ) {
						sprintf( tooltip_text, translator::translate( "Unloading (%i%%)!" ), cnv->get_loading_level() );
					}
					else if(  cnv->get_schedule()->get_current_entry().minimum_loading == 0  &&  cnv->get_schedule()->get_current_entry().waiting_time >0  ) {
						// is on a schedule
						sprintf(tooltip_text, translator::translate("Loading (%i%%) departure %s!"), cnv->get_loading_level(), tick_to_string(cnv->get_departure_ticks(),true));
					}
					else if( cnv->get_loading_limit()==0 ){
						sprintf( tooltip_text, translator::translate( "Loading (%i%%)!" ), cnv->get_loading_level(), cnv->get_loading_limit() );
					}
					else {
						sprintf( tooltip_text, translator::translate( "Loading (%i->%i%%)!" ), cnv->get_loading_level(), cnv->get_loading_limit() );
					}
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::EDIT_SCHEDULE:
//			case convoi_t::ROUTING_1:
				if(  state>=3  ) {
					tstrncpy( tooltip_text, translator::translate("Schedule changing!"), lengthof(tooltip_text) );
					color = color_idx_to_rgb(COL_YELLOW);
				}
				break;

			case convoi_t::DRIVING:
				if(  state>=3  ) {
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

		if(  env_t::show_vehicle_states == 2  &&  !tooltip_text[ 0 ]  ) {
			// show line name or simply convoi name
			color = color_idx_to_rgb( cnv->get_owner()->get_player_color1() + 7 );
			if(  cnv->get_line().is_bound()  ) {
				snprintf( tooltip_text, lengthof( tooltip_text ), "%s - %s", cnv->get_line()->get_name(), cnv->get_name() );
			}
			else {
				snprintf( tooltip_text, lengthof( tooltip_text ), "%s", cnv->get_name() );
			}
		}

		// something to show?
		if(  tooltip_text[0]  ) {
			const int raster_width = get_current_tile_raster_width();
			get_screen_offset( xpos, ypos, raster_width );
			xpos += tile_raster_scale_x(get_xoff(), raster_width);
			ypos += tile_raster_scale_y(get_yoff(), raster_width)+14;
			if(ypos>LINESPACE+32  &&  ypos+LINESPACE<display_get_clip_wh().yy) {
				display_ddd_proportional_clip( xpos, ypos, color, color_idx_to_rgb(COL_BLACK), tooltip_text, true );
			}
		}
	}
}
