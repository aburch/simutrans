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

#include "besch/vehikel_besch.h"
#include "besch/roadsign_besch.h"

#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "dings/crossing.h"
#include "dings/roadsign.h"

#include "vehicle/simvehikel.h"
#include "vehicle/overtaker.h"

#include "utils/simstring.h"
#include "utils/cbuffer_t.h"


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



/*
 * Debugging helper - translate state value to human readable name
 * @author Hj- Malthaner
 */
static const char * state_names[convoi_t::MAX_STATES] =
{
	"INITIAL",
	"FAHRPLANEINGABE",
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


void convoi_t::init(karte_t *wl, spieler_t *sp)
{
	welt = wl;
	besitzer_p = sp;

	is_electric = false;
	sum_gesamtgewicht = sum_gewicht = sum_gear_und_leistung = sum_leistung = 0;
	previous_delta_v = 0;
	min_top_speed = 9999999;

	fpl = NULL;
	line = linehandle_t();
	line_id = INVALID_LINE_ID;

	anz_vehikel = 0;
	steps_driven = -1;
	withdraw = false;
	has_obsolete = false;
	no_load = false;
	wait_lock = 0;
	go_on_ticks = WAIT_INFINITE;

	jahresgewinn = 0;

	alte_richtung = ribi_t::keine;
	next_wolke = 0;

	state = INITIAL;

	*name_and_id = 0;
	name_offset = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;

	max_record_speed = 0;
	akt_speed_soll = 0;            // Sollgeschwindigkeit
	akt_speed = 0;                 // momentane Geschwindigkeit
	sp_soll = 0;

	next_stop_index = 65535;

	line_update_pending = linehandle_t();

	home_depot = koord3d::invalid;
	last_stop_pos = koord3d::invalid;
}


convoi_t::convoi_t(karte_t* wl, loadsave_t* file) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	init(wl, 0);
	rdwr(file);
}


convoi_t::convoi_t(spieler_t* sp) : fahr(max_vehicle, NULL)
{
	self = convoihandle_t(this);
	init(sp->get_welt(), sp);
	set_name( "Unnamed" );
	welt->add_convoi( self );
	init_financial_history();
}


convoi_t::~convoi_t()
{
	assert(self.is_bound());
	assert(anz_vehikel==0);

DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_follow_convoi()==self) {
		welt->set_follow_convoi( convoihandle_t() );
	}

	welt->sync_remove( this );
	welt->rem_convoi( self );

	// force asynchronous recalculation
	if(fpl) {
		if(!fpl->ist_abgeschlossen()) {
			destroy_win((long)fpl);
		}
		if(fpl->get_count()>0  &&  !line.is_bound()  ) {
			welt->set_schedule_counter();
		}
		delete fpl;
	}

	// @author hsiegeln - deregister from line
	unset_line();

	self.detach();

	destroy_win((long)this);
}



void
convoi_t::laden_abschliessen()
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
			vehikel_t* v = fahr[i];
			v->set_convoi(this);
		}
		return;
	}

	bool realing_position = false;
	if(anz_vehikel>0) {
DBG_MESSAGE("convoi_t::laden_abschliessen()","state=%s, next_stop_index=%d", state_names[state] );
		for( uint8 i=0;  i<anz_vehikel;  i++ ) {
			vehikel_t* v = fahr[i];
			v->set_erstes(i == 0u);
			v->set_letztes(i == (anz_vehikel - 1u));
			// this sets the convoi and will renew the block reservation, if needed!
			v->set_convoi(this);

			// wrong alingmant here => must relocate
			if(v->need_realignment()) {
				// diagonal => convoi must restart
				realing_position |= (v->get_fahrtrichtung()&0x0A)!=0  &&  (state==DRIVING  ||  is_waiting());
			}
		}
DBG_MESSAGE("convoi_t::laden_abschliessen()","next_stop_index=%d", next_stop_index );
		// lines are still unknown during loading!
		if (line_id != INVALID_LINE_ID) {
			// if a line is assigned, set line!
			register_with_line(line_id);
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

			v->darf_rauchen(false);
			fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 );
			train_length -= v->get_besch()->get_length();
			v->darf_rauchen(true);

			// eventually reserve this again
			grund_t *gr=welt->lookup(v->get_pos());
			// airplanes may have no ground ...
			schiene_t *sch0 = dynamic_cast<schiene_t *>( gr->get_weg(fahr[i]->get_waytype()) );
			if(sch0) {
				sch0->reserve(self);
			}
		}
		fahr[0]->set_erstes(true);
	}
}



void convoi_t::rotate90( const sint16 y_size )
{
	last_stop_pos.rotate90( y_size );
	record_pos.rotate90( y_size );
	home_depot.rotate90( y_size );
	for(  uint32 i=0;  i<route.get_max_n()+1u;  i++  ) {
		route.access_position_bei(i).rotate90( y_size );
	}
	if(fpl) {
		fpl->rotate90( y_size );
	}
	// eventually correct freight destinations (and remove all stale freight)
	for(  int i=0;  i<anz_vehikel;  i++  ) {
		fahr[i]->remove_stale_freight();
	}
}



/**
 * Gibt die Position des Convois zurück.
 * @return Position des Convois
 * @author Hj. Malthaner
 */
koord3d
convoi_t::get_pos() const
{
	if(anz_vehikel > 0 && fahr[0]) {
		return state==INITIAL ? home_depot : fahr[0]->get_pos();
	} else {
		return koord3d::invalid;
	}
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void
convoi_t::set_name(const char *name)
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
 * Vehicles of the convoi add their running cost by using this
 * method
 * @author Hj. Malthaner
 */
void convoi_t::add_running_cost(sint32 cost)
{
	// Fahrtkosten
	jahresgewinn += cost;
	get_besitzer()->buche(cost, COST_VEHICLE_RUN);

	// hsiegeln
	book(cost, CONVOI_OPERATIONS);
	book(cost, CONVOI_PROFIT);
}



/* Calculates (and sets) new akt_speed
 * needed for driving, entering and leaving a depot)
 */
void convoi_t::calc_acceleration(long delta_t)
{
	// Prissi: more pleasant and a little more "physical" model *
	int sum_friction_weight = 0;
	sum_gesamtgewicht = 0;
	// calculate total friction
	for(unsigned i=0; i<anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		int total_vehicle_weight = v->get_gesamtgewicht();

		sum_friction_weight += v->get_frictionfactor() * total_vehicle_weight;
		sum_gesamtgewicht += total_vehicle_weight;
	}

	// try to simulate quadratic friction
	if(sum_gesamtgewicht != 0) {
		/*
		 * The parameter consist of two parts (optimized for good looking):
		 *  - every vehicle in a convoi has a the friction of its weight
		 *  - the dynamic friction is calculated that way, that v^2*weight*frictionfactor = 200 kW
		 * => heavier loaded and faster traveling => less power for acceleration is available!
		 * since delta_t can have any value, we have to scale the step size by this value.
		 * however, there is a quadratic friction term => if delta_t is too large the calculation may get weird results
		 * @author prissi
		 */

		/* but for integer, we have to use the order below and calculate actually 64*deccel, like the sum_gear_und_leistung
		 * since akt_speed=10/128 km/h and we want 64*200kW=(100km/h)^2*100t, we must multiply by (128*2)/100
		 * But since the acceleration was too fast, we just deccelerate 4x more => >>6 instead >>8 */
		sint32 deccel = ( ( (akt_speed*sum_friction_weight)>>6 )*(akt_speed>>2) ) / 25 + (sum_gesamtgewicht*64);	// this order is needed to prevent overflows!

		// prissi:
		// integer sucks with planes => using floats ...
		sint32 delta_v =  (sint32)( ( (double)( (akt_speed>akt_speed_soll?0l:sum_gear_und_leistung) - deccel)*(double)delta_t)/(double)sum_gesamtgewicht);

		// we normalize delta_t to 1/64th and check for speed limit */
//		sint32 delta_v = ( ( (akt_speed>akt_speed_soll?0l:sum_gear_und_leistung) - deccel) * delta_t)/sum_gesamtgewicht;

		// we need more accurate arithmetic, so we store the previous value
		delta_v += previous_delta_v;
		previous_delta_v = delta_v & 0x0FFF;
		// and finally calculate new speed
		akt_speed = max(akt_speed_soll>>4, akt_speed+(sint32)(delta_v>>12l) );
	}
	else {
		// very old vehicle ...
		akt_speed += 16;
	}

	// obey speed maximum with additional const brake ...
	if(akt_speed > akt_speed_soll) {
		akt_speed -= 24;
		if(akt_speed > akt_speed_soll+kmh_to_speed(20)) {
			akt_speed = akt_speed_soll+kmh_to_speed(20);
		}
	}

	// new record?
	if(akt_speed > max_record_speed) {
		max_record_speed = akt_speed;
		record_pos = fahr[0]->get_pos().get_2d();
	}
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
			break;

		case FAHRPLANEINGABE:
			// schedule window closed?
			if(fpl!=NULL  &&  fpl->ist_abgeschlossen()) {

				set_schedule(fpl);

				if(fpl->get_count()==0) {
					// no entry => no route ...
					state = NO_ROUTE;
					wait_lock = 0;
				}
				else {
					// Schedule changed at station
					// this station? then complete loading task else drive on
					state = (get_pos() == fpl->get_current_eintrag().pos ? LOADING : ROUTING_1);
				}
			}
			break;

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
					uint32 sp_hat = fahr[0]->fahre_basis(1<<12);
					int v_nr = get_vehicle_at_length((steps_driven++)>>4);
					// stop when depot reached
					if(state==INITIAL) {
						break;
					}
					// until all are moving or something went wrong (sp_hat==0)
					if(sp_hat==0  ||  v_nr==anz_vehikel) {
						steps_driven = -1;
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
					}
				}
			}
			break;	// LEAVING_DEPOT

		case DRIVING:
			{
				calc_acceleration(delta_t);

				// now actually move the units
				sp_soll += (akt_speed*delta_t);
				uint32 sp_hat = fahr[0]->fahre_basis(sp_soll);
				// stop when depot reached ...
				if(state==INITIAL) {
					break;
				}
				// now move the rest (so all vehikel are moving synchroniously)
				for(unsigned i=1; i<anz_vehikel; i++) {
					fahr[i]->fahre_basis(sp_hat);
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
 * @author Hanjsörg Malthaner
 */
int convoi_t::drive_to(koord3d start, koord3d ziel)
{
	INT_CHECK("simconvoi 293");

	if (anz_vehikel == 0 || !fahr[0]->calc_route(start, ziel, speed_to_kmh(min_top_speed), &route)) {
		get_besitzer()->bescheid_vehikel_problem(self,ziel);
		// wait 10s before next attempt
		wait_lock = 10000;
		return false;
	}
	return true;
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
	// moved check to here, as this will apply the same update
	// logic/constraints convois have for manual schedule manipulation
	if (line_update_pending.is_bound()) {
		check_pending_updates();
	}

	if(wait_lock!=0) {
		return;
	}

	switch(state) {

		case LOADING:
			laden();
			break;

		case DUMMY4:
		case DUMMY5:
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
					drive_to(v->get_pos(), fpl->get_current_eintrag().pos);
					if(!route.empty()) {
						vorfahren();
					}
					else {
						state = NO_ROUTE;
					}
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
					// wait 10s before next attempt
					wait_lock = 10000;
				}
				else {
					// Hajo: now calculate a new route
					drive_to(v->get_pos(), fpl->get_current_eintrag().pos);
					if(!route.empty()) {
						vorfahren();
					}
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
					if(haltestelle_t::get_halt(welt,v->get_pos()).is_bound()) {
						v->play_sound();
					}
				}
				if(restart_speed>=0) {
					akt_speed = restart_speed;
				}
				// wait a little before next try
				if(state==CAN_START  ||  state==CAN_START_ONE_MONTH) {
					wait_lock = 1000;
					set_tiles_overtaking( 0 );
				}
				else if(state==CAN_START_TWO_MONTHS) {
					wait_lock = 2500;
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
				// wait a little before next try
				if(state==WAITING_FOR_CLEARANCE_ONE_MONTH  ||  state==WAITING_FOR_CLEARANCE_TWO_MONTHS) {
					wait_lock = 2500;
				}
			}
			break;

		// must be here; may otherwise confuse window management
		case SELF_DESTRUCT:
			besitzer_p->buche( calc_restwert(), get_pos().get_2d(), COST_NEW_VEHICLE );
			besitzer_p->buche( -calc_restwert(), get_pos().get_2d(), COST_ASSETS );
			welt->set_dirty();
			destroy();
			break;

		default:	/* keeps compiler silent*/
			break;
	}
}



void convoi_t::neues_jahr()
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
	// everything normal: update histroy
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
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
		// convoi has obsolete vehicles?
		const int month_now = welt->get_timeline_year_month();
		has_obsolete = false;
		for(unsigned j=0;  j<get_vehikel_anzahl();  j++ ) {
			if (fahr[j]->get_besch()->is_retired(month_now)) {
				has_obsolete = true;
				break;
			}
		}
	}
}



void
convoi_t::betrete_depot(depot_t *dep)
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
	destroy_win((long)this);

	// Hajo: since 0.81.5exp it's safe to
	// remove the current sync object from
	// the sync list from inside sync_step()
	welt->sync_remove(this);
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
			home_depot = route.position_bei(0);
			fahr[0]->set_pos( home_depot );
		}

		alte_richtung = ribi_t::keine;
		no_load = false;

		state = ROUTING_1;

		// recalc weight and image
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->beladen( home_depot.get_2d(), halthandle_t() );
		}
		// calc state for convoi
		calc_loading();

		if(line.is_bound()) {
			// might have changed the vehicles in this car ...
			line->recalc_catg_index();
		}
		else {
			welt->set_schedule_counter();
		}

		DBG_MESSAGE("convoi_t::start()","Convoi %s wechselt von INITIAL nach ROUTING_1", name_and_id);
	} else {
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
		char buf[128];

		// we still book the money for the trip; however, the frieght will be lost
		calc_gewinn();

		akt_speed = 0;
		sprintf(buf, translator::translate("!1_DEPOT_REACHED"), get_name());
		welt->get_message()->add_message(buf, v->get_pos().get_2d(),message_t::convoi, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);

		betrete_depot(dp);
	}
	else {
		// no depot reached, check for stop!
		halthandle_t halt = gr->ist_wasser() ? haltestelle_t::get_halt(welt, v->get_pos()) : gr->get_halt();
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
		}
	}
	wait_lock = 0;
}



/**
 * Wartet bis Fahrzeug 0 freie Fahrt meldet
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
		fahr.resize(max_rail_vehicle, NULL);
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

		const vehikel_besch_t *info = v->get_besch();
		if(info->get_leistung()) {
			is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
		}
		sum_leistung += info->get_leistung();
		sum_gear_und_leistung += info->get_leistung()*info->get_gear();
		sum_gewicht += info->get_gewicht();
		min_top_speed = min( min_top_speed, kmh_to_speed( v->get_besch()->get_geschw() ) );
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;
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

DBG_MESSAGE("convoi_t::add_vehikel()","now %i of %i total vehikels.",anz_vehikel,max_vehicle);
	return true;
}


vehikel_t *
convoi_t::remove_vehikel_bei(uint16 i)
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

			const vehikel_besch_t *info = v->get_besch();
			sum_leistung -= info->get_leistung();
			sum_gear_und_leistung -= info->get_leistung()*info->get_gear();
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
			has_obsolete = false;
			const int month_now = welt->get_timeline_year_month();
			for(unsigned i=0; i<anz_vehikel; i++) {
				has_obsolete |= fahr[i]->get_besch()->is_retired(month_now);
			}
		}

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
	return v;
}

void
convoi_t::set_erstes_letztes()
{
	// anz_vehikel muss korrekt init sein
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
	enum states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::set_fahrplan()", "new=%p, old=%p", f, fpl);

	if(f == NULL) {
		return false;
	}

	// happens to be identical?
	if(fpl!=f) {
		// destroy a possibly open schedule window
		if(fpl &&  !fpl->ist_abgeschlossen()) {
			destroy_win((long)fpl);
			delete fpl;
		}
		fpl = f;
	}

	// remove wrong freight
	for(unsigned i=0; i<anz_vehikel; i++) {
		fahr[i]->remove_stale_freight();
	}

	// ok, now we have a schedule
	if(old_state!=INITIAL) {
		state = FAHRPLANEINGABE;
		if(  !line.is_bound()  ) {
			// asynchronous recalculation of routes
			welt->set_schedule_counter();
		}
	}
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
bool
convoi_t::can_go_alte_richtung()
{
	// invalid route? nothing to test, must start new
	if(route.empty()) {
		return false;
	}

	// going backwards? then recalculate all
	ribi_t::ribi neue_richtung_rwr = ribi_t::rueckwaerts(fahr[0]->calc_richtung(route.position_bei(0).get_2d(), route.position_bei(min(2,route.get_max_n())).get_2d()));
//	DBG_MESSAGE("convoi_t::go_alte_richtung()","neu=%i,rwr_neu=%i,alt=%i",neue_richtung_rwr,ribi_t::rueckwaerts(neue_richtung_rwr),alte_richtung);
	if(neue_richtung_rwr&alte_richtung) {
		akt_speed = 8;
		return false;
	}

	// now get the actual length and the tile length
	int convoi_length = 15;
	unsigned i;	// for visual C++
	const vehikel_t* pred = NULL;
	for(i=0; i<anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		grund_t *gr = welt->lookup(v->get_pos());

		convoi_length += v->get_besch()->get_length();

		if(gr==NULL  ||  (pred!=NULL  &&  (abs(v->get_pos().x-pred->get_pos().x)>=2  ||  abs(v->get_pos().y-pred->get_pos().y)>=2))) {
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

		pred = v;
	}
	int length = min((convoi_length/16)+4,route.get_max_n()-1);	// maximum length in tiles to check

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
	// eventually we need to add their positions to the convois route
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
	length = min((convoi_length/8),route.get_max_n()-1);	// maximum length in tiles to check
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



void
convoi_t::vorfahren()
{
	// Hajo: init speed settings
	sp_soll = 0;
	set_tiles_overtaking( 0 );

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
		if(  steps_driven>0  ||  !can_go_alte_richtung()  ) {

			// since start may have been changed
			k0 = route.position_bei(0);

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
				/* we will set by this method the pos_prev to the starting point;
				 * otherwise it may be elsewhere, especially on curves and with already
				 * broken convois.
				 */
				v->set_pos(k0);
				v->neue_fahrt(0, true);
				gr=welt->lookup(v->get_pos());
				if(gr) {
					v->set_pos(k0);
					v->betrete_feld();
				}
			}

			// move one train length to the start position ...
			int train_length = 0;
			for(unsigned i=0; i<anz_vehikel-1u; i++) {
				train_length += fahr[i]->get_besch()->get_length(); // this give the length in 1/TILE_STEPS of a full tile
			}
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->get_fahrtrichtung();
			if(neue_richtung==ribi_t::sued  ||  neue_richtung==ribi_t::ost) {
				train_length += fahr[anz_vehikel-1]->get_besch()->get_length();
			}
			else {
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			fahr[0]->set_erstes(false); // switches off signal checks ...
			for(unsigned i=0; i<anz_vehikel; i++) {
				vehikel_t* v = fahr[i];

				v->darf_rauchen(false);
				fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 );
				train_length -= v->get_besch()->get_length();	// this give the length in 1/TILE_STEPS of a full tile => all cars closely coupled!
				v->darf_rauchen(true);
			}
			fahr[0]->set_erstes(true);
		}
		state = CAN_START;

		// to advance more smoothly
		int restart_speed=-1;
		if(fahr[0]->ist_weg_frei(restart_speed)) {
			// can reserve new block => drive on
			if(haltestelle_t::get_halt(welt,k0).is_bound()) {
				fahr[0]->play_sound();
			}
			wait_lock = 0;
			state = DRIVING;
		}
	}

	// finally reserve route (if needed)
	if(  fahr[0]->get_waytype()!=air_wt  ) {
		// do not prereserve for airplanes
		for(unsigned i=0; i<anz_vehikel; i++) {
			// eventually reserve this
			schiene_t * sch0 = dynamic_cast<schiene_t *>( welt->lookup(fahr[i]->get_pos())->get_weg(fahr[i]->get_waytype()) );
			if(sch0) {
				sch0->reserve(this->self);
			}
			else {
				break;
			}
		}
	}

	INT_CHECK("simconvoi 711");
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
	// some versions may produce broken safegames apparently
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
		for(unsigned i=0; i<anz_vehikel; i++) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			vehikel_t *v = 0;

			const bool first = (i==0);
			const bool last = (i==anz_vehikel-1);
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
				sum_gear_und_leistung += info->get_leistung()*info->get_gear();
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
				// add to blockstrecke
				if(v->get_waytype()==track_wt  ||  v->get_waytype()==monorail_wt  ||  v->get_waytype()==maglev_wt  ||  v->get_waytype()==narrowgauge_wt) {
					schiene_t* sch = (schiene_t*)gr->get_weg(v->get_waytype());
					if(sch) {
						sch->reserve(self);
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

	if(file->get_version()<=88003) {
		// load statistics
		int j;
		for (j = 0; j<3; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
		for (j = 2; j<5; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}
	else {
		// load statistics
		for (int j = 0; j<MAX_CONVOI_COST; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
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
	if(file->get_version()>=99017) {
		if(file->is_saving()) {
			if(go_on_ticks==WAIT_INFINITE) {
				file->rdwr_long( go_on_ticks, "dt" );
			}
			else {
				uint32 diff_ticks = welt->get_zeit_ms()>go_on_ticks ? 0 : go_on_ticks-welt->get_zeit_ms();
				file->rdwr_long( diff_ticks, "dt" );
			}
		}
		else {
			file->rdwr_long( go_on_ticks, "dt" );
			if(go_on_ticks!=WAIT_INFINITE) {
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
}


void
convoi_t::zeige_info()
{
	if(!in_depot()) {

		if(umgebung_t::verbose_debug) {
			dump();
		}

		create_win( new convoi_info_t(self), w_info, (long)this );
	}
}


void convoi_t::info(cbuffer_t & buf) const
{
	const vehikel_t* v = fahr[0];
	if (v != NULL) {
    char tmp[128];

    sprintf(tmp, "\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->get_besch()->get_geschw(), get_running_cost() / 100.0);
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



// sort order of convoi
void convoi_t::set_sortby(uint8 sort_order)
{
	freight_info_order = sort_order;
	freight_info_resort = true;
}



//chaches the last info; resorts only when needed
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
			while(iter_vehicle_ware.next()) {
				ware_t ware = iter_vehicle_ware.get_current();
				for(unsigned i=0;  i<total_fracht.get_count();  i++ ) {

					// could this be joined with existing freight?
					ware_t &tmp = total_fracht[i];

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
					total_fracht.push_back(ware);
				}
			}

			INT_CHECK("simvehikel 876");
		}
		buf.clear();

		// apend info on total capacity
		slist_tpl <ware_t>capacity;
		for(i=0;  i<warenbauer_t::get_waren_anzahl();  i++  ) {
			if(max_loaded_waren[i]>0  &&  i!=warenbauer_t::INDEX_NONE) {
				ware_t ware(warenbauer_t::get_info(i));
				ware.menge = max_loaded_waren[i];
				if(ware.get_catg()==0) {
					capacity.append( ware );
				} else {
					// append to category?
					slist_tpl<ware_t>::iterator j   = capacity.begin();
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



void convoi_t::open_schedule_window()
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

	// darf der spieler diesen convoi umplanen ?
	if(get_besitzer() != NULL &&
		get_besitzer() != welt->get_active_player()) {
		return;
	}

	// manipulation of schedule not allowd while:
	// - just starting
	// - a line update is pending
	if(  state==FAHRPLANEINGABE  ||  line_update_pending.is_bound()  ) {
		create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		return;
	}

	if(state==DRIVING) {
		// book the current value of goods
		calc_gewinn();
	}

	akt_speed = 0;	// stop the train ...
	state = FAHRPLANEINGABE;
	alte_richtung = fahr[0]->get_fahrtrichtung();

	// Fahrplandialog oeffnen
	create_win( new fahrplan_gui_t(fpl,get_besitzer(),self), w_info, (long)fpl );
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Nachfolger von
 * vor gelten - daher muß vor != NULL sein..
 */
bool
convoi_t::pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
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
 */
bool
convoi_t::pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
	const vehikel_besch_t *soll;

	if(!hinter->get_vorgaenger_count()) {
		// Alle Vorgänger erlaubt
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
			return true;
		}
	}
	//DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
	//		 "No matching predecessor found.");
	return false;
}



bool
convoi_t::pruefe_alle()
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
void convoi_t::laden()
{
	if(state == FAHRPLANEINGABE) {
		return;
	}

	halthandle_t halt = haltestelle_t::get_halt(welt, fpl->get_current_eintrag().pos);
	// eigene haltestelle ?
	if (halt.is_bound()) {
		const koord k = fpl->get_current_eintrag().pos.get_2d();
		const spieler_t* owner = halt->get_besitzer();
		if(  owner == get_besitzer()  ||  owner == welt->get_spieler(1)  ) {
			// loading/unloading ...
			hat_gehalten(k, halt);
		}
	}

	if(go_on_ticks==WAIT_INFINITE  &&  fpl->get_current_eintrag().waiting_time_shift>0) {
		go_on_ticks = welt->get_zeit_ms() + (welt->ticks_per_tag >> (16-fpl->get_current_eintrag().waiting_time_shift));
	}

	INT_CHECK("simconvoi 1077");

	// Nun wurde ein/ausgeladen werden
	if(loading_level>=loading_limit  ||  no_load  ||  welt->get_zeit_ms()>go_on_ticks)  {

		if(withdraw  &&  loading_level==0) {
			// destroy when empty
			besitzer_p->buche( calc_restwert(), COST_NEW_VEHICLE );
			besitzer_p->buche( -calc_restwert(), COST_ASSETS );
			welt->set_dirty();
			destroy();
			return;
		}

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<anz_vehikel; i++) {
			book(get_vehikel(i)->get_fracht_max()-get_vehikel(i)->get_fracht_menge(), CONVOI_CAPACITY);
		}

		// Advance schedule
		fpl->advance();
		state = ROUTING_1;
	}
	// This is the minimum time it takes for loading
	wait_lock = WTT_LOADING;
}


/**
 * calculate income for last hop
 * @author Hj. Malthaner
 */
void convoi_t::calc_gewinn()
{
	sint64 gewinn = 0;

	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];
		gewinn += v->calc_gewinn(v->last_stop_pos, v->get_pos().get_2d() );
		v->last_stop_pos = v->get_pos().get_2d();
	}

	if(gewinn) {
		besitzer_p->buche(gewinn, fahr[0]->get_pos().get_2d(), COST_INCOME);
		jahresgewinn += gewinn;

		book(gewinn, CONVOI_PROFIT);
		book(gewinn, CONVOI_REVENUE);
	}
}


/**
 * convoi an haltestelle anhalten
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(koord k, halthandle_t halt)
{
	sint64 gewinn = 0;
	grund_t *gr=welt->lookup(fahr[0]->get_pos());

	int station_lenght=0;
	if(gr->ist_wasser()) {
		// habour has any size
		station_lenght = 24*16;
	}
	else {
		// calculate real station length
		koord zv = koord( ribi_t::rueckwaerts(fahr[0]->get_fahrtrichtung()) );
		koord3d pos = fahr[0]->get_pos();
		const grund_t *grund = welt->lookup(pos);
		while(  grund  &&  grund->get_halt() == halt  ) {
			station_lenght += TILE_STEPS;
			pos += zv;
			grund = welt->lookup(pos);
		}
	}

	// only load vehicles in station
	// don't load when vehicle is being withdrawn
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		station_lenght -= v->get_besch()->get_length();
		if(station_lenght<0) {
			break;
		}

		// calc_revenue
		gewinn += v->calc_gewinn(v->last_stop_pos, v->get_pos().get_2d() );
		v->last_stop_pos = v->get_pos().get_2d();

		freight_info_resort |= v->entladen(k, halt);
		if(!no_load) {
			// do not load anymore
			freight_info_resort |= v->beladen(k, halt);
		}

	}

	// any loading went on?
	calc_loading();
	loading_limit = fpl->get_current_eintrag().ladegrad;

	if(gewinn) {
		besitzer_p->buche(gewinn, fahr[0]->get_pos().get_2d(), COST_INCOME);
		jahresgewinn += gewinn;

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
 * Schedule convoid for self destruction. Will be executed
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
	}
	state = SELF_DESTRUCT;
	wait_lock = 0;
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



/**
 * Checks if this convoi has a driveable route
 * @author Hanjsörg Malthaner
 */
bool convoi_t::hat_keine_route() const
{
  return (state==NO_ROUTE);
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
	else if(fpl  &&  fpl->get_count()>0) {
		// since this schedule is no longer served
		welt->set_schedule_counter();
	}
	line = org_line;
	line_id = org_line->get_line_id();
	schedule_t *new_fpl= org_line->get_schedule()->copy();
	set_schedule(new_fpl);
	line->add_convoy(self);
}

/**
* register_with_line
* sets the convoi's line by using the line_id, rather than the line object
* CAUTION: THIS CALL WILL NOT SET A NEW FAHRPLAN!!! IT WILL JUST REGISTER THE CONVOI WITH THE LINE
* @author hsiegeln
*/
void convoi_t::register_with_line(uint16 id)
{
	DBG_DEBUG("convoi_t::register_with_line()","%s registers for %d", name_and_id, id);

	linehandle_t new_line = besitzer_p->simlinemgmt.get_line_by_id(id);
	if(new_line.is_bound()) {
		line = new_line;
		line_id = new_line->get_line_id();
		line->add_convoy(self);
	}
	else {
		line_id = INVALID_LINE_ID;
		line = linehandle_t();
	}
}



/**
* unset line
* removes convoy from route without destroying its fahrplan
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



void convoi_t::prepare_for_new_schedule(schedule_t *f)
{
	alte_richtung = fahr[0]->get_fahrtrichtung();

	state = FAHRPLANEINGABE;
	set_schedule(f);

	// Hajo: set_fahrplan sets state to ROUTING_1
	// need to undo that
	state = FAHRPLANEINGABE;
}



void convoi_t::book(sint64 amount, int cost_type)
{
	assert(  cost_type<MAX_CONVOI_COST);

	financial_history[0][cost_type] += amount;
	if (line.is_bound()) {
		line->book(amount, simline_t::convoi_to_line_catgory[cost_type] );
	}

	if(cost_type == CONVOI_TRANSPORTED_GOODS) {
		besitzer_p->buche(amount, COST_ALL_TRANSPORTED);
	}
}

void
convoi_t::init_financial_history()
{
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			financial_history[k][j] = 0;
		}
	}
}

sint32
convoi_t::get_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) {
		running_cost += fahr[i]->get_betriebskosten();
	}
	return running_cost;
}



void convoi_t::check_pending_updates()
{
	if (line_update_pending.is_bound()  &&  line.is_bound()) {
		destroy_win((long)fpl);	// close the schedule window, if open
		int aktuell = fpl->get_aktuell(); // save current position of schedule
		// destroy old schedule and all related windows
		if(fpl &&  !fpl->ist_abgeschlossen()) {
			destroy_win((long)fpl);
		}
		delete fpl;
		fpl = line->get_schedule()->copy();
		fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
		if(state!=INITIAL) {
			state = FAHRPLANEINGABE;
		}
		line_update_pending = linehandle_t();
	}
}



/*
 * the current state saved as color
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
 */
uint8 convoi_t::get_status_color() const
{
	if(state==INITIAL) {
		// in depot/under assembly
		return COL_WHITE;
	}
	else if(state==WAITING_FOR_CLEARANCE_ONE_MONTH  ||  state==CAN_START_ONE_MONTH  ||  hat_keine_route()) {
		// stuck or no route
		return COL_ORANGE;
	}
	else if(financial_history[0][CONVOI_PROFIT]+financial_history[1][CONVOI_PROFIT]<0) {
		// ok, not performing best
		return COL_RED;
	} else if((financial_history[0][CONVOI_OPERATIONS]|financial_history[1][CONVOI_OPERATIONS])==0) {
		// nothing moved
		return COL_YELLOW;
	}
	else if(has_obsolete) {
		return COL_BLUE;
	}
	// normal state
	return COL_BLACK;
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

		if(  route_index >= route.get_max_n()  ) {
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
				if(rb->is_free_route()  ||  rb->is_traffic_light()  ) {
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

		if ( route_index >= route.get_max_n() ) {
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
