/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>

#include "simdebug.h"
#include "simunits.h"
#include "world/simworld.h"
#include "simware.h"
#include "player/finance.h" // convert_money
#include "player/simplay.h"
#include "simconvoi.h"
#include "simhalt.h"
#include "obj/depot.h"
#include "gui/simwin.h"
#include "tool/simmenu.h"
#include "simcolor.h"
#include "simmesg.h"
#include "simintr.h"
#include "simlinemgmt.h"
#include "simline.h"
#include "freight_list_sorter.h"

#include "gui/minimap.h"
#include "gui/convoi_info.h"
#include "gui/depot_frame.h"
#include "gui/messagebox.h"
#include "gui/convoi_detail.h"
#include "ground/grund.h"
#include "obj/way/schiene.h" // for railblocks

#include "descriptor/citycar_desc.h"
#include "descriptor/roadsign_desc.h"
#include "descriptor/vehicle_desc.h"

#include "dataobj/schedule.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"

#include "display/viewport.h"

#include "obj/crossing.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"


#include "utils/simrandom.h"
#include "utils/simstring.h"
#include "utils/cbuffer.h"

#include "vehicle/air_vehicle.h"
#include "vehicle/overtaker.h"
#include "vehicle/rail_vehicle.h"
#include "vehicle/road_vehicle.h"
#include "vehicle/simroadtraffic.h"
#include "vehicle/water_vehicle.h"


/*
 * Waiting time for loading (ms)
 */
#define WTT_LOADING 2000


karte_ptr_t convoi_t::welt;

/*
 * Debugging helper - translate state value to human readable name
 */
static const char * state_names[convoi_t::MAX_STATES] =
{
	"INITIAL",
	"EDIT_SCHEDULE",
	"ROUTING_1",
	"",
	"",
	"NO_ROUTE",
	"DRIVING",
	"LOADING",
	"WAITING_FOR_CLEARANCE",
	"WAITING_FOR_CLEARANCE_ONE_MONTH",
	"CAN_START",
	"CAN_START_ONE_MONTH",
	"SELF_DESTRUCT",
	"WAITING_FOR_CLEARANCE_TWO_MONTHS",
	"CAN_START_TWO_MONTHS",
	"LEAVING_DEPOT",
	"ENTERING_DEPOT"
};


/**
 * Calculates speed of slowest vehicle in the given array
 */
static int calc_min_top_speed(const array_tpl<vehicle_t*>& fahr, uint8 vehicle_count)
{
	int min_top_speed = SPEED_UNLIMITED;
	for(uint8 i=0; i<vehicle_count; i++) {
		min_top_speed = min(min_top_speed, kmh_to_speed( fahr[i]->get_desc()->get_topspeed() ) );
	}
	return min_top_speed;
}


void convoi_t::init(player_t *player)
{
	owner = player;

	is_electric = false;
	sum_gesamtweight = sum_weight = 0;
	sum_running_costs = sum_fixed_costs = sum_gear_and_power = previous_delta_v = 0;
	sum_power = 0;
	min_top_speed = SPEED_UNLIMITED;
	speedbonus_kmh = SPEED_UNLIMITED; // speed_to_kmh() not needed

	schedule = NULL;
	schedule_target = koord3d::invalid;
	line = linehandle_t();

	vehicle_count = 0;
	steps_driven = -1;
	withdraw = false;
	has_obsolete = false;
	no_load = false;
	wait_lock = 0;
	arrived_time = 0;

	jahresgewinn = 0;
	total_distance_traveled = 0;

	distance_since_last_stop = 0;
	sum_speed_limit = 0;
	maxspeed_average_count = 0;
	next_reservation_index = 0;

	alte_richtung = ribi_t::none;
	next_wolke = 0;

	state = INITIAL;
	unloading_state = false;

	*name_and_id = 0;
	name_offset = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;

	speed_limit = SPEED_UNLIMITED;
	max_record_speed = 0;
	brake_speed_soll = SPEED_UNLIMITED;
	akt_speed_soll = 0;            // target speed
	akt_speed = 0;                 // current speed
	sp_soll = 0;

	next_stop_index = 65535;
	last_load_tick = 0;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;

	recalc_data_front = true;
	recalc_data = true;
	recalc_speed_limit = true;
}


convoi_t::convoi_t(loadsave_t* file) : fahr(default_vehicle_length, NULL)
{
	self = convoihandle_t();
	init(0);
	rdwr(file);
}


convoi_t::convoi_t(player_t* player) : fahr(default_vehicle_length, NULL)
{
	self = convoihandle_t(this);
	player->book_convoi_number(1);
	init(player);
	set_name( "Unnamed" );
	welt->add_convoi( self );
	init_financial_history();
}


convoi_t::~convoi_t()
{
	owner->book_convoi_number( -1);

	assert(self.is_bound());
	assert(vehicle_count==0);

	// close windows
	destroy_win( magic_convoi_info+self.get_id() );

DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_viewport()->get_follow_convoi()==self) {
		welt->get_viewport()->set_follow_convoi( convoihandle_t() );
	}

	welt->sync.remove( this );
	welt->rem_convoi( self );

	// if lineless convoy -> unregister from stops
	if(  !line.is_bound()  ) {
		unregister_stops();
	}

	// force asynchronous recalculation
	if(schedule) {
		if(!schedule->is_editing_finished()) {
			destroy_win((ptrdiff_t)schedule);
		}
		if (!schedule->empty() && !line.is_bound()) {
			welt->set_schedule_counter();
		}
		delete schedule;
	}

	// deregister from line (again)
	unset_line();

	self.detach();
}


// waypoint: no stop, resp. for airplanes in air (i.e. no air strip below)
bool convoi_t::is_waypoint( koord3d ziel ) const
{
	if(  fahr[0]->get_waytype() == air_wt  ) {
		// separate logic for airplanes, since the can have waypoints over stops etc.
		grund_t *gr = welt->lookup_kartenboden(ziel.get_2d());
		if(  gr == NULL  ||  gr->get_weg(air_wt) == NULL  ) {
			// during flight always a waypoint
			return true;
		}
		else if(  gr->get_depot()  ) {
			// but a depot is not a waypoint
			return false;
		}
		// so we are on a taxiway/runway here ...
	}
	return !haltestelle_t::get_halt(ziel,get_owner()).is_bound();
}


/**
 * unreserves the whole remaining route
 */
void convoi_t::unreserve_route()
{
	// need a route, vehicles, and vehicles must belong to this convoi
	// (otherwise crash during loading when fahr[0]->convoi is not initialized yet
	if(  !route.empty()  &&  vehicle_count>0  &&  fahr[0]->get_convoi() == this  ) {
		rail_vehicle_t* lok = dynamic_cast<rail_vehicle_t*>(fahr[0]);
		if (lok) {
			// free all reserved blocks
			route_t::index_t dummy;
			lok->block_reserver(get_route(), back()->get_route_index(), dummy, dummy,  100000, false, true);
		}
	}
}


/**
 * reserves route until next_reservation_index
 */
void convoi_t::reserve_route()
{
	if(  !route.empty()  &&  vehicle_count>0  &&  (is_waiting()  ||  state==DRIVING  ||  state==LEAVING_DEPOT)  ) {
		for(  route_t::index_t idx = back()->get_route_index();  idx < next_reservation_index  /*&&  idx < route.get_count()*/;  idx++  ) {
			if(  grund_t *gr = welt->lookup( route.at(idx) )  ) {
				if(  schiene_t *sch = (schiene_t *)gr->get_weg( front()->get_waytype() )  ) {
					sch->reserve( self, ribi_type( route.at(max(1u,idx)-1u), route.at(min(route.get_count()-1u,idx+1u)) ) );
				}
			}
		}
	}
}

/**
 * Sets route_index of all vehicles to startindex.
 * Puts all vehicles on tile at this position in the route.
 * Convoy stills needs to be pushed that the convoy is right on track.
 * @returns length of convoy minus last vehicle
 */
uint32 convoi_t::move_to(route_t::index_t const start_index)
{
	steps_driven = -1;
	koord3d k = route.at(start_index);
	grund_t* gr = welt->lookup(k);

	uint32 train_length = 0;
	for (unsigned i = 0; i != vehicle_count; ++i) {
		vehicle_t& v = *fahr[i];

		if(  grund_t const* gr = welt->lookup(v.get_pos())  ) {
			v.mark_image_dirty(v.get_image(), 0);
			v.leave_tile();
			// maybe unreserve this
			if(  schiene_t* const rails = obj_cast<schiene_t>(gr->get_weg(v.get_waytype()))  ) {
				rails->unreserve(&v);
			}
		}
		// propagate new index to vehicle, will set all movement related variables, in particular pos
		v.initialise_journey(start_index, true);
		// now put vehicle on the tile
		if (gr) {
			v.enter_tile(gr);
		}

		if (i != vehicle_count - 1U) {
			train_length += v.get_desc()->get_length();
		}
	}
	return train_length;
}


void convoi_t::finish_rd()
{
	if(schedule==NULL) {
		if(  state!=INITIAL  ) {
			grund_t *gr = welt->lookup(home_depot);
			if(gr  &&  gr->get_depot()) {
				dbg->warning( "convoi_t::finish_rd()","No schedule during loading convoi %i: State will be initial!", self.get_id() );
				for( uint8 i=0;  i<vehicle_count;  i++ ) {
					fahr[i]->set_pos(home_depot);
				}
				state = INITIAL;
			}
			else {
				dbg->error( "convoi_t::finish_rd()","No schedule during loading convoi %i: Convoi will be destroyed!", self.get_id() );
				for( uint8 i=0;  i<vehicle_count;  i++ ) {
					fahr[i]->set_pos(koord3d::invalid);
				}
				destroy();
				return;
			}
		}
		// anyway reassign convoi pointer ...
		for( uint8 i=0;  i<vehicle_count;  i++ ) {
			vehicle_t* v = fahr[i];
			v->set_convoi(this);
			if(  state!=INITIAL  &&  welt->lookup(v->get_pos())  ) {
				// mark vehicle as used
				v->set_driven();
			}
		}
		return;
	}
	else {
		// restore next schedule target for non-stop waypoint handling
		const koord3d ziel = schedule->get_current_entry().pos;
		if(  vehicle_count>0  &&  is_waypoint(ziel)  ) {
			schedule_target = ziel;
		}
	}

	bool realign_position = false;
	if(  vehicle_count>0  ) {
DBG_MESSAGE("convoi_t::finish_rd()","state=%s, next_stop_index=%d", state_names[state], next_stop_index );
		// only realign convois not leaving depot to avoid jumps through signals
		if(  steps_driven!=-1  ) {
			for( uint8 i=0;  i<vehicle_count;  i++ ) {
				vehicle_t* v = fahr[i];
				v->set_leading( i==0 );
				v->set_last( i+1==vehicle_count );
				v->calc_height();
				// this sets the convoi and will renew the block reservation, if needed!
				v->set_convoi(this);
			}
		}
		else {
			// test also for realignment
			sint16 step_pos = 0;
			koord3d drive_pos;
			uint8 const diagonal_vehicle_steps_per_tile = (uint8)(130560U / welt->get_settings().get_pak_diagonal_multiplier());
			for( uint8 i=0;  i<vehicle_count;  i++ ) {
				vehicle_t* v = fahr[i];
				v->set_leading( i==0 );
				v->set_last( i+1==vehicle_count );
				v->calc_height();
				// this sets the convoi and will renew the block reservation, if needed!
				v->set_convoi(this);

				// wrong alignment here => must relocate
				if(v->need_realignment()) {
					// diagonal => convoi must restart
					realign_position |= ribi_t::is_bend(v->get_direction())  &&  (state==DRIVING  ||  is_waiting());
				}
				// if version is 99.17 or lower, some convois are broken, i.e. had too large gaps between vehicles
				if(  !realign_position  &&  state!=INITIAL  &&  state!=LEAVING_DEPOT  ) {
					if(  i==0  ) {
						step_pos = v->get_steps();
					}
					else {
						if(  drive_pos!=v->get_pos()  ) {
							// with long vehicles on diagonals, vehicles need not to be on consecutive tiles
							// do some guessing here
							uint32 dist = koord_distance(drive_pos, v->get_pos());
							if (dist>1) {
								step_pos += (dist-1) * diagonal_vehicle_steps_per_tile;
							}
							step_pos += ribi_t::is_bend(v->get_direction()) ? diagonal_vehicle_steps_per_tile : VEHICLE_STEPS_PER_TILE;
						}
						dbg->message("convoi_t::finish_rd()", "v: pos(%s) steps(%d) len=%d ribi=%d prev (%s) step(%d)", v->get_pos().get_str(), v->get_steps(), v->get_desc()->get_length()*16, v->get_direction(),  drive_pos.get_2d().get_str(), step_pos);
						if(  abs( v->get_steps() - step_pos )>15  ) {
							// not where it should be => realign
							realign_position = true;
							dbg->warning( "convoi_t::finish_rd()", "convoi (%s) is broken => realign", get_name() );
						}
					}
					step_pos -= v->get_desc()->get_length_in_steps();
					drive_pos = v->get_pos();
				}
			}
		}
DBG_MESSAGE("convoi_t::finish_rd()","next_stop_index=%d", next_stop_index );

		linehandle_t new_line  = line;
		if(  !new_line.is_bound()  ) {
			// if there is a line with id=0 in the savegame try to assign cnv to this line
			new_line = get_owner()->simlinemgmt.get_line_with_id_zero();
		}
		if(  new_line.is_bound()  ) {
			if (  !schedule->matches( welt, new_line->get_schedule() )  ) {
				// 101 version produced broken line ids => we have to find our line the hard way ...
				vector_tpl<linehandle_t> lines;
				get_owner()->simlinemgmt.get_lines(schedule->get_type(), &lines);
				new_line = linehandle_t();
				for(linehandle_t const l : lines) {
					if(  schedule->matches( welt, l->get_schedule() )  ) {
						// if a line is assigned, set line!
						new_line = l;
						break;
					}
				}
			}
			// now the line should match our schedule or else ...
			if(new_line.is_bound()) {
				line = new_line;
				line->add_convoy(self);
				DBG_DEBUG("convoi_t::finish_rd()","%s registers for %d", name_and_id, line.get_id());
			}
			else {
				line = linehandle_t();
			}
		}
	}
	else {
		// no vehicles in this convoi?!?
		dbg->error( "convoi_t::finish_rd()","No vehicles in Convoi %i: will be destroyed!", self.get_id() );
		destroy();
		return;
	}
	// put convoi again right on track?
	if(realign_position  &&  vehicle_count>1) {
		// display just a warning
		dbg->warning("convoi_t::finish_rd()","cnv %i is currently too long.",self.get_id());

		if (route.empty()) {
			// realigning needs a route
			state = NO_ROUTE;
			owner->report_vehicle_problem( self, koord3d::invalid );
			dbg->error( "convoi_t::finish_rd()", "No valid route, but needs realignment at (%s)!", fahr[0]->get_pos().get_str() );
		}
		else {
			// since start may have been changed
			route_t::index_t start_index = max(1,fahr[vehicle_count-1]->get_route_index())-1;
			if (start_index > route.get_count()) {
				dbg->error( "convoi_t::finish_rd()", "Routeindex of last vehicle of (%s) too large (%hu > %hu)!", get_name(), start_index, route.get_count() );
				start_index = 0;
			}

			uint32 train_length = move_to(start_index) + 1;
			const koord3d last_start = fahr[0]->get_pos();

			// now advance all convoi until it is completely on the track
			fahr[0]->set_leading(false); // switches off signal checks ...
			for(unsigned i=0; i<vehicle_count; i++) {
				vehicle_t* v = fahr[i];

				v->get_smoke(false);
				fahr[i]->do_drive( (VEHICLE_STEPS_PER_CARUNIT*train_length)<<YARDS_PER_VEHICLE_STEP_SHIFT );
				train_length -= v->get_desc()->get_length();
				v->get_smoke(true);

				// eventually reserve this again
				grund_t *gr=welt->lookup(v->get_pos());
				// airplanes may have no ground ...
				if (schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(fahr[i]->get_waytype()))) {
					sch0->reserve(self,ribi_t::none);
				}
			}
			fahr[0]->set_leading(true);
			if(  state != INITIAL  &&  state != EDIT_SCHEDULE  &&  fahr[0]->get_pos() != last_start  ) {
				state = WAITING_FOR_CLEARANCE;
			}
		}
	}
	if(  state==LOADING  ) {
		// the fully the shorter => register again as older convoi
		wait_lock = 2000-loading_level*20;
		if (distance_since_last_stop > 0) {
			calc_gewinn();
		}
	}
	// when saving with open window, this can happen
	if(  state==EDIT_SCHEDULE  ) {
		if (env_t::networkmode) {
			wait_lock = 30000; // 60s to drive on, if the client in question had left
		}
		schedule->finish_editing();
	}
	// remove wrong freight
	check_freight();
	// some convois had wrong old direction in them
	if(  state<DRIVING  ||  state==LOADING  ) {
		alte_richtung = fahr[0]->get_direction();
	}
	// if lineless convoy -> register itself with stops
	if(  !line.is_bound()  ) {
		register_stops();
	}

	calc_speedbonus_kmh();
}


// since now convoi states go via tool_t
void convoi_t::call_convoi_tool( const char function, const char *extra ) const
{
	tool_t *tmp_tool = create_tool( TOOL_CHANGE_CONVOI | SIMPLE_TOOL );
	cbuffer_t param;
	param.printf("%c,%u", function, self.get_id());
	if(  extra  &&  *extra  ) {
		param.printf(",%s", extra);
	}
	tmp_tool->set_default_param(param);
	welt->set_tool( tmp_tool, get_owner() );
	// since init always returns false, it is safe to delete immediately
	delete tmp_tool;
}


void convoi_t::rotate90( const sint16 y_size )
{
	record_pos.rotate90( y_size );
	home_depot.rotate90( y_size );
	route.rotate90( y_size );
	if(  schedule_target!=koord3d::invalid  ) {
		schedule_target.rotate90( y_size );
	}
	if(schedule) {
		schedule->rotate90( y_size );
	}
	for(  int i=0;  i<vehicle_count;  i++  ) {
		fahr[i]->rotate90_freight_destinations( y_size );
	}
	// eventually correct freight destinations (and remove all stale freight)
	check_freight();
}


/**
 * Return the convoi position.
 * @return Convoi position
 */
koord3d convoi_t::get_pos() const
{
	if(vehicle_count > 0 && fahr[0]) {
		return state==INITIAL ? home_depot : fahr[0]->get_pos();
	}
	else {
		return koord3d::invalid;
	}
}


/**
 * Sets the name. Creates a copy of name.
 */
void convoi_t::set_name(const char *name, bool with_new_id)
{
	if(  with_new_id  ) {
		char buf[128];
		name_offset = sprintf(buf,"(%i) ",self.get_id() );
		tstrncpy(buf + name_offset, translator::translate(name, welt->get_settings().get_name_language_id()), lengthof(buf) - name_offset);
		tstrncpy(name_and_id, buf, lengthof(name_and_id));
	}
	else {
		char buf[128];
		// check if there is a id in the name string
		name_offset = sprintf(buf,"(%i) ",self.get_id() );
		if(  strlen(name) < name_offset  ||  strncmp(buf,name,name_offset)!=0) {
			name_offset = 0;
		}
		tstrncpy(buf+name_offset, name+name_offset, sizeof(buf)-name_offset);
		tstrncpy(name_and_id, buf, lengthof(name_and_id));
	}
	// now tell the windows that we were renamed
	convoi_info_t *info = dynamic_cast<convoi_info_t*>(win_get_magic( magic_convoi_info+self.get_id()));
	if (info) {
		info->update_data();
	}
	if(  in_depot()  ) {
		const grund_t *const ground = welt->lookup( get_home_depot() );
		if(  ground  ) {
			const depot_t *const depot = ground->get_depot();
			if(  depot  ) {
				depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
				if(  frame  ) {
					frame->update_data();
				}
			}
		}
	}
}


// length of convoi (16 is one tile)
uint32 convoi_t::get_length() const
{
	uint32 len = 0;
	for( uint8 i=0; i<vehicle_count; i++ ) {
		len += fahr[i]->get_desc()->get_length();
	}
	return len;
}


/**
 * convoi add their running cost for traveling one tile
 */
void convoi_t::add_running_cost( const weg_t *weg )
{
	jahresgewinn += sum_running_costs;

	if(  weg  &&  weg->get_owner()!=get_owner()  &&  weg->get_owner()!=NULL  ) {
		// running on non-public way costs toll (since running costs are positive => invert)
		sint32 toll = -(sum_running_costs*welt->get_settings().get_way_toll_runningcost_percentage())/100l;
		if(  welt->get_settings().get_way_toll_waycost_percentage()  ) {
			if(  weg->is_electrified()  &&  needs_electrification()  ) {
				// toll for using electricity
				grund_t *gr = welt->lookup(weg->get_pos());
				for(  int i=1;  i<gr->get_top();  i++  ) {
					obj_t *d=gr->obj_bei(i);
					if(  wayobj_t const* const wo = obj_cast<wayobj_t>(d)  )  {
						if(  wo->get_waytype()==weg->get_waytype()  ) {
							toll += (wo->get_desc()->get_maintenance()*welt->get_settings().get_way_toll_waycost_percentage())/100l;
							break;
						}
					}
				}
			}
			// now add normal way toll be maintenance
			toll += (weg->get_desc()->get_maintenance()*welt->get_settings().get_way_toll_waycost_percentage())/100l;
		}
		weg->get_owner()->book_toll_received( toll, get_schedule()->get_waytype() );
		get_owner()->book_toll_paid(         -toll, get_schedule()->get_waytype() );
		book( -toll, CONVOI_WAYTOLL);
		book( -toll, CONVOI_PROFIT);

	}
	get_owner()->book_running_costs( sum_running_costs, get_schedule()->get_waytype());

	book( sum_running_costs, CONVOI_OPERATIONS );
	book( sum_running_costs, CONVOI_PROFIT );

	total_distance_traveled ++;
	distance_since_last_stop++;

	sum_speed_limit += speed_to_kmh( min( min_top_speed, speed_limit ));
	book( 1, CONVOI_DISTANCE );
}


/**
 * Returns residual power given power, weight, and current speed.
 * @param speed (in internal speed unit)
 * @param total_power sum of power times gear (see calculation of sum_gear_and_power)
 * @param friction_weight weight including friction of the convoy
 * @param total_weight weight of the convoy
 * @returns residual power
 */
static inline sint32 res_power(sint64 speed, sint32 total_power, sint64 friction_weight, sint64 total_weight)
{
	sint32 res = total_power - (sint32)( ( (sint64)speed * ( (friction_weight * (sint64)speed ) / 3125ll + 1ll) ) / 2048ll + (total_weight * 64ll) / 1000ll);
	return res;
}

/* Calculates (and sets) new akt_speed
 * needed for driving, entering and leaving a depot)
 */
void convoi_t::calc_acceleration(uint32 delta_t)
{

	if(  !recalc_data  &&  !recalc_speed_limit  &&  !recalc_data_front  &&  (
		(sum_friction_weight == sum_gesamtweight  &&  akt_speed_soll <= akt_speed  &&  akt_speed_soll+24 >= akt_speed)  ||
		(sum_friction_weight > sum_gesamtweight  &&  akt_speed_soll == akt_speed)  )
		) {
		// at max speed => go with max speed and finish calculation here
		// at slopes/curves, only do this if there is absolutely now change
		akt_speed = akt_speed_soll;
		return;
	}

	// only compute this if a vehicle in the convoi hopped
	if(  recalc_data  ||  recalc_speed_limit  ) {
		// calculate total friction and lowest speed limit
		const vehicle_t* v = front();
		speed_limit = min( min_top_speed, v->get_speed_limit() );
		if (recalc_data) {
			sum_gesamtweight   = v->get_total_weight();
			sum_friction_weight = v->get_frictionfactor() * sum_gesamtweight;
		}

		for(  unsigned i=1; i<vehicle_count; i++  ) {
			const vehicle_t* v = fahr[i];
			speed_limit = min( speed_limit, v->get_speed_limit() );

			if (recalc_data) {
				int total_vehicle_weight = v->get_total_weight();
				sum_friction_weight += v->get_frictionfactor() * total_vehicle_weight;
				sum_gesamtweight += total_vehicle_weight;
			}
		}
		recalc_data = recalc_speed_limit = false;
		akt_speed_soll = min( speed_limit, brake_speed_soll );
	}

	if(  recalc_data_front  ) {
		// brake at the end of stations/in front of signals and crossings
		const uint32 tiles_left = 1 + get_next_stop_index() - front()->get_route_index();
		brake_speed_soll = SPEED_UNLIMITED;
		if(  tiles_left < 4  ) {
			static sint32 brake_speed_countdown[4] = {
				kmh_to_speed(25),
				kmh_to_speed(50),
				kmh_to_speed(100),
				kmh_to_speed(200)
			};
			brake_speed_soll = brake_speed_countdown[tiles_left];
		}
		akt_speed_soll = min( speed_limit, brake_speed_soll );
		recalc_data_front = false;
	}

	// more pleasant and a little more "physical" model

	// try to simulate quadratic friction
	if(sum_gesamtweight != 0) {
		/*
		 * The parameter consist of two parts (optimized for good looking):
		 *  - every vehicle in a convoi has a the friction of its weight
		 *  - the dynamic friction is calculated that way, that v^2*weight*frictionfactor = 200 kW
		 *    This means that if a vehicle is loaded heavier and/or travels faster, less
		 *    power for acceleration is available.
		 *    since delta_t can have any value, we have to scale the step size by this value.
		 *    However, there is a quadratic friction term => if delta_t is too large the calculation may get weird results
		 *
		 * but for integer, we have to use the order below and calculate actually 64*deccel, like the sum_gear_and_power
		 * since akt_speed=10/128 km/h and we want 64*200kW=(100km/h)^2*100t, we must multiply by (128*2)/100
		 * But since the acceleration was too fast, we just decelerate 4x more => >>6 instead >>8
		 */
		//sint32 deccel = ( ( (akt_speed*sum_friction_weight)>>6 )*(akt_speed>>2) ) / 25 + (sum_gesamtweight*64); // this order is needed to prevent overflows!
		//sint32 deccel = (sint32)( ( (sint64)akt_speed * (sint64)sum_friction_weight * (sint64)akt_speed ) / (25ll*256ll) + sum_gesamtweight * 64ll) / 1000ll; // intermediate still overflows so sint64
		//sint32 deccel = (sint32)( ( (sint64)akt_speed * ( (sum_friction_weight * (sint64)akt_speed ) / 3125ll + 1ll) ) / 2048ll + (sum_gesamtweight * 64ll) / 1000ll);

		// note: result can overflow sint32 and double so we use sint64. Planes are ok.
		//sint32 delta_v =  (sint32)( ( (double)( (akt_speed>akt_speed_soll?0l:sum_gear_and_power) - deccel)*(double)delta_t)/(double)sum_gesamtweight);

		sint64 residual_power = res_power(akt_speed, akt_speed>akt_speed_soll? 0l : sum_gear_and_power, sum_friction_weight, sum_gesamtweight);

		// we normalize delta_t to 1/64th and check for speed limit */
		//sint32 delta_v = ( ( (akt_speed>akt_speed_soll?0l:sum_gear_and_power) - deccel) * delta_t)/sum_gesamtweight;
		sint64 delta_v = ( residual_power * (sint64)delta_t * 1000ll) / (sint64)sum_gesamtweight;

		// we need more accurate arithmetic, so we store the previous value
		delta_v += previous_delta_v;
		previous_delta_v = (uint16) (delta_v & 0x00000FFFll);
		// and finally calculate new speed
		akt_speed = max(akt_speed_soll>>4, akt_speed+(sint32)(delta_v>>12l) );
	}
	else {
		// very old vehicle ...
		akt_speed += 16;
	}

	// obey speed maximum with additional const brake ...
	if(akt_speed > akt_speed_soll) {
		if (akt_speed > akt_speed_soll + 24) {
			akt_speed -= 24;
			if(akt_speed > akt_speed_soll+kmh_to_speed(20)) {
				akt_speed = akt_speed_soll+kmh_to_speed(20);
			}
		}
		else {
			akt_speed = akt_speed_soll;
		}
	}

	// new record?
	if(akt_speed > max_record_speed) {
		max_record_speed = akt_speed;
		record_pos = fahr[0]->get_pos().get_2d();
	}
}


/**
 * Calculates maximal possible speed.
 * Uses iterative technique to take care of integer arithmetic.
 */
sint32 convoi_t::calc_max_speed(uint64 total_power, uint64 total_weight, sint32 speed_limit)
{
	// precision is 0.5 km/h
	const sint32 tol = kmh_to_speed(1)/2;
	// bisection to find max speed
	sint32 pl,pr,pm;
	sint64 sl,sr,sm;

	// test speed_limit
	sr = speed_limit;
	pr = res_power(sr, (sint32)total_power, total_weight, total_weight);
	if (pr >= 0) {
		return (sint32)sr; // convoy can travel at speed given by speed_limit
	}
	sl = 1;
	pl = res_power(sl, (sint32)total_power, total_weight, total_weight);
	if (pl <= 0) {
		return 0; // no power to move at all
	}

	// bisection algorithm to find speed for which residual power is zero
	while (sr - sl > tol) {
		sm = (sl + sr)/2;
		if (sm == sl) break;

		pm = res_power(sm, (sint32)total_power, total_weight, total_weight);

		if (((sint64)pl)*pm <= 0) {
			pr = pm;
			sr = sm;
		}
		else {
			pl = pm;
			sl = sm;
		}
	}
	return (sint32)sl;
}


int convoi_t::get_vehicle_at_length(uint16 length)
{
	int current_length = 0;
	for( int i=0;  i<vehicle_count;  i++  ) {
		current_length += fahr[i]->get_desc()->get_length();
		if(length<current_length) {
			return i;
		}
	}
	return vehicle_count;
}


// moves all vehicles of a convoi
sync_result convoi_t::sync_step(uint32 delta_t)
{
	// still have to wait before next action?
	wait_lock -= delta_t;
	if(wait_lock > 0) {
		return SYNC_OK;
	}
	wait_lock = 0;

	switch(state) {
		case INITIAL:
			// in depot, should not be in sync list, remove
			return SYNC_REMOVE;

		case EDIT_SCHEDULE:
		case ROUTING_1:
		case DUMMY4:
		case DUMMY5:
		case NO_ROUTE:
		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
			// this is an async task, see step()
			break;

		case ENTERING_DEPOT:
			break;

		case LEAVING_DEPOT:
			{
				// ok, so we will accelerate
				akt_speed_soll = max( akt_speed_soll, kmh_to_speed(30) );
				calc_acceleration(delta_t);
				sp_soll += (akt_speed*delta_t);

				// now actually move the units
				while(sp_soll>>12) {
					// Attempt to move one step.
					uint32 sp_hat = fahr[0]->do_drive(1<<YARDS_PER_VEHICLE_STEP_SHIFT);
					if(  sp_hat>0  ) {
						steps_driven++;
					}
					int v_nr = get_vehicle_at_length(steps_driven>>4);
					// stop when depot reached
					if (state==INITIAL) {
						return SYNC_REMOVE;
					}
					if (state==ROUTING_1) {
						break;
					}
					if(  v_nr==vehicle_count  ) {
						// all are moving
						steps_driven = -1;
						state = DRIVING;
						return SYNC_OK;
					}
					else if(  sp_hat==0  ) {
						// something went wrong. wait for next sync_step()
						return SYNC_OK;
					}
					// now only the right numbers
					for(int i=1; i<=v_nr; i++) {
						fahr[i]->do_drive(sp_hat);
					}
					sp_soll -= sp_hat;
				}
				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<vehicle_count;  i++  ) {
						fahr[i]->make_smoke();
					}
				}
			}
			break; // LEAVING_DEPOT

		case DRIVING:
			{
				calc_acceleration(delta_t);

				// now actually move the units
				sp_soll += (akt_speed*delta_t);
				uint32 sp_hat = fahr[0]->do_drive(sp_soll);
				// stop when depot reached ...
				if(state==INITIAL) {
					return SYNC_REMOVE;
				}
				// now move the rest (so all vehicles are moving synchronously)
				for(unsigned i=1; i<vehicle_count; i++) {
					fahr[i]->do_drive(sp_hat);
				}
				// maybe we have been stopped by something => avoid wide jumps
				sp_soll = (sp_soll-sp_hat) & 0x0FFF;

				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<vehicle_count;  i++  ) {
						fahr[i]->make_smoke();
					}
				}
			}
			break; // DRIVING

		case LOADING:
			// loading is an async task, see laden()
			break;

		case WAITING_FOR_CLEARANCE:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
			// waiting is asynchronous => fixed waiting order and route search
			break;

		case SELF_DESTRUCT:
			// see step, since destruction during a screen update may give strange effects
			break;

		default:
			dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
	}

	return SYNC_OK;
}


/**
 * Berechne route von Start- zu Zielkoordinate
 */
bool convoi_t::drive_to()
{
	if(  vehicle_count>0  ) {

		// unreserve all tiles that are covered by the train but do not contain one of the wagons,
		// otherwise repositioning of the train drive_to may lead to stray reserved tiles
		if (dynamic_cast<rail_vehicle_t*>(fahr[0])!=NULL  &&  vehicle_count > 1) {
			// route-index points to next position in route
			// it is completely off when convoi leaves depot
			route_t::index_t index0 = min(fahr[0]->get_route_index()-1, route.get_count());

			for(uint8 i=1; i<vehicle_count; i++) {
				const route_t::index_t index1 = fahr[i]->get_route_index();

				for(route_t::index_t j = index1; j<index0; j++) {
					// unreserve track on tiles between wagons
					grund_t *gr = welt->lookup(route.at(j));
					if (schiene_t *track = (schiene_t *)gr->get_weg( front()->get_waytype() ) ) {
						track->unreserve(self);
					}
				}

				index0 = min(index1-1, route.get_count());
			}
		}

		koord3d start = fahr[0]->get_pos();
		koord3d ziel = schedule->get_current_entry().pos;

		// avoid stopping mid-halt
		if(  start==ziel  ) {
			halthandle_t halt = haltestelle_t::get_halt(ziel,get_owner());
			if(  halt.is_bound()  &&  route.is_contained(start)  ) {
				for(  uint32 i=route.index_of(start);  i<route.get_count();  i++  ) {
					grund_t *gr = welt->lookup(route.at(i));
					if(  gr  && gr->get_halt()==halt  ) {
						ziel = gr->get_pos();
					}
					else {
						break;
					}
				}
			}
		}

		if(  !fahr[0]->calc_route( start, ziel, speed_to_kmh(min_top_speed), &route )  ) {
			if(  state != NO_ROUTE  ) {
				state = NO_ROUTE;
				get_owner()->report_vehicle_problem( self, ziel );
			}
			// wait 25s before next attempt
			wait_lock = 25000;
		}
		else {
			bool route_ok = true;
			const uint8 current_stop = schedule->get_current_stop();
			if(  fahr[0]->get_waytype() != water_wt  ) {
				air_vehicle_t *const plane = dynamic_cast<air_vehicle_t *>(fahr[0]);
				uint32 takeoff = 0, search = 0, landing = 0;
				air_vehicle_t::flight_state plane_state = air_vehicle_t::taxiing;
				if(  plane  ) {
					// due to the complex state system of aircrafts, we have to save index and state
					plane->get_event_index( plane_state, takeoff, search, landing );
				}

				// set next schedule target position if next is a waypoint
				if(  is_waypoint(ziel)  ) {
					schedule_target = ziel;
				}

				// continue route search until the destination is a station
				while(  is_waypoint(ziel)  ) {
					start = ziel;
					schedule->advance();
					ziel = schedule->get_current_entry().pos;

					if(  schedule->get_current_stop() == current_stop  ) {
						// looped around without finding a halt => entire schedule is waypoints.
						break;
					}

					route_t next_segment;
					if(  !fahr[0]->calc_route( start, ziel, speed_to_kmh(min_top_speed), &next_segment )  ) {
						// do we still have a valid route to proceed => then go until there
						if(  route.get_count()>1  ) {
							break;
						}
						// we are stuck on our first routing attempt => give up
						if(  state != NO_ROUTE  ) {
							state = NO_ROUTE;
							get_owner()->report_vehicle_problem( self, ziel );
						}
						// wait 25s before next attempt
						wait_lock = 25000;
						route_ok = false;
						break;
					}
					else {
						bool looped = false;
						if(  fahr[0]->get_waytype() != air_wt  ) {
							 // check if the route circles back on itself (only check the first tile, should be enough)
							looped = route.is_contained(next_segment.at(1));
#if 0
							// this will forbid an eight figure, which might be clever to avoid a problem of reserving one own track
							for(  unsigned i = 1;  i<next_segment.get_count();  i++  ) {
								if(  route.is_contained(next_segment.at(i))  ) {
									looped = true;
									break;
								}
							}
#endif
						}

						if(  looped  ) {
							// proceed upto the waypoint before the loop. Will pause there for a new route search.
							break;
						}
						else {
							uint32 count_offset = route.get_count()-1;
							route.append( &next_segment);
							if(  plane  ) {
								// maybe we need to restore index
								air_vehicle_t::flight_state dummy1;
								uint32 new_takeoff, new_search, new_landing;
								plane->get_event_index( dummy1, new_takeoff, new_search, new_landing );
								if(  takeoff == 0x7FFFFFFF  &&  new_takeoff != 0x7FFFFFFF  ) {
									takeoff = new_takeoff + count_offset;
								}
								if(  landing == 0x7FFFFFFF  &&  new_landing != 0x7FFFFFFF  ) {
									landing = new_landing + count_offset;
								}
								if(  search == 0x7FFFFFFF  &&  new_search != 0x7FFFFFFF ) {
									search = new_search + count_offset;
								}
							}
						}
					}
				}

				if(  plane  ) {
					// due to the complex state system of aircrafts, we have to restore index and state
					plane->set_event_index( plane_state, takeoff, search, landing );
				}
			}

			schedule->set_current_stop(current_stop);
			if(  route_ok  ) {
				vorfahren();
				return true;
			}
		}
	}
	return false;
}


/**
 * Ein Fahrzeug hat ein Problem erkannt und erzwingt die
 * Berechnung einer neuen Route
 */
void convoi_t::suche_neue_route()
{
	state = ROUTING_1;
	wait_lock = 0;
}


/**
 * Asynchrne step methode des Convois
 */
void convoi_t::step()
{
	if(  wait_lock > 0  ) {
		return;
	}

	// moved check to here, as this will apply the same update
	// logic/constraints convois have for manual schedule manipulation
	if (line_update_pending.is_bound()) {
		check_pending_updates();
	}

	if( state==LOADING ) {
		// handled seperately, since otherwise departues are delayed
		laden();
	}

	switch(state) {

		case LOADING:
			// handled above
			break;

		case DUMMY4:
		case DUMMY5:
			break;

		case EDIT_SCHEDULE:
			// schedule window closed?
			if(schedule!=NULL  &&  schedule->is_editing_finished()) {

				set_schedule(schedule);
				schedule_target = koord3d::invalid;

				if(  schedule->empty()  ) {
					// no entry => no route ...
					state = NO_ROUTE;
					owner->report_vehicle_problem( self, koord3d::invalid );
				}
				else {
					// Schedule changed at station
					// this station? then complete loading task else drive on
					halthandle_t h = haltestelle_t::get_halt( get_pos(), get_owner() );
					if(  h.is_bound()  &&  h==haltestelle_t::get_halt( schedule->get_current_entry().pos, get_owner() )  ) {
						if (route.get_count() > 0) {
							koord3d const& pos = route.back();
							if (h == haltestelle_t::get_halt(pos, get_owner())) {
								state = get_pos() == pos ? LOADING : DRIVING;
								break;
							}
						}
						else {
							if(  drive_to()  ) {
								state = DRIVING;
								break;
							}
						}
					}

					if(  schedule->get_current_entry().pos==get_pos()  ) {
						// position in depot: waiting
						grund_t *gr = welt->lookup(schedule->get_current_entry().pos);
						if(  gr  &&  gr->get_depot()  ) {
							betrete_depot( gr->get_depot() );
						}
						else {
							state = ROUTING_1;
						}
					}
					else {
						// go to next
						state = ROUTING_1;
					}
				}
			}
			break;

		case ROUTING_1:
			{
				vehicle_t* v = fahr[0];

				if(  schedule->empty()  ) {
					state = NO_ROUTE;
					owner->report_vehicle_problem( self, koord3d::invalid );
				}
				else {
					// check first, if we are already there:
					assert( schedule->get_current_stop()<schedule->get_count()  );
					if(  v->get_pos()==schedule->get_current_entry().pos  ) {
						schedule->advance();
					}
					// now calculate a new route
					drive_to();
					// finally, was there a record last time?
					if(max_record_speed>welt->get_record_speed(fahr[0]->get_waytype())) {
						welt->notify_record(self, max_record_speed, record_pos);
					}
				}
			}
			break;

		case NO_ROUTE:
			// stuck vehicles
			if (schedule->empty()) {
				// no entries => no route ...
			}
			else {
				// now calculate a new route
				drive_to();
			}
			break;

		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
			{
				vehicle_t* v = fahr[0];

				sint32 restart_speed = -1;
				if(  v->can_enter_tile( restart_speed, 0 )  ) {
					// can reserve new block => drive on
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
					if(haltestelle_t::get_halt(v->get_pos(),owner).is_bound()) {
						v->play_sound();
					}
				}
				else if(  steps_driven==0  ) {
					// on rail depot tile, do not reserve this
					if(  grund_t *gr = welt->lookup(fahr[0]->get_pos())  ) {
						if (schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(fahr[0]->get_waytype()))) {
							sch0->unreserve(fahr[0]);
						}
					}
				}
				if(restart_speed>=0) {
					akt_speed = restart_speed;
				}
				if(state==CAN_START  ||  state==CAN_START_ONE_MONTH) {
					set_tiles_overtaking( 0 );
				}
			}
			break;

		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
		case WAITING_FOR_CLEARANCE:
			{
				sint32 restart_speed = -1;
				if(  fahr[0]->can_enter_tile( restart_speed, 0 )  ) {
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
				}
				if(restart_speed>=0) {
					akt_speed = restart_speed;
				}
				if(state!=DRIVING) {
					set_tiles_overtaking( 0 );
				}
			}
			break;

		// must be here; may otherwise confuse window management
		case SELF_DESTRUCT:
			welt->set_dirty();
			destroy();
			return; // must not continue method after deleting this object

		default: /* keeps compiler silent*/
			break;
	}

	// calculate new waiting time
	switch( state ) {
		// handled by routine
		case LOADING:
			break;

		// immediate action needed
		case SELF_DESTRUCT:
		case LEAVING_DEPOT:
		case ENTERING_DEPOT:
		case DRIVING:
		case DUMMY4:
		case DUMMY5:
			wait_lock = 0;
			break;

		// just waiting for action here
		case INITIAL:
			welt->sync.remove(this);
			// FALLTHROUGH
		case EDIT_SCHEDULE:
		case NO_ROUTE:
			wait_lock = max( wait_lock, 25000 );
			break;

		// Same as waiting for free way
		case ROUTING_1:
			// immediate start required
			if (wait_lock == 0) break;
			// FALLTHROUGH
		case CAN_START:
		case WAITING_FOR_CLEARANCE:
			// unless a longer wait is requested
			if (wait_lock > 2500) {
				break;
			}
			// FALLTHROUGH

		// waiting for free way, not too heavy, not to slow
		case CAN_START_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
			// to avoid having a convoi stuck at a heavy traffic intersection/signal, the waiting time is randomized
			wait_lock = simrand(5000)+1;
			break;
		default: ;
	}
}


void convoi_t::new_year()
{
	jahresgewinn = 0;
}


void convoi_t::new_month()
{
	// should not happen: leftover convoi without vehicles ...
	if(vehicle_count==0) {
		DBG_DEBUG("convoi_t::new_month()","no vehicles => self destruct!");
		self_destruct();
		return;
	}
	// update statistics of average speed
	if(  maxspeed_average_count==0  ) {
		financial_history[0][CONVOI_MAXSPEED] = distance_since_last_stop>0 ? get_speedbonus_kmh() : 0;
	}
	maxspeed_average_count = 0;
	// everything normal: update histroy
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	// remind every new month again
	if(  state==NO_ROUTE  ) {
		get_owner()->report_vehicle_problem( self, get_pos() );
	}
	// check for traffic jam
	if(state==WAITING_FOR_CLEARANCE) {
		state = WAITING_FOR_CLEARANCE_ONE_MONTH;
		// check, if now free ...
		// might also reset the state!
		sint32 restart_speed = -1;
		if(  fahr[0]->can_enter_tile( restart_speed, 0 )  ) {
			state = DRIVING;
		}
		if(restart_speed>=0) {
			akt_speed = restart_speed;
		}
	}
	else if(state==WAITING_FOR_CLEARANCE_ONE_MONTH) {
		// make sure, not another vehicle with same line is loading in front
		bool notify = true;
		// check, if we are not waiting for load
		if(  line.is_bound()  &&  loading_level==0  ) {
			for(  uint i=0;  i < line->count_convoys();  i++  ) {
				convoihandle_t cnv = line->get_convoy(i);
				if(  cnv.is_bound()  &&  cnv->get_state()==LOADING  &&  cnv->get_loading_level() < cnv->get_loading_limit()  ) {
					// convoi on this line is waiting for load => assume we are waiting behind
					notify = false;
					break;
				}
			}
		}
		if(  notify  ) {
			get_owner()->report_vehicle_problem( self, koord3d::invalid );
		}
		state = WAITING_FOR_CLEARANCE_TWO_MONTHS;
	}
	// check for traffic jam
	if(state==CAN_START) {
		state = CAN_START_ONE_MONTH;
	}
	else if(state==CAN_START_ONE_MONTH  ||  state==CAN_START_TWO_MONTHS  ) {
		get_owner()->report_vehicle_problem( self, koord3d::invalid );
		state = CAN_START_TWO_MONTHS;
	}
	// check for obsolete vehicles in the convoi
	if(!has_obsolete  &&  welt->use_timeline()) {
		// convoi has obsolete vehicles?
		const int month_now = welt->get_timeline_year_month();
		has_obsolete = false;
		for(unsigned j=0;  j<get_vehicle_count();  j++ ) {
			if (fahr[j]->get_desc()->is_retired(month_now)) {
				has_obsolete = true;
				break;
			}
		}
	}
	// book fixed cost as running cost
	book( sum_fixed_costs, CONVOI_OPERATIONS );
	book( sum_fixed_costs, CONVOI_PROFIT );
	// since convois can be in the depot, the waytypewithout schedule is determined from the vechile (if there)
	// one can argue that convois in depots should not cost money ...
	waytype_t wtyp = ignore_wt;
	if(  schedule_t* s = get_schedule()  ) {
		wtyp = s->get_waytype();
	}
	else if(  !fahr.empty()  ) {
		wtyp = fahr[0]->get_waytype();
	}
	get_owner()->book_running_costs( sum_fixed_costs, wtyp );
	jahresgewinn += sum_fixed_costs;
}


void convoi_t::betrete_depot(depot_t *dep)
{
	// first remove reservation, if train is still on track
	unreserve_route();

	// remove vehicles from world data structure
	for(unsigned i=0; i<vehicle_count; i++) {
		vehicle_t* v = fahr[i];

		grund_t* gr = welt->lookup(v->get_pos());
		if(gr) {
			// remove from blockstrecke
			v->set_last(true);
			v->leave_tile();
			v->set_flag( obj_t::not_on_map );
		}
	}

	dep->convoi_arrived(self, get_schedule());

	destroy_win( magic_convoi_info+self.get_id() );

	maxspeed_average_count = 0;
	state = INITIAL;
}


void convoi_t::start()
{
	if(  state == EDIT_SCHEDULE  &&  schedule  &&  schedule->is_editing_finished()  ) {
		// go to defined starting state
		wait_lock = 0;
		step();
	}

	if(state == INITIAL || state == ROUTING_1) {

		// set home depot to location of depot convoi is leaving
		if(route.empty()) {
			home_depot = fahr[0]->get_pos();
		}
		else {
			home_depot = route.front();
			fahr[0]->set_pos( home_depot );
		}
		// put the convoi on the depot ground, to get automatic rotation
		// (vorfahren() will remove it anyway again.)
		grund_t *gr = welt->lookup( home_depot );
		assert(gr);
		gr->obj_add( fahr[0] );

		// put into sync list
		welt->sync.add(this);

		alte_richtung = ribi_t::none;
		no_load = false;

		state = ROUTING_1;

		// recalc weight and image
		// also for any vehicle entered a depot, set_letztes is true! => reset it correctly
		sint64 restwert_delta = 0;
		for(unsigned i=0; i<vehicle_count; i++) {
			fahr[i]->set_leading( false );
			fahr[i]->set_last( false );
			restwert_delta -= fahr[i]->calc_sale_value();
			fahr[i]->set_driven();
			restwert_delta += fahr[i]->calc_sale_value();
			fahr[i]->clear_flag( obj_t::not_on_map );
		}
		fahr[0]->set_leading( true );
		fahr[vehicle_count-1]->set_last( true );
		// do not show the vehicle - it will be wrong positioned -vorfahren() will correct this
		fahr[0]->set_image(IMG_EMPTY);

		// update finances for used vehicle reduction when first driven
		owner->update_assets( restwert_delta, get_schedule()->get_waytype());

		// calc state for convoi
		calc_loading();
		calc_speedbonus_kmh();
		maxspeed_average_count = 0;

		if(line.is_bound()) {
			// might have changed the vehicles in this car ...
			line->recalc_catg_index();
		}
		else {
			welt->set_schedule_counter();
		}
		wait_lock = 0;

		DBG_MESSAGE("convoi_t::start()","Convoi %s wechselt von INITIAL nach ROUTING_1", name_and_id);
	}
	else {
		dbg->warning("convoi_t::start()","called with state=%s\n",state_names[state]);
	}
}


/* called, when at a destination
 * can be waypoint, depot or a stop
 * called from the first vehicle_t of a convoi */
void convoi_t::ziel_erreicht()
{
	const vehicle_t* v = fahr[0];
	alte_richtung = v->get_direction();

	// check, what is at destination!
	const grund_t *gr = welt->lookup(v->get_pos());
	depot_t *dp = gr->get_depot();

	if(dp) {
		// ok, we are entering a depot
		cbuffer_t buf;

		// we still book the money for the trip; however, the freight will be deleted (by the vehicle in the depot itself)
		calc_gewinn();

		akt_speed = 0;
		buf.printf( translator::translate("%s has entered a depot."), get_name() );
		welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_owner()->get_player_nr(), IMG_EMPTY);

		betrete_depot(dp);
	}
	else {
		// no depot reached, check for stop!
		halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_entry().pos,owner);
		if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			calc_gewinn();

			akt_speed = 0;
			halt->book(1, HALT_CONVOIS_ARRIVED);
			state = LOADING;
			arrived_time = welt->get_ticks();
			last_load_tick = arrived_time;
		}
		else {
			// Neither depot nor station: waypoint
			schedule->advance();
			state = ROUTING_1;
		}
	}
	wait_lock = 0;
}


/**
 * Wartet bis Fahrzeug 0 freie Fahrt meldet
 */
void convoi_t::warten_bis_weg_frei(sint32 restart_speed)
{
	if(!is_waiting()) {
		state = WAITING_FOR_CLEARANCE;
		wait_lock = 0;
	}
	if(restart_speed>=0) {
		// langsam anfahren
		akt_speed = restart_speed;
	}
}


bool convoi_t::add_vehicle(vehicle_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehicle()","at pos %i of %i total vehicles.",vehicle_count,fahr.get_count());
	// extend array if requested
	if(vehicle_count == fahr.get_count()) {
		fahr.resize(vehicle_count+1,NULL);
DBG_MESSAGE("convoi_t::add_vehicle()","extend array_tpl to %i totals.",fahr.get_count());
	}
	// now append
	if (vehicle_count < fahr.get_count()) {
		v->set_convoi(this);

		if(infront) {
			for(unsigned i = vehicle_count; i > 0; i--) {
				fahr[i] = fahr[i - 1];
			}
			fahr[0] = v;
		}
		else {
			fahr[vehicle_count] = v;
		}
		vehicle_count ++;

		const vehicle_desc_t *info = v->get_desc();
		if(info->get_power()) {
			is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
		}
		sum_power += info->get_power();
		sum_gear_and_power += info->get_power()*info->get_gear();
		sum_weight += info->get_weight();
		sum_running_costs -= info->get_running_cost();
		sum_fixed_costs -= welt->scale_with_month_length( info->get_fixed_cost() );
		min_top_speed = min( min_top_speed, kmh_to_speed( v->get_desc()->get_topspeed() ) );
		sum_gesamtweight = sum_weight;
		calc_loading();
		freight_info_resort = true;
		// Add good_catg_index:
		if(v->get_cargo_max() != 0) {
			const goods_desc_t *ware=v->get_cargo_type();
			if(ware!=goods_manager_t::none  ) {
				goods_catg_index.append_unique( ware->get_catg_index() );
			}
		}
		// check for obsolete
		if(!has_obsolete  &&  welt->use_timeline()) {
			has_obsolete = info->is_retired( welt->get_timeline_year_month() );
		}
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

DBG_MESSAGE("convoi_t::add_vehicle()","now %i of %i total vehicles.",vehicle_count,fahr.get_count());
	return true;
}


vehicle_t *convoi_t::remove_vehicle_at(uint16 i)
{
	vehicle_t *v = NULL;
	if(i<vehicle_count) {
		v = fahr[i];
		if(v != NULL) {
			for(unsigned j=i; j<vehicle_count-1u; j++) {
				fahr[j] = fahr[j + 1];
			}

			v->set_convoi(NULL);

			--vehicle_count;
			fahr[vehicle_count] = NULL;

			const vehicle_desc_t *info = v->get_desc();
			sum_power -= info->get_power();
			sum_gear_and_power -= info->get_power()*info->get_gear();
			sum_weight -= info->get_weight();
			sum_running_costs += info->get_running_cost();
			sum_fixed_costs += welt->scale_with_month_length( info->get_fixed_cost() );
		}
		sum_gesamtweight = sum_weight;
		calc_loading();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(vehicle_count > 0) {
			set_erstes_letztes();
		}

		// calculate new minimum top speed
		min_top_speed = calc_min_top_speed(fahr, vehicle_count);

		// check for obsolete
		if(has_obsolete) {
			has_obsolete = false;
			const int month_now = welt->get_timeline_year_month();
			for(unsigned i=0; i<vehicle_count; i++) {
				has_obsolete |= fahr[i]->get_desc()->is_retired(month_now);
			}
		}

		recalc_catg_index();

		// still requires electrifications?
		if(is_electric) {
			is_electric = false;
			for(unsigned i=0; i<vehicle_count; i++) {
				if(fahr[i]->get_desc()->get_power()) {
					is_electric |= fahr[i]->get_desc()->get_engine_type()==vehicle_desc_t::electric;
				}
			}
		}
	}
	return v;
}


// recalc what good this convoy is moving
void convoi_t::recalc_catg_index()
{
	goods_catg_index.clear();

	for(  uint8 i = 0;  i < get_vehicle_count();  i++  ) {
		// Only consider vehicles that really transport something
		// this helps against routing errors through passenger
		// trains pulling only freight wagons
		if(get_vehicle(i)->get_cargo_max() == 0) {
			continue;
		}
		const goods_desc_t *ware=get_vehicle(i)->get_cargo_type();
		if(ware!=goods_manager_t::none  ) {
			goods_catg_index.append_unique( ware->get_catg_index() );
		}
	}
	/* since during composition of convois all kinds of composition could happen,
	 * we do not enforce schedule recalculation here; it will be done anyway all times when leaving the INTI state ...
	 */
}


void convoi_t::set_erstes_letztes()
{
	// vehicle_count muss korrekt init sein
	if(vehicle_count>0) {
		fahr[0]->set_leading(true);
		for(unsigned i=1; i<vehicle_count; i++) {
			fahr[i]->set_leading(false);
			fahr[i - 1]->set_last(false);
		}
		fahr[vehicle_count - 1]->set_last(true);
	}
	else {
		dbg->warning("convoi_t::set_erstes_letzes()", "called with vehicle_count==0!");
	}
}


// remove wrong freight when schedule changes etc.
void convoi_t::check_freight()
{
	for(unsigned i=0; i<vehicle_count; i++) {
		fahr[i]->remove_stale_cargo();
	}
	calc_loading();
	freight_info_resort = true;
}


bool convoi_t::set_schedule(schedule_t * f)
{
	if(  state==SELF_DESTRUCT  ) {
		return false;
	}

	DBG_DEBUG("convoi_t::set_schedule()", "new=%p, old=%p", f, schedule);
	assert(f != NULL);

	// happens to be identical?
	if(schedule!=f) {
		// now check, we we have been bond to a line we are about to lose:
		bool changed = false;
		if(  line.is_bound()  ) {
			if(  !f->matches( welt, line->get_schedule() )  ) {
				// change from line to individual schedule
				// -> unset line now and register stops from new schedule later
				changed = true;
				unset_line();
			}
		}
		else {
			if(  !f->matches( welt, schedule )  ) {
				// merely change schedule and do not involve line
				// -> unregister stops from old schedule now and register stops from new schedule later
				changed = true;
				unregister_stops();
			}
		}
#ifdef BAD_IDEA
		// destroy a possibly open schedule window
		if(  schedule  &&  !schedule->is_editing_finished()  ) {
			destroy_win((ptrdiff_t)schedule);
			delete schedule;
		}
#endif
		schedule = f;
		if(  changed  ) {
			// if line is unset or schedule is changed
			// -> register stops from new schedule
			register_stops();
			welt->set_schedule_counter(); // must trigger refresh
		}
	}

	// remove wrong freight
	check_freight();

	// ok, now we have a schedule
	if(state != INITIAL) {
		state = EDIT_SCHEDULE;
	}
	// to avoid jumping trains
	alte_richtung = fahr[0]->get_direction();
	wait_lock = 0;
	return true;
}


schedule_t *convoi_t::create_schedule()
{
	if(schedule == NULL) {
		const vehicle_t* v = fahr[0];

		if (v != NULL) {
			schedule = v->generate_new_schedule();
			schedule->finish_editing();
		}
	}

	return schedule;
}


/* checks, if we go in the same direction;
 * true: convoy prepared
 * false: must recalculate position
 * on all error we better use the normal starting procedure ...
 */
bool convoi_t::can_go_alte_richtung()
{
	// invalid route? nothing to test, must start new
	if(route.empty()) {
		return false;
	}

	// going backwards? then recalculate all
	ribi_t::ribi neue_richtung_rwr = ribi_t::backward(fahr[0]->calc_direction(route.front(), route.at(min(2, route.get_count() - 1))));
//	DBG_MESSAGE("convoi_t::go_alte_richtung()","neu=%i,rwr_neu=%i,alt=%i",neue_richtung_rwr,ribi_t::backward(neue_richtung_rwr),alte_richtung);
	if(neue_richtung_rwr&alte_richtung) {
		akt_speed = 8;
		return false;
	}

	// now get the actual length and the tile length
	uint16 convoi_length = 15;
	uint16 tile_length = 24;
	unsigned i; // for visual C++
	const vehicle_t* pred = NULL;
	for(i=0; i<vehicle_count; i++) {
		const vehicle_t* v = fahr[i];
		grund_t *gr = welt->lookup(v->get_pos());

		// not last vehicle?
		// the length of last vehicle does not matter when it comes to positioning of vehicles
		if ( i+1 < vehicle_count) {
			convoi_length += v->get_desc()->get_length();
		}

		if(gr==NULL  ||  (pred!=NULL  &&  (abs(v->get_pos().x-pred->get_pos().x)>=2  ||  abs(v->get_pos().y-pred->get_pos().y)>=2))  ) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_richtung()","broken convoy (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}

		// now check, if ribi is straight and train is not
		ribi_t::ribi weg_ribi = gr->get_weg_ribi_unmasked(v->get_waytype());
		if(ribi_t::is_straight(weg_ribi)  &&  (weg_ribi|v->get_direction())!=weg_ribi) {
			dbg->warning("convoi_t::go_alte_richtung()","convoy with wrong vehicle directions (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}

		if(  pred  &&  pred->get_pos()!=v->get_pos()  ) {
			tile_length += (ribi_t::is_straight(welt->lookup(pred->get_pos())->get_weg_ribi_unmasked(pred->get_waytype())) ? 16 : 8192/vehicle_t::get_diagonal_multiplier())*koord_distance(pred->get_pos(),v->get_pos());
		}

		pred = v;
	}
	// check if convoi is way too short (even for diagonal tracks)
	tile_length += (ribi_t::is_straight(welt->lookup(fahr[vehicle_count-1]->get_pos())->get_weg_ribi_unmasked(fahr[vehicle_count-1]->get_waytype())) ? 16 : 8192/vehicle_t::get_diagonal_multiplier());
	if(  convoi_length>tile_length  ) {
		dbg->warning("convoi_t::go_alte_richtung()","convoy too short (id %i) => fixing!",self.get_id());
		akt_speed = 8;
		return false;
	}

	uint16 length = min((convoi_length/16u)+4u,route.get_count()); // maximum length in tiles to check

	// we just check, whether we go back (i.e. route tiles other than zero have convoi vehicles on them)
	for( int index=1;  index<length;  index++ ) {
		grund_t *gr=welt->lookup(route.at(index));
		// now check, if we are already here ...
		for(unsigned i=0; i<vehicle_count; i++) {
			if (gr->obj_ist_da(fahr[i])) {
				// we are turning around => start slowly and rebuilt train
				akt_speed = 8;
				return false;
			}
		}
	}

//DBG_MESSAGE("convoi_t::go_alte_richtung()","alte=%d, neu_rwr=%d",alte_richtung,neue_richtung_rwr);

	// we continue our journey; however later cars need also a correct route entry
	// eventually we need to add their positions to the convois route
	koord3d pos = fahr[0]->get_pos();
	assert(pos == route.front());
	if(welt->lookup(pos)->get_depot()) {
		return false;
	}
	else {
		for(i=0; i<vehicle_count; i++) {
			vehicle_t* v = fahr[i];
			// eventually add current position to the route
			if (route.front() != v->get_pos() && route.at(1) != v->get_pos()) {
				route.insert(v->get_pos());
			}
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we must set the current route index (instead assuming 1)
	length = min((convoi_length/8u),route.get_count()-1); // maximum length in tiles to check
	bool ok=false;
	for(i=0; i<vehicle_count; i++) {
		vehicle_t* v = fahr[i];

		// this is such awkward, since it takes into account different vehicle length
		const koord3d vehicle_start_pos = v->get_pos();
		for( route_t::index_t idx = 0;  idx<=length;  idx++  ) {
			if(route.at(idx)==vehicle_start_pos) {
				// set route index, no recalculations necessary
				v->initialise_journey(idx, false );
				ok = true;

				// check direction
				uint8 richtung = v->get_direction();
				uint8 neu_richtung = v->calc_direction( route.at(max(idx-1,0)), v->get_pos_next());
				// we need to move to this place ...
				if(neu_richtung!=richtung  &&  (i!=0  ||  vehicle_count==1  ||  ribi_t::is_bend(neu_richtung)) ) {
					// 90 deg bend!
					return false;
				}

				break;
			}
		}
		// too short?!? (rather broken then!)
		if(!ok) {
			return false;
		}
	}

	return true;
}


// put the convoi on its way
void convoi_t::vorfahren()
{
	// init speed settings
	sp_soll = 0;
	set_tiles_overtaking( 0 );
	recalc_data_front = true;
	recalc_data = true;

	koord3d k0 = route.front();
	grund_t *gr = welt->lookup(k0);
	bool at_dest = false;
	if(gr  &&  gr->get_depot()) {
		// start in depot
		for(unsigned i=0; i<vehicle_count; i++) {
			vehicle_t* v = fahr[i];

			// remove from old position
			grund_t* gr = welt->lookup(v->get_pos());
			if(gr) {
				gr->obj_remove(v);
				if(gr->ist_uebergang()) {
					crossing_t *cr = gr->find<crossing_t>(2);
					cr->release_crossing(v);
				}
				// eventually unreserve this
				if(  schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(fahr[i]->get_waytype()))  ) {
					sch0->unreserve(v);
				}
			}
			v->initialise_journey(0, true);
			// set at new position
			gr = welt->lookup(v->get_pos());
			assert(gr);
			v->enter_tile(gr);
		}

		// just advances the first vehicle
		vehicle_t* v0 = fahr[0];
		v0->set_leading(false); // switches off signal checks ...
		v0->get_smoke(false);
		steps_driven = 0;
		// drive half a tile:
		for(int i=0; i<vehicle_count; i++) {
			fahr[i]->do_drive( (VEHICLE_STEPS_PER_TILE/2)<<YARDS_PER_VEHICLE_STEP_SHIFT );
		}
		v0->get_smoke(true);
		v0->set_leading(true); // switches on signal checks to reserve the next route

		// until all other are on the track
		state = CAN_START;
	}
	else {
		// still leaving depot (steps_driven!=0) or going in other direction or misalignment?
		if(  steps_driven>0  ||  !can_go_alte_richtung()  ) {

			// start route from the beginning at index 0, place everything on start
			uint32 train_length = move_to(0);

			// move one train length to the start position ...
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->get_direction();
			if(neue_richtung==ribi_t::south  ||  neue_richtung==ribi_t::east) {
				// drive the convoi to the same position, but do not hop into next tile!
				if(  train_length%16==0  ) {
					// any space we need => just add
					train_length += fahr[vehicle_count-1]->get_desc()->get_length();
				}
				else {
					// limit train to front of tile
					train_length += min( (train_length%CARUNITS_PER_TILE)-1, fahr[vehicle_count-1]->get_desc()->get_length() );
				}
			}
			else {
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			fahr[0]->set_leading(false); // switches off signal checks ...
			uint32 dist = VEHICLE_STEPS_PER_CARUNIT*train_length<<YARDS_PER_VEHICLE_STEP_SHIFT;
			for(unsigned i=0; i<vehicle_count; i++) {
				vehicle_t* v = fahr[i];

				v->get_smoke(false);
				uint32 const driven = fahr[i]->do_drive( dist );
				if (i==0  &&  driven < dist) {
					// we are already at our destination
					at_dest = true;
				}
				// this gives the length in carunits, 1/CARUNITS_PER_TILE of a full tile => all cars closely coupled!
				v->get_smoke(true);

				uint32 const vlen = ((VEHICLE_STEPS_PER_CARUNIT*v->get_desc()->get_length())<<YARDS_PER_VEHICLE_STEP_SHIFT);
				if (vlen > dist) {
					break;
				}
				dist = driven - vlen;
			}
			fahr[0]->set_leading(true);
		}
		if (!at_dest) {
			state = CAN_START;

			// to advance more smoothly
			sint32 restart_speed = -1;
			if(  fahr[0]->can_enter_tile( restart_speed, 0 )  ) {
				// can reserve new block => drive on
				if(haltestelle_t::get_halt(k0,owner).is_bound()) {
					fahr[0]->play_sound();
				}
				state = DRIVING;
			}
		}
		else {
			ziel_erreicht();
		}
	}

	// finally reserve route (if needed)
	if(  fahr[0]->get_waytype()!=air_wt  &&  !at_dest  ) {
		// do not pre-reserve for airplanes
		for(unsigned i=0; i<vehicle_count; i++) {
			// eventually reserve this
			vehicle_t const& v = *fahr[i];
			if (schiene_t* const sch0 = obj_cast<schiene_t>(welt->lookup(v.get_pos())->get_weg(v.get_waytype()))) {
				sch0->reserve(self,ribi_t::none);
			}
			else {
				break;
			}
		}
	}

	wait_lock = 0;
	INT_CHECK("simconvoi 711");
}


void convoi_t::rdwr_convoihandle_t(loadsave_t *file, convoihandle_t &cnv)
{
	if(  file->is_version_atleast(112, 3)  ) {
		uint16 id = (file->is_saving()  &&  cnv.is_bound()) ? cnv.get_id() : 0;
		file->rdwr_short( id );
		if (file->is_loading()) {
			cnv.set_id( id );
		}
	}
}


void convoi_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "convoi_t" );

	sint32 dummy;
	sint32 owner_n = welt->sp2num(owner);

	if(file->is_saving()) {
		if(  file->is_version_less(101, 0)  ) {
			file->wr_obj_id("Convoi");
			// the matching read is in karte_t::laden(loadsave*)...
		}
	}

	// do the update, otherwise we might lose the line after save & reload
	if(file->is_saving()  &&  line_update_pending.is_bound()) {
		check_pending_updates();
	}

	simline_t::rdwr_linehandle_t(file, line);

	// we want persistent convoihandles so we can keep dialogues open in network games
	if(  file->is_loading()  ) {
		if(  file->is_version_less(112, 3)  ) {
			self = convoihandle_t( this );
		}
		else {
			uint16 id;
			file->rdwr_short( id );
			self = convoihandle_t( this, id );
		}
	}
	else if(  file->is_version_atleast(112, 3)  ) {
		uint16 id = self.get_id();
		file->rdwr_short( id );
	}

	dummy = vehicle_count;
	file->rdwr_long(dummy);
	vehicle_count = (uint8)dummy;

	if(file->is_version_less(99, 14)) {
		// was anz_ready
		file->rdwr_long(dummy);
	}

	file->rdwr_long(wait_lock);
	// some versions may produce broken safegames apparently
	if(wait_lock > 60000) {
		dbg->warning("convoi_t::sync_prepre()","Convoi %d: wait lock out of bounds: wait_lock = %d, setting to 60000",self.get_id(), wait_lock);
		wait_lock = 60000;
	}

	bool dummy_bool=false;
	file->rdwr_bool(dummy_bool);
	file->rdwr_long(owner_n);
	file->rdwr_long(akt_speed);
	file->rdwr_long(akt_speed_soll);
	file->rdwr_long(sp_soll);
	file->rdwr_enum(state);
	file->rdwr_enum(alte_richtung);

	// read the yearly income (which has since then become a 64 bit value)
	// will be recalculated later directly from the history
	if(file->is_version_less(89, 4)) {
		file->rdwr_long(dummy);
	}

	route.rdwr(file);

	if(file->is_loading()) {
		// extend array if requested (only needed for trains)
		if(vehicle_count > fahr.get_count()) {
			fahr.resize(vehicle_count, NULL);
		}
		owner = welt->get_player( owner_n );

		// sanity check for values ... plus correction
		if(sp_soll < 0) {
			sp_soll = 0;
		}
	}

	file->rdwr_str(name_and_id + name_offset, lengthof(name_and_id) - name_offset);
	if(file->is_loading()) {
		set_name(name_and_id+name_offset); // will add id automatically
	}

	koord3d dummy_pos;
	if(file->is_saving()) {
		for(unsigned i=0; i<vehicle_count; i++) {
			file->wr_obj_id( fahr[i]->get_typ() );
			fahr[i]->rdwr_from_convoi(file);
		}
	}
	else {
		bool override_monorail = false;
		is_electric = false;
		for(  uint8 i=0;  i<vehicle_count;  i++  ) {
			obj_t::typ typ = (obj_t::typ)file->rd_obj_id();
			vehicle_t *v = 0;

			const bool first = (i==0);
			const bool last = (i==vehicle_count-1u);
			if(override_monorail) {
				// ignore type for ancient monorails
				v = new monorail_vehicle_t(file, first, last);
			}
			else {
				switch(typ) {
					case obj_t::old_automobil:
					case obj_t::road_vehicle: v = new road_vehicle_t(file, first, last);  break;
					case obj_t::old_waggon:
					case obj_t::rail_vehicle:    v = new rail_vehicle_t(file, first, last);     break;
					case obj_t::old_schiff:
					case obj_t::water_vehicle:    v = new water_vehicle_t(file, first, last);     break;
					case obj_t::old_aircraft:
					case obj_t::air_vehicle:    v = new air_vehicle_t(file, first, last);     break;
					case obj_t::old_monorailwaggon:
					case obj_t::monorail_vehicle:    v = new monorail_vehicle_t(file, first, last);     break;
					case obj_t::maglev_vehicle:         v = new maglev_vehicle_t(file, first, last);     break;
					case obj_t::narrowgauge_vehicle:    v = new narrowgauge_vehicle_t(file, first, last);     break;
					default:
						dbg->fatal("convoi_t::convoi_t()","Can't load vehicle type %d", typ);
				}
			}

			// no matching vehicle found?
			if(v->get_desc()==NULL) {
				// will create orphan object, but better than crashing at deletion ...
				dbg->error("convoi_t::convoi_t()","Can't load vehicle and no replacement found!");
				i --;
				vehicle_count --;
				continue;
			}

			// in very old games, monorail was a railway
			// so we need to convert this
			// freight will be lost, but game will be loadable
			if(i==0  &&  v->get_desc()->get_waytype()==monorail_wt  &&  v->get_typ()==obj_t::rail_vehicle) {
				override_monorail = true;
				vehicle_t *v_neu = new monorail_vehicle_t( v->get_pos(), v->get_desc(), v->get_owner(), NULL );
				v->discard_cargo();
				delete v;
				v = v_neu;
			}

			if(file->is_version_less(99, 4)) {
				dummy_pos.rdwr(file);
			}

			const vehicle_desc_t *info = v->get_desc();
			assert(info);

			// if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				sum_power += info->get_power();
				sum_gear_and_power += info->get_power()*info->get_gear();
				sum_weight += info->get_weight();
				sum_running_costs -= info->get_running_cost();
				sum_fixed_costs -= welt->scale_with_month_length( info->get_fixed_cost() );
				is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
				has_obsolete |= welt->use_timeline()  &&  info->is_retired( welt->get_timeline_year_month() );
				// we do not add maintenance here, the fixed costs are booked as running costs
			}

			// some versions save vehicles after leaving depot with koord3d::invalid
			if(v->get_pos()==koord3d::invalid) {
				state = INITIAL;
			}

			if(state!=INITIAL) {
				grund_t *gr;
				gr = welt->lookup(v->get_pos());
				if(!gr) {
					gr = welt->lookup_kartenboden(v->get_pos().get_2d());
					if(gr) {
						dbg->error("convoi_t::rdwr()", "invalid position %s for vehicle %s in state %d (setting to %i,%i,%i)", v->get_pos().get_str(), v->get_name(), state, gr->get_pos().x, gr->get_pos().y, gr->get_pos().z );
						v->set_pos( gr->get_pos() );
					}
					else {
						dbg->fatal("convoi_t::rdwr()", "invalid position %s for vehicle %s in state %d", v->get_pos().get_str(), v->get_name(), state);
					}
					state = INITIAL;
				}
				// add to blockstrecke
				if(v->get_waytype()==track_wt  ||  v->get_waytype()==monorail_wt  ||  v->get_waytype()==maglev_wt  ||  v->get_waytype()==narrowgauge_wt) {
					schiene_t* sch = (schiene_t*)gr->get_weg(v->get_waytype());
					if(sch) {
						sch->reserve(self,ribi_t::none);
					}
					// add to crossing
					if(gr->ist_uebergang()) {
						gr->find<crossing_t>()->add_to_crossing(v);
					}
				}
				if(  gr->get_top()>253  ) {
					dbg->warning( "convoi_t::rdwr()", "cannot put vehicle on ground at (%s)", gr->get_pos().get_str() );
				}
				gr->obj_add(v);
				v->clear_flag(obj_t::not_on_map);
			}
			else {
				v->set_flag(obj_t::not_on_map);
			}

			// add to convoi
			fahr[i] = v;
		}
		sum_gesamtweight = sum_weight;
	}

	bool has_schedule = (schedule != NULL);
	file->rdwr_bool(has_schedule);
	if(has_schedule) {
		//DBG_MESSAGE("convoi_t::rdwr()","convoi has a schedule, state %s!",state_names[state]);
		const vehicle_t* v = fahr[0];
		if(file->is_loading() && v) {
			schedule = v->generate_new_schedule();
		}
		// hack to load corrupted games -> there is a schedule
		// but no vehicle so we can't determine the exact type of
		// schedule needed. This hack is safe because convois
		// without vehicles get deleted right after loading.
		// Since generic schedules are not allowed, we use a train_schedule_t
		if(schedule == 0) {
			schedule = new train_schedule_t();
		}

		// now read the schedule, we have one for sure here
		schedule->rdwr( file );
	}

	if(file->is_loading()) {
		next_wolke = 0;
		calc_loading();
	}

	// calculate new minimum top speed
	min_top_speed = calc_min_top_speed(fahr, vehicle_count);

	// since sp_ist became obsolete, sp_soll is used modulo 65536
	sp_soll &= 65535;

	if(file->is_version_less(88, 4)) {
		// load statistics
		int j;
		for (j = 0; j<3; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (j = 2; j<5; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][CONVOI_DISTANCE] = 0;
			financial_history[k][CONVOI_MAXSPEED] = 0;
			financial_history[k][CONVOI_WAYTOLL] = 0;
		}
	}
	else if(  file->is_version_less(102, 3)  ){
		// load statistics
		for (int j = 0; j<5; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][CONVOI_DISTANCE] = 0;
			financial_history[k][CONVOI_MAXSPEED] = 0;
			financial_history[k][CONVOI_WAYTOLL] = 0;
		}
	}
	else if(  file->is_version_less(111, 1)  ){
		// load statistics
		for (int j = 0; j<6; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][CONVOI_MAXSPEED] = 0;
			financial_history[k][CONVOI_WAYTOLL] = 0;
		}
	}
	else if(  file->is_version_less(112, 8)  ){
		// load statistics
		for (int j = 0; j<7; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][CONVOI_WAYTOLL] = 0;
		}
	}
	else
	{
		// load statistics
		for (int j = 0; j<MAX_CONVOI_COST; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}

	// the convoi odometer
	if(  file->is_version_atleast(102, 3)  ){
		file->rdwr_longlong( total_distance_traveled);
	}

	// since it was saved as an signed int
	// we recalc it anyhow
	if(file->is_loading()) {
		jahresgewinn = 0;
		for(int i=welt->get_last_month()%12;  i>=0;  i--  ) {
			jahresgewinn += financial_history[i][CONVOI_PROFIT];
		}
	}

	// save/restore pending line updates
	if(file->is_version_atleast(84, 9)  &&  file->is_version_less(99, 13)) {
		file->rdwr_long(dummy); // ignore
	}
	if(file->is_loading()) {
		line_update_pending = linehandle_t();
	}

	if(file->is_version_atleast(84, 10)) {
		home_depot.rdwr(file);
	}

	// Old versions recorded last_stop_pos in convoi, not in vehicle
	koord3d last_stop_pos_convoi = koord3d(0,0,0);
	if (vehicle_count !=0) {
		last_stop_pos_convoi = fahr[0]->last_stop_pos;
	}
	if(file->is_version_atleast(87, 1)) {
		last_stop_pos_convoi.rdwr(file);
	}
	else {
		last_stop_pos_convoi =
			!route.empty()   ? route.front()      :
			vehicle_count != 0 ? fahr[0]->get_pos() :
			koord3d(0, 0, 0);
	}

	// for leaving the depot routine
	if(file->is_version_less(99, 14)) {
		steps_driven = -1;
	}
	else {
		file->rdwr_short(steps_driven);
	}

	// waiting time left ...
	if(file->is_version_atleast(99, 17)) {
		if(file->is_saving()) {
			if(  has_schedule  &&  schedule->get_current_entry().waiting_time > 0  ) {
				uint32 diff_ticks = arrived_time + schedule->get_current_entry().get_waiting_ticks() - welt->get_ticks();
				file->rdwr_long(diff_ticks);
			}
			else {
				uint32 diff_ticks = 0xFFFFFFFFu; // write old WAIT_INFINITE value for backwards compatibility
				file->rdwr_long(diff_ticks);
			}
		}
		else {
			uint32 diff_ticks = 0;
			file->rdwr_long(diff_ticks);
			arrived_time = has_schedule ? welt->get_ticks() - schedule->get_current_entry().get_waiting_ticks() + diff_ticks : 0;
		}
	}

	// since 99015, the last stop will be maintained by the vehicles themselves
	if(file->is_version_less(99, 15)) {
		for(unsigned i=0; i<vehicle_count; i++) {
			fahr[i]->last_stop_pos = last_stop_pos_convoi;
		}
	}

	// overtaking status
	if(file->is_version_less(100, 1)) {
		set_tiles_overtaking( 0 );
	}
	else {
		file->rdwr_byte(tiles_overtaking);
		set_tiles_overtaking( tiles_overtaking );
	}
	// no_load, withdraw
	if(file->is_version_less(102, 1)) {
		no_load = false;
		withdraw = false;
	}
	else {
		file->rdwr_bool(no_load);
		file->rdwr_bool(withdraw);
	}

	if(file->is_version_atleast(111, 1)) {
		file->rdwr_long( distance_since_last_stop );
		file->rdwr_long( sum_speed_limit );
	}

	if(  file->is_version_atleast(111, 2)  ) {
		file->rdwr_long( maxspeed_average_count );
	}

	if(  file->is_version_atleast(111, 3)  ) {
		file->rdwr_short( next_stop_index );
		file->rdwr_short( next_reservation_index );
	}

	if(  file->is_loading()  ) {
		reserve_route();
		recalc_catg_index();
		unloading_state = false;
	}
}


void convoi_t::open_info_window()
{
	if(  in_depot()  ) {
		// if ownership matches, we can try to open the depot dialog
		if(  get_owner()==welt->get_active_player()  ) {
			grund_t *const ground = welt->lookup( get_home_depot() );
			if(  ground  ) {
				depot_t *const depot = ground->get_depot();
				if(  depot  ) {
					depot->show_info();
					// try to activate this particular convoy in the depot
					depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
					if(  frame  ) {
						frame->activate_convoi(self);
					}
				}
			}
		}
	}
	else {
		if(  env_t::verbose_debug >= log_t::LEVEL_ERROR  ) {
			dump();
		}
		create_win( new convoi_info_t(self), w_info, magic_convoi_info+self.get_id() );
	}
}


void convoi_t::info(cbuffer_t & buf) const
{
	const vehicle_t* v = fahr[0];
	if (v != NULL) {
		char tmp[128];

		buf.printf("\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->get_desc()->get_topspeed(), get_running_cost() / 100.0);
		buf.printf(" %s: %ikW\n", translator::translate("Leistung"), sum_power);
		buf.printf(" %s: %ld (%ld) t\n", translator::translate("Gewicht"), (long)sum_weight, (long)(sum_gesamtweight - sum_weight));
		buf.printf(" %s: ", translator::translate("Gewinn"));
		money_to_string(tmp, (double)jahresgewinn);
		buf.append(tmp);
		buf.append("\n");
	}
}


// sort order of convoi
void convoi_t::set_sortby(uint8 sort_order)
{
	freight_info_order = sort_order;
	freight_info_resort = true;
}


// caches the last info; resorts only when needed
void convoi_t::get_freight_info(cbuffer_t & buf)
{
	if(freight_info_resort) {
		freight_info_resort = false;
		// rebuilt the list with goods ...
		vector_tpl<ware_t> total_fracht;

		size_t const n = goods_manager_t::get_count();
		ALLOCA(uint32, max_loaded_waren, n);
		MEMZERON(max_loaded_waren, n);

		for(  uint32 i = 0;  i != vehicle_count;  ++i  ) {
			const vehicle_t* v = fahr[i];

			// first add to capacity indicator
			const goods_desc_t* ware_desc = v->get_desc()->get_freight_type();
			const uint16 menge = v->get_desc()->get_capacity();
			if(menge>0  &&  ware_desc!=goods_manager_t::none) {
				max_loaded_waren[ware_desc->get_index()] += menge;
			}

			// then add the actual load
			for(ware_t ware : v->get_cargo()) {
				for(ware_t & tmp : total_fracht) {
					// could this be joined with existing freight?

					// for pax: join according next stop
					// for all others we *must* use target coordinates
					if( ware.same_destination(tmp) ) {
						tmp.menge += ware.menge;
						ware.menge = 0;
						break;
					}
				}

				// if != 0 we could not join it to existing => load it
				if(ware.menge != 0) {
					total_fracht.append(ware);
				}
			}

			INT_CHECK("simconvoi 2643");
		}
		buf.clear();

		// apend info on total capacity
		slist_tpl <ware_t>capacity;
		for (uint16 i = 0; i != n; ++i) {
			if(max_loaded_waren[i]>0  &&  i!=goods_manager_t::INDEX_NONE) {
				ware_t ware(goods_manager_t::get_info(i));
				ware.menge = max_loaded_waren[i];
				// append to category?
				slist_tpl<ware_t>::iterator j   = capacity.begin();
				slist_tpl<ware_t>::iterator end = capacity.end();
				while (j != end && j->get_desc()->get_catg_index() < ware.get_desc()->get_catg_index()) ++j;
				if (j != end && j->get_desc()->get_catg_index() == ware.get_desc()->get_catg_index()) {
					j->menge += max_loaded_waren[i];
				} else {
					// not yet there
					capacity.insert(j, ware);
				}
			}
		}

		// show new info
		freight_list_sorter_t::sort_freight(total_fracht, buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, &capacity, "loaded");
	}
}


void convoi_t::open_schedule_window( bool show )
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %hu, State = %d, Lock = %d", self.get_id(), (int)state, wait_lock);

	// manipulation of schedule not allowed while:
	// - just starting
	// - a line update is pending
	if(  (state==EDIT_SCHEDULE  ||  line_update_pending.is_bound())  &&  get_owner()==welt->get_active_player()  ) {
		if (show) {
			create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		}
		return;
	}

	if(state==DRIVING) {
		// book the current value of goods
		calc_gewinn();
	}

	akt_speed = 0; // stop the train ...
	if(state!=INITIAL) {
		state = EDIT_SCHEDULE;
	}
	wait_lock = 25000;
	alte_richtung = fahr[0]->get_direction();

	ptrdiff_t magic = (ptrdiff_t)(magic_convoi_info + self.get_id());
	if(  show  ) {
		// Open schedule dialog
		if(  convoi_info_t *info = dynamic_cast<convoi_info_t*>(win_get_magic( magic )) ) {
			info->change_schedule();
			top_win(info);
		}
		else {
			create_schedule();
			create_win( new convoi_info_t(self,true), w_info, magic );
		}
		// TODO: what happens if no client opens the window??
	}
	schedule->start_editing();
}


/**
 * Check validity of convoi with respect to vehicle constraints
 */
bool convoi_t::pruefe_alle()
{
	bool ok = vehicle_count == 0  ||  fahr[0]->get_desc()->can_follow(NULL);
	unsigned i;

	const vehicle_t* pred = fahr[0];
	for(i = 1; ok && i < vehicle_count; i++) {
		const vehicle_t* v = fahr[i];
		ok = pred->get_desc()->can_lead(v->get_desc())  &&
				 v->get_desc()->can_follow(pred->get_desc());
		pred = v;
	}
	if(ok) {
		ok = pred->get_desc()->can_lead(NULL);
	}

	return ok;
}


/**
 * Kontrolliert Be- und Entladen
 *
 * minimum_loading is now stored in the object (not returned)
 */
void convoi_t::laden()
{
	if(  state == EDIT_SCHEDULE  ) {
		return;
	}

	// just wait a little longer if this is a non-bound halt
	wait_lock = (WTT_LOADING*2)+(self.get_id())%1024;

	// find station (ours or public)
	halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_entry().pos,owner);
	if(  halt.is_bound()  ) {
		// queue for (un)loading, does (un)loading once per step
		halt->request_loading( self );
	}
}


/**
 * calculate income for last hop
 */
void convoi_t::calc_gewinn()
{
	sint64 gewinn = 0;

	for(unsigned i=0; i<vehicle_count; i++) {
		vehicle_t* v = fahr[i];
		sint64 tmp;
		gewinn += tmp = v->calc_revenue(v->last_stop_pos, v->get_pos() );
		// get_schedule is needed as v->get_waytype() returns track_wt for trams (instead of tram_wt
		owner->book_revenue(tmp, fahr[0]->get_pos().get_2d(), get_schedule()->get_waytype(), v->get_cargo_type()->get_index() );
		v->last_stop_pos = v->get_pos();
	}

	// update statistics of average speed
	if(  distance_since_last_stop  ) {
		financial_history[0][CONVOI_MAXSPEED] *= maxspeed_average_count;
		financial_history[0][CONVOI_MAXSPEED] += get_speedbonus_kmh();
		maxspeed_average_count ++;
		financial_history[0][CONVOI_MAXSPEED] /= maxspeed_average_count;
	}
	distance_since_last_stop = 0;
	sum_speed_limit = 0;

	if(gewinn) {
		jahresgewinn += gewinn;

		book(gewinn, CONVOI_PROFIT);
		book(gewinn, CONVOI_REVENUE);
	}
}



uint32 convoi_t::get_departure_ticks(uint32 ticks_at_arrival) const
{
	// we need to make it this complicated, otherwise times versus the end of a month could be missed
	uint32 arrived_month_tick = ticks_at_arrival & ~(welt->ticks_per_world_month - 1);
	uint32 arrived_ticks = ticks_at_arrival - arrived_month_tick;
	uint32 delta = schedule->get_current_entry().get_absolute_departures();
	uint32 departure_ticks = schedule->get_current_entry().get_waiting_ticks();

	// there could be more than one departure per month => find the next one
	for( uint i = 0; i<delta; i++ ) {
		uint32 next_depature_slot = departure_ticks + (i*(welt->ticks_per_world_month/delta));
		if( next_depature_slot > arrived_ticks ) {
			return arrived_month_tick+next_depature_slot;
		}
	}
	// nothing there => depart slot is first one in next month
	return arrived_month_tick+welt->ticks_per_world_month+departure_ticks;
}



/**
 * convoi an haltestelle anhalten
 *
 * minimum_loading is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(halthandle_t halt)
{
	// now find out station length
	uint16 vehicles_loading = 0;
	if(fahr[0]->get_desc()->get_waytype() == water_wt) {
		// harbour and river stop load any size
		vehicles_loading = vehicle_count;
	}
	else if (vehicle_count == 1  &&  CARUNITS_PER_TILE >= fahr[0]->get_desc()->get_length()) {
		vehicles_loading = 1;
		// one vehicle, which fits into one tile
	}
	else {
		// difference between actual station length and vehicle lenghts
		sint16 station_length = -fahr[vehicles_loading]->get_desc()->get_length();
		// iterate through tiles in straight line and look for station
		koord zv = koord( ribi_t::backward(fahr[0]->get_direction()) );
		koord3d pos = fahr[0]->get_pos();
		grund_t *gr=welt->lookup(pos);
		// start on bridge?
		pos.z += gr->get_weg_yoff() / TILE_HEIGHT_STEP;
		do {
			// advance one station tile
			station_length += CARUNITS_PER_TILE;

			while(station_length >= 0) {
				vehicles_loading++;
				if (vehicles_loading < vehicle_count) {
					station_length -= fahr[vehicles_loading]->get_desc()->get_length();
				}
				else {
					// all vehicles fit into station
					goto station_tile_search_ready;
				}
			}

			// search for next station tile
			pos += zv;
			gr = welt->lookup(pos);
			if (gr == NULL) {
				gr = welt->lookup(pos-koord3d(0,0,1));
				if (gr == NULL) {
					gr = welt->lookup(pos-koord3d(0,0,2));
				}
				if (gr  &&  (pos.z != gr->get_hoehe() + gr->get_weg_yoff()/TILE_HEIGHT_STEP) ) {
					// not end/start of bridge
					break;
				}
			}

		}  while(  gr  &&  gr->get_halt() == halt  );
		// finished
station_tile_search_ready: ;
	}

	// next stop in schedule will be a depot
	bool next_depot = false;
	bool has_pax=goods_catg_index.is_contained( 0 );
	bool has_mail=goods_catg_index.is_contained( 1 );
	bool has_goods=goods_catg_index.get_count()>(has_pax+has_mail);

	// prepare a list of all destination halts in the schedule
	vector_tpl<halthandle_t> destination_halts(schedule->get_count());
	if (!no_load) {
		const uint8 count = schedule->get_count();
		for(  uint8 i=1;  i<count;  i++  ) {
			const uint8 wrap_i = (i + schedule->get_current_stop()) % count;

			const halthandle_t plan_halt = haltestelle_t::get_halt(schedule->entries[wrap_i].pos, owner);
			if(plan_halt == halt) {
				// we will come later here again ...
				break;
			}
			else if(  !plan_halt.is_bound()  ) {
				if(  grund_t *gr = welt->lookup( schedule->entries[wrap_i].pos )  ) {
					if(  gr->get_depot()  ) {

						next_depot = i==1;
						// do not load for stops after a depot
						break;
					}
				}
				continue;
			}
			for( uint8 const idx : goods_catg_index ) {
				if(  !halt->get_halt_served_this_step()[idx].is_contained(plan_halt)  ) {
					// not somebody loaded before us in the queue
					destination_halts.append(plan_halt);
				}
			}
		}
	}

	// only load vehicles in station
	// don't load when vehicle is being withdrawn
	bool changed_loading_level = false;
	unloading_state = true;
	uint32 time = 0; // min time for loading/unloading

	uint32 current_tick = welt->get_ticks();
	if (last_load_tick==0) {
		// first stop here, so no time passed yet
		last_load_tick = current_tick;
	}
	const uint32 loading_ms = current_tick - last_load_tick;
	last_load_tick = current_tick;

	// cargo type of previous vehicle that could not be filled
	const goods_desc_t* cargo_type_prev = NULL;
	bool wants_more = false;

	for(unsigned i=0; i<vehicles_loading; i++) {
		vehicle_t* v = fahr[i];

		// has no freight ...
		if (fahr[i]->get_cargo_max() == 0) {
			continue;
		}

		uint32 min_loading_step_time = v->get_desc()->get_loading_time() / v->get_cargo_max() + 1;
		time = max(time, min_loading_step_time);
		uint16 max_amount = next_depot ? 32767 : (v->get_cargo_max() * loading_ms) / ((uint32)v->get_desc()->get_loading_time() + 1) + 1;
		uint16 amount = v->unload_cargo(halt, next_depot, max_amount );

		if( amount > 0 ) {
			// could unload something, so we are open for new stuff
			cargo_type_prev = NULL;
		}

		if(  max_amount > amount  &&  !no_load  &&  !next_depot  &&  v->get_total_cargo() < v->get_cargo_max()  ) {
			// load if: not unloaded something first or not filled and not forbidden (no_load or next is depot)
			if(cargo_type_prev==NULL  ||  !cargo_type_prev->is_interchangeable(v->get_cargo_type())) {
				// load
				amount += v->load_cargo(halt, destination_halts, max_amount-amount);
				unloading_state &= amount==0;
			}
			if (v->get_total_cargo() < v->get_cargo_max()  &&  amount < max_amount  ) {
				// not full, but running out of goods
				cargo_type_prev = v->get_cargo_type();
			}
		}

		if(  amount  ) {
			v->mark_image_dirty(v->get_image(), 0);
			v->calc_image();
			changed_loading_level = true;
		}

		wants_more |= (amount == max_amount)  &&  (v->get_total_cargo() < v->get_cargo_max());
	}
	freight_info_resort |= changed_loading_level;
	if(  changed_loading_level  ) {
		halt->recalc_status();
	}

	// any unloading/loading went on?
	if(  changed_loading_level  ) {
		calc_loading();
	}
	else {
		// nothing to unload or load: waiting for cargo
		unloading_state = false;
	}
	loading_limit = schedule->get_current_entry().minimum_loading;

	assert(time > 0  ||  !wants_more);
	// time == 0 => wants_more == false, can only happen if all vehicles in station have no transport capacity (i.e., locomotives)
	wait_lock = time;

	// check if departure time is reached
	bool departure_time_reached = false;
	if (schedule->get_current_entry().get_absolute_departures()) {

		sint32 ticks_until_departure = get_departure_ticks() - welt->get_ticks();
		if(  ticks_until_departure <= 0  ) {
			// depart if nothing to load (wants_more==false)
			departure_time_reached = !wants_more;
		}
		else {
			// else continue loading (even if full) until departure time reached
			if (wait_lock==0) {
				// no loading time imposed, make sure we do not wait too long if the departure is imminent
				wait_lock = min( WTT_LOADING, ticks_until_departure );
			}
			wants_more = true;
		}
	}

	// continue loading
	if (wants_more) {
		//  there might be still cargo available (or cnv has to wait until departure)
		return;
	}

	// loading is finished => maybe drive on
	if(  loading_level >= loading_limit  ||  no_load
		||  departure_time_reached
		||  (schedule->get_current_entry().waiting_time > 0  &&  (welt->get_ticks() - arrived_time) > schedule->get_current_entry().get_waiting_ticks()  )  ) {

		if(  withdraw  &&  (loading_level == 0  ||  goods_catg_index.empty())  ) {
			// destroy when empty
			self_destruct();
			return;
		}

		calc_speedbonus_kmh();

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<vehicle_count; i++) {
			book(get_vehicle(i)->get_cargo_max()-get_vehicle(i)->get_total_cargo(), CONVOI_CAPACITY);
		}

		// Advance schedule
		schedule->advance();
		state = ROUTING_1;
		loading_limit = 0;
	}

	INT_CHECK("convoi_t::hat_gehalten");
}


sint64 convoi_t::calc_restwert() const
{
	sint64 result = 0;

	for(uint i=0; i<vehicle_count; i++) {
		result += fahr[i]->calc_sale_value();
	}
	return result;
}


/**
 * Calculate loading_level and loading_limit. This depends on current state (loading or not).
 */
void convoi_t::calc_loading()
{
	int fracht_max = 0;
	int fracht_menge = 0;
	for(unsigned i=0; i<vehicle_count; i++) {
		const vehicle_t* v = fahr[i];
		fracht_max += v->get_cargo_max();
		fracht_menge += v->get_total_cargo();
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
	loading_limit = 0; // will be set correctly from hat_gehalten() routine

	// since weight has changed
	recalc_data=true;
}


void convoi_t::calc_speedbonus_kmh()
{
	// init with default
	const sint32 cnv_min_top_kmh = speed_to_kmh( min_top_speed );
	speedbonus_kmh = cnv_min_top_kmh;
	// flying aircraft have 0 friction --> speed not limited by power, so just use top_speed
	if(  front()!=NULL  &&  front()->get_waytype() != air_wt  ) {
		sint32 total_max_weight = 0;
		sint32 total_weight = 0;
		for(  unsigned i=0;  i<vehicle_count;  i++  ) {
			const vehicle_desc_t* const desc = fahr[i]->get_desc();
			total_max_weight += desc->get_weight();
			total_weight += fahr[i]->get_total_weight(); // convoi_t::sum_gesamweight may not be updated yet when this method is called...
			if(  desc->get_freight_type() == goods_manager_t::none  ) {
				; // nothing
			}
			else if(  desc->get_freight_type()->get_catg() == 0  ) {
				// use full weight for passengers, mail, and special goods
				total_max_weight += desc->get_freight_type()->get_weight_per_unit() * desc->get_capacity();
			}
			else {
				// use actual weight for regular goods
				total_max_weight += fahr[i]->get_cargo_weight();
			}
		}
		// very old vehicles have zero weight ...
		if(  total_weight>0  ) {

			speedbonus_kmh = speed_to_kmh( calc_max_speed(sum_gear_and_power, total_max_weight, min_top_speed) );

			// convoi overtakers use current actual weight for achievable speed
			if(  front()->get_overtaker()  ) {
				max_power_speed = calc_max_speed(sum_gear_and_power, total_weight, min_top_speed);
			}
		}
	}
}


// return the current bonus speed
sint32 convoi_t::get_speedbonus_kmh() const
{
	if(  distance_since_last_stop > 0  &&  front()!=NULL  &&  front()->get_waytype() != air_wt  ) {
		return min( speedbonus_kmh, (sint32)(sum_speed_limit / distance_since_last_stop) );
	}
	return speedbonus_kmh;
}


// return the current bonus speed
uint32 convoi_t::get_average_kmh() const
{
	if(  distance_since_last_stop > 0  ) {
		return sum_speed_limit / distance_since_last_stop;
	}
	return speedbonus_kmh;
}


/**
 * Schedule convois for self destruction. Will be executed
 * upon next sync step
 */
void convoi_t::self_destruct()
{
	line_update_pending = linehandle_t(); // does not bother to add it to a new line anyway ...
	// convois in depot are not contained in the map array!
	if(state==INITIAL) {
		destroy();
	}
	else {
		state = SELF_DESTRUCT;
		wait_lock = 0;
	}
}


/**
 * Helper method to remove convois from the map that cannot
 * removed normally (i.e. by sending to a depot) anymore.
 * This is a workaround for bugs in the game.
 */
void convoi_t::destroy()
{
	// can be only done here, with a valid convoihandle ...
	if(fahr[0]) {
		fahr[0]->set_convoi(NULL);
	}

	if(  state == INITIAL  ) {
		// in depot => not on map
		for(  uint8 i = vehicle_count;  i-- != 0;  ) {
			fahr[i]->set_flag( obj_t::not_on_map );
		}
	}
	state = SELF_DESTRUCT;

	if(schedule!=NULL  &&  !schedule->is_editing_finished()) {
		destroy_win((ptrdiff_t)schedule);
	}

	if(  line.is_bound()  ) {
		// needs to be done here to remove correctly ware catg from lines
		unset_line();
		delete schedule;
		schedule = NULL;
	}

	// pay the current value
	owner->book_new_vehicle( calc_restwert(), get_pos().get_2d(), fahr[0] ? fahr[0]->get_desc()->get_waytype() : ignore_wt );

	for(  uint8 i = vehicle_count;  i-- != 0;  ) {
		if(  !fahr[i]->get_flag( obj_t::not_on_map )  ) {
			// remove from rails/roads/crossings
			grund_t *gr = welt->lookup(fahr[i]->get_pos());
			fahr[i]->set_last( true );
			fahr[i]->leave_tile();
			if(  gr  &&  gr->ist_uebergang()  ) {
				gr->find<crossing_t>()->release_crossing(fahr[i]);
			}
			fahr[i]->set_flag( obj_t::not_on_map );

		}
		// no need to substract maintenance, since it is booked as running costs

		fahr[i]->discard_cargo();
		fahr[i]->cleanup(owner);
		delete fahr[i];
	}
	vehicle_count = 0;

	delete this;
}


/**
 * Debug info nach stderr
 */
void convoi_t::dump() const
{
	dbg->debug("convoi::dump()",
		"\nvehicle_count = %d\n"
		"wait_lock = %d\n"
		"owner_n = %d\n"
		"akt_speed = %d\n"
		"akt_speed_soll = %d\n"
		"sp_soll = %d\n"
		"state = %d\n"
		"statename = %s\n"
		"alte_richtung = %d\n"
		"jahresgewinn = %ld\n" // %lld crashes mingw now, cast gewinn to long ...
		"name = '%s'\n"
		"line_id = '%d'\n"
		"schedule = '%p'",
		(int)vehicle_count,
		(int)wait_lock,
		(int)welt->sp2num(owner),
		(int)akt_speed,
		(int)akt_speed_soll,
		(int)sp_soll,
		(int)state,
		(const char *)(state_names[state]),
		(int)alte_richtung,
		(long)(jahresgewinn/100),
		(const char *)name_and_id,
		line.is_bound() ? line.get_id() : 0,
		(const void *)schedule );
}


void convoi_t::book(sint64 amount, int cost_type)
{
	assert(  cost_type<MAX_CONVOI_COST);

	financial_history[0][cost_type] += amount;
	if (line.is_bound()) {
		line->book( amount, simline_t::convoi_to_line_catgory(cost_type) );
	}
}


void convoi_t::init_financial_history()
{
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][j] = 0;
		}
	}
}


sint64 convoi_t::get_purchase_cost() const
{
	sint64 purchase_cost = 0;
	for(  unsigned i = 0;  i < get_vehicle_count();  i++  ) {
		purchase_cost += fahr[i]->get_desc()->get_price();
	}
	return purchase_cost;
}


/**
* set line
* since convoys must operate on a copy of the route's schedule, we apply a fresh copy
*/
void convoi_t::set_line(linehandle_t org_line)
{
	// to remove a convoi from a line, call unset_line()
	if(!org_line.is_bound()) {
		unset_line();
	}
	if(  line.is_bound()  ) {
		unset_line();
	}
	else {
		// originally a lineless convoy -> unregister itself from stops as it now belongs to a line
		unregister_stops();
		// must trigger refresh if old schedule was not empty
		if (schedule  &&  !schedule->empty()) {
			welt->set_schedule_counter();
		}
	}
	line_update_pending = org_line;
	check_pending_updates();
}


/**
* unset line
* removes convoy from route without destroying its schedule
* => no need to recalculate connections!
*/
void convoi_t::unset_line()
{
	if(  line.is_bound()  ) {
DBG_DEBUG("convoi_t::unset_line()", "removing old destinations from line=%d, schedule=%p",line.get_id(),schedule);
		line->remove_convoy(self);
		line = linehandle_t();
		line_update_pending = linehandle_t();
	}
}


// matches two halts; if the pos is not identical, maybe the halt still is the same
bool convoi_t::matches_halt( const koord3d pos1, const koord3d pos2 )
{
	halthandle_t halt1 = haltestelle_t::get_halt(pos1, owner );
	return pos1==pos2  ||  (halt1.is_bound()  &&  halt1==haltestelle_t::get_halt( pos2, owner ));
}


// updates a line schedule and tries to find the best next station to go
void convoi_t::check_pending_updates()
{
	if(  line_update_pending.is_bound()  ) {
		// create dummy schedule
		if(  schedule==NULL  ) {
			schedule = create_schedule();
		}
		schedule_t* new_schedule = line_update_pending->get_schedule();
		int current_stop = schedule->get_current_stop(); // save current position of schedule
		bool is_same = false;
		bool is_depot = false;
		koord3d current = koord3d::invalid, depot = koord3d::invalid;

		if (schedule->empty() || new_schedule->empty()) {
			// was no entry or is no entry => goto  1st stop
			current_stop = 0;
		}
		else {
			// something to check for ...
			current = schedule->get_current_entry().pos;

			if(  current_stop<new_schedule->get_count() &&  current==new_schedule->entries[current_stop].pos  ) {
				// next pos is the same => keep the convoi state
				is_same = true;
			}
			else {
				// check depot first (must also keept this state)
				is_depot = (welt->lookup(current)  &&  welt->lookup(current)->get_depot() != NULL);

				if(is_depot) {
					// depot => current_stop+1 (depot will be restore later before this)
					depot = current;
					schedule->remove();
					current = schedule->get_current_entry().pos;
				}

				/* there could be only one entry that matches best:
				 * we try first same sequence as in old schedule;
				 * if not found, we try for same nextnext station
				 * (To detect also places, where only the platform
				 *  changed, we also compare the halthandle)
				 */
				const koord3d next = schedule->entries[(current_stop+1)%schedule->get_count()].pos;
				const koord3d nextnext = schedule->entries[(current_stop+2)%schedule->get_count()].pos;
				const koord3d nextnextnext = schedule->entries[(current_stop+3)%schedule->get_count()].pos;
				int how_good_matching = 0;
				const uint8 new_count = new_schedule->get_count();

				for(  uint8 i=0;  i<new_count;  i++  ) {
					int quality =
						matches_halt(current,new_schedule->entries[i].pos)*3 +
						matches_halt(next,new_schedule->entries[(i+1)%new_count].pos)*4 +
						matches_halt(nextnext,new_schedule->entries[(i+2)%new_count].pos)*2 +
						matches_halt(nextnextnext,new_schedule->entries[(i+3)%new_count].pos);
					if(  quality>how_good_matching  ) {
						// better match than previous: but depending of distance, the next number will be different
						if(  matches_halt(current,new_schedule->entries[i].pos)  ) {
							current_stop = i;
						}
						else if(  matches_halt(next,new_schedule->entries[(i+1)%new_count].pos)  ) {
							current_stop = i+1;
						}
						else if(  matches_halt(nextnext,new_schedule->entries[(i+2)%new_count].pos)  ) {
							current_stop = i+2;
						}
						else if(  matches_halt(nextnextnext,new_schedule->entries[(i+3)%new_count].pos)  ) {
							current_stop = i+3;
						}
						current_stop %= new_count;
						how_good_matching = quality;
					}
				}

				if(how_good_matching==0) {
					// nothing matches => take the one from the line
					current_stop = new_schedule->get_current_stop();
				}
				// if we go to same, then we do not need route recalculation ...
				is_same = matches_halt(current,new_schedule->entries[current_stop].pos);
			}
		}

		// we may need to update the line and connection tables
		if(  !line.is_bound()  ) {
			line_update_pending->add_convoy(self);
		}
		line = line_update_pending;
		line_update_pending = linehandle_t();

		// destroy old schedule and all related windows
		if(!schedule->is_editing_finished()) {
			schedule->copy_from( new_schedule );
			schedule->set_current_stop(current_stop); // set new schedule current position to best match
			schedule->start_editing();
		}
		else {
			schedule->copy_from( new_schedule );
			schedule->set_current_stop(current_stop); // set new schedule current position to one before best match
		}

		if(is_depot) {
			// next was depot. restore it
			schedule->insert(welt->lookup(depot));
			schedule->set_current_stop( (schedule->get_current_stop()+schedule->get_count()-1)%schedule->get_count() );
		}

		if (state != INITIAL) {
			// remove wrong freight
			check_freight();

			if(is_same  ||  is_depot) {
				/* same destination
				 * We are already there => keep current state
				 */
			}
			else {
				// need re-routing
				state = EDIT_SCHEDULE;
			}
			// make this change immediately
			if(  state!=LOADING  ) {
				wait_lock = 0;
			}
		}
	}
}


/**
 * Register the convoy with the stops in the schedule
 */
void convoi_t::register_stops()
{
	if(  schedule  ) {
		for(schedule_entry_t const& i : schedule->entries) {
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, get_owner());
			if(  halt.is_bound()  ) {
				halt->add_convoy(self);
			}
		}
	}
}


/**
 * Unregister the convoy from the stops in the schedule
 */
void convoi_t::unregister_stops()
{
	if(  schedule  ) {
		for(schedule_entry_t const& i : schedule->entries) {
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, get_owner());
			if(  halt.is_bound()  ) {
				halt->remove_convoy(self);
			}
		}
	}
}


// set next stop before breaking will occur (or route search etc.)
// currently only used for tracks
void convoi_t::set_next_stop_index(route_t::index_t n)
{
	// stop at station or signals, not at waypoints
	if(  n==route_t::INVALID_INDEX  ) {
		// find out if stop or waypoint, waypoint: do not brake at waypoints
		grund_t const* const gr = welt->lookup(route.back());
		if(  gr  &&  gr->is_halt()  ) {
			n = route.get_count()-1-1; // extra -1 to brake 1 tile earlier when entering station
		}
	}
	next_stop_index = n+1;
}


/* including this route_index, the route was reserved the last time
 * currently only used for tracks
 */
void convoi_t::set_next_reservation_index(route_t::index_t n)
{
	// stop at station or signals, not at waypoints
	if(  n==route_t::INVALID_INDEX  ) {
		n = route.get_count()-1;
	}
	next_reservation_index = n;
}


/*
 * the current state saved as color
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
 */
PIXVAL convoi_t::get_status_color() const
{
	if(state==INITIAL) {
		// in depot/under assembly
		return SYSCOL_TEXT_HIGHLIGHT;
	}
	else if (state == WAITING_FOR_CLEARANCE_ONE_MONTH || state == CAN_START_ONE_MONTH || get_state() == NO_ROUTE) {
		// stuck or no route
		return color_idx_to_rgb(COL_ORANGE);
	}
	else if(financial_history[0][CONVOI_PROFIT]+financial_history[1][CONVOI_PROFIT]<0) {
		// ok, not performing best
		return MONEY_MINUS;
	}
	else if((financial_history[0][CONVOI_OPERATIONS]|financial_history[1][CONVOI_OPERATIONS])==0) {
		// nothing moved
		return SYSCOL_TEXT_UNUSED;
	}
	else if(has_obsolete) {
		return SYSCOL_OBSOLETE;
	}
	// normal state
	return SYSCOL_TEXT;
}


// returns station tiles needed for this convoi
uint16 convoi_t::get_tile_length() const
{
	uint16 carunits=0;
	for(uint8 i=0;  i<vehicle_count-1;  i++) {
		carunits += fahr[i]->get_desc()->get_length();
	}
	// the last vehicle counts differently in stations and for reserving track
	// (1) add 8 = 127/256 tile to account for the driving in stations in north/west direction
	//     see at the end of vehicle_t::hop()
//	carunits += max(CARUNITS_PER_TILE/2, fahr[vehicle_count-1]->get_desc()->get_length());
	// (2) for length of convoi for loading in stations the length of the last vehicle matters
	//     see convoi_t::hat_gehalten
	carunits += fahr[vehicle_count-1]->get_desc()->get_length();

	uint16 tiles = (carunits + CARUNITS_PER_TILE - 1) / CARUNITS_PER_TILE;
	return tiles;
}


// if withdraw and empty, then self destruct
void convoi_t::set_withdraw(bool new_withdraw)
{
	withdraw = new_withdraw;
	if(  withdraw  &&  (loading_level==0  ||  goods_catg_index.empty())) {
		// test if convoi in depot and not driving
		grund_t *gr = welt->lookup( get_pos());
		if(  gr  &&  gr->get_depot()  &&  state == INITIAL  ) {
#if 1
			// do not touch line bound convois in depots
			withdraw = false;
			no_load = false;
#else
			// disassemble also line bound convois in depots
			gr->get_depot()->disassemble_convoi(self, true);
#endif
		}
		else {
			self_destruct();
		}
	}
}


/**
 * conditions for a city car to overtake another overtaker.
 * The city car is not overtaking/being overtaken.
 */
bool convoi_t::can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other)
{
	if(fahr[0]->get_waytype()!=road_wt) {
		return false;
	}

	if (!other_overtaker->can_be_overtaken()) {
		return false;
	}

	if(  other_speed == 0  ) {
		/* overtaking a loading convoi
		 * => we can do a lazy check, since halts are always straight
		 */
		grund_t *gr = welt->lookup(get_pos());
		if(  gr==NULL  ) {
			// should never happen, since there is a vehicle in front of us ...
			return false;
		}
		weg_t *str = gr->get_weg(road_wt);
		if(  str==0  ) {
			// also this is not possible, since a car loads in front of is!?!
			return false;
		}

		route_t::index_t idx = fahr[0]->get_route_index();
		const sint32 tiles = other_speed == 0 ? 2 : (steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) + get_tile_length() + 1;
		if(  tiles > 0  &&  idx+(uint32)tiles >= route.get_count()  ) {
			// needs more space than there
			return false;
		}

		for(  sint32 i=0;  i<tiles;  i++  ) {
			grund_t *gr = welt->lookup( route.at( idx+i ) );
			if(  gr==NULL  ) {
				return false;
			}
			weg_t *str = gr->get_weg(road_wt);
			if(  str==0  ) {
				return false;
			}
			// not overtaking on railroad crossings or normal crossings ...
			if(  str->is_crossing() ) {
				return false;
			}
			if(  ribi_t::is_threeway(str->get_ribi())  ) {
				return false;
			}
			// Check for other vehicles on the next tile
			const uint8 top = gr->get_top();
			for(  uint8 j=1;  j<top;  j++  ) {
				if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j))  ) {
					// check for other traffic on the road
					const overtaker_t *ov = v->get_overtaker();
					if(ov) {
						if(this!=ov  &&  other_overtaker!=ov) {
							return false;
						}
					}
					else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
						return false;
					}
				}
			}
		}
		set_tiles_overtaking( tiles );
		return true;
	}

	int diff_speed = akt_speed - other_speed;
	if(  diff_speed < kmh_to_speed(5)  ) {
		return false;
	}

	// Number of tiles overtaking will take
	int n_tiles = 0;

	// Distance it takes overtaking (unit: vehicle_steps) = my_speed * time_overtaking
	// time_overtaking = tiles_to_overtake/diff_speed
	// tiles_to_overtake = convoi_length + current pos within tile + (pos_other_convoi within tile + length of other convoi) - one tile
	sint32 distance = akt_speed*(fahr[0]->get_steps()+get_length_in_steps()+steps_other-VEHICLE_STEPS_PER_TILE)/diff_speed;
	sint32 time_overtaking = 0;

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	// First phase: no traffic except me and my overtaken car in the dangerous zone
	route_t::index_t route_index = fahr[0]->get_route_index()+1;
	koord3d pos = fahr[0]->get_pos();
	koord3d pos_prev = route_index > 2 ? route.at(route_index-2) : pos;
	koord3d pos_next;

	while( distance > 0 ) {

		if(  route_index >= route.get_count()  ) {
			return false;
		}

		pos_next = route.at(route_index++);
		grund_t *gr = welt->lookup(pos);
		// no ground, or slope => about
		if(  gr==NULL  ||  gr->get_weg_hang()!=slope_t::flat  ) {
			return false;
		}

		weg_t *str = gr->get_weg(road_wt);
		if(  str==NULL  ) {
			return false;
		}
		// the only roadsign we must account for are choose points and traffic lights
		if(  str->has_sign()  ) {
			const roadsign_t *rs = gr->find<roadsign_t>(1);
			if(rs) {
				const roadsign_desc_t *rb = rs->get_desc();
				if(rb->is_choose_sign()  ||  rb->is_traffic_light()  ) {
					// because we may need to stop here ...
					return false;
				}
			}
		}
		// not overtaking on railroad crossings or on normal crossings ...
		if(  str->is_crossing()  ||  ribi_t::is_threeway(str->get_ribi())  ) {
			return false;
		}
		// street gets too slow (TODO: should be able to be correctly accounted for)
		if(  akt_speed > kmh_to_speed(str->get_max_speed())  ) {
			return false;
		}

		int d = ribi_t::is_straight(str->get_ribi()) ? VEHICLE_STEPS_PER_TILE : vehicle_base_t::get_diagonal_vehicle_steps_per_tile();
		distance -= d;
		time_overtaking += d;

		// Check for other vehicles
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			if (vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j))) {
				// check for other traffic on the road
				const overtaker_t *ov = v->get_overtaker();
				if(ov) {
					if(this!=ov  &&  other_overtaker!=ov) {
						return false;
					}
				}
				else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
					// sheeps etc.
					return false;
				}
			}
		}
		n_tiles++;
		pos_prev = pos;
		pos = pos_next;
	}

	// Second phase: only facing traffic is forbidden

	// the original routine was checking using the maximum road speed.
	// However, we can tolerate slower vehicles if they are closer

	// Furthermore, if we reach the end of the route for a vehcile as fast as us,
	// we simply assume it to be ok too
	sint32 overtaking_distance = time_overtaking;
	distance = 0; // distance to needed traveled to crash int us from this point
	time_overtaking = (time_overtaking << 16)/akt_speed;
	while(  time_overtaking > 0  ) {

		if(  route_index >= route.get_count()  ) {
			return distance>=time_overtaking; // we assume ok, if there is enough distance when we would face ourselves
		}

		pos_next = route.at(route_index++);
		grund_t *gr= welt->lookup(pos);
		if(  gr==NULL  ) {
			// will cause a route search, but is ok
			break;
		}

		weg_t *str = gr->get_weg(road_wt);
		if(  str==NULL  ) {
			break;
		}
		// cannot check for oncoming traffic over crossings
		if(  ribi_t::is_threeway(str->get_ribi()) ) {
			return false;
		}

		if(  ribi_t::is_straight(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / kmh_to_speed(str->get_max_speed());
			distance -= VEHICLE_STEPS_PER_TILE;
		}
		else {
			time_overtaking -= (vehicle_base_t::get_diagonal_vehicle_steps_per_tile()<<16) / kmh_to_speed(str->get_max_speed());
			distance -= vehicle_base_t::get_diagonal_vehicle_steps_per_tile();
		}

		// Check for other vehicles in facing direction
		ribi_t::ribi their_direction = ribi_t::backward( fahr[0]->calc_direction(pos_prev, pos_next) );
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j));
			if(  v  &&  v->get_direction() == their_direction  &&  v->get_overtaker()  ) {
				// tolerated distance us>them: total_distance*akt_speed > current_distance*other_speed
				if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
					convoi_t* const ocnv = car->get_convoi();
					if(  ocnv  &&  ocnv->get_max_power_speed()*distance > akt_speed*overtaking_distance  ) {
						return false;
					}
				}
				else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
					if(  caut->get_desc()->get_topspeed()*distance > akt_speed*overtaking_distance  ) {
						return false;
					}
				}
			}
		}
		pos_prev = pos;
		pos = pos_next;
	}

	set_tiles_overtaking( 1+n_tiles );
	other_overtaker->set_tiles_overtaking( -1-(n_tiles*(akt_speed-diff_speed))/akt_speed );
	return true;
}


sint64 convoi_t::get_stat_converted(int month, int cost_type) const
{
	sint64 value = financial_history[month][cost_type];
	switch(cost_type) {
		case CONVOI_REVENUE:
		case CONVOI_OPERATIONS:
		case CONVOI_PROFIT:
		case CONVOI_WAYTOLL:
			value = convert_money(value);
			break;
		default: ;
	}
	return value;
}


const char* convoi_t::send_to_depot(bool local)
{
	// iterate over all depots and try to find shortest route
	route_t *shortest_route = new route_t();
	route_t *route = new route_t();
	koord3d home = koord3d::invalid;
	vehicle_t *v = front();
	for(depot_t* const depot : depot_t::get_depot_list()) {
		if (depot->get_waytype() != v->get_desc()->get_waytype()  ||  depot->get_owner() != get_owner()) {
			continue;
		}
		koord3d pos = depot->get_pos();

		if(!shortest_route->empty()  &&  koord_distance(pos, get_pos()) >= shortest_route->get_count()-1) {
			// the current route is already shorter, no need to search further
			continue;
		}
		if (v->calc_route(get_pos(), pos, 50, route)) { // do not care about speed
			if(  route->get_count() < shortest_route->get_count()  ||  shortest_route->empty()  ) {
				// just swap the pointers
				sim::swap(shortest_route, route);
				home = pos;
			}
		}
	}
	delete route;
	DBG_MESSAGE("shortest route has ", "%i hops", shortest_route->get_count()-1);

	if (local) {
		if (convoi_info_t *info = dynamic_cast<convoi_info_t*>(win_get_magic( magic_convoi_info+self.get_id()))) {
			info->route_search_finished();
		}
	}
	// if route to a depot has been found, update the convoi's schedule
	const char *txt;
	if(  !shortest_route->empty()  ) {
		schedule_t *schedule = get_schedule()->copy();
		schedule->insert(welt->lookup(home));
		schedule->set_current_stop( (schedule->get_current_stop()+schedule->get_count()-1)%schedule->get_count() );
		set_schedule(schedule);
		txt = "Convoi has been sent\nto the nearest depot\nof appropriate type.\n";
	}
	else {
		txt = "Home depot not found!\nYou need to send the\nconvoi to the depot\nmanually.";
	}
	delete shortest_route;

	return txt;
}
