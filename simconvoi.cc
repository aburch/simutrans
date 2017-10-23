/**
 * convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#include <stdlib.h>
#include <algorithm>

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

#include "bauer/vehikelbauer.h"

#include "descriptor/vehicle_desc.h"
#include "descriptor/roadsign_desc.h"
#include "descriptor/building_desc.h"

#include "dataobj/schedule.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/replace_data.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"
#include "dataobj/livery_scheme.h"

#include "display/viewport.h"

#include "obj/crossing.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"
#include "obj/signal.h"

#include "vehicle/simvehicle.h"
#include "vehicle/overtaker.h"

#include "utils/simstring.h"
#include "utils/cbuffer_t.h"

#include "convoy.h"

#include "descriptor/goods_desc.h"

#ifdef MULTI_THREAD
#include "utils/simthread.h"
static pthread_mutex_t step_convois_mutex = PTHREAD_MUTEX_INITIALIZER;
static vector_tpl<pthread_t> unreserve_threads;
static pthread_attr_t thread_attributes;
waytype_t convoi_t::current_waytype = road_wt; 
uint16 convoi_t::current_unreserver = 0;
#endif

//#if _MSC_VER
//#define snprintf _snprintf
//#endif

/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 500
#define WAIT_INFINITE 9223372036854775807ll

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
	"ROUTING_2",
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
	"ENTERING_DEPOT",
	"REVERSING",
	"OUT_OF_RANGE",
	"EMERGENCY_STOP",
	"JUST_FOUND_ROUTE"
};

// Reset some values.  Used in init and replacing.
void convoi_t::reset()
{
	is_electric = false;
	//sum_gesamtweight = sum_weight = 0;
	previous_delta_v = 0;

	withdraw = false;
	has_obsolete = false;
	no_load = false;
	depot_when_empty = false;
	reverse_schedule = false;

	jahresgewinn = 0;

	set_akt_speed(0);          // momentane Geschwindigkeit / current speed
	sp_soll = 0;
	//brake_speed_soll = 2147483647; // ==SPEED_UNLIMITED

	highest_axle_load = 0;
	longest_min_loading_time = 0;
	longest_max_loading_time = 0;
	current_loading_time = 0;
	clear_departures();
	for(uint8 i = 0; i < MAX_CONVOI_COST; i ++)
	{
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}
}

void convoi_t::init(player_t *player)
{
	owner = player;

	schedule = NULL;
	replace = NULL;
	fpl_target = koord3d::invalid;
	line = linehandle_t();

	reset();

	vehicle_count = 0;
	steps_driven = -1;
	wait_lock = 0;
	go_on_ticks = WAIT_INFINITE;

	jahresgewinn = 0;
	total_distance_traveled = 0;
	steps_since_last_odometer_increment = 0;

	//distance_since_last_stop = 0;
	//sum_speed_limit = 0;
	//maxspeed_average_count = 0;
	next_reservation_index = 0;

	alte_direction = ribi_t::none;
	next_wolke = 0;

	state = INITIAL;

	*name_and_id = 0;
	name_offset = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;
	free_seats = 0;

	//speed_limit = SPEED_UNLIMITED;
	//brake_speed_soll = SPEED_UNLIMITED;
	akt_speed_soll = 0;             // target speed
	set_akt_speed(0);                 // momentane Geschwindigkeit
	sp_soll = 0;

	next_stop_index = INVALID_INDEX;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;
	last_signal_pos = koord3d::invalid;
	last_stop_id = 0;

	reversable = false;
	reversed = false;
	re_ordered = false;

	livery_scheme_index = 0;

	needs_full_route_flush = false;
}

convoi_t::convoi_t(loadsave_t* file) : vehicle(max_vehicle, NULL)
{
	self = convoihandle_t();
	init(0);
	replace = NULL;
	is_choosing = false;
	max_signal_speed = SPEED_UNLIMITED;

	no_route_retry_count = 0;
	rdwr(file);
	current_stop = schedule == NULL ? 255 : schedule->get_current_stop() - 1;

	// Added by : Knightly
	old_schedule = NULL;
	free_seats = 0;
	recalc_catg_index();
	has_obsolete = calc_obsolescence(welt->get_timeline_year_month());

	validate_vehicle_summary();
	validate_adverse_summary();
	validate_freight_summary();
	validate_weight_summary();
	get_continuous_power();
	get_starting_force();
	get_braking_force();
}

convoi_t::convoi_t(player_t* player) : vehicle(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	player->book_convoi_number(1);
	init(player);
	set_name( "Unnamed" );
	welt->add_convoi( self );
	init_financial_history();
	current_stop = 255;
	is_choosing = false;
	max_signal_speed = SPEED_UNLIMITED;

	// Added by : Knightly
	old_schedule = NULL;
	free_seats = 0;

	livery_scheme_index = 0;

	no_route_retry_count = 0;
}


convoi_t::~convoi_t()
{
	owner->book_convoi_number( -1);

	assert(self.is_bound());
	assert(vehicle_count==0);

	close_windows();

DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_viewport()->get_follow_convoi()==self) {
		welt->get_viewport()->set_follow_convoi( convoihandle_t() );
	}

	welt->sync.remove( this );
	welt->rem_convoi( self );

	clear_estimated_times();

	// Knightly : if lineless convoy -> unregister from stops
	if(  !line.is_bound()  ) {
		unregister_stops();
	}

	// force asynchronous recalculation
	if(schedule) {
		if(!schedule->is_editing_finished()) {
			destroy_win((ptrdiff_t)schedule);
		}

		if (!schedule->empty() && !line.is_bound())
		{
			// New method - recalculate as necessary

			// Added by : Knightly
			haltestelle_t::refresh_routing(schedule, goods_catg_index, owner);
		}
		delete schedule;
	}

	clear_replace();

	// @author hsiegeln - deregister from line (again) ...
	unset_line();

	self.detach();
}

void convoi_t::close_windows()
{
	// close windows
	destroy_win( magic_convoi_info+self.get_id() );
	destroy_win( magic_convoi_detail+self.get_id() );
	destroy_win( magic_replace+self.get_id() );
}

// waypoint: no stop, resp. for airplanes in air (i.e. no air strip below)
bool convoi_t::is_waypoint( koord3d ziel ) const
{
		if (vehicle[0]->get_waytype() == air_wt) {
		grund_t *gr = welt->lookup_kartenboden(ziel.get_2d());
		if(  gr == NULL  ||  gr->get_weg(air_wt) == NULL  ) {
			return true;
		}
	}
	
	const grund_t* gr = welt->lookup(ziel);
	return !haltestelle_t::get_halt(ziel,get_owner()).is_bound() && !(gr && gr->get_depot());
}

#ifdef MULTI_THREAD

void convoi_t::unreserve_route_range(route_range_specification range)
{
	const vector_tpl<weg_t *> &all_ways = weg_t::get_alle_wege();
	for (uint32 i = range.start; i < range.end; i++)
	{
		weg_t* const way = all_ways[i];
		if (way->get_waytype() == convoi_t::current_waytype)
		{
			//schiene_t* const sch = obj_cast<schiene_t>(way);
			schiene_t* const sch = way->is_rail_type() ? (schiene_t*)way : NULL;
			if (sch && sch->get_reserved_convoi().get_id() == convoi_t::current_unreserver)
			{
				convoihandle_t ch;
				ch.set_id(convoi_t::current_unreserver);
				sch->unreserve(ch);
			}
		}
	}
}

#endif

/**
 * unreserves the whole remaining route
 */
void convoi_t::unreserve_route()
{
	// Clears all reserved tiles on the whole map belonging to this convoy.
#ifdef MULTI_THREAD_ROUTE_UNRESERVER

	current_unreserver = self.get_id();
	current_waytype = front()->get_waytype();

	simthread_barrier_wait(&karte_t::unreserve_route_barrier);
	simthread_barrier_wait(&karte_t::unreserve_route_barrier);

	current_unreserver = 0;
	current_waytype = invalid_wt;
	
#else
	FOR(vector_tpl<weg_t*>, const way, weg_t::get_alle_wege())
	{
		if(way->get_waytype() == front()->get_waytype())
		{
			//schiene_t* const sch = obj_cast<schiene_t>(way);
			schiene_t* const sch = way->is_rail_type() ? (schiene_t*)way : NULL;
			if(sch && sch->get_reserved_convoi() == self)
			{
				sch->unreserve(front());
			}
		}
	}
#endif

	set_needs_full_route_flush(false);
}

void convoi_t::reserve_own_tiles()
{
	if(vehicle_count > 0) 
	{
		for (unsigned i = 0; i != vehicle_count; ++i)
		{
			vehicle_t& v = *vehicle[i];
			grund_t* gr = welt->lookup(v.get_pos());
			if(gr)
			{
				if(schiene_t *sch = (schiene_t *)gr->get_weg(front()->get_waytype())) 
				{
					if(!route.empty())
					{
						sch->reserve(self, ribi_type( route.at(max(1u,1)-1u), route.at(min(route.get_count()-1u,0+1u))));
					}
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
	for (unsigned i = 0; i != vehicle_count; ++i) {
		vehicle_t& v = *vehicle[i];

		if (grund_t* gr = welt->lookup(v.get_pos())) {
			v.mark_image_dirty(v.get_image(), 0);
			v.leave_tile();
			// maybe unreserve this
			weg_t* const way = v.get_weg(); 
			schiene_t* const rails = obj_cast<schiene_t>(way);
			if(rails) 
			{
				if(state != REVERSING)
				{
					rails->unreserve(&v);
				}
			}

			if (gr)
			{
				// It is not clear why this is necessary in
				// Extended but not in Standard, but without
				// this, remnents of images will appear
				// whenever vehicles move off or reverse. 
				gr->mark_image_dirty();
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
#ifdef MULTI_THREAD
	world()->stop_path_explorer();
#endif
	if(schedule==NULL) {
		if(  state!=INITIAL  ) {
			emergency_go_to_depot();
			return;
		}
		// anyway reassign convoi pointer ...
		for( uint8 i=0;  i<vehicle_count;  i++ ) {
			vehicle[i]->set_convoi(this);
			// This call used to update the fixed monthly maintenance costs; this was deleted due to
			// the fact that these change each month due to depreciation, so this doesn't work.
			// It would be good to restore it.
			// --neroden
			// vehicle[i]->finish_rd();
		}
		return;
	}
	else {
		// restore next schedule target for non-stop waypoint handling
		const koord3d ziel = schedule->get_current_eintrag().pos;
		if(  is_waypoint(ziel)  ) {
			fpl_target = ziel;
		}
	}

	bool realing_position = false;
	if(  vehicle_count>0  ) {
		DBG_MESSAGE("convoi_t::finish_rd()","state=%s, next_stop_index=%d", state_names[state], next_stop_index );

	const uint32 max_route_index = get_route() ? get_route()->get_count() - 1 : 0;

	// only realign convois not leaving depot to avoid jumps through signals
		if(  steps_driven!=-1  ) {
			for( uint8 i=0;  i<vehicle_count;  i++ ) {
				vehicle_t* v = vehicle[i];
				if(v->get_route_index() > max_route_index && max_route_index > 0 && i > 0)
				{
					dbg->error("convoi_t::finish_rd()", "Route index is %i, whereas maximum route index is %i for convoy %i", v->get_route_index(), max_route_index, self.get_id());
					v->set_route_index(front()->get_route_index());
				}
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
				vehicle_t* v = vehicle[i];
				/*if(v->get_route_index() > max_route_index && max_route_index > 0 && i > 0)
				{
					dbg->error("convoi_t::finish_rd()", "Route index is %i, whereas maximum route index is %i for convoy %i", v->get_route_index(), max_route_index, self.get_id());
					v->set_route_index(front()->get_route_index());
				}*/
				v->set_leading( i==0 );
				v->set_last( i+1==vehicle_count );
				v->calc_height();
				// this sets the convoi and will renew the block reservation, if needed!
				v->set_convoi(this);

				// wrong alignment here => must relocate
				if(v->need_realignment()) {
					// diagonal => convoi must restart
					realing_position |= ribi_t::is_bend(v->get_direction())  &&  (state==DRIVING  ||  is_waiting());
				}
				// if version is 99.17 or lower, some convois are broken, i.e. had too large gaps between vehicles
				/*if(  !realing_position  &&  state!=INITIAL  &&  state!=LEAVING_DEPOT  ) {
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
						DBG_MESSAGE("convoi_t::finish_rd()", "v: pos(%s) steps(%d) len=%d ribi=%d prev (%s) step(%d)", v->get_pos().get_str(), v->get_steps(), v->get_desc()->get_length()*16, v->get_direction(),  drive_pos.get_2d().get_str(), step_pos);
						/*
						// This causes convoys re-loading from diagonal tiles to fail in Simutrans-Extended
						// and no longer seems to be necessary. This might conceivably also cause network
						// desyncs in some cases, although this has not been tested. 
						if(  abs( v->get_steps() - step_pos )>15  ) {
							// not where it should be => realing
							realing_position = true;
							dbg->warning( "convoi_t::finish_rd()", "convoi (%s) is broken => realign", get_name() );
						} // /*
					}
					step_pos -= v->get_desc()->get_length_in_steps();
					drive_pos = v->get_pos();
				}*/
			}
		}
DBG_MESSAGE("convoi_t::finish_rd()","next_stop_index=%d", next_stop_index );

		linehandle_t new_line = line;
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
				line->add_convoy(self, true);
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
	if(realing_position  &&  vehicle_count>1) {
		// display just a warning
		DBG_MESSAGE("convoi_t::finish_rd()","cnv %i is currently too long.",self.get_id());

		if (route.empty()) {
			// realigning needs a route
			state = NO_ROUTE;
			owner->report_vehicle_problem( self, koord3d::invalid );
			dbg->error( "convoi_t::finish_rd()", "No valid route, but needs realignment at (%s)!", front()->get_pos().get_str() );
		}
		else {
			// since start may have been changed
			const uint8 v_count = vehicle_count - 1;
			uint16 last_route_index = vehicle[v_count]->get_route_index();
			if(last_route_index > route.get_count() - 1 && v_count > 0)
			{
				last_route_index = 0;
				dbg->warning("convoi_t::finish_rd()", "Convoy %i's route index is out of range: resetting to zero", self.get_id());
			}
			uint16 start_index = min(max(1u, vehicle[vehicle_count - 1u]->get_route_index() - 1u), route.get_count() - 1u); 

			uint32 train_length = move_to(start_index) + 1;
			const koord3d last_start = front()->get_pos();

			// now advance all convoi until it is completely on the track
			front()->set_leading(false); // switches off signal checks ...
			for(unsigned i=0; i<vehicle_count; i++) {
				vehicle_t* v = vehicle[i];

				v->get_smoke(false); //"Allowed to smoke" (Google)
				vehicle[i]->do_drive( (VEHICLE_STEPS_PER_CARUNIT*train_length)<<YARDS_PER_VEHICLE_STEP_SHIFT );
				train_length -= v->get_desc()->get_length();
				v->get_smoke(true);

				// eventually reserve this again
				grund_t *gr=welt->lookup(v->get_pos());
				// airplanes may have no ground ...
				if(gr)
				{
					if (schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(vehicle[i]->get_waytype()))) {
						sch0->reserve(self, ribi_t::none);
					}
				}
			}
			front()->set_leading(true);
			if(  state != INITIAL  &&  state != EDIT_SCHEDULE  &&  front()->get_pos() != last_start  ) {
				state = WAITING_FOR_CLEARANCE;
			}
		}
	}
	// when saving with open window, this can happen
	if(  state==EDIT_SCHEDULE  ) {
		if (env_t::networkmode) {
			wait_lock = 30000; // milliseconds to drive on, if the client in question had left
		}
		schedule->finish_editing();
	}
	// remove wrong freight
	check_freight();
	// some convois had wrong old direction in them
	if(  state<DRIVING  ||  state==LOADING  ) {
		alte_direction = front()->get_direction();
	}
	// Knightly : if lineless convoy -> register itself with stops
	if (state > INITIAL)
	{
		if (!line.is_bound()) {
			register_stops();
		}
	}

	for(int i = 0; i < vehicle_count; i++)
	{
		vehicle[i]->remove_stale_cargo();
	}

	loading_limit = schedule->get_current_eintrag().minimum_loading;
	if(state == LOADING)
	{
		laden();
	}
}


// since now convoi states go via tool_t
void convoi_t::call_convoi_tool( const char function, const char *extra)
{
	tool_t *tool = create_tool( TOOL_CHANGE_CONVOI | SIMPLE_TOOL );
	cbuffer_t param;
	param.printf("%c,%u", function, self.get_id());
	if(  extra  &&  *extra  ) {
		param.printf(",%s", extra);
	}
	tool->set_default_param(param);
	welt->set_tool( tool, get_owner() );
	// since init always returns false, it is safe to delete immediately
	delete tool;
}


void convoi_t::rotate90( const sint16 y_size )
{
	home_depot.rotate90( y_size );
	last_signal_pos.rotate90(y_size); 
	route.rotate90( y_size );
	if(schedule) {
		schedule->rotate90( y_size );
	}
	for(  int i=0;  i<vehicle_count;  i++  ) {
		vehicle[i]->rotate90_freight_destinations( y_size );
	}
	// eventually correct freight destinations (and remove all stale freight)
	check_freight();
}


/**
 * Gibt die Position des Convois zurück.
 * @return Position des Convois
 * @author Hj. Malthaner
 */
koord3d convoi_t::get_pos() const
{
	if(vehicle_count > 0 && front()) {
		return state==INITIAL ? home_depot : front()->get_pos();
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
	convoi_detail_t *detail = dynamic_cast<convoi_detail_t*>(win_get_magic( magic_convoi_detail+self.get_id()));
	if (detail) {
		detail->update_data();
	}
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
		len += vehicle[i]->get_desc()->get_length();
	}
	return len;
}


/**
 * convoi add their running cost for travelling one tile
 * @author Hj. Malthaner
 */
void convoi_t::add_running_cost(sint64 cost, const weg_t *weg)
{
	jahresgewinn += cost;

	if(weg && weg->get_owner() != get_owner() && weg->get_owner() != NULL && (!welt->get_settings().get_toll_free_public_roads() || (weg->get_waytype() != road_wt || weg->get_player_nr() != 1)))
	{
		// running on non-public way costs toll (since running costas are positive => invert)
		sint32 toll = -(cost * welt->get_settings().get_way_toll_runningcost_percentage()) / 100l;
		if(welt->get_settings().get_way_toll_waycost_percentage())
		{
			if(weg->is_electrified() && needs_electrification())
			{
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
			toll += (weg->get_desc()->get_maintenance() * welt->get_settings().get_way_toll_waycost_percentage()) / 100l;
		}
		weg->get_owner()->book_toll_received( toll, get_schedule()->get_waytype() );
		get_owner()->book_toll_paid(         -toll, get_schedule()->get_waytype() );
//		book( -toll, CONVOI_WAYTOLL);
		book( -toll, CONVOI_PROFIT);
	}

	get_owner()->book_running_costs( cost, get_schedule()->get_waytype());
	book( cost, CONVOI_OPERATIONS );
	book( cost, CONVOI_PROFIT );
}

void convoi_t::increment_odometer(uint32 steps)
{ 
	steps_since_last_odometer_increment += steps;
	if (steps_since_last_odometer_increment < welt->get_settings().get_steps_per_km())
	{
		return;
	}
	
	// Increment the way distance: used for apportioning revenue by owner of ways.
	// Use steps, as only relative distance is important here.
	sint8 player;
	waytype_t waytype = front()->get_waytype();
	weg_t* way = front()->get_weg();
	if(way == NULL)
	{
		player = owner->get_player_nr();
	}
	else
	{
		const player_t* owner = way->get_owner(); 
		if(waytype == road_wt && owner && owner->is_public_service() && welt->get_settings().get_toll_free_public_roads())
		{
			player = owner->get_player_nr();
		}
		else
		{
			player = way->get_player_nr();
		}
	}

	if(player < 0)
	{
		player = owner->get_player_nr();
	}

	FOR(departure_map, &i, departures)
	{
		i.value.increment_way_distance(player, steps_since_last_odometer_increment);
	}

	const sint64 km = steps_since_last_odometer_increment / welt->get_settings().get_steps_per_km();
	book( km, CONVOI_DISTANCE );
	total_distance_traveled += km;
	steps_since_last_odometer_increment -= km * steps_since_last_odometer_increment;
	koord3d pos = koord3d::invalid;
	weg_t* weg = NULL;
	sint32 running_cost = 0;
	bool must_add = false;
	for(uint8 i= 0; i < vehicle_count; i++) 
	{
		const vehicle_t& v = *vehicle[i];
		if (v.get_pos() != pos)
		{
			if (must_add)
			{
				add_running_cost(running_cost, weg);
			}
			pos = v.get_pos();
			weg = v.get_weg();
			running_cost = 0;
		}
		must_add = true;
		running_cost -= v.get_desc()->get_running_cost(welt);
	}

	if (waytype == air_wt)
	{
		// Halve the running cost if we are circling or taxiing.
		air_vehicle_t* aircraft = (air_vehicle_t*)front(); 
		if (!aircraft->is_using_full_power())
		{
			running_cost /= 2;
		}
	}

	if (must_add)
	{
		add_running_cost(running_cost, weg);
	}
}

bool convoi_t::has_tall_vehicles()
{
	bool is_tall = false;

	for (uint32 i = 0; i < vehicle_count; i++)
	{
		is_tall |= get_vehicle(i)->get_desc()->get_is_tall();
		if (is_tall)
		{
			break;
		}
	}
	return is_tall;
}

// BG, 06.11.2011
bool convoi_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed)
{
	route_infos.clear();
	const grund_t* gr = welt->lookup(ziel);
	if(gr && front()->get_waytype() == air_wt && gr->get_halt().is_bound() && welt->lookup(ziel)->get_halt()->has_no_control_tower())
	{
		return false;
	}

	bool success = front()->calc_route(start, ziel, max_speed, has_tall_vehicles(), &route);
	rail_vehicle_t* rail_vehicle = NULL;
	switch(front()->get_waytype())
	{
	case track_wt:
	case narrowgauge_wt:
	case tram_wt:
	case monorail_wt:
	case maglev_wt:
		rail_vehicle = (rail_vehicle_t*)front();
		if(rail_vehicle->get_working_method() == token_block || rail_vehicle->get_working_method() == one_train_staff)
		{
			// If we calculate a new route while in token block, we must remember this
			// so that, when it comes to clearing the route, a full flush can be performed
			// rather than simply iterating back over the route, which will not be enough
			// in this case.
			needs_full_route_flush = true;
		}
	default:
		return success;
	};
	return success;
}

void convoi_t::update_route(uint32 index, const route_t &replacement)
{
	// replace route with replacement starting at index.
	route_infos.clear();
	route.remove_koord_from(index);
	route.append(&replacement);
}

void convoi_t::replace_route(const route_t &replacement)
{
	route_infos.clear();
	route.clear();
	route.append(&replacement);
}


// BG, 06.11.2011
inline weg_t *get_weg_on_grund(const grund_t *grund, const waytype_t waytype)
{
	return grund != NULL ? grund->get_weg(waytype) : NULL;
}

/* Calculates (and sets) new akt_speed and sp_soll
 * needed for driving, entering and leaving a depot)
 */
void convoi_t::calc_acceleration(uint32 delta_t)
{
	// existing_convoy_t is designed to become a part of convoi_t.
	// There it will help to minimize updating convoy summary data.
	vehicle_t &front = *this->front();

	const uint32 route_count = route.get_count(); // at least ziel will be there, even if calculating a route failed.
	const uint16 current_route_index = front.get_route_index(); // actually this is current route index + 1!!!

	/*
	 * get next speed limit of my route.
	 */
#ifndef DEBUG_PHYSICS
	sint32
#endif
	next_speed_limit = 0; // 'limit' for next stop is 0, of course.
	uint32 next_stop_index = get_next_stop_index(); // actually this is next stop index + 1!!!
	if(next_stop_index >= 65000u) // BG, 07.10.2011: currently only rail_vehicle_t sets next_stop_index.
	// BG, 09.08.2012: use ">= 65000u" as INVALID_INDEX (65530u) sometimes is incermented or decremented.
	{
		next_stop_index = route_count;
	}
#ifndef DEBUG_PHYSICS
	sint32 steps_til_limit;
	sint32 steps_til_brake;
#endif
	const sint32 brake_steps = calc_min_braking_distance(welt->get_settings(), get_weight_summary(), akt_speed);
	// use get_route_infos() for the first time accessing route_infos to eventually initialize them.
	const uint32 route_infos_count = get_route_infos().get_count();
	if(route_infos_count > 0 && route_infos_count >= next_stop_index && next_stop_index > current_route_index)
	{
		sint32 i = current_route_index - 1;
		if(i < 0)
		{
			i = 0;
		}
		const convoi_t::route_info_t &current_info = route_infos.get_element(i);
		if(current_info.speed_limit != vehicle_t::speed_unlimited())
		{
			update_max_speed(speed_to_kmh(current_info.speed_limit));
		}
		const convoi_t::route_info_t &limit_info = route_infos.get_element(next_stop_index - 1);
		steps_til_limit = route_infos.calc_steps(current_info.steps_from_start, limit_info.steps_from_start);
		steps_til_brake = steps_til_limit - brake_steps;
		switch(limit_info.direction)
		{
			case ribi_t::north:
			case ribi_t::west:
				// BG, 10-11-2011: vehicle_t::hop() reduces the length of the tile, if convoy is going to stop on the tile.
				// Most probably for eye candy reasons vehicles do not exactly move on their tiles.
				// We must do the same here to avoid abrupt stopping.
				sint32 	delta_tile_len = current_info.steps_from_start;
				if(i > 0) delta_tile_len -= route_infos.get_element(i - 1).steps_from_start;
				delta_tile_len -= (delta_tile_len/2) + 1;
				steps_til_limit -= delta_tile_len;
				steps_til_brake -= delta_tile_len;
				break;
		}
#ifdef DEBUG_ACCELERATION
		static const char *debug_fmt1 = "%d) at tile% 4u next limit of% 4d km/h, current speed% 4d km/h,% 6d steps til brake,% 6d steps til stop";
		dbg->warning("convoi_t::calc_acceleration 1", debug_fmt1, current_route_index - 1, next_stop_index, speed_to_kmh(next_speed_limit), speed_to_kmh(akt_speed), steps_til_brake, steps_til_limit);
#endif
		// Brake for upcoming speed limit?
		sint32 min_limit = akt_speed; // no need to check limits above min_limit, as it won't lead to further restrictions
		sint32 steps_from_start = current_info.steps_from_start; // speed has to be reduced before entering the tile. Thus distance from start has to be taken from previous tile.
		for(i++; i < next_stop_index; i++)
		{
			const convoi_t::route_info_t &limit_info = route_infos.get_element(i);
			if(limit_info.speed_limit < min_limit)
			{
				min_limit = limit_info.speed_limit;
				const sint32 limit_steps = brake_steps - calc_min_braking_distance(welt->get_settings(), get_weight_summary(), limit_info.speed_limit);
				const sint32 route_steps = route_infos.calc_steps(current_info.steps_from_start, steps_from_start);
				const sint32 st = route_steps - limit_steps;

				if(steps_til_brake > st)
				{
					next_speed_limit = limit_info.speed_limit;
					steps_til_limit = route_steps;
					steps_til_brake = st;
#ifdef DEBUG_ACCELERATION
					dbg->warning("convoi_t::calc_acceleration 2", debug_fmt1, current_route_index - 1, i, speed_to_kmh(next_speed_limit), speed_to_kmh(akt_speed), steps_til_brake, steps_til_limit);
#endif
				}
			}
			steps_from_start = limit_info.steps_from_start;
		}
	}
	else
	{
		steps_til_limit = route_infos.calc_tiles((sint32) current_route_index, (sint32) next_stop_index) * VEHICLE_STEPS_PER_TILE;
		steps_til_brake = steps_til_limit - brake_steps;
	}
	sint32 steps_left_on_current_tile = (sint32)front.get_steps_next() + 1 - (sint32)front.get_steps();
	steps_til_brake += steps_left_on_current_tile;
	steps_til_limit += steps_left_on_current_tile;

	/*
	 * calculate movement in the next delta_t ticks.
	 */
	akt_speed_soll = min(get_min_top_speed(), max_signal_speed);
	calc_move(welt->get_settings(), delta_t, akt_speed_soll, next_speed_limit, steps_til_limit, steps_til_brake, akt_speed, sp_soll, v);
}

void convoi_t::route_infos_t::set_holding_pattern_indexes(sint32 current_route_index, sint32 touchdown_route_index)
{
	if (touchdown_route_index != INVALID_INDEX && current_route_index < touchdown_route_index - (HOLDING_PATTERN_LENGTH + HOLDING_PATTERN_OFFSET))
	{
		hp_start_index = touchdown_route_index - (HOLDING_PATTERN_LENGTH + HOLDING_PATTERN_OFFSET);
		hp_end_index   = hp_start_index + HOLDING_PATTERN_LENGTH;
		hp_start_step  = get_element(hp_start_index - 1).steps_from_start;
		hp_end_step    = get_element(hp_end_index   - 1).steps_from_start;
	}
	else
	{
		// no holding pattern correction, if aircraft passed the start of it.
		hp_start_index = -1;
		hp_end_index = -1;
		hp_start_step = -1;
		hp_end_step = -1;
	}
}


// extracted from convoi_t::calc_acceleration()
convoi_t::route_infos_t& convoi_t::get_route_infos()
{
	if (route_infos.get_count() == 0 && route.get_count() > 0)
	{
		vehicle_t &front = *this->front();
		const uint32 route_count = route.get_count(); // at least ziel will be there, even if calculating a route failed.
		const uint16 current_route_index = front.get_route_index(); // actually this is current route index + 1!!!
		fixed_list_tpl<sint16, 192> corner_data;
		const waytype_t waytype = front.get_waytype();

		// calc route infos
		route_infos.set_count(route_count);
		// The below may be a slight optimisation, but it causes uninitialised variables in aircraft in holding patterns.
		//uint32 i = min(max(0, current_route_index - 2), route_count - 1);
		uint32 i = 0;

		koord3d current_tile = route.at(i);
		convoi_t::route_info_t &start_info = route_infos.get_element(i);
		start_info.direction = front.get_direction();
		start_info.steps_from_start = 0;
		const weg_t *current_weg = get_weg_on_grund(welt->lookup(current_tile), waytype);
		start_info.speed_limit = front.calc_speed_limit(current_weg, NULL, &corner_data, start_info.direction, start_info.direction);

		sint32 takeoff_index = front.get_takeoff_route_index();
		sint32 touchdown_index = front.get_touchdown_route_index();
		for (i++; i < route_count; i++)
		{
			convoi_t::route_info_t &current_info = route_infos.get_element(i - 1);
			convoi_t::route_info_t &this_info = route_infos.get_element(i);
			const koord3d this_tile = route.at(i);
			const koord3d next_tile = route.at(min(i + 1, route_count - 1));
			this_info.speed_limit = welt->lookup_kartenboden(this_tile.get_2d())->is_water() ? vehicle_t::speed_unlimited() : kmh_to_speed(950); // Do not alow supersonic flight over land.
			this_info.steps_from_start = current_info.steps_from_start + front.get_tile_steps(current_tile.get_2d(), next_tile.get_2d(), this_info.direction);
			const weg_t *this_weg = get_weg_on_grund(welt->lookup(this_tile), waytype);
			if (i >= touchdown_index || i <= takeoff_index)
			{
				// not an aircraft (i <= takeoff_index == INVALID_INDEX == 65530u) or
				// aircraft on ground (not between takeoff_index and touchdown_index): get speed limit
				current_info.speed_limit = this_weg ? front.calc_speed_limit(this_weg, current_weg, &corner_data, this_info.direction, current_info.direction) : vehicle_t::speed_unlimited();
			}
			current_tile = this_tile;
			current_weg = this_weg;
		}
		route_infos.set_holding_pattern_indexes(current_route_index, touchdown_index);

	}
	return route_infos;
}


int convoi_t::get_vehicle_at_length(uint16 length)
{
	int current_length = 0;
	for( int i=0;  i<vehicle_count;  i++  ) {
		current_length += vehicle[i]->get_desc()->get_length();
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
		case ROUTING_2:
		case ROUTE_JUST_FOUND:
		case DUMMY5:
		case OUT_OF_RANGE:
		case NO_ROUTE:
		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
		case REVERSING:	
			// Hajo: this is an async task, see step() or threaded_step()
			break;

		case ENTERING_DEPOT:
			break;

		case LEAVING_DEPOT:
			{
				// ok, so we will accelerate
				calc_acceleration(delta_t);
				//moved to inside calc_acceleration(): sp_soll += (akt_speed*delta_t);
				// Make sure that the last_stop_pos is set here so as not
				// to skew average speed readings from vehicles emerging
				// from depots.
				// @author: jamespetts
				front()->last_stop_pos = front()->get_pos();
				last_stop_id = 0;

				// now actually move the units
				while(sp_soll>>12) {
					// Attempt to move one step.
					uint32 sp_hat = front()->do_drive(1<<YARDS_PER_VEHICLE_STEP_SHIFT);
					int v_nr = get_vehicle_at_length((++steps_driven)>>4);
					// stop when depot reached
					if(state==INITIAL  ||  state==ROUTING_1) {
						break;
					}
					// until all are moving or something went wrong (sp_hat==0)
					if(sp_hat==0  ||  v_nr==vehicle_count) {
						// Attempted fix of depot squashing problem:
						// but causes problems with signals.
						//if (v_nr==vehicle_count) {
							steps_driven = -1;
						//}
						//else {
						//}

 						state = DRIVING;
 						return SYNC_OK;
					}
					// now only the right numbers
					for(int i=1; i<=v_nr; i++) {
						vehicle[i]->do_drive(sp_hat);
					}
					sp_soll -= sp_hat;
				}
				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<vehicle_count;  i++  ) {
						vehicle[i]->make_smoke();
						vehicle[i]->last_stop_pos = vehicle[i]->get_pos();
					}
				}
			}

			clear_departures();

			break;	// LEAVING_DEPOT

		case DRIVING:
			{
				calc_acceleration(delta_t);
				//akt_speed = kmh_to_speed(50);

				// now actually move the units
				//moved to inside calc_acceleration():
				//sp_soll += (akt_speed*delta_t);

				// While sp_soll is a signed integer do_drive() accepts an unsigned integer.
				// Thus running backwards is impossible.  Instead sp_soll < 0 is converted to very large
				// distances and results in "teleporting" the convoy to the end of its pre-caclulated route.
				uint32 sp_hat = front()->do_drive(sp_soll < 0 ? 0 : sp_soll);
				// stop when depot reached ...
				if(state==INITIAL) {
					return SYNC_REMOVE;
				}
				// now move the rest (so all vehicle are moving synchroniously)
				for(unsigned i=1; i<vehicle_count; i++) {
					vehicle[i]->do_drive(max(1,sp_hat)); 
				}
				// maybe we have been stopped be something => avoid wide jumps
				sp_soll = (sp_soll-sp_hat) & 0x0FFF;

				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<vehicle_count;  i++  ) {
						vehicle[i]->make_smoke();
					}
				}
			}
			break;	// DRIVING

		case LOADING:
			// Hajo: loading is an async task, see laden()
			break;

		case WAITING_FOR_CLEARANCE:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
		case WAITING_FOR_CLEARANCE_TWO_MONTHS:
		case EMERGENCY_STOP:
			// Hajo: waiting is asynchronous => fixed waiting order and route search
			break;

		case SELF_DESTRUCT:
			// see step, since destruction during a screen update ma give stange effects
			break;

		default:
			dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
			break;
	}

	return SYNC_OK;
}

bool convoi_t::prepare_for_routing()
{
	if (vehicle_count > 0 && schedule)
	{
		koord3d start = front()->get_pos();
		koord3d ziel = schedule->get_current_eintrag().pos;
		const koord3d original_ziel = ziel;

		// Check whether the next stop is within range.
		if (min_range > 0)
		{
			int count = 0;
			const uint8 original_index = schedule->get_current_stop();
			const grund_t* gr = welt->lookup(ziel);
			const depot_t* depot = gr ? gr->get_depot() : NULL;
			while (count < schedule->get_count() && !haltestelle_t::get_halt(ziel, owner).is_bound() && !depot)
			{
				// The next stop is a waypoint - advance 
				reverse_schedule ? schedule->advance_reverse() : schedule->advance();
				ziel = schedule->get_current_eintrag().pos;
				count++;
			}
			uint16 distance;
			if (original_index == schedule->get_count() - 1 && schedule->is_mirrored())
			{
				// We do not want the distance from the end to the start in this case, but the distance from
				// end to the immediately previous stop
				
				distance = (shortest_distance(schedule->entries[schedule->get_count() - 1].pos.get_2d(), schedule->entries[schedule->get_count() - 2].pos.get_2d()) * welt->get_settings().get_meters_per_tile()) / 1000u;
			}
			else
			{
				distance = (shortest_distance(start.get_2d(), ziel.get_2d()) * welt->get_settings().get_meters_per_tile()) / 1000u;
			}
			schedule->set_current_stop(original_index);
			ziel = original_ziel;
			if (distance > min_range)
			{
				state = OUT_OF_RANGE;
				get_owner()->report_vehicle_problem(self, ziel);
				return false;
			}
		}

		halthandle_t destination_halt = haltestelle_t::get_halt(ziel, get_owner());
		halthandle_t this_halt = haltestelle_t::get_halt(start, get_owner());
		if (this_halt.is_bound() && this_halt == destination_halt)
		{
			// For some reason, the schedule has failed to advance. Advance it before calculating the route.
			reverse_schedule ? schedule->advance_reverse() : schedule->advance();
			ziel = schedule->get_current_eintrag().pos;
		}

		// avoid stopping midhalt
		if (start == ziel) {
			if (destination_halt.is_bound() && route.is_contained(start)) {
				for (uint32 i = route.index_of(start); i < route.get_count(); i++) {
					grund_t *gr = welt->lookup(route.at(i));
					if (gr  && gr->get_halt() == destination_halt) {
						ziel = gr->get_pos();
					}
					else {
						break;
					}
				}
			}
		}
		/*
		const bool rail_type = front()->get_waytype() == track_wt || front()->get_waytype() == tram_wt || front()->get_waytype() == narrowgauge_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt;

		if (rail_type)
		{
			rail_vehicle_t* rail_vehicle = (rail_vehicle_t*)front();
			// If this is token block working, the route must only be unreserved if the token is released.
			if (rail_vehicle->get_working_method() != token_block && rail_vehicle->get_working_method() != one_train_staff)
			{
				unreserve_route();
				reserve_own_tiles();
			}
		}
		else if (front()->get_waytype() == air_wt)
		{
			// Sea and road waytypes do not have any sort of reserveation, and calls to unreserve_route() are expensive.
			unreserve_route();
		}*/
	}

	return true;
}


/**
 * Berechne route von Start- zu Zielkoordinate
 * "Compute route from starting to goal coordinate" (Babelfish)
 * @author Hanjsörg Malthaner
 */
bool convoi_t::drive_to()
{
	koord3d start = front()->get_pos();
	koord3d ziel = schedule->get_current_eintrag().pos;
	const koord3d original_ziel = ziel;

	const bool check_onwards = front()->get_waytype() == road_wt || front()->get_waytype() == track_wt || front()->get_waytype() == tram_wt || front()->get_waytype() == narrowgauge_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt;
	
	bool success = calc_route(start, ziel, speed_to_kmh(get_min_top_speed()));
		
	grund_t* gr = welt->lookup(ziel);
	grund_t* gr_current = welt->lookup(start); 

	if(check_onwards && gr && !gr->get_depot())
	{
		// We need to calculate the full route through to the next signal or reversing point
		// to avoid ignoring signals.
		int counter = schedule->get_count();

		schedule_entry_t* schedule_entry = &schedule->entries[schedule->get_current_stop()];
		bool update_line = false;
		while(success && counter--)
		{
			if(schedule_entry->reverse == -1 && (!gr_current || !gr_current->get_depot()))
			{
				schedule_entry->reverse = check_destination_reverse() ? 1 : 0;
				schedule->set_reverse(schedule_entry->reverse, schedule->get_current_stop()); 
				if(line.is_bound())
				{
					schedule_t* line_schedule = line->get_schedule();
					schedule_entry_t &line_entry = line_schedule->entries[schedule->get_current_stop()];
					line_entry.reverse = schedule_entry->reverse;
					update_line = true;	
				}
			}

			if(schedule_entry->reverse == 1 || haltestelle_t::get_halt(schedule_entry->pos, get_owner()).is_bound())
			{
				// The convoy must stop at the current route's end.
				break;
			}
			advance_schedule();
			schedule_entry = &schedule->entries[schedule->get_current_stop()];
			success = front()->reroute(route.get_count() - 1, schedule_entry->pos);
		}

		if (update_line)
		{
#ifdef MULTI_THREAD
			pthread_mutex_lock(&step_convois_mutex);
			world()->stop_path_explorer();
#endif
			simlinemgmt_t::update_line(line);
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&step_convois_mutex);
#endif
		}
	}

	if(!success)
	{
		if(state != NO_ROUTE)
		{
			state = NO_ROUTE;
			no_route_retry_count = 0;
#ifdef MULTI_THREAD
			pthread_mutex_lock(&step_convois_mutex);
#endif
			get_owner()->report_vehicle_problem( self, ziel );
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&step_convois_mutex);
#endif
		}
		// wait 25s before next attempt
		wait_lock = 25000;
	}
	else {
		bool route_ok = true;
		const uint8 current_stop = schedule->get_current_stop();
		if(  front()->get_waytype() != water_wt  ) {
			air_vehicle_t *plane = dynamic_cast<air_vehicle_t *>(front());
			uint32 takeoff = 0, search = 0, landing = 0;
			air_vehicle_t::flight_state plane_state = air_vehicle_t::taxiing;
			if(  plane  ) {
				// due to the complex state system of aircrafts, we have to save index and state
				plane->get_event_index( plane_state, takeoff, search, landing );
			}

			ziel = schedule->get_current_eintrag().pos;

			// set next schedule target position if next is a waypoint
			if(  is_waypoint(ziel)  ) {
				fpl_target = ziel;
			}

			// continue route search until the destination is a station
			while(  is_waypoint(ziel)  ) {
				start = ziel;
				schedule->advance();
				ziel = schedule->get_current_eintrag().pos;

				if(  schedule->get_current_stop() == current_stop  ) {
					// looped around without finding a halt => entire schedule is waypoints.
					break;
				}

				route_t next_segment;
				if (!front()->calc_route(start, ziel, speed_to_kmh(get_min_top_speed()), has_tall_vehicles(), &next_segment)) {
					// do we still have a valid route to proceed => then go until there
					if(  route.get_count()>1  ) {
						break;
					}
					// we are stuck on our first routing attempt => give up
					if(  state != NO_ROUTE  ) {
						state = NO_ROUTE;
#ifdef MULTI_THREAD
						pthread_mutex_lock(&step_convois_mutex);
#endif
						get_owner()->report_vehicle_problem( self, ziel );
#ifdef MULTI_THREAD
						pthread_mutex_unlock(&step_convois_mutex);
#endif
					}
					// wait 25s before next attempt
					wait_lock = 25000;
					route_ok = false;
					break;
				}
				else {
					bool looped = false;
					if(  front()->get_waytype() != air_wt  ) {
							// check if the route circles back on itself (only check the first tile, should be enough)
						looped = route.is_contained(next_segment.at(1));
#if 0
						// this will forbid an eight firure, which might be clever to avoid a problem of reserving one own track
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
						fpl_target = koord3d::invalid;
						break;
					}
					else {
						uint32 count_offset = route.get_count()-1;
						route.append( &next_segment);
						if(  plane  ) {
							// maybe we need to restore index
							uint32 dummy2;
							air_vehicle_t::flight_state dummy1;
							uint32 new_takeoff, new_search, new_landing;
							plane->get_event_index( dummy1, new_takeoff, new_search, new_landing );
							if(  takeoff == 0x7FFFFFFF  &&  new_takeoff != 0x7FFFFFFF  ) {
								takeoff = new_takeoff + count_offset;
							}
							if(  landing == 0x7FFFFFFF  &&  new_landing != 0x7FFFFFFF  ) {
								landing = new_landing + count_offset;
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

		fpl_target = ziel;
		if(  route_ok  ) {
			// When this was single threaded, this was an immediate call to vorfahren(), but this cannot be called when multi-threaded.
			state = ROUTE_JUST_FOUND;
			return true;
		}
	}
	return false;
}


/**
 * Ein Fahrzeug hat ein Problem erkannt und erzwingt die
 * Berechnung einer neuen Route
 *
 * "A vehicle recognized and forces a problem the computation of a new route" (Babelfish)
 * @author Hanjsörg Malthaner
 */
void convoi_t::suche_neue_route()
{
	state = ROUTING_1;
	wait_lock = 0;
}

/**
 * Things that call a convoy's route finding
 * but not block reserving
 */
void convoi_t::threaded_step()
{
	if (state == ROUTING_2)
	{
		// Only perform route finding in the threaded step
		// because we need to be able to run this at the same
		// time as player interaction in network mode.
		// ROUTING_2 can only be set in step(), so this is
		// deterministic. 

		drive_to();
	}
}

/**
 * Asynchroneous single-threaded stepping of convoys
 * @author Hj. Malthaner
 */
void convoi_t::step()
{
	if(wait_lock !=0)
	{
		return;
	}

	// moved check to here, as this will apply the same update
	// logic/constraints convois have for manual schedule manipulation
	if (line_update_pending.is_bound()) {
		check_pending_updates();
	}

	bool autostart = false;
	uint8 position;
	bool rev;
	grund_t* gr;

	switch(state)
	{
		case INITIAL:
			// If there is a pending replacement, just do it
			if (replace && replace->get_replacing_vehicles()->get_count()>0)
			{
				autostart = replace->get_autostart();

				// Knightly : before replacing, copy the existing set of goods category indices
				minivec_tpl<uint8> old_goods_catg_index(goods_catg_index.get_count());
				for( uint8 i = 0; i < goods_catg_index.get_count(); ++i )
				{
					old_goods_catg_index.append( goods_catg_index[i] );
				}

				const grund_t *gr = welt->lookup(home_depot);
				depot_t *dep;
				if ( gr && (dep = gr->get_depot()) ) {
					char buf[256];
					name_offset = sprintf(buf,"(%i) ",self.get_id() );
					tstrncpy(buf + name_offset, translator::translate(front()->get_desc()->get_name()), 116);
					const bool keep_name = strcmp(get_name(), buf);	
					vector_tpl<vehicle_t*> new_vehicles;
					vehicle_t* veh = NULL;
					// Acquire the new one
					ITERATE_PTR(replace->get_replacing_vehicles(),i)
					{
						veh = NULL;
						// First - check whether there are any of the required vehicles already
						// in the convoy (free)
						for(uint8 k = 0; k < vehicle_count; k++)
						{
							if(vehicle[k]->get_desc() == replace->get_replacing_vehicle(i))
							{
								veh = remove_vehicle_bei(k);
								break;
							}
						}

						if(veh == NULL && replace->get_allow_using_existing_vehicles())
						{
							 // Second - check whether there are any of the required vehicles already
							 // in the depot (more or less free).
							veh = dep->find_oldest_newest(replace->get_replacing_vehicle(i), true, &new_vehicles);
						}

						if (veh == NULL && !replace->get_retain_in_depot())
						{
							// Third - check whether the vehicle can be upgraded (cheap)
							// Note: if "retain in depot" is selected, do not upgrade, as
							// the old vehicles will be needed (e.g., for a cascade).
							for(uint16 j = 0; j < vehicle_count; j ++)
							{
								for(uint8 c = 0; c < vehicle[j]->get_desc()->get_upgrades_count(); c ++)
								{
									if(replace->get_replacing_vehicle(i) == vehicle[j]->get_desc()->get_upgrades(c))
									{
										veh = vehicle_builder_t::build(get_pos(), get_owner(), NULL, replace->get_replacing_vehicle(i), true);
										upgrade_vehicle(j, veh);
										remove_vehicle_bei(j);
										goto end_loop;
									}
								}
							}
						}
end_loop:

						if(veh == NULL)
						{
							// Fourth - if all else fails, buy from new (expensive).
							veh = dep->buy_vehicle(replace->get_replacing_vehicle(i), livery_scheme_index);
						}

						// This new method is needed to enable this method to iterate over
						// the existing vehicles in the convoy while it is adding new vehicles.
						// They must be added to temporary storage, and appended to the existing
						// convoy at the end, after the existing convoy has been deleted.
						assert(veh);
						if(veh)
						{
							new_vehicles.append(veh);
						}

					}

					//First, delete the existing convoy
					for(sint8 a = vehicle_count-1;  a >= 0; a--)
					{
						if(!replace->get_retain_in_depot())
						{
							//Sell any vehicles not upgraded or kept.
							sint64 value = vehicle[a]->calc_sale_value();
							waytype_t wt = vehicle[a]->get_desc()->get_waytype();
							owner->book_new_vehicle( value, dep->get_pos().get_2d(),wt );
							delete vehicle[a];
							vehicle_count--;
						}
						else
						{
							vehicle_t* old_veh = remove_vehicle_bei(a);
							old_veh->discard_cargo();
							old_veh->set_leading(false);
							old_veh->set_last(false);
							dep->get_vehicle_list().append(old_veh);				
						}
					}
					vehicle_count = 0;
					reset();

					//Next, add all the new vehicles to the convoy in order.
					ITERATE(new_vehicles,b)
					{
						dep->append_vehicle(self, new_vehicles[b], false, false);
					}

					if (!keep_name)
					{
						set_name(front()->get_desc()->get_name());
					}

					clear_replace();

					if (line.is_bound()) {
						line->recalc_status();
						if (line->get_replacing_convoys_count()==0) {
							char buf[256];
							sprintf(buf, translator::translate("Replacing\nvehicles of\n%-20s\ncompleted"), line->get_name());
							welt->get_message()->add_message(buf, home_depot.get_2d(),message_t::general, PLAYER_FLAG|get_owner()->get_player_nr(), IMG_EMPTY);
						}

					}

					// Knightly : recalculate goods category index and determine if refresh is needed
					recalc_catg_index();

					minivec_tpl<uint8> differences(goods_catg_index.get_count() + old_goods_catg_index.get_count());

					// removed categories : present in old category list but not in new category list
					for ( uint8 i = 0; i < old_goods_catg_index.get_count(); ++i )
					{
						if ( ! goods_catg_index.is_contained( old_goods_catg_index[i] ) )
						{
							differences.append( old_goods_catg_index[i] );
						}
					}

					// added categories : present in new category list but not in old category list
					for ( uint8 i = 0; i < goods_catg_index.get_count(); ++i )
					{
						if ( ! old_goods_catg_index.is_contained( goods_catg_index[i] ) )
						{
							differences.append( goods_catg_index[i] );
						}
					}

					if ( differences.get_count() > 0 )
					{
						if ( line.is_bound() )
						{
							// let the line decide if refresh is needed
							line->recalc_catg_index();
						}
						else
						{
							// refresh only those categories which are either removed or added to the category list
							haltestelle_t::refresh_routing(schedule, differences, owner);
						}
					}

					if (autostart) {
						dep->start_convoi(self, false);
					}
				}
			}
			break;

		case ROUTING_2:
		case DUMMY5:
		break;

		case REVERSING:
			if(wait_lock == 0)
			{
				position = schedule ? schedule->get_current_stop() : 0;
				rev = !reverse_schedule;
				schedule->increment_index(&position, &rev);
				halthandle_t this_halt = haltestelle_t::get_halt(front()->get_pos(), owner);
				if(this_halt == haltestelle_t::get_halt(schedule->entries[position].pos, owner))
				{
					// Load any newly arrived passengers/mail bundles/goods before setting off.
					laden();
				}
				// A convoy starts after reversing
				state = CAN_START;
				
				if(front()->last_stop_pos == front()->get_pos())
				{
					book_waiting_times();
				}
			}

			break;

		case EDIT_SCHEDULE:
			// schedule window closed?
			if (schedule != NULL && schedule->is_editing_finished())
			{
				set_schedule(schedule);
				fpl_target = koord3d::invalid;

				if (schedule->empty())
				{
					// no entry => no route ...
					state = NO_ROUTE;
					// A convoy without a schedule should not be left lingering on the map.
					emergency_go_to_depot();
					// Get out of this routine; object might be destroyed.
					return;
				}
				else
				{				
					// The schedule window might be closed whilst this vehicle is still loading.
					// Do not allow the player to cheat by sending the vehicle on its way before it has finished.
					bool can_go = true;
					const uint32 reversing_time = schedule->get_current_eintrag().reverse == 1 ? calc_reverse_delay() : 0;
					can_go = can_go || welt->get_ticks() > go_on_ticks;
					can_go = can_go && welt->get_ticks() > arrival_time + ((sint64)current_loading_time - (sint64)reversing_time);
					can_go = can_go || no_load;

					grund_t *gr = welt->lookup(schedule->get_current_eintrag().pos);
					depot_t * this_depot = NULL;
					bool go_to_depot = false;
					if (gr)
					{
						this_depot = gr->get_depot();
						if (this_depot && this_depot->is_suitable_for(front()))
						{
							if (schedule->get_current_eintrag().pos == get_pos())
							{
								// If it's a suitable depot, move into the depot
								// This check must come before the station check, because for
								// ships we may be in a depot and at a sea stop!
								enter_depot(gr->get_depot());
								break;
							}
							else
							{
								// The go to depot command has been set previously and has not been unset.
								can_go = true;
								wait_lock = (arrival_time + ((sint64)current_loading_time - (sint64)reversing_time)) - welt->get_ticks();
								go_to_depot = true;
							}
						}
					}

					halthandle_t h = haltestelle_t::get_halt(get_pos(), get_owner());
					if (!go_to_depot && h.is_bound() && h == haltestelle_t::get_halt(schedule->get_current_eintrag().pos, get_owner()))
					{
						// We are at the station we are scheduled to be at
						// (possibly a different platform)
						if (route.get_count() > 0)
						{
							koord3d const& pos = route.back();
							if (h == haltestelle_t::get_halt(pos, get_owner()))
							{
								// If this is also the station at the end of the current route
								// (the correct platform)
								if (get_pos() == pos)
								{
									// And this is also the correct platform... then load.
									state = LOADING;
									break;
								}
								else
								{
									// Right station, wrong platform
									can_go ? state = DRIVING : state = LOADING;
									break;
								}
							}
						}
						else
						{
							// We're at the scheduled station,
							// but there is no programmed route.
							if (can_go)
							{
								// Checking for a route is now multi-threaded
								// Calculate a route in the next threaded step.
								if (prepare_for_routing())
								{
									state = ROUTING_2;
								}
								break;
							}
							else 
							{
								state = LOADING;
							}
						}
					}

					// We aren't at our destination; start routing.
					can_go ? state = ROUTING_1 : state = LOADING;
				}
			}
			else
			{
				break;
			}

		case ROUTING_1:
		{
			vehicle_t* v = front();

			if (schedule->empty()) {
				state = NO_ROUTE;
				owner->report_vehicle_problem(self, koord3d::invalid);

			}
			else {
				// check first, if we are already there:
				assert(schedule->get_current_stop()<schedule->get_count());
				if (v->get_pos() == schedule->get_current_eintrag().pos) {
					advance_schedule();
				}
				// Calculate a route in the next threaded step.
				if (prepare_for_routing())
				{
					state = ROUTING_2;
				}
			}
		}
		break;

		case OUT_OF_RANGE:
		case NO_ROUTE:
		{
			reserve_own_tiles();
			// stuck vehicles
			if (no_route_retry_count < 7)
			{
				no_route_retry_count++;
			}
			if (no_route_retry_count >= 3 && front()->get_waytype() != water_wt && (!welt->lookup(get_pos())->is_water()))
			{
				// If the convoy is stuck for too long, send it to a depot.
				emergency_go_to_depot();
				// get out of this routine; vehicle might be destroyed
				return;
			}
			else if (schedule->empty())
			{
				// no entries => no route ...
			}
			else
			{
				// Calculate a route in the next threaded step.
				if (prepare_for_routing())
				{
					state = ROUTING_2;
				}
			}
			break;
		}

		case ROUTE_JUST_FOUND:
			gr = welt->lookup(get_route()->back());

			if (front()->get_waytype() == track_wt || front()->get_waytype() == tram_wt || front()->get_waytype() == narrowgauge_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt && gr && !gr->get_depot())
			{
				rail_vehicle_t* rv = (rail_vehicle_t*)back();
				rail_vehicle_t* rv_front = (rail_vehicle_t*)front();
				if (rv_front->get_working_method() != one_train_staff && rv_front->get_working_method() != token_block)
				{
					rv->unreserve_station();
				}
			}
			vorfahren();
			break;
			// This used to be part of drive_to(), but this was not suitable for use in a multi-threaded state.

		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
			{
				vehicle_t* v = front();

				sint32 restart_speed = -1;
				if( v->can_enter_tile( restart_speed, 0 ) ) {
					// can reserve new block => drive on
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
					if(haltestelle_t::get_halt(v->get_pos(),owner).is_bound()) {
						v->play_sound();
					}
				}
				else if(  steps_driven==0  ) {
					// on rail depot tile, do not reserve this
					if(  grund_t *gr = welt->lookup(front()->get_pos())  ) {
						if (schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(front()->get_waytype()))) {
							sch0->unreserve(front());
						}
					}
				}
				if(restart_speed>=0) {
					set_akt_speed(restart_speed);
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
				if (front()->get_convoi() != this)
				{
					front()->set_convoi(this);
				}

				if (front()->can_enter_tile(restart_speed,0)) {
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
				}
				if(restart_speed>=0) {
					set_akt_speed(restart_speed);
				}
				if(state!=DRIVING) {
					set_tiles_overtaking( 0 );
				}
			}

			if (wait_lock < 1)
			{
				position = schedule ? schedule->get_current_stop() : 0;
				rev = !reverse_schedule;
				schedule->increment_index(&position, &rev);
				halthandle_t this_halt = haltestelle_t::get_halt(front()->get_pos(), owner);
				if (this_halt == haltestelle_t::get_halt(schedule->entries[position].pos, owner))
				{
					// Load any newly arrived passengers/mail bundles/goods while waiting for a signal to clear.
					laden();
				}
			}
			break;

		case LOADING:
			laden();
			if (state != SELF_DESTRUCT)
			{
				if(get_depot_when_empty() && has_no_cargo())
				{
					go_to_depot(false, (replace && replace->get_use_home_depot()));
				}
				break;
			}
			// no break
			// continue with case SELF_DESTRUCT.

		// must be here; may otherwise confuse window management
		case SELF_DESTRUCT:
			welt->set_dirty();
			destroy();
			return; // must not continue method after deleting this object

		default:	/* keeps compiler silent*/
			break;
	}
	// calculate new waiting time
	vector_tpl<linehandle_t> lines;
	switch( state ) {
		// handled by routine
		case LOADING:
			break;

		// immediate action needed
		case LEAVING_DEPOT:
			get_owner()->simlinemgmt.get_lines(schedule->get_type(), &lines);	
			FOR(vector_tpl<linehandle_t>, const l, lines)
			{
				if(schedule->matches(welt, l->get_schedule()))
				{
					// if a line is assigned, set line!
					const uint32 needs_refresh = l->count_convoys();
					set_line(l);
					line->renew_stops();
					break;
				}
			}
			if(!line.is_bound())
			{
				register_stops();
			}
			if (front()->get_waytype() != air_wt)
			{
				front()->play_sound();	
			}
			// Fallthrough intended.
		case SELF_DESTRUCT:
		case ENTERING_DEPOT:
		case DRIVING:
		case ROUTING_2:
		case DUMMY5:
			wait_lock = 0;
			break;

		// just waiting for action here
		case INITIAL:
		case EDIT_SCHEDULE:
			wait_lock = max( wait_lock, 25000 );
			break;

		case EMERGENCY_STOP:
			if(wait_lock < 1)
			{
				state = WAITING_FOR_CLEARANCE;
			}

		// action soon needed
		case ROUTING_1:
		case CAN_START:
		case WAITING_FOR_CLEARANCE:
			//wait_lock = 500;
			// Bernd Gabriel: simutrans extended may have presets the wait_lock before. Don't overwrite it here, if it ought to wait longer.
			wait_lock = max(wait_lock, 250);
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

void convoi_t::advance_schedule() {
	if(schedule->get_current_stop() == 0) {
		arrival_to_first_stop.add_to_tail(welt->get_ticks());
	}

	// check if the convoi should switch direction
	if(  schedule->is_mirrored() && schedule->get_current_stop()==schedule->get_count()-1  ) {
		reverse_schedule = true;
	}
	else if( schedule->is_mirrored() && schedule->get_current_stop()==0  ) {
		reverse_schedule = false;
	}
	// advance the schedule cursor
	if (reverse_schedule) {
		schedule->advance_reverse();
	} else {
		schedule->advance();
	}
}

void convoi_t::new_year()
{
    jahresgewinn = 0;
}


uint16 convoi_t::get_overcrowded() const
{
	uint16 overcrowded = 0;
	for(uint8 i = 0; i < vehicle_count; i ++)
	{
		overcrowded += vehicle[i]->get_overcrowding();
	}
	return overcrowded;
}

uint8 convoi_t::get_comfort() const
{
	uint32 comfort = 0;
	uint8 passenger_vehicles = 0;
	uint16 passenger_seating = 0;

	uint16 capacity;
	const uint8 catering_level = get_catering_level(0);

	for(uint8 i = 0; i < vehicle_count; i ++)
	{
		if(vehicle[i]->get_cargo_type()->get_catg_index() == 0)
		{
			passenger_vehicles ++;
			capacity = vehicle[i]->get_desc()->get_capacity();
			comfort += vehicle[i]->get_comfort(catering_level) * capacity;
			passenger_seating += capacity;
		}
	}
	if(passenger_vehicles < 1 || passenger_seating < 1)
	{
		// Avoid division if possible
		return 0;
	}

	// There must be some passenger vehicles if we are here.
	comfort /= passenger_seating;

	return comfort;
}

void convoi_t::new_month()
{
	// should not happen: leftover convoi without vehicles ...
	if(vehicle_count==0) {
		DBG_DEBUG("convoi_t::new_month()","no vehicles => self destruct!");
		self_destruct();
		return;
	}

	// Deduct monthly fixed maintenance costs.
	// @author: jamespetts
	sint64 monthly_cost = 0;
	for(unsigned j=0;  j<get_vehicle_count();  j++ )
	{
		// Monthly cost is positive, but add it up to a negative number for booking.
		monthly_cost -= get_vehicle(j)->get_desc()->get_fixed_cost(welt);
	}
	jahresgewinn += monthly_cost;
	book( monthly_cost, CONVOI_OPERATIONS );
	book( monthly_cost, CONVOI_PROFIT );
	// This is way too tedious a way to get my waytype...
	waytype_t my_waytype;
	if (get_schedule()) {
		my_waytype = get_schedule()->get_waytype();
	}
	else if (get_vehicle_count()) {
		my_waytype = get_vehicle(0)->get_desc()->get_waytype();
	}
	else {
		my_waytype = ignore_wt;
	}
	get_owner()->book_vehicle_maintenance(monthly_cost, my_waytype);

	// everything normal: update history
	for (int j = 0; j<MAX_CONVOI_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>0; k--)
		{
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}

	if(financial_history[1][CONVOI_AVERAGE_SPEED] == 0)
	{
		// Last month's average speed is recorded as zero. This means that no
		// average speed data have been recorded in the last month, making
		// revenue calculations inaccurate. Use the second previous month's average speed
		// for the previous month's average speed.
		financial_history[1][CONVOI_AVERAGE_SPEED] = financial_history[2][CONVOI_AVERAGE_SPEED];
	}

	for(uint8 i = 0; i < MAX_CONVOI_COST; i ++)
	{
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}

	// remind every new month again
	if(state == NO_ROUTE)
	{
		get_owner()->report_vehicle_problem(self, get_pos());
	}
	else if(state == OUT_OF_RANGE)
	{
		get_owner()->report_vehicle_problem(self, schedule->get_current_eintrag().pos);
	}
	// check for traffic jam
	if(state==WAITING_FOR_CLEARANCE) {
		state = WAITING_FOR_CLEARANCE_ONE_MONTH;
		// check, if now free ...
		// migh also reset the state!
		sint32 restart_speed = -1;
		if (front()->can_enter_tile(restart_speed, 0)) {
			state = DRIVING;
		}
		if(restart_speed>=0) {
			set_akt_speed(restart_speed);
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
		has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
	}
}

/**
 * Make a convoi enter a depot.
 */
void convoi_t::enter_depot(depot_t *dep)
{
	// first remove reservation, if train is still on track
	unreserve_route();

	if(front()->get_waytype() == track_wt || front()->get_waytype()  == tram_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt)
	{
		rail_vehicle_t* w = (rail_vehicle_t*)front(); 
		w->set_working_method(drive_by_sight); 
	}

	if(reversed)
	{
		// Put the convoy back into "forward" position
		if(reversed)
		{
			reversable = front()->get_desc()->get_can_lead_from_rear() || (vehicle_count == 1 && front()->get_desc()->is_bidirectional());
		}
		else
		{
			reversable = back()->get_desc()->get_can_lead_from_rear() || (vehicle_count == 1 && front()->get_desc()->is_bidirectional());
		}
		reverse_order(reversable);
	}

	// Clear the departure table...
	clear_departures();

	// Set the speed to zero...
	set_akt_speed(0);

	// Make this the new home depot...
	// (Will be done again in convoi_arrived, but make sure to do it early in case of crashes)
	home_depot=dep->get_pos();

	// Hajo: remove vehicles from world data structure
	for(unsigned i=0; i<vehicle_count; i++) {
		vehicle_t* v = vehicle[i];

		// remove from old position
		grund_t* gr = welt->lookup(v->get_pos());
		if(gr) {
			// remove from blockstrecke
			v->set_last(true);
			v->leave_tile();
			v->set_flag( obj_t::not_on_map );
		}
	}
	last_signal_pos = koord3d::invalid;
	dep->convoi_arrived(self, get_schedule());

	close_windows();

	state = INITIAL;
	wait_lock = 0;
}


void convoi_t::start()
{
	if(state == INITIAL || state == ROUTING_1) {

		// set home depot to location of depot convoi is leaving
		if(route.empty())
		{
			home_depot = front()->get_pos();
		}
		else
		{
			home_depot = route.front();
			front()->set_pos( home_depot );
		}
		// put the convoi on the depot ground, to get automatical rotation
		// (vorfahren() will remove it anyway again.)
		grund_t *gr = welt->lookup( home_depot );
		assert(gr);
		gr->obj_add( front() );

		alte_direction = ribi_t::none;
		no_load = false;
		depot_when_empty = false;

		// if the schedule is mirrored, convoys starting
		// reversed should go directly to the end.
		if( schedule->is_mirrored() && reverse_schedule ) {
			schedule->advance_reverse();
		}

		state = ROUTING_1;

		// recalc weight and image
		// also for any vehicle entered a depot, set_last is true! => reset it correctly
		sint64 restwert_delta = 0;
		for(unsigned i=0; i<vehicle_count; i++) {
			vehicle[i]->set_leading( false );
			vehicle[i]->set_last( false );
			restwert_delta -= vehicle[i]->calc_sale_value();
			vehicle[i]->set_driven();
			restwert_delta += vehicle[i]->calc_sale_value();
			vehicle[i]->clear_flag( obj_t::not_on_map );
			vehicle[i]->load_cargo( halthandle_t() );
		}
		front()->set_leading( true );
		vehicle[vehicle_count-1]->set_last( true );
		// do not show the vehicle - it will be wrong positioned -vorfahren() will correct this
		front()->set_image(IMG_EMPTY);

		// update finances for used vehicle reduction when first driven
		owner->update_assets( restwert_delta, get_schedule()->get_waytype());

		// calc state for convoi
		calc_loading();

		if(line.is_bound()) {
			// might have changed the vehicles in this car ...
			line->recalc_catg_index();
		}
		else
		{
			// New method - recalculate as necessary

			// Added by : Knightly
			haltestelle_t::refresh_routing(schedule, goods_catg_index, owner);
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
	const vehicle_t* v = front();
	alte_direction = v->get_direction();

	// check, what is at destination!
	const grund_t *gr = welt->lookup(v->get_pos());
	depot_t *dp = gr->get_depot();

	// double-check for right depot type based on first vehicle
	if(dp && dp->is_suitable_for(v) ) {
		// ok, we are entering a depot

		// Provide the message since we got here "on schedule".
		cbuffer_t buf;

		if (!replace || !replace->get_autostart()) {
			buf.printf( translator::translate("!1_DEPOT_REACHED"), get_name() );
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_owner()->get_player_nr(), IMG_EMPTY);
		}

		enter_depot(dp);
	}
	else {
		// no suitable depot reached, check for stop!
		halthandle_t halt = haltestelle_t::get_halt(v->get_pos(),owner);
		if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			set_akt_speed(0);
			halt->book(1, HALT_CONVOIS_ARRIVED);
			state = LOADING;
			go_on_ticks = WAIT_INFINITE;	// we will eventually wait from now on
			if(front()->get_waytype() == air_wt)
			{
				air_vehicle_t* aircraft = (air_vehicle_t*)front();
				if(aircraft->get_flyingheight() > 0)
				{
					// VTOL aircraft landing - set to landed state.
					aircraft->force_land();
				}
			}
		}
		else if(schedule->get_current_eintrag().pos == get_pos()) {
			// Neither depot nor station: waypoint
			advance_schedule();
			state = ROUTING_1;
			if(replace && depot_when_empty &&  has_no_cargo()) {
				depot_when_empty=false;
				no_load=false;
				go_to_depot(false);
			}
		}
		else
		{
			state = ROUTING_1;
		}
	}
	wait_lock = 0;
}


/**
 * Wait until vehicle 0 returns go-ahead
 * @author Hj. Malthaner
 */
void convoi_t::warten_bis_weg_frei(sint32 restart_speed)
{
	if(!is_waiting()) {
		if(state != EMERGENCY_STOP || wait_lock == 0)
		{
			if(state != ROUTING_1)
			{
				state = WAITING_FOR_CLEARANCE;
			}
			wait_lock = 0;
		}
	}
	if(restart_speed>=0) {
		// langsam anfahren
		// "slow start" (Google)
		set_akt_speed(restart_speed);
	}
}


bool convoi_t::add_vehicle(vehicle_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehicle()","at pos %i of %i totals.",vehicle_count,max_vehicle);
	// extend array if requested (only needed for trains)
	if(vehicle_count == max_vehicle) {
DBG_MESSAGE("convoi_t::add_vehicle()","extend array_tpl to %i totals.",max_rail_vehicle);
		//vehicle.resize(max_rail_vehicle, NULL);
		vehicle.resize(max_rail_vehicle);
	}
	// now append
	if (vehicle_count < vehicle.get_count()) {
		v->set_convoi(this);

		if(infront) {
			for(unsigned i = vehicle_count; i > 0; i--) {
				vehicle[i] = vehicle[i - 1];
			}
			vehicle[0] = v;
		} else {
			vehicle[vehicle_count] = v;
		}
		vehicle_count ++;

		// Added by		: Knightly
		// Adapted from : simline_t
		// Purpose		: Try to update supported goods category of this convoy
		if (v->get_cargo_max() > 0)
		{
			const goods_desc_t *ware_type = v->get_cargo_type();
			if (ware_type != goods_manager_t::none)
				goods_catg_index.append_unique(ware_type->get_catg_index(), 1);
		}


		const vehicle_desc_t *info = v->get_desc();
		if(info->get_power()) {
			is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
		}
		//sum_power += info->get_power();
		//if(info->get_engine_type() == vehicle_desc_t::steam)
		//{
		//	power_from_steam += info->get_power();
		//	power_from_steam_with_gear += info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor();
		//}
		//sum_gear_and_power += (info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
		//sum_weight += info->get_weight();
		//min_top_speed = min( min_top_speed, kmh_to_speed( v->get_desc()->get_topspeed() ) );
		//sum_gesamtweight = sum_weight;
		calc_loading();
		invalidate_vehicle_summary();
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
			has_obsolete = info->is_obsolete( welt->get_timeline_year_month(), welt );
		}
		player_t::add_maintenance( get_owner(), info->get_maintenance(), info->get_waytype() );
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

	calc_min_range();
	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();
	calc_direction_steps();

DBG_MESSAGE("convoi_t::add_vehicle()","now %i of %i total vehicles.",vehicle_count,max_vehicle);
	return true;
}

void convoi_t::upgrade_vehicle(uint16 i, vehicle_t* v)
{
	// Adapted from the add/remove vehicle functions
	// @author: jamespetts, February 2010

DBG_MESSAGE("convoi_t::upgrade_vehicle()","at pos %i of %i totals.",i,max_vehicle);

	// now append
	v->set_convoi(this);
	vehicle_t* old_vehicle = vehicle[i];
	vehicle[i] = v;

	// Amend the name if the name is the default name and it is the first vehicle
	// being replaced.

	if(i == 0)
	{
		char buf[128];
		name_offset = sprintf(buf,"(%i) ",self.get_id() );
		tstrncpy(buf + name_offset, translator::translate(old_vehicle->get_desc()->get_name()), 116);
		if(!strcmp(get_name(), buf))
		{
			set_name(v->get_desc()->get_name());
		}
	}

	// Added by		: Knightly
	// Adapted from : simline_t
	// Purpose		: Try to update supported goods category of this convoy
	if (v->get_cargo_max() > 0)
	{
		const goods_desc_t *ware_type = v->get_cargo_type();
		if (ware_type != goods_manager_t::none)
			goods_catg_index.append_unique(ware_type->get_catg_index(), 1);
	}

	const vehicle_desc_t *info = v->get_desc();
	// still requires electrification?
	if(is_electric) {
		is_electric = false;
		for(unsigned i=0; i<vehicle_count; i++) {
			if(vehicle[i]->get_desc()->get_power()) {
				is_electric |= vehicle[i]->get_desc()->get_engine_type()==vehicle_desc_t::electric;
			}
		}
	}

	if(info->get_power()) {
		is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
	}

	//min_top_speed = calc_min_top_speed(tdriver, vehicle_count);

	// Add power and weight of the new vehicle
	//sum_power += info->get_power();
	//sum_gear_and_power += (info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
	//sum_weight += info->get_weight();
	//sum_gesamtweight = sum_weight;

	// Remove power and weight of the old vehicle
	//info = old_vehicle->get_desc();
	//sum_power -= info->get_power();
	//sum_gear_and_power -= (info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
	//sum_weight -= info->get_weight();

	calc_loading();
	invalidate_vehicle_summary();
	freight_info_resort = true;
	recalc_catg_index();

	// check for obsolete
	if(has_obsolete)
	{
		has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

	calc_min_range();
	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();
	calc_direction_steps();

	delete old_vehicle;

DBG_MESSAGE("convoi_t::upgrade_vehicle()","now %i of %i total vehicles.",i,max_vehicle);
}

vehicle_t *convoi_t::remove_vehicle_bei(uint16 i)
{
	vehicle_t *v = NULL;
	if(i<vehicle_count) {
		v = vehicle[i];
		if(v != NULL) {
			for(unsigned j=i; j<vehicle_count-1u; j++) {
				vehicle[j] = vehicle[j + 1];
			}

			v->set_convoi(NULL);

			--vehicle_count;
			vehicle[vehicle_count] = NULL;

			// Added by		: Knightly
			// Purpose		: To recalculate the list of supported goods category
			recalc_catg_index();

			//const vehicle_desc_t *info = v->get_desc();
			//sum_power -= info->get_power();
			//sum_gear_and_power -= (info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
			//sum_weight -= info->get_weight();
			//sum_running_costs += info->get_operating_cost();
			//player_t::add_maintenance( get_owner(), -info->get_maintenance(), info->get_waytype() );
		}
		//sum_gesamtweight = sum_weight;
		calc_loading();
		invalidate_vehicle_summary();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(vehicle_count > 0) {
			set_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		//min_top_speed = calc_min_top_speed(tdriver, vehicle_count);

		// check for obsolete
		if(has_obsolete) {
			has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
		}

		recalc_catg_index();

		// still requires electrifications?
		if(is_electric) {
			is_electric = false;
			for(unsigned i=0; i<vehicle_count; i++) {
				if(vehicle[i]->get_desc()->get_power()) {
					is_electric |= vehicle[i]->get_desc()->get_engine_type()==vehicle_desc_t::electric;
				}
			}
		}
	}

	calc_min_range();
	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();
	if(!vehicle.empty() && vehicle[0])
	{
		calc_direction_steps();
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


//"set only last" (Google)
void convoi_t::set_erstes_letztes()
{
	// vehicle_count muss korrekt init sein
	// "anz vehicle must be correctly INIT" (Babelfish)
	if(vehicle_count>0) {
		front()->set_leading(true);
		for(unsigned i=1; i<vehicle_count; i++) {
			vehicle[i]->set_leading(false);
			vehicle[i - 1]->set_last(false);
		}
		back()->set_last(true);
	}
	else {
		dbg->warning("convoi_t::set_erstes_letzes()", "called with vehicle_count==0!");
	}
}


// remove wrong freight when schedule changes etc.
void convoi_t::check_freight()
{
	for(unsigned i=0; i<vehicle_count; i++) {
		vehicle[i]->remove_stale_cargo();
	}
	calc_loading();
	freight_info_resort = true;
}


bool convoi_t::set_schedule(schedule_t * sch)
{
	if(  state==SELF_DESTRUCT  ) {
		return false;
	}

	DBG_DEBUG("convoi_t::set_schedule()", "new=%p, old=%p", sch, schedule);
	assert(sch != NULL);

	if(!line.is_bound() && state != INITIAL)
	{
		// New method - recalculate as necessary

		// Added by : Knightly
		if ( schedule == sch && old_schedule )	// Case : Schedule window of operating convoy
		{
			if ( !old_schedule->matches(welt, schedule) )
			{
				haltestelle_t::refresh_routing(old_schedule, goods_catg_index, owner);
				haltestelle_t::refresh_routing(schedule, goods_catg_index, owner);
			}
		}
		else
		{
			if (schedule != sch)
			{
				haltestelle_t::refresh_routing(schedule, goods_catg_index, owner);
			}
			haltestelle_t::refresh_routing(sch, goods_catg_index, owner);
		}
	}

	if (schedule == sch && old_schedule) {
		schedule = old_schedule;
	}
	
	// happens to be identical?
	if(schedule!=sch) {
		// now check, we we have been bond to a line we are about to lose:
		bool changed = false;
		if(  line.is_bound()  ) {
			if(  !sch->matches(welt, schedule)) {
				changed = true;
			}
			if(  !sch->matches( welt, line->get_schedule() )  ) {
				// change from line to individual schedule
				//		-> unset line now and register stops from new schedule later
				changed = true;
				unset_line();
			}
		}
		else {
			if(  !sch->matches( welt, schedule )  ) {
				// Knightly : merely change schedule and do not involve line
				//				-> unregister stops from old schedule now and register stops from new schedule later
				changed = true;
				unregister_stops();
			}
		}
		// destroy a possibly open schedule window
		//"is completed" (Google)
		if(  schedule  &&  !schedule->is_editing_finished()  ) {
			destroy_win((ptrdiff_t)schedule);
			delete schedule;
		}
		schedule = sch;
		if(  changed  )
		{
			// Knightly : if line is unset or schedule is changed
			//				-> register stops from new schedule
			if(!line.is_bound())
			{
				register_stops();
			}

			// Also, clear the departures table, which may now be out of date.
			clear_departures();
		}
	}

	// remove wrong freight
	check_freight();

	// ok, now we have a schedule
	if(state != INITIAL)
	{
		state = EDIT_SCHEDULE;
	}
	// to avoid jumping trains
	alte_direction = front()->get_direction();
	wait_lock = 0;
	return true;
}


schedule_t *convoi_t::create_schedule()
{
	if(schedule == NULL) {
		const vehicle_t* v = front();

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
bool convoi_t::can_go_alte_direction()
{
	// invalid route? nothing to test, must start new
	if(route.empty()) {
		return false;
	}

	// going backwards? then recalculate all
	ribi_t::ribi neue_direction_rwr = ribi_t::backward(front()->calc_direction(route.front().get_2d(), route.at(min(2, route.get_count() - 1)).get_2d()));
//	DBG_MESSAGE("convoi_t::go_alte_direction()","neu=%i,rwr_neu=%i,alt=%i",neue_direction_rwr,ribi_t::backward(neue_direction_rwr),alte_direction);
	if(neue_direction_rwr&alte_direction) {
		set_akt_speed(8);
		return false;
	}

	// now get the actual length and the tile length
	uint16 convoi_length = 15;
	uint16 tile_length = 24;
	unsigned i;	// for visual C++
	const vehicle_t* pred = NULL;
	for(i=0; i<vehicle_count; i++) {
		const vehicle_t* v = vehicle[i];
		grund_t *gr = welt->lookup(v->get_pos());

		// not last vehicle?
		// the length of last vehicle does not matter when it comes to positioning of vehicles
		if ( i+1 < vehicle_count) {
			convoi_length += v->get_desc()->get_length();
		}

		if(gr==NULL  ||  (pred!=NULL  &&  (abs(v->get_pos().x-pred->get_pos().x)>=2  ||  abs(v->get_pos().y-pred->get_pos().y)>=2))  ) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_direction()","broken convoy (id %i) found => fixing!",self.get_id());
			set_akt_speed(8);
			return false;
		}

		// now check, if ribi is straight and train is not
		ribi_t::ribi weg_ribi = gr->get_weg_ribi_unmasked(v->get_waytype());
		if(ribi_t::is_straight(weg_ribi)  &&  (weg_ribi|v->get_direction())!=weg_ribi) {
			dbg->warning("convoi_t::go_alte_direction()","convoy with wrong vehicle directions (id %i) found => fixing!",self.get_id());
			set_akt_speed(8);
			return false;
		}

		if(  pred  &&  pred->get_pos()!=v->get_pos()  ) {
			tile_length += (ribi_t::is_straight(welt->lookup(pred->get_pos())->get_weg_ribi_unmasked(pred->get_waytype())) ? 16 : 8192/vehicle_t::get_diagonal_multiplier())*koord_distance(pred->get_pos(),v->get_pos());
		}

		pred = v;
	}
	// check if convoi is way too short (even for diagonal tracks)
	tile_length += (ribi_t::is_straight(welt->lookup(vehicle[vehicle_count-1]->get_pos())->get_weg_ribi_unmasked(vehicle[vehicle_count-1]->get_waytype())) ? 16 : 8192/vehicle_t::get_diagonal_multiplier());
	if(  convoi_length>tile_length  ) {
		dbg->warning("convoi_t::go_alte_direction()","convoy too short (id %i) => fixing!",self.get_id());
		set_akt_speed(8);
		return false;
	}

	uint16 length = min((convoi_length/16u)+4u,route.get_count());	// maximum length in tiles to check

	// we just check, whether we go back (i.e. route tiles other than zero have convoi vehicles on them)
	for( int index=1;  index<length;  index++ ) {
		grund_t *gr=welt->lookup(route.at(index));
		// now check, if we are already here ...
		for(unsigned i=0; i<vehicle_count; i++) {
			if (gr->obj_ist_da(vehicle[i])) {
				// we are turning around => start slowly and rebuilt train
				set_akt_speed(8);
				return false;
			}
		}
	}

//DBG_MESSAGE("convoi_t::go_alte_direction()","alte=%d, neu_rwr=%d",alte_direction,neue_direction_rwr);

// we continue our journey; however later cars need also a correct route entry
// eventually we need to add their positions to the convois route
	koord3d pos = front()->get_pos();
	assert(pos == route.front());
	if (welt->lookup(pos)->get_depot()) {
		return false;
	}
	else {
		for (i = 0; i<vehicle_count; i++) {
			vehicle_t* v = get_vehicle(i); 
			// eventually add current position to the route
			if (route.front() != v->get_pos() && route.at(1) != v->get_pos()) {
				route.insert(v->get_pos());
			}
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we must set the current route index (instead assuming 1)
	length = min((convoi_length / 8u), route.get_count() - 1);	// maximum length in tiles to check
	bool ok = false;
	for (i = 0; i<vehicle_count; i++) {
		vehicle_t* v = get_vehicle(i); 

		// this is such awkward, since it takes into account different vehicle length
		const koord3d vehicle_start_pos = v->get_pos();
		for (int idx = 0; idx <= length; idx++) {
			if (route.at(idx) == vehicle_start_pos) {
				// set route index, no recalculations necessary
				v->initialise_journey(idx, false);
				ok = true;

				// check direction
				uint8 richtung = v->get_direction();
				uint8 neu_richtung = v->calc_direction(get_route()->at(max(idx - 1, 0)).get_2d(), v->get_pos_next().get_2d());
				// we need to move to this place ...
				if (neu_richtung != richtung && (i != 0 || vehicle_count == 1 || ribi_t::is_bend(neu_richtung))) {
					// 90 deg bend!
					return false;
				}

				break;
			}
		}
		// too short?!? (rather broken then!)
		if (!ok) {
			return false;
		}
	}

	return true;
}


// put the convoi on its way
void convoi_t::vorfahren()
{
	// Hajo: init speed settings
	sp_soll = 0;
	set_tiles_overtaking( 0 );
	uint32 reverse_delay = 0;

	must_recalc_data();

	koord3d k0 = route.front();
	grund_t *gr = welt->lookup(k0);
	bool at_dest = false;
	if(gr  &&  gr->get_depot()) {
		// start in depot
		for(unsigned i=0; i<vehicle_count; i++) {
			vehicle_t* v = vehicle[i];

			grund_t* gr = welt->lookup(v->get_pos());
			if(gr) {
				gr->obj_remove(v);
				if(gr->ist_uebergang()) {
					crossing_t *cr = gr->find<crossing_t>(2);
					cr->release_crossing(v);
				}
				// eventually unreserve this
				if(  schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(vehicle[i]->get_waytype()))  ) {
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
		vehicle_t* v0 = front();
		v0->set_leading(false); // switches off signal checks ...
		v0->get_smoke(false);
		steps_driven = 0;
		// drive half a tile:
		for(int i=0; i<vehicle_count; i++) {
			vehicle[i]->do_drive( (VEHICLE_STEPS_PER_TILE/2)<<YARDS_PER_VEHICLE_STEP_SHIFT );
		}
		v0->get_smoke(true);
		v0->set_leading(true); // switches on signal checks to reserve the next route

		// until all other are on the track
		state = CAN_START;
	}
	else
	{
		const bool must_change_direction = !can_go_alte_direction();
		// still leaving depot (steps_driven!=0) or going in other direction or misalignment?
		if(steps_driven > 0 || must_change_direction)
		{
			//Convoy needs to reverse
			//@author: jamespetts
			if(must_change_direction)
			{				
				switch(front()->get_waytype())
				{
					case air_wt:
					case water_wt:
						// Boats and aircraft do not need to change direction
						book_departure_time(welt->get_ticks());
						book_waiting_times();
						break;

					default:

						const bool reverse_as_unit = re_ordered ? front()->get_desc()->get_can_lead_from_rear() : back()->get_desc()->get_can_lead_from_rear();

						reversable = reverse_as_unit || (vehicle_count == 1 && front()->get_desc()->is_bidirectional());

						reverse_delay = calc_reverse_delay();

						state = REVERSING;
						if(front()->last_stop_pos == front()->get_pos())
						{
							// The convoy does not depart until it has reversed.
							book_departure_time(welt->get_ticks() + reverse_delay);
						}
						
						reverse_order(reversable);
				}
			}

			// start route from the beginning at index 0, place everything on start
			uint32 train_length = move_to(0);

			// move one train length to the start position ...
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_direction = front()->get_direction_of_travel();
			if(neue_direction==ribi_t::south  ||  neue_direction==ribi_t::east)
			{
				// drive the convoi to the same position, but do not hop into next tile!
				if(  train_length%16==0  ) {
					// any space we need => just add
					train_length += vehicle[vehicle_count-1]->get_desc()->get_length();
				}
				else {
					// limit train to front of tile
					train_length += min( (train_length%CARUNITS_PER_TILE)-1, vehicle[vehicle_count-1]->get_desc()->get_length() );
				}
			}
			else
			{
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			front()->set_leading(false); // switches off signal checks ...

			/*if(reversed && (reversable || front()->is_reversed()))
			{
				//train_length -= front()->get_desc()->get_length();
				train_length = 0;
				for(sint8 i = vehicle_count - 1; i >= 0; i--)
				{
					vehicle_t* v = vehicle[i];
					v->get_smoke(false);
					vehicle[i]->do_drive( ((OBJECT_OFFSET_STEPS)*train_length)<<12 ); //"fahre" = "go" (Google)
					train_length += (v->get_desc()->get_length());	// this give the length in 1/OBJECT_OFFSET_STEPS of a full tile => all cars closely coupled!
					v->get_smoke(true);
				}
				train_length -= back()->get_desc()->get_length();
			}

			else
			{*/
				//if(!reversable && front()->get_desc()->is_bidirectional())
				//{
				//	//This can sometimes relieve excess setting back on reversing.
				//	//Unfortunately, it seems to produce bizarre results on occasion.
				//	train_length -= (front()->get_desc()->get_length()) / 2;
				//	train_length = train_length > 0 ? train_length : 0;
				//}
				for(sint8 i = 0; i < vehicle_count; i++)
				{
					vehicle_t* v = vehicle[i];
					v->get_smoke(false);
					vehicle[i]->do_drive( (VEHICLE_STEPS_PER_CARUNIT*train_length)<<YARDS_PER_VEHICLE_STEP_SHIFT );
					train_length -= v->get_desc()->get_length();
					// this gives the length in carunits, 1/CARUNITS_PER_TILE of a full tile => all cars closely coupled!
					v->get_smoke(true);
				//}

			}
			front()->set_leading(true);
		}

		else if(front()->last_stop_pos == front()->get_pos())
		{
			book_departure_time(welt->get_ticks());
			book_waiting_times();
		}

		int counter = 1;
		schedule_t* sch = schedule;

		if(line.is_bound())
		{
			 counter = 2;
		}
		bool need_to_update_line = false;
		while(counter > 0 && sch->get_count() > 0)
		{
			uint8 stop = sch->get_current_stop();
			bool rev = !reverse_schedule;
			sch->increment_index(&stop, &rev);
			if(stop < sch->get_count())
			{
				// It might be possible for "stop" to be > the number of
				// items in the schedule if the schedule has changed recently.
				if(haltestelle_t::get_halt(sch->entries[stop].pos, owner) != (haltestelle_t::get_halt(get_pos(), owner)))
				{
					// It is possible that the last entry was a skipped waypoint.
					sch->increment_index(&stop, &rev);
				}
				
				if((sch->entries[stop].reverse == 1 != (state == REVERSING)) && (state != ROUTE_JUST_FOUND || front()->get_waytype() != road_wt))
				{
					need_to_update_line = true;
					const sint8 reverse_state = state == REVERSING ? 1 : 0;
					sch->set_reverse(reverse_state, stop);
				}

				break;
			}

			sch->increment_index(&stop, &rev);
			counter --;
			if(counter > 0)
			{
				sch = line->get_schedule();
			}
		}

		if(need_to_update_line)
		{
			const linehandle_t line = get_line();
			if(line.is_bound())
			{
				simlinemgmt_t::update_line(line);
			}
		}

		if(!at_dest)
		{	
			if(state != REVERSING)
			{
				// A convoy starts without reversing
				state = CAN_START;
			}
			// to advance more smoothly
			sint32 restart_speed = -1;
			if(state != REVERSING)
			{
				if(front()->can_enter_tile(restart_speed, 0)) 
				{
					// can reserve new block => drive on
					if (haltestelle_t::get_halt(k0, owner).is_bound() && front()->get_waytype() != air_wt) // Aircraft play sounds on taking off instead of taxiing
					{
						front()->play_sound();
					}
					state = DRIVING;
				}
			}
			else
			{
				// If a rail type vehicle is reversing in a station, reserve the entire platform.
				const waytype_t wt = front()->get_waytype();
				const bool rail_type = front()->get_waytype() == track_wt || front()->get_waytype() == tram_wt || front()->get_waytype() == narrowgauge_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt;
				if(rail_type)
				{
					grund_t* vgr = gr;
					schiene_t *w = gr ? (schiene_t *)vgr->get_weg(wt) : NULL;
					if(w)
					{
						// First, reserve under the entire train (which might be outside the station in part)
						for(unsigned i = 0; i < vehicle_count; i++)
						{
							vehicle_t const& v = *vehicle[i];
							vgr = welt->lookup(v.get_pos());
							if(!vgr)
							{
								continue;
							}
							w = (schiene_t *)vgr->get_weg(wt);
							if(!w)
							{
								continue;
							}
							w->reserve(self, vehicle[i]->get_direction());
						}

						// Next, reserve the rest (if any) of the platform.
						grund_t* to = gr;
						if(to)
						{
							ribi_t::ribi direction_of_travel = front()->get_direction();
							koord3d last_pos = gr->get_pos();
							while(haltestelle_t::get_halt(to->get_pos(), owner).is_bound())
							{		
								w = (schiene_t *)to->get_weg(wt);
								if(!w || !ribi_t::is_single(direction_of_travel))
								{
									// If direction_of_travel is not a proper single direction,
									// odd tile reservations causing blockages can occur.
									break;
								}
								if(last_pos != to->get_pos())
								{
									w->reserve(self, direction_of_travel);
									last_pos = to->get_pos();
								}
								to->get_neighbour(to, wt, direction_of_travel);
								direction_of_travel = vehicle_t::calc_direction(last_pos.get_2d(), to->get_pos().get_2d());
								if(last_pos == to->get_pos())
								{
									// Prevent infinite loops.
									break;
								}
							}
						}
					}
				}
			}
		}
		else {
			ziel_erreicht();
		}
	}

	// finally reserve route (if needed)
	if(  front()->get_waytype()!=air_wt  &&  !at_dest  ) {
		// do not pre-reserve for aircraft
		for(unsigned i=0; i<vehicle_count; i++) {
			// eventually reserve this
			vehicle_t const& v = *vehicle[i];
			const grund_t* gr = welt->lookup(v.get_pos());
			if(gr)
			{
				if (schiene_t* const sch0 = obj_cast<schiene_t>(gr->get_weg(v.get_waytype()))) {
					sch0->reserve(self, front()->get_direction());
				}
			}
			else {
				break;
			}
		}
	}
	// and let airplane start on ground, if there is an airstrip
	if(  front()->get_waytype()==air_wt  ) {
		if(  welt->lookup_kartenboden( route.at(0).get_2d() )->get_weg(air_wt)  ) {
			front()->set_convoi(this);
		}
	}

	wait_lock = reverse_delay;
	//INT_CHECK("simconvoi 711");
}

void convoi_t::reverse_order(bool rev)
{
	// Code snippet obtained and adapted from:
	// http://www.cprogramming.com/snippets/show.php?tip=15&count=30&page=0
	// by John Shao (public domain work)

	uint8 a = 0;
    vehicle_t* reverse;
	uint8 b  = vehicle_count;

	working_method_t wm = drive_by_sight;
	if(front()->get_waytype() == track_wt || front()->get_waytype() == tram_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt)
	{
		rail_vehicle_t* w = (rail_vehicle_t*)front(); 
		wm = w->get_working_method();
	}

	if(rev)
	{
		front()->set_leading(false);
	}
	else
	{
		if(!back()->get_desc()->is_bidirectional())
		{
			// Do not change the order at all if the last vehicle is not bidirectional
			return;
		}

		a++;
		if(front()->get_desc()->get_power() > 0)
		{
			// If this is a locomotive, check for tenders/pair units.
			if (front()->get_desc()->get_trailer_count() > 0 && vehicle[1]->get_desc()->get_leader_count() > 0)
			{
				a ++;
			}

			// Check for double-headed (and triple headed, etc.) tender locomotives
			uint8 first = a;
			uint8 second = a + 1;
			while(vehicle_count > second && (vehicle[first]->get_desc()->get_power() > 0 || vehicle[second]->get_desc()->get_power() > 0))
			{
				if(vehicle[first]->get_desc()->get_trailer_count() > 0 && vehicle[second]->get_desc()->get_leader_count() > 0)
				{
					a ++;
				}
				first++;
				second++;
			}
			if (vehicle_count > 1 && vehicle[1]->get_desc()->get_power() == 0 && vehicle[1]->get_desc()->get_trailer_count() == 1 && vehicle[1]->get_desc()->get_trailer(0) && vehicle[1]->get_desc()->get_trailer(0)->get_power() == 0 && vehicle[1]->get_desc()->get_trailer(0)->get_value() == 0)
			{
				// Multiple tenders or Garretts with powered front units.
				a ++;
			}
		}

		// Check whether this is a Garrett type vehicle (with unpowered front units).
		if(vehicle[0]->get_desc()->get_power() == 0 && vehicle[0]->get_desc()->get_capacity() == 0)
		{
			// Possible Garrett
			const uint8 count = front()->get_desc()->get_trailer_count();
			if(count > 0 && vehicle[1]->get_desc()->get_power() > 0 && vehicle[1]->get_desc()->get_trailer_count() > 0)
			{
				// Garrett detected
				a ++;
			}
		}

		// Check for a goods train with a brake van
		if((vehicle[vehicle_count - 2]->get_desc()->get_freight_type()->get_catg_index() > 1)
			&& 	vehicle[vehicle_count - 2]->get_desc()->get_can_be_at_rear() == false)
		{
			b--;
		}

		for(uint8 i = 1; i < vehicle_count; i++)
		{
			if(vehicle[i]->get_desc()->get_power() > 0)
			{
				a++;
			}
		}
	}

	back()->set_last(false);

	for( ; a<--b; a++) //increment a and decrement b until they meet each other
	{
		reverse = vehicle[a]; //put what's in a into swap spacekarte_t::load(
		vehicle[a] = vehicle[b]; //put what's in b into a
		vehicle[b] = reverse; //put what's in the swap (a) into b
	}

	if(!rev)
	{
		front()->set_leading(true);
	}

	back()->set_last(true);

	reversed = !reversed;
	if (rev)
	{
		re_ordered = !re_ordered;
	}

	for(const_iterator i = begin(); i != end(); ++i)
	{
		(*i)->set_reversed(reversed);
	}

	if(front()->get_waytype() == track_wt || front()->get_waytype()  == tram_wt || front()->get_waytype() == maglev_wt || front()->get_waytype() == monorail_wt)
	{
		rail_vehicle_t* w = (rail_vehicle_t*)front(); 
		w->set_working_method(wm); 
	}

	welt->set_dirty();
}


void convoi_t::rdwr_convoihandle_t(loadsave_t *file, convoihandle_t &cnv)
{
	if(  file->get_version()>112002  ) {
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
		if(  file->get_version()<101000  ) {
			file->wr_obj_id("Convoi");
			// the matching read is in karte_t::load(loadsave*)...
		}
	}

	// do the update, otherwise we might lose the line after save & reload
	if(file->is_saving()  &&  line_update_pending.is_bound()) {
		check_pending_updates();
	}

	simline_t::rdwr_linehandle_t(file, line);

	// we want persistent convoihandles so we can keep dialoges open in network games
	if(  file->is_loading()  ) {
		if(  file->get_version()<=112002  ) {
			self = convoihandle_t( this );
		}
		else {
			uint16 id;
			file->rdwr_short( id );
			self = convoihandle_t( this, id );
		}
	}
	else if(  file->get_version()>112002  ) {
		uint16 id = self.get_id();
		file->rdwr_short( id );
	}

	dummy = vehicle_count;
	file->rdwr_long(dummy);
	vehicle_count = (uint8)dummy;

	if(file->get_version()<99014) {
		// was anz_ready
		file->rdwr_long(dummy);
	}

	file->rdwr_long(wait_lock);
	// some versions may produce broken savegames apparently
	if(wait_lock > 1470000 && file->get_extended_version() < 11)
	{ 
		// max as was set by NO_ROUTE in former times. This code is deprecated now as the wait_lock can be higher with the convoy spacing feature.
		dbg->warning("convoi_t::sync_prepre()","Convoi %d: wait lock out of bounds: wait_lock = %d, setting to 1470000",self.get_id(), wait_lock);
		wait_lock = 1470000;
	}

	bool dummy_bool=false;
	file->rdwr_bool(dummy_bool);
	file->rdwr_long(owner_n);
	file->rdwr_long(akt_speed);
	sint32 akt_speed_soll = 0; // Former variable now unused
	file->rdwr_long(akt_speed_soll);
	file->rdwr_long(sp_soll);
	file->rdwr_enum(state);
	file->rdwr_enum(alte_direction);

	// read the yearly income (which has since then become a 64 bit value)
	// will be recalculated later directly from the history
	if(file->get_version()<=89003) {
		file->rdwr_long(dummy);
	}

	route.rdwr(file);

	if(file->is_loading()) {
		// extend array if requested (only needed for trains)
		if(vehicle_count > max_vehicle) {
			vehicle.resize(max_rail_vehicle, NULL);
		}
		owner = welt->get_player( owner_n );

		// Hajo: sanity check for values ... plus correction
		if(sp_soll < 0) {
			sp_soll = 0;
		}
		set_akt_speed(akt_speed);
	}

	file->rdwr_str(name_and_id + name_offset, lengthof(name_and_id) - name_offset);
	if(file->is_loading()) {
		set_name(name_and_id+name_offset);	// will add id automatically
	}

	koord3d dummy_pos;
	if(file->is_saving()) {
		for(unsigned i=0; i<vehicle_count; i++) {
			file->wr_obj_id( vehicle[i]->get_typ() );
			vehicle[i]->rdwr_from_convoi(file);
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
				v = new monorail_rail_vehicle_t(file, first, last);
			}
			else {
				switch(typ) {
					case obj_t::old_road_vehicle:
					case obj_t::road_vehicle: v = new road_vehicle_t(file, first, last);  break;
					case obj_t::old_waggon:
					case obj_t::rail_vehicle:    v = new rail_vehicle_t(file, first, last);     break;
					case obj_t::old_schiff:
					case obj_t::water_vehicle:    v = new water_vehicle_t(file, first, last);     break;
					case obj_t::old_aircraft:
					case obj_t::air_vehicle:    v = new air_vehicle_t(file, first, last);     break;
					case obj_t::old_monorail_vehicle:
					case obj_t::monorail_vehicle:    v = new monorail_rail_vehicle_t(file, first, last);     break;
					case obj_t::maglev_vehicle:         v = new maglev_rail_vehicle_t(file, first, last);     break;
					case obj_t::narrowgauge_vehicle:    v = new narrowgauge_rail_vehicle_t(file, first, last);     break;
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
				vehicle_t *v_neu = new monorail_rail_vehicle_t( v->get_pos(), v->get_desc(), v->get_owner(), NULL );
				v->discard_cargo();
				delete v;
				v = v_neu;
			}

			if(file->get_version()<99004) {
				dummy_pos.rdwr(file);
			}

			const vehicle_desc_t *info = v->get_desc();
			assert(info);

			// Hajo: if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				//sum_power += info->get_power();
				//sum_gear_and_power += (info->get_power() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
				//sum_weight += info->get_weight();
				has_obsolete |= welt->use_timeline()  &&  info->is_retired( welt->get_timeline_year_month() );
				is_electric |= info->get_engine_type()==vehicle_desc_t::electric;
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
				// add to blockstrecke "block stretch" (Google). Possibly "block section"?
				if(gr && (v->get_waytype()==track_wt  ||  v->get_waytype()==monorail_wt  ||  v->get_waytype()==maglev_wt  ||  v->get_waytype()==narrowgauge_wt)) {
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
			vehicle[i] = v;
		}
		//sum_gesamtweight = sum_weight;
	}

	bool has_fpl = (schedule != NULL);
	file->rdwr_bool(has_fpl);
	if(has_fpl) {
		//DBG_MESSAGE("convoi_t::rdwr()","convoi has a schedule, state %s!",state_names[state]);
		const vehicle_t* v = front();
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
		set_akt_speed(akt_speed);
		invalidate_vehicle_summary();
	}

	// Hajo: calculate new minimum top speed
	//min_top_speed = calc_min_top_speed(tdriver, vehicle_count);

	// Hajo: since sp_ist became obsolete, sp_soll is used modulo 65536
	sp_soll &= 65535;

	if(file->get_version()<=88003)
	{
		// load statistics
		int j;
		for (j = 0; j < 3; j++)
		{
			for (int k = MAX_MONTHS-1; k >= 0; k--)
			{
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_extended_version() <= 1) || (j == CONVOI_REFUNDS && file->get_extended_version() < 8))
				{
					// Versions of Extended saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Extended < 8
					// did not store refund information.
					if(file->is_loading())
					{
						financial_history[k][j] = 0;
					}
					continue;
				}
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (j = 2; j < 5; j++)
		{
			for (int k = MAX_MONTHS-1; k >= 0; k--)
			{
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_extended_version() <= 1) || (j == CONVOI_REFUNDS && file->get_extended_version() < 8))
				{
					// Versions of Extended saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Extended < 8
					// did not store refund information.
					if(file->is_loading())
					{
						financial_history[k][j] = 0;
					}
					continue;
				}
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][CONVOI_DISTANCE] = 0;
			//financial_history[k][CONVOI_WAYTOLL] = 0;
		}
	}
	else if(file->get_version() <= 102002 || (file->get_extended_version() < 7 && file->get_extended_version() != 0))
	{
		// load statistics
		for (int j = 0; j<7; j++)
		{
			for (int k = MAX_MONTHS-1; k>=0; k--)
			{
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_extended_version() <= 1) || (j == CONVOI_REFUNDS && file->get_extended_version() < 8))
				{
					// Versions of Extended saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Extended < 8
					// did not store refund information.
					if(file->is_loading())
					{
						financial_history[k][j] = 0;
					}
					continue;
				}
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (int j = 7; j<MAX_CONVOI_COST; j++)
		{
			for (int k = MAX_MONTHS-1; k>=0; k--)
			{
				financial_history[k][j] = 0;
			}
		}
	}
	else
	{
		// load statistics
		for (int j = 0; j < MAX_CONVOI_COST; j++)
		{
			switch (j)
			{
			case CONVOI_AVERAGE_SPEED:
			case CONVOI_COMFORT:
				if (file->get_extended_version() < 2)
				{
					// Versions of Extended saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values.
					if (file->is_loading())
					{
						for (int k = MAX_MONTHS-1; k >= 0; k--)
						{
							financial_history[k][j] = 0;
						}
					}
					continue;
				}
				break;

			case CONVOI_DISTANCE:
				if (file->get_extended_version() < 7)
				{
					// Simutrans-Standard: distances in tiles, not km. Convert.
					sint64 distance;
					if(file->is_loading())
					{
						for (int k = MAX_MONTHS-1; k >= 0; k--)
						{
							file->rdwr_longlong(distance);
							financial_history[k][j] = (distance *welt->get_settings().get_meters_per_tile()) / 1000;
						}
					}
					else
					{
						for (int k = MAX_MONTHS-1; k >= 0; k--)
						{
							distance = (financial_history[k][j] * 1000) /welt->get_settings().get_meters_per_tile();
							file->rdwr_longlong(distance);
						}
					}
					continue;
				}
				break;

			case CONVOI_REFUNDS:
				if (file->get_extended_version() < 8)
				{
					// versions of Extended < 8 did not store refund information.
					if (file->is_loading())
					{
						for (int k = MAX_MONTHS-1; k >= 0; k--)
						{
							financial_history[k][j] = 0;
						}
						if (file->get_extended_version() == 0 && file->get_version() >= 111001)
						{
							// CONVOI_MAXSPEED - not used in Extended
							sint64 dummy = 0;
							for (int k = MAX_MONTHS-1; k >= 0; k--)
							{
								file->rdwr_longlong(dummy);
							}
						}
					}
					continue;
				}
				break;
			}

			for (int k = MAX_MONTHS-1; k >= 0; k--)
			{
				file->rdwr_longlong(financial_history[k][j]);
			}
		}

		if (file->is_loading())
		{
			if (file->get_extended_version() == 0 && file->get_version() >= 112008 )
			{
				// CONVOY_WAYTOLL - not used in Extended
				sint64 dummy = 0;
				for (int k = MAX_MONTHS-1; k >= 0; k--)
				{
					file->rdwr_longlong(dummy);
				}
			}
		}
	}

	// the convoy odometer
	if(file->get_version() > 102002)
	{
		if(file->get_extended_version() < 7)
		{
			//Simutrans-Standard save - this value is in tiles, not km. Convert.
			if(file->is_loading())
			{
				sint64 tile_distance;
				file->rdwr_longlong(tile_distance);
				total_distance_traveled = (tile_distance *welt->get_settings().get_meters_per_tile()) / 1000;
			}
			else
			{
				sint64 km_distance = (total_distance_traveled * 1000) /welt->get_settings().get_meters_per_tile();
				file->rdwr_longlong(km_distance);
			}
		}
		else
		{
			file->rdwr_longlong(total_distance_traveled);
		}

	}

	if(file->get_version() >= 102003 && file->get_extended_version() >= 7)
	{
		if(file->get_extended_version() <= 8)
		{
			uint8 old_tiles = uint8(steps_since_last_odometer_increment / VEHICLE_STEPS_PER_TILE);
			file->rdwr_byte(old_tiles);
			steps_since_last_odometer_increment = old_tiles * VEHICLE_STEPS_PER_TILE;
		}
		else if (file->get_extended_version() > 8 && file->get_extended_version() < 10)
		{
			double tiles_since_last_odometer_increment = double(steps_since_last_odometer_increment) / VEHICLE_STEPS_PER_TILE;
			file->rdwr_double(tiles_since_last_odometer_increment);
			steps_since_last_odometer_increment = sint64(tiles_since_last_odometer_increment * VEHICLE_STEPS_PER_TILE );
		}
		else if(file->get_extended_version() >= 9 && file->get_version() >= 110006)
		{
			file->rdwr_longlong(steps_since_last_odometer_increment);
		}
	}

	// since it was saved as an signed int
	// we recalc it anyhow
	if(file->is_loading())
	{
		jahresgewinn = 0; //"annual profit" (Babelfish)
		for(int i=welt->get_last_month()%12;  i>=0;  i--  )
		{
			jahresgewinn += financial_history[i][CONVOI_PROFIT];
		}
	}

	// save/restore pending line updates
	if(file->get_version()>84008   &&  file->get_version()<99013) {
		file->rdwr_long(dummy);	// ignore
	}
	if(file->is_loading()) {
		line_update_pending = linehandle_t();
	}

	if(file->get_version() > 84009) {
		home_depot.rdwr(file);
	}

	// Old versions recorded last_stop_pos in convoi, not in vehicle
	koord3d last_stop_pos_convoi = koord3d(0,0,0);
	if (vehicle_count !=0) {
		last_stop_pos_convoi = front()->last_stop_pos;
	}
	if(file->get_version()>=87001) {
		last_stop_pos_convoi.rdwr(file);
	}
	else {
		last_stop_pos_convoi =
		!route.empty()   ? route.front()      :
		vehicle_count != 0 ? front()->get_pos() :
		koord3d(0, 0, 0);
	}

	// for leaving the depot routine
	if(file->get_version()<99014) {
		steps_driven = -1;
	}
	else {
		file->rdwr_short(steps_driven);
	}

	// waiting time left ...
	if(file->get_version()>=99017)
	{
		if(file->is_saving())
		{
			if(go_on_ticks == WAIT_INFINITE)
			{
				if(file->get_extended_version() <= 1)
				{
					uint32 old_go_on_ticks = (uint32)go_on_ticks;
					file->rdwr_long(old_go_on_ticks);
				}
				else
				{
					file->rdwr_longlong(go_on_ticks);
				}
			}
			else
			{
				sint64 diff_ticks = welt->get_ticks()>go_on_ticks ? 0 : go_on_ticks-welt->get_ticks();
				file->rdwr_longlong(diff_ticks);
			}
		}
		else
		{
			if(file->get_extended_version() <= 1)
			{
				uint32 old_go_on_ticks = (uint32)go_on_ticks;
				file->rdwr_long( old_go_on_ticks);
				go_on_ticks = old_go_on_ticks;
			}
			else
			{
				file->rdwr_longlong(go_on_ticks);
			}

			if(go_on_ticks!=WAIT_INFINITE)
			{
				go_on_ticks += welt->get_ticks();
			}
		}
	}

	// since 99015, the last stop will be maintained by the vehicles themselves
	if(file->get_version()<99015) {
		for(unsigned i=0; i<vehicle_count; i++) {
			vehicle[i]->last_stop_pos = last_stop_pos_convoi;
		}
	}

	// overtaking status
	if(file->get_version()<100001) {
		set_tiles_overtaking( 0 );
	}
	else {
		file->rdwr_byte(tiles_overtaking);
		set_tiles_overtaking( tiles_overtaking );
	}

	// no_load, withdraw
	if(file->get_version()<102001) {
		no_load = false;
		withdraw = false;
	}
	else {
		file->rdwr_bool(no_load);
		file->rdwr_bool(withdraw);
	}

	// reverse_schedule
	if(file->get_extended_version() >= 9)
	{
		file->rdwr_bool(reverse_schedule);
	}
	else if(file->is_loading())
	{
		reverse_schedule = false;
	}

	if(file->is_loading())
	{
		calc_min_range();
		highest_axle_load = calc_highest_axle_load();
		longest_min_loading_time = calc_longest_min_loading_time();
		longest_max_loading_time = calc_longest_max_loading_time();
		calc_direction_steps();
	}

	if(file->get_extended_version() >= 1)
	{
		file->rdwr_bool(reversed);
		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 12)
		{
			file->rdwr_bool(re_ordered);
		}

		//Replacing settings
		// BG, 31-MAR-2010: new replacing code starts with exp version 8:
		bool is_replacing = replace && (file->get_extended_version() >= 8);
		file->rdwr_bool(is_replacing);

		if(file->get_extended_version() >= 8)
		{
			if(is_replacing)
			{
				if(file->is_saving())
				{
					replace->rdwr(file);
				}
				else
				{
					replace = new replace_data_t(file);
				}
			}
			file->rdwr_bool(depot_when_empty);
		}
		else
		{
			// Original vehicle replacing settings - stored in convoi_t.
			bool old_autostart;
			file->rdwr_bool(old_autostart);
			file->rdwr_bool(depot_when_empty);

			uint16 replacing_vehicles_count = 0;

			vector_tpl<const vehicle_desc_t *> *replacing_vehicles;

			if(file->is_saving())
			{
				// BG, 31-MAR-2010: new replacing code starts with exp version 8.
				// BG, 31-MAR-2010: to keep compatibility with exp versions < 8
				//  at least the number of replacing vehicles (always 0) must be written.
				//replacing_vehicles = replace->get_replacing_vehicles();
				//replacing_vehicles_count = replacing_vehicles->get_count();
				file->rdwr_short(replacing_vehicles_count);
			}
			else
			{
				file->rdwr_short(replacing_vehicles_count);
				if (replacing_vehicles_count > 0)
				{
					// BG, 31-MAR-2010: new replacing code starts with exp version 8.
					// BG, 31-MAR-2010: but we must read all related data from file.
					replacing_vehicles = new vector_tpl<const vehicle_desc_t *>;
					for(uint16 i = 0; i < replacing_vehicles_count; i ++)
					{
						char vehicle_name[256];
						file->rdwr_str(vehicle_name, 256);
						const vehicle_desc_t* desc = vehicle_builder_t::get_info(vehicle_name);
						if(desc == NULL)
						{
							desc = vehicle_builder_t::get_info(translator::compatibility_name(vehicle_name));
						}
						if(desc == NULL)
						{
							dbg->warning("convoi_t::rdwr()","no vehicle pak for '%s' search for something similar", vehicle_name);
						}
						else
						{
							replacing_vehicles->append(desc);
						}
					}
					// BG, 31-MAR-2010: new replacing code starts with exp version 8.
					// BG, 31-MAR-2010: we must not create 'replace'. I does not work correct.
					delete replacing_vehicles;
				}
			}
		}
	}
	if(file->get_extended_version() >= 2)
	{
		if(file->get_extended_version() <= 9)
		{
			const grund_t* gr = welt->lookup(front()->last_stop_pos);
			if(gr && schedule)
			{
				departure_data_t dep;
				uint8 entry = schedule->get_current_stop();
				bool rev = !reverse_schedule;
				schedule->increment_index(&entry, &rev);
				departure_point_t departure_point(entry, !rev);

				dep = departures.get(departure_point);
				
				uint16 last_halt_id = gr->get_halt().get_id();
				sint64 departure_time = dep.departure_time;
				file->rdwr_longlong(departure_time);
				if(file->is_loading())
				{
					departures.clear();
					dep.departure_time = departure_time;
					departures.put(departure_point, dep);
				}
			}
		}
		else if(file->get_extended_version() < 12)
		{
			// Sadly, it is not possible to reconstruct departure data for these intermediate saved games,
			// as there is inherent ambiguity as to what stops match with which schedule entries (which was
			// the reason for changing this system in the first place). All that can be done on loading is 
			// to clear the departures and set dummy values.

			if(file->is_loading())
			{
				uint32 count = 0;
				file->rdwr_long(count);
				departures.clear();
				uint16 dummy_id = 0;
				sint64 dummy_departure_time = 0;
				for(uint i = 0; i < count; i ++)
				{
					file->rdwr_short(dummy_id);
					file->rdwr_longlong(dummy_departure_time);
					if(file->get_version() >= 110007)
					{
						uint8 player_count = 0;
						file->rdwr_byte(player_count);
						for(uint i = 0; i < player_count; i ++)
						{
							uint32 dummy_accumulated_distance;
							file->rdwr_long(dummy_accumulated_distance);
						}
					}
				}
			}

			if(file->is_saving())
			{
				// In theory, this could be set up when saving, but it is unlikely that mail version 12 games
				// can be made backwards compatible to version 11 or earlier in any event because of factory
				// entries in passenger packets, so this would be superfluous. 
				uint32 count = 0;
				file->rdwr_long(count);
			}
		}
		else // Extended version >= 12
		{
			departure_point_t departure_point;
			sint64 departure_time;
			uint32 accumulated_distance;
			if(file->is_saving())
			{
				uint32 count = departures.get_count();
				file->rdwr_long(count);
				FOR(departure_map, const& iter, departures)
				{
					departure_point = iter.key;
					departure_time = iter.value.departure_time;
					file->rdwr_short(departure_point.entry);
					file->rdwr_short(departure_point.reversed);
					file->rdwr_longlong(departure_time);
					if(file->get_version() >= 110007)
					{
						uint8 player_count = MAX_PLAYER_COUNT + 2;
						file->rdwr_byte(player_count);
						for(int i = 0; i < player_count; i ++)
						{
							accumulated_distance = iter.value.get_way_distance(i);
							file->rdwr_long(accumulated_distance);
						}
					}
				}

				
				count = departures_already_booked.get_count();
				file->rdwr_long(count);
				uint16 x;
				uint16 y;
				FOR(departure_time_map, const& iter, departures_already_booked)
				{
					x = iter.key.x;
					y = iter.key.y;
					departure_time = iter.value;
					file->rdwr_short(x);
					file->rdwr_short(y);
					file->rdwr_longlong(departure_time);
				}

				uint16 total;
				uint16 ave_count;
				count = journey_times_between_schedule_points.get_count();
				file->rdwr_long(count);
				FOR(timings_map, const& iter, journey_times_between_schedule_points)
				{
					departure_point = iter.key;
					total = iter.value.total;
					ave_count = iter.value.count;
					file->rdwr_short(departure_point.entry);
					file->rdwr_short(departure_point.reversed);
					file->rdwr_short(total);
					file->rdwr_short(ave_count);
				}
			}

			else if(file->is_loading())
			{
				uint32 count = 0;
				file->rdwr_long(count);
				// Do NOT use clear_departures here, as this clears the estimated times
				// in the halts that have already been loaded, as the halts load before 
				// the convoys.
				departures.clear();
				departures_already_booked.clear();
				journey_times_between_schedule_points.clear();
				for(int i = 0; i < count; i ++)
				{
					file->rdwr_short(departure_point.entry);
					file->rdwr_short(departure_point.reversed);
					file->rdwr_longlong(departure_time);
					departure_data_t dep;
					dep.departure_time = departure_time;
					if(file->get_version() >= 110007)
					{
						uint8 player_count = 0;
						file->rdwr_byte(player_count);
						for(uint i = 0; i < player_count; i ++)
						{
							uint32 accumulated_distance;
							file->rdwr_long(accumulated_distance);
							dep.set_distance(i, accumulated_distance);
						}
					}
					departures.put(departure_point, dep);
				}

				file->rdwr_long(count);
				uint16 x;
				uint16 y;
				sint64 departure_time;
				for(int i = 0; i < count; i ++)
				{
					file->rdwr_short(x);
					file->rdwr_short(y);
					file->rdwr_longlong(departure_time);

					id_pair pair(x, y);
					departures_already_booked.put(pair, departure_time);
				}

				file->rdwr_long(count);
				uint16 total;
				uint16 ave_count;
				for(int i = 0; i < count; i ++)
				{
					file->rdwr_short(departure_point.entry);
					file->rdwr_short(departure_point.reversed);
					file->rdwr_short(total);
					file->rdwr_short(ave_count);
					average_tpl<uint16> ave;
					ave.total = total;
					ave.count = ave_count;
					journey_times_between_schedule_points.put(departure_point, ave);
				}
			}
		}
		const uint8 count = file->get_version() < 103000 ? CONVOI_DISTANCE : MAX_CONVOI_COST;
		for(uint8 i = 0; i < count; i ++)
		{
			file->rdwr_long(rolling_average[i]);
			file->rdwr_short(rolling_average_count[i]);
		}
	}
	else if(file->is_loading())
	{
		clear_departures();
	}

	if(file->get_extended_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_short(livery_scheme_index);
	}
	else
	{
		livery_scheme_index = 0;
	}

	if(file->get_extended_version() >= 10)
	{
		if(file->is_saving())
		{
			uint32 count = average_journey_times.get_count();
			file->rdwr_long(count);

			if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
			{
				// New 32-bit journey times
				FOR(journey_times_map, const& iter, average_journey_times)
				{
					id_pair idp = iter.key;
					file->rdwr_short(idp.x);
					file->rdwr_short(idp.y);
					uint32 value = iter.value.count;
					file->rdwr_long(value);
					value = iter.value.total;
					file->rdwr_long(value);
				}
			}
			else
			{
				FOR(journey_times_map, const& iter, average_journey_times)
				{
					id_pair idp = iter.key;
					file->rdwr_short(idp.x);
					file->rdwr_short(idp.y);
					sint16 value = (sint16)iter.value.count;
					file->rdwr_short(value);
					if (iter.value.total == UINT32_MAX_VALUE)
					{
						value = 65535;
					}
					else
					{
						value = (sint16)iter.value.total;
					}
					file->rdwr_short(value);
				}
			}
		}
		else
		{
			uint32 count = 0;
			file->rdwr_long(count);
			average_journey_times.clear();

			if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
			{
				// New 32-bit journey times
				for (uint32 i = 0; i < count; i++)
				{
					id_pair idp;
					file->rdwr_short(idp.x);
					file->rdwr_short(idp.y);

					uint32 count;
					uint32 total;
					file->rdwr_long(count);
					file->rdwr_long(total);

					average_tpl<uint32> average;
					average.count = count;
					average.total = total;

					average_journey_times.put(idp, average);
				}
			}
			else
			{
				for (uint32 i = 0; i < count; i++)
				{
					id_pair idp;
					file->rdwr_short(idp.x);
					file->rdwr_short(idp.y);

					uint16 count;
					uint16 total;
					file->rdwr_short(count);
					file->rdwr_short(total);				

					average_tpl<uint32> average;
					average.count = (uint32)count;
					if (total == 65535)
					{
						average.total = UINT32_MAX_VALUE;
					}
					else
					{
						average.total = (uint32)total;
					}

					average_journey_times.put(idp, average);
				}
			}
		}

		file->rdwr_longlong(arrival_time);

		//arrival_to_first_stop table
		//
		uint8 items_count = arrival_to_first_stop.get_count();
		file->rdwr_byte(items_count);
		if (file->is_loading() )
		{
			for (uint8 i = 0; i < items_count; ++i )
			{
				sint64 item_value;
				file->rdwr_longlong( item_value );
				arrival_to_first_stop.add_to_tail(item_value);
			}
		}
		else
		{
			for (uint8 i = 0; i < items_count; ++i )
			{
				sint64 item_value = arrival_to_first_stop[i];
				file->rdwr_longlong( item_value );
			}
		}

		file->rdwr_long(current_loading_time);
		if(file->get_version() >= 111000)
		{
			file->rdwr_byte(no_route_retry_count);
			if(no_route_retry_count > 7)
			{
				dbg->warning("convoi_t::rdwr()","Convoi %d: no_route_retry_count out of bounds:  = %d, setting to 7",self.get_id(), no_route_retry_count);
				no_route_retry_count = 7;
			}
		}
	}

	else
	{
		arrival_time = welt->get_ticks();
	}

	if(file->get_version() >= 111001 && file->get_extended_version() == 0)
	{
		uint32 dummy = 0;
		file->rdwr_long( dummy ); // Was distance_since_last_stop
		file->rdwr_long( dummy ); // Was sum_speed_limit
	}


	if(file->get_version()>=111002 && file->get_extended_version() == 0)
	{
		// Was maxspeed_average_count
		uint32 dummy = 0;
		file->rdwr_long(dummy);
	}

	if(file->get_version() >= 111002 && file->get_extended_version() >= 10)
	{
		file->rdwr_short(last_stop_id);
		v.rdwr(file);
	}

	if(  file->get_version()>=111003  ) {
		file->rdwr_short( next_stop_index );
		file->rdwr_short( next_reservation_index );
	}

#ifdef SPECIAL_RESCUE_12_5
	if(file->get_extended_version() >= 12 && file->is_saving()) 
#else
	if(file->get_extended_version() >= 12)
#endif
	{
		file->rdwr_bool(needs_full_route_flush);
	}

#ifdef SPECIAL_RESCUE_12_6
	if(file->get_extended_version() >= 12 && file->is_saving()) 
#else
	if(file->get_extended_version() >= 12)
#endif
	{
		bool ic = is_choosing;
		file->rdwr_bool(ic);
		is_choosing = ic;

		file->rdwr_long(max_signal_speed); 
		last_signal_pos.rdwr(file); 

		if(file->get_extended_revision() >= 8)
		{
			// This allows for compatibility with saves from the reserving-through branch
			uint8 has_reserved = 0;
			file->rdwr_byte(has_reserved);
		}
	}

	// This must come *after* all the loading/saving.
	if(  file->is_loading()  ) {
		recalc_catg_index();
	}

	if (akt_speed < 0)
	{
		set_akt_speed(0);
	}
}


void convoi_t::show_info()
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


#if 0
void convoi_t::info(cbuffer_t & buf) const
{
	const vehicle_t* v = front();
	if (v != NULL) {
		char tmp[128];

		buf.printf("\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->get_desc()->get_topspeed(), get_running_cost(welt) / 100.0F);

		buf.printf(" %s: %ikW\n", translator::translate("Leistung"), sum_power );

		buf.printf(" %s: %i (%i) t\n", translator::translate("Gewicht"), sum_weight, sum_gesamtweight-sum_weight );

		buf.printf(" %s: ", translator::translate("Gewinn")  );

		money_to_string( tmp, (double)jahresgewinn );
		buf.append(tmp);
		buf.append("\n");
	}
}
#endif

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
			const vehicle_t* v = vehicle[i];

			// first add to capacity indicator
			const goods_desc_t* ware_desc = v->get_desc()->get_freight_type();
			const uint16 menge = v->get_desc()->get_capacity();
			if(menge>0  &&  ware_desc!=goods_manager_t::none) {
				max_loaded_waren[ware_desc->get_index()] += menge;
			}

			// then add the actual load
			FOR(slist_tpl<ware_t>, ware, v->get_cargo())
			{
				// if != 0 we could not join it to existing => load it
				if(ware.menge != 0) 
				{
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

	// manipulation of schedule not allowd while:
	// - just starting
	// - a line update is pending
	// - the convoy is in the process of finding a route (multi-threaded)
	if(  (is_locked()  ||  line_update_pending.is_bound())  &&  get_owner()==welt->get_active_player()  ) {
		if (show) {
			create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		}
		return;
	}

	set_akt_speed(0);	// stop the train ...
	if(state!=INITIAL) {
		state = EDIT_SCHEDULE;
	}
	wait_lock = 25000;
	alte_direction = front()->get_direction();

	// Added by : Knightly
	// Purpose  : To keep a copy of the original schedule before opening schedule window
	if (schedule)
	{
		old_schedule = schedule->copy();
	}

	if(  show  ) {
		// Fahrplandialog oeffnen
		create_win( new schedule_gui_t(schedule,get_owner(),self), w_info, (ptrdiff_t)schedule );
		// TODO: what happens if no client opens the window??
	}
	if(schedule)
	{
		schedule->start_editing();
	}
}


bool convoi_t::pruefe_alle() //"examine all" (Babelfish)
/**
 * Check validity of convoi with respect to vehicle constraints
 */
{
	bool ok = vehicle_count == 0  ||  front()->get_desc()->can_follow(NULL);
	unsigned i;

	const vehicle_t* pred = front();
	for(i = 1; ok && i < vehicle_count; i++) {
		const vehicle_t* v = vehicle[i];
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
void convoi_t::laden() //"load" (Babelfish)
{
	// Calculate average speed and journey time
	// @author: jamespetts
	
	halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_eintrag().pos, owner);
	halthandle_t this_halt = haltestelle_t::get_halt(get_pos(), owner);

	if(!this_halt.is_bound())
	{
		state = CAN_START;
		dbg->error("void convoi_t::laden()", "Trying to load at halt %s when not at a halt", halt.is_bound() ? halt->get_name() : "none");
		return;
	}

	// The calculation of the journey distance does not need to use normalised halt locations for comparison, so
	// a more accurate distance can be used. Query whether the formula from halt_detail.cc should be used here instead
	// (That formula has the effect of finding the distance between the nearest points of two halts).

	departure_point_t departure(schedule->get_current_stop(), reverse_schedule);
	const koord3d pos3d = front()->get_pos();
	koord pos;
	halthandle_t new_halt = haltestelle_t::get_halt(pos3d, front()->get_owner());
	if(new_halt.is_bound() || !halt.is_bound())
	{
		pos = pos3d.get_2d();
		if(halt != new_halt)
		{
			halt = new_halt;
		}
	}
	else
	{
		pos = halt->get_basis_pos();
	}

	const uint32 journey_distance = shortest_distance(front()->get_pos().get_2d(), front()->last_stop_pos.get_2d());
	const uint16 this_halt_id = halt.get_id();

	// last_stop_pos will be set to get_pos().get_2d() in hat_gehalten (called from inside halt->request_loading later)
	// so code inside if will be executed once. At arrival time.
	minivec_tpl<uint8> departure_entries_to_remove(schedule->get_count());
	bool clear_departures = false;

	if(journey_distance > 0 && last_stop_id != this_halt_id)
	{		
		arrival_time = welt->get_ticks();
		inthashtable_tpl<uint16, sint64> best_times_in_schedule; // Key: halt ID; value: departure time.
		FOR(departure_map, const& iter, departures)
		{			
			const sint64 journey_time_ticks = arrival_time - iter.value.departure_time;
			if(iter.key.entry > schedule->get_count() - 1)
			{
				// For some reason, the schedule has been changed but the departures have not been cleared.
				// Abort this operation, or else it will crash on the next line, and clear departuers.
				departures.clear();
				break;
			}
			const koord3d halt_position = schedule->entries.get_element(iter.key.entry).pos; 
			const halthandle_t departure_halt = haltestelle_t::get_halt(halt_position, front()->get_owner()); 
			if(departure_halt.is_bound())
			{
				if(best_times_in_schedule.is_contained(departure_halt.get_id()))
				{
					// There is more than one departure from this stop to here. Check which is the fastest.
					const sint64 other_journey_time_ticks = arrival_time - best_times_in_schedule.get(departure_halt.get_id());
					if(departures_already_booked.get(id_pair(departure_halt.get_id(), this_halt_id)) != iter.value.departure_time)
					{
						if(journey_time_ticks < other_journey_time_ticks)
						{
							best_times_in_schedule.set(departure_halt.get_id(), iter.value.departure_time);
						}
					}
					else
					{
						// If the best departure is already booked, do not allow lesser departures to be registered.
						best_times_in_schedule.remove(departure_halt.get_id());
					}
				}
				else if((departures_already_booked.get(id_pair(departure_halt.get_id(), this_halt_id)) < iter.value.departure_time))
				{
					best_times_in_schedule.put(departure_halt.get_id(), iter.value.departure_time);
				}
			}
		}

		bool rev = !reverse_schedule;
		uint8 schedule_entry = schedule->get_current_stop();
		schedule->increment_index(&schedule_entry, &rev); 
		departure_point_t this_departure(schedule_entry, !rev);
		sint64 latest_journey_time;
		const bool departure_missing = !departures.is_contained(this_departure);
		const sint32 journey_distance_meters = journey_distance * welt->get_settings().get_meters_per_tile();
		if(departure_missing)
		{
			//Fall back to ultimate backup: half maximum speed.
			const sint32 assumed_average_speed = speed_to_kmh(get_min_top_speed()) * 5; // (Equivalent to / 2 * 10)
			latest_journey_time = (journey_distance_meters * 6) / assumed_average_speed;
		}
		else
		{
			latest_journey_time = welt->ticks_to_tenths_of_minutes(arrival_time - departures.get(this_departure).departure_time);
		}
		if(latest_journey_time <= 0)
		{
			// Necessary to prevent divisions by zero.
			// This code should never be reached.
			dbg->error("void convoi_t::laden()", "Journey time (%i) is zero or less"); 
			latest_journey_time = 1;
		}

		if(journey_times_between_schedule_points.is_contained(this_departure))
		{
			// The autoreduce function has the effect of making older timings less and less significant to this figure.
			journey_times_between_schedule_points.access(this_departure)->add_autoreduce((uint16)latest_journey_time, timings_reduction_point);
		}
		else
		{
			average_tpl<uint16> ave;
			ave.add((uint16)latest_journey_time);
			journey_times_between_schedule_points.put(this_departure, ave);
		}

		const sint32 average_speed = (journey_distance_meters * 3) / ((sint32)latest_journey_time * 5);

		// For some odd reason, in some cases, laden() is called when the journey time is
		// excessively low, resulting in perverse average speeds and journey times.
		// Testing shows that this still recurs even after the enhanced point to point
		// system (April 2014).
		if(average_speed <= get_vehicle_summary().max_speed)
		{
			book(average_speed, CONVOI_AVERAGE_SPEED);
			if(average_speed > welt->get_record_speed(vehicle[0]->get_waytype())) 
			{
				welt->notify_record(self, average_speed, pos);
			}

			typedef inthashtable_tpl<uint16, sint64> int_map;
			FOR(int_map, const& iter, best_times_in_schedule)
			{
				id_pair pair(iter.key, this_halt_id);
				const sint32 this_journey_time = (uint32)welt->ticks_to_tenths_of_minutes(arrival_time - iter.value);

				departures_already_booked.set(pair, iter.value);
				const average_tpl<uint32> *average_check = average_journey_times.access(pair);
				if(!average_check)
				{
					average_tpl<uint32> average_new;
					average_new.add(this_journey_time);
					average_journey_times.put(pair, average_new);
				}
				else
				{
					average_journey_times.access(pair)->add_autoreduce(this_journey_time, timings_reduction_point);
				}
				if(line.is_bound())
				{
					const average_tpl<uint32> *average = get_average_journey_times().access(pair);
					if(!average)
					{
						average_tpl<uint32> average_new;
						average_new.add(this_journey_time);
						get_average_journey_times().put(pair, average_new);
					}
					else
					{
						get_average_journey_times().access(pair)->add_autoreduce(this_journey_time, timings_reduction_point);
					}
				}
			}
		}

		// Recalculate comfort
		const uint8 comfort = get_comfort();
		if(comfort)
		{
			book(get_comfort(), CONVOI_COMFORT);
		}

		FOR(departure_map, & iter, departures)
		{
			// Accumulate distance since each previous stop on the schedule
			iter.value.add_overall_distance(journey_distance);
		}

		if(state == EDIT_SCHEDULE) //"ENTER SCHEDULE" (Google)
		{
			return;
		}
	}
	
	if(halt.is_bound())
	{
		//const player_t* owner = halt->get_owner(); //"get owner" (Google)
		const grund_t* gr = welt->lookup(schedule->get_current_eintrag().pos);
		const weg_t *w = gr ? gr->get_weg(schedule->get_waytype()) : NULL;
		bool tram_stop_public = false;
		if(schedule->get_waytype() == tram_wt)
		{
			const grund_t* gr = welt->lookup(schedule->get_current_eintrag().pos);
			const weg_t *street = gr ? gr->get_weg(road_wt) : NULL;
			if(street && (street->get_owner() == get_owner() || street->get_owner() == NULL || street->get_owner()->allows_access_to(get_owner()->get_player_nr())))
			{
				tram_stop_public = true;
			}
		}
		const player_t *player = w ? w->get_owner() : NULL;
		if(halt->check_access(get_owner()) || (w && player == NULL) || (w && player->allows_access_to(get_owner()->get_player_nr())) || tram_stop_public)
		{
			// loading/unloading ...
			// NOTE: Revenue is calculated here.
			halt->request_loading(self);
		}
	}
	else
	{
		// If there is no halt here, nothing will call request loading and the state can never be changed.
		state = CAN_START;
	}

	if(wait_lock == 0)
	{
		wait_lock = WTT_LOADING;
	}
}

sint64 convoi_t::calc_revenue(const ware_t& ware, array_tpl<sint64> & apportioned_revenues)
{
	// Cannot not charge for journey if the journey distance is more than a certain proportion of the straight line distance.
	// This eliminates the possibility of cheating by building circuitous routes, or the need to prevent that by always using
	// the straight line distance, which makes the game difficult and unrealistic.
	// If the origin has been deleted since the packet departed, then the best that we can do is guess by
	// trebling the distance to the last stop.
	uint32 max_distance;
	if(ware.get_last_transfer().is_bound()) 
	{
		max_distance = shortest_distance(ware.get_last_transfer()->get_basis_pos(), front()->get_pos().get_2d()) * 2;
	}
	else
	{
		max_distance = shortest_distance(front()->last_stop_pos.get_2d(), front()->get_pos().get_2d()) * 3;
	}
	// Because the "departures" hashtable now contains not halts but timetable entries, it is necessary to iterate
	// through the timetable to find the last time that this convoy called at the stop in question.

	uint8 entry = schedule->get_current_stop();
	bool rev = !reverse_schedule; // Must be negative as going through the schedule backwards: must reverse this when used.
	const int schedule_count = schedule->is_mirrored() ? schedule->get_count() * 2 : schedule->get_count();
	departure_data_t dep;
	for(int i = 0; i < schedule_count; i++)
	{	
		schedule->increment_index(&entry, &rev);
		const uint16 halt_id = haltestelle_t::get_halt(schedule->entries[entry].pos, owner).get_id();
		if(halt_id == ware.get_last_transfer().get_id())
		{
			dep = departures.get(departure_point_t(entry, !rev));
			break;
		}
	}

	uint32 travel_distance = dep.get_overall_distance();
	if(travel_distance == 0)
	{
		// Something went wrong, make a wild guess
		// (This can happen when the departure halt has been deleted
		// or made inaccessible to this player since departure).
		travel_distance = max_distance / 2;
	}
	const uint32 travel_distance_meters = travel_distance * welt->get_settings().get_meters_per_tile();

	const uint32 revenue_distance = min(travel_distance, max_distance);
	// First try to get the journey minutes and average speed
	// for the point to point trip.  If that fails use line average.
	// (neroden really believes we should use the minutes and speed for THIS trip.)
	// (It saves vast amounts of computational effort,
	//  and gives the player a quicker response to improved service.)
	sint64 journey_tenths = 0;
	sint64 average_speed;
	bool valid_journey_time = false;
	
	if(ware.get_last_transfer().is_bound())
	{
		const grund_t* gr = welt->lookup(front()->get_pos());
		if (gr) 
		{
			id_pair my_ordered_pair = id_pair(ware.get_last_transfer().get_id(), gr->get_halt().get_id());
			journey_tenths = get_average_journey_times().get(my_ordered_pair).get_average();
			if (journey_tenths != 0)
			{
				// No unreasonably short journeys...
				average_speed = kmh_from_meters_and_tenths(travel_distance_meters, journey_tenths);
				if(average_speed > speed_to_kmh(get_min_top_speed()))
				{
					dbg->warning("sint64 convoi_t::calc_revenue", "Average speed (%i) for %s exceeded maximum speed (%i); falling back to overall average", average_speed, get_name(), speed_to_kmh(get_min_top_speed()));
				}
				else 
				{
					// We seem to have a believable speed...
					if(average_speed == 0)
					{
						average_speed = 1;
					}
					valid_journey_time = true;
				}
			}
		}
	}

	if(!valid_journey_time)
	{
		// Fallback to the overall average speed for the line under several situations:
		// - if there are no data for point-to-point timings;
		// - if the point-to-point timings are less than 1/10 of a minute (unreasonably short)
		// - if the average speed is faster than the top speed of the convoi (absurdity)
		if(!line.is_bound()) 
		{
			// No line - must use convoy
			if(financial_history[1][CONVOI_AVERAGE_SPEED] == 0) {
				average_speed = financial_history[0][CONVOI_AVERAGE_SPEED];
			}
			else
			{
				average_speed = financial_history[1][CONVOI_AVERAGE_SPEED];
			}
		}
		else
		{
			if(line->get_finance_history(1, LINE_AVERAGE_SPEED) == 0) {
				average_speed = line->get_finance_history(0, LINE_AVERAGE_SPEED);
			}
			else
			{
				average_speed = line->get_finance_history(1, LINE_AVERAGE_SPEED);
			}
		}
		if(average_speed == 0)
		{
			average_speed = 1;
		}
		journey_tenths = tenths_from_meters_and_kmh(travel_distance_meters, average_speed);
	}

	sint64 starting_distance;
	if (ware.get_origin().is_bound())
	{
		sint64 distance_from_ultimate_origin
			= (sint64)shortest_distance(ware.get_origin()->get_basis_pos(), front()->get_pos().get_2d());
		starting_distance = distance_from_ultimate_origin - (sint64)revenue_distance;
		if (starting_distance < 0)
		{
			// Artifact of convoluted routing
			starting_distance = 0;
		}
	}
	else
	{
		starting_distance = 0;
	}

	const uint32 revenue_distance_meters = revenue_distance * welt->get_settings().get_meters_per_tile();
	const uint32 starting_distance_meters = starting_distance * welt->get_settings().get_meters_per_tile();

	const sint64 ref_speed = welt->get_average_speed(front()->get_desc()->get_waytype());
	const sint64 relative_speed_percentage = (100ll * average_speed) / ref_speed - 100ll;

	const goods_desc_t* goods = ware.get_desc();

	sint64 fare;
	if (ware.is_passenger())
	{
		// Comfort
		// First get our comfort.

		// Again, use the average for the line for revenue computation.
		// (neroden believes we should use the ACTUAL comfort on THIS trip;
		// although it is "less realistic" it provides faster revenue responsiveness
		// for the player to improvements in comfort)

		uint8 comfort = 100;
		if(line.is_bound())
		{
			if(line->get_finance_history(1, LINE_COMFORT) < 1)
			{
				comfort = line->get_finance_history(0, LINE_COMFORT);
			} 
			else 
			{
				comfort = line->get_finance_history(1, LINE_COMFORT);
			}
		} else {
			// No line - must use convoy
			if(financial_history[1][CONVOI_COMFORT] < 1)
			{
				comfort = financial_history[0][CONVOI_COMFORT];
			} 
			else 
			{
				comfort = financial_history[1][CONVOI_COMFORT];
			}
		}

		// Now, get our catering level.
		uint8 catering_level = get_catering_level(goods->get_catg_index());

		// Finally, get the fare.
		fare = goods->get_fare_with_comfort_catering_speedbonus( welt, comfort, catering_level, journey_tenths,
					(sint16)relative_speed_percentage, revenue_distance_meters, starting_distance_meters);
	} 
	else if(ware.is_mail())
	{
		// Make an arbitrary comfort, mail doesn't care about comfort
		const uint8 comfort = 0;
		// Get our "TPO" level.
		uint8 catering_level = get_catering_level(goods->get_catg_index());
		// Finally, get the fare.
		fare = goods->get_fare_with_comfort_catering_speedbonus( welt, comfort, catering_level, journey_tenths,
					(sint16)relative_speed_percentage, revenue_distance_meters, starting_distance_meters);
	} 
	else
	{
		// Freight ignores comfort and catering and TPO.
		// So here we can skip the complicated version for speed.
		fare = goods->get_fare_with_speedbonus( (sint16)relative_speed_percentage, revenue_distance_meters, starting_distance_meters);
	}
	// Note that fare comes out in units of 1/4096 of a simcent, for computational precision

	// Finally (!!) multiply by the number of items.
	const sint64 revenue = fare * (sint64)ware.menge;

	// Now apportion the revenue.
	uint32 total_way_distance = 0;
	for(uint8 i = 0; i <= MAX_PLAYER_COUNT; i ++)
	{
		total_way_distance += dep.get_way_distance(i);
	}
	// The apportioned revenue array is passed in.  It should be the right size already.
	// Make sure our returned array is the right size (should do nothing)
	apportioned_revenues.resize(MAX_PLAYER_COUNT);
	// It will have some accumulated revenues in it from unloading previous vehicles in the same convoi.
	for(uint8 i = 0; i < MAX_PLAYER_COUNT; i ++)
	{
		if(owner->get_player_nr() == i)
		{
			// Never apportion revenue to the convoy-owning player
			continue;
		}
		// Now, if the player's tracks were actually used, apportion revenue
		uint32 player_way_distance = dep.get_way_distance(i);
		if(player_way_distance > 0)
		{
			// We allocate even for players who may not exist; we'll check before paying them.
			apportioned_revenues[i] += (revenue * player_way_distance) / total_way_distance;
		}
	}

	return revenue;
}


/**
 * convoi an haltestelle anhalten
 * "Convoi stop at stop" (Google translations)
 * @author Hj. Malthaner
 *
 * V.Meyer: minimum_loading is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(halthandle_t halt)
{
	sint64 accumulated_revenue = 0;

	// This holds the revenues as apportioned to different players by track
	// Initialize it to the correct size and blank out all entries
	// It will be added to by ::entladen for each vehicle
	array_tpl<sint64> apportioned_revenues (MAX_PLAYER_COUNT, 0);

	grund_t *gr = welt->lookup(front()->get_pos());

	// now find out station length
	int station_length=0;
	if(  gr->is_water()  ) {
		// harbour has any size
		station_length = 24*16;
	}
	else
	{
		// calculate real station length
		koord zv = koord( ribi_t::backward(front()->get_direction()) );
		koord3d pos = front()->get_pos();
		const grund_t *grund = welt->lookup(pos);
		if(  grund->get_weg_yoff()==TILE_HEIGHT_STEP  )
		{
			// start on bridge?
			pos.z ++;
		}
		while(  grund  &&  grund->get_halt() == halt  ) {
			station_length += OBJECT_OFFSET_STEPS;
			pos += zv;
			grund = welt->lookup(pos);
			if(  grund==NULL  )
			{
				grund = welt->lookup(pos-koord3d(0,0,1));
				if(  grund &&  grund->get_weg_yoff()!=TILE_HEIGHT_STEP  )
				{
					// not end/start of bridge
					break;
				}
			}
		}
	}

	last_stop_id = halt.get_id();

	halt->update_alternative_seats(self);
	// only load vehicles in station

	// Save the old stop position; don't load/unload again if we don't move.
	const koord3d old_last_stop_pos = front()->last_stop_pos;

	uint16 changed_loading_level = 0;
	int number_loadable_vehicles = vehicle_count; // Will be shortened for short platform

	// We only unload & load vehicles which are within the station.
	// To fix: this creates undesired behavior for long trains, because the
	// cars in the back will not load/unload in "short" platforms.
	//
	// Proposed fix: load passengers from back of the train to the front,
	// selectively delaying loading passengers destined for "short" platforms
	// in order to get them into the cars in the front of the station.  Just like
	// real conductors do.
	//
	// This will require tracking platform length in the schedule object.
	// --neroden

	// First, unload vehicles.

	uint8 convoy_length = 0;
	for(int i = 0; i < vehicle_count ; i++)
	{
		vehicle_t* v = vehicle[i];

		convoy_length += v->get_desc()->get_length();
		if(convoy_length > station_length)
		{
			number_loadable_vehicles = i;
			break;
		}

		// Reset last_stop_pos for all vehicles.
		koord3d pos = v->get_pos();
		if(haltestelle_t::get_halt(pos, v->get_owner()).is_bound())
		{
			v->last_stop_pos = pos;
		}
		else
		{
			v->last_stop_pos = halt->get_basis_pos3d();
		}
		// hat_gehalten can be called when the convoy hasn't moved... at all.
		// We should avoid the unloading code when this happens (for speed).
		if(  old_last_stop_pos != front()->get_pos()  ) {
			//Unload
			sint64 revenue_from_unloading = 0;
			uint16 amount_unloaded = v->unload_cargo(halt, revenue_from_unloading, apportioned_revenues);
			changed_loading_level += amount_unloaded;

			// Convert from units of 1/4096 of a simcent to units of ONE simcent.  Should be FAST (powers of two).
			sint64 revenue_cents_from_unloading = (revenue_from_unloading + 2048ll) / 4096ll;
			if (amount_unloaded && revenue_cents_from_unloading == 0) {
				// if we unloaded something, provide some minimum revenue.  But not if we unloaded nothing.
				revenue_cents_from_unloading = 1;
			}
			// This call needs to be here, per-vehicle, in order to record different freight types properly.
			owner->book_revenue( revenue_cents_from_unloading, front()->get_pos().get_2d(), get_schedule()->get_waytype(), v->get_cargo_type()->get_index() );
			// The finance code will add up the on-screen messages
			// But add up the total for the port and station use charges
			accumulated_revenue += revenue_cents_from_unloading;
			book(revenue_cents_from_unloading, CONVOI_PROFIT);
			book(revenue_cents_from_unloading, CONVOI_REVENUE);
		}
	}
	if(no_load)
	{
		for(int i = 0; i < number_loadable_vehicles ; i++)
		{
			vehicle_t* v = vehicle[i];
			// do not load -- but call load_cargo() to recalculate vehicle weight
			v->load_cargo(halthandle_t());
		}
	}
	else // not "no_load"
	{
		bool skip_catg[255] = {false};

		lines_loaded_t line_data;
		line_data.line = get_line();
		if(line_data.line.is_bound())
		{
			line_data.reversed = is_reversed();
			line_data.current_stop = get_schedule()->get_current_stop();

			// flag all categories that have already had all available freight loaded onto convois on this line from this halt this step
			FOR(vector_tpl<lines_loaded_t>, const& i, halt->access_lines_loaded())
			{
				if(i.line == line_data.line  &&  i.reversed == line_data.reversed  &&  i.current_stop == line_data.current_stop)
				{
					skip_catg[i.catg_index] = true;
				}
			}
		}
		// Load vehicles to their regular level. And then load vehicles to their overcrowded level.
		// Modified by TurfIt, March 2014
		for(int j = 0;  j < 2;  j++)
		{
			const bool overcrowd = (j == 1);
			for(int i = 0; i < number_loadable_vehicles ; i++)
			{
				vehicle_t* v = vehicle[i];
				const uint8 catg_index = v->get_cargo_type()->get_catg_index();
				if(!skip_catg[catg_index])
				{
					bool skip_convois = false;
					bool skip_vehicles = false;
					changed_loading_level += v->load_cargo(halt, overcrowd, &skip_convois, &skip_vehicles);
					if(skip_convois  ||  skip_vehicles)
					{
						// not enough freight was available to fill vehicle, or the stop can't supply this type of cargo ..> don't try to load this category again from this halt onto vehicles in convois on this line this step
						skip_catg[catg_index] = true;
						if(!skip_vehicles  &&  line_data.line.is_bound())
						{
							line_data.catg_index = catg_index;
							halt->access_lines_loaded().append( line_data );
						}
					}
				}
			}
		}
	}

	if(changed_loading_level > 0)
	{
		freight_info_resort = true;
	}
	if(  changed_loading_level  ) {
		halt->recalc_status();
	}

	// any loading went on?
	calc_loading();
	loading_limit = schedule->get_current_eintrag().minimum_loading; // minimum_loading = max. load.
	const bool wait_for_time = schedule->get_current_eintrag().wait_for_time; 
	highest_axle_load = calc_highest_axle_load(); // Bernd Gabriel, Mar 10, 2010: was missing.
	if(  old_last_stop_pos != front()->get_pos()  )
	{
		// Only calculate the loading time once, on arriving at the stop:
		// otherwise, the change in the vehicle's load will not be calculated
		// correctly.
		current_loading_time = calc_current_loading_time(changed_loading_level);
	}

	if(accumulated_revenue)
	{
		jahresgewinn += accumulated_revenue; 

		// Check the apportionment of revenue.
		// The proportion paid to other players is
		// not deducted from revenue, but rather
		// added as a "WAY_TOLL" cost.

		for(int i = 0; i < MAX_PLAYER_COUNT; i ++)
		{
			if(i == owner->get_player_nr())
			{
				continue;
			}
			player_t* player = welt->get_player(i);
			// Only pay tolls to players who actually exist
			if(player && apportioned_revenues[i])
			{
				// This is the screwy factor of 3000 -- fix this sometime
				sint64 modified_apportioned_revenue = (apportioned_revenues[i] + 1500ll) / 3000ll;
				modified_apportioned_revenue *= welt->get_settings().get_way_toll_revenue_percentage();
				modified_apportioned_revenue /= 100;
				if(welt->get_active_player() == player)
				{
					player->add_money_message(modified_apportioned_revenue, front()->get_pos().get_2d());
				}
				player->book_toll_received(modified_apportioned_revenue, get_schedule()->get_waytype() );
				owner->book_toll_paid(-modified_apportioned_revenue, get_schedule()->get_waytype() );
				book(-modified_apportioned_revenue, CONVOI_PROFIT);
			}
		}

		//  Apportion revenue for air/sea ports
		if(halt.is_bound() && halt->get_owner() != owner)
		{
			sint64 port_charge = 0;
			if(gr->is_water())
			{
				// This must be a sea port.
				port_charge = (accumulated_revenue * welt->get_settings().get_seaport_toll_revenue_percentage()) / 100;
			}
			else if(front()->get_desc()->get_waytype() == air_wt)
			{
				// This is an aircraft - this must be an airport.
				port_charge = (accumulated_revenue * welt->get_settings().get_airport_toll_revenue_percentage()) / 100;
			}
			if (port_charge != 0) {
				if(welt->get_active_player() == halt->get_owner())
				{
					halt->get_owner()->add_money_message(port_charge, front()->get_pos().get_2d());
				}
				halt->get_owner()->book_toll_received(port_charge, get_schedule()->get_waytype() );
				owner->book_toll_paid(-port_charge, get_schedule()->get_waytype() );
				book(-port_charge, CONVOI_PROFIT);
			}
		}
	}

	if(state == REVERSING)
	{
		return;
	}

	const sint64 now = welt->get_ticks();
	if(arrival_time > now)
	{
		// This is a workaround for an odd bug the origin of which is as yet unclear.
		go_on_ticks = WAIT_INFINITE;
		arrival_time = now;
	}
	const uint32 reversing_time = schedule->get_current_eintrag().reverse > 0 ? calc_reverse_delay() : 0;
	bool running_late = false;
	sint64 go_on_ticks_waiting = WAIT_INFINITE;
	const sint64 earliest_departure_time = arrival_time + ((sint64)current_loading_time - (sint64)reversing_time);
	if(go_on_ticks == WAIT_INFINITE)
	{
		const sint64 departure_time = (arrival_time + (sint64)current_loading_time) - (sint64)reversing_time;
		if(haltestelle_t::get_halt(get_pos(), get_owner()) != haltestelle_t::get_halt(schedule->get_current_eintrag().pos, get_owner()))
		{
			// Sometimes, for some reason, the loading method is entered with the wrong schedule entry. Make sure that this does not cause
			// convoys to become stuck trying to get a full load at stops where this is not possible (freight consumers, etc.).
			loading_limit = 0;
		}
		if((!loading_limit || loading_level >= loading_limit) && !wait_for_time)
		{
			// Simple case: do not wait for a full load or a particular time.
			go_on_ticks = std::max(departure_time, arrival_time);
		}
		else 
		{
			// Wait for a % load or a spacing slot.
			sint64 go_on_ticks_spacing = WAIT_INFINITE;
			
			if(line.is_bound() && schedule->get_spacing() && line->count_convoys())
			{
				// Departures/month
				const sint64 spacing = welt->ticks_per_world_month / (sint64)schedule->get_spacing();
				const sint64 spacing_shift = (sint64)schedule->get_current_eintrag().spacing_shift * welt->ticks_per_world_month / (sint64)welt->get_settings().get_spacing_shift_divisor();
				const sint64 wait_from_ticks = ((now - spacing_shift) / spacing) * spacing + spacing_shift; // remember, it is integer division
				const sint64 queue_pos = halt.is_bound() ? halt->get_queue_pos(self) : 1ll;
				go_on_ticks_spacing = (wait_from_ticks + spacing * queue_pos) - reversing_time;
			}
			if(schedule->get_current_eintrag().waiting_time_shift > 0)
			{
				// Maximum wait time
				go_on_ticks_waiting = now + (welt->ticks_per_world_month >> (16ll - (sint64)schedule->get_current_eintrag().waiting_time_shift)) - (sint64)reversing_time;
			}
			go_on_ticks = std::min(go_on_ticks_spacing, go_on_ticks_waiting);
			go_on_ticks = std::max(departure_time, go_on_ticks);
			running_late = wait_for_time && (go_on_ticks_waiting < go_on_ticks_spacing);
			if(running_late)
			{
				go_on_ticks = earliest_departure_time;
			}
		}
	}

	// loading is finished => maybe drive on
	bool can_go = false;
	
	can_go = loading_level >= loading_limit && (now >= go_on_ticks || !wait_for_time);
	//can_go = can_go || (now >= go_on_ticks_waiting && !wait_for_time); // This is pre-14 August 2016 code
	can_go = can_go || (now >= go_on_ticks && !wait_for_time);
	can_go = can_go || running_late; 
	can_go = can_go || no_load;
	can_go = can_go && state != WAITING_FOR_CLEARANCE && state != WAITING_FOR_CLEARANCE_ONE_MONTH && state != WAITING_FOR_CLEARANCE_TWO_MONTHS;
	can_go = can_go && now > earliest_departure_time;
	if(can_go) {

		if(withdraw  &&  (loading_level==0  ||  goods_catg_index.empty())) {
			// destroy when empty
			self_destruct();
			return;
		}

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<vehicle_count; i++) {
			book(get_vehicle(i)->get_cargo_max()-get_vehicle(i)->get_total_cargo(), CONVOI_CAPACITY);
		}

		// Advance schedule
		advance_schedule();
		state = ROUTING_1;
	}

	// reset the wait_lock
	if(state == ROUTING_1)
	{
		wait_lock = 0;
	}
	else
	{
		
		if (loading_limit > 0 && !wait_for_time)
		{
			wait_lock = (earliest_departure_time - now) / 2;
		}
		else
		{
			wait_lock = (go_on_ticks - now) / 2;
		}
		// The random extra wait here is designed to avoid processing every convoy at once
		wait_lock += (self.get_id()) % 1024;
		if (wait_lock < 0 )
		{
			wait_lock = 0;
		}
		else if(wait_lock > 8192 && go_on_ticks == WAIT_INFINITE)
		{
			// This is needed because the above calculation (from Standard) produces excessively
			// large numbers on occasions due to the conversion in Extended of certain values
			// (karte_t::ticks and go_on_ticks) to sint64. It would be better ultimately to fix that,
			// but this seems to work for now.
			wait_lock = 8192;
		}
	}
}


sint64 convoi_t::calc_sale_value() const
{

	sint64 result = 0;

	for(uint i=0; i<vehicle_count; i++) {
		result += vehicle[i]->calc_sale_value();
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
	int seats_max = 0;
	int seats_menge = 0;

	for(unsigned i=0; i<vehicle_count; i++) {
		const vehicle_t* v = vehicle[i];
		if ( v->get_cargo_type() == goods_manager_t::passengers ) {
			seats_max += v->get_cargo_max();
			seats_menge += v->get_total_cargo();
		}
		else {
			fracht_max += v->get_cargo_max();
			fracht_menge += v->get_total_cargo();
		}
	}
	if (seats_max)
	{
		fracht_max += seats_max;
		fracht_menge += seats_menge;
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
	loading_limit = 0;	// will be set correctly from hat_gehalten() routine
	free_seats = seats_max > seats_menge ? seats_max - seats_menge : 0;

	// since weight has changed
	invalidate_weight_summary();
}


// return the current bonus speed
uint32 convoi_t::get_average_kmh() 
{
	halthandle_t halt = haltestelle_t::get_halt(schedule->get_current_eintrag().pos, owner);
	id_pair idp(last_stop_id, halt.get_id());
	average_tpl<uint32>* avr = get_average_journey_times().access(idp);
	return avr ? avr->get_average() : get_vehicle_summary().max_speed;
}


/**
 * Schedule convoi for self destruction. Will be executed
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
	if(front()) {
		front()->set_convoi(NULL);
	}

	if(  state == INITIAL  ) {
		// in depot => not on map
		for(  uint8 i = vehicle_count;  i-- != 0;  ) {
			vehicle[i]->set_flag( obj_t::not_on_map );
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

	// pay the current value, remove monthly maint
	waytype_t wt = front() ? front()->get_desc()->get_waytype() : ignore_wt;
	owner->book_new_vehicle( calc_sale_value(), get_pos().get_2d(), wt);

	for(  uint8 i = vehicle_count;  i-- != 0;  ) {
		if(  !vehicle[i]->get_flag( obj_t::not_on_map )  ) {
			// remove from rails/roads/crossings
			grund_t *gr = welt->lookup(vehicle[i]->get_pos());
			vehicle[i]->set_last( true );
			vehicle[i]->leave_tile();
			if(  gr  &&  gr->ist_uebergang()  ) {
				gr->find<crossing_t>()->release_crossing(vehicle[i]);
			}
			vehicle[i]->set_flag( obj_t::not_on_map );

		}
		player_t::add_maintenance(owner, -vehicle[i]->get_desc()->get_maintenance(), vehicle[i]->get_desc()->get_waytype());
		vehicle[i]->discard_cargo();
		vehicle[i]->cleanup(owner);
		delete vehicle[i];
	}
	vehicle_count = 0;

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
		"\nvehicle_count = %d\n"
		"wait_lock = %d\n"
		"owner_n = %d\n"
		"akt_speed = %d\n"
		"sp_soll = %d\n"
		"state = %d\n"
		"statename = %s\n"
		"alte_direction = %d\n"
		"jahresgewinn = %ld\n"	// %lld crashes mingw now, cast gewinn to long ...
		"name = '%s'\n"
		"line_id = '%d'\n"
		"schedule = '%p'",
		(int)vehicle_count,
		(int)wait_lock,
		(int)welt->sp2num(owner),
		(int)akt_speed,
		(int)sp_soll,
		(int)state,
		(const char *)(state_names[state]),
		(int)alte_direction,
		(long)(jahresgewinn/100),
		(const char *)name_and_id,
		line.is_bound() ? line.get_id() : 0,
		(const void *)schedule );
}


void convoi_t::book(sint64 amount, convoi_cost_t cost_type)
{
	assert(  cost_type<MAX_CONVOI_COST);

	if(cost_type != CONVOI_AVERAGE_SPEED && cost_type != CONVOI_COMFORT)
	{
		// Summative types
		financial_history[0][cost_type] += amount;
	}
	else
	{
		// Average types
		if(rolling_average_count[cost_type] == 65535)
		{
			rolling_average_count[cost_type] /= 2;
			rolling_average[cost_type] /= 2;
		}
		rolling_average[cost_type] += (uint32)amount;
		rolling_average_count[cost_type] ++;
		const sint64 tmp = (sint64)rolling_average[cost_type] / (sint64)rolling_average_count[cost_type];
		financial_history[0][cost_type] = tmp;
	}
	if (line.is_bound())
	{
		line->book(amount, simline_t::convoi_to_line_catgory(cost_type) );
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


sint32 convoi_t::get_running_cost() const
{
	sint32 running_cost = 0;
	for(  unsigned i = 0;  i < get_vehicle_count();  i++  ) {
		running_cost += vehicle[i]->get_running_cost(welt);
	}
	return running_cost;
}

sint32 convoi_t::get_per_kilometre_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehicle_count(); i++) { 
		sint32 vehicle_running_cost = vehicle[i]->get_desc()->get_running_cost(welt);
		running_cost += vehicle_running_cost;
	}
	return running_cost;
}

uint32 convoi_t::get_fixed_cost() const
{
	uint32 maint = 0;
	for (unsigned i = 0; i<get_vehicle_count(); i++) {
		maint += vehicle[i]->get_fixed_cost(welt);
	}
	return maint;
}

sint64 convoi_t::get_purchase_cost() const
{
	sint64 purchase_cost = 0;
	for(  unsigned i = 0;  i < get_vehicle_count();  i++  ) {
		purchase_cost += vehicle[i]->get_desc()->get_value();
	}
	return purchase_cost;
}

void convoi_t::clear_average_speed()
{
	financial_history[0][CONVOI_AVERAGE_SPEED] = 0;
	financial_history[1][CONVOI_AVERAGE_SPEED] = 0;

	arrival_to_first_stop.clear();
}

/**
* set line
* since convoys must operate on a copy of the route's schedule, we apply a fresh copy
* @author hsiegeln
*/
void convoi_t::set_line(linehandle_t org_line)
{
	// to remove a convoi from a line, call unset_line(); passing a NULL is not allowed!
	bool need_to_reset_average_speed = false;
	if(!org_line.is_bound())
	{
		return;
	}
	if(line.is_bound())
	{
		unset_line();
		need_to_reset_average_speed = true;
	}
	else
	{
		need_to_reset_average_speed = !schedule || !schedule->matches(welt, org_line->get_schedule());

		// Knightly : originally a lineless convoy -> unregister itself from stops as it now belongs to a line
		unregister_stops();	
	}
	
	if (need_to_reset_average_speed)
	{
		clear_average_speed();
	}

	line_update_pending = org_line;
	check_pending_updates();
}


/**
* unset line
* removes convoy from route without destroying its schedule
* => no need to recalculate connetions!
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
		schedule_t* new_fpl = line_update_pending->get_schedule();
		uint8 current_stop = schedule->get_current_stop(); // save current position of schedule
		bool is_same = false;
		bool is_depot = false;
		koord3d current = koord3d::invalid, depot = koord3d::invalid;

		if (schedule->empty() || new_fpl->empty()) {
			// There was no entry or is no entry: go to the first stop
			current_stop = 0;
		}
		else {
			// something to check for ...
			current = schedule->get_current_eintrag().pos;

			if(  current_stop<new_fpl->get_count() &&  current==new_fpl->entries[current_stop].pos  ) {
				// next pos is the same => keep the convoi state
				is_same = true;
			}
			else {
				// check depot first (must also keept this state)
				is_depot = (welt->lookup(current)  &&  welt->lookup(current)->get_depot() != NULL);

				if(is_depot) {
					// depot => current_stop+1 (depot will be restored later before this)
					depot = current;
					schedule->remove();
					if(schedule->empty())
					{
						goto end_check;
					}
					current = schedule->get_current_eintrag().pos;
				}

				/* there could be only one entry that matches best:
				 * we try first same sequence as in old schedule;
				 * if not found, we try for same nextnext station
				 * (To detect also places, where only the platform
				 *  changed, we also compare the halthandle)
				 */
				if(current_stop > schedule->entries.get_count() - 1)
				{
					current_stop = schedule->entries.get_count() - 1;
				}
				uint8 index = current_stop;
				bool reverse = reverse_schedule;
				koord3d next[4];
				for( uint8 i=0; i<4; i++ ) {
					next[i] = schedule->entries[index].pos;
					schedule->increment_index(&index, &reverse);
				}
				const int weights[4] = {3, 4, 2, 1};
				int how_good_matching = 0;
				const uint8 new_count = new_fpl->get_count();

				for(  uint8 i=0;  i<new_count;  i++  )
				{
					index = i;
					reverse = reverse_schedule;
					int quality = 0;
					for( uint8 j=0; j<4; j++ )
					{
						quality += matches_halt(next[j],new_fpl->entries[index].pos) * weights[j];
						new_fpl->increment_index(&index, &reverse);
					}
					if(  quality>how_good_matching  )
					{
						// better match than previous: but depending of distance, the next number will be different
						index = i;
						reverse = reverse_schedule;
						for( uint8 j=0; j<4; j++ )
						{
							if( matches_halt(next[j], new_fpl->entries[index].pos) )
							if( new_fpl->entries[index].pos==next[j] )
							{
								current_stop = index;
								break;
							}
							new_fpl->increment_index(&index, &reverse);
						}
						how_good_matching = quality;
					}
				}

				if(how_good_matching==0) {
					// nothing matches => take the one from the line
					current_stop = new_fpl->get_current_stop();
				}
				// if we go to same, then we do not need route recalculation ...
				is_same = current_stop < new_fpl->get_count() && matches_halt(current,new_fpl->entries[current_stop].pos);
			}
		}
end_check:

		// we may need to update the line and connection tables
		if(  !line.is_bound()  ) {
			line_update_pending->add_convoy(self);
		}
		line = line_update_pending;
		line_update_pending = linehandle_t();

		// destroy old schedule and all related windows
		if(!schedule->is_editing_finished()) {
			schedule->copy_from( new_fpl );
			schedule->set_current_stop(current_stop); // set new schedule current position to best match
			schedule->start_editing();
		}
		else {
			schedule->copy_from( new_fpl );
			schedule->set_current_stop(current_stop); // set new schedule current position to one before best match
		}

		if(is_depot) {
			// next was depot. restore it
			schedule->insert(welt->lookup(depot), 0, 0, 0, owner == welt->get_active_player());
			// Insert will move the pointer past the inserted item; move back to it
			schedule->advance_reverse();
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
	if(schedule)
	{
		FOR(minivec_tpl<schedule_entry_t>, const &i, schedule->entries)
		{
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, get_owner());
			if(halt.is_bound())
			{
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

bool convoi_t::check_destination_reverse(route_t* current_route, route_t* target_rt)
{
	route_t next_route;
	if(!current_route)
	{
		current_route = &route;
	}
	bool success = target_rt; 
	if(!target_rt)
	{
		uint8 index = schedule->get_current_stop();
		koord3d start_pos = schedule->entries[index].pos;
		bool rev = get_reverse_schedule();
		schedule->increment_index(&index, &rev);
		const koord3d next_ziel = schedule->entries[index].pos;
		success = next_route.calc_route(welt, start_pos, next_ziel, front(), speed_to_kmh(get_min_top_speed()), get_highest_axle_load(), has_tall_vehicles(), get_tile_length(), welt->get_settings().get_max_route_steps(), get_weight_summary().weight / 1000);
		target_rt = &next_route;
	}

	if(success)
	{
		ribi_t::ribi old_dir = front()->calc_direction(current_route->at(current_route->get_count() - 2).get_2d(), current_route->back().get_2d());
		ribi_t::ribi new_dir = front()->calc_direction(target_rt->at(0).get_2d(), target_rt->at(1).get_2d());
		return old_dir & ribi_t::backward(new_dir);
	}
	else
	{
		return false;
	}
}

// set next stop before breaking will occur (or route search etc.)
// currently only used for tracks
void convoi_t::set_next_stop_index(uint16 n)
{
	// stop at station or signals, not at waypoints
   if(n == INVALID_INDEX && !route.empty())
   {
	   // Find out if this schedule entry is a stop or a waypoint, waypoint:
	   // do not brake at non-reversing waypoints
	   bool reverse_waypoint = false;
	   koord3d route_end = route.back();
	   bool update_line = false;

	   if(front()->get_typ() != obj_t::air_vehicle)
	   {
		   const int count = schedule->get_count();
		   for(int i = 0; i < count; i ++)
		   {
			   schedule_entry_t &entries = schedule->entries[i];
			   if(entries.pos == route_end)
			   {
				   if(entries.reverse == -1)
				   {
					   grund_t* gr = world()->lookup(entries.pos);
					   if (gr && gr->get_depot())
					   {
						   schedule->set_reverse(1, i); 
					   }
					   else
					   {
						   entries.reverse = check_destination_reverse() ? 1 : 0;
						   schedule->set_reverse(entries.reverse, i);

						   if (line.is_bound())
						   {
							   schedule_t* line_schedule = line->get_schedule();
							   if (line_schedule->get_count() > i)
							   {
								   schedule_entry_t &line_entry = line_schedule->entries[i];
								   line_entry.reverse = entries.reverse;
								   update_line = true;
							   }
						   }
					   }
				   }
					reverse_waypoint = entries.reverse == 1;

					break;
			   }
		   }

		   if (update_line)
		   {
			   simlinemgmt_t::update_line(line);
		   }
	   }

	   grund_t const* const gr = welt->lookup(route_end);
	   rail_vehicle_t* rv = (rail_vehicle_t*)front();
	   if(gr && (gr->is_halt() || reverse_waypoint) && (front()->get_typ() != obj_t::rail_vehicle || rv->get_working_method() != one_train_staff))
	   {
		   n = route.get_count() - 1;
	   }
   }
	next_stop_index = n + 1;
}


/* including this route_index, the route was reserved the laste time
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
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), DARK_PURPLE (convoy has overcrowded vehicles), BLUE (at least one convoi vehicle is obsolete)
 */
COLOR_VAL convoi_t::get_status_color() const
{
	if(state==INITIAL)
	{
		// in depot/under assembly
		return SYSCOL_TEXT_HIGHLIGHT;
	}
	else if (state == WAITING_FOR_CLEARANCE_ONE_MONTH || state == CAN_START_ONE_MONTH || get_state() == NO_ROUTE || get_state() == OUT_OF_RANGE || get_state() == EMERGENCY_STOP) {
		// stuck or no route
		return COL_ORANGE;
	}
	else if(financial_history[0][CONVOI_PROFIT]+financial_history[1][CONVOI_PROFIT]<0)
	{
		// ok, not performing best
		return COL_RED;
	}
	else if((financial_history[0][CONVOI_OPERATIONS]|financial_history[1][CONVOI_OPERATIONS])==0)
	{
		// nothing moved
		return COL_YELLOW;
	}
	else if(get_overcrowded() > 0)
	{
		// Overcrowded
		return COL_DARK_PURPLE;
	}
	else if(has_obsolete)
	{
		return COL_DARK_BLUE;
	}
	// normal state
	return SYSCOL_TEXT;
}

uint8
convoi_t::get_catering_level(uint8 type) const
{
	uint8 max_catering_level = 0;
	uint8 current_catering_level;
	//for(sint16 i = vehicle.get_size() - 1; i >= 0; i --)
	for(uint8 i = 0; i < vehicle_count; i ++)
	{
		vehicle_t* v = get_vehicle(i);
		if(v == NULL)
		{
			continue;
		}
		current_catering_level = v->get_desc()->get_catering_level();
		if(current_catering_level > max_catering_level && v->get_desc()->get_freight_type()->get_catg_index() == type)
		{
			max_catering_level = current_catering_level;
		}
	}
	return max_catering_level;
}


 /**
 * True if this convoy has the same vehicles as the other
 * @author isidoro
 * @author neroden (fixed)
 */
bool convoi_t::has_same_vehicles(convoihandle_t other) const
{
	if (!other.is_bound())
	{
		return false;
	}
	if (get_vehicle_count()!=other->get_vehicle_count())
	{
		return false;
	}
	// We must compare both in the 'same direction' and in the 'reverse direction'.
	// We cannot use the 'reverse' flag to tell the difference.
	bool forward_compare_good = true;
	for (int i=0; i<get_vehicle_count(); i++)
	{
		if (get_vehicle(i)->get_desc()!=other->get_vehicle(i)->get_desc())
		{
			forward_compare_good = false;
			break;
		}
	}
	if (forward_compare_good) {
		return true;
	}
	bool reverse_compare_good = true;
	for (int i=0, j=get_vehicle_count()-1; i<get_vehicle_count(); i++, j--)
	{
		if (get_vehicle(j)->get_desc()!=other->get_vehicle(i)->get_desc())
		{
			reverse_compare_good = false;
			break;
		}
	}
	if (reverse_compare_good) {
		return true;
	}
	return false;
}

/*
 * Will find a depot for the vehicle "master".
 */
class depot_finder_t : public test_driver_t
{
private:
	vehicle_t *master;
	uint16 traction_type;
public:
	depot_finder_t( convoihandle_t cnv, uint16 tt )
	{
		master = cnv->get_vehicle(0);
		assert(master!=NULL);
		traction_type = tt;
	};
	virtual waytype_t get_waytype() const
	{
		return master->get_waytype();
	};
	virtual bool check_next_tile( const grund_t* gr) const
	{
		return master->check_next_tile(gr);
	};
	virtual bool  is_target( const grund_t* gr, const grund_t* )
	{
		return gr->get_depot() && gr->get_depot()->get_owner() == master->get_owner() && gr->get_depot()->get_tile()->get_desc()->get_enabled() & traction_type;
	};
	virtual ribi_t::ribi get_ribi( const grund_t* gr) const
	{
		return master->get_ribi(gr);
	};
	virtual int get_cost( const grund_t* gr, const sint32, koord from_pos)
	{
		return master->get_cost(gr, 0, from_pos);
	};
};

void convoi_t::set_depot_when_empty(bool new_dwe)
{
	if(loading_level > 0)
	{
		depot_when_empty = new_dwe;
	}
	else if(new_dwe)
	{
		go_to_depot(get_owner() == welt->get_active_player());
	}
}

/**
 * Convoy is sent to depot.  Return value, success or not.
 */
bool convoi_t::go_to_depot(bool show_success, bool use_home_depot)
{
	if(!schedule->is_editing_finished())
	{
		return false;
	}

	if (schedule)
	{
		old_schedule = schedule->copy();
	}

	// limit update to certain states that are considered to be safe for schedule updates
	int state = get_state();
	if(state==convoi_t::EDIT_SCHEDULE) {
DBG_MESSAGE("convoi_t::go_to_depot()","convoi state %i => cannot change schedule ... ", state );
		return false;
	}

	// Identify this convoi's traction types.  We want to match at least one.
	uint16 traction_types = 0;
	uint16 shifter;
	if(replace)
	{
		ITERATE_PTR(replace->get_replacing_vehicles(), i)
		{
			if(replace->get_replacing_vehicle(i)->get_power() == 0)
			{
				continue;
			}
			shifter = 1 << replace->get_replacing_vehicle(i)->get_engine_type();
			traction_types |= shifter;
		}
	}
	else
	{
		for(uint8 i = 0; i < vehicle_count; i ++)
		{
			if(vehicle[i]->get_desc()->get_power() == 0)
			{
				continue;
			}
			shifter = 1 << vehicle[i]->get_desc()->get_engine_type();
			traction_types |= shifter;
		}
	}

	air_vehicle_t* aircraft = NULL;

	if(get_vehicle(0)->get_typ() == obj_t::air_vehicle)
	{
		// Flying aircraft cannot find a depot in the normal way, so go to the home depot.
		aircraft = (air_vehicle_t*)get_vehicle(0);
		if(!aircraft->is_on_ground())
		{
			use_home_depot = true;
		}
	}
	
	const uint32 range = (uint32)get_min_range();
	bool home_depot_valid = false;
	if (use_home_depot) {
		// Check for a valid home depot.  It is quite easy to get savegames with
		// invalid home depot coordinates.
		const grund_t* test_gr = welt->lookup(get_home_depot());
		if (test_gr) {
			depot_t* test_depot = test_gr->get_depot();
			if (test_depot) {
				// It's a depot -- is it suitable?
				home_depot_valid = test_depot->is_suitable_for(get_vehicle(0), traction_types);
			}
		}
		if(range == 0 || (shortest_distance(get_pos().get_2d(), get_home_depot().get_2d()) * (uint32)welt->get_settings().get_meters_per_tile()) / 1000 > range)
		{
			home_depot_valid = false;
		}
		// The home depot seems OK, but what if we can't get there?
		// Don't consider it a valid home depot if we already failed to get there.
		if (home_depot_valid && (state == NO_ROUTE || state == OUT_OF_RANGE)) {
			if (schedule) {
				const schedule_entry_t & current_entry = schedule->get_current_eintrag();
				if ( current_entry.pos == get_home_depot() ) {
					// We were already trying to get there... and failed.
					home_depot_valid = false;
				}
			}
		}
	}


	koord3d depot_pos;
	route_t route;
	bool home_depot_found = false;
	bool other_depot_found = false;
	if (use_home_depot && home_depot_valid) {
		// The home depot is OK, use it.
		depot_pos = get_home_depot();
		home_depot_found = true;
	}
	else {
		// See if we're already sitting on top of a depot.
		// (The route finder won't find a depot if we're standing on top of it, believe it or not.)
		bool current_location_valid = false;
		const grund_t* test_gr = welt->lookup( get_vehicle(0)->get_pos() );
		if (test_gr) {
			depot_t* test_depot = test_gr->get_depot();
			if (test_depot) {
				// It's a depot -- is it suitable?
				current_location_valid = test_depot->is_suitable_for(get_vehicle(0), traction_types);
			}
		}
		if (current_location_valid) {
			depot_pos = get_vehicle(0)->get_pos();
			other_depot_found = true;
		}
		else {
			// OK, we're not standing on a depot.  Find a route to a depot.
			depot_finder_t finder(self, traction_types);
			route.find_route(welt, get_vehicle(0)->get_pos(), &finder, speed_to_kmh(get_min_top_speed()), ribi_t::all, get_highest_axle_load(), get_tile_length(), get_weight_summary().weight / 1000, 0x7FFFFFFF, has_tall_vehicles()); 
			if (!route.empty()) {
				depot_pos = route.at(route.get_count() - 1);
				if(range == 0 || (shortest_distance(get_pos().get_2d(), depot_pos.get_2d()) * (uint32)welt->get_settings().get_meters_per_tile()) / 1000 <= range)
				{
					other_depot_found = true;
				}
			}
		}
	}

	if(((aircraft && !aircraft->is_on_ground()) || use_home_depot) && home_depot_valid)
	{
		// Flying aircraft cannot find a route using the normal means: send to their home depot instead.
		aircraft->calc_route(get_pos(), home_depot, speed_to_kmh(get_min_top_speed()), has_tall_vehicles(), &route);
		if(!route.empty())
		{
			depot_pos = route.at(route.get_count() - 1);
			if(range == 0 || (shortest_distance(get_pos().get_2d(), depot_pos.get_2d()) * (uint32)welt->get_settings().get_meters_per_tile()) / 1000 <= range)
			{
				home_depot_found = true;
			}
		}
	}

	bool transport_success = false;
	if (home_depot_found || other_depot_found) {
		if ( get_vehicle(0)->get_pos() == depot_pos ) {
			// We're already there.  Just enter the depot (speed things up!)
			const grund_t* my_gr = welt->lookup(depot_pos);
			if (my_gr) {
				depot_t* my_depot = my_gr->get_depot();
				if (my_depot) {
					enter_depot(my_depot);
					transport_success = true;
				}
			}
		}
		else 
		{
			schedule_t* f = schedule->copy();
			bool schedule_insertion_succeeded = f->insert(welt->lookup(depot_pos));
			// Insert will move the pointer past the inserted item; move back to it
			f->advance_reverse();
			// We still have to call set_schedule
			bool schedule_setting_succeeded = set_schedule(f);
			transport_success = schedule_insertion_succeeded && schedule_setting_succeeded;
		}
	}

	// show result
	const char* txt;
	bool success = false;
	if (home_depot_found && transport_success)
	{
		txt = "The convoy has been sent\nto its home depot.\n";
		success = true;
	}
	else if (other_depot_found && transport_success)
	{
		txt = "Convoi has been sent\nto the nearest depot\nof appropriate type.\n";
		success = true;
	}
	else if (!home_depot_found && !other_depot_found)
	{
		txt = "No suitable depot found!\nYou need to send the\nconvoi to the depot\nmanually.";
		success = false;
	}
	else if (!transport_success)
	{
		txt = "Depot found but could not be inserted in schedule.  This is a bug!";
		dbg->warning("convoi_t::go_to_depot()", "Depot found but could not be inserted in schedule for convoy %s", get_name());
		success = false;
	}
	if ( (!success || show_success) && get_owner() == welt->get_active_player())
	{
		create_win(new news_img(txt), w_time_delete, magic_none);
	}
	return success;
}

bool convoi_t::has_no_cargo() const
{
	if (loading_level==0)
	{
		return true;
	}
	if (loading_level!=100)
	{
		return false;
	}
	/* a convoy with max capacity of zero, has always loading_level==100 */
	for(unsigned i=0; i<vehicle_count; i++)
	{
		if (vehicle[i]->get_cargo_max()>0)
		{
			return false;
		}
	}
	return true;
}

// returns tiles needed for this convoi
uint16 convoi_t::get_tile_length() const
{
	uint16 carunits=0;
	for(sint8 i=0;  i<vehicle_count-1;  i++) {
		carunits += vehicle[i]->get_desc()->get_length();
	}
	// the last vehicle counts differently in stations and for reserving track
	// (1) add 8 = 127/256 tile to account for the driving in stations in north/west direction
	//     see at the end of vehicle_t::hop()
	// (2) for length of convoi for loading in stations the length of the last vehicle matters
	//     see convoi_t::hat_gehalten
	carunits += max(CARUNITS_PER_TILE/2, vehicle[vehicle_count-1]->get_desc()->get_length());

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
	if(front()->get_waytype()!=road_wt) {
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
			// should never happen, since there is a vehcile in front of us ...
			return false;
		}
		weg_t *str = gr->get_weg(road_wt);
		if(  str==0  ) {
			// also this is not possible, since a car loads in front of is!?!
			return false;
		}

		uint16 idx = front()->get_route_index();
		const sint32 tiles = other_speed == 0 ? 2 : (steps_other - 1) / (CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) + get_tile_length() + 1;
		if(  tiles > 0  &&  idx+(uint32)tiles >= route.get_count()  ) {
			// needs more space than there
			return false;
		}

		for (sint32 i = 0; i < tiles; i++) {
			grund_t *gr = welt->lookup(route.at(idx + i));
			if (gr == NULL) {
				return false;
			}
			weg_t *str = gr->get_weg(road_wt);
			if (str == 0) {
				return false;
			}
			// not overtaking on railroad crossings or normal crossings ...
			if (str->is_crossing()) {
				return false;
			}
			if (ribi_t::is_threeway(str->get_ribi())) {
				return false;
			}
			// Check for other vehicles on the next tile
			const uint8 top = gr->get_top();
			for (uint8 j = 1; j < top; j++) {
				if (vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j))) {
					// check for other traffic on the road
					const overtaker_t *ov = v->get_overtaker();
					if (ov) {
						if (this != ov  &&  other_overtaker != ov) {
							return false;
						}
					}
					else if (v->get_waytype() == road_wt  &&  v->get_typ() != obj_t::pedestrian) {
						return false;
					}
				}
			}
		}
		set_tiles_overtaking(tiles);
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
	// tiles_to_overtake = convoi_length + current pos within tile + (pos_other_convoi wihtin tile + length of other convoi) - one tile
	int distance = akt_speed*(front()->get_steps()+get_length_in_steps()+steps_other-VEHICLE_STEPS_PER_TILE)/diff_speed;
	int time_overtaking = 0;

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	// First phase: no traffic except me and my overtaken car in the dangerous zone
	unsigned int route_index = front()->get_route_index()+1;
	koord pos_prev = front()->get_pos_prev().get_2d();
	koord3d pos = front()->get_pos();
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
					// because we need to stop here ...
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
		pos_prev = pos.get_2d();
		pos = pos_next;
	}

	// Second phase: only facing traffic is forbidden
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will substract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	// Conditions for the street are milder: e.g. if no street, no facing traffic
	if(akt_speed == 0)
	{
		return false;
	}
	time_overtaking = (time_overtaking << 16)/ akt_speed;
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
		if(  ribi_t::is_threeway(str->get_ribi()) ) {
			return false;
		}

		if(  ribi_t::is_straight(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / kmh_to_speed(str->get_max_speed());
		}
		else {
			time_overtaking -= (vehicle_base_t::get_diagonal_vehicle_steps_per_tile()<<16) / kmh_to_speed(str->get_max_speed());
		}

		// Check for other vehicles in facing direction
		ribi_t::ribi their_direction = ribi_t::backward( front()->calc_direction(pos_prev, pos_next.get_2d()) );
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j));
			if (v && v->get_direction() == their_direction && v->get_overtaker()) {
				return false;
			}
		}
		pos_prev = pos.get_2d();
		pos = pos_next;
	}

	set_tiles_overtaking( 1+n_tiles );
	other_overtaker->set_tiles_overtaking( -1-(n_tiles*(akt_speed-diff_speed))/akt_speed );
	return true;
}

/**
 * Format remaining loading time from go_on_ticks
 */
void convoi_t::snprintf_remaining_loading_time(char *p, size_t size) const
{
	const uint32 reverse_delay = calc_reverse_delay();
	uint32 loading_time = current_loading_time;
	const sint64 current_ticks = welt->get_ticks();
	const grund_t* gr = welt->lookup(this->get_pos());
	if(gr && welt->get_ticks() - arrival_time > reverse_delay && gr->is_halt())
	{
		// The reversing time must not be cumulative with the loading time, as
		// passengers can board trains etc. while they are changing direction.
		// Only do this where the reversing point is a stop, not a waypoint.
		if(reverse_delay <= loading_time)
		{
			loading_time -= reverse_delay;
		}
		else
		{
			loading_time = 0;
		}
	}

	sint32 remaining_ticks;

	if (go_on_ticks != WAIT_INFINITE && go_on_ticks >= current_ticks)
	{
		remaining_ticks = (sint32)(go_on_ticks - current_ticks);
	}

	else if (((arrival_time + current_loading_time) - reverse_delay) >= current_ticks)
	{
		remaining_ticks = (sint32)(((arrival_time + current_loading_time) - reverse_delay) - current_ticks);
	}
	else
	{
		remaining_ticks = 0;
	}

	uint32 ticks_left = 0;

	if(remaining_ticks >= 0)
	{
		ticks_left = remaining_ticks;
	}
	welt->sprintf_ticks(p, size, ticks_left);
}

/**
 * Format remaining loading time from go_on_ticks
 */
void convoi_t::snprintf_remaining_reversing_time(char *p, size_t size) const
{
	welt->sprintf_ticks(p, size, wait_lock);
}

void convoi_t::snprintf_remaining_emergency_stop_time(char *p, size_t size) const
{
	welt->sprintf_ticks(p, size, wait_lock);
}

uint32 convoi_t::calc_highest_axle_load()
{
	uint32 heaviest = 0;
	for(uint8 i = 0; i < vehicle_count; i ++)
	{
		const uint32 tmp = vehicle[i]->get_desc()->get_axle_load();
		if(tmp > heaviest)
		{
			heaviest = tmp;
		}
	}
	return heaviest;
}

uint32 convoi_t::calc_longest_min_loading_time()
{
	uint32 longest = 0;
	for(int i = 0; i < vehicle_count; i ++)
	{
		const uint32 tmp = vehicle[i]->get_desc()->get_min_loading_time();
		if(tmp > longest)
		{
			longest = tmp;
		}
	}
	return longest;
}

uint32 convoi_t::calc_longest_max_loading_time()
{
	uint32 longest = 0;
	for(int i = 0; i < vehicle_count; i ++)
	{
		const uint32 tmp = vehicle[i]->get_desc()->get_max_loading_time();
		if(tmp > longest)
		{
			longest = tmp;
		}
	}
	return longest;
}

void convoi_t::calc_min_range()
{
	uint16 min = 0;
	for(int i = 0; i < vehicle_count; i ++)
	{
		const uint16 range = vehicle[i]->get_desc()->get_range();
		if(range > 0 && min == 0)
		{
			min = range;
		}
		else if(range > 0 && range < min)
		{
			min = range;
		}
	}
	min_range = min;
}

void convoi_t::calc_direction_steps()
{
	const sint32 top_speed_kmh = speed_to_kmh(get_min_top_speed());
	const sint32 corner_force_divider = welt->get_settings().get_corner_force_divider(vehicle[0]->get_waytype()); 
	const sint32 max_limited_radius = ((top_speed_kmh * top_speed_kmh) * corner_force_divider) / 87;
	const sint16 max_tile_steps = (max_limited_radius / welt->get_settings().get_meters_per_tile()) * 2; // This must be multiplied by two because each diagonal step takes two tiles.
	for(int i = 0; i < vehicle_count; i ++)
	{
		vehicle[i]->set_direction_steps(max_tile_steps);
	}	
}

// Bernd Gabriel, 18.06.2009: extracted from new_month()
bool convoi_t::calc_obsolescence(uint16 timeline_year_month)
{
	// convoi has obsolete vehicles?
	for(int j = get_vehicle_count(); --j >= 0; ) {
		if (vehicle[j]->get_desc()->is_obsolete(timeline_year_month, welt)) {
			return true;
		}
	}
	return false;
}

void convoi_t::clear_replace()
{
	if(replace)
	{
		replace->decrement_convoys(self);
		replace = NULL;
	}
}

 void convoi_t::set_replace(replace_data_t *new_replace)
 {
	 if(new_replace != NULL && replace != new_replace)
	 {
		 new_replace->increment_convoys(self);
	 }
	 if(replace != NULL)
	 {
		 replace->decrement_convoys(self);
	 }
	 replace = new_replace;
 }

 void convoi_t::apply_livery_scheme()
 {
	 const livery_scheme_t* const scheme = welt->get_settings().get_livery_scheme(livery_scheme_index);
	 if(!scheme)
	 {
		 return;
	 }
	 const uint16 date = welt->get_timeline_year_month();
	 for (int i = 0; i < vehicle_count; i++)
	 {
		vehicle_t& v = *vehicle[i];
		const char* liv = scheme->get_latest_available_livery(date, v.get_desc());
		if(liv)
		{
			// Only change the livery if there is an available scheme livery.
			v.set_current_livery(liv);
		}
	 }
 }

 uint16 convoi_t::get_livery_scheme_index() const
 {
	 if(line.is_bound())
	 {
		 return line->get_livery_scheme_index();
	 }
	 else
	 {
		 return livery_scheme_index;
	 }
 }

 uint32 convoi_t::calc_reverse_delay() const
 {
	if (front()->get_waytype() == road_wt)
	{ 
		return welt->get_settings().get_road_reverse_time();
	}

	 uint32 reverse_delay;

	if(reversable)
	{
		// Multiple unit or similar: quick reverse
		reverse_delay = welt->get_settings().get_unit_reverse_time();
	}
	else if(front()->get_desc()->is_bidirectional())
	{
		// Loco hauled, no turntable.
		if(vehicle_count > 1 && vehicle[vehicle_count-2]->get_desc()->get_can_be_at_rear() == false
			&& vehicle[vehicle_count-2]->get_desc()->get_freight_type()->get_catg_index() > 1)
		{
			// Goods train with brake van - longer reverse time.
			reverse_delay = (welt->get_settings().get_hauled_reverse_time() * 14) / 10;
		}
		else
		{
			reverse_delay = welt->get_settings().get_hauled_reverse_time();
		}
	}
	else
	{
		// Locomotive needs turntable: slow reverse
		reverse_delay = welt->get_settings().get_turntable_reverse_time();
	}

	return reverse_delay;
 }

 void convoi_t::book_waiting_times()
 {
	 halthandle_t halt;
	 halt.set_id(last_stop_id);
	 if(!halt.is_bound())
	 {
		 return;
	 }
	 const sint64 current_time = welt->get_ticks();
	 uint16 waiting_minutes;
	 const uint16 airport_wait = front()->get_typ() == obj_t::air_vehicle ? welt->get_settings().get_min_wait_airport() : 0;
	 for(uint8 i = 0; i < vehicle_count; i++) 
	 {
		FOR(slist_tpl< ware_t>, const& iter, vehicle[i]->get_cargo())
		{
			if(iter.get_last_transfer().get_id() == halt.get_id())
			{
				waiting_minutes = max(get_waiting_minutes(current_time - iter.arrival_time), airport_wait);
				// Only times of one minute or larger are registered, to avoid registering zero wait-time when a passenger
				// alights a convoy and then immediately re-boards that same convoy.
				if(waiting_minutes > 0)
				{
#ifdef MULTI_THREAD
					pthread_mutex_lock(&step_convois_mutex);
#endif
					halt->add_waiting_time(waiting_minutes, iter.get_zwischenziel(), iter.get_desc()->get_catg_index());
#ifdef MULTI_THREAD
					pthread_mutex_unlock(&step_convois_mutex);
#endif
				}
			}
		}
	}
 }

 void convoi_t::book_departure_time(sint64 time)
 {
	halthandle_t halt;
	halt.set_id(last_stop_id);
	if(halt.is_bound())
	{
		// Only book a departure time if this convoy is at a station/stop.
		departure_data_t dep;
		dep.departure_time = time;
		uint8 schedule_entry = schedule->get_current_stop();
		bool rev_rev = !reverse_schedule;
		bool has_not_found_halt = true;
		while (has_not_found_halt)
		{
			schedule->increment_index(&schedule_entry, &rev_rev);
			const koord3d halt_position = schedule->entries.get_element(schedule_entry).pos;
			const halthandle_t halt = haltestelle_t::get_halt(halt_position, front()->get_owner());
			has_not_found_halt = !halt.is_bound();
		}
		departure_point_t departure_point(schedule_entry, !rev_rev);

		departures.set(departure_point, dep);
		if((schedule->entries[schedule_entry].minimum_loading > 0 || schedule->entries[schedule_entry].wait_for_time) && schedule->get_spacing() > 0 && line.is_bound())
		{
			line->book(1, LINE_DEPARTURES);
		}

		// Set the departure time from the current location (which is no longer an estimate).
		halt->set_estimated_departure_time(self.get_id(), time);

		// Estimate arrival and departure times at and from the subsequent stops on the schedule.
		bool rev = reverse_schedule;
		const uint8 count = schedule->is_mirrored() ? schedule->get_count() * 2 : schedule->get_count();
		sint64 journey_time_ticks = 0;
		sint64 eta = time;
		sint64 etd = eta;
		vector_tpl<uint16> halts_already_processed;
		const uint32 reverse_delay = calc_reverse_delay();
		schedule->increment_index(&schedule_entry, &rev);
		for(uint8 i = 0; i < count; i++)
		{		
			uint32 journey_time_tenths_minutes = (uint32)journey_times_between_schedule_points.get(departure_point).get_average();
			if(journey_time_tenths_minutes == 0)
			{
				// Journey time uninitialised - use average or estimated average speed instead.
				uint8 next_schedule_entry = schedule_entry;
				schedule->increment_index(&next_schedule_entry, &rev);
				const koord3d stop1_pos = schedule->entries[schedule_entry].pos;
				const koord3d stop2_pos = schedule->entries[next_schedule_entry].pos;
				const uint16 distance = shortest_distance(stop1_pos.get_2d(), stop2_pos.get_2d());
				const uint32 current_average_speed = (uint32)(get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) > 0 ? 
													  get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) : 
													   (speed_to_kmh(get_min_top_speed()) >> 1));
				journey_time_tenths_minutes = welt->travel_time_tenths_from_distance(distance, current_average_speed);
			}

			journey_time_ticks = welt->get_seconds_to_ticks(journey_time_tenths_minutes * 6);
			eta = etd;
			eta += journey_time_ticks;
			etd += journey_time_ticks;
			halt = haltestelle_t::get_halt(schedule->entries[schedule_entry].pos, owner);
			
			if(halt.is_bound() && !halts_already_processed.is_contained(halt.get_id()))
			{
				halt->set_estimated_arrival_time(self.get_id(), eta);
				const sint64 max_waiting_time = schedule->get_current_eintrag().waiting_time_shift ? welt->ticks_per_world_month >> (16ll - (sint64)schedule->get_current_eintrag().waiting_time_shift) : WAIT_INFINITE;
				if((schedule->entries[schedule_entry].minimum_loading > 0 || schedule->entries[schedule_entry].wait_for_time) && schedule->get_spacing() > 0)
				{				
					// Add spacing time. 
					const sint64 spacing = welt->ticks_per_world_month / (sint64)schedule->get_spacing();
					const sint64 spacing_shift = (sint64)schedule->get_current_eintrag().spacing_shift * welt->ticks_per_world_month / (sint64)welt->get_settings().get_spacing_shift_divisor();
					const sint64 wait_from_ticks = ((eta - spacing_shift) / spacing) * spacing + spacing_shift; // remember, it is integer division
					const sint64 spaced_departure = min(max_waiting_time, (wait_from_ticks + spacing)) - reverse_delay;
					etd += (spaced_departure - eta);
				}
				else if(schedule->entries[schedule_entry].minimum_loading > 0 && schedule->get_spacing() == 0)
				{
					if(schedule->get_current_eintrag().waiting_time_shift)
					{
						etd += max_waiting_time;
					}
					else
					{
						// Add loose estimate for loading to the %
						etd += seconds_to_ticks(900, welt->get_settings().get_meters_per_tile()); // 15 minutes.
					}
				}
				etd += current_loading_time;
			}
			if(schedule->entries[schedule_entry].reverse == 1)
			{
				// Add reversing time if this must reverse.
				etd += reverse_delay;
			}			

			if(halt.is_bound() && !halts_already_processed.is_contained(halt.get_id()))
			{
				halt->set_estimated_departure_time(self.get_id(), etd);	
				halts_already_processed.append(halt.get_id());
			}
			schedule->increment_index(&schedule_entry, &rev);
			departure_point.entry = schedule_entry;
			departure_point.reversed = rev;
		}
	}
	else
	{
		dbg->warning("void convoi_t::book_departure_time(sint64 time)", "Cannot find last halt to set departure time");
	}
 }


 uint16 convoi_t::get_waiting_minutes(uint32 waiting_ticks)
 {
	// Waiting time is reduced (2* ...) instead of (3 * ...) because, in real life, people
	// can organise their journies according to timetables, so waiting is more efficient.

	 // NOTE: distance_per_tile is now a percentage figure rather than a floating point - divide by an extra factor of 100.
	//return (2 *welt->get_settings().get_distance_per_tile() * waiting_ticks) / 40960;

	// Note: waiting times now in *tenths* of minutes (hence difference in arithmetic)
	//uint16 test_minutes_1 = ((float)1 / (1 / (waiting_ticks / 4096.0) * 20) *welt->get_settings().get_distance_per_tile() * 600.0F);
	//uint16 test_minutes_2 = (2 *welt->get_settings().get_distance_per_tile() * waiting_ticks) / 409.6;

	//return (welt->get_settings().get_meters_per_tile() * waiting_ticks) / (409600L/2);

	// Feb 2012: Waiting times no longer reduced by 1/3, since connections can now be optimised by players in ways not previously possible,
	// and since many players run very high frequency networks so waiting times rarely need reducing. (Carl Baker)
	return (welt->get_settings().get_meters_per_tile() * waiting_ticks) / (409600L/3);

	//const uint32 value = (2 *welt->get_settings().get_distance_per_tile() * waiting_ticks) / 409.6F;
	//return value <= 65535 ? value : 65535;

	//Old method (both are functionally equivalent, except for reduction in time. Would be fully equivalent if above was 3 * ...):
	//return ((float)1 / (1 / (waiting_ticks / 4096.0) * 20) *welt->get_settings().get_distance_per_tile() * 60.0F);
}

 uint32 convoi_t::calc_current_loading_time(uint16 load_charge)
 {
	if(longest_max_loading_time == longest_min_loading_time || load_charge == 0)
	{
		return longest_min_loading_time;
	}

	uint16 total_capacity = 0;
	for(uint8 i = 0; i < vehicle_count; i ++)
	{
		total_capacity += vehicle[i]->get_desc()->get_capacity();
		total_capacity += vehicle[i]->get_desc()->get_overcrowded_capacity();
	}
	// Multiply this by 2, as goods/passengers can both board and alight, so
	// the maximum load charge is twice the capacity: all alighting, then all
	// boarding.
	total_capacity *= 2;
	const sint32 percentage = (load_charge * 100) / total_capacity;
	const sint32 difference = abs((((sint32)longest_max_loading_time - (sint32)longest_min_loading_time)) * percentage) / 100;
	return difference + longest_min_loading_time;
 }

 obj_t::typ convoi_t::get_depot_type() const
 {
	 const waytype_t wt = front()->get_waytype();
	 switch(wt)
	 {
	 case road_wt:
		 return obj_t::strassendepot;
	 case track_wt:
		 return obj_t::bahndepot;
	 case water_wt:
		 return obj_t::schiffdepot;
	 case monorail_wt:
		 return obj_t::monoraildepot;
	 case maglev_wt:
		 return obj_t::maglevdepot;
	 case tram_wt:
		 return obj_t::tramdepot;
	 case narrowgauge_wt:
		 return obj_t::narrowgaugedepot;
	 case air_wt:
		 return obj_t::airdepot;
	 default:
		 dbg->error("obj_t::typ convoi_t::get_depot_type() const", "Invalid waytype: cannot find correct depot type");
		 return obj_t::strassendepot;
	 };
 }

void convoi_t::emergency_go_to_depot()
{
	// First try going to a depot the normal way.
	if(!go_to_depot(false))
	{
		// Teleport to depot if cannot get there by normal means.
		depot_t* dep = welt->lookup(home_depot) ? welt->lookup(home_depot)->get_depot() : NULL;
		if(!dep)
		{
			dep = depot_t::find_depot(this->get_pos(), get_depot_type(), get_owner(), true);
		}
		if(dep)
		{
			// Only do this if a depot can be found, or else a crash will result.

			// Give a different message than the usual depot arrival message.
			cbuffer_t buf;
#ifdef MULTI_THREAD
			pthread_mutex_lock(&step_convois_mutex);
#endif
			buf.printf( translator::translate("No route to depot for convoy %s: teleported to depot!"), get_name() );
			const vehicle_t* v = front();

			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_owner()->get_player_nr(), IMG_EMPTY);

			enter_depot(dep);
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&step_convois_mutex);
#endif
			// Do NOT do the convoi_arrived here: it's done in enter_depot!
			state = INITIAL;
			schedule->set_current_stop(0);
		}
		else
		{
			// We can't send it to a depot, because there are no appropriate depots.  Destroy it!

			cbuffer_t buf;
#ifdef MULTI_THREAD
			pthread_mutex_lock(&step_convois_mutex);
#endif
			buf.printf( translator::translate("No route and no depot for convoy %s: convoy has been sold!"), get_name() );
			const vehicle_t* v = front();
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_owner()->get_player_nr(), IMG_EMPTY);

			// The player can engineer this deliberately by blowing up depots and tracks, so it isn't always a game error.
			// But it usually is an error, so report it as one.
			dbg->error("void convoi_t::emergency_go_to_depot()", "Could not find a depot to which to send convoy %i", self.get_id() );

			destroy();
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&step_convois_mutex);
#endif
		}
	}
}

journey_times_map& convoi_t::get_average_journey_times()
{
	if(line.is_bound())
	{
		return line->get_average_journey_times();
	}
	else
	{
		return average_journey_times;
	}
}


void convoi_t::clear_departures()
{
	departures.clear();
	departures_already_booked.clear();
	journey_times_between_schedule_points.clear();
	clear_estimated_times();
}

sint64 convoi_t::get_stat_converted(int month, convoi_cost_t cost_type) const
{
	sint64 value = financial_history[month][cost_type];
	switch(cost_type) {
		case CONVOI_REVENUE:
		case CONVOI_OPERATIONS:
		case CONVOI_PROFIT:
		case CONVOI_REFUNDS:
			value = convert_money(value);
			break;
		default: ;
	}
	return value;
}

// BG, 31.12.2012: virtual methods of lazy_convoy_t:
// Bernd Gabriel, Dec, 25 2009
sint16 convoi_t::get_current_friction()
{
	return get_vehicle_count() > 0 ? get_friction_of_waytype(front()->get_waytype()) : 0;
}

void convoi_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{
	vehicle.clear();
	uint32 count = get_vehicle_count();
	if (count > 0)
	{
		for (const_iterator i = begin(); i != end(); ++i )
		{
			vehicle.add_vehicle(*(*i)->get_desc());
		}
		vehicle.update_summary(get_vehicle(count-1)->get_desc()->get_length());
	}
}


void convoi_t::update_adverse_summary(adverse_summary_t &adverse)
{
	adverse.clear();
	uint16 count = get_vehicle_count();
	if (count > 0)
	{
		for (const_iterator i = begin(); i != end(); ++i )
		{
			adverse.add_vehicle(**i);
		}
		adverse.fr /= count;
	}
}


void convoi_t::update_freight_summary(freight_summary_t &freight)
{
	freight.clear();
	for (const_iterator i = begin(); i != end(); ++i )
	{
		freight.add_vehicle(*(*i)->get_desc());
	}
}


void convoi_t::update_weight_summary(weight_summary_t &weight)
{
	weight.weight = 0;
	sint64 sin = 0;
	for (const_iterator i = begin(); i != end(); ++i )
	{
		const vehicle_t &v = **i;
		sint32 kgs = v.get_total_weight();
		weight.weight += kgs;
		sin += kgs * v.get_frictionfactor();
	}
	weight.weight_cos = weight.weight;
	weight.weight_sin = (sin + 500) / 1000;
}


float32e8_t convoi_t::get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */)
{
	float32e8_t force = 0, br = get_adverse_summary().br;
	for (const_iterator i = begin(); i != end(); ++i )
	{
		vehicle_t &v = **i;
		const uint16 bf = v.get_desc()->get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf * (uint32) 1000;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground.
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += br * v.get_total_weight();
		}
	}
	return force;
}


float32e8_t convoi_t::get_force_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 force = 0;
	sint32 v = speed;

	for (const_iterator i = begin(); i != end(); ++i )
	{
		force += (*i)->get_desc()->get_effective_force_index(v);
	}

	//for (array_tpl<vehicle_t*>::iterator i = vehicle.begin(), n = i + get_vehicle_count(); i != n; ++i)
	//	force += (*i)->get_desc()->get_effective_force_index(v);

	return power_index_to_power(force, welt->get_settings().get_global_force_factor_percent());
}


float32e8_t convoi_t::get_power_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 power = 0;
	sint32 v = speed;
	for (const_iterator i = begin(); i != end(); ++i )
	{
		power += (*i)->get_desc()->get_effective_power_index(v);
	}
	return power_index_to_power(power, welt->get_settings().get_global_power_factor_percent());
}
// BG, 31.12.2012: end of virtual methods of lazy_convoy_t

void convoi_t::clear_estimated_times()
{
	if(!schedule)
	{
		return;
	}
	halthandle_t halt;
	uint8 entry = schedule->get_current_stop();
	bool rev = reverse_schedule;
	for(int i = 0; i < schedule->get_count(); i++)
	{		
		halt = haltestelle_t::get_halt(schedule->entries[entry].pos, owner);
		
		if(halt.is_bound())
		{
			halt->clear_estimated_timings(self.get_id());
		}

		schedule->increment_index(&entry, &rev);
	}
}
