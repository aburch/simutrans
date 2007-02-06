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
#include "simware.h"
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

#include "tpl/minivec_tpl.h"

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
						if (gr->gib_typ() != grund_t::fundament) return false;
						break;
					case 'H':
						// no house
						if (gr->gib_typ() == grund_t::fundament) return false;
						break;
					case 'n':
						// nature/empty
						if (!gr->ist_natur() || gr->kann_alle_obj_entfernen(NULL) != NULL) return false;
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
int stadt_t::bewerte_pos(const koord pos, const char* regel)
{
	int w;
	w  = bewerte_loc(pos, regel,   0);
	w += bewerte_loc(pos, regel,  90);
	w += bewerte_loc(pos, regel, 180);
	w += bewerte_loc(pos, regel, 270);
	return w;
}


void stadt_t::bewerte_strasse(koord k, int rd, const char* regel)
{
	if (simrand(rd) == 0) {
		best_strasse.check(k, bewerte_pos(k, regel));
	}
}


void stadt_t::bewerte_haus(koord k, int rd, const char* regel)
{
	if (simrand(rd) == 0) {
		best_haus.check(k, bewerte_pos(k, regel));
	}
}


/*
 * Zeichen in Regeln:
 * S = darf keine Strasse sein
 * s = muss Strasse sein
 * n = muss Natur sein
 * t = muss Stop sein ...
 * . = beliebig
 */
void stadt_t::bewerte()
{
	// Zufallspos im Stadtgebiet
	const int speed = 8;
	const koord k(lo + koord(simrand(ur.x - lo.x + 1), simrand(ur.y - lo.y + 1)));

	best_haus.reset(k);
	best_strasse.reset(k);
	for (int i = 0; i < num_house_rules; i++) {
		bewerte_haus(k, speed + house_rules[i].chance, house_rules[i].rule);
	}

	for (int i = 0; i < num_road_rules; i++) {
		bewerte_strasse(k, speed + road_rules[i].chance, road_rules[i].rule);
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
		while (*rule == ' ') rule++;

		// find out rule size
		int size = 0;
		int maxlen = strlen(rule);
		while (size < maxlen && rule[size] != ' ') size++;

		if (size > 7 || maxlen < size * (size + 1) - 1 || (size & 1) == 0 || size <= 2) {
			dbg->fatal("stadt_t::cityrules_init()", "house rule %d has bad format!", i + 1);
		}

		// put rule into memory
		const uint8 offset = (7 - size) / 2;
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
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
		while (*rule == ' ') rule++;

		// find out rule size
		int size = 0;
		int maxlen = strlen(rule);
		while (size < maxlen && rule[size] != ' ') size++;

		if (size > 7 || maxlen < size * (size + 1) - 1 || (size & 1) == 0 || size <= 2) {
			dbg->fatal("stadt_t::cityrules_init()", "road rule %d has bad format!", i + 1);
		}

		// put rule into memory
		const uint8 offset = (7 - size) / 2;
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				road_rules[i].rule[(offset + y) * 7 + x + offset] = rule[x + y * (size + 1)];
			}
		}
	}
	return true;
}



/*************************************************** create stop names ****************************************************/

static const char* const zentrum_namen[] =
{
// "%s %s", "%s Haupt %s", "%s Zentral %s", "%s %s Mitte", "%s Transfer %s"
	"1center", "2center", "3center", "4center", "5center", "6center", "7center"
};

static const char* const nord_namen[] =
{
// "%s Nord %s"
	"1nord"
};

static const char* const nordost_namen[] =
{
// "%s Nordost %s"
	"1nordost"
};

static const char* const ost_namen[] =
{
// "%s Ost %s"
	"1ost"
};

static const char* const suedost_namen[] =
{
// "%s Südost %s"
	"1suedost"
};

static const char* const sued_namen[] =
{
// "%s Süd %s"
	"1sued"
};

static const char* const suedwest_namen[] =
{
// "%s Südwest %s"
	"1suedwest"
};

static const char* const west_namen[] =
{
// "%s West %s"
	"1west"
};

static const char* const nordwest_namen[] =
{
// "%s Nordwest %s"
	"1nordwest"
};

static const char* const aussen_namen[] =
{
// "%s Zweig %s", "%s Neben %s", "%s Land %s", "%s Außen %s"
	"1extern", "2extern", "3extern", "4extern",
};


/**
 * Creates a station name
 * @param number if >= 0, then a number is added to the name
 * @author Hj. Malthaner
 */
const char *
stadt_t::haltestellenname(koord k, const char *typ, int number)
{
	const char* num_city_base = "%s city %d %s";
	const char* num_land_base = "%s land %d %s";
	const char* base = "1center";	// usually gets to %s %s
	const char* building = NULL;
	bool outside = false;
	char buf[512]; // temperary name storage

	if (number >= 0 && umgebung_t::numbered_stations) {
		// numbered stations here
		int li_gr = lo.x + 2;
		int re_gr = ur.x - 2;
		int ob_gr = lo.y + 2;
		int un_gr = ur.y - 2;

		if (li_gr < k.x && re_gr > k.x && ob_gr < k.y && un_gr > k.y) {
			zentrum_namen_cnt++;
			sprintf(buf, translator::translate(num_city_base),
				this->name,
				zentrum_namen_cnt,
				translator::translate(typ)
			);
		} else {
			aussen_namen_cnt++;
			sprintf(buf, translator::translate(num_land_base),
				this->name,
				aussen_namen_cnt,
				translator::translate(typ)
			);
		}
	} else {
		// standard names:
		// order: factory, attraction, direction, normal name

		// prissi: first we try a factory name
		halthandle_t halt = haltestelle_t::gib_halt(welt, k);
		if (halt.is_bound()) {
			// first factories (so with same distance, they have priority)
			int this_distance = 999;
			slist_iterator_tpl<fabrik_t*> fab_iter(halt->gib_fab_list());
			while (fab_iter.next()) {
				int distance = abs_distance(fab_iter.get_current()->gib_pos().gib_2d(), k);
				if (distance < this_distance) {
					building = fab_iter.get_current()->gib_name();
					distance = this_distance;
				}
			}
			// check for other special building (townhall, monument, tourst attraction)
			const sint16 catchment_area = welt->gib_einstellungen()->gib_station_coverage();
			for (int x = -catchment_area; x <= catchment_area; x++) {
				for (int y = -catchment_area; y <= catchment_area; y++) {
					const planquadrat_t* plan = welt->lookup(koord(x, y) + k);
					int distance = abs(x) + abs(y);
					if (plan != NULL && abs(x + y) < this_distance) {
						gebaeude_t* gb = dynamic_cast<gebaeude_t*>(plan->gib_kartenboden()->suche_obj(ding_t::gebaeude));
						if (gb != NULL) {
							if (gb->is_monument()) {
								building = translator::translate(gb->gib_name());
								this_distance = distance;
							} else if (gb->ist_rathaus() ||
									gb->gib_tile()->gib_besch()->gib_utyp() == hausbauer_t::sehenswuerdigkeit  || // land attraction
									gb->gib_tile()->gib_besch()->gib_utyp() == hausbauer_t::special) { // town attraction
								building = make_single_line_string(translator::translate(gb->gib_tile()->gib_besch()->gib_name()), 2);
								this_distance = distance;
							}
						}
					}
				}
			}
		}

		// no factory found => take the usual name scheme
		if (building == NULL) {
			int li_gr = lo.x + 2;
			int re_gr = ur.x - 2;
			int ob_gr = lo.y + 2;
			int un_gr = ur.y - 2;

			if (li_gr < k.x && re_gr > k.x && ob_gr < k.y && un_gr > k.y) {
				base = zentrum_namen[zentrum_namen_cnt % lengthof(zentrum_namen)];
				zentrum_namen_cnt++;
			} else if (li_gr - 6 < k.x && re_gr + 6 > k.x && ob_gr - 6 < k.y && un_gr + 6 > k.y) {
				if (k.y < ob_gr) {
					if (k.x < li_gr) {
						base = nordwest_namen[0];
					} else if (k.x > re_gr) {
						base = nordost_namen[0];
					} else {
						base = nord_namen[0];
					}
				} else if (k.y > un_gr) {
					if (k.x < li_gr) {
						base = suedwest_namen[0];
					} else if (k.x > re_gr) {
						base = suedost_namen[0];
					} else {
						base = sued_namen[0];
					}
				} else {
					if (k.x <= li_gr) {
						base = west_namen[0];
					} else if (k.x >= re_gr) {
						base = ost_namen[0];
					} else {
						base = zentrum_namen[zentrum_namen_cnt % lengthof(zentrum_namen)];
						zentrum_namen_cnt++;
					}
				}
			} else {
				base = aussen_namen[aussen_namen_cnt % lengthof(aussen_namen)];
				aussen_namen_cnt++;
				outside = true;
			}
		}

		// compose the name
		if (building == NULL) {
			sprintf(buf, translator::translate(base),
				this->name,
				translator::translate(typ)
			);
		} else {
			// with factories
			sprintf(buf, translator::translate("%s building %s %s"),
				this->name,
				building,
				translator::translate(typ)
			);
		}
	}

	const long len = strlen(buf) + 1;
	char* name = new char[len];
	tstrncpy(name, buf, len);

	return name;
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

			const grund_t* gr = plan->gib_kartenboden();
			if (((1 << welt->get_climate(gr->gib_hoehe())) & cl) == 0) {
				return false;
			}

			if (ist_randfeld(d)) {
				return
					gr->gib_grund_hang() == hang_t::flach &&     // Flach
					gr->gib_typ() == grund_t::boden &&           // Boden -> keine GEbäude
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // Höchstens Strassen
					gr->kann_alle_obj_entfernen(NULL) == NULL;   // Irgendwas verbaut den Platz?
			} else {
				return
					gr->gib_grund_hang() == hang_t::flach &&
					gr->gib_typ() == grund_t::boden &&
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

			if (((1 << welt->get_climate(gr->gib_hoehe())) & cl) == 0) {
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
					gr->gib_grund_hang() == hang_t::flach &&
					gr->gib_typ() == grund_t::boden &&
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // Höchstens Strassen
					!gr->is_halt() &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			} else {
				// Hier soll das Haus hin - wir ersetzen auch andere Gebäude, aber keine Wege!
				return
					gr->gib_grund_hang() == hang_t::flach && (
						gr->gib_typ() == grund_t::boden && gr->ist_natur() ||
						gr->gib_typ() == grund_t::fundament
					) &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			}
		}
};


// this function adds houses to the city house list
void stadt_t::add_gebaeude_to_stadt(const gebaeude_t* gb)
{
	if (gb != NULL) {
		const haus_tile_besch_t* tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse(tile->gib_layout());
		koord pos = gb->gib_pos().gib_2d() - tile->gib_offset();
		koord k;

		// add all tiles
		for (k.y = 0; k.y < size.y; k.y++) {
			for (k.x = 0; k.x < size.x; k.x++) {
				gebaeude_t* add_gb = dynamic_cast<gebaeude_t*>(welt->lookup(pos + k)->gib_kartenboden()->first_obj());
//				DBG_MESSAGE("stadt_t::add_gebaeude_to_stadt()", "geb=%p at (%i,%i)", add_gb, pos.x + k.x, pos.y + k.y);
				buildings.append(add_gb, tile->gib_besch()->gib_level() + 1, 16);
				add_gb->setze_stadt(this);
			}
		}
		// check borders
		pruefe_grenzen(pos);
		if(size!=koord(1,1)) {
			pruefe_grenzen(pos+size+koord(-1,-1));
		}
	}
}



// this function removes houses from the city house list
void stadt_t::remove_gebaeude_from_stadt(const gebaeude_t* gb)
{
	buildings.remove(gb);
	recalc_city_size();
}



// recalculate house informations (used for target selection)
void stadt_t::recount_houses()
{
	DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);
	buildings.clear();
	for (sint16 y = lo.y; y <= ur.y; y++) {
		for (sint16 x = lo.x; x <= ur.x; x++) {
			const grund_t* gr = welt->lookup_kartenboden(koord(x, y));
			gebaeude_t* gb = dynamic_cast<gebaeude_t*>(gr->first_obj());
			if (gb != NULL && (
						gb->ist_rathaus() ||
						gb->gib_haustyp() != gebaeude_t::unbekannt ||
						gb->is_monument()
					) &&
					welt->suche_naechste_stadt(koord(x, y)) == this) {
				// no attraction, just normal buidlings or townhall
				buildings.append(gb, gb->gib_tile()->gib_besch()->gib_level() + 1, 16);
				gb->setze_stadt(this);
			}
		}
	}
	DBG_MESSAGE("recount_houses()", "%s has %i bev", gib_name(), gib_einwohner());
}



// recalculate the spreading of a city
// will be updated also after house deletion
void stadt_t::recalc_city_size()
{
	lo = koord(welt->gib_groesse_x(), welt->gib_groesse_y());
	ur = koord(0, 0);
	for (uint i = 0; i < buildings.get_count(); i++) {
		const koord pos = buildings[i]->gib_pos().gib_2d();
		if (lo.x > pos.x) lo.x = pos.x;
		if (lo.y > pos.y) lo.y = pos.y;
		if (ur.x < pos.x) ur.x = pos.x;
		if (ur.y < pos.y) ur.y = pos.y;
	}

	lo.x -= 1;
	lo.y -= 1;
	ur.x += 1;
	ur.y += 1;

	if (lo.x < 0) {
		lo.x = 0;
	}
	if (lo.y < 0) {
		lo.y = 0;
	}
	if (ur.x >= welt->gib_groesse_x()) {
		ur.x = welt->gib_groesse_x();
	}
	if (ur.y >= welt->gib_groesse_y()) {
		ur.y = welt->gib_groesse_y();
	}
}



void stadt_t::init_pax_ziele()
{
	const int gr_x = welt->gib_groesse_x();
	const int gr_y = welt->gib_groesse_y();

	for (int j = 0; j < 128; j++) {
		for (int i = 0; i < 128; i++) {
			const koord pos(i * gr_x / 128, j * gr_y / 128);
			const grund_t* gr = welt->lookup(pos)->gib_kartenboden();
//			DBG_MESSAGE("stadt_t::init_pax_ziele()", "at %i,%i = (%i,%i)", i, j, pos.x, pos.y);
			pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i, j) = reliefkarte_t::calc_relief_farbe(gr);
//			pax_ziele_alt.at(i, j) = pax_ziele_neu.at(i, j) = 0;
		}
	}
}


stadt_t::~stadt_t()
{
	// close window if needed
	if (stadt_info != NULL) {
		destroy_win(stadt_info);
	}
	// remove city info and houses
	while (!buildings.empty()) {
		// old buildings are not where they think they are, so we ask for map floor
		grund_t* gr = welt->lookup_kartenboden(buildings.front()->gib_pos().gib_2d());
		if (gr != NULL) {
			koord pos = gr->gib_pos().gib_2d();
//			delete buildings.front();
			gr->obj_loesche_alle(welt->gib_spieler(1));
			uint8 new_slope = (gr->gib_hoehe() == welt->min_hgt(pos) ? 0 : welt->calc_natural_slope(pos));
			welt->access(pos)->kartenboden_setzen(new boden_t(welt, koord3d(pos, welt->min_hgt(pos)), new_slope));
		} else {
			buildings.remove_at(0);
		}
	}
}


stadt_t::stadt_t(karte_t* wl, spieler_t* sp, koord pos, int citizens) :
	buildings(16),
	pax_ziele_alt(128, 128),
	pax_ziele_neu(128, 128),
	arbeiterziele(4)
{
	welt = wl;
	assert(wl->ist_in_kartengrenzen(pos));

	step_count = 0;
	next_step = 0;
	step_interval = 1;
	next_bau_step = 0;

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

	DBG_MESSAGE("stadt_t::stadt_t()", "Welt %p", welt);
	fflush(NULL);
	/* get a unique cityname */
	/* 9.1.2005, prissi */
	const weighted_vector_tpl<stadt_t*>& staedte = welt->gib_staedte();
	const int anz_staedte = staedte.get_count();
	const int name_list_count = translator::get_count_city_name();

	DBG_MESSAGE("stadt_t::stadt_t()", "number of towns %i", anz_staedte);
	fflush(NULL);
	// start at random position
	int start_cont = simrand(name_list_count);
	const char* list_name;

	list_name = translator::get_city_name(start_cont);
	for (int i = 0; i < name_list_count; i++) {
		// get a name
		list_name = translator::get_city_name(start_cont + i);
		// check if still unused
		for (int j = 0; j < anz_staedte; j++) {
			// noch keine stadt mit diesem namen?
			if (strcmp(list_name, staedte[j]->gib_name()) == 0) goto next_name;
		}
		DBG_MESSAGE("stadt_t::stadt_t()", "'%s' is unique", list_name);
		break;
next_name:;
	}
	strcpy(name, list_name);
	DBG_MESSAGE("stadt_t::stadt_t()", "founding new city named '%s'", name);

	// 1. Rathaus bei 0 Leuten bauen
	check_bau_rathaus(true);

	wachstum = 0;

	// this way, new cities are properly recognized
	pax_transport = citizens;
	pax_erzeugt = 0;

	// initialize history array
	for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
		for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_year[year][hist_type] = 0;
		}
	}
	for (uint month = 0; month < MAX_CITY_HISTORY_YEARS; month++) {
		for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_month[month][hist_type] = 0;
		}
	}
	city_history_year[0][HIST_CITICENS]  = last_year_bev  = gib_einwohner();
	city_history_month[0][HIST_CITICENS] = last_month_bev = gib_einwohner();
	this_year_transported = 0;
	this_year_pax = 0;

	stadt_info = NULL;
}



stadt_t::stadt_t(karte_t* wl, loadsave_t* file) :
	buildings(16),
	pax_ziele_alt(128, 128),
	pax_ziele_neu(128, 128),
	arbeiterziele(4)
{
	welt = wl;
	step_count = 0;
	next_step = 0;
	step_interval = 1;
	next_bau_step = 0;

	pax_erzeugt = 0;
	pax_transport = 0;

	wachstum = 0;

	rdwr(file);

	verbinde_fabriken();

	stadt_info = NULL;
}


void stadt_t::rdwr(loadsave_t* file)
{
	int besitzer_n;

	if (file->is_saving()) {
		besitzer_n = welt->sp2num(besitzer_p);
	}
	file->rdwr_str(name, 31);
	pos.rdwr(file);
	file->rdwr_delim("Plo: ");
	uint32 lli = lo.x;
	uint32 lob = lo.y;
	uint32 lre = ur.x;
	uint32 lun = ur.y;
	file->rdwr_long(lli, " ");
	file->rdwr_long(lob, "\n");
	file->rdwr_delim("Pru: ");
	file->rdwr_long(lre, " ");
	file->rdwr_long(lun, "\n");
	lo.x = lli;
	lo.y = lob;
	ur.x = lre;
	ur.y = lun;
	file->rdwr_delim("Bes: ");
	file->rdwr_long(besitzer_n, "\n");
	file->rdwr_long(bev, " ");
	file->rdwr_long(arb, " ");
	file->rdwr_long(won, "\n");
	file->rdwr_long(zentrum_namen_cnt, " ");
	file->rdwr_long(aussen_namen_cnt, "\n");

	if (file->is_loading()) {
		besitzer_p = welt->gib_spieler(besitzer_n);
	}

	// we probably need to save the city history
	if (file->get_version() < 86000) {
		DBG_DEBUG("stadt_t::rdwr()", "is old version: No history!");
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
		city_history_year[0][HIST_CITICENS] = last_year_bev  = gib_einwohner();
		city_history_year[0][HIST_CITICENS] = last_month_bev = gib_einwohner();
		this_year_transported = 0;
		this_year_pax = 0;
	} else {
		// 86.00.0 introduced city history
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
		// since we add it internally
		file->rdwr_long(last_year_bev, " ");
		file->rdwr_long(last_month_bev, " ");
		file->rdwr_long(this_year_transported, " ");
		file->rdwr_long(this_year_pax, "\n");
	}

	// 08-Jan-03: Due to some bugs in the special buildings/town hall
	// placement code, li,re,ob,un could've gotten irregular values
	// If a game is loaded, the game might suffer from such an mistake
	// and we need to correct it here.
	DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);

	if(lo==koord(0,0)) {
		lo = pos - koord(33,33);
	}
	if(ur==koord(0,0)) {
		ur = pos + koord(33,33);
	}

	if (lo.x < 0) lo.x = max(0, pos.x - 33);
	if (ur.x >= welt->gib_groesse_x() - 1) ur.x = min(welt->gib_groesse_x() - 2, pos.x + 33);

	if (lo.y < 0) lo.y = max(0, pos.y - 33);
	if (ur.y >= welt->gib_groesse_y() - 1) ur.y = min(welt->gib_groesse_y() - 2, pos.y + 33);
}


/**
 * Wird am Ende der Laderoutine aufgerufen, wenn die Welt geladen ist
 * und nur noch die Datenstrukturenneu verknüpft werden müssen.
 * @author Hj. Malthaner
 */
void stadt_t::laden_abschliessen()
{
	// very young city => need to grow again
	if (pax_transport > 0 && pax_erzeugt == 0) {
		step_bau();
		pax_transport = 0;
	}

	welt->lookup(pos)->gib_kartenboden()->setze_text(name);

	verbinde_fabriken();
	init_pax_ziele();
	recount_houses();

	// init step counter with meaningful value
	step_interval = (2 << 18u) / (buildings.get_count() * 4 + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	recalc_city_size();

	next_step = 0;
	next_bau_step = 0;
}


/* returns the money dialoge of a city
 * @author prissi
 */
stadt_info_t* stadt_t::gib_stadt_info(void)
{
	if (stadt_info == NULL) {
		stadt_info = new stadt_info_t(this);
	}
	return stadt_info;
}


/* add workers to factory list */
void stadt_t::add_factory_arbeiterziel(fabrik_t* fab)
{
	// no fish swarms ...
	if (strcmp("fish_swarm", fab->gib_besch()->gib_name()) != 0) {
		const koord k = fab->gib_pos().gib_2d() - pos;

		// worker do not travel more than 50 tiles
		if (k.x * k.x + k.y * k.y < CONNECT_TO_TOWN_SQUARE_DISTANCE) {
			DBG_MESSAGE("stadt_t::add_factory_arbeiterziel()", "found %s with level %i", fab->gib_name(), fab->gib_besch()->gib_pax_level());
			fab->add_arbeiterziel(this);
			// do not add a factory twice!
			arbeiterziele.append_unique(fab, fab->gib_besch()->gib_pax_level(), 4);
		}
	}
}


/* calculates the factories which belongs to certain cities */
/* we connect all factories, which are closer than 50 tiles radius */
void stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "search factories near %s (center at %i,%i)", gib_name(), pos.x, pos.y);

	slist_iterator_tpl<fabrik_t*> fab_iter(welt->gib_fab_list());
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
	step_bau();
	pax_erzeugt = 0;
	DBG_MESSAGE("stadt_t::change_size()", "%i + %i", bev, delta_citicens);
	if (delta_citicens > 0) {
		pax_transport = delta_citicens;
		step_bau();
		pax_transport = 0;
	}
	if (delta_citicens < 0) {
		pax_transport = 0;
		if (bev > -delta_citicens) {
			bev += delta_citicens;
		} else {
//			remove_city();
			bev = 0;
		}
		step_bau();
	}
	DBG_MESSAGE("stadt_t::change_size()", "%i+%i", bev, delta_citicens);
}


void stadt_t::step(long delta_t)
{
	// Ist es Zeit für einen neuen step?
	next_step += delta_t;
	next_bau_step += delta_t;

	step_interval = (8 << 18u) / (buildings.get_count() * umgebung_t::passenger_factor + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	// create passenger rate proportional to town size
	while (step_interval < next_step) {
		step_passagiere();
		step_count++;
		next_step -= step_interval;
	}

	// construction with a fixed rate (otherwise the towns would grow with different speed)
	while (stadt_t::step_bau_interval < next_bau_step) {
		step_bau();
		next_bau_step -= stadt_t::step_bau_interval;
	}

	// update history (might be changed do to construction/destroying of houses)
	city_history_month[0][HIST_CITICENS] = gib_einwohner();	// total number
	city_history_year[0][HIST_CITICENS] = gib_einwohner();

	city_history_month[0][HIST_GROWTH] = gib_einwohner()-last_month_bev;	// growth
	city_history_year[0][HIST_GROWTH] = gib_einwohner()-last_year_bev;

	city_history_month[0][HIST_TRANSPORTED] = pax_transport;	// pax transported
	city_history_year[0][HIST_TRANSPORTED] = this_year_transported+pax_transport;

	city_history_month[0][HIST_GENERATED] = pax_erzeugt;	// and all post and passengers generated
	city_history_year[0][HIST_GENERATED] = this_year_pax+pax_erzeugt;
}


/* updates the city history
 * @author prissi
 */
void stadt_t::roll_history()
{
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
	for (int i = MAX_CITY_HISTORY_MONTHS - 1; i > 0; i--) {
		for (int hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
			city_history_month[i][hist_type] = city_history_month[i - 1][hist_type];
		}
	}
	// init this month
	for (int hist_type = 1; hist_type < MAX_CITY_HISTORY; hist_type++) {
		city_history_month[0][hist_type] = 0;
	}
	city_history_month[0][0] = gib_einwohner();

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


void stadt_t::neuer_monat()
{
	const int gr_x = welt->gib_groesse_x();
	const int gr_y = welt->gib_groesse_y();

	pax_ziele_alt.copy_from(pax_ziele_neu);
	for (int j = 0; j < 128; j++) {
		for (int i = 0; i < 128; i++) {
			const koord pos(i * gr_x / 128, j * gr_y / 128);
			const grund_t* gr = welt->lookup(pos)->gib_kartenboden();
			pax_ziele_neu.at(i, j) = pax_ziele_neu.at(i, j) = reliefkarte_t::calc_relief_farbe(gr);
		}
	}

	roll_history();

	pax_erzeugt = 0;
	pax_transport = 0;
}


void stadt_t::step_bau()
{
	bool new_town = (bev == 0);

	// prissi: growth now size dependent
	if (pax_erzeugt == 0) {
		wachstum = pax_transport;
	} else if (bev < 1000) {
		wachstum = (pax_transport * 2) / (pax_erzeugt + 1);
	} else if (bev < 10000) {
		wachstum = (pax_transport * 4) / (pax_erzeugt + 1);
	} else {
		wachstum = (pax_transport * 8) / (pax_erzeugt + 1);
	}

	// Hajo: let city grow in steps of 1
//	for (int n = 0; n < (1 + wachstum); n++) {
	// @author prissi: No growth without development
	for (int n = 0; n < wachstum; n++) {
		bev++; // Hajo: bevoelkerung wachsen lassen

		INT_CHECK("simcity 338");

		for (int i = 0; i < 30 && bev * 2 > won + arb + 100; i++) {
			bewerte();
			INT_CHECK("simcity 271");
			baue();
			INT_CHECK("simcity 273");
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

	// restart at first buiulding?
	if (step_count >= buildings.get_count()) {
		step_count = 0;
	}
	const gebaeude_t* gb = buildings[step_count];

	// prissi: since now backtravels occur, we damp the numbers a little
	const int num_pax =
		(wtyp == warenbauer_t::passagiere) ?
			(gb->gib_tile()->gib_besch()->gib_level()      + 6) >> 2 :
			(gb->gib_tile()->gib_besch()->gib_post_level() + 5) >> 2;

	// create pedestrians in the near area?
	if (umgebung_t::fussgaenger && wtyp == warenbauer_t::passagiere) {
		haltestelle_t::erzeuge_fussgaenger(welt, gb->gib_pos(), num_pax);
		INT_CHECK("simcity 269");
	}

	// suitable start search
	const koord k = gb->gib_pos().gib_2d();
	const planquadrat_t* plan = welt->lookup(k);
	const halthandle_t* halt_list = plan->get_haltlist();

	// suitable start search
	halthandle_t start_halt = halthandle_t();
	for (uint h = 0; h < plan->get_haltlist_count(); h++) {
		halthandle_t halt = halt_list[h];
		if (halt->is_enabled(wtyp) && halt->gib_ware_summe(wtyp) <= halt->get_capacity()) {
			start_halt = halt;
			break;
		}
	}

	// Hajo: track number of generated passengers.
	pax_erzeugt += num_pax;

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
				erzeuge_verkehrsteilnehmer(start_halt->gib_basis_pos(), step_count, ziel);
			}
#endif
			// Dario: Check if there's a stop near destination
			const planquadrat_t* dest_plan = welt->lookup(ziel);
			const halthandle_t* dest_list = dest_plan->get_haltlist();
			bool can_walk_ziel = false;

			// suitable end search
			unsigned ziel_count = 0;
			for (uint h = 0; h < dest_plan->get_haltlist_count(); h++) {
				halthandle_t halt = dest_list[h];
				if (halt->is_enabled(wtyp)) {
					ziel_count++;
					if (halt == start_halt) {
						can_walk_ziel = true;
						break; // because we found at least one valid step ...
					}
				}
			}

			if (ziel_count == 0) {
// DBG_MESSAGE("stadt_t::step_passagiere()", "No stop near dest (%d, %d)", ziel.x, ziel.y);
				// Thus, routing is not possible and we do not need to do a calculation.
				// Mark ziel as destination without route and continue.
				merke_passagier_ziel(ziel, COL_DARK_ORANGE);
				start_halt->add_pax_no_route(pax_left_to_do);
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				erzeuge_verkehrsteilnehmer(start_halt->gib_basis_pos(), step_count, ziel);
#endif
				continue;
			}

			// check, if they can walk there?
			if (can_walk_ziel) {
				// so we have happy passengers
				start_halt->add_pax_happy(pax_left_to_do);
				merke_passagier_ziel(ziel, COL_YELLOW);
				pax_transport += pax_left_to_do;
				continue;
			}

			// ok, they are not in walking distance
			ware_t pax(wtyp);
			pax.setze_zielpos(ziel);
			pax.menge = (wtyp == warenbauer_t::passagiere ? pax_left_to_do : max(1, pax_left_to_do >> 2));

			// now, finally search a route; this consumes most of the time
			koord return_zwischenziel = koord::invalid; // for people going back ...
			start_halt->suche_route(pax, will_return ? &return_zwischenziel : NULL);

			if (pax.gib_ziel() != koord::invalid) {
				// so we have happy traveling passengers
				start_halt->starte_mit_route(pax);
				start_halt->add_pax_happy(pax.menge);
				// and show it
				merke_passagier_ziel(ziel, COL_YELLOW);
				pax_transport += pax.menge;
			} else {
				start_halt->add_pax_no_route(pax_left_to_do);
				merke_passagier_ziel(ziel, COL_DARK_ORANGE);
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				erzeuge_verkehrsteilnehmer(start_halt->gib_basis_pos(), step_count, ziel);
#endif
			}

			// send them also back
			if (will_return != no_return && pax.gib_ziel() != koord::invalid) {
				// this comes most of the times for free and balances also the amounts!
				halthandle_t ret_halt = welt->lookup(pax.gib_ziel())->gib_halt();
				if (will_return != town_return) {
					// restore normal mail amount => more mail from attractions and factories than going to them
					pax.menge = pax_left_to_do;
				}

				// we just have to ensure, the ware can be delivered at this station
				warenziel_t wz(start_halt->gib_basis_pos(), wtyp);
				bool found = false;
				for (uint i = 0; i < plan->get_haltlist_count(); i++) {
					halthandle_t test_halt = halt_list[i];
					if (test_halt->is_enabled(wtyp) && (start_halt == test_halt || test_halt->gib_warenziele()->contains(wz))) {
						found = true;
						start_halt = test_halt;
						break;
					}
				}

				// now try to add them to the target halt
				if (ret_halt->gib_ware_summe(wtyp) <= ret_halt->get_capacity()) {
					// prissi: not overcrowded and can recieve => add them
					if (found) {
						ware_t return_pax (wtyp);

						return_pax.menge = pax_left_to_do;
						return_pax.setze_zielpos(k);
						return_pax.setze_ziel(start_halt->gib_basis_pos());
						return_pax.setze_zwischenziel(return_zwischenziel);

						ret_halt->starte_mit_route(return_pax);
						ret_halt->add_pax_happy(pax_left_to_do);
					} else {
						// no route back
						ret_halt->add_pax_no_route(pax_left_to_do);
					}
				}
			}
		}
	} else {
		// all overcrowded are unhappy
		for (unsigned h = 0; h < plan->get_haltlist_count(); h++) {
			halthandle_t halt = halt_list[h];
			if (halt->is_enabled(wtyp)) {
				halt->add_pax_unhappy(num_pax);
			}
		}
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

	INT_CHECK( "simcity 1551" );
}


/**
 * gibt einen zufällingen gleichverteilten Punkt innerhalb der
 * Stadtgrenzen zurück
 * @author Hj. Malthaner
 */
koord stadt_t::gib_zufallspunkt() const
{
	assert(buildings.get_count()>0);
	const gebaeude_t* gb = buildings.at_weight(simrand(buildings.get_sum_weight()));
	koord k = gb->gib_pos().gib_2d();
	if(!welt->ist_in_kartengrenzen(k)) {
		// this building should not be in this list, since it has been already deleted!
		dbg->error("stadt_t::gib_zufallspunkt()", "illegal building %s removing!", gb->gib_name());
		const_cast<stadt_t*>(this)->buildings.remove(gb);
		k = koord(0, 0);
	}
	return k;
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
		return fab->gib_pos().gib_2d();
	} else if (rand < TOURIST_PAX + FACTORY_PAX && welt->gib_ausflugsziele().get_sum_weight() > 0) {
		*will_return = tourist_return;	// tourists will return
		const gebaeude_t* gb = welt->gib_random_ausflugsziel();
		return gb->gib_pos().gib_2d();
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
		return zielstadt->gib_zufallspunkt();
	}
}


void stadt_t::merke_passagier_ziel(koord k, int color)
{
	const koord p = koord(
		((k.x * 127) / welt->gib_groesse_x()) & 127,
		((k.y * 127) / welt->gib_groesse_y()) & 127
	);
	pax_ziele_neu.at(p) = color;
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
			const weighted_vector_tpl<gebaeude_t*>& attractions = welt->gib_ausflugsziele();
			int dist = welt->gib_groesse_x() * welt->gib_groesse_y();
			for (uint i = 0; i < attractions.get_count(); i++) {
				int d = koord_distance(attractions[i]->gib_pos(), pos);
				if (d < dist) {
					dist = d;
				}
			}
			return dist;
		}

		bool strasse_bei(sint16 x, sint16 y) const
		{
			const grund_t* bd = welt->lookup(koord(x, y))->gib_kartenboden();
			return bd != NULL && bd->hat_weg(road_wt);
		}

		virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const
		{
			if (bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
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
	const haus_besch_t* besch = hausbauer_t::gib_special(bev, welt->get_timeline_year_month(), welt->get_climate(welt->max_hgt(pos)));
	if (besch != NULL) {
		if (simrand(100) < (uint)besch->gib_chance()) {
			// baue was immer es ist
			int rotate = 0;
			bool is_rotate = besch->gib_all_layouts() > 10;
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos, besch->gib_b(), besch->gib_h(), besch->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				if (besch->gib_all_layouts() > 1) {
					rotate = (simrand(20) & 2) + is_rotate;
				}
				hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos)->gib_kartenboden()->gib_pos(), rotate, besch);
				// tell the player, if not during initialization
				if (!new_town) {
					char buf[256];
					sprintf(buf, translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), gib_name(), make_single_line_string(translator::translate(besch->gib_name()), 2), bev);
					message_t::get_instance()->add_message(buf, best_pos, message_t::tourist, CITY_KI, besch->gib_tile(0)->gib_hintergrund(0, 0, 0));
				}
			}
		}
	}

	if ((bev & 511) == 0) {
		// errect a monoment
		besch = hausbauer_t::waehle_denkmal(welt->get_timeline_year_month());
		if (besch) {
			koord total_size = koord(2 + besch->gib_b(), 2 + besch->gib_h());
			koord best_pos(denkmal_platz_sucher_t(welt).suche_platz(pos, total_size.x, total_size.y, besch->get_allowed_climate_bits()));

			if (best_pos != koord::invalid) {
				bool ok = false;

				// Wir bauen das Denkmal nur, wenn schon mindestens eine Strasse da ist
				for (int i = 0; i < total_size.x && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(i, -1))->gib_kartenboden()->hat_weg(road_wt) ||
						welt->access(best_pos + koord(i, total_size.y))->gib_kartenboden()->hat_weg(road_wt);
				}
				for (int i = 0; i < total_size.y && !ok; i++) {
					ok = ok ||
						welt->access(best_pos + koord(total_size.x, i))->gib_kartenboden()->hat_weg(road_wt) ||
						welt->access(best_pos + koord(-1, i))->gib_kartenboden()->hat_weg(road_wt);
				}
				if (ok) {
					// Straßenkreis um das Denkmal legen
					for (int i = 0; i < total_size.y; i++) {
						baue_strasse(best_pos + koord(0, i), NULL, true);
						baue_strasse(best_pos + koord(total_size.x - 1, i), NULL, true);
					}
					for (int i = 0; i < total_size.x; i++) {
						baue_strasse(best_pos + koord(i, 0), NULL, true);
						baue_strasse(best_pos + koord(i, total_size.y - 1), NULL, true);
					}
					// and then built it
					hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos + koord(1, 1))->gib_kartenboden()->gib_pos(), 0, besch);
					hausbauer_t::denkmal_gebaut(besch);
					add_gebaeude_to_stadt(dynamic_cast<const gebaeude_t*>(welt->lookup(best_pos + koord(1, 1))->gib_kartenboden()->first_obj()));
					// tell the player, if not during initialization
					if (!new_town) {
						char buf[256];
						sprintf(buf, translator::translate("With a big festival\n%s built\na new monument.\n%i citicens rejoiced."), gib_name(), bev);
						message_t::get_instance()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->gib_tile(0)->gib_hintergrund(0, 0, 0));
					}
				}
			}
		}
	}
}


void stadt_t::check_bau_rathaus(bool new_town)
{
	const haus_besch_t* besch = hausbauer_t::gib_rathaus(bev, welt->get_timeline_year_month(), welt->get_climate(welt->max_hgt(pos)));
	if (besch != NULL) {
		grund_t* gr = welt->lookup(pos)->gib_kartenboden();
		gebaeude_t* gb = dynamic_cast<gebaeude_t*>(gr->first_obj());
		bool neugruendung = !gb || !gb->ist_rathaus();
		bool umziehen = !neugruendung;
		koord alte_str(koord::invalid);
		koord best_pos(pos);
		koord k;

		DBG_MESSAGE("check_bau_rathaus()", "bev=%d, new=%d", bev, neugruendung);

		if (!neugruendung) {
			if (gb->gib_tile()->gib_besch()->gib_level() == besch->gib_level()) {
				DBG_MESSAGE("check_bau_rathaus()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}

			const haus_besch_t* besch_alt = gb->gib_tile()->gib_besch();
			koord pos_alt = gr->gib_pos().gib_2d() - gb->gib_tile()->gib_offset();
			koord groesse_alt = besch_alt->gib_groesse(gb->gib_tile()->gib_layout());

			// do we need to move
			if (besch->gib_b() <= groesse_alt.x && besch->gib_h() <= groesse_alt.y) {
				// no, the size is ok
				umziehen = false;
			} else if (welt->lookup(pos + koord(0, besch_alt->gib_h()))->gib_kartenboden()->hat_weg(road_wt)) {
				// we need to built a new road, thus we will use the old as a starting point (if found)
				alte_str =  pos + koord(0, besch_alt->gib_h());
			}

			// clear the old townhall
			gr->setze_text(NULL);

			for (k.x = 0; k.x < groesse_alt.x; k.x++) {
				for (k.y = 0; k.y < groesse_alt.y; k.y++) {
					// we itereate over all tiles, since the townhalls are allowed sizes bigger than 1x1

					gr = welt->lookup(pos_alt + k)->gib_kartenboden();
		DBG_MESSAGE("stadt_t::check_bau_rathaus()", "loesch %p", gr->first_obj());
					gr->obj_loesche_alle(NULL);

					if (umziehen) {
						DBG_MESSAGE("stadt_t::check_bau_rathaus()", "delete townhall tile %i,%i (gb=%p)", k.x, k.y, gb);
						// replace old space by normal houses level 0 (must be 1x1!)
						gb = hausbauer_t::neues_gebaeude(welt, NULL, gr->gib_pos(), 0, hausbauer_t::gib_wohnhaus(0, welt->get_timeline_year_month(), welt->get_climate(welt->max_hgt(pos))), NULL);
						add_gebaeude_to_stadt(gb);
					}
				}
			}
		}

		// Now built the new townhall
		int layout = simrand(besch->gib_all_layouts() - 1);
		if (neugruendung || umziehen) {
			best_pos = rathausplatz_sucher_t(welt).suche_platz(pos, besch->gib_b(layout), besch->gib_h(layout) + 1, besch->get_allowed_climate_bits());
		}
		hausbauer_t::baue(welt, besitzer_p, welt->lookup(best_pos)->gib_kartenboden()->gib_pos(), layout, besch);
		DBG_MESSAGE("new townhall", "use layout=%i", layout);
		add_gebaeude_to_stadt(dynamic_cast<const gebaeude_t*>(welt->lookup(best_pos)->gib_kartenboden()->first_obj()));
		DBG_MESSAGE("stadt_t::check_bau_rathaus()", "add townhall (bev=%i, ptr=%p)", buildings.get_sum_weight(),welt->lookup(best_pos)->gib_kartenboden()->first_obj());

		// Orstnamen hinpflanzen
		welt->lookup(best_pos)->gib_kartenboden()->setze_text(name);

		// tell the player, if not during initialization
		if (!new_town) {
			char buf[256];
			sprintf(buf, translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), gib_name(), gib_einwohner());
			message_t::get_instance()->add_message(buf, pos, message_t::city, CITY_KI, besch->gib_tile(layout, 0, 0)->gib_hintergrund(0, 0, 0));
		}

		// Strasse davor verlegen
		k = koord(0, besch->gib_h(layout));
		for (k.x = 0; k.x < besch->gib_b(layout); k.x++) {
			if (baue_strasse(best_pos + k, NULL, true)) {
				;
			} else if(k.x==0) {
				// Hajo: Strassenbau nicht versuchen, eines der Felder
				// ist schon belegt
				alte_str == koord::invalid;
			}
		}

		if (umziehen && alte_str != koord::invalid) {
			// Strasse vom ehemaligen Rathaus zum neuen verlegen.
			wegbauer_t bauer(welt, NULL);
			bauer.route_fuer(wegbauer_t::strasse, welt->get_city_road());
			bauer.calc_route(welt->lookup(alte_str)->gib_kartenboden()->gib_pos(), welt->lookup(best_pos + koord(0, besch->gib_h(layout)))->gib_kartenboden()->gib_pos());
			bauer.baue();
			pruefe_grenzen(best_pos);
		} else if (neugruendung) {
			lo = best_pos - koord(2, 2);
			ur = best_pos + koord(besch->gib_b(layout), besch->gib_h(layout)) + koord(2, 2);
		}
		pos = best_pos;
	}

	// Hajo: paranoia - ensure correct bounds in all cases
	//       I don't know if best_pos is always valid
	if (lo.x < 0) lo.x = 0;
	if (ur.x >= welt->gib_groesse_x() - 1) ur.x = welt->gib_groesse_x() - 1;

	if (lo.y < 0) lo.y = 0;
	if (ur.y >= welt->gib_groesse_y() - 1) ur.y = welt->gib_groesse_y() - 1;
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
				const fabrik_besch_t* market = fabrikbauer_t::get_random_consumer(true, (climate_bits)(1 << welt->get_climate(welt->lookup(pos)->gib_kartenboden()->gib_hoehe())));
				if (market != NULL) {
					bool rotate = false;

					koord3d	market_pos = welt->lookup(pos)->gib_kartenboden()->gib_pos();
					DBG_MESSAGE("stadt_t::check_bau_factory", "adding new industry at %i inhabitants.", gib_einwohner());
					int n = fabrikbauer_t::baue_hierarchie(welt, NULL, market, rotate, &market_pos, welt->gib_spieler(1));
					// tell the player
					char buf[256];
					sprintf(buf, translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(market->gib_name()), gib_name(), n);
					message_t::get_instance()->add_message(buf, market_pos.gib_2d(), message_t::industry, CITY_KI, market->gib_haus()->gib_tile(0)->gib_hintergrund(0, 0, 0));
					break;
				}
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
			t = gb->gib_haustyp();
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
	koord( 0, -1),
	koord( 1,  0),
	koord( 0,  1),
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
	const koord3d pos(gr->gib_pos());

	// no covered by a downgoing monorail?
	if (gr->ist_natur() &&
			gr->kann_alle_obj_entfernen(welt->gib_spieler(1)) == NULL && (
				gr->gib_grund_hang() == hang_t::flach ||
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

		if (sum_gewerbe > sum_industrie && sum_gewerbe > sum_wohnung) {
			h = hausbauer_t::gib_gewerbe(0, current_month, cl);
			if (h != NULL) {
				arb += 20;
			}
		}

		if (h == NULL && sum_industrie > sum_gewerbe && sum_industrie > sum_wohnung) {
			h = hausbauer_t::gib_industrie(0, current_month, cl);
			if (h != NULL) {
				arb += 20;
			}
		}

		if (h == NULL && sum_wohnung > sum_industrie && sum_wohnung > sum_gewerbe) {
			h = hausbauer_t::gib_wohnhaus(0, current_month, cl);
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
				if (gr && gr->gib_weg_hang() == gr->gib_grund_hang()) {
					strasse_t* weg = (strasse_t*)gr->gib_weg(road_wt);
					if (weg != NULL) {
						if (i < 4 && streetdir == -1) {
							// update directions (NESW)
							streetdir = i;
						}
						weg->setze_gehweg(true);
						// if not current city road standard, then replace it
						if (weg->gib_besch() != welt->get_city_road()) {
							if(weg->gib_besitzer()!=NULL && !gr->gib_depot() && !gr->is_halt()) {
								spieler_t *sp = weg->gib_besitzer();
								if(sp) {
									sp->add_maintenance(-weg->gib_besch()->gib_wartung());
								}
								weg->setze_besitzer(NULL); // make public
							}
							weg->setze_besch(welt->get_city_road());
						}
						gr->calc_bild();
						reliefkarte_t::gib_karte()->calc_map_pixel(gr->gib_pos().gib_2d());
					}
				}
			}

			hausbauer_t::baue(welt, NULL, pos, streetdir == -1 ? 0 : streetdir, h);

			gebaeude_t* gb = dynamic_cast<gebaeude_t*>(welt->lookup_kartenboden(k)->first_obj());
			add_gebaeude_to_stadt(gb);
		}
	}
}


void stadt_t::erzeuge_verkehrsteilnehmer(koord pos, int level, koord target)
{
	const int verkehr_level = welt->gib_einstellungen()->gib_verkehr_level();
	if (verkehr_level > 0 && level % (17 - verkehr_level) == 0) {
		koord k;
		for (k.y = pos.y - 1; k.y <= pos.y + 1; k.y++) {
			for (k.x = pos.x - 1; k.x <= pos.x + 1; k.x++) {
				if (welt->ist_in_kartengrenzen(k)) {
					grund_t* gr = welt->lookup(k)->gib_kartenboden();
					const weg_t* weg = gr->gib_weg(road_wt);

					if (weg != NULL && (
								gr->gib_weg_ribi_unmasked(road_wt) == ribi_t::nordsued ||
								gr->gib_weg_ribi_unmasked(road_wt) == ribi_t::ostwest
							)) {
						if (stadtauto_t::gib_anzahl_besch() > 0) {
							stadtauto_t* vt = new stadtauto_t(welt, gr->gib_pos(), target);
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


void stadt_t::renoviere_gebaeude(koord k)
{
	const gebaeude_t::typ alt_typ = was_ist_an(k);
	if (alt_typ == gebaeude_t::unbekannt) {
		return; // kann nur bekannte gebaeude renovieren
	}

	gebaeude_t* gb = dynamic_cast<gebaeude_t*>(welt->lookup(k)->gib_kartenboden()->first_obj());
	if (gb == NULL) {
		dbg->error("stadt_t::renoviere_gebaeude()", "could not find a building at (%d,%d) but there should be one!", k.x, k.y);
		return;
	}

	if (gb->get_stadt() != this) {
		return; // not our town ...
	}

	// hier sind wir sicher dass es ein Gebaeude ist
	const int level = gb->gib_tile()->gib_besch()->gib_level();

	// bisher gibt es 2 Sorten Haeuser
	// arbeit-spendende und wohnung-spendende
	const int will_arbeit  = (bev - arb) / 4;  // Nur ein viertel arbeitet
	const int will_wohnung = (bev - won);

	// does the timeline allow this buildings?
	const uint16 current_month = welt->get_timeline_year_month();
	const climate cl = welt->get_climate(welt->max_hgt(k));

	// der Bauplatz muss bewertet werden
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
		h = hausbauer_t::gib_gewerbe(try_level, current_month, cl);
		if (h != NULL && h->gib_level() >= try_level) {
			will_haben = gebaeude_t::gewerbe;
			sum = sum_gewerbe;
		}
	}
	// check for industry, also if we wanted com, but there was no com good enough ...
	if (sum_industrie > sum_industrie && sum_industrie > sum_wohnung || (sum_gewerbe > sum_wohnung && will_haben == gebaeude_t::unbekannt)) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == gebaeude_t::industrie ? level + 1 : level);
		h = hausbauer_t::gib_industrie(try_level , current_month, cl);
		if (h != NULL && h->gib_level() >= try_level) {
			will_haben = gebaeude_t::industrie;
			sum = sum_industrie;
		}
	}
	// check for residence
	// (sum_wohnung>sum_industrie  &&  sum_wohnung>sum_gewerbe
	if (will_haben == gebaeude_t::unbekannt) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == gebaeude_t::wohnung ? level + 1 : level);
		h = hausbauer_t::gib_wohnhaus(try_level, current_month, cl);
		if (h != NULL && h->gib_level() >= try_level) {
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
			grund_t* gr = welt->lookup(k + neighbours[i])->gib_kartenboden();
			if (gr != NULL && gr->gib_weg_hang() == gr->gib_grund_hang()) {
				strasse_t* weg = static_cast<strasse_t*>(gr->gib_weg(road_wt));
				if (weg != NULL) {
					if (i < 4 && streetdir == 0) {
						// update directions (NESW)
						streetdir = i;
					}
					weg->setze_gehweg(true);
					// if not current city road standard, then replace it
					if (weg->gib_besch() != welt->get_city_road()) {
						if (weg->gib_besitzer() != NULL && !gr->gib_depot() && !gr->is_halt()) {
							spieler_t *sp = weg->gib_besitzer();
							if(sp) {
								sp->add_maintenance(-weg->gib_besch()->gib_wartung());
							}
							weg->setze_besitzer(NULL); // make public
						}
						weg->setze_besch(welt->get_city_road());
					}
					gr->calc_bild();
					reliefkarte_t::gib_karte()->calc_map_pixel(gr->gib_pos().gib_2d());
				} else if (gr->gib_typ() == grund_t::fundament) {
					// do not renovate, if the building is already in a neighbour tile
					gebaeude_t* gb = dynamic_cast<gebaeude_t*>(gr->first_obj());
					if (gb != NULL && gb->gib_tile()->gib_besch() == h) {
						return;
					}
				}
			}
		}

		remove_gebaeude_from_stadt(gb);
		switch (alt_typ) {
			case gebaeude_t::wohnung:   won -= level * 10; break;
			case gebaeude_t::gewerbe:   arb -= level * 20; break;
			case gebaeude_t::industrie: arb -= level * 20; break;
			default: break;
		}

		hausbauer_t::umbauen(gb, h, streetdir);
		welt->lookup(k)->gib_kartenboden()->calc_bild();

		switch (will_haben) {
			case gebaeude_t::wohnung:   won += h->gib_level() * 10; break;
			case gebaeude_t::gewerbe:   arb += h->gib_level() * 20; break;
			case gebaeude_t::industrie: arb += h->gib_level() * 20; break;
			default: break;
		}

		gebaeude_t* gb = dynamic_cast<gebaeude_t*>(welt->lookup(k)->gib_kartenboden()->first_obj());
		add_gebaeude_to_stadt(gb);

		// printf("Renovierung mit %d Industrie, %d Gewerbe, %d  Wohnung\n", sum_industrie, sum_gewerbe, sum_wohnung);

		erzeuge_verkehrsteilnehmer(k, h->gib_level(), koord::invalid);
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
	grund_t* bd = welt->lookup(k)->gib_kartenboden();

	// water?!?
	if (bd->gib_hoehe() <= welt->gib_grundwasser()) {
		return false;
	}

	if (bd->gib_typ() != grund_t::boden) {
		// not on monorails, foundations, tunnel or bridges
		return false;
	}

	// we must not built on water or runways etc.
	if (bd->hat_wege() && !bd->hat_weg(road_wt) && !bd->hat_weg(track_wt)) {
		return false;
	}

	// somebody else's things on it?
	if (bd->kann_alle_obj_entfernen(welt->gib_spieler(1))) {
		return false;
	}

	// initially allow all possible directions ...
	ribi_t::ribi allowed_dir = (bd->gib_grund_hang() != hang_t::flach ? ribi_t::doppelt(ribi_typ(bd->gib_weg_hang())) : (ribi_t::ribi)ribi_t::alle);
	ribi_t::ribi connection_roads = ribi_t::keine;

	// we have here a road: check for four corner stops
	gebaeude_t *gb = (gebaeude_t *)(bd->suche_obj(ding_t::gebaeude));
	if(gb) {
		// nothing to connect
		if(gb->gib_tile()->gib_besch()->gib_all_layouts()==4) {
			// single way
			allowed_dir = ribi_t::layout_to_ribi[gb->gib_tile()->gib_layout()];
		}
		else if(gb->gib_tile()->gib_besch()->gib_all_layouts()) {
			// through way
			allowed_dir = ribi_t::doppelt( ribi_t::layout_to_ribi[gb->gib_tile()->gib_layout()] );
		}
		else {
			dbg->error("stadt_t::baue_strasse()", "building on road with not directions at %i,%i?!?", k.x, k.y );
		}
	}

	// we must not built on water or runways etc.
	// only crossing or tramways allowed
	if (bd->hat_weg(track_wt)) {
		weg_t* sch = bd->gib_weg(track_wt);
		if (sch->gib_besch()->gib_styp() != 7) {
			// not a tramway
			ribi_t::ribi r = sch->gib_ribi_unmasked();
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
			grund_t* bd2 = welt->lookup(k + koord::nsow[r])->gib_kartenboden();
			if (bd2 == NULL || bd2->gib_typ() == grund_t::fundament) {
				// not connecting to a building of course ...
				allowed_dir &= ~ribi_t::nsow[r];
			} else if (bd2->gib_typ()!=grund_t::boden  &&  ribi_t::nsow[r]!=ribi_typ(bd2->gib_grund_hang())) {
				// not the same slope => tunnel or bridge
				allowed_dir &= ~ribi_t::nsow[r];
			} else if (bd2->hat_weg(road_wt)) {
				gebaeude_t *gb = (gebaeude_t *)(bd2->suche_obj(ding_t::gebaeude));
				if(gb) {
					// nothing to connect
					if(gb->gib_tile()->gib_besch()->gib_all_layouts()==4) {
						// single way
						if(ribi_t::nsow[r]!=ribi_t::rueckwaerts(ribi_t::layout_to_ribi[gb->gib_tile()->gib_layout()])) {
							allowed_dir &= ~ribi_t::nsow[r];
						}
						else {
							// otherwise allowed ...
							connection_roads |= ribi_t::nsow[r];
						}
					}
					else if(gb->gib_tile()->gib_besch()->gib_all_layouts()==2) {
						// through way
						if((ribi_t::doppelt( ribi_t::layout_to_ribi[gb->gib_tile()->gib_layout()] )&ribi_t::nsow[r])==0) {
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
				else if(bd2->gib_depot()) {
					// do not enter depots
					allowed_dir &= ~ribi_t::nsow[r];
				}
				else {
					// otherwise allowed ...
					connection_roads |= ribi_t::nsow[r];
				}
			}
		}
	}

	// now add the ribis to the other ways (if there)
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & connection_roads) {
			grund_t* bd2 = welt->lookup(k + koord::nsow[r])->gib_kartenboden();
			weg_t* w2 = bd2->gib_weg(road_wt);
			w2->ribi_add(ribi_t::rueckwaerts(ribi_t::nsow[r]));
			bd2->calc_bild();
		}
	}

	if (connection_roads != ribi_t::keine || forced) {

		if (!bd->weg_erweitern(road_wt, connection_roads)) {
			strasse_t* weg = new strasse_t(welt);
			weg->setze_gehweg(true);
			weg->setze_besch(welt->get_city_road());
			// Hajo: city roads should not belong to any player => so we can ignore any contruction costs ...
			bd->neuen_weg_bauen(weg, connection_roads, sp);
		}
		return true;
	}

	return false;
}


void stadt_t::baue()
{
//	printf("Haus: %d  Strasse %d\n", best_haus_wert, best_strasse_wert);

	if (best_strasse.found()) {
		if (!baue_strasse(best_strasse.gib_pos(), NULL, false)) {
			// cannot built street, will terminate it with house?!?
			// prissi: we will just ignore this
//			baue_gebaeude(best_strasse.gib_pos());
		}
		// prissi: only houses will extend a city
		// to make the core more compact
//		pruefe_grenzen(best_strasse.gib_pos());
	}

	INT_CHECK("simcity 1156");

	if (best_haus.found()) {
		baue_gebaeude(best_haus.gib_pos());
		pruefe_grenzen(best_haus.gib_pos());
	}

	INT_CHECK("simcity 1163");

	if (!best_haus.found() && !best_strasse.found() &&
		was_ist_an(best_haus.gib_pos()) != gebaeude_t::unbekannt) {

		if (simrand(8) <= 2) { // nicht so oft renovieren
			renoviere_gebaeude(best_haus.gib_pos());
			INT_CHECK("simcity 876");
		}
	}
}


void stadt_t::pruefe_grenzen(koord k)
{
	if (k.x <= lo.x && k.x > 1) {
		lo.x = k.x - 2;
	}
	if (k.y <= lo.y && k.y > 1) {
		lo.y = k.y - 2;
	}

	if (k.x >= ur.x && k.x < welt->gib_groesse_x() - 3) {
		ur.x = k.x + 2;
	}
	if (k.y >= ur.y && k.y < welt->gib_groesse_y() - 3) {
		ur.y = k.y + 2;
	}
}


// geeigneten platz zur Stadtgruendung durch Zufall ermitteln
vector_tpl<koord>* stadt_t::random_place(const karte_t* wl, const int anzahl)
{
	int cl = 0;
	for (int i = 0; i < MAX_CLIMATES; i++) {
		if (hausbauer_t::gib_rathaus(0, 0, (climate)i)) {
			cl |= (1 << i);
		}
	}
	DBG_DEBUG("karte_t::init()", "get random places in climates %x", cl);
	slist_tpl<koord>* list = wl->finde_plaetze(2, 3, (climate_bits)cl);
	DBG_DEBUG("karte_t::init()", "found %i places", list->count());
	vector_tpl<koord>* result = new vector_tpl<koord>(anzahl);

	for (int i = 0; i < anzahl; i++) {
		int len = list->count();
		// check distances of all cities to their respective neightbours
		while (len > 0) {
			const int index = simrand(len);
			int minimum_dist = 0x7FFFFFFF;  // init with maximum
			koord k = list->at(index);

			// check minimum distance
			for (int j = 0; j < i; j++) {
				int dist = abs(k.x - (*result)[j].x) + abs(k.y - (*result)[j].y);
				if (minimum_dist > dist) {
					minimum_dist = dist;
				}
			}
			list->remove(k);
			len--;
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


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 * @see simwin
 */
char* stadt_t::info(char* buf) const
{
	buf += sprintf(buf, "%s: %d (+%d)\n",
		translator::translate("City size"),
		gib_einwohner(),
		gib_wachstum()
	);

	buf += sprintf(buf, translator::translate("%d buildings\n"), buildings.get_count());

	buf += sprintf(buf, "\n%d,%d - %d,%d\n\n",
		lo.x, lo.y, ur.x , ur.y
	);

	buf += sprintf(buf, "%s: %d\n%s: %d\n\n",
		translator::translate("Unemployed"),
		bev - arb,
		translator::translate("Homeless"),
		bev - won
	);

	return buf;
}


/**
 * A short info about the city stats
 * @author Hj. Malthaner
 */
void stadt_t::get_short_info(cbuffer_t& buf) const
{
	buf.append(gib_name());
	buf.append(": ");
	buf.append(gib_einwohner());
	buf.append(" (+");
	buf.append(gib_wachstum());
	buf.append(")");
}
