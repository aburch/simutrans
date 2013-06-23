/**
 * convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#include <stdlib.h>

#include "simdebug.h"
#include "simunits.h"
#include "simworld.h"
#include "simware.h"
#include "player/finance.h" // convert_money
#include "player/simplay.h"
#include "simconvoi.h"
#include "simhalt.h"
#include "simdepot.h"
#include "simwin.h"
#include "simmenu.h"
#include "simcolor.h"
#include "simmesg.h"
#include "simintr.h"
#include "simlinemgmt.h"
#include "simline.h"
#include "simtools.h"
#include "freight_list_sorter.h"
#include "simtools.h"

#include "gui/karte.h"
#include "gui/convoi_info_t.h"
#include "gui/fahrplan_gui.h"
#include "gui/depot_frame.h"
#include "gui/messagebox.h"
#include "gui/convoi_detail_t.h"
#include "boden/grund.h"
#include "boden/wege/schiene.h"	// for railblocks

#include "bauer/vehikelbauer.h"

#include "besch/vehikel_besch.h"
#include "besch/roadsign_besch.h"
#include "besch/haus_besch.h"

#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/replace_data.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"
#include "dataobj/livery_scheme.h"

#include "dings/crossing.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"

#include "vehicle/simvehikel.h"
#include "vehicle/overtaker.h"

#include "utils/simstring.h"
#include "utils/cbuffer_t.h"

#include "convoy.h"

#include "besch/ware_besch.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 500
#define WAIT_INFINITE 9223372036854775807


karte_t *convoi_t::welt = NULL;

/*
 * Debugging helper - translate state value to human readable name
 * @author Hj- Malthaner
 */
static const char * state_names[convoi_t::MAX_STATES] =
{
	"INITIAL",
	"FAHRPLANEINGABE", //"Schedule input"
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
	"ENTERING_DEPOT"
};


/**
 * Calculates speed of slowest vehicle in the given array
 * @author Hj. Matthaner
 */
//static int calc_min_top_speed(const array_tpl<vehikel_t*>& fahr, uint8 anz_vehikel)
//{
//	int min_top_speed = SPEED_UNLIMITED;
//	for(uint8 i=0; i<anz_vehikel; i++) {
//		min_top_speed = min(min_top_speed, kmh_to_speed( fahr[i]->get_besch()->get_geschw() ) );
//	}
//	return min_top_speed;
//}


// Reset some values.  Used in init and replacing.
void convoi_t::reset()
{
	is_electric = false;
	//sum_gesamtgewicht = sum_gewicht = 0; 
	previous_delta_v = 0;

	withdraw = false;
	has_obsolete = false;
	no_load = false;
	depot_when_empty = false;
	reverse_schedule = false;

	jahresgewinn = 0;

	max_record_speed = 0;
	set_akt_speed(0);          // momentane Geschwindigkeit / current speed
	sp_soll = 0;
	//brake_speed_soll = 2147483647; // ==SPEED_UNLIMITED

	highest_axle_load = 0;
	longest_min_loading_time = 0;
	longest_max_loading_time = 0;
	current_loading_time = 0;
	departures->clear();
	for(uint8 i = 0; i < MAX_CONVOI_COST; i ++)
	{	
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}
}

void convoi_t::init(karte_t *wl, spieler_t *sp)
{
	welt = wl;
	besitzer_p = sp;

	average_journey_times = new koordhashtable_tpl<id_pair, average_tpl<uint16> >;
	departures = new inthashtable_tpl<uint16, departure_data_t>;

	reset();

	fpl = NULL;
	replace = NULL;
	line = linehandle_t();

	anz_vehikel = 0;
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

	alte_richtung = ribi_t::keine;
	next_wolke = 0;

	state = INITIAL;

	*name_and_id = 0;
	name_offset = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;
	free_seats = 0;

	max_record_speed = 0;
	//brake_speed_soll = SPEED_UNLIMITED;
	akt_speed_soll = 0;            // Sollgeschwindigkeit
	set_akt_speed(0);                 // momentane Geschwindigkeit
	sp_soll = 0;

	next_stop_index = INVALID_INDEX;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;
	last_stop_pos = koord3d::invalid;
	last_stop_id = 0;

	reversable = false;
	reversed = false;
	//recalc_data = true;

	livery_scheme_index = 0;
}


convoi_t::convoi_t(karte_t* wl, loadsave_t* file) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t();
	init(wl, 0);
	replace = NULL;

	no_route_retry_count = 0;
	rdwr(file);
	current_stop = fpl == NULL ? 255 : fpl->get_aktuell() - 1;

	// Added by : Knightly
	old_fpl = NULL;
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

convoi_t::convoi_t(spieler_t* sp) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	sp->book_convoi_number(1);
	init(sp->get_welt(), sp);
	set_name( "Unnamed" );
	welt->add_convoi( self );
	init_financial_history();
	current_stop = 255;

	// Added by : Knightly
	old_fpl = NULL;
	free_seats = 0;

	livery_scheme_index = 0;

	no_route_retry_count = 0;
}


convoi_t::~convoi_t()
{
	besitzer_p->book_convoi_number( -1);

	assert(self.is_bound());
	assert(anz_vehikel==0);

	close_windows();

DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_follow_convoi()==self) {
		welt->set_follow_convoi( convoihandle_t() );
	}

	welt->sync_remove( this );
	welt->rem_convoi( self );

	// Knightly : if lineless convoy -> unregister from stops
	if(  !line.is_bound()  ) {
		unregister_stops();
	}

	// force asynchronous recalculation
	if(fpl) {
		if(!fpl->ist_abgeschlossen()) { //"is completed" (Google)
			destroy_win((ptrdiff_t)fpl);
		}
	
		if (!fpl->empty() && !line.is_bound()) 
		{
			// New method - recalculate as necessary
			
			// Added by : Knightly
			haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p);
		}
		delete fpl;
	}

	clear_replace();

	delete average_journey_times;
	delete departures;

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

/**
 * unreserves the whole remaining route
 */
void convoi_t::unreserve_route()
{
	// need a route, vehicles, and vehicles must belong to this convoi
	// (otherwise crash during loading when fahr[0]->convoi is not initialized yet
	if(  !route.empty()  &&  anz_vehikel>0  &&  fahr[0]->get_convoi() == this  ) {
		waggon_t* lok = dynamic_cast<waggon_t*>(fahr[0]);
		if (lok) {
			// free all reserved blocks
			uint16 dummy;
			lok->block_reserver(get_route(), back()->get_route_index(), dummy, dummy,  100000, false, true);
		}
	}
}


/**
 * unreserves the whole remaining route
 */
void convoi_t::reserve_route()
{
	if(  !route.empty()  &&  anz_vehikel>0  &&  (is_waiting()  ||  state==DRIVING  ||  state==LEAVING_DEPOT)  ) {
		for(  int idx = back()->get_route_index();  idx < next_reservation_index  &&  idx < route.get_count();  idx++  ) {
			if(  grund_t *gr = welt->lookup( route.position_bei(idx) )  ) {
				if(  schiene_t *sch = (schiene_t *)gr->get_weg( front()->get_waytype() )  ) {
					sch->reserve( self, ribi_typ( route.position_bei(max(1u,idx)-1u), route.position_bei(min(route.get_count()-1u,idx+1u)) ) );
				}
			}
		}
	}
}


uint32 convoi_t::move_to(karte_t const& welt, koord3d const& k, uint16 const start_index)
{
	uint32 train_length = 0;
	for (unsigned i = 0; i != anz_vehikel; ++i) {
		vehikel_t& v = *fahr[i];

		steps_driven = -1;

		if (grund_t const* const gr = welt.lookup(v.get_pos())) {
			v.mark_image_dirty(v.get_bild(), v.get_hoff());
			v.verlasse_feld();
			// maybe unreserve this
			if (schiene_t* const rails = ding_cast<schiene_t>(gr->get_weg(v.get_waytype()))) {
				rails->unreserve(&v);
			}
		}

		/* Set pos_prev to the starting point this way.  Otherwise it may be
		 * elsewhere, especially on curves and with already broken convois. */
		v.set_pos(k);
		v.neue_fahrt(start_index, true);
		if (welt.lookup(v.get_pos())) {
			v.set_pos(k);
			v.betrete_feld();
		}

		if (i != anz_vehikel - 1U) {
			train_length += v.get_besch()->get_length();
		}
	}
	return train_length;
}


void convoi_t::laden_abschliessen()
{
	if(fpl==NULL) {
		if(  state!=INITIAL  ) {
			emergency_go_to_depot();
			return;
		}
		// anyway reassign convoi pointer ...
		for( uint8 i=0;  i<anz_vehikel;  i++ ) {
			fahr[i]->set_convoi(this);
			// This call used to update the fixed monthly maintenance costs; this was deleted due to
			// the fact that these change each month due to depreciation, so this doesn't work.
			// It would be good to restore it.
			// --neroden
			// fahr[i]->laden_abschliessen();
		}
		return;
	}

	bool realing_position = false;
	if(  anz_vehikel>0  ) {
DBG_MESSAGE("convoi_t::laden_abschliessen()","state=%s, next_stop_index=%d", state_names[state], next_stop_index );
	
	const uint32 max_route_index = get_route() ? get_route()->get_count() - 1 : 0;

	// only realign convois not leaving depot to avoid jumps through signals
		if(  steps_driven!=-1  ) {
			for( uint8 i=0;  i<anz_vehikel;  i++ ) {
				vehikel_t* v = fahr[i];
				if(v->get_route_index() > max_route_index && max_route_index > 0 && i > 0)
				{
					dbg->error("convoi_t::laden_abschliessen()", "Route index is %i, whereas maximum route index is %i for convoy %i", v->get_route_index(), max_route_index, self.get_id());
					v->set_route_index(fahr[0]->get_route_index());
				}
				v->set_erstes( i==0 );
				v->set_letztes( i+1==anz_vehikel );
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
				vehikel_t* v = fahr[i];
				/*if(v->get_route_index() > max_route_index && max_route_index > 0 && i > 0)
				{
					dbg->error("convoi_t::laden_abschliessen()", "Route index is %i, whereas maximum route index is %i for convoy %i", v->get_route_index(), max_route_index, self.get_id());
					v->set_route_index(fahr[0]->get_route_index());
				}*/
				v->set_erstes( i==0 );
				v->set_letztes( i+1==anz_vehikel );
				// this sets the convoi and will renew the block reservation, if needed!
				v->set_convoi(this);

				// wrong alignment here => must relocate
				if(v->need_realignment()) {
					// diagonal => convoi must restart
					realing_position |= ribi_t::ist_kurve(v->get_fahrtrichtung())  &&  (state==DRIVING  ||  is_waiting());
				}
				// if version is 99.17 or lower, some convois are broken, i.e. had too large gaps between vehicles
				if(  !realing_position  &&  state!=INITIAL  &&  state!=LEAVING_DEPOT  ) {
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
							step_pos += ribi_t::ist_kurve(v->get_fahrtrichtung()) ? diagonal_vehicle_steps_per_tile : VEHICLE_STEPS_PER_TILE;
						}
						dbg->message("convoi_t::laden_abschliessen()", "v: pos(%s) steps(%d) len=%d ribi=%d prev (%s) step(%d)", v->get_pos().get_str(), v->get_steps(), v->get_besch()->get_length()*16, v->get_fahrtrichtung(),  drive_pos.get_2d().get_str(), step_pos);
						if(  abs( v->get_steps() - step_pos )>15  ) {
							// not where it should be => realing
							realing_position = true;
							dbg->warning( "convoi_t::laden_abschliessen()", "convoi (%s) is broken => realign", get_name() );
						}
					}
					step_pos -= v->get_besch()->get_length_in_steps();
					drive_pos = v->get_pos();
				}
			}
		}
DBG_MESSAGE("convoi_t::laden_abschliessen()","next_stop_index=%d", next_stop_index );

		linehandle_t new_line = line;
		if(  !new_line.is_bound()  ) {
			// if there is a line with id=0 in the savegame try to assign cnv to this line
			new_line = get_besitzer()->simlinemgmt.get_line_with_id_zero();
		}
		if(  new_line.is_bound()  ) {
			if (  !fpl->matches( welt, new_line->get_schedule() )  ) {
				// 101 version produced broken line ids => we have to find our line the hard way ...
				vector_tpl<linehandle_t> lines;
				get_besitzer()->simlinemgmt.get_lines(fpl->get_type(), &lines);
				new_line = linehandle_t();
				FOR(vector_tpl<linehandle_t>, const l, lines) {
					if(  fpl->matches( welt, l->get_schedule() )  ) {
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
				DBG_DEBUG("convoi_t::laden_abschliessen()","%s registers for %d", name_and_id, line.get_id());
			}
			else {
				line = linehandle_t();
			}
		}
	}
	else {
		// no vehicles in this convoi?!?
		dbg->error( "convoi_t::laden_abschliessen()","No vehicles in Convoi %i: will be destroyed!", self.get_id() );
		destroy();
		return;
	}
	// put convoi agian right on track?
	if(realing_position  &&  anz_vehikel>1) {
		// display just a warning
		dbg->warning("convoi_t::laden_abschliessen()","cnv %i is currently too long.",self.get_id());

		if (route.empty()) {
			// realigning needs a route
			state = NO_ROUTE;
			besitzer_p->bescheid_vehikel_problem( self, koord3d::invalid );
			dbg->error( "convoi_t::laden_abschliessen()", "No valid route, but needs realignment at (%s)!", fahr[0]->get_pos().get_str() );
		}
		else {
			// since start may have been changed
			uint16 start_index = max(2,fahr[anz_vehikel-1]->get_route_index())-2;
			koord3d k0 = fahr[anz_vehikel-1]->get_pos();

			uint32 train_length = move_to(*welt, k0, start_index) + 1;
			const koord3d last_start = fahr[0]->get_pos();

			// now advance all convoi until it is completely on the track
			fahr[0]->set_erstes(false); // switches off signal checks ...
			for(unsigned i=0; i<anz_vehikel; i++) {
				vehikel_t* v = fahr[i];

				v->darf_rauchen(false); //"Allowed to smoke" (Google)
				fahr[i]->fahre_basis( (VEHICLE_STEPS_PER_CARUNIT*train_length)<<YARDS_PER_VEHICLE_STEP_SHIFT );
				train_length -= v->get_besch()->get_length();
				v->darf_rauchen(true);

				// eventually reserve this again
				grund_t *gr=welt->lookup(v->get_pos());
				// airplanes may have no ground ...
				if(schiene_t* const sch0 = ding_cast<schiene_t>(gr->get_weg(fahr[i]->get_waytype()))) {
					sch0->reserve(self,ribi_t::keine);
				}
			}
			fahr[0]->set_erstes(true);
			if(  state != INITIAL  &&  state != FAHRPLANEINGABE  &&  fahr[0]->get_pos() != last_start  ) {
				state = WAITING_FOR_CLEARANCE;
			}
		}
	}
	// when saving with open window, this can happen
	if(  state==FAHRPLANEINGABE  ) {
		if (umgebung_t::networkmode) {
			wait_lock = 30000; // 30s to drive on, if the client in question had left
		}
		fpl->eingabe_abschliessen();
	}
	// remove wrong freight
	check_freight();
	// some convois had wrong old direction in them
	if(  state<DRIVING  ||  state==LOADING  ) {
		alte_richtung = fahr[0]->get_fahrtrichtung();
	}
	// Knightly : if lineless convoy -> register itself with stops
	if(  !line.is_bound()  ) {
		register_stops();
	}

	for(int i = 0; i < anz_vehikel; i++) 
	{
		fahr[i]->remove_stale_freight();
	}
}


// since now convoi states go via werkzeug_t
void convoi_t::call_convoi_tool( const char function, const char *extra)
{
	werkzeug_t *w = create_tool( WKZ_CONVOI_TOOL | SIMPLE_TOOL );
	cbuffer_t param;
	param.printf("%c,%u", function, self.get_id());
	if(  extra  &&  *extra  ) {
		param.printf(",%s", extra);
	}
	w->set_default_param(param);
	welt->set_werkzeug( w, get_besitzer() );
	// since init always returns false, it is safe to delete immediately
	delete w;
}


void convoi_t::rotate90( const sint16 y_size )
{
	last_stop_pos.rotate90( y_size );
	record_pos.rotate90( y_size );
	home_depot.rotate90( y_size );
	route.rotate90( y_size );
	if(fpl) {
		fpl->rotate90( y_size );
	}
	for(  int i=0;  i<anz_vehikel;  i++  ) {
		fahr[i]->rotate90_freight_destinations( y_size );
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
	for( uint8 i=0; i<anz_vehikel; i++ ) {
		len += fahr[i]->get_besch()->get_length();
	}
	return len;
}


/**
 * convoi add their running cost for travelling one tile
 * Also, increment the odometer.
 * @author Hj. Malthaner
 */
void convoi_t::add_running_cost(sint64 cost, const weg_t *weg)
{
	jahresgewinn += cost;

	if(weg && weg->get_besitzer() != get_besitzer() && weg->get_besitzer() != NULL && (!welt->get_settings().get_toll_free_public_roads() || (weg->get_waytype() != road_wt || weg->get_player_nr() != 1)))
	{
		// running on non-public way costs toll (since running costas are positive => invert)
		sint32 toll = -(cost * welt->get_settings().get_way_toll_runningcost_percentage()) / 100l;
		if(welt->get_settings().get_way_toll_waycost_percentage())
		{
			if(weg->is_electrified() && needs_electrification())
			{
				// toll for using electricity
				grund_t *gr = welt->lookup(weg->get_pos());
				for(int i = 1; i < gr->get_top(); i++) 
				{
					ding_t *d = gr->obj_bei(i);
					if(wayobj_t const* const wo = ding_cast<wayobj_t>(d))  
					{
						if(wo->get_waytype() == weg->get_waytype())
						{
							toll += (wo->get_besch()->get_wartung() * welt->get_settings().get_way_toll_waycost_percentage()) / 100l;
							break;
						}
					}
				}
			}
			// now add normal way toll be maintenance
			toll += (weg->get_besch()->get_wartung() * welt->get_settings().get_way_toll_waycost_percentage()) / 100l;
		}
		weg->get_besitzer()->book_toll_received( toll, get_schedule()->get_waytype() );
		get_besitzer()->book_toll_paid(         -toll, get_schedule()->get_waytype() );
	}

	get_besitzer()->book_running_costs( cost, get_schedule()->get_waytype());
	book( cost, CONVOI_OPERATIONS );
	book( cost, CONVOI_PROFIT );
}

void convoi_t::increment_odometer(uint32 steps)
{ 
	// Increament the way distance: used for apportioning revenue by owner of ways.
	// Use steps, as only relative distance is important here.
	sint8 player;
	waytype_t waytpe = fahr[0]->get_waytype();
	const grund_t* gr = welt->lookup(get_pos());
	weg_t* way = gr ? gr->get_weg(waytpe) : NULL;
	if(way == NULL)
	{
		player = besitzer_p->get_player_nr();
	}
	else
	{
		if(waytpe == road_wt && way->get_player_nr() == 1 && welt->get_settings().get_toll_free_public_roads())
		{
			player = besitzer_p->get_player_nr();
		}
		else
		{
			player = way->get_player_nr();
		}
	}

	if(player < 0)
	{
		player = besitzer_p->get_player_nr();
	}

	FOR(departure_map, & i, *departures)
	{
		i.value.increment_way_distance(player, steps);
	}
	
	steps_since_last_odometer_increment += steps;
	if (steps_since_last_odometer_increment < welt->get_settings().get_steps_per_km())
	{
		return;
	}

	const sint64 km = steps_since_last_odometer_increment / welt->get_settings().get_steps_per_km();
	book( km, CONVOI_DISTANCE );
	total_distance_traveled += km;
	steps_since_last_odometer_increment -= km * steps_since_last_odometer_increment;
	for(uint8 i= 0; i < anz_vehikel; i++) 
	{
		const grund_t *gr = welt->lookup(fahr[0]->get_pos());
		if(gr)
		{
			add_running_cost(-fahr[i]->get_besch()->get_running_cost(welt), gr->get_weg(fahr[0]->get_waytype()));
		}
		else
		{
			add_running_cost(-fahr[i]->get_besch()->get_running_cost(welt), NULL);
		}
	}
}

// BG, 06.11.2011
bool convoi_t::calc_route(koord3d start, koord3d ziel, sint32 max_speed)
{
	route_infos.clear();
	const grund_t* gr = welt->lookup(ziel);
	if(gr && fahr[0]->get_waytype() == air_wt && gr->get_halt().is_bound() && welt->lookup(ziel)->get_halt()->has_no_control_tower())
	{
		return false;
	}
	return fahr[0]->calc_route(start, ziel, max_speed, &route);
}

void convoi_t::update_route(uint32 index, const route_t &replacement)
{
	// replace route with replacement starting at index.
	route_infos.clear();
	route.remove_koord_from(index);
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
void convoi_t::calc_acceleration(long delta_t)
{
	// existing_convoy_t is designed to become a part of convoi_t. 
	// There it will help to minimize updating convoy summary data.
	convoi_t &convoy = *this;
	vehikel_t &front = *this->front();

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
	if (next_stop_index >= 65000u) // BG, 07.10.2011: currently only waggon_t sets next_stop_index. 
	// BG, 09.08.2012: use ">= 65000u" as INVALID_INDEX (65530u) sometimes is incermented or decremented.
	{
		next_stop_index = route_count;
	}
#ifndef DEBUG_PHYSICS
	sint32 steps_til_limit;
	sint32 steps_til_brake;
#endif
	const sint32 brake_steps = convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), akt_speed);
	// use get_route_infos() for the first time accessing route_infos to eventually initialize them.
	const uint32 route_infos_count = get_route_infos().get_count();
	if (route_infos_count > 0 && route_infos_count >= next_stop_index && next_stop_index > current_route_index)
	{
		sint32 i = current_route_index - 1;
		if(i < 0)
		{
			i = 0;
		}
		const convoi_t::route_info_t &current_info = route_infos.get_element(i);
		if (current_info.speed_limit != SPEED_UNLIMITED)
		{
			convoy.update_max_speed(speed_to_kmh(current_info.speed_limit));
		}
		const convoi_t::route_info_t &limit_info = route_infos.get_element(next_stop_index - 1);
		steps_til_limit = route_infos.calc_steps(current_info.steps_from_start, limit_info.steps_from_start);
		steps_til_brake = steps_til_limit - brake_steps;
		switch (limit_info.direction)
		{
			case ribi_t::nord:
			case ribi_t::west:
				// BG, 10-11-2011: vehikel_t::hop() reduces the length of the tile, if convoy is going to stop on the tile.
				// Most probably for eye candy reasons vehicles do not exactly move on their tiles.
				// We must do the same here to avoid abrupt stopping.
				sint32 	delta_tile_len = current_info.steps_from_start;
				if (i > 0) delta_tile_len -= route_infos.get_element(i - 1).steps_from_start;
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
		for (i++; i < next_stop_index; i++)
		{
			const convoi_t::route_info_t &limit_info = route_infos.get_element(i);
			if (limit_info.speed_limit < min_limit)
			{
				min_limit = limit_info.speed_limit;
				const sint32 limit_steps = brake_steps - convoy.calc_min_braking_distance(welt->get_settings(), convoy.get_weight_summary(), limit_info.speed_limit);
				const sint32 route_steps = route_infos.calc_steps(current_info.steps_from_start, steps_from_start);
				const sint32 st = route_steps - limit_steps;

				if (steps_til_brake > st)
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
	akt_speed_soll = get_min_top_speed();
	convoy.calc_move(welt->get_settings(), delta_t, akt_speed_soll, next_speed_limit, steps_til_limit, steps_til_brake, akt_speed, sp_soll, v);
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
		vehikel_t &front = *this->front();
		const uint32 route_count = route.get_count(); // at least ziel will be there, even if calculating a route failed.
		const uint16 current_route_index = front.get_route_index(); // actually this is current route index + 1!!!
		fixed_list_tpl<sint16, 16> corner_data;
		const waytype_t waytype = front.get_waytype();

		// calc route infos
		route_infos.set_count(route_count);
		uint16 i = max(0, current_route_index - 2);

		koord3d current_tile = route.position_bei(i);
		convoi_t::route_info_t &start_info = route_infos.get_element(i);
		start_info.direction = front.get_fahrtrichtung();
		start_info.steps_from_start = 0;
		const weg_t *current_weg = get_weg_on_grund(welt->lookup(current_tile), waytype);
		start_info.speed_limit = front.calc_speed_limit(current_weg, NULL, &corner_data, start_info.direction, start_info.direction);

		sint32 takeoff_index = front.get_takeoff_route_index();
		sint32 touchdown_index = front.get_touchdown_route_index();
		for (i++; i < route_count; i++)
		{
			convoi_t::route_info_t &current_info = route_infos.get_element(i - 1);
			convoi_t::route_info_t &this_info = route_infos.get_element(i);
			const koord3d this_tile = route.position_bei(i);
			const koord3d next_tile = route.position_bei(min(i + 1, route_count - 1));
			this_info.speed_limit = SPEED_UNLIMITED;
			this_info.steps_from_start = current_info.steps_from_start + front.get_tile_steps(current_tile.get_2d(), next_tile.get_2d(), this_info.direction);
			const weg_t *this_weg = get_weg_on_grund(welt->lookup(this_tile), waytype);
			if (i >= touchdown_index || i <= takeoff_index)
			{
				// not an aircraft (i <= takeoff_index == INVALID_INDEX == 65530u) or
				// aircraft on ground (not between takeoff_index and touchdown_index): get speed limit
				current_info.speed_limit = this_weg ? front.calc_speed_limit(this_weg, current_weg, &corner_data, this_info.direction, current_info.direction) : SPEED_UNLIMITED;
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
	for( int i=0;  i<anz_vehikel;  i++  ) {
		current_length += fahr[i]->get_besch()->get_length();
		if(length<current_length) {
			return i;
		}
	}
	return anz_vehikel;
}


// moves all vehicles of a convoi
bool convoi_t::sync_step(long delta_t)
{
	// still have to wait before next action?
	wait_lock -= delta_t;
	if(wait_lock <= 0) {
		wait_lock = 0;
	}
	else {
		return true;
	}

	switch(state) {
		case INITIAL:
			// jemand muß start aufrufen, damit der convoi von INITIAL
			// nach ROUTING_1 geht, das kann nicht automatisch gehen
			
			//someone must call start, so that convoi from INITIAL 
			//to ROUTING_1 goes, which cannot go automatically (Google)
			break;

		case FAHRPLANEINGABE:
		case ROUTING_1:
		case DUMMY4:
		case DUMMY5:
		case NO_ROUTE:
		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
		case REVERSING:
			// Hajo: this is an async task, see step()
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
				fahr[0]->last_stop_pos = fahr[0]->get_pos().get_2d();
				last_stop_id = 0;

				// now actually move the units
				while(sp_soll>>12) {
					// Attempt to move one step.
					uint32 sp_hat = fahr[0]->fahre_basis(1<<YARDS_PER_VEHICLE_STEP_SHIFT);
					int v_nr = get_vehicle_at_length((++steps_driven)>>4);
					// stop when depot reached
					if(state==INITIAL  ||  state==ROUTING_1) {
						break;
					}
					// until all are moving or something went wrong (sp_hat==0)
					if(sp_hat==0  ||  v_nr==anz_vehikel) {
						// Attempted fix of depot squashing problem:
						// but causes problems with signals.
						//if (v_nr==anz_vehikel) {
							steps_driven = -1;
						//}
						//else {
						//}

 						state = DRIVING;
 						return true;
					}
					// now only the right numbers
					for(int i=1; i<=v_nr; i++) {
						fahr[i]->fahre_basis(sp_hat);
					}
					sp_soll -= sp_hat;
				}
				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<anz_vehikel;  i++  ) {
						fahr[i]->rauche();
						fahr[i]->last_stop_pos = fahr[i]->get_pos().get_2d();
					}
				}
			}

			departures->clear();

			break;	// LEAVING_DEPOT
			
		case DRIVING:
			{
				calc_acceleration(delta_t);
				//akt_speed = kmh_to_speed(50);

				// now actually move the units
				//moved to inside calc_acceleration(): 
				//sp_soll += (akt_speed*delta_t);

				// While sp_soll is a signed integer fahre_basis() accepts an unsigned integer. 
				// Thus running backwards is impossible.  Instead sp_soll < 0 is converted to very large 
				// distances and results in "teleporting" the convoy to the end of its pre-caclulated route.
				uint32 sp_hat = fahr[0]->fahre_basis(sp_soll < 0 ? 0 : sp_soll);
				// stop when depot reached ...
				if(state==INITIAL) {
					break;
				}
				// now move the rest (so all vehikel are moving synchroniously)
				for(unsigned i=1; i<anz_vehikel; i++) {
					fahr[i]->fahre_basis(sp_hat); //"move basis"
				}
				// maybe we have been stopped be something => avoid wide jumps
				sp_soll = (sp_soll-sp_hat) & 0x0FFF;

				// smoke for the engines
				next_wolke += delta_t;
				if(next_wolke>500) {
					next_wolke = 0;
					for(int i=0;  i<anz_vehikel;  i++  ) {
						fahr[i]->rauche();
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
			// Hajo: waiting is asynchronous => fixed waiting order and route search
			break;

		case SELF_DESTRUCT:
			// see step, since destruction during a screen update ma give stange effects
			break;

		default:
			dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
			break;
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
	if(  anz_vehikel>0 && fpl  ) 
	{
		koord3d start = fahr[0]->get_pos();
		koord3d ziel = fpl->get_current_eintrag().pos;

		// avoid stopping midhalt
		if(  start==ziel  ) {
			halthandle_t halt = haltestelle_t::get_halt(welt,ziel,get_besitzer());
			if(  halt.is_bound()  &&  route.is_contained(start)  ) {
				for(  uint32 i=route.index_of(start);  i<route.get_count();  i++  ) {
					grund_t *gr = welt->lookup(route.position_bei(i));
					if(  gr  && gr->get_halt()==halt  ) {
						ziel = gr->get_pos();
					}
					else {
						break;
					}
				}
			}
		}

		bool success = calc_route(start, ziel, speed_to_kmh(get_min_top_speed()));
		grund_t* gr = welt->lookup(ziel);
		bool extend_route = gr;
		switch (fahr[0]->get_waytype())
		{
			// convoys of these way types extend their routes in fahr[0]->ist_weg_frei() and thus don't need it here.
			case air_wt:
			case track_wt:
			case monorail_wt:
			case maglev_wt:
			case tram_wt:
			case narrowgauge_wt:
				extend_route = false;
		}

		if(extend_route && !gr->get_depot())
		{
			// We need to calculate the full route through to the next signal or reversing point
			// to avoid ignoring signals. 
			int counter = fpl->get_count();

			linieneintrag_t const * schedule_entry = &fpl->get_current_eintrag();
			while(success && counter--)
			{
				if(schedule_entry->reverse || haltestelle_t::get_halt(welt, schedule_entry->pos, get_besitzer()).is_bound())
				{
					// convoy must stop at current route end.
					break;
				}
				advance_schedule();
				schedule_entry = &fpl->get_current_eintrag();
				success = fahr[0]->reroute(route.get_count() - 1, schedule_entry->pos);
			}
		}

		if(!success)
		{
			if(state != NO_ROUTE) 
			{
				state = NO_ROUTE;
				get_besitzer()->bescheid_vehikel_problem( self, ziel );
			}
			// wait 25s before next attempt
			wait_lock = 25000;
		}
		else
		{
			vorfahren();
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
 * Asynchrne step methode des Convois
 * @author Hj. Malthaner
 */
void convoi_t::step()
{
	if(wait_lock!=0) {
		return;
	}

	// moved check to here, as this will apply the same update
	// logic/constraints convois have for manual schedule manipulation
	if (line_update_pending.is_bound()) {
		check_pending_updates();
	}

	bool autostart = false;

	switch(state) {

		case INITIAL:
			// If there is a pending replacement, just do it
			if (replace && replace->get_replacing_vehicles()->get_count()>0) 
			{
				
				autostart = replace->get_autostart();

				// Knightly : before replacing, copy the existing set of goods category index
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
					tstrncpy(buf + name_offset, translator::translate(fahr[0]->get_besch()->get_name()), 116);
					const bool keep_name = strcmp(get_name(), buf);	
					vector_tpl<vehikel_t*> new_vehicles;
					vehikel_t* veh = NULL;
					// Acquire the new one
					ITERATE_PTR(replace->get_replacing_vehicles(),i)
					{
						veh = NULL;
						// First - check whether there are any of the required vehicles already
						// in the convoy (free)
						for(uint8 k = 0; k < anz_vehikel; k++)
						{
							if(fahr[k]->get_besch() == replace->get_replacing_vehicle(i))
							{
								veh = remove_vehikel_bei(k);
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
							for(uint16 j = 0; j < anz_vehikel; j ++)
							{	
								for(uint8 c = 0; c < fahr[j]->get_besch()->get_upgrades_count(); c ++)
								{
									if(replace->get_replacing_vehicle(i) == fahr[j]->get_besch()->get_upgrades(c))
									{
										veh = vehikelbauer_t::baue(get_pos(), get_besitzer(), NULL, replace->get_replacing_vehicle(i), true); 
										upgrade_vehicle(j, veh);
										remove_vehikel_bei(j);
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
					for(sint8 a = anz_vehikel-1;  a >= 0; a--) 
					{
						if(!replace->get_retain_in_depot())
						{
							//Sell any vehicles not upgraded or kept.
							sint64 value = fahr[a]->calc_restwert();
							waytype_t wt = fahr[a]->get_besch()->get_waytype();
							besitzer_p->book_new_vehicle( value, dep->get_pos().get_2d(),wt );
							delete fahr[a];
							anz_vehikel--;
						}
						else
						{
							vehikel_t* old_veh = remove_vehikel_bei(a);
							old_veh->loesche_fracht();
							old_veh->set_erstes(false);
							old_veh->set_letztes(false);
							dep->get_vehicle_list().append(old_veh);
						}
					}
					anz_vehikel = 0;
					reset();

					//Next, add all the new vehicles to the convoy in order.
					ITERATE(new_vehicles,b)
					{
						dep->append_vehicle(self, new_vehicles[b], false, false);
					}
					
					if (!keep_name) 
					{
						set_name(fahr[0]->get_besch()->get_name());
					}

					clear_replace();

					if (line.is_bound()) {
						line->recalc_status();
						if (line->get_replacing_convoys_count()==0) {
							char buf[256];
							sprintf(buf, translator::translate("Replacing\nvehicles of\n%-20s\ncompleted"), line->get_name());
							welt->get_message()->add_message(buf, home_depot.get_2d(),message_t::general, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);
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
							haltestelle_t::refresh_routing(fpl, differences, besitzer_p);
						}
					}

					if (autostart) {
						dep->start_convoi(self, false);
					}
				}
			}
			break;

		case DUMMY4:
		case DUMMY5:
		break;

		case REVERSING:
			if(wait_lock == 0)
			{
				state = CAN_START;
				if(fahr[0]->last_stop_pos == fahr[0]->get_pos().get_2d())
				{
					book_waiting_times();
				}
			}
			
			break;

		case FAHRPLANEINGABE:
			// schedule window closed?
			if(fpl!=NULL  &&  fpl->ist_abgeschlossen()) {

				set_schedule(fpl);

				if(  fpl->empty()  ) 
				{
					// no entry => no route ...
					state = NO_ROUTE;
					// A convoy without a schedule should not be left lingering on the map.
					emergency_go_to_depot();
					// Get out of this routine; object might be destroyed.
					return;
				}
				else {
					if(  fpl->get_current_eintrag().pos==get_pos()  ) {
						// We are at the scheduled location
						grund_t *gr = welt->lookup(fpl->get_current_eintrag().pos);
						if(  gr  ) {
							depot_t * this_depot = gr->get_depot();
							// double-check for right depot type based on first vehicle
							if (  this_depot &&  this_depot->is_suitable_for(fahr[0])  ) {
								// If it's a suitable depot, move into the depot
								// This check must come before the station check, because for
								// ships we may be in a depot and at a sea stop!
								enter_depot( gr->get_depot() );
								break;
							}
						}
					}
					halthandle_t h = haltestelle_t::get_halt( welt, get_pos(), get_besitzer() );
					if(  h.is_bound()  &&  h==haltestelle_t::get_halt( welt, fpl->get_current_eintrag().pos, get_besitzer() )  ) {
						// We are at the station we are scheduled to be at
						// (possibly a different platform)
						if (route.get_count() > 0) {
							koord3d const& pos = route.back();
							if (h == haltestelle_t::get_halt(welt, pos, get_besitzer())) {
								// If this is also the station at the end of the current route
								// (the correct platform)
								if (get_pos() == pos) {
									// And this is also the correct platform... then load.
									state = LOADING;
									break;
								}
								else {
									// Right station, wrong platform
									state = DRIVING;
									break;
								}
							}
						}
						else {
							// We're at the scheduled station,
							// but there is no programmed route.
							if(  drive_to()  ) {
								state = DRIVING;
								break;
							}
						}
					}

					// We aren't at our destination; start routing.
					state = ROUTING_1;
				}
			}
			break;

		case ROUTING_1:
			{
				vehikel_t* v = fahr[0];

				if(  fpl->empty()  ) {
					state = NO_ROUTE;
					besitzer_p->bescheid_vehikel_problem( self, koord3d::invalid );
				}
				else {
					// check first, if we are already there:
					assert( fpl->get_aktuell()<fpl->get_count()  );
					if(  v->get_pos()==fpl->get_current_eintrag().pos  ) {
						advance_schedule();
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
			no_route_retry_count ++;
			if(no_route_retry_count >= 3)
			{
				// If the convoy is stuck for too long, send it to a depot.
				emergency_go_to_depot();
				// get out of this routine; vehicle might be destroyed
				return;
			}
			else if (fpl->empty()) 
			{
				// no entries => no route ...
			} 
			else 
			{
				// Hajo: now calculate a new route
				drive_to();
			}
			break;

		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
			{
				vehikel_t* v = fahr[0];

				int restart_speed=-1;
				if (v->ist_weg_frei(restart_speed,false)) {
					// can reserve new block => drive on
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
					if(haltestelle_t::get_halt(welt,v->get_pos(),besitzer_p).is_bound()) {
						v->play_sound();
					}
				}
				else if(  steps_driven==0  ) {
					// on rail depot tile, do not reserve this
					if(  grund_t *gr = welt->lookup(fahr[0]->get_pos())  ) {
						if (schiene_t* const sch0 = ding_cast<schiene_t>(gr->get_weg(fahr[0]->get_waytype()))) {
							sch0->unreserve(fahr[0]);
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
				int restart_speed=-1;
				if (fahr[0]->ist_weg_frei(restart_speed,false)) {
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
				}
				if(restart_speed>=0) {
					set_akt_speed(restart_speed);
				}
				if(state!=DRIVING) {
					set_tiles_overtaking( 0 );
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
		case FAHRPLANEINGABE:
		case NO_ROUTE:
			wait_lock = 25000;
			break;

		// action soon needed
		case ROUTING_1:
		case CAN_START:
		case WAITING_FOR_CLEARANCE:
			//wait_lock = 500;
			// Bernd Gabriel: simutrans experimental may have presets the wait_lock before. Don't overwrite it here, if it ought to wait longer.
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
	if(fpl->get_aktuell() == 0) {
		arrival_to_first_stop.add_to_tail(welt->get_zeit_ms());
	}

	// check if the convoi should switch direction
	if(  fpl->is_mirrored() && fpl->get_aktuell()==fpl->get_count()-1  ) {
		reverse_schedule = true;
	}
	else if( fpl->is_mirrored() && fpl->get_aktuell()==0  ) {
		reverse_schedule = false;
	}
	// advance the schedule cursor
	if (reverse_schedule) {
		fpl->advance_reverse();
	} else {
		fpl->advance();
	}
}

void convoi_t::neues_jahr()
{
    jahresgewinn = 0;
}


uint16 convoi_t::get_overcrowded() const
{
	uint16 overcrowded = 0;
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		overcrowded += fahr[i]->get_overcrowding();
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
	
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		if(fahr[i]->get_fracht_typ()->get_catg_index() == 0)
		{
			passenger_vehicles ++;
			capacity = fahr[i]->get_besch()->get_zuladung();
			comfort += fahr[i]->get_comfort(catering_level) * capacity;
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
	if(anz_vehikel==0) {
		DBG_DEBUG("convoi_t::new_month()","no vehicles => self destruct!");
		self_destruct();
		return;
	}

	// Deduct monthly fixed maintenance costs.
	// @author: jamespetts
	sint64 monthly_cost = 0;
	for(unsigned j=0;  j<get_vehikel_anzahl();  j++ ) 
	{
		// Monthly cost is positive, but add it up to a negative number for booking.
		monthly_cost -= get_vehikel(j)->get_besch()->get_fixed_cost(welt);
	}
	jahresgewinn += monthly_cost;
	book( monthly_cost, CONVOI_OPERATIONS );
	book( monthly_cost, CONVOI_PROFIT );
	// This is way too tedious a way to get my waytype...
	waytype_t my_waytype;
	if (get_schedule()) {
		my_waytype = get_schedule()->get_waytype();
	}
	else if (get_vehikel_anzahl()) {
		my_waytype = get_vehikel(0)->get_besch()->get_waytype();
	}
	else {
		my_waytype = ignore_wt;
	}
	get_besitzer()->book_vehicle_maintenance(monthly_cost, my_waytype);

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
	if(  state==NO_ROUTE  ) {
		get_besitzer()->bescheid_vehikel_problem( self, get_pos() );
	}
	// check for traffic jam
	if(state==WAITING_FOR_CLEARANCE) {
		state = WAITING_FOR_CLEARANCE_ONE_MONTH;
		// check, if now free ...
		// migh also reset the state!
		int restart_speed=-1;
		if (fahr[0]->ist_weg_frei(restart_speed,false)) {
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
			get_besitzer()->bescheid_vehikel_problem( self, koord3d::invalid );
		}
		state = WAITING_FOR_CLEARANCE_TWO_MONTHS;
	}
	// check for traffic jam
	if(state==CAN_START) {
		state = CAN_START_ONE_MONTH;
	}
	else if(state==CAN_START_ONE_MONTH  ||  state==CAN_START_TWO_MONTHS  ) {
		get_besitzer()->bescheid_vehikel_problem( self, koord3d::invalid );
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

	if(reversed)
	{
		// Put the train back into "forward" position
		if(reversed)
		{
			reversable = fahr[0]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
		}
		else
		{
			reversable = fahr[anz_vehikel - 1]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
		}
		reverse_order(reversable);
	}

	// Clear the departure table...
	departures->clear();

	// Set the speed to zero...
	set_akt_speed(0);

	// Make this the new home depot...
	// (Will be done again in convoi_arrived, but make sure to do it early in case of crashes)
	home_depot=dep->get_pos();

	// Hajo: remove vehicles from world data structure
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		grund_t* gr = welt->lookup(v->get_pos());
		if(gr) {
			// remove from blockstrecke
			v->set_letztes(true);
			v->verlasse_feld();
			v->set_flag( ding_t::not_on_map );
		}
	}

	dep->convoi_arrived(self, get_schedule());

	close_windows();

	// Hajo: since 0.81.5exp it's safe to
	// remove the current sync object from
	// the sync list from inside sync_step()
	welt->sync_remove(this);
	state = INITIAL;
	wait_lock = 0;
}


void convoi_t::start()
{
	if(state == INITIAL || state == ROUTING_1) {

		// set home depot to location of depot convoi is leaving
		if(route.empty()) 
		{
			home_depot = fahr[0]->get_pos();
		}
		else 
		{
			home_depot = route.front();
			fahr[0]->set_pos( home_depot );
		}
		// put the convoi on the depot ground, to get automatical rotation
		// (vorfahren() will remove it anyway again.)
		grund_t *gr = welt->lookup( home_depot );
		assert(gr);
		gr->obj_add( fahr[0] );

		alte_richtung = ribi_t::keine;
		no_load = false;
		depot_when_empty = false;

		// if the schedule is mirrored, convoys starting
		// reversed should go directly to the end.
		if( fpl->is_mirrored() && reverse_schedule ) {
			fpl->advance_reverse();
		}

		state = ROUTING_1;

		// recalc weight and image
		// also for any vehicle entered a depot, set_letztes is true! => reset it correctly
		sint64 restwert_delta = 0;
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->set_erstes( false );
			fahr[i]->set_letztes( false );
			restwert_delta -= fahr[i]->calc_restwert();
			fahr[i]->set_driven();
			restwert_delta += fahr[i]->calc_restwert();
			fahr[i]->clear_flag( ding_t::not_on_map );
			fahr[i]->beladen( halthandle_t() );
		}
		fahr[0]->set_erstes( true );
		fahr[anz_vehikel-1]->set_letztes( true );
		// do not show the vehicle - it will be wrong positioned -vorfahren() will correct this
		fahr[0]->set_bild(IMG_LEER);

		// update finances for used vehicle reduction when first driven
		besitzer_p->update_assets( restwert_delta, get_schedule()->get_waytype());

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
			haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p);
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
 * called from the first vehikel_t of a convoi */
void convoi_t::ziel_erreicht()
{
	const vehikel_t* v = fahr[0];
	alte_richtung = v->get_fahrtrichtung();

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
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);
		}

		enter_depot(dp);
	}
	else {
		// no suitable depot reached, check for stop!
		halthandle_t halt = haltestelle_t::get_halt(welt, v->get_pos(),besitzer_p);
		if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			set_akt_speed(0);
			halt->book(1, HALT_CONVOIS_ARRIVED);
			state = LOADING;
			go_on_ticks = WAIT_INFINITE;	// we will eventually wait from now on
		}
		else {
			// Neither depot nor station: waypoint
			advance_schedule();
			state = ROUTING_1;
			if(replace && depot_when_empty &&  has_no_cargo()) {
				depot_when_empty=false;
				no_load=false;
				go_to_depot(false);
			}
		}
	}
	wait_lock = 0;
}


/**
 * Wait until vehicle 0 returns go-ahead
 * @author Hj. Malthaner
 */
void convoi_t::warten_bis_weg_frei(int restart_speed)
{
	if(!is_waiting()) {
		state = WAITING_FOR_CLEARANCE;
		wait_lock = 0;
	}
	if(restart_speed>=0) {
		// langsam anfahren
		// "slow start" (Google)
		set_akt_speed(restart_speed);
	}
}


bool convoi_t::add_vehikel(vehikel_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehikel()","at pos %i of %i totals.",anz_vehikel,max_vehicle);
	// extend array if requested (only needed for trains)
	if(anz_vehikel == max_vehicle) {
DBG_MESSAGE("convoi_t::add_vehikel()","extend array_tpl to %i totals.",max_rail_vehicle);
		//fahr.resize(max_rail_vehicle, NULL);
		fahr.resize(max_rail_vehicle);
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

		// Added by		: Knightly
		// Adapted from : simline_t
		// Purpose		: Try to update supported goods category of this convoy
		if (v->get_fracht_max() > 0) 
		{
			const ware_besch_t *ware_type = v->get_fracht_typ();
			if (ware_type != warenbauer_t::nichts)
				goods_catg_index.append_unique(ware_type->get_catg_index(), 1);
		}


		const vehikel_besch_t *info = v->get_besch();
		if(info->get_leistung()) {
			is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
		}
		//sum_leistung += info->get_leistung();
		//if(info->get_engine_type() == vehikel_besch_t::steam)
		//{
		//	power_from_steam += info->get_leistung();
		//	power_from_steam_with_gear += info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor();
		//}
		//sum_gear_und_leistung += (info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
		//sum_gewicht += info->get_gewicht();
		//min_top_speed = min( min_top_speed, kmh_to_speed( v->get_besch()->get_geschw() ) );
		//sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		invalidate_vehicle_summary();
		freight_info_resort = true;
		// Add good_catg_index:
		if(v->get_fracht_max() != 0) {
			const ware_besch_t *ware=v->get_fracht_typ();
			if(ware!=warenbauer_t::nichts  ) {
				goods_catg_index.append_unique( ware->get_catg_index() );
			}
		}
		// check for obsolete
		if(!has_obsolete  &&  welt->use_timeline()) {
			has_obsolete = v->get_besch()->is_obsolete( welt->get_timeline_year_month(), welt );
		}
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();

DBG_MESSAGE("convoi_t::add_vehikel()","now %i of %i total vehikels.",anz_vehikel,max_vehicle);
	return true;
}

void convoi_t::upgrade_vehicle(uint16 i, vehikel_t* v)
{
	// Adapted from the add/remove vehicle functions
	// @author: jamespetts, February 2010

DBG_MESSAGE("convoi_t::upgrade_vehicle()","at pos %i of %i totals.",i,max_vehicle);
	
	// now append
	v->set_convoi(this);
	vehikel_t* old_vehicle = fahr[i];
	fahr[i] = v;

	// Amend the name if the name is the default name and it is the first vehicle
	// being replaced.

	if(i == 0)
	{
		char buf[128];
		name_offset = sprintf(buf,"(%i) ",self.get_id() );
		tstrncpy(buf + name_offset, translator::translate(old_vehicle->get_besch()->get_name()), 116);
		if(!strcmp(get_name(), buf))
		{
			set_name(v->get_besch()->get_name());
		}
	}

	// Added by		: Knightly
	// Adapted from : simline_t
	// Purpose		: Try to update supported goods category of this convoy
	if (v->get_fracht_max() > 0) 
	{
		const ware_besch_t *ware_type = v->get_fracht_typ();
		if (ware_type != warenbauer_t::nichts)
			goods_catg_index.append_unique(ware_type->get_catg_index(), 1);
	}

	const vehikel_besch_t *info = v->get_besch();
	// still requires electrification?
	if(is_electric) {
		is_electric = false;
		for(unsigned i=0; i<anz_vehikel; i++) {
			if(fahr[i]->get_besch()->get_leistung()) {
				is_electric |= fahr[i]->get_besch()->get_engine_type()==vehikel_besch_t::electric;
			}
		}
	}

	if(info->get_leistung()) {
		is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
	}

	//min_top_speed = calc_min_top_speed(fahr, anz_vehikel);
	
	// Add power and weight of the new vehicle
	//sum_leistung += info->get_leistung();
	//sum_gear_und_leistung += (info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
	//sum_gewicht += info->get_gewicht();
	//sum_gesamtgewicht = sum_gewicht;

	// Remove power and weight of the old vehicle
	//info = old_vehicle->get_besch();
	//sum_leistung -= info->get_leistung();
	//sum_gear_und_leistung -= (info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
	//sum_gewicht -= info->get_gewicht();

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

	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();
	
	delete old_vehicle;

DBG_MESSAGE("convoi_t::upgrade_vehicle()","now %i of %i total vehikels.",i,max_vehicle);
}

vehikel_t *convoi_t::remove_vehikel_bei(uint16 i)
{
	vehikel_t *v = NULL;
	if(i<anz_vehikel) {
		v = fahr[i];
		if(v != NULL) {
			for(unsigned j=i; j<anz_vehikel-1u; j++) {
				fahr[j] = fahr[j + 1];
			}

			v->set_convoi(NULL);

			--anz_vehikel;
			fahr[anz_vehikel] = NULL;

			// Added by		: Knightly
			// Purpose		: To recalculate the list of supported goods category
			recalc_catg_index();

			//const vehikel_besch_t *info = v->get_besch();
			//sum_leistung -= info->get_leistung();
			//sum_gear_und_leistung -= (info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
			//sum_gewicht -= info->get_gewicht();
		}
		//sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		invalidate_vehicle_summary();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(anz_vehikel > 0) {
			set_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		//min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

		// check for obsolete
		if(has_obsolete) {
			has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
		}

		recalc_catg_index();

		// still requires electrifications?
		if(is_electric) {
			is_electric = false;
			for(unsigned i=0; i<anz_vehikel; i++) {
				if(fahr[i]->get_besch()->get_leistung()) {
					is_electric |= fahr[i]->get_besch()->get_engine_type()==vehikel_besch_t::electric;
				}
			}
		}
	}

	highest_axle_load = calc_highest_axle_load();
	longest_min_loading_time = calc_longest_min_loading_time();
	longest_max_loading_time = calc_longest_max_loading_time();

	return v;
}

// recalc what good this convoy is moving
void convoi_t::recalc_catg_index()
{
	goods_catg_index.clear();

	for(  uint8 i = 0;  i < get_vehikel_anzahl();  i++  ) {
		// Only consider vehicles that really transport something
		// this helps against routing errors through passenger
		// trains pulling only freight wagons
		if(get_vehikel(i)->get_fracht_max() == 0) {
			continue;
		}
		const ware_besch_t *ware=get_vehikel(i)->get_fracht_typ();
		if(ware!=warenbauer_t::nichts  ) {
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
	// anz_vehikel muss korrekt init sein
	// "anz vehicle must be correctly INIT" (Babelfish)
	if(anz_vehikel>0) {
		fahr[0]->set_erstes(true);
		for(unsigned i=1; i<anz_vehikel; i++) {
			fahr[i]->set_erstes(false);
			fahr[i - 1]->set_letztes(false);
		}
		fahr[anz_vehikel - 1]->set_letztes(true);
	}
	else {
		dbg->warning("convoi_t::set_erstes_letzes()", "called with anz_vehikel==0!");
	}
}


// remove wrong freight when schedule changes etc.
void convoi_t::check_freight()
{
	for(unsigned i=0; i<anz_vehikel; i++) {
		fahr[i]->remove_stale_freight();
	}
	calc_loading();
	freight_info_resort = true;
}


bool convoi_t::set_schedule(schedule_t * f)
{
	if(  state==SELF_DESTRUCT  ) {
		return false;
	}

	states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::set_schedule()", "new=%p, old=%p", f, fpl);
	assert(f != NULL);

	if(!line.is_bound() && old_state != INITIAL)
	{
		// New method - recalculate as necessary

		// Added by : Knightly
		if ( fpl == f && old_fpl )	// Case : Schedule window of operating convoy
		{
			if ( !old_fpl->matches(welt, fpl) )
			{
				haltestelle_t::refresh_routing(old_fpl, goods_catg_index, besitzer_p);
				haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p);
			}
		}
		else
		{
			if (fpl != f)
			{
				haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p);
			}
			haltestelle_t::refresh_routing(f, goods_catg_index, besitzer_p);
		}
	}
	
	// happens to be identical?
	if(fpl!=f) {
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
			if(  !f->matches( welt, fpl )  ) {
				// Knightly : merely change schedule and do not involve line
				//				-> unregister stops from old schedule now and register stops from new schedule later
				changed = true;
				unregister_stops();
			}
		}
		// destroy a possibly open schedule window
		//"is completed" (Google)
		if(  fpl  &&  !fpl->ist_abgeschlossen()  ) {
			destroy_win((ptrdiff_t)fpl);
			delete fpl;
		}
		fpl = f;
		if(  changed  ) {
			// Knightly : if line is unset or schedule is changed
			//				-> register stops from new schedule
			register_stops();
			// Also, clear the departures table, which may now
			// be out of date.
			departures->clear();
		}
	}

	// remove wrong freight
	check_freight();

	// ok, now we have a schedule
	if(old_state != INITIAL) 
	{
		state = FAHRPLANEINGABE;
	}
	// to avoid jumping trains
	alte_richtung = fahr[0]->get_fahrtrichtung();
	wait_lock = 0;
	return true;
}


schedule_t *convoi_t::create_schedule()
{
	if(fpl == NULL) {
		const vehikel_t* v = fahr[0];

		if (v != NULL) {
			fpl = v->erzeuge_neuen_fahrplan();
			fpl->eingabe_abschliessen();
		}
	}

	return fpl;
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
	ribi_t::ribi neue_richtung_rwr = ribi_t::rueckwaerts(fahr[0]->calc_richtung(route.front().get_2d(), route.position_bei(min(2, route.get_count() - 1)).get_2d()));
//	DBG_MESSAGE("convoi_t::go_alte_richtung()","neu=%i,rwr_neu=%i,alt=%i",neue_richtung_rwr,ribi_t::rueckwaerts(neue_richtung_rwr),alte_richtung);
	if(neue_richtung_rwr&alte_richtung) {
		set_akt_speed(8);
		return false;
	}

	// now get the actual length and the tile length
	uint16 convoi_length = 15;
	uint16 tile_length = 24;
	unsigned i;	// for visual C++
	const vehikel_t* pred = NULL;
	for(i=0; i<anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		grund_t *gr = welt->lookup(v->get_pos());

		// not last vehicle?
		// the length of last vehicle does not matter when it comes to positioning of vehicles
		if ( i+1 < anz_vehikel) {
			convoi_length += v->get_besch()->get_length();
		}

		if(gr==NULL  ||  (pred!=NULL  &&  (abs(v->get_pos().x-pred->get_pos().x)>=2  ||  abs(v->get_pos().y-pred->get_pos().y)>=2))  ) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_richtung()","broken convoy (id %i) found => fixing!",self.get_id());
			set_akt_speed(8);
			return false;
		}

		// now check, if ribi is straight and train is not
		ribi_t::ribi weg_ribi = gr->get_weg_ribi_unmasked(v->get_waytype());
		if(ribi_t::ist_gerade(weg_ribi)  &&  (weg_ribi|v->get_fahrtrichtung())!=weg_ribi) {
			dbg->warning("convoi_t::go_alte_richtung()","convoy with wrong vehicle directions (id %i) found => fixing!",self.get_id());
			set_akt_speed(8);
			return false;
		}

		if(  pred  &&  pred->get_pos()!=v->get_pos()  ) {
			tile_length += (ribi_t::ist_gerade(welt->lookup(pred->get_pos())->get_weg_ribi_unmasked(pred->get_waytype())) ? 16 : 8192/vehikel_t::get_diagonal_multiplier())*koord_distance(pred->get_pos(),v->get_pos());
		}

		pred = v;
	}
	// check if convoi is way too short (even for diagonal tracks)
	tile_length += (ribi_t::ist_gerade(welt->lookup(fahr[anz_vehikel-1]->get_pos())->get_weg_ribi_unmasked(fahr[anz_vehikel-1]->get_waytype())) ? 16 : 8192/vehikel_t::get_diagonal_multiplier());
	if(  convoi_length>tile_length  ) {
		dbg->warning("convoi_t::go_alte_richtung()","convoy too short (id %i) => fixing!",self.get_id());
		set_akt_speed(8);
		return false;
	}

	uint16 length = min((convoi_length/16u)+4u,route.get_count());	// maximum length in tiles to check

	// we just check, wether we go back (i.e. route tiles other than zero have convoi vehicles on them)
	for( int index=1;  index<length;  index++ ) {
		grund_t *gr=welt->lookup(route.position_bei(index));
		// now check, if we are already here ...
		for(unsigned i=0; i<anz_vehikel; i++) {
			if (gr->obj_ist_da(fahr[i])) {
				// we are turning around => start slowly and rebuilt train
				set_akt_speed(8);
				return false;
			}
		}
	}

//DBG_MESSAGE("convoi_t::go_alte_richtung()","alte=%d, neu_rwr=%d",alte_richtung,neue_richtung_rwr);

	// we continue our journey; however later cars need also a correct route entry
	// eventually we need to add their positions to the convoi's route
	koord3d pos = fahr[0]->get_pos();
	assert(pos == route.front());
	if(welt->lookup(pos)->get_depot()) {
		return false;
	}
	else {
		for(i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];
			// eventually add current position to the route
			if (route.front() != v->get_pos() && route.position_bei(1) != v->get_pos()) {
				route.insert(v->get_pos());
			}
			// eventually we need to add also a previous position to this path
			if(v->get_besch()->get_length()>8  &&  i+1<anz_vehikel) {
				if (route.front() != v->get_pos_prev() && route.position_bei(1) != v->get_pos_prev()) {
					route.insert(v->get_pos_prev());
				}
			}
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we must set the current route index (instead assuming 1)
	length = min((convoi_length/8u),route.get_count()-1);	// maximum length in tiles to check
	bool ok=false;
	for(i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		// this is such akward, since it takes into account different vehicle length
		const koord3d vehicle_start_pos = v->get_pos();
		for( int idx=0;  idx<=length;  idx++  ) {
			if(route.position_bei(idx)==vehicle_start_pos) {
				v->neue_fahrt(idx, false );
				ok = true;
				break;
			}
		}
		// too short?!? (rather broken then!)
		if(!ok) {
			return false;
		}
	}

	// on curves the vehicle may be already on the next tile but with a wrong direction
	for(i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		uint8 richtung = v->get_fahrtrichtung();
		uint8 neu_richtung = v->richtung();
		// we need to move to this place ...
		if(neu_richtung!=richtung  &&  (i!=0  ||  anz_vehikel==1  ||  ribi_t::ist_kurve(neu_richtung)) ) {
			// 90 deg bend!
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
		for(unsigned i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];

			grund_t* gr = welt->lookup(v->get_pos());
			if(gr) {
				gr->obj_remove(v);
				if(gr->ist_uebergang()) {
					crossing_t *cr = gr->find<crossing_t>(2);
					cr->release_crossing(v);
				}
				// eventually unreserve this
				if (schiene_t* const sch0 = ding_cast<schiene_t>(gr->get_weg(fahr[i]->get_waytype()))) {
					sch0->unreserve(v);
				}
			}
			v->neue_fahrt(0, true);
			v->betrete_feld();
		}

		// just advances the first vehicle
		vehikel_t* v0 = fahr[0];
		v0->set_erstes(false); // switches off signal checks ...
		v0->darf_rauchen(false);
		steps_driven = 0;
		// drive half a tile:
		for(int i=0; i<anz_vehikel; i++) {
			fahr[i]->fahre_basis( (VEHICLE_STEPS_PER_TILE/2)<<YARDS_PER_VEHICLE_STEP_SHIFT );
		}
		v0->darf_rauchen(true);
		v0->set_erstes(true); // switches on signal checks to reserve the next route

		// until all other are on the track
		state = CAN_START;
	}
	else 
	{
		const bool must_change_direction = !can_go_alte_richtung();
		// still leaving depot (steps_driven!=0) or going in other direction or misalignment?
		if(steps_driven > 0 || must_change_direction) 
		{
			//Convoy needs to reverse
			//@author: jamespetts
			if(must_change_direction)
			{
				switch(fahr[0]->get_waytype())
				{
					case road_wt:
					case air_wt:
					case water_wt:
						// Road vehicles, boats and aircraft do not need to change direction
						book_departure_time(welt->get_zeit_ms());
						book_waiting_times();
						break;

					default:
						
						if(reversed)
						{
							reversable = fahr[0]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
						}
						else
						{
							reversable = fahr[anz_vehikel - 1]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
						}
						
						reverse_delay = calc_reverse_delay();
						
						uint16 loading_time = current_loading_time;

						const grund_t* gr = welt->lookup(get_pos());
						if(gr && welt->get_zeit_ms() - arrival_time > reverse_delay && gr->is_halt())
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

						state = REVERSING;
						if(fahr[0]->last_stop_pos == fahr[0]->get_pos().get_2d())
						{
							// The convoy does not depart until it has reversed.
							book_departure_time(welt->get_zeit_ms() + reverse_delay);
						}
						reverse_order(reversable);
				}
			}

			// since start may have been changed
			k0 = route.front();

			uint32 train_length = move_to(*welt, k0, 0);

			// move one train length to the start position ...
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->get_direction_of_travel();
			if(neue_richtung==ribi_t::sued  ||  neue_richtung==ribi_t::ost)
			{
				// drive the convoi to the same position, but do not hop into next tile!
				if(  train_length%16==0  ) {
					// any space we need => just add
					train_length += fahr[anz_vehikel-1]->get_besch()->get_length();
				}
				else {
					// limit train to front of tile
					train_length += min( (train_length%CARUNITS_PER_TILE)-1, fahr[anz_vehikel-1]->get_besch()->get_length() );
				}
			}
			else
			{
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			fahr[0]->set_erstes(false); // switches off signal checks ...

			if(reversed && (reversable || fahr[0]->is_reversed()))
			{
				//train_length -= fahr[0]->get_besch()->get_length();
				train_length = 0;
				for(sint8 i = anz_vehikel - 1; i >= 0; i--)
				{
					vehikel_t* v = fahr[i];
					v->darf_rauchen(false);
					fahr[i]->fahre_basis( ((OBJECT_OFFSET_STEPS)*train_length)<<12 ); //"fahre" = "go" (Google)
					train_length += (v->get_besch()->get_length());	// this give the length in 1/OBJECT_OFFSET_STEPS of a full tile => all cars closely coupled!
					v->darf_rauchen(true);
				}
				train_length -= fahr[anz_vehikel - 1]->get_besch()->get_length();
			}

			else
			{
				//if(!reversable && fahr[0]->get_besch()->is_bidirectional())
				//{
				//	//This can sometimes relieve excess setting back on reversing.
				//	//Unfortunately, it seems to produce bizarre results on occasion.
				//	train_length -= (fahr[0]->get_besch()->get_length()) / 2;
				//	train_length = train_length > 0 ? train_length : 0;
				//}
				for(sint8 i = 0; i < anz_vehikel; i++)
				{
					vehikel_t* v = fahr[i];
					v->darf_rauchen(false);
					fahr[i]->fahre_basis( (VEHICLE_STEPS_PER_CARUNIT*train_length)<<YARDS_PER_VEHICLE_STEP_SHIFT );
					train_length -= v->get_besch()->get_length();
					// this gives the length in carunits, 1/CARUNITS_PER_TILE of a full tile => all cars closely coupled!
					v->darf_rauchen(true);
				}
					
			}
			fahr[0]->set_erstes(true);
		}

		else if(fahr[0]->last_stop_pos == fahr[0]->get_pos().get_2d())
		{
			book_departure_time(welt->get_zeit_ms());
			book_waiting_times();
		}

		int counter = 1;
		schedule_t* schedule = fpl;
		if(line.is_bound())
		{
			 counter = 2;
		}
		while(counter > 0 && schedule->get_count() > 0)
		{
			uint8 stop = fpl->get_aktuell();
			bool rev = !reverse_schedule;
			schedule->increment_index(&stop, &rev);
			//const uint8 last_stop = stop;
			if(stop < schedule->get_count())
			{
				// It might be possible for "stop" to be > the number of 
				// items in the schedule if the schedule has changed recently.
				schedule->eintrag[stop].reverse = (state == REVERSING);
			}
			//const bool check_rev = rev;
			schedule->increment_index(&stop, &rev);
			counter --;
			if(counter > 0)
			{
				schedule = line->get_schedule();
			}
		}

		if(!at_dest)
		{
			if(state != REVERSING)
			{
				state = CAN_START;
			}
			// to advance more smoothly
			int restart_speed=-1;
			if(fahr[0]->ist_weg_frei(restart_speed,false)) {
				// can reserve new block => drive on
				if(haltestelle_t::get_halt(welt,k0,besitzer_p).is_bound()) {
					fahr[0]->play_sound();
				}
				if(state != REVERSING)
				{
					state = DRIVING;
				}
			}
		}
		else {
			ziel_erreicht();
		}
	}

	// finally reserve route (if needed)
	if(  fahr[0]->get_waytype()!=air_wt  &&  !at_dest  ) {
		// do not prereserve for airplanes
		for(unsigned i=0; i<anz_vehikel; i++) {
			// eventually reserve this
			vehikel_t const& v = *fahr[i];
			const grund_t* gr = welt->lookup(v.get_pos());
			if(gr)
			{
				if (schiene_t* const sch0 = ding_cast<schiene_t>(welt->lookup(v.get_pos())->get_weg(v.get_waytype()))) {
					sch0->reserve(self,ribi_t::keine);
				}
			}
			else {
				break;
			}
		}
	}

	wait_lock = reverse_delay;
	//INT_CHECK("simconvoi 711");
}

void
convoi_t::reverse_order(bool rev)
{
	// Code snippet obtained and adapted from:
	// http://www.cprogramming.com/snippets/show.php?tip=15&count=30&page=0
	// by John Shao (public domain work)
	
	uint8 a = 0;
    vehikel_t* reverse;
	uint8 b  = anz_vehikel;

	if(rev)
	{
		fahr[0]->set_erstes(false);
	}
	else
	{
		if(!fahr[anz_vehikel - 1]->get_besch()->is_bidirectional())
		{
			//Do not change the order at all if the last vehicle is not bidirectional
			return;
		}

		a++;
		if(fahr[0]->get_besch()->get_leistung() > 0)
		{
			// If this is a locomotive, check for tenders. 
			a += fahr[0]->get_besch()->get_nachfolger_count();
			if(anz_vehikel >= a && fahr[a]->get_besch()->get_leistung() > 0)
			{
				// Check for double-headed tender locomotives
				a += fahr[a]->get_besch()->get_nachfolger_count();
			}
			if(anz_vehikel > 1 && fahr[1]->get_besch()->get_leistung() == 0 && fahr[1]->get_besch()->get_nachfolger_count() == 1 && fahr[1]->get_besch()->get_nachfolger(0)->get_leistung() == 0 && fahr[1]->get_besch()->get_nachfolger(0)->get_preis() == 0)
			{
				// Multiple tenders or Garretts with powered front units.
				a ++;
			}
		}

		//Check whether this is a Garrett type vehicle
		if(fahr[0]->get_besch()->get_leistung() == 0 && fahr[0]->get_besch()->get_zuladung() == 0)
		{
			// Possible Garrett
			const uint8 count = fahr[0]->get_besch()->get_nachfolger_count();
			if(count > 0 && fahr[1]->get_besch()->get_leistung() > 0 && fahr[1]->get_besch()->get_nachfolger_count() > 0)
			{
				// Garrett detected
				a ++;
			}
		}

		//Check for a goods train with a brake van
		if((fahr[anz_vehikel - 2]->get_besch()->get_ware()->get_catg_index() > 1)
			&& 	fahr[anz_vehikel - 2]->get_besch()->get_can_be_at_rear() == false)
		{
			b--;
		}

		for(uint8 i = 1; i < anz_vehikel; i++)
		{
			if(fahr[i]->get_besch()->get_leistung() > 0)
			{
				a++;
			}
		}
	}

	fahr[anz_vehikel - 1]->set_letztes(false);

	for( ; a<--b; a++) //increment a and decrement b until they meet each other
	{
		reverse = fahr[a]; //put what's in a into swap space
		fahr[a] = fahr[b]; //put what's in b into a
		fahr[b] = reverse; //put what's in the swap (a) into b
	}

	if(!rev)
	{
		fahr[0]->set_erstes(true);
	}

	fahr[anz_vehikel - 1]->set_letztes(true);

	if(reversed)
	{
		reversed = false;
		for(uint8 i = 0; i < anz_vehikel; i ++)
		{
			fahr[i]->set_reversed(false);
		}
	}
	else
	{
		reversed = true;
		for(uint8 i = 0; i < anz_vehikel; i ++)
		{
			fahr[i]->set_reversed(true);
		}
	}
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
	sint32 besitzer_n = welt->sp2num(besitzer_p);

	if(file->is_saving()) {
		if(  file->get_version()<101000  ) {
			file->wr_obj_id("Convoi");
			// the matching read is in karte_t::laden(loadsave*)...
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

	dummy = anz_vehikel;
	file->rdwr_long(dummy);
	anz_vehikel = (uint8)dummy;

	if(file->get_version()<99014) {
		// was anz_ready
		file->rdwr_long(dummy);
	}

	file->rdwr_long(wait_lock);
	// some versions may produce broken savegames apparently
	if(wait_lock > 60000) {
		dbg->warning("convoi_t::sync_prepre()","Convoi %d: wait lock out of bounds: wait_lock = %d, setting to 60000",self.get_id(), wait_lock);
		wait_lock = 60000;
	}

	bool dummy_bool=false;
	file->rdwr_bool(dummy_bool);
	file->rdwr_long(besitzer_n);
	file->rdwr_long(akt_speed);
	sint32 akt_speed_soll = 0; // Former variable now unused
	file->rdwr_long(akt_speed_soll);
	file->rdwr_long(sp_soll);
	file->rdwr_enum(state);
	file->rdwr_enum(alte_richtung);

	// read the yearly income (which has since then become a 64 bit value)
	// will be recalculated later directly from the history
	if(file->get_version()<=89003) {
		file->rdwr_long(dummy);
	}

	route.rdwr(file);

	if(file->is_loading()) {
		// extend array if requested (only needed for trains)
		if(anz_vehikel > max_vehicle) {
			fahr.resize(max_rail_vehicle, NULL);
		}
		besitzer_p = welt->get_spieler( besitzer_n );

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
		for(unsigned i=0; i<anz_vehikel; i++) {
			file->wr_obj_id( fahr[i]->get_typ() );
			fahr[i]->rdwr_from_convoi(file);
		}
	}
	else {
		bool override_monorail = false;
		is_electric = false;
		for(  uint8 i=0;  i<anz_vehikel;  i++  ) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			vehikel_t *v = 0;

			const bool first = (i==0);
			const bool last = (i==anz_vehikel-1u);
			if(override_monorail) {
				// ignore type for ancient monorails
				v = new monorail_waggon_t(welt, file, first, last);
			}
			else {
				switch(typ) {
					case ding_t::old_automobil:
					case ding_t::automobil: v = new automobil_t(welt, file, first, last);  break;
					case ding_t::old_waggon:
					case ding_t::waggon:    v = new waggon_t(welt, file, first, last);     break;
					case ding_t::old_schiff:
					case ding_t::schiff:    v = new schiff_t(welt, file, first, last);     break;
					case ding_t::old_aircraft:
					case ding_t::aircraft:    v = new aircraft_t(welt, file, first, last);     break;
					case ding_t::old_monorailwaggon:
					case ding_t::monorailwaggon:    v = new monorail_waggon_t(welt, file, first, last);     break;
					case ding_t::maglevwaggon:         v = new maglev_waggon_t(welt, file, first, last);     break;
					case ding_t::narrowgaugewaggon:    v = new narrowgauge_waggon_t(welt, file, first, last);     break;
					default:
						dbg->fatal("convoi_t::convoi_t()","Can't load vehicle type %d", typ);
				}
			}

			// no matching vehicle found?
			if(v->get_besch()==NULL) {
				// will create orphan object, but better than crashing at deletion ...
				dbg->error("convoi_t::convoi_t()","Can't load vehicle and no replacement found!");
				i --;
				anz_vehikel --;
				continue;
			}

			// in very old games, monorail was a railway
			// so we need to convert this
			// freight will be lost, but game will be loadable
			if(i==0  &&  v->get_besch()->get_waytype()==monorail_wt  &&  v->get_typ()==ding_t::waggon) {
				override_monorail = true;
				vehikel_t *v_neu = new monorail_waggon_t( v->get_pos(), v->get_besch(), v->get_besitzer(), NULL );
				v->loesche_fracht();
				delete v;
				v = v_neu;
			}

			if(file->get_version()<99004) {
				dummy_pos.rdwr(file);
			}

			const vehikel_besch_t *info = v->get_besch();

			// Hajo: if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				//sum_leistung += info->get_leistung();
				//sum_gear_und_leistung += (info->get_leistung() * info->get_gear() *welt->get_settings().get_global_power_factor_percent() + 50) / 100;
				//sum_gewicht += info->get_gewicht();
				is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
			}
			else {
				DBG_MESSAGE("convoi_t::rdwr()","no vehikel info!");
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
						sch->reserve(self,ribi_t::keine);
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
				v->clear_flag(ding_t::not_on_map);
			}
			else {
				v->set_flag(ding_t::not_on_map);
			}

			// add to convoi
			fahr[i] = v;
		}
		//sum_gesamtgewicht = sum_gewicht;
	}

	bool has_fpl = (fpl != NULL);
	file->rdwr_bool(has_fpl);
	if(has_fpl) {
		//DBG_MESSAGE("convoi_t::rdwr()","convoi has a schedule, state %s!",state_names[state]);
		const vehikel_t* v = fahr[0];
		if(file->is_loading() && v) {
			fpl = v->erzeuge_neuen_fahrplan();
		}
		// Hajo: hack to load corrupted games -> there is a schedule
		// but no vehicle so we can't determine the exact type of
		// schedule needed. This hack is safe because convois
		// without vehicles get deleted right after loading.
		// Since generic schedules are not allowed, we use a zugfahrplan_t
		if(fpl == 0) {
			fpl = new zugfahrplan_t();
		}

		// Hajo: now read the schedule, we have one for sure here
		fpl->rdwr( file );
	}

	if(file->is_loading()) {
		next_wolke = 0;
		calc_loading();
		set_akt_speed(akt_speed);
		invalidate_vehicle_summary();
	}

	// Hajo: calculate new minimum top speed
	//min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

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
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1) || (j == CONVOI_REFUNDS && file->get_experimental_version() < 8))
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Experimental < 8
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
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1) || (j == CONVOI_REFUNDS && file->get_experimental_version() < 8))
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Experimental < 8
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
		}
	}
//BG: superfluous 102002 check:	else if(file->get_version() <= 102002 || (file->get_version() < 102002 && file->get_experimental_version() < 7))
	else if(file->get_version() <= 102002 || (file->get_experimental_version() < 7 && file->get_experimental_version() != 0))
	{
		// load statistics
		for (int j = 0; j<7; j++) 
		{
			for (int k = MAX_MONTHS-1; k>=0; k--) 
			{
				if(((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1) || (j == CONVOI_REFUNDS && file->get_experimental_version() < 8))
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Experimental < 8
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
				if (file->get_experimental_version() < 2)
				{
					// Versions of Experimental saves with 1 and below
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
				if (file->get_experimental_version() < 7)
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
				if (file->get_experimental_version() < 8)
				{
					// versions of Experimental < 8 did not store refund information.
					if (file->is_loading())
					{
						for (int k = MAX_MONTHS-1; k >= 0; k--)
						{
							financial_history[k][j] = 0;
						}
						if (file->get_experimental_version() == 0 && file->get_version() >= 111001)
						{
							// Convoy maximum speed - not used in Experimental
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
	}

	// the convoy odometer
	if(file->get_version() > 102002)
	{
		if(file->get_experimental_version() < 7)
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

	if(file->get_version() >= 102003 && file->get_experimental_version() >= 7)
	{
		if(file->get_experimental_version() <= 8)
		{
			uint8 old_tiles = uint8(steps_since_last_odometer_increment / VEHICLE_STEPS_PER_TILE);
			file->rdwr_byte(old_tiles);
			steps_since_last_odometer_increment = old_tiles * VEHICLE_STEPS_PER_TILE;
		}
		else if (file->get_experimental_version() > 8 && file->get_experimental_version() < 10)
		{
			double tiles_since_last_odometer_increment = double(steps_since_last_odometer_increment) / VEHICLE_STEPS_PER_TILE;
			file->rdwr_double(tiles_since_last_odometer_increment);
			steps_since_last_odometer_increment = sint64(tiles_since_last_odometer_increment * VEHICLE_STEPS_PER_TILE );
		}
		else if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
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

	if(file->get_version()>=87001) {
		last_stop_pos.rdwr(file);
	}
	else {
		last_stop_pos =
			!route.empty()   ? route.front()      :
			anz_vehikel != 0 ? fahr[0]->get_pos() :
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
				if(file->get_experimental_version() <= 1)
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
				sint64 diff_ticks = welt->get_zeit_ms()>go_on_ticks ? 0 : go_on_ticks-welt->get_zeit_ms();
				file->rdwr_longlong(diff_ticks);
			}
		}
		else 
		{
			if(file->get_experimental_version() <= 1)
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
				go_on_ticks += welt->get_zeit_ms();
			}
		}
	}

	// since 99015, the last stop will be maintained by the vehikels themselves
	if(file->get_version()<99015) {
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->last_stop_pos = last_stop_pos.get_2d();
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
	if(file->get_experimental_version() >= 9)
	{
		file->rdwr_bool(reverse_schedule);
	}
	else if(file->is_loading())
	{
		reverse_schedule = false;
	}
	
	if(file->is_loading())
	{
		highest_axle_load = calc_highest_axle_load();
		longest_min_loading_time = calc_longest_min_loading_time();
		longest_max_loading_time = calc_longest_max_loading_time();
	}

	if(file->get_experimental_version() >= 1)
	{
		file->rdwr_bool(reversed);
		
		//Replacing settings
		// BG, 31-MAR-2010: new replacing code starts with exp version 8:
		bool is_replacing = replace && (file->get_experimental_version() >= 8);
		file->rdwr_bool(is_replacing);

		if(file->get_experimental_version() >= 8)
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

			vector_tpl<const vehikel_besch_t *> *replacing_vehicles;

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
					replacing_vehicles = new vector_tpl<const vehikel_besch_t *>;
					for(uint16 i = 0; i < replacing_vehicles_count; i ++)
					{
						char vehicle_name[256];
						file->rdwr_str(vehicle_name, 256);
						const vehikel_besch_t* besch = vehikelbauer_t::get_info(vehicle_name);
						if(besch == NULL) 
						{
							besch = vehikelbauer_t::get_info(translator::compatibility_name(vehicle_name));
						}
						if(besch == NULL)
						{
							dbg->warning("convoi_t::rdwr()","no vehicle pak for '%s' search for something similar", vehicle_name);
						}
						else
						{
							replacing_vehicles->append(besch);
						}
					}
					// BG, 31-MAR-2010: new replacing code starts with exp version 8.
					// BG, 31-MAR-2010: we must not create 'replace'. I does not work correct. 
					delete replacing_vehicles;
				}
			}
		}
	}
	if(file->get_experimental_version() >= 2)
	{
		if(file->get_experimental_version() <= 9)
		{
			const planquadrat_t* plan = welt->lookup(fahr[0]->last_stop_pos);
			if(plan)
			{
				uint16 last_halt_id =plan->get_halt().get_id();
				sint64 departure_time = departures->get(last_halt_id).departure_time;
				file->rdwr_longlong(departure_time);
				if(file->is_loading())
				{
					departures->clear();
					departure_data_t dep;
					dep.departure_time = departure_time;
					departures->put(last_halt_id, dep);
				}
			}
		}
		else
		{
			uint16 id;
			sint64 departure_time;
			uint32 accumulated_distance;
			if(file->is_saving())
			{
				uint32 count = departures->get_count();
				file->rdwr_long(count);
				FOR(departure_map, const& iter, *departures)
				{
					id = iter.key;
					departure_time = iter.value.departure_time;
					file->rdwr_short(id);
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
			}

			else if(file->is_loading())
			{
				uint32 count = 0;
				file->rdwr_long(count);
				departures->clear();
				for(int i = 0; i < count; i ++)
				{
					file->rdwr_short(id);
					file->rdwr_longlong(departure_time);
					departure_data_t dep;
					dep.departure_time = departure_time;
					if(file->get_version() >= 110007)
					{
						uint8 player_count = 0;
						file->rdwr_byte(player_count);
						for(int i = 0; i < player_count; i ++)
						{
              uint32 accumulated_distance;
							file->rdwr_long(accumulated_distance);
							dep.set_distance(i, accumulated_distance);
						}
					}
					departures->put(id, dep);
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
		departures->clear();
	}

	if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_short(livery_scheme_index);
	}
	else
	{
		livery_scheme_index = 0;
	}

	if(file->get_experimental_version() >= 10)
	{
		if(file->is_saving())
		{
			uint32 count = average_journey_times->get_count();
			file->rdwr_long(count);

			FOR(journey_times_map, const& iter, *average_journey_times)
			{
				id_pair idp = iter.key;
				file->rdwr_short(idp.x);
				file->rdwr_short(idp.y);
				sint16 value = iter.value.count;
				file->rdwr_short(value);
				value = iter.value.total;
				file->rdwr_short(value);
			}
			
		}
		else
		{
			uint32 count = 0;
			file->rdwr_long(count);
			average_journey_times->clear();
			for(uint32 i = 0; i < count; i ++)
			{
				id_pair idp;
				file->rdwr_short(idp.x);
				file->rdwr_short(idp.y);
				
				uint16 count;
				uint16 total;
				file->rdwr_short(count);
				file->rdwr_short(total);				

				average_tpl<uint16> average;
				average.count = count;
				average.total = total;

				average_journey_times->put(idp, average);
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
		}
	}
	
	else
	{
		arrival_time = welt->get_zeit_ms();
	}
	
	if(file->get_version() >= 111001 && file->get_experimental_version() == 0) 
	{
		uint32 dummy = 0;
		file->rdwr_long( dummy ); // Was distance_since_last_stop
		file->rdwr_long( dummy ); // Was sum_speed_limit
	}


	if(file->get_version()>=111002 && file->get_experimental_version() == 0) 
	{
		// Was maxspeed_average_count
		uint32 dummy = 0;
		file->rdwr_long(dummy);
	}
	
	if(file->get_version() >= 111002 && file->get_experimental_version() >= 10)
	{
		file->rdwr_short(last_stop_id);
		v.rdwr(file);
	}

	if(  file->get_version()>=111003  ) {
		file->rdwr_short( next_stop_index );
		file->rdwr_short( next_reservation_index );
	}

	// This must come *after* all the loading/saving.
	if(  file->is_loading()  ) {
		reserve_route();
		recalc_catg_index();
	}

	if (akt_speed < 0)
	{
		set_akt_speed(0);
	}
}


void convoi_t::zeige_info()
{
	if(  in_depot()  ) {
		// Knightly : if ownership matches, we can try to open the depot dialog
		if(  get_besitzer()==welt->get_active_player()  ) {
			grund_t *const ground = welt->lookup( get_home_depot() );
			if(  ground  ) {
				depot_t *const depot = ground->get_depot();
				if(  depot  ) {
					depot->zeige_info();
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
		if(  umgebung_t::verbose_debug  ) {
			dump();
		}
		create_win( new convoi_info_t(self), w_info, magic_convoi_info+self.get_id() );
	}
}


#if 0
void convoi_t::info(cbuffer_t & buf) const
{
	const vehikel_t* v = fahr[0];
	if (v != NULL) {
		char tmp[128];

		buf.printf("\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->get_besch()->get_geschw(), get_running_cost(welt) / 100.0F);

		buf.printf(" %s: %ikW\n", translator::translate("Leistung"), sum_leistung );

		buf.printf(" %s: %i (%i) t\n", translator::translate("Gewicht"), sum_gewicht, sum_gesamtgewicht-sum_gewicht );

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

		size_t const n = warenbauer_t::get_waren_anzahl();
		ALLOCA(uint32, max_loaded_waren, n);
		MEMZERON(max_loaded_waren, n);

		for(  uint32 i = 0;  i != anz_vehikel;  ++i  ) {
			const vehikel_t* v = fahr[i];

			// first add to capacity indicator
			const ware_besch_t* ware_besch = v->get_besch()->get_ware();
			const uint16 menge = v->get_besch()->get_zuladung();
			if(menge>0  &&  ware_besch!=warenbauer_t::nichts) {
				max_loaded_waren[ware_besch->get_index()] += menge;
			}

			// then add the actual load
			FOR(slist_tpl<ware_t>, ware, v->get_fracht()) {
				FOR(vector_tpl<ware_t>, & tmp, total_fracht) {
					// could this be joined with existing freight?

					if(ware.can_merge_with(tmp))
					{
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

			INT_CHECK("simvehikel 876");
		}
		buf.clear();

		// apend info on total capacity
		slist_tpl <ware_t>capacity;
		for (size_t i = 0; i != n; ++i) {
			if(max_loaded_waren[i]>0  &&  i!=warenbauer_t::INDEX_NONE) {
				ware_t ware(warenbauer_t::get_info(i));
				ware.menge = max_loaded_waren[i];
				// append to category?
				slist_tpl<ware_t>::iterator j   = capacity.begin();
				slist_tpl<ware_t>::iterator end = capacity.end();
				while (j != end && j->get_besch()->get_catg_index() < ware.get_besch()->get_catg_index()) ++j;
				if (j != end && j->get_besch()->get_catg_index() == ware.get_besch()->get_catg_index()) {
					j->menge += max_loaded_waren[i];
				} else {
					// not yet there
					capacity.insert(j, ware);
				}
			}
		}

		// show new info
		freight_list_sorter_t::sort_freight(total_fracht, buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, &capacity, "loaded", welt);
	}
}


void convoi_t::open_schedule_window( bool show )
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

	// manipulation of schedule not allowd while:
	// - just starting
	// - a line update is pending
	if(  (state==FAHRPLANEINGABE  ||  line_update_pending.is_bound())  &&  get_besitzer()==welt->get_active_player()  ) {
		if (show) {
			create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		}
		return;
	}

	set_akt_speed(0);	// stop the train ...
	if(state!=INITIAL) {
		state = FAHRPLANEINGABE;
	}
	wait_lock = 25000;
	alte_richtung = fahr[0]->get_fahrtrichtung();

	// Added by : Knightly
	// Purpose  : To keep a copy of the original schedule before opening schedule window
	if (fpl)
	{
		old_fpl = fpl->copy();
	}

	if(  show  ) {
		// Fahrplandialog oeffnen
		create_win( new fahrplan_gui_t(fpl,get_besitzer(),self), w_info, (ptrdiff_t)fpl );
		// TODO: what happens if no client opens the window??
	}
	fpl->eingabe_beginnen();
}


bool convoi_t::pruefe_alle() //"examine all" (Babelfish)
/**
 * Check validity of convoi with respect to vehicle constraints
 */
{
	bool ok = anz_vehikel == 0  ||  fahr[0]->get_besch()->can_follow(NULL);
	unsigned i;

	const vehikel_t* pred = fahr[0];
	for(i = 1; ok && i < anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		ok = pred->get_besch()->can_lead(v->get_besch())  &&
				 v->get_besch()->can_follow(pred->get_besch());
		pred = v;
	}
	if(ok) {
		ok = pred->get_besch()->can_lead(NULL);
	}

	return ok;
}


/**
 * Kontrolliert Be- und Entladen
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::laden() //"load" (Babelfish)
{
	// Calculate average speed and journey time
	// @author: jamespetts
	
	halthandle_t halt = haltestelle_t::get_halt(welt, fpl->get_current_eintrag().pos,besitzer_p);
	id_pair pair(last_stop_id, halt.get_id());
	
	// The calculation of the journey distance does not need to use normalised halt locations for comparison, so
	// a more accurate distance can be used. Query whether the formula from halt_detail.cc should be used here instead
	// (That formula has the effect of finding the distance between the nearest points of two halts).
	const uint32 journey_distance = shortest_distance(fahr[0]->get_pos().get_2d(), fahr[0]->last_stop_pos);

	//last_stop_pos will be set to get_pos().get_2d() in hat_gehalten (called from inside halt->request_loading later
	//so code inside if will be executed once. At arrival time.
	if(journey_distance > 0)
	{
		arrival_time = welt->get_zeit_ms();
		sint64 journey_time = welt->ticks_to_tenths_of_minutes(arrival_time - departures->get(last_stop_id).departure_time);
		if(journey_time <= 0)
		{
			// Necessary to prevent divisions by zero.
			journey_time = 1;
		}
		const sint32 journey_distance_meters = journey_distance * welt->get_settings().get_meters_per_tile();
		const sint32 average_speed = (journey_distance_meters * 3) / ((sint32)journey_time * 5);
		
		// For some odd reason, in some cases, laden() is called when the journey time is
		// excessively low, resulting in perverse average speeds and journey times.
		if(average_speed <= speed_to_kmh(get_min_top_speed()))
		{
			book(average_speed, CONVOI_AVERAGE_SPEED);

			bool reverse = !reverse_schedule;
			uint8 current_stop = fpl->get_aktuell();

			fpl->increment_index(&current_stop, &reverse);
			grund_t* gr = welt->lookup(fpl->eintrag[current_stop].pos);
			if(gr)
			{
				pair.x = gr->get_halt().get_id();
			}
			else
			{
				pair.x = 0;
			}

			const uint8 starting_stop = current_stop;

			id_pair idp;
			idp.y = pair.y;
			idp.x = pair.x;

			do
			{			
				// Book the journey times from all origins served by this convoy,
				// and for which data are available, to this destination.

				bool allow_resetting_line_average = false;

				if(!departures->is_contained(idp.x))
				{
					fpl->increment_index(&current_stop, &reverse);
					grund_t* gr = welt->lookup(fpl->eintrag[current_stop].pos);
					if(gr)
					{
						pair.x = gr->get_halt().get_id();
						idp.x = pair.x;
						continue;
					}
				}

				journey_time = welt->ticks_to_tenths_of_minutes(arrival_time - departures->get(pair.x).departure_time);
				
				average_tpl<uint16> *average = average_journey_times->access(idp);
				if(!average)
				{
					average_tpl<uint16> average;
					average.add(journey_time);
					average_journey_times->put(idp, average);
				}
				else
				{
					// Check for anomalies as might be created by exotic timetable arrangements 
					// (e.g. - D shaped timetables or multiple branching and re-joining)
					// and apply a quick and somewhat dirty workaround.

					
					if(journey_time > average->get_average() * 2 || journey_time < average->get_average() / 2)
					{
						// Anomaly detected - check to see whether this can be caused by odd timetabling.
						// If not, then this must be caused by traffic fluctuations, and should remain.
						const uint8 fpl_count = fpl->get_count();
						uint32 this_stop_count = 0;
						for(uint8 i = 0; i < fpl_count; i ++)
						{
							const grund_t* gr_2 = welt->lookup(fpl->eintrag[i].pos);
							if(gr_2 && gr_2->get_halt().get_id() == idp.x)
							{
								this_stop_count ++;
							}
						}

						if(this_stop_count >= 2)
						{
							// More than one entry - might be a timetable issue.
							if(journey_time > average->get_average() * 2)
							{
								dbg->message("void convoi_t::laden()", "Possible timetable anomaly detected. Skipping inserting journey time (convoy).");
							}
							else if(journey_time < average->get_average() / 2)
							{
								dbg->message("void convoi_t::laden()", "Possible timetable anomaly detected. Resetting average journey times (convoy).");	
								average->reset();
								allow_resetting_line_average = true;
								goto write_basic;
							}
						}
						else
						{
							goto write_basic;
						}

					}
					else
					{
write_basic:
						average_journey_times->access(pair)->add_check_overflow_16(journey_time);
					}
				}
				if(line.is_bound())
				{
					average_tpl<uint16> *average = get_average_journey_times()->access(idp);
					if(!average)
					{
						average_tpl<uint16> average;
						average.add(journey_time);
						get_average_journey_times()->put(idp, average);
					}
					else
					{
						// Check for anomalies as might be created by exotic timetable arrangements 
						// (e.g. - D shaped timetables or multiple branching and re-joining)
						// and apply a quick and somewhat dirty workaround.
	
						if(journey_time > average->get_average() * 2 || journey_time < average->get_average() / 2)
						{
							// Anomaly detected - check to see whether this can be caused by odd timetabling.
							// If not, then this must be caused by traffic fluctuations, and should remain.
							const uint8 fpl_count = fpl->get_count();
							uint32 this_stop_count = 0;
							for(uint8 i = 0; i < fpl_count; i ++)
							{
								const grund_t* gr_2 = welt->lookup(fpl->eintrag[i].pos);
								if(gr_2 && gr_2->get_halt().get_id() == idp.x)
								{
									this_stop_count ++;
								}
							}

							if(this_stop_count >= 2)
							{
								// More than one entry - might be a timetable issue.
								if(journey_time > average->get_average() * 2)
								{
									dbg->message("void convoi_t::laden()", "Possible timetable anomaly detected. Skipping inserting journey time (line).");
								}
								else if(allow_resetting_line_average && journey_time < average->get_average() / 2)
								{
									dbg->message("void convoi_t::laden()", "Possible timetable anomaly detected. Resetting average journey times (line).");	
									average->reset();
									goto write_basic_line;
								}
							}
							else
							{
								goto write_basic_line;
							}

						}
						else
						{
write_basic_line:

							get_average_journey_times()->access(idp)->add_check_overflow_16(journey_time);
						}
					}
				}

				fpl->increment_index(&current_stop, &reverse);
				grund_t* gr = welt->lookup(fpl->eintrag[current_stop].pos);
				if(gr)
				{
					pair.x = gr->get_halt().get_id();
				}
				else
				{
					pair.x = 0;
				}
				idp.x = pair.x;
			}
			while(starting_stop != current_stop && idp.x != idp.y);
		}
	}

	bool clear_departures = false;
	minivec_tpl<uint8> departure_entries_to_remove(fpl->get_count());
	
	if(!is_circular_route())
	{
		uint8 stop = fpl->get_aktuell();
		uint8 previous_stop = stop;

		bool rev = reverse_schedule;
		bool anti_rev = !rev;
		halthandle_t stop_hh;
		halthandle_t previous_stop_hh;

		for(int i = 0; i < fpl->get_count(); i ++)
		{
			fpl->increment_index(&previous_stop, &anti_rev);
			fpl->increment_index(&stop, &rev);
			if(i == 0)
			{
				clear_departures = reverse_schedule != rev;
				if(clear_departures)
				{
					break;
				}
			}
			const grund_t* gr_this = welt->lookup(fpl->eintrag[stop].pos);
			const grund_t* gr_previous = welt->lookup(fpl->eintrag[previous_stop].pos);
			if(gr_previous && gr_this)
			{
				stop_hh = gr_this->get_halt();
				previous_stop_hh = gr_previous->get_halt();
			}
			else
			{
				// Something has gone wrong.
				dbg->error("void convoi_t::laden() ", "Cannot lookup halt");
				continue;
			}

			if(previous_stop_hh.get_id() == stop_hh.get_id())
			{
				departure_entries_to_remove.append(stop_hh.get_id());
			}
			else
			{
				clear_departures = false;
				break;
			}
			// If we reach the end of the list without breaking,
			// we can simply clear the whole list.
			clear_departures = true;
		}
	}
		
	// Recalculate comfort
	const uint8 comfort = get_comfort();
	if(comfort)
	{
		book(get_comfort(), CONVOI_COMFORT);
	}

	FOR(departure_map, & iter, *departures)
	{
		// Accumulate distance 
		const grund_t* gr = welt->lookup(fpl->get_current_eintrag().pos);
		if(gr && is_circular_route() && iter.key == gr->get_halt().get_id())
		{
			// If this is a circular route, reset distances from this halt,
			// as the list of departures is never reset for a circular route,
			// so, without this resetting, the distance would accumulate for
			// ever!
			iter.value.reset_distances();
		}
		else
		{
			iter.value.add_overall_distance(journey_distance);
		}
	}

	if(state == FAHRPLANEINGABE) //"ENTER SCHEDULE" (Google)
	{ 
		return;
	}

	if (halt.is_bound()) 
	{
		const koord k = fpl->get_current_eintrag().pos.get_2d(); //"eintrag" = "entry" (Google)
		//const spieler_t* owner = halt->get_besitzer(); //"get owner" (Google)
		const grund_t* gr = welt->lookup(fpl->get_current_eintrag().pos);
		const weg_t *w = gr ? gr->get_weg(fpl->get_waytype()) : NULL;
		bool tram_stop_public = false;
		if(fpl->get_waytype() == tram_wt)
		{
			const grund_t* gr = welt->lookup(fpl->get_current_eintrag().pos);
			const weg_t *street = gr ? gr->get_weg(road_wt) : NULL;
			if(street && (street->get_besitzer() == get_besitzer() || street->get_besitzer() == NULL || street->get_besitzer()->allows_access_to(get_besitzer()->get_player_nr())))
			{
				tram_stop_public = true;
			}
		}
		const spieler_t *sp = w ? w->get_besitzer() : NULL;
		if(halt->check_access(get_besitzer()) || (w && sp == NULL) || (w && sp->allows_access_to(get_besitzer()->get_player_nr())) || tram_stop_public)
		{
			// loading/unloading ...
			// NOTE: Revenue is calculated here.
			halt->request_loading(self);
		}
	}

	if(clear_departures)
	{
		// If this is the end of the schedule, the departure times need to be reset
		// so as to avoid over-writing good journey time data with data comprising 
		// the time between the last departure on the previous run to the current
		// stop, which will give excessively long times.
		departures->clear();
	}
	else
	{
		// In many cases, simply clearing the list does not help, such as a 
		// Y shaped route. In such cases, halts must be pruned
		// selectively. 

		ITERATE(departure_entries_to_remove, i)
		{
			departures->remove(departure_entries_to_remove[i]);
		}
	}

	if (wait_lock == 0 ) {
		wait_lock = WTT_LOADING;
	}

	if(line.is_bound())
	{
		line->calc_is_alternating_circular_route();
	}
}

sint64 convoi_t::calc_revenue(const ware_t& ware, array_tpl<sint64> & apportioned_revenues)
{
	sint64 overall_average_speed;
	
	if(!line.is_bound())
	{
		// No line - must use convoy
		if(financial_history[1][CONVOI_AVERAGE_SPEED] == 0)
		{
			overall_average_speed = financial_history[0][CONVOI_AVERAGE_SPEED];
		}
		else
		{	
			overall_average_speed = financial_history[1][CONVOI_AVERAGE_SPEED];
		}
	}

	else
	{
		if(line->get_finance_history(1, LINE_AVERAGE_SPEED) == 0)
		{
			overall_average_speed = line->get_finance_history(0, LINE_AVERAGE_SPEED);
		}
		else
		{	
			overall_average_speed = line->get_finance_history(1, LINE_AVERAGE_SPEED);
		}
	}

	if(overall_average_speed == 0)
	{
		overall_average_speed = 1;
	}

	// Cannot not charge for journey if the journey distance is more than a certain proportion of the straight line distance.
	// This eliminates the possibility of cheating by building circuitous routes, or the need to prevent that by always using
	// the straight line distance, which makes the game difficult and unrealistic. 
	// If the origin has been deleted since the packet departed, then the best that we can do is guess by
	// trebling the distance to the last stop.
	const uint32 max_distance = ware.get_last_transfer().is_bound() ? 
		shortest_distance(ware.get_last_transfer()->get_basis_pos(), fahr[0]->get_pos().get_2d()) * 2 :
		3 * shortest_distance(last_stop_pos.get_2d(), fahr[0]->get_pos().get_2d());
	const departure_data_t dep = departures->get(ware.get_last_transfer().get_id());
	const uint32 distance = dep.get_overall_distance() > 0 ? dep.get_overall_distance() : max_distance / 2;
	const uint32 revenue_distance = distance < max_distance ? distance : max_distance;
	
	uint16 journey_minutes = 0;
	if(ware.get_last_transfer().is_bound())
	{
		const planquadrat_t* plan = welt->lookup(fahr[0]->get_pos().get_2d());
		journey_minutes = plan ? (get_average_journey_times()->get(id_pair(ware.get_last_transfer().get_id(), plan->get_halt().get_id())).get_average()) / 10 : 0;
	}

	sint64 average_speed;
	if(journey_minutes == 0)
	{
		// Fallback to the overall average speed if there are no data for point-to-point timings.
		// 100/1667 = 60min/hr / 1000 m/km
		journey_minutes = (((distance * 100) / overall_average_speed) * welt->get_settings().get_meters_per_tile()) / 1667;
		average_speed = overall_average_speed;
	}
	else
	{
		const sint32 journey_distance_meters = distance * welt->get_settings().get_meters_per_tile();
		average_speed = (journey_distance_meters * 3) / (journey_minutes * 50);
		if(average_speed == 0)
		{
			average_speed = 1;
		}
	}

	if(average_speed > speed_to_kmh(get_min_top_speed()))
	{
		dbg->warning("sint64 convoi_t::calc_revenue", "Average speed (%i) for %s exceeded maximum speed (%i); falling back to overall average", average_speed, get_name(), speed_to_kmh(get_min_top_speed()));
		journey_minutes = (((distance * 100) / overall_average_speed) * welt->get_settings().get_meters_per_tile()) / 1667;
		average_speed = overall_average_speed;
	}
	
	const ware_besch_t* goods = ware.get_besch();
	const sint64 starting_distance = ware.get_origin().is_bound() ? (sint64)shortest_distance(ware.get_origin()->get_basis_pos(), fahr[0]->get_pos().get_2d()) - (sint64)revenue_distance : 0ll;
	const sint64 base_fare = goods->get_fare(revenue_distance, (starting_distance > 0 ? (uint32)starting_distance : 0));
	const sint64 min_fare = (base_fare * 1000ll) / 4ll;
	const sint64 max_fare = base_fare * 4000ll;
	const uint16 speed_bonus_rating = calc_adjusted_speed_bonus(goods->get_speed_bonus(), distance);
	const sint64 ref_speed = welt->get_average_speed(fahr[0]->get_besch()->get_waytype());
	const sint64 speed_base = (100ll * average_speed) / ref_speed - 100ll;
	const sint64 base_bonus = (base_fare * (1000ll + speed_base * speed_bonus_rating));
	const sint64 min_revenue = max(min_fare, base_bonus);
	const sint64 max_revenue = min(max_fare, min_revenue);
	const sint64 revenue = max_revenue * (sint64)ware.menge;
	sint64 final_revenue = revenue;

	const uint16 happy_percentage = ware.get_last_transfer().is_bound() ? ware.get_last_transfer()->get_unhappy_percentage(1) : 100;
	if(speed_bonus_rating > 0 && happy_percentage > 0)
	{
		// Reduce revenue if the origin stop is crowded, if speed is important for the cargo.
		sint64 tmp = ((sint64)speed_bonus_rating * revenue) / 100ll;
		tmp *= ((sint64)happy_percentage * 2) / 100ll;
		final_revenue -= tmp;
	}
	
	if(final_revenue && ware.is_passenger())
	{
		//Passengers care about their comfort
		const uint8 tolerable_comfort = calc_tolerable_comfort(journey_minutes);

		uint8 comfort = 100ll;
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
		}
		else
		{
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
		
		// Comfort matters more the longer the journey.
		// @author: jamespetts, March 2010
		sint64 comfort_modifier;
		if(journey_minutes <=welt->get_settings().get_tolerable_comfort_short_minutes())
		{
			comfort_modifier = 20ll;
		}
		else if(journey_minutes >=welt->get_settings().get_tolerable_comfort_median_long_minutes())
		{
			comfort_modifier = 100ll;
		}
		else
		{
			const uint16 differential = journey_minutes - welt->get_settings().get_tolerable_comfort_short_minutes();
			const uint16 max_differential =welt->get_settings().get_tolerable_comfort_median_long_minutes() -welt->get_settings().get_tolerable_comfort_short_minutes();
			const sint64 proportion = differential * 100 / max_differential;
			comfort_modifier = (80ll * proportion / 100ll) + 20ll;
		}

		if(comfort > tolerable_comfort)
		{
			// Apply luxury bonus
			const uint8 max_differential = welt->get_settings().get_max_luxury_bonus_differential();
			const uint8 differential = comfort - tolerable_comfort;
			const sint64 multiplier = (welt->get_settings().get_max_luxury_bonus_percent() * comfort_modifier) / 100ll;
			if(differential >= max_differential)
			{
				final_revenue += (revenue * multiplier) / 100ll;
			}
			else
			{
				const sint64 proportion = (differential * 100ll) / max_differential;
				final_revenue += (revenue * (sint64)(multiplier * proportion)) / 10000ll;
			}
		}
		else if(comfort < tolerable_comfort)
		{
			// Apply discomfort penalty
			const uint8 max_differential = welt->get_settings().get_max_discomfort_penalty_differential();
			const uint8 differential = tolerable_comfort - comfort;
			sint64 multiplier = (welt->get_settings().get_max_discomfort_penalty_percent() * comfort_modifier) / 100ll;
			multiplier = multiplier < 95ll ? multiplier : 95ll;
			if(differential >= max_differential)
			{
				final_revenue -= (revenue * multiplier) / 100ll;
			}
			else
			{
				const sint64 proportion = (differential * 100ll) / max_differential;
				final_revenue -= (revenue * (multiplier * proportion)) / 10000ll;
			}
		}
		
		// Do nothing if comfort == tolerable_comfort			
	}

	// Add catering or TPO revenue
	const uint8 catering_level = get_catering_level(ware.get_besch()->get_catg_index());
	if(catering_level > 0)
	{
		if(ware.is_mail())
		{
			// Mail
			if(journey_minutes >=welt->get_settings().get_tpo_min_minutes())
			{
				final_revenue += (sint64)(welt->get_settings().get_tpo_revenue() * 1000 * ware.menge);
			}
		}
		else if(ware.is_passenger())
		{				
			// Passengers
			sint64 proportion = 0;
			// Knightly : Reorganised the switch cases to get rid of goto statements
			switch(catering_level)
			{
			default:
			case 5:
				if(journey_minutes >= welt->get_settings().get_catering_level4_minutes())
				{
					if(journey_minutes > welt->get_settings().get_catering_level5_minutes())
					{
						final_revenue += (welt->get_settings().get_catering_level5_max_revenue() * 1000 * ware.menge);
						break;
					}
					
					proportion = (sint64)((journey_minutes - welt->get_settings().get_catering_level4_minutes()) * 1000) / (welt->get_settings().get_catering_level5_minutes() - welt->get_settings().get_catering_level4_minutes());
					final_revenue += (max(proportion * (sint64)(welt->get_settings().get_catering_level5_max_revenue()), ((sint64)(welt->get_settings().get_catering_level4_max_revenue() * 1000) + 4000)) * ware.menge);
					break;
				}

			case 4:
				if(journey_minutes >= welt->get_settings().get_catering_level3_minutes())
				{
					if(journey_minutes > welt->get_settings().get_catering_level4_minutes())
					{
						final_revenue += (sint64)(welt->get_settings().get_catering_level4_max_revenue() * 1000 * ware.menge);
						break;
					}
					
					proportion = ((journey_minutes -welt->get_settings().get_catering_level3_minutes()) * 1000) / (welt->get_settings().get_catering_level4_minutes() - welt->get_settings().get_catering_level3_minutes());
					final_revenue += (max(proportion * (sint64)(welt->get_settings().get_catering_level4_max_revenue()), ((sint64)(welt->get_settings().get_catering_level3_max_revenue() * 1000) + 4000)) * ware.menge);
					break;
				}

			case 3:
				if(journey_minutes >= welt->get_settings().get_catering_level2_minutes())
				{
					if(journey_minutes > welt->get_settings().get_catering_level3_minutes())
					{
						final_revenue += (sint64)(welt->get_settings().get_catering_level3_max_revenue() * 1000 * ware.menge);
						break;
					}
					
					proportion = ((journey_minutes - welt->get_settings().get_catering_level2_minutes()) * 1000) / (welt->get_settings().get_catering_level3_minutes() - welt->get_settings().get_catering_level2_minutes());
					final_revenue += (max(proportion * (sint64)(welt->get_settings().get_catering_level3_max_revenue()), ((sint64)(welt->get_settings().get_catering_level2_max_revenue() * 1000) + 4000)) * ware.menge);
					break;
				}

			case 2:
				if(journey_minutes >= welt->get_settings().get_catering_level1_minutes())
				{
					if(journey_minutes > welt->get_settings().get_catering_level2_minutes())
					{
						final_revenue += (sint64)(welt->get_settings().get_catering_level2_max_revenue() * 1000 * ware.menge);
						break;
					}
					
					proportion = ((journey_minutes - welt->get_settings().get_catering_level1_minutes()) * 1000) / (welt->get_settings().get_catering_level2_minutes() - welt->get_settings().get_catering_level1_minutes());
					final_revenue += (max(proportion * (sint64)(welt->get_settings().get_catering_level2_max_revenue()), ((sint64)(welt->get_settings().get_catering_level1_max_revenue() * 1000) + 4000)) * ware.menge);
					break;
				}

			case 1:
				if(journey_minutes < welt->get_settings().get_catering_min_minutes())
				{
					break;
				}
				if(journey_minutes > welt->get_settings().get_catering_level1_minutes())
				{
					final_revenue += (sint64)(welt->get_settings().get_catering_level1_max_revenue() * 1000 * ware.menge);
					break;
				}

				proportion = ((journey_minutes - welt->get_settings().get_catering_min_minutes()) * 1000) / (welt->get_settings().get_catering_level1_minutes() - welt->get_settings().get_catering_min_minutes());
				final_revenue += (sint64)(proportion * (welt->get_settings().get_catering_level1_max_revenue()) * ware.menge);
				break;
			};
		}
	}

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
		if(besitzer_p->get_player_nr() == i)
		{
			// Never apportion revenue to the convoy-owning player
			continue;
		}
		// Now, if the player's tracks were actually used, apportion revenue
		uint32 player_way_distance = dep.get_way_distance(i);
		if(player_way_distance > 0)
		{
			// We allocate even for players who may not exist; we'll check before paying them.
			apportioned_revenues[i] += (final_revenue * player_way_distance) / total_way_distance;
		}
	}
	
	return final_revenue;
}


uint8 convoi_t::calc_tolerable_comfort(uint16 journey_minutes, karte_t* w) 
{
	const uint16 comfort_short_minutes = w->get_settings().get_tolerable_comfort_short_minutes();
	const uint8 comfort_short = w->get_settings().get_tolerable_comfort_short();
	if(journey_minutes <= comfort_short_minutes)
	{
		return comfort_short;
	}

	const uint16 comfort_median_short_minutes = w->get_settings().get_tolerable_comfort_median_short_minutes();
	const uint8 comfort_median_short = w->get_settings().get_tolerable_comfort_median_short();
	if(journey_minutes == comfort_median_short_minutes)
	{
		return comfort_median_short;
	}
	if(journey_minutes < comfort_median_short_minutes)
	{
		const uint32 percentage = ((journey_minutes - comfort_short_minutes) * 1000) / (comfort_median_short_minutes - comfort_short_minutes);
		return ((percentage * (comfort_median_short - comfort_short)) / 1000) + comfort_short;
	}

	const uint16 comfort_median_median_minutes = w->get_settings().get_tolerable_comfort_median_median_minutes();
	const uint8 comfort_median_median = w->get_settings().get_tolerable_comfort_median_median();
	if(journey_minutes == comfort_median_median_minutes)
	{
		return comfort_median_median;
	}
	if(journey_minutes < comfort_median_median_minutes)
	{
		const uint32 percentage = ((journey_minutes - comfort_median_short_minutes) * 1000) / (comfort_median_median_minutes - comfort_median_short_minutes);
		return ((percentage * (comfort_median_median - comfort_median_short)) / 1000) + comfort_median_short;
	}

	const uint16 comfort_median_long_minutes = w->get_settings().get_tolerable_comfort_median_long_minutes();
	const uint8 comfort_median_long = w->get_settings().get_tolerable_comfort_median_long();
	if(journey_minutes == comfort_median_long_minutes)
	{
		return comfort_median_long;
	}
	if(journey_minutes < comfort_median_long_minutes)
	{
		const uint32 percentage = ((journey_minutes - comfort_median_median_minutes) * 1000) / (comfort_median_long_minutes - comfort_median_median_minutes);
		return ((percentage * (comfort_median_long - comfort_median_median)) / 1000) + comfort_median_median;
	}

	const uint16 comfort_long_minutes = w->get_settings().get_tolerable_comfort_long_minutes();
	const uint8 comfort_long = w->get_settings().get_tolerable_comfort_long();
	if(journey_minutes >= comfort_long_minutes)
	{
		return comfort_long;
	}

	const uint32 percentage = ((journey_minutes - comfort_median_long_minutes) * 1000) / (comfort_long_minutes - comfort_median_long_minutes);
	return ((percentage * (comfort_long - comfort_median_long)) / 1000) + comfort_median_long;
}

// Returns SECONDS not minutes
uint32 convoi_t::calc_max_tolerable_journey_time(uint16 comfort, karte_t* w)
{
	const uint16 comfort_short_minutes = w->get_settings().get_tolerable_comfort_short_minutes();
	const uint8 comfort_short = w->get_settings().get_tolerable_comfort_short();
	if(comfort < comfort_short)
	{
		const uint16 percentage = (comfort_short * 100) / comfort;
		return 60 * (comfort_short_minutes * 100) / percentage;
	}
	if(comfort == comfort_short)
	{
		return 60 * comfort_short_minutes;
	}

	const uint16 comfort_median_short_minutes = w->get_settings().get_tolerable_comfort_median_short_minutes();
	const uint8 comfort_median_short = w->get_settings().get_tolerable_comfort_median_short();
	if(comfort < comfort_median_short)
	{
		const uint16 percentage = ((comfort_median_short - comfort_short) * 100) / (comfort - comfort_short);
		return 60 * (((comfort_median_short_minutes - comfort_short_minutes) * 100) / percentage) + comfort_short_minutes;
	}
	if(comfort == comfort_median_short)
	{
		return 60 * comfort_median_short_minutes;
	}

	const uint16 comfort_median_median_minutes = w->get_settings().get_tolerable_comfort_median_median_minutes();
	const uint8 comfort_median_median = w->get_settings().get_tolerable_comfort_median_median();
	if(comfort < comfort_median_median)
	{
		const uint16 percentage = ((comfort_median_median - comfort_median_short) * 100) / (comfort - comfort_median_short);
		return 60 * ( (((comfort_median_median_minutes - comfort_median_short_minutes) * 100) / percentage) + comfort_median_short_minutes );
	}
	if(comfort == comfort_median_median)
	{
		return 60 * comfort_median_median_minutes;
	}

	const uint16 comfort_median_long_minutes = w->get_settings().get_tolerable_comfort_median_long_minutes();
	const uint8 comfort_median_long = w->get_settings().get_tolerable_comfort_median_long();
	if(comfort < comfort_median_long)
	{
		const uint16 percentage = ((comfort_median_long - comfort_median_median) * 100) / (comfort - comfort_median_median);
		return 60 * ( (((comfort_median_long_minutes - comfort_median_median_minutes) * 100) / percentage) + comfort_median_median_minutes );
	}
	if(comfort == comfort_median_long)
	{
		return 60 * comfort_median_long_minutes;
	}

	const uint16 comfort_long_minutes = w->get_settings().get_tolerable_comfort_long_minutes();
	const uint8 comfort_long = w->get_settings().get_tolerable_comfort_long();
	if(comfort < comfort_long)
	{
		const uint16 percentage = ((comfort_long - comfort_median_long) * 100) / (comfort - comfort_median_long);
		return 60 * ( (((comfort_long_minutes - comfort_median_long_minutes) * 100) / percentage) + comfort_median_long_minutes );
	}
	if(comfort == comfort_long)
	{
		return 60 * comfort_long_minutes;
	}
	else
	{
		const uint16 percentage =  (comfort * 100) / comfort_long;
		return 60 * (comfort_long_minutes * percentage) / 100;
	}
}

uint16 convoi_t::calc_adjusted_speed_bonus(uint16 base_bonus, uint32 distance, karte_t* w)
{
	const uint32 min_distance = w != NULL ? w->get_settings().get_min_bonus_max_distance() : 10;
	if(distance <= min_distance)
	{
		return 0;
	}

	const uint16 global_multiplier = welt->get_settings().get_speed_bonus_multiplier_percent();
	const uint16 max_distance = w != NULL ? w->get_settings().get_max_bonus_min_distance() : 16;
	const uint16 multiplier = w != NULL ? w->get_settings().get_max_bonus_multiplier_percent() : 30;
	
	if(distance >= max_distance)
	{
		return base_bonus * multiplier * global_multiplier / 10000;
	}

	const uint16 median_distance = w != NULL ? w->get_settings().get_median_bonus_distance() : 128;
	if(median_distance == 0)
	{
		// There is no median, so scale evenly.
		const uint32 percentage = ((distance - min_distance) * 100) / (max_distance - min_distance);
		const uint32 return_figure = ((base_bonus * multiplier) * percentage) / 10000;
		return (uint16)return_figure;
	}

	// There is a median, so scale differently each side of the median.

	if(distance == median_distance)
	{
		return (base_bonus * global_multiplier) / 100;
	}

	if(distance < median_distance)
	{
		const uint32 percentage = ((distance - min_distance) * 100) / (median_distance - min_distance);
		const uint32 return_figure =  (base_bonus * percentage * global_multiplier) / 10000;
		return (uint16)return_figure;
	}

	// If the program gets here, it must be true that:
	// distance > median_distance

	const uint32 percentage = ((distance - median_distance) * 100) / (max_distance - min_distance);
	uint32 intermediate_bonus = ((base_bonus * multiplier) / 100) - base_bonus;
	intermediate_bonus *= percentage;
	intermediate_bonus /= 100;
	return ((intermediate_bonus + base_bonus) * global_multiplier) / 100;
}

/**
 * convoi an haltestelle anhalten
 * "Convoi stop at stop" (Google translations)
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(halthandle_t halt)
{
	sint64 accumulated_revenue = 0;

	// This holds the revenues as apportioned to different players by track
	// Initialize it to the correct size and blank out all entries
	// It will be added to by ::entladen for each vehicle
	array_tpl<sint64> apportioned_revenues (MAX_PLAYER_COUNT, 0);

	grund_t *gr = welt->lookup(fahr[0]->get_pos());

	// now find out station length
	int station_length=0;
	if(  gr->ist_wasser()  ) {
		// harbour has any size
		station_length = 24*16;
	}
	else
	{
		// calculate real station length
		koord zv = koord( ribi_t::rueckwaerts(fahr[0]->get_fahrtrichtung()) );
		koord3d pos = fahr[0]->get_pos();
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

	// Save the old stop position; don't unload again if we don't move.
	const koord old_last_stop_pos = fahr[0]->last_stop_pos;

	uint16 changed_loading_level = 0;
	int number_loadable_vehicles = anz_vehikel; // Will be shortened for short platform

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
	for(int i = 0; i < anz_vehikel ; i++)
	{
		vehikel_t* v = fahr[i];

		convoy_length += v->get_besch()->get_length();
		if(convoy_length > station_length)
		{
			number_loadable_vehicles = i;
			break;
		}

		// Reset last_stop_pos for all vehicles.
		koord3d pos = v->get_pos();
		if(haltestelle_t::get_halt(welt, pos, v->get_besitzer()).is_bound())
		{
			v->last_stop_pos = pos.get_2d();
		}
		else
		{
			v->last_stop_pos = halt->get_basis_pos();
		}
		// hat_behalten can be called when the convoy hasn't moved... at all.
		// We should avoid the unloading code when this happens (for speed).
		if(old_last_stop_pos != fahr[0]->get_pos().get_2d()) {
			//Unload
			sint64 revenue_from_unloading = 0;
			uint16 amount_unloaded = v->entladen(halt, revenue_from_unloading, apportioned_revenues);
			changed_loading_level += amount_unloaded;
			// James has done something extremely screwy with revenue.  We have to divide it by 3000. FIXME.
			sint64 modified_revenue_from_unloading = (revenue_from_unloading + 1500ll) / 3000ll;
			if (amount_unloaded && modified_revenue_from_unloading == 0) {
				// if we unloaded something, provide some minimum revenue.  But not if we unloaded nothing.
				modified_revenue_from_unloading = 1;
			}
			// This call needs to be here, per-vehicle, in order to record different freight types properly.
			besitzer_p->book_revenue( modified_revenue_from_unloading, fahr[0]->get_pos().get_2d(), get_schedule()->get_waytype(), v->get_fracht_typ()->get_index() );
			// The finance code will add up the on-screen messages
			// But add up the total for the port and station use charges
			accumulated_revenue += modified_revenue_from_unloading;
			book(modified_revenue_from_unloading, CONVOI_PROFIT);
			book(modified_revenue_from_unloading, CONVOI_REVENUE);
		}
	}
	if (no_load) {
		for(int i = 0; i < number_loadable_vehicles ; i++)
		{
			vehikel_t* v = fahr[i];
			// do not load -- but call beladen() to recalculate vehikel weight
			v->beladen(halthandle_t());
		}
	}
	else // not "no_load"
	{
		// Load vehicles to their regular level.
		for(int i = 0; i < number_loadable_vehicles ; i++)
		{
			const bool overcrowd = false;
			vehikel_t* v = fahr[i];
			changed_loading_level += v->beladen(halt, overcrowd);
		}
		// Finally, load vehicles to their overcrowded level.
		for(int i = 0; i < number_loadable_vehicles ; i++)
		{
			const bool overcrowd = true;
			vehikel_t* v = fahr[i];
			changed_loading_level += v->beladen(halt, overcrowd);
		}
	}

	freight_info_resort |= changed_loading_level;
	if(  changed_loading_level  ) {
		halt->recalc_status();
	}

	// any loading went on?
	calc_loading();
	loading_limit = fpl->get_current_eintrag().ladegrad; // ladegrad = max. load.
	highest_axle_load = calc_highest_axle_load(); // Bernd Gabriel, Mar 10, 2010: was missing.
	if(old_last_stop_pos != fahr[0]->get_pos().get_2d())
	{
		// Only calculate the loading time once, on arriving at the stop:
		// otherwise, the change in the vehicle's load will not be calculated
		// correctly.
		calc_current_loading_time(changed_loading_level);
	}

	if(accumulated_revenue) 
	{
		jahresgewinn += accumulated_revenue; //"annual profit" (Babelfish)

		// Check the apportionment of revenue.
		// The proportion paid to other players is
		// not deducted from revenue, but rather
		// added as a "WAY_TOLL" cost.

		for(int i = 0; i < MAX_PLAYER_COUNT; i ++)
		{
			if(i == besitzer_p->get_player_nr())
			{
				continue;
			}
			spieler_t* sp = welt->get_spieler(i);
			// Only pay tolls to players who actually exist
			if(sp && apportioned_revenues[i])
			{
				// This is the screwy factor of 3000 -- fix this sometime
				sint64 modified_apportioned_revenue = (apportioned_revenues[i] + 1500ll) / 3000ll;
				modified_apportioned_revenue *= welt->get_settings().get_way_toll_revenue_percentage();
				modified_apportioned_revenue /= 100;
				if(welt->get_active_player() == sp)
				{
					sp->add_money_message(modified_apportioned_revenue, fahr[0]->get_pos().get_2d());
				}
				sp->book_toll_received(modified_apportioned_revenue, get_schedule()->get_waytype() );
				besitzer_p->book_toll_paid(-modified_apportioned_revenue, get_schedule()->get_waytype() );
				book(-modified_apportioned_revenue, CONVOI_PROFIT);
			}
		}

		//  Apportion revenue for air/sea ports
		if(halt.is_bound() && halt->get_besitzer() != besitzer_p)
		{
			sint64 port_charge = 0;
			if(gr->ist_wasser())
			{
				// This must be a sea port.
				port_charge = (accumulated_revenue * welt->get_settings().get_seaport_toll_revenue_percentage()) / 100;
			}
			else if(fahr[0]->get_besch()->get_waytype() == air_wt)
			{
				// This is an aircraft - this must be an airport.
				port_charge = (accumulated_revenue * welt->get_settings().get_airport_toll_revenue_percentage()) / 100;
			}
			if (port_charge != 0) {
				if(welt->get_active_player() == halt->get_besitzer())
				{
					halt->get_besitzer()->add_money_message(port_charge, fahr[0]->get_pos().get_2d());
				}
				halt->get_besitzer()->book_toll_received(port_charge, get_schedule()->get_waytype() );
				besitzer_p->book_toll_paid(-port_charge, get_schedule()->get_waytype() );
				book(-port_charge, CONVOI_PROFIT);
			}
		}
	}

	if(state == REVERSING)
	{
		return;
	}

	const uint32 reversing_time = fpl->get_current_eintrag().reverse ? calc_reverse_delay() : 0;
	if(go_on_ticks == WAIT_INFINITE) 
	{
		const sint64 departure_time = (arrival_time + current_loading_time) - reversing_time;
		if(!loading_limit || loading_level >= loading_limit) 
		{
			go_on_ticks = max(departure_time, arrival_time);
		} 
		else 
		{
			sint64 go_on_ticks_spacing = WAIT_INFINITE;
			if (line.is_bound() && fpl->get_spacing() && line->count_convoys()) 
			{
				// Spacing cnv/month
				uint32 spacing = welt->ticks_per_world_month/fpl->get_spacing();
				uint32 spacing_shift = fpl->get_current_eintrag().spacing_shift * welt->ticks_per_world_month / welt->get_settings().get_spacing_shift_divisor();
				sint64 wait_from_ticks = ((welt->get_zeit_ms() - spacing_shift) / spacing) * spacing + spacing_shift; // remember, it is integer division
				int queue_pos = halt.is_bound() ? halt->get_queue_pos(self) : 1;
				go_on_ticks_spacing = (wait_from_ticks + spacing * queue_pos) - reversing_time;
			}
			sint64 go_on_ticks_waiting = WAIT_INFINITE;
			if (fpl->get_current_eintrag().waiting_time_shift > 0)
			{
				// Max. wait for load
				go_on_ticks_waiting = welt->get_zeit_ms() + (welt->ticks_per_world_month >> (16 - fpl->get_current_eintrag().waiting_time_shift)) - reversing_time;
			}
			go_on_ticks = (std::min)(go_on_ticks_spacing, go_on_ticks_waiting);
			go_on_ticks = (std::max)(departure_time, go_on_ticks);
		}
	}

	// loading is finished => maybe drive on
	bool can_go = false;
	can_go = loading_level >= loading_limit;
	can_go = can_go || welt->get_zeit_ms() > go_on_ticks;
	can_go = can_go && welt->get_zeit_ms() > arrival_time + ((sint64)current_loading_time - (sint64)reversing_time);
	can_go = can_go || no_load;
	if(can_go) {

		if(withdraw  &&  (loading_level==0  ||  goods_catg_index.empty())) {
			// destroy when empty
			self_destruct();
			return;
		}

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<anz_vehikel; i++) {
			book(get_vehikel(i)->get_fracht_max()-get_vehikel(i)->get_fracht_menge(), CONVOI_CAPACITY);
		}

		// Advance schedule
		advance_schedule();
		state = ROUTING_1;
	}

	// reset the wait_lock
	if ( state == ROUTING_1 ) {
		wait_lock = 0;
	} else {
		// The random extra wait here is designed to avoid processing every convoi at once
		wait_lock = (go_on_ticks - welt->get_zeit_ms())/2 + (self.get_id())%1024;
		if (wait_lock < 0 ) {
			wait_lock = 0;
		}
	}
}


sint64 convoi_t::calc_restwert() const
{
	
	sint64 result = 0;

	for(uint i=0; i<anz_vehikel; i++) {
		result += fahr[i]->calc_restwert();
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

	for(unsigned i=0; i<anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		if ( v->get_fracht_typ() == warenbauer_t::passagiere ) {
			seats_max += v->get_fracht_max();
			seats_menge += v->get_fracht_menge();
		}
		else {
			fracht_max += v->get_fracht_max();
			fracht_menge += v->get_fracht_menge();
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
	if(fahr[0]) {
		fahr[0]->set_convoi(NULL);
	}

	if(  state == INITIAL  ) {
		// in depot => not on map
		for(  uint8 i = anz_vehikel;  i-- != 0;  ) {
			fahr[i]->set_flag( ding_t::not_on_map );
		}
	}
	state = SELF_DESTRUCT;

	if(fpl!=NULL  &&  !fpl->ist_abgeschlossen()) {
		destroy_win((ptrdiff_t)fpl);
	}
	
	if(  line.is_bound()  ) {
		// needs to be done here to remove correctly ware catg from lines
		unset_line();
		delete fpl;
		fpl = NULL;
	}

	// pay the current value, remove monthly maint
	waytype_t wt = fahr[0] ? fahr[0]->get_besch()->get_waytype() : ignore_wt;
	besitzer_p->book_new_vehicle( calc_restwert(), get_pos().get_2d(), wt);

	for(  uint8 i = anz_vehikel;  i-- != 0;  ) {
		if(  !fahr[i]->get_flag( ding_t::not_on_map )  ) {
			// remove from rails/roads/crossings
			grund_t *gr = welt->lookup(fahr[i]->get_pos());
			fahr[i]->set_letztes( true );
			fahr[i]->verlasse_feld();
			if(  gr  &&  gr->ist_uebergang()  ) {
				gr->find<crossing_t>()->release_crossing(fahr[i]);
			}
			fahr[i]->set_flag( ding_t::not_on_map );

		}
		fahr[i]->loesche_fracht();
		fahr[i]->entferne(besitzer_p);
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
		"besitzer_n = %d\n"
		"akt_speed = %d\n"
		"sp_soll = %d\n"
		"state = %d\n"
		"statename = %s\n"
		"alte_richtung = %d\n"
		"jahresgewinn = %ld\n"	// %lld crashes mingw now, cast gewinn to long ...
		"name = '%s'\n"
		"line_id = '%d'\n"
		"fpl = '%p'",
		(int)anz_vehikel,
		(int)wait_lock,
		(int)welt->sp2num(besitzer_p),
		(int)akt_speed,
		(int)sp_soll,
		(int)state,
		(const char *)(state_names[state]),
		(int)alte_richtung,
		(long)(jahresgewinn/100),
		(const char *)name_and_id,
		line.is_bound() ? line.get_id() : 0,
		(const void *)fpl );
}


void convoi_t::book(sint64 amount, int cost_type)
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
	for(  unsigned i = 0;  i < get_vehikel_anzahl();  i++  ) {
		running_cost += fahr[i]->get_running_cost(welt);
	}
	return running_cost;
}

sint32 convoi_t::get_per_kilometre_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) { //"anzahl" = "number" (Babelfish)
		sint32 vehicle_running_cost = fahr[i]->get_besch()->get_running_cost(welt);
		running_cost += vehicle_running_cost;
	}
	return running_cost;
}

uint32 convoi_t::get_fixed_cost() const
{
	uint32 maint = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) {
		maint += fahr[i]->get_fixed_cost(welt); 
	}
	return maint;
}

sint64 convoi_t::get_purchase_cost() const
{
	sint64 purchase_cost = 0;
	for(  unsigned i = 0;  i < get_vehikel_anzahl();  i++  ) {
		purchase_cost += fahr[i]->get_besch()->get_preis();
	}
	return purchase_cost;
}


/**
* set line
* since convoys must operate on a copy of the route's fahrplan, we apply a fresh copy
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
	}
	line_update_pending = org_line;
	check_pending_updates();
}


/**
* unset line
* removes convoy from route without destroying its fahrplan
* => no need to recalculate connetions!
* @author hsiegeln
*/
void convoi_t::unset_line()
{
	if(  line.is_bound()  ) {
DBG_DEBUG("convoi_t::unset_line()", "removing old destinations from line=%d, fpl=%p",line.get_id(),fpl);
		line->remove_convoy(self);
		line = linehandle_t();
		line_update_pending = linehandle_t();
	}
}


// matches two halts; if the pos is not identical, maybe the halt still is the same
bool convoi_t::matches_halt( const koord3d pos1, const koord3d pos2 )
{
	halthandle_t halt1 = haltestelle_t::get_halt( welt, pos1, besitzer_p );
	return pos1==pos2  ||  (halt1.is_bound()  &&  halt1==haltestelle_t::get_halt( welt, pos2, besitzer_p ));
}


// updates a line schedule and tries to find the best next station to go
void convoi_t::check_pending_updates()
{
	if(  line_update_pending.is_bound()  ) {
		// create dummy schedule
		if(  fpl==NULL  ) {
			fpl = create_schedule();
		}
		schedule_t* new_fpl = line_update_pending->get_schedule();
		int aktuell = fpl->get_aktuell(); // save current position of schedule
		bool is_same = false;
		bool is_depot = false;
		koord3d current = koord3d::invalid, depot = koord3d::invalid;

		if (fpl->empty() || new_fpl->empty()) {
			// was no entry or is no entry => goto  1st stop
			aktuell = 0;
		}
		else {
			// something to check for ...
			current = fpl->get_current_eintrag().pos;

			if(  aktuell<new_fpl->get_count() &&  current==new_fpl->eintrag[aktuell].pos  ) {
				// next pos is the same => keep the convoi state
				is_same = true;
			}
			else {
				// check depot first (must also keept this state)
				is_depot = (welt->lookup(current)  &&  welt->lookup(current)->get_depot() != NULL);

				if(is_depot) {
					// depot => aktuell+1 (depot will be restore later before this)
					depot = current;
					fpl->remove();
					current = fpl->get_current_eintrag().pos;
				}

				/* there could be only one entry that matches best:
				 * we try first same sequence as in old schedule;
				 * if not found, we try for same nextnext station
				 * (To detect also places, where only the platform
				 *  changed, we also compare the halthandle)
				 */
				if(aktuell > fpl->eintrag.get_count() - 1)
				{
					aktuell = fpl->eintrag.get_count() - 1;
				}
				uint8 index = aktuell;
				bool reverse = reverse_schedule;
				koord3d next[4];
				for( uint8 i=0; i<4; i++ ) {
					next[i] = fpl->eintrag[index].pos;
					fpl->increment_index(&index, &reverse);
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
						quality += matches_halt(next[j],new_fpl->eintrag[index].pos) * weights[j];
						new_fpl->increment_index(&index, &reverse);
					}
					if(  quality>how_good_matching  ) 
					{
						// better match than previous: but depending of distance, the next number will be different
						index = i;
						reverse = reverse_schedule;
						for( uint8 j=0; j<4; j++ ) 
						{
							if( matches_halt(next[j], new_fpl->eintrag[index].pos) )
							if( new_fpl->eintrag[index].pos==next[j] ) 
							{
								aktuell = index;
								break;
							}
							new_fpl->increment_index(&index, &reverse);
						}
						how_good_matching = quality;
					}
				}

				if(how_good_matching==0) {
					// nothing matches => take the one from the line
					aktuell = new_fpl->get_aktuell();
				}
				// if we go to same, then we do not need route recalculation ...
				is_same = aktuell < new_fpl->get_count() && matches_halt(current,new_fpl->eintrag[aktuell].pos);
			}
		}

		// we may need to update the line and connection tables
		if(  !line.is_bound()  ) {
			line_update_pending->add_convoy(self);
		}
		line = line_update_pending;
		line_update_pending = linehandle_t();

		// destroy old schedule and all related windows
		if(!fpl->ist_abgeschlossen()) {
			fpl->copy_from( new_fpl );
			fpl->set_aktuell(aktuell); // set new schedule current position to best match
			fpl->eingabe_beginnen();
		}
		else {
			fpl->copy_from( new_fpl );
			fpl->set_aktuell(aktuell); // set new schedule current position to one before best match
		}

		if(is_depot) {
			// next was depot. restore it
			fpl->insert(welt->lookup(depot), 0, 0, 0, besitzer_p == welt->get_active_player());
			// Insert will move the pointer past the inserted item; move back to it
			fpl->advance_reverse();
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
				state = FAHRPLANEINGABE;
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
	if(  fpl  ) {
		FOR(minivec_tpl<linieneintrag_t>, const& i, fpl->eintrag) {
			halthandle_t const halt = haltestelle_t::get_halt(welt, i.pos, get_besitzer());
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
	if(  fpl  ) {
		FOR(minivec_tpl<linieneintrag_t>, const& i, fpl->eintrag) {
			halthandle_t const halt = haltestelle_t::get_halt(welt, i.pos, get_besitzer());
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
   if(  n==INVALID_INDEX  ) 
   {
	   // find out if stop or waypoint, waypoint: do not brake at waypoints
	   const koord3d &route_end = route.back();
	   const int count = fpl->get_count();
	   bool reverse_waypoint = false;
	   for(int i = 0; i < count; i ++)
	   {
		   const linieneintrag_t &eintrag = fpl->eintrag[i];
		   if(eintrag.pos == route_end)
		   {
				reverse_waypoint = eintrag.reverse;
				break;
		   }
	   }

	   grund_t const* const gr = welt->lookup(route_end);
	   if(  gr  &&  (gr->is_halt() || reverse_waypoint)  ) 
	   {
		   n = route.get_count()-1;
	   }
   }
	next_stop_index = n+1;
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
		return COL_WHITE;
	} else if (state == WAITING_FOR_CLEARANCE_ONE_MONTH || state == CAN_START_ONE_MONTH || get_state() == NO_ROUTE) {
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
	return COL_BLACK;
}

uint8 
convoi_t::get_catering_level(uint8 type) const
{
	uint8 max_catering_level = 0;
	uint8 current_catering_level;
	//for(sint16 i = fahr.get_size() - 1; i >= 0; i --)
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		vehikel_t* v = get_vehikel(i);
		if(v == NULL)
		{
			continue;
		}
		current_catering_level = v->get_besch()->get_catering_level();
		if(current_catering_level > max_catering_level && v->get_besch()->get_ware()->get_catg_index() == type)
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
	if (get_vehikel_anzahl()!=other->get_vehikel_anzahl()) 
	{
		return false;
	}
	// We must compare both in the 'same direction' and in the 'reverse direction'.
	// We cannot use the 'reverse' flag to tell the difference.
	bool forward_compare_good = true;
	for (int i=0; i<get_vehikel_anzahl(); i++)
	{
		if (get_vehikel(i)->get_besch()!=other->get_vehikel(i)->get_besch())
		{
			forward_compare_good = false;
			break;
		}
	}
	if (forward_compare_good) {
		return true;
	}
	bool reverse_compare_good = true;
	for (int i=0, j=get_vehikel_anzahl()-1; i<get_vehikel_anzahl(); i++, j--)
	{
		if (get_vehikel(j)->get_besch()!=other->get_vehikel(i)->get_besch())
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
class depot_finder_t : public fahrer_t
{
private:
	vehikel_t *master;
	uint8 traction_type;
public:
	depot_finder_t( convoihandle_t cnv, uint8 tt ) 
	{ 
		master = cnv->get_vehikel(0); 
		assert(master!=NULL); 
		traction_type = tt;
	};
	virtual waytype_t get_waytype() const 
	{ 
		return master->get_waytype(); 
	};
	virtual bool ist_befahrbar( const grund_t* gr ) const 
	{ 
		return master->ist_befahrbar(gr); 
	};
	virtual bool ist_ziel( const grund_t* gr, const grund_t* ) 
	{ 
		return gr->get_depot() && gr->get_depot()->get_besitzer() == master->get_besitzer() && gr->get_depot()->get_tile()->get_besch()->get_enabled() & traction_type; 
	};
	virtual ribi_t::ribi get_ribi( const grund_t* gr) const
	{ 
		return master->get_ribi(gr); 
	};
	virtual int get_kosten( const grund_t* gr, const sint32, koord from_pos) 
	{ 
		return master->get_kosten(gr, 0, from_pos);
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
		go_to_depot(get_besitzer() == welt->get_active_player());
	}
}

/**
 * Convoy is sent to depot.  Return value, success or not.
 */
bool convoi_t::go_to_depot(bool show_success, bool use_home_depot)
{
	if(!fpl->ist_abgeschlossen()) 
	{
		return false;
	}

	// limit update to certain states that are considered to be safe for fahrplan updates
	int state = get_state();
	if(state==convoi_t::FAHRPLANEINGABE) {
DBG_MESSAGE("convoi_t::go_to_depot()","convoi state %i => cannot change schedule ... ", state );
		return false;
	}

	// Identify this convoi's traction types.  We want to match at least one.
	uint8 traction_types = 0;
	uint8 shifter;
	if(replace)
	{
		ITERATE_PTR(replace->get_replacing_vehicles(), i)
		{
			if(replace->get_replacing_vehicle(i)->get_leistung() == 0)
			{
				continue;
			}
			shifter = 1 << replace->get_replacing_vehicle(i)->get_engine_type();
			traction_types |= shifter;
		}
	}
	else
	{
		for(uint8 i = 0; i < anz_vehikel; i ++)
		{
			if(fahr[i]->get_besch()->get_leistung() == 0)
			{
				continue;
			}
			shifter = 1 << fahr[i]->get_besch()->get_engine_type();
			traction_types |= shifter;
		}
	}

	bool home_depot_valid = false;
	if (use_home_depot) {
		// Check for a valid home depot.  It is quite easy to get savegames with
		// invalid home depot coordinates.
		const grund_t* test_gr = welt->lookup(get_home_depot());
		if (test_gr) {
			depot_t* test_depot = test_gr->get_depot();
			if (test_depot) {
				// It's a depot -- is it suitable?
				home_depot_valid = test_depot->is_suitable_for(get_vehikel(0), traction_types);
			}
		}
		// The home depot seems OK, but what if we can't get there?
		// Don't consider it a valid home depot if we already failed to get there.
		if (home_depot_valid && state == NO_ROUTE) {
			if (fpl) {
				const linieneintrag_t & current_entry = fpl->get_current_eintrag();
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
		const grund_t* test_gr = welt->lookup( get_vehikel(0)->get_pos() );
		if (test_gr) {
			depot_t* test_depot = test_gr->get_depot();
			if (test_depot) {
				// It's a depot -- is it suitable?
				current_location_valid = test_depot->is_suitable_for(get_vehikel(0), traction_types);
			}
		}
		if (current_location_valid) {
			depot_pos = get_vehikel(0)->get_pos();
			other_depot_found = true;
		}
		else {
			// OK, we're not standing on a depot.  Find a route to a depot.
			depot_finder_t finder(self, traction_types);
			route.find_route(welt, get_vehikel(0)->get_pos(), &finder, speed_to_kmh(get_min_top_speed()), ribi_t::alle, get_highest_axle_load(), 0x7FFFFFFF);
			if (!route.empty()) {
				depot_pos = route.position_bei(route.get_count() - 1);
				other_depot_found = true;
			}
		}
	}

	bool transport_success = false;
	if (home_depot_found || other_depot_found) {
		if ( get_vehikel(0)->get_pos() == depot_pos ) {
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
		else {
			// Work directly on the schedule (consider changing this to make a new copy)
			bool schedule_insertion_succeeded = fpl->insert( welt->lookup(depot_pos) );
			// Insert will move the pointer past the inserted item; move back to it
			fpl->advance_reverse();
			// We still have to call set_schedule
			bool schedule_setting_succeeded = set_schedule(fpl);
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
		success = false;
	}
	if ( (!success || show_success) && get_besitzer() == welt->get_active_player())
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
	for(unsigned i=0; i<anz_vehikel; i++) 
	{
		if (fahr[i]->get_fracht_max()>0) 
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
	for(sint8 i=0;  i<anz_vehikel-1;  i++) {
		carunits += fahr[i]->get_besch()->get_length();
	}
	// the last vehicle counts differently in stations and for reserving track
	// (1) add 8 = 127/256 tile to account for the driving in stations in north/west direction
	//     see at the end of vehikel_t::hop()
	// (2) for length of convoi for loading in stations the length of the last vehicle matters
	//     see convoi_t::hat_gehalten
	carunits += max(CARUNITS_PER_TILE/2, fahr[anz_vehikel-1]->get_besch()->get_length());

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
			// should never happen, since there is a vehcile in front of us ...
			return false;
		}
		weg_t *str = gr->get_weg(road_wt);
		if(  str==0  ) {
			// also this is not possible, since a car loads in front of is!?!
			return false;
		}

		uint16 idx = fahr[0]->get_route_index();
		const sint32 tiles = (steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) + get_tile_length() + 1;
		if(  tiles > 0  &&  idx+(uint32)tiles >= route.get_count()  ) {
			// needs more space than there
			return false;
		}

		for(  sint32 i=0;  i<tiles;  i++  ) {
			grund_t *gr = welt->lookup( route.position_bei( idx+i ) );
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
				if(  vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(j))  ) {
					// check for other traffic on the road
					const overtaker_t *ov = v->get_overtaker();
					if(ov) {
						if(this!=ov  &&  other_overtaker!=ov) {
							return false;
						}
					}
					else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=ding_t::fussgaenger  ) {
						return false;
					}
				}
			}
		}
		convoi_t *ocnv = dynamic_cast<convoi_t *>(other_overtaker);
		set_tiles_overtaking( 2 + ocnv->get_length()/CARUNITS_PER_TILE + get_length()/CARUNITS_PER_TILE );
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
	int distance = akt_speed*(fahr[0]->get_steps()+get_length_in_steps()+steps_other-VEHICLE_STEPS_PER_TILE)/diff_speed;
	int time_overtaking = 0;

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	// First phase: no traffic except me and my overtaken car in the dangerous zone
	unsigned int route_index = fahr[0]->get_route_index()+1;
	koord pos_prev = fahr[0]->get_pos_prev().get_2d();
	koord3d pos = fahr[0]->get_pos();
	koord3d pos_next;

	while( distance > 0 ) {

		if(  route_index >= route.get_count()  ) {
			return false;
		}

		pos_next = route.position_bei(route_index++);
		grund_t *gr = welt->lookup(pos);
		// no ground, or slope => about
		if(  gr==NULL  ||  gr->get_weg_hang()!=hang_t::flach  ) {
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
				const roadsign_besch_t *rb = rs->get_besch();
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

		int d = ribi_t::ist_gerade(str->get_ribi()) ? VEHICLE_STEPS_PER_TILE : vehikel_basis_t::get_diagonal_vehicle_steps_per_tile();
		distance -= d;
		time_overtaking += d;

		// Check for other vehicles
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			if (vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(j))) {
				// check for other traffic on the road
				const overtaker_t *ov = v->get_overtaker();
				if(ov) {
					if(this!=ov  &&  other_overtaker!=ov) {
						return false;
					}
				}
				else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=ding_t::fussgaenger  ) {
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

		pos_next = route.position_bei(route_index++);
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

		if(  ribi_t::ist_gerade(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / kmh_to_speed(str->get_max_speed());
		}
		else {
			time_overtaking -= (vehikel_basis_t::get_diagonal_vehicle_steps_per_tile()<<16) / kmh_to_speed(str->get_max_speed());
		}

		// Check for other vehicles in facing direction
		ribi_t::ribi their_direction = ribi_t::rueckwaerts( fahr[0]->calc_richtung(pos_prev, pos_next.get_2d()) );
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(j));
			if (v && v->get_fahrtrichtung() == their_direction && v->get_overtaker()) {
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
	const sint64 current_ticks = welt->get_zeit_ms();
	const grund_t* gr = welt->lookup(this->get_pos());
	if(gr && welt->get_zeit_ms() - arrival_time > reverse_delay && gr->is_halt())
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
	const sint32 remaining_ticks = (sint32)(departures->get(last_stop_id).departure_time - welt->get_zeit_ms());
	uint32 ticks_left = 0;
	if(remaining_ticks >= 0)
	{
		ticks_left = remaining_ticks;
	}
	welt->sprintf_ticks(p, size, ticks_left);
}

uint32 convoi_t::calc_highest_axle_load()
{
	uint32 heaviest = 0;
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		uint32 tmp = fahr[i]->get_besch()->get_axle_load();
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
	for(int i = 0; i < anz_vehikel; i ++)
	{
		uint32 tmp = fahr[i]->get_besch()->get_min_loading_time();
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
	for(int i = 0; i < anz_vehikel; i ++)
	{
		uint32 tmp = fahr[i]->get_besch()->get_max_loading_time();
		if(tmp > longest)
		{
			longest = tmp;
		}
	}
	return longest;
}

// Bernd Gabriel, 18.06.2009: extracted from new_month()
bool convoi_t::calc_obsolescence(uint16 timeline_year_month)
{
	// convoi has obsolete vehicles?
	for(int j = get_vehikel_anzahl(); --j >= 0; ) {
		if (fahr[j]->get_besch()->is_obsolete(timeline_year_month, welt)) {
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
	 for (int i = 0; i < anz_vehikel; i++) 
	 {
		vehikel_t& v = *fahr[i];
		const char* liv = scheme->get_latest_available_livery(date, v.get_besch());
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
	uint32 reverse_delay;

	if(reversable)
	{
		// Multiple unit or similar: quick reverse
		reverse_delay = welt->get_settings().get_unit_reverse_time();
	}
	else if(fahr[0]->get_besch()->is_bidirectional())
	{
		// Loco hauled, no turntable.
		if(anz_vehikel > 1 && fahr[anz_vehikel-2]->get_besch()->get_can_be_at_rear() == false
			&& fahr[anz_vehikel-2]->get_besch()->get_ware()->get_catg_index() > 1)
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
	 const sint64 current_time = welt->get_zeit_ms();
	 uint16 waiting_minutes;
	 const uint16 airport_wait = fahr[0]->get_typ() == ding_t::aircraft ? welt->get_settings().get_min_wait_airport() : 0;
	 for(uint8 i = 0; i < anz_vehikel; i++) 
	 {
		FOR(slist_tpl< ware_t>, const& iter, fahr[i]->get_fracht()) 
		{	
			if(iter.get_last_transfer().get_id() == halt.get_id())
			{
				waiting_minutes = max(get_waiting_minutes(current_time - iter.arrival_time), airport_wait);
				// Only times of one minute or larger are registered, to avoid registering zero wait-time when a passenger
				// alights a convoy and then immediately re-boards that same convoy.
				if(waiting_minutes > 19) 
				{
					halt->add_waiting_time(waiting_minutes, iter.get_zwischenziel(), iter.get_besch()->get_catg_index());
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
		departures->set(last_stop_id, dep);
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

 void convoi_t::calc_current_loading_time(uint16 load_charge)
 {
	if(longest_max_loading_time == longest_min_loading_time || load_charge == 0)
	{
		current_loading_time = longest_min_loading_time;
		return;
	}

	uint16 total_capacity = 0;
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		total_capacity += fahr[i]->get_besch()->get_zuladung();
		total_capacity += fahr[i]->get_besch()->get_overcrowded_capacity();
	}
	// Multiply this by 2, as goods/passengers can both board and alight, so
	// the maximum load charge is twice the capacity: all alighting, then all
	// boarding.
	total_capacity *= 2;
	const sint32 percentage = (load_charge * 100) / total_capacity;
	const sint32 difference = abs((((sint32)longest_max_loading_time - (sint32)longest_min_loading_time)) * percentage) / 100;
	current_loading_time = difference + longest_min_loading_time;
 }

 bool convoi_t::is_circular_route() const
 {
	// Three lines used here to aid debugging.
	const uint32 departures_count = departures->get_count();
	const uint8 schedule_count = fpl->get_count();
	return departures_count == schedule_count;
 }

 ding_t::typ convoi_t::get_depot_type() const
 {
	 const waytype_t wt = fahr[0]->get_waytype();
	 switch(wt)
	 {
	 case road_wt:
		 return ding_t::strassendepot;
	 case track_wt:
		 return ding_t::bahndepot;
	 case water_wt:
		 return ding_t::schiffdepot;
	 case monorail_wt:
		 return ding_t::monoraildepot;
	 case maglev_wt:
		 return ding_t::maglevdepot;
	 case tram_wt:
		 return ding_t::tramdepot;
	 case narrowgauge_wt:
		 return ding_t::narrowgaugedepot;
	 case air_wt:
		 return ding_t::airdepot;
	 default:
		 dbg->error("ding_t::typ convoi_t::get_depot_type() const", "Invalid waytype: cannot find correct depot type");
		 return ding_t::strassendepot;
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
			dep = depot_t::find_depot(this->get_pos(), get_depot_type(), get_besitzer(), true);
		}
		if(dep)
		{
			// Only do this if a depot can be found, or else a crash will result.

			// Give a different message than the usual depot arrival message.
			cbuffer_t buf;
			buf.printf( translator::translate("No route to depot for convoy %s: teleported to depot!"), get_name() );
			const vehikel_t* v = fahr[0];
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);

			enter_depot(dep);
			// Do NOT do the convoi_arrived here: it's done in enter_depot!
			state = INITIAL;
			fpl->set_aktuell(0);
		}
		else
		{
			// We can't send it to a depot, because there are no appropriate depots.  Destroy it!

			cbuffer_t buf;
			buf.printf( translator::translate("No route and no depot for convoy %s: convoy has been sold!"), get_name() );
			const vehikel_t* v = fahr[0];
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::warnings, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);

			// The player can engineer this deliberately by blowing up depots and tracks, so it isn't always a game error.
			// But it usually is an error, so report it as one.
			dbg->error("void convoi_t::emergency_go_to_depot()", "Could not find a depot to which to send convoy %i", self.get_id() );

			destroy();
		}
	}
}

koordhashtable_tpl<id_pair, average_tpl<uint16> > * const convoi_t::get_average_journey_times()
{
	if(line.is_bound())
	{
		if(line->get_is_alternating_circle_route() && reverse_schedule)
		{
			return line->get_average_journey_times_reverse_circular();
		}
		else
		{
			return line->get_average_journey_times();
		}
	}
	else
	{
		return average_journey_times;
	}
}

void convoi_t::clear_departures()
{
	departures->clear();
}

sint64 convoi_t::get_stat_converted(int month, int cost_type) const
{
	sint64 value = financial_history[month][cost_type];
	switch(cost_type) {
		case CONVOI_REVENUE:
		case CONVOI_OPERATIONS:
		case CONVOI_PROFIT:
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
	return get_vehikel_anzahl() > 0 ? get_friction_of_waytype(get_vehikel(0)->get_waytype()) : 0;
}

void convoi_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{
	vehicle.clear();
	uint32 count = get_vehikel_anzahl();
	for (uint32 i = count; i-- > 0; )
	{
		vehicle.add_vehicle(*get_vehikel(i)->get_besch());
	}
	if (count > 0)
	{
		vehicle.update_summary(get_vehikel(count-1)->get_besch()->get_length());
	}
}


void convoi_t::update_adverse_summary(adverse_summary_t &adverse)
{
	adverse.clear();
	uint16 count = get_vehikel_anzahl();
	for (uint16 i = count; i-- > 0; )
	{
		vehikel_t &v = *get_vehikel(i);
		adverse.add_vehicle(v);
	}
	if (count > 0)
	{
		adverse.fr /= count;
	}
}


void convoi_t::update_freight_summary(freight_summary_t &freight)
{		
	freight.clear();
	for (uint16 i = get_vehikel_anzahl(); i-- > 0; )
	{
		const vehikel_besch_t &b = *get_vehikel(i)->get_besch();
		freight.add_vehicle(b);
	}
}


void convoi_t::update_weight_summary(weight_summary_t &weight)
{
	weight.weight = 0;
	sint64 sin = 0;
	for (uint16 i = get_vehikel_anzahl(); i-- > 0; )
	{
		const vehikel_t &v = *get_vehikel(i);
		sint32 kgs = v.get_gesamtgewicht();
		weight.weight += kgs;
		sin += kgs * v.get_frictionfactor();
	}
	weight.weight_cos = weight.weight;
	weight.weight_sin = (sin + 500) / 1000;
}


float32e8_t convoi_t::get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */)
{
	float32e8_t force = 0, br = get_adverse_summary().br;
	for (uint16 i = get_vehikel_anzahl(); i-- > 0; )
	{
		vehikel_t &v = *get_vehikel(i);
		const uint16 bf = v.get_besch()->get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf * (uint32) 1000;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground. 
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += br * v.get_gesamtgewicht();
		}
	}
	return force;
}
 

float32e8_t convoi_t::get_force_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 force = 0;
	sint32 v = speed;

	for (uint16 i = get_vehikel_anzahl(); i-- > 0; )
	{
		force += get_vehikel(i)->get_besch()->get_effective_force_index(v);
	}

	//for (array_tpl<vehikel_t*>::iterator i = fahr.begin(), n = i + get_vehikel_anzahl(); i != n; ++i)
	//	force += (*i)->get_besch()->get_effective_force_index(v);

	return power_index_to_power(force, get_welt()->get_settings().get_global_power_factor_percent());
}


float32e8_t convoi_t::get_power_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 power = 0;
	sint32 v = speed;
	for (uint16 i = get_vehikel_anzahl(); i-- > 0; )
	{
		power += get_vehikel(i)->get_besch()->get_effective_power_index(v);
	}
	return power_index_to_power(power, get_welt()->get_settings().get_global_power_factor_percent());
}
// BG, 31.12.2012: end of virtual methods of lazy_convoy_t
