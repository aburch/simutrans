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
#include "boden/boden.h"
#include "simworld.h"
#include "simplay.h"
#include "simplan.h"
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
#include "gui/stadt_info.h"

#include "besch/haus_besch.h"

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

#include "tpl/slist_mit_gewichten_tpl.h"
#include "tpl/minivec_tpl.h"

#include "utils/cbuffer_t.h"
#include "utils/simstring.h"


/**
 * Zeitintervall für step
 * @author Hj. Malthaner
 */
const uint32 stadt_t::step_interval = 7000;

karte_t *stadt_t::welt = NULL;	// one is enough ...

/**
 * Number of house building rules
 * @author Hj. Malthaner
 */
static int num_house_rules = 0;

/**
 * Number of road building rules
 * @author Hj. Malthaner
 */
static int num_road_rules = 0;


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
    !gr->gib_weg(weg_t::wasser) &&     // Höchstens Strassen
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
        if(d.x>0  ||  d.y>0) {
        	if(welt->max_hgt(pos)!=welt->max_hgt(pos+d)) {
        		// height wrong!
        		return false;
        	}
        }

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
    !gr->gib_weg(weg_t::wasser) &&     // Höchstens Strassen
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


// this function adds houses to the city house list
void stadt_t::add_gebaeude_to_stadt(const gebaeude_t *gb)
{
	if(gb) {
		const haus_tile_besch_t *tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );
		koord k, pos = gb->gib_pos().gib_2d() - tile->gib_offset();

		// add all tiles
		for(k.y = 0; k.y < size.y; k.y ++) {
			for(k.x = 0; k.x < size.x; k.x ++) {
				gebaeude_t *add_gb = dynamic_cast<gebaeude_t *>(welt->lookup(pos+k)->gib_kartenboden()->obj_bei(0));
//				DBG_MESSAGE("stadt_t::add_gebaeude_to_stadt()","geb=%p at (%i,%i)",add_gb,pos.x+k.x,pos.y+k.y);
				buildings.append(add_gb,tile->gib_besch()->gib_level()+1,16);
				add_gb->setze_stadt(this);
			}
		}
	}
}



// this function removes houses from the city house list
void stadt_t::remove_gebaeude_from_stadt(const gebaeude_t *gb)
{
	if(gb) {
		const haus_tile_besch_t *tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );
		koord k, pos = gb->gib_pos().gib_2d() - tile->gib_offset();
//		DBG_MESSAGE("stadt_t::remove_gebaeude_from_stadt()","at (%i,%i)",pos.x,pos.y);

		// remove all tiles
		for(k.y = 0; k.y < size.y; k.y ++) {
			for(k.x = 0; k.x < size.x; k.x ++) {
				gebaeude_t *rem_ge= dynamic_cast<gebaeude_t *>(welt->lookup(pos+k)->gib_kartenboden()->obj_bei(0));
//				DBG_MESSAGE("stadt_t::remove_gebaeude_from_stadt()","geb=%p at (%i,%i)",gb,pos.x+k.x,pos.y+k.y);
				if(rem_ge==NULL) {
					dbg->error("stadt_t::remove_gebaeude_from_stadt()","Nothing on %i,%i",pos.x+k.x,pos.y+k.y);
				}
				buildings.remove(rem_ge);
				rem_ge->setze_stadt(NULL);
			}
		}
	}
}



// recalculate house informations (used for target selection)
void
stadt_t::recount_houses()
{
	buildings.clear();
	for( sint16 y=lo.y;  y<=ur.y;  y++  ) {
		for( sint16 x=lo.x;  x<=ur.x; x++  ) {
			grund_t *gr=welt->lookup(koord(x,y))->gib_kartenboden();
			gebaeude_t *gb=dynamic_cast<gebaeude_t *>(gr->obj_bei(0));
			if(gb  &&  (gb->ist_rathaus()  ||  gb->gib_haustyp()!=gebaeude_t::unbekannt)  && welt->suche_naechste_stadt(koord(x,y))==this) {
				// no attraction, just normal buidlings or townhall
				buildings.append(gb,gb->gib_tile()->gib_besch()->gib_level()+1,16);
				gb->setze_stadt(this);
			}
		}
	}
}




void
stadt_t::init_pax_ziele()
{
	const int gr_x = welt->gib_groesse_x();
	const int gr_y = welt->gib_groesse_y();

	for(int j=0; j<128; j++) {
		for(int i=0; i<128; i++) {
			const koord pos (i*gr_x/128, j*gr_y/128);
//DBG_MESSAGE("stadt_t::init_pax_ziele()","at %i,%i = (%i,%i)",i,j,pos.x,pos.y);
			pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i ,j) = reliefkarte_t::calc_relief_farbe(welt, pos);
			//      pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i ,j) = 0;
		}
	}
}



// fress some memory
stadt_t::~stadt_t()
{
	// close window if needed
	if(stadt_info) {
		destroy_win( stadt_info );
	}
	// remove city info and houses
	while(buildings.get_count()>0) {
		grund_t *gr=welt->lookup(buildings.at(0)->gib_pos());
		if(gr) {
			koord pos=buildings.at(0)->gib_pos().gib_2d();
			gr->obj_loesche_alle(NULL);
			welt->
			access(pos)->kartenboden_setzen(new boden_t(welt, koord3d(pos,welt->min_hgt(pos) ) ), false);
		}
		else {
			buildings.remove_at(0);
		}
	}
}



stadt_t::stadt_t(karte_t *wl, spieler_t *sp, koord pos,int citizens) :
	buildings(16),
    pax_ziele_alt(128, 128),
    pax_ziele_neu(128, 128),
    arbeiterziele(4)
{
	welt = wl;
	assert(wl->ist_in_kartengrenzen(pos));

	step_count = 0;
	next_step = 0;

	pax_erzeugt = 0;
	pax_transport = 0;

	besitzer_p = sp;

	this->pos = pos;

	bev = 0;
	arb = 0;
	won = 0;

	lo = ur = pos;

	zentrum_namen_cnt = 0;
	aussen_namen_cnt = 0;

DBG_MESSAGE("stadt_t::stadt_t()","Welt %p",welt);fflush(NULL);
	/* get a unique cityname */
	/* 9.1.2005, prissi */
	const weighted_vector_tpl<stadt_t *> *staedte=welt->gib_staedte();
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
    pax_transport = citizens;
    pax_erzeugt = 0;

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
	last_year_bev = city_history_year[0][HIST_CITICENS] = gib_einwohner();
	last_month_bev = city_history_month[0][HIST_CITICENS] = gib_einwohner();
	this_year_transported = 0;
	this_year_pax = 0;

	stadt_info = NULL;
}



stadt_t::stadt_t(karte_t *wl, loadsave_t *file) :
	buildings(16),
	pax_ziele_alt(128, 128),
	pax_ziele_neu(128, 128),
	arbeiterziele(4)
{
	welt = wl;
	step_count = 0;
	next_step = 0;

	pax_erzeugt = 0;
	pax_transport = 0;

	wachstum = 0;

	rdwr(file);

	verbinde_fabriken();

	stadt_info = NULL;
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
	uint32	lli=lo.x, lob=lo.y, lre=ur.x, lun=ur.y;
	file->rdwr_long(lli, " ");
	file->rdwr_long(lob, "\n");
	file->rdwr_delim("Pru: ");
	file->rdwr_long(lre, " ");
	file->rdwr_long(lun, "\n");
	lo.x=lli;	lo.y=lob;	ur.x=lre; ur.y=lun;
	file->rdwr_delim("Bes: ");
	file->rdwr_long(besitzer_n, "\n");
	file->rdwr_long(bev, " ");
	file->rdwr_long(arb, " ");
	file->rdwr_long(won, "\n");
	file->rdwr_long(zentrum_namen_cnt, " ");
	file->rdwr_long(aussen_namen_cnt, "\n");

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
		last_year_bev = city_history_year[0][HIST_CITICENS] = gib_einwohner();
		last_month_bev = city_history_month[0][HIST_CITICENS] = gib_einwohner();
		this_year_transported = 0;
		this_year_pax = 0;
	}
	else {
		// 86.00.0 introduced city history
		for (int year = 0;year<MAX_CITY_HISTORY_YEARS;year++) {
			for (int hist_type = 0; hist_type<4; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type], " ");
			}
		}
		for (int month=0; month<MAX_CITY_HISTORY_MONTHS; month++) {
			for (int hist_type=0; hist_type<4; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type], " ");
			}
		}
		// since we add it internally
		file->rdwr_long(last_year_bev, " ");
		file->rdwr_long(last_month_bev, " ");
		file->rdwr_long(this_year_transported, " ");
		file->rdwr_long(this_year_pax, "\n");
	}
/*
	// convert to new display
	if (file->get_version() < 88005) {
		last_year_bev *= 2;
		last_month_bev *= 2;
		for (int year = 0;year<MAX_CITY_HISTORY_YEARS;year++) {
			city_history_year[year][HIST_CITICENS] *= 2;
			city_history_year[year][HIST_GROWTH] *= 2;
		}
		for (int month=0; month<MAX_CITY_HISTORY_MONTHS; month++) {
			city_history_month[month][HIST_CITICENS] *= 2;
			city_history_month[month][HIST_GROWTH] *= 2;
		}
	}
*/
    // 08-Jan-03: Due to some bugs in the special buildings/town hall
    // placement code, li,re,ob,un could've gotten irregular values
    // If a game is loaded, the game might suffer from such an mistake
    // and we need to correct it here.
	if(lo.x < 0) lo.x = 0;
	if(ur.x >= welt->gib_groesse_x()-1) ur.x = welt->gib_groesse_x() - 1;

	if(lo.y < 0) lo.y = 0;
	if(ur.y >= welt->gib_groesse_y()-1) ur.y = welt->gib_groesse_y() - 1;
}




/**
 * Wird am Ende der Laderoutine aufgerufen, wenn die Welt geladen ist
 * und nur noch die Datenstrukturenneu verknüpft werden müssen.
 * @author Hj. Malthaner
 */
void stadt_t::laden_abschliessen()
{
	// very young city => need to grow again
	if(pax_transport>0  &&  pax_erzeugt==0) {
		step_bau();
		pax_transport = 0;
	}

	welt->lookup(pos)->gib_kartenboden()->setze_text( name );

	verbinde_fabriken();
	init_pax_ziele();
	recount_houses();

	// recount the size of a city
	lo = koord( welt->gib_groesse_x(), welt->gib_groesse_y() );
	ur = koord(0,0);
	for(  unsigned i=0;  i<buildings.get_count();  i++  ) {
		const koord pos=buildings.get(i)->gib_pos().gib_2d();
		if(lo.x>pos.x) lo.x=pos.x;
		if(lo.y>pos.y) lo.y=pos.y;
		if(ur.x<pos.x) ur.x=pos.x;
		if(ur.y<pos.y) ur.y=pos.y;
	}
	if(ur.x>=welt->gib_groesse_x()) {
		ur.x = welt->gib_groesse_x();
	}
	if(ur.y>=welt->gib_groesse_y()) {
		ur.y = welt->gib_groesse_y();
	}

	next_step = 0;
}



/* returns the money dialoge of a city
 * @author prissi
 */
stadt_info_t *
stadt_t::gib_stadt_info(void)
{
	if(stadt_info==NULL) {
		stadt_info = new stadt_info_t(this);
	}
	return stadt_info;
}



/* add workers to factory list */
void
stadt_t::add_factory_arbeiterziel(fabrik_t *fab)
{
	// no fish swarms ...
	if(strcmp("fish_swarm",fab->gib_besch()->gib_name())!=0) {
		const koord k = fab->gib_pos().gib_2d() - pos;

		// worker do not travel more than 50 tiles
		if(k.x*k.x + k.y*k.y < CONNECT_TO_TOWN_SQUARE_DISTANCE) {
DBG_MESSAGE("stadt_t::add_factory_arbeiterziel()","found %s with level %i", fab->gib_name(), fab->gib_besch()->gib_pax_level() );
			fab->add_arbeiterziel( this );
			// do not add a factory twice!
			arbeiterziele.append_unique( fab, fab->gib_besch()->gib_pax_level(), 4 );
		}
	}
}



/* calculates the factories which belongs to certain cities */
/* we connect all factories, which are closer than 50 tiles radius */
void
stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()","search factories near %s (center at %i,%i)",gib_name(), pos.x, pos.y  );

	slist_iterator_tpl<fabrik_t *> fab_iter (welt->gib_fab_list());
	arbeiterziele.clear();

	while(fab_iter.next()) {
		add_factory_arbeiterziel( fab_iter.get_current() );
	}
	DBG_MESSAGE("stadt_t::verbinde_fabriken()","is connected with %i factories (sum_weight=%i).",arbeiterziele.get_count(), arbeiterziele.get_sum_weight() );
}



/* change size of city
 * @author prissi */
void stadt_t::change_size( long delta_citicens )
{
	step_bau();
	pax_erzeugt = 0;
DBG_MESSAGE("stadt_t::change_size()","%i+%i",bev,delta_citicens);
	if(delta_citicens>0) {
		pax_transport = delta_citicens;
		step_bau();
		pax_transport = 0;
	}
	if(delta_citicens<0) {
		pax_transport = 0;
		if(bev>-delta_citicens) {
			bev += delta_citicens;
		}
		else {
//			remove_city();
			bev = 0;
		}
		step_bau();
	}
DBG_MESSAGE("stadt_t::change_size()","%i+%i",bev,delta_citicens);
}



void
stadt_t::step(long delta_t)
{
	// Ist es Zeit für einen neuen step?
	next_step += delta_t;
	if(stadt_t::step_interval < next_step) {
		// Alle 7 sekunden ein step (or we need to skip very much ... )
		next_step -= stadt_t::step_interval;

		// Zaehlt steps seit instanziierung
		step_count ++;
		step_passagiere();

		if( (step_count & 3) == 0) {
			step_bau();
		}

		// update history
		city_history_month[0][HIST_CITICENS] = gib_einwohner();	// total number
		city_history_year[0][HIST_CITICENS] = gib_einwohner();

		city_history_month[0][HIST_GROWTH] = gib_einwohner()-last_month_bev;	// growth
		city_history_year[0][HIST_GROWTH] = gib_einwohner()-last_year_bev;

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
	city_history_month[0][HIST_CITICENS] = gib_einwohner();	// total number
	city_history_year[0][HIST_CITICENS] = gib_einwohner();

	city_history_month[0][HIST_GROWTH] = gib_einwohner()-last_month_bev;	// growth
	city_history_year[0][HIST_GROWTH] = gib_einwohner()-last_year_bev;

	city_history_month[0][HIST_TRANSPORTED] = pax_transport;	// pax transported
	this_year_transported += pax_transport;
	city_history_year[0][HIST_TRANSPORTED] = this_year_transported;

	city_history_month[0][HIST_GENERATED] = pax_erzeugt;	// and all post and passengers generated
	this_year_pax += pax_erzeugt;
	city_history_year[0][HIST_GENERATED] = this_year_pax;

	// init differences
	last_month_bev = gib_einwohner();

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
	city_history_month[0][0] = gib_einwohner();

	//need to roll year too?
	if(   welt->get_last_month()==0  ) {
		for (i=MAX_CITY_HISTORY_YEARS-1; i>0; i--) {
			for (int hist_type = 0; hist_type<MAX_CITY_HISTORY; hist_type++) {
				city_history_year[i][hist_type] = city_history_year[i-1][hist_type];
			}
		}
		// init this year
		for (int hist_type = 1; hist_type<MAX_CITY_HISTORY; hist_type++) {
			city_history_year[0][hist_type] = 0;
		}
		last_year_bev = gib_einwohner();
		city_history_year[0][HIST_CITICENS] = gib_einwohner();
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
	const int gr_x = welt->gib_groesse_x();
	const int gr_y = welt->gib_groesse_y();

	pax_ziele_alt.copy_from(pax_ziele_neu);
	for(int j=0; j<128; j++) {
		for(int i=0; i<128; i++) {
			const koord pos (i*gr_x/128, j*gr_y/128);

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

	// prissi: growth now size dependent
	if(pax_erzeugt==0) {
		wachstum = pax_transport;
	} else if(bev<1000) {
		wachstum = (pax_transport *2) / (pax_erzeugt+1);
	} else if(bev<10000) {
		wachstum = (pax_transport *4) / (pax_erzeugt+1);
	}
	else {
		wachstum = (pax_transport *8) / (pax_erzeugt+1);
	}

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



/* this creates passengers and mail for everything is is therefore one of the CPU hogs of the machine
 * think trice, before applying optimisation here ...
 */
void
stadt_t::step_passagiere()
{
//	DBG_MESSAGE("stadt_t::step_passagiere()","%s step_passagiere called (%d,%d - %d,%d)\n", name, li,ob,re,un);
//	long t0 = get_current_time_millis();

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
	for(k.y = lo.y+row; k.y <= ur.y; k.y += 8) {
		for(k.x = lo.x+col; k.x <= ur.x; k.x += 2) {
			const gebaeude_t *gb = dynamic_cast<const gebaeude_t *> (welt->lookup(k)->gib_kartenboden()->obj_bei(0));

			// is there a house (we do not check ownership here for overlapping towns;
			// will generate more passengers, but should not be problematic
			// but we will skip attarctions, since the generate return passengers automatically
			if(gb!=NULL  &&  (gb->gib_haustyp()!=gebaeude_t::unbekannt  ||  gb->ist_rathaus()  ||  gb->is_monument())) {

				// prissi: since now backtravels occur, we damp the numbers a little
				const int num_pax =
					(wtyp == warenbauer_t::passagiere) ?
					(gb->gib_tile()->gib_besch()->gib_level() + 6) >> 2 :
					(gb->gib_tile()->gib_besch()->gib_post_level() + 5) >> 2;

				// create pedestrians?
				if(umgebung_t::fussgaenger) {
					haltestelle_t::erzeuge_fussgaenger(welt, welt->lookup(k)->gib_kartenboden()->gib_pos(), (num_pax>>3)+1);
					INT_CHECK("simcity 269");
				}

				// suitable start search
				const minivec_tpl<halthandle_t> &halt_list = welt->access(k)->get_haltlist();

				// only continue, if this is a good start halt
				if(halt_list.get_count()>0) {

					// Find passenger destination
					for(int pax_routed=0;  pax_routed<num_pax;  pax_routed+=7) {

						// just the defualt "No route" stop
						halthandle_t start_halt = halt_list.at(0);

						// number of passengers that want to travel
						// Hajo: for efficiency we try to route not every
						// single pax, but packets. If possible, we do 7 passengers at a time
						// the last packet might have less then 7 pax
						int pax_left_to_do = MIN(7, num_pax - pax_routed);

						// Hajo: track number of generated passengers.
						// prissi: we do it inside the loop to take also care of non-routable passengers
						pax_erzeugt += pax_left_to_do;

						// Ziel für Passagier suchen
						pax_zieltyp will_return;
						const koord ziel = finde_passagier_ziel(&will_return);

#ifdef DESTINATION_CITYCARS
						//citycars with destination
						if(pax_routed==0) {
							erzeuge_verkehrsteilnehmer( start_halt->gib_basis_pos(), step_count, ziel );
						}
#endif
						// Dario: Check if there's a stop near destination
						const minivec_tpl <halthandle_t> &dest_list = welt->access(ziel)->get_haltlist();

						// suitable end search
						unsigned ziel_count=0;
						for( unsigned h=0;  h<dest_list.get_count();  h++ ) {
							halthandle_t halt = dest_list.at(h);
							if(halt->is_enabled(wtyp)) {
								ziel_count ++;
								break;	// because we found at least one valid step ...
							}
						}

						if(ziel_count==0){
// DBG_MESSAGE("stadt_t::step_passagiere()", "No stop near dest (%d, %d)", ziel.x, ziel.y);
							// Thus, routing is not possible and we do not need to do a calculation.
							// Mark ziel as destination without route and continue.
							merke_passagier_ziel(ziel, COL_DARK_ORANGE);
							start_halt->add_pax_no_route(pax_left_to_do);
#ifdef DESTINATION_CITYCARS
							//citycars with destination
							erzeuge_verkehrsteilnehmer( start_halt->gib_basis_pos(), step_count, ziel );
#endif
							continue;
						}

						// check, if they can walk there?
						if(!dest_list.is_contained(start_halt)) {

							// ok, they are not in walking distance
							ware_t pax (wtyp);
							pax.setze_zielpos( ziel );
							pax.menge = (wtyp==warenbauer_t::passagiere)?pax_left_to_do:MAX(1,pax_left_to_do>>2);

							// prissi: 11-Mar-2005
							// we try all stations to find one not overcrowded
							unsigned halte;
							bool overcrowded=true;
							for( halte=0;  halte<halt_list.get_count();  halte++  ) {
								start_halt = halt_list.get(halte);
								if(start_halt->gib_ware_summe(wtyp) <= start_halt->get_capacity()) {
									// prissi: not overcrowded so now try routing
									overcrowded = false;
									break;
								}
								else {
									// Hajo: Station crowded:
									// they are appalled but will try other stations
									start_halt->add_pax_unhappy(pax.menge);	// since mail can be less than pax_left_to_do
									start_halt = halthandle_t();
								}
							}

							// all crowded?
							if(!start_halt.is_bound()) {
								// so we cannot go there => mark it
								merke_passagier_ziel(ziel, COL_DARK_ORANGE);
								continue;
							}

							// now, finally search a route; this consumes most of the time
							koord return_zwischenziel=koord::invalid;	// for people going back ...
							if(!overcrowded  ||  will_return) {
								start_halt->suche_route(pax, will_return?&return_zwischenziel:NULL );
							}
							if(!overcrowded) {
								if(pax.gib_ziel() != koord::invalid) {
									// so we have happy traveling passengers
									start_halt->starte_mit_route(pax);
									start_halt->add_pax_happy(pax.menge);
									// and show it
									merke_passagier_ziel(ziel, COL_YELLOW);
									pax_transport += pax.menge;
								}
								else {
									start_halt->add_pax_no_route(pax_left_to_do);
									merke_passagier_ziel(ziel, COL_DARK_ORANGE);
#ifdef DESTINATION_CITYCARS
									//citycars with destination
									erzeuge_verkehrsteilnehmer( start_halt->gib_basis_pos(), step_count, ziel );
#endif
								}
							}

							// send them also back
							if(will_return!=no_return  &&  pax.gib_ziel() != koord::invalid) {

								// this comes most of the times for free and balances also the amounts!
								halthandle_t ret_halt = welt->lookup(pax.gib_ziel())->gib_halt();
								if(will_return!=town_return) {
									// restore normal mail amount => more mail from attractions and factories than going to them
									pax.menge = pax_left_to_do;
								}

								// we just have to ensure, the ware can be delivered at this station
								warenziel_t wz(start_halt->gib_basis_pos(),wtyp);
								bool found = false;
								for(  unsigned i=0;  i<halt_list.get_count();  i++  ) {
									halthandle_t test_halt=halt_list.at(i);
									if(test_halt->is_enabled(wtyp)  &&  (start_halt==test_halt  ||  test_halt->gib_warenziele()->contains(wz))  ) {
										found = true;
										start_halt = test_halt;
										break;
									}
								}

								// now try to add them to the target halt
								if(ret_halt->gib_ware_summe(wtyp)<=ret_halt->get_capacity()) {
									// prissi: not overcrowded and can recieve => add them
									if(found) {
										ware_t return_pax (wtyp);

										return_pax.menge = pax_left_to_do;
										return_pax.setze_zielpos( k );
										return_pax.setze_ziel( start_halt->gib_basis_pos() );
										return_pax.setze_zwischenziel( return_zwischenziel );

										ret_halt->starte_mit_route(return_pax);
										ret_halt->add_pax_happy(pax_left_to_do);
									}
									else {
										// no route back
										ret_halt->add_pax_no_route(pax_left_to_do);
									}
								}
								else {
									// Hajo: Station crowded:
									ret_halt->add_pax_unhappy(pax_left_to_do);
								}
							}
						}
						else {
							// ok, they find their destination here too
							// (this works even for mail on stations, which does not accept mail (but is much faster than checking)
							// so we declare them happy
							start_halt->add_pax_happy(pax_left_to_do);
							// and show it
							merke_passagier_ziel(ziel, COL_YELLOW);
							pax_transport += pax_left_to_do;
						}

					} // while
				}
				else {
					// no halt to start
					// fake one ride to get a proper display of destinations (although there may be more) ...
					pax_zieltyp will_return;
					const koord ziel = finde_passagier_ziel(&will_return);
#ifdef DESTINATION_CITYCARS
					//citycars with destination
					erzeuge_verkehrsteilnehmer( k, step_count, ziel );
#endif
					merke_passagier_ziel(ziel, COL_DARK_ORANGE);
					pax_erzeugt += num_pax;
					// check destination stop to add no route
					const minivec_tpl <halthandle_t> &dest_no_src_list = welt->access(ziel)->get_haltlist();
					for( unsigned halte=0;  halte<dest_no_src_list.get_count();  halte++  ) {
						halthandle_t halt = dest_no_src_list.get(halte);
						if(halt->is_enabled(wtyp)) {
							// add no route back
							halt->add_pax_no_route(num_pax);
						}
					}

				}

			}
		}
		INT_CHECK("simcity 525");
	}

//	long t1 = get_current_time_millis();
//	DBG_MESSAGE("stadt_t::step_passagiere()","Zeit für Passagierstep: %ld ms\n", (long)(t1-t0));
}



/**
 * gibt einen zufällingen gleichverteilten Punkt innerhalb der
 * Stadtgrenzen zurück
 * @author Hj. Malthaner
 */
koord
stadt_t::gib_zufallspunkt()
{
	const gebaeude_t *gb=buildings.at_weight( simrand(buildings.get_sum_weight()) );
	koord k=gb->gib_pos().gib_2d();
	if(!welt->ist_in_kartengrenzen(k)) {
		// this building should not be in this list, since it has been already deleted!
		dbg->error("stadt_t::gib_zufallspunkt()","illegal building %s removing!",gb->gib_name());
		buildings.remove(gb);
		k = koord(0,0);
	}
	return k;
}



/* this function generates a random target for passenger/mail
 * changing this strongly affects selection of targets and thus game strategy
 */
koord
stadt_t::finde_passagier_ziel(pax_zieltyp *will_return)
{
	const int rand = simrand(100);

	// about 1/3 are workers
	if(rand<FACTORY_PAX  &&  arbeiterziele.get_sum_weight()>0) {
		const fabrik_t *fab = arbeiterziele.at_weight( simrand(arbeiterziele.get_sum_weight()) );
		*will_return = factoy_return;	// worker will return
		return fab->gib_pos().gib_2d();
	}
	else if(rand<TOURIST_PAX+FACTORY_PAX  &&  welt->gib_ausflugsziele().get_sum_weight()>0) {
		*will_return = tourist_return;	// tourists will return
		const gebaeude_t *gb = welt->gib_random_ausflugsziel();
		return gb->gib_pos().gib_2d();
	}
	else{
		// if we reach here, at least a single town existes ...
		const stadt_t *zielstadt = welt->get_random_stadt();

		// we like nearer towns more
		if(ABS(zielstadt->pos.x - pos.x)+ABS(zielstadt->pos.y - pos.y)>120  ) {
			// retry once ...
			zielstadt = welt->get_random_stadt();
		}

		// long distance traveller? => then we return
		*will_return = (this!=zielstadt) ? town_return : no_return;
		return ((stadt_t *)zielstadt)->gib_zufallspunkt();
	}
	// we could never reach here (but happend once anyway?!?)
	dbg->fatal("stadt_t::finde_passagier_ziel()","no passenger ziel found!");
	assert(0);
	return koord::invalid;
}



void
stadt_t::merke_passagier_ziel(koord k, int color)
{
    const koord p = koord( ((k.x*127)/welt->gib_groesse_x())&127, ((k.y*128)/welt->gib_groesse_y())&127 );
    pax_ziele_neu.at(p) = color;
}



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
		const weighted_vector_tpl<gebaeude_t *> & attractions = welt->gib_ausflugsziele();
		int dist = welt->gib_groesse_x()*welt->gib_groesse_y();
		for(  unsigned i=0;  i<attractions.get_count();  i++  ) {
			int d = koord_distance(attractions.at(i)->gib_pos(),pos);
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
			// try to built a little away from previous ones
			if(find_dist_next_special(pos)<b+h+1) {
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
	const haus_besch_t *besch = hausbauer_t::gib_special(bev,welt->get_timeline_year_month());
	if(besch) {
		if(simrand(100) < (unsigned)besch->gib_chance()) {
			// baue was immer es ist
			int rotate = 0;
			bool is_rotate = besch->gib_all_layouts()>10;
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos,  besch->gib_b(), besch->gib_h(),&is_rotate);

			if(best_pos != koord::invalid) {
				// then built it
				if(besch->gib_all_layouts()>1) {
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
		// errect a monoment
		//
		besch = hausbauer_t::waehle_denkmal(welt->get_timeline_year_month());
		if(besch) {
			koord total_size = koord( 2+besch->gib_b(), 2+besch->gib_h() );
			koord best_pos ( denkmal_platz_sucher_t(welt,besitzer_p).suche_platz(pos,total_size.x,total_size.y ) );

			if(best_pos != koord::invalid) {
				int i;
				bool ok = false;

				// Wir bauen das Denkmal nur, wenn schon mindestens eine Strasse da ist
				for(i = 0; i < total_size.x && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(i, -1))->gib_kartenboden()->gib_weg(weg_t::strasse) ||
						welt->access(best_pos + koord(i, total_size.y))->gib_kartenboden()->gib_weg(weg_t::strasse);
				}
				for(i = 0; i < total_size.y && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(total_size.x, i))->gib_kartenboden()->gib_weg(weg_t::strasse) ||
						welt->access(best_pos + koord(-1, i))->gib_kartenboden()->gib_weg(weg_t::strasse);
				}
				if(ok) {
					// Straßenkreis um das Denkmal legen
					for(i=0; i<total_size.y; i++) {
						baue_strasse(best_pos + koord(0,i), NULL, true);
						baue_strasse(best_pos + koord(total_size.x-1,i), NULL, true);
					}
					for(i=0; i<total_size.x; i++) {
						baue_strasse(best_pos + koord(i,0), NULL, true);
						baue_strasse(best_pos + koord(i,total_size.y-1), NULL, true);
					}
					// and then built it
					hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos + koord(1, 1))->gib_kartenboden()->gib_pos(), 0, besch);
					hausbauer_t::denkmal_gebaut(besch);
					add_gebaeude_to_stadt( dynamic_cast<const gebaeude_t *>(welt->lookup(best_pos+koord(1,1))->gib_kartenboden()->obj_bei(0)) );
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
	const haus_besch_t *besch = hausbauer_t::gib_rathaus(bev,welt->get_timeline_year_month());
	if(besch) {
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		gebaeude_t *gb = dynamic_cast<gebaeude_t *>(gr->obj_bei(0));
		bool neugruendung = !gb || !gb->ist_rathaus();
		bool umziehen = !neugruendung;
		koord alte_str ( koord::invalid );
		koord best_pos ( pos );
		koord k;

		DBG_MESSAGE("check_bau_rathaus()","bev=%d, new=%d", bev, neugruendung);

		if(!neugruendung) {
			if(gb->gib_tile()->gib_besch()->gib_level() == besch->gib_level())  {
				DBG_MESSAGE("check_bau_rathaus()","town hall already ok.");
				return; // Rathaus ist schon okay
			}

			const haus_besch_t *besch_alt = gb->gib_tile()->gib_besch();
			koord pos_alt = gr->gib_pos().gib_2d() - gb->gib_tile()->gib_offset();
			koord groesse_alt = besch_alt->gib_groesse(gb->gib_tile()->gib_layout());

			// do we need to move
			if(besch->gib_b()<=groesse_alt.x  &&  besch->gib_h()<=groesse_alt.y) {
				// no, the size is ok
				umziehen = false;
			}
			else if(welt->lookup(pos + koord(0, besch_alt->gib_h()))->gib_kartenboden()->gib_weg(weg_t::strasse)) {
				// we need to built a new road, thus we will use the old as a starting point (if found)
				alte_str =  pos + koord(0, besch_alt->gib_h());
			}

			// clear the old townhall
			gr->setze_text(NULL);
			for(k.x=0; k.x<groesse_alt.x; k.x++) {
				for(k.y=0; k.y<groesse_alt.y; k.y++) {
					gr = welt->lookup(pos_alt + k)->gib_kartenboden();
					gr->setze_besitzer(NULL);	// everyone ground ...
					gb = dynamic_cast<gebaeude_t *>(gr->obj_bei(0));
					if(gb) {
						gb->setze_besitzer(NULL);
DBG_MESSAGE("stadt_t::check_bau_rathaus()","deleted townhall (bev=%i)",buildings.get_sum_weight());
					}
					gr->obj_loesche_alle(NULL);
					if(umziehen) {
DBG_MESSAGE("stadt_t::check_bau_rathaus()","delete townhall tile %i,%i (gb=%p)",k.x,k.y,gb);
						// replace old space by normal houses level 0 (must be 1x1!)
						gb = hausbauer_t::neues_gebaeude(welt, NULL, gr->gib_pos(), 0, hausbauer_t::gib_wohnhaus(0,welt->get_timeline_year_month()), NULL );
						add_gebaeude_to_stadt( gb );
					}
				}
			}
		}

		//
		// Now built the new townhall
		//
		int layout = simrand(besch->gib_all_layouts()-1);
		if(neugruendung || umziehen) {
			best_pos = rathausplatz_sucher_t(welt, besitzer_p).suche_platz(pos, besch->gib_b(layout), besch->gib_h(layout)+1);
		}
		hausbauer_t::baue(welt, besitzer_p,welt->lookup(best_pos)->gib_kartenboden()->gib_pos(), layout, besch);
DBG_MESSAGE("new townhall","use layout=%i",layout);
		add_gebaeude_to_stadt( dynamic_cast<const gebaeude_t *>(welt->lookup(best_pos)->gib_kartenboden()->obj_bei(0)) );
DBG_MESSAGE("stadt_t::check_bau_rathaus()","add townhall (bev=%i)",buildings.get_sum_weight());

		// Orstnamen hinpflanzen
		welt->lookup(best_pos)->gib_kartenboden()->setze_text(name);

		// tell the player, if not during initialization
		if(!new_town) {
			char buf[256];
			sprintf(buf, translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), gib_name(), gib_einwohner() );
			message_t::get_instance()->add_message(buf,pos,message_t::city,CITY_KI,besch->gib_tile(layout,0,0)->gib_hintergrund(0,0) );
		}

		// Strasse davor verlegen
		// Hajo: nur, wenn der Boden noch niemand gehört!
		k = koord(0, besch->gib_h(layout));
		for(k.x = 0; k.x < besch->gib_b(layout); k.x++) {
			grund_t * gr = welt->lookup(best_pos+k)->gib_kartenboden();
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
			bauer.route_fuer(wegbauer_t::strasse, welt->get_city_road() );
			bauer.calc_route(alte_str, best_pos + koord(0, besch->gib_h(layout)));
			bauer.baue();
			pruefe_grenzen(best_pos);
		}
		else if(neugruendung) {
			lo = best_pos - koord(2,2);
			ur = best_pos + koord(besch->gib_b(layout),besch->gib_h(layout)) + koord(2,2);
		}
		pos = best_pos;
	}

	// Hajo: paranoia - ensure correct bounds in all cases
	//       I don't know if best_pos is always valid
	if(lo.x < 0) lo.x = 0;
	if(ur.x >= welt->gib_groesse_x()-1) ur.x = welt->gib_groesse_x() - 1;

	if(lo.y < 0) lo.y = 0;
	if(ur.y >= welt->gib_groesse_y()-1) ur.y = welt->gib_groesse_y() - 1;
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
				if(market!=NULL) {
					bool	rotate=false;

					koord3d	market_pos=welt->lookup(pos)->gib_kartenboden()->gib_pos();
	DBG_MESSAGE("stadt_t::check_bau_factory","adding new industry at %i inhabitants.",gib_einwohner() );
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
 * T = not a stop	// added in 88.03.3
 * t = is a stop // added in 88.03.3
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
						ok = gr->gib_weg(weg_t::strasse);
						break;
					case 'S':
						ok = !gr->gib_weg(weg_t::strasse);
						break;
					case 'h':
						// is house
						ok = gr->gib_typ()==grund_t::fundament;
						break;
					case 'H':
						// no house
						ok = gr->gib_typ()!=grund_t::fundament;
						break;
					case 'n':
						ok = gr->ist_natur() && gr->kann_alle_obj_entfernen(NULL)==NULL;
						break;
					case 't':
						// here is a stop/extension building
						ok = gr->gib_halt().is_bound();
						break;
					case 'T':
						// no stop
						ok = !gr->gib_halt().is_bound();
						break;
				}
			}
			else {
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
    const koord k (lo + koord( simrand(ur.x-lo.x+1),simrand(ur.y-lo.y+1)) );

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
		const gebaeude_t *gb = dynamic_cast<const gebaeude_t *>(plan->gib_kartenboden()->obj_bei(0));
		if(gb) {
			t = gb->gib_haustyp();
		}
	}
//	DBG_MESSAGE("stadt_t::was_ist_an()","an pos %d,%d ist ein gebaeude vom typ %d", k.x, k.y, t);

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


// return the eight neighbours
static koord neighbours[8]=
{
	koord(-1,-1), 	koord(-1,0), 	koord(-1,1),
	koord(0,-1), 	koord(0,1),
	koord(1,-1), 	koord(1,0), 	koord(1,1)
};

void
stadt_t::baue_gebaeude(const koord k)
{
	grund_t *gr=welt->lookup(k)->gib_kartenboden();
	const koord3d pos ( gr->gib_pos() );

	if(gr->ist_natur()  &&  gr->kann_alle_obj_entfernen(welt->gib_spieler(1))==NULL  &&  welt->lookup(koord3d(k,welt->max_hgt(k)))!=NULL) {

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

  		const uint16 current_month = welt->get_timeline_year_month();

		if(sum_gewerbe > sum_industrie && sum_gewerbe > sum_wohnung) {
			const haus_besch_t *h= hausbauer_t::gib_gewerbe(0,current_month);
			if(h) {
				hausbauer_t::baue(welt, NULL, pos, 0, h );
				arb += 20;
			}
		}

		if(sum_industrie > sum_gewerbe && sum_industrie > sum_wohnung) {
			const haus_besch_t *h= hausbauer_t::gib_industrie(0,current_month);
			if(h) {
				hausbauer_t::baue(welt, NULL, pos, 0, h );
				arb += 20;
			}
		}

		if(sum_wohnung > sum_industrie && sum_wohnung > sum_gewerbe) {
			const haus_besch_t *h= hausbauer_t::gib_wohnhaus(0,current_month);
			hausbauer_t::baue(welt, NULL, pos, 0, h );
			if(h) {
				hausbauer_t::baue(welt, NULL, pos, 0, h );
				won += 10;
			}
		}

		const gebaeude_t *gb = dynamic_cast<const gebaeude_t *>(welt->lookup(k)->gib_kartenboden()->obj_bei(0));
		add_gebaeude_to_stadt( gb );

		// check for pavement
		for(  int i=0;  i<8;  i++ ) {
			grund_t *gr = welt->lookup(k+neighbours[i])->gib_kartenboden();
			if(gr  &&  !gr->ist_bruecke()  &&  !gr->ist_tunnel()) {
				strasse_t *weg = dynamic_cast <strasse_t *>(gr->gib_weg(weg_t::strasse));
				if(weg) {
					weg->setze_gehweg(true);
					// if not current city road standard, then replace it
					if(weg->gib_besch()!=welt->get_city_road()) {
						if(gr->gib_besitzer()!=NULL  &&  !gr->gib_depot()  &&  !gr->gib_halt().is_bound()) {
							gr->gib_besitzer()->add_maintenance(-weg->gib_besch()->gib_wartung());
							gr->setze_besitzer( NULL );	// make public
						}
						weg->setze_besch( welt->get_city_road() );
					}
					gr->calc_bild();
				}
			}
		}
	}
}


void
stadt_t::erzeuge_verkehrsteilnehmer(koord pos, int level,koord target)
{
	const int verkehr_level = welt->gib_einstellungen()->gib_verkehr_level();
	if(verkehr_level>0  && level%(17-verkehr_level)==0) {

		koord k;
		for(k.y = pos.y-1; k.y<=pos.y+1; k.y ++) {
			for(k.x = pos.x-1; k.x<=pos.x+1; k.x ++) {
				if(welt->ist_in_kartengrenzen(k)) {
					grund_t *gr = welt->lookup(k)->gib_kartenboden();
					const weg_t *weg = gr->gib_weg(weg_t::strasse);

					if(weg &&
					   (gr->gib_weg_ribi_unmasked(weg_t::strasse) == ribi_t::nordsued ||
					   gr->gib_weg_ribi_unmasked(weg_t::strasse) == ribi_t::ostwest)) {

						if(stadtauto_t::gib_anzahl_besch() > 0) {
							stadtauto_t *vt = new stadtauto_t(welt, gr->gib_pos(),target);
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

	gebaeude_t *gb = dynamic_cast<gebaeude_t *>(welt->lookup(k)->gib_kartenboden()->obj_bei(0));
	if(gb == NULL) {
		dbg->error("stadt_t::renoviere_gebaeude()","could not find a building at (%d,%d) but there should be one!", k.x, k.y);
		return;
	}

	if(gb->get_stadt()!=this) {
		return; // not our town ...
	}

	// hier sind wir sicher dass es ein Gebaeude ist
	const int level = gb->gib_tile()->gib_besch()->gib_level();

	// bisher gibt es 2 Sorten Haeuser
	// arbeit-spendende und wohnung-spendende
	const int will_arbeit = (bev - arb) / 4;  // Nur ein viertel arbeitet
	const int will_wohnung = (bev - won);

	// does the timeline allow this buildings?
	const uint16 current_month = welt->get_timeline_year_month();
	const bool can_built_industrie = hausbauer_t::gib_industrie(0,current_month)!=NULL;
	const bool can_built_gewerbe = hausbauer_t::gib_gewerbe(0,current_month)!=NULL;
//	const bool can_built_wohnhaus = hausbauer_t::gib_wohnhaus(0,current_month)!=NULL;

	// der Bauplatz muss bewertet werden
	const int passt_industrie = bewerte_industrie(k);
	const int passt_gewerbe = bewerte_gewerbe(k);
	const int passt_wohnung = bewerte_wohnung(k);

	// verlust durch abriss
	const int sum_gewerbe = passt_gewerbe + will_arbeit;
	const int sum_industrie = passt_industrie + will_arbeit;
	const int sum_wohnung = passt_wohnung + will_wohnung;

	gebaeude_t::typ will_haben = gebaeude_t::unbekannt;
	int sum = 0;

	// try to built
	const haus_besch_t *h=0;
	if(can_built_gewerbe  &&  sum_gewerbe>sum_industrie  &&  sum_gewerbe>sum_wohnung) {
		// we must check, if we can really update to higher level ...
		const int try_level=(alt_typ==gebaeude_t::gewerbe) ? level+1 : level;
		h = hausbauer_t::gib_gewerbe(try_level,current_month);
		if(h!=NULL  &&  h->gib_level()>=try_level) {
			will_haben = gebaeude_t::gewerbe;
			sum = sum_gewerbe;
		}
	}
	// check for industry, also if we wanted com, but there was no com good enough ...
	if(can_built_industrie  &&  sum_industrie>sum_industrie  &&  sum_industrie>sum_wohnung  ||  (sum_gewerbe>sum_wohnung  &&  will_haben==gebaeude_t::unbekannt)) {
		// we must check, if we can really update to higher level ...
		const int try_level=(alt_typ==gebaeude_t::industrie) ? level+1 : level;
		h = hausbauer_t::gib_industrie(try_level,current_month);
		if(h!=NULL  &&  h->gib_level()>=try_level) {
			will_haben = gebaeude_t::industrie;
			sum = sum_industrie;
		}
	}
	// check for residence
	// (sum_wohnung>sum_industrie  &&  sum_wohnung>sum_gewerbe
	if(will_haben==gebaeude_t::unbekannt) {
		// we must check, if we can really update to higher level ...
		const int try_level=(alt_typ==gebaeude_t::wohnung) ? level+1 : level;
		h = hausbauer_t::gib_wohnhaus(try_level,current_month);
		if(h!=NULL  &&  h->gib_level()>=try_level) {
			will_haben = gebaeude_t::wohnung;
			sum = sum_wohnung;
		}
		else {
			h = NULL;
		}
	}

	if(alt_typ != will_haben) {
		sum -= level * 10;
	}

	// good enough to renovate, and we found a building?
	if(sum>0  &&  h!=NULL) {
//    DBG_MESSAGE("stadt_t::renoviere_gebaeude()","renovation at %i,%i (%i level) of typ %i to typ %i with desire %i",k.x,k.y,alt_typ,will_haben,sum);
		remove_gebaeude_from_stadt(gb);

		if(will_haben == gebaeude_t::wohnung) {
			hausbauer_t::umbauen(welt, gb, h);
			won += h->gib_level() * 10;
		} else if(will_haben == gebaeude_t::gewerbe) {
			hausbauer_t::umbauen(welt, gb, h);
			arb += h->gib_level() * 20;
		} else if(will_haben == gebaeude_t::industrie) {
			hausbauer_t::umbauen(welt, gb, h);
			arb += h->gib_level() * 20;
		}

		add_gebaeude_to_stadt( dynamic_cast<const gebaeude_t *>(welt->lookup(k)->gib_kartenboden()->obj_bei(0)) );

		if(alt_typ==gebaeude_t::industrie)
			arb -= level * 20;
		if(alt_typ==gebaeude_t::gewerbe)
			arb -= level * 20;
		if(alt_typ==gebaeude_t::wohnung)
			won -= level * 10;

		// printf("Renovierung mit %d Industrie, %d Gewerbe, %d  Wohnung\n", sum_industrie, sum_gewerbe, sum_wohnung);

		erzeuge_verkehrsteilnehmer(k, h->gib_level(), koord::invalid );

		// check for pavement
		for(  int i=0;  i<8;  i++ ) {
			grund_t *gr = welt->lookup(k+neighbours[i])->gib_kartenboden();
			if(gr  &&  !gr->ist_bruecke()  &&  !gr->ist_tunnel()) {
				strasse_t *weg = dynamic_cast <strasse_t *>(gr->gib_weg(weg_t::strasse));
				if(weg) {
					weg->setze_gehweg(true);
					// if not current city road standard, then replace it
					if(weg->gib_besch()!=welt->get_city_road()) {
						if(gr->gib_besitzer()!=NULL  &&  !gr->gib_depot()  &&  !gr->gib_halt().is_bound()) {
							gr->gib_besitzer()->add_maintenance(-weg->gib_besch()->gib_wartung());
							gr->setze_besitzer( NULL );	// make public
						}
						weg->setze_besch( welt->get_city_road() );
					}
					gr->calc_bild();
				}
			}
		}
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

	// water?!?
	if(bd->gib_hoehe() <= welt->gib_grundwasser()) {
		return false;
	}

	const hang_t::typ typ = bd->gib_grund_hang();
	if( hang_t::ist_wegbar(typ) &&  bd->kann_alle_obj_entfernen(welt->gib_spieler(1))==NULL) {

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
				if(bd2  &&  !bd2->ist_tunnel()  &&   !bd2->ist_bruecke()  &&  bd2->gib_depot()==NULL) {
					if(bd2->gib_halt().is_bound()) {
						// check type of stop:
						const gebaeude_t *gb = dynamic_cast<const gebaeude_t *>(bd2->suche_obj(ding_t::gebaeude));
						if(gb  &&  (gb->gib_tile()->gib_besch()->gib_all_layouts()!=2  ||  (gb->gib_tile()->gib_layout()&1)!=(r>>1))) {
							// four corner stop or not parallel
							return false;
						}
					}
					weg_t *weg2 = bd2->gib_weg(weg_t::strasse);
					if(weg2) {
						weg2->ribi_add(ribi_t::rueckwaerts(ribi_t::nsow[r]));
						bd2->calc_bild();
						ribi |= ribi_t::nsow[r];
					}
				}
			}
		}

		if(ribi != ribi_t::keine || forced) {

			if(!bd->weg_erweitern(weg_t::strasse, ribi)) {
				strasse_t *weg = new strasse_t(welt);
				weg->setze_gehweg( true );
				weg->setze_besch(welt->get_city_road());
				// Hajo: city roads should not belong to any player
				bd->neuen_weg_bauen(weg, ribi, sp);
			}
			else {
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
	if(k.x<=lo.x  &&  k.x>0) {
		lo.x = k.x-1;
	}
	if(k.y<=lo.y  &&  k.y>0) {
		lo.y = k.y-1;
	}

	if(k.x>=ur.x  &&  k.x<welt->gib_groesse_x()-2) {
		ur.x = k.x+1;
	}
	if(k.y>=ur.y  &&  k.y<welt->gib_groesse_y()-2) {
		ur.y = k.y+1;
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
    const char *base = "1center";	// usually gets to %s %s
    const char *building=NULL;

	// prissi: first we try a factory name
	halthandle_t halt=haltestelle_t::gib_halt(welt,k);
	if(halt.is_bound()) {
		slist_iterator_tpl <fabrik_t *> fab_iter(halt->gib_fab_list());
		while( fab_iter.next() ) {
			building = fab_iter.get_current()->gib_name();
			break;
		}
	}

	// no factory found => take the usual name scheme
	if(building==NULL) {
		int li_gr = lo.x + 2;
		int re_gr = ur.x - 2;
		int ob_gr = lo.y + 2;
		int un_gr = ur.y - 2;

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
	}

    char buf [512];

	if(number>=0 && umgebung_t::numbered_stations) {
		int n=sprintf(buf, translator::translate(base),
			this->name,
			translator::translate(typ));
		sprintf(buf+n, " (%d)",
			number);
	}
	else if(building==NULL) {
		sprintf(buf, translator::translate(base),
			this->name,
			translator::translate(typ));
	}
	else {
		// with factories
		sprintf(buf, translator::translate("%s building %s %s"),
			this->name,
			building,
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
DBG_DEBUG("karte_t::init()","get random places");
	slist_tpl<koord> *list = wl->finde_plaetze(2,3);
DBG_DEBUG("karte_t::init()","found %i places",list->count());
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
  buf += sprintf(buf, "%s: %d (+%d)\n",
     translator::translate("City size"),
     gib_einwohner(),
     gib_wachstum()
     );

buf += sprintf(buf,translator::translate("%d buildings\n"),buildings.get_count());

  buf += sprintf(buf, "\n%d,%d - %d,%d\n\n",
     lo.x, lo.y, ur.x , ur.y
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
  buf.append(")");
}
