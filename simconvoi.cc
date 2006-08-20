/**
 * simconvoi.cc
 *
 * convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#include <math.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "simcosts.h"
#include "simworld.h"
#include "simplay.h"
#include "simconvoi.h"
#include "simvehikel.h"
#include "simhalt.h"
#include "simdepot.h"
#include "simwin.h"
#include "simcolor.h"
#include "simmesg.h"
#include "blockmanager.h"
#include "simintr.h"
#include "simlinemgmt.h"
#include "simline.h"

#include "gui/karte.h"
#include "gui/convoi_info_t.h"
#include "gui/fahrplan_gui.h"
#include "gui/messagebox.h"
#include "boden/grund.h"
#include "besch/vehikel_besch.h"
#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "utils/tocstring.h"
#include "utils/simstring.h"


// zeige debugging info in infofenster wenn definiert
// #define DEBUG 1


/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 2000


/*
 * Debugging helper - translate state value to human readable name
 * @author Hj- Malthaner
 */
static const char * state_names[] =
{
  "INITIAL",
  "FAHRPLANEINGABE",
  "ROUTING_1",
  "ROUTING_2",
  "ROUTING_4",
  "ROUTING_5",
  "DRIVING",
  "LOADING",
  "WAITING_FOR_CLEARANCE"
};


/**
 * Calculates speed of slowest vehicle in the given array
 * @author Hj. Matthaner
 */
static int calc_min_top_speed(array_tpl <vehikel_t *> *fahr, int anz_vehikel)
{
  int min_top_speed = 9999999;
  for(int i=0; i<anz_vehikel; i++) {
    min_top_speed = MIN(min_top_speed, fahr->at(i)->gib_speed());
  }

  return min_top_speed;
}


void convoi_t::init_buttons()
{
    if(!in_depot()) {
	if(!convoi_info) {
	    convoi_info = new convoi_info_t(self);
	}
    } else {
	//  V.Meyer: destroy convoi info when entering the depot
	destroy_win(convoi_info);
	delete convoi_info;
	convoi_info = NULL;
    }
}


/**
 * Initialize all variables with default values.
 * Each constructor must call this method first!
 * @author Hj. Malthaner
 */
void convoi_t::init(karte_t *wl, spieler_t *sp)
{
  welt = wl;
  besitzer_p = sp;

  sum_gesamtgewicht = sum_gewicht = sum_gear_und_leistung = sum_leistung = 0;
  previous_delta_v = 0;
  min_top_speed = 9999999;

  fahr = new array_tpl<vehikel_t *> (max_vehicle);

  fpl = NULL;
  line = NULL;
  line_id = -1;

  for(int i=0; i<fahr->get_size(); i++) {
    fahr->at(i) = NULL;
  }

  convoi_info = NULL;


  anz_vehikel = 0;
  anz_ready = 0;
  ist_fahrend = true;
  wait_lock = 0;

  jahresgewinn = 0;

  alte_richtung = ribi_t::keine;
  next_wolke = welt->gib_zeit_ms() + 100;

  state = INITIAL;

  *name = 0;

  loading_level = 0;
  loading_limit = 0;

  akt_speed_soll = 0;            // Sollgeschwindigkeit
  akt_speed = 0;                 // momentane Geschwindigkeit
  sp_soll = 0;

  line_update_pending = -1;

  home_depot = koord3d(0,0,0);
}


convoi_t::convoi_t(karte_t *wl, loadsave_t *file) :
  tmp_pos(max_rail_vehicle), self(this)
{
  init(wl, 0);

  rdwr(file);

  // if a line is assigned, set line!
  // @author hsiegeln
  if (line_id > -1) {
    register_with_line(line_id);
  }

  init_buttons();
}


convoi_t::convoi_t(karte_t *wl, spieler_t *sp) :
  tmp_pos(max_rail_vehicle), self(this)
{
  init(wl, sp);

  tstrncpy(name, translator::translate("Unnamed"), 128);

  anhalten(0);

  welt->add_convoi( self );

  init_financial_history();
  init_buttons();
}


convoi_t::~convoi_t()
{
  // DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);

  destroy_win(convoi_info);

  welt->sync_remove( this );
  welt->rem_convoi( self );

  // @author hsiegeln - deregister from line
  unset_line();

  // Hajo: finally clean up

  self.detach();

  delete fahr;
  fahr = 0;

  delete convoi_info;
  convoi_info = 0;

  fpl = 0;
}


/**
 * Gibt die Position des Convois zurück.
 * @return Position des Convois
 * @author Hj. Malthaner
 */
koord3d
convoi_t::gib_pos() const
{
  if(anz_vehikel > 0 && fahr->get(0)) {
    return fahr->get(0)->gib_pos();
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
  tstrncpy(this->name, translator::translate(name), 128);
}


/**
 * wird vom ersten Fahrzeug des Convois aufgerufen, um Aenderungen
 * der Grundgeschwindigkeit zu melden. Berechnet (Brems-) Beschleunigung
 * @author Hj. Malthaner
 */
void
convoi_t::setze_akt_speed_soll(int as)
{
  akt_speed_soll = as;
}



/**
 * Vehicles of the convoi add their running cost by using this
 * method
 * @author Hj. Malthaner
 */
void convoi_t::add_running_cost(int cost)
{
  // Fahrtkosten
  jahresgewinn += cost;
  gib_besitzer()->buche(cost, COST_VEHICLE_RUN);

  // hsiegeln
  book(cost, CONVOI_OPERATIONS);
  book(cost, CONVOI_PROFIT);
}


/**
 * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
void
convoi_t::sync_prepare()
{
  if(wait_lock > 0) {
    // Hajo: got a saved game that had wait_lock set to 345891
    // I have no idea how that can happen, but we can work
    // around such problems here. -> wait at most 1 minute

    if(wait_lock > 60000) {

      dbg->warning("convoi_t::sync_prepre()",
		   "Convoi %d: wait lock out of bounds:"
		   "wait_lock = %d, setting to 60000",
		   self.get_id(), wait_lock);

      wait_lock = 60000;
    }

  } else {
    wait_lock = 0;
  }
}


/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool
convoi_t::sync_step(long delta_t)
{
  // DBG_MESSAGE("convoi_t::sync_step()", "%p, state %d", this, state);

  // moved check to here, as this will apply the same update
  // logic/constraints convois have for manual schedule manipulation
  if (line_update_pending > -1) {
    if ((state != ROUTING_4) && (state != ROUTING_5)) {
      check_pending_updates();
    }
  }

  wait_lock -= delta_t;


  if(wait_lock <= 0) {
    // step ++;

    switch(state) {
    case INITIAL:
      // jemand muß start aufrufen, damit der convoi von INITIAL
      // nach ROUTING_1 geht, das kann nicht automatisch gehen
      break;

    case FAHRPLANEINGABE:
      if((fpl != NULL) && (fpl->ist_abgeschlossen())) {

	setze_fahrplan(fpl);

	// V.Meyer: If we are at destination - complete loading task!
	state = gib_pos() == fpl->eintrag.get(fpl->aktuell).pos ? LOADING : ROUTING_1;
	calc_loading();

	// rebuild destination lists after schedule has been changed
	const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
	slist_iterator_tpl <halthandle_t> iter (list);

	while( iter.next() ) {
	  iter.get_current()->rebuild_destinations();
	  INT_CHECK("convoi_t 384");
	}
      }
      break;

    case ROUTING_1:

      wait_lock += WTT_LOADING;

      state = ROUTING_2;
      // printf("Convoi wechselt von ROUTING_1 nach ROUTING_2\n");
      break;

    case ROUTING_2:
      // Hajo: async calc new route, nothing to do here
      break;

    case ROUTING_4:
	fahr->at(0)->play_sound();
	state = ROUTING_5;
	// printf("Convoi wechselt von ROUTING_4 nach ROUTING_5\n");
	break;

    case ROUTING_5:
      // Hajo: this is an async task, see step()

      // hier müssen alle vorrücken
      // da wir im letzten zug-feld starten
      break;


	case DRIVING:
		// teste, ob wir neu ansetzen müssen ?
		if(anz_ready == 0) {

			// wir sind noch nicht am ziel
			// jetzt wird gefahren
			if(ist_fahrend) {
				// Prissi: more pleasant and a little more "physical" model *

				// first set speed limit
				int speed_limit=MIN(akt_speed_soll,min_top_speed);

				int sum_friction_weight = 0;
				sum_gesamtgewicht = 0;
				// calculate total friction
				for(int i=0; i < anz_vehikel; i++) {
					int total_vehicle_weight;

					total_vehicle_weight = fahr->at(i)->gib_gesamtgewicht();
					sum_friction_weight += fahr->at(i)->gib_frictionfactor()*total_vehicle_weight;
					sum_gesamtgewicht += total_vehicle_weight;
				}

				// try to simulate quadratic friction
				if(sum_gesamtgewicht != 0) {
				/*
				* The parameter consist of two parts (optimized for good looking):
				*  - every vehicle in a convoi has a constant friction of 32 per vehicle.
				*  - the dynamic friction is calculated that way, that v^2*weight*frictionfactor = 200 kW
				* => the more heavy and the more fast the less power for acceleration is available!
				* since delta_t can have any value, we have to scale the step size by this value.
				* however, there is a quadratic friction term => if delta_t is too large the calculation may get weird results
				* @author prissi
				*/
				/* with floats, one would write: akt_speed*ak_speed*iTotalFriction*100 / (12,8*12,8) + 32*anz_vehikel;
				* but for interger, we have to use the order below and calculate actualle 64*deccel, like the sum_gear_und_leistung */
				/* since akt_speed=10/128 km/h and we want 64*200kW=(100km/h)^2*100t, we must multiply by (128*2)/100 */
				/* But since the acceleration was too fast, we just deccelerate 4x more => >>6 instead >>8 */
				int deccel = ( ( (akt_speed*sum_friction_weight)>>6 )*akt_speed ) / 100 + (sum_gesamtgewicht*64);	// this order is needed to prevent overflows!
				// we normalize delta_t to 1/64th and check for speed limit */
				int delta_v = ( ( (akt_speed>speed_limit?0:sum_gear_und_leistung) - deccel) *delta_t)/sum_gesamtgewicht;
				// we need more accurate arithmetik, so we store the previous value
				delta_v += previous_delta_v;
				previous_delta_v = delta_v & 0xFFF;
				// and finally calculate new speed
				akt_speed = MAX(speed_limit>>4, akt_speed+(delta_v>>12) );
//DBG_MESSAGE("convoi_t::sync_step","accel %d, deccel %d, akt_speed %d, delta_t %d, delta_v %d",sum_gear_und_leistung,deccel,akt_speed,delta_t,delta_v );
			}
			else {
				// very old vehicle ...
				akt_speed += 16;
			}
			// obey speed maximum with additional const brake ...
			if(akt_speed > speed_limit) {
				akt_speed -= 24;
			}

			sp_soll += (akt_speed*delta_t) / 64;
			while(1024 < sp_soll && anz_ready == 0) {
				sp_soll -= 1024;

				// for(int i=anz_vehikel-1; i >= 0; i--) {
				for(int i=0; i < anz_vehikel && state != WAITING_FOR_CLEARANCE; i++) {
					fahr->at(i)->sync_step();
				}
			}
		}

		// smoke for the engines (only first can smoke )
#if 0
		// correct but slower ...
		if(welt->gib_zeit_ms() > next_wolke) {
			fahr->at(0)->rauche();
			next_wolke += 500;
			if(next_wolke < welt->gib_zeit_ms()+100) {
				next_wolke = welt->gib_zeit_ms()+100;
			}
		}
#else
		if(welt->gib_zeit_ms() > next_wolke) {
			next_wolke = welt->gib_zeit_ms()+500;
			fahr->at(0)->rauche();
		}
#endif
	} // end if(anz_ready==0)
	else {
		// Ziel erreicht
		alte_richtung = fahr->at(0)->gib_fahrtrichtung();
		state = LOADING;
		akt_speed = 0;

		// pruefen ob wir ein depot erreicht haben
		const grund_t *gr = welt->lookup(fahr->at(0)->gib_pos());
		depot_t * dp = gr->gib_depot();

		if(dp) {
			// ok, we are entering a depot
			char buf[128];
			sprintf(buf, translator::translate("!1_DEPOT_REACHED"), gib_name());
			message_t::get_instance()->add_message(buf,fahr->at(0)->gib_pos().gib_2d(),message_t::convoi,gib_besitzer()->kennfarbe,IMG_LEER);

			// Hajo: Fenster zu sonst Absturz bei Verkauf
			destroy_win(convoi_info);
			betrete_depot(dp);

			// Hajo: since 0.81.5exp it's safe to
			// remove the current sync object from
			// the sync list from inside sync_step()
			welt->sync_remove(this);

			state = INITIAL;
			return true;  // Hajo: convoi is still alive
		}
		else {
			// no depot reached, so book values for stop!
			halthandle_t halt = haltestelle_t::gib_halt(welt, fahr->at(0)->gib_pos());
			// we could have reached a non-haltestelle stop, so check before booking!
			if (halt.is_bound()) {
				halt->book(1, HALT_CONVOIS_ARRIVED);
			}
		}

		// Gewinn für transport einstreichen
		calc_gewinn();
	}

	break;

    case LOADING:
      // Hajo: loading is an async task, see laden()
      break;

    case WAITING_FOR_CLEARANCE:
      {
	int restart_speed;
	if(fahr->at(0)->ist_weg_frei(restart_speed)) {
	  state = DRIVING;
	}
      }
      break;
    case SELF_DESTRUCT:
	 for(int f=0; f<anz_vehikel; f++) {
      	besitzer_p->buche(fahr->at(f)->calc_restwert(), fahr->at(f)->gib_pos().gib_2d(), COST_NEW_VEHICLE);
      }
      destroy();
      break;
    default:
      dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
    }
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

  bool ok = route.calc_route(welt, start, ziel, fahr->at(0));

  for(int i=0; i<anz_vehikel; i++) {
    fahr->at(i)->neue_fahrt();
  }


  INT_CHECK("simconvoi 297");

  // printf("Convoi %p hat Route errechnet\n", this);


  if(!ok) {
    gib_besitzer()->bescheid_vehikel_problem(self,ziel);
  }

  return ok;
}


/**
 * Berechne route zum nächsten Halt
 * @author Hanjsörg Malthaner
 */
void convoi_t::drive_to_next_stop()
{
  fahrplan_t *fpl = gib_fahrplan();

  assert(fpl != NULL);

  if(fpl->aktuell < fpl->maxi) {
    fpl->aktuell ++;
  } else {
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
  // XXX ?

  state = ROUTING_1;
}


/**
 * Advance route by one step.
 * @return next position on route
 * @author Hanjsörg Malthaner
 */
koord3d convoi_t::advance_route(const int n) const
{
  koord3d result (-1, -1, -1);

  if(n <= route.gib_max_n()) {
    result = route.position_bei(n);
  } else {
    result = route.position_bei(route.gib_max_n());
  }

  return result;
}


/**
 * Asynchrne step methode des Convois
 * @author Hj. Malthaner
 */
void convoi_t::step()
{

  switch(state) {

  case LOADING:
    laden();
    break;

  case ROUTING_2:
    // Hajo: now calculate a new route
    drive_to(fahr->at(0)->gib_pos(),
	     fpl->eintrag.at(fpl->get_aktuell()).pos);

    if(route.gib_max_n() > 0) {
      // Hajo: ROUTING_3 is no more, go to ROUTING_4 directly
      state = ROUTING_4;
    }
    break;

  case ROUTING_5:
    vorfahren();
    break;

  default:
    // Hajo: do nothing
    break;
  }
}


void
convoi_t::neues_jahr()
{
    jahresgewinn = 0;
}


void
convoi_t::new_month()
{
  for (int j = 0; j<MAX_CONVOI_COST; j++) {
    for (int k = MAX_MONTHS-1; k>0; k--) {
      financial_history[k][j] = financial_history[k-1][j];
    }
    financial_history[0][j] = 0;
  }

  financial_history[0][CONVOI_VEHICLES] = anz_vehikel;
}


void
convoi_t::betrete_depot(depot_t *dep)
{
	// Hajo: remove vehicles from world data structure
	for(int i=0; i<anz_vehikel; i++) {
	  welt->lookup(fahr->at(i)->gib_pos())->obj_remove(fahr->at(i), fahr->at(i)->gib_besitzer());
	}

	// reset railblocks
	for(int i=0; i<anz_vehikel; i++) {
		blockmanager::gib_manager()->pruefe_blockstrecke(welt, tmp_pos.at(i));
	}

	dep->convoi_arrived(self, true);
	init_buttons();
}


void
convoi_t::start()
{
  if(state == INITIAL || state == ROUTING_1) {

		// set home depot to location of depot convoi is leaving
		set_home_depot(gib_pos());

    for(int i=0; i<anz_vehikel; i++) {
      grund_t * gr = welt->lookup(fahr->at(i)->gib_pos());

      if(!gr->obj_ist_da(fahr->at(i))) {
	gr->obj_add( fahr->at(i) );
      }
    }

    ist_fahrend = true;
    alte_richtung = ribi_t::keine;


    state = ROUTING_1;
    calc_loading();

    DBG_MESSAGE("convoi_t::start()",
		 "Convoi %s wechselt von INITIAL nach ROUTING_1",
		 name);



  } else {
    dbg->warning("convoi_t::start()",
		 "called with state=%s\n",
		 state_names[state]);
  }
}


/**
 * Bereit-Meldung entgegennehmen
 * @author Hj. Malthaner
 */
void convoi_t::ziel_erreicht(vehikel_t *)
{
  wait_lock += 8*50;
  anz_ready ++;
  // Wir sind am Ziel, sagt uns ein Fahrzeug.
}


/**
 * Wartet bis Fahrzeug 0 freie Fahrt meldet
 * @author Hj. Malthaner
 */
void convoi_t::warten_bis_weg_frei(int restart_speed)
{
  if(restart_speed != -1) {
    // langsam anfahren
    akt_speed = restart_speed;
  }

  state = WAITING_FOR_CLEARANCE;
}


void convoi_t::anhalten(int restart_speed)
{
	if(ist_fahrend == true) {
		ist_fahrend = false;
		if(restart_speed != -1) {
			// langsam anfahren
			akt_speed = restart_speed;
		}
	}
}


void convoi_t::weiterfahren()
{
  if(ist_fahrend == false) {
    // printf("convoi_t::weiterfahren: convoi %p fährt an.\n", this);

    ist_fahrend = true;
  }
}


bool
convoi_t::add_vehikel(vehikel_t *v, bool infront)
{
	// extend array if requested (only needed for trains)
	if(anz_vehikel == max_vehicle) {
		array_tpl <vehikel_t *> *f = new array_tpl<vehikel_t *> (max_rail_vehicle);
		for(int i=0; i<max_vehicle; i++) {
			f->at(i) = fahr->at(i);
		}
		for(int i=max_vehicle;  i<max_rail_vehicle; i++) {
			f->at(i) = NULL;
		}
		delete fahr;
		fahr = f;
	}
	// now append
	if(anz_vehikel < fahr->get_size()) {
		v->setze_convoi(this);

		if(infront) {
			for(int i = anz_vehikel; i > 0; i--) {
				fahr->at(i) = fahr->at(i - 1);
			}
			fahr->at(0) = v;
		} else {
			fahr->at(anz_vehikel) = v;
		}
		anz_vehikel ++;

		const vehikel_besch_t *info = v->gib_besch();
		sum_leistung += info->gib_leistung();
		sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
		sum_gewicht += info->gib_gewicht();
		min_top_speed = MIN(min_top_speed, v->gib_speed());
		sum_gesamtgewicht = sum_gewicht;
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	setze_erstes_letztes();

	return true;
}


vehikel_t *
convoi_t::remove_vehikel_bei(int i)
{
    vehikel_t *v = NULL;

    if(i>=0 && i<anz_vehikel) {

	v = fahr->at(i);

	if(v != NULL) {
	    for(int j=i; j<anz_vehikel-1; j++) {
		fahr->at(j) = fahr->at(j+1);
	    }

	    --anz_vehikel;
	    fahr->at(anz_vehikel) = NULL;

	    v->setze_convoi(NULL);

    	    const vehikel_besch_t *info = v->gib_besch();
	    sum_leistung -= info->gib_leistung();
	    sum_gear_und_leistung -= info->gib_leistung()*info->get_gear();
	    sum_gewicht -= info->gib_gewicht();
	}
    }
	sum_gesamtgewicht = sum_gewicht;

    // der convoi hat jetzt ein neues ende
    if(anz_vehikel > 0) {
	setze_erstes_letztes();
    }

    // Hajo: calculate new minimum top speed
    min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

    return v;
}

void
convoi_t::setze_erstes_letztes()
{
	// anz_vehikel muss korrekt init sein
	if(anz_vehikel>0) {
		fahr->at(0)->setze_erstes(true);
		for(int i=1; i<anz_vehikel; i++) {
			fahr->at(i)->setze_erstes(false);
			fahr->at(i-1)->setze_letztes(false);
		}
		fahr->at(anz_vehikel-1)->setze_letztes(true);
	}
	else {
		dbg->warning("convoi_t::setze_erstes_letzes()", "called with anz_vehikel==0!");
	}
}


bool
convoi_t::setze_fahrplan(fahrplan_t * f)
{
  DBG_DEBUG("convoi_t::setze_fahrplan()", "new=%p, old=%p", f, fpl);

    if(f == NULL) {
	return false;
    }

    if((fpl != NULL) && (f != fpl)) {
	delete fpl;
    }


    fpl = f;

    for(int i=0; i<anz_vehikel; i++) {
	fahr->at(i)->remove_stale_freight();
    }


    fahr->at(anz_vehikel) = NULL;

    state = ROUTING_1;

    return true;
}


fahrplan_t *
convoi_t::erzeuge_fahrplan()
{
    if(fpl == NULL) {
	if(fahr->at(0) != NULL) {
	    fpl = fahr->at(0)->erzeuge_neuen_fahrplan();

	    fpl->eingabe_abschliessen();
	}
    }

    return fpl;
};


void
convoi_t::go_alte_richtung()
{
  // wir wollen den zug neu aufstellen
  // dazu muss die lok evtl. einige felder zurück gesetzt werden
  // und es müssen einige felder in die route eingefügt werden


  koord3d pos ( fahr->at(0)->gib_pos() );
  int i;
  for(i=1; i<anz_vehikel; i++) {
    const koord3d k = fahr->at(i)->gib_pos();

    if(pos != k) {
      route.insert(k);
      pos = k;
    }
  }

  const koord3d k0 = route.position_bei(0);
  const koord3d k1 = route.position_bei(1);

  for(i=0; i<anz_vehikel; i++) {
    welt->lookup(k0)->obj_add(fahr->at(i));
    fahr->at(i)->setze_pos(k0);
    fahr->at(i)->starte_neue_route(k0, k1);
    // fahr->at(i)->setze_offsets(0,0);
  }
}


void
convoi_t::go_neue_richtung()
{
  const koord3d k0 = route.position_bei(0);
  const koord3d k1 = route.position_bei(1);

  for(int i=0; i<anz_vehikel; i++) {
    welt->lookup(k0)->obj_add(fahr->at(i));
    fahr->at(i)->setze_pos(k0);
    fahr->at(i)->starte_neue_route(k0, k1);
    // fahr->at(i)->setze_offsets(0,0);
  }
}


void
convoi_t::vorfahren()
{
	// Hajo: init speed settings
	sp_soll = 0;
	akt_speed = 8;
	setze_akt_speed_soll(fahr->at(0)->gib_speed());
	anz_ready = 0;

	int dummy1, dummy2;
	ribi_t::ribi neue_richtung =  fahr->at(0)->calc_richtung(route.position_bei(0).gib_2d(), route.position_bei(1).gib_2d(), dummy1, dummy2);

	INT_CHECK("simconvoi 651");

	int i;
	for(i=0; i<anz_vehikel; i++) {
		tmp_pos.at(i) = fahr->at(i)->gib_pos();
		const bool ok = welt->lookup(tmp_pos.get(i))->obj_remove(fahr->at(i), besitzer_p)  != NULL;

		if(!ok) {
			dbg->error("convoi_t::vorfahren()", "Vehicle %d couldn't be removed.", i);
		}
	}


	if(alte_richtung == neue_richtung) {
		go_alte_richtung();
	}
	else {
		go_neue_richtung();
	}

	for(i=0; i<anz_vehikel; i++) {
		switch(neue_richtung) {
			case ribi_t::west:
				fahr->at(i)->setze_offsets(10,5);
				break;
			case ribi_t::ost :
				fahr->at(i)->setze_offsets(-2,-1);
				break;
			case ribi_t::nord:
				fahr->at(i)->setze_offsets(-10,5);
				break;
			case ribi_t::sued:
				fahr->at(i)->setze_offsets(2,-1);
				break;
			default:
				fahr->at(i)->setze_offsets(0,0);
				break;
		}
	}

	// reset railblocks
	for(i=0; i<anz_vehikel; i++) {
		blockmanager::gib_manager()->pruefe_blockstrecke(welt, tmp_pos.at(i));
	}

	// einige richtungen brauchen extra anfahrsteps
	int extra = 0;
	if(alte_richtung == neue_richtung) {
		if((anz_vehikel & 1) == 1) {
			extra = 8;
		}
	}

	fahr->at(0)->setze_erstes( false );
	for(i=0; i<anz_vehikel; i++) {

		// raucher beim vorfahren abschalten
		fahr->at(i)->darf_rauchen(false);

		const int steps = (anz_vehikel-i-1)*8 + extra;
		for(int j=0; j<steps; j++) {
			fahr->at(i)->sync_step();
		}

		fahr->at(i)->darf_rauchen(true);
	}
	fahr->at(0)->setze_erstes( true );

	INT_CHECK("simconvoi 711");

	state = DRIVING;
	calc_loading();
}


void
convoi_t::rdwr(loadsave_t *file)
{
    int besitzer_n = welt->sp2num(besitzer_p);
    int i;

    if(file->is_saving()) {
        file->wr_obj_id("Convoi");
    }

    // for new line management we need to load/save the assigned line id
    // @author hsiegeln
    file->rdwr_int(line_id, " ");

    file->rdwr_int(anz_vehikel, " ");
    file->rdwr_int(anz_ready, " ");
    file->rdwr_long(wait_lock, " ");
    file->rdwr_bool(ist_fahrend, " ");
    file->rdwr_int(besitzer_n, "\n");
    file->rdwr_int(akt_speed, " ");
    file->rdwr_int(akt_speed_soll, " ");
    file->rdwr_int(sp_soll, " ");
    file->rdwr_enum(state, " ");
    file->rdwr_enum(alte_richtung, " ");
    file->rdwr_long(jahresgewinn, "\n");

    route.rdwr(file);


    if(file->is_loading()) {

		// extend array if requested (only needed for trains)
		if(anz_vehikel > max_vehicle) {
			fahr = new array_tpl<vehikel_t *> (max_rail_vehicle);
			for(int i=0; i<max_rail_vehicle; i++) {
				fahr->at(i) =NULL;
			}
		}

        besitzer_p = welt->gib_spieler( besitzer_n );

	// Hajo: sanity check for values ... plus correction

	if(sp_soll < 0) {
	  sp_soll = 0;
	}
    }

    file->rdwr_str(name, sizeof(name));

    for(i=0; i<anz_vehikel; i++) {
        if(file->is_saving()) {
	    fahr->at(i)->rdwr(file, true);
    	    tmp_pos.at(i).rdwr(file);
        }
        else {
            ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
	    vehikel_t *v = 0;

	    switch(typ) {
	    case ding_t::automobil: v = new automobil_t(welt, file);  break;
	    case ding_t::waggon:    v = new waggon_t(welt, file);     break;
	    case ding_t::schiff:    v = new schiff_t(welt, file);     break;
	    default:
	        dbg->fatal("convoi_t::convoi_t()","Can't load vehicle type %d", typ);
	    }
	    tmp_pos.at(i).rdwr(file);

	    assert(v != 0);
	    fahr->at(i)  = v;
	    v->setze_convoi( this );

	    const vehikel_besch_t *info = v->gib_besch();

	    // Hajo: if we load a game from a file which was saved from a
	    // game with a different vehicle.tab, there might be no vehicle
	    // info
	    if(info) {
		sum_leistung += info->gib_leistung();
		sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
		sum_gewicht += info->gib_gewicht();
	    }


	    if(state != INITIAL) {
		grund_t *gr;

		gr = welt->lookup(v->gib_pos());
		if(!gr) {
		  gr = welt->lookup(v->gib_pos().gib_2d())->gib_kartenboden();
		  dbg->warning("convoi_t::rdwr()",
			       "invalid position %s for vehicle %s in state %d (setting to ground %s)",
			       k3_to_cstr(v->gib_pos()).chars(),
			       v->gib_name(),
			       state,
			       k3_to_cstr(gr->gib_pos()).chars());
		}
		gr->obj_add(v);
	    }
        }
    }
	sum_gesamtgewicht = sum_gewicht;

    bool has_fpl = fpl != NULL;

    file->rdwr_bool(has_fpl, "");

    if(has_fpl) {
      if(file->is_loading() && fahr->at(0)) {
	fpl = fahr->at(0)->erzeuge_neuen_fahrplan();
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
        next_wolke = welt->gib_zeit_ms() + 100;

	calc_loading();
    }


    // Hajo: calculate new minimum top speed
    min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

    // Hajo: since sp_ist became obsolete, sp_soll is
    //       used modulo 1024
    sp_soll &= 1023;

    // load statistics
    for (int j = 0; j<MAX_LINE_COST; j++) {
      for (int k = MAX_MONTHS-1; k>=0; k--) {
	file->rdwr_longlong(financial_history[k][j], " ");
      }
    }

    // save/restore pending line updates
		if (file->get_version() > 84008) {
			file->rdwr_int(line_update_pending, "\n");
		}

		if (file->get_version() > 84009) {
			home_depot.rdwr(file);
		}

}


void
convoi_t::zeige_info()
{
  if(!in_depot()) {

    if(umgebung_t::verbose_debug) {
      dump();
      if(anz_vehikel > 0 && fahr->at(0)) {
	fahr->at(0)->dump();
      }
    }

    init_buttons();
    destroy_win(convoi_info);
    create_win(-1, -1, convoi_info, w_info);
  }
}


void convoi_t::info(cbuffer_t & buf) const
{
  if(fahr->at(0) != NULL) {
    char tmp[128];

    sprintf(tmp,"\n %d/%dkm/h (%1.2f$/km)\n",
		   speed_to_kmh(min_top_speed),
		   speed_to_kmh(fahr->at(0)->gib_speed()),
		   get_running_cost()/100.0
		   );
    buf.append(tmp);

    buf.append(" ");
    buf.append(translator::translate("Leistung"));
    buf.append(": ");
    buf.append(sum_leistung);
    buf.append("KW\n ");
    buf.append(translator::translate("Gewicht"));
    buf.append(": ");
	/* changed to stored values (may be wrong afterloading until frist stop is reached) */
    buf.append(sum_gewicht);
    buf.append(" (");
    buf.append(sum_gesamtgewicht-sum_gewicht);
    buf.append(") t\n ");
    buf.append(translator::translate("Gewinn"));
    buf.append(": ");
    buf.append(jahresgewinn/100);
    buf.append("$\n");
  }
}


void convoi_t::get_freight_info(cbuffer_t & buf)
{
  int j = 1;

  for(int i=0; i<anz_vehikel; i++) {
    if(fahr->at(i)->gib_fracht_max() > 0) {

      buf.append(" ");
      buf.append(j++);
      buf.append(".) ");
      buf.append(fahr->at(i)->gib_fracht_menge());
      buf.append("/");
      buf.append(fahr->at(i)->gib_fracht_max());
      buf.append(translator::translate(fahr->at(i)->gib_fracht_mass()));
      buf.append(" ");
      buf.append(fahr->at(i)->gib_fracht_typ()->gib_catg() == 0
		 ?
		 translator::translate(fahr->at(i)->gib_fracht_typ()->gib_name())
		 :
		 translator::translate(fahr->at(i)->gib_fracht_typ()->gib_catg_name()));
      buf.append("\n");

      fahr->at(i)->gib_fracht_info(buf);
    }
  }

  buf.append("\n");
}


void
convoi_t::open_schedule_window()
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

	// darf der spieler diesen convoi umplanen ?
	if(gib_besitzer() != NULL &&
		gib_besitzer() != welt->gib_spieler(0)) {
		return;
	}

	// manipulation of schedule not allowd while:
	// -convoi is in routing state 4 or 5
	// -a line update is pending
	if(state==FAHRPLANEINGABE  ||  state == ROUTING_4 || state == ROUTING_5 || line_update_pending > -1) {
		create_win(-1, -1, 120, new nachrichtenfenster_t(welt, translator::translate("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!")), w_autodelete);
		return;
	}

	calc_gewinn();
	anhalten(8);
	state = FAHRPLANEINGABE;
	alte_richtung = fahr->at(0)->gib_fahrtrichtung();

	// Fahrplandialog oeffnen
	fahrplan_gui_t *fpl_gui = new fahrplan_gui_t(welt, self, fahr->at(0)->gib_besitzer());
	fpl_gui->zeige_info();

	weiterfahren();
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die für die Nachfolger von
 * vor gelten - daher muß vor != NULL sein..
 */
bool
convoi_t::pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
    int	i;
    const vehikel_besch_t *soll;

    if(!vor->gib_nachfolger_count()) {
	// Alle Nachfolger erlaubt
	return true;
    }
    for(i=0; i < vor->gib_nachfolger_count(); i++) {
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
    int	i;

    if(!hinter->gib_vorgaenger_count()) {
	// Alle Vorgänger erlaubt
	return true;
    }
    for(i=0; i < hinter->gib_vorgaenger_count(); i++) {
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
    bool ok = !anz_vehikel || pruefe_vorgaenger(NULL, fahr->at(0)->gib_besch());
    int i;

    for(i = 1; ok && i < anz_vehikel; i++) {
	ok =
	    pruefe_nachfolger(fahr->at(i - 1)->gib_besch(), fahr->at(i)->gib_besch()) &&
	    pruefe_vorgaenger(fahr->at(i - 1)->gib_besch(), fahr->at(i)->gib_besch());
    }
    if(ok)
	ok = pruefe_nachfolger(fahr->at(i - 1)->gib_besch(), NULL);

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

	const koord k = fpl->eintrag.get(fpl->aktuell).pos.gib_2d();

	halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(fpl->aktuell).pos);
	// eigene haltestelle ?
	if(halt.is_bound()  &&  (halt->gib_besitzer()==gib_besitzer()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
		// default -> nicht warten
		hat_gehalten(k, halt);
	}

	INT_CHECK("simconvoi 1077");

	// Nun kann ein/ausgeladen werden
	if(get_loading_level() >= get_loading_limit())  {

		// add available capacity after loading(!) to statistics
		for (int i = 0; i<anz_vehikel; i++) {
			book(gib_vehikel(i)->gib_fracht_max()-gib_vehikel(i)->gib_fracht_menge(), CONVOI_CAPACITY);
		}
		wait_lock += WTT_LOADING;

		// Advance schedule
		drive_to_next_stop();
		state = ROUTING_2;
	}
	else {
		// Hajo: wait a few frames ... 250ms looks ok to me
		wait_lock = 250;
	}
}


/**
 * calculate income for last hop
 * @author Hj. Malthaner
 */
void convoi_t::calc_gewinn()
{
  int gewinn = 0;

  for(int i=0; i<anz_vehikel; i++) {
    vehikel_t *v = fahr->at(i);
    gewinn += v->calc_gewinn(route.position_bei(0),
//			     route.position_bei(MAX(route.gib_max_n(), 0))
				fahr->at(0)->gib_pos()
			     );

  }

  besitzer_p->buche(gewinn, fahr->at(0)->gib_pos().gib_2d(), COST_INCOME);
  jahresgewinn += gewinn;

	// hsiegeln
	book(gewinn, CONVOI_PROFIT);
	book(gewinn, CONVOI_REVENUE);
}


/**
 * convoi an haltestelle anhalten
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(koord k, halthandle_t halt)
{
	// haltestellenquote neu berechnen
	const int quote = anz_vehikel == 1 ? 32 : 16;
	int i;

	for(i=0; i<anz_vehikel; i++) {
		// recalculate connections ...
		halt->hat_gehalten(quote, fahr->at(i)->gib_fracht_typ(), fpl);
	}

	// entladen und beladen
	for(i=0; i<anz_vehikel; i++) {

		// Nur diejenigen Fahrzeuge be-/entladen, die im Bahnhof sind
		vehikel_t *v = fahr->at(i);
		const halthandle_t &halt = haltestelle_t::gib_halt(welt, v->gib_pos());

		if(halt.is_bound()) {
			// Hajo: die waren wissen wohin sie wollen
			// zuerst alle die hier aus/umsteigen wollen ausladen
			v->entladen(k, halt);

			// Hajo: danach neue waren einladen
			v->beladen(k, halt);

			// Jeder zusätzliche Waggon braucht etwas Zeit
			wait_lock += (WTT_LOADING/4);
		}
	}

	calc_loading();
}


int convoi_t::calc_restwert() const
{
    int result = 0;

    for(int i=0; i<anz_vehikel; i++) {
	result += fahr->at(i)->calc_restwert();
    }
    return result;
}


/**
 * Calclulate loading_level and loading_limit. This depends on current
 * state (loading or not).
 * @author Volker Meyer
 * @date  20.06.2003
 */
void convoi_t::calc_loading()
{
	int i;

	loading_limit = 0;
	if(state == LOADING) {

		halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(fpl->aktuell).pos );

		// eigene haltestelle ?
		if(halt.is_bound()  &&   !(halt->gib_besitzer()==gib_besitzer()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
			// eine oeffentliche haltestelle ?
			halt = halthandle_t();
		}

		// default -> nicht warten
		if(halt.is_bound()) {
			loading_limit = fpl->eintrag.get(fpl->aktuell).ladegrad;
		}
	}

	int fracht_max = 0;
	int fracht_menge = 0;
	for(i=0; i<anz_vehikel; i++) {
		// Nur diejenigen Fahrzeuge be-/entladen, die im Bahnhof sind
		vehikel_t *v = fahr->at(i);
		const halthandle_t &halt = haltestelle_t::gib_halt(welt, v->gib_pos());

		if(state != LOADING || halt.is_bound()) {
			fracht_max += v->gib_fracht_max();
			fracht_menge += v->gib_fracht_menge();
		}
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
}


/**
 * Schedule convoid for self destruction. Will be executed
 * upon next sync step
 * @author Hj. Malthaner
 */
void convoi_t::self_destruct()
{
  state = SELF_DESTRUCT;
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
  welt->sync_remove(this);
  welt->rem_convoi(self);

#ifdef _MSC_VER
  koord3d * pos = (koord3d *)alloca(anz_vehikel * sizeof(koord3d));
#else
  koord3d pos [anz_vehikel];
#endif
  int i;

  for(i=0; i<anz_vehikel; i++) {
    pos[i] = fahr->at(i)->gib_pos();

    // remove vehicle's marker from the reliefmap
    reliefkarte_t::gib_karte()->recalc_relief_farbe(fahr->at(i)->gib_pos().gib_2d());

    delete fahr->at(i);
  }

  // reset railblocks
  for(i=0; i<anz_vehikel; i++) {
    blockmanager::gib_manager()->pruefe_blockstrecke(welt, pos[i]);
  }


	// rebuild destination lists after convoi has has been changed removed
	const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
	slist_iterator_tpl <halthandle_t> iter (list);

	while( iter.next() ) {
	  iter.get_current()->rebuild_destinations();
	  INT_CHECK("convoi_t 384");
	}

  delete this;
}


/**
 * Debug info nach stderr
 * @author Hj. Malthaner
 * @date 04-Sep-03
 */
void convoi_t::dump() const
{
    fprintf(stderr, "anz_vehikel = %d\n", anz_vehikel);
    fprintf(stderr, "anz_ready = %d\n", anz_ready);
    fprintf(stderr, "wait_lock = %ld\n", wait_lock);
    fprintf(stderr, "ist_fahrend = %d\n", ist_fahrend);
    fprintf(stderr, "besitzer_n = %d\n", welt->sp2num(besitzer_p));
    fprintf(stderr, "akt_speed = %d\n", akt_speed);
    fprintf(stderr, "akt_speed_soll = %d\n", akt_speed_soll);
    fprintf(stderr, "sp_soll = %d\n", sp_soll);
    fprintf(stderr, "state = %d\n", state);
    fprintf(stderr, "statename = %s\n", state_names[state]);
    fprintf(stderr, "alte_richtung = %d\n", alte_richtung);
    fprintf(stderr, "jahresgewinn = %ld\n", jahresgewinn);

    fprintf(stderr, "name = '%s'\n", name);
    fprintf(stderr, "line_id = '%d'\n", line_id);
    fprintf(stderr, "fpl = '%p'\n", fpl);
}


/**
 * Checks if this convoi has a driveable route
 * @author Hanjsörg Malthaner
 */
bool convoi_t::hat_keine_route() const
{
  return route.gib_max_n() < 0;
}


/**
* get line
* @author hsiegeln
*/
simline_t * convoi_t::get_line() const
{
  return line;
}

/**
* set line
* since convoys must operate on a copy of the route's fahrplan, we apply a fresh copy
* @author hsiegeln
*/
void convoi_t::set_line(simline_t * line)
{
	// to remove a convoi from a line, call unset_line(); passing a NULL is not allowed!
	if (line == NULL)
	{
		return;
	}

	if (this->line != NULL)
	{
		unset_line();
	}
	this->line = line;
	this->line_id = line->get_id();
	fahrplan_t * new_fpl = line->get_fahrplan()->copy();
	setze_fahrplan(new_fpl);
	line->add_convoy(this);
}

/**
* register_with_line
* sets the convoi's line by using the line_id, rather than the line object
* CAUTION: THIS CALL WILL NOT SET A NEW FAHRPLAN!!! IT WILL JUST REGISTER THE CONVOI WITH THE LINE
* @author hsiegeln
*/
void convoi_t::register_with_line(int id)
{
  DBG_DEBUG("convoi_t::register_with_line()",
	       "%s registers for %d", name, id);

	simline_t * line = welt->simlinemgmt->get_line_by_id(id);
	if (line != NULL)
	{
		this->line = line;
		this->line_id = line->get_id();
		line->add_convoy(this);
	}
}

/**
* unset line
* removes convoy from route without destroying its fahrplan
* @author hsiegeln
*/
void convoi_t::unset_line()
{
	if (line != NULL)
	{
		this->line->remove_convoy(this);
		this->line = NULL;
		this->line_id = -1;
	}
}

void
convoi_t::prepare_for_new_schedule(fahrplan_t *f)
{
	alte_richtung = fahr->at(0)->gib_fahrtrichtung();
	anhalten(8);
	state = FAHRPLANEINGABE;

	setze_fahrplan(f);
	// Hajo: setze_fahrplan sets state to ROUTING_1
	// need to undo that
	state = FAHRPLANEINGABE;

	this->fpl->eingabe_beginnen();
	weiterfahren();
}

void
convoi_t::book(sint64 amount, int cost_type)
{
	if (cost_type > MAX_CONVOI_COST)
	{
		// THIS SHOULD NEVER HAPPEN!
		// CHECK CODE
		DBG_MESSAGE("convoi_t::book()", "function was called with cost_type: %i, which is not valid (MAX_CONVOI_COST=%i)", cost_type, MAX_CONVOI_COST);
		return;
	}

	financial_history[0][cost_type] += amount;
	if (has_line())
	{
		line->book(amount, cost_type);
	}

	if (cost_type == CONVOI_TRANSPORTED_GOODS) {
		besitzer_p->buche(amount, COST_TRANSPORTED_GOODS);
	}
}

void
convoi_t::init_financial_history()
{
	for (int j = 0; j<MAX_CONVOI_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
			financial_history[k][j] = 0;
		}
	}
    financial_history[0][CONVOI_VEHICLES] = anz_vehikel;
}

int
convoi_t::get_running_cost() const
{
		int running_cost = 0;
	for (int i = 0; i<gib_vehikel_anzahl(); i++)
	{
		running_cost += fahr->at(i)->gib_betriebskosten();
	}
	return running_cost;
}

void
convoi_t::check_pending_updates()
{
	if (line_update_pending > -1) {
			simline_t * line = welt->simlinemgmt->get_line_by_id(line_update_pending);
			// the line could have been deleted in the meantime
			// if line was deleted ignore line update; convoi will continue with existing schedule
			if (line != NULL) {
				int aktuell = fpl->get_aktuell(); // save current position of schedule
	  		fpl = new fahrplan_t(line->get_fahrplan());
	  		fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
	  		state = FAHRPLANEINGABE;
	  	}
			line_update_pending = -1;
  	}
}
