/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../obj/way/strasse.h"
#include "../ground/grund.h"
#include "../ground/boden.h"
#include "../gui/simwin.h"
#include "simworld.h"
#include "../simware.h"
#include "../player/simplay.h"
#include "simplan.h"
#include "../display/simimg.h"
#include "../vehicle/simroadtraffic.h"
#include "../simhalt.h"
#include "../simfab.h"
#include "simcity.h"
#include "../simmesg.h"
#include "../simcolor.h"

#include "../gui/minimap.h"
#include "../gui/city_info.h"

#include "../descriptor/building_desc.h"

#include "../simintr.h"
#include "../simdebug.h"

#include "../obj/gebaeude.h"

#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/environment.h"

#include "building_placefinder.h"
#include "../builder/wegbauer.h"
#include "../builder/brueckenbauer.h"
#include "../builder/hausbauer.h"
#include "../builder/fabrikbauer.h"
#include "../utils/cbuffer.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"


#define PACKET_SIZE (7)

/**
 * This variable is used to control the fractional precision of growth to prevent loss when quantities are small.
 * Growth calculations use 64 bit signed integers.
 * Although this is actually scale factor, a power of two is recommended for optimization purposes.
 */
static sint64 const CITYGROWTH_PER_CITIZEN = 1ll << 32; // Q31.32 fractional form.

karte_ptr_t stadt_t::welt; // one is enough ...


/********************************* From here on cityrules stuff *****************************************/


/**
 * in this fixed interval, construction will happen
 * 21s = 21000 per house
 */
const uint32 stadt_t::city_growth_step = 21000;

/**
 * this is the default factor to prefer clustering
 */
uint32 stadt_t::cluster_factor = 10;

/*
 * chance to do renovation instead new building (in percent)
 */
static uint32 renovation_percentage = 12;

/*
 * minimum ratio of city area to building area to allow expansion
 * the higher this value, the slower the city expansion if there are still "holes"
 */
static uint32 min_building_density = 25;

// the following are the scores for the different building types
static sint16 ind_start_score =   0;
static sint16 com_start_score = -10;
static sint16 res_start_score =   0;

// order: res, com, ind
static sint16 ind_neighbour_score[] = { -8, 0,  8 };
static sint16 com_neighbour_score[] = {  1, 8,  1 };
static sint16 res_neighbour_score[] = {  8, 0, -8 };

/**
 * Rule data structure
 * maximum 7x7 rules
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
 * S = not a road
 * s = is a road
 * n = is nature/empty
 * H = not a house
 * h = is a house
 * T = not a stop // added in 88.03.3
 * t = is a stop  // added in 88.03.3
 * u = good slope for way
 * U = not a slope for ways
 * . = beliebig
 */

// here '.' is ignored, since it will not be tested anyway
static char const* const allowed_chars_in_rule = "SsnHhTtUu";

/**
 * @param pos position to check
 * @param regel the rule to evaluate
 * @return true on match, false otherwise
 */
bool stadt_t::bewerte_loc(const koord pos, const rule_t &regel, int rotation)
{
	//printf("Test for (%s) in rotation %d\n", pos.get_str(), rotation);
	koord k;

	for(rule_entry_t const& r : regel.rule) {
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
				if (gr->get_typ() != grund_t::fundament  ||  gr->obj_bei(0)->get_typ()!=obj_t::gebaeude) return false;
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
				if (!slope_t::is_way(gr->get_grund_hang())) return false;
				break;
			case 'u':
				// road may be buildable
				if (slope_t::is_way(gr->get_grund_hang())) return false;
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
 * @note but the rules should explicitly forbid building then?!?
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
 */
bool stadt_t::cityrules_init()
{
	tabfile_t cityconf;
	// first take user data, then user global data
	const std::string user_dir=env_t::user_dir;
	if (!cityconf.open((user_dir+"cityrules.tab").c_str())) {
		if (!cityconf.open((user_dir+"addons/"+env_t::pak_name+"config/cityrules.tab").c_str())) {
			if (!cityconf.open((env_t::pak_dir+"config/cityrules.tab").c_str())) {
				dbg->fatal("stadt_t::init()", "Can't read cityrules.tab" );
			}
		}
	}

	tabfileobj_t contents;
	cityconf.read(contents);

	char buf[128];

	cluster_factor = (uint32)contents.get_int("cluster_factor", 10);
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
		const uint8 offset = (7 - (uint8)size) / 2;
		for (uint y = 0; y < size; y++) {
			for (uint x = 0; x < size; x++) {
				const char flag = rule[x + y * (size + 1)];
				// check for allowed characters; ignore '.';
				// leave midpoint out, should be 'n', which is checked in build() anyway
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
		const uint8 offset = (7 - (uint8)size) / 2;
		for (uint y = 0; y < size; y++) {
			for (uint x = 0; x < size; x++) {
				const char flag = rule[x + y * (size + 1)];
				// check for allowed characters; ignore '.';
				// leave midpoint out, should be 'n', which is checked in build() anyway
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
*/
void stadt_t::cityrules_rdwr(loadsave_t *file)
{
	if(  file->is_version_atleast(112, 8)  ) {
		file->rdwr_long( cluster_factor );
	}

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
 * Search a free place for a monument building
 * Im Gegensatz zum building_placefinder_t werden Strassen auf den Raendern
 * toleriert.
 */
class monument_placefinder_t : public placefinder_t {
	public:
		monument_placefinder_t(karte_t* welt, sint16 radius) : placefinder_t(welt, radius) {}

		bool is_tile_ok(koord pos, koord d, climate_bits cl) const OVERRIDE
		{
			const planquadrat_t* plan = welt->access(pos + d);

			// can't build here
			if (plan == NULL) {
				return false;
			}

			const grund_t* gr = plan->get_kartenboden();
			if(  ((1 << welt->get_climate( gr->get_pos().get_2d() )) & cl) == 0  ) {
				return false;
			}

			if (is_boundary_tile(d)) {
				return
					gr->get_grund_hang() == slope_t::flat &&     // Flat
					gr->get_typ() == grund_t::boden &&           // Boden -> no building
					(!gr->hat_wege() || gr->hat_weg(road_wt)) && // only roads
					gr->kann_alle_obj_entfernen(NULL) == NULL;   // Irgendwas verbaut den Platz?
			}
			else {
				return
					gr->get_grund_hang() == slope_t::flat &&
					gr->get_typ() == grund_t::boden &&
					gr->ist_natur() &&                         // No way here
					gr->kann_alle_obj_entfernen(NULL) == NULL; // Irgendwas verbaut den Platz?
			}
		}
};


/**
 * townhall_placefinder_t:
 */
class townhall_placefinder_t : public placefinder_t {
	public:
		townhall_placefinder_t(karte_t* welt, uint8 dir_) : placefinder_t(welt), dir(dir_) {}

		bool is_tile_ok(koord pos, koord d, climate_bits cl) const OVERRIDE
		{
			const grund_t* gr = welt->lookup_kartenboden(pos + d);
			if (gr == NULL  ||  gr->get_grund_hang() != slope_t::flat) {
				return false;
			}

			if(  ((1 << welt->get_climate( gr->get_pos().get_2d() )) & cl) == 0  ) {
				return false;
			}

			if (d.x > 0 || d.y > 0) {
				if (welt->lookup_kartenboden(pos)->get_hoehe() != gr->get_hoehe()) {
					// height wrong!
					return false;
				}
			}

			if ( ((dir & ribi_t::south)!=0  &&  d.y == h - 1) ||
				((dir & ribi_t::west)!=0  &&  d.x == 0) ||
				((dir & ribi_t::north)!=0  &&  d.y == 0) ||
				((dir & ribi_t::east)!=0  &&  d.x == w - 1)) {
				// we want to build a road here:
				return
					gr->get_typ() == grund_t::boden &&
					(!gr->hat_wege() || (gr->hat_weg(road_wt) && !gr->has_two_ways())) && // build only on roads, no other ways
					!gr->is_halt() &&
					gr->kann_alle_obj_entfernen(NULL) == NULL;
			} else {
				// we want to build the townhall here: maybe replace existing buildings
				return ((gr->get_typ()==grund_t::boden  &&  gr->ist_natur()) || gr->get_typ()==grund_t::fundament) &&
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
	static vector_tpl<grund_t*> gb_tiles;
	if (gb != NULL) {
		uint16 level = gb->get_tile()->get_desc()->get_level()+1;
		gb->get_tile_list( gb_tiles );
		for(grund_t* gr : gb_tiles ) {
			gebaeude_t* add_gb = gr->find<gebaeude_t>();
			if( ordered ) {
				buildings.insert_ordered( add_gb, level, compare_gebaeude_pos );
			}
			else {
				buildings.append( add_gb, level );
			}
			add_gb->set_stadt( this );
			if( add_gb->get_tile()->get_desc()->is_townhall() ) {
				has_townhall = true;
			}
		}
		// no update of city limits
		// as has_low_density may depend on the order the buildings list is filled
		if (!ordered) {
			// check borders
			koord size = gb_tiles.back()->get_pos().get_2d()-gb_tiles.front()->get_pos().get_2d()+koord( 1, 1 );
			pruefe_grenzen(pos, size);
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
	buildings.append(gb, gb->get_tile()->get_desc()->get_level() + 1);
}


void stadt_t::pruefe_grenzen(koord /*k*/, koord /*size*/)
{
	/* somehow we sometimes end up during growth with a high density
	 * city that is not recognized as such (happens more frequently with pak64.japan)
	 * Therefore, we just recalc the borders each time; building events
	 * are few, and the penalty is not strong.
	 * See version control for old code.
	 */
	recalc_city_size();
}



// returns true, if there is a halt serving at least one building of the town.
bool stadt_t::is_within_players_network(const player_t* player) const
{
	vector_tpl<halthandle_t> halts;
	// Find all stations whose coverage affects this city
	for(  weighted_vector_tpl<gebaeude_t *>::const_iterator i = buildings.begin();  i != buildings.end();  ++i  ) {
		gebaeude_t* gb = *i;
		if(  const planquadrat_t *plan = welt->access( gb->get_pos().get_2d() )  ) {
			if(  plan->get_haltlist_count() > 0   ) {
				const halthandle_t *const halt_list = plan->get_haltlist();
				for(  int h = 0;  h < plan->get_haltlist_count();  h++  ) {
					if(  halt_list[h].is_bound()  &&  (halt_list[h]->get_pax_enabled()  || halt_list[h]->get_mail_enabled() )  &&  halt_list[h]->has_available_network(player)  ) {
						return true;
					}
				}
			}
		}
	}
	return false;
}



// recalculate the spreading of a city
// will be updated also after house deletion
void stadt_t::recalc_city_size()
{
	// WARNING: do not call this during multithreaded loading,
	// as has_low_density may depend on the order the buildings list is filled
	lo = pos;
	ur = pos;
	for(gebaeude_t* const i : buildings) {
		if (i->get_tile()->get_desc()->get_type() != building_desc_t::headquarters) {
			koord const& gb_pos = i->get_pos().get_2d();
			koord const& gb_size = i->get_tile()->get_desc()->get_size(i->get_tile()->get_layout());
			lo.clip_max(gb_pos);
			ur.clip_min(gb_pos + gb_size);
		}
	}

	has_low_density = (buildings.get_count()<10  ||  (buildings.get_count()*100l)/((ur.x-lo.x)*(ur.y-lo.y)+1) > min_building_density);
	if(  has_low_density  ) {
		// wider borders for faster growth of sparse small towns
		lo -= koord(2,2);
		ur += koord(2,2);
	}

	lo.clip_min(koord(0,0));
	ur.clip_max(koord(welt->get_size().x-1,welt->get_size().y-1));
}


void stadt_t::init_pax_destinations()
{
	pax_destinations_old.clear();
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;
}


void stadt_t::factory_entry_t::rdwr(loadsave_t *file)
{
	if(  file->is_version_atleast(110, 5)  ) {
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
	factory = fabrik_t::get_fab( koord(factory_pos_x, factory_pos_y) );
}


const stadt_t::factory_entry_t* stadt_t::factory_set_t::get_entry(const fabrik_t *const factory) const
{
	for(factory_entry_t const& e : entries) {
		if (e.factory == factory) {
			return &e;
		}
	}

	return NULL; // not found
}


stadt_t::factory_entry_t* stadt_t::factory_set_t::get_random_entry()
{
	if(  total_remaining>0  ) {
		sint32 weight = simrand(total_remaining);
		for(factory_entry_t & entry : entries) {
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
	ratio_stale = true; // always trigger recalculation of ratio
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
#define SUPPLY_FACTOR (9) // out of 2^SUPPLY_BITS

void stadt_t::factory_set_t::recalc_generation_ratio(const sint32 default_percent, const sint64 *city_stats, const int stats_count, const int stat_type)
{
	ratio_stale = false; // reset flag

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
	}
	else if(  !welt->get_settings().get_factory_enforce_demand()  ||  average_generated == 0  ) {
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
	if(  welt->get_settings().get_factory_enforce_demand()  &&  (generation_ratio >> RATIO_BITS) == (uint32)default_percent && average_generated > 0 && total_demand > 0  ) {
		const sint64 supply_promille = ( ( (average_generated << 10) * (sint64)default_percent ) / 100 ) / (sint64)target_supply;
		if(  supply_promille < 1024  ) {
			// expected supply is really smaller than target supply
			for(factory_entry_t & entry : entries) {
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
	for(factory_entry_t & entry : entries) {
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
	for(factory_entry_t & e : entries) {
		e.new_month();
	}
	total_remaining = 0;
	total_generated = 0;
	ratio_stale = true;
}


void stadt_t::factory_set_t::rdwr(loadsave_t *file)
{
	if(  file->is_version_atleast(110, 5)  ) {
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
	for(factory_entry_t & e : entries) {
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

	if(  minimap_t::get_instance()->is_city_selected(this)  ) {
		minimap_t::get_instance()->set_selected_city(NULL);
	}

	// only if there is still a world left to delete from
	if( welt->get_size().x > 1 ) {

		welt->lookup_kartenboden(pos)->set_text(NULL);

		if (!welt->is_destroying()) {
			// remove city info and houses
			while (!buildings.empty()) {

				gebaeude_t* const gb = buildings.pop_back();
				assert(  gb!=NULL  &&  !buildings.is_contained(gb)  );

				if(gb->get_tile()->get_desc()->get_type()==building_desc_t::headquarters) {
					stadt_t *city = welt->find_nearest_city(gb->get_pos().get_2d());
					gb->set_stadt( city );
					if(city) {
						city->buildings.append(gb, gb->get_passagier_level());
					}
				}
				else {
					gb->set_stadt( NULL );
					hausbauer_t::remove(welt->get_public_player(),gb);
				}
			}
			// avoid the bookkeeping if world gets destroyed
		}
	}
}


static bool name_used(weighted_vector_tpl<stadt_t*> const& cities, char const* const name)
{
	for(stadt_t* const i : cities) {
		if (strcmp(i->get_name(), name) == 0) {
			return true;
		}
	}
	return false;
}


stadt_t::stadt_t(player_t* player, koord pos, sint32 citizens) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
{
	assert(welt->is_within_limits(pos));

	step_count = 0;
	pax_destinations_new_change = 0;
	next_step = 0;
	step_interval = 1;
	next_growth_step = 0;
	has_low_density = false;
	has_townhall = false;

	stadtinfo_options = 3; // citizen and growth

	owner = player;

	this->pos = pos;
	last_center = koord::invalid;

	bev = 0;
	arb = 0;
	won = 0;

	lo = ur = pos;

	/* get a unique cityname */
	char                          const* n       = "simcity";
	weighted_vector_tpl<stadt_t*> const& staedte = welt->get_cities();

	const vector_tpl<char*>& city_names = translator::get_city_name_list();

	// make sure we do only ONE random call regardless of how many names are available (to avoid desyncs in network games)
	if(  const uint32 count = city_names.get_count()  ) {
		uint32 idx = simrand( count );
		static const uint32 some_primes[] = { 19, 31, 109, 199, 409, 571, 631, 829, 1489, 1999, 2341, 2971, 3529, 4621, 4789, 7039, 7669, 8779, 9721 };
		// find prime that does not divide count
		uint32 offset = 1;
		for(  uint8 i=0;  i < lengthof(some_primes);  i++  ) {
			if(  count % some_primes[i] != 0  ) {
				offset = some_primes[i];
				break;
			}
		}
		// as count % offset != 0 we are guaranteed to test all city names
		for(uint32 i=0; i<count; i++) {
			char const* const cand = city_names[idx];
			if(  !name_used(staedte, cand)  ) {
				n = cand;
				break;
			}
			idx = (idx+offset) % count;
		}
	}
	else {
		/* the one random call to avoid desyncs */
		simrand(5);
	}
	DBG_MESSAGE("stadt_t::stadt_t()", "founding new city named '%s'", n);
	name = n;
	has_townhall = false;

	// 1. Rathaus bei 0 Leuten bauen
	check_bau_townhall(true);

	unsupplied_city_growth = 0;
	allow_citygrowth = true;

	// only build any houses if townhall is already there
	// city should be deleted if it has no buildings
	if (!buildings.empty()) {
		change_size( citizens, true );
	}

	// fill with start citizen ...
	sint64 bew = get_einwohner();
	for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
		city_history_year[year][HIST_CITIZENS] = bew;
	}
	for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
		city_history_month[month][HIST_CITIZENS] = bew;
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
	city_history_year[0][HIST_CITIZENS]  = get_einwohner();
	city_history_month[0][HIST_CITIZENS] = get_einwohner();
#ifdef DESTINATION_CITYCARS
	number_of_cars = 0;
#endif
}


stadt_t::stadt_t(loadsave_t* file) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
{
	step_count = 0;
	next_step = 0;
	step_interval = 1;
	next_growth_step = 0;
	has_low_density = false;
	has_townhall = false;

	unsupplied_city_growth = 0;
	stadtinfo_options = 3;

	rdwr(file);
}


void stadt_t::rdwr(loadsave_t* file)
{
	sint32 owner_n;

	if (file->is_saving()) {
		owner_n = welt->sp2num(owner);
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
	file->rdwr_long(owner_n);
	file->rdwr_long(bev);
	file->rdwr_long(arb);
	file->rdwr_long(won);

	if(  file->is_version_atleast(112, 9)  ) {
		// Must record the partial (less than 1 citizen) growth factor
		// Otherwise we will get network desyncs
		// Also allows accumulation of small growth factors
		file->rdwr_longlong(unsupplied_city_growth);
	}
	else if( file->is_loading()  ) {
		unsupplied_city_growth = 0;
	}
	// old values zentrum_namen_cnt : aussen_namen_cnt
	if(file->is_version_less(99, 18)) {
		sint32 dummy=0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}

	if (file->is_loading()) {
		owner = welt->get_player(owner_n);
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
		city_history_year[0][HIST_CITIZENS] = get_einwohner();
		city_history_year[0][HIST_CITIZENS] = get_einwohner();
	}

	// we probably need to load/save the city history
	if (file->is_version_less(86, 0)) {
		DBG_DEBUG("stadt_t::rdwr()", "is old version: No history!");
	} else if(file->is_version_less(99, 16)) {
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
	else if(  file->is_version_less(120, 1)  ) {
		// 99.17.0 extended city history
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				if(  hist_type==HIST_PAS_WALKED  ||  hist_type==HIST_MAIL_WALKED  ) {
					continue;
				}
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) {
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) {
				if(  hist_type==HIST_PAS_WALKED  ||  hist_type==HIST_MAIL_WALKED  ) {
					continue;
				}
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long( stadtinfo_options);
	}
	else {
		// 120,001 with walking (direct connections) recored seperately
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

	// differential history
	if (  file->is_version_less(120, 1)  ) {
		if (  file->is_loading()  ) {
			// Initalize differential statistics assuming a differential of 0.
			city_growth_get_factors(city_growth_factor_previous, 0);
		}
	}
	else {
		// load/save differential statistics.
		for (uint32 i = 0; i < GROWTH_FACTOR_NUMBER; i++) {
			file->rdwr_longlong(city_growth_factor_previous[i].demand);
			file->rdwr_longlong(city_growth_factor_previous[i].supplied);
		}
	}

	if(file->is_version_atleast(99, 15)  &&  file->is_version_less(99, 16)) {
		sint32 dummy = 0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}

	// since 102.2 there are static cities
	if(file->is_version_atleast(102, 2)) {
		file->rdwr_bool(allow_citygrowth);
	}
	else if(  file->is_loading()  ) {
		allow_citygrowth = true;
	}

	// save townhall road position
	if(file->is_version_atleast(102, 3)) {
		townhall_road.rdwr(file);
	}
	else if(  file->is_loading()  ) {
		townhall_road = koord::invalid;
	}

	// data related to target factories
	target_factories_pax.rdwr( file );
	target_factories_mail.rdwr( file );

	if (file->is_version_atleast(122, 1)) {
		file->rdwr_long(step_count);
		file->rdwr_long(next_step);
		file->rdwr_long(next_growth_step);
	}
	else if (file->is_loading()) {
		step_count = 0;
		next_step = 0;
		next_growth_step = 0;
	}

	if(file->is_loading()) {
		// Due to some bugs in the special buildings/town hall
		// placement code, li,re,ob,un could've gotten irregular values
		// If a game is loaded, the game might suffer from such an mistake
		// and we need to correct it here.
		DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);

		// recalculate borders
		recalc_city_size();
	}
}


void stadt_t::finish_rd()
{
	// there might be broken savegames
	if (!name) {
		set_name( "simcity" );
	}

	if (!has_townhall) {
		dbg->warning("stadt_t::finish_rd()", "City %s has no valid townhall after loading the savegame, try to build a new one.", get_name());
		check_bau_townhall(true);
	}
	// new city => need to grow
	if (buildings.empty()) {
		step_grow_city(true);
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
		gebaeude_t const* const gb = obj_cast<gebaeude_t>(welt->lookup_kartenboden(pos)->first_obj());
		if(  gb  &&  gb->is_townhall()  ) {
			koord k(gb->get_tile()->get_desc()->get_size(gb->get_tile()->get_layout()));
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
	// townhall position may be changed a little!
	sparse_tpl<PIXVAL> pax_destinations_temp(koord( PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE ));

	PIXVAL color;
	koord pos;
	for( uint16 i = 0; i < pax_destinations_new.get_data_count(); i++ ) {
		pax_destinations_new.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set( PAX_DESTINATIONS_SIZE-1-pos.y, pos.x, color );
	}
	swap<PIXVAL>( pax_destinations_temp, pax_destinations_new );

	pax_destinations_temp.clear();
	for( uint16 i = 0; i < pax_destinations_old.get_data_count(); i++ ) {
		pax_destinations_old.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set( PAX_DESTINATIONS_SIZE-1-pos.y, pos.x, color );
	}
	pax_destinations_new_change ++;
	swap<PIXVAL>( pax_destinations_temp, pax_destinations_old );
}


void stadt_t::set_name(const char *new_name)
{
	if (new_name == NULL) {
		return;
	}
	name = new_name;
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gr->set_text( new_name );
	}
	city_info_t *win = dynamic_cast<city_info_t*>(win_get_magic((ptrdiff_t)this));
	if (win) {
		win->update_data();
	}
}


/* show city info dialogue */
void stadt_t::open_info_window()
{
	create_win( new city_info_t(this), w_info, (ptrdiff_t)this );
}


/* calculates the factories which belongs to certain cities */
void stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "search factories near %s (center at %i,%i)", get_name(), pos.x, pos.y);
	assert( target_factories_pax.get_entries().empty() );
	assert( target_factories_mail.get_entries().empty() );

	for(fabrik_t* const fab : welt->get_fab_list()) {
		const uint32 count = fab->get_target_cities().get_count();
		if(  count < welt->get_settings().get_factory_worker_maximum_towns()  &&  koord_distance(fab->get_pos(), pos) < welt->get_settings().get_factory_worker_radius()  ) {
			fab->add_target_city(this);
		}
	}
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "is connected with %i/%i factories (total demand=%i/%i) for pax/mail.", target_factories_pax.get_entries().get_count(), target_factories_mail.get_entries().get_count(), target_factories_pax.total_demand, target_factories_mail.total_demand);
}


/* change size of city */
void stadt_t::change_size( sint64 delta_citizen, bool new_town)
{
	DBG_MESSAGE("stadt_t::change_size()", "%i + %i", bev, delta_citizen);
	if(  delta_citizen > 0  ) {
		unsupplied_city_growth += delta_citizen * CITYGROWTH_PER_CITIZEN;
		step_grow_city(new_town);
	}
	if(  delta_citizen < 0  ) {
		if(  bev > -delta_citizen  ) {
			bev += (sint32)delta_citizen;
		}
		else {
//				remove_city();
			bev = 1;
		}
		step_grow_city(new_town);
	}
	DBG_MESSAGE("stadt_t::change_size()", "%i+%i", bev, delta_citizen);
}


void stadt_t::step(uint32 delta_t)
{
	// recalculate factory going ratios where necessary
	const sint16 factory_worker_percentage = welt->get_settings().get_factory_worker_percentage();
	if(  target_factories_pax.ratio_stale  ) {
		target_factories_pax.recalc_generation_ratio( factory_worker_percentage, *city_history_month, MAX_CITY_HISTORY, HIST_PAS_GENERATED);
	}
	if(  target_factories_mail.ratio_stale  ) {
		target_factories_mail.recalc_generation_ratio( factory_worker_percentage, *city_history_month, MAX_CITY_HISTORY, HIST_MAIL_GENERATED);
	}

	// is it time for the next step?
	next_step += delta_t;
	next_growth_step += delta_t;

	step_interval = (1 << 21U) / (buildings.get_count() * welt->get_settings().get_passenger_factor() + 1);
	if (step_interval < 1) {
		step_interval = 1;
	}

	while(next_growth_step > stadt_t::city_growth_step) {
		calc_growth();
		step_grow_city();
		next_growth_step -= stadt_t::city_growth_step;
	}

	// create passenger rate proportional to town size
	while(next_step > step_interval) {
		step_passagiere();
		step_count++;
		next_step -= step_interval;
	}

	// update history (might be changed do to construction/destroying of houses)
	city_history_month[0][HIST_CITIZENS] = get_einwohner(); // total number
	city_history_year[0][HIST_CITIZENS] = get_einwohner();

	city_history_month[0][HIST_GROWTH] = city_history_month[0][HIST_CITIZENS]-city_history_month[1][HIST_CITIZENS]; // growth
	city_history_year[0][HIST_GROWTH] = city_history_year[0][HIST_CITIZENS]-city_history_year[1][HIST_CITIZENS];

	city_history_month[0][HIST_BUILDING] = buildings.get_count();
	city_history_year[0][HIST_BUILDING] = buildings.get_count();
}


/* updates the city history
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
	city_history_month[0][HIST_CITIZENS] = get_einwohner();
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
		city_history_year[0][HIST_CITIZENS] = get_einwohner();
		city_history_year[0][HIST_BUILDING] = buildings.get_count();
		city_history_year[0][HIST_GOODS_NEEDED] = 0;
	}
}


void stadt_t::city_growth_get_factors(city_growth_factor_t(&factors)[GROWTH_FACTOR_NUMBER], uint32 const month) const {
	// optimize view of history for convenience
	sint64 const (&h)[MAX_CITY_HISTORY] = city_history_month[month];

	// go through each index one at a time
	uint32 index = 0;

	// passenger growth factors
	factors[index  ].demand   = h[HIST_PAS_GENERATED];
	factors[index++].supplied = h[HIST_PAS_TRANSPORTED] + h[HIST_PAS_WALKED];

	// mail growth factors
	factors[index  ].demand   = h[HIST_MAIL_GENERATED];
	factors[index++].supplied = h[HIST_MAIL_TRANSPORTED] + h[HIST_MAIL_WALKED];

	// goods growth factors
	factors[index  ].demand   = h[HIST_GOODS_NEEDED];
	factors[index++].supplied = h[HIST_GOODS_RECEIVED];

	assert(index == GROWTH_FACTOR_NUMBER);
}

sint32 stadt_t::city_growth_base(uint32 const rprec, uint32 const cprec)
{
	// Resolve constant references.
	settings_t const & s = welt->get_settings();
	sint32 const weight[GROWTH_FACTOR_NUMBER] = { s.get_passenger_multiplier(), s.get_mail_multiplier(),
	    s.get_goods_multiplier() };

	sint32 acc = 0; // The weighted satisfaction accululator.
	sint32 div = 0; // The weight dividend.
	sint32 total = 0; // The total weight.

	// initalize const growth array
	city_growth_factor_t growthfactors[GROWTH_FACTOR_NUMBER];
	city_growth_get_factors(growthfactors, 0);

	// Loop through each growth factor and compute it.
	for( uint32 i = 0; i < GROWTH_FACTOR_NUMBER; i += 1 ) {
		// Resolve the weight.
		total += weight[i];

		// Compute the differentials.
		sint64 const had = growthfactors[i].demand   - city_growth_factor_previous[i].demand;
		sint64 const got = growthfactors[i].supplied - city_growth_factor_previous[i].supplied;
		city_growth_factor_previous[i].demand   = growthfactors[i].demand;
		city_growth_factor_previous[i].supplied = growthfactors[i].supplied;

		// If we had anything to satisfy add it to weighting otherwise skip.
		if( had == 0 ) {
			continue;
		}

		// Compute fractional satisfaction.
		sint32 const frac = (sint32)((got << cprec) / had);

		// Add to weight and div.
		acc += frac * weight[i];
		div += weight[i];
	}

	// If there was not anything to satisfy then use last month averages.
	if (  div == 0  ) {
		// initalize growth factor array
		city_growth_factor_t prev_growthfactors[GROWTH_FACTOR_NUMBER];
		city_growth_get_factors(prev_growthfactors, 1);

		for( uint32 i = 0; i < GROWTH_FACTOR_NUMBER; i += 1 ){

			// Extract the values of growth.
			sint64 const had = prev_growthfactors[i].demand;
			sint64 const got = prev_growthfactors[i].supplied;

			// If we had anything to satisfy add it to weighting otherwise skip.
			if( had == 0 ) {
				continue;
			}

			// Compute fractional satisfaction.
			sint32 const frac = (sint32)((got << cprec) / had);

			// Add to weight and div.
			acc += frac * weight[i];
			div += weight[i];
		}
	}

	// Return computed result. If still no demand then assume no growth to prevent self-growing new hamlets.
	return div != 0 ? (total * (acc / div)) >> (cprec - rprec) : 0;
}


void stadt_t::city_growth_monthly(uint32 const month)
{
	// initalize growth factor array
	city_growth_factor_t growthfactors[GROWTH_FACTOR_NUMBER];
	city_growth_get_factors(growthfactors, month);

	// Perform roll over.
	for( uint32 i = 0; i < GROWTH_FACTOR_NUMBER; i += 1 ){
		// Compute the differentials.
		sint64 const had = growthfactors[i].demand   - city_growth_factor_previous[i].demand;
		sint64 const got = growthfactors[i].supplied - city_growth_factor_previous[i].supplied;
		city_growth_factor_previous[i].demand   = -had;
		city_growth_factor_previous[i].supplied = -got;
	}
}


void stadt_t::new_month( bool recalc_destinations )
{
	swap<PIXVAL>( pax_destinations_old, pax_destinations_new );
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;

	city_growth_monthly(0);
	roll_history();
	target_factories_pax.new_month();
	target_factories_mail.new_month();

	const sint16 factory_worker_percentage = welt->get_settings().get_factory_worker_percentage();
//	settings_t const& s = welt->get_settings();
	target_factories_pax.recalc_generation_ratio( factory_worker_percentage, *city_history_month, MAX_CITY_HISTORY, HIST_PAS_GENERATED);
	target_factories_mail.recalc_generation_ratio( factory_worker_percentage, *city_history_month, MAX_CITY_HISTORY, HIST_MAIL_GENERATED);
	recalc_target_cities();

	// center has moved => change attraction weights
	if(  recalc_destinations  ||  last_center != get_center()  ) {
		last_center = get_center();
		recalc_target_attractions();
	}

	if(  !private_car_t::list_empty()  &&  welt->get_settings().get_traffic_level() > 0  ) {
		// spawn eventual citycars
		// the more transported, the less are spawned
		// the larger the city, the more spawned ...

		/* original implementation that is replaced by integer-only version below
		double pfactor = (double)(city_history_month[1][HIST_PAS_TRANSPORTED]) / (double)(city_history_month[1][HIST_PAS_GENERATED]+1);
		double mfactor = (double)(city_history_month[1][HIST_MAIL_TRANSPORTED]) / (double)(city_history_month[1][HIST_MAIL_GENERATED]+1);
		double gfactor = (double)(city_history_month[1][HIST_GOODS_RECEIVED]) / (double)(city_history_month[1][HIST_GOODS_NEEDED]+1);

		double factor = pfactor > mfactor ? (gfactor > pfactor ? gfactor : pfactor ) : mfactor;
		factor = (1.0-factor)*city_history_month[1][HIST_CITIZENS];
		factor = log10( factor );
		*/

		// placeholder for fractions
#		define decl_stat(name, i0, i1) sint64 name##_stat[2]; name##_stat[0] = city_history_month[1][i0];  name##_stat[1] = city_history_month[1][i1]+1;

		// defines and initializes local sint64[2] arrays
		decl_stat(pax, HIST_PAS_TRANSPORTED, HIST_PAS_GENERATED);
		decl_stat(mail, HIST_MAIL_TRANSPORTED, HIST_MAIL_GENERATED);
		decl_stat(good, HIST_GOODS_RECEIVED, HIST_GOODS_NEEDED);

		// true if s1[0] / s1[1] > s2[0] / s2[1]
#		define comp_stats(s1,s2) ( s1[0]*s2[1] > s2[0]*s1[1] )
		// computes (1.0 - s[0]/s[1]) * city_history_month[1][HIST_CITIZENS]
#		define comp_factor(s) (city_history_month[1][HIST_CITIZENS] *( s[1]-s[0] )) / s[1]

		uint32 factor = (uint32)( comp_stats(pax_stat, mail_stat) ? (comp_stats(good_stat, pax_stat) ? comp_factor(good_stat) : comp_factor(pax_stat)) : comp_factor(mail_stat) );
		factor = log10(factor);

#ifndef DESTINATION_CITYCARS
		uint16 number_of_cars = simrand( factor * welt->get_settings().get_traffic_level() ) / 16;

		city_history_month[0][HIST_CITYCARS] = number_of_cars;
		city_history_year[0][HIST_CITYCARS] += number_of_cars;

		koord k;
		koord pos = get_zufallspunkt();
		for (k.y = pos.y - 3; k.y < pos.y + 3; k.y++) {
			for (k.x = pos.x - 3; k.x < pos.x + 3; k.x++) {
				if(number_of_cars==0) {
					return;
				}

				if(  grund_t* gr = welt->lookup_kartenboden(k)  ) {
					if(  weg_t* w = gr->get_weg(road_wt)  ) {
						// do not spawn on privte roads or if there is already a car
						if(  ribi_t::is_twoway(w->get_ribi_unmasked())  &&  player_t::check_owner(NULL,w->get_owner())  &&  gr->find<private_car_t>() == NULL  ) {
							private_car_t* vt = new private_car_t(gr, koord::invalid);
							gr->obj_add(vt);
							welt->sync.add(vt);
							number_of_cars--;
						}
					}
				}
			}
		}

		// correct statistics for ungenerated cars
		city_history_month[0][HIST_CITYCARS] -= number_of_cars;
		city_history_year[0][HIST_CITYCARS] -= number_of_cars;
#else
		city_history_month[0][HIST_CITYCARS] = 0;
		number_of_cars = simrand( factor * welt->get_settings().get_traffic_level() ) / 16;
#endif
	}
}


void stadt_t::calc_growth()
{
	// now iterate over all factories to get the ratio of producing version non-producing factories
	// we use the incoming storage as a measure and we will only look for end consumers (power stations, markets)
	for(factory_entry_t const& i : target_factories_pax.get_entries()) {
		fabrik_t *const fab = i.factory;
		if (fab->get_lieferziele().empty() && !fab->get_suppliers().empty()) {
			// consumer => check for it storage
			const factory_desc_t *const desc = fab->get_desc();
			for(  int i=0;  i<desc->get_supplier_count();  i++  ) {
				city_history_month[0][HIST_GOODS_NEEDED] ++;
				city_history_year[0][HIST_GOODS_NEEDED] ++;
				if(  fab->input_vorrat_an( desc->get_supplier(i)->get_input_type() )>0  ) {
					city_history_month[0][HIST_GOODS_RECEIVED] ++;
					city_history_year[0][HIST_GOODS_RECEIVED] ++;
				}
			}
		}
	}

	// maybe this town should stay static
	if(  !allow_citygrowth  ) {
		unsupplied_city_growth = 0;
		return;
	}

	// Compute base growth.
	sint32 const total_supply_percentage = city_growth_base();

	// By construction, this is a percentage times 2^6 (Q25.6).
	// Although intended to be out of 100, it can be more or less.
	// We will divide it by 2^4=16 for traditional reasons, which means
	// it generates 2^2 (=4) or fewer people at 100%.

	// smaller towns should grow slower to have villages for a longer time
	sint32 const weight_factor =
		bev <  1000 ? welt->get_settings().get_growthfactor_small()  :
		bev < 10000 ? welt->get_settings().get_growthfactor_medium() :
		welt->get_settings().get_growthfactor_large();

	// now compute the growth for this step
	sint32 growth_factor = weight_factor > 0 ? total_supply_percentage / weight_factor : 0;

	// Scale up growth to have a larger fractional component. This allows small growth units to accumulate in the case of long months.
	sint64 new_unsupplied_city_growth = growth_factor * (CITYGROWTH_PER_CITIZEN / 16);

	// Growth is scaled down by month length.
	// The result is that ~ the same monthly growth will occur independent of month length.
	new_unsupplied_city_growth = welt->inverse_scale_with_month_length( new_unsupplied_city_growth );

	// Add the computed growth to the growth accumulator.
	// Future growth scale factors can be applied here.
	unsupplied_city_growth += new_unsupplied_city_growth;
}


// does constructions ...
void stadt_t::step_grow_city( bool new_town )
{
	// Try harder to build if this is a new town
	int num_tries = new_town ? 1000 : 30;

	// since we use internally a finer value ...
	const sint64 growth_steps = unsupplied_city_growth / CITYGROWTH_PER_CITIZEN;
	if(  growth_steps > 0  ) {
		unsupplied_city_growth %= CITYGROWTH_PER_CITIZEN;
	}

	// let city grow in steps of 1
	for(  sint64 n = 0;  n < growth_steps;  n++  ) {
		bev++;

		for(  int i = 0;  i < num_tries  &&  bev * 2 > won + arb + 100;  i++  ) {
			build();
		}

		check_bau_spezial(new_town);
		check_bau_townhall(new_town);
		check_bau_factory(new_town); // add industry? (not during creation)
		INT_CHECK("simcity 275");
	}
}


/* this creates passengers and mail for everything is is therefore one of the CPU hogs of the machine
 * think trice, before applying optimisation here ...
 */
void stadt_t::step_passagiere()
{
	// decide whether to generate passengers or mail
	const bool ispass = simrand(GENERATE_RATIO_PASS + GENERATE_RATIO_MAIL) < GENERATE_RATIO_PASS;
	const goods_desc_t *const wtyp = ispass ? goods_manager_t::passengers : goods_manager_t::mail;
	const uint32 history_type = ispass ? HIST_BASE_PASS : HIST_BASE_MAIL;
	factory_set_t &target_factories = ispass ? target_factories_pax : target_factories_mail;

	// restart at first building?
	if (step_count >= buildings.get_count()) {
		step_count = 0;
	}
	if (buildings.empty()) {
		return;
	}
	const gebaeude_t* gb = buildings[step_count];

	// since now backtravels occur, we damp the numbers a little
	const uint32 num_pax =
		(ispass) ?
			(gb->get_tile()->get_desc()->get_level()      + 6) >> 2 :
			(gb->get_tile()->get_desc()->get_mail_level() + 8) >> 3 ;

	// create pedestrians in the near area?
	if (welt->get_settings().get_random_pedestrians()  &&  ispass) {
		haltestelle_t::generate_pedestrians(gb->get_pos(), num_pax);
	}

	// suitable start search
	const koord origin_pos = gb->get_pos().get_2d();
	const planquadrat_t *const plan = welt->access(origin_pos);
	const halthandle_t *const halt_list = plan->get_haltlist();

	// suitable start search
	static vector_tpl<halthandle_t> start_halts(16);
	start_halts.clear();
	for (uint h = 0; h < plan->get_haltlist_count(); h++) {
		halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(wtyp)  &&  !halt->is_overcrowded(wtyp->get_index())  ) {
			start_halts.append(halt);
		}
	}

	// track number of generated passengers.
	city_history_year[0][history_type + HIST_OFFSET_GENERATED] += num_pax;
	city_history_month[0][history_type + HIST_OFFSET_GENERATED] += num_pax;

	// only continue, if this is a good start halt
	if(  !start_halts.empty()  ) {
		// Find passenger destination
		for(  uint pax_routed=0, pax_left_to_do=0;  pax_routed < num_pax;  pax_routed += pax_left_to_do  ) {
			// number of passengers that want to travel
			// for efficiency we try to route not every single pax, but packets.
			// If possible, we do 7 passengers at a time
			// the last packet might have less than 7 pax
			pax_left_to_do = min(PACKET_SIZE, num_pax - pax_routed);

			// search target for the passenger
			pax_return_type will_return;
			factory_entry_t *factory_entry = NULL;
			stadt_t *dest_city = NULL;
			const koord dest_pos = find_destination(target_factories, city_history_month[0][history_type + HIST_OFFSET_GENERATED], &will_return, factory_entry, dest_city);
			if(  factory_entry  ) {
				if (welt->get_settings().get_factory_enforce_demand()) {
					// ensure no more than remaining amount
					pax_left_to_do = min( pax_left_to_do, factory_entry->remaining );
					factory_entry->remaining -= pax_left_to_do;
					target_factories.total_remaining -= pax_left_to_do;
				}
				target_factories.total_generated += pax_left_to_do;
				factory_entry->factory->book_stat(pax_left_to_do, ispass ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED);
			}

			ware_t pax(wtyp);
			pax.set_zielpos(dest_pos);
			pax.menge = pax_left_to_do;
			pax.to_factory = ( factory_entry ? 1 : 0 );

			ware_t return_pax(wtyp);

			// now, finally search a route; this consumes most of the time
			int const route_result = haltestelle_t::search_route( &start_halts[0], start_halts.get_count(), welt->get_settings().is_no_routing_over_overcrowding(), pax, &return_pax);
			halthandle_t start_halt = return_pax.get_ziel();
			if(  route_result==haltestelle_t::ROUTE_OK  ) {
				// so we have happy traveling passengers
				start_halt->starte_mit_route(pax);
				start_halt->add_pax_happy(pax.menge);

				// people were transported so are logged
				city_history_year[0][history_type + HIST_OFFSET_TRANSPORTED] += pax_left_to_do;
				city_history_month[0][history_type + HIST_OFFSET_TRANSPORTED] += pax_left_to_do;

				// destination logged
				merke_passagier_ziel(dest_pos, color_idx_to_rgb(COL_YELLOW));
			}
			else if(  route_result==haltestelle_t::ROUTE_WALK  ) {
				if(  factory_entry  ) {
					// workers and mail delivered instantly to factory
					factory_entry->factory->liefere_an(wtyp, pax_left_to_do);
				}

				// log walked at stop
				start_halt->add_pax_walked(pax_left_to_do);

				// people who walk or deliver by hand logged as walking
				city_history_year[0][history_type + HIST_OFFSET_WALKED] += pax_left_to_do;
				city_history_month[0][history_type + HIST_OFFSET_WALKED] += pax_left_to_do;

				// probably not a good idea to mark them as player only cares about remote traffic
				//merke_passagier_ziel(dest_pos, color_idx_to_rgb(COL_YELLOW));
			}
			else if(  route_result==haltestelle_t::ROUTE_OVERCROWDED  ) {
				// overcrowded routes cause unhappiness to be logged

				if(  start_halt.is_bound()  ) {
					start_halt->add_pax_unhappy(pax_left_to_do);
				}
				else {
					// all routes to goal are overcrowded -> register at first stop (closest)
					for(halthandle_t const s : start_halts) {
						s->add_pax_unhappy(pax_left_to_do);
						merke_passagier_ziel(dest_pos, color_idx_to_rgb(COL_ORANGE));
						break;
					}
				}

				// destination logged
				merke_passagier_ziel(dest_pos, color_idx_to_rgb(COL_ORANGE));
			}
			else if (  route_result == haltestelle_t::NO_ROUTE  ) {
				// since there is no route from any start halt -> register no route at first halts (closest)
				for(halthandle_t const s : start_halts) {
					s->add_pax_no_route(pax_left_to_do);
					break;
				}
				merke_passagier_ziel(dest_pos, color_idx_to_rgb(COL_DARK_ORANGE));
#ifdef DESTINATION_CITYCARS
				//citycars with destination
				generate_private_cars( origin_pos, dest_pos );
#endif
			}

			// return passenger traffic
			if(  will_return != no_return  ) {
				// compute return amount
				uint32 pax_return = pax_left_to_do;

				// apply return modifiers
				if(  will_return != city_return  &&  wtyp == goods_manager_t::mail  ) {
					// attractions and factories return more mail than they receive
					pax_return *= MAIL_RETURN_MULTIPLIER_PRODUCERS;
				}

				// log potential return passengers at destination city
				dest_city->city_history_year[0][history_type + HIST_OFFSET_GENERATED] += pax_return;
				dest_city->city_history_month[0][history_type + HIST_OFFSET_GENERATED] += pax_return;

				// factories generate return traffic
				if (  factory_entry  ) {
					factory_entry->factory->book_stat(pax_return, (ispass ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED));
				}

				// route type specific logic
				if(  route_result == haltestelle_t::ROUTE_OK  ) {
					// send return packet
					halthandle_t return_halt = pax.get_ziel();
					if(  !return_halt->is_overcrowded(wtyp->get_index())  ) {
						// stop can receive passengers

						// register departed pax/mail at factory
						if (factory_entry) {
							factory_entry->factory->book_stat(pax_return, ispass ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED);
						}

						// setup ware packet
						return_pax.menge = pax_return;
						return_pax.set_zielpos(origin_pos);
						return_halt->starte_mit_route(return_pax);

						// log departed at stop
						return_halt->add_pax_happy(pax_return);

						// log departed at destination city
						dest_city->city_history_year[0][history_type + HIST_OFFSET_TRANSPORTED] += pax_return;
						dest_city->city_history_month[0][history_type + HIST_OFFSET_TRANSPORTED] += pax_return;
					}
					else {
						// stop is crowded
						return_halt->add_pax_unhappy(pax_return);
					}

				}
				else if(  route_result == haltestelle_t::ROUTE_WALK  ) {
					// walking can produce return flow as a result of commuters to industry, monuments or stupidly big stops

					// register departed pax/mail at factory
					if (  factory_entry  ) {
						factory_entry->factory->book_stat(pax_return, ispass ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED);
					}

					// log walked at stop (source and destination stops are the same)
					start_halt->add_pax_walked(pax_return);

					// log people who walk or deliver by hand
					dest_city->city_history_year[0][history_type + HIST_OFFSET_WALKED] += pax_return;
					dest_city->city_history_month[0][history_type + HIST_OFFSET_WALKED] += pax_return;
				}
				else if(  route_result == haltestelle_t::ROUTE_OVERCROWDED  ) {
					// overcrowded routes cause unhappiness to be logged

					if (pax.get_ziel().is_bound()) {
						pax.get_ziel()->add_pax_unhappy(pax_return);
					}
					else {
						// the unhappy passengers will be added to the first stops near destination (might be none)
						const planquadrat_t *const dest_plan = welt->access(dest_pos);
						const halthandle_t *const dest_halt_list = dest_plan->get_haltlist();
						for (uint h = 0; h < dest_plan->get_haltlist_count(); h++) {
							halthandle_t halt = dest_halt_list[h];
							if (halt->is_enabled(wtyp)) {
								halt->add_pax_unhappy(pax_return);
								break;
							}
						}
					}
				}
				else if (route_result == haltestelle_t::NO_ROUTE) {
					// passengers who cannot find a route will be added to the first stops near destination (might be none)
					const planquadrat_t *const dest_plan = welt->access(dest_pos);
					const halthandle_t *const dest_halt_list = dest_plan->get_haltlist();
					for (uint h = 0; h < dest_plan->get_haltlist_count(); h++) {
						halthandle_t halt = dest_halt_list[h];
						if (halt->is_enabled(wtyp)) {
							halt->add_pax_no_route(pax_return);
							break;
						}
					}
				}
			}
			INT_CHECK( "simcity 1579" );
		}
	}
	else {
		// assume no free stop to start at all
		bool is_there_any_stop = false;

		// the unhappy passengers will be added to the first stop if any
		for(  uint h=0;  h<plan->get_haltlist_count(); h++  ) {
			halthandle_t halt = plan->get_haltlist()[h];
			if(  halt->is_enabled(wtyp)  ) {
				halt->add_pax_unhappy(num_pax);
				is_there_any_stop = true; // only overcrowded
				break;
			}
		}

		// all passengers without suitable start:
		// fake one ride to get a proper display of destinations (although there may be more) ...
		pax_return_type will_return;
		factory_entry_t *factory_entry = NULL;
		stadt_t *dest_city = NULL;
		const koord ziel = find_destination(target_factories, city_history_month[0][history_type + HIST_OFFSET_GENERATED], &will_return, factory_entry, dest_city);
		if(  factory_entry  ) {
			// consider at most 1 packet's amount as factory-going
			sint32 amount = min(PACKET_SIZE, num_pax);
			if(  welt->get_settings().get_factory_enforce_demand()  ) {
				// ensure no more than remaining amount
				amount = min( amount, factory_entry->remaining );
				factory_entry->remaining -= amount;
				target_factories.total_remaining -= amount;
			}
			target_factories.total_generated += amount;
			factory_entry->factory->book_stat( amount, ( ispass ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED ) );
		}


		// log reverse flow at destination city for accurate tally
		if(  will_return != no_return  ) {
			uint32 pax_return = num_pax;

			// apply return modifiers
			if(  will_return != city_return  &&  wtyp == goods_manager_t::mail  ) {
				// attractions and factories return more mail than they receive
				pax_return *= MAIL_RETURN_MULTIPLIER_PRODUCERS;
			}

			// log potential return passengers at destination city
			dest_city->city_history_year[0][history_type + HIST_OFFSET_GENERATED] += pax_return;
			dest_city->city_history_month[0][history_type + HIST_OFFSET_GENERATED] += pax_return;

			// factories generate return traffic
			if (  factory_entry  ) {
				factory_entry->factory->book_stat(pax_return, (ispass ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED));
			}

			// passengers with no route will be added to the first stops near destination (might be none)
			const planquadrat_t *const dest_plan = welt->access(ziel);
			const halthandle_t *const dest_halt_list = dest_plan->get_haltlist();
			for (uint h = 0; h < dest_plan->get_haltlist_count(); h++) {
				halthandle_t halt = dest_halt_list[h];
				if (  halt->is_enabled(wtyp)  ) {
					if(  is_there_any_stop  ) {
						// "just" overcrowded
						halt->add_pax_unhappy(pax_return);
					}
					else {
						// no stops at all
						halt->add_pax_no_route(pax_return);
					}
					break;
				}
			}
		}

#ifdef DESTINATION_CITYCARS
		//citycars with destination
		generate_private_cars( origin_pos, ziel );
#endif
		merke_passagier_ziel(ziel, color_idx_to_rgb(COL_ORANGE));
		// we show unhappy instead no route for destination stop
	}
}


koord stadt_t::get_zufallspunkt() const
{
	if(!buildings.empty()) {
		gebaeude_t* const gb = pick_any_weighted(buildings);
		koord k = gb->get_pos().get_2d();
		if(!welt->is_within_limits(k)) {
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
		weight_by_distance( city->get_einwohner()+1, shortest_distance( get_center(), city->get_center() ) )
	);
}


void stadt_t::recalc_target_cities()
{
	target_cities.clear();
	for(stadt_t* const c : welt->get_cities()) {
		add_target_city(c);
	}
}


void stadt_t::add_target_attraction(gebaeude_t *const attraction)
{
	assert( attraction != NULL );
	target_attractions.append(
		attraction,
		weight_by_distance( attraction->get_passagier_level() << 4, shortest_distance( get_center(), attraction->get_pos().get_2d() ) )
	);
}


void stadt_t::recalc_target_attractions()
{
	target_attractions.clear();
	for(gebaeude_t* const a : welt->get_attractions()) {
		add_target_attraction(a);
	}
}


/* this function generates a random target for passenger/mail
 * changing this strongly affects selection of targets and thus game strategy
 */
koord stadt_t::find_destination(factory_set_t &target_factories, const sint64 generated, pax_return_type* will_return, factory_entry_t* &factory_entry, stadt_t* &dest_city)
{
	// generate factory traffic when required
	if(  target_factories.total_remaining>0  &&  (sint64)target_factories.generation_ratio>((sint64)(target_factories.total_generated*100)<<RATIO_BITS)/(generated+1)  ) {
		factory_entry_t *const entry = target_factories.get_random_entry();
		assert( entry );
		*will_return = factory_return; // worker will return
		factory_entry = entry;
		dest_city = this; // factory is part of city
		return entry->factory->get_pos().get_2d();
	}

	// chance to generate tourist traffic
	const sint16 rand = simrand(100 - (target_factories.generation_ratio >> RATIO_BITS));
	if(  rand < welt->get_settings().get_tourist_percentage()  &&  target_attractions.get_sum_weight() > 0  ) {
		*will_return = tourist_return; // tourists will return
		gebaeude_t *const &attraction = pick_any_weighted(target_attractions);
		dest_city = attraction->get_stadt(); // unsure if return value always valid
		if (dest_city == NULL) {
			// if destination city was invalid assume this city is the source
			dest_city = this;
		}
		return attraction->get_pos().get_2d();
	}

	// generate general traffic between buildings

	// since the locality is already taken into account for us, we just use the random weight
	stadt_t *const selected_city = pick_any_weighted( target_cities );
	// no return trip if the destination is inside the same city
	*will_return = selected_city == this ? no_return : city_return;
	dest_city = selected_city;
	// find a random spot inside the selected city
	return selected_city->get_zufallspunkt();

}


void stadt_t::merke_passagier_ziel(koord k, PIXVAL color)
{
	const koord p = koord(
		((k.x * PAX_DESTINATIONS_SIZE) / welt->get_size().x) & (PAX_DESTINATIONS_SIZE-1),
		((k.y * PAX_DESTINATIONS_SIZE) / welt->get_size().y) & (PAX_DESTINATIONS_SIZE-1)
	);
	pax_destinations_new_change ++;
	pax_destinations_new.set(p, color);
}


/**
 * building_place_with_road_finder:
 * Search a free place for a building using function suche_platz() (search place).
 * added: Minimum distance between monuments
 */
class building_place_with_road_finder: public building_placefinder_t
{
	public:
		/// if false, this will the check 'do not build next other to special buildings'
		bool big_city;

		building_place_with_road_finder(karte_t* welt, sint16 radius, bool big) : building_placefinder_t(welt, radius), big_city(big) {}

		// get distance to next special building
		int find_dist_next_special(koord pos) const
		{
			const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_attractions();
			int dist = welt->get_size().x * welt->get_size().y;
			for(gebaeude_t* const i : attractions  ) {
				int const d = koord_distance(i->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			for(stadt_t * const city : welt->get_cities() ) {
				int const d = koord_distance(city->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			return dist;
		}

		bool is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const OVERRIDE
		{
			if(  !building_placefinder_t::is_area_ok(pos, w, h, cl)  ) {
				return false;
			}
			bool next_to_road = false;
			// not direct next to factories or townhalls
			for (sint16 x = -1; x < w; x++) {
				for (sint16 y = -1;  y < h; y++) {
					grund_t *gr = welt->lookup_kartenboden(pos + koord(x,y));
					if (!gr) {
						return false;
					}
					if (  0 <= x  &&  x < w-1  &&  0 <= y  &&  y < h-1  ) {
						// inside: nothing on top like elevated monorails?
						if(  gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1)  )!=NULL) {
							// something on top (monorail or powerlines)
							return false;
						}

					}
					else {
						// border: not direct next to special buildings
						if (big_city) {
							if(  gebaeude_t *gb=gr->find<gebaeude_t>()  ) {
								const building_desc_t::btype utyp = gb->get_tile()->get_desc()->get_type();
								if(  building_desc_t::attraction_city <= utyp  &&  utyp <= building_desc_t::headquarters) {
									return false;
								}
							}
						}
						// but near a road if possible
						if (!next_to_road) {
							next_to_road = gr->hat_weg(road_wt);
						}
					}
				}
			}
			if (!next_to_road) {
				return false;
			}

			// try to built a little away from previous ones
			if (big_city  &&  find_dist_next_special(pos) < w + h + welt->get_settings().get_special_building_distance()  ) {
				return false;
			}
			return true;
		}
};


void stadt_t::check_bau_spezial(bool new_town)
{
	// tourist attraction buildings
	const building_desc_t* desc = hausbauer_t::get_special( bev, building_desc_t::attraction_city, welt->get_timeline_year_month(), new_town, welt->get_climate(pos) );
	if (desc != NULL) {
		if (simrand(100) < (uint)desc->get_distribution_weight()) {

			bool big_city = buildings.get_count() >= 10;
			bool is_rotate = desc->get_all_layouts() > 1;
			sint16 radius = koord_distance( get_rechtsunten(), get_linksoben() )/2 + 10;
			// find place
			koord best_pos = building_place_with_road_finder(welt, radius, big_city).find_place(pos, desc->get_x(), desc->get_y(), desc->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				int rotate = 0;
				if (desc->get_all_layouts() > 1) {
					rotate = (simrand(20) & 2) + is_rotate;
				}
				hausbauer_t::build( owner, best_pos, rotate, desc );
				// tell the player, if not during initialization
				if (!new_town) {
					cbuffer_t buf;
					buf.printf( translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), get_name(), make_single_line_string(translator::translate(desc->get_name()), 2), get_einwohner());
					welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, desc->get_tile(0)->get_background(0, 0, 0));
				}
			}
		}
	}

	if ((bev & 511) == 0) {
		// Build a monument
		desc = hausbauer_t::get_random_monument(welt->get_timeline_year_month());
		if (desc) {
			koord total_size = koord(2 + desc->get_x(), 2 + desc->get_y());
			sint16 radius = koord_distance( get_rechtsunten(), get_linksoben() )/2 + 10;
			koord best_pos(monument_placefinder_t(welt, radius).find_place(pos, total_size.x, total_size.y, desc->get_allowed_climate_bits()));

			if (best_pos != koord::invalid) {
				// check if borders around the monument are inside the map limits
				const bool pre_ok = welt->is_within_limits( koord(best_pos) - koord(1, 1) )  &&  \
					welt->is_within_limits( koord(best_pos) + total_size + koord(1, 1) );
				if (!pre_ok){
					return;
				}

				bool ok=false;

				// We build the monument only if there is already at least one road
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
							build_road(best_pos + koord(0, i), NULL, true);
						}
						gr = welt->lookup_kartenboden(best_pos + koord(total_size.x - 1, i));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							build_road(best_pos + koord(total_size.x - 1, i), NULL, true);
						}
					}
					for (int i = 0; i < total_size.x; i++) {
						// only build in same height and not on slopes...
						const grund_t *gr = welt->lookup_kartenboden(best_pos + koord(i, 0));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							build_road(best_pos + koord(i, 0), NULL, true);
						}
						gr = welt->lookup_kartenboden(best_pos + koord(i, total_size.y - 1));
						if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
							build_road(best_pos + koord(i, total_size.y - 1), NULL, true);
						}
					}
					// and then build it
					const gebaeude_t* gb = hausbauer_t::build(owner, best_pos + koord(1, 1), 0, desc);
					hausbauer_t::monument_erected(desc);
					add_gebaeude_to_stadt(gb);
					// tell the player, if not during initialization
					if (!new_town) {
						cbuffer_t buf;
						buf.printf( translator::translate("With a big festival\n%s built\na new monument.\n%i citicens rejoiced."), get_name(), get_einwohner() );
						welt->get_message()->add_message(buf, best_pos + koord(1, 1), message_t::city, CITY_KI, desc->get_tile(0)->get_background(0, 0, 0));
					}
				}
			}
		}
	}
}


void stadt_t::check_bau_townhall(bool new_town)
{
	const building_desc_t* desc = hausbauer_t::get_special( has_townhall ? bev : 0, building_desc_t::townhall, welt->get_timeline_year_month(), (bev == 0) || !has_townhall, welt->get_climate(pos) );
	if(desc != NULL) {
		grund_t* gr = welt->lookup_kartenboden(pos);
		gebaeude_t* gb = obj_cast<gebaeude_t>(gr->first_obj());
		bool neugruendung = !has_townhall ||  !gb || !gb->is_townhall();
		bool umziehen = !neugruendung;
		koord alte_str(koord::invalid);
		koord best_pos(pos);
		koord k;
		int old_layout(0);

		DBG_MESSAGE("check_bau_townhall()", "bev=%d, new=%d name=%s", bev, neugruendung, name.c_str());

		if(  umziehen  ) {

			const building_desc_t* desc_old = gb->get_tile()->get_desc();
			if (desc_old->get_level() == desc->get_level()) {
				DBG_MESSAGE("check_bau_townhall()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}
			old_layout = gb->get_tile()->get_layout();
			const sint8 old_z = gb->get_pos().z;
			koord pos_alt = best_pos = gr->get_pos().get_2d() - gb->get_tile()->get_offset();
			// guess layout for broken townhall's
			if(desc_old->get_x() != desc_old->get_y()  &&  desc_old->get_all_layouts()==1) {
				// test all layouts
				koord corner_offset(desc_old->get_x()-1, desc_old->get_y()-1);
				for(uint8 test_layout = 0; test_layout<4; test_layout++) {
					// is there a part of our townhall in this corner
					grund_t *gr0 = welt->lookup_kartenboden(pos + corner_offset);
					gebaeude_t const* const gb0 = gr0 ? obj_cast<gebaeude_t>(gr0->first_obj()) : 0;
					if (gb0  &&  gb0->is_townhall()  &&  gb0->get_tile()->get_desc()==desc_old  &&  gb0->get_stadt()==this) {
						old_layout = test_layout;
						pos_alt = best_pos = gr->get_pos().get_2d() + koord(test_layout%3!=0 ? corner_offset.x : 0, test_layout&2 ? corner_offset.y : 0);
						break;
					}
					corner_offset = koord(-corner_offset.y, corner_offset.x);
				}
			}
			koord groesse_alt = desc_old->get_size(old_layout);

			// do we need to move
			if(  old_layout<=desc->get_all_layouts()  &&  desc->get_x(old_layout) <= groesse_alt.x  &&  desc->get_y(old_layout) <= groesse_alt.y  ) {
				// no, the size is ok
				// still need to check whether the existing townhall is not broken in some way
				umziehen = false;
				for(k.y = 0; k.y < groesse_alt.y; k.y ++) {
					for(k.x = 0; k.x < groesse_alt.x; k.x ++) {
						// for buildings with holes the hole could be on a different height ->gr==NULL
						bool ok = false;
						if (grund_t *gr = welt->lookup_kartenboden(k + pos)) {
							if(gebaeude_t *gb_part = gr->find<gebaeude_t>()) {
								// there may be buildings with holes, so we only remove our building!
								if(gb_part->get_tile()  ==  desc_old->get_tile(old_layout, k.x, k.y)) {
									ok = true;
								}
							}
						}
						umziehen |= !ok;
					}
				}
				if (!umziehen) {
					// correct position if new townhall is smaller than old
					if(  old_layout == 0  ) {
						best_pos.y -= desc->get_y(old_layout) - groesse_alt.y;
					}
					else if (old_layout == 1) {
						best_pos.x -= desc->get_x(old_layout) - groesse_alt.x;
					}
				}
			}
			if (umziehen) {
				// we need to built a new road, thus we will use the old as a starting point (if found)
				if (welt->lookup_kartenboden(townhall_road)  &&  welt->lookup_kartenboden(townhall_road)->hat_weg(road_wt)) {
					alte_str = townhall_road;
				}
				else {
					koord k = pos + (old_layout==0 ? koord(0, desc_old->get_y()) : koord(desc_old->get_x(),0) );
					if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
						alte_str = k;
					}
					else {
						k = pos - (old_layout==0 ? koord(0, desc_old->get_y()) : koord(desc_old->get_x(),0) );
						if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
							alte_str = k;
						}
					}
				}
			}

			// remove old townhall
			if(  gb  ) {
				DBG_MESSAGE("stadt_t::check_bau_townhall()", "delete townhall at (%s)", pos_alt.get_str());
				hausbauer_t::remove(NULL, gb);
			}

			// replace old space by normal houses level 0 (must be 1x1!)
			if(  umziehen  ) {
				for (k.x = 0; k.x < groesse_alt.x; k.x++) {
					for (k.y = 0; k.y < groesse_alt.y; k.y++) {
						// we iterate over all tiles, since the townhalls are allowed sizes bigger than 1x1
						const koord pos = pos_alt + k;
						gr = welt->lookup_kartenboden(pos);
						if (gr  &&  gr->ist_natur() &&  gr->kann_alle_obj_entfernen(NULL) == NULL  &&
							  (  gr->get_grund_hang() == slope_t::flat  ||  welt->lookup(koord3d(k, welt->max_hgt(k))) == NULL  ) ) {
							DBG_MESSAGE("stadt_t::check_bau_townhall()", "fill empty spot at (%s)", pos.get_str());
							build_city_building(pos);
						}
					}
				}
			}
			else {
				// make tiles flat, hausbauer_t::remove could have set some natural slopes
				for(  k.x = 0;  k.x < desc->get_x(old_layout);  k.x++  ) {
					for(  k.y = 0;  k.y < desc->get_y(old_layout);  k.y++  ) {
						gr = welt->lookup_kartenboden(best_pos + k);
						if(  gr  &&  gr->ist_natur()  ) {
							// make flat and use right height
							gr->set_grund_hang(slope_t::flat);
							gr->set_pos( koord3d( best_pos + k, old_z ) );
						}
					}
				}
			}
		}

		// Now built the new townhall (remember old orientation)
		int layout = umziehen || neugruendung ? simrand(desc->get_all_layouts()) : old_layout % desc->get_all_layouts();
		// on which side should we place the road?
		uint8 dir;
		// offset of building within searched place, start and end of road
		koord offset(0,0), road0(0,0),road1(0,0);
		dir = ribi_t::layout_to_ribi[layout & 3];
		switch(dir) {
			case ribi_t::east:
				road0.x = desc->get_x(layout);
				road1.x = desc->get_x(layout);
				road1.y = desc->get_y(layout)-1;
				break;
			case ribi_t::north:
				road1.x = desc->get_x(layout)-1;
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
				road1.y = desc->get_y(layout)-1;
				if (neugruendung || umziehen) {
					offset.x = 1;
				}
				else {
					// offset already included in in position of old townhall
					road0.x=-1;
					road1.x=-1;
				}
				break;
			case ribi_t::south:
			default:
				road0.y = desc->get_y(layout);
				road1.x = desc->get_x(layout)-1;
				road1.y = desc->get_y(layout);
		}
		if (neugruendung || umziehen) {
			best_pos = townhall_placefinder_t(welt, dir).find_place(pos, desc->get_x(layout) + (dir & ribi_t::eastwest ? 1 : 0), desc->get_y(layout) + (dir & ribi_t::northsouth ? 1 : 0), desc->get_allowed_climate_bits());
		}
		// check, if the was something found
		if(best_pos==koord::invalid) {
			dbg->error( "stadt_t::check_bau_townhall", "no better position found!" );
			return;
		}
		gebaeude_t const* const new_gb = hausbauer_t::build(owner, best_pos + offset, layout, desc);
		DBG_MESSAGE("new townhall", "use layout=%i", layout);
		add_gebaeude_to_stadt(new_gb);
		// sets has_townhall to true
		DBG_MESSAGE("stadt_t::check_bau_townhall()", "add townhall (bev=%i, ptr=%p)", buildings.get_sum_weight(),welt->lookup_kartenboden(best_pos)->first_obj());

		// if not during initialization
		if (!new_town) {
			cbuffer_t buf;
			buf.printf(translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), name.c_str(), get_einwohner());
			welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, desc->get_tile(layout, 0, 0)->get_background(0, 0, 0));
		}
		else {
			welt->lookup_kartenboden(best_pos + offset)->set_text( name );
		}

		if (neugruendung || umziehen) {
			// build the road in front of the townhall
			if (road0!=road1) {
				way_builder_t bauigel(NULL);
				bauigel.init_builder(way_builder_t::strasse, welt->get_city_road(), NULL, NULL);
				bauigel.set_build_sidewalk(true);
				bauigel.calc_straight_route(welt->lookup_kartenboden(best_pos + road0)->get_pos(), welt->lookup_kartenboden(best_pos + road1)->get_pos());
				bauigel.build();
			}
			else {
				build_road(best_pos + road0, NULL, true);
			}
			townhall_road = best_pos + road0;
		}
		if (umziehen  &&  alte_str != koord::invalid) {
			// Strasse vom ehemaligen Rathaus zum neuen verlegen.
			way_builder_t bauer(NULL);
			bauer.init_builder(way_builder_t::strasse | way_builder_t::terraform_flag, welt->get_city_road());
			bauer.calc_route(welt->lookup_kartenboden(alte_str)->get_pos(), welt->lookup_kartenboden(townhall_road)->get_pos());
			bauer.build();
		}
		else if (neugruendung) {
			lo = best_pos+offset - koord(2, 2);
			ur = best_pos+offset + koord(desc->get_x(layout), desc->get_y(layout)) + koord(2, 2);
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


/**
 * eventually adds a new industry
 * so with growing number of inhabitants the industry grows
 */
void stadt_t::check_bau_factory(bool new_town)
{
	uint32 const inc = welt->get_settings().get_industry_increase_every();
	if(  !new_town && inc > 0  &&  bev % inc == 0  ) {
		uint32 const div = bev / inc;
		for( uint8 i = 0; i < 8; i++  ) {
			if(  div == (1u<<i)  ) {
				DBG_MESSAGE("stadt_t::check_bau_factory", "adding new industry at %i inhabitants.", get_einwohner());
				factory_builder_t::increase_industry_density( true );
			}
		}
	}
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

			building_desc_t::btype t = building_desc_t::unknown;
			if (const grund_t* gr = welt->lookup_kartenboden(k)) {
				if (gebaeude_t const* const gb = obj_cast<gebaeude_t>(gr->first_obj())) {
					t = gb->get_tile()->get_desc()->get_type();
				}
			}

			int i = -1;
			switch(t) {
				case building_desc_t::city_res: i = 0; break;
				case building_desc_t::city_com: i = 1; break;
				case building_desc_t::city_ind: i = 2; break;
				default: ;
			}
			if (i >= 0) {
				ind_score += ind_neighbour_score[i];
				com_score += com_neighbour_score[i];
				res_score += res_neighbour_score[i];
			}
		}
	}
}


// return the eight neighbors:
// orthogonal before diagonal
static koord const neighbors[] = {
	koord( 0,  1),
	koord( 1,  0),
	koord( 0, -1),
	koord(-1,  0),
	// now the diagonals
	koord( 1,  1),
	koord( 1, -1),
	koord(-1,  1),
	koord(-1, -1)
};

static koord const area3x3[] = {
	koord( 0, 1),  //  1x2
	koord( 1, 0),  //  2x1
	koord( 1, 1),  //  2x2
	koord( 2, 0),  //  3x1
	koord( 2, 1),  //  3x2
	koord( 0, 2),  //  1x3
	koord( 1, 2),  //  2x3
	koord( 2, 2)   //  3x3
};

// updates one surrounding road with current city road
bool update_city_street(koord pos)
{
// !!! We should take the bulding size into consideration, missing!!!
	const way_desc_t* cr = world()->get_city_road();
	for(  int i=0;  i<8;  i++  ) {
		if(  grund_t *gr = world()->lookup_kartenboden(pos+neighbors[i])  ) {
			if(  weg_t* const weg = gr->get_weg(road_wt)  ) {
				// Check if any changes are needed.
				if(  !weg->hat_gehweg()  ||  weg->get_desc() != cr  ) {
					player_t *sp = weg->get_owner();
					if(  sp  ){
						player_t::add_maintenance(sp, -weg->get_desc()->get_maintenance(), road_wt);
						weg->set_owner(NULL); // make public
					}
					weg->set_gehweg(true);
					weg->set_desc(cr);
					gr->calc_image();
					minimap_t::get_instance()->calc_map_pixel(pos+neighbors[i]);
					return true; // update only one road per renovation
				}
			}
		}
	}
	return false;
}


// return layout
#define CHECK_NEIGHBOUR (128)
static int const building_layout[] = { CHECK_NEIGHBOUR | 0, 0, 1, 4, 2, 0, 5, CHECK_NEIGHBOUR | 1, 3, 7, 1, CHECK_NEIGHBOUR | 0, 6, CHECK_NEIGHBOUR | 3, CHECK_NEIGHBOUR | 2, CHECK_NEIGHBOUR | 0 };


// calculates the "best" oreintation of a citybuilding
int stadt_t::orient_city_building(const koord k, const building_desc_t *h, koord maxarea )
{
	/*******************************************************
	* these are the layout possible for city buildings
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
	if(  h == NULL  ) {
		return -1;
	}

	// old, highly sophisticated routines for 1x1 buildings
	if(  h->get_x()*h->get_y()==1  ) {
		if(  grund_t *gr = welt->lookup_kartenboden(k)  ) {
			int rotation = 0;
			int max_layout = h->get_all_layouts()-1;
			if(  max_layout  ) {
				// check for pavement
				int streetdir = 0;
				for(  int i = 0;  i < 4;  i++  ) {
					// Neighbors goes through these in 'preferred' order, orthogonal first
					gr = welt->lookup_kartenboden(k + neighbors[i]);
					if(  gr  &&  gr->get_weg_hang() == gr->get_grund_hang()  &&  gr->hat_weg(road_wt)  ){
						streetdir += (1 << i);
					}
				}
				// not completely unique layout, see if any of the neighbouring building gives a hint
				rotation = building_layout[streetdir] & ~CHECK_NEIGHBOUR;
				// for four rotation stop here ...
				if(  max_layout<7  ) {
					return rotation & max_layout;
				}
				// now this is an eight roation building, so we must put in more effort
				bool unique_orientation = !(building_layout[streetdir] & CHECK_NEIGHBOUR);
				if(  !unique_orientation  ) {
					// no unique answer, check nearby buildings (more likely to fail)
					int gb_dir = 0;
					for(  int i = 0;  i < 4;  i++  ) {
						// look for adjacent buildings
						gr = welt->lookup_kartenboden(k + neighbors[i]);
						if(  gr  &&  gr->get_typ()==grund_t::fundament  ){
							if(  gr->find<gebaeude_t>()  ) {
								gb_dir |= (1<<i);
							}
						}
					}
					// lets hope this gives an unique answer
					static uint8 gb_dir_to_layout[16] = { CHECK_NEIGHBOUR, 1, 0, 6, 1, 1, 7, CHECK_NEIGHBOUR, 0, 5, 0, CHECK_NEIGHBOUR, 4, CHECK_NEIGHBOUR, CHECK_NEIGHBOUR, CHECK_NEIGHBOUR };
					if(  gb_dir_to_layout[gb_dir] == CHECK_NEIGHBOUR  ) {
						return rotation;
					}
					// ok our answer is unique, now we just check for left and right via street nearby
					rotation = gb_dir_to_layout[gb_dir];
					if(  rotation<2  ) {
						// check on which side is the road
						if(  streetdir & 0x0C  ) {
							return rotation + 2;
						}
					}
				}
			}
			return rotation;
		}
		return -1;
	}

	// if we arrive here, we have a multitile building
	if(  grund_t *gr = welt->lookup_kartenboden(k)  ) {
		int rotation = -1;
		int max_layout = h->get_all_layouts()-1;
		if(  max_layout  ) {

			// we counting the streetiles, but asymmetric buildings will have an uneven number; we init with negative width
			int streetdir[4];
			for(  int i = 0;  i < 4;  i++  ) {
				streetdir[i] = -h->get_x(i&1);
			}
			int roads_found = 0;

			// now just counting street tiles north/south of the house ...
			sint16 extra_offset = h->get_y(0)-1;
			for(  int offset = -1;  offset <= h->get_x(0);  offset++  ) {
				gr = welt->lookup_kartenboden(k + koord(offset,1+extra_offset) );
				if(  gr  &&  gr->hat_weg(road_wt)  ){
					streetdir[0] ++;
					roads_found ++;
				}
				gr = welt->lookup_kartenboden(k + koord(offset,-1) );
				if(  gr  &&  gr->hat_weg(road_wt)  ){
					streetdir[2] ++;
					roads_found ++;
				}
			}
			// ... and east/west
			for(  int offset = -1;  offset <= h->get_x(1);  offset++  ) {
				// Neighbors goes through these in 'preferred' order, orthogonal first
				gr = welt->lookup_kartenboden(k + koord(1+extra_offset,offset) );
				if(  gr  &&  gr->hat_weg(road_wt)  ){
					streetdir[1] ++;
					roads_found ++;
				}
				// Neighbors goes through these in 'preferred' order, orthogonal first
				gr = welt->lookup_kartenboden(k + koord(-1,offset) );
				if(  gr  &&  gr->hat_weg(road_wt)  ){
					streetdir[3] ++;
					roads_found ++;
				}
			}

			// first, make sure we found some roads
			if(  roads_found > 0  ) {

				// now find the two sides with most streets around
				int largest_dir_count = -9999, largest_2nd_count = -9999;
				int largest_dir = -1, largest_2nd_dir = -1;
				for(  int i=0;  i<4;  i++  ) {
					if(  streetdir[i] > largest_dir_count  ) {
						largest_dir_count = streetdir[i];
						largest_dir = i;
					}
					else if(  streetdir[i] > largest_2nd_count  ) {
						largest_2nd_dir = i;
					}
				}

				// so we have two adjacent corners with the most roads
				if(  max_layout > 3  &&  ((largest_2nd_dir-largest_dir)==1  ||  (largest_2nd_dir-largest_dir)==3)  ) {
					// corner cases: only roads on two sides
					if(  streetdir[0]<0  &&  streetdir[1]<0  ) {
						rotation = 6;
					}
					else if(  streetdir[1]<0  &&  streetdir[2]<0  ) {
						rotation = 7;
					}
					else if(  streetdir[2]<0  &&  streetdir[3]<0  ) {
						rotation = 4;
					}
					else if(  streetdir[3]<0  &&  streetdir[0]<0  ) {
						rotation = 5;
					}
					// some valid found?
					if(  rotation >=0  &&  h->get_x(rotation) <= maxarea.x  &&  h->get_y(rotation) <= maxarea.y  ) {
						return rotation;
					}
				}

				// now we have to check right of it
				if(  streetdir[(largest_dir+1)&3] == largest_dir_count  ) {
					// but since we take the first, if the next two corner have the same street count, better rotate one further
					if(  streetdir[(largest_dir+2)&3] == largest_dir_count  ) {
						// both next corners same count
						rotation = largest_dir+1;
					}
					else {
						rotation = largest_dir;
					}
					rotation &= max_layout;
				}
				// and left of it
				else if(  streetdir[(largest_dir+3)&3] == largest_dir_count  ) {
					// but since we take the first, if the next two corner have the same street count, better rotate one further
					if(  streetdir[(largest_dir+2)&3] == largest_dir_count  ) {
						// both next corners same count
						rotation = largest_dir+3;
					}
					else {
						rotation = largest_dir;
					}
					rotation &= max_layout;
				}
				// this is the really only longest one
				else {
					rotation = largest_dir & max_layout;
				}

				// some valid found?
				if(  rotation >= 0  &&  h->get_x(rotation) <= maxarea.x  &&  h->get_y(rotation) <= maxarea.y  ) {
					return rotation;
				}

				return -1;
			}

			// no roads or not fitting
			if(  roads_found == 0  ) {
				bool even_ok = (maxarea.x <= h->get_x(0)  &&  maxarea.y <= h->get_y(0));
				bool odd_ok = (maxarea.x <= h->get_x(1)  &&  maxarea.y <= h->get_y(1));

				if(  even_ok  &&  odd_ok  ) {
					// no roads at all and both rotations fit => random oreintation (but no corner)
					return simrand(4) % max_layout;
				}
				else if(  even_ok  ) {
					return (simrand(2)*2) % max_layout;
				}
				else if(  odd_ok  ) {
					return (simrand(2)*2+1) % max_layout;
				}
				// nothing fits, should not happen!
				assert(1);
			}

			// we have a preferred orientation, but it does not fit  => gave up
			return -1;
		}

		// we landed here but maxlayout == 1, so we have only to test for fit
		if(  maxarea.x <= h->get_x(0)  &&  maxarea.y <= h->get_y(0)  ) {
			return 0;
		}

	}

	return -1;
}


void stadt_t::build_city_building(const koord k)
{
	grund_t* gr = welt->lookup_kartenboden(k);

	// Not building on ways (this was actually tested before be the cityrules), but you can construct manually
	if(  !gr->ist_natur() ) {
		return;
	}
	// test ownership of all objects that can block construction
	for(  uint8 i = 0;  i < gr->obj_count();  i++  ) {
		obj_t *const obj = gr->obj_bei(i);
		if(  obj->is_deletable(NULL) != NULL  &&  obj->get_typ() != obj_t::pillar  ) {
			return;
		}
	}
	// Refuse to build on a slope, when there is a ground right on top of it (=> the house would sit on the bridge then!)
	if(  gr->get_grund_hang() != slope_t::flat  &&  welt->lookup(koord3d(k, welt->max_hgt(k))) != NULL  ) {
		return;
	}

	// Divide unemployed by 4, because it counts towards commercial and industrial,
	// and both of those count 'double' for population relative to residential.
	int employment_wanted  = get_unemployed() / 4;
	int housing_wanted = get_homeless();

	int industrial_suitability, commercial_suitability, residential_suitability;
	bewerte_res_com_ind(k, industrial_suitability, commercial_suitability, residential_suitability );

	const int sum_industrial   = industrial_suitability  + employment_wanted;
	const int sum_commercial = commercial_suitability  + employment_wanted;
	const int sum_residential   = residential_suitability + housing_wanted;

	// does the timeline allow this building?
	const uint16 current_month = welt->get_timeline_year_month();
	const climate cl = welt->get_climate(k);


	// Run through orthogonal neighbors (only) looking for which cluster to build
	// This is a bitmap -- up to 32 clustering types are allowed.
	uint32 neighbor_building_clusters = 0;
	uint8 area_level=0; // used to calculate the maximum size
	sint8 zpos = gr->get_pos().z + slope_t::max_diff(gr->get_grund_hang());
	for(  int i = 0;  i < 4;  i++  ) {
		grund_t* gr = welt->lookup_kartenboden(k + neighbors[i]);
		if(  gr  &&  gr->get_typ() == grund_t::fundament  &&  gr->obj_bei(0)->get_typ() == obj_t::gebaeude  ) {
			// We have a building as a neighbor...
			if(  gebaeude_t const* const gb = obj_cast<gebaeude_t>(gr->first_obj())  ) {
				// We really have a building as a neighbor...
				const building_desc_t* neighbor_building = gb->get_tile()->get_desc();
				neighbor_building_clusters |= neighbor_building->get_clusters();
				if(  gb->get_pos().z == zpos  &&  neighbor_building->get_x()*neighbor_building->get_y()==1  ) {
					// also in right height and citybuilding, and (1x1) (so we don not tear down existing larger building, even if we can do that)
					area_level |= (neighbor_building->is_city_building() << i);
				}
			}
			else if(  gr->ist_natur()  &&  gr->get_pos().z+slope_t::max_diff(gr->get_grund_hang())==zpos  ) {
				// we can of course also build on nature
					area_level |= (1 << i);
			}
		}
	}

	// since the above test is only enough for 2x1 and 1x2, we must test more tiles, if we want larger
	koord maxsize=koord(1,1);
	switch(  area_level&3  ) {
		case 3:
			if(  hausbauer_t::get_largest_city_building_area() > 2  ) {
				// now test the remaining tiles for even laregr size
				for(  area_level=2;  area_level < lengthof(area3x3);  area_level++  ) {
					grund_t* gr = welt->lookup_kartenboden(k + area3x3[area_level]);
					if(  gr  &&  gr->get_typ() == grund_t::fundament  &&  gr->obj_bei(0)->get_typ() == obj_t::gebaeude  ) {
						// We have a building as a neighbor...
						if(  gebaeude_t const* const testgb = obj_cast<gebaeude_t>(gr->first_obj())  ) {
							// We really have a building as a neighbor...
							const building_desc_t* neighbor_building = testgb->get_tile()->get_desc();
							if(  testgb->get_pos().z == zpos  &&  neighbor_building->is_city_building()  &&  neighbor_building->get_x()*neighbor_building->get_y()==1  ) {
								// also in right height and citybuilding
								maxsize = area3x3[area_level]+koord(1,1);
								continue;
							}
						}
						else if(  gr->ist_natur()  &&  gr->get_pos().z+slope_t::max_diff(gr->get_grund_hang())==zpos  ) {
							// we can of course also build on nature
							maxsize = area3x3[area_level]+koord(1,1);
							continue;
						}
					}
					// we only reach here upon unsuitable ground
					break;
				}
			}
			break;
		case 2:
			maxsize = area3x3[1]+koord(1,1);
			break;
		case 1:
			maxsize = area3x3[0]+koord(1,1);
			break;
		default:
			break;
	}

	// Find a house to build
	const building_desc_t* h = NULL;

	if (sum_commercial > sum_industrial  &&  sum_commercial >= sum_residential) {
		h = hausbauer_t::get_commercial(0, current_month, cl, neighbor_building_clusters, koord(1,1), maxsize);
		if (h != NULL) {
			arb += (h->get_level()+1) * 20;
		}
	}

	if (h == NULL  &&  sum_industrial > sum_residential  &&  sum_industrial >= sum_commercial) {
		h = hausbauer_t::get_industrial(0, current_month, cl, neighbor_building_clusters, koord(1,1), maxsize);
		if (h != NULL) {
			arb += (h->get_level()+1) * 20;
		}
	}

	if (h == NULL  &&  sum_residential > sum_industrial  &&  sum_residential >= sum_commercial) {
		h = hausbauer_t::get_residential(0, current_month, cl, neighbor_building_clusters, koord(1,1), maxsize);
		if (h != NULL) {
			// will be aligned next to a street
			won += (h->get_level()+1) * 10;
		}
	}

	// so we found at least one suitable building for this place
	int rotation = orient_city_building( k, h, maxsize );
	if(  rotation >= 0  ) {
		const gebaeude_t* gb = hausbauer_t::build(NULL, k, rotation, h);
		add_gebaeude_to_stadt(gb);
	}
	// to be extended for larger building ...
	update_city_street(k);
}



void stadt_t::renovate_city_building(gebaeude_t *gb)
{
	const building_desc_t::btype alt_typ = gb->get_tile()->get_desc()->get_type();
	if(  !gb->is_city_building()  ) {
		return; // only renovate res, com, ind
	}

	// Now we are sure that this is a city building
	const building_desc_t *gb_desc = gb->get_tile()->get_desc();
	const int level = gb_desc->get_level();

	if(  welt->get_timeline_year_month() > gb_desc->no_renovation_month()  ) {
		// this is a historic city building (as defined by the pak set author), so do not renovate
		return;
	}

	koord k = gb->get_pos().get_2d() - gb->get_tile()->get_offset();

	// Divide unemployed by 4, because it counts towards commercial and industrial,
	// and both of those count 'double' for population relative to residential.
	const int employment_wanted  = get_unemployed() / 4;
	const int housing_wanted = get_homeless() / 4;

	int industrial_suitability, commercial_suitability, residential_suitability;
	bewerte_res_com_ind(k, industrial_suitability, commercial_suitability, residential_suitability );

	const int sum_industrial   = industrial_suitability  + employment_wanted;
	const int sum_commercial = commercial_suitability  + employment_wanted;
	const int sum_residential   = residential_suitability + housing_wanted;

	// does the timeline allow this building?
	const uint16 current_month = welt->get_timeline_year_month();
	const climate cl = welt->get_climate( gb->get_pos().get_2d() );

	// Run through orthogonal neighbors (only), and oneself  looking for which cluster to build
	// This is a bitmap -- up to 32 clustering types are allowed.
	uint32 neighbor_building_clusters = gb->get_tile()->get_desc()->get_clusters();
	sint8 zpos = gb->get_pos().z;
	koord minsize = gb->get_tile()->get_desc()->get_size(gb->get_tile()->get_layout());
	// since we handle buildings larger than (1x1) we test all periphery
	koord lu = k - koord( 1, 1 );
	koord rd = k + minsize;
	for( sint16 x = lu.x; x<=rd.x; x++ ) {
		for( sint16 y = lu.y; y<=rd.y; y++ ) {
			if(  koord(x,y)!=k  ) {
				if(  grund_t *gr = welt->lookup_kartenboden(x,y)  ) {
					if(  gebaeude_t const* const testgb = gr->find<gebaeude_t>()  ) {
						if(  testgb->get_tile()->get_desc() != gb->get_tile()->get_desc()  &&  testgb->is_city_building()  ) {
							// We really have a different building as a neighbor...
							const building_desc_t* neighbor_building = testgb->get_tile()->get_desc();
							neighbor_building_clusters |= neighbor_building->get_clusters();
						}
					}
				}
			}
		}
	}

	// now test the surrounding tiles for larger size
	koord maxsize=minsize;
	if(  hausbauer_t::get_largest_city_building_area() > 1  ) {
		for(  uint area_level=0;  area_level < lengthof(area3x3);  area_level++  ) {
			grund_t* gr = welt->lookup_kartenboden(k + area3x3[area_level]);
			if(  gr  &&  gr->get_typ() == grund_t::fundament  &&  gr->obj_bei(0)->get_typ() == obj_t::gebaeude  ) {
				// We have a building as a neighbor...
				if(  gebaeude_t const* const testgb = obj_cast<gebaeude_t>(gr->first_obj())  ) {
					// We really have a building here
					const building_desc_t* neighbor_building = testgb->get_tile()->get_desc();
					if(  neighbor_building->is_city_building()  ) {

						if(  gb->get_tile()->get_desc() == neighbor_building   &&   testgb->get_tile()->get_offset() == area3x3[area_level]  ) {
							// part of same building
							maxsize = area3x3[ area_level ] + koord( 1, 1 );
							continue;
						}
						if(  testgb->get_pos().z == zpos   &&   neighbor_building->get_x()*neighbor_building->get_y() == 1 ) {
							// also in right height and citybuilding
							maxsize = area3x3[ area_level ] + koord( 1, 1 );
							continue;
						}
					}
				}
#if 0
/* since we only replace tiles, natur is not allowed for the moment, but could be easily changed */
				else if(  gr->ist_natur()  &&  gr->get_pos().z+slope_t::max_diff(gr->get_grund_hang())==zpos  ) {
					// we can of course also build on nature
					maxsize = area3x3[area_level]+koord(1,1);
					continue;
				}
#endif
			}
			// we only reach here upon unsuitable ground
			break;
		}
	}

	building_desc_t::btype want_to_have = building_desc_t::unknown;
	int sum = 0;

	// try to build
	const building_desc_t* h = NULL;
	if (sum_commercial > sum_industrial && sum_commercial > sum_residential) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == building_desc_t::city_com ? level + 1 : level);
		h = hausbauer_t::get_commercial(try_level, current_month, cl, neighbor_building_clusters, minsize, maxsize );
		if(  h != NULL  &&  h->get_level() >= try_level  ) {
			want_to_have = building_desc_t::city_com;
			sum = sum_commercial;
		}
	}
	// check for industry, also if we wanted com, but there was no com good enough ...
	if(    (sum_industrial > sum_commercial  &&  sum_industrial > sum_residential) ||
	       (sum_commercial > sum_residential  &&  want_to_have == building_desc_t::unknown)  ) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == building_desc_t::city_ind ? level + 1 : level);
		h = hausbauer_t::get_industrial(try_level , current_month, cl, neighbor_building_clusters, minsize, maxsize );
		if(  h != NULL  &&  h->get_level() >= try_level  ) {
			want_to_have = building_desc_t::city_ind;
			sum = sum_industrial;
		}
	}
	// check for residence
	// (sum_wohnung>sum_industrie  &&  sum_wohnung>sum_gewerbe
	if(  want_to_have == building_desc_t::unknown  ) {
		// we must check, if we can really update to higher level ...
		const int try_level = (alt_typ == building_desc_t::city_res ? level + 1 : level);
		h = hausbauer_t::get_residential(try_level, current_month, cl, neighbor_building_clusters, minsize, maxsize );
		if(  h != NULL  &&  h->get_level() >= try_level  ) {
			want_to_have = building_desc_t::city_res;
			sum = sum_residential;
		}
		else {
			h = NULL;
		}
	}

	if (alt_typ != want_to_have) {
		sum -= level * 10;
	}

	// good enough to renovate, and we found a building?
	if(  sum > 0  &&  h != NULL  ) {
//		DBG_MESSAGE("stadt_t::renovate_city_building()", "renovation at %i,%i (%i level) of typ %i to typ %i with desire %i", k.x, k.y, alt_typ, want_to_have, sum);

		// no renovation for now, if new is smaller
		assert(  gb_desc->get_x()*gb_desc->get_y() <= h->get_x()*h->get_y()  );

		int rotation = 0;
		if(  h->get_all_layouts()>1  ) {

			// only do this of symmetric of small enough building
			if(  h->get_x()==h->get_y()  ||  (h->get_x()<maxsize.y  &&  h->get_x()<maxsize.x)  ) {
				// check for pavement
				int streetdir = 0;
				for(  int i = 0;  i < 4;  i++  ) {
					// Neighbors goes through these in 'preferred' order, orthogonal first
					grund_t *gr = welt->lookup_kartenboden(k + neighbors[i]);
					if(  gr  &&  gr->get_weg_hang() == gr->get_grund_hang()  &&  gr->hat_weg(road_wt)  ){
						streetdir += (1 << i);
					}
				}
				// not completely unique layout, see if any of the neighbouring building gives a hint
				rotation = building_layout[streetdir] & ~CHECK_NEIGHBOUR;
				bool unique_orientation = !(building_layout[streetdir] & CHECK_NEIGHBOUR);
				// only streets in diagonal corners => make a house there in L direction
				if(  !streetdir  ) {
					int count = 0;
					for(  int i = 4;  i < 8;  i++  ) {
						// Neighbors goes through these in 'preferred' order, orthogonal first
						grund_t *gr = welt->lookup_kartenboden(k + neighbors[i]);
						if(  gr  &&  gr->get_weg_hang() == gr->get_grund_hang()  &&  gr->hat_weg(road_wt)  ) {
							rotation = i;
							count ++;
						}
					}
					unique_orientation = (count==1);
				}
				if(  !unique_orientation  ) {
					int max_layout = h->get_all_layouts()-1;
					for(  int i = 0;  i < 4;  i++  ) {
						// Neighbors goes through these in 'preferred' order, orthogonal first
						grund_t *gr = welt->lookup_kartenboden(k + neighbors[i]);
						if(  gr  &&  gr->get_typ()==grund_t::fundament  ){
							if(  gebaeude_t *gb = gr->find<gebaeude_t>()  ) {
								if(  gb->get_tile()->get_desc()->get_all_layouts() > max_layout  ) {
									// so take the roation of the next bilding with similar or more rotations
									rotation = gb->get_tile()->get_layout();
									max_layout = gb->get_tile()->get_desc()->get_all_layouts();
									if(  h->get_clusters() == 0  &&  gb->get_tile()->get_desc() == h  ) {
										// we only renovate, if there is not an identical building (unless its a cluster)
										return;
									}
								}
							}
						}
					}
				}
			}
			else {
				// asymmetric building
				rotation = (h->get_x(0)<=maxsize.x  &&  h->get_y(0)<=maxsize.y) ? 0 : 1;
			}
		}

		// we only renovate, if there is not an identical building (unless its a cluster)
		if(  !h->get_clusters() ) {
			for( int i = 0; i < 8; i++ ) {
				koord p = k;
				p.x += neighbors[i].x > 0 ? h->get_x( rotation ) : neighbors[i].x;
				p.y += neighbors[i].y > 0 ? h->get_y( rotation ) : neighbors[i].y;
				grund_t *gr = welt->lookup_kartenboden(p);
				if( gr  &&  gr->get_typ() == grund_t::fundament ) {
					if( gebaeude_t *gb = gr->find<gebaeude_t>() ) {
						if( gb->get_tile()->get_desc() == h ) {
							// same building => do not renovate
							return;
						}
					}
				}
			}
		}

		// ok now finally replace
		for(  int x=0;  x<h->get_x(rotation);  x++  ) {
			for(  int y=0;  y<h->get_y(rotation);  y++  ) {
				koord kpos = k+koord(x,y);
				grund_t *gr = welt->lookup_kartenboden(kpos);
				gebaeude_t *oldgb = gr->find<gebaeude_t>();
				switch(oldgb->get_tile()->get_desc()->get_type()) {
					case building_desc_t::city_res: won -= level * 10; break;
					case building_desc_t::city_com: arb -= level * 20; break;
					case building_desc_t::city_ind: arb -= level * 20; break;
					default: break;
				}
				// exchange building; try to face it to street in front
				oldgb->mark_images_dirty();
				oldgb->set_tile( h->get_tile(rotation, x, y), true );
				welt->lookup_kartenboden(kpos)->calc_image();
				update_gebaeude_from_stadt(oldgb);
				update_city_street(kpos);
				switch(h->get_type()) {
					case building_desc_t::city_res: won += h->get_level() * 10; break;
					case building_desc_t::city_com: arb += h->get_level() * 20; break;
					case building_desc_t::city_ind: arb += h->get_level() * 20; break;
					default: break;
				}
			}
		}
	}
}


#ifdef DESTINATION_CITYCARS
void stadt_t::generate_private_cars(koord pos, koord target)
{
	if (!private_car_t::list_empty()  &&  number_of_cars>0  ) {
		koord k;
		for (k.y = pos.y - 1; k.y <= pos.y + 1; k.y++) {
			for (k.x = pos.x - 1; k.x <= pos.x + 1; k.x++) {
				if(  grund_t* gr = welt->lookup_kartenboden(k) ) {
					const weg_t* weg = gr->get_weg(road_wt);
					if (weg != NULL && (
								weg->get_ribi_unmasked(road_wt) == ribi_t::northsouth ||
						        weg->get_ribi_unmasked(road_wt) == ribi_t::eastwest)  &&
						        player_t::check_owner(NULL,w->get_owner()
					) {
						// already a car here => avoid congestion
						if(gr->obj_bei(gr->get_top()-1)->is_moving()) {
							continue;
						}
						private_car_t* vt = new private_car_t(gr, target);
						gr->obj_add(vt);
						welt->sync.add(vt);
						city_history_month[0][HIST_CITYCARS] ++;
						city_history_year[0][HIST_CITYCARS] ++;
						number_of_cars --;
						return;
					}
				}
			}
		}
	}
}
#endif


/**
 * baut ein Stueck Strasse
 * @param k Bauposition
 */
bool stadt_t::build_road(const koord k, player_t* player_, bool forced)
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

	// If not able to build here, try to make artificial slope
	slope_t::type slope = bd->get_grund_hang();
	if (!slope_t::is_way(slope)) {
		climate c = welt->get_climate(k);
		if (welt->can_flatten_tile(NULL, k, bd->get_hoehe()+1, true)) {
			welt->flatten_tile(NULL, k, bd->get_hoehe()+1, true);
		}
		else if(  bd->get_hoehe() > welt->get_water_hgt(k)  &&  welt->can_flatten_tile(NULL, k, bd->get_hoehe() )  ) {
			welt->flatten_tile(NULL, k, bd->get_hoehe());
		}
		else {
			return false;
		}
		// kartenboden may have changed - also ensure is land
		bd = welt->lookup_kartenboden(k);
		if (bd->get_typ() == grund_t::wasser) {
			welt->set_water_hgt_nocheck(k, bd->get_hoehe()-1);
			welt->access(k)->correct_water();
			welt->set_climate(k, c, true);
			bd = welt->lookup_kartenboden(k);
		}
	}

	// initially allow all possible directions ...
	ribi_t::ribi allowed_dir = (bd->get_grund_hang() != slope_t::flat ? ribi_t::doubles(ribi_type(bd->get_weg_hang())) : (ribi_t::ribi)ribi_t::all);

	// we have here a road: check for four corner stops
	const gebaeude_t* gb = bd->find<gebaeude_t>();
	if(gb) {
		// nothing to connect
		if(gb->get_tile()->get_desc()->get_all_layouts()==4) {
			// single way
			allowed_dir = ribi_t::layout_to_ribi[gb->get_tile()->get_layout()];
		}
		else if(gb->get_tile()->get_desc()->get_all_layouts()) {
			// through way
			allowed_dir = ribi_t::doubles( ribi_t::layout_to_ribi[gb->get_tile()->get_layout() & 1] );
		}
		else {
			dbg->error("stadt_t::build_road()", "building on road with not directions at %i,%i?!?", k.x, k.y );
		}
	}

	// we must not built on water or runways etc.
	// only crossing or tramways allowed
	if(  bd->hat_weg(track_wt)  ) {
		weg_t* sch = bd->get_weg(track_wt);
		if (sch->get_desc()->get_styp() != type_tram) {
			// not a tramway
			ribi_t::ribi r = sch->get_ribi_unmasked();
			if (!ribi_t::is_straight(r)) {
				// no building on crossings, curves, dead ends
				return false;
			}
			// just the other directions are allowed
			allowed_dir &= ~r;
		}
	}

	// determine now, in which directions we can connect to another road
	ribi_t::ribi connection_roads = ribi_t::none;
	// add ribi's to connection_roads if possible
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nesw[r] & allowed_dir) {
			// now we have to check for several problems ...
			grund_t* bd2;
			if(bd->get_neighbour(bd2, invalid_wt, ribi_t::nesw[r])) {
				if(bd2->get_typ()==grund_t::fundament  ||  bd2->get_typ()==grund_t::wasser) {
					// not connecting to a building of course ...
				}
				else if (!bd2->ist_karten_boden()) {
					// do not connect to elevated ways / bridges
				}
				else if (bd2->get_typ()==grund_t::tunnelboden  &&  ribi_t::nesw[r]!=ribi_type(bd2->get_grund_hang())) {
					// not the correct slope
				}
				else if (bd2->get_typ()==grund_t::brueckenboden
					&&  (bd2->get_grund_hang()==slope_t::flat  ?  ribi_t::nesw[r]!=ribi_type(bd2->get_weg_hang())
					                                           :  ribi_t::backward(ribi_t::nesw[r])!=ribi_type(bd2->get_grund_hang()))) {
					// not the correct slope
				}
				else if(bd2->hat_weg(road_wt)) {
					const gebaeude_t* gb = bd2->find<gebaeude_t>();
					if(gb) {
						uint8 layouts = gb->get_tile()->get_desc()->get_all_layouts();
						// nothing to connect
						if(layouts==4) {
							// single way
							if(ribi_t::nesw[r]==ribi_t::backward(ribi_t::layout_to_ribi[gb->get_tile()->get_layout()])) {
								// allowed ...
								connection_roads |= ribi_t::nesw[r];
							}
						}
						else if(layouts==2 || layouts==8 || layouts==16) {
							// through way
							if((ribi_t::doubles( ribi_t::layout_to_ribi[gb->get_tile()->get_layout() & 1] )&ribi_t::nesw[r])!=0) {
								// allowed ...
								connection_roads |= ribi_t::nesw[r];
							}
						}
						else {
							dbg->error("stadt_t::build_road()", "building on road with not directions at %i,%i?!?", k.x, k.y );
						}
					}
					else if(bd2->get_depot()) {
						// do not enter depots
					}
					else {
						// check slopes
						way_builder_t bauer( NULL );
						bauer.init_builder( way_builder_t::strasse | way_builder_t::terraform_flag, welt->get_city_road() );
						if(  bauer.check_slope( bd, bd2 )  ) {
							// allowed ...
							connection_roads |= ribi_t::nesw[r];
						}
					}
				}
			}
		}
	}

	// now add the ribis to the other ways (if there)
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nesw[r] & connection_roads) {
			grund_t* bd2 = welt->lookup_kartenboden(k + koord::nesw[r]);
			weg_t* w2 = bd2->get_weg(road_wt);
			w2->ribi_add(ribi_t::backward(ribi_t::nesw[r]));
			bd2->calc_image();
			bd2->set_flag( grund_t::dirty );
		}
	}

	if (connection_roads != ribi_t::none || forced) {

		if (!bd->weg_erweitern(road_wt, connection_roads)) {
			strasse_t* weg = new strasse_t();
			// city roads should not belong to any player => so we can ignore any construction costs ...
			weg->set_desc(welt->get_city_road());
			weg->set_gehweg(true);
			bd->neuen_weg_bauen(weg, connection_roads, player_);
			bd->calc_image(); // otherwise the
		}
		// check to bridge a river
		if(ribi_t::is_single(connection_roads)  &&  !bd->has_two_ways()  ) {
			koord zv = koord(ribi_t::backward(connection_roads));
			grund_t *bd_next = welt->lookup_kartenboden( k + zv );
			if(bd_next  &&  (bd_next->is_water()  ||  (bd_next->hat_weg(water_wt)  &&  bd_next->get_weg(water_wt)->get_desc()->get_styp()== type_river))) {
				// ok there is a river
				const bridge_desc_t *bridge = bridge_builder_t::find_bridge(road_wt, welt->get_city_road()->get_topspeed(), welt->get_timeline_year_month() );
				if(  bridge==NULL  ) {
					// does not have a bridge available ...
					return false;
				}
				const char *err = NULL;
				sint8 bridge_height;
				koord3d end = bridge_builder_t::find_end_pos(NULL, bd->get_pos(), zv, bridge, err, bridge_height, false);
				if(err  ||   koord_distance( k, end.get_2d())>3) {
					// try to find shortest possible
					end = bridge_builder_t::find_end_pos(NULL, bd->get_pos(), zv, bridge, err, bridge_height, true);
				}
				// if the river is nagigable, we need a two hight slope, so we have to start on a flat tile
				if(  err  &&  *err!=0  &&  strcmp(err,"Bridge is too long for this type!\n")!=0  &&  bd->get_weg_hang()!=slope_t::flat    ) {
					slope_t::type old_slope = bd->get_grund_hang();
					sint8 h_diff = slope_t::max_diff( old_slope );
					// raise up the tile
					bd->set_grund_hang( slope_t::flat );
					bd->set_hoehe( bd->get_hoehe() + h_diff );
					// transfer objects to on new grund
					for(  int i=0;  i<bd->get_top();  i++  ) {
						bd->obj_bei(i)->set_pos( bd->get_pos() );
					}

					end = bridge_builder_t::find_end_pos(NULL, bd->get_pos(), zv, bridge, err, bridge_height, false);
					if(err  ||   koord_distance( k, end.get_2d())>3) {
						// try to find shortest possible
						end = bridge_builder_t::find_end_pos(NULL, bd->get_pos(), zv, bridge, err, bridge_height, true);
					}
					// not successful: restore old slope
					if( (err  &&  *err != 0)  ||  end==koord3d::invalid  || koord_distance( k, end.get_2d())>5 ) {
						bd->set_grund_hang( old_slope );
						bd->set_hoehe( bd->get_hoehe() - h_diff );
						// transfer objects to on new grund
						for(  int i=0;  i<bd->get_top();  i++  ) {
							bd->obj_bei(i)->set_pos( bd->get_pos() );
						}
					}
					else {
						// update slope graphics on tile and tile in front
						if( grund_t *bd_recalc = welt->lookup_kartenboden( k + koord( 0, 1 ) ) ) {
							bd_recalc->check_update_underground();
						}
						if( grund_t *bd_recalc = welt->lookup_kartenboden( k + koord( 1, 0 ) ) ) {
							bd_recalc->check_update_underground();
						}
						if( grund_t *bd_recalc = welt->lookup_kartenboden( k + koord( 1, 1 ) ) ) {
							bd_recalc->check_update_underground();
						}
						bd->mark_image_dirty();
					}
				}
				if((err==NULL||*err == 0)  &&   koord_distance( k, end.get_2d())<=5  &&  welt->is_within_limits((end+zv).get_2d())) {
					bridge_builder_t::build_bridge(NULL, bd->get_pos(), end, zv, bridge_height, bridge, welt->get_city_road());
					// try to build one connecting piece of road
					build_road( (end+zv).get_2d(), NULL, false);
					// try to build a house near the bridge end
					uint32 old_count = buildings.get_count();
					for(uint8 i=0; i<lengthof(koord::neighbours)  &&  buildings.get_count() == old_count; i++) {
						koord c(end.get_2d()+zv+koord::neighbours[i]);
						if (welt->is_within_limits(c)) {
							build_city_building(end.get_2d()+zv+koord::neighbours[i]);
						}
					}
				}
			}
		}
		return true;
	}

	return false;
}


// will check a single random pos in the city, then build will be called
void stadt_t::build()
{
	const koord k(lo + koord::koord_random(ur.x - lo.x + 2,ur.y - lo.y + 2)-koord(1,1) );

	// do not build on any border tile
	if(  !welt->is_within_limits(k+koord(1,1))  ||  k.x<=0  ||  k.y<=0  ) {
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
		uint32 offset = simrand(num_road_rules); // start with random rule
		for (uint32 i = 0; i < num_road_rules  &&  !best_strasse.found(); i++) {
			uint32 rule = ( i+offset ) % num_road_rules;
			bewerte_strasse(k, 8 + road_rules[rule]->chance, *road_rules[rule]);
		}
		// ok => then built road
		if (best_strasse.found()) {
			build_road(best_strasse.get_pos(), NULL, false);
			INT_CHECK("simcity 1156");
			return;
		}

		// not good for road => test for house

		// since only a single location is checked, we can stop after we have found a positive rule
		best_haus.reset(k);
		const uint32 num_house_rules = house_rules.get_count();
		offset = simrand(num_house_rules); // start with random rule
		for(  uint32 i = 0;  i < num_house_rules  &&  !best_haus.found();  i++  ) {
			uint32 rule = ( i+offset ) % num_house_rules;
			bewerte_haus(k, 8 + house_rules[rule]->chance, *house_rules[rule]);
		}
		// one rule applied?
		if(  best_haus.found()  ) {
			build_city_building(best_haus.get_pos());
			INT_CHECK("simcity 1163");
			return;
		}

	}

	// renovation (only done when nothing matches a certain location
	if(  !buildings.empty()  &&  simrand(100) <= renovation_percentage  ) {
		// try to find a public owned building
		for(  uint8 i=0;  i<4;  i++  ) {
			gebaeude_t* const gb = pick_any(buildings);
			if(  player_t::check_owner(gb->get_owner(),NULL)  ) {
				renovate_city_building(gb);
				break;
			}
		}
		INT_CHECK("simcity 876");
	}
}


// find suitable places for cities
vector_tpl<koord>* stadt_t::random_place(const sint32 count, sint16 old_x, sint16 old_y)
{
	int cl = 0;
	for (int i = 0; i < MAX_CLIMATES; i++) {
		if (hausbauer_t::get_special(0, building_desc_t::townhall, welt->get_timeline_year_month(), false, (climate)i)) {
			cl |= (1 << i);
		}
	}
	DBG_DEBUG("karte_t::init()", "get random places in climates %x", cl);
	// search at least places which are 5x5 squares large
	slist_tpl<koord>* list = welt->find_squares( 5, 5, (climate_bits)cl, old_x, old_y);
	DBG_DEBUG("karte_t::init()", "found %i places", list->get_count());
	vector_tpl<koord>* result = new vector_tpl<koord>(count);

	// pre processed array: max 1 city from each square can be built
	// each entry represents a cell of minimum_city_distance/2 length and width
	const uint32 minimum_city_distance = welt->get_settings().get_minimum_city_distance();
	const uint32 xmax = (2*welt->get_size().x)/minimum_city_distance+1;
	const uint32 ymax = (2*welt->get_size().y)/minimum_city_distance+1;
	array2d_tpl< vector_tpl<koord> > places(xmax, ymax);
	while (!list->empty()) {
		const koord k = list->remove_first();
		places.at( (2*k.x)/minimum_city_distance, (2*k.y)/minimum_city_distance).append(k);
	}
	// weighted index vector into places array
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
	const uint32 xmax2 = welt->get_size().x/minimum_city_distance+1;
	const uint32 ymax2 = welt->get_size().y/minimum_city_distance+1;
	array2d_tpl< vector_tpl<koord> > result_places(xmax2, ymax2);

	for (int i = 0; i < count; i++) {
		// check distances of all cities to their respective neighbours
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
						for(koord const& l : result_places.at(i, j)) {
							if (koord_distance(k, l) < minimum_city_distance) {
								goto too_close;
							}
						}
					}
				}
			}

			// all cities are far enough => ok, find next place
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

		if (index_to_places.empty() && i < count - 1) {
			dbg->warning("stadt_t::random_place()", "Not enough places found!");
			break;
		}
	}
	delete list;

	return result;
}
