/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * construction of cities, creation of passengers
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "boden/wege/strasse.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "simworld.h"
#include "simware.h"
#include "player/simplay.h"
#include "simplan.h"
#include "simimg.h"
#include "vehicle/simverkehr.h"
#include "simtools.h"
#include "simhalt.h"
#include "simfab.h"
#include "simcity.h"
#include "simmesg.h"
#include "simcolor.h"

#include "gui/karte.h"
#include "gui/stadt_info.h"

#include "besch/haus_besch.h"

#include "simintr.h"
#include "simdebug.h"

#include "dings/gebaeude.h"

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
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"


karte_t* stadt_t::welt = NULL; // one is enough ...

/********************************* From here on cityrules stuff *****************************************/


/**
 * in this fixed interval, construction will happen
 * 21s = 21000 per house
 */
const uint32 stadt_t::step_bau_interval = 21000;

/**
 * try to built cities at least this distance apart
 * @author prissi
 */
static int minimum_city_distance = 16;

/*
 * chance to do renovation instead new building (in percent)
 * @author prissi
 */
static uint32 renovation_percentage = 12;

/*
 * minimum ratio of city area to building area to allow expansion
 * the higher this value, the slower the city expansion if there are still "holes"
 * @author prissi
 */
static uint32 min_building_desity = 25;

/**
 * add a new consumer every % people increase
 * @author prissi
 */
static int industry_increase_every[8];


// the following are the scores for the different building types
static int ind_start_score =   0;
static int com_start_score = -10;
static int res_start_score =   0;

// order: res com, ind, given by gebaeude_t::typ
static int ind_neighbour_score[] = { -8, 0,  8 };
static int com_neighbour_score[] = {  1, 8,  1 };
static int res_neighbour_score[] = {  8, 0, -8 };

/**
 * Rule data structure
 * maximum 7x7 rules
 * @author Hj. Malthaner
 */
struct rule_t {
	int  chance;
	char rule[49];
};

// house rules
static int num_house_rules = 0;
static struct rule_t* house_rules = 0;

// and road rules
static int num_road_rules = 0;
static struct rule_t* road_rules = 0;

// rotation of the rules ...
static const uint8 rotate_rules_0[] = {
	 0,  1,  2,  3,  4,  5,  6,
	 7,  8,  9, 10, 11, 12, 13,
	14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27,
	28, 29, 30, 31, 32, 33, 34,
	35, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 47, 48
};


static const uint8 rotate_rules_90[] = {
	6, 13, 20, 27, 34, 41, 48,
	5, 12, 19, 26, 33, 40, 47,
	4, 11, 18, 25, 32, 39, 46,
	3, 10, 17, 24, 31, 38, 45,
	2,  9, 16, 23, 30, 37, 44,
	1,  8, 15, 22, 29, 36, 43,
	0,  7, 14, 21, 28, 35, 42
};

static const uint8 rotate_rules_180[] = {
	48, 47, 46, 45, 44, 43, 42,
	41, 40, 39, 38, 37, 36, 35,
	34, 33, 32, 31, 30, 29, 28,
	27, 26, 25, 24, 23, 22, 21,
	20, 19, 18, 17, 16, 15, 14,
	13, 12, 11, 10,  9,  8,  7,
	 6,  5,  4,  3,  2,  1,  0
};

static const uint8 rotate_rules_270[] = {
	42, 35, 28, 21, 14,  7, 0,
	43, 36, 29, 22, 15,  8, 1,
	44, 37, 30, 23, 16,  9, 2,
	45, 38, 31, 24, 17, 10, 3,
	46, 39, 32, 25, 18, 11, 4,
	47, 40, 33, 26, 19, 12, 5,
	48, 41, 34, 27, 20, 13, 6
};

/**
 * Symbols in rules:
 * S = darf keine Strasse sein
 * s = muss Strasse sein
 * n = muss Natur sein
 * H = darf kein Haus sein
 * h = muss Haus sein
 * T = not a stop	// added in 88.03.3
 * t = is a stop // added in 88.03.3
 * u = good slope for way
 * U = not a slope for ways
 * . = beliebig
 *
 * @param pos position to check
 * @param regel the rule to evaluate
 * @return true on match, false otherwise
 * @author Hj. Malthaner
 */
bool stadt_t::bewerte_loc(const koord pos, const char* regel, int rotation)
{
	koord k;
	const uint8* index_to_rule = 0;
	switch (rotation) {
		case   0: index_to_rule = rotate_rules_0;   break;
		case  90: index_to_rule = rotate_rules_90;  break;
		case 180: index_to_rule = rotate_rules_180; break;
		case 270: index_to_rule = rotate_rules_270; break;
	}

	uint8 rule_index = 0;
	for (k.y = pos.y - 3; k.y <= pos.y + 3; k.y++) {
		for (k.x = pos.x - 3; k.x <= pos.x + 3; k.x++) {
			const char rule = regel[index_to_rule[rule_index++]];
			if (rule != 0) {
				const grund_t* gr = welt->lookup_kartenboden(k);
				if (gr == NULL) {
					// outside of the map => cannot apply this rule
					return false;
				}

				switch (rule) {
					case 's':
						// road?
						if (!gr->hat_weg(road_wt)) return false;
						break;
					case 'S':
						// not road?
						if (gr->hat_weg(road_wt)) return false;
						break;
					case 'h':
						// is house
						if (gr->get_typ() != grund_t::fundament  ||  gr->obj_bei(0)->get_typ()!=ding_t::gebaeude) return false;
						break;
					case 'H':
						// no house
						if (gr->get_typ() == grund_t::fundament) return false;
						break;
					case 'n':
						// nature/empty
						if (!gr->ist_natur() || gr->kann_alle_obj_entfernen(NULL) != NULL) return false;
						break;
 					case 'u':
 						// unbuildable for road
 						if (!hang_t::ist_wegbar(gr->get_grund_hang())) return false;
 						break;
 					case 'U':
 						// road may be buildable
 						if (hang_t::ist_wegbar(gr->get_grund_hang())) return false;
 						break;
					case 't':
						// here is a stop/extension building
						if (!gr->is_halt()) return false;
						break;
					case 'T':
						// no stop
						if (gr->is_halt()) return false;
						break;
				}
			}
		}
	}
	return true;
}


/**
 * Check rule in all transformations at given position
 * prissi: but the rules should explicitly forbid building then?!?
 * @author Hj. Malthaner
 */
sint32 stadt_t::bewerte_pos(const koord pos, const char* regel)
{
	// will be called only a single time, so we can stop after a single match
	if(bewerte_loc(pos, regel,   0) ||
		 bewerte_loc(pos, regel,  90) ||
		 bewerte_loc(pos, regel, 180) ||
		 bewerte_loc(pos, regel, 270)) {
		return 1;
	}
	return 0;
}


void stadt_t::bewerte_strasse(koord k, sint32 rd, const char* regel)
{
	if (simrand(rd) == 0) {
		best_strasse.check(k, bewerte_pos(k, regel));
	}
}


void stadt_t::bewerte_haus(koord k, sint32 rd, const char* regel)
{
	if (simrand(rd) == 0) {
		best_haus.check(k, bewerte_pos(k, regel));
	}
}




/**
 * Reads city configuration data
 * @author Hj. Malthaner
 */
bool stadt_t::cityrules_init(cstring_t objfilename)
{
	tabfile_t cityconf;
	// first take user data, then user global data
	cstring_t user_dir=umgebung_t::user_dir;
	if (!cityconf.open(user_dir+"cityrules.tab")) {
		if (!cityconf.open(objfilename+"config/cityrules.tab")) {
			dbg->fatal("stadt_t::init()", "Can't read cityrules.tab" );
			return false;
		}
	}

	tabfileobj_t contents;
	cityconf.read(contents);

	char buf[128];

	minimum_city_distance = contents.get_int("minimum_city_distance", 16);
	renovation_percentage = (uint32)contents.get_int("renovation_percentage", 25);
	min_building_desity = (uint32)contents.get_int("minimum_building_desity", 25);
	int ind_increase = contents.get_int("industry_increase_every", 0);
	for (int i = 0; i < 8; i++) {
		industry_increase_every[i] = ind_increase << i;
	}

	// init the building value tables
	ind_start_score = contents.get_int("ind_start_score", 0);
	ind_neighbour_score[0] = contents.get_int("ind_near_res", -8);
	ind_neighbour_score[1] = contents.get_int("ind_near_com",  0);
	ind_neighbour_score[2] = contents.get_int("ind_near_ind",  8);

	com_start_score = contents.get_int("com_start_score", -10);
	com_neighbour_score[0] = contents.get_int("com_near_res", 1);
	com_neighbour_score[1] = contents.get_int("com_near_com", 8);
	com_neighbour_score[2] = contents.get_int("com_near_ind", 1);

	res_start_score = contents.get_int("res_start_score", 0);
	res_neighbour_score[0] = contents.get_int("res_near_res",  8);
	res_neighbour_score[1] = contents.get_int("res_near_com",  0);
	res_neighbour_score[2] = contents.get_int("res_near_ind", -8);

	num_house_rules = 0;
	for (;;) {
		sprintf(buf, "house_%d", num_house_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_house_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %d house building rules", num_house_rules);

	num_road_rules = 0;
	for (;;) {
		sprintf(buf, "road_%d", num_road_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_road_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %d road building rules", num_road_rules);

	house_rules = new struct rule_t[num_house_rules];
	for (int i = 0; i < num_house_rules; i++) {
		sprintf(buf, "house_%d.chance", i + 1);
		house_rules[i].chance = contents.get_int(buf, 0);
		memset(house_rules[i].rule, 0, sizeof(house_rules[i].rule));

		sprintf(buf, "house_%d", i + 1);
		const char* rule = contents.get_string(buf, "");

		// skip leading spaces (use . for padding)
		while (*rule == ' ') {
			rule++;
		}

		// find out rule size
		size_t size = 0;
		size_t maxlen = strlen(rule);
		while (size < maxlen  &&  rule[size]!=' ') {
			size++;
		}

		if (size > 7  ||  maxlen < size * (size + 1) - 1  ||  (size & 1) == 0  ||  size <= 2 ) {
			dbg->fatal("stadt_t::cityrules_init()", "house rule %d has bad format!", i + 1);
		}

		// put rule into memory
		const uint offset = (7 - (uint)size) / 2;
		for (uint y = 0; y < size; y++) {
			for (uint x = 0; x < size; x++) {
				house_rules[i].rule[(offset + y) * 7 + x + offset] = rule[x + y * (size + 1)];
			}
		}
	}

	road_rules = new struct rule_t[num_road_rules];
	for (int i = 0; i < num_road_rules; i++) {
		sprintf(buf, "road_%d.chance", i + 1);
		road_rules[i].chance = contents.get_int(buf, 0);
		memset(road_rules[i].rule, 0, sizeof(road_rules[i].rule));

		sprintf(buf, "road_%d", i + 1);
		const char* rule = contents.get_string(buf, "");

		// skip leading spaces (use . for padding)
		while (*rule == ' ') {
			rule++;
		}

		// find out rule size
		size_t size = 0;
		size_t maxlen = strlen(rule);
		while (size < maxlen && rule[size] != ' ') {
			size++;
		}

		if (  size > 7  ||  maxlen < size * (size + 1) - 1  ||  (size & 1) == 0  ||  size <= 2  ) {
			dbg->fatal("stadt_t::cityrules_init()", "road rule %d has bad format!", i + 1);
		}

		// put rule into memory
		const uint8 offset = (7 - (uint)size) / 2;
		for (uint y = 0; y < size; y++) {
			for (uint x = 0; x < size; x++) {
				road_rules[i].rule[(offset + y) * 7 + x + offset] = rule[x + y * (size + 1)];
			}
		}
	}
	return true;
}



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
	public:
		denkmal_platz_sucher_t(karte_t* welt) : platzsucher_t(welt) {}

		virtual bool ist_feld_ok(koord pos, koord d, climate_bits cl) const
		{
			const planquadrat_t* plan = welt->lookup(pos + d);

			// Hajo: can't build here
			if (plan == NULL) return false;

			const grund_t* gr = plan->get_kartenboden();
			if (((1 << welt->get_climate(gr->get_hoehe())) & cl) == 0) {
				return false;
			}

			if (ist_randfeld(d)) {
				return
					gr->get_grund_hang() == hang_t::flach &&     // Flach
					gr->get_typ() == grund_t::boden &&           // Boden -> keine GEbäude
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // Höchstens Strassen
					gr->kann_alle_obj_entfernen(NULL) == NULL;   // Irgendwas verbaut den Platz?
			} else {
				return
					gr->get_grund_hang() == hang_t::flach &&
					gr->get_typ() == grund_t::boden &&
					gr->ist_natur() &&                         // Keine Wege hier
					gr->kann_alle_obj_entfernen(NULL) == NULL; // Irgendwas verbaut den Platz?
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
	public:
		rathausplatz_sucher_t(karte_t* welt) : platzsucher_t(welt) {}

		virtual bool ist_feld_ok(koord pos, koord d, climate_bits cl) const
		{
			const grund_t* gr = welt->lookup_kartenboden(pos + d);
			if (gr == NULL) return false;

			if (((1 << welt->get_climate(gr->get_hoehe())) & cl) == 0) {
				return false;
			}

			if (d.x > 0 || d.y > 0) {
				if (welt->max_hgt(pos) != welt->max_hgt(pos + d)) {
					// height wrong!
					return false;
				}
			}

			if (d.y == h - 1) {
				// Hier soll eine Strasse hin
				return
					gr->get_grund_hang() == hang_t::flach &&
					gr->get_typ() == grund_t::boden &&
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // Höchstens Strassen
					!gr->is_halt() &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			} else {
				// Hier soll das Haus hin - wir ersetzen auch andere Gebäude, aber keine Wege!
				return
					gr->get_grund_hang()==hang_t::flach && (
						(gr->get_typ()==grund_t::boden  &&  gr->ist_natur()) ||
						gr->get_typ()==grund_t::fundament
					) &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			}
		}
};


// this function adds houses to the city house list
void stadt_t::add_gebaeude_to_stadt(const gebaeude_t* gb)
{
	if (gb != NULL) {
		const haus_tile_besch_t* tile  = gb->get_tile();
		koord size = tile->get_besch()->get_groesse(tile->get_layout());
		const koord pos = gb->get_pos().get_2d() - tile->get_offset();
		koord k;

		// add all tiles
		for (k.y = 0; k.y < size.y; k.y++) {
			for (k.x = 0; k.x < size.x; k.x++) {
				gebaeude_t* add_gb = dynamic_cast<gebaeude_t*>(welt->lookup_kartenboden(pos + k)->first_obj());
				if(add_gb) {
					if(add_gb->get_tile()->get_besch()!=gb->get_tile()->get_besch()) {
						dbg->error( "stadt_t::add_gebaeude_to_stadt()","two buildings \"%s\" and \"%s\" at (%i,%i): Game will crash during deletion", add_gb->get_tile()->get_besch()->get_name(), gb->get_tile()->get_besch()->get_name(), pos.x + k.x, pos.y + k.y);
						buildings.remove(add_gb);
					}
					else {
						buildings.append(add_gb, tile->get_besch()->get_level() + 1, 16);
					}
					add_gb->set_stadt(this);
				}
			}
		}
		// check borders
		pruefe_grenzen(pos);
		if(size!=koord(1,1)) {
			pruefe_grenzen(pos+size-koord(1,1));
		}
	}
}



// this function removes houses from the city house list
void stadt_t::remove_gebaeude_from_stadt(gebaeude_t* gb)
{
	buildings.remove(gb);
	gb->set_stadt(NULL);
	recalc_city_size();
}



// just updates the weight count of this building (after a renovation)
void stadt_t::update_gebaeude_from_stadt(gebaeude_t* gb)
{
	buildings.remove(gb);
	buildings.append(gb, gb->get_tile()->get_besch()->get_level() + 1, 16);
}



void stadt_t::pruefe_grenzen(koord k)
{
	if(  has_low_density  ) {
		// has extra wide borders => change density calculation
		has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/(abs(ur.x-lo.x-4)*abs(ur.y-lo.y-4)+1) > min_building_desity);
		if(!has_low_density)  {
			// full recalc needed due to map borders ...
			recalc_city_size();
			return;
		}
	}
	else {
		has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/((ur.x-lo.x)*(ur.y-lo.y)+1) > min_building_desity);
		if(has_low_density)  {
			// wide borders again ..
			lo -= koord(2,2);
			ur += koord(2,2);
		}
	}
	// now just add single coordinates
	if(  has_low_density  ) {
		if (k.x < lo.x+2) {
			lo.x = k.x - 2;
		}
		if (k.y < lo.y+2) {
			lo.y = k.y - 2;
		}

		if (k.x > ur.x-2) {
			ur.x = k.x + 2;
		}
		if (k.y > ur.y-2) {
			ur.y = k.y + 2;
		}
	}
	else {
		// first grow within ...
		if (k.x < lo.x) {
			lo.x = k.x;
		}
		if (k.y < lo.y) {
			lo.y = k.y;
		}

		if (k.x > ur.x) {
			ur.x = k.x;
		}
		if (k.y > ur.y) {
			ur.y = k.y;
		}
	}

	if (lo.x < 0) {
		lo.x = 0;
	}
	if (lo.y < 0) {
		lo.y = 0;
	}
	if (ur.x >= welt->get_groesse_x()) {
		ur.x = welt->get_groesse_x()-1;
	}
	if (ur.y >= welt->get_groesse_y()) {
		ur.y = welt->get_groesse_y()-1;
	}
}



// recalculate the spreading of a city
// will be updated also after house deletion
void stadt_t::recalc_city_size()
{
	lo = pos;
	ur = pos;
	for(  uint32 i=0;  i<buildings.get_count();  i++  ) {
		if(buildings[i]->get_tile()->get_besch()->get_utyp()!=haus_besch_t::firmensitz) {
			const koord gb_pos = buildings[i]->get_pos().get_2d();
			if (lo.x > gb_pos.x) {
				lo.x = gb_pos.x;
			}
			if (lo.y > gb_pos.y) {
				lo.y = gb_pos.y;
			}
			if (ur.x < gb_pos.x) {
				ur.x = gb_pos.x;
			}
			if (ur.y < gb_pos.y) {
				ur.y = gb_pos.y;
			}
		}
	}

	has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/((ur.x-lo.x)*(ur.y-lo.y)+1) > min_building_desity);
	if(  has_low_density  ) {
		// wider borders for faster growth of sparse small towns
		lo.x -= 2;
		lo.y -= 2;
		ur.x += 2;
		ur.y += 2;
	}

	if (lo.x < 0) {
		lo.x = 0;
	}
	if (lo.y < 0) {
		lo.y = 0;
	}
	if (ur.x >= welt->get_groesse_x()) {
		ur.x = welt->get_groesse_x()-1;
	}
	if (ur.y >= welt->get_groesse_y()) {
		ur.y = welt->get_groesse_y()-1;
	}
}



void stadt_t::init_pax_destinations()
{
	pax_destinations_old.clear();
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;
}


stadt_t::~stadt_t()
{
	// close info win
	destroy_win((long)this);

	if(  reliefkarte_t::get_karte()->get_city() == this  ) {
		reliefkarte_t::get_karte()->set_city(NULL);
	}

	// olny if there is still a world left to delete from
	if(welt->get_groesse_x()>1) {

		welt->lookup_kartenboden(pos)->set_text(NULL);

		// remove city info and houses
		while (!buildings.empty()) {
			// old buildings are not where they think they are, so we ask for map floor
			gebaeude_t* gb = (gebaeude_t *)buildings.front();
			buildings.remove(gb);
			assert(  gb!=NULL  &&  !buildings.is_contained(gb)  );
			if(gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::firmensitz) {
				stadt_t *city = welt->suche_naechste_stadt(gb->get_pos().get_2d());
				gb->set_stadt( city );
				if(city) {
					city->buildings.append(gb,gb->get_passagier_level(),16);
				}
			}
			else {
				gb->set_stadt( NULL );
				hausbauer_t::remove(welt,welt->get_spieler(1),gb);
			}
		}
	}
	free( (void *)name );
}


stadt_t::stadt_t(spieler_t* sp, koord pos, sint32 citizens) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	arbeiterziele(4)
{
	welt = sp->get_welt();
	assert(welt->ist_in_kartengrenzen(pos));

	step_count = 0;
	pax_destinations_new_change = 0;
	next_step = 0;
	step_interval = 1;
	next_bau_step = 0;
	has_low_density = false;

	stadtinfo_options = 3;	// citicens and growth

	besitzer_p = sp;

	this->pos = pos;

	bev = 0;
	arb = 0;
	won = 0;

	lo = ur = pos;

	DBG_MESSAGE("stadt_t::stadt_t()", "Welt %p", welt);
	fflush(NULL);
	/* get a unique cityname */
	/* 9.1.2005, prissi */
	const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
	const int name_list_count = translator::get_count_city_name();

	fflush(NULL);
	// start at random position
	int start_cont = simrand(name_list_count);

	// get a unique name
	const char* list_name;
	list_name = translator::get_city_name(start_cont);
	for (int i = 0; i < name_list_count; i++) {
		// get a name
		list_name = translator::get_city_name(start_cont + i);
		// check if still unused
		for (weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j) {
			// noch keine stadt mit diesem namen?
			if (strcmp(list_name, (*j)->get_name()) == 0) goto next_name;
		}
		DBG_MESSAGE("stadt_t::stadt_t()", "'%s' is unique", list_name);
		break;
next_name:;
	}
	name = strdup(list_name);

	DBG_MESSAGE("stadt_t::stadt_t()", "founding new city named '%s'", list_name);

	// 1. Rathaus bei 0 Leuten bauen
	check_bau_rathaus(true);

	wachstum = 0;
	change_size( citizens );

	// fill with start citicens ...
	sint64 bew = get_einwohner();
	for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
		city_history_year[year][HIST_CITICENS] = bew;
	}
	for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
		city_history_month[month][HIST_CITICENS] = bew;
	}

	// initialize history array
	for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
		for (uint hist_type = 1; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_year[year][hist_type] = 0;
		}
	}
	for (uint month = 0; month < MAX_CITY_HISTORY_YEARS; month++) {
		for (uint hist_type = 1; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_month[month][hist_type] = 0;
		}
	}
	city_history_year[0][HIST_CITICENS]  = get_einwohner();
	city_history_month[0][HIST_CITICENS] = get_einwohner();
}



stadt_t::stadt_t(karte_t* wl, loadsave_t* file) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
{
	welt = wl;
	step_count = 0;
	next_step = 0;
	step_interval = 1;
	next_bau_step = 0;
	has_low_density = false;

	wachstum = 0;
	name = NULL;
	stadtinfo_options = 3;

	rdwr(file);

	verbinde_fabriken();
}


void stadt_t::rdwr(loadsave_t* file)
{
	sint32 besitzer_n;

	if (file->is_saving()) {
		besitzer_n = welt->sp2num(besitzer_p);
	}
	file->rdwr_str(name);
	pos.rdwr(file);
	uint32 lli = lo.x;
	uint32 lob = lo.y;
	uint32 lre = ur.x;
	uint32 lun = ur.y;
	file->rdwr_long(lli, " ");
	file->rdwr_long(lob, "\n");
	file->rdwr_long(lre, " ");
	file->rdwr_long(lun, "\n");
	lo.x = lli;
	lo.y = lob;
	ur.x = lre;
	ur.y = lun;
	file->rdwr_long(besitzer_n, "\n");
	file->rdwr_long(bev, " ");
	file->rdwr_long(arb, " ");
	file->rdwr_long(won, "\n");
	// old values zentrum_namen_cnt : aussen_namen_cnt
	if(file->get_version()<99018) {
		sint32 dummy=0;
		file->rdwr_long(dummy, " ");
		file->rdwr_long(dummy, "\n");
	}

	if (file->is_loading()) {
		besitzer_p = welt->get_spieler(besitzer_n);
	}

	if(file->is_loading()) {
		// initialize history array
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				city_history_year[year][hist_type] = 0;
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				city_history_month[month][hist_type] = 0;
			}
		}
		city_history_year[0][HIST_CITICENS] = get_einwohner();
		city_history_year[0][HIST_CITICENS] = get_einwohner();
	}

	// we probably need to load/save the city history
	if (file->get_version() < 86000) {
		DBG_DEBUG("stadt_t::rdwr()", "is old version: No history!");
	} else if(file->get_version()<99016) {
		// 86.00.0 introduced city history
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
			for (uint hist_type = 0; hist_type < 2; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type], " ");
			}
			for (uint hist_type = 4; hist_type < 6; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type], " ");
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < 2; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type], " ");
			}
			for (uint hist_type = 4; hist_type < 6; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type], " ");
			}
		}
		// not needed any more
		sint32 dummy = 0;
		file->rdwr_long(dummy, " ");
		file->rdwr_long(dummy, "\n");
		file->rdwr_long(dummy, " ");
		file->rdwr_long(dummy, "\n");
	}
	else {
		// 99.17.0 extended city history
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type], " ");
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type], " ");
			}
		}
		// save button settings for this town
		file->rdwr_long( stadtinfo_options, "si" );
	}

	if(file->get_version()>99014  &&  file->get_version()<99016) {
		sint32 dummy = 0;
		file->rdwr_long(dummy, " ");
		file->rdwr_long(dummy, "\n");
	}


	if(file->is_loading()) {
		// 08-Jan-03: Due to some bugs in the special buildings/town hall
		// placement code, li,re,ob,un could've gotten irregular values
		// If a game is loaded, the game might suffer from such an mistake
		// and we need to correct it here.
		DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);

		// recalculate borders
		recalc_city_size();
	}
}



/**
 * Wird am Ende der Laderoutine aufgerufen, wenn die Welt geladen ist
 * und nur noch die Datenstrukturenneu verknüpft werden müssen.
 * @author Hj. Malthaner
 */
void stadt_t::laden_abschliessen()
{
	// there might be broken savegames
	if(name==NULL) {
		set_name( "simcity" );
	}

	// new city => need to grow
	if(buildings.get_count()==0) {
		step_bau();
	}

	// clear the minimaps
	init_pax_destinations();

	// init step counter with meaningful value
	step_interval = (2 << 18u) / (buildings.get_count() * 4 + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	recalc_city_size();

	next_step = 0;
	next_bau_step = 0;
}



void stadt_t::rotate90( const sint16 y_size )
{
	// rotate town origin
	pos.rotate90( y_size );
	// rotate an rectangle
	lo.rotate90( y_size );
	ur.rotate90( y_size );
	sint16 lox = lo.x;
	lo.x = ur.x;
	ur.x = lox;
	// reset building search
	best_strasse.reset(pos);
	best_haus.reset(pos);
	// rathaus position may be changed a little!
	sparse_tpl<uint8> pax_destinations_temp(koord( PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE ));

	uint8 color;
	koord pos;
	for( uint16 i = 0; i < pax_destinations_new.get_data_count(); i++ ) {
		pax_destinations_new.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set( PAX_DESTINATIONS_SIZE-1-pos.y, pos.x, color );
	}
	swap<uint8>( pax_destinations_temp, pax_destinations_new );

	pax_destinations_temp.clear();
	for( uint16 i = 0; i < pax_destinations_old.get_data_count(); i++ ) {
		pax_destinations_old.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set( PAX_DESTINATIONS_SIZE-1-pos.y, pos.x, color );
	}
	pax_destinations_new_change ++;
	swap<uint8>( pax_destinations_temp, pax_destinations_old );
}



void stadt_t::set_name(const char *new_name)
{
	if(name==NULL  ||  strcmp(name,new_name)) {
		free( (void *)name );
		name = strdup( new_name );
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gr->set_text( new_name );
	}
}



/* show city info dialoge
 * @author prissi
 */
void
stadt_t::zeige_info(void)
{
	create_win( new stadt_info_t(this), w_info, (long)this );
}


/* add workers to factory list */
void stadt_t::add_factory_arbeiterziel(fabrik_t* fab)
{
	const koord k = fab->get_pos().get_2d() - pos;
	// worker do not travel more than 50 tiles
	if(  (k.x*k.x) + (k.y*k.y) < CONNECT_TO_TOWN_SQUARE_DISTANCE  ) {
		// no fish swarms ...
		if (strcmp("fish_swarm", fab->get_besch()->get_name()) != 0) {
//			DBG_MESSAGE("stadt_t::add_factory_arbeiterziel()", "found %s with level %i", fab->get_name(), fab->get_besch()->get_pax_level());
			fab->add_arbeiterziel(this);
			arbeiterziele.append_unique(fab, fab->get_besch()->get_pax_level(), 4);
		}
	}
}


/* calculates the factories which belongs to certain cities */
/* we connect all factories, which are closer than 50 tiles radius */
void stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "search factories near %s (center at %i,%i)", get_name(), pos.x, pos.y);

	slist_iterator_tpl<fabrik_t*> fab_iter(welt->get_fab_list());
	arbeiterziele.clear();
	while (fab_iter.next()) {
		add_factory_arbeiterziel(fab_iter.get_current());
	}
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "is connected with %i factories (sum_weight=%i).", arbeiterziele.get_count(), arbeiterziele.get_sum_weight());
}



/* change size of city
 * @author prissi */
void stadt_t::change_size(long delta_citicens)
{
	DBG_MESSAGE("stadt_t::change_size()", "%i + %i", bev, delta_citicens);
	if (delta_citicens > 0) {
		wachstum = delta_citicens<<4;
		step_bau();
	}
	if (delta_citicens < 0) {
		wachstum = 0;
		if (bev > -delta_citicens) {
			bev += delta_citicens;
		}
		else {
//			remove_city();
			bev = 0;
		}
		step_bau();
	}
	wachstum = 0;
	DBG_MESSAGE("stadt_t::change_size()", "%i+%i", bev, delta_citicens);
}



void stadt_t::step(long delta_t)
{
	if(delta_t>20000) {
		delta_t = 1;
	}

	// Ist es Zeit für einen neuen step?
	next_step += delta_t;
	next_bau_step += delta_t;

	step_interval = (1 << 21u) / (buildings.get_count() * welt->get_einstellungen()->get_passenger_factor() + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	while(stadt_t::step_bau_interval < next_bau_step) {
		calc_growth();
		step_bau();
		next_bau_step -= stadt_t::step_bau_interval;
	}

	// create passenger rate proportional to town size
	while(step_interval < next_step) {
		step_passagiere();
		step_count++;
		next_step -= step_interval;
	}

	// update history (might be changed do to construction/destroying of houses)
	city_history_month[0][HIST_CITICENS] = get_einwohner();	// total number
	city_history_year[0][HIST_CITICENS] = get_einwohner();

	city_history_month[0][HIST_GROWTH] = city_history_month[0][HIST_CITICENS]-city_history_month[1][HIST_CITICENS];	// growth
	city_history_year[0][HIST_GROWTH] = city_history_year[0][HIST_CITICENS]-city_history_year[1][HIST_CITICENS];

	city_history_month[0][HIST_BUILDING] = buildings.get_count();
	city_history_year[0][HIST_BUILDING] = buildings.get_count();
}



/* updates the city history
 * @author prissi
 */
void stadt_t::roll_history()
{
	// roll months
	for (int i = MAX_CITY_HISTORY_MONTHS - 1; i > 0; i--) {
		for (int hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_month[i][hist_type] = city_history_month[i - 1][hist_type];
		}
	}
	// init this month
	for (int hist_type = 1; hist_type < MAX_CITY_HISTORY; hist_type++) {
		city_history_month[0][hist_type] = 0;
	}
	city_history_month[0][HIST_CITICENS] = get_einwohner();
	city_history_month[0][HIST_BUILDING] = buildings.get_count();
	city_history_month[0][HIST_GOODS_NEEDED] = 0;

	//need to roll year too?
	if (welt->get_last_month() == 0) {
		for (int i = MAX_CITY_HISTORY_YEARS - 1; i > 0; i--) {
			for (int hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				city_history_year[i][hist_type] = city_history_year[i - 1][hist_type];
			}
		}
		// init this year
		for (int hist_type = 1; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_year[0][hist_type] = 0;
		}
		city_history_year[0][HIST_CITICENS] = get_einwohner();
		city_history_year[0][HIST_BUILDING] = buildings.get_count();
		city_history_year[0][HIST_GOODS_NEEDED] = 0;
	}

}


void stadt_t::neuer_monat()
{
	swap<uint8>( pax_destinations_old, pax_destinations_new );
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;

	roll_history();

	if (!stadtauto_t::list_empty()) {
		// spawn eventuall citycars
		// the more transported, the less are spawned
		// the larger the city, the more spawned ...

		double pfactor = (double)(city_history_month[1][HIST_PAS_TRANSPORTED]) / (double)(city_history_month[1][HIST_PAS_GENERATED]+1);
		double mfactor = (double)(city_history_month[1][HIST_MAIL_TRANSPORTED]) / (double)(city_history_month[1][HIST_MAIL_GENERATED]+1);
		double gfactor = (double)(city_history_month[1][HIST_GOODS_RECIEVED]) / (double)(city_history_month[1][HIST_GOODS_NEEDED]+1);

		double factor = pfactor > mfactor ? (gfactor > pfactor ? gfactor : pfactor ) : mfactor;
		factor = (1.0-factor)*city_history_month[1][HIST_CITICENS];
		factor = log10( factor );
		uint16 number_of_cars = simrand( (uint16)(factor * (double)welt->get_einstellungen()->get_verkehr_level()) ) / 16;

		city_history_month[0][HIST_CITYCARS] = number_of_cars;
		city_history_year[0][HIST_CITYCARS] += number_of_cars;

		koord k;
		koord pos = get_zufallspunkt();
		for (k.y = pos.y - 3; k.y < pos.y + 3; k.y++) {
			for (k.x = pos.x - 3; k.x < pos.x + 3; k.x++) {
				if(number_of_cars==0) {
					return;
				}

				grund_t* gr = welt->lookup_kartenboden(k);
				if (gr != NULL && gr->get_weg(road_wt) && ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt)) && gr->find<stadtauto_t>() == NULL) {
					stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), koord::invalid);
					gr->obj_add(vt);
					welt->sync_add(vt);
					number_of_cars--;
				}
			}
		}
	}
}


void stadt_t::calc_growth()
{
	// now iterate over all factories to get the ratio of producing version nonproducing factories
	// we use the incoming storage as a measure und we will only look for end consumers (power stations, markets)
	for (weighted_vector_tpl<fabrik_t*>::const_iterator iter = arbeiterziele.begin(), end = arbeiterziele.end(); iter != end; ++iter) {
		fabrik_t *fab = *iter;
		if(fab->get_lieferziele().get_count()==0  &&  fab->get_suppliers().get_count()!=0) {
			// consumer => check for it storage
			const fabrik_besch_t *besch = fab->get_besch();
			for(  int i=0;  i<besch->get_lieferanten();  i++  ) {
				city_history_month[0][HIST_GOODS_NEEDED] ++;
				city_history_year[0][HIST_GOODS_NEEDED] ++;
				if(  fab->input_vorrat_an( besch->get_lieferant(i)->get_ware() )>0  ) {
					city_history_month[0][HIST_GOODS_RECIEVED] ++;
					city_history_year[0][HIST_GOODS_RECIEVED] ++;
				}
			}
		}
	}

	/* four parts contribute to town growth:
	 * passenger transport 40%, mail 20%, goods (30%), and electricity (10%)
	 */
	sint32 pas = (city_history_month[0][HIST_PAS_TRANSPORTED] * (40<<6)) / (city_history_month[0][HIST_PAS_GENERATED] + 1);
	sint32 mail = (city_history_month[0][HIST_MAIL_TRANSPORTED] * (20<<6)) / (city_history_month[0][HIST_MAIL_GENERATED] + 1);
	sint32 electricity = 0;
	sint32 goods = city_history_month[0][HIST_GOODS_NEEDED]==0 ? 0 : (city_history_month[0][HIST_GOODS_RECIEVED] * (20<<6)) / (city_history_month[0][HIST_GOODS_NEEDED]);

	// smaller towns should growth slower to have villages for a longer time
	sint32 weight_factor = 100;
	if(bev<1000) {
		weight_factor = 400;
	}
	else if(bev<10000) {
		weight_factor = 200;
	}

	// now give the growth for this step
	wachstum += (pas+mail+electricity+goods) / weight_factor;
}


// does constructions ...
void stadt_t::step_bau()
{
	bool new_town = (bev == 0);
	// since we use internall a finer value ...
	const int	growth_step = (wachstum>>4);
	wachstum &= 0x0F;

	// Hajo: let city grow in steps of 1
	// @author prissi: No growth without development
	for (int n = 0; n < growth_step; n++) {
		bev++; // Hajo: bevoelkerung wachsen lassen

		for (int i = 0; i < 30 && bev * 2 > won + arb + 100; i++) {
			baue();
		}

		check_bau_spezial(new_town);
		check_bau_rathaus(new_town);
		check_bau_factory(new_town); // add industry? (not during creation)
		INT_CHECK("simcity 275");
	}
}


/* this creates passengers and mail for everything is is therefore one of the CPU hogs of the machine
 * think trice, before applying optimisation here ...
 */
void stadt_t::step_passagiere()
{
//	DBG_MESSAGE("stadt_t::step_passagiere()", "%s step_passagiere called (%d,%d - %d,%d)\n", name, li, ob, re, un);
//	long t0 = get_current_time_millis();

	// post oder pax erzeugen ?
	const ware_besch_t* wtyp;
	if (simrand(400) < 300) {
		wtyp = warenbauer_t::passagiere;
	}
	else {
		wtyp = warenbauer_t::post;
	}
	const int history_type = (wtyp == warenbauer_t::passagiere) ? HIST_PAS_TRANSPORTED : HIST_MAIL_TRANSPORTED;

	// restart at first buiulding?
	if (step_count >= buildings.get_count()) {
		step_count = 0;
	}
	if(buildings.get_count()==0) {
		return;
	}
	const gebaeude_t* gb = buildings[step_count];

	// prissi: since now backtravels occur, we damp the numbers a little
	const int num_pax =
		(wtyp == warenbauer_t::passagiere) ?
			(gb->get_tile()->get_besch()->get_level()      + 6) >> 2 :
			max(1,(gb->get_tile()->get_besch()->get_post_level() + 5) >> 4);

	// create pedestrians in the near area?
	if (welt->get_einstellungen()->get_random_pedestrians() && wtyp == warenbauer_t::passagiere) {
		haltestelle_t::erzeuge_fussgaenger(welt, gb->get_pos(), num_pax);
	}

	// suitable start search
	const koord k = gb->get_pos().get_2d();
	const planquadrat_t* plan = welt->lookup(k);
	const halthandle_t* halt_list = plan->get_haltlist();

	// suitable start search
	halthandle_t start_halt = halthandle_t();
	for (uint h = 0; h < plan->get_haltlist_count(); h++) {
		halthandle_t halt = halt_list[h];
		if(  halt->is_enabled(wtyp)  &&  !halt->is_overcrowded(wtyp->get_catg_index())  ) {
			start_halt = halt;
			break;
		}
	}

	// Hajo: track number of generated passengers.
	city_history_year[0][history_type+1] += num_pax;
	city_history_month[0][history_type+1] += num_pax;

	// only continue, if this is a good start halt
	if (start_halt.is_bound()) {
		// Find passenger destination
		for (int pax_routed = 0; pax_routed < num_pax; pax_routed += 7) {
			// number of passengers that want to travel
			// Hajo: for efficiency we try to route not every
			// single pax, but packets. If possible, we do 7 passengers at a time
			// the last packet might have less then 7 pax
			int pax_left_to_do = min(7, num_pax - pax_routed);

			// Ziel für Passagier suchen
			pax_zieltyp will_return;
			const koord ziel = finde_passagier_ziel(&will_return);

#ifdef DESTINATION_CITYCARS
			//citycars with destination
			if (pax_routed == 0) {
				erzeuge_verkehrsteilnehmer(start_halt->get_basis_pos(), step_count, ziel);
			}
#endif
			// Dario: Check if there's a stop near destination
			const planquadrat_t* dest_plan = welt->lookup(ziel);
			const halthandle_t* dest_list = dest_plan->get_haltlist();
			bool can_walk_ziel = false;

			// suitable end search
			unsigned ziel_count = 0;
			for(  uint8 h = 0;  h < dest_plan->get_haltlist_count();  h++  ) {
				halthandle_t halt = dest_list[h];
				if(  halt->is_enabled(wtyp)  ) {
					ziel_count++;
					if(  halt == start_halt  ) {
						can_walk_ziel = true;
						break; // because we found at least one valid step ...
					}
				}
			}

			if(  ziel_count == 0  ) {
// DBG_MESSAGE("stadt_t::step_passagiere()", "No stop near dest (%d, %d)", ziel.x, ziel.y);
				// Thus, routing is not possible and we do not need to do a calculation.
				// Mark ziel as destination without route and continue.
				merke_passagier_ziel(ziel, COL_DARK_ORANGE);
				start_halt->add_pax_no_route(pax_left_to_do);
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				erzeuge_verkehrsteilnehmer(start_halt->get_basis_pos(), step_count, ziel);
#endif
				continue;
			}

			// check, if they can walk there?
			if (can_walk_ziel) {
				// so we have happy passengers
				start_halt->add_pax_happy(pax_left_to_do);
				merke_passagier_ziel(ziel, COL_YELLOW);
				city_history_year[0][history_type] += pax_left_to_do;
				city_history_month[0][history_type] += pax_left_to_do;
				continue;
			}

			// ok, they are not in walking distance
			ware_t pax(wtyp);
			pax.set_zielpos(ziel);
			pax.menge = (wtyp == warenbauer_t::passagiere ? pax_left_to_do : 1 );

			// now, finally search a route; this consumes most of the time
			koord return_zwischenziel = koord::invalid; // for people going back ...
			const int route_result = start_halt->suche_route( pax, will_return ? &return_zwischenziel : NULL, welt->get_einstellungen()->is_no_routing_over_overcrowding() );

			if(  route_result == haltestelle_t::ROUTE_OK  ) {
				// so we have happy traveling passengers
				start_halt->starte_mit_route(pax);
				start_halt->add_pax_happy(pax.menge);
				// and show it
				merke_passagier_ziel(ziel, COL_YELLOW);
				city_history_year[0][history_type] += pax.menge;
				city_history_month[0][history_type] += pax.menge;
			}
			else if(  route_result==haltestelle_t::ROUTE_OVERCROWDED  ) {
				merke_passagier_ziel(ziel, COL_ORANGE );
				start_halt->add_pax_unhappy(pax_left_to_do);
				if(  will_return  ) {
					pax.get_ziel()->add_pax_unhappy(pax_left_to_do);
				}
			}
			else {
				start_halt->add_pax_no_route(pax_left_to_do);
				merke_passagier_ziel(ziel, COL_DARK_ORANGE);
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				erzeuge_verkehrsteilnehmer(start_halt->get_basis_pos(), step_count, ziel);
#endif
			}

			// send them also back
			if(  route_result==haltestelle_t::ROUTE_OK  &&  will_return != no_return  ) {
				// this comes most of the times for free and balances also the amounts!
				halthandle_t ret_halt = pax.get_ziel();

				// we just have to ensure, the ware can be delivered at this station
				bool found = false;
				for (uint i = 0; i < plan->get_haltlist_count(); i++) {
					halthandle_t test_halt = halt_list[i];
					if (test_halt->is_enabled(wtyp)  &&  (start_halt==test_halt  ||  test_halt->get_warenziele(wtyp->get_catg_index())->is_contained(start_halt))) {
						found = true;
						start_halt = test_halt;
						break;
					}
				}

				// now try to add them to the target halt
				uint32 max_ware = ret_halt->get_capacity(wtyp->get_index());
				if(  !ret_halt->is_overcrowded(wtyp->get_catg_index())  ) {
					// prissi: not overcrowded and can recieve => add them
					if (found) {
						ware_t return_pax (wtyp);

						// always use normal amount for return pas/mail
						// (for mail pax.menge might have been smaller!)
						return_pax.menge = pax_left_to_do;
						return_pax.set_zielpos(k);
						return_pax.set_ziel(start_halt);
						return_pax.set_zwischenziel(welt->lookup(return_zwischenziel)->get_halt());

						ret_halt->starte_mit_route(return_pax);
						ret_halt->add_pax_happy(pax_left_to_do);
					} else {
						// no route back
						ret_halt->add_pax_no_route(pax_left_to_do);
					}
				}
				else {
					// return halt crowded
					ret_halt->add_pax_unhappy(pax_left_to_do);
				}
			}
			INT_CHECK( "simcity 1579" );
		}
	}
	else {
		// the unhappy passengers will be added to all crowded stops
		// however, there might be no stop too
		for(  uint h=0;  h<plan->get_haltlist_count(); h++  ) {
			halthandle_t halt = halt_list[h];
			if (halt->is_enabled(wtyp)) {
//			assert(halt->get_ware_summe(wtyp)>halt->get_capacity();
				halt->add_pax_unhappy(num_pax);
			}
		}

		// all passengers without suitable start:
		// fake one ride to get a proper display of destinations (although there may be more) ...
		pax_zieltyp will_return;
		const koord ziel = finde_passagier_ziel(&will_return);
#ifdef DESTINATION_CITYCARS
		//citycars with destination
		erzeuge_verkehrsteilnehmer(k, step_count, ziel);
#endif
		merke_passagier_ziel(ziel, COL_DARK_ORANGE);
		// we do not show no route for destination stop!
	}

//	long t1 = get_current_time_millis();
//	DBG_MESSAGE("stadt_t::step_passagiere()", "Zeit für Passagierstep: %ld ms\n", (long)(t1 - t0));
}


/**
 * gibt einen zufällingen gleichverteilten Punkt innerhalb der
 * Stadtgrenzen zurück
 * @author Hj. Malthaner
 */
koord stadt_t::get_zufallspunkt() const
{
	if(!buildings.empty()) {
		gebaeude_t* gb = buildings.at_weight(simrand(buildings.get_sum_weight()));
		koord k = gb->get_pos().get_2d();
		if(!welt->ist_in_kartengrenzen(k)) {
			// this building should not be in this list, since it has been already deleted!
			dbg->error("stadt_t::get_zufallspunkt()", "illegal building in city list of %s: %p removing!", this->get_name(), gb);
			const_cast<stadt_t*>(this)->buildings.remove(gb);
			k = koord(0, 0);
		}
		return k;
	}
	// might happen on slow computers during creation of new cities or start of map
	return koord(0,0);
}


/* this function generates a random target for passenger/mail
 * changing this strongly affects selection of targets and thus game strategy
 */
koord stadt_t::finde_passagier_ziel(pax_zieltyp* will_return)
{
	const int rand = simrand(100);

	// about 1/3 are workers
	if (rand < FACTORY_PAX && arbeiterziele.get_sum_weight() > 0) {
		const fabrik_t* fab = arbeiterziele.at_weight(simrand(arbeiterziele.get_sum_weight()));
		*will_return = factoy_return;	// worker will return
		return fab->get_pos().get_2d();
	} else if (rand < TOURIST_PAX + FACTORY_PAX && welt->get_ausflugsziele().get_sum_weight() > 0) {
		*will_return = tourist_return;	// tourists will return
		const gebaeude_t* gb = welt->get_random_ausflugsziel();
		return gb->get_pos().get_2d();
	} else {
		// if we reach here, at least a single town existes ...
		const stadt_t* zielstadt = welt->get_random_stadt();

		// we like nearer towns more
		if (abs(zielstadt->pos.x - pos.x) + abs(zielstadt->pos.y - pos.y) > 120) {
			// retry once ...
			zielstadt = welt->get_random_stadt();
		}

		// long distance traveller? => then we return
		*will_return = (this != zielstadt) ? town_return : no_return;
		return zielstadt->get_zufallspunkt();
	}
}


void stadt_t::merke_passagier_ziel(koord k, uint8 color)
{
	const koord p = koord(
		((k.x * PAX_DESTINATIONS_SIZE) / welt->get_groesse_x()) & (PAX_DESTINATIONS_SIZE-1),
		((k.y * PAX_DESTINATIONS_SIZE) / welt->get_groesse_y()) & (PAX_DESTINATIONS_SIZE-1)
	);
	pax_destinations_new_change ++;
	pax_destinations_new.set(p, color);
}


/**
 * bauplatz_mit_strasse_sucher_t:
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 * added: Minimum distance between monuments
 * @author V. Meyer/prissi
 */
class bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t
{
	public:
		bauplatz_mit_strasse_sucher_t(karte_t* welt) : bauplatz_sucher_t (welt) {}

		// get distance to next factory
		int find_dist_next_special(koord pos) const
		{
			const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_ausflugsziele();
			int dist = welt->get_groesse_x() * welt->get_groesse_y();
			for (weighted_vector_tpl<gebaeude_t*>::const_iterator i = attractions.begin(), end = attractions.end(); i != end; ++i) {
				int d = koord_distance((*i)->get_pos(), pos);
				if (d < dist) {
					dist = d;
				}
			}
			return dist;
		}

		bool strasse_bei(sint16 x, sint16 y) const
		{
			const grund_t* bd = welt->lookup(koord(x, y))->get_kartenboden();
			return bd != NULL && bd->hat_weg(road_wt);
		}

		virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const
		{
			if (bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
				// nothing on top like elevated monorails?
				for (sint16 y = pos.y;  y < pos.y + h; y++) {
					for (sint16 x = pos.x; x < pos.x + b; x++) {
						grund_t *gr = welt->lookup_kartenboden(koord(x,y));
						if(gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1))!=NULL) {
							// something on top (monorail or powerlines)
							return false;
						}
					}
				}
				// try to built a little away from previous ones
				if (find_dist_next_special(pos) < b + h + 1) {
					return false;
				}
				// now check for road connection
				for (int i = pos.y; i < pos.y + h; i++) {
					if (strasse_bei(pos.x - 1, i) || strasse_bei(pos.x + b, i)) {
						return true;
					}
				}
				for (int i = pos.x; i < pos.x + b; i++) {
					if (strasse_bei(i, pos.y - 1) || strasse_bei(i, pos.y + h)) {
						return true;
					}
				}
			}
			return false;
		}
};


void stadt_t::check_bau_spezial(bool new_town)
{
	// touristenattraktion bauen
	const haus_besch_t* besch = hausbauer_t::get_special(bev, haus_besch_t::attraction_city, welt->get_timeline_year_month(), new_town, welt->get_climate(welt->max_hgt(pos)));
	if (besch != NULL) {
		if (simrand(100) < (uint)besch->get_chance()) {
			// baue was immer es ist
			int rotate = 0;
			bool is_rotate = besch->get_all_layouts() > 10;
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos, besch->get_b(), besch->get_h(), besch->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				if (besch->get_all_layouts() > 1) {
					rotate = (simrand(20) & 2) + is_rotate;
				}
				hausbauer_t::baue( welt, besitzer_p, welt->lookup(best_pos)->get_kartenboden()->get_pos(), rotate, besch );
				// tell the player, if not during initialization
				if (!new_town) {
					char buf[256];
					sprintf(buf, translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), get_name(), make_single_line_string(translator::translate(besch->get_name()), 2), bev);
					welt->get_message()->add_message(buf, best_pos, message_t::tourist, CITY_KI, besch->get_tile(0)->get_hintergrund(0, 0, 0));
				}
			}
		}
	}

	if ((bev & 511) == 0) {
		// errect a monoment
		besch = hausbauer_t::waehle_denkmal(welt->get_timeline_year_month());
		if (besch) {
			koord total_size = koord(2 + besch->get_b(), 2 + besch->get_h());
			koord best_pos(denkmal_platz_sucher_t(welt).suche_platz(pos, total_size.x, total_size.y, besch->get_allowed_climate_bits()));

			if (best_pos != koord::invalid) {
				bool ok = false;

				// Wir bauen das Denkmal nur, wenn schon mindestens eine Strasse da ist
				for (int i = 0; i < total_size.x && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(i, -1))->get_kartenboden()->hat_weg(road_wt) ||
						welt->access(best_pos + koord(i, total_size.y))->get_kartenboden()->hat_weg(road_wt);
				}
				for (int i = 0; i < total_size.y && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(total_size.x, i))->get_kartenboden()->hat_weg(road_wt) ||
						welt->access(best_pos + koord(-1, i))->get_kartenboden()->hat_weg(road_wt);
				}
				if (ok) {
					// Straßenkreis um das Denkmal legen
					sint16 h=welt->lookup_kartenboden(best_pos)->get_hoehe();
					for (int i = 0; i < total_size.y; i++) {
						// only build in same height and not on slopes...
						const grund_t *gr = welt->lookup_kartenboden(best_pos + koord(0, i));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							baue_strasse(best_pos + koord(0, i), NULL, true);
						}
						gr = welt->lookup_kartenboden(best_pos + koord(total_size.x - 1, i));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							baue_strasse(best_pos + koord(total_size.x - 1, i), NULL, true);
						}
					}
					for (int i = 0; i < total_size.x; i++) {
						// only build in same height and not on slopes...
						const grund_t *gr = welt->lookup_kartenboden(best_pos + koord(i, 0));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							baue_strasse(best_pos + koord(i, 0), NULL, true);
						}
						gr = welt->lookup_kartenboden(best_pos + koord(i, total_size.y - 1));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							baue_strasse(best_pos + koord(i, total_size.y - 1), NULL, true);
						}
					}
					// and then build it
					const gebaeude_t* gb = hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos + koord(1, 1))->get_kartenboden()->get_pos(), 0, besch);
					hausbauer_t::denkmal_gebaut(besch);
					add_gebaeude_to_stadt(gb);
					// tell the player, if not during initialization
					if (!new_town) {
						char buf[256];
						sprintf(buf, translator::translate("With a big festival\n%s built\na new monument.\n%i citicens rejoiced."), get_name(), bev);
						welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(0)->get_hintergrund(0, 0, 0));
					}
				}
			}
		}
	}
}


void stadt_t::check_bau_rathaus(bool new_town)
{
	const haus_besch_t* besch = hausbauer_t::get_special(bev, haus_besch_t::rathaus, welt->get_timeline_year_month(), new_town, welt->get_climate(welt->max_hgt(pos)));
	if (besch != NULL) {
		grund_t* gr = welt->lookup(pos)->get_kartenboden();
		gebaeude_t* gb = dynamic_cast<gebaeude_t*>(gr->first_obj());
		bool neugruendung = !gb || !gb->ist_rathaus();
		bool umziehen = !neugruendung;
		koord alte_str(koord::invalid);
		koord best_pos(pos);
		koord k;

		DBG_MESSAGE("check_bau_rathaus()", "bev=%d, new=%d name=%s", bev, neugruendung, name);

		if (!neugruendung) {

			const haus_besch_t* besch_alt = gb->get_tile()->get_besch();
			if (besch_alt->get_level() == besch->get_level()) {
				DBG_MESSAGE("check_bau_rathaus()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}

			koord pos_alt = best_pos = gr->get_pos().get_2d() - gb->get_tile()->get_offset();
			koord groesse_alt = besch_alt->get_groesse(gb->get_tile()->get_layout());

			// do we need to move
			if (besch->get_b() <= groesse_alt.x && besch->get_h() <= groesse_alt.y) {
				// no, the size is ok
				umziehen = false;
			} else {
				koord k = pos + koord(0, besch_alt->get_h());
				if (welt->lookup(k)->get_kartenboden()->hat_weg(road_wt)) {
					// we need to built a new road, thus we will use the old as a starting point (if found)
					alte_str = k;
				}
			}

			for (k.x = 0; k.x < groesse_alt.x; k.x++) {
				for (k.y = 0; k.y < groesse_alt.y; k.y++) {
					// we itereate over all tiles, since the townhalls are allowed sizes bigger than 1x1

					gr = welt->lookup(pos_alt + k)->get_kartenboden();
		DBG_MESSAGE("stadt_t::check_bau_rathaus()", "loesch %p", gr->first_obj());
					gr->obj_loesche_alle(NULL);

					if (umziehen) {
						DBG_MESSAGE("stadt_t::check_bau_rathaus()", "delete townhall tile %i,%i (gb=%p)", k.x, k.y, gb);
						// replace old space by normal houses level 0 (must be 1x1!)
						gb = hausbauer_t::neues_gebaeude(welt, NULL, gr->get_pos(), 0, hausbauer_t::get_wohnhaus(0, welt->get_timeline_year_month(), welt->get_climate(welt->max_hgt(pos))), NULL);
						add_gebaeude_to_stadt(gb);
					}
				}
			}
		}

		// Now built the new townhall
		int layout = simrand(besch->get_all_layouts() - 1);
		if (neugruendung || umziehen) {
			best_pos = rathausplatz_sucher_t(welt).suche_platz(pos, besch->get_b(layout), besch->get_h(layout) + 1, besch->get_allowed_climate_bits());
		}
		// check, if the was something found
		if(best_pos==koord::invalid) {
			dbg->error( "stadt_t::check_bau_rathaus", "no better postion found!" );
			return;
		}
		gb = hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos)->get_kartenboden()->get_pos(), layout, besch);
		DBG_MESSAGE("new townhall", "use layout=%i", layout);
		add_gebaeude_to_stadt(gb);
		DBG_MESSAGE("stadt_t::check_bau_rathaus()", "add townhall (bev=%i, ptr=%p)", buildings.get_sum_weight(),welt->lookup(best_pos)->get_kartenboden()->first_obj());

		// if not during initialization
		if (!new_town) {
			// tell the player
			char buf[256];
			sprintf( buf, translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), name, get_einwohner() );
			welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(layout, 0, 0)->get_hintergrund(0, 0, 0));
		}

		// Strasse davor verlegen
		k = koord(0, besch->get_h(layout));
		for (k.x = 0; k.x < besch->get_b(layout); k.x++) {
			if (baue_strasse(best_pos + k, NULL, true)) {
				;
			} else if(k.x==0) {
				// Hajo: Strassenbau nicht versuchen, eines der Felder
				// ist schon belegt
				alte_str == koord::invalid;
			}
		}

		if (umziehen  &&  alte_str != koord::invalid) {
			// Strasse vom ehemaligen Rathaus zum neuen verlegen.
			wegbauer_t bauer(welt, NULL);
			bauer.route_fuer(wegbauer_t::strasse, welt->get_city_road());
			bauer.calc_route(welt->lookup(alte_str)->get_kartenboden()->get_pos(), welt->lookup(best_pos + koord(0, besch->get_h(layout)))->get_kartenboden()->get_pos());
			bauer.baue();
		} else if (neugruendung) {
			lo = best_pos - koord(2, 2);
			ur = best_pos + koord(besch->get_b(layout), besch->get_h(layout)) + koord(2, 2);
		}
		// update position (where the name is)
		welt->lookup_kartenboden(pos)->set_text( NULL );
		pos = best_pos;
		welt->lookup_kartenboden(pos)->set_text( name );
	}
}


/* eventually adds a new industry
 * so with growing number of inhabitants the industry grows
 * @date 12.1.05
 * @author prissi
 */
void stadt_t::check_bau_factory(bool new_town)
{
	if (!new_town && industry_increase_every[0] > 0 && bev % industry_increase_every[0] == 0) {
		for (int i = 0; i < 8; i++) {
			if (industry_increase_every[i] == bev) {
				DBG_MESSAGE("stadt_t::check_bau_factory", "adding new industry at %i inhabitants.", get_einwohner());
				fabrikbauer_t::increase_industry_density( welt, true );
			}
		}
	}
}


gebaeude_t::typ stadt_t::was_ist_an(const koord k) const
{
	const grund_t* gr = welt->lookup_kartenboden(k);
	gebaeude_t::typ t = gebaeude_t::unbekannt;

	if (gr != NULL) {
		const gebaeude_t* gb = dynamic_cast<const gebaeude_t*>(gr->first_obj());
		if (gb != NULL) {
			t = gb->get_haustyp();
		}
	}
	return t;
}


// find out, what building matches best
void stadt_t::bewerte_res_com_ind(const koord pos, int &ind_score, int &com_score, int &res_score)
{
	koord k;

	ind_score = ind_start_score;
	com_score = com_start_score;
	res_score = res_start_score;

	for (k.y = pos.y - 2; k.y <= pos.y + 2; k.y++) {
		for (k.x = pos.x - 2; k.x <= pos.x + 2; k.x++) {
			gebaeude_t::typ t = was_ist_an(k);
			if (t != gebaeude_t::unbekannt) {
				ind_score += ind_neighbour_score[t];
				com_score += com_neighbour_score[t];
				res_score += res_neighbour_score[t];
			}
		}
	}
}


// return the eight neighbours
static koord neighbours[] = {
	koord( 0,  1),
	koord( 1,  0),
	koord( 0, -1),
	koord(-1,  0),
	// now the diagonals
	koord(-1, -1),
	koord( 1, -1),
	koord(-1,  1),
	koord( 1,  1)
};


void stadt_t::baue_gebaeude(const koord k)
{
	grund_t* gr = welt->lookup_kartenboden(k);
	const koord3d pos(gr->get_pos());

	// no covered by a downgoing monorail?
	if (gr->ist_natur() &&
			gr->kann_alle_obj_entfernen(welt->get_spieler(1)) == NULL && (
				gr->get_grund_hang() == hang_t::flach ||
				welt->lookup(koord3d(k, welt->max_hgt(k))) == NULL
			)) {
		// bisher gibt es 2 Sorten Haeuser
		// arbeit-spendende und wohnung-spendende

		int will_arbeit  = (bev - arb) / 4;  // Nur ein viertel arbeitet
		int will_wohnung = (bev - won);

		// der Bauplatz muss bewertet werden
		int passt_industrie, passt_gewerbe, passt_wohnung;
		bewerte_res_com_ind(k, passt_industrie, passt_gewerbe, passt_wohnung );

		const int sum_gewerbe   = passt_gewerbe   + will_arbeit;
		const int sum_industrie = passt_industrie + will_arbeit;
		const int sum_wohnung   = passt_wohnung   + will_wohnung;

		const uint16 current_month = welt->get_timeline_year_month();
		const haus_besch_t* h = NULL;
		const climate cl = welt->get_climate(welt->max_hgt(k));

		if (sum_gewerbe > sum_industrie  &&  sum_gewerbe > sum_wohnung) {
			h = hausbauer_t::get_gewerbe(0, current_month, cl);
			if (h != NULL) {
				arb += 20;
			}
		}

		if (h == NULL  &&  sum_industrie > sum_gewerbe  &&  sum_industrie > sum_wohnung) {
			h = hausbauer_t::get_industrie(0, current_month, cl);
			if (h != NULL) {
				arb += 20;
			}
		}

		if (h == NULL  &&  sum_wohnung > sum_industrie  &&  sum_wohnung > sum_gewerbe) {
			h = hausbauer_t::get_wohnhaus(0, current_month, cl);
			if (h != NULL) {
				// will be aligned next to a street
				won += 10;
			}
		}

		// we have something to built here ...
		if (h != NULL) {
			// check for pavement
			int streetdir = -1;
			for (int i = 0; i < 8; i++) {
				gr = welt->lookup_kartenboden(k + neighbours[i]);
				if (gr && gr->get_weg_hang() == gr->get_grund_hang()) {
					strasse_t* weg = (strasse_t*)gr->get_weg(road_wt);
					if (weg != NULL) {
						if (i < 4 && streetdir == -1) {
							// update directions (SENW)
							streetdir = i;
						}
						weg->set_gehweg(true);
						// if not current city road standard, then replace it
						if (weg->get_besch() != welt->get_city_road()) {
							if(weg->get_besitzer()!=NULL && !gr->get_depot() && !gr->is_halt()) {
								spieler_t *sp = weg->get_besitzer();
								if(sp) {
									spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());
								}
								weg->set_besitzer(NULL); // make public
							}
							weg->set_besch(welt->get_city_road());
						}
						gr->calc_bild();
						reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
					}
				}
			}

			const gebaeude_t* gb = hausbauer_t::baue(welt, NULL, pos, streetdir == -1 ? 0 : streetdir, h);
			add_gebaeude_to_stadt(gb);
		}
	}
}


void stadt_t::erzeuge_verkehrsteilnehmer(koord pos, sint32 level, koord target)
{
	const int verkehr_level = welt->get_einstellungen()->get_verkehr_level();
	if (verkehr_level > 0 && level % (17 - verkehr_level) == 0) {
		koord k;
		for (k.y = pos.y - 1; k.y <= pos.y + 1; k.y++) {
			for (k.x = pos.x - 1; k.x <= pos.x + 1; k.x++) {
				if (welt->ist_in_kartengrenzen(k)) {
					grund_t* gr = welt->lookup(k)->get_kartenboden();
					const weg_t* weg = gr->get_weg(road_wt);

					if (weg != NULL && (
								gr->get_weg_ribi_unmasked(road_wt) == ribi_t::nordsued ||
								gr->get_weg_ribi_unmasked(road_wt) == ribi_t::ostwest
							)) {
#ifdef DESTINATION_CITYCARS
						// already a car here => avoid congestion
						if(gr->obj_bei(gr->get_top()-1)->is_moving()) {
							continue;
						}
#endif
						if (!stadtauto_t::list_empty()) {
							stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), target);
							gr->obj_add(vt);
							welt->sync_add(vt);
						}
						return;
					}
				}
			}
		}
	}
}


void stadt_t::renoviere_gebaeude(gebaeude_t* gb)
{
	const gebaeude_t::typ alt_typ = gb->get_haustyp();
	if (alt_typ == gebaeude_t::unbekannt) {
		return; // only renovate res, com, ind
	}

	if (gb->get_tile()->get_besch()->get_b()*gb->get_tile()->get_besch()->get_h()!=1) {
		return; // too big ...
	}

	// hier sind wir sicher dass es ein Gebaeude ist
	const int level = gb->get_tile()->get_besch()->get_level();

	// bisher gibt es 2 Sorten Haeuser
	// arbeit-spendende und wohnung-spendende
	const int will_arbeit  = (bev - arb) / 4;  // Nur ein viertel arbeitet
	const int will_wohnung = (bev - won);

	// does the timeline allow this buildings?
	const uint16 current_month = welt->get_timeline_year_month();
	const climate cl = welt->get_climate(gb->get_pos().z);

	// der Bauplatz muss bewertet werden
	const koord k = gb->get_pos().get_2d();
	int passt_industrie;
	int passt_gewerbe;
	int passt_wohnung;
	bewerte_res_com_ind(k, passt_industrie, passt_gewerbe, passt_wohnung);

	// verlust durch abriss
	const int sum_gewerbe   = passt_gewerbe   + will_arbeit;
	const int sum_industrie = passt_industrie + will_arbeit;
	const int sum_wohnung   = passt_wohnung   + will_wohnung;

	gebaeude_t::typ will_haben = gebaeude_t::unbekannt;
	int sum = 0;

	// try to built
	const haus_besch_t* h = NULL;
	if (sum_gewerbe > sum_industrie && sum_gewerbe > sum_wohnung) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == gebaeude_t::gewerbe ? level + 1 : level);
		h = hausbauer_t::get_gewerbe(try_level, current_month, cl);
		if (h != NULL && h->get_level() >= try_level) {
			will_haben = gebaeude_t::gewerbe;
			sum = sum_gewerbe;
		}
	}
	// check for industry, also if we wanted com, but there was no com good enough ...
	if ((sum_industrie > sum_industrie && sum_industrie > sum_wohnung) || (sum_gewerbe > sum_wohnung && will_haben == gebaeude_t::unbekannt)) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == gebaeude_t::industrie ? level + 1 : level);
		h = hausbauer_t::get_industrie(try_level , current_month, cl);
		if (h != NULL && h->get_level() >= try_level) {
			will_haben = gebaeude_t::industrie;
			sum = sum_industrie;
		}
	}
	// check for residence
	// (sum_wohnung>sum_industrie  &&  sum_wohnung>sum_gewerbe
	if (will_haben == gebaeude_t::unbekannt) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == gebaeude_t::wohnung ? level + 1 : level);
		h = hausbauer_t::get_wohnhaus(try_level, current_month, cl);
		if (h != NULL && h->get_level() >= try_level) {
			will_haben = gebaeude_t::wohnung;
			sum = sum_wohnung;
		} else {
			h = NULL;
		}
	}

	if (alt_typ != will_haben) {
		sum -= level * 10;
	}

	// good enough to renovate, and we found a building?
	if (sum > 0 && h != NULL) {
//		DBG_MESSAGE("stadt_t::renoviere_gebaeude()", "renovation at %i,%i (%i level) of typ %i to typ %i with desire %i", k.x, k.y, alt_typ, will_haben, sum);

		// check for pavement
		// and make sure our house is not on a neighbouring tile, to avoid boring towns
		int streetdir = 0;
		for (int i = 0; i < 8; i++) {
			grund_t* gr = welt->lookup(k + neighbours[i])->get_kartenboden();
			if (gr != NULL && gr->get_weg_hang() == gr->get_grund_hang()) {
				strasse_t* weg = static_cast<strasse_t*>(gr->get_weg(road_wt));
				if (weg != NULL) {
					if (i < 4 && streetdir == 0) {
						// update directions (NESW)
						streetdir = i;
					}
					weg->set_gehweg(true);
					// if not current city road standard, then replace it
					if (weg->get_besch() != welt->get_city_road()) {
						if (weg->get_besitzer() != NULL && !gr->get_depot() && !gr->is_halt()) {
							spieler_t *sp = weg->get_besitzer();
							if(sp) {
								spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());
							}
							weg->set_besitzer(NULL); // make public
						}
						weg->set_besch(welt->get_city_road());
					}
					gr->calc_bild();
					reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
				} else if (gr->get_typ() == grund_t::fundament) {
					// do not renovate, if the building is already in a neighbour tile
					gebaeude_t* gb = dynamic_cast<gebaeude_t*>(gr->first_obj());
					if (gb != NULL && gb->get_tile()->get_besch() == h) {
						return;
					}
				}
			}
		}

		switch (alt_typ) {
			case gebaeude_t::wohnung:   won -= level * 10; break;
			case gebaeude_t::gewerbe:   arb -= level * 20; break;
			case gebaeude_t::industrie: arb -= level * 20; break;
			default: break;
		}

		// exchange building; try to face it to street in front
		gb->set_tile( h->get_tile(streetdir, 0, 0) );
		welt->lookup(k)->get_kartenboden()->calc_bild();
		update_gebaeude_from_stadt(gb);

		switch (will_haben) {
			case gebaeude_t::wohnung:   won += h->get_level() * 10; break;
			case gebaeude_t::gewerbe:   arb += h->get_level() * 20; break;
			case gebaeude_t::industrie: arb += h->get_level() * 20; break;
			default: break;
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
bool stadt_t::baue_strasse(const koord k, spieler_t* sp, bool forced)
{
	grund_t* bd = welt->lookup(k)->get_kartenboden();

	// water?!?
	if (bd->get_hoehe() <= welt->get_grundwasser()) {
		return false;
	}

	if (bd->get_typ() != grund_t::boden) {
		// not on monorails, foundations, tunnel or bridges
		return false;
	}

	// we must not built on water or runways etc.
	if (bd->hat_wege() && !bd->hat_weg(road_wt) && !bd->hat_weg(track_wt)) {
		return false;
	}

	// somebody else's things on it?
	if (bd->kann_alle_obj_entfernen(welt->get_spieler(1))) {
		return false;
	}

	// initially allow all possible directions ...
	ribi_t::ribi allowed_dir = (bd->get_grund_hang() != hang_t::flach ? ribi_t::doppelt(ribi_typ(bd->get_weg_hang())) : (ribi_t::ribi)ribi_t::alle);
	ribi_t::ribi connection_roads = ribi_t::keine;

	// we have here a road: check for four corner stops
	const gebaeude_t* gb = bd->find<gebaeude_t>();
	if(gb) {
		// nothing to connect
		if(gb->get_tile()->get_besch()->get_all_layouts()==4) {
			// single way
			allowed_dir = ribi_t::layout_to_ribi[gb->get_tile()->get_layout()];
		}
		else if(gb->get_tile()->get_besch()->get_all_layouts()) {
			// through way
			allowed_dir = ribi_t::doppelt( ribi_t::layout_to_ribi[gb->get_tile()->get_layout()] );
		}
		else {
			dbg->error("stadt_t::baue_strasse()", "building on road with not directions at %i,%i?!?", k.x, k.y );
		}
	}

	// we must not built on water or runways etc.
	// only crossing or tramways allowed
	if (bd->hat_weg(track_wt)) {
		weg_t* sch = bd->get_weg(track_wt);
		if (sch->get_besch()->get_styp() != 7) {
			// not a tramway
			ribi_t::ribi r = sch->get_ribi_unmasked();
			if (!ribi_t::ist_gerade(r)) {
				// no building on crossings, curves, dead ends
				return false;
			}
			// just the other directions are allowed
			allowed_dir &= ~r;
		}
	}

	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & allowed_dir) {
			// now we have to check for several problems ...
			grund_t* bd2;
			if(bd->get_neighbour(bd2, invalid_wt, koord::nsow[r])) {
				if(bd2->get_typ()==grund_t::fundament) {
					// not connecting to a building of course ...
					allowed_dir &= ~ribi_t::nsow[r];
				} else if (bd2->get_typ()!=grund_t::boden  &&  ribi_t::nsow[r]!=ribi_typ(bd2->get_grund_hang())) {
					// not the same slope => tunnel or bridge
					allowed_dir &= ~ribi_t::nsow[r];
				} else if(bd2->hat_weg(road_wt)) {
					const gebaeude_t* gb = bd2->find<gebaeude_t>();
					if(gb) {
						uint8 layouts = gb->get_tile()->get_besch()->get_all_layouts();
						// nothing to connect
						if(layouts==4) {
							// single way
							if(ribi_t::nsow[r]!=ribi_t::rueckwaerts(ribi_t::layout_to_ribi[gb->get_tile()->get_layout()])) {
								allowed_dir &= ~ribi_t::nsow[r];
							}
							else {
								// otherwise allowed ...
								connection_roads |= ribi_t::nsow[r];
							}
						}
						else if(layouts==2 || layouts==8 || layouts==16) {
							// through way
							if((ribi_t::doppelt( ribi_t::layout_to_ribi[gb->get_tile()->get_layout()] )&ribi_t::nsow[r])==0) {
								allowed_dir &= ~ribi_t::nsow[r];
							}
							else {
								// otherwise allowed ...
								connection_roads |= ribi_t::nsow[r];
							}
						}
						else {
							dbg->error("stadt_t::baue_strasse()", "building on road with not directions at %i,%i?!?", k.x, k.y );
						}
					}
					else if(bd2->get_depot()) {
						// do not enter depots
						allowed_dir &= ~ribi_t::nsow[r];
					}
					else {
						// otherwise allowed ...
						connection_roads |= ribi_t::nsow[r];
					}
				}
			}
			else {
				// illegal slope ...
				allowed_dir &= ~ribi_t::nsow[r];
			}
		}
	}

	// now add the ribis to the other ways (if there)
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & connection_roads) {
			grund_t* bd2 = welt->lookup(k + koord::nsow[r])->get_kartenboden();
			weg_t* w2 = bd2->get_weg(road_wt);
			w2->ribi_add(ribi_t::rueckwaerts(ribi_t::nsow[r]));
			bd2->calc_bild();
			bd2->set_flag( grund_t::dirty );
		}
	}

	if (connection_roads != ribi_t::keine || forced) {

		if (!bd->weg_erweitern(road_wt, connection_roads)) {
			strasse_t* weg = new strasse_t(welt);
			// Hajo: city roads should not belong to any player => so we can ignore any contruction costs ...
			weg->set_besch(welt->get_city_road());
			weg->set_gehweg(true);
			bd->neuen_weg_bauen(weg, connection_roads, sp);
			bd->calc_bild();	// otherwise the
		}
		return true;
	}

	return false;
}


void stadt_t::baue()
{
	// will check a single random pos in the city, then baue will be called
	const koord k(lo + koord(simrand(ur.x - lo.x + 1), simrand(ur.y - lo.y + 1)));

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr==NULL) {
		return;
	}

	// checks only make sense on empty ground
	if(gr->ist_natur()) {

		// since only a single location is checked, we can stop after we have found a positive rule
		best_strasse.reset(k);
		int offset = simrand(num_road_rules);	// start with random rule
		for (int i = 0; i < num_road_rules  &&  !best_strasse.found(); i++) {
			int rule = ( i+offset ) % num_road_rules;
			bewerte_strasse(k, 8 + road_rules[rule].chance, road_rules[rule].rule);
		}
		// ok => then built road
		if (best_strasse.found()) {
			baue_strasse(best_strasse.get_pos(), NULL, false);
			INT_CHECK("simcity 1156");
			return;
		}

		// not good for road => test for house

		// since only a single location is checked, we can stop after we have found a positive rule
		best_haus.reset(k);
		offset = simrand(num_house_rules);	// start with random rule
		for (int i = 0; i < num_house_rules  &&  !best_haus.found(); i++) {
			int rule = ( i+offset ) % num_house_rules;
			bewerte_haus(k, 8 + house_rules[rule].chance, house_rules[rule].rule);
		}
		// one rule applied?
		if (best_haus.found()) {
			baue_gebaeude(best_haus.get_pos());
			INT_CHECK("simcity 1163");
			return;
		}

	}

	// renovation (only done when nothing matches a certain location
	if (!buildings.empty()  &&  simrand(100) <= renovation_percentage) {
		renoviere_gebaeude(buildings[simrand(buildings.get_count())]);
		INT_CHECK("simcity 876");
	}
}


// geeigneten platz zur Stadtgruendung durch Zufall ermitteln
vector_tpl<koord>* stadt_t::random_place(
	const karte_t* wl, const sint32 anzahl, sint16 old_x, sint16 old_y)
{
	int cl = 0;
	for (int i = 0; i < MAX_CLIMATES; i++) {
		if (hausbauer_t::get_special(0, haus_besch_t::rathaus, 0, 0, (climate)i)) {
			cl |= (1 << i);
		}
	}
	DBG_DEBUG("karte_t::init()", "get random places in climates %x", cl);
	slist_tpl<koord>* list = wl->finde_plaetze(2, 3, (climate_bits)cl, old_x, old_y);
	DBG_DEBUG("karte_t::init()", "found %i places", list->get_count());
	vector_tpl<koord>* result = new vector_tpl<koord>(anzahl);

	for (int i = 0; i < anzahl; i++) {
		int len = list->get_count();
		// check distances of all cities to their respective neightbours
		while (len > 0) {
			int minimum_dist = 0x7FFFFFFF;  // init with maximum
			koord k;
			const int index = simrand(len);
			k = list->at(index);
			list->remove(k);
			len--;

			// check minimum distance
			for (int j = 0; (j < i) && minimum_dist > minimum_city_distance; j++) {
				int dist = abs(k.x - (*result)[j].x) + abs(k.y - (*result)[j].y);
				if (minimum_dist > dist) {
					minimum_dist = dist;
				}
			}
			if (minimum_dist > minimum_city_distance) {
				// all citys are far enough => ok, find next place
				result->append(k);
				break;
			}
			// if we reached here, the city was not far enough => try again
		}

		if (len <= 0 && i < anzahl - 1) {
			dbg->warning("stadt_t::random_place()", "Not enough places found!");
			break;
		}
	}
	list->clear();
	delete list;

	return result;
}
