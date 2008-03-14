/**
 * convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#include <math.h>
#include <stdlib.h>

#include "simdebug.h"
#include "simworld.h"
#include "simware.h"
#include "simplay.h"
#include "simconvoi.h"
#include "vehicle/simvehikel.h"
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

#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "dings/crossing.h"

#include "utils/tocstring.h"
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
static const char * state_names[] =
{
	"INITIAL",
	"FAHRPLANEINGABE",
	"ROUTING_1",
	"DUMMY",
	"DUMMY",
	"NO_ROUTE",
	"DRIVING",
	"LOADING",
	"WAITING_FOR_CLEARANCE",
	"WAITING_FOR_CLEARANCE_ONE_MONTH",
	"CAN_START",
	"CAN_START_ONE_MONTH",
	"",	// self destruct
	"WAITING_FOR_CLEARANCE_TWO_MONTHS",
	"CAN_START_TWO_MONTHS",
};


/**
 * Calculates speed of slowest vehicle in the given array
 * @author Hj. Matthaner
 */
static int calc_min_top_speed(const array_tpl<vehikel_t*>& fahr, int anz_vehikel)
{
	int min_top_speed = 9999999;
	for(int i=0; i<anz_vehikel; i++) {
		min_top_speed = min(min_top_speed, kmh_to_speed( fahr[i]->gib_besch()->gib_geschw() ) );
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

	line_update_pending = INVALID_LINE_ID;

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
	setze_name( "Unnamed" );
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

	// @author hsiegeln - deregister from line
	unset_line();

	self.detach();

	// force asynchronous recalculation
	if(fpl) {
		destroy_win((long)fpl);
		if(fpl->maxi()>0) {
			welt->set_schedule_counter();
		}
		delete fpl;
	}

	destroy_win((long)this);
}



void
convoi_t::laden_abschliessen()
{
	if(anz_vehikel>0) {
DBG_MESSAGE("convoi_t::laden_abschliessen()","state=%s, next_stop_index=%d", state_names[state] );
		for( unsigned i=0;  i<anz_vehikel;  i++ ) {
			vehikel_t* v = fahr[i];
			v->setze_erstes(i == 0);
			v->setze_letztes(i == anz_vehikel - 1U);
			// this sets the convoi and will renew the block reservation, if needed!
			v->setze_convoi(this);
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
		destroy();
	}
}



void convoi_t::rotate90( const sint16 y_size )
{
	last_stop_pos.rotate90( y_size );
	record_pos.rotate90( y_size );
	home_depot.rotate90( y_size );
	for(  int i=0;  i<=route.gib_max_n();  i++  ) {
		route.access_position_bei(i).rotate90( y_size );
	}
	if(fpl) {
		fpl->rotate90( y_size );
	}
}



/**
 * Gibt die Position des Convois zurück.
 * @return Position des Convois
 * @author Hj. Malthaner
 */
koord3d
convoi_t::gib_pos() const
{
	if(anz_vehikel > 0 && fahr[0]) {
		return state==INITIAL ? home_depot : fahr[0]->gib_pos();
	} else {
		return koord3d::invalid;
	}
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void
convoi_t::setze_name(const char *name)
{
	char buf[128];
	name_offset = sprintf(buf,"(%i) ",self.get_id() );
	tstrncpy(buf+name_offset, translator::translate(name), 116);
	tstrncpy(name_and_id, buf, lengthof(name_and_id));
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
	gib_besitzer()->buche(cost, COST_VEHICLE_RUN);

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
		int total_vehicle_weight = v->gib_gesamtgewicht();

		sum_friction_weight += v->gib_frictionfactor() * total_vehicle_weight;
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
		record_pos = fahr[0]->gib_pos().gib_2d();
	}
}



int convoi_t::get_vehicle_at_length(uint16 length)
{
	int current_length = 0;
	for( int i=0;  i<anz_vehikel;  i++  ) {
		current_length += fahr[i]->gib_besch()->get_length();
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

				setze_fahrplan(fpl);
				welt->set_schedule_counter();

				if(fpl->maxi()==0) {
					// no entry => no route ...
					state = NO_ROUTE;
					wait_lock = 0;
				}
				else {
					// Schedule changed at station
					// this station? then complete loading task else drive on
					state = (gib_pos() == fpl->eintrag[fpl->aktuell].pos ? LOADING : ROUTING_1);
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
		gib_besitzer()->bescheid_vehikel_problem(self,ziel);
		// wait 10s before next attempt
		wait_lock = 10000;
		return false;
	}
	return true;
}



/**
 * Berechne route zum nächsten Halt
 * @author Hanjsörg Malthaner
 */
void convoi_t::drive_to_next_stop()
{
	fahrplan_t *fpl = gib_fahrplan();
	assert(fpl != NULL);

	if(fpl->aktuell+1 < fpl->maxi()) {
		fpl->aktuell ++;
	}
	else {
		fpl->aktuell = 0;
	}
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
	if (line_update_pending != INVALID_LINE_ID) {
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

				if(fpl->maxi()==0) {
					state = NO_ROUTE;
				}
				else {
					// check first, if we are already there:
					if(fpl->aktuell>=fpl->maxi()  ||  v->gib_pos()==fpl->eintrag[fpl->get_aktuell()].pos) {
						drive_to_next_stop();
					}
					// Hajo: now calculate a new route
					drive_to(v->gib_pos(), fpl->eintrag[fpl->get_aktuell()].pos);
					if(route.gib_max_n() > 0) {
						vorfahren();
					}
					else {
						state = NO_ROUTE;
					}
					// finally, was there a record last time?
					if(max_record_speed>welt->get_record_speed(fahr[0]->gib_waytype())) {
						welt->notify_record(self, max_record_speed, record_pos);
					}
				}
			}
			break;

		case NO_ROUTE:
			// stucked vehicles
			{
				vehikel_t* v = fahr[0];

				if(fpl->maxi()==0) {
					// no entries => no route ...
					gib_besitzer()->bescheid_vehikel_problem(self, v->gib_pos());
					// wait 10s before next attempt
					wait_lock = 10000;
				}
				else {
					// Hajo: now calculate a new route
					drive_to(v->gib_pos(), fpl->eintrag[fpl->get_aktuell()].pos);
					if(route.gib_max_n() > 0) {
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
					if(haltestelle_t::gib_halt(welt,v->gib_pos()).is_bound()) {
						v->play_sound();
					}
				}
				if(restart_speed>=0) {
					akt_speed = restart_speed;
				}
				// wait a little before next try
				if(state==CAN_START  ||  state==CAN_START_ONE_MONTH) {
					wait_lock = 1000;
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
				// wait a little before next try
				if(state==WAITING_FOR_CLEARANCE_ONE_MONTH  ||  state==WAITING_FOR_CLEARANCE_TWO_MONTHS) {
					wait_lock = 2500;
				}
			}
			break;

		// must be here; may otherwise confuse window management
		case SELF_DESTRUCT:
			besitzer_p->buche( calc_restwert(), gib_pos().gib_2d(), COST_NEW_VEHICLE );
			welt->setze_dirty();
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
		gib_besitzer()->bescheid_vehikel_problem(self,koord3d::invalid);
		state = WAITING_FOR_CLEARANCE_TWO_MONTHS;
	}
	// check for traffic jam
	if(state==CAN_START) {
		state = CAN_START_ONE_MONTH;
	}
	else if(state==CAN_START_ONE_MONTH) {
		gib_besitzer()->bescheid_vehikel_problem(self,koord3d::invalid);
		state = CAN_START_TWO_MONTHS;
	}
	// check for obsolete vehicles in the convoi
	if(!has_obsolete  &&  welt->use_timeline()) {
		// convoi has obsolete vehicles?
		const int month_now = welt->get_timeline_year_month();
		has_obsolete = false;
		for(unsigned j=0;  j<gib_vehikel_anzahl();  j++ ) {
			if (fahr[j]->gib_besch()->is_retired(month_now)) {
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

		grund_t* gr = welt->lookup(v->gib_pos());
		if(gr) {
			// remove from blockstrecke
			v->setze_letztes(true);
			v->verlasse_feld();
		}
	}

	dep->convoi_arrived(self, self->gib_fahrplan()!=0);
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
		if(route.gib_max_n()>0) {
			home_depot = route.position_bei(0);
			fahr[0]->setze_pos( home_depot );
		}
		else {
			home_depot = fahr[0]->gib_pos();
		}

		alte_richtung = ribi_t::keine;
		no_load = false;

		state = ROUTING_1;
		// recalc weight and image
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->beladen( home_depot.gib_2d(), halthandle_t() );
		}
		// calc state for convoi
		calc_loading();


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
	alte_richtung = v->gib_fahrtrichtung();

	// check, what is at destination!
	const grund_t *gr = welt->lookup(v->gib_pos());
	depot_t *dp = gr->gib_depot();

	if(dp) {
		// ok, we are entering a depot
		char buf[128];

		// we still book the money for the trip; however, the frieght will be lost
		calc_gewinn();

		akt_speed = 0;
		sprintf(buf, translator::translate("!1_DEPOT_REACHED"), gib_name());
		message_t::get_instance()->add_message(buf, v->gib_pos().gib_2d(),message_t::convoi, gib_besitzer()->get_player_nr(), IMG_LEER);

		betrete_depot(dp);
	}
	else {
		// no depot reached, check for stop!
		halthandle_t halt = gr->ist_wasser() ? haltestelle_t::gib_halt(welt, v->gib_pos()) : gr->gib_halt();
		if(  halt.is_bound() &&  gr->gib_weg_ribi(v->gib_waytype())!=0  ) {
			// seems to be a stop, so book the money for the trip
			akt_speed = 0;
			halt->book(1, HALT_CONVOIS_ARRIVED);
			state = LOADING;
			go_on_ticks = WAIT_INFINITE;	// we will eventually wait from now on
		}
		else {
			// Neither depot nor station: waypoint
			drive_to_next_stop();
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
		v->setze_convoi(this);

		if(infront) {
			for(unsigned i = anz_vehikel; i > 0; i--) {
				fahr[i] = fahr[i - 1];
			}
			fahr[0] = v;
		} else {
			fahr[anz_vehikel] = v;
		}
		anz_vehikel ++;

		const vehikel_besch_t *info = v->gib_besch();
		if(info->gib_leistung()) {
			is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
		}
		sum_leistung += info->gib_leistung();
		sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
		sum_gewicht += info->gib_gewicht();
		min_top_speed = min( min_top_speed, kmh_to_speed( v->gib_besch()->gib_geschw() ) );
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;
		// check for obsolete
		if(!has_obsolete  &&  welt->use_timeline()) {
			has_obsolete = v->gib_besch()->is_retired( welt->get_timeline_year_month() );
		}
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	setze_erstes_letztes();

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

			v->setze_convoi(NULL);

			--anz_vehikel;
			fahr[anz_vehikel] = NULL;

			const vehikel_besch_t *info = v->gib_besch();
			sum_leistung -= info->gib_leistung();
			sum_gear_und_leistung -= info->gib_leistung()*info->get_gear();
			sum_gewicht -= info->gib_gewicht();
		}
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(anz_vehikel > 0) {
			setze_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

		// check for obsolete
		if(has_obsolete) {
			has_obsolete = false;
			const int month_now = welt->get_timeline_year_month();
			for(unsigned i=0; i<anz_vehikel; i++) {
				has_obsolete |= fahr[i]->gib_besch()->is_retired(month_now);
			}
		}

		// still requires electrifications?
		if(is_electric) {
			is_electric = false;
			for(unsigned i=0; i<anz_vehikel; i++) {
				if(fahr[i]->gib_besch()->gib_leistung()) {
					is_electric |= fahr[i]->gib_besch()->get_engine_type()==vehikel_besch_t::electric;
				}
			}
		}
	}
	return v;
}

void
convoi_t::setze_erstes_letztes()
{
	// anz_vehikel muss korrekt init sein
	if(anz_vehikel>0) {
		fahr[0]->setze_erstes(true);
		for(unsigned i=1; i<anz_vehikel; i++) {
			fahr[i]->setze_erstes(false);
			fahr[i - 1]->setze_letztes(false);
		}
		fahr[anz_vehikel - 1]->setze_letztes(true);
	}
	else {
		dbg->warning("convoi_t::setze_erstes_letzes()", "called with anz_vehikel==0!");
	}
}



bool convoi_t::setze_fahrplan(fahrplan_t * f)
{
	enum states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::setze_fahrplan()", "new=%p, old=%p", f, fpl);

	if(f == NULL) {
		return false;
	}

	// happens to be identical?
	if(fpl!=f) {
		// delete, if not equal
		if(fpl) {
			delete fpl;
		}
	}
	fpl = NULL;

	// rebuild destination for the new schedule
	fpl = f;

	// asynchronous recalculation of routes
	welt->set_schedule_counter();

	// remove wrong freight
	for(unsigned i=0; i<anz_vehikel; i++) {
		fahr[i]->remove_stale_freight();
	}

	// ok, now we have a schedule
	if(old_state!=INITIAL) {
		state = FAHRPLANEINGABE;
	}
	return true;
}



fahrplan_t *convoi_t::erzeuge_fahrplan()
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
	// invalid route?
	if(route.gib_max_n()<1) {
		return false;
	}

	// going backwards? then recalculate all
	ribi_t::ribi neue_richtung_rwr = ribi_t::rueckwaerts(fahr[0]->calc_richtung(route.position_bei(0).gib_2d(), route.position_bei(min(2,route.gib_max_n())).gib_2d()));
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
		grund_t *gr = welt->lookup(v->gib_pos());

		convoi_length += v->gib_besch()->get_length();

		if(gr==NULL  ||  pred!=NULL  &&  (abs(v->gib_pos().x-pred->gib_pos().x)>=2  ||  abs(v->gib_pos().y-pred->gib_pos().y)>=2)) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_richtung()","broken convoy (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}

		// now check, if ribi is straight and train is not
		ribi_t::ribi weg_ribi = gr->gib_weg_ribi_unmasked(v->gib_waytype());
		if(ribi_t::ist_gerade(weg_ribi)  &&  (weg_ribi|v->gib_fahrtrichtung())!=weg_ribi) {
			dbg->warning("convoi_t::go_alte_richtung()","convoy with wrong vehicle directions (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}

		pred = v;
	}
	int length = min((convoi_length/16)+4,route.gib_max_n()-1);	// maximum length in tiles to check

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
	koord3d pos = fahr[0]->gib_pos();
	assert(pos==route.position_bei(0));
	if(welt->lookup(pos)->gib_depot()) {
		return false;
	}
	else {
		for(i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];
			// eventually add current position to the route
			if(route.position_bei(0)!=v->gib_pos()  &&  route.position_bei(1)!=v->gib_pos()) {
				route.insert(v->gib_pos());
			}
			// eventually we need to add also a previous position to this path
			if(v->gib_besch()->get_length()>8  &&  i+1<anz_vehikel) {
				if(route.position_bei(0)!=v->gib_pos_prev()  &&  route.position_bei(1)!=v->gib_pos_prev()) {
					route.insert(v->gib_pos_prev());
				}
			}
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we must set the current route index (instead assuming 1)
	length = min((convoi_length/8),route.gib_max_n()-1);	// maximum length in tiles to check
	bool ok=false;
	for(i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		// this is such akward, since it takes into account different vehicle length
		const koord3d vehicle_start_pos = v->gib_pos();
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

		uint8 richtung = v->gib_fahrtrichtung();
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

	setze_akt_speed_soll( vehikel_t::SPEED_UNLIMITED );

	koord3d k0 = route.position_bei(0);
	grund_t *gr = welt->lookup(k0);
	if(gr  &&  gr->gib_depot()) {
		// start in depot
		for(unsigned i=0; i<anz_vehikel; i++) {
			vehikel_t* v = fahr[i];

			grund_t* gr = welt->lookup(v->gib_pos());
			if(gr) {
				gr->obj_remove(v);
				if(gr->ist_uebergang()) {
					crossing_t *cr = gr->find<crossing_t>(2);
					cr->release_crossing(v);
				}
				// eventually unreserve this
				schiene_t * sch0 = dynamic_cast<schiene_t *>( gr->gib_weg(fahr[i]->gib_waytype()) );
				if(sch0) {
					sch0->unreserve(v);
				}
			}
			v->neue_fahrt(0, true);
			v->betrete_feld();
		}

		// just advances the first vehicle
		vehikel_t* v0 = fahr[0];
		v0->setze_erstes(false); // switches off signal checks ...
		v0->darf_rauchen(false);
		steps_driven = 0;
		// drive half a tile:
		for(int i=0; i<anz_vehikel; i++) {
			fahr[i]->fahre_basis( 128<<12 );
		}
		v0->darf_rauchen(true);
		v0->setze_erstes(true); // switches on signal checks to reserve the next route

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
				grund_t* gr = welt->lookup(v->gib_pos());
				if(gr) {
					v->mark_image_dirty( v->gib_bild(), v->gib_hoff() );
					v->verlasse_feld();
					// eventually unreserve this
					schiene_t * sch0 = dynamic_cast<schiene_t *>( gr->gib_weg(fahr[i]->gib_waytype()) );
					if(sch0) {
						sch0->unreserve(v);
					}
				}
				/* we will set by this method the pos_prev to the starting point;
				 * otherwise it may be elsewhere, especially on curves and with already
				 * broken convois.
				 */
				v->setze_pos(k0);
				v->neue_fahrt(0, true);
				gr=welt->lookup(v->gib_pos());
				if(gr) {
					v->setze_pos(k0);
					v->betrete_feld();
				}
			}

			// move one train length to the start position ...
			int train_length = 0;
			for(unsigned i=0; i<anz_vehikel-1u; i++) {
				train_length += fahr[i]->gib_besch()->get_length(); // this give the length in 1/TILE_STEPS of a full tile
			}
			// in north/west direction, we leave the vehicle away to start as much back as possible
			ribi_t::ribi neue_richtung = fahr[0]->gib_fahrtrichtung();
			if(neue_richtung==ribi_t::sued  ||  neue_richtung==ribi_t::ost) {
				train_length += fahr[anz_vehikel-1]->gib_besch()->get_length();
			}
			else {
				train_length += 1;
			}
			train_length = max(1,train_length);

			// now advance all convoi until it is completely on the track
			fahr[0]->setze_erstes(false); // switches off signal checks ...
			for(unsigned i=0; i<anz_vehikel; i++) {
				vehikel_t* v = fahr[i];

				v->darf_rauchen(false);
				fahr[i]->fahre_basis( ((TILE_STEPS)*train_length)<<12 );
				train_length -= v->gib_besch()->get_length();	// this give the length in 1/TILE_STEPS of a full tile => all cars closely coupled!
				v->darf_rauchen(true);
			}
			fahr[0]->setze_erstes(true);
		}
		state = CAN_START;

		// to advance more smoothly
		int restart_speed=-1;
		if(fahr[0]->ist_weg_frei(restart_speed)) {
			// can reserve new block => drive on
			if(haltestelle_t::gib_halt(welt,k0).is_bound()) {
				fahr[0]->play_sound();
			}
			wait_lock = 0;
			state = DRIVING;
		}
	}

	// finally reserve route (if needed)
	for(unsigned i=0; i<anz_vehikel; i++) {
		// eventually reserve this
		schiene_t * sch0 = dynamic_cast<schiene_t *>( welt->lookup(fahr[i]->gib_pos())->gib_weg(fahr[i]->gib_waytype()) );
		if(sch0) {
			sch0->reserve(this->self);
		}
		else {
			break;
		}
	}

	INT_CHECK("simconvoi 711");
}


void
convoi_t::rdwr(loadsave_t *file)
{
	sint32 dummy;
	sint32 besitzer_n = welt->sp2num(besitzer_p);

	if(file->is_saving()) {
		file->wr_obj_id("Convoi");
		if(file->is_saving()) {
			line_id = line.is_bound() ? line->get_line_id() : INVALID_LINE_ID;
		}
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
		besitzer_p = welt->gib_spieler( besitzer_n );

		// Hajo: sanity check for values ... plus correction
		if(sp_soll < 0) {
			sp_soll = 0;
		}
	}

	file->rdwr_str(name_and_id+name_offset,116);
	if(file->is_loading()) {
		setze_name(name_and_id+name_offset);	// will add id automatically
	}

	koord3d dummy_pos;
	if(file->is_saving()) {
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->rdwr(file, true);
		}
	}
	else {
		bool override_monorail = false;
		is_electric = false;
		for(unsigned i=0; i<anz_vehikel; i++) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			vehikel_t *v = 0;

			if(override_monorail) {
				// ignore type for ancient monorails
				v = new monorail_waggon_t(welt, file);
			}
			else {
				switch(typ) {
					case ding_t::old_automobil:
					case ding_t::automobil: v = new automobil_t(welt, file);  break;
					case ding_t::old_waggon:
					case ding_t::waggon:    v = new waggon_t(welt, file);     break;
					case ding_t::old_schiff:
					case ding_t::schiff:    v = new schiff_t(welt, file);     break;
					case ding_t::old_aircraft:
					case ding_t::aircraft:    v = new aircraft_t(welt, file);     break;
					case ding_t::old_monorailwaggon:
					case ding_t::monorailwaggon:    v = new monorail_waggon_t(welt, file);     break;
					default:
						dbg->fatal("convoi_t::convoi_t()","Can't load vehicle type %d", typ);
				}
			}

			// no matching vehicle found?
			if(v->gib_besch()==NULL) {
				// will create orphan object, but better than crashing at deletion ...
				dbg->error("convoi_t::convoi_t()","Can't load vehicle and no replacement found!");
				i --;
				continue;
			}

			// in very old games, monorail was a railway
			// so we need to convert this
			// freight will be lost, but game will be loadable
			if(i==0  &&  v->gib_besch()->get_waytype()==monorail_wt  &&  v->gib_typ()==ding_t::waggon) {
				override_monorail = true;
				vehikel_t *v_neu = new monorail_waggon_t( v->gib_pos(), v->gib_besch(), v->gib_besitzer(), NULL );
				v->loesche_fracht();
				delete v;
				v = v_neu;
			}

			if(file->get_version()<99004) {
				dummy_pos.rdwr(file);
			}

			const vehikel_besch_t *info = v->gib_besch();

			// Hajo: if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				sum_leistung += info->gib_leistung();
				sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
				sum_gewicht += info->gib_gewicht();
				is_electric |= info->get_engine_type()==vehikel_besch_t::electric;
			}
			else {
				DBG_MESSAGE("convoi_t::rdwr()","no vehikel info!");
			}

			// some versions save vehicles after leaving depot with koord3d::invalid
			if(v->gib_pos()==koord3d::invalid) {
				state = INITIAL;
			}

			if(state!=INITIAL) {
				grund_t *gr;
				gr = welt->lookup(v->gib_pos());
				if(!gr) {
					gr = welt->lookup_kartenboden(v->gib_pos().gib_2d());
					if(gr) {
						dbg->error("convoi_t::rdwr()", "invalid position %s for vehicle %s in state %d (setting to ground %s)", (const char*)k3_to_cstr(v->gib_pos()), v->gib_name(), state, (const char*)k3_to_cstr(v->gib_pos()));
						v->setze_pos( gr->gib_pos() );
					}
					else {
						dbg->fatal("convoi_t::rdwr()", "invalid position %s for vehicle %s in state %d (setting to ground %s)", (const char*)k3_to_cstr(v->gib_pos()), v->gib_name(), state, (const char*)k3_to_cstr(v->gib_pos()));
					}
					state = INITIAL;
				}
				// add to blockstrecke
				if(gr  &&  v->gib_waytype()==track_wt  ||  v->gib_waytype()==monorail_wt) {
					schiene_t* sch = (schiene_t*)gr->gib_weg(v->gib_waytype());
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
		if(fpl == 0) {
			fpl = new fahrplan_t();
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
		line_update_pending = INVALID_LINE_ID;
	}

	if(file->get_version() > 84009) {
		home_depot.rdwr(file);
	}

	if(file->get_version()>=87001) {
		last_stop_pos.rdwr(file);
	}
	else {
		last_stop_pos = route.gib_max_n() > 1 ? route.position_bei(0) : (anz_vehikel > 0 ? fahr[0]->gib_pos() : koord3d(0, 0, 0));
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
				uint32 diff_ticks = welt->gib_zeit_ms()>go_on_ticks ? 0 : go_on_ticks-welt->gib_zeit_ms();
				file->rdwr_long( diff_ticks, "dt" );
			}
		}
		else {
			file->rdwr_long( go_on_ticks, "dt" );
			if(go_on_ticks!=WAIT_INFINITE) {
				go_on_ticks += welt->gib_zeit_ms();
			}
		}
	}

	// since 99015, the last stop will be maintained by the vehikels themselves
	if(file->get_version()<99015) {
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->last_stop_pos = last_stop_pos.gib_2d();
		}
	}
}


void
convoi_t::zeige_info()
{
	if(!in_depot()) {

		if(umgebung_t::verbose_debug) {
			dump();
			if (anz_vehikel > 0 && fahr[0]) {
				fahr[0]->dump();
			}
		}

		create_win( new convoi_info_t(self), w_info, (long)this );
	}
}


void convoi_t::info(cbuffer_t & buf) const
{
	const vehikel_t* v = fahr[0];
	if (v != NULL) {
    char tmp[128];

    sprintf(tmp, "\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), v->gib_besch()->gib_geschw(), get_running_cost() / 100.0);
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

		ALLOCA(uint32, max_loaded_waren, warenbauer_t::gib_waren_anzahl());
		memset( max_loaded_waren, 0, sizeof(uint32)*warenbauer_t::gib_waren_anzahl() );

		unsigned i;
		for(i=0; i<anz_vehikel; i++) {
			const vehikel_t* v = fahr[i];

			// first add to capacity indicator
			const ware_besch_t* ware_besch = v->gib_besch()->gib_ware();
			const uint16 menge = v->gib_besch()->gib_zuladung();
			if(menge>0  &&  ware_besch!=warenbauer_t::nichts) {
				max_loaded_waren[ware_besch->gib_index()] += menge;
			}

			// then add the actual load
			slist_iterator_tpl<ware_t> iter_vehicle_ware(v->gib_fracht());
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

				// if != 0 we could not joi it to existing => load it
				if(ware.menge != 0) {
					total_fracht.append(ware,16);
				}

			}

			INT_CHECK("simvehikel 876");
		}
		buf.clear();

		// apend info on total capacity
		slist_tpl <ware_t>capacity;
		for(i=0;  i<warenbauer_t::gib_waren_anzahl();  i++  ) {
			if(max_loaded_waren[i]>0  &&  i!=warenbauer_t::INDEX_NONE) {
				ware_t ware(warenbauer_t::gib_info(i));
				ware.menge = max_loaded_waren[i];
				if(ware.gib_catg()==0) {
					capacity.append( ware );
				} else {
					// append to category?
					slist_tpl<ware_t>::iterator j   = capacity.begin();
					slist_tpl<ware_t>::iterator end = capacity.end();
					while (j != end && j->gib_catg() < ware.gib_catg()) ++j;
					if (j != end && j->gib_catg() == ware.gib_catg()) {
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
	if(gib_besitzer() != NULL &&
		gib_besitzer() != welt->get_active_player()) {
		return;
	}

	// manipulation of schedule not allowd while:
	// - just starting
	// - a line update is pending
	if(  state==FAHRPLANEINGABE  ||  line_update_pending!=INVALID_LINE_ID  ) {
		create_win( new news_img("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!"), w_time_delete, magic_none );
		return;
	}

	if(state==DRIVING) {
		// book the current value of goods
		calc_gewinn();
	}

	akt_speed = 0;	// stop the train ...
	state = FAHRPLANEINGABE;
	alte_richtung = fahr[0]->gib_fahrtrichtung();

	// Fahrplandialog oeffnen
	create_win( new fahrplan_gui_t(fpl,gib_besitzer(),self), w_info, (long)fpl );
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Nachfolger von
 * vor gelten - daher muß vor != NULL sein..
 */
bool
convoi_t::pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
	const vehikel_besch_t *soll;

	if(!vor->gib_nachfolger_count()) {
		// Alle Nachfolger erlaubt
		return true;
	}
	for(int i=0; i < vor->gib_nachfolger_count(); i++) {
		soll = vor->gib_nachfolger(i);
		//DBG_MESSAGE("convoi_t::pruefe_an_index()",
		//    "checking successor: should be %d, is %d",
		//    soll ? soll->gib_name() : "none",
		//    hinter ? hinter->gib_name() : "none");

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

	if(!hinter->gib_vorgaenger_count()) {
		// Alle Vorgänger erlaubt
		return true;
	}
	for(int i=0; i < hinter->gib_vorgaenger_count(); i++) {
		soll = hinter->gib_vorgaenger(i);
		//DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
		//	     "checking predecessor: should be %s, is %s",
		//	     soll ? soll->gib_name() : "none",
		//	     vor ? vor->gib_name() : "none");

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
	bool ok = (anz_vehikel == 0 || pruefe_vorgaenger(NULL, fahr[0]->gib_besch()));
	unsigned i;

	const vehikel_t* pred = fahr[0];
	for(i = 1; ok && i < anz_vehikel; i++) {
		const vehikel_t* v = fahr[i];
		ok = pruefe_nachfolger(pred->gib_besch(), v->gib_besch()) &&
				 pruefe_vorgaenger(pred->gib_besch(), v->gib_besch());
		pred = v;
	}
	if(ok) {
		ok = pruefe_nachfolger(pred->gib_besch(), NULL);
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
	const koord k = fpl->eintrag[fpl->aktuell].pos.gib_2d();

	halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag[fpl->aktuell].pos);
	// eigene haltestelle ?
	if (halt.is_bound()) {
		const spieler_t* owner = halt->gib_besitzer();
		if (owner == gib_besitzer()  || owner == welt->gib_spieler(1)) {
			// loading/unloading ...
			hat_gehalten(k, halt);
		}
	}

	if(go_on_ticks==WAIT_INFINITE  &&  fpl->eintrag[fpl->aktuell].waiting_time_shift>0) {
		go_on_ticks = welt->gib_zeit_ms() + (welt->ticks_per_tag >> (16-fpl->eintrag[fpl->aktuell].waiting_time_shift));
	}

	INT_CHECK("simconvoi 1077");

	// Nun wurde ein/ausgeladen werden
	if(loading_level>=loading_limit  ||  no_load  ||  welt->gib_zeit_ms()>go_on_ticks)  {

		if(withdraw  &&  loading_level==0) {
			// destroy when empty
			besitzer_p->buche( calc_restwert(), COST_NEW_VEHICLE );
			destroy();
			return;
		}

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<anz_vehikel; i++) {
			book(gib_vehikel(i)->gib_fracht_max()-gib_vehikel(i)->gib_fracht_menge(), CONVOI_CAPACITY);
		}

		// Advance schedule
		drive_to_next_stop();
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
		gewinn += v->calc_gewinn(v->last_stop_pos, v->gib_pos().gib_2d() );
		v->last_stop_pos = v->gib_pos().gib_2d();
	}

	if(gewinn) {
		besitzer_p->buche(gewinn, fahr[0]->gib_pos().gib_2d(), COST_INCOME);
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
	grund_t *gr=welt->lookup(fahr[0]->gib_pos());

	int station_lenght=0;
	if(gr->ist_wasser()) {
		// habour has any size
		station_lenght = 24*16;
	}
	else {
		// calculate real station length
		koord zv = koord( ribi_t::rueckwaerts(fahr[0]->gib_fahrtrichtung()) );
		koord pos = fahr[0]->gib_pos().gib_2d();
		const planquadrat_t *plan=welt->lookup(pos);
		while(plan  &&  plan->gib_halt()==halt) {
			station_lenght += TILE_STEPS;
			pos += zv;
			plan = welt->lookup(pos);
		}
	}

	// only load vehicles in station
	// don't load when vehicle is being withdrawn
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];

		station_lenght -= v->gib_besch()->get_length();
		if(station_lenght<0) {
			break;
		}

		// calc_revenue
		gewinn += v->calc_gewinn(v->last_stop_pos, v->gib_pos().gib_2d() );
		v->last_stop_pos = v->gib_pos().gib_2d();

		freight_info_resort |= v->entladen(k, halt);
		if(!no_load) {
			// do not load anymore
			freight_info_resort |= v->beladen(k, halt);
		}

	}

	// any loading went on?
	calc_loading();
	loading_limit = fpl->eintrag[fpl->aktuell].ladegrad;

	if(gewinn) {
		besitzer_p->buche(gewinn, fahr[0]->gib_pos().gib_2d(), COST_INCOME);
		jahresgewinn += gewinn;

		book(gewinn, CONVOI_PROFIT);
		book(gewinn, CONVOI_REVENUE);
	}
}


uint32 convoi_t::calc_restwert() const
{
	int result = 0;

	for(unsigned i=0; i<anz_vehikel; i++) {
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
		fracht_max += v->gib_fracht_max();
		fracht_menge += v->gib_fracht_menge();
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
		fahr[0]->setze_convoi(NULL);
	}

	for(int i=anz_vehikel-1;  i>=0; i--) {
		if(  !fahr[i]->get_flag( ding_t::not_on_map )  ) {
			// remove from rails/roads/crossings
			grund_t *gr = welt->lookup(fahr[i]->gib_pos());
			fahr[i]->setze_letztes( true );
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
    fprintf(stderr, "convoi::dump()");
    fprintf(stderr, "anz_vehikel = %d\n", anz_vehikel);
    fprintf(stderr, "wait_lock = %d\n", wait_lock);
    fprintf(stderr, "besitzer_n = %d\n", welt->sp2num(besitzer_p));
    fprintf(stderr, "akt_speed = %d\n", akt_speed);
    fprintf(stderr, "akt_speed_soll = %d\n", akt_speed_soll);
    fprintf(stderr, "sp_soll = %d\n", sp_soll);
    fprintf(stderr, "state = %d\n", state);
    fprintf(stderr, "statename = %s\n", state_names[state]);
    fprintf(stderr, "alte_richtung = %d\n", alte_richtung);
    fprintf(stderr, "jahresgewinn = %lld\n", jahresgewinn);

    fprintf(stderr, "name = '%s'\n", name_and_id);
    fprintf(stderr, "line_id = '%d'\n", line_id);
    fprintf(stderr, "fpl = '%p'\n", fpl);
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
	line = org_line;
	line_id = org_line->get_line_id();
	fahrplan_t * new_fpl= new fahrplan_t( org_line->get_fahrplan() );
	setze_fahrplan(new_fpl);
	line->add_convoy(self);
	// force asynchronous recalculation
	welt->set_schedule_counter();
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



void convoi_t::prepare_for_new_schedule(fahrplan_t *f)
{
	alte_richtung = fahr[0]->gib_fahrtrichtung();

	state = FAHRPLANEINGABE;
	setze_fahrplan(f);

	// Hajo: setze_fahrplan sets state to ROUTING_1
	// need to undo that
	state = FAHRPLANEINGABE;
}



void convoi_t::book(sint64 amount, int cost_type)
{
	if (cost_type>MAX_CONVOI_COST) {
		// THIS SHOULD NEVER HAPPEN!
		// CHECK CODE
		DBG_MESSAGE("convoi_t::book()", "function was called with cost_type: %i, which is not valid (MAX_CONVOI_COST=%i)", cost_type, MAX_CONVOI_COST);
		return;
	}

	financial_history[0][cost_type] += amount;
	if (line.is_bound()) {
		line->book(amount, simline_t::convoi_to_line_catgory[cost_type] );
	}

	if (cost_type == CONVOI_TRANSPORTED_GOODS) {
		besitzer_p->buche(amount, COST_TRANSPORTED_GOODS);
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
	for (unsigned i = 0; i<gib_vehikel_anzahl(); i++) {
		running_cost += fahr[i]->gib_betriebskosten();
	}
	return running_cost;
}



void convoi_t::check_pending_updates()
{
	if (line_update_pending != INVALID_LINE_ID) {
		linehandle_t line = besitzer_p->simlinemgmt.get_line_by_id(line_update_pending);
		// the line could have been deleted in the meantime
		// if line was deleted ignore line update; convoi will continue with existing schedule
		if(line.is_bound()) {
			int aktuell = fpl->get_aktuell(); // save current position of schedule
			fpl = new fahrplan_t(line->get_fahrplan());
			fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
			if(state!=INITIAL) {
				state = FAHRPLANEINGABE;
			}
		}
		line_update_pending = INVALID_LINE_ID;
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
