/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 *
 * construction of cities, creation of passengers
 *
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "boden/wege/strasse.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "simwin.h"
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
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/fabrikbauer.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"


#define PACKET_SIZE (7)

karte_t* stadt_t::welt = NULL; // one is enough ...

/********************************* From here on cityrules stuff *****************************************/


/**
 * in this fixed interval, construction will happen
 * 21s = 21000 per house
 */
const uint32 stadt_t::step_bau_interval = 21000;

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
static uint32 min_building_density = 25;

// the following are the scores for the different building types
static sint16 ind_start_score =   0;
static sint16 com_start_score = -10;
static sint16 res_start_score =   0;

// order: res com, ind, given by gebaeude_t::typ
static sint16 ind_neighbour_score[] = { -8, 0,  8 };
static sint16 com_neighbour_score[] = {  1, 8,  1 };
static sint16 res_neighbour_score[] = {  8, 0, -8 };

/**
 * Rule data structure
 * maximum 7x7 rules
 * @author Hj. Malthaner
 */
class rule_entry_t {
public:
	uint8 x,y;
	char flag;
	rule_entry_t(uint8 x_=0, uint8 y_=0, char f_='.') : x(x_), y(y_), flag(f_) {}

	void rdwr(loadsave_t* file)
	{
		file->rdwr_byte(x);
		file->rdwr_byte(y);
		uint8 c = flag;
		file->rdwr_byte(c);
		flag = c;
	}
};

class rule_t {
public:
	sint16  chance;
	vector_tpl<rule_entry_t> rule;
	rule_t(uint32 count=0) : chance(0), rule(count) {}

	void rdwr(loadsave_t* file)
	{
		file->rdwr_short(chance);

		if (file->is_loading()) {
			rule.clear();
		}
		uint32 count = rule.get_count();
		file->rdwr_long(count);
		for(uint32 i=0; i<count; i++) {
			if (file->is_loading()) {
				rule.append(rule_entry_t());
			}
			rule[i].rdwr(file);
		}
	}
};

// house rules
static vector_tpl<rule_t *> house_rules;

// and road rules
static vector_tpl<rule_t *> road_rules;

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
 */

// here '.' is ignored, since it will not be tested anyway
static char const* const allowed_chars_in_rule = "SsnHhTtUu";

/*
 * @param pos position to check
 * @param regel the rule to evaluate
 * @return true on match, false otherwise
 * @author Hj. Malthaner
 */
bool stadt_t::bewerte_loc(const koord pos, const rule_t &regel, int rotation)
{
	//printf("Test for (%s) in rotation %d\n", pos.get_str(), rotation);
	koord k;

	FOR(vector_tpl<rule_entry_t>, const& r, regel.rule) {
		uint8 x,y;
		switch (rotation) {
			default:
			case   0: x=r.x; y=r.y; break;
			case  90: x=r.y; y=6-r.x; break;
			case 180: x=6-r.x; y=6-r.y; break;
			case 270: x=6-r.y; y=r.x; break;
		}

		const koord k(pos.x+x-3, pos.y+y-3);
		const grund_t* gr = welt->lookup_kartenboden(k);
		if (gr == NULL) {
			// outside of the map => cannot apply this rule
			return false;
		}
		switch (r.flag) {
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
			case 'U':
				// unbuildable for road
				if (!hang_t::ist_wegbar(gr->get_grund_hang())) return false;
				break;
			case 'u':
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
			default: ;
				// ignore
		}
	}
	return true;
}


/**
 * Check rule in all transformations at given position
 * prissi: but the rules should explicitly forbid building then?!?
 * @author Hj. Malthaner
 */
sint32 stadt_t::bewerte_pos(const koord pos, const rule_t &regel)
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


void stadt_t::bewerte_strasse(koord k, sint32 rd, const rule_t &regel)
{
	if (simrand(rd) == 0) {
		best_strasse.check(k, bewerte_pos(k, regel));
	}
}


void stadt_t::bewerte_haus(koord k, sint32 rd, const rule_t &regel)
{
	if (simrand(rd) == 0) {
		best_haus.check(k, bewerte_pos(k, regel));
	}
}


/**
 * Reads city configuration data
 * @author Hj. Malthaner
 */
bool stadt_t::cityrules_init(const std::string &objfilename)
{
	tabfile_t cityconf;
	// first take user data, then user global data
	const std::string user_dir=umgebung_t::user_dir;
	if (!cityconf.open((user_dir+"cityrules.tab").c_str())) {
		if (!cityconf.open((objfilename+"config/cityrules.tab").c_str())) {
			dbg->fatal("stadt_t::init()", "Can't read cityrules.tab" );
			return false;
		}
	}

	tabfileobj_t contents;
	cityconf.read(contents);

	char buf[128];

	renovation_percentage = (uint32)contents.get_int("renovation_percentage", 25);
	// to keep compatible with the typo, here both are ok
	min_building_density = (uint32)contents.get_int("minimum_building_desity", 25);
	min_building_density = (uint32)contents.get_int("minimum_building_density", min_building_density);

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

	uint32 num_house_rules = 0;
	for (;;) {
		sprintf(buf, "house_%d", num_house_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_house_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %d house building rules", num_house_rules);

	uint32 num_road_rules = 0;
	for (;;) {
		sprintf(buf, "road_%d", num_road_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_road_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %d road building rules", num_road_rules);

	clear_ptr_vector( house_rules );
	for (uint32 i = 0; i < num_house_rules; i++) {
		house_rules.append(new rule_t());
		sprintf(buf, "house_%d.chance", i + 1);
		house_rules[i]->chance = contents.get_int(buf, 0);

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
		const uint8 offset = (7 - (uint)size) / 2;
		for (uint y = 0; y < size; y++) {
			for (uint x = 0; x < size; x++) {
				const char flag = rule[x + y * (size + 1)];
				// check for allowed characters; ignore '.';
				// leave midpoint out, should be 'n', which is checked in baue() anyway
				if ((x+offset!=3  ||  y+offset!=3)  &&  (flag!=0  &&  strchr(allowed_chars_in_rule, flag))) {
					house_rules[i]->rule.append(rule_entry_t(x+offset,y+offset,flag));
				}
				else {
					if ((x+offset!=3  ||  y+offset!=3)  &&  flag!='.') {
						dbg->warning("stadt_t::cityrules_init()", "house rule %d entry (%d,%d) is '%c' and will be ignored", i + 1, x+offset, y+offset, flag);
					}
				}
			}
		}
	}

	clear_ptr_vector( road_rules );
	for (uint32 i = 0; i < num_road_rules; i++) {
		road_rules.append(new rule_t());
		sprintf(buf, "road_%d.chance", i + 1);
		road_rules[i]->chance = contents.get_int(buf, 0);

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
				const char flag = rule[x + y * (size + 1)];
				// check for allowed characters; ignore '.';
				// leave midpoint out, should be 'n', which is checked in baue() anyway
				if ((x+offset!=3  ||  y+offset!=3)  &&  (flag!=0  &&  strchr(allowed_chars_in_rule, flag))) {
					road_rules[i]->rule.append(rule_entry_t(x+offset,y+offset,flag));
				}
				else {
					if ((x+offset!=3  ||  y+offset!=3)  &&  flag!='.') {
						dbg->warning("stadt_t::cityrules_init()", "road rule %d entry (%d,%d) is '%c' and will be ignored", i + 1, x+offset, y+offset, flag);
					}
				}
			}
		}
	}
	return true;
}

/**
* Reads/writes city configuration data from/to a savegame
* called from karte_t::speichern and karte_t::laden
* only written for networkgames
* @author Dwachs
*/
void stadt_t::cityrules_rdwr(loadsave_t *file)
{
	file->rdwr_long(renovation_percentage);
	file->rdwr_long(min_building_density);

	file->rdwr_short(ind_start_score);
	file->rdwr_short(ind_neighbour_score[0]);
	file->rdwr_short(ind_neighbour_score[1]);
	file->rdwr_short(ind_neighbour_score[2]);

	file->rdwr_short(com_start_score);
	file->rdwr_short(com_neighbour_score[0]);
	file->rdwr_short(com_neighbour_score[1]);
	file->rdwr_short(com_neighbour_score[2]);

	file->rdwr_short(res_start_score);
	file->rdwr_short(res_neighbour_score[0]);
	file->rdwr_short(res_neighbour_score[1]);
	file->rdwr_short(res_neighbour_score[2]);

	// house rules
	if (file->is_loading()) {
		clear_ptr_vector( house_rules );
	}
	uint32 count = house_rules.get_count();
	file->rdwr_long(count);
	for(uint32 i=0; i<count; i++) {
		if (file->is_loading()) {
			house_rules.append(new rule_t());
		}
		house_rules[i]->rdwr(file);
	}
	// road rules
	if (file->is_loading()) {
		clear_ptr_vector( road_rules );
	}
	count = road_rules.get_count();
	file->rdwr_long(count);
	for(uint32 i=0; i<count; i++) {
		if (file->is_loading()) {
			road_rules.append(new rule_t());
		}
		road_rules[i]->rdwr(file);
	}
}

/**
 * denkmal_platz_sucher_t:
 *
 * Sucht einen freien Bauplatz
 * Im Gegensatz zum bauplatz_sucher_t werden Strassen auf den Raendern
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
					gr->get_typ() == grund_t::boden &&           // Boden -> no building
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // only roads
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
		rathausplatz_sucher_t(karte_t* welt, uint8 dir_) : platzsucher_t(welt), dir(dir_) {}

		virtual bool ist_feld_ok(koord pos, koord d, climate_bits cl) const
		{
			const grund_t* gr = welt->lookup_kartenboden(pos + d);
			if (gr == NULL  ||  gr->get_grund_hang() != hang_t::flach) return false;

			if (((1 << welt->get_climate(gr->get_hoehe())) & cl) == 0) {
				return false;
			}

			if (d.x > 0 || d.y > 0) {
				if (welt->lookup_kartenboden(pos)->get_hoehe() != gr->get_hoehe()) {
					// height wrong!
					return false;
				}
			}

			if ( ((dir & ribi_t::sued)!=0  &&  d.y == h - 1) ||
				((dir & ribi_t::west)!=0  &&  d.x == 0) ||
				((dir & ribi_t::nord)!=0  &&  d.y == 0) ||
				((dir & ribi_t::ost)!=0  &&  d.x == b - 1)) {
				// we want to build a road here:
				return
					gr->get_typ() == grund_t::boden &&
					(!gr->hat_wege() || (gr->hat_weg(road_wt) && !gr->has_two_ways())) && // build only on roads, no other ways
					!gr->is_halt() &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			} else {
				// we want to build the townhall here: maybe replace existing buildings
				return ((gr->get_typ()==grund_t::boden  &&  gr->ist_natur()) ||	gr->get_typ()==grund_t::fundament) &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			}
		}
private:
	uint8 dir;
};


static bool compare_gebaeude_pos(const gebaeude_t* a, const gebaeude_t* b)
{
	const uint32 pos_a = (a->get_pos().y<<16)+a->get_pos().x;
	const uint32 pos_b = (b->get_pos().y<<16)+b->get_pos().x;
	return pos_a<pos_b;
}


// this function adds houses to the city house list
void stadt_t::add_gebaeude_to_stadt(const gebaeude_t* gb, bool ordered)
{
	if (gb != NULL) {
		const haus_tile_besch_t* tile  = gb->get_tile();
		koord size = tile->get_besch()->get_groesse(tile->get_layout());
		const koord pos = gb->get_pos().get_2d() - tile->get_offset();
		koord k;

		// add all tiles
		for (k.y = 0; k.y < size.y; k.y++) {
			for (k.x = 0; k.x < size.x; k.x++) {
				if (gebaeude_t* const add_gb = ding_cast<gebaeude_t>(welt->lookup_kartenboden(pos + k)->first_obj())) {
					if(add_gb->get_tile()->get_besch()!=gb->get_tile()->get_besch()) {
						dbg->error( "stadt_t::add_gebaeude_to_stadt()","two buildings \"%s\" and \"%s\" at (%i,%i): Game will crash during deletion", add_gb->get_tile()->get_besch()->get_name(), gb->get_tile()->get_besch()->get_name(), pos.x + k.x, pos.y + k.y);
						buildings.remove(add_gb);
					}
					else {
						if(  ordered  ) {
							buildings.insert_ordered(add_gb, tile->get_besch()->get_level() + 1, compare_gebaeude_pos, 16);
						}
						else {
							buildings.append(add_gb, tile->get_besch()->get_level() + 1, 16);
						}
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
		has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/(abs(ur.x-lo.x-4)*abs(ur.y-lo.y-4)+1) > min_building_density);
		if(!has_low_density)  {
			// full recalc needed due to map borders ...
			recalc_city_size();
			return;
		}
	}
	else {
		has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/((ur.x-lo.x)*(ur.y-lo.y)+1) > min_building_density);
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
	FOR(weighted_vector_tpl<gebaeude_t*>, const i, buildings) {
		if (i->get_tile()->get_besch()->get_utyp() != haus_besch_t::firmensitz) {
			koord const& gb_pos = i->get_pos().get_2d();
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

	has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/((ur.x-lo.x)*(ur.y-lo.y)+1) > min_building_density);
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


void stadt_t::factory_entry_t::rdwr(loadsave_t *file)
{
	if(  file->get_version()>=110005  ) {
		koord factory_pos;
		if(  file->is_saving()  ) {
			factory_pos = factory->get_pos().get_2d();
		}
		factory_pos.rdwr( file );
		if(  file->is_loading()  ) {
			// position will be resolved back into fabrik_t* later
			factory_pos_x = factory_pos.x;
			factory_pos_y = factory_pos.y;
		}
		file->rdwr_long( demand );
		file->rdwr_long( supply );
		file->rdwr_long( remaining );
	}
}


void stadt_t::factory_entry_t::resolve_factory()
{
	factory = fabrik_t::get_fab( welt, koord(factory_pos_x, factory_pos_y) );
}


const stadt_t::factory_entry_t* stadt_t::factory_set_t::get_entry(const fabrik_t *const factory) const
{
	FOR(vector_tpl<factory_entry_t>, const& e, entries) {
		if (e.factory == factory) {
			return &e;
		}
	}
	return NULL;	// not found
}


stadt_t::factory_entry_t* stadt_t::factory_set_t::get_random_entry()
{
	if(  total_remaining>0  ) {
		sint32 weight = simrand(total_remaining);
		FOR(vector_tpl<factory_entry_t>, & entry, entries) {
			if(  entry.remaining>0  ) {
				if(  weight<entry.remaining  ) {
					return &entry;
				}
				weight -= entry.remaining;
			}
		}
	}
	return NULL;
}


void stadt_t::factory_set_t::update_factory(fabrik_t *const factory, const sint32 demand)
{
	if(  entries.is_contained( factory_entry_t(factory) )  ) {
		// existing target factory
		factory_entry_t &entry = entries[ entries.index_of( factory_entry_t(factory) ) ];
		total_demand += demand - entry.demand;
		entry.demand = demand;
		// entry.supply, entry.remaining and total_remaining will be adjusted in recalc_generation_ratio()
	}
	else {
		// new target factory
		entries.append( factory_entry_t(factory, demand) );
		total_demand += demand;
	}
	ratio_stale = true;		// always trigger recalculation of ratio
}


void stadt_t::factory_set_t::remove_factory(fabrik_t *const factory)
{
	if(  entries.is_contained( factory_entry_t(factory) )  ) {
		factory_entry_t &entry = entries[ entries.index_of( factory_entry_t(factory) ) ];
		total_demand -= entry.demand;
		total_remaining -= entry.remaining;
		entries.remove( entry );
		ratio_stale = true;
	}
}


#define SUPPLY_BITS   (3)
#define SUPPLY_FACTOR (9)	// out of 2^SUPPLY_BITS
void stadt_t::factory_set_t::recalc_generation_ratio(const sint32 default_percent, const sint64 *city_stats, const int stats_count, const int stat_type)
{
	ratio_stale = false;		// reset flag

	// calculate an average of at most 3 previous months' pax/mail generation amounts
	uint32 months = 0;
	sint64 average_generated = 0;
	sint64 month_generated;
	while(  months<3  &&  (month_generated=city_stats[(months+1)*stats_count+stat_type])>0  ) {
		average_generated += month_generated;
		++months;
	}
	if(  months>1  ) {
		average_generated /= months;
	}

	/* ratio formula -> ((supply * 100) / total) shifted by RATIO_BITS */
	// we supply 1/8 more than demand
	const sint32 target_supply = (total_demand * SUPPLY_FACTOR + ((1<<(DEMAND_BITS+SUPPLY_BITS))-1)) >> (DEMAND_BITS+SUPPLY_BITS);
	if(  total_demand==0  ) {
		// no demand -> zero ratio
		generation_ratio = 0;
	} else if (!welt->get_settings().get_factory_enforce_demand() || average_generated == 0) {
		// demand not enforced or no pax generation data from previous month(s) -> simply use default ratio
		generation_ratio = (uint32)default_percent << RATIO_BITS;
	}
	else {
		// ratio of target supply (plus allowances for rounding up) to previous months' average generated pax (less 6.25% or 1/16 allowance for fluctuation), capped by default ratio
		const sint64 default_ratio = (sint64)default_percent << RATIO_BITS;
		const sint64 supply_ratio = ((sint64)((target_supply+(sint32)entries.get_count())*100)<<RATIO_BITS) / (average_generated-(average_generated>>4)+1);
		generation_ratio = (uint32)( default_ratio<supply_ratio ? default_ratio : supply_ratio );
	}

	// adjust supply and remaining figures
	if (welt->get_settings().get_factory_enforce_demand() && (generation_ratio >> RATIO_BITS) == (uint32)default_percent && average_generated > 0 && total_demand > 0) {
		const sint64 supply_promille = ( ( (average_generated << 10) * (sint64)default_percent ) / 100 ) / (sint64)target_supply;
		if(  supply_promille<1024  ) {
			// expected supply is really smaller than target supply
			FOR(vector_tpl<factory_entry_t>, & entry, entries) {
				const sint32 new_supply = (sint32)( ( (sint64)entry.demand * SUPPLY_FACTOR * supply_promille + ((1<<(DEMAND_BITS+SUPPLY_BITS+10))-1) ) >> (DEMAND_BITS+SUPPLY_BITS+10) );
				const sint32 delta_supply = new_supply - entry.supply;
				if(  delta_supply==0  ) {
					continue;
				}
				else if(  delta_supply>0  ||  (entry.remaining+delta_supply)>=0  ) {
					// adjust remaining figures by the change in supply
					total_remaining += delta_supply;
					entry.remaining += delta_supply;
				}
				else {
					// avoid deducting more than allowed
					total_remaining -= entry.remaining;
					entry.remaining = 0;
				}
				entry.supply = new_supply;
			}
			return;
		}
	}
	// expected supply is unknown or sufficient to meet target supply
	FOR(vector_tpl<factory_entry_t>, & entry, entries) {
		const sint32 new_supply = ( entry.demand * SUPPLY_FACTOR + ((1<<(DEMAND_BITS+SUPPLY_BITS))-1) ) >> (DEMAND_BITS+SUPPLY_BITS);
		const sint32 delta_supply = new_supply - entry.supply;
		if(  delta_supply==0  ) {
			continue;
		}
		else if(  delta_supply>0  ||  (entry.remaining+delta_supply)>=0  ) {
			// adjust remaining figures by the change in supply
			total_remaining += delta_supply;
			entry.remaining += delta_supply;
		}
		else {
			// avoid deducting more than allowed
			total_remaining -= entry.remaining;
			entry.remaining = 0;
		}
		entry.supply = new_supply;
	}
}


void stadt_t::factory_set_t::new_month()
{
	FOR(vector_tpl<factory_entry_t>, & e, entries) {
		e.new_month();
	}
	total_remaining = 0;
	total_generated = 0;
	ratio_stale = true;
}


void stadt_t::factory_set_t::rdwr(loadsave_t *file)
{
	if(  file->get_version()>=110005  ) {
		uint32 entry_count = entries.get_count();
		file->rdwr_long(entry_count);
		if(  file->is_loading()  ) {
			entries.resize( entry_count );
			factory_entry_t entry;
			for(  uint32 e=0;  e<entry_count;  ++e  ) {
				entry.rdwr( file );
				total_demand += entry.demand;
				total_remaining += entry.remaining;
				entries.append( entry );
			}
		}
		else {
			for(  uint32 e=0;  e<entry_count;  ++e  ) {
				entries[e].rdwr( file );
			}
		}
		file->rdwr_long( total_generated );
	}
}


void stadt_t::factory_set_t::resolve_factories()
{
	uint32 remove_count = 0;
	FOR(vector_tpl<factory_entry_t>, & e, entries) {
		e.resolve_factory();
		if (!e.factory) {
			remove_count ++;
		}
	}
	for(  uint32 e=0;  e<remove_count;  ++e  ) {
		this->remove_factory( NULL );
	}
}


stadt_t::~stadt_t()
{
	// close info win
	destroy_win((ptrdiff_t)this);

	if(  reliefkarte_t::get_karte()->get_city() == this  ) {
		reliefkarte_t::get_karte()->set_city(NULL);
	}

	// olny if there is still a world left to delete from
	if(welt->get_groesse_x()>1) {

		welt->lookup_kartenboden(pos)->set_text(NULL);

		// remove city info and houses
		while (!buildings.empty()) {
			// old buildings are not where they think they are, so we ask for map floor
			gebaeude_t* const gb = buildings.front();
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
}


static bool name_used(weighted_vector_tpl<stadt_t*> const& cities, char const* const name)
{
	FOR(weighted_vector_tpl<stadt_t*>, const i, cities) {
		if (strcmp(i->get_name(), name) == 0) {
			return true;
		}
	}
	return false;
}


stadt_t::stadt_t(spieler_t* sp, koord pos, sint32 citizens) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
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
	last_center = koord::invalid;

	bev = 0;
	arb = 0;
	won = 0;

	lo = ur = pos;

	DBG_MESSAGE("stadt_t::stadt_t()", "Welt %p", welt);

	/* get a unique cityname */
	char                          const* n       = "simcity";
	weighted_vector_tpl<stadt_t*> const& staedte = welt->get_staedte();
	for (vector_tpl<char*> city_names(translator::get_city_name_list()); !city_names.empty();) {
		size_t      const idx  = simrand(city_names.get_count());
		char const* const cand = city_names[idx];
		if (name_used(staedte, cand)) {
			city_names[idx] = city_names.back();
			city_names.pop_back();
		} else {
			n = cand;
			break;
		}
	}
	DBG_MESSAGE("stadt_t::stadt_t()", "founding new city named '%s'", n);
	name = n;

	// 1. Rathaus bei 0 Leuten bauen
	check_bau_rathaus(true);

	wachstum = 0;
	allow_citygrowth = true;
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
	stadtinfo_options = 3;

	rdwr(file);
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
	file->rdwr_long(lli);
	file->rdwr_long(lob);
	file->rdwr_long(lre);
	file->rdwr_long(lun);
	lo.x = lli;
	lo.y = lob;
	ur.x = lre;
	ur.y = lun;
	file->rdwr_long(besitzer_n);
	file->rdwr_long(bev);
	file->rdwr_long(arb);
	file->rdwr_long(won);
	// old values zentrum_namen_cnt : aussen_namen_cnt
	if(file->get_version()<99018) {
		sint32 dummy=0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
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
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
			for (uint hist_type = 4; hist_type < 6; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < 2; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
			for (uint hist_type = 4; hist_type < 6; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// not needed any more
		sint32 dummy = 0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}
	else {
		// 99.17.0 extended city history
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long( stadtinfo_options);
	}

	if(file->get_version()>99014  &&  file->get_version()<99016) {
		sint32 dummy = 0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}

	// since 102.2 there are static cities
	if(file->get_version()>102001) {
		file->rdwr_bool(allow_citygrowth);
	}
	else if(  file->is_loading()  ) {
		allow_citygrowth = true;
	}
	// save townhall road position
	if(file->get_version()>102002) {
		townhall_road.rdwr(file);
	}
	else if(  file->is_loading()  ) {
		townhall_road = koord::invalid;
	}

	// data related to target factories
	target_factories_pax.rdwr( file );
	target_factories_mail.rdwr( file );

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
 * und nur noch die Datenstrukturenneu verknuepft werden muessen.
 * @author Hj. Malthaner
 */
void stadt_t::laden_abschliessen()
{
	step_count = 0;
	next_step = 0;
	next_bau_step = 0;

	// there might be broken savegames
	if (!name) {
		set_name( "simcity" );
	}

	// new city => need to grow
	if (buildings.empty()) {
		step_bau();
	}

	// clear the minimaps
	init_pax_destinations();

	// init step counter with meaningful value
	step_interval = (2 << 18u) / (buildings.get_count() * 4 + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	if(townhall_road==koord::invalid) {
		// guess road tile based on current orientation
		gebaeude_t const* const gb = ding_cast<gebaeude_t>(welt->lookup_kartenboden(pos)->first_obj());
		if(  gb  &&  gb->ist_rathaus()  ) {
			koord k(gb->get_tile()->get_besch()->get_groesse(gb->get_tile()->get_layout()));
			switch (gb->get_tile()->get_layout()) {
				default:
				case 0:
					townhall_road = pos + koord(0, k.y);
					break;
				case 1:
					townhall_road = pos + koord(k.x, 0);
					break;
				case 2:
					townhall_road = pos + koord(0, -1);
					break;
				case 3:
					townhall_road = pos + koord(-1, 0);
					break;
			}
		}
	}
	recalc_city_size();

	next_step = 0;
	next_bau_step = 0;

	// resolve target factories
	target_factories_pax.resolve_factories();
	target_factories_mail.resolve_factories();
}


void stadt_t::rotate90( const sint16 y_size )
{
	// rotate town origin
	pos.rotate90( y_size );
	townhall_road.rotate90( y_size );
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
	name = new_name;
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gr->set_text( new_name );
	}
	stadt_info_t *win = dynamic_cast<stadt_info_t*>(win_get_magic((ptrdiff_t)this));
	if (win) {
		win->update_data();
	}
}


/* show city info dialoge
 * @author prissi
 */
void stadt_t::zeige_info(void)
{
	create_win( new stadt_info_t(this), w_info, (ptrdiff_t)this );
}


/* calculates the factories which belongs to certain cities */
void stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "search factories near %s (center at %i,%i)", get_name(), pos.x, pos.y);
	assert( target_factories_pax.get_entries().empty() );
	assert( target_factories_mail.get_entries().empty() );

	FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
		const uint32 count = fab->get_target_cities().get_count();
		settings_t const& s = welt->get_settings();
		if (count < s.get_factory_worker_maximum_towns() && koord_distance(fab->get_pos(), pos) < s.get_factory_worker_radius()) {
			fab->add_target_city(this);
		}
	}
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "is connected with %i/%i factories (total demand=%i/%i) for pax/mail.",
		target_factories_pax.get_entries().get_count(), target_factories_mail.get_entries().get_count(), target_factories_pax.total_demand, target_factories_mail.total_demand);
}


/* change size of city
 * @author prissi */
void stadt_t::change_size(sint32 delta_citicens)
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
//				remove_city();
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

	settings_t const& s = welt->get_settings();
	// recalculate factory going ratios where necessary
	if(  target_factories_pax.ratio_stale  ) {
		target_factories_pax.recalc_generation_ratio(s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_PAS_GENERATED);
	}
	if(  target_factories_mail.ratio_stale  ) {
		target_factories_mail.recalc_generation_ratio(s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_MAIL_GENERATED);
	}

	// is it time for the next step?
	next_step += delta_t;
	next_bau_step += delta_t;

	step_interval = (1 << 21U) / (buildings.get_count() * s.get_passenger_factor() + 1);
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


void stadt_t::neuer_monat( bool recalc_destinations )
{
	swap<uint8>( pax_destinations_old, pax_destinations_new );
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;

	roll_history();
	target_factories_pax.new_month();
	target_factories_mail.new_month();
	settings_t const& s = welt->get_settings();
	target_factories_pax.recalc_generation_ratio( s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_PAS_GENERATED);
	target_factories_mail.recalc_generation_ratio(s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_MAIL_GENERATED);
	recalc_target_cities();

	// center has moved => change attraction weights
	if(  recalc_destinations  ||  last_center != get_center()  ) {
		last_center = get_center();
		recalc_target_attractions();
	}

	if (!stadtauto_t::list_empty()  &&  s.get_verkehr_level()>0) {
		// spawn eventuall citycars
		// the more transported, the less are spawned
		// the larger the city, the more spawned ...

		/* original implementation that is replaced by integer-only version below
		double pfactor = (double)(city_history_month[1][HIST_PAS_TRANSPORTED]) / (double)(city_history_month[1][HIST_PAS_GENERATED]+1);
		double mfactor = (double)(city_history_month[1][HIST_MAIL_TRANSPORTED]) / (double)(city_history_month[1][HIST_MAIL_GENERATED]+1);
		double gfactor = (double)(city_history_month[1][HIST_GOODS_RECIEVED]) / (double)(city_history_month[1][HIST_GOODS_NEEDED]+1);

		double factor = pfactor > mfactor ? (gfactor > pfactor ? gfactor : pfactor ) : mfactor;
		factor = (1.0-factor)*city_history_month[1][HIST_CITICENS];
		factor = log10( factor );
		*/

		// placeholder for fractions
#		define decl_stat(name, i0, i1) sint64 name##_stat[2]; name##_stat[0] = city_history_month[1][i0];  name##_stat[1] = city_history_month[1][i1]+1;
		// defines and initializes local sint64[2] arrays
		decl_stat(pax, HIST_PAS_TRANSPORTED, HIST_PAS_GENERATED);
		decl_stat(mail, HIST_MAIL_TRANSPORTED, HIST_MAIL_GENERATED);
		decl_stat(good, HIST_GOODS_RECIEVED, HIST_GOODS_NEEDED);

		// true if s1[0] / s1[1] > s2[0] / s2[1]
#		define comp_stats(s1,s2) ( s1[0]*s2[1] > s2[0]*s1[1] )
		// computes (1.0 - s[0]/s[1]) * city_history_month[1][HIST_CITICENS]
#		define comp_factor(s) (city_history_month[1][HIST_CITICENS] *( s[1]-s[0] )) / s[1]

		uint32 factor = (uint32)( comp_stats(pax_stat, mail_stat) ? (comp_stats(good_stat, pax_stat) ? comp_factor(good_stat) : comp_factor(pax_stat)) : comp_factor(mail_stat) );
		factor = log10(factor);

		uint16 number_of_cars = simrand( factor * s.get_verkehr_level() ) / 16;

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
	FOR(vector_tpl<factory_entry_t>, const& i, target_factories_pax.get_entries()) {
		fabrik_t *const fab = i.factory;
		if (fab->get_lieferziele().empty() && !fab->get_suppliers().empty()) {
			// consumer => check for it storage
			const fabrik_besch_t *const besch = fab->get_besch();
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

	// maybe this town should stay static
	if(  !allow_citygrowth  ) {
		wachstum = 0;
		return;
	}

	/* four parts contribute to town growth:
	 * passenger transport 40%, mail 20%, goods (30%), and electricity (10%)
	 */
	sint64     const(& h)[MAX_CITY_HISTORY] = city_history_month[0];
	settings_t const&  s           = welt->get_settings();
	sint32     const   pas         = (sint32)( (h[HIST_PAS_TRANSPORTED]  * (s.get_passenger_multiplier() << 6)) / (h[HIST_PAS_GENERATED]  + 1) );
	sint32     const   mail        = (sint32)( (h[HIST_MAIL_TRANSPORTED] * (s.get_mail_multiplier()      << 6)) / (h[HIST_MAIL_GENERATED] + 1) );
	sint32     const   electricity = 0;
	sint32     const   goods       = (sint32)( h[HIST_GOODS_NEEDED] == 0 ? 0 : (h[HIST_GOODS_RECIEVED] * (s.get_goods_multiplier() << 6)) / (h[HIST_GOODS_NEEDED]) );

	// smaller towns should growth slower to have villages for a longer time
	sint32 const weight_factor =
		bev <  1000 ? s.get_growthfactor_small()  :
		bev < 10000 ? s.get_growthfactor_medium() :
		s.get_growthfactor_large();

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
	// post oder pax erzeugen ?
	const ware_besch_t *const wtyp = ( simrand(400)<300 ? warenbauer_t::passagiere : warenbauer_t::post );
	const int history_type = (wtyp == warenbauer_t::passagiere) ? HIST_PAS_TRANSPORTED : HIST_MAIL_TRANSPORTED;
	factory_set_t &target_factories = (  wtyp==warenbauer_t::passagiere ? target_factories_pax : target_factories_mail );

	// restart at first buiulding?
	if (step_count >= buildings.get_count()) {
		step_count = 0;
	}
	if (buildings.empty()) {
		return;
	}
	const gebaeude_t* gb = buildings[step_count];

	// prissi: since now backtravels occur, we damp the numbers a little
	const int num_pax =
		(wtyp == warenbauer_t::passagiere) ?
			(gb->get_tile()->get_besch()->get_level()      + 6) >> 2 :
			(gb->get_tile()->get_besch()->get_post_level() + 8) >> 3 ;

	// create pedestrians in the near area?
	if (welt->get_settings().get_random_pedestrians() && wtyp == warenbauer_t::passagiere) {
		haltestelle_t::erzeuge_fussgaenger(welt, gb->get_pos(), num_pax);
	}

	// suitable start search
	const koord origin_pos = gb->get_pos().get_2d();
	const planquadrat_t *const plan = welt->lookup(origin_pos);
	const halthandle_t *const halt_list = plan->get_haltlist();

	// suitable start search
	static vector_tpl<halthandle_t> start_halts(16);
	start_halts.clear();
	for (uint h = 0; h < plan->get_haltlist_count(); h++) {
		halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(wtyp)  &&  !halt->is_overcrowded(wtyp->get_catg_index())  ) {
			start_halts.append(halt);
		}
	}

	// Hajo: track number of generated passengers.
	city_history_year[0][history_type+1] += num_pax;
	city_history_month[0][history_type+1] += num_pax;

	// only continue, if this is a good start halt
	if (!start_halts.empty()) {
		// Find passenger destination
		for(  int pax_routed=0, pax_left_to_do=0;  pax_routed<num_pax;  pax_routed+=pax_left_to_do  ) {
			// number of passengers that want to travel
			// Hajo: for efficiency we try to route not every
			// single pax, but packets. If possible, we do 7 passengers at a time
			// the last packet might have less then 7 pax
			pax_left_to_do = min(PACKET_SIZE, num_pax - pax_routed);

			// search target for the passenger
			pax_return_type will_return;
			factory_entry_t *factory_entry = NULL;
			const koord dest_pos = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, factory_entry);
			if(  factory_entry  ) {
				if (welt->get_settings().get_factory_enforce_demand()) {
					// ensure no more than remaining amount
					pax_left_to_do = min( pax_left_to_do, factory_entry->remaining );
					factory_entry->remaining -= pax_left_to_do;
					target_factories.total_remaining -= pax_left_to_do;
				}
				target_factories.total_generated += pax_left_to_do;
				factory_entry->factory->book_stat( pax_left_to_do, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED ) );
			}

#ifdef DESTINATION_CITYCARS
			//citycars with destination
			if (pax_routed == 0) {
				erzeuge_verkehrsteilnehmer(start_halt->get_basis_pos(), step_count, ziel);
			}
#endif

			ware_t pax(wtyp);
			pax.set_zielpos(dest_pos);
			pax.menge = pax_left_to_do;
			pax.to_factory = ( factory_entry ? 1 : 0 );

			ware_t return_pax(wtyp);

			// now, finally search a route; this consumes most of the time
			int const route_result = haltestelle_t::search_route( &start_halts[0], start_halts.get_count(), welt->get_settings().is_no_routing_over_overcrowding(), pax, &return_pax);
			halthandle_t start_halt = return_pax.get_ziel();
			if(  route_result==haltestelle_t::ROUTE_OK  ) {
				// register departed pax/mail at factory
				if(  factory_entry  ) {
					factory_entry->factory->book_stat( pax.menge, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
				}
				// so we have happy traveling passengers
				start_halt->starte_mit_route(pax);
				start_halt->add_pax_happy(pax.menge);
				// and show it
				merke_passagier_ziel(dest_pos, COL_YELLOW);
				city_history_year[0][history_type] += pax.menge;
				city_history_month[0][history_type] += pax.menge;
			}
			else if(  route_result==haltestelle_t::ROUTE_WALK  ) {
				if(  factory_entry  ) {
					// workers who walk to the factory or customers who walk to the consumer store
					factory_entry->factory->book_stat( pax_left_to_do, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
					factory_entry->factory->liefere_an(wtyp, pax_left_to_do);
				}
				// people who walk or mail delivered by hand do not count as transported or happy
				start_halt->add_pax_walked(num_pax);
			}
			else if(  route_result==haltestelle_t::ROUTE_OVERCROWDED  ) {
				merke_passagier_ziel(dest_pos, COL_ORANGE );
				if (start_halt.is_bound()) {
					start_halt->add_pax_unhappy(pax_left_to_do);
					if(  will_return  ) {
						pax.get_ziel()->add_pax_unhappy(pax_left_to_do);
					}
				}
				else {
					// all routes to goal are overcrowded -> register at all start halts
					FOR(vector_tpl<halthandle_t>, const s, start_halts) {
						s->add_pax_unhappy(pax_left_to_do);
						merke_passagier_ziel(dest_pos, COL_ORANGE);
					}
				}
			}
			else {
				// since there is no route from any start halt -> register no route at all start halts
				FOR(vector_tpl<halthandle_t>, const s, start_halts) {
					s->add_pax_no_route(pax_left_to_do);
				}
				merke_passagier_ziel(dest_pos, COL_DARK_ORANGE);
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				erzeuge_verkehrsteilnehmer(start_halt->get_basis_pos(), step_count, ziel);
#endif
			}

			// send them also back
			if(  route_result==haltestelle_t::ROUTE_OK  &&  will_return!=no_return  ) {
				halthandle_t return_halt = pax.get_ziel();
				if(  !return_halt->is_overcrowded( wtyp->get_catg_index() )  ) {
					// prissi: not overcrowded and can receive => add them
					if(  will_return!=city_return  &&  wtyp==warenbauer_t::post  ) {
						// attractions/factory generate more mail than they receive
						return_pax.menge = pax_left_to_do*3;
					}
					else {
						// use normal amount for return pas/mail
						return_pax.menge = pax_left_to_do;
					}
					return_pax.set_zielpos(origin_pos);

					return_halt->starte_mit_route(return_pax);
					return_halt->add_pax_happy(pax_left_to_do);
				}
				else {
					// return halt crowded
					return_halt->add_pax_unhappy(pax_left_to_do);
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
				halt->add_pax_unhappy(num_pax);
			}
		}

		// all passengers without suitable start:
		// fake one ride to get a proper display of destinations (although there may be more) ...
		pax_return_type will_return;
		factory_entry_t *factory_entry = NULL;
		const koord ziel = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, factory_entry);
		if(  factory_entry  ) {
			// consider at most 1 packet's amount as factory-going
			sint32 amount = min(PACKET_SIZE, num_pax);
			if (welt->get_settings().get_factory_enforce_demand()) {
				// ensure no more than remaining amount
				amount = min( amount, factory_entry->remaining );
				factory_entry->remaining -= amount;
				target_factories.total_remaining -= amount;
			}
			target_factories.total_generated += amount;
			factory_entry->factory->book_stat( amount, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED ) );
		}
#ifdef DESTINATION_CITYCARS
		//citycars with destination
		erzeuge_verkehrsteilnehmer(k, step_count, ziel);
#endif
		merke_passagier_ziel(ziel, COL_DARK_ORANGE);
		// we do not show no route for destination stop!
	}
}


/**
 * returns a random and uniformly distributed point within city borders
 * @author Hj. Malthaner
 */
koord stadt_t::get_zufallspunkt() const
{
	if(!buildings.empty()) {
		gebaeude_t* const gb = pick_any_weighted(buildings);
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


void stadt_t::add_target_city(stadt_t *const city)
{
	target_cities.append(
		city,
		weight_by_distance( city->get_einwohner()+1, shortest_distance( get_center(), city->get_center() ) ),
		64u
	);
}


void stadt_t::recalc_target_cities()
{
	target_cities.clear();
	FOR(weighted_vector_tpl<stadt_t*>, const c, welt->get_staedte()) {
		add_target_city(c);
	}
}


void stadt_t::add_target_attraction(gebaeude_t *const attraction)
{
	assert( attraction != NULL );
	target_attractions.append(
		attraction,
		weight_by_distance( attraction->get_passagier_level() << 4, shortest_distance( get_center(), attraction->get_pos().get_2d() ) ),
		64u
	);
}


void stadt_t::recalc_target_attractions()
{
	target_attractions.clear();
	FOR(weighted_vector_tpl<gebaeude_t*>, const a, welt->get_ausflugsziele()) {
		add_target_attraction(a);
	}
}


/* this function generates a random target for passenger/mail
 * changing this strongly affects selection of targets and thus game strategy
 */
koord stadt_t::find_destination(factory_set_t &target_factories, const sint64 generated, pax_return_type* will_return, factory_entry_t* &factory_entry)
{
	// Knightly : first, check to see if the pax/mail is factory-going
	if(  target_factories.total_remaining>0  &&  (sint64)target_factories.generation_ratio>((sint64)(target_factories.total_generated*100)<<RATIO_BITS)/(generated+1)  ) {
		factory_entry_t *const entry = target_factories.get_random_entry();
		assert( entry );
		*will_return = factory_return;	// worker will return
		factory_entry = entry;
		return entry->factory->get_pos().get_2d();
	}

	const sint16 rand = simrand(100 - (target_factories.generation_ratio >> RATIO_BITS));

	if(  rand<welt->get_settings().get_tourist_percentage()  &&  target_attractions.get_sum_weight()>0  ) {
		*will_return = tourist_return;	// tourists will return
		return pick_any_weighted( target_attractions )->get_pos().get_2d();
	}
	else {
		// since the locality is already taken into accout for us, we just use the random weight
		const stadt_t *const selected_city = pick_any_weighted( target_cities );
		// no return trip if the destination is inside the same city
		*will_return = selected_city == this ? no_return : city_return;
		// find a random spot inside the selected city
		return selected_city->get_zufallspunkt();
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

		// get distance to next special building
		int find_dist_next_special(koord pos) const
		{
			const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_ausflugsziele();
			int dist = welt->get_groesse_x() * welt->get_groesse_y();
			FOR(  weighted_vector_tpl<gebaeude_t*>, const i, attractions  ) {
				int const d = koord_distance(i->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			FOR(  weighted_vector_tpl<stadt_t *>, const city, welt->get_staedte() ) {
				int const d = koord_distance(city->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			return dist;
		}

		bool strasse_bei(sint16 x, sint16 y) const
		{
			const grund_t* bd = welt->lookup_kartenboden(koord(x, y));
			return bd != NULL && bd->hat_weg(road_wt);
		}

		virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const
		{
			if(  bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)  ) {
				// nothing on top like elevated monorails?
				for (sint16 y = pos.y;  y < pos.y + h; y++) {
					for (sint16 x = pos.x; x < pos.x + b; x++) {
						grund_t *gr = welt->lookup_kartenboden(koord(x,y));
						if(  gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1)  )!=NULL) {
							// something on top (monorail or powerlines)
							return false;
						}
					}
				}
				// not direct next to factories or townhalls
				for (sint16 y = pos.y-1;  y < pos.y + h+1; y++) {
					if(  grund_t *gr = welt->lookup_kartenboden( koord(pos.x-1,y) )  ) {
						if(  gebaeude_t *gb=gr->find<gebaeude_t>()  ) {
							if(  gb->get_haustyp() > 0  &&  gb->get_haustyp() < 8  ) {
								return false;
							}
						}
					}
					if(  grund_t *gr = welt->lookup_kartenboden( koord(pos.x+b,y) )  ) {
						if(  gebaeude_t *gb=gr->find<gebaeude_t>()  ) {
							if(  gb->get_haustyp() > 0  &&  gb->get_haustyp() < 8  ) {
								return false;
							}
						}
					}
				}
				for (sint16 x = pos.x; x < pos.x + b; x++) {
					if(  grund_t *gr = welt->lookup_kartenboden( koord(x,pos.y-1) )  ) {
						if(  gebaeude_t *gb=gr->find<gebaeude_t>()  ) {
							if(  gb->get_haustyp() > 0  &&  gb->get_haustyp() < 8  ) {
								return false;
							}
						}
					}
					if(  grund_t *gr = welt->lookup_kartenboden( koord(x,pos.y+h) )  ) {
						if(  gebaeude_t *gb=gr->find<gebaeude_t>()  ) {
							if(  gb->get_haustyp() > 0  &&  gb->get_haustyp() < 8  ) {
								return false;
							}
						}
					}
				}
				// try to built a little away from previous ones
				if (find_dist_next_special(pos) < b + h + welt->get_settings().get_special_building_distance()  ) {
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
			bool is_rotate = besch->get_all_layouts() > 1;
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos, besch->get_b(), besch->get_h(), besch->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				int rotate = 0;
				if (besch->get_all_layouts() > 1) {
					rotate = (simrand(20) & 2) + is_rotate;
				}
				hausbauer_t::baue( welt, besitzer_p, welt->lookup_kartenboden(best_pos)->get_pos(), rotate, besch );
				// tell the player, if not during initialization
				if (!new_town) {
					cbuffer_t buf;
					buf.printf( translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), get_name(), make_single_line_string(translator::translate(besch->get_name()), 2), get_einwohner());
					welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(0)->get_hintergrund(0, 0, 0));
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
					// build roads around the monument
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
					const gebaeude_t* gb = hausbauer_t::baue(welt, besitzer_p, welt->lookup_kartenboden(best_pos + koord(1, 1))->get_pos(), 0, besch);
					hausbauer_t::denkmal_gebaut(besch);
					add_gebaeude_to_stadt(gb);
					// tell the player, if not during initialization
					if (!new_town) {
						cbuffer_t buf;
						buf.printf( translator::translate("With a big festival\n%s built\na new monument.\n%i citicens rejoiced."), get_name(), get_einwohner() );
						welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(0)->get_hintergrund(0, 0, 0));
					}
				}
			}
		}
	}
}


void stadt_t::check_bau_rathaus(bool new_town)
{
	const haus_besch_t* besch = hausbauer_t::get_special(bev, haus_besch_t::rathaus, welt->get_timeline_year_month(), bev==0, welt->get_climate(welt->max_hgt(pos)));
	if(besch != NULL) {
		grund_t* gr = welt->lookup_kartenboden(pos);
		gebaeude_t* gb = ding_cast<gebaeude_t>(gr->first_obj());
		bool neugruendung = !gb || !gb->ist_rathaus();
		bool umziehen = !neugruendung;
		koord alte_str(koord::invalid);
		koord best_pos(pos);
		koord k;
		int old_layout(0);

		DBG_MESSAGE("check_bau_rathaus()", "bev=%d, new=%d name=%s", bev, neugruendung, name.c_str());

		if(  bev!=0  ) {

			const haus_besch_t* besch_alt = gb->get_tile()->get_besch();
			if (besch_alt->get_level() == besch->get_level()) {
				DBG_MESSAGE("check_bau_rathaus()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}
			old_layout = gb->get_tile()->get_layout();
			const sint8 old_z = gb->get_pos().z;
			koord pos_alt = best_pos = gr->get_pos().get_2d() - gb->get_tile()->get_offset();
			// guess layout for broken townhall's
			if(besch_alt->get_b() != besch_alt->get_h()  &&  besch_alt->get_all_layouts()==1) {
				// test all layouts
				koord corner_offset(besch_alt->get_b()-1, besch_alt->get_h()-1);
				for(uint8 test_layout = 0; test_layout<4; test_layout++) {
					// is there a part of our townhall in this corner
					grund_t *gr0 = welt->lookup_kartenboden(pos + corner_offset);
					gebaeude_t const* const gb0 = gr0 ? ding_cast<gebaeude_t>(gr0->first_obj()) : 0;
					if (gb0  &&  gb0->ist_rathaus()  &&  gb0->get_tile()->get_besch()==besch_alt  &&  gb0->get_stadt()==this) {
						old_layout = test_layout;
						pos_alt = best_pos = gr->get_pos().get_2d() + koord(test_layout%3!=0 ? corner_offset.x : 0, test_layout&2 ? corner_offset.y : 0);
						welt->lookup_kartenboden(pos_alt)->set_text("hier");
						break;
					}
					corner_offset = koord(-corner_offset.y, corner_offset.x);
				}
			}
			koord groesse_alt = besch_alt->get_groesse(old_layout);

			// do we need to move
			if(  old_layout<=besch->get_all_layouts()  &&  besch->get_b(old_layout) <= groesse_alt.x  &&  besch->get_h(old_layout) <= groesse_alt.y  ) {
				// no, the size is ok
				// correct position if new townhall is smaller than old
				if(  old_layout == 0  ) {
					best_pos.y -= besch->get_h(old_layout) - groesse_alt.y;
				}
				else if (old_layout == 1) {
					best_pos.x -= besch->get_b(old_layout) - groesse_alt.x;
				}
				umziehen = false;
			}
			else {
				// we need to built a new road, thus we will use the old as a starting point (if found)
				if (welt->lookup_kartenboden(townhall_road)  &&  welt->lookup_kartenboden(townhall_road)->hat_weg(road_wt)) {
					alte_str = townhall_road;
				}
				else {
					koord k = pos + (old_layout==0 ? koord(0, besch_alt->get_h()) : koord(besch_alt->get_b(),0) );
					if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
						alte_str = k;
					}
					else {
						k = pos - (old_layout==0 ? koord(0, besch_alt->get_h()) : koord(besch_alt->get_b(),0) );
						if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
							alte_str = k;
						}
					}
				}
			}

			// remove old townhall
			if(  gb  ) {
				DBG_MESSAGE("stadt_t::check_bau_rathaus()", "delete townhall at (%s)", pos_alt.get_str());
				hausbauer_t::remove(welt, NULL, gb);
			}

			// replace old space by normal houses level 0 (must be 1x1!)
			if(  umziehen  ) {
				for (k.x = 0; k.x < groesse_alt.x; k.x++) {
					for (k.y = 0; k.y < groesse_alt.y; k.y++) {
						// we iterate over all tiles, since the townhalls are allowed sizes bigger than 1x1
						const koord pos = pos_alt + k;
						gr = welt->lookup_kartenboden(pos);
						if (gr  &&  gr->ist_natur() &&  gr->kann_alle_obj_entfernen(NULL) == NULL  &&
							  (  gr->get_grund_hang() == hang_t::flach  ||  welt->lookup(koord3d(k, welt->max_hgt(k))) == NULL  ) ) {
							DBG_MESSAGE("stadt_t::check_bau_rathaus()", "fill empty spot at (%s)", pos.get_str());
							baue_gebaeude(pos);
						}
					}
				}
			}
			else {
				// make tiles flat, hausbauer_t::remove could have set some natural slopes
				for(  k.x = 0;  k.x < besch->get_b(old_layout);  k.x++  ) {
					for(  k.y = 0;  k.y < besch->get_h(old_layout);  k.y++  ) {
						gr = welt->lookup_kartenboden(best_pos + k);
						if(  gr  &&  gr->ist_natur()  ) {
							// make flat and use right height
							gr->set_grund_hang(hang_t::flach);
							gr->set_pos( koord3d( best_pos + k, old_z ) );
						}
					}
				}
			}
		}

		// Now built the new townhall (remember old orientation)
		int layout = umziehen || neugruendung ? simrand(besch->get_all_layouts()) : old_layout % besch->get_all_layouts();
		// on which side should we place the road?
		uint8 dir;
		// offset of bulding within searched place, start and end of road
		koord offset(0,0), road0(0,0),road1(0,0);
		dir = ribi_t::layout_to_ribi[layout & 3];
		switch(dir) {
			case ribi_t::ost:
				road0.x = besch->get_b(layout);
				road1.x = besch->get_b(layout);
				road1.y = besch->get_h(layout)-1;
				break;
			case ribi_t::nord:
				road1.x = besch->get_b(layout)-1;
				if (neugruendung || umziehen) {
					offset.y = 1;
				}
				else {
					// offset already included in position of old townhall
					road0.y=-1;
					road1.y=-1;
				}
				break;
			case ribi_t::west:
				road1.y = besch->get_h(layout)-1;
				if (neugruendung || umziehen) {
					offset.x = 1;
				}
				else {
					// offset already included in in position of old townhall
					road0.x=-1;
					road1.x=-1;
				}
				break;
			case ribi_t::sued:
			default:
				road0.y = besch->get_h(layout);
				road1.x = besch->get_b(layout)-1;
				road1.y = besch->get_h(layout);
		}
		if (neugruendung || umziehen) {
			best_pos = rathausplatz_sucher_t(welt, dir).suche_platz(pos, besch->get_b(layout) + (dir & ribi_t::ostwest ? 1 : 0), besch->get_h(layout) + (dir & ribi_t::nordsued ? 1 : 0), besch->get_allowed_climate_bits());
		}
		// check, if the was something found
		if(best_pos==koord::invalid) {
			dbg->error( "stadt_t::check_bau_rathaus", "no better postion found!" );
			return;
		}
		gebaeude_t const* const new_gb = hausbauer_t::baue(welt, besitzer_p, welt->lookup_kartenboden(best_pos + offset)->get_pos(), layout, besch);
		DBG_MESSAGE("new townhall", "use layout=%i", layout);
		add_gebaeude_to_stadt(new_gb);
		DBG_MESSAGE("stadt_t::check_bau_rathaus()", "add townhall (bev=%i, ptr=%p)", buildings.get_sum_weight(),welt->lookup_kartenboden(best_pos)->first_obj());

		// if not during initialization
		if (!new_town) {
			cbuffer_t buf;
			buf.printf(translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), name.c_str(), get_einwohner());
			welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(layout, 0, 0)->get_hintergrund(0, 0, 0));
		}
		else {
			welt->lookup_kartenboden(best_pos + offset)->set_text( name );
		}

		if (neugruendung || umziehen) {
			// build the road in front of the townhall
			if (road0!=road1) {
				wegbauer_t bauigel(welt, NULL);
				bauigel.route_fuer(wegbauer_t::strasse, welt->get_city_road(), NULL, NULL);
				bauigel.set_build_sidewalk(true);
				bauigel.calc_straight_route(welt->lookup_kartenboden(best_pos + road0)->get_pos(), welt->lookup_kartenboden(best_pos + road1)->get_pos());
				bauigel.baue();
			}
			else {
				baue_strasse(best_pos + road0, NULL, true);
			}
			townhall_road = best_pos + road0;
		}
		if (umziehen  &&  alte_str != koord::invalid) {
			// Strasse vom ehemaligen Rathaus zum neuen verlegen.
			wegbauer_t bauer(welt, NULL);
			bauer.route_fuer(wegbauer_t::strasse | wegbauer_t::terraform_flag, welt->get_city_road());
			bauer.calc_route(welt->lookup_kartenboden(alte_str)->get_pos(), welt->lookup_kartenboden(townhall_road)->get_pos());
			bauer.baue();
		} else if (neugruendung) {
			lo = best_pos+offset - koord(2, 2);
			ur = best_pos+offset + koord(besch->get_b(layout), besch->get_h(layout)) + koord(2, 2);
		}
		const koord new_pos = best_pos + offset;
		if(  pos!=new_pos  ) {
			// update position (where the name is)
			welt->lookup_kartenboden(pos)->set_text( NULL );
			pos = new_pos;
			welt->lookup_kartenboden(pos)->set_text( name );
		}
	}
}


/* eventually adds a new industry
 * so with growing number of inhabitants the industry grows
 * @date 12.1.05
 * @author prissi
 */
void stadt_t::check_bau_factory(bool new_town)
{
	uint32 const inc = welt->get_settings().get_industry_increase_every();
	if (!new_town && inc > 0 && bev % inc == 0) {
		uint32 const div = bev / inc;
		for (uint8 i = 0; i < 8; i++) {
			if (div==(1u<<i)) {
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
		if (gebaeude_t const* const gb = ding_cast<gebaeude_t>(gr->first_obj())) {
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


// return layout
static int gebaeude_layout[] = {0,0,1,4,2,0,5,1,3,7,1,0,6,3,2,0};


void stadt_t::baue_gebaeude(const koord k)
{
	grund_t* gr = welt->lookup_kartenboden(k);
	const koord3d pos(gr->get_pos());

	// no covered by a downgoing monorail?
	if (gr->ist_natur() &&
		  gr->kann_alle_obj_entfernen(NULL) == NULL  &&
		  (  gr->get_grund_hang() == hang_t::flach  ||  welt->lookup(koord3d(k, welt->max_hgt(k))) == NULL  )
	) {
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
				arb += h->get_level() * 20;
			}
		}

		if (h == NULL  &&  sum_industrie > sum_gewerbe  &&  sum_industrie > sum_wohnung) {
			h = hausbauer_t::get_industrie(0, current_month, cl);
			if (h != NULL) {
				arb += h->get_level() * 20;
			}
		}

		if (h == NULL  &&  sum_wohnung > sum_industrie  &&  sum_wohnung > sum_gewerbe) {
			h = hausbauer_t::get_wohnhaus(0, current_month, cl);
			if (h != NULL) {
				// will be aligned next to a street
				won += h->get_level() * 10;
			}
		}

		/*******************************************************
		 * this are the layout possible for city buildings
		********************************************************
		dims=1,1,1
		+---+
		|000|
		|0 0|
		|000|
		+---+
		dims=1,1,2
		+---+
		|001|
		|1 1|
		|100|
		+---+
		dims=1,1,4
		+---+
		|221|
		|3 1|
		|300|
		+---+
		dims=1,1,8
		+---+
		|625|
		|3 1|
		|704|
		+---+
		********************************************************/

		// we have something to built here ...
		if (h != NULL) {
			// check for pavement
			int streetdir = 0;
			for (int i = 0; i < 8; i++) {
				gr = welt->lookup_kartenboden(k + neighbours[i]);
				if (gr && gr->get_weg_hang() == gr->get_grund_hang()) {
					strasse_t* weg = (strasse_t*)gr->get_weg(road_wt);
					if (weg != NULL) {
						if (i < 4) {
							// update directions (SENW)
							streetdir += (1 << i);
						}
						weg->set_gehweg(true);
						// if not current city road standard, then replace it
						if (weg->get_besch() != welt->get_city_road()) {
							spieler_t *sp = weg->get_besitzer();
							if (sp == NULL  ||  !gr->get_depot()) {
								spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());

								weg->set_besitzer(NULL); // make public
								weg->set_besch(welt->get_city_road());
							}
						}
						gr->calc_bild();
						reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
					}
				}
			}

			const gebaeude_t* gb = hausbauer_t::baue(welt, NULL, pos, gebaeude_layout[streetdir], h);
			add_gebaeude_to_stadt(gb);
		}
	}
}


void stadt_t::erzeuge_verkehrsteilnehmer(koord pos, sint32 level, koord target)
{
	int const verkehr_level = welt->get_settings().get_verkehr_level();
	if (verkehr_level > 0 && level % (17 - verkehr_level) == 0) {
		koord k;
		for (k.y = pos.y - 1; k.y <= pos.y + 1; k.y++) {
			for (k.x = pos.x - 1; k.x <= pos.x + 1; k.x++) {
				if (welt->ist_in_kartengrenzen(k)) {
					grund_t* gr = welt->lookup_kartenboden(k);
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
	if(  (sum_industrie > sum_gewerbe  &&  sum_industrie > sum_wohnung) || (sum_gewerbe > sum_wohnung  &&  will_haben == gebaeude_t::unbekannt)  ) {
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
			grund_t* gr = welt->lookup_kartenboden(k + neighbours[i]);
			if (gr != NULL && gr->get_weg_hang() == gr->get_grund_hang()) {
				if (weg_t* const weg = gr->get_weg(road_wt)) {
					if (i < 4) {
						// update directions (SENW)
						streetdir += (1 << i);
					}
					weg->set_gehweg(true);
					// if not current city road standard, then replace it
					if (weg->get_besch() != welt->get_city_road()) {
						spieler_t *sp = weg->get_besitzer();
						if (sp == NULL  ||  !gr->get_depot()) {
							spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());

							weg->set_besitzer(NULL); // make public
							weg->set_besch(welt->get_city_road());
						}
					}
					gr->calc_bild();
					reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
				} else if (gr->get_typ() == grund_t::fundament) {
					// do not renovate, if the building is already in a neighbour tile
					gebaeude_t const* const gb = ding_cast<gebaeude_t>(gr->first_obj());
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
		gb->mark_images_dirty();
		gb->set_tile( h->get_tile(gebaeude_layout[streetdir], 0, 0), true );
		welt->lookup_kartenboden(k)->calc_bild();
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
	grund_t* bd = welt->lookup_kartenboden(k);

	if (bd->get_typ() != grund_t::boden) {
		// not on water, monorails, foundations, tunnel or bridges
		return false;
	}

	// we must not built on water or runways etc.
	if(  bd->hat_wege()  &&  !bd->hat_weg(road_wt)  &&  !bd->hat_weg(track_wt)  ) {
		return false;
	}

	// somebody else's things on it?
	if(  bd->kann_alle_obj_entfernen(NULL)  ) {
		return false;
	}

	// dwachs: If not able to built here, try to make artificial slope
	hang_t::typ slope = bd->get_grund_hang();
	if (!hang_t::ist_wegbar(slope)) {
		if (welt->can_ebne_planquadrat(k, bd->get_hoehe()+1, true)) {
			welt->ebne_planquadrat(NULL, k, bd->get_hoehe()+1, true);
		}
		else if (bd->get_hoehe() > welt->get_grundwasser()  &&  welt->can_ebne_planquadrat(k, bd->get_hoehe())) {
			welt->ebne_planquadrat(NULL, k, bd->get_hoehe());
		}
		else {
			return false;
		}
	}

	// initially allow all possible directions ...
	ribi_t::ribi allowed_dir = (bd->get_grund_hang() != hang_t::flach ? ribi_t::doppelt(ribi_typ(bd->get_weg_hang())) : (ribi_t::ribi)ribi_t::alle);

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
			allowed_dir = ribi_t::doppelt( ribi_t::layout_to_ribi[gb->get_tile()->get_layout() & 1] );
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

	// determine now, in which directions we can connect to another road
	ribi_t::ribi connection_roads = ribi_t::keine;
	// add ribi's to connection_roads if possible
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & allowed_dir) {
			// now we have to check for several problems ...
			grund_t* bd2;
			if(bd->get_neighbour(bd2, invalid_wt, ribi_t::nsow[r])) {
				if(bd2->get_typ()==grund_t::fundament  ||  bd2->get_typ()==grund_t::wasser) {
					// not connecting to a building of course ...
				} else if (!bd2->ist_karten_boden()) {
					// do not connect to elevated ways / bridges
				} else if (bd2->get_typ()==grund_t::tunnelboden  &&  ribi_t::nsow[r]!=ribi_typ(bd2->get_grund_hang())) {
					// not the correct slope
				} else if (bd2->get_typ()==grund_t::brueckenboden
					&&  (bd2->get_grund_hang()==hang_t::flach  ?  ribi_t::nsow[r]!=ribi_typ(bd2->get_weg_hang())
					                                           :  ribi_t::rueckwaerts(ribi_t::nsow[r])!=ribi_typ(bd2->get_grund_hang()))) {
					// not the correct slope
				} else if(bd2->hat_weg(road_wt)) {
					const gebaeude_t* gb = bd2->find<gebaeude_t>();
					if(gb) {
						uint8 layouts = gb->get_tile()->get_besch()->get_all_layouts();
						// nothing to connect
						if(layouts==4) {
							// single way
							if(ribi_t::nsow[r]==ribi_t::rueckwaerts(ribi_t::layout_to_ribi[gb->get_tile()->get_layout()])) {
								// allowed ...
								connection_roads |= ribi_t::nsow[r];
							}
						}
						else if(layouts==2 || layouts==8 || layouts==16) {
							// through way
							if((ribi_t::doppelt( ribi_t::layout_to_ribi[gb->get_tile()->get_layout() & 1] )&ribi_t::nsow[r])!=0) {
								// allowed ...
								connection_roads |= ribi_t::nsow[r];
							}
						}
						else {
							dbg->error("stadt_t::baue_strasse()", "building on road with not directions at %i,%i?!?", k.x, k.y );
						}
					}
					else if(bd2->get_depot()) {
						// do not enter depots
					}
					else {
						// check slopes
						if (wegbauer_t::check_slope(bd, bd2 )) {
							// allowed ...
							connection_roads |= ribi_t::nsow[r];
						}
					}
				}
			}
		}
	}

	// now add the ribis to the other ways (if there)
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & connection_roads) {
			grund_t* bd2 = welt->lookup_kartenboden(k + koord::nsow[r]);
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
		// check to bridge a river
		if (ribi_t::ist_einfach(connection_roads)) {
			koord zv = koord(ribi_t::rueckwaerts(connection_roads));
			grund_t *bd_next = welt->lookup_kartenboden( k + zv );
			if (bd_next  &&  (bd_next->ist_wasser()  ||  (bd_next->hat_weg(water_wt)  &&  bd_next->get_weg(water_wt)->get_besch()->get_styp()==255))) {
				// ok there is a river
				const bruecke_besch_t *bridge = brueckenbauer_t::find_bridge(road_wt, 50, welt->get_timeline_year_month() );
				if(  bridge==NULL  ) {
					// does not have a bridge available ...
					return false;
				}
				const char *err = NULL;
				koord3d end = brueckenbauer_t::finde_ende(welt, NULL, bd->get_pos(), zv, bridge, err, false);
				if (err  ||   koord_distance( k, end.get_2d())>3) {
					// try to find shortest possible
					end = brueckenbauer_t::finde_ende(welt, NULL, bd->get_pos(), zv, bridge, err, true);
				}
				if (err==NULL  &&   koord_distance( k, end.get_2d())<=3) {
					brueckenbauer_t::baue_bruecke(welt, welt->get_spieler(1), bd->get_pos(), end, zv, bridge, welt->get_city_road());
					// try to build one connecting piece of road
					baue_strasse( (end+zv).get_2d(), NULL, false);
					// try to build a house near the bridge end
					uint32 old_count = buildings.get_count();
					for(uint8 i=0; i<lengthof(koord::neighbours)  &&  buildings.get_count() == old_count; i++) {
						baue_gebaeude(end.get_2d()+zv+koord::neighbours[i]);
					}
				}
			}
		}
		return true;
	}

	return false;
}


// will check a single random pos in the city, then baue will be called
void stadt_t::baue()
{
	const koord k(lo + koord::koord_random(ur.x - lo.x + 2,ur.y - lo.y + 2)-koord(1,1) );

	// do not build on any border tile
	if(  !welt->ist_in_kartengrenzen(k+koord(1,1))  ||  k.x<=0  ||  k.y<=0  ) {
		return;
	}

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr==NULL) {
		return;
	}

	// checks only make sense on empty ground
	if(gr->ist_natur()) {

		// since only a single location is checked, we can stop after we have found a positive rule
		best_strasse.reset(k);
		const uint32 num_road_rules = road_rules.get_count();
		uint32 offset = simrand(num_road_rules);	// start with random rule
		for (uint32 i = 0; i < num_road_rules  &&  !best_strasse.found(); i++) {
			uint32 rule = ( i+offset ) % num_road_rules;
			bewerte_strasse(k, 8 + road_rules[rule]->chance, *road_rules[rule]);
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
		const uint32 num_house_rules = house_rules.get_count();
		offset = simrand(num_house_rules);	// start with random rule
		for (uint32 i = 0; i < num_house_rules  &&  !best_haus.found(); i++) {
			uint32 rule = ( i+offset ) % num_house_rules;
			bewerte_haus(k, 8 + house_rules[rule]->chance, *house_rules[rule]);
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
		// try to find a public owned building
		for(uint8 i=0; i<4; i++) {
			gebaeude_t* const gb = pick_any(buildings);
			if(  spieler_t::check_owner(gb->get_besitzer(),NULL)  ) {
				renoviere_gebaeude(gb);
				break;
			}
		}
		INT_CHECK("simcity 876");
	}
}


// find suitable places for cities
vector_tpl<koord>* stadt_t::random_place(const karte_t* wl, const sint32 anzahl, sint16 old_x, sint16 old_y)
{
	int cl = 0;
	for (int i = 0; i < MAX_CLIMATES; i++) {
		if (hausbauer_t::get_special(0, haus_besch_t::rathaus, wl->get_timeline_year_month(), false, (climate)i)) {
			cl |= (1 << i);
		}
	}
	DBG_DEBUG("karte_t::init()", "get random places in climates %x", cl);
	// search at least places which are 5x5 squares large
	slist_tpl<koord>* list = wl->finde_plaetze( 5, 5, (climate_bits)cl, old_x, old_y);
	DBG_DEBUG("karte_t::init()", "found %i places", list->get_count());
	vector_tpl<koord>* result = new vector_tpl<koord>(anzahl);

	// pre processed array: max 1 city from each square can be built
	// each entry represents a cell of minimum_city_distance/2 length and width
	const uint32 minimum_city_distance = wl->get_settings().get_minimum_city_distance();
	const uint32 xmax = (2*wl->get_groesse_x())/minimum_city_distance+1;
	const uint32 ymax = (2*wl->get_groesse_y())/minimum_city_distance+1;
	array2d_tpl< vector_tpl<koord> > places(xmax, ymax);
	while (!list->empty()) {
		const koord k = list->remove_first();
		places.at( (2*k.x)/minimum_city_distance, (2*k.y)/minimum_city_distance).append(k);
	}
	// weigthed index vector into places array
	weighted_vector_tpl<koord> index_to_places(xmax*ymax);
	for(uint32 i=0; i<xmax; i++) {
		for(uint32 j=0; j<ymax; j++) {
			vector_tpl<koord> const& p = places.at(i, j);
			if (!p.empty()) {
				index_to_places.append(koord(i,j), p.get_count());
			}
		}
	}
	// post-processing array:
	// each entry represents a cell of minimum_city_distance length and width
	// to limit the search for neighboring cities
	const uint32 xmax2 = wl->get_groesse_x()/minimum_city_distance+1;
	const uint32 ymax2 = wl->get_groesse_y()/minimum_city_distance+1;
	array2d_tpl< vector_tpl<koord> > result_places(xmax2, ymax2);

	for (int i = 0; i < anzahl; i++) {
		// check distances of all cities to their respective neightbours
		while (!index_to_places.empty()) {
			// find a random cell
			koord const ip = pick_any_weighted(index_to_places);
			// remove this cell from index list
			index_to_places.remove(ip);
			vector_tpl<koord>& p = places.at(ip);
			// get random place in the cell
			if (p.empty()) continue;
			uint32 const j = simrand(p.get_count());
			koord  const k = p[j];

			const koord k2mcd = koord( k.x/minimum_city_distance, k.y/minimum_city_distance );
			for (sint32 i = k2mcd.x - 1; i <= k2mcd.x + 1; ++i) {
				for (sint32 j = k2mcd.y - 1; j <= k2mcd.y + 1; ++j) {
					if (i>=0 && i<(sint32)xmax2 && j>=0 && j<(sint32)ymax2) {
						FOR(vector_tpl<koord>, const& l, result_places.at(i, j)) {
							if (koord_distance(k, l) < minimum_city_distance) {
								goto too_close;
							}
						}
					}
				}
			}

			// all citys are far enough => ok, find next place
			result->append(k);
			result_places.at(k2mcd).append(k);
			break;

too_close:
			// remove the place from the list
			p.remove_at(j);
			// re-insert in index list with new weight
			if (!p.empty()) {
				index_to_places.append(ip, p.get_count());
			}
			// if we reached here, the city was not far enough => try again
		}

		if (index_to_places.empty() && i < anzahl - 1) {
			dbg->warning("stadt_t::random_place()", "Not enough places found!");
			break;
		}
	}
	delete list;

	return result;
}
