/**
 * convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#include <math.h>
#include <stdlib.h>

#include "simdebug.h"
#include "simworld.h"
#include "simware.h"
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
#include "freight_list_sorter.h"

#include "gui/karte.h"
#include "gui/convoi_info_t.h"
#include "gui/fahrplan_gui.h"
#include "gui/messagebox.h"
#include "boden/grund.h"
#include "boden/wege/schiene.h"	// for railblocks

#include "bauer/vehikelbauer.h"

#include "besch/vehikel_besch.h"
#include "besch/roadsign_besch.h"

#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/replace_data.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "dings/crossing.h"
#include "dings/roadsign.h"

#include "vehicle/simvehikel.h"
#include "vehicle/overtaker.h"

#include "utils/simstring.h"
#include "utils/cbuffer_t.h"

#include "convoy.h"

// zeige debugging info in infofenster wenn definiert
// #define DEBUG 1

/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 2000

/*
 * Waiting time for infinite loading (ms)
 * @author Hj- Malthaner
 */
#define WAIT_INFINITE 0xFFFFFFFFu



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
static int calc_min_top_speed(const array_tpl<vehikel_t*>& fahr, uint8 anz_vehikel)
{
	int min_top_speed = 9999999;
	for(uint8 i=0; i<anz_vehikel; i++) {
		min_top_speed = min(min_top_speed, kmh_to_speed( fahr[i]->get_besch()->get_geschw() ) );
	}
	return min_top_speed;
}


// Reset some values.  Used in init and replacing.
void convoi_t::reset()
{
	is_electric = false;
	//sum_gesamtgewicht = sum_gewicht = sum_gear_und_leistung = sum_leistung = power_from_steam = power_from_steam_with_gear = 0;
	sum_gesamtgewicht = sum_gewicht = sum_leistung = sum_gear_und_leistung = 0;
	previous_delta_v = 0;
	min_top_speed = 9999999;

	withdraw = false;
	has_obsolete = false;
	no_load = false;
	replace = NULL;
	depot_when_empty = false;

	jahresgewinn = 0;

	max_record_speed = 0;
	akt_speed_soll = 0;     // Sollgeschwindigkeit / set speed
	akt_speed = 0;          // momentane Geschwindigkeit / current speed
	sp_soll = 0;

	heaviest_vehicle = 0;
	longest_loading_time = 0;
	last_departure_time = welt->get_zeit_ms();
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

	reset();

	fpl = NULL;
	line = linehandle_t();
	line_id = INVALID_LINE_ID;

	anz_vehikel = 0;
	steps_driven = -1;
	wait_lock = 0;
	go_on_ticks = WAIT_INFINITE;

	jahresgewinn = 0;
	total_distance_traveled = 0;
	tiles_since_last_odometer_increment = 0;

	alte_richtung = ribi_t::keine;
	next_wolke = 0;

	state = INITIAL;

	*name_and_id = 0;
	name_offset = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;

	next_stop_index = 65535;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;
	last_stop_pos = koord3d::invalid;

	reversable = false;
	reversed = false;
}


convoi_t::convoi_t(karte_t* wl, loadsave_t* file) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	init(wl, 0);
	rdwr(file);
	current_stop = fpl == NULL ? 255 : fpl->get_aktuell() - 1;

	// Added by : Knightly
	old_fpl = NULL;
	recalc_catg_index();
	has_obsolete = calc_obsolescence(welt->get_timeline_year_month());

	replace = NULL;
}

convoi_t::convoi_t(spieler_t* sp) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	init(sp->get_welt(), sp);
	set_name( "Unnamed" );
	welt->add_convoi( self );
	init_financial_history();
	current_stop = 255;

	// Added by : Knightly
	old_fpl = NULL;
}


convoi_t::~convoi_t()
{
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

	// force asynchronous recalculation
	if(fpl) {
		if(!fpl->ist_abgeschlossen()) { //"is completed" (Google)
			destroy_win((long)fpl);
		}
	
		if(fpl->get_count()>0  &&  !line.is_bound()  ) 
		{
			// New method - recalculate as necessary
			
			// Added by : Knightly
			haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p, welt->get_einstellungen()->get_default_path_option());
		}
		delete fpl;
	}

	if(replace)
	{
		replace->decrement_convoys();
	}

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


void convoi_t::laden_abschliessen()
{
	if(fpl==NULL) {
		if(  state!=INITIAL  ) {
			grund_t *gr = welt->lookup(home_depot);
			if(gr  &&  gr->get_depot()) {
				dbg->warning( "convoi_t::laden_abschliessen()","No schedule during loading convoi %i: State will be initial!", self.get_id() );
				for( uint8 i=0;  i<anz_vehikel;  i++ ) {
					fahr[i]->get_pos() = home_depot;
				}
				state = INITIAL;
			}
			else {
				dbg->error( "convoi_t::laden_abschliessen()","No schedule during loading convoi %i: Convoi will be destroyed!", self.get_id() );
				for( uint8 i=0;  i<anz_vehikel;  i++ ) {
					fahr[i]->get_pos() = koord3d::invalid;
				}
				destroy();
			}
		}
		// anyway reassign convoi pointer ...
		for( uint8 i=0;  i<anz_vehikel;  i++ ) {
			fahr[i]->set_convoi(this);
			// BG, 06.06.2009: loader does not call laden_abschliessen() for vehicles in depots
			fahr[i]->laden_abschliessen();
		}
		return;
	}

	bool realing_position = false;
	if(anz_vehikel>0) {
DBG_MESSAGE("convoi_t::laden_abschliessen()","state=%s, next_stop_index=%d", state_names[state] );
		for( uint8 i=0;  i<anz_vehikel;  i++ ) {
			vehikel_t* v = fahr[i];
			v->set_erstes( i==0 );
			v->set_letztes( i+1==anz_vehikel );
			// this sets the convoi and will renew the block reservation, if needed!
			v->set_convoi(this);

			// wrong alingmant here => must relocate
			if(v->need_realignment()) {
				// diagonal => convoi must restart
				realing_position |= (v->get_fahrtrichtung()&0x0A)!=0  &&  (state==DRIVING  ||  is_waiting());
			}
		}
DBG_MESSAGE("convoi_t::laden_abschliessen()","next_stop_index=%d", next_stop_index );
		// lines have been still unknown during loading for old savegames!
		if (line_id != INVALID_LINE_ID) {
			// if a line is assigned, set line!
			linehandle_t new_line = besitzer_p->simlinemgmt.get_line_by_id(line_id);
			if(  new_line.is_bound()  &&  !fpl->matches( welt, new_line->get_schedule() )  ) {
				// 101 version produced broken line ids => we have to find our line the hard way ...
				vector_tpl<linehandle_t> lines;
				get_besitzer()->simlinemgmt.get_lines(fpl->get_type(), &lines);
				new_line = linehandle_t();
				for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; i++) {
					linehandle_t l = *i;
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
				line_id = new_line->get_line_id();
				line->add_convoy(self);
				DBG_DEBUG("convoi_t::register_with_line()","%s registers for %d", name_and_id, line_id);
			}
			else {
				line_id = INVALID_LINE_ID;
				line = linehandle_t();
			}
		}
	}
	else {
		// no vehicles in this convoi?!?
		dbg->error( "convoi_t::laden_abschliessen()","No vehicles in Convoi %i: will be destroyed!", self.get_id() );
		destroy();
	}
	// put convoi agian right on track?
	if(realing_position  &&  anz_vehikel>1) {
		// display just a warning
		dbg->warning("convoi_t::laden_abschliessen()","cnv %i is currently too long.",self.get_id());

		// since start may have been changed
		uint16 start_index = max(2,fahr[anz_vehikel-1]->get_route_index())-2;
		koord3d k0 = fahr[anz_vehikel-1]->get_pos();
		uint32 train_length = 1;	// length in 1/16 of tile

		for(unsigned i=0; i<anz_vehikel; i++) {

			vehikel_t *v = fahr[i];
			steps_driven = -1;
			grund_t* gr = welt->lookup(v->get_pos());
			if(gr) {
				v->mark_image_dirty( v->get_bild(), v->get_hoff() );
				v->verlasse_feld();
				// eventually unreserve this
				schiene_t * sch0 = dynamic_cast<schiene_t *>( gr->get_weg(fahr[i]->get_waytype()) );
				if(sch0) {
					sch0->unreserve(v);
				}
			}

			// steps to advance afterwards ...
			if(  i < (anz_vehikel-1u)  ) {
				train_length += fahr[i]->get_besch()->get_length();
			}

			/* we will set by this method the pos_prev to the starting point;
			 * otherwise it may be elsewhere, especially on curves and with already
			 * broken convois.
			 */
			v->set_pos(k0);
			v->neue_fahrt(start_index, true);
			gr=welt->lookup(v->get_pos());
			if(gr) {
				v->set_pos(k0);
				v->betrete_feld();
			}
		}
		train_length = max(1,train_length);

		// now advance all convoi until it is completely on the track
		fahr[0]->set_erstes(false); // switches off signal checks ...
		for(unsigned i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];

			v->darf_rauchen(false); //"Allowed to smoke" (Google)
			fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 );
			train_length -= v->get_besch()->get_length();
			v->darf_rauchen(true);

			// eventually reserve this again
			grund_t *gr=welt->lookup(v->get_pos());
			// airplanes may have no ground ...
			schiene_t *sch0 = dynamic_cast<schiene_t *>( gr->get_weg(fahr[i]->get_waytype()) );
			if(sch0) {
				sch0->reserve(self,ribi_t::keine);
			}
		}
		fahr[0]->set_erstes(true);
	}
}



// since now convoi states go via werkzeug_t
void convoi_t::call_convoi_tool( const char function, const char *extra)
{
	werkzeug_t *w = create_tool( WKZ_CONVOI_TOOL | SIMPLE_TOOL );
	char cmd[3] = { function, ',', 0 };
	cbuffer_t param(8192);
	param.append( cmd );
	param.append( self.get_id() );
	if(  extra  &&  *extra  ) {
		param.append( "," );
		param.append( extra );
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
	// eventually correct freight destinations (and remove all stale freight)
	for(int i = 0; i < anz_vehikel; i++) 
	{
		fahr[i]->remove_stale_freight();
	}
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
void convoi_t::set_name(const char *name)
{
	char buf[128];
	name_offset = sprintf(buf,"(%i) ",self.get_id() );
	tstrncpy(buf+name_offset, translator::translate(name), 116);
	tstrncpy(name_and_id, buf, lengthof(name_and_id));
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
void convoi_t::add_running_cost(sint64 cost)
{
	jahresgewinn += cost;

	get_besitzer()->buche(cost, COST_VEHICLE_RUN);

	book( cost, CONVOI_OPERATIONS );
	book( cost, CONVOI_PROFIT );
}

void convoi_t::increment_odometer()
{
	tiles_since_last_odometer_increment ++;
	// Need to use clipping here when converting a float to a uint8 to round down.
	const float distance_per_tile = welt->get_einstellungen()->get_distance_per_tile();
	const uint8 km = tiles_since_last_odometer_increment * distance_per_tile;
	if(km >= 1)
	{
		book( km, CONVOI_DISTANCE );
		total_distance_traveled += km;
		tiles_since_last_odometer_increment -= (km / distance_per_tile);
	}
}



/* Calculates (and sets) new akt_speed
 * needed for driving, entering and leaving a depot)
 */
void convoi_t::calc_acceleration(long delta_t)
{
	// existing_convoy_t is designed to become a part of convoi_t. 
	// There it will help to minimize updating convoy summary data.
	existing_convoy_t convoy(*this);
	convoy.calc_move(delta_t, get_welt()->get_einstellungen()->get_distance_per_tile(), akt_speed_soll, akt_speed, sp_soll);
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
				akt_speed_soll = max( akt_speed_soll, kmh_to_speed(30) );
				calc_acceleration(delta_t);
				//moved to inside calc_acceleration(): sp_soll += (akt_speed*delta_t);
				// Make sure that the last_stop_pos is set here so as not
				// to skew average speed readings from vehicles emerging 
				// from depots.
				// @author: jamespetts
				fahr[0]->last_stop_pos = fahr[0]->get_pos().get_2d();

				// now actually move the units
				while(sp_soll>>12) {
					uint32 sp_hat = fahr[0]->fahre_basis(1<<12);
					int v_nr = get_vehicle_at_length((++steps_driven)>>4);
					// stop when depot reached
					if(state==INITIAL) {
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

			last_departure_time = welt->get_zeit_ms();

			break;	// LEAVING_DEPOT
			
		case DRIVING:
			{
				calc_acceleration(delta_t);

				// now actually move the units
				//moved to inside calc_acceleration(): sp_soll += (akt_speed*delta_t);
				uint32 sp_hat = fahr[0]->fahre_basis(sp_soll);
				// stop when depot reached ...
				if(state==INITIAL) {
					break;
				}
				// now move the rest (so all vehikel are moving synchroniously)
				for(unsigned i=1; i<anz_vehikel; i++) {
					fahr[i]->fahre_basis(sp_hat); //"cycle basis" (Google)
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
	if(  anz_vehikel>0  ) {
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

		if(  !fahr[0]->calc_route(start, ziel, speed_to_kmh(min_top_speed), &route)  ) {
			state = NO_ROUTE;
			get_besitzer()->bescheid_vehikel_problem(self,ziel);
			// wait 10s before next attempt
			wait_lock = 25000;
		}
		else {
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

	// moved check to here, as this will apply the same update
	// logic/constraints convois have for manual schedule manipulation
	if (line_update_pending.is_bound()) {
		check_pending_updates();
	}

	if(wait_lock!=0) {
		return;
	}

	bool autostart = false;

	switch(state) {

		case INITIAL:
			// If there is a pending replacement, just do it
			if (replace && replace->get_replacing_vehicles()->get_count()>0) {

				autostart = replace->get_autostart();

				// Knightly : before replacing, copy the existing set of goods category index
				minivec_tpl<uint8> old_goods_catg_index(goods_catg_index.get_count());
				for( uint8 i = 0; i < goods_catg_index.get_count(); ++i ) 
				{
					old_goods_catg_index.append( goods_catg_index[i] );
				}

				const grund_t *gr = welt->lookup(home_depot);
				depot_t *dep;
				if ( gr && (dep=gr->get_depot()) ) {
					bool keep_name=(anz_vehikel>0 && strcmp(get_name(),fahr[0]->get_besch()->get_name())!=0);						
					vector_tpl<vehikel_t*> new_vehicles;

					// Acquire the new one
					ITERATE_PTR(replace->get_replacing_vehicles(),i)
					{
						vehikel_t* veh = NULL;
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

						// This part of the code is abandoned, as it causes problems: vehicles that have already been bought
						// are added twice to the convoy, causing bizarre corruption issues.
						/*if( veh == NULL)
						{
							 //Second - check whether there are any of the required vehicles already
							 //in the depot (more or less free).
							veh = dep->find_oldest_newest(replacing_vehicles[i], true);
						}*/

						if (veh == NULL) 
						{
							// Third - check whether the vehicle can be upgraded (cheap)
							for(uint16 j = 0; j < anz_vehikel; j ++)
							{
								
								for(uint8 c = 0; c < fahr[j]->get_besch()->get_upgrades_count(); c ++)
								{
									if(replace->get_replacing_vehicle(i) == fahr[j]->get_besch()->get_upgrades(c))
									{
										veh = remove_vehikel_bei(j);
										//TODO: Use the new method of replacing here, creating a new vehicle rather than simply replacing the type.
										veh->set_besch(replace->get_replacing_vehicle(i));
										dep->buy_vehicle(veh->get_besch()/*, true*/);
										goto end_loop;
									}
								}
							}
end_loop:	
							if(veh == NULL)
							{
								// Fourth - if all else fails, buy from new (expensive).
								veh = dep->buy_vehicle(replace->get_replacing_vehicle(i)/*, false*/);
							}
						}
						
						// This new method is needed to enable this method to iterate over
						// the existing vehicles in the convoy while it is adding new vehicles.
						// They must be added to temporary storage, and appended to the existing
						// convoy at the end, after the existing convoy has been deleted.
						new_vehicles.append(veh);
						
					}

					//First, delete the existing convoy
					for(sint8 a = anz_vehikel-1;  a >= 0; a--) 
					{
						//Sell any vehicles not upgraded or kept.
						sint64 value = fahr[a]->calc_restwert();
						besitzer_p->buche( value, dep->get_pos().get_2d(), COST_NEW_VEHICLE );
						besitzer_p->buche( -value, COST_ASSETS );	
						delete fahr[a];
					}
					anz_vehikel = 0;
					reset();

					//Next, add all the new vehicles to the convoy in order.
					ITERATE(new_vehicles,b)
					{
						dep->append_vehicle(self, new_vehicles[b], false);
					}
					
					if (!keep_name) {
						set_name(fahr[0]->get_besch()->get_name());
					}
					if(replace)
					{
						replace->decrement_convoys();
						set_replace(NULL);
					}
					if (line.is_bound()) {
						line->recalc_status();
						if (line->get_replacing_convoys_count()==0) {
							char buf[128];
							sprintf(buf, translator::translate("Replacing\nvehicles of\n%-20s\ncompleted"), line->get_name());
							welt->get_message()->add_message(buf, home_depot.get_2d(),message_t::convoi, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);
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
							haltestelle_t::refresh_routing(fpl, differences, besitzer_p, welt->get_einstellungen()->get_default_path_option());
						}
					}


					if (autostart) {
						dep->start_convoi(self);
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
			}
			
			break;

		case FAHRPLANEINGABE:
			// schedule window closed?
			if(fpl!=NULL  &&  fpl->ist_abgeschlossen()) {

				set_schedule(fpl);

				if(fpl->get_count()==0) {
					// no entry => no route ...
					state = NO_ROUTE;
				}
				else {
					// Schedule changed at station
					// this station? then complete loading task else drive on
					halthandle_t h = haltestelle_t::get_halt( welt, get_pos(), get_besitzer() );
					if(  h.is_bound()  &&  h==haltestelle_t::get_halt( welt, fpl->get_current_eintrag().pos, get_besitzer() )  ) {
						if(  route.get_count()>0  &&  h==haltestelle_t::get_halt( welt, route.position_bei(route.get_count()-1), get_besitzer() )  ){
							state = get_pos()==route.position_bei(route.get_count()-1) ? LOADING : DRIVING;
							break;
						}
						else {
							if(  drive_to()  ) {
								state = DRIVING;
								break;
							}
						}
					}

					if(  fpl->get_current_eintrag().pos==get_pos()  ) {
						// position in depot: waiting
						grund_t *gr = welt->lookup(fpl->get_current_eintrag().pos);
						if(  gr  &&  gr->get_depot()  ) {
							state = INITIAL;
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
				vehikel_t* v = fahr[0];

				if(fpl->get_count()==0) {
					state = NO_ROUTE;
				}
				else {
					// check first, if we are already there:
					assert( fpl->get_aktuell()<fpl->get_count()  );
					if(  v->get_pos()==fpl->get_current_eintrag().pos  ) {
						fpl->advance();
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
			// stucked vehicles
			{
				vehikel_t* v = fahr[0];

				if(fpl->get_count()==0) {
					// no entries => no route ...
					get_besitzer()->bescheid_vehikel_problem(self, v->get_pos());
				}
				else {
					// Hajo: now calculate a new route
					drive_to();
				}
			}
			break;

		case CAN_START:
		case CAN_START_ONE_MONTH:
		case CAN_START_TWO_MONTHS:
			{
				vehikel_t* v = fahr[0];

				int restart_speed=-1;
				if (v->ist_weg_frei(restart_speed)) {
					// can reserve new block => drive on
					state = (steps_driven>=0) ? LEAVING_DEPOT : DRIVING;
					if(haltestelle_t::get_halt(welt,v->get_pos(),besitzer_p).is_bound()) {
						v->play_sound();
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
				int restart_speed=-1;
				if (fahr[0]->ist_weg_frei(restart_speed)) {
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

		case LOADING:
			laden();
			if (state != SELF_DESTRUCT)
			{
				if(get_depot_when_empty() && has_no_cargo())
				{
					go_to_depot(false);
				}
				break;
			}
			// no break. continue with case SELF_DESTRUCT.

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
			wait_lock = max(wait_lock, 500);
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
	uint16 base_comfort = 0;
	uint8 passenger_vehicles = 0;
	
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		if(fahr[i]->get_fracht_typ()->get_catg_index() == 0)
		{
			passenger_vehicles ++;
			base_comfort += fahr[i]->get_comfort();
		}
	}
	if(passenger_vehicles < 1)
	{
		return 0;
	}
	
	else if(passenger_vehicles > 1)
	{
		// Avoid division if possible
		base_comfort /= passenger_vehicles;
	}
	
	const uint8 catering_level = get_catering_level(0);
	switch(catering_level)
	{
	case 0:
		return base_comfort;
		break;

	case 1:
		return base_comfort + 5;
		break;

	case 2:
		return base_comfort + 10;
		break;

	case 3:
		return base_comfort + 16;
		break;

	case 4:
		return base_comfort + 20;
		break;

	default:
	case 5:
		return base_comfort + 25;
		break;
	};
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
	uint32 running_cost = 0;
	for(unsigned j=0;  j<get_vehikel_anzahl();  j++ ) 
	{
		running_cost += fahr[j]->get_besch()->get_fixed_maintenance(welt);
	}
	
	add_running_cost(welt->calc_adjusted_monthly_figure(running_cost));

	// everything normal: update history
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}

	for(uint8 i = 0; i < MAX_CONVOI_COST; i ++)
	{	
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}

	// check for traffic jam
	if(state==WAITING_FOR_CLEARANCE) {
		state = WAITING_FOR_CLEARANCE_ONE_MONTH;
		// check, if now free ...
		// migh also reset the state!
		int restart_speed=-1;
		if (fahr[0]->ist_weg_frei(restart_speed)) {
			state = DRIVING;
		}
		if(restart_speed>=0) {
			akt_speed = restart_speed;
		}
	}
	else if(state==WAITING_FOR_CLEARANCE_ONE_MONTH) {
		get_besitzer()->bescheid_vehikel_problem(self,koord3d::invalid);
		state = WAITING_FOR_CLEARANCE_TWO_MONTHS;
	}
	// check for traffic jam
	if(state==CAN_START) {
		state = CAN_START_ONE_MONTH;
	}
	else if(state==CAN_START_ONE_MONTH) {
		get_besitzer()->bescheid_vehikel_problem(self,koord3d::invalid);
		state = CAN_START_TWO_MONTHS;
	}
	// check for obsolete vehicles in the convoi
	if(!has_obsolete  &&  welt->use_timeline()) {
		has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
	}
}



void convoi_t::betrete_depot(depot_t *dep)
{
	// Hajo: remove vehicles from world data structure
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		grund_t* gr = welt->lookup(v->get_pos());
		if(gr) {
			// remove from blockstrecke
			v->set_letztes(true);
			v->verlasse_feld();
		}
	}

	dep->convoi_arrived(self, self->get_schedule()!=0);

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
			home_depot = route.position_bei(0);
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

		state = ROUTING_1;

		// recalc weight and image
		// also for any vehicle entered a depot, set_letztes is true! => reset it correctly
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->set_erstes( false );
			fahr[i]->set_letztes( false );
			fahr[i]->beladen( home_depot.get_2d(), halthandle_t() );
		}
		fahr[0]->set_erstes( true );
		fahr[anz_vehikel-1]->set_letztes( true );
		// do not show the vehicle - it will be wrong positioned -vorfahren() will correct this
		fahr[0]->set_bild(IMG_LEER);

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
			haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p, welt->get_einstellungen()->get_default_path_option());
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

	if(dp) {
		// ok, we are entering a depot
		cbuffer_t buf(256);
		if(reversed)
		{
			//Always enter a depot facing forward
			reversable = fahr[anz_vehikel - 1]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
			reverse_order(reversable);
		}

		// we still book the money for the trip; however, the frieght will be lost
		last_departure_time = welt->get_zeit_ms();

		akt_speed = 0;
		if (!replace || !replace->get_autostart()) {
			buf.printf( translator::translate("!1_DEPOT_REACHED"), get_name() );
			welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::convoi, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);
		}

		home_depot=v->get_pos();
		betrete_depot(dp);
	}
	else {
		// no depot reached, check for stop!
		halthandle_t halt = haltestelle_t::get_halt(welt, v->get_pos(),besitzer_p);
		if(  halt.is_bound() &&  gr->get_weg_ribi(v->get_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			akt_speed = 0;
			halt->book(1, HALT_CONVOIS_ARRIVED);
			state = LOADING;
			go_on_ticks = WAIT_INFINITE;	// we will eventually wait from now on
		}
		else {
			// Neither depot nor station: waypoint
			fpl->advance();
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
 * Wartet bis Fahrzeug 0 freie Fahrt meldet
 * Wait until the vehicle returns 0 free ride (Google)
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
		akt_speed = restart_speed;
	}
}



bool
convoi_t::add_vehikel(vehikel_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehikel()","at pos %i of %i totals.",anz_vehikel,max_vehicle);
	// extend array if requested (only needed for trains)
	if(anz_vehikel == max_vehicle) {
DBG_MESSAGE("convoi_t::add_vehikel()","extend array_tpl to %i totals.",max_rail_vehicle);
		//fahr.resize(max_rail_vehicle, NULL);
		fahr.resize(max_rail_vehicle);
	}
	// now append
	if (anz_vehikel < fahr.get_size()) {
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
		sum_leistung += info->get_leistung();
		//if(info->get_engine_type() == vehikel_besch_t::steam)
		//{
		//	power_from_steam += info->get_leistung();
		//	power_from_steam_with_gear += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
		//}
		sum_gear_und_leistung += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
		sum_gewicht += info->get_gewicht();
		min_top_speed = min( min_top_speed, kmh_to_speed( v->get_besch()->get_geschw() ) );
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;
		// Add good_catg_index:
		if(v->get_fracht_max() != 0) {
			const ware_besch_t *ware=v->get_fracht_typ();
			if(ware!=warenbauer_t::nichts  ) {
				goods_catg_index.append_unique( ware->get_catg_index(), 1 );
			}
		}
		// check for obsolete
		if(!has_obsolete  &&  welt->use_timeline()) {
			has_obsolete = v->get_besch()->is_retired( welt->get_timeline_year_month() );
		}
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

	heaviest_vehicle = calc_heaviest_vehicle();
	longest_loading_time = calc_longest_loading_time();

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

	min_top_speed = calc_min_top_speed(fahr, anz_vehikel);
	
	// Add power and weight of the new vehicle
	sum_leistung += info->get_leistung();
	//if(info->get_engine_type() == vehikel_besch_t::steam)
	//{
	//	power_from_steam += info->get_leistung();
	//	power_from_steam_with_gear += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
	//}
	sum_gear_und_leistung += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
	sum_gewicht += info->get_gewicht();
	sum_gesamtgewicht = sum_gewicht;

	// Remove power and weight of the old vehicle
	info = old_vehicle->get_besch();
	sum_leistung -= info->get_leistung();
	//if(info->get_engine_type() == vehikel_besch_t::steam)
	//{
	//	power_from_steam -= info->get_leistung();
	//	power_from_steam_with_gear -= info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
	//}
	sum_gear_und_leistung -= info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
	sum_gewicht -= info->get_gewicht();

	calc_loading();
	freight_info_resort = true;
	recalc_catg_index();

	// check for obsolete
	if(has_obsolete) 
	{
		has_obsolete = calc_obsolescence(welt->get_timeline_year_month());
	}

	// der convoi hat jetzt ein neues ende
	set_erstes_letztes();

	heaviest_vehicle = calc_heaviest_vehicle();
	longest_loading_time = calc_longest_loading_time();
	
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

			const vehikel_besch_t *info = v->get_besch();
			sum_leistung -= info->get_leistung();
			//if(info->get_engine_type() == vehikel_besch_t::steam)
			//{
			//	power_from_steam -= info->get_leistung();
			//	power_from_steam_with_gear -= info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
			//}
			sum_gear_und_leistung -= info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
			sum_gewicht -= info->get_gewicht();
		}
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(anz_vehikel > 0) {
			set_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

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

	heaviest_vehicle = calc_heaviest_vehicle();
	longest_loading_time = calc_longest_loading_time();

	return v;
}

// recalc what good this convoy is moving
void convoi_t::recalc_catg_index()
{
	// first copy old
	minivec_tpl<uint8> old_goods_catg_index(goods_catg_index.get_count());
	for(  uint i=0;  i<goods_catg_index.get_count();  i++  ) {
		old_goods_catg_index.append( goods_catg_index[i] );
	}
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
			goods_catg_index.append_unique( ware->get_catg_index(), 1 );
		}
	}
	/* since during composition of convois all kinds of composition could happen,
	 * we do not enforce schedule recalculation here; it will be done anyway all times when leaving the INTI state ...
	 */
#if 0
	// if different => schedule need recalculation
	if(  goods_catg_index.get_count()!=old_goods_catg_index.get_count()  ) {
		// surely changed
		welt->set_schedule_counter();
	}
	else {
		// maybe changed => must test all entries
		for(  uint i=0;  i<goods_catg_index.get_count();  i++  ) {
			if(  !old_goods_catg_index.is_contained(goods_catg_index[i])  ) {
				// different => recalc
				welt->set_schedule_counter();
				break;
			}
		}
	}
#endif
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



bool convoi_t::set_schedule(schedule_t * f)
{
	if(  state==SELF_DESTRUCT  ) {
		return false;
	}

	enum states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::set_schedule()", "new=%p, old=%p", f, fpl);

	if(f == NULL) {
		if(  line.is_bound()  ) {
			unset_line();
		}
		if(  state==INITIAL  ) {
			delete fpl;
			fpl = NULL;
			return true;
		}
		return false;
	}

	if(!line.is_bound() && old_state != INITIAL)
	{
		// New method - recalculate as necessary

		// Added by : Knightly
		if ( fpl == f && old_fpl )	// Case : Schedule window of operating convoy
		{
			if ( !old_fpl->matches(welt, fpl) )
			{
				haltestelle_t::refresh_routing(old_fpl, goods_catg_index, besitzer_p, 0);
				haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p, welt->get_einstellungen()->get_default_path_option());
			}
		}
		else
		{
			if (fpl != f)
			{
				haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p, 0);
			}
			haltestelle_t::refresh_routing(f, goods_catg_index, besitzer_p, welt->get_einstellungen()->get_default_path_option());
		}
	}
	
	// happens to be identical?
	if(fpl!=f) {
		// now check, we we have been bond to a line we are about to loose:
		if(  line.is_bound()  &&  !f->matches( welt, line->get_schedule() )  ) {
			unset_line();
		}
		// destroy a possibly open schedule window
		if(fpl &&  !fpl->ist_abgeschlossen()) 
		{ 
			//"is completed" (Google)
			destroy_win((long)fpl);
			delete fpl;
		}
		fpl = f;
	}

	// remove wrong freight
	for(unsigned i=0; i<anz_vehikel; i++) 
	{
		fahr[i]->remove_stale_freight();
	}

	// ok, now we have a schedule
	if(old_state != INITIAL) 
	{
		state = FAHRPLANEINGABE;
	}
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
	ribi_t::ribi neue_richtung_rwr = ribi_t::rueckwaerts(fahr[0]->calc_richtung(route.position_bei(0).get_2d(), route.position_bei(min(2,route.get_count()-1)).get_2d()));
//	DBG_MESSAGE("convoi_t::go_alte_richtung()","neu=%i,rwr_neu=%i,alt=%i",neue_richtung_rwr,ribi_t::rueckwaerts(neue_richtung_rwr),alte_richtung);
	if(neue_richtung_rwr&alte_richtung) {
		akt_speed = 8;
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


		convoi_length += v->get_besch()->get_length();

		if(gr==NULL  ||  (pred!=NULL  &&  (abs(v->get_pos().x-pred->get_pos().x)>=2  ||  abs(v->get_pos().y-pred->get_pos().y)>=2))  ) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_richtung()","broken convoy (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}

		// now check, if ribi is straight and train is not
		ribi_t::ribi weg_ribi = gr->get_weg_ribi_unmasked(v->get_waytype());
		if(ribi_t::ist_gerade(weg_ribi)  &&  (weg_ribi|v->get_fahrtrichtung())!=weg_ribi) {
			dbg->warning("convoi_t::go_alte_richtung()","convoy with wrong vehicle directions (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
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
		akt_speed = 8;
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
				akt_speed = 8;
				return false;
			}
		}
	}

//DBG_MESSAGE("convoi_t::go_alte_richtung()","alte=%d, neu_rwr=%d",alte_richtung,neue_richtung_rwr);

	// we continue our journey; however later cars need also a correct route entry
	// eventually we need to add their positions to the convoi's route
	koord3d pos = fahr[0]->get_pos();
	assert(pos==route.position_bei(0));
	if(welt->lookup(pos)->get_depot()) {
		return false;
	}
	else {
		for(i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];
			// eventually add current position to the route
			if(route.position_bei(0)!=v->get_pos()  &&  route.position_bei(1)!=v->get_pos()) {
				route.insert(v->get_pos());
			}
			// eventually we need to add also a previous position to this path
			if(v->get_besch()->get_length()>8  &&  i+1<anz_vehikel) {
				if(route.position_bei(0)!=v->get_pos_prev()  &&  route.position_bei(1)!=v->get_pos_prev()) {
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

	uint16 reverse_delay = 0;

	set_akt_speed_soll( vehikel_t::SPEED_UNLIMITED );

	koord3d k0 = route.position_bei(0);
	grund_t *gr = welt->lookup(k0);
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
				schiene_t * sch0 = dynamic_cast<schiene_t *>( gr->get_weg(fahr[i]->get_waytype()) );
				if(sch0) {
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
			fahr[i]->fahre_basis( 128<<12 );
		}
		v0->darf_rauchen(true);
		v0->set_erstes(true); // switches on signal checks to reserve the next route

		// until all other are on the track
		state = CAN_START;
	}
	else {
		// still leaving depot (steps_driven!=0) or going in other direction or misalignment?
		if(  steps_driven>0  ||  !can_go_alte_richtung()  ) 
		{

			//Convoy needs to reverse
			//@author: jamespetts
			if(!can_go_alte_richtung())
			{
				switch(fahr[0]->get_waytype())
				{
					case road_wt:
					case air_wt:
						//Road vehicles and aircraft do not need to change direction
						//Canal barges *may* change direction, so water is omitted.
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

						if(reversable)
						{
							//Multiple unit or similar: quick reverse
							reverse_delay = welt->get_einstellungen()->get_unit_reverse_time();
						}
						else if(fahr[0]->get_besch()->is_bidirectional())
						{
							//Loco hauled, no turntable.
							reverse_delay = welt->get_einstellungen()->get_hauled_reverse_time();
						}
						else
						{
							//Locomotive needs turntable: slow reverse
							reverse_delay = welt->get_einstellungen()->get_turntable_reverse_time();
						}

						reverse_order(reversable);
						state = REVERSING;
				}
			}

			// since start may have been changed
			k0 = route.position_bei(0);

			for(unsigned i=0; i<anz_vehikel; i++) {

				vehikel_t *v = fahr[i];
				steps_driven = -1;
				grund_t* gr = welt->lookup(v->get_pos());
				if(gr) {
					v->mark_image_dirty( v->get_bild(), v->get_hoff() );
					v->verlasse_feld(); //"leave field" (Google)
					// eventually unreserve this
					schiene_t * sch0 = dynamic_cast<schiene_t *>( gr->get_weg(fahr[i]->get_waytype()) );
					if(sch0) {
						sch0->unreserve(v);
					}
				}
				/* we will set by this method the pos_prev to the starting point;
				 * otherwise it may be elsewhere, especially on curves and with already
				 * broken convois.
				 */
				v->set_pos(k0);
				v->neue_fahrt(0, true);
				gr=welt->lookup(v->get_pos());
				if(gr) {
					v->set_pos(k0);
					v->betrete_feld(); //"enter field" (Google)
				}
			}

			// move one train length to the start position ...
			int train_length = 0;
			for(unsigned i=0; i<anz_vehikel-1u; i++) 
			{
				train_length += fahr[i]->get_besch()->get_length(); // this give the length in 1/TILE_STEPS of a full tile
			}
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->get_direction_of_travel();
			if(neue_richtung==ribi_t::sued  ||  neue_richtung==ribi_t::ost)
			{
				train_length += fahr[anz_vehikel-1]->get_besch()->get_length();
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
					fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 ); //"fahre" = "go" (Google)
					train_length += (v->get_besch()->get_length());	// this give the length in 1/TILE_STEPS of a full tile => all cars closely coupled!
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
					fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 ); //"fahre" = "go" (Google)
					train_length -= (v->get_besch()->get_length());	// this give the length in 1/TILE_STEPS of a full tile => all cars closely coupled!
					v->darf_rauchen(true);
				}
					
			}
			fahr[0]->set_erstes(true);
		}

		if(state != REVERSING)
		{
			state = CAN_START;
		}

		// to advance more smoothly
		int restart_speed=-1;
		if(fahr[0]->ist_weg_frei(restart_speed)) {
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

	// finally reserve route (if needed)
	if(  fahr[0]->get_waytype()!=air_wt  ) {
		// do not prereserve for airplanes
		for(unsigned i=0; i<anz_vehikel; i++) {
			// eventually reserve this
			schiene_t * sch0 = dynamic_cast<schiene_t *>( welt->lookup(fahr[i]->get_pos())->get_weg(fahr[i]->get_waytype()) );
			if(sch0) {
				sch0->reserve(self,ribi_t::keine);
			}
			else {
				break;
			}
		}
	}

	wait_lock = reverse_delay;
	INT_CHECK("simconvoi 711");
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

		for(uint8 i = 1; i < anz_vehikel; i++)
		{
			if(fahr[i]->get_besch()->get_leistung() > 0)
			{
				a++;
			}
		}
	}

	fahr[anz_vehikel - 1]->set_letztes(false);
    
    for(a; a<--b; a++) //increment a and decrement b until they meet each other
    {
		reverse = fahr[a];		//put what's in a into swap space
        fahr[a] = fahr[b];		//put what's in b into a
        fahr[b] = reverse;		//put what's in the swap (a) into b
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



void
convoi_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "convoi_t" );

	sint32 dummy;
	sint32 besitzer_n = welt->sp2num(besitzer_p);

	if(file->is_saving()) {
		if(  file->get_version()<101000  ) {
			file->wr_obj_id("Convoi");
		}
		line_id = line.is_bound() ? line->get_line_id() : INVALID_LINE_ID;
	}

	// for new line management we need to load/save the assigned line id
	// @author hsiegeln
	if(file->get_version()<88003) {
		dummy = 0;
		file->rdwr_long(dummy, " ");
		line_id = (uint16)dummy;
	}
	else {
		file->rdwr_short(line_id, " ");
	}

	dummy = anz_vehikel;
	file->rdwr_long(dummy, " ");
	anz_vehikel = (uint8)dummy;

	if(file->get_version()<99014) {
		// was anz_ready
		file->rdwr_long(dummy, " ");
	}

	file->rdwr_long(wait_lock, " ");
	// some versions may produce broken savegames apparently
	if(wait_lock > 60000) {
		dbg->warning("convoi_t::sync_prepre()","Convoi %d: wait lock out of bounds: wait_lock = %d, setting to 60000",self.get_id(), wait_lock);
		wait_lock = 60000;
	}

	bool dummy_bool=false;
	file->rdwr_bool(dummy_bool, " ");
	file->rdwr_long(besitzer_n, "\n");
	file->rdwr_long(akt_speed, " ");
	file->rdwr_long(akt_speed_soll, " ");
	file->rdwr_long(sp_soll, " ");
	file->rdwr_enum(state, " ");
	file->rdwr_enum(alte_richtung, " ");

	// read the yearly income (which has since then become a 64 bit value)
	// will be recalculated later directly from the history
	if(file->get_version()<=89003) {
		file->rdwr_long(dummy, "\n");
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
	}

	file->rdwr_str(name_and_id+name_offset,116);
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
				sum_leistung += info->get_leistung();
				//if(info->get_engine_type() == vehikel_besch_t::steam)
				//{
				//	power_from_steam += info->get_leistung();
				//	power_from_steam_with_gear += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
				//}
				sum_gear_und_leistung += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
				sum_gewicht += info->get_gewicht();
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
						dbg->fatal("convoi_t::rdwr()", "invalid position %s for vehicle %s in state %d (setting to %i,%i,%i)", v->get_pos().get_str(), v->get_name(), state, gr->get_pos().x, gr->get_pos().y, gr->get_pos().z );
					}
					state = INITIAL;
				}
				// add to blockstrecke "block stretch" (Google). Possibly "block section"?
				if(v->get_waytype()==track_wt  ||  v->get_waytype()==monorail_wt  ||  v->get_waytype()==maglev_wt  ||  v->get_waytype()==narrowgauge_wt) {
					schiene_t* sch = (schiene_t*)gr->get_weg(v->get_waytype());
					if(sch) {
						sch->reserve(self,ribi_t::keine);
					}
					// add to crossing
					if(gr->ist_uebergang()) {
						gr->find<crossing_t>()->add_to_crossing(v);
					}
				}
				gr->obj_add(v);
			}

			// add to convoi
			fahr[i] = v;
		}
		sum_gesamtgewicht = sum_gewicht;
	}

	bool has_fpl = (fpl != NULL);
	file->rdwr_bool(has_fpl, "");
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
	}

	// Hajo: calculate new minimum top speed
	min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

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
				if((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1)
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values.
					financial_history[k][j] = 0;
					continue;
				}
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
		for (j = 2; j < 5; j++) 
		{
			for (int k = MAX_MONTHS-1; k >= 0; k--) 
			{
				if((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1)
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values.
					financial_history[k][j] = 0;
					continue;
				}
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			financial_history[k][CONVOI_DISTANCE] = 0;
		}
	}
//BG: superfluous 102002 check:	else if(file->get_version() <= 102002 || (file->get_version() < 102002 && file->get_experimental_version() < 7))
	else if(file->get_version() <= 102002 || file->get_experimental_version() < 7)
	{
		// load statistics
		for (int j = 0; j<7; j++) 
		{
			for (int k = MAX_MONTHS-1; k>=0; k--) 
			{
				if((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1)
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values.
					financial_history[k][j] = 0;
					continue;
				}
				file->rdwr_longlong(financial_history[k][j], " ");
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
			for (int k = MAX_MONTHS-1; k >= 0; k--) 
			{
				if((j == CONVOI_AVERAGE_SPEED || j == CONVOI_COMFORT) && file->get_experimental_version() <= 1)
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values.
					financial_history[k][j] = 0;
					continue;
				}

				else if(j == CONVOI_DISTANCE && file->get_experimental_version() < 7)
				{
					// Simutrans-Standard: distances in tiles, not km. Convert.
					sint64 distance;
					file->rdwr_longlong(distance, " ");
					financial_history[k][j] = (double)distance * welt->get_einstellungen()->get_distance_per_tile();
					continue;
				}
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}

	// the convoi odometer
	if(file->get_version() >= 102003 && file->get_experimental_version() >= 7)
	{
		file->rdwr_longlong( total_distance_traveled, "" );
	}
	else if(file->get_version() > 102002)
	{
		//Simutrans-Standard save - this value is in tiles, not km. Convert.
		sint64 tile_distance;
		file->rdwr_longlong( tile_distance, "" );
		total_distance_traveled = (double)tile_distance * welt->get_einstellungen()->get_distance_per_tile();
	}

	if(file->get_version() >= 102003 && file->get_experimental_version() >= 7)
	{
		file->rdwr_byte(tiles_since_last_odometer_increment, "");
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
		file->rdwr_long(dummy, "\n");	// ignore
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
		last_stop_pos = !route.empty() ? route.position_bei(0) : (anz_vehikel > 0 ? fahr[0]->get_pos() : koord3d(0, 0, 0));
	}

	// for leaving the depot routine
	if(file->get_version()<99014) {
		steps_driven = -1;
	}
	else {
		file->rdwr_short( steps_driven, "s" );
	}

	// waiting time left ...
	if(file->get_version()>=99017) 
	{
		if(file->is_saving()) 
		{
			if(go_on_ticks==WAIT_INFINITE) 
			{
				if(file->get_experimental_version() <= 1)
				{
					uint32 old_go_on_ticks = (uint32)go_on_ticks;
					file->rdwr_long( old_go_on_ticks, "dt" );
				}
				else
				{
					file->rdwr_longlong(go_on_ticks, "dt" );
				}
			}
			else 
			{
				sint64 diff_ticks = welt->get_zeit_ms()>go_on_ticks ? 0 : go_on_ticks-welt->get_zeit_ms();
				file->rdwr_longlong(diff_ticks, "dt" );
			}
		}
		else 
		{
			if(file->get_experimental_version() <= 1)
			{
				uint32 old_go_on_ticks = (uint32)go_on_ticks;				
				file->rdwr_long( old_go_on_ticks, "dt" );
				go_on_ticks = old_go_on_ticks;
			}
			else
			{
				file->rdwr_longlong(go_on_ticks, "dt" );
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
		file->rdwr_byte( tiles_overtaking, "o" );
		set_tiles_overtaking( tiles_overtaking );
	}
	


	// no_load, withdraw
	if(file->get_version()<102001) {
		no_load = false;
		withdraw = false;
	}
	else {
		file->rdwr_bool( no_load, "" );
		file->rdwr_bool( withdraw, "" );
	}
	
	// Simutrans-Experimental specific parameters. 
	// Must *always* go after standard parameters.

	heaviest_vehicle = calc_heaviest_vehicle();
	longest_loading_time = calc_longest_loading_time();

	if(file->get_experimental_version() >= 1)
	{
		file->rdwr_bool(reversed, "");
		
		//Replacing settings
		// BG, 31-MAR-2010: new replacing code starts with exp version 8:
		bool is_replacing = replace && (file->get_experimental_version() >= 8);
		file->rdwr_bool(is_replacing, "");

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
			file->rdwr_bool(depot_when_empty, "");
		}
		else
		{
			// Original vehicle replacing settings - stored in convoi_t.
			bool old_autostart;
			file->rdwr_bool(old_autostart, "");
			file->rdwr_bool(depot_when_empty, "");

			uint16 replacing_vehicles_count = 0;

			vector_tpl<const vehikel_besch_t *> *replacing_vehicles;

			if(file->is_saving())
			{
				// BG, 31-MAR-2010: new replacing code starts with exp version 8.
				// BG, 31-MAR-2010: to keep compatibility with exp versions < 8 
				//  at least the number of replacing vehicles (always 0) must be written. 
				//replacing_vehicles = replace->get_replacing_vehicles();
				//replacing_vehicles_count = replacing_vehicles->get_count();
				file->rdwr_short(replacing_vehicles_count, "");
				//ITERATE_PTR(replacing_vehicles, i)
				//{
				//	const char *s = replacing_vehicles->get_element(i)->get_name();
				//	file->rdwr_str(s);
				//}
			}
			else
			{
				file->rdwr_short(replacing_vehicles_count, "");
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
					//replace = new replace_data_t();
					//replace->set_autostart(old_autostart);
					//replace->set_replacing_vehicles(replacing_vehicles);
				}
			}
		}
	}
	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_longlong(last_departure_time, "");
		const uint8 count = file->get_version() < 103000 ? CONVOI_DISTANCE : MAX_CONVOI_COST;
		for(uint8 i = 0; i < count; i ++)
		{	
			file->rdwr_long(rolling_average[i], "");
			file->rdwr_short(rolling_average_count[i], "");
		}		
	}
	else
	{
		// For loading older games, assume that the convoy
		// left twenty seconds ago, to avoid anomalies when
		// measuring average speed. 
		last_departure_time = welt->get_zeit_ms() - 20000;
	}

	// This must come *after* all the loading/saving.
	if( file->is_loading() ) {
		recalc_catg_index();
	}
}


void convoi_t::zeige_info()
{
	if(!in_depot()) {

		if(umgebung_t::verbose_debug) {
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

		sprintf(tmp, "\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->get_besch()->get_geschw(), get_running_cost() / 100.0F);
		buf.append(tmp);

		sprintf(tmp," %s: %ikW\n", translator::translate("Leistung"), sum_leistung );
		buf.append(tmp);

		sprintf(tmp," %s: %i (%i) t\n", translator::translate("Gewicht"), sum_gewicht, sum_gesamtgewicht-sum_gewicht );
		buf.append(tmp);

		sprintf(tmp," %s: ", translator::translate("Gewinn")  );
		buf.append(tmp);

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



//caches the last info; resorts only when needed
void convoi_t::get_freight_info(cbuffer_t & buf)
{
	if(freight_info_resort) {
		freight_info_resort = false;
		// rebuilt the list with goods ...
		vector_tpl<ware_t> total_fracht;

		ALLOCA(uint32, max_loaded_waren, warenbauer_t::get_waren_anzahl());
		memset( max_loaded_waren, 0, sizeof(uint32)*warenbauer_t::get_waren_anzahl() );

		unsigned i;
		for(i=0; i<anz_vehikel; i++) {
			const vehikel_t* v = fahr[i];

			// first add to capacity indicator
			const ware_besch_t* ware_besch = v->get_besch()->get_ware();
			const uint16 menge = v->get_besch()->get_zuladung();
			if(menge>0  &&  ware_besch!=warenbauer_t::nichts) {
				max_loaded_waren[ware_besch->get_index()] += menge;
			}

			// then add the actual load
			slist_iterator_tpl<ware_t> iter_vehicle_ware(v->get_fracht());
			while(iter_vehicle_ware.next()) 
			{
				ware_t ware = iter_vehicle_ware.get_current();
				ITERATE(total_fracht, i)
				{

					// could this be joined with existing freight?
					ware_t &tmp = total_fracht[i];

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

		// append info on total capacity
		slist_tpl<ware_t> capacity;
		for(i=0;  i<warenbauer_t::get_waren_anzahl();  i++  ) {
			if(max_loaded_waren[i]>0  &&  i!=warenbauer_t::INDEX_NONE) {
				ware_t ware(warenbauer_t::get_info(i));
				ware.menge = max_loaded_waren[i];
				if(ware.get_catg()==0) {
					capacity.append( ware );
				} else {
					// append to category?
					slist_tpl<ware_t>::iterator j  = capacity.begin();
					slist_tpl<ware_t>::iterator end = capacity.end();
					while (j != end && j->get_catg() < ware.get_catg()) ++j;
					if (j != end && j->get_catg() == ware.get_catg()) {
						j->menge += max_loaded_waren[i];
					} else {
						// not yet there
						capacity.insert(j, ware);
					}
				}
			}
		}

		// show new info
		freight_list_sorter_t::sort_freight(&total_fracht, buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, &capacity, "loaded");
	}
}


void convoi_t::open_schedule_window( bool show )
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

	// manipulation of schedule not allowd while:
	// - just starting
	// - a line update is pending
	if(  (state==FAHRPLANEINGABE  ||  line_update_pending.is_bound())  &&  get_besitzer()==welt->get_active_player()  ) {
		create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		return;
	}

	if(state==DRIVING) {
		// book the current value of goods
		last_departure_time = welt->get_zeit_ms();
	}

	akt_speed = 0;	// stop the train ...
	state = FAHRPLANEINGABE;
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
		create_win( new fahrplan_gui_t(fpl,get_besitzer(),self), w_info, (long)fpl );
		// TODO: what happens if no client opens the window??
	}
	fpl->eingabe_beginnen();
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Nachfolger von
 * vor gelten - daher muß vor != NULL sein..
 * TRANSLATION: (Google)
 * Vehicles are often triggered only in certain combinations restrictions are approved,
 * the successor to continue to apply - therefore must be done before != NULL.
 */
bool convoi_t::pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
	const vehikel_besch_t *soll;

	if(!vor->get_nachfolger_count()) {
		// Alle Nachfolger erlaubt
		return true;
	}
	for(int i=0; i < vor->get_nachfolger_count(); i++) {
		soll = vor->get_nachfolger(i);
		//DBG_MESSAGE("convoi_t::pruefe_an_index()",
		//    "checking successor: should be %d, is %d",
		//    soll ? soll->get_name() : "none",
		//    hinter ? hinter->get_name() : "none");

		if(hinter == soll) {
			// Diese Beschränkung erlaubt unseren Nachfolger
			// This restriction allows our successors (Google translations)
			return true;
		}
	}
	//DBG_MESSAGE("convoi_t::pruefe_an_index()",
	//		 "No matching successor found.");
	return false;
}

/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Vorgänger von
 *  hinter gelten - daher muß hinter != NULL sein.
 * 
 * Vehicles are often only fit in certain combinations, 
 * the restrictions are approved, the predecessor of 
 * behind apply - must be behind! = NULL. (Google)
 */
bool convoi_t::pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
	const vehikel_besch_t *soll; //"Soll" = should (Google)

	if(!hinter->get_vorgaenger_count()) {
		// Alle Vorgänger erlaubt
		// "All previous permits" (Google).
		return true;
	}
	for(int i=0; i < hinter->get_vorgaenger_count(); i++) {
		soll = hinter->get_vorgaenger(i);
		//DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
		//	     "checking predecessor: should be %s, is %s",
		//	     soll ? soll->get_name() : "none",
		//	     vor ? vor->get_name() : "none");

		if(vor == soll) {
			// Diese Beschränkung erlaubt unseren Vorgänger
			// 	This restriction allows our predecessors (Google)
			return true;
		}
	}
	//DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
	//		 "No matching predecessor found.");
	return false;
}


bool convoi_t::pruefe_alle() //"examine all" (Babelfish)
{
	bool ok = (anz_vehikel == 0 || pruefe_vorgaenger(NULL, fahr[0]->get_besch()));
	unsigned i;

	const vehikel_t* pred = fahr[0];
	for(i = 1; ok && i < anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		ok = pruefe_nachfolger(pred->get_besch(), v->get_besch()) &&
				 pruefe_vorgaenger(pred->get_besch(), v->get_besch());
		pred = v;
	}
	if(ok) {
		ok = pruefe_nachfolger(pred->get_besch(), NULL);
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
	//Calculate average speed
	//@author: jamespetts
	const uint32 journey_distance = accurate_distance(fahr[0]->get_pos().get_2d(), fahr[0]->last_stop_pos);
	
	if(current_stop != fpl->get_aktuell())
	{
		const double journey_time = (welt->get_zeit_ms() - last_departure_time) / 4096.0F;
		const uint16 average_speed = ((double)journey_distance / journey_time) * 20.0;
		book(average_speed, CONVOI_AVERAGE_SPEED);
		last_departure_time = welt->get_zeit_ms();
		
		// Recalculate comfort
		book(get_comfort(), CONVOI_COMFORT);
	}

	for(uint8 i = 0; i < anz_vehikel; i++)
	{
		// Accumulate distance 
		slist_iterator_tpl<ware_t> iter_cargo(fahr[i]->get_fracht());
		while(iter_cargo.next())
		{
			iter_cargo.access_current().add_distance(journey_distance);
		}
	}

	if(state == FAHRPLANEINGABE) //"ENTER SCHEDULE" (Google)
	{ 
		return;
	}

	halthandle_t halt = haltestelle_t::get_halt(welt, fpl->get_current_eintrag().pos,besitzer_p);
	// eigene haltestelle ?
	// "own stop?" (Babelfish)
	if (halt.is_bound()) 
	{
		const koord k = fpl->get_current_eintrag().pos.get_2d(); //"eintrag" = "entry" (Google)
		const spieler_t* owner = halt->get_besitzer(); //"get owner" (Google)
		if(  owner == get_besitzer()  ||  owner == welt->get_spieler(1)  ) 
		{
			// loading/unloading ...
			hat_gehalten(k, halt);
		}
	}

	if(go_on_ticks == WAIT_INFINITE && fpl->get_current_eintrag().waiting_time_shift > 0) 
	{
		go_on_ticks = welt->get_zeit_ms() + (welt->ticks_per_tag >> (16-fpl->get_current_eintrag().waiting_time_shift));
	}

	// Nun wurde ein/ausgeladen werden
	// "Now, a / unloaded" (Google)
	if(loading_level >= loading_limit  ||  no_load  ||  welt->get_zeit_ms() > go_on_ticks)  
	{

		// This is the minimum time it takes for loading
		wait_lock = longest_loading_time;

		if(withdraw  &&  loading_level==0) {
			// destroy when empty
			self_destruct();
			return;
		}

		// add available capacity after loading(!) to statistics
		for (uint8 i = 0; i < anz_vehikel; i++) 
		{
			book(get_vehikel(i)->get_fracht_max()-get_vehikel(i)->get_fracht_menge(), CONVOI_CAPACITY);
		}

		// Advance schedule
		fpl->advance();
		state = ROUTING_1;
	}

	else {
		// just wait a little longer to get maximum load ...
		wait_lock = (longest_loading_time*2)+(self.get_id())%1024;
	}
}

sint64 convoi_t::calc_revenue(ware_t& ware)
{
	float average_speed;
	
	if(!line.is_bound())
	{
		// No line - must use convoy
		if(financial_history[1][CONVOI_AVERAGE_SPEED] < 1)
		{
			average_speed = financial_history[0][CONVOI_AVERAGE_SPEED];
		}
		else
		{	
			average_speed = financial_history[1][CONVOI_AVERAGE_SPEED];
		}
	}

	else
	{
		if(line->get_finance_history(1, LINE_AVERAGE_SPEED) < 1)
		{
			average_speed = line->get_finance_history(0, LINE_AVERAGE_SPEED);
		}
		else
		{	
			average_speed = line->get_finance_history(1, LINE_AVERAGE_SPEED);
		}
	}

	// Cannot not charge for journey if the journey distance is more than a certain proportion of the straight line distance.
	// This eliminates the possibility of cheating by building circuitous routes, or the need to prevent that by always using
	// the straight line distance, which makes the game difficult and unrealistic. 
	// If the origin has been deleted since the packet departed, then the best that we can do is guess by
	// trebling the distance to the last stop.
	const uint32 max_distance = ware.get_origin().is_bound() ? accurate_distance(ware.get_origin()->get_basis_pos(), fahr[0]->get_pos().get_2d()) * 2.2 : 3 * accurate_distance(last_stop_pos.get_2d(), fahr[0]->get_pos().get_2d());
	const uint32 distance = ware.get_accumulated_distance();
	const uint32 revenue_distance = distance < max_distance ? distance : max_distance;

	ware.reset_accumulated_distance();

	//Multiply by a factor (default: 0.3) to ensure that it fits the scale properly. Journey times can easily appear too long.
	uint16 journey_minutes = (((float)distance / average_speed) * welt->get_einstellungen()->get_distance_per_tile() * 60.0F);

	const ware_besch_t* goods = ware.get_besch();
	const uint16 price = goods->get_preis();
	const sint32 min_price = price << 7;
	const uint16 speed_bonus_rating = calc_adjusted_speed_bonus(goods->get_speed_bonus(), distance);
	const sint32 ref_speed = welt->get_average_speed( fahr[0]->get_besch()->get_waytype() );
	const sint32 speed_base = (100 * average_speed) / ref_speed - 100;
	const sint32 base_bonus = (price * (1000 + speed_base * speed_bonus_rating));
	const sint64 revenue = (sint64)(min_price > base_bonus ? min_price : base_bonus) * (sint64)revenue_distance * (sint64)ware.menge;
	sint64 final_revenue = revenue;

	const float happy_ratio = ware.get_origin().is_bound() ? ware.get_origin()->get_unhappy_proportion(1) : 1;
	if(speed_bonus_rating > 0 && happy_ratio > 0)
	{
		// Reduce revenue if the origin stop is crowded, if speed is important for the cargo.
		sint64 tmp = ((float)speed_bonus_rating / 100.0F) * revenue;
		tmp *= (happy_ratio * 2);
		final_revenue -= tmp;
	}
	
	if(final_revenue && ware.is_passenger())
	{
		//Passengers care about their comfort
		const uint8 tolerable_comfort = calc_tolerable_comfort(journey_minutes);
		
		// Comfort matters more the longer the journey.
		// @author: jamespetts, March 2010
		float comfort_modifier;
		if(journey_minutes <= welt->get_einstellungen()->get_tolerable_comfort_short_minutes())
		{
			comfort_modifier = 0.2F;
		}
		else if(journey_minutes >= welt->get_einstellungen()->get_tolerable_comfort_median_long_minutes())
		{
			comfort_modifier = 1.0F;
		}
		else
		{
			const uint8 differential = journey_minutes - welt->get_einstellungen()->get_tolerable_comfort_short_minutes();
			const uint8 max_differential = welt->get_einstellungen()->get_tolerable_comfort_median_long_minutes() - welt->get_einstellungen()->get_tolerable_comfort_short_minutes();
			const float proportion = (float)differential / (float)max_differential;
			comfort_modifier = (0.8F * proportion) + 0.2F;
		}

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

		if(comfort > tolerable_comfort)
		{
			// Apply luxury bonus
			const uint8 max_differential = welt->get_einstellungen()->get_max_luxury_bonus_differential();
			const uint8 differential = comfort - tolerable_comfort;
			const float multiplier = welt->get_einstellungen()->get_max_luxury_bonus() * comfort_modifier;
			if(differential >= max_differential)
			{
				final_revenue += (revenue * multiplier);
			}
			else
			{
				const float proportion = (float)differential / (float)max_differential;
				final_revenue += revenue * (multiplier * proportion);
			}
		}
		else if(comfort < tolerable_comfort)
		{
			// Apply discomfort penalty
			const uint8 max_differential = welt->get_einstellungen()->get_max_discomfort_penalty_differential();
			const uint8 differential = tolerable_comfort - comfort;
			float multiplier = welt->get_einstellungen()->get_max_discomfort_penalty() * comfort_modifier;
			multiplier = multiplier < 0.95F ? multiplier : 0.95F;
			if(differential >= max_differential)
			{
				final_revenue -= (revenue * multiplier);
			}
			else
			{
				const float proportion = (float)differential / (float)max_differential;
				final_revenue -= revenue * (multiplier * proportion);
			}
		}
		
		// Do nothing if comfort == tolerable_comfort			
	}

	//Add catering or TPO revenue
	const uint8 catering_level = get_catering_level(ware.get_besch()->get_catg_index());
	if(catering_level > 0)
	{
		if(ware.is_mail())
		{
			// Mail
			if(journey_minutes >= welt->get_einstellungen()->get_tpo_min_minutes())
			{
				final_revenue += (welt->get_einstellungen()->get_tpo_revenue() * ware.menge);
			}
		}
		else if(ware.is_passenger())
		{
			// Passengers
			float proportion = 0.0F;
			// Knightly : Reorganised the switch cases to get rid of goto statements
			switch(catering_level)
			{

			case 5:
			default:
				if(journey_minutes >= welt->get_einstellungen()->get_catering_level4_minutes())
				{
					if(journey_minutes > welt->get_einstellungen()->get_catering_level5_minutes())
					{
						final_revenue += (welt->get_einstellungen()->get_catering_level5_max_revenue() * ware.menge);
						break;
					}
					
					proportion = (journey_minutes - welt->get_einstellungen()->get_catering_level4_max_revenue()) / (welt->get_einstellungen()->get_catering_level5_minutes() - welt->get_einstellungen()->get_catering_level4_minutes());
					final_revenue += (proportion * (welt->get_einstellungen()->get_catering_level5_max_revenue() * ware.menge));
					break;
				}

			case 4:
				if(journey_minutes >= welt->get_einstellungen()->get_catering_level3_minutes())
				{
					if(journey_minutes > welt->get_einstellungen()->get_catering_level4_minutes())
					{
						final_revenue += (welt->get_einstellungen()->get_catering_level4_max_revenue() * ware.menge);
						break;
					}
					
					proportion = (journey_minutes - welt->get_einstellungen()->get_catering_level3_max_revenue()) / (welt->get_einstellungen()->get_catering_level4_minutes() - welt->get_einstellungen()->get_catering_level3_minutes());
					final_revenue += (proportion * (welt->get_einstellungen()->get_catering_level4_max_revenue() * ware.menge));
					break;
				}

			case 3:
				if(journey_minutes >= welt->get_einstellungen()->get_catering_level2_minutes())
				{
					if(journey_minutes > welt->get_einstellungen()->get_catering_level3_minutes())
					{
						final_revenue += (welt->get_einstellungen()->get_catering_level3_max_revenue() * ware.menge);
						break;
					}
					
					proportion = (journey_minutes - welt->get_einstellungen()->get_catering_level2_max_revenue()) / (welt->get_einstellungen()->get_catering_level3_minutes() - welt->get_einstellungen()->get_catering_level2_minutes());
					final_revenue += (proportion * (welt->get_einstellungen()->get_catering_level3_max_revenue() * ware.menge));
					break;
				}

			case 2:
				if(journey_minutes >= welt->get_einstellungen()->get_catering_level1_minutes())
				{
					if(journey_minutes > welt->get_einstellungen()->get_catering_level2_minutes())
					{
						final_revenue += (welt->get_einstellungen()->get_catering_level2_max_revenue() * ware.menge);
						break;
					}
					
					proportion = (journey_minutes - welt->get_einstellungen()->get_catering_level1_max_revenue()) / (welt->get_einstellungen()->get_catering_level2_minutes() - welt->get_einstellungen()->get_catering_level1_minutes());
					final_revenue += (proportion * (welt->get_einstellungen()->get_catering_level2_max_revenue() * ware.menge));
					break;
				}

			case 1:
				if(journey_minutes < welt->get_einstellungen()->get_catering_min_minutes())
				{
					break;
				}
				if(journey_minutes > welt->get_einstellungen()->get_catering_level1_minutes())
				{
					final_revenue += (welt->get_einstellungen()->get_catering_level1_max_revenue() * ware.menge);
					break;
				}

				proportion = (journey_minutes - welt->get_einstellungen()->get_catering_min_minutes()) / (welt->get_einstellungen()->get_catering_level1_minutes() - welt->get_einstellungen()->get_catering_min_minutes());
				final_revenue += (proportion * (welt->get_einstellungen()->get_catering_level1_max_revenue() * ware.menge));
				break;

			};
		}
	}

	final_revenue = (final_revenue + 1500ll) / 3000ll;
	
	return final_revenue;
}

uint8 convoi_t::calc_tolerable_comfort(uint16 journey_minutes, karte_t* w) 
{
	const uint16 comfort_short_minutes = w->get_einstellungen()->get_tolerable_comfort_short_minutes();
	const uint8 comfort_short = w->get_einstellungen()->get_tolerable_comfort_short();
	if(journey_minutes <= comfort_short_minutes)
	{
		return comfort_short;
	}

	const uint16 comfort_median_short_minutes = w->get_einstellungen()->get_tolerable_comfort_median_short_minutes();
	const uint8 comfort_median_short = w->get_einstellungen()->get_tolerable_comfort_median_short();
	if(journey_minutes == comfort_median_short_minutes)
	{
		return comfort_median_short;
	}
	if(journey_minutes < comfort_median_short_minutes)
	{
		const float proportion = (float)(journey_minutes - comfort_short_minutes) / (float)(comfort_median_short_minutes - comfort_short_minutes);
		return (proportion * (comfort_median_short - comfort_short)) + comfort_short;
	}

	const uint16 comfort_median_median_minutes = w->get_einstellungen()->get_tolerable_comfort_median_median_minutes();
	const uint8 comfort_median_median = w->get_einstellungen()->get_tolerable_comfort_median_median();
	if(journey_minutes == comfort_median_median_minutes)
	{
		return comfort_median_median;
	}
	if(journey_minutes < comfort_median_median_minutes)
	{
		const float proportion = (float)(journey_minutes - comfort_median_short_minutes) / (float)(comfort_median_median_minutes - comfort_median_short_minutes);
		return (proportion * (comfort_median_median - comfort_median_short)) + comfort_median_short;
	}

	const uint16 comfort_median_long_minutes = w->get_einstellungen()->get_tolerable_comfort_median_long_minutes();
	const uint8 comfort_median_long = w->get_einstellungen()->get_tolerable_comfort_median_long();
	if(journey_minutes == comfort_median_long_minutes)
	{
		return comfort_median_long;
	}
	if(journey_minutes < comfort_median_long_minutes)
	{
		const float proportion = (float)(journey_minutes - comfort_median_median_minutes) / (float)(comfort_median_long_minutes - comfort_median_median_minutes);
		return (proportion * (comfort_median_long - comfort_median_median)) + comfort_median_median;
	}
	
	const uint16 comfort_long_minutes = w->get_einstellungen()->get_tolerable_comfort_long_minutes();
	const uint8 comfort_long = w->get_einstellungen()->get_tolerable_comfort_long();
	if(journey_minutes >= comfort_long_minutes)
	{
		return comfort_long;
	}

	const float proportion = (float)(journey_minutes - comfort_median_long_minutes) / (float)(comfort_long_minutes - comfort_median_long_minutes);
	return (proportion * (comfort_long - comfort_median_long)) + comfort_median_long;
}

uint16 convoi_t::calc_adjusted_speed_bonus(uint16 base_bonus, uint32 distance, karte_t* w)
{
	const uint32 min_distance = w != NULL ? w->get_einstellungen()->get_min_bonus_max_distance() : 10;
	if(distance <= min_distance)
	{
		return 0;
	}

	const uint16 max_distance = w != NULL ? w->get_einstellungen()->get_max_bonus_min_distance() : 16;
	const float multiplier = w != NULL ? w->get_einstellungen()->get_max_bonus_multiplier() : 30;
	
	if(distance >= max_distance)
	{
		return base_bonus * multiplier;
	}

	const uint16 median_distance = w != NULL ? w->get_einstellungen()->get_median_bonus_distance() : 128;
	if(median_distance == 0)
	{
		// There is no median, so scale evenly.
		const double proportion = (double)(distance - min_distance) / (double)(max_distance - min_distance);
		return (base_bonus * multiplier) * proportion;
	}

	// There is a median, so scale differently each side of the median.

	if(distance == median_distance)
	{
		return base_bonus;
	}

	if(distance < median_distance)
	{
		const double proportion = (double)(distance - min_distance) / (double)(median_distance - min_distance);
		return base_bonus * proportion;
	}

	// If the program gets here, it must be true that:
	// distance > median_distance

	const double proportion = (double)(distance - median_distance) / (double)(max_distance - min_distance);
	uint16 intermediate_bonus = (base_bonus * multiplier) - base_bonus;
	intermediate_bonus *= proportion;
	return intermediate_bonus + base_bonus;
}

/**
 * convoi an haltestelle anhalten
 * "Convoi stop at stop" (Google translations)
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(koord k, halthandle_t halt) //"has held" (Google)
{
	sint64 gewinn = 0;
	grund_t *gr = welt->lookup(fahr[0]->get_pos());

	int station_length = 0;
	if(gr->ist_wasser()) 
	{
		// habour has any size
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
			pos.z += Z_TILE_STEP;
		}
		while(  grund  &&  grund->get_halt() == halt  ) 
		{
			station_length += TILE_STEPS;
			pos += zv;
			grund = welt->lookup(pos);
			if(  grund==NULL  ) 
			{
				grund = welt->lookup(pos-koord3d(0,0,Z_TILE_STEP));
				if(  grund &&  grund->get_weg_yoff()!=TILE_HEIGHT_STEP  )
				{
					// not end/start of bridge
					break;
				}
			}
		}
	}

	// only load vehicles in station

	//int convoy_length = 0;
	bool second_run = anz_vehikel <= 1;
	uint8 convoy_length = 0;
	bool changed_loading_level = false;
	for(uint8 i=0; i < anz_vehikel ; i++) 
	{
		vehikel_t* v = fahr[i];

		convoy_length += v->get_besch()->get_length();
		if(convoy_length > station_length) 
		{
			break;
		}

		// we need not to call this on the same position		if(  v->last_stop_pos != v->get_pos().get_2d()  ) {		// calc_revenue
		if(!second_run || anz_vehikel == 1)
		{
			//convoi_t *tmp = this;	
			v->last_stop_pos = v->get_pos().get_2d();
			//Unload
			v->current_revenue = 0;
			freight_info_resort |= v->entladen(k, halt);
			gewinn += v->current_revenue;
		}

		changed_loading_level |= v->entladen(k, halt);
		if(!no_load) {
			// load
			changed_loading_level |= v->beladen(k, halt);
		}
		else 
		{
			// do not load anymore - but call beladen() to recalculate vehikel weight
			v->beladen(k, halthandle_t());
		}

		// Run this routine twice: first, load all vehicles to their non-overcrowded capacity.
		// Then, allow them to run to their overcrowded capacity.
		if(!second_run && i >= anz_vehikel - 1)
		{
			//Reset counter for one more go
			second_run = true;
			i = -1; // Bernd Gabriel, 05.07.2009: was 0, but 0 will skip first vehicle due to i++ at end of loop;
			// Bernd Gabriel, 05.07.2009: must reinitialize convoy_length
			convoy_length = 0;
		}
	}
	freight_info_resort |= changed_loading_level;
	if(  changed_loading_level  ) {
		halt->recalc_status();
	}

	// any loading went on?
	calc_loading();
	loading_limit = fpl->get_current_eintrag().ladegrad; //"charge degree" (??) (Babelfish)
	heaviest_vehicle = calc_heaviest_vehicle(); // Bernd Gabriel, Mar 10, 2010: was missing.

	if(gewinn) 
	{
		jahresgewinn += gewinn; //"annual profit" (Babelfish)
		besitzer_p->buche(gewinn, fahr[0]->get_pos().get_2d(), COST_INCOME);
		book(gewinn, CONVOI_PROFIT);
		book(gewinn, CONVOI_REVENUE);
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
 * Calclulate loading_level and loading_limit. This depends on current state (loading or not).
 * @author Volker Meyer
 * @date  20.06.2003
 */
void convoi_t::calc_loading()
{
	int fracht_max = 0;
	int fracht_menge = 0;
	for(unsigned i=0; i<anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		fracht_max += v->get_fracht_max();
		fracht_menge += v->get_fracht_menge();
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
	loading_limit = 0;	// will be set correctly from hat_gehalten() routine
}


/**
 * Schedule convoi for self destruction. Will be executed
 * upon next sync step
 * @author Hj. Malthaner
 */
void convoi_t::self_destruct()
{
	// convois in depot are not contained in the map array!
	if(state==INITIAL) {
		for(  int i=0;  i<anz_vehikel;  i++  ) {
			fahr[i]->set_flag( ding_t::not_on_map );
		}
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

	if(fpl!=NULL  &&  !fpl->ist_abgeschlossen()) {
		destroy_win((long)fpl);
	}

	if(  line.is_bound()  ) {
		// needs to be done here to remove correctly ware catg from lines
		unset_line();
		delete fpl;
		fpl = NULL;
	}

	if(  state != INITIAL  ) {
		state = SELF_DESTRUCT;
	}

	// pay the current value
	besitzer_p->buche( calc_restwert(), get_pos().get_2d(), COST_NEW_VEHICLE );
	besitzer_p->buche( -calc_restwert(), COST_ASSETS );

	for(int i=anz_vehikel-1;  i>=0; i--) {
		if(  !fahr[i]->get_flag( ding_t::not_on_map )  ) {
			// remove from rails/roads/crossings
			grund_t *gr = welt->lookup(fahr[i]->get_pos());
			fahr[i]->set_letztes( true );
			fahr[i]->verlasse_feld();
			if(gr  &&  gr->ist_uebergang()) {
				gr->find<crossing_t>()->release_crossing(fahr[i]);
			}
			fahr[i]->set_flag( ding_t::not_on_map );

		}
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
		"akt_speed_soll = %d\n"
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
		(int)akt_speed_soll,
		(int)sp_soll,
		(int)state,
		(const char *)(state_names[state]),
		(int)alte_richtung,
		(long)(jahresgewinn/100),
		(const char *)name_and_id,
		(int)line_id,
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
		rolling_average[cost_type] += amount;
		rolling_average_count[cost_type] ++;
		sint32 tmp = rolling_average[cost_type] / rolling_average_count[cost_type];
		financial_history[0][cost_type] = tmp;
	}
	if (line.is_bound()) 
	{
		line->book(amount, simline_t::convoi_to_line_catgory[cost_type] );
	}

	if(cost_type == CONVOI_TRANSPORTED_GOODS) 
	{
		besitzer_p->buche(amount, COST_ALL_TRANSPORTED);
	}
}


void convoi_t::init_financial_history()
{
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			financial_history[k][j] = 0;
		}
	}
}

sint32 convoi_t::get_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) { //"anzahl" = "number" (Babelfish)
		sint32 vehicle_running_cost = fahr[i]->get_betriebskosten(welt); //"get_operatingCost" (Google). "Fahr" = "Drive" (Babelfish)
		running_cost += vehicle_running_cost;
	}
	return running_cost;
}

sint32 convoi_t::get_per_kilometre_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) { //"anzahl" = "number" (Babelfish)
		sint32 vehicle_running_cost = fahr[i]->get_besch()->get_base_running_costs(welt);
		running_cost += vehicle_running_cost;
	}
	return running_cost;
}

uint32 convoi_t::get_fixed_maintenance() const
{
	uint32 maint = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) {
		maint += fahr[i]->get_fixed_maintenance(welt); 
	}
	return maint;
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
	if(line.is_bound()) {
		unset_line();
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
	if(line.is_bound()) {
DBG_DEBUG("convoi_t::unset_line()", "removing old destinations from line=%d, fpl=%p",line.get_id(),fpl);
		line->remove_convoy(self);
		line = linehandle_t();
	}
	// just to be sure ...
	line_id = INVALID_LINE_ID;
}

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

		if(fpl->get_count()==0  ||  new_fpl->get_count()==0) {
			// was no entry or is no entry => goto  1st stop
			aktuell = 0;
		}
		else {
			// something to check for ...
			current = fpl->get_current_eintrag().pos;

			if(aktuell<new_fpl->get_count()  &&  current==new_fpl->eintrag[aktuell].pos  ) {
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
				 */
				const koord3d next = fpl->eintrag[(aktuell+1)%fpl->get_count()].pos;
				const koord3d nextnext = fpl->eintrag[(aktuell+2)%fpl->get_count()].pos;
				const koord3d nextnextnext = fpl->eintrag[(aktuell+3)%fpl->get_count()].pos;
				int how_good_matching = 0;
				const uint8 new_count = new_fpl->get_count();

				for(  uint8 i=0;  i<new_count;  i++  ) {
					int quality =
						(new_fpl->eintrag[i].pos==current)*3 +
						(new_fpl->eintrag[(i+1)%new_count].pos==next)*4 +
						(new_fpl->eintrag[(i+2)%new_count].pos==nextnext)*2 +
						(new_fpl->eintrag[(i+3)%new_count].pos==nextnextnext);
					if(  quality>how_good_matching  ) {
						// better match than previous: but depending of distance, the next number will be different
						if(new_fpl->eintrag[i].pos==current) {
							aktuell = i;
						}
						else if(new_fpl->eintrag[(i+1)%new_count].pos==next) {
							aktuell = i+1;
						}
						else if(new_fpl->eintrag[(i+2)%new_count].pos==nextnext) {
							aktuell = i+2;
						}
						else if(new_fpl->eintrag[(i+3)%new_count].pos==nextnextnext) {
							aktuell = i+3;
						}
						aktuell %= new_count;
						how_good_matching = quality;
					}
				}

				if(how_good_matching==0) {
					// nothing matches => take the one from the line
					aktuell = new_fpl->get_aktuell();
				}
				// if we go to same, then we do not need route recalculation ...
				is_same = new_fpl->eintrag[aktuell].pos==current;
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
			fpl->insert(welt->lookup(depot));
			fpl->set_aktuell( (fpl->get_aktuell()+fpl->get_count()-1)%fpl->get_count() );
		}

 		if(state!=INITIAL) {
			if(is_same  ||  is_depot) {
				/* same destination
				 * We are already there => remove wrong freight and keep current state
				 */
				for(uint8 i=0; i<anz_vehikel; i++) {
					fahr[i]->remove_stale_freight();
				}
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



/*
 * the current state saved as color
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), DARK_PURPLE (convoy has overcrowded vehicles), BLUE (at least one convoi vehicle is obsolete)
 */
uint8 convoi_t::get_status_color() const
{
	if(state==INITIAL)
	{
		// in depot/under assembly
		return COL_WHITE;
	}
	else if(state==WAITING_FOR_CLEARANCE_ONE_MONTH  ||  state==CAN_START_ONE_MONTH  ||  hat_keine_route()) 
	{
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

//void convoi_t::set_replacing_vehicles(const vector_tpl<const vehikel_besch_t *> *rv)
//{
//	replacing_vehicles.clear();
//	replacing_vehicles.resize(rv->get_count());  // To save some memory
//	for (unsigned int i=0; i<rv->get_count(); ++i) {
//		replacing_vehicles.append((*rv)[i]);
//	}
//}
 

 /**
 * True if this convoy has the same vehicles as the other
 * @author isidoro
 */
bool convoi_t::has_same_vehicles(convoihandle_t other) const
{
	if (other.is_bound()) {
		if (get_vehikel_anzahl()!=other->get_vehikel_anzahl()) {
			return false;
		}
		for (int i=0; i<get_vehikel_anzahl(); i++) {
			if (get_vehikel(i)->get_besch()!=other->get_vehikel(i)->get_besch()) {
				return false;
			}
		}
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
public:
	depot_finder_t( convoihandle_t cnv ) 
	{ 
		master = cnv->get_vehikel(0); 
		assert(master!=NULL); 
	};
	virtual waytype_t get_waytype() const 
	{ 
		return master->get_waytype(); 
	};
	virtual bool ist_befahrbar( const grund_t* gr ) const 
	{ 
		return master->ist_befahrbar(gr); 
	};
	virtual bool ist_ziel( const grund_t* gr, const grund_t* ) const 
	{ 
		return gr->get_depot() && gr->get_depot()->get_besitzer() == master->get_besitzer(); 
	};
	virtual ribi_t::ribi get_ribi( const grund_t* gr) const
	{ 
		return master->get_ribi(gr); 
	};
	virtual int get_kosten( const grund_t*, uint32) const 
	{ 
		return 1; 
	};
};


/**
 * Convoy is sent to depot.  Return value, success or not.
 */
bool convoi_t::go_to_depot(bool show_success)
{
	/*if (convoi_info_t::route_search_in_progress) 
	{
		return false;
	}*/
	// limit update to certain states that are considered to be safe for fahrplan updates
	int state = get_state();
	if(state==convoi_t::FAHRPLANEINGABE) {
DBG_MESSAGE("convoi_t::go_to_depot()","convoi state %i => cannot change schedule ... ", state );
		return false;
	}
	//convoi_info_t::route_search_in_progress = true;

	route_t route;
	depot_finder_t finder( self );
	route.find_route( welt, get_vehikel(0)->get_pos(), &finder, 0, ribi_t::alle, 0x7FFFFFFF);

	// if route to a depot has been found, update the convoy's schedule
	bool b_depot_found = false;
	
	if(!route.empty())
	{
		koord3d depot_pos = route.position_bei(route.get_count()-1);
		schedule_t *fpl = get_schedule();
		fpl->insert(get_welt()->lookup(depot_pos));
		fpl->set_aktuell( (fpl->get_aktuell()+fpl->get_count()-1)%fpl->get_count() );
		b_depot_found = set_schedule(fpl);
	}
	if(!b_depot_found)
	{
		// Second try - if the new system does not work, try the old system instead.
		// iterate over all depots and try to find shortest route
		slist_iterator_tpl<depot_t *> depot_iter(depot_t::get_depot_list());
		route_t * shortest_route = new route_t();
		route_t * route = new route_t();
		koord3d home = koord3d(0,0,0);
		while (depot_iter.next()) 
		{
			depot_t *depot = depot_iter.get_current();
			if(depot->get_wegtyp()!=get_vehikel(0)->get_besch()->get_waytype() || depot->get_besitzer() != get_besitzer()) 
			{
				continue;
			}
			koord3d pos = depot->get_pos();
			if(!shortest_route->empty() && koord_distance(pos.get_2d(),get_pos().get_2d())>=shortest_route->get_count()-1) 
			{
				// the current route is already shorter, no need to search further
				continue;
			}
			bool found = get_vehikel(0)->calc_route(get_pos(), pos, 50, route); // do not care about speed
			if (found) 
			{
				if(  route->get_count()-1 < shortest_route->get_count()-1    ||    shortest_route->empty()  ) 
				{
					shortest_route->kopiere(route);
					home = pos;
				}
			}
		}
		DBG_MESSAGE("shortest route has ", "%i hops", shortest_route->get_count()-1);

		if(!shortest_route->empty()) 
		{
			koord3d depot_pos = route->position_bei(route->get_count()-1);
			schedule_t *fpl = get_schedule();
			fpl->insert(get_welt()->lookup(home));
			fpl->set_aktuell( (fpl->get_aktuell()+fpl->get_count()-1)%fpl->get_count() );
			b_depot_found = set_schedule(fpl);
		}

		delete shortest_route;
		delete route;
	}
	/*convoi_info_t::route_search_in_progress = false;*/

	// show result
	const char* txt;
	if (b_depot_found) 
	{
		txt = "Convoi has been sent\nto the nearest depot\nof appropriate type.\n";
	} 
	else
	{
		txt = "Home depot not found!\nYou need to send the\nconvoi to the depot\nmanually.";
	}
	if (!b_depot_found || show_success) 
	{
		create_win( new news_img(txt), w_time_delete, magic_none);
	}
	return b_depot_found;
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
	uint16 tiles=0;
	// the last vehicle does not count!
	for(sint8 i=0;  i<anz_vehikel-1;  i++) {
		tiles += fahr[i]->get_besch()->get_length();
	}
	// add 127/256 tile to account for the driving in stations in north/west direction
	// see at the end of vehikel_t::hop()
	return (tiles*16 + 256-1 + 127)/256;
	// was originally (tiles+16-1)/16;
}



// if withdraw and empty, then self destruct
void convoi_t::set_withdraw(bool new_withdraw)
{
	withdraw = new_withdraw;
	if(  withdraw  &&  loading_level==0  ) {
		// test if convoi in depot and not driving
		grund_t *gr = welt->lookup( get_pos());
		if (gr && gr->get_depot()  &&  state == INITIAL) {
			gr->get_depot()->disassemble_convoi(self, true);
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
bool convoi_t::can_overtake(overtaker_t *other_overtaker, int other_speed, int steps_other, int diagonal_length)
{
	if(fahr[0]->get_waytype()!=road_wt) {
		return false;
	}

	if (!other_overtaker->can_be_overtaken()) {
		return false;
	}

	assert( anz_vehikel>0 );

	int diff_speed = akt_speed - other_speed;
	if(  diff_speed < kmh_to_speed(5)  ) {
		return false;
	}

	// Number of tiles overtaking will take
	int n_tiles = 0;

	// Distance it takes overtaking (unit:256*tile) = my_speed * time_overtaking
	// time_overtaking = tiles_to_overtake/diff_speed
	// tiles_to_overtake = convoi_length + pos_other_convoi
	int distance = 256 + akt_speed*((get_length()*16)+steps_other)/diff_speed;
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

		int d = ribi_t::ist_gerade(str->get_ribi()) ? 256 : diagonal_length;
		distance -= d;
		time_overtaking += d;

		// Check for other vehicles
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t *v = (vehikel_basis_t *)gr->obj_bei(j);
			if(v->is_moving()) {
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
	time_overtaking = (time_overtaking << 16)/akt_speed;
	while ( time_overtaking > 0 ) {

		if ( route_index >= route.get_count() ) {
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

		time_overtaking -= (ribi_t::ist_gerade(str->get_ribi()) ? 256<<16 : diagonal_length<<16)/kmh_to_speed(str->get_max_speed());

		// Check for other vehicles in facing direction
		ribi_t::ribi their_direction = ribi_t::rueckwaerts( fahr[0]->calc_richtung(pos_prev, pos_next.get_2d()) );
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t *v = (vehikel_basis_t *)gr->obj_bei(j);
			if(v->is_moving()  &&  v->get_fahrtrichtung()==their_direction  &&  v->get_overtaker()) {
				return false;
			}
		}
		pos_prev = pos.get_2d();
		pos = pos_next;
	}

	set_tiles_overtaking( 1+n_tiles );
	other_overtaker->set_tiles_overtaking( -1-(n_tiles/2) );
	return true;
}

uint32 
convoi_t::calc_heaviest_vehicle()
{
	uint32 heaviest = 0;
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		uint32 tmp = fahr[i]->get_sum_weight();
		if(tmp > heaviest)
		{
			heaviest = tmp;
		}
	}
	return heaviest;
}

uint16
convoi_t::calc_longest_loading_time()
{
	uint16 longest = 0;
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		uint16 tmp = fahr[i]->get_besch()->get_loading_time();
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
		if (fahr[j]->get_besch()->is_retired(timeline_year_month)) {
			return true;
		}
	}
	return false;
}

void convoi_t::clear_replace()
{
	if(replace)
	{
		replace->decrement_convoys();
		replace = NULL;
	}
}

 void convoi_t::set_replace(replace_data_t *new_replace)
 { 
	 if(new_replace != NULL && replace != new_replace)
	 {
		 new_replace->increment_convoys();
	 }
	 replace = new_replace;
 }

 void convoi_t::propogate_replace(replace_data_t *rpl, char replace_type)
 {
	 if(replace)
	 {

	 }
 }
