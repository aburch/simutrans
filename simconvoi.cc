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

#include "bauer/vehikelbauer.h"

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
	sum_gesamtgewicht = sum_gewicht = sum_gear_und_leistung = sum_leistung = power_from_steam = power_from_steam_with_gear = 0;
	previous_delta_v = 0;
	min_top_speed = 9999999;

	withdraw = false;
	has_obsolete = false;
	no_load = false;
	replace = false;
	depot_when_empty = false;

	jahresgewinn = 0;

	max_record_speed = 0;
	akt_speed_soll = 0;            // Sollgeschwindigkeit
	akt_speed = 0;                 // momentane Geschwindigkeit
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
	autostart = true;
	wait_lock = 0;
	go_on_ticks = WAIT_INFINITE;

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

	// @author hsiegeln - deregister from line
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
	sum_gesamtgewicht = 0; // "Total weight" (Google)
	// calculate total friction
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t* v = fahr[i];
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
	
		// Slight reduction of extent to which speed increases friction.
		// @author: jamespetts, August 2009
		const sint32 adjusted_speed = akt_speed >> 2;
		const sint32 deccel = ( ( ((adjusted_speed)*sum_friction_weight)>>5 )*(adjusted_speed) ) / 33 + (sum_gesamtgewicht*64);	// this order is needed to prevent overflows!

		// prissi:
		// integer sucks with planes => using floats ...
		//sint32 delta_v =  (sint32)( ( (double)( (akt_speed>akt_speed_soll?0l:sum_gear_und_leistung) - deccel)*(double)delta_t)/(double)sum_gesamtgewicht);
		sint32 delta_v =  (sint32)( ( (double)( (akt_speed>akt_speed_soll ? 0l : calc_adjusted_power()) - deccel) * (double)delta_t )/ (double)sum_gesamtgewicht);
		//"leistung" = "performance" (Google)

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

sint32 convoi_t::calc_adjusted_power()
{
	switch(fahr[0]->get_waytype())
	{
		// Adjustment of power only applies to steam vehicles
		// with direct drive (as in steam railway locomotives),
		// not steam vehicles with geared driving (as with steam
		// road vehicles) or propellor shaft driving (as with
		// water craft). Aircraft and maglev, although not
		// likely ever to be steam, cannot conceivably have
		// direct drive. 

	case road_wt:
	case water_wt:
	case air_wt:
	case maglev_wt:
	case invalid_wt:
	case ignore_wt:
		return sum_gear_und_leistung;
	}
	const uint16 max_speed = fahr[0]->get_besch()->get_geschw();
	float highpoint_speed = (max_speed >= 60) ? max_speed - 30 : 30;
	const uint16 current_speed = speed_to_kmh(akt_speed);
	
	// Within 15% of top speed - locomotive less efficient
	float high_speed = (float)max_speed * 0.85F; 
	
	if(power_from_steam < 1 || current_speed > highpoint_speed && current_speed < high_speed)
	{
		// Either no steam engines, or going fast
		// enough that it makes no difference,
		// so the simple formula prevails.
		return sum_gear_und_leistung;
	}
	// There must be a steam locomotive here.
	// So, reduce the power at higher and lower speeds.
	// Should be approx (for medium speed locomotive):
	// 40% power at 15kph 70% power at 25kph;
	// 85% power at 32kph; and 100% power at >50kph.
	// See here for details: http://www.railway-technical.com/st-vs-de.shtml

	//This is needed to add back at the end.
	const uint32 power_without_steam = sum_gear_und_leistung - power_from_steam_with_gear;
	
	float speed_factor;
	
	if(power_from_steam > 500)
	{
		speed_factor = 1.0F;
	}
	else
	{
		float difference = 400.0F - (power_from_steam  - 50.0F);
		float proportion = difference / 400.0F;
		float factor = 0.66F * proportion;
		speed_factor = 1 + factor;
	}
	
	//These values are needed to apply different power reduction factors
	//depending on the maximum speed.

	float lowpoint_speed = highpoint_speed * 0.3F;
	float midpoint_speed = lowpoint_speed * 2.0F;

	if(current_speed <= lowpoint_speed)
	{
		speed_factor *= 0.4F;
	}
	else if(current_speed <= midpoint_speed)
	{
		float speed_differential_actual = (float)current_speed - (float)lowpoint_speed;
		float speed_differential_maximum = (float)midpoint_speed - (float)lowpoint_speed;
		float factor_modification = speed_differential_actual / speed_differential_maximum;
		speed_factor *= ((factor_modification * 0.4F) + 0.4F);
	}
	else if(current_speed <= high_speed)
	{
		// Not at high speed
		float speed_differential_actual = (float)current_speed - (float)midpoint_speed;
		float speed_differential_maximum = (float)highpoint_speed - (float)lowpoint_speed;
		float factor_modification = speed_differential_actual / speed_differential_maximum;
		speed_factor *= ((factor_modification * 0.15F) + 0.8F);
	}
	else
	{
		// Must be within 15% of top speed here.
		float speed_differential_actual = (float)max_speed - (float)current_speed;
		float speed_differential_maximum = (float)max_speed - (float)high_speed;
		float factor_modification = speed_differential_actual / speed_differential_maximum;
		speed_factor *= ((factor_modification * 0.15F) + 0.8F);
	}

	uint32 modified_power_from_steam = power_from_steam_with_gear * speed_factor;
	return modified_power_from_steam + power_without_steam;
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
			// schedule window closed?
			if(fpl!=NULL  &&  fpl->ist_abgeschlossen()) {

				set_schedule(fpl);
				
				// Added by : Knightly
				// Purpose  : Remove the original schedule after update
				if (old_fpl)
				{
					delete old_fpl;
					old_fpl = NULL;
				}

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
			else {
				// still entring => check only each 500ms for change
				wait_lock = 500;
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
				sp_soll += (akt_speed*delta_t);
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

	switch(state) {

		case INITIAL:

			// If there is a pending replacement, just do it
			if (get_replace() && replacing_vehicles.get_count()>0) {
				const grund_t *gr = welt->lookup(home_depot);
				depot_t *dep;
				if ( gr && (dep=gr->get_depot()) ) {
					bool keep_name=(anz_vehikel>0 && strcmp(get_name(),fahr[0]->get_besch()->get_name())!=0);						
					vector_tpl<vehikel_t*> new_vehicles;

					// Acquire the new one
					ITERATE(replacing_vehicles,i)
					{
						vehikel_t* veh = NULL;
						// First - check whether there are any of the required vehicles already
						// in the convoy (free)
						for(uint8 k = 0; k < anz_vehikel; k++)
						{
							if(fahr[k]->get_besch() == replacing_vehicles[i])
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
									if(replacing_vehicles[i] == fahr[j]->get_besch()->get_upgrades(c))
									{
										veh = remove_vehikel_bei(j);
										veh->set_besch(replacing_vehicles[i]);
										dep->buy_vehicle(veh->get_besch(), true);
										goto end_loop;
									}
								}
							}
end_loop:	
							if(veh == NULL)
							{
								// Fourth - if all else fails, buy from new (expensive).
								veh = dep->buy_vehicle(replacing_vehicles[i], false);
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
					set_replace(false);
					replacing_vehicles.clear();
					if (line.is_bound()) {
						line->recalc_status();
						if (line->get_replacing_convoys_count()==0) {
							char buf[128];
							sprintf(buf, translator::translate("Replacing\nvehicles of\n%-20s\ncompleted"), line->get_name());
							welt->get_message()->add_message(buf, home_depot.get_2d(),message_t::convoi, PLAYER_FLAG|get_besitzer()->get_player_nr(), IMG_LEER);
						}

					}
					if (autostart) {
						dep->start_convoi(self);
					}
				}
			}
			break;

		case LOADING:
			laden();
			if(get_depot_when_empty() && has_no_cargo())
			{
				go_to_depot(false);
			}
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
						vorfahren(); //"Drive"
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
					if(haltestelle_t::get_halt(welt,v->get_pos(),besitzer_p).is_bound()) {
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
	
	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		base_comfort += fahr[i]->get_comfort();
	}
	if(anz_vehikel > 1)
	{
		// Avoid division if possible
		base_comfort /= anz_vehikel;
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
	add_running_cost(-(sint32)welt->calc_adjusted_monthly_figure(running_cost));

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
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr[i]->beladen( home_depot.get_2d(), halthandle_t() );
		}
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
		if(reversed)
		{
			//Always enter a depot facing forward
			reversable = fahr[anz_vehikel - 1]->get_besch()->get_can_lead_from_rear() || (anz_vehikel == 1 && fahr[0]->get_besch()->is_bidirectional());
			reverse_order(reversable);
		}

		// we still book the money for the trip; however, the frieght will be lost
		last_departure_time = welt->get_zeit_ms();

		akt_speed = 0;
		if (!replace || !autostart) {
			sprintf(buf, translator::translate("!1_DEPOT_REACHED"), get_name());
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
		if(info->get_engine_type() == vehikel_besch_t::steam)
		{
			power_from_steam += info->get_leistung();
			power_from_steam_with_gear += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
		}
		sum_gear_und_leistung += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
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

	heaviest_vehicle = calc_heaviest_vehicle();
	longest_loading_time = calc_longest_loading_time();

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

			// Added by		: Knightly
			// Purpose		: To recalculate the list of supported goods category
			recalc_catg_index();

			const vehikel_besch_t *info = v->get_besch();
			sum_leistung -= info->get_leistung();
			if(info->get_engine_type() == vehikel_besch_t::steam)
			{
				power_from_steam -= info->get_leistung();
				power_from_steam_with_gear -= info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
			}
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

void
convoi_t::set_erstes_letztes() //"set only last" (Google)
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
	enum states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::set_fahrplan()", "new=%p, old=%p", f, fpl);

	if(f == NULL) 
	{
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
	if(fpl != f) 
	{
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
convoi_t::vorfahren() //"move forward" (Babelfish)
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
		state = CAN_START;

		// to advance more smoothly
		int restart_speed=-1;
		if(fahr[0]->ist_weg_frei(restart_speed)) {
			// can reserve new block => drive on
			if(haltestelle_t::get_halt(welt,k0,besitzer_p).is_bound()) {
				fahr[0]->play_sound();
			}
			//wait_lock = 0;
			wait_lock = reverse_delay;
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
			if(fahr[a]->get_besch()->get_leistung() > 0)
			{
				// Check for double-headed tender locomotives
				a += fahr[a]->get_besch()->get_nachfolger_count();
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
				if(info->get_engine_type() == vehikel_besch_t::steam)
				{
					power_from_steam += info->get_leistung();
					power_from_steam_with_gear += info->get_leistung() * info->get_gear() * welt->get_einstellungen()->get_global_power_factor();
				}
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
				file->rdwr_longlong(financial_history[k][j], " ");
			}
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
				sint64 diff_ticks= welt->get_zeit_ms()>go_on_ticks ? 0 : go_on_ticks-welt->get_zeit_ms();
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
		file->rdwr_bool(replace, "");
		file->rdwr_bool(autostart, "");
		file->rdwr_bool(depot_when_empty, "");

		uint16 replacing_vehicles_count;

		if(file->is_saving())
		{
			replacing_vehicles_count = replacing_vehicles.get_count();
			file->rdwr_short(replacing_vehicles_count, "");
			ITERATE(replacing_vehicles, i)
			{
				const char *s = replacing_vehicles[i]->get_name();
				file->rdwr_str(s);
			}
		}
		else
		{
			
			file->rdwr_short(replacing_vehicles_count, "");
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
					replacing_vehicles.append(besch);
				}
			}
		}
	}
	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_longlong(last_departure_time, "");
		for(uint8 i = 0; i < MAX_CONVOI_COST; i ++)
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
}


void
convoi_t::zeige_info()
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

		ALLOCA(uint32, capacity_by_catg_index, warenbauer_t::get_max_catg_index());
		memset( capacity_by_catg_index, 0, sizeof(uint32)*warenbauer_t::get_max_catg_index() );

		unsigned i;
		for(i=0; i<anz_vehikel; i++) {
			const vehikel_t* v = fahr[i];

			// first add to capacity indicator
			const ware_besch_t* ware_besch = v->get_besch()->get_ware();
			const uint16 menge = v->get_besch()->get_zuladung();
			if(menge>0  &&  ware_besch!=warenbauer_t::nichts) {
				capacity_by_catg_index[ware_besch->get_catg_index()] += menge;
			}

			// then add the actual load
#ifdef SLIST_FREIGHT
			slist_iterator_tpl<ware_t> iter_vehicle_ware(v->get_fracht());
			while(iter_vehicle_ware.next()) 
			{
				ware_t ware = iter_vehicle_ware.get_current();
#else
			const vector_tpl<ware_t> &freight = v->get_fracht();
			ITERATE(freight, j)
			{
				ware_t ware = freight[j];
#endif
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
#ifdef SLIST_FREIGHT
		slist_tpl <ware_t>capacity;
#else
		vector_tpl <ware_t>capacity;
#endif
		
		for( uint8 j = 0; j < warenbauer_t::get_max_catg_index(); j ++ ) 
		{
			if( capacity_by_catg_index[j] > 0 ) 
			{
				ware_t ware( warenbauer_t::get_info_catg_index(j) );
				ware.menge = capacity_by_catg_index[j];
				capacity.append( ware );
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
		last_departure_time = welt->get_zeit_ms();
	}

	akt_speed = 0;	// stop the train ...
	state = FAHRPLANEINGABE;
	alte_richtung = fahr[0]->get_fahrtrichtung();

	// Added by : Knightly
	// Purpose  : To keep a copy of the original schedule before opening schedule window
	if (fpl)
	{
		old_fpl = fpl->copy();
	}

	// Fahrplandialog oeffnen
	create_win( new fahrplan_gui_t(fpl,get_besitzer(),self), w_info, (long)fpl );
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Nachfolger von
 * vor gelten - daher muß vor != NULL sein..
 * TRANSLATION: (Google)
 * Vehicles are often triggered only in certain combinations restrictions are approved,
 * the successor to continue to apply - therefore must be done before != NULL.
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
bool
convoi_t::pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
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



bool
convoi_t::pruefe_alle() //"examine all" (Babelfish)
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
#ifdef SLIST_FREIGHT
		slist_iterator_tpl<ware_t> iter_cargo(fahr[i]->get_fracht());
		while(iter_cargo.next())
		{
			iter_cargo.access_current().add_distance(journey_distance);
#else
		vector_tpl<ware_t> &freight = fahr[i]->get_freight_to_change();
		ITERATE(freight,j)
		{
			freight[j].add_distance(journey_distance);
#endif
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

	INT_CHECK("simconvoi 1077");
	
	current_stop = fpl->get_aktuell();

	// Nun wurde ein/ausgeladen werden
	// "Now, a / unloaded" (Google)
	if(loading_level >= loading_limit  ||  no_load  ||  welt->get_zeit_ms() > go_on_ticks)  
	{

		if(withdraw && has_no_cargo()) 
		{
			// destroy when empty
			besitzer_p->buche( calc_restwert(), COST_NEW_VEHICLE );
			besitzer_p->buche( -calc_restwert(), COST_ASSETS );
			welt->set_dirty();
			destroy();
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
		
	// This is the minimum time it takes for loading
	//wait_lock = WTT_LOADING;
	wait_lock = longest_loading_time;
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
	
	if(ware.is_passenger())
	{
		//Passengers care about their comfort
		const uint8 tolerable_comfort = calc_tolerable_comfort(journey_minutes);
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
			const float multiplier = welt->get_einstellungen()->get_max_luxury_bonus();
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
			const float multiplier = welt->get_einstellungen()->get_max_discomfort_penalty();
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

const uint16 convoi_t::calc_adjusted_speed_bonus(uint16 base_bonus, uint32 distance, karte_t* w)
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
	// don't load if vehicle is being withdrawn
	int convoy_length = 0;
	bool second_run = anz_vehikel <= 1;
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

		if(!no_load) 
		{
			// load 
			freight_info_resort |= v->beladen(k, halt, second_run);
		}
		else 
		{
			// do not load anymore - but call beladen() to recalculate vehikel weight
			freight_info_resort |= v->beladen(k, halthandle_t());
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

	// any loading went on?
	calc_loading();
	loading_limit = fpl->get_current_eintrag().ladegrad; //"charge degree" (??) (Babelfish)

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
		fahr[i]->before_delete();
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
	if(!org_line.is_bound()) 
	{
		return;
	}

	/*
	minivec_tpl<uint8> supported_categories;

	for(uint8 i = 0; i < anz_vehikel; i ++)
	{
		const uint8 catg_index = fahr[i]->get_fracht_typ()->get_catg_index();
		supported_categories.append(catg_index);
	}
	*/

	if(line.is_bound()) 
	{
		unset_line();
	}
	else if(fpl && fpl->get_count() > 0) 
	{
		// since this schedule is no longer served

		// Added by : Knightly
		haltestelle_t::refresh_routing(fpl, goods_catg_index, besitzer_p, welt->get_einstellungen()->get_default_path_option());
	}
	
	line = org_line;
	line_id = org_line->get_line_id();
	schedule_t *new_fpl= org_line->get_schedule()->copy();
	set_schedule(new_fpl);

	line->add_convoy(self);
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

void
convoi_t::init_financial_history()
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
	uint32 running_cost = 0;
	for (unsigned i = 0; i<get_vehikel_anzahl(); i++) {
		running_cost += fahr[i]->get_fixed_maintenance(welt); 
	}
	return running_cost;
}

void convoi_t::check_pending_updates()
{
	if (line_update_pending.is_bound()  &&  line.is_bound()) {
		int aktuell = fpl->get_aktuell(); // save current position of schedule
		line = line_update_pending;
		line_update_pending = linehandle_t();
		// destroy old schedule and all related windows
		if(fpl &&  !fpl->ist_abgeschlossen()) {
			fpl->copy_from( line->get_schedule() );
			fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
			fpl->eingabe_beginnen();
		}
		else {
			fpl->copy_from( line->get_schedule() );
			fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
		}
		if(state!=INITIAL) {
			state = FAHRPLANEINGABE;
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

void convoi_t::set_replacing_vehicles(const vector_tpl<const vehikel_besch_t *> *rv)
{
	replacing_vehicles.clear();
	replacing_vehicles.resize(rv->get_count());  // To save some memory
	for (unsigned int i=0; i<rv->get_count(); ++i) {
		replacing_vehicles.append((*rv)[i]);
	}
}
 

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
		koord3d depot_pos = route.position_bei(route.get_max_n());
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
			if(!shortest_route->empty() && koord_distance(pos.get_2d(),get_pos().get_2d())>=shortest_route->get_max_n()) 
			{
				// the current route is already shorter, no need to search further
				continue;
			}
			bool found = get_vehikel(0)->calc_route(get_pos(), pos, 50, route); // do not care about speed
			if (found) 
			{
				if(  route->get_max_n() < shortest_route->get_max_n()    ||    shortest_route->empty()  ) 
				{
					shortest_route->kopiere(route);
					home = pos;
				}
			}
		}
		DBG_MESSAGE("shortest route has ", "%i hops", shortest_route->get_max_n());

		if(!shortest_route->empty()) 
		{
			koord3d depot_pos = route->position_bei(route->get_max_n());
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
	for(uint8 i=0;  i<anz_vehikel;  i++) {
		tiles += fahr[i]->get_besch()->get_length();
	}
	return (tiles+16-1)/16;
}



// if withdraw and empty, then self destruct
void convoi_t::set_withdraw(bool new_withdraw)
{
	withdraw = new_withdraw;
	if(  withdraw  &&  loading_level==0  ) {
		// test if convoi in depot
		grund_t *gr = welt->lookup( get_pos());
		if (gr && gr->get_depot()) {
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

// Added by		: Knighty
// Adapted from : simline_t
// Purpose		: To recalculate the list of supported goods categories
void convoi_t::recalc_catg_index()
{
	goods_catg_index.clear();

	for(uint8 i = 0; i < anz_vehikel; i++) 
	{
		if (fahr[i]->get_fracht_max() > 0)
		{
			const ware_besch_t *ware_type = fahr[i]->get_fracht_typ();
			if (ware_type != warenbauer_t::nichts) 
			{
				goods_catg_index.append_unique(ware_type->get_catg_index(), 1);
			}
		}
	}
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

/**
 * Bernd Gabriel, 23.06.2009: convoi_metrics_t: 
 *
 * Extracted from gui_convoy_assembler_t::zeichnen() and gui_convoy_label_t::zeichnen()
 */

#ifndef MAXUINT32
#define MAXUINT32 ((uint32)~((uint32)0))
#endif

void convoy_metrics_t::reset()
{
	power = 0;
	length = 0;
	vehicle_weight = 0;
	min_freight_weight = 0;
	max_freight_weight = 0;
	max_top_speed = MAXUINT32;	
}

void convoy_metrics_t::get_possible_freight_weight(uint8 catg_index, uint32 &min_weight, uint32 &max_weight)
{
	max_weight = 0;
	min_weight = MAXUINT32;
	for (uint16 j=0; j<warenbauer_t::get_waren_anzahl(); j++) {
		const ware_besch_t &ware = *warenbauer_t::get_info(j);
		if (ware.get_catg_index() == catg_index) {
			uint32 weight = ware.get_weight_per_unit();
			if (max_weight < weight) 
			{
				max_weight = weight;
			}
			if (min_weight > weight) 
			{
				min_weight = weight;
			}
		}
	}
	// No freight of given category found? Then there is no min weight!
	if (min_weight == MAXUINT32) 
	{
		min_weight = 0;
	}
}

void convoy_metrics_t::add_vehicle(const vehikel_besch_t &besch)
{
	length += besch.get_length();
	vehicle_weight += besch.get_gewicht();
	uint32 payload = besch.get_zuladung();
	if (payload > 0)
	{
		uint32 min_weight, max_weight;
		get_possible_freight_weight(besch.get_ware()->get_catg_index(), min_weight, max_weight);
		min_freight_weight += (min_weight * payload + 499) / 1000;
		max_freight_weight += (max_weight * payload + 499) / 1000;
	}
	if (max_top_speed > besch.get_geschw())
	{
		max_top_speed = besch.get_geschw();
	}
}

void convoy_metrics_t::calc(convoi_t &cnv)
{
	reset();
	for(unsigned i = cnv.get_vehikel_anzahl();  i-- > 0; ) 
	{
		add_vehicle(*cnv.get_vehikel(i)->get_besch());
	}
	//BG, 30.08.2009: cannot use cnv.calc_adjusted_power() here.
	/* 
	 * Convoy_metrics are used to display them in the depot or convoy frame only.
	 * They show the top speeds for given power and full resp. empty convoy.
	 * Nominal power and top speed given in vehikel_besch_t both are max values and this power is valid for this top speed.
	 * Thus we need the max power here instead of the actual speed controlled power.
	 *
	 * It looked a bit strange while playing: 2 identical steam trains (8ft Stirling with 1 KBay-Mail and 6 KBay-Pax)
	 * The convoy frame of the standing train said, convoy was able to run 53..59 km/h, 
	 * while frame of the running train said, convoy could run 85 km/h.
	 * The user (at least me) expects always the same value: the top speed at top speed power, which is the 85.
	 *
	 * cnv.calc_adjusted_power() returns the power at current speed. 
	 * Thus do the same calculation with top speed (using get_effective_power_index(max_top_speed)).
	 *
	 * At first glance the effective power calculation might have been skipped at all, but the convoy top speed 
	 * can differ from the vehicle's top speed and thus steam engine might still be in the reduced power range.
	 */
	power = cnv.get_effective_power(max_top_speed);
}

void convoy_metrics_t::calc(karte_t &world, vector_tpl<const vehikel_besch_t *> &vehicles)
{
	reset();
	for(unsigned i = vehicles.get_count();  i-- > 0; ) {
		add_vehicle(*vehicles[i]);
	}
	//BG, 30.08.2009: power calculation was missing:
	for(unsigned i = vehicles.get_count();  i-- > 0; ) {
		power += vehicles[i]->get_effective_power_index(max_top_speed);
	}
	power *= world.get_einstellungen()->get_global_power_factor() / 64;
}

// Calculate the maximum speed in kmh the given power in kWh can move the given weight in t.
#define max_kmh(power, weight) (sqrt(((power)/(weight))-1) * 50)

uint32 convoy_metrics_t::get_speed(uint32 weight) 
{ 
	// This was correct for the old physics formulae, but not now. Use maximum theoretical
	// speed, not maximum actual speed until a new formula can be found.

	return min(max_top_speed, (uint32) max_kmh(power, weight)); 
	//return max_top_speed;
}

/* Calculate convoy's effective power according to given speed.
 *
 * The power of a steam engine depends on its speed.
 *
 * @author Bernd Gabriel, Sep, 17 2009: temporary fix. A convoy_metrics_t should become part of a convoi_t.
 */
float convoi_t::get_effective_power(uint32 speed) 
{ 
	uint32 power = 0;
	uint32 weight = 0;
	for(unsigned i = anz_vehikel; i-- > 0; )
	{
		vehikel_t &vehicle = *fahr[i];
		power += vehicle.get_besch()->get_effective_power_index(speed);
		weight += vehicle.get_gesamtgewicht();
	}
	sum_gesamtgewicht = weight;
	return power * get_welt()->get_einstellungen()->get_global_power_factor() / 64;
}

/* Set the new desired speed. 
 *
 * It will be reached after several calls to calc_acceleration().
 *
 * @param set_akt_speed in simutrans speed representation (see: kmh_to_speed()).
 * @author Bernd Gabriel, Sep, 17 2009: temporary fix. A convoy_metrics_t should become part of a convoi_t.
 */
void convoi_t::set_akt_speed_soll(sint32 set_akt_speed) 
{ 
	float power = get_effective_power(min_top_speed); //FIXME: This is an issue with the physics. Bernd Gabriel is rewriting this.
	akt_speed_soll = min(set_akt_speed, min(min_top_speed, kmh_to_speed((uint32) max_kmh(power, sum_gesamtgewicht)))); 
}
