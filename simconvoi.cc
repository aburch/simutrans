/**
 * convoi_t Class for vehicle associations
 * Hansjörg Malthaner
 */

#include <stdlib.h>
#include <math.h>

#include "simdebug.h"
#include "simunits.h"
#include "simworld.h"
#include "simware.h"
#include "player/finance.h" // convert_money
#include "player/simplay.h"
#include "simconvoi.h"
#include "simhalt.h"
#include "simdepot.h"
#include "gui/simwin.h"
#include "simmenu.h"
#include "simcolor.h"
#include "simmesg.h"
#include "simintr.h"
#include "simlinemgmt.h"
#include "simline.h"
#include "freight_list_sorter.h"

#include "gui/karte.h"
#include "gui/convoi_info_t.h"
#include "gui/schedule_gui.h"
#include "gui/depot_frame.h"
#include "gui/messagebox.h"
#include "gui/convoi_detail_t.h"
#include "boden/grund.h"
#include "boden/wege/schiene.h"	// for railblocks
#include "boden/wege/strasse.h"

#include "descriptor/vehicle_desc.h"
#include "descriptor/roadsign_desc.h"

#include "dataobj/schedule.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"

#include "display/viewport.h"

#include "obj/crossing.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"

#include "vehicle/simvehicle.h"
#include "vehicle/overtaker.h"

#include "utils/simrandom.h"
#include "utils/simstring.h"
#include "utils/cbuffer_t.h"


/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 2000


karte_ptr_t convoi_t::welt;

/*
 * Debugging helper - translate state value to human readable name
 * @author Hj- Malthaner
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
	"SELF_DESTRUCT",	// self destruct
	"WAITING_FOR_CLEARANCE_TWO_MONTHS",
	"CAN_START_TWO_MONTHS",
	"LEAVING_DEPOT",
	"ENTERING_DEPOT",
	"COUPLED",
	"COUPLED_LOADING"
};


void convoi_t::init(player_t *player)
{
	owner = player;

	is_electric = false;
	sum_gesamtweight = sum_weight = 0;
	sum_running_costs = sum_gear_and_power = previous_delta_v = 0;
	sum_power = 0;
	min_top_speed = SPEED_UNLIMITED;
	speedbonus_kmh = SPEED_UNLIMITED; // speed_to_kmh() not needed

	schedule = NULL;
	schedule_target = koord3d::invalid;
	line = linehandle_t();

	anz_vehikel = 0;
	steps_driven = -1;
	withdraw = false;
	has_obsolete = false;
	no_load = false;
	wait_lock = 0;
	arrived_time = 0;

	requested_change_lane = false;

	jahresgewinn = 0;
	total_distance_traveled = 0;

	distance_since_last_stop = 0;
	sum_speed_limit = 0;
	maxspeed_average_count = 0;
	next_reservation_index = 0;
	reserved_tiles.clear();

	alte_richtung = ribi_t::none;
	next_wolke = 0;

	state = INITIAL;

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
	next_coupling_index = INVALID_INDEX;
	next_coupling_steps = 0;
	
	coupling_done = false;
	next_initial_direction = ribi_t::none;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;
	yielding_quit_index = -1;
	lane_affinity = 0;

	recalc_data_front = true;
	recalc_data = true;

	next_cross_lane = false;
	request_cross_ticks = 0;
	prev_tiles_overtaking = 0;
	
	longblock_signal_request.valid = false;
	crossing_reservation_index.clear();
	recalc_min_top_speed = true;
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
	assert(anz_vehikel==0);

	// close windows
	destroy_win( magic_convoi_info+self.get_id() );

DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_viewport()->get_follow_convoi()==self) {
		welt->get_viewport()->set_follow_convoi( convoihandle_t() );
	}

	welt->sync.remove( this );
	welt->rem_convoi( self );

	// Knightly : if lineless convoy -> unregister from stops
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

	// @author hsiegeln - deregister from line (again) ...
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
	if(  !route.empty()  &&  anz_vehikel>0  &&  fahr[0]->get_convoi() == this  ) {
		rail_vehicle_t* lok = dynamic_cast<rail_vehicle_t*>(fahr[0]);
		if (lok) {
			// free all reserved blocks
			uint16 dummy;
			lok->block_reserver(get_route(), back()->get_route_index(), dummy, dummy,  100000, false, true);
			// free all tiles held by reserved_tiles
			if(  reserved_tiles.get_count()>0  ) {
				vector_tpl<koord3d> tiles_convoy_on;
				for(  uint16 i=0;  i<anz_vehikel;  i++  ) {
					tiles_convoy_on.append_unique(fahr[i]->get_pos());
				}
				for(  uint32 i=0;  i<reserved_tiles.get_count();  i++  ) {
					grund_t* gr = welt->lookup(reserved_tiles[i]);
					schiene_t *sch = gr ? (schiene_t *)gr->get_weg( front()->get_waytype() ) : NULL;
					if(  sch  &&  !tiles_convoy_on.is_contained(reserved_tiles[i])  ) {
						sch->unreserve(this->self);
					}
				}
				reserved_tiles.clear();
			}
		}
	}
}


/**
 * reserves route until next_reservation_index
 */
void convoi_t::reserve_route()
{
	if(  reserved_tiles.get_count()>0  ) {
		// reservation is controlled by reserved_tiles
		for(  uint32 idx = 0;  idx < reserved_tiles.get_count();  idx++  ) {
			if(  grund_t *gr = welt->lookup( reserved_tiles[idx] )  ) {
				if(  schiene_t *sch = (schiene_t *)gr->get_weg( front()->get_waytype() )  ) {
					sch->reserve( self, ribi_type( reserved_tiles[max(1u,idx)-1u], reserved_tiles[min(reserved_tiles.get_count()-1u,idx+1u)] ) );
				}
			}
		}
	}
	else if(  !route.empty()  &&  anz_vehikel>0  &&  (is_waiting()  ||  state==DRIVING  ||  state==LEAVING_DEPOT)  ){
		// reservation is controlled by next_reservation_index
		for(  int idx = back()->get_route_index();  idx < next_reservation_index  /*&&  idx < route.get_count()*/;  idx++  ) {
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
uint32 convoi_t::move_to(uint16 const start_index)
{
	steps_driven = -1;
	koord3d k = route.at(start_index);
	grund_t* gr = welt->lookup(k);

	uint32 train_length = 0;
	for (unsigned i = 0; i != anz_vehikel; ++i) {
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

		if (i != anz_vehikel - 1U) {
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
				for( uint8 i=0;  i<anz_vehikel;  i++ ) {
					fahr[i]->set_pos(home_depot);
				}
				state = INITIAL;
			}
			else {
				dbg->error( "convoi_t::finish_rd()","No schedule during loading convoi %i: Convoi will be destroyed!", self.get_id() );
				for( uint8 i=0;  i<anz_vehikel;  i++ ) {
					fahr[i]->set_pos(koord3d::invalid);
				}
				destroy();
				return;
			}
		}
		// anyway reassign convoi pointer ...
		for( uint8 i=0;  i<anz_vehikel;  i++ ) {
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
		if(  anz_vehikel>0  &&  is_waypoint(ziel)  ) {
			schedule_target = ziel;
		}
	}

	bool realign_position = false;
	if(  anz_vehikel>0  ) {
DBG_MESSAGE("convoi_t::finish_rd()","state=%s, next_stop_index=%d", state_names[state], next_stop_index );
		// only realign convois not leaving depot to avoid jumps through signals
		if(  steps_driven!=-1  ) {
			for( uint8 i=0;  i<anz_vehikel;  i++ ) {
				vehicle_t* v = fahr[i];
				v->set_leading( i==0 );
				v->set_last( i+1==anz_vehikel );
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
			for( uint8 i=0;  i<anz_vehikel;  i++ ) {
				vehicle_t* v = fahr[i];
				v->set_leading( i==0 );
				v->set_last( i+1==anz_vehikel );
				v->calc_height();
				// this sets the convoi and will renew the block reservation, if needed!
				v->set_convoi(this);

				// wrong alignment here => must relocate
				if(v->need_realignment()) {
					// diagonal => convoi must restart
					realign_position |= ribi_t::is_bend(v->get_direction())  &&  (state==DRIVING  ||  is_waiting());
				}
				// if version is 99.17 or lower, some convois are broken, i.e. had too large gaps between vehicles
				if(  !realign_position  &&  state!=INITIAL  &&  state!=LEAVING_DEPOT  &&  state != COUPLED  &&  state != COUPLED_LOADING  ) {
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
				FOR(vector_tpl<linehandle_t>, const l, lines) {
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
	if(realign_position  &&  anz_vehikel>1) {
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
			uint16 start_index = max(1,fahr[anz_vehikel-1]->get_route_index())-1;
			if (start_index > route.get_count()) {
				dbg->error( "convoi_t::finish_rd()", "Routeindex of last vehicle of (%s) too large!", get_name() );
				start_index = 0;
			}

			uint32 train_length = move_to(start_index) + 1;
			const koord3d last_start = fahr[0]->get_pos();

			// now advance all convoi until it is completely on the track
			fahr[0]->set_leading(false); // switches off signal checks ...
			for(unsigned i=0; i<anz_vehikel; i++) {
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
	}
	// when saving with open window, this can happen
	if(  state==EDIT_SCHEDULE  ) {
		if (env_t::networkmode) {
			wait_lock = 30000; // 60s to drive on, if the client in question had left
		}
		schedule->finish_editing();
	}
	// If this is a coupled convoi, the front car is not a leading car of the entire convoi.
	if(  state==COUPLED  ||  state==COUPLED_LOADING  ) {
		front()->set_leading(false);
	}
	// If this has a child convoi, the last car is not the last of the entire convoi.
	if(  coupling_convoi.is_bound()  ) {
		back()->set_last(false);
	}
	// remove wrong freight
	check_freight();
	// some convois had wrong old direction in them
	if(  state<DRIVING  ||  state==LOADING  ) {
		alte_richtung = fahr[0]->get_direction();
	}
	// Knightly : if lineless convoy -> register itself with stops
	if(  !line.is_bound()  ) {
		register_stops();
	}

	check_electrification();
	calc_min_top_speed();
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
	for(  int i=0;  i<anz_vehikel;  i++  ) {
		fahr[i]->rotate90_freight_destinations( y_size );
	}
	// eventually correct freight destinations (and remove all stale freight)
	check_freight();
}


/**
 * Return the convoi position.
 * @return Convoi position
 * @author Hj. Malthaner
 */
koord3d convoi_t::get_pos() const
{
	if(anz_vehikel > 0 && fahr[0]) {
		return state==INITIAL ? home_depot : fahr[0]->get_pos();
	}
	else {
		return koord3d::invalid;
	}
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
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
	for( uint8 i=0; i<anz_vehikel; i++ ) {
		len += fahr[i]->get_desc()->get_length();
	}
	return len;
}

uint32 convoi_t::get_entire_convoy_length() const
{
	uint32 len = 0;
	convoihandle_t c = self;
	while(  c.is_bound()  ) {
		len += c->get_length();
		c = c->get_coupling_convoi();
	}
	return len;
}


/**
 * convoi add their running cost for traveling one tile
 * @author Hj. Malthaner
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
	convoihandle_t c = self;
	bool rsl = recalc_speed_limit; // Must speed limit be recalculated?
	bool rmt = recalc_min_top_speed; // Must min_top_speed be recalculated?
	while(  c.is_bound()  &&  !rsl  ) {
		rsl |= c->get_recalc_speed_limit();
		rmt |= c->get_recalc_min_top_speed();
		c = c->get_coupling_convoi();
	}
	
	if(  !recalc_data  &&  !rsl  &&  !rmt  &&  !recalc_data_front  &&  (
		(sum_friction_weight == sum_gesamtweight  &&  akt_speed_soll <= akt_speed  &&  akt_speed_soll+24 >= akt_speed)  ||
		(sum_friction_weight > sum_gesamtweight  &&  akt_speed_soll == akt_speed)  )
		) {
		// at max speed => go with max speed and finish calculation here
		// at slopes/curves, only do this if there is absolutely now change
		akt_speed = akt_speed_soll;
		return;
	}
	
	if(  rmt  ) {
		// min_top_speed and is_electric must be recalculated.
		check_electrification();
		calc_min_top_speed();
		recalc_min_top_speed = false;
		// broadcast min_top_speed
		// is_electric must be updated only for the front convoy.
		c = coupling_convoi;
		while(  c.is_bound()  ) {
			c->set_min_top_speed(min_top_speed);
			c->reset_recalc_min_top_speed();
			c = c->get_coupling_convoi();
		}
	}

	// Dwachs: only compute this if a vehicle in the convoi hopped
	if(  recalc_data  ||  rsl  ) {
		if(  recalc_data  ) {
			sum_friction_weight = sum_gesamtweight = 0;
		}
		// calculate total friction and lowest speed limit
		speed_limit = min_top_speed;
		
		c = self;
		while(  c.is_bound()  ) {
			for(  uint8 i=0;  i<c->get_vehicle_count();  i++  ) {
				const vehicle_t* v = c->get_vehikel(i);
				speed_limit = min( speed_limit, v->get_speed_limit() );

				if (recalc_data) {
					int total_vehicle_weight = v->get_total_weight();
					sum_friction_weight += v->get_frictionfactor() * total_vehicle_weight;
					sum_gesamtweight += total_vehicle_weight;
				}
			}
			c = c->get_coupling_convoi();
		}
		if(  get_yielding_quit_index() != -1 && speed_limit > kmh_to_speed(15)  ) {
			//When yielding lane, speed_limit is 15 kmph lower.
			speed_limit -= kmh_to_speed(15);
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

	// Prissi: more pleasant and a little more "physical" model *

	// try to simulate quadratic friction
	if(sum_gesamtweight != 0) {
		/*
			* The parameter consist of two parts (optimized for good looking):
			*  - every vehicle in a convoi has a the friction of its weight
			*  - the dynamic friction is calculated that way, that v^2*weight*frictionfactor = 200 kW
			* => heavier loaded and faster traveling => less power for acceleration is available!
			* since delta_t can have any value, we have to scale the step size by this value.
			* however, there is a quadratic friction term => if delta_t is too large the calculation may get weird results
			* @author prissi
			*/

		/* but for integer, we have to use the order below and calculate actually 64*deccel, like the sum_gear_and_power
			* since akt_speed=10/128 km/h and we want 64*200kW=(100km/h)^2*100t, we must multiply by (128*2)/100
			* But since the acceleration was too fast, we just decelerate 4x more => >>6 instead >>8 */
		//sint32 deccel = ( ( (akt_speed*sum_friction_weight)>>6 )*(akt_speed>>2) ) / 25 + (sum_gesamtweight*64);	// this order is needed to prevent overflows!
		//sint32 deccel = (sint32)( ( (sint64)akt_speed * (sint64)sum_friction_weight * (sint64)akt_speed ) / (25ll*256ll) + sum_gesamtweight * 64ll) / 1000ll; // intermediate still overflows so sint64
		//sint32 deccel = (sint32)( ( (sint64)akt_speed * ( (sum_friction_weight * (sint64)akt_speed ) / 3125ll + 1ll) ) / 2048ll + (sum_gesamtweight * 64ll) / 1000ll);

		// prissi: integer sucks with planes => using floats ...
		// turfit: result can overflow sint32 and double so onto sint64. planes ok.
		//sint32 delta_v =  (sint32)( ( (double)( (akt_speed>akt_speed_soll?0l:sum_gear_and_power) - deccel)*(double)delta_t)/(double)sum_gesamtweight);
		
		// calculate sum_gear_and_power of all coupled convoys
		sint32 gear_and_power = sum_gear_and_power;
		c = get_coupling_convoi();
		while(c.is_bound()) {
			gear_and_power += c->get_sum_gear_and_power();
			c = c->get_coupling_convoi();
		}

		sint64 residual_power = res_power(akt_speed, akt_speed>akt_speed_soll? 0l : gear_and_power, sum_friction_weight, sum_gesamtweight);

		// we normalize delta_t to 1/64th and check for speed limit */
		//sint32 delta_v = ( ( (akt_speed>akt_speed_soll?0l:sum_gear_and_power) - deccel) * delta_t)/sum_gesamtweight;
		sint64 delta_v = ( residual_power * (sint64)delta_t * 1000ll) / (sint64)sum_gesamtweight;

		// we need more accurate arithmetic, so we store the previous value
		delta_v += previous_delta_v;
		previous_delta_v = (sint32)(delta_v & 0x00000FFFll);
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
	
	// broadcast new akt_speed to all coupling convoys
	c = get_coupling_convoi();
	while(  c.is_bound()  ) {
		c->set_akt_speed(akt_speed);
		c = c->get_coupling_convoi();
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
	for( int i=0;  i<anz_vehikel;  i++  ) {
		current_length += fahr[i]->get_desc()->get_length();
		if(length<current_length) {
			return i;
		}
	}
	return anz_vehikel;
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
			// Hajo: this is an async task, see step()
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
					int v_nr = get_vehicle_at_length((++steps_driven)>>4);
					// stop when depot reached
					if (state==INITIAL) {
						return SYNC_REMOVE;
					}
					if (state==ROUTING_1) {
						break;
					}
					// until all are moving or something went wrong (sp_hat==0)
					if(sp_hat==0  ||  v_nr==anz_vehikel) {
						steps_driven = -1;
						state = DRIVING;
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
					for(int i=0;  i<anz_vehikel;  i++  ) {
						fahr[i]->make_smoke();
					}
				}
			}
			break;	// LEAVING_DEPOT

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
				// now move the rest (so all vehikel are moving synchronously)
				convoihandle_t mc = self;
				while(  mc.is_bound()  ) {
					for(  uint8 i=(mc==self?1:0);  i<mc->get_vehicle_count();  i++  ) {
						mc->get_vehikel(i)->do_drive(sp_hat);
					}
					mc = mc->get_coupling_convoi();
				}
				// maybe we have been stopped be something => avoid wide jumps
				sp_soll = (sp_soll-sp_hat) & 0x0FFF;

				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					mc = self;
					while(  mc.is_bound()  ) {
						for(  uint8 i=0;  i<mc->get_vehicle_count();  i++  ) {
							mc->get_vehikel(i)->make_smoke();
						}
						mc = mc->get_coupling_convoi();
					}
				}
			}
			break;	// DRIVING

		case COUPLED:
		case COUPLED_LOADING:
			break;
			
		case LOADING:
			// Hajo: loading is an async task, see laden()
			break;

		case WAITING_FOR_CLEARANCE:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
			// Hajo: waiting is asynchronous => fixed waiting order and route search
			break;

		case SELF_DESTRUCT:
			// see step, since destruction during a screen update may give strange effects
			break;

		default:
			dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
			break;
	}

	return SYNC_OK;
}


/**
 * Berechne route von Start- zu Zielkoordinate
 * @author Hanjsörg Malthaner
 */
bool convoi_t::drive_to()
{
	if(  anz_vehikel>0  ) {

		// unreserve all tiles that are covered by the train but do not contain one of the wagons,
		// otherwise repositioning of the train drive_to may lead to stray reserved tiles
		if (dynamic_cast<rail_vehicle_t*>(fahr[0])!=NULL  &&  anz_vehikel > 1) {
			// route-index points to next position in route
			// it is completely off when convoi leaves depot
			uint16 index0 = min(fahr[0]->get_route_index()-1, route.get_count());
			for(uint8 i=1; i<anz_vehikel; i++) {
				uint16 index1 = fahr[i]->get_route_index();
				for(uint16 j = index1; j<index0; j++) {
					// unreserve track on tiles between wagons
					grund_t *gr = welt->lookup(route.at(j));
					if (schiene_t *track = (schiene_t *)gr->get_weg( front()->get_waytype() ) ) {
						track->unreserve(self);
					}
				}
				index0 = min(index1-1, route.get_count());
			}
		}
		// Also for road vehicles, unreserve tiles.
		else if(road_vehicle_t* r = dynamic_cast<road_vehicle_t*>(fahr[0])) {
			r->unreserve_all_tiles();
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
 * @author Hanjsörg Malthaner
 */
void convoi_t::suche_neue_route()
{
	state = ROUTING_1;
	wait_lock = 0;
}


/**
 * Asynchrne step methode des Convois
 * @author Hj. Malthaner
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

	strasse_t* str;
	switch(state) {

		case LOADING:
			laden();
			//When loading, vehicle should not be on passing lane.
			str = (strasse_t*)welt->lookup(get_pos())->get_weg(road_wt);
			if(  str  &&  str->get_overtaking_mode()!=inverted_mode  &&  str->get_overtaking_mode()!=halt_mode  ) set_tiles_overtaking(0);
			break;

		case DUMMY4:
		case DUMMY5:
			break;

		case EDIT_SCHEDULE:
			// schedule window closed?
			if(schedule!=NULL  &&  schedule->is_editing_finished()) {

				set_schedule(schedule);
				schedule_target = koord3d::invalid;
				
				// unreserve all tiles
				unreserve_route();

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
					// Hajo: now calculate a new route
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
				// Hajo: now calculate a new route
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
				if(  fahr[0]->get_waytype()==road_wt  ) {
					sint8 overtaking_mode = static_cast<strasse_t*>(welt->lookup(get_pos())->get_weg(road_wt))->get_overtaking_mode();
					if(  (state==CAN_START  ||  state==CAN_START_ONE_MONTH)  &&  overtaking_mode>oneway_mode  &&  overtaking_mode!=inverted_mode  ) {
						set_tiles_overtaking( 0 );
					}
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
				if(  fahr[0]->get_waytype()==road_wt  ) {
					sint8 overtaking_mode = static_cast<strasse_t*>(welt->lookup(get_pos())->get_weg(road_wt))->get_overtaking_mode();
					if(  state!=DRIVING  &&  overtaking_mode>oneway_mode  &&  overtaking_mode!=inverted_mode  ) {
						set_tiles_overtaking( 0 );
					}
				}
			}
			break;

		// must be here; may otherwise confuse window management
		case SELF_DESTRUCT:
			welt->set_dirty();
			destroy();
			return; // must not continue method after deleting this object
			
		case DRIVING:
			if(fahr[0]->get_waytype()==track_wt  ||  fahr[0]->get_waytype()==monorail_wt  ||  fahr[0]->get_waytype()==maglev_wt  ||  fahr[0]->get_waytype()==narrowgauge_wt) {
				rail_vehicle_t* v = dynamic_cast<rail_vehicle_t*>(fahr[0]);
				if(  v  &&  longblock_signal_request.valid  ) {
					// process longblock signal judgement request
					sint32 dummy = -1;
					v->check_longblock_signal(longblock_signal_request.sig, longblock_signal_request.next_block, dummy);
					set_longblock_signal_judge_request_invalid();
				}
			}
			break;

		default:	/* keeps compiler silent*/
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
			/* FALLTHROUGH */
		case EDIT_SCHEDULE:
		case NO_ROUTE:
			wait_lock = max( wait_lock, 25000 );
			break;

		// action soon needed
		case ROUTING_1:
		case CAN_START:
		case WAITING_FOR_CLEARANCE:
			wait_lock = max( wait_lock, 500 );
			break;

		// waiting for free way, not too heavy, not to slow
		case CAN_START_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
			wait_lock = 2500;
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
	if(anz_vehikel==0) {
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
}


void convoi_t::betrete_depot(depot_t *dep)
{
	// first remove reservation, if train is still on track
	unreserve_route();

	// Hajo: remove vehicles from world data structure
	for(unsigned i=0; i<anz_vehikel; i++) {
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
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->set_leading( false );
			fahr[i]->set_last( false );
			restwert_delta -= fahr[i]->calc_sale_value();
			fahr[i]->set_driven();
			restwert_delta += fahr[i]->calc_sale_value();
			fahr[i]->clear_flag( obj_t::not_on_map );
		}
		fahr[0]->set_leading( true );
		fahr[anz_vehikel-1]->set_last( true );
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
	convoihandle_t c = self;
	// broadcast alte_richtung
	while(  c.is_bound()  ) {
		c->set_alte_richtung(c->front()->get_direction());
		c = c->get_coupling_convoi();
	}
	
	const vehicle_t* v = fahr[0];

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
		halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_entry().pos,owner);
		// check for coupling
		if(  next_coupling_index!=INVALID_INDEX  &&  next_coupling_index<=v->get_route_index()  ) {
			const uint16 route_index = v->get_route_index();
			const grund_t* grc[2];
			grc[0] = gr;
			grc[1] = route_index>=get_route()->get_count() ? NULL : welt->lookup(get_route()->at(route_index));
			// find convoy to couple with
			// convoy can be on the next tile of coupling_index.
			for(  uint8 i=0;  i<2;  i++  ) {
				const grund_t* g = grc[i];
				if(  !g  ) {
					continue;
				}
				for(  uint8 pos=1;  pos<(volatile uint8)g->get_top();  pos++  ) {
					if(  vehicle_t* const v = dynamic_cast<vehicle_t*>(g->obj_bei(pos))  ) {
						// there is a suitable waiting convoy for coupling -> this is coupling point.
						if(  can_start_coupling(v->get_convoi())  &&  v->get_convoi()->is_loading()  ) {
							akt_speed = 0;
							if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
								halt->book(1, HALT_CONVOIS_ARRIVED);
							}
							if(  ribi_t::backward(front()->get_direction())==v->get_convoi()->get_next_initial_direction()  ) {
								// this convoy leads the other.
								couple_convoi(v->get_convoi()->self);
							} else {
								// this convoy follows the other.
								v->get_convoi()->couple_convoi(self);
							}
							wait_lock = 0;
							set_next_coupling(INVALID_INDEX, 0);
							v->get_convoi()->set_coupling_done(true);
							coupling_done = true;
							return;
						}
					}
				}
			}
			// convoy to couple with is not found!
			set_next_coupling(INVALID_INDEX, 0);
		}
		
		arrived_time = welt->get_ticks();
		// no depot reached, no coupling, check for stop!
		if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			halt->book(1, HALT_CONVOIS_ARRIVED);
			c = self;
			while(  c.is_bound()  ) {
				c->set_akt_speed(0);
				c->set_state(c==self ? LOADING : COUPLED_LOADING);
				c->set_arrived_time(arrived_time);
				c = c->get_coupling_convoi();
			}
			
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
 * @author Hj. Malthaner
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


bool convoi_t::add_vehikel(vehicle_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehikel()","at pos %i of %i total vehikels.",anz_vehikel,fahr.get_count());
	// extend array if requested
	if(anz_vehikel == fahr.get_count()) {
		fahr.resize(anz_vehikel+1,NULL);
DBG_MESSAGE("convoi_t::add_vehikel()","extend array_tpl to %i totals.",fahr.get_count());
	}
	// now append
	if (anz_vehikel < fahr.get_count()) {
		v->set_convoi(this);

		if(infront) {
			for(unsigned i = anz_vehikel; i > 0; i--) {
				fahr[i] = fahr[i - 1];
			}
			fahr[0] = v;
		} else {
			fahr[anz_vehikel] = v;
		}
		anz_vehikel ++;

		const vehicle_desc_t *info = v->get_desc();
		if(info->get_power()) {
			is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
		}
		sum_power += info->get_power();
		sum_gear_and_power += info->get_power()*info->get_gear();
		sum_weight += info->get_weight();
		sum_running_costs -= info->get_running_cost();
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
		player_t::add_maintenance( get_owner(), info->get_maintenance(), info->get_waytype() );
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

DBG_MESSAGE("convoi_t::add_vehikel()","now %i of %i total vehikels.",anz_vehikel,fahr.get_count());
	return true;
}


vehicle_t *convoi_t::remove_vehikel_bei(uint16 i)
{
	vehicle_t *v = NULL;
	if(i<anz_vehikel) {
		v = fahr[i];
		if(v != NULL) {
			for(unsigned j=i; j<anz_vehikel-1u; j++) {
				fahr[j] = fahr[j + 1];
			}

			v->set_convoi(NULL);

			--anz_vehikel;
			fahr[anz_vehikel] = NULL;

			const vehicle_desc_t *info = v->get_desc();
			sum_power -= info->get_power();
			sum_gear_and_power -= info->get_power()*info->get_gear();
			sum_weight -= info->get_weight();
			sum_running_costs += info->get_running_cost();
			player_t::add_maintenance( get_owner(), -info->get_maintenance(), info->get_waytype() );
		}
		sum_gesamtweight = sum_weight;
		calc_loading();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(anz_vehikel > 0) {
			set_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		min_top_speed = calc_min_top_speed();

		// check for obsolete
		if(has_obsolete) {
			has_obsolete = false;
			const int month_now = welt->get_timeline_year_month();
			for(unsigned i=0; i<anz_vehikel; i++) {
				has_obsolete |= fahr[i]->get_desc()->is_retired(month_now);
			}
		}

		recalc_catg_index();

		// still requires electrifications?
		if(is_electric) {
			is_electric = false;
			for(unsigned i=0; i<anz_vehikel; i++) {
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
		if(get_vehikel(i)->get_cargo_max() == 0) {
			continue;
		}
		const goods_desc_t *ware=get_vehikel(i)->get_cargo_type();
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
	// anz_vehikel muss korrekt init sein
	if(anz_vehikel>0) {
		fahr[0]->set_leading(true);
		for(unsigned i=1; i<anz_vehikel; i++) {
			fahr[i]->set_leading(false);
			fahr[i - 1]->set_last(false);
		}
		fahr[anz_vehikel - 1]->set_last(true);
	}
	else {
		dbg->warning("convoi_t::set_erstes_letzes()", "called with anz_vehikel==0!");
	}
}


// remove wrong freight when schedule changes etc.
void convoi_t::check_freight()
{
	for(unsigned i=0; i<anz_vehikel; i++) {
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
				//		-> unset line now and register stops from new schedule later
				changed = true;
				unset_line();
			}
		}
		else {
			if(  !f->matches( welt, schedule )  ) {
				// Knightly : merely change schedule and do not involve line
				//				-> unregister stops from old schedule now and register stops from new schedule later
				changed = true;
				unregister_stops();
			}
		}
		// destroy a possibly open schedule window
		if(  schedule  &&  !schedule->is_editing_finished()  ) {
			destroy_win((ptrdiff_t)schedule);
			delete schedule;
		}
		schedule = f;
		if(  changed  ) {
			// Knightly : if line is unset or schedule is changed
			//				-> register stops from new schedule
			register_stops();
			welt->set_schedule_counter();	// must trigger refresh
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
	unsigned i;	// for visual C++
	const vehicle_t* pred = NULL;
	convoihandle_t inspecting = self;
	// calculate length including the coupling convoy.
	while(  inspecting.is_bound()  ) {
		for(i=0; i<inspecting->get_vehicle_count(); i++) {
			const vehicle_t* v = inspecting->get_vehikel(i);
			grund_t *gr = welt->lookup(v->get_pos());

			// not last vehicle?
			// the length of last vehicle does not matter when it comes to positioning of vehicles
			if (  i+1 < inspecting->get_vehicle_count()  ||  inspecting->get_coupling_convoi().is_bound()  ) {
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
		
		// see the coupling convoy
		if(  !inspecting->get_coupling_convoi().is_bound()  ) {
			// we preserve the last convoi
			break;
		} else {
			inspecting = inspecting->get_coupling_convoi();
		}
	}
	
	// check if convoi is way too short (even for diagonal tracks)
	// inspecting represents the last convoy of all couled convoys.
	tile_length += (ribi_t::is_straight(welt->lookup(inspecting->back()->get_pos())->get_weg_ribi_unmasked(inspecting->back()->get_waytype())) ? 16 : 8192/vehicle_t::get_diagonal_multiplier());
	if(  convoi_length>tile_length  ) {
		dbg->warning("convoi_t::go_alte_richtung()","convoy too short (id %i) => fixing!",self.get_id());
		akt_speed = 8;
		return false;
	}

	uint16 length = min((convoi_length/16u)+4u,route.get_count());	// maximum length in tiles to check

	// we just check, whether we go back (i.e. route tiles other than zero have convoi vehicles on them)
	// TODO: support convoy coupling
	for( int index=1;  index<length;  index++ ) {
		grund_t *gr=welt->lookup(route.at(index));
		// now check, if we are already here ...
		for(unsigned i=0; i<anz_vehikel; i++) {
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
	inspecting = self; // reset inspecting to the first convoy
	if(welt->lookup(pos)->get_depot()) {
		return false;
	}
	else {
		while(  inspecting.is_bound()  ) {
			for(i=0; i<inspecting->get_vehicle_count(); i++) {
				vehicle_t* v = inspecting->get_vehikel(i);
				// eventually add current position to the route
				if (route.front() != v->get_pos() && route.at(1) != v->get_pos()) {
					route.insert(v->get_pos());
				}
			}
			inspecting = inspecting->get_coupling_convoi();
		}
		// now route was constructed. we broadcast the route to the coupled convoys.
		inspecting = self->get_coupling_convoi();
		while(  inspecting.is_bound()  ) {
			inspecting->access_route()->clear();
			inspecting->access_route()->append(get_route());
			inspecting = inspecting->get_coupling_convoi();
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we must set the current route index (instead assuming 1)
	length = min((convoi_length/8u),route.get_count()-1);	// maximum length in tiles to check
	inspecting = self; // reset inspecting to the first convoy
	bool ok=false;
	while(  inspecting.is_bound()  ) {
		for(i=0; i<inspecting->get_vehicle_count(); i++) {
			vehicle_t* v = inspecting->get_vehikel(i);

			// this is such awkward, since it takes into account different vehicle length
			const koord3d vehicle_start_pos = v->get_pos();
			for( int idx=0;  idx<=length;  idx++  ) {
				if(route.at(idx)==vehicle_start_pos) {
					// set route index, no recalculations necessary
					v->initialise_journey(idx, false );
					ok = true;

					// check direction
					uint8 richtung = v->get_direction();
					uint8 neu_richtung = v->calc_direction( route.at(max(idx-1,0)), v->get_pos_next());
					// we need to move to this place ...
					if(neu_richtung!=richtung  &&  (i!=0  ||  anz_vehikel==1  ||  ribi_t::is_bend(neu_richtung)) ) {
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
		inspecting = inspecting->get_coupling_convoi();
	}

	return true;
}


// put the convoi on its way
void convoi_t::vorfahren()
{
	// Hajo: init speed settings
	sp_soll = 0;
	if(  get_tiles_overtaking()<=0  ) {
		set_tiles_overtaking(0);
	}
	recalc_data_front = true;
	recalc_data = true;

	koord3d k0 = route.front();
	grund_t *gr = welt->lookup(k0);
	bool at_dest = false;
	if(gr  &&  gr->get_depot()) {
		// start in depot
		for(unsigned i=0; i<anz_vehikel; i++) {
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
		for(int i=0; i<anz_vehikel; i++) {
			fahr[i]->do_drive( (VEHICLE_STEPS_PER_TILE/2)<<YARDS_PER_VEHICLE_STEP_SHIFT );
		}
		v0->get_smoke(true);
		v0->set_leading(true); // switches on signal checks to reserve the next route

		// until all other are on the track
		state = CAN_START;
	}
	else {
		// copy route to all coupling convoys in advance.
		convoihandle_t inspecting = coupling_convoi;
		while(  inspecting.is_bound()  ) {
			inspecting->access_route()->clear();
			inspecting->access_route()->append(get_route());
			inspecting = inspecting->get_coupling_convoi();
		}
		
		// still leaving depot (steps_driven!=0) or going in other direction or misalignment?
		if(  steps_driven>0  ||  !can_go_alte_richtung()  ) {

			// start route from the beginning at index 0, place everything on start
			uint32 train_length = 0;
			inspecting = self;
			while(  inspecting.is_bound()  ) {
				train_length += inspecting->move_to(0);
				if(  inspecting->get_coupling_convoi().is_bound()  ) {
					// since move_to() returns convoy length minus last vehicle length...
					train_length += inspecting->back()->get_desc()->get_length();
					inspecting = inspecting->get_coupling_convoi();
				} else {
					break;
				}
			}
			// now inspecting represents the last convoy of all coupled convoys.

			// move one train length to the start position ...
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->get_direction();
			if(neue_richtung==ribi_t::south  ||  neue_richtung==ribi_t::east) {
				// drive the convoi to the same position, but do not hop into next tile!
				if(  train_length%16==0  ) {
					// any space we need => just add
					train_length += inspecting->back()->get_desc()->get_length();
				}
				else {
					// limit train to front of tile
					train_length += min( (train_length%CARUNITS_PER_TILE)-1, inspecting->back()->get_desc()->get_length() );
				}
			}
			else {
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			fahr[0]->set_leading(false); // switches off signal checks ...
			uint32 dist = VEHICLE_STEPS_PER_CARUNIT*train_length<<YARDS_PER_VEHICLE_STEP_SHIFT;
			inspecting = self;
			while(  inspecting.is_bound()  ) {
				for(unsigned i=0; i<inspecting->get_vehicle_count(); i++) {
					vehicle_t* v = inspecting->get_vehikel(i);

					v->get_smoke(false);
					uint32 const driven = v->do_drive( dist );
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
				inspecting = inspecting->get_coupling_convoi();
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
		for(unsigned i=0; i<anz_vehikel; i++) {
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
	// and calculate crossing reservation.
	if(  fahr[0]->get_waytype()==track_wt  ) {
		calc_crossing_reservation();
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

	dummy = anz_vehikel;
	file->rdwr_long(dummy);
	anz_vehikel = (uint8)dummy;

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
		if(anz_vehikel > fahr.get_count()) {
			fahr.resize(anz_vehikel, NULL);
		}
		owner = welt->get_player( owner_n );

		// Hajo: sanity check for values ... plus correction
		if(sp_soll < 0) {
			sp_soll = 0;
		}
	}

	file->rdwr_str(name_and_id + name_offset, lengthof(name_and_id) - name_offset);
	if(file->is_loading()) {
		set_name(name_and_id+name_offset);	// will add id automatically
	}

	koord3d dummy_pos;
	if(file->is_saving()) {
		for(unsigned i=0; i<anz_vehikel; i++) {
			file->wr_obj_id( fahr[i]->get_typ() );
			fahr[i]->rdwr_from_convoi(file);
		}
	}
	else {
		bool override_monorail = false;
		for(  uint8 i=0;  i<anz_vehikel;  i++  ) {
			obj_t::typ typ = (obj_t::typ)file->rd_obj_id();
			vehicle_t *v = 0;

			const bool first = (i==0);
			const bool last = (i==anz_vehikel-1u);
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
				anz_vehikel --;
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

			// Hajo: if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				sum_power += info->get_power();
				sum_gear_and_power += info->get_power()*info->get_gear();
				sum_weight += info->get_weight();
				sum_running_costs -= info->get_running_cost();
				has_obsolete |= welt->use_timeline()  &&  info->is_retired( welt->get_timeline_year_month() );
				player_t::add_maintenance( get_owner(), info->get_maintenance(), info->get_waytype() );
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
		// Hajo: hack to load corrupted games -> there is a schedule
		// but no vehicle so we can't determine the exact type of
		// schedule needed. This hack is safe because convois
		// without vehicles get deleted right after loading.
		// Since generic schedules are not allowed, we use a train_schedule_t
		if(schedule == 0) {
			schedule = new train_schedule_t();
		}

		// Hajo: now read the schedule, we have one for sure here
		schedule->rdwr( file );
	}

	if(file->is_loading()) {
		next_wolke = 0;
		calc_loading();
	}

	// Hajo: since sp_ist became obsolete, sp_soll is used modulo 65536
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
		file->rdwr_long(dummy);	// ignore
	}
	if(file->is_loading()) {
		line_update_pending = linehandle_t();
	}

	if(file->is_version_atleast(84, 10)) {
		home_depot.rdwr(file);
	}

	// Old versions recorded last_stop_pos in convoi, not in vehicle
	koord3d last_stop_pos_convoi = koord3d(0,0,0);
	if (anz_vehikel !=0) {
		last_stop_pos_convoi = fahr[0]->last_stop_pos;
	}
	if(file->is_version_atleast(87, 1)) {
		last_stop_pos_convoi.rdwr(file);
	}
	else {
		last_stop_pos_convoi =
			!route.empty()   ? route.front()      :
			anz_vehikel != 0 ? fahr[0]->get_pos() :
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
			if(  has_schedule  &&  schedule->get_current_entry().waiting_time_shift > 0  ) {
				uint32 diff_ticks = arrived_time + (welt->ticks_per_world_month >> (16 - schedule->get_current_entry().waiting_time_shift)) - welt->get_ticks();
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
			arrived_time = has_schedule ? welt->get_ticks() - (welt->ticks_per_world_month >> (16 - schedule->get_current_entry().waiting_time_shift)) + diff_ticks : 0;
		}
	}

	// since 99015, the last stop will be maintained by the vehikels themselves
	if(file->is_version_less(99, 15)) {
		for(unsigned i=0; i<anz_vehikel; i++) {
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
		if(  !file->is_loading()  &&  next_reservation_index>=route.get_count()  ) {
			// sanitize next_reservation_index because a longblocksignal can set next_reservation_index an illegal number.
			next_reservation_index = route.get_count()-1;
		}
		file->rdwr_short( next_reservation_index );
		// If this convoy is an aircraft, next_reservation_index must be 0. sanitaze next_reservation_index because next_reservation_index often be an illegal number. The cause of this problem is still not found!
		const waytype_t typ = front()->get_waytype();
		const bool rail_convoy = typ==track_wt  ||  typ==tram_wt  ||  typ==maglev_wt  ||  typ==monorail_wt  ||  typ==narrowgauge_wt;
		if(  !rail_convoy  &&  next_reservation_index!=0  &&  file->is_loading()  ) {
			dbg->warning( "convoi_t::rdwr()","next_reservation_index of convoy %d is %d while this is not a rail convoy. next_reservation_index is sanitized to 0.", self.get_id(), next_reservation_index );
			next_reservation_index = 0;
		}
	}
	
	// for new reservation system with reserved_tiles
	if(  file->get_OTRP_version()>=18  ) {
		uint32 count = reserved_tiles.get_count();
		file->rdwr_long(count);
		if(  file->is_loading()  ) { // reading
			reserved_tiles.clear();
			for(  uint32 i=0;  i<count;  i++  ) {
				koord3d pos;
				pos.rdwr(file);
				reserved_tiles.append(pos);
			}
		}
		else { // writing
			for(  uint32 i=0;  i<count;  i++  ) {
				reserved_tiles[i].rdwr(file);
			}
		}
	}

	if(  (env_t::previous_OTRP_data  &&  file->is_version_atleast(120, 6))  ||  file->get_OTRP_version() >= 14  ) {
		file->rdwr_long(yielding_quit_index);
		file->rdwr_byte(lane_affinity);
		file->rdwr_long(lane_affinity_end_index);
	}
	
	if(  file->get_OTRP_version()>=20  ) {
		uint16 cnt = crossing_reservation_index.get_count();
		file->rdwr_short(cnt);
		if(  file->is_loading()  ) {
			crossing_reservation_index.clear();
			for(uint16 i=0; i<cnt; i++) {
				uint16 first, second;
				file->rdwr_short(first);
				file->rdwr_short(second);
				crossing_reservation_index.append(std::pair<uint16,uint16>(first, second));
			}
		}
		else {
			for(uint16 i=0; i<crossing_reservation_index.get_count(); i++) {
				file->rdwr_short(crossing_reservation_index[i].first);
				file->rdwr_short(crossing_reservation_index[i].second);
			}
		}
	}
	
	// coupling related things
	if(  file->get_OTRP_version()>=22  ) {
		rdwr_convoihandle_t( file, coupling_convoi );
		file->rdwr_short( next_coupling_index );
		file->rdwr_byte( next_coupling_steps );
		file->rdwr_bool(coupling_done);
		file->rdwr_byte(next_initial_direction);
	}

	if(  file->is_loading()  ) {
		reserve_route();
		recalc_catg_index();
	}
}


void convoi_t::open_info_window()
{
	if(  in_depot()  ) {
		// Knightly : if ownership matches, we can try to open the depot dialog
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
		if(  env_t::verbose_debug  ) {
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
		buf.printf(" %s: %i (%i) t\n", translator::translate("Gewicht"), sum_weight, sum_gesamtweight - sum_weight);
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

		for(  uint32 i = 0;  i != anz_vehikel;  ++i  ) {
			const vehicle_t* v = fahr[i];

			// first add to capacity indicator
			const goods_desc_t* ware_desc = v->get_desc()->get_freight_type();
			const uint16 menge = v->get_desc()->get_capacity();
			if(menge>0  &&  ware_desc!=goods_manager_t::none) {
				max_loaded_waren[ware_desc->get_index()] += menge;
			}

			// then add the actual load
			FOR(slist_tpl<ware_t>, ware, v->get_cargo()) {
				FOR(vector_tpl<ware_t>, & tmp, total_fracht) {
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
		for (size_t i = 0; i != n; ++i) {
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
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

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

	akt_speed = 0;	// stop the train ...
	if(state!=INITIAL) {
		state = EDIT_SCHEDULE;
	}
	wait_lock = 25000;
	alte_richtung = fahr[0]->get_direction();

	if(  show  ) {
		// Open schedule dialog
		create_win( new schedule_gui_t(schedule,get_owner(),self), w_info, (ptrdiff_t)schedule );
		// TODO: what happens if no client opens the window??
	}
	schedule->start_editing();
}


/**
 * Check validity of convoi with respect to vehicle constraints
 */
bool convoi_t::pruefe_alle()
{
	bool ok = anz_vehikel == 0  ||  fahr[0]->get_desc()->can_follow(NULL);
	unsigned i;

	const vehicle_t* pred = fahr[0];
	for(i = 1; ok && i < anz_vehikel; i++) {
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
 * @author Hj. Malthaner
 *
 * V.Meyer: minimum_loading is now stored in the object (not returned)
 */
void convoi_t::laden()
{
	if(  state == EDIT_SCHEDULE  ) {
		return;
	}

	// just wait a little longer if this is a non-bound halt
	wait_lock = (WTT_LOADING*2)+(self.get_id())%1024;

	halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_entry().pos,owner);
	// eigene haltestelle ?
	if(  halt.is_bound()  ) {
		const player_t* halt_owner = halt->get_owner();
		if(  halt_owner == get_owner()  ||  halt_owner == welt->get_public_player()  ) {
			// loading/unloading ...
			halt->request_loading( self );
		}
	}
}


/**
 * calculate income for last hop
 * @author Hj. Malthaner
 */
void convoi_t::calc_gewinn()
{
	sint64 gewinn = 0;

	for(unsigned i=0; i<anz_vehikel; i++) {
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


/**
 * convoi an haltestelle anhalten
 * @author Hj. Malthaner
 *
 * V.Meyer: minimum_loading is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(halthandle_t halt)
{
	grund_t *gr=welt->lookup(fahr[0]->get_pos());

	// now find out station length
	uint16 vehicles_loading = 0;
	if(  gr->is_water()  ) {
		// harbour has any size
		vehicles_loading = anz_vehikel;
	}
	else {
		// calculate real station length
		// and numbers of vehicles that can be (un)loaded
		koord zv = koord( ribi_t::backward(fahr[0]->get_direction()) );
		koord3d pos = fahr[0]->get_pos();
		// start on bridge?
		pos.z += gr->get_weg_yoff() / TILE_HEIGHT_STEP;
		// difference between actual station length and vehicle lenghts
		sint16 station_length = -fahr[vehicles_loading]->get_desc()->get_length();
		do {
			// advance one station tile
			station_length += CARUNITS_PER_TILE;

			while(station_length >= 0) {
				vehicles_loading++;
				if (vehicles_loading < anz_vehikel) {
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
						// do not load for stops after a depot
						break;
					}
				}
				continue;
			}
			destination_halts.append(plan_halt);
		}
	}

	// only load vehicles in station
	// don't load when vehicle is being withdrawn
	bool changed_loading_level = false;
	uint32 time = WTT_LOADING;	// min time for loading/unloading
	sint64 gewinn = 0;

	// cargo type of previous vehicle that could not be filled
	const goods_desc_t* cargo_type_prev = NULL;

	for(unsigned i=0; i<vehicles_loading; i++) {
		vehicle_t* v = fahr[i];

		// we need not to call this on the same position
		if(  v->last_stop_pos != v->get_pos()  ) {
			sint64 tmp;
			// calc_revenue
			gewinn += tmp = v->calc_revenue(v->last_stop_pos, v->get_pos() );
			owner->book_revenue(tmp, fahr[0]->get_pos().get_2d(), get_schedule()->get_waytype(), v->get_cargo_type()->get_index());
			v->last_stop_pos = v->get_pos();
		}

		const grund_t *gr = welt->lookup( schedule->entries[(schedule->get_current_stop()+1)%schedule->get_count()].pos );
		const bool next_depot = gr  &&  gr->get_depot();
		uint16 amount = v->unload_cargo(halt, next_depot  );

		if(  !no_load  &&  !next_depot  &&  v->get_total_cargo() < v->get_cargo_max()  ) {
			// load if: unloaded something (might go back) or previous non-filled car requested different cargo type
			if (amount>0  ||  cargo_type_prev==NULL  ||  !cargo_type_prev->is_interchangeable(v->get_cargo_type())) {
				// load
				amount += v->load_cargo(halt, destination_halts);
			}
			if (v->get_total_cargo() < v->get_cargo_max()) {
				// not full
				cargo_type_prev = v->get_cargo_type();
			}
		}

		if(  amount  ) {
			time = max( time, (amount*v->get_desc()->get_loading_time()) / max(v->get_cargo_max(), 1) );
			v->mark_image_dirty(v->get_image(), 0);
			v->calc_image();
			changed_loading_level = true;
		}
	}
	freight_info_resort |= changed_loading_level;
	if(  changed_loading_level  ) {
		halt->recalc_status();
	}

	// any unloading/loading went on?
	if(  changed_loading_level  ) {
		calc_loading();
	}
	loading_limit = schedule->get_current_entry().minimum_loading;

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
	
	// uncouple convoy if needed.
	if(  coupling_convoi.is_bound()  &&  !can_continue_coupling()  ) {
		uncouple_convoi();
	}
	
	convoihandle_t c = self;
	if(  recalc_min_top_speed  ) {
		check_electrification();
		calc_min_top_speed();
		while(  c.is_bound()  ) {
			c->set_min_top_speed(min_top_speed);
			c->reset_recalc_min_top_speed();
			c = c->get_coupling_convoi();
		}
	}
	
	if(  coupling_convoi.is_bound()  ) {
		// load/unload cargo of coupling convoy.
		coupling_convoi->hat_gehalten(halt);
	}

	bool departure_cond = true;
	bool need_coupling_at_this_stop = false;
	c = self;
	// A coupled convoy does not have to judge the departure.
	while(  !is_coupled()  &&  c.is_bound()  ) {
		bool cond = c->get_loading_level() >= c->get_schedule()->get_current_entry().minimum_loading; // minimum loading
		need_coupling_at_this_stop |= (c->get_schedule()->get_current_entry().coupling_point==1  &&  !c->is_coupling_done()  &&  !(c->get_coupling_convoi().is_bound()  &&  c->is_coupled())); // coupling done?
		cond |= c->get_no_load(); // no load
		cond |= (c->get_schedule()->get_current_entry().waiting_time_shift > 0  &&  welt->get_ticks() - arrived_time > (welt->ticks_per_world_month >> (16 - c->get_schedule()->get_current_entry().waiting_time_shift)) ); // waiting time
		departure_cond &= cond;
		c = c->get_coupling_convoi();
	}
	departure_cond &= !need_coupling_at_this_stop;
	
	if(  need_coupling_at_this_stop  &&  next_initial_direction==ribi_t::none  ) {
		// calc the initial direction to the next stop.
		route_t r;
		route_t::route_result_t res = r.calc_route(welt, front()->get_pos(), schedule->get_next_entry().pos, front(), speed_to_kmh(min_top_speed), 8888);
		if(  res==route_t::no_route  ||  r.get_count()<2  ) {
			// assume we do not turn here
			next_initial_direction = front()->get_direction();
		} else {
			next_initial_direction = ribi_type(r.at(0), r.at(1));
		}
	}

	// loading is finished => maybe drive on
	if(  is_coupled()  ||  departure_cond  ) {

		if(  !is_coupled()  &&  !coupling_convoi.is_bound()  &&  withdraw  &&  (loading_level == 0  ||  goods_catg_index.empty())  ) {
			// destroy when empty, alone
			self_destruct();
			return;
		}

		calc_speedbonus_kmh();

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<anz_vehikel; i++) {
			book(get_vehikel(i)->get_cargo_max()-get_vehikel(i)->get_total_cargo(), CONVOI_CAPACITY);
		}

		// Advance schedule
		if(  !is_coupled()  ) {
			schedule->advance();
			state = ROUTING_1;
			loading_limit = 0;
			coupling_done = false;
			// Advance schedule of coupling convoy recursively.
			convoihandle_t c_cnv = coupling_convoi;
			while(  c_cnv.is_bound()  ) {
				c_cnv->get_schedule()->advance();
				c_cnv->set_state(COUPLED);
				c_cnv->set_coupling_done(false);
				c_cnv = c_cnv->get_coupling_convoi();
			}
		}
		
	}

	INT_CHECK( "convoi_t::hat_gehalten" );

	// at least wait the minimum time for loading
	wait_lock = time;
}


sint64 convoi_t::calc_restwert() const
{
	sint64 result = 0;

	for(uint i=0; i<anz_vehikel; i++) {
		result += fahr[i]->calc_sale_value();
	}
	return result;
}


/**
 * Calculate loading_level and loading_limit. This depends on current state (loading or not).
 * @author Volker Meyer
 * @date  20.06.2003
 */
void convoi_t::calc_loading()
{
	int fracht_max = 0;
	int fracht_menge = 0;
	for(unsigned i=0; i<anz_vehikel; i++) {
		const vehicle_t* v = fahr[i];
		fracht_max += v->get_cargo_max();
		fracht_menge += v->get_total_cargo();
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
	loading_limit = 0;	// will be set correctly from hat_gehalten() routine

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
		for(  unsigned i=0;  i<anz_vehikel;  i++  ) {
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
 * @author Hj. Malthaner
 */
void convoi_t::self_destruct()
{
	line_update_pending = linehandle_t();	// does not bother to add it to a new line anyway ...
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
 * @author Hj. Malthaner
 * @date  12-Jul-03
 */
void convoi_t::destroy()
{
	// can be only done here, with a valid convoihandle ...
	if(fahr[0]) {
		fahr[0]->set_convoi(NULL);
	}

	if(  state == INITIAL  ) {
		// in depot => not on map
		for(  uint8 i = anz_vehikel;  i-- != 0;  ) {
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

	for(  uint8 i = anz_vehikel;  i-- != 0;  ) {
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
		player_t::add_maintenance( owner, -fahr[i]->get_desc()->get_maintenance(), fahr[i]->get_desc()->get_waytype() );

		fahr[i]->discard_cargo();
		fahr[i]->cleanup(owner);
		delete fahr[i];
	}
	anz_vehikel = 0;

	delete this;
}


/**
 * Debug info nach stderr
 * @author Hj. Malthaner
 * @date 04-Sep-03
 */
void convoi_t::dump() const
{
	dbg->debug("convoi::dump()",
		"\nanz_vehikel = %d\n"
		"wait_lock = %d\n"
		"owner_n = %d\n"
		"akt_speed = %d\n"
		"akt_speed_soll = %d\n"
		"sp_soll = %d\n"
		"state = %d\n"
		"statename = %s\n"
		"alte_richtung = %d\n"
		"jahresgewinn = %ld\n"	// %lld crashes mingw now, cast gewinn to long ...
		"name = '%s'\n"
		"line_id = '%d'\n"
		"schedule = '%p'",
		(int)anz_vehikel,
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


sint32 convoi_t::get_fix_cost() const
{
	sint32 running_cost = 0;
	for(  unsigned i = 0;  i < get_vehicle_count();  i++  ) {
		running_cost += fahr[i]->get_desc()->get_maintenance();
	}
	return running_cost;
}


sint32 convoi_t::get_running_cost() const
{
	sint32 running_cost = 0;
	for(  unsigned i = 0;  i < get_vehicle_count();  i++  ) {
		running_cost += fahr[i]->get_operating_cost();
	}
	return running_cost;
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
* @author hsiegeln
*/
void convoi_t::set_line(linehandle_t org_line)
{
	// to remove a convoi from a line, call unset_line(); passing a NULL is not allowed!
	if(!org_line.is_bound()) {
		return;
	}
	if(  line.is_bound()  ) {
		unset_line();
	}
	else {
		// Knightly : originally a lineless convoy -> unregister itself from stops as it now belongs to a line
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
* @author hsiegeln
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
 * @author Knightly
 */
void convoi_t::register_stops()
{
	if(  schedule  ) {
		FOR(minivec_tpl<schedule_entry_t>, const& i, schedule->entries) {
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, get_owner());
			if(  halt.is_bound()  ) {
				halt->add_convoy(self);
			}
		}
	}
}


/**
 * Unregister the convoy from the stops in the schedule
 * @author Knightly
 */
void convoi_t::unregister_stops()
{
	if(  schedule  ) {
		FOR(minivec_tpl<schedule_entry_t>, const& i, schedule->entries) {
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, get_owner());
			if(  halt.is_bound()  ) {
				halt->remove_convoy(self);
			}
		}
	}
}


// set next stop before breaking will occur (or route search etc.)
// currently only used for tracks
void convoi_t::set_next_stop_index(uint16 n)
{
	// stop at station or signals, not at waypoints
	if(  n==INVALID_INDEX  ) {
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
void convoi_t::set_next_reservation_index(uint16 n)
{
	// stop at station or signals, not at waypoints
	if(  n==INVALID_INDEX  ) {
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
		return color_idx_to_rgb(COL_BLUE);
	}
	// normal state
	return SYSCOL_TEXT;
}


// returns tiles needed for this convoi
uint16 convoi_t::get_tile_length(bool entire) const
{
	uint16 carunits=0;
	convoihandle_t c = self;
	while(  c.is_bound()  ) {
		for(uint8 i=0;  i<c->get_vehicle_count()-1;  i++) {
			carunits += c->get_vehikel(i)->get_desc()->get_length();
		}
		if(  !entire  ||  !c->get_coupling_convoi().is_bound()  ) {
			break;
		} else {
			carunits += c->back()->get_desc()->get_length();
			c = c->get_coupling_convoi();
		}
	}
	// the last vehicle counts differently in stations and for reserving track
	// (1) add 8 = 127/256 tile to account for the driving in stations in north/west direction
	//     see at the end of vehicle_t::hop()
	// (2) for length of convoi for loading in stations the length of the last vehicle matters
	//     see convoi_t::hat_gehalten
	carunits += max(CARUNITS_PER_TILE/2, fahr[anz_vehikel-1]->get_desc()->get_length());

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
 * @author isidoro
 */
bool convoi_t::can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other)
{
	if(  fahr[0]->get_waytype()!=road_wt  ) {
		return false;
	}
	grund_t *gr = welt->lookup(get_pos());
	if(  gr==NULL  ) {
		// should never happen, since there is a vehicle in front of us ...
		return false;
	}
	strasse_t *str = (strasse_t*)gr->get_weg(road_wt);
	if(  str==0  ) {
		// also this is not possible, since a car loads in front of is!?!
		return false;
	}
	overtaking_mode_t overtaking_mode = str->get_overtaking_mode();
	if (  !other_overtaker->can_be_overtaken()  &&  overtaking_mode > oneway_mode  ) {
		return false;
	}
	if(  overtaking_mode == prohibited_mode  ){
		// This road prohibits overtaking.
		return false;
	}
	
	if(  str->is_reserved_by_others((road_vehicle_t*)fahr[0], true, ((road_vehicle_t*)fahr[0])->get_pos_prev(), fahr[0]->get_pos_next())  ) {
		// Passing lane of the next tile is reserved by other vehicle.
		return false;
	}
	

	if(  other_speed == 0  ) {
		/* overtaking a loading convoi
		 * => we can do a lazy check, since halts are always straight
		 */

		// In this scope, variable "overtaking_mode" represents the strictest mode of tiles needed for overtaking.

		uint16 idx = fahr[0]->get_route_index();
		const sint32 tiles = (steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) + get_tile_length() + 1;

		for(  sint32 i=0;  i<tiles;  i++  ) {
			grund_t *gr = welt->lookup( route.at( idx+i ) );
			if(  gr==NULL  ) {
				return false;
			}
			strasse_t *str = (strasse_t*)gr->get_weg(road_wt);
			if(  str==0  ) {
				return false;
			}
			// not overtaking on railroad crossings or normal crossings ...
			if(  str->is_crossing() ) {
				return false;
			}
			if(  ribi_t::is_threeway(str->get_ribi())  &&  overtaking_mode > oneway_mode  ) {
				// On one-way road, overtaking on threeway is allowed.
				return false;
			}
			const overtaking_mode_t mode_of_the_tile = str->get_overtaking_mode();
			if(  mode_of_the_tile>overtaking_mode  ) {
				// update overtaking_mode to a stricter condition.
				overtaking_mode = mode_of_the_tile;
			}
			if(  overtaking_mode==prohibited_mode  ) {
				return false;
			}
			if(  idx+(uint32)i==route.get_count()-1  &&  i<tiles-1  ) {
				// reach the end of route before examination of all tiles needed for overtaking.
				// convoy can stop on the passing lane only in halt_mode.
				if(  overtaking_mode==halt_mode  ) {
					set_tiles_overtaking(tiles);
					return true;
				} else {
					return false;
				}
			}
			if(  overtaking_mode>oneway_mode  ) {
				// Check for other vehicles on the next tile
				const uint8 top = gr->get_top();
				//These conditions is abandoned on one-way road to overtake in traffic jam.
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
		}
		set_tiles_overtaking( tiles );
		return true;
	}

	// Around the end of route, overtaking moving convoi should not be allowed.
	if(  get_route()->get_count() - fahr[0]->get_route_index() < 5  ) {
		return false;
	}
	// Do not overtake a vehicle which has higher max_power_speed than this.
	if(  other_overtaker->get_max_power_speed() - this->get_max_power_speed() > kmh_to_speed(5)  ) {
		return false;
	}

	// The flag whether the convoi is in traffic jam. When this is true, we must calculate overtaking in a different way.
	// On one-way road, other_speed is current speed. Otherwise, other_speed is the theoretical max power speed.
	bool in_congestion = false;
	int diff_speed = akt_speed - other_speed;
	if(  diff_speed < kmh_to_speed(5)  ) {
		// Overtaking in traffic jam is only accepted on one-way road.
		if(  overtaking_mode <= oneway_mode  ) {
			grund_t *gr = welt->lookup(get_pos());
			strasse_t *str=(strasse_t *)gr->get_weg(road_wt);
			if(  str==NULL  ) {
				return false;
			}else if(  akt_speed < fmin(max_power_speed, str->get_max_speed())/2  &&  diff_speed >= kmh_to_speed(0)  ){
				//Warning: diff_speed == 0 is acceptable. We must consider the case diff_speed == 0.
				in_congestion = true;
			}else{
				return false;
			}
		}
		else {
			return false;
		}
	}

	// Number of tiles overtaking will take
	int n_tiles = 0;

	// Distance it takes overtaking (unit: vehicle_steps) = my_speed * time_overtaking
	// time_overtaking = tiles_to_overtake/diff_speed
	// tiles_to_overtake = convoi_length + current pos within tile + (pos_other_convoi within tile + length of other convoi) - one tile
	int distance = 1;
	if(  !in_congestion  ) {
		distance = akt_speed*(fahr[0]->get_steps()+get_length_in_steps()+steps_other-VEHICLE_STEPS_PER_TILE)/diff_speed;
	}
	else {
		distance = max_power_speed*(fahr[0]->get_steps()+get_length_in_steps()+steps_other-VEHICLE_STEPS_PER_TILE)/(max_power_speed-other_speed);
	}

	int time_overtaking = 0;

	// Conditions for overtaking (originally):
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	// First phase: no traffic except me and my overtaken car in the dangerous zone
	unsigned int route_index = fahr[0]->get_route_index()+1;
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
		if(  gr==NULL  ) {
			return false;
		}
		strasse_t *str = (strasse_t*)gr->get_weg(road_wt);
		if(  str==NULL  ) {
			return false;
		}
		overtaking_mode_t overtaking_mode_loop = str->get_overtaking_mode();
		if(  overtaking_mode_loop > oneway_mode  &&  gr->get_weg_hang() != slope_t::flat  ) {
			return false;
		}

		if(  overtaking_mode_loop > twoway_mode  ){
			// Since the other vehicle is moving...
			return false;
		}
		// the only roadsign we must account for are choose points and traffic lights
		if(  str->has_sign()  ) {
			const roadsign_t *rs = gr->find<roadsign_t>(1);
			if(rs) {
				const roadsign_desc_t *rb = rs->get_desc();
				if(  rb->is_choose_sign()  ) {
					// because we need to stop here ...
					return false;
				}
				//We consider traffic-lights on two-way road.
				if(  rb->is_traffic_light()  &&  overtaking_mode_loop >= twoway_mode  ) {
					return false;
				}
			}
		}
		// not overtaking on railroad crossings or on normal crossings ...
		if(  overtaking_mode_loop >= twoway_mode  &&  (str->is_crossing()  ||  ribi_t::is_threeway(str->get_ribi()))  ) {
			return false;
		}
		// street gets too slow (TODO: should be able to be correctly accounted for)
		if(  overtaking_mode_loop >= twoway_mode  &&  akt_speed > kmh_to_speed(str->get_max_speed())  ) {
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
						if(  overtaking_mode_loop <= oneway_mode  ) {
							//If ov goes same directory, should not return false
							ribi_t::ribi their_direction = ribi_t::backward( fahr[0]->calc_direction(pos_prev, pos_next) );
							vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j));
							if (v && v->get_direction() == their_direction && v->get_overtaker()) {
								return false;
							}
						}
						else {
							return false;
						}
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
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will subtract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	// Conditions for the street are milder: e.g. if no street, no facing traffic
	time_overtaking = (time_overtaking << 16)/akt_speed;
	while(  time_overtaking > 0  ) {

		if(  route_index >= route.get_count()  ) {
			return false;
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

		if(  ribi_t::is_straight(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / kmh_to_speed(str->get_max_speed());
		}
		else {
			time_overtaking -= (vehicle_base_t::get_diagonal_vehicle_steps_per_tile()<<16) / kmh_to_speed(str->get_max_speed());
		}

		// Check for other vehicles in facing direction
		ribi_t::ribi their_direction = ribi_t::backward( fahr[0]->calc_direction(pos_prev, pos_next) );
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j));
			if (v && v->get_direction() == their_direction && v->get_overtaker()) {
				return false;
			}
		}
		pos_prev = pos;
		pos = pos_next;
	}

	set_tiles_overtaking( 1+n_tiles );
	//The parameter about being overtaken is no longer meaningful on one-way road.
	if(  overtaking_mode > oneway_mode  ) {
		other_overtaker->set_tiles_overtaking( -1-(n_tiles*(akt_speed-diff_speed))/akt_speed );
	}
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
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
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

/*
 * Functions to yield lane space to vehicles on passing lane.
 * More natural movement controll is desired!
 * @author teamhimeH
 */
void convoi_t::yield_lane_space()
{
	// we do not allow lane yielding when the end of route is close.
	if(  speed_limit > kmh_to_speed(20)  &&  fahr[0]->get_route_index() < get_route()->get_count() - 4u  ) {
		yielding_quit_index = fahr[0]->get_route_index() + 3u;
		must_recalc_speed_limit();
	}
}

bool convoi_t::calc_lane_affinity(uint8 lane_affinity_sign)
{
	if(  lane_affinity_sign!=0  &&  lane_affinity_sign<4  ) {
		uint16 test_index = fahr[0]->get_route_index();
		while(  test_index < route.get_count()  ) {
			grund_t *gr = welt->lookup(route.at(test_index));
			if(  !gr  ) {
				// way (weg) not existent (likely destroyed)
				return false;
			}
			strasse_t *str = (strasse_t *)gr->get_weg(road_wt);
			if(  !str  ||  gr->get_top() > 250  ||  str->get_overtaking_mode() > oneway_mode  ) {
				// too many cars here or no street or not one-way road
				return false;
			}
			ribi_t::ribi str_ribi = str->get_ribi_unmasked();
			if(  str_ribi == ribi_t::all  ||  ribi_t::is_threeway(str_ribi)  ) {
				// It's a intersection.
				if(  test_index == 0  ||  test_index == route.get_count() - 1  ) {
					// cannot calculate prev_dir or next_dir
					return false;
				}
				ribi_t::ribi prev_dir = vehicle_base_t::calc_direction(welt->lookup(route.at(test_index-1))->get_pos(),welt->lookup(route.at(test_index))->get_pos());
				ribi_t::ribi next_dir = vehicle_base_t::calc_direction(welt->lookup(route.at(test_index))->get_pos(),welt->lookup(route.at(test_index+1))->get_pos());
				ribi_t::ribi str_left = (ribi_t::rotate90l(prev_dir) & str_ribi) == 0 ? prev_dir : ribi_t::rotate90l(prev_dir);
				ribi_t::ribi str_right = (ribi_t::rotate90(prev_dir) & str_ribi) == 0 ? prev_dir : ribi_t::rotate90(prev_dir);
				if(  next_dir == str_left  &&  (lane_affinity_sign & 1) != 0  ) {
					// fix to left lane
					if(  welt->get_settings().is_drive_left()  ) {
						lane_affinity = -1;
					}
					else {
						lane_affinity = 1;
					}
					lane_affinity_end_index = test_index;
					return true;
				}
				else if(  next_dir == str_right  &&  (lane_affinity_sign & 2) != 0  ) {
					// fix to right lane
					if(  welt->get_settings().is_drive_left()  ) {
						lane_affinity = 1;
					}
					else {
						lane_affinity = -1;
					}
					lane_affinity_end_index = test_index;
					return true;
				}
				else {
					return false;
				}
			}
			test_index++;
		}
	}
	return false;
}

void convoi_t::refresh(sint8 prev_tiles_overtaking, sint8 current_tiles_overtaking) {
	if(  fahr[0]  &&  fahr[0]->get_waytype()==road_wt  &&  (prev_tiles_overtaking==0)^(current_tiles_overtaking==0)  ){
		for(uint8 i=0; i<anz_vehikel; i++) {
			road_vehicle_t* rv = dynamic_cast<road_vehicle_t*>(fahr[i]);
			if(rv) {
				rv->refresh();
			}
		}
	}
}

bool convoi_t::get_next_cross_lane() {
	if(  !next_cross_lane  ) {
		// no request
		return false;
	}
	// next_cross_lane is true. Is the request obsolete?
	sint64 diff = welt->get_ticks() - request_cross_ticks;
	// If more than 8 sec, it's obsolete.
	if(  diff>8000*16/welt->get_time_multiplier()  ) {
		next_cross_lane = false;
	}
	return next_cross_lane;
}

void convoi_t::set_next_cross_lane(bool n) {
	if(  !n  ) {
		next_cross_lane = false;
		request_cross_ticks = 0;
		return;
	} else if(  next_cross_lane  ) {
		return;
	}
	// check time
	sint64 diff = welt->get_ticks() - request_cross_ticks;
	if(  request_cross_ticks==0  ||  diff<0  ||  diff>10000*16/welt->get_time_multiplier()  ) {
		next_cross_lane = true;
		request_cross_ticks = welt->get_ticks();
	}
}

void convoi_t::request_longblock_signal_judge(signal_t *sig, uint16 next_block) {
	longblock_signal_request.sig = sig;
	longblock_signal_request.next_block = next_block;
	longblock_signal_request.valid = true;
}

void convoi_t::clear_reserved_tiles(){
	if(  reserved_tiles.get_count()==0  ) {
		// nothing to do.
		return;
	}
	// determine next_reservation_index
	for(  sint32 i=route.get_count()-1;  i>=0;  i--  ) {
		if(  reserved_tiles.is_contained(route.at(i))  ) {
			// set next_reservation_index
			set_next_reservation_index(i);
			break;
		}
	}
	// unreserve all tiles that are not in route
	for(  uint32 i=0;  i<reserved_tiles.get_count();  i++  ) {
		if(  !route.is_contained(reserved_tiles[i])  ) {
			// unreserve the tile
			grund_t* gr = welt->lookup(reserved_tiles[i]);
			schiene_t* sch = gr ? (schiene_t*)gr->get_weg(front()->get_waytype()) : NULL;
			if(  sch  ) {
				sch->unreserve(self);
			}
		}
	}
	reserved_tiles.clear();
}

// this function should be called from rail vehicles
void convoi_t::calc_crossing_reservation() {
	crossing_reservation_index.clear();
	for(  uint32 i=route.get_count()-1;  i>0;  i--  ) {
		const grund_t *gr = welt->lookup(route.at(i));
		const schiene_t *sch1 = dynamic_cast<schiene_t*>(gr ? gr->get_weg(front()->get_waytype()) : NULL);
		const crossing_t* cr = (sch1  &&  sch1->is_crossing()) ? gr->find<crossing_t>(2) : NULL;
		if(  !cr  ) {
			// this tile is not a crossing.
			continue;
		}
		// calculate distance to reserve
		const weg_t* w1 = gr->get_weg_nr(0);
		const weg_t* w2 = gr->get_weg_nr(1);
		if(  !w1  ||  !w2  ) {
			// way does not exist?
			continue;
		}
		uint16 max_speed_1 = w1->get_waytype()==front()->get_waytype() ? w1->get_max_speed() : w2->get_max_speed();
		max_speed_1 = min(max_speed_1, speed_to_kmh(min_top_speed));
		const uint16 max_speed_2 = w1->get_waytype()==front()->get_waytype() ? w2->get_max_speed() : w1->get_max_speed();
		// max_speed_2 can be 0. ex) crossing with a narrow river
		const uint8 distance = max_speed_2>0 ? cr->get_length()*max_speed_1/max_speed_2 : 1;
		const uint16 res_start_idx = max(i-distance,1);
		crossing_reservation_index.append(std::pair<uint16, uint16>(res_start_idx,i));
	}
}


bool convoi_t::couple_convoi(convoihandle_t coupled) {
	coupled->set_state(COUPLED_LOADING);
	set_state(LOADING);
	coupling_convoi = coupled;
	coupling_convoi->front()->set_leading(false);
	back()->set_last(false);
	must_recalc_min_top_speed();
	return true;
}

convoihandle_t convoi_t::uncouple_convoi() {
	if(  !coupling_convoi.is_bound()  ) {
		return convoihandle_t();
	}
	convoihandle_t ret = coupling_convoi;
	coupling_convoi->set_state(is_loading() ? LOADING : ROUTING_1);
	coupling_convoi->front()->set_leading(true);
	back()->set_last(true);
	must_recalc_min_top_speed();
	// for child convoy, recalculate is_electric and min_top_speed immediately.
	coupling_convoi->check_electrification();
	const sint32 mts = coupling_convoi->calc_min_top_speed();
	convoihandle_t c = coupling_convoi->get_coupling_convoi();
	// broadcast min_top_speed
	while(  c.is_bound()  ) {
		c->set_min_top_speed(mts);
		c = c->get_coupling_convoi();
	}
	coupling_convoi = convoihandle_t();
	return ret;
}

bool convoi_t::can_continue_coupling() const {
	if(  !coupling_convoi.is_bound()  ) {
		// this convoy is not coupling with others!
		return false;
	}
	// Do the next entries have same position?
	const schedule_entry_t t = schedule->get_next_entry();
	const schedule_entry_t c = coupling_convoi->get_schedule()->get_next_entry();
	if(  t.pos!=c.pos  ) {
		return false;
	}
	return true;
}

bool convoi_t::can_start_coupling(convoi_t* parent) const {
	/* conditions to continue coupling
	* 1) next schedule entry is same except for coupling_point parameter.
	* 2) The next planned platform has adequate length for entire convoy length.
	* 3) current schedule entry is same except for coupling_point parameter
	* 4) current schedule entry has appropriate coupling_point for both convoys
	* 5) The coming platform has adequate length for entire convoy.
	*/
	const schedule_entry_t t_c = schedule->get_current_entry();
	const schedule_entry_t p_c = parent->get_schedule()->get_current_entry();
	const schedule_entry_t t_n = schedule->get_next_entry();
	const schedule_entry_t p_n = parent->get_schedule()->get_next_entry();
	
	if(  p_c.coupling_point!=1  ||  t_c.coupling_point!=2  ) {
		// rejected by coupling_point condition.
		return false;
	}
	// Is current and next entry same?
	if(  t_c.pos!=p_c.pos  ||  t_n.pos!=p_n.pos  ) {
		return false;
	}
	// Does the current and next platform has adequate length?
	return true;
}

bool convoi_t::is_waiting_for_coupling() const {
	convoihandle_t c = self;
	bool waiting_for_coupling = false;
	while(  c.is_bound()  ) {
		waiting_for_coupling |= (!c->get_coupling_convoi().is_bound()  &&  c->get_schedule()->get_current_entry().coupling_point==1);
		c = c->get_coupling_convoi();
	}
	return waiting_for_coupling;
}

bool convoi_t::check_electrification() {
	is_electric = false;
	convoihandle_t c = self;
	while(  c.is_bound()  ) {
		for(uint8 i=0;  i<c->get_vehicle_count();  i++) {
			is_electric |= c->get_vehikel(i)->get_desc()->get_engine_type()==vehicle_desc_t::electric;
		}
		c = c->get_coupling_convoi();
	}
	return is_electric;
}

sint32 convoi_t::calc_min_top_speed() {
	min_top_speed = SPEED_UNLIMITED;
	convoihandle_t c = self;
	while(  c.is_bound()  ) {
		for(uint8 i=0; i<c->get_vehicle_count(); i++) {
			min_top_speed = min(min_top_speed, kmh_to_speed( c->get_vehikel(i)->get_desc()->get_topspeed() ) );
		}
		c = c->get_coupling_convoi();
	}
	return min_top_speed;
}
