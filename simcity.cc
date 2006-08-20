/*
 * simcity.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simcity.cc
 *
 * Stadbau- und verwaltung
 *
 * Hj. Malthaner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boden/wege/strasse.h"
#include "boden/grund.h"
#include "simworld.h"
#include "simplay.h"
#include "simimg.h"
#include "simverkehr.h"
#include "simtools.h"
#include "simhalt.h"
#include "simfab.h"
#include "simcity.h"
#include "simmesg.h"
#include "simtime.h"
#include "simcolor.h"
#include "gui/karte.h"
#include "besch/haus_besch.h"
#include "tpl/slist_mit_gewichten_tpl.h"

#include "simintr.h"
#include "simdebug.h"

#include "dings/gebaeude.h"
#include "dings/leitung2.h"

#include "dataobj/translator.h"
#include "dataobj/einstellungen.h"
#include "dataobj/loadsave.h"
#include "dataobj/tabfile.h"
#include "dataobj/umgebung.h"

#include "sucher/bauplatz_sucher.h"
#include "bauer/warenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/fabrikbauer.h"

#include "utils/cstring_t.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"


class spieler_t;

/**
 * Number of road building rules
 * @author Hj. Malthaner
 */
static int num_road_rules = 0;


/**
 * Number of house building rules
 * @author Hj. Malthaner
 */
static int num_house_rules = 0;


/**
 * Rule data structure
 * @author Hj. Malthaner
 */
struct rule_t {
  int  chance;
  char rule[12];
};


/**
 * Road building rules
 * @author Hj. Malthaner
 */
static struct rule_t * road_rules = 0;


/**
 * House building rules
 * @author Hj. Malthaner
 */
static struct rule_t * house_rules = 0;



/**
 * try to built cities at least this distance apart
 * @author prissi
 */
static int minimum_city_distance = 16;


/**
 * add a new consumer every % people increase
 * @author prissi
 */
static int industry_increase_every[8];


//------------ haltestellennamen -----------------------

static const int anz_zentrum = 5;
static const char * zentrum_namen [anz_zentrum] =
{
//     "%s %s", "%s Haupt %s", "%s Zentral %s", "%s %s Mitte", "%s Transfer %s"

    "1center", "2center", "3center", "4center", "5center"
};

static const int anz_nord = 1;
static const char * nord_namen [anz_nord] =
{
//     "%s Nord %s"
    "1nord"
};

static const int anz_nordost = 1;
static const char * nordost_namen [anz_nordost] =
{
//     "%s Nordost %s"
    "1nordost"
};

static const int anz_ost = 1;
static const char * ost_namen [anz_ost] =
{
//     "%s Ost %s"
     "1ost"
};

static const int anz_suedost = 1;
static const char * suedost_namen [anz_suedost] =
{
//     "%s Südost %s"
     "1suedost"
};

static const int anz_sued = 1;
static const char * sued_namen [anz_sued] =
{
//     "%s Süd %s"
    "1sued"
};

static const int anz_suedwest = 1;
static const char * suedwest_namen [anz_suedwest] =
{
//     "%s Südwest %s"
    "1suedwest"
};

static const int anz_west = 1;
static const char * west_namen [anz_west] =
{
//     "%s West %s"
    "1west"
};

static const int anz_nordwest = 1;
static const char * nordwest_namen [anz_nordwest] =
{
//     "%s Nordwest %s"
    "1nordwest"
};

static const int anz_aussen = 4;
static const char * aussen_namen [anz_aussen] =
{
//     "%s Zweig %s", "%s Neben %s", "%s Land %s", "%s Außen %s"
    "1extern", "2extern", "3extern", "4extern",
};


/**
 * denkmal_platz_sucher_t:
 *
 * Sucht einen freien Bauplatz
 * Im Gegensatz zum bauplatz_sucher_t werden Strassen auf den Rändern
 * toleriert.
 *
 * 22-Dec-02: Hajo: added safety checks for gr != 0 and plan != 0
 *
 * @author V. Meyer
 */
class denkmal_platz_sucher_t : public platzsucher_t {
    spieler_t *besitzer;
public:
    denkmal_platz_sucher_t(karte_t *welt, spieler_t *sp) : platzsucher_t(welt), besitzer(sp) {}

    virtual bool ist_feld_ok(koord pos, koord d) const {
        const planquadrat_t *plan = welt->lookup(pos + d);

  // Hajo: can't build here
  if(plan == 0) return false;

  const grund_t *gr = plan->gib_kartenboden();
  if(ist_randfeld(d)) {
      return
          gr != 0 &&
    gr->gib_grund_hang() == hang_t::flach &&  // Flach
    gr->gib_typ() == grund_t::boden &&    // Boden -> keine GEbäude
    !gr->gib_weg(weg_t::schiene) &&     // Höchstens Strassen
    gr->kann_alle_obj_entfernen(NULL) == NULL;  // Irgendwas verbaut den Platz?
  }
  else {
      return
          gr != 0 &&
    gr->gib_grund_hang() == hang_t::flach &&
    gr->gib_typ() == grund_t::boden &&
    gr->ist_natur() &&        // Keine Wege hier
    gr->kann_alle_obj_entfernen(NULL) == NULL;  // Irgendwas verbaut den Platz?
  }
    }
};


/**
 * rathausplatz_sucher_t:
 *
 * 22-Dec-02: Hajo: added safety checks for gr != 0 and plan != 0
 *
 * @author V. Meyer
 */
class rathausplatz_sucher_t : public platzsucher_t {
    spieler_t *besitzer;
public:
    rathausplatz_sucher_t(karte_t *welt, spieler_t *sp) : platzsucher_t(welt), besitzer(sp) {}

    virtual bool ist_feld_ok(koord pos, koord d) const {
        const planquadrat_t *plan = welt->lookup(pos + d);

  // Hajo: can't build here
  if(plan == 0) return false;

  const grund_t *gr = plan->gib_kartenboden();
  if(d.y == h - 1) {
      // Hier soll eine Strasse hin
      return
          gr != 0 &&
    gr->gib_grund_hang() == hang_t::flach &&
    gr->gib_typ() == grund_t::boden &&
    !gr->gib_weg(weg_t::schiene) &&
    !gr->gib_halt().is_bound() &&
    gr->kann_alle_obj_entfernen(NULL) == NULL;
  } else {
      // Hier soll das Haus hin - wir ersetzen auch andere Gebäude, aber keine Wege!
      return
          gr != 0 &&
    gr->gib_grund_hang() == hang_t::flach &&
    (gr->gib_typ() == grund_t::boden && gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
    gr->kann_alle_obj_entfernen(NULL) == NULL;
  }
    }
};


/**
 * Zeitintervall für step
 * @author Hj. Malthaner
 */
const int stadt_t::step_interval = 7000;


void
stadt_t::init_pax_ziele()
{
	const int gr = welt->gib_groesse();

	for(int j=0; j<96; j++) {
		for(int i=0; i<96; i++) {
			const koord pos (i*gr/96, j*gr/96);
			pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i ,j) = reliefkarte_t::calc_relief_farbe(welt, pos);
			//      pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i ,j) = 0;
		}
	}
}

stadt_t::stadt_t(karte_t *wl, spieler_t *sp, koord pos,int citizens) :
    welt(wl),
    pax_ziele_alt(96, 96),
    pax_ziele_neu(96, 96)

{
    assert(wl->ist_in_kartengrenzen(pos));

    step_count = 0;
    next_step = welt->gib_zeit_ms()+step_interval+simrand(step_interval);

    pax_erzeugt = 0;
    pax_transport = 0;

    besitzer_p = sp;

    this->pos = pos;

    bev = 0;
    arb = 0;
    won = 0;

    li = re = pos.x;
    ob = un = pos.y;

    zentrum_namen_cnt = 0;
    aussen_namen_cnt = 0;

DBG_MESSAGE("stadt_t::stadt_t()","Welt %p",welt);fflush(NULL);
    /* get a unique cityname */
    /* 9.1.2005, prissi */
    const vector_tpl<stadt_t *> *staedte=welt->gib_staedte();
    const int anz_staedte = staedte->get_count()-1;
    const int name_list_count = translator::get_count_city_name();
    bool not_unique=true;

DBG_MESSAGE("stadt_t::stadt_t()","number of towns %i",anz_staedte);fflush(NULL);

    // start at random position
    int start_cont = simrand(name_list_count);
   char list_name[64];

    translator::get_city_name( list_name, start_cont );
    {
    	    for(  int i=0;  i<name_list_count  &&  not_unique;  i++  ) {
    	    	 // get a name
            translator::get_city_name( list_name, start_cont+i );
          	not_unique = false;
            // check if still unused
            for(  int j=0;  j<anz_staedte;  j++  ) {
                // noch keine stadt mit diesem namen?
                if(  staedte->get(j)!=this   &&  staedte->get(j)!=NULL  ) {
                	not_unique |= (strcmp( list_name, staedte->get(j)->gib_name() )==0);
                }
            }
DBG_MESSAGE("stadt_t::stadt_t()", "'%s' is%s unique", list_name, not_unique?" not":"");
        }
    }
    strcpy(name, list_name);
DBG_MESSAGE("stadt_t::stadt_t()","founding new city named '%s'",name);

    // 1. Rathaus bei 0 Leuten bauen
    check_bau_rathaus(true);

    wachstum = 0;

    // this way, new cities are properly recognized
    pax_erzeugt = 0;
    pax_transport = citizens/8;
    step_bau();
    pax_erzeugt = 0;
    pax_transport = 0;

    /**
     * initialize history array
     * @author prissi
     */
	for (int year=0; year<MAX_CITY_HISTORY_YEARS; year++) {
		for (int hist_type=0; hist_type<MAX_CITY_HISTORY; hist_type++) {
			city_history_year[year][hist_type] = 0;
		}
	}
	for (int month=0; month<MAX_CITY_HISTORY_YEARS; month++) {
		for (int hist_type=0; hist_type<MAX_CITY_HISTORY; hist_type++) {
			city_history_month[month][hist_type] = 0;
		}
	}
	last_year_bev = city_history_year[0][HIST_CITICENS] = bev;
	last_month_bev = city_history_month[0][HIST_CITICENS] = bev;
	this_year_transported = 0;
	this_year_pax = 0;
}


stadt_t::stadt_t(karte_t *wl, loadsave_t *file) :
    welt(wl),
    pax_ziele_alt(96, 96),
    pax_ziele_neu(96, 96)
{
    step_count = 0;
    next_step = welt->gib_zeit_ms()+step_interval+simrand(step_interval);

    pax_erzeugt = 0;
    pax_transport = 0;

    wachstum = 0;

    rdwr(file);

    verbinde_fabriken();
}


void stadt_t::rdwr(loadsave_t *file)
{
	int besitzer_n;

	if(file->is_saving()) {
		besitzer_n = welt->sp2num(besitzer_p);
	}
	file->rdwr_str(name, 31);
	pos.rdwr( file );
	file->rdwr_delim("Plo: ");
	file->rdwr_int(li, " ");
	file->rdwr_int(ob, "\n");
	file->rdwr_delim("Pru: ");
	file->rdwr_int(re, " ");
	file->rdwr_int(un, "\n");
	file->rdwr_delim("Bes: ");
	file->rdwr_int(besitzer_n, "\n");
	file->rdwr_int(bev, " ");
	file->rdwr_int(arb, " ");
	file->rdwr_int(won, "\n");
	file->rdwr_int(zentrum_namen_cnt, " ");
	file->rdwr_int(aussen_namen_cnt, "\n");

	if(file->is_loading()) {
		besitzer_p = welt->gib_spieler(besitzer_n);
	}

	// we probably need to save the city history
	if (file->get_version() < 86000) {
DBG_DEBUG("stadt_t::rdwr()","is old version: No history!");
		/**
		* initialize history array
		* @author prissi
		*/
		for (int year=0; year<MAX_CITY_HISTORY_YEARS; year++) {
			for (int hist_type=0; hist_type<MAX_CITY_HISTORY; hist_type++) {
				city_history_year[year][hist_type] = 0;
			}
		}
		for (int month=0; month<MAX_CITY_HISTORY_MONTHS; month++) {
			for (int hist_type=0; hist_type<MAX_CITY_HISTORY; hist_type++) {
				city_history_month[month][hist_type] = 0;
			}
		}
		last_year_bev = city_history_year[0][HIST_CITICENS] = bev;
		last_month_bev = city_history_month[0][HIST_CITICENS] = bev;
		this_year_transported = 0;
		this_year_pax = 0;
	}
	else {
		// 86.00.0 introduced city history
		for (int year = 0;year<MAX_CITY_HISTORY_YEARS;year++) {
			for (int hist_type = 0; hist_type<MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type], " ");
			}
		}
		for (int month=0; month<MAX_CITY_HISTORY_MONTHS; month++) {
			for (int hist_type=0; hist_type<MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type], " ");
			}
		}
		// since we add it internally
		file->rdwr_int(last_year_bev, " ");
		file->rdwr_int(last_month_bev, " ");
		file->rdwr_int(this_year_transported, " ");
		file->rdwr_int(this_year_pax, "\n");
	}

    // 08-Jan-03: Due to some bugs in the special buildings/town hall
    // placement code, li,re,ob,un could've gotten irregular values
    // If a game is loaded, the game might suffer from such an mistake
    // and we need to correct it here.

    if(li < 0) li = 0;
    if(re >= welt->gib_groesse()) re = welt->gib_groesse() - 1;

    if(ob < 0) ob = 0;
    if(un >= welt->gib_groesse()) un = welt->gib_groesse() - 1;
}


/**
 * Wird am Ende der Laderoutine aufgerufen, wenn die Welt geladen ist
 * und nur noch die Datenstrukturenneu verknüpft werden müssen.
 * @author Hj. Malthaner
 */
void stadt_t::laden_abschliessen()
{
    verbinde_fabriken();

    // alten namen freigeben
    // das wurde mit strdup() angelegt, muss also mit free()
    // freigegegeben werden

    // Leider doch nicht immer mit strdup angelegt :(
    //free((void *)welt->lookup(pos)->gib_boden()->text());

    // neu verbinden
    welt->lookup(pos)->gib_kartenboden()->setze_text( name );

    init_pax_ziele();
}


/* calculates the factories which belongs to certain cities */
void
stadt_t::verbinde_fabriken()
{
	slist_tpl<fabrik_t *>fab_list;
	slist_iterator_tpl<fabrik_t *> fab_iter (welt->gib_fab_list());

	while(fab_iter.next()) {
		fabrik_t *fab = fab_iter.get_current();
		// do not check for fish_swarm
		if(strcmp("fish_swarm",fab->gib_besch()->gib_name())!=0) {
			fab_list.insert( fab );
		}
	}

	max_pax_arbeiterziele = 0;
	for(int i=0; i<16; i++) {
		slist_iterator_tpl<fabrik_t *> iter (fab_list);

		// die arbeiter pendeln nicht allzu weit
		int mind = 5000;
		fabrik_t *best = NULL;

		while(iter.next()) {
			fabrik_t *fab = iter.get_current();
			const koord k = fab->gib_pos().gib_2d();
			const int d = (k.x-pos.x) * (k.x-pos.x) + (k.y-pos.y) * (k.y-pos.y);

			// alway connect if closer
			if(d < mind) {
				mind = d;
				best = fab;
			}
		}
		// add as new workes target
		if(best != NULL) {
			best->add_arbeiterziel( this );
			arbeiterziele.insert( best );
			max_pax_arbeiterziele += best->gib_besch()->gib_pax_level();
			fab_list.remove( best );
		}
	}
}


/* add workers to factory list */
void
stadt_t::add_factory_arbeiterziel(fabrik_t *fab)
{
	// no fish swarms ...
	if(strcmp("fish_swarm",fab->gib_besch()->gib_name())!=0) {
		fab->add_arbeiterziel( this );
		// do not add a factory twice!
		if(!arbeiterziele.contains(fab)){
			arbeiterziele.insert( fab );
			max_pax_arbeiterziele += fab->gib_besch()->gib_pax_level();
		}
	}
}



void
stadt_t::step()
{
	// Ist es Zeit für einen neuen step?
	if(welt->gib_zeit_ms() > next_step) {
		// Alle 7 sekunden ein step (or we need to skip ... )
		next_step = MAX( welt->gib_zeit_ms(), next_step+step_interval );

		// Zaehlt steps seit instanziierung
		step_count ++;
		step_passagiere();

		if( (step_count & 3) == 0) {
			step_bau();
		}

		// update history
		city_history_month[0][HIST_CITICENS] = bev;	// total number
		city_history_year[0][HIST_CITICENS] = bev;

		city_history_month[0][HIST_GROWTH] = bev-last_month_bev;	// growth
		city_history_year[0][HIST_GROWTH] = bev-last_year_bev;

		city_history_month[0][HIST_TRANSPORTED] = pax_transport;	// pax transported
		city_history_year[0][HIST_TRANSPORTED] = this_year_transported+pax_transport;

		city_history_month[0][HIST_GENERATED] = pax_erzeugt;	// and all post and passengers generated
		city_history_year[0][HIST_GENERATED] = this_year_pax+pax_erzeugt;
	}
}



/* updates the city history
 * @author prissi
 */
void
stadt_t::roll_history()
{
	int i;

	// first update history
	city_history_month[0][HIST_CITICENS] = bev;	// total number
	city_history_year[0][HIST_CITICENS] = bev;

	city_history_month[0][HIST_GROWTH] = bev-last_month_bev;	// growth
	city_history_year[0][HIST_GROWTH] = bev-last_year_bev;

	city_history_month[0][HIST_TRANSPORTED] = pax_transport;	// pax transported
	this_year_transported += pax_transport;
	city_history_year[0][HIST_TRANSPORTED] = this_year_transported;

	city_history_month[0][HIST_GENERATED] = pax_erzeugt;	// and all post and passengers generated
	this_year_pax += pax_erzeugt;
	city_history_year[0][HIST_GENERATED] = this_year_pax;

	// init differences
	last_month_bev = bev;

	// roll months
	for (i=MAX_CITY_HISTORY_MONTHS-1; i>0; i--) {
		for (int hist_type = 0; hist_type<MAX_CITY_HISTORY; hist_type++) {
			city_history_month[i][hist_type] = city_history_month[i-1][hist_type];
		}
	}
	// init this month
	for (int hist_type = 1; hist_type<MAX_CITY_HISTORY; hist_type++) {
		city_history_month[0][hist_type] = 0;
	}
	city_history_month[0][0] = bev;

	//need to roll year too?
	if(    (welt->gib_zeit_ms() >> karte_t::ticks_bits_per_tag)%12==0  ) {
		for (i=MAX_CITY_HISTORY_YEARS-1; i>0; i--)
		{
			for (int hist_type = 0; hist_type<MAX_CITY_HISTORY; hist_type++)
			{
				city_history_year[i][hist_type] = city_history_year[i-1][hist_type];
			}
		}
		// init this year
		for (int hist_type = 1; hist_type<MAX_CITY_HISTORY; hist_type++) {
			city_history_year[0][hist_type] = 0;
		}
		last_year_bev = bev;
		city_history_year[0][HIST_CITICENS] = bev;
		city_history_year[0][HIST_GROWTH] = 0;
		city_history_year[0][HIST_TRANSPORTED] = 0;
		city_history_year[0][HIST_GENERATED] = 0;
		// init difference counters
		this_year_transported = 0;
		this_year_pax = 0;
	}

}



void
stadt_t::neuer_monat()
{
	const int gr = welt->gib_groesse();

	pax_ziele_alt.copy_from(pax_ziele_neu);


	for(int j=0; j<96; j++) {
		for(int i=0; i<96; i++) {
			const koord pos (i*gr/96, j*gr/96);

			pax_ziele_neu.at(i, j) = reliefkarte_t::calc_relief_farbe(welt, pos);
			//      pax_ziele_neu.at(i, j) = 0;
		}
	}

	roll_history();

	pax_erzeugt = 0;
	pax_transport = 0;
}



void
stadt_t::step_bau()
{
  bool new_town = (bev==0);
  wachstum = (pax_transport << 3) / (pax_erzeugt+1);

  // Hajo: let city grow in steps of 1
//  for(int n = 0; n < (1 + wachstum); n++) {
  // @author prissi: No growth without development
  for(int n = 0; n < wachstum; n++) {

    bev ++; // Hajo: bevoelkerung wachsen lassen

    INT_CHECK("simcity 338");

    int i=0;
    while(i < 30 && bev+bev > won+arb+100) {
      bewerte();
      INT_CHECK("simcity 271");
      baue();
      INT_CHECK("simcity 273");

      i++;
    }

    check_bau_spezial(new_town);
    INT_CHECK("simcity 275");
    check_bau_rathaus(new_town);
    // add industry? (not during creation)
    check_bau_factory(new_town);
  }
}



void
stadt_t::step_passagiere()
{
// long t0 = get_current_time_millis();
// printf("%s step_passagiere called (%d,%d - %d,%d)\n", name, li,ob,re,un);

	INT_CHECK("simcity 434");

	// post oder pax erzeugen ?
	const ware_besch_t * wtyp;
	if(simrand(400) < 300) {
		wtyp = warenbauer_t::passagiere;
	}
	else {
		wtyp = warenbauer_t::post;
	}

	const int row = (step_count & 7);
	const int col = ((step_count >> 3) & 1);
	koord k;

// printf("row=%d, col=%d\n", row, col);

    // nur jede achte Zeile bearbeiten
	for(k.y = ob+row; k.y <= un; k.y += 8) {
		for(k.x = li+col; k.x <= re; k.x += 2) {
			const gebaeude_t *gb = dynamic_cast<const gebaeude_t *> (welt->lookup(k)->gib_kartenboden()->obj_bei(1));

			// ist das dort ein gebaeude ?
			if(gb != NULL) {

				// prissi: since now correctly numbers are used, double initially passengers
				const int num_pax =
					(wtyp == warenbauer_t::passagiere) ?
					(gb->gib_tile()->gib_besch()->gib_level() + 6) >> 1 :
					(gb->gib_post_level() + 3) >> 1;

					// create pedestrians?
					if(umgebung_t::fussgaenger) {
						haltestelle_t::erzeuge_fussgaenger(welt, welt->lookup(k)->gib_kartenboden()->gib_pos(), (num_pax>>3)+1);
						INT_CHECK("simcity 269");
					}

				// starthaltestelle suchen
				const vector_tpl<halthandle_t> &halt_list = welt->lookup(k)->gib_kartenboden()->get_haltlist();

				if(halt_list.get_count() > 0) {
					halthandle_t halt = halt_list.get(0);

//				// Hajo: track number of generated passengers.
//				pax_erzeugt += num_pax;
// prissi: see below
					//printf("  distributing %d pax\n", num_pax);

					// Find passenger destination
					for(int pax_routed=0;  pax_routed<num_pax;  pax_routed+=7) {

						// number of passengers that want to travel
						// if possible, we do 7 passengers at a time
						int pax_left_to_do = MIN(7, num_pax - pax_routed);
//						pax_routed += pax_left_to_do;  // pax_menge are routed this step

						// Hajo: track number of generated passengers.
						// prissi: we do it inside the loop to take also care of non-routable passengers
						pax_erzeugt += pax_left_to_do;

						// Ziel für Passagier suchen
						bool will_return;
						const koord ziel = finde_passagier_ziel(&will_return);

						// Dario: Check if there's a stop near destination
						const vector_tpl <halthandle_t> &ziel_list =
							welt->lookup(ziel)->gib_kartenboden()->get_haltlist();

						if(ziel_list.get_count() == 0){
// DBG_MESSAGE("stadt_t::step_passagiere()", "No stop near dest (%d, %d)", ziel.x, ziel.y);
							// Thus, routing is not possible and we do not need to do a calculation.
							// Mark ziel as destination without route and continue.
							merke_passagier_ziel(ziel, DUNKELORANGE);
							continue;
						}
//						else {
//DBG_DEBUG("stadt_t::step_passagiere()", "Stop near dest (%d, %d)", ziel.x, ziel.y);
//						} // End: Check if there's a stop near destination

						// Passgierziel in Passagierzielkarte eintragen
						merke_passagier_ziel(ziel, GELB);

						ware_t pax (wtyp);
						pax.setze_zielpos( ziel );
						pax.menge = (wtyp==warenbauer_t::passagiere)?pax_left_to_do:MAX(1,pax_left_to_do>>3);

						// prissi: 11-Mar-2005
						// we try all stations to find one not overcrowded
						int halte;
						for(halte=0;  halte<halt_list.get_count();  halte++  ) {
							halt = halt_list.get(halte);
							if(halt->gib_ware_summe(wtyp) <= (halt->gib_grund_count() << 7)) {
								// prissi: not overcrowded
								// so now try routing
								break;
							}
							else {
								// Hajo: Station crowded:
								// they are appalled but will try other stations
								halt->add_pax_unhappy(pax_left_to_do);
							}
						}
						if(halte==halt_list.get_count()) {
							// prissi: only overcrowded stations found ...
							// ### maybe we should still send them back
							continue;
						}

						// Hajo: for efficiency we try to route not every
						// single pax, but packets.
						// the last packet might have less then 7 pax
						koord return_zwischenziel;	// for people going back ...
						bool schon_da;
						// prissi: since we got also a comprehensive list of target destinations,
						// we can avoid the search, if there is a target station next to us
						if(ziel_list.is_contained(halt)) {
							schon_da = true;
						}
						else {
							schon_da = halt->suche_route(pax, halt, will_return?&return_zwischenziel:NULL );
						}

						if(!schon_da) {
							if(pax.gib_ziel() != koord::invalid) {
								// so we have happy traveling passengers
								halt->starte_mit_route(pax);	// liefere an recalculates route, so we use this!

								pax_transport += pax.menge;
								halt->add_pax_happy(pax.menge);

								if(will_return) {
									// send them also back
									// this comes free for calculation and balances also the amounts!
									ware_t return_pax (wtyp);
									return_pax.menge = pax_left_to_do;
									return_pax.setze_zielpos( k );
									return_pax.setze_ziel( halt->gib_basis_pos() );
									return_pax.setze_zwischenziel( return_zwischenziel );
									// now try to add them to the target halt
									// we try all stations to find one not overcrowded
									halthandle_t zwischen_halt=haltestelle_t::gib_halt( welt, return_zwischenziel );
//DBG_DEBUG("will_return","from %s to %s via %s",halt->gib_name(),ziel_list.get(0)->gib_name(),zwischen_halt->gib_name());
									for(halte=0;  halte<ziel_list.get_count();  halte++  ) {
										halt = ziel_list.get(halte);
										if(halt->is_connected(zwischen_halt,wtyp)) {
											if(halt->gib_ware_summe(wtyp)<=(halt->gib_grund_count() << 7)) {
												// prissi: not overcrowded
												// so add them
												halt->starte_mit_route(return_pax);
												halt->add_pax_happy(pax_left_to_do);
												break;
											}
											else {
												// Hajo: Station crowded:
												// they are appalled but will try other stations
												halt->add_pax_unhappy(pax_left_to_do);
											}
										}
										else {
											halt->add_pax_no_route(pax_left_to_do);
										}
									}
								}
							}
							else {
								halt->add_pax_no_route(pax_left_to_do);
								merke_passagier_ziel(ziel, DUNKELORANGE);
							}
						}
						else {
							// ok, they find their destination here too
							// so we declare them happy
							pax_transport += pax.menge;
							halt->add_pax_happy(pax.menge);
						}

					} // while
				}
				else {
					// no halt are in the list
					// Hajo: fake a ride to get a proper display of destinations
					// but this building may generated more ...
					bool will_return;
					const koord ziel = finde_passagier_ziel(&will_return);
					merke_passagier_ziel(ziel, DUNKELORANGE);
				}
			}

		}
		INT_CHECK("simcity 525");
	}

  // long t1 = get_current_time_millis();
  // printf("Zeit für Passagierstep: %ld ms\n", (long)(t1-t0));
}


/**
 * gibt einen zufällingen gleichverteilten Punkt innerhalb der
 * Stadtgrenzen zurück
 * @author Hj. Malthaner
 */
koord
stadt_t::gib_zufallspunkt() const
{
  koord ziel (li + simrand(re - li + 1), ob + simrand(un - ob + 1));

  // Ist das Ziel geeignet?
  const planquadrat_t * plan = welt->lookup(ziel);
  const grund_t * gr = 0;

  if(plan && (gr = plan->gib_kartenboden())) {
    if(gr->ist_wasser()) {
      // DBG_MESSAGE("stadt_t::finde_passagier_ziel()", "water -> reroll");

      // ungeeignet -> 2. Versuch
      ziel = koord(li + simrand(re - li + 1), ob + simrand(un - ob + 1));
    }
  }

  return ziel;
}


koord
stadt_t::finde_passagier_ziel(bool *will_return)
{
	const int rand = simrand(128);

	if(arbeiterziele.is_empty()  ||  max_pax_arbeiterziele==0  ||  rand < 110) {

		koord ziel;

		// Ziel in stadt oder ausflugsziel ?
		// Jeder 4. macht einen Ausflug
		if((rand & 3) == 0  &&  !welt->gib_ausflugsziele().is_empty() ) {
			const gebaeude_t *gb = welt->gib_random_ausflugsziel();
			*will_return = true;	// tourists will return
			ziel = gb->gib_pos().gib_2d();
// DBG_MESSAGE("stadt_t::finde_passagier_ziel()", "created a tourist to %d,%d", ziel.x, ziel.y);
		}
		else {
			const vector_tpl<stadt_t *> *staedte = welt->gib_staedte();
			const stadt_t *zielstadt = staedte->get(simrand(welt->gib_einstellungen()->gib_anzahl_staedte()));

			// nahe staedte bevorzugen
			if(ABS(zielstadt->pos.x - pos.x) + ABS(zielstadt->pos.y - pos.y) > 80) {
				// wenn erste Wahl zu weit weg, dann noch mal versuchen
				zielstadt = staedte->get(simrand(welt->gib_einstellungen()->gib_anzahl_staedte()));
			}
			ziel = zielstadt->gib_zufallspunkt();
			*will_return = false;
		}

		return ziel;
	}
	else {
#ifndef FAB_PAX
		// generate worker
		const int target_pax = simrand(max_pax_arbeiterziele);
		int target = 0;

//DBG_DEBUG("stadt_t::finde_passagier_ziel()","fatory random %i (max %i)", target_pax, max_pax_arbeiterziele );
		slist_iterator_tpl<fabrik_t *> fab_iter (arbeiterziele);
		while(fab_iter.next()) {
			fabrik_t *fab = fab_iter.get_current();
			target += fab->gib_besch()->gib_pax_level();
//DBG_DEBUG("stadt_t::finde_passagier_ziel()","fatory current %i", target, fab->gib_besch()->gib_name() );
			if(target>target_pax) {
				*will_return = true;
				return fab->gib_pos().gib_2d();
			}
		}
#else
		const int i = simrand(arbeiterziele.count());
		const fabrik_t * fab = arbeiterziele.at(i);

		*will_return = true;	// worker will return
		return fab->gib_pos().gib_2d();
#endif
	}
	// we should never reach here!
dbg->fatal("stadt_t::finde_passagier_ziel()","no passenger ziel found!");
	return koord::invalid;
}


void
stadt_t::merke_passagier_ziel(koord k, int color)
{
    const int groesse = welt->gib_groesse();
    const koord p = koord((k.x*96)/groesse, (k.y*96)/groesse);

    // printf("Merke ziel %d,%d\n", p.x, p.y);

    // pax_ziele_neu.at(p) = 121;
    pax_ziele_neu.at(p) = color;
}

/*
void
stadt_t::merke_passagier_ziel(koord k, int pax)
{
    const int groesse = welt->gib_groesse();
    const koord p = koord((k.x*96)/groesse, (k.y*96)/groesse);

    pax_ziele_neu.at(p) += pax;
}
*/


/**
 * bauplatz_mit_strasse_sucher_t:
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 * added: Minimum distance between monuments
 * @author V. Meyer/prissi
 */
class bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {

	public:
		bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t (welt) {}

	// get distance to next factory
	int find_dist_next_special(koord pos) const {
		const slist_tpl<gebaeude_t *> & list = welt->gib_ausflugsziele();
		slist_iterator_tpl <gebaeude_t *> iter (list);
		int dist = welt->gib_groesse()*2;
		while(iter.next()) {
			gebaeude_t * gb = iter.get_current();
			int d = koord_distance(gb->gib_pos(),pos);
			if(d<dist) {
				dist = d;
			}
		}
		return dist;
	}

	bool strasse_bei(int x, int y) const {
		grund_t *bd = welt->lookup(koord(x, y))->gib_kartenboden();
		return bd && bd->gib_weg(weg_t::strasse);
	}

	virtual bool ist_platz_ok(koord pos, int b, int h) const {
		if(bauplatz_sucher_t::ist_platz_ok(pos, b, h)) {
			// try to built a little away from previous factory
			if(find_dist_next_special(pos)<b+h) {
				return false;
			}
			// now check for road connection
			int i;
			for(i = pos.y; i < pos.y + h; i++) {
				if(strasse_bei(pos.x - 1, i) ||  strasse_bei(pos.x + b, i)) {
					return true;
				}
			}
			for(i = pos.x; i < pos.x + b; i++) {
				if(strasse_bei(i, pos.y - 1) ||  strasse_bei(i, pos.y + h)) {
					return true;
				}
			}
		}
		return false;
	}
};




void
stadt_t::check_bau_spezial(bool new_town)
{
	// touristenattraktion bauen
	const haus_besch_t *besch = hausbauer_t::gib_special(bev);
	if(besch) {
		if(simrand(100) < besch->gib_chance()) {
			// baue was immer es ist
			int rotate;
			bool is_rotate;
//			koord best_pos ( bauplatz_sucher_t(welt).suche_platz(pos, besch->gib_b(), besch->gib_h(), &rotate)  );
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos,  besch->gib_b(), besch->gib_h(),&is_rotate);

			if(best_pos != koord::invalid) {
				// then built it
				if(besch->gib_b()==besch->gib_h()) {
					// do we have more versions?
					rotate = simrand(3);
				}
				else {
					rotate = (simrand(20)&2) + is_rotate;
				}
				hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos)->gib_kartenboden()->gib_pos(), rotate, besch);
				// tell the player, if not during initialization
				if(!new_town) {
					char buf[256];
					sprintf(buf, translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), gib_name(), make_single_line_string(translator::translate(besch->gib_name()),2), bev );
					message_t::get_instance()->add_message(buf,best_pos,message_t::tourist,CITY_KI,besch->gib_tile(0)->gib_hintergrund(0,0) );
				}
			}
		}
	}

	if((bev & 511) == 0) {
		//
		// Denkmal bauen:
		//
		besch = hausbauer_t::waehle_denkmal();
		if(besch) {
			koord best_pos ( denkmal_platz_sucher_t(welt,
				besitzer_p).suche_platz(pos, 3, 3) );

			if(best_pos != koord::invalid) {
				int i, j;
				bool ok = false;

				// Wir bauen das Denkmal nur, wenn schon mindestens eine Strasse da ist
				for(i = 0; i < 3 && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(3, i))->gib_kartenboden()->gib_weg(weg_t::strasse) ||
						welt->access(best_pos + koord(i, -1))->gib_kartenboden()->gib_weg(weg_t::strasse) ||
						welt->access(best_pos + koord(-1, i))->gib_kartenboden()->gib_weg(weg_t::strasse) ||
						welt->access(best_pos + koord(i, 3))->gib_kartenboden()->gib_weg(weg_t::strasse);
				}
				if(ok) {
					// Straßenkreis um das Denkmal legen
					for(i = 0; i < 3; i++) {
						for(j = 0; j < 3; j++) {
							if(i != 1 || j != 1) {
								baue_strasse(best_pos + koord(i, j), NULL, true);
							 }
						}
					}
					// and then built it
					hausbauer_t::baue(welt, besitzer_p,
					      welt->lookup(best_pos + koord(1, 1))->gib_kartenboden()->gib_pos(), 0, besch);
					hausbauer_t::denkmal_gebaut(besch);
					// tell the player, if not during initialization
					if(!new_town) {
						char buf[256];
						sprintf(buf, translator::translate("With a big festival\n%s built\na new monument.\n%i citicens rejoiced."), gib_name(), bev );
						message_t::get_instance()->add_message(buf,best_pos,message_t::city,CITY_KI,besch->gib_tile(0)->gib_hintergrund(0,0) );
					}
				}
			}
		}
	}
}



void
stadt_t::check_bau_rathaus(bool new_town)
{
    const haus_besch_t *besch = hausbauer_t::gib_rathaus(bev);

    if(besch) {
  grund_t *gr = welt->lookup(pos)->gib_kartenboden();
  gebaeude_t *gb = dynamic_cast<gebaeude_t *>(gr->obj_bei(1));
  bool neugruendung = !gb || !gb->ist_rathaus();
  bool umziehen = !neugruendung;
  koord alte_str ( koord::invalid );
  koord best_pos ( pos );
  koord k;

  DBG_MESSAGE("check_bau_rathaus()",
         "bev=%d, new=%d", bev, neugruendung);


  if(!neugruendung) {
      if(gb->gib_tile()->gib_besch()->gib_level() == besch->gib_level())  {
        DBG_MESSAGE("check_bau_rathaus()",
         "town hall already ok.");

    return; // Rathaus ist schon okay
      }

      const haus_besch_t *besch_alt = gb->gib_tile()->gib_besch();
      koord pos_alt = gr->gib_pos().gib_2d() - gb->gib_tile()->gib_offset();
      koord groesse_alt = besch_alt->gib_groesse(gb->gib_tile()->gib_layout());

      // Wir gehen hier von quadratisch aus:
      if(besch->gib_h() <= groesse_alt.y) {
    umziehen = false;   // Platz reicht - Umzug nicht nötig
      }
      else if(welt->lookup(pos + koord(0, besch_alt->gib_h()))->gib_kartenboden()->gib_weg(weg_t::strasse)) {
    // Wenn die Strasse vor dem alten Rathaus noch da ist,
    // verbinden wir sie nachher mit der neuen Position.
    alte_str =  pos + koord(0, besch_alt->gib_h());
      }

      //
      // Jetzt räumen wir so oder so das alte Rathaus ab.
      //
      for(k.x = 0; k.x < groesse_alt.x; k.x++) {
    for(k.y = 0; k.y < groesse_alt.y; k.y++) {
        gr = welt->lookup(pos_alt + k)->gib_kartenboden();

        gr->obj_bei(0)->setze_besitzer(NULL); // da muß ein Fundament sein!
        gr->setze_besitzer(NULL);     // Nicht mehr Rathaus, wieder abreissbar!

        if(!umziehen && k.x < besch->gib_b() && k.y < besch->gib_h()) {
      // Platz für neues Rathaus freimachen
      ding_t *gb = gr->obj_bei(1);
      gr->obj_remove(gb, besitzer_p);
      gb->setze_pos(koord3d::invalid); // Hajo: mark 'not on map'
      delete gb;
        }
        else {
      // Altes Rathaus durch Wohnhaus(0) ersetzen - Wohnhaus(0) muß 1x1 sein!
      hausbauer_t::umbauen(welt, gb, hausbauer_t::gib_wohnhaus(0));
      gb->setze_besitzer(NULL);
        }
    }

    // printf("Alte Position=%d,%d\n", pos_alt.x+k.x, pos_alt.y+k.y);
    // welt->lookup(pos_alt + k)->gib_kartenboden()->setze_besitzer(NULL);
      }
      gr->setze_text(NULL);
  }


  //
  // Neues Rathaus bauen
  //
  if(neugruendung || umziehen) {
      best_pos = rathausplatz_sucher_t(welt, besitzer_p).suche_platz(
    pos, besch->gib_b(), besch->gib_h() + 1);
  }
        hausbauer_t::baue(welt, besitzer_p,
      welt->lookup(best_pos)->gib_kartenboden()->gib_pos(), 0, besch);

  // Orstnamen hinpflanzen
  welt->lookup(best_pos)->gib_kartenboden()->setze_text(name);

	// tell the player, if not during initialization
	if(!new_town) {
		char buf[256];
		sprintf(buf, translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), gib_name(), bev );
		message_t::get_instance()->add_message(buf,pos,message_t::city,CITY_KI,besch->gib_tile(0)->gib_vordergrund(0,0) );
	}

  // Strasse davor verlegen
  // Hajo: nur, wenn der Boden noch niemand gehört!
  k = koord(0, (int)besch->gib_h());
  for(k.x = 0; k.x < besch->gib_b(); k.x++) {
    grund_t * gr = welt->lookup(best_pos+k)->gib_kartenboden();

    // printf("Position=%d,%d\n", (best_pos+k).x, (best_pos+k).y);
    // printf("Besitzer=%d\n", welt->sp2num(gr->gib_besitzer()));

    if(gr->gib_besitzer() == 0) {
      baue_strasse(best_pos + k, NULL, true);
    } else {
      // Hajo: Strassenbau nicht versuchen, eines der Felder
      // ist schon belegt
      alte_str == koord::invalid;
    }
  }
  if(umziehen && alte_str != koord::invalid ) {
      // Strasse vom ehemaligen Rathaus zum neuen verlegen.
      wegbauer_t bauer(welt, NULL);
      bauer.route_fuer(wegbauer_t::strasse,
           wegbauer_t::gib_besch("city_road"));
      bauer.calc_route(alte_str, best_pos + koord(0, besch->gib_h()));
      bauer.baue();

      /* Hajo: seems to be buggy and not needed at all
             *       I leave it here if Volker wants to check that
             *       case again
      // Ort verschieben:
      li += best_pos.x - pos.x;
      re += best_pos.x - pos.x;
      ob += best_pos.y - pos.y;
      un += best_pos.y - pos.y;
      */
  }
  else if(neugruendung) {
      li = best_pos.x - 2;
      re = best_pos.x + besch->gib_b() + 2;
      ob = best_pos.y - 2;
      un = best_pos.y + besch->gib_h() + 2;
  }
  pos = best_pos;
    }

    // Hajo: paranoia - ensure correct bounds in all cases
    //       I don't know if best_pos is always valid

    if(li < 0) li = 0;
    if(re >= welt->gib_groesse()) re = welt->gib_groesse() - 1;

    if(ob < 0) ob = 0;
    if(un >= welt->gib_groesse()) un = welt->gib_groesse() - 1;
}




/* eventually adds a new industry
 * so with growing number of inhabitants the industry grows
 * @date 12.1.05
 * @author prissi
 */
void
stadt_t::check_bau_factory(bool new_town)
{
	if(!new_town  &&  industry_increase_every[0]>0  &&  bev%industry_increase_every[0]==0) {
		for(int i=0;  i<8;  i++  ) {
			if(industry_increase_every[i]==bev) {
				const fabrik_besch_t *market=fabrikbauer_t::get_random_consumer(true);
		//		koord size=market->gib_haus()->gib_groesse();
				bool	rotate=false;

				koord3d	market_pos=welt->lookup(pos)->gib_kartenboden()->gib_pos();
DBG_MESSAGE("stadt_t::check_bau_factory","adding new industry at %i inhabitants.",bev);
				int n=fabrikbauer_t::baue_hierarchie(welt, NULL, market, rotate, &market_pos, welt->gib_spieler(1) );
				// tell the player
				char buf[256];
				sprintf(buf, translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(market->gib_name()), gib_name(), n );
				message_t::get_instance()->add_message(buf,market_pos.gib_2d(),message_t::industry,CITY_KI,market->gib_haus()->gib_tile(0)->gib_hintergrund(0,0) );
				break;
			}
		}
	}
}


/*
 * transformationfunktionen fuer die Regeln
 *
 * Regeln liegen Zeilenweise im Speicher
 */

// spiegeln in y

void
stadt_t::trans_y(const char *regel, char *tr)
{
  int x,y;

  for(x=0; x<3; x++) {
    for(y=0; y<3; y++) {
      tr[x + ((2-y)<<2)] = regel[x+(y<<2)];
    }
  }
}

// drehen 90 Grad links

void
stadt_t::trans_l(const char *regel, char *tr)
{
  int x,y;

  for(x=0; x<3; x++) {
    for(y=0; y<3; y++) {
      const int xx = y;
      const int yy = 2-x;
      tr[xx + (yy<<2)] = regel[x+(y<<2)];
    }
  }
}

// drehen 90 Grad rechts

void
stadt_t::trans_r(const char *regel, char *tr)
{
  int x,y;

  for(x=0; x<3; x++) {
    for(y=0; y<3; y++) {
      const int xx = 2-y;
      const int yy = x;
      tr[xx + (yy<<2)] = regel[x+(y<<2)];
    }
  }
}


/**
 * Symbols in rules:
 * S = darf keine Strasse sein
 * s = muss Strasse sein
 * n = muss Natur sein
 * H = darf kein Haus sein
 * h = muss Haus sein
 * . = beliebig
 *
 * @param pos position to check
 * @param regel the rule to evaluate
 * @return true on match, false otherwise
 * @author Hj. Malthaner
 */
bool
stadt_t::bewerte_loc(const koord pos, const char *regel)
{
  koord k;
  bool ok=true;

  for(k.y=pos.y-1; ok && k.y<=pos.y+1; k.y++) {
    for(k.x=pos.x-1; ok && k.x<=pos.x+1; k.x++) {
      const planquadrat_t * plan = welt->lookup(k);
      grund_t *gr;

      if(plan && (gr = plan->gib_kartenboden())) {

  switch(regel[(k.x-pos.x+1) + ((k.y-pos.y+1)<<2)]) {
  case 's':
    ok = gr->gib_weg(weg_t::strasse) &&
      (gr->gib_top() <= 0);
    break;
  case 'S':
    ok = !gr->gib_weg(weg_t::strasse);
    break;
  case 'h':
    ok = gr->obj_bei(0) != 0 &&
      gr->obj_bei(0)->gib_typ() == ding_t::gebaeudefundament;
    break;
  case 'H':
    ok = gr->obj_bei(0) == 0 ||
      gr->obj_bei(0)->gib_typ() != ding_t::gebaeudefundament;
    break;

  case 'n':
    ok = gr->ist_natur() && !gr->gib_besitzer();
    break;
  }
      } else {
  ok = false;
      }
    }
  }
  return ok;
}


/**
 * Check rule in all transformations at given position
 * @author Hj. Malthaner
 */
int
stadt_t::bewerte_pos(const koord pos, const char *regel)
{
    char tr[12];    // fuer die 'transformierte' Regel

    int w=bewerte_loc(pos, regel);
    trans_y(regel, tr);
    w+=bewerte_loc(pos, tr);

    trans_l(regel, tr);
    w+=bewerte_loc(pos, tr);

    trans_r(regel, tr);
    w+=bewerte_loc(pos, tr);

    return w;
}

void
stadt_t::bewerte_strasse(koord k, int rd, char *regel)
{
    if(simrand(rd) == 0) {
  best_strasse.check(k, bewerte_pos(k, regel));
    }
}

void
stadt_t::bewerte_haus(koord k, int rd, char *regel)
{
    if(simrand(rd) == 0) {
  best_haus.check(k, bewerte_pos(k, regel));
    }
}

/*
 * Zeichen in Regeln:
 * S = darf keine Strasse sein
 * s = muss Strasse sein
 * n = muss Natur sein
 * . = beliebig
 */

void
stadt_t::bewerte()
{
    const int speed = 8;

    // Zufallspos im Stadtgebiet
    const koord k (li + simrand(re-li+1),
       ob + simrand(un-ob+1));

    best_haus.reset(k);
    best_strasse.reset(k);

    /*
    bewerte_haus(k, speed-4,"....n.sss"); // Haus am Strassenrand
    bewerte_haus(k, speed+12,"....n.ss.");  // Haus am Strassenrand
    bewerte_haus(k, speed+12,"....n..ss");  // Haus am Strassenrand
    INT_CHECK("simcity 710");

    bewerte_strasse(k, speed+3,"...SnSssS");  // Kurve 1
    bewerte_strasse(k, speed+3,"...SnSSss");  // Kurve 2

    bewerte_strasse(k, speed+3,"...SnSSs.");  // Strassenende weiterbauen
    INT_CHECK("simcity 716");
    bewerte_strasse(k, speed+3,"...SnS.sS");  // Strassenende weiterbauen
    bewerte_strasse(k, speed,"....n.SsS");  // Strassenende weiterbauen
    bewerte_strasse(k, speed+1,"...nnnsss");    // Einmuendung in Natur
    */

    /*
    // mit hoher Wahrscheinlichkeit Haus bauen, wenn...
    bewerte_haus(k, speed-2, "... Hn. sss"); // Haus am Strassenrand
    bewerte_haus(k, speed-2, "... .nH sss"); // Haus am Strassenrand
    // auf jeden Fall Haus bauen, wenn...
    bewerte_haus(k, speed-7, "... .ns .ss"); // Haus an der Ecke
    bewerte_haus(k, speed-7, "... sn. ss."); // Haus an der Ecke
    bewerte_haus(k, speed-7, ".h. hns .sH"); // fehlendes Haus vom Block
    bewerte_haus(k, speed-7, ".h. snh Hs."); // fehlendes Haus vom Block
    bewerte_haus(k, speed-7, ".h. hnh .s."); // Haus "mittendrin"

    // bewerte_strasse(k, speed+18,".H. SnS .sS"); // Strassenende weiterbauen
    bewerte_strasse(k, speed+4,".H. SnS .sS"); // Strassenende weiterbauen
    //bewerte_strasse(k, speed+4, ".H. .n. SsS"); // Strassenende weiterbauen
    bewerte_strasse(k, speed+2, ".H. .n. SsS"); // Strassenende weiterbauen
    bewerte_strasse(k, speed-2, "... SnS SsS"); // Strassenende weiterbauen
    bewerte_strasse(k, speed-2, ".h. hns .sh"); // Lücke schließen
    bewerte_strasse(k, speed-2, ".h. snh hs."); // Lücke schließen

    // Einmündung bauen (nie bauen, wenn es Sackgasse zu einem Haus ist)
    bewerte_strasse(k, speed+0, ".H. nnn sss"); // Einmuendung in Natur
    bewerte_strasse(k, speed-5, "SHS nnn sss"); // Einmuendung in Natur
    bewerte_strasse(k, speed-7, ".H. hnh sss"); // Einmuendung zwischen 2 Häusern
    */
    int i;
    for(i=0; i<num_house_rules; i++) {
      bewerte_haus(k,
       speed+house_rules[i].chance,
       house_rules[i].rule);
    }


    for(i=0; i<num_road_rules; i++) {
      bewerte_strasse(k,
          speed+road_rules[i].chance,
          road_rules[i].rule);
    }
}


gebaeude_t::typ
stadt_t::was_ist_an(const koord k) const
{
    const planquadrat_t *plan = welt->lookup(k);
    gebaeude_t::typ t = gebaeude_t::unbekannt;

    if(plan) {
  const ding_t *d = plan->gib_kartenboden()->obj_bei(1);
  const gebaeude_t *gb = dynamic_cast<const gebaeude_t *>(d);

  if(gb) {
      t = gb->gib_haustyp();
  }
    }

//    printf("an pos %d,%d ist ein gebaeude vom typ %d\n", x, y, t);

    return t;
}

int
stadt_t::bewerte_industrie(const koord pos)
{
    koord k;
    int score = 0;

    for(k.y=pos.y-2; k.y<=pos.y+2; k.y++) {
  for(k.x=pos.x-2; k.x<=pos.x+2; k.x++) {
      if(was_ist_an(k) == gebaeude_t::industrie) {
    score += 8;
      } else if(was_ist_an(k) == gebaeude_t::wohnung) {
    score -= 8;
      }

  }
    }
    return score;
}

int
stadt_t::bewerte_gewerbe(const koord pos)
{
    koord k;
    /*
    int score = -20;

    for(k.y=pos.y-2; k.y<=pos.y+2; k.y++) {
  for(k.x=pos.x-2; k.x<=pos.x+2; k.x++) {
      if(was_ist_an(k) == gebaeude_t::industrie) {
    score += 4;
      } else if(was_ist_an(k) == gebaeude_t::wohnung) {
    score += 8;
      }
  }
    }
*/

    int score = -10;

    for(k.y=pos.y-2; k.y<=pos.y+2; k.y++) {
  for(k.x=pos.x-2; k.x<=pos.x+2; k.x++) {
      if(was_ist_an(k) == gebaeude_t::industrie) {
    score += 1;
      } else if(was_ist_an(k) == gebaeude_t::wohnung) {
    score += 1;
      } else if(was_ist_an(k) == gebaeude_t::gewerbe) {
    score += 8;
      }
  }
    }


    return score;
}

int
stadt_t::bewerte_wohnung(const koord pos)
{
    koord k;
    int score = 0;

    for(k.y=pos.y-2; k.y<=pos.y+2; k.y++) {
  for(k.x=pos.x-2; k.x<=pos.x+2; k.x++) {
      if(was_ist_an(k) == gebaeude_t::wohnung) {
    score += 8;
      } else if(was_ist_an(k) == gebaeude_t::industrie) {
    score -= 8;
      }
  }
    }
    return score;
}

void
stadt_t::baue_gebaeude(const koord k)
{
  const koord3d pos ( welt->lookup(k)->gib_kartenboden()->gib_pos() );

  if(welt->lookup(pos)->ist_natur()  &&
  	welt->lookup(pos)->suche_obj(ding_t::leitung)==NULL  &&
  	welt->lookup(pos)->suche_obj(ding_t::pumpe)==NULL  &&
  	welt->lookup(pos)->suche_obj(ding_t::senke)==NULL) {

    // bisher gibt es 2 Sorten Haeuser
    // arbeit-spendende und wohnung-spendende

    int will_arbeit = (bev - arb) / 4;  // Nur ein viertel arbeitet
    int will_wohnung = (bev - won);

    // der Bauplatz muss bewertet werden

    const int passt_industrie = bewerte_industrie(k);
    INT_CHECK("simcity 813");
    const int passt_gewerbe = bewerte_gewerbe(k);
    INT_CHECK("simcity 815");
    const int passt_wohnung = bewerte_wohnung(k);


    const int sum_gewerbe = passt_gewerbe + will_arbeit;
    const int sum_industrie = passt_industrie + will_arbeit;
    const int sum_wohnung = passt_wohnung + will_wohnung;

    //    printf("%d Leute, %d ohne Arbeit, %d ohne Wohnung\n",bev, will_arbeit, will_wohnung);
    //    printf("%d Industrie, %d Gewerbe, %d  Wohnung\n", sum_industrie, sum_gewerbe, sum_wohnung);

    if(sum_gewerbe > sum_industrie && sum_gewerbe > sum_wohnung) {
      hausbauer_t::baue(welt, NULL, pos, 0, hausbauer_t::gib_gewerbe(0));
      arb += 20;
    }

    if(sum_industrie > sum_gewerbe && sum_industrie > sum_wohnung) {
      hausbauer_t::baue(welt, NULL, pos, 0, hausbauer_t::gib_industrie(0));
      arb += 20;
    }

    if(sum_wohnung > sum_industrie && sum_wohnung > sum_gewerbe) {
      hausbauer_t::baue(welt, NULL, pos, 0, hausbauer_t::gib_wohnhaus(0));
      won += 10;
    }
  }
}


void
stadt_t::erzeuge_verkehrsteilnehmer(koord pos, int level)
{
    if(welt->gib_einstellungen()->gib_verkehr_level() != 16 &&
       (level == welt->gib_einstellungen()->gib_verkehr_level() ||
  level == welt->gib_einstellungen()->gib_verkehr_level()*2)) {

  koord k;

  for(k.y = pos.y-1; k.y<=pos.y+1; k.y ++) {
      for(k.x = pos.x-1; k.x<=pos.x+1; k.x ++) {
    if(welt->ist_in_kartengrenzen(k)) {
        grund_t *gr = welt->lookup(k)->gib_kartenboden();
        const weg_t *weg = gr->gib_weg(weg_t::strasse);

        if(weg &&
      (gr->gib_weg_ribi_unmasked(weg_t::strasse) == ribi_t::nordsued ||
                        gr->gib_weg_ribi_unmasked(weg_t::strasse) == ribi_t::ostwest))
                    {
      if(stadtauto_t::gib_anzahl_besch() > 0) {
          stadtauto_t *vt = new stadtauto_t(welt, gr->gib_pos());
          gr->obj_add( vt );
          welt->sync_add( vt );
      }
      return;
        }
    }
      }
  }
    }
}

void
stadt_t::renoviere_gebaeude(koord k)
{
    const gebaeude_t::typ alt_typ = was_ist_an(k);

    if(alt_typ == gebaeude_t::unbekannt) {
  return;   // kann nur bekannte gebaeude renovieren
    }

    gebaeude_t *gb = dynamic_cast<gebaeude_t *>(welt->lookup(k)->gib_kartenboden()->obj_bei(1));

    if(gb == NULL) {
  dbg->error("stadt_t::renoviere_gebaeude()",
       "could not find a building at (%d,%d) but there should be one!", k.x, k.y);
  return;
    }

    // hier sind wir sicher dass es ein Gebaeude ist

    const int level = gb->gib_tile()->gib_besch()->gib_level();

    // bisher gibt es 2 Sorten Haeuser
    // arbeit-spendende und wohnung-spendende

    const int will_arbeit = (bev - arb) / 4;  // Nur ein viertel arbeitet
    const int will_wohnung = (bev - won);

    // der Bauplatz muss bewertet werden

    const int passt_industrie = bewerte_industrie(k);
    const int passt_gewerbe = bewerte_gewerbe(k);
    const int passt_wohnung = bewerte_wohnung(k);

    // verlust durch abriss

    const int sum_gewerbe = passt_gewerbe + will_arbeit;
    const int sum_industrie = passt_industrie + will_arbeit;
    const int sum_wohnung = passt_wohnung + will_wohnung;


    // printf("Pruefe Renovierung\n");

    gebaeude_t::typ will_haben = gebaeude_t::unbekannt;
    int sum = 0;

    if(sum_gewerbe > sum_industrie && sum_gewerbe > sum_wohnung) {
  will_haben = gebaeude_t::gewerbe;
  sum = sum_gewerbe;
    } else if ( sum_industrie > sum_gewerbe && sum_industrie > sum_wohnung) {
  will_haben = gebaeude_t::industrie;
  sum = sum_industrie;
    } else if ( sum_wohnung > sum_industrie && sum_wohnung > sum_gewerbe) {
  will_haben = gebaeude_t::wohnung;
  sum = sum_wohnung;
    }

    if(alt_typ != will_haben) {
  sum -= level * 10;
    }

    if(sum > 0) {
  const int neu_lev = (alt_typ == will_haben) ? level + 1 : level;

  if(will_haben == gebaeude_t::wohnung) {
      hausbauer_t::umbauen(welt, gb, hausbauer_t::gib_wohnhaus(neu_lev));
      won += neu_lev * 10;
  } else if(will_haben == gebaeude_t::gewerbe) {
      hausbauer_t::umbauen(welt, gb, hausbauer_t::gib_gewerbe(neu_lev));
      arb += neu_lev * 20;
  } else if(will_haben == gebaeude_t::industrie) {
      hausbauer_t::umbauen(welt, gb, hausbauer_t::gib_industrie(neu_lev));
      arb += neu_lev * 20;
  }

  // printf("Renovierung mit %d Industrie, %d Gewerbe, %d  Wohnung\n", sum_industrie, sum_gewerbe, sum_wohnung);

  if(alt_typ == gebaeude_t::industrie)
      arb -= level * 20;
  if(alt_typ == gebaeude_t::gewerbe)
      arb -= level * 20;
  if(alt_typ == gebaeude_t::wohnung)
      won -= level * 10;

  erzeuge_verkehrsteilnehmer(k, neu_lev);
    }
}

/**
 * baut ein Stueck Strasse
 *
 * @param k         Bauposition
 *
 * @author Hj. Malthaner, V. Meyer
 */
bool
stadt_t::baue_strasse(koord k, spieler_t *sp, bool forced)
{
  grund_t *bd = welt->lookup(k)->gib_kartenboden();

  // check for old road
  if(bd->gib_hoehe() <= welt->gib_grundwasser()) {
    return false;
  }

  const hang_t::typ typ = welt->get_slope( k );

  if( hang_t::ist_wegbar(typ) &&
      bd->kann_alle_obj_entfernen(welt->gib_spieler(1)) == NULL) {

    ribi_t::ribi ribi = ribi_t::keine;

    for(int r = 0; r < 4; r++) {
      bool ok = false;
      const hang_t::typ typ2 = welt->get_slope(k + koord::nsow[r]);

      // prüfe hanglage auf richtung
      switch(ribi_t::nsow[r]) {
      case ribi_t::ost:
      case ribi_t::west:
  ok = hang_t::ist_wegbar_ow(typ) && hang_t::ist_wegbar_ow(typ2);
  break;
      case ribi_t::nord:
      case ribi_t::sued:
  ok = hang_t::ist_wegbar_ns(typ) && hang_t::ist_wegbar_ns(typ2);
  break;
      }
      if(ok) {
  grund_t *bd2 = welt->access(k + koord::nsow[r])->gib_kartenboden();
  weg_t *weg2 = bd2->gib_weg(weg_t::strasse);

  if(weg2) {
    weg2->ribi_add(ribi_t::rueckwaerts(ribi_t::nsow[r]));
    bd2->calc_bild();
    ribi |= ribi_t::nsow[r];
  }
      }
    }
    if(ribi != ribi_t::keine || forced) {

      if(!bd->weg_erweitern(weg_t::strasse, ribi)) {
  strasse_t *weg = new strasse_t(welt);
  weg->setze_gehweg( true );
  weg->setze_besch(wegbauer_t::gib_besch("city_road"));

  // Hajo: city roads should not belong to any player
  bd->neuen_weg_bauen(weg, ribi, sp);
      } else {
  bd->setze_besitzer( sp );
      }
      return true;
    }
  }
  return false;
}


void
stadt_t::baue()
{
//    printf("Haus: %d  Strasse %d\n",best_haus_wert, best_strasse_wert);

    if(best_strasse.found()) {
  if(!baue_strasse(best_strasse.gib_pos(), NULL, false)) {
      baue_gebaeude(best_strasse.gib_pos());
  }
  pruefe_grenzen(best_strasse.gib_pos());
    }

    INT_CHECK("simcity 1156");

    if(best_haus.found()) {
  baue_gebaeude(best_haus.gib_pos());
  pruefe_grenzen(best_haus.gib_pos());
    }

    INT_CHECK("simcity 1163");

    if(!best_haus.found() && !best_strasse.found() &&
       was_ist_an(best_haus.gib_pos()) != gebaeude_t::unbekannt) {

  if(simrand(8) <= 2) { // nicht so oft renovieren
      renoviere_gebaeude(best_haus.gib_pos());
      INT_CHECK("simcity 876");
        }
    }
}

void
stadt_t::pruefe_grenzen(koord k)
{
    if(k.x <= li && k.x > 0) {
  li = k.x - 1;
    }

    if(k.x >= re && k.x < welt->gib_groesse() -1) {
  re = k.x+1;
    }

    if(k.y <= ob && k.y > 0) {
  ob = k.y-1;
    }

    if(k.y >= un && k.y< welt->gib_groesse()-1) {
  un = k.y+1;
    }
}


/**
 * Creates a station name
 * @param number if >= 0, then a number is added to the name
 * @author Hj. Malthaner
 */
const char *
stadt_t::haltestellenname(koord k, const char *typ, int number)
{
    const char *base = "%s Fehler %s";

    int li_gr = li + 2;
    int re_gr = re - 2;
    int ob_gr = ob + 2;
    int un_gr = un - 2;

    if(li_gr<k.x && re_gr>k.x && ob_gr<k.y && un_gr>k.y) {
  base = zentrum_namen[zentrum_namen_cnt % anz_zentrum];
  zentrum_namen_cnt ++;

    } else if(li_gr-6<k.x && re_gr+6>k.x && ob_gr-6<k.y && un_gr+6>k.y) {
  if(k.y < ob_gr) {
      if(k.x<li_gr) {
    base = nordwest_namen[0];
      } else if(k.x>re_gr) {
    base = nordost_namen[0];
      } else {
    base = nord_namen[0];
      }
  } else if(k.y > un_gr) {
      if(k.x<li_gr) {
    base = suedwest_namen[0];
      } else if(k.x>re_gr) {
    base = suedost_namen[0];
      } else {
    base = sued_namen[0];
      }
  } else {
      if(k.x <= li_gr) {
    base = west_namen[0];
      } else if(k.x >= re_gr) {
    base = ost_namen[0];
      } else {
    base = zentrum_namen[zentrum_namen_cnt % anz_zentrum];
    zentrum_namen_cnt ++;
      }
  }
    } else {
  base = aussen_namen[aussen_namen_cnt % anz_aussen];
  aussen_namen_cnt ++;
    }


    char buf [256];

    if(number >= 0 && umgebung_t::numbered_stations) {
      const int n = sprintf(buf, translator::translate(base),
          this->name,
          translator::translate(typ));

      sprintf(buf+n, " (%d)", number);
    } else {
      sprintf(buf, translator::translate(base),
        this->name,
        translator::translate(typ));
    }

    const int len = strlen(buf) + 1;
    char *name = new char[len];
    tstrncpy(name, buf, len);

    return name;
}


// geeigneten platz zur Stadtgruendung durch Zufall ermitteln
vector_tpl<koord> *
stadt_t::random_place(const karte_t *wl, const int anzahl)
{
	slist_tpl<koord> *list = wl->finde_plaetze(2,3);
	vector_tpl<koord> *result = new vector_tpl<koord> (anzahl);

	for(int  i=0;  i<anzahl;  i++) {

		int len = list->count();
		// check distances of all cities to their respective neightbours
		while(len > 0) {
			const int index = simrand(len);
			int minimum_dist = 0x7FFFFFFF;  // init with maximum
			koord k = list->at(index);

			// check minimum distance
			for(  int j=0;  j<i;  j++  ) {
				int dist = abs(k.x-result->at(j).x) + abs(k.y-result->at(j).y);
				if(  minimum_dist>dist  ) {
					minimum_dist = dist;
				}
			}
			list->remove( k );
			len --;
			if(  minimum_dist>minimum_city_distance  ) {
				// all citys are far enough => ok, find next place
				result->append( k );
				break;
			}
			// if we reached here, the city was not far enough => try again
		}

		if(len<=0  &&  i<anzahl-1) {
			if(i<=0) {
dbg->fatal("stadt_t::random_place()","Not a single place found!");
				delete result;
				return NULL;
			}
dbg->warning("stadt_t::random_place()","Not enough places found!");
			break;
		}
	}
	list->clear();
	delete(list);

	return result;
}


/**
 * Reads city configuration data
 * @author Hj. Malthaner
 */
bool stadt_t::init()
{
  const char *filename = "config/cityrules.tab";
  tabfile_t cityconf;
  const bool ok = cityconf.open(filename);

  if(ok) {
    tabfileobj_t contents;
    cityconf.read(contents);

    char buf[128];

    minimum_city_distance = contents.get_int("minimum_city_distance", 16);
    int ind_increase = contents.get_int("industry_increase_every", 0);
    for( int i=0;  i<8;  i++  ) {
    	industry_increase_every[i] = (ind_increase<<i);
    }

    num_house_rules = 0;
    while (true) {
      sprintf(buf, "house_%d", num_house_rules+1);
      if(contents.get_string(buf, 0)) {
  num_house_rules ++;
      } else {
  break;
      }
    }

    DBG_MESSAGE("stadt_t::init()", "Read %d house building rules", num_house_rules);


    num_road_rules = 0;
    while (true) {
      sprintf(buf, "road_%d", num_road_rules+1);
      if(contents.get_string(buf, 0)) {
  num_road_rules ++;
      } else {
  break;
      }
    }

    DBG_MESSAGE("stadt_t::init()", "Read %d road building rules", num_road_rules);

    house_rules = new struct rule_t [num_house_rules];
    road_rules = new struct rule_t [num_road_rules];

    int i;
    for(i=0; i<num_house_rules; i++) {
      sprintf(buf, "house_%d", i+1);
      const char * rule = contents.get_string(buf, "");
      tstrncpy(house_rules[i].rule, rule+1, 12);
      house_rules[i].rule[11] = '\0';

      sprintf(buf, "house_%d.chance", i+1);
      house_rules[i].chance = contents.get_int(buf, 0);

      DBG_DEBUG("stadt_t::init()", "house: %d, '%s'", house_rules[i].chance, house_rules[i].rule);
    }


    for(i=0; i<num_road_rules; i++) {
      sprintf(buf, "road_%d", i+1);
      const char * rule = contents.get_string(buf, "");
      tstrncpy(road_rules[i].rule, rule+1, 12);
      road_rules[i].rule[11] = '\0';

      sprintf(buf, "road_%d.chance", i+1);
      road_rules[i].chance = contents.get_int(buf, 0);

      DBG_DEBUG("stadt_t::init()", "road: %d, '%s'", road_rules[i].chance, road_rules[i].rule);
    }


  } else {
    dbg->fatal("stadt_t::init()", "Can't read '%s'", filename);
  }

  return ok;
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 * @see simwin
 */
char * stadt_t::info(char *buf) const
{

  buf += sprintf(buf, "%s: %d (+%d)\n\n",
     translator::translate("City size"),
     gib_einwohner(),
     gib_wachstum()
     );


  buf += sprintf(buf, "%d,%d - %d,%d\n\n",
     li, ob, re , un
     );


  buf += sprintf(buf, "%s: %d\n%s: %d\n\n",
     translator::translate("Unemployed"),
     bev-arb,
     translator::translate("Homeless"),
     bev-won);


  return buf;
}


/**
 * A short info about the city stats
 * @author Hj. Malthaner
 */
void stadt_t::get_short_info(cbuffer_t & buf) const
{
  buf.append(get_name());
  buf.append(": ");
  buf.append(gib_einwohner());
  buf.append(" (+");
  buf.append(gib_wachstum());
  buf.append(")\n");
}
