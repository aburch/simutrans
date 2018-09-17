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
#include <limits>

#include "boden/wege/strasse.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "simware.h"
#include "player/simplay.h"
#include "simplan.h"
#include "display/simimg.h"
#include "vehicle/simroadtraffic.h"
#include "utils/simrandom.h"
#include "simhalt.h"
#include "simfab.h"
#include "simcity.h"
#include "simmesg.h"
#include "simcolor.h"

#include "gui/karte.h"
#include "gui/city_info.h"

#include "descriptor/building_desc.h"
#include "descriptor/citycar_desc.h"

#include "simintr.h"
#include "simdebug.h"

#include "obj/gebaeude.h"
#include "obj/roadsign.h"
#include "obj/leitung2.h"
#include "obj/wayobj.h"

#include "dataobj/translator.h"
#include "dataobj/settings.h"
#include "dataobj/loadsave.h"
#include "dataobj/tabfile.h"
#include "dataobj/environment.h"
#include "dataobj/route.h"

#include "finder/building_placefinder.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/fabrikbauer.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"
#ifdef DEBUG_WEIGHTMAPS
#include "utils/dbg_weightmap.h"
#endif

#include "tpl/minivec_tpl.h"

// since we use 32 bit per growth steps, we use this variable to take care of the remaining sub citizen growth
#define CITYGROWTH_PER_CITICEN (0x0000000100000000ll)

karte_ptr_t stadt_t::welt; // one is enough ...


// Electricity demand information.
// @author: jamespetts
// @author: neroden
// (But much of this code is adapted from the speed bonus code,
// written by Prissi).

class electric_consumption_record_t {
public:
	sint64 year;
	sint16 consumption_percent;
	electric_consumption_record_t( sint64 y = 0, sint16 consumption = 0 ) {
		year = y*12;
		consumption_percent = consumption;
	};
};

static uint16 default_electricity_consumption = 100;

static vector_tpl<electric_consumption_record_t> electricity_consumption[1];

void stadt_t::electricity_consumption_init(const std::string &objfilename)
{
	tabfile_t consumption_file;
	// first take user data, then user global data
	if (!consumption_file.open((objfilename+"config/electricity.tab").c_str()))
	{
		dbg->message("stadt_t::electricity_consumption_init()", "Error opening config/electricity.tab.\nWill use default value." );
		return;
	}

	tabfileobj_t contents;
	consumption_file.read(contents);

	/* init the values from line with the form year, proportion, year, proportion
	 * must be increasing order!
	 */
	int *tracks = contents.get_ints("electricity_consumption");
	if((tracks[0]&1)==1)
	{
		dbg->message("stadt_t::electricity_consumption_init()", "Ill formed line in config/electricity.tab.\nWill use default value. Format is year,ownership percentage[ year,ownership percentage]!" );
		electricity_consumption->clear();
		return;
	}
	electricity_consumption[0].resize( tracks[0]/2 );
	for(  int i=1;  i<tracks[0];  i+=2  )
	{
		electric_consumption_record_t c( tracks[i], tracks[i+1] );
		electricity_consumption[0].append( c );
	}
	delete [] tracks;
}

/**
* Reads/writes electricity consumption data from/to a savegame
* called from karte_t::save and karte_t::load
* only written for network games
* @author jamespetts
*/
void stadt_t::electricity_consumption_rdwr(loadsave_t *file)
{
	if(file->get_extended_version() < 9)
	{
		 return;
	}

	if(file->is_saving())
	{
		uint32 count = electricity_consumption[0].get_count();
		file->rdwr_long(count);
		ITERATE(electricity_consumption[0], i)
		{
			file->rdwr_longlong(electricity_consumption[0].get_element(i).year);
			file->rdwr_short(electricity_consumption[0].get_element(i).consumption_percent);
		}
	}

	else
	{
		electricity_consumption->clear();
		uint32 counter;
		file->rdwr_long(counter);
		sint64 year = 0;
		uint16 consumption_percent = 0;
		for(uint32 c = 0; c < counter; c ++)
		{
			file->rdwr_longlong(year);
			file->rdwr_short(consumption_percent);
			electric_consumption_record_t ele(year / 12, consumption_percent);
			electricity_consumption[0].append( ele );
		}
	}
}


// Returns a percentage
uint16 stadt_t::get_electricity_consumption(sint32 monthyear) const
{

	if(monthyear == 0)
	{
		return default_electricity_consumption;
	}

	// ok, now lets see if we have data for this
	if(electricity_consumption->get_count())
	{
		uint i=0;
		while(i < electricity_consumption->get_count() && monthyear >= electricity_consumption[0][i].year)
		{
			i++;
		}
		if(i == electricity_consumption->get_count())
		{
			// past final year
			return electricity_consumption[0][i-1].consumption_percent;
		}
		else if(i == 0)
		{
			// before first year
			return electricity_consumption[0][0].consumption_percent;
		}
		else
		{
			// interpolate linear
			const sint32 delta_consumption_percent = electricity_consumption[0][i].consumption_percent - electricity_consumption[0][i-1].consumption_percent;
			const sint64 delta_years = electricity_consumption[0][i].year - electricity_consumption[0][i-1].year;
			return (((delta_consumption_percent*(monthyear-electricity_consumption[0][i-1].year)) / delta_years ) + electricity_consumption[0][i-1].consumption_percent);
		}
	}
	else
	{
		return default_electricity_consumption;
	}
}


/********************************* From here on cityrules stuff *****************************************/


/**
 * in this fixed interval, construction will happen
 * 21s = 21000 per house
 */
const uint32 stadt_t::city_growth_step = 21000;

/**
 * this is the default factor applied to increase the likelihood
 * of getting a building with a matched cluster
 * The appropriate size for this really depends on what percentage of buildings at any given level
 * are part of a cluster -- in pak128.britain it needs to be as high as 100 to generate terraces
 */
uint32 stadt_t::cluster_factor = 100;

/*
 * distribution_weight of success when a city tries to extend a road along a bridge
 * The rest of the time it will fail and build somewhere else instead
 * (This avoids overbuilding of free bridges)
 * @author neroden
 */
static uint32 bridge_success_percentage = 25;

/*
 * distribution_weight to do renovation instead new building (in percent)
 * @author prissi
 */
static uint32 renovation_percentage = 25;

/*
 * how many buildings will be renovated in one step
 */
static uint32 renovations_count = 1;

/*
 * how hard we want to find them
 */
static uint32 renovations_try = 3;

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

// order: res, com, ind
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
	sint16  distribution_weight;
	vector_tpl<rule_entry_t> rule;
	rule_t(uint32 count=0) : distribution_weight(0), rule(count) {}

	void rdwr(loadsave_t* file)
	{
		file->rdwr_short(distribution_weight);

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
 * T = not a stop	// added in 88.03.3
 * t = is a stop // added in 88.03.3
 * u = good slope for way
 * U = not a slope for ways
 * . = beliebig
 */

//enum {	rule_anything	= 0,	// .
//	rule_no_road 	= 1, 	// S
//	rule_is_road 	= 2, 	// s
// 	rule_is_natur  	= 4, 	// n
//  	rule_no_house	= 8, 	// H
//   	rule_is_house	= 16, 	// h
//   	rule_no_stop 	= 32,	// T
//	rule_is_stop	= 64, 	// t
// 	rule_good_slope	= 128, 	// u
//  	rule_bad_slope	= 256,	// U
//   	rule_indefinite	= 512,
//   	rule_known	= 1024,	// location already evaluated
//    	rule_any_rule = rule_indefinite -1,
//};
//
///*
// * translation of char rules to the integers
// */
//uint16 rule_char_to_int(const char r)
//{
//	switch (r) {
//		case '.': return rule_anything;
//		case 'S': return rule_no_road;
//		case 's': return rule_is_road;
//		case 'h': return rule_is_house;
//		case 'H': return rule_no_house;
//		case 'n': return rule_is_natur;
//		case 'u': return rule_good_slope;
//		case 'U': return rule_bad_slope;
//		case 't': return rule_is_stop;
//		case 'T': return rule_no_stop;
//		default:  return rule_indefinite;
//	}
//}
//
//// static array to cache evaluations of locations
//static sparse_tpl<uint16>*location_cache = NULL;
//
//static stadt_t* location_cache_city = NULL;
//
//static uint64 cache_hits=0, cache_writes=0;
//#define use_cache
//
//void stadt_t::reset_location_cache(koord size) {
//#ifdef use_cache
//	if (location_cache) delete location_cache;
//	location_cache = new sparse_tpl<uint16> (size);
//	location_cache_city = NULL;
//#endif
//}
//
//void stadt_t::disable_location_cache() {
//	if (location_cache) delete location_cache;
//	location_cache = NULL;
//	location_cache_city = NULL;
//}
//
//void clear_location_cache(stadt_t *city)
//{
//	printf("Location Cache: hits/writes = %lld/%lld\n", cache_hits, cache_writes);
//	if(location_cache)
//	{
//		location_cache->clear();
//	}
//	location_cache_city = city;
//	cache_hits=0;
//	cache_writes=0;
//}
//
///*
// * checks loc against all possible rules, stores this in location_cache
// * cache must be active (not NULL) - this must be checked before calling
// */
//uint16 stadt_t::bewerte_loc_cache(const koord pos, bool force)
//{
//	uint16 flag=0;
//#ifdef use_cache
////	if (location_cache) {
//		if (location_cache_city!=this) {
//			clear_location_cache(this);
//		}
//		else if (!force) {
//			flag = location_cache->get(pos);
//			cache_hits++;
//		}
////	}
//#endif
//	if (flag==0) {
//		const grund_t* gr = welt->lookup_kartenboden(pos);
//		// outside
//		if (gr==NULL) return 0;
//		// now do all the tests
//		flag |= gr->hat_weg(road_wt) ? rule_is_road : rule_no_road;
//		flag |= gr->get_typ() == grund_t::fundament ? (gr->obj_bei(0)->get_typ()==obj_t::gebaeude ? rule_is_house :0) : rule_no_house;
//		if (gr->ist_natur() && gr->kann_alle_obj_entfernen(NULL)== NULL) {
//			flag |= rule_is_natur;
//		}
//		flag |= gr->is_halt() ? rule_is_stop : rule_no_stop;
//		flag |= slope_t::is_way(gr->get_grund_hang()) ? rule_good_slope : rule_bad_slope;
//#ifdef use_cache
//		if (location_cache) {
//			location_cache->set(pos, flag | rule_known);
//			cache_writes ++;
//		}
//#endif
//	}
//	return flag & rule_any_rule;
//}


// here '.' is ignored, since it will not be tested anyway
static char const* const allowed_chars_in_rule = "SsnHhTtUu";


/*
 * @param pos position to check
 * @param regel the rule to evaluate
 * @return true on match, false otherwise
 * @author Hj. Malthaner
 */

//bool stadt_t::bewerte_loc(const koord pos, const rule_t &regel, uint16 rotation)
//{
//	//printf("Test for (%s) in rotation %d\n", pos.get_str(), rotation);
//	koord k;
//	const bool uses_cache = location_cache != NULL;
//
//		for(uint32 i=0; i<regel.rule.get_count(); i++){
//		rule_entry_t &r = regel.rule[i];
//		uint8 x,y;
//		switch (rotation) {
//			case   0: x=r.x; y=r.y; break;
//			case  90: x=r.y; y=6-r.x; break;
//			case 180: x=6-r.x; y=6-r.y; break;
//			case 270: x=6-r.y; y=r.x; break;
//		}
//
//		if (r.flag!=0) {
//			const koord k(pos.x+x-3, pos.y+y-3);
//
//			if (uses_cache) {
//				if ((bewerte_loc_cache(k) & r.flag) ==0) return false;
//			}
//			else {
//				const grund_t* gr = welt->lookup_kartenboden(k);
//				if (gr == NULL) {
//					// outside of the map => cannot apply this rule
//					return false;
//				}
//
//				switch (r.flag) {
//					case rule_is_road:
//						// road?
//						if (!gr->hat_weg(road_wt)) return false;
//						break;
//
//					case rule_no_road:
//						// not road?
//						if (gr->hat_weg(road_wt)) return false;
//						break;
//
//					case rule_is_house:
//						// is house
//						if (gr->get_typ() != grund_t::fundament  ||  gr->obj_bei(0)->get_typ()!=obj_t::gebaeude) return false;
//						break;
//
//					case rule_no_house:
//						// no house
//						if (gr->get_typ() == grund_t::fundament) return false;
//						break;
//
//					case rule_is_natur:
//						// nature/empty
//						if (!gr->ist_natur() || gr->kann_alle_obj_entfernen(NULL) != NULL) return false;
//						break;
//
// 					case rule_bad_slope:
// 						// unbuildable for road
// 						if (!slope_t::is_way(gr->get_grund_hang())) return false;
// 						break;
//
// 					case rule_good_slope:
// 						// road may be buildable
// 						if (slope_t::is_way(gr->get_grund_hang())) return false;
// 						break;
//
//					case rule_is_stop:
//						// here is a stop/extension building
//						if (!gr->is_halt()) return false;
//						break;
//
//					case rule_no_stop:
//						// no stop
//						if (gr->is_halt()) return false;
//						break;
//				}
//			}
bool stadt_t::bewerte_loc_has_public_road(const koord pos)
{
	grund_t *gr = welt->lookup_kartenboden(pos);
	weg_t *weg = gr->get_weg(road_wt);

	if (weg) {
		wayobj_t *wo = gr->get_wayobj(road_wt);
		if (wo && wo->get_desc()->is_noise_barrier()) {
			return false;
		}

		const roadsign_t* rs = gr->find<roadsign_t>();
		if (rs && rs->get_desc()->is_private_way()) {
			return false;
		}

		return true;
	}

	return false;
}

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
				// public road?
				if (!bewerte_loc_has_public_road(k)) return false;
				break;
			case 'S':
				// no road at all, not even a private one?
				if (bewerte_loc_has_public_road(k)) return false;
				break;
			case 'h':
				// is house.
				if (gr->get_typ() != grund_t::fundament  ||
				    (gr->obj_bei(0) && gr->obj_bei(0)->get_typ()!=obj_t::gebaeude))
				{
					return false;
				}
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
	//printf("Success\n");
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

bool stadt_t::maybe_build_road(koord k, bool map_generation)
{
	best_strasse.reset(k);
	const uint32 num_road_rules = road_rules.get_count();
	uint32 offset = simrand(num_road_rules, "void stadt_t::build");	// start with random rule
	for (uint32 i = 0; i < num_road_rules  &&  !best_strasse.found(); i++) {
		uint32 rule = ( i+offset ) % num_road_rules;
		sint32 rd = 8 + road_rules[rule]->distribution_weight;

		if (simrand(rd, "void stadt_t::bewerte_strasse") == 0) {
			best_strasse.check(k, bewerte_pos(k, *road_rules[rule]));
		}
	}

	if (best_strasse.found()) {
		bool success = build_road(best_strasse.get_pos(), NULL, false, map_generation);
		INT_CHECK("simcity 5095");
		return success;
	}

	return false;
}


void stadt_t::bewerte_haus(koord k, sint32 rd, const rule_t &regel)
{
	if (simrand(rd, "stadt_t::bewerte_haus") == 0) {
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
	const std::string user_dir=env_t::user_dir;
	if (!cityconf.open((user_dir+"cityrules.tab").c_str())) {
		if (!cityconf.open((objfilename+"config/cityrules.tab").c_str())) {
			dbg->fatal("stadt_t::init()", "Can't read cityrules.tab" );
			return false;
		}
	}

	tabfileobj_t contents;
	cityconf.read(contents);

	char buf[128];

	cluster_factor = (uint32)contents.get_int("cluster_factor", 100);
	bridge_success_percentage = (uint32)contents.get_int("bridge_success_percentage", 25);
	renovation_percentage = (uint32)contents.get_int("renovation_percentage", renovation_percentage);
	renovations_count = (uint32)contents.get_int("renovations_count", renovations_count);
	renovations_try   = (uint32)contents.get_int("renovations_try", renovations_try);

	// to keep compatible with the typo, here both are ok
	// NOTE: This parameter is disused at present following Neroden's change in the building code as of July 2013.
	// This has the result that cities are all square on flat terrain, which is unfortunate.
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
		sprintf(buf, "house_%u", num_house_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_house_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %u house building rules", num_house_rules);

	uint32 num_road_rules = 0;
	for (;;) {
		sprintf(buf, "road_%u", num_road_rules + 1);
		if (contents.get_string(buf, 0)) {
			num_road_rules++;
		} else {
			break;
		}
	}
	DBG_MESSAGE("stadt_t::init()", "Read %u road building rules", num_road_rules);

	clear_ptr_vector( house_rules );
	for (uint32 i = 0; i < num_house_rules; i++) {
		house_rules.append(new rule_t());
		sprintf(buf, "house_%u.distribution_weight", i + 1);
		house_rules[i]->distribution_weight = contents.get_int(buf, 0);

		sprintf(buf, "house_%u", i + 1);
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
			dbg->fatal("stadt_t::cityrules_init()", "house rule %u has bad format!", i + 1);
		}

		// put rule into memory
		const uint8 offset = (7 - (uint)size) / 2;
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
		dbg->message("stadt_t::cityrules_init()", "House-Rule %d: distribution_weight %d\n",i,house_rules[i]->distribution_weight);
		for(uint32 j=0; j< house_rules[i]->rule.get_count(); j++) {
			dbg->message("stadt_t::cityrules_init()", "House-Rule %d: Pos (%d,%d) Flag %d\n",i,house_rules[i]->rule[j].x,house_rules[i]->rule[j].y,house_rules[i]->rule[j].flag);
			}
	}

	clear_ptr_vector( road_rules );
	for (uint32 i = 0; i < num_road_rules; i++) {
		road_rules.append(new rule_t());
		sprintf(buf, "road_%d.distribution_weight", i + 1);
		road_rules[i]->distribution_weight = contents.get_int(buf, 0);

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
		dbg->message("stadt_t::cityrules_init()", "Road-Rule %d: distribution_weight %d\n",i,road_rules[i]->distribution_weight);
		for(uint32 j=0; j< road_rules[i]->rule.get_count(); j++)
			dbg->message("stadt_t::cityrules_init()", "Road-Rule %d: Pos (%d,%d) Flag %d\n",i,road_rules[i]->rule[j].x,road_rules[i]->rule[j].y,road_rules[i]->rule[j].flag);

	}
	return true;
}

/**
* Reads/writes city configuration data from/to a savegame
* called from karte_t::save and karte_t::load
* only written for networkgames
* @author Dwachs
*/
void stadt_t::cityrules_rdwr(loadsave_t *file)
{
	const uint32 exp_ver = file->get_extended_version();
	if(exp_ver > 0 && exp_ver < 9)
	{
		 return;
	}

	const uint32 std_ver = file->get_version();
	if( exp_ver == 0 && std_ver >= 112008 ) {
		file->rdwr_long( cluster_factor );
	}

	file->rdwr_long(renovation_percentage);
	file->rdwr_long(min_building_density);

	// cluster_factor and bridge_success_percentage added by neroden.
	// It's not clear how to version this, but it *is* only
	// for networked games... both is *needed* for network games though

	// NOTE: This code is not *only* called for network games.
	if(exp_ver >= 12 || (exp_ver > 0 && std_ver >= 112005))
	{
		file->rdwr_long(cluster_factor);
		file->rdwr_long(bridge_success_percentage);
	}

	if(file->get_extended_version() >= 12 || (file->get_version() >= 112007 && file->get_extended_version() >= 11))
	{
		file->rdwr_long(renovations_try);
		file->rdwr_long(renovations_count);
	}

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
 * monument_placefinder_t:
 *
 * Search a free place for a building
 * Im Gegensatz zum building_placefinder_t werden Strassen auf den Raendern
 * toleriert.
 * Search a free place for a monument building
 * 22-Dec-02: Hajo: added safety checks for gr != 0 and plan != 0
 *
 * @author V. Meyer
 */
class monument_placefinder_t : public placefinder_t {
	public:
		monument_placefinder_t(karte_t* welt, sint16 radius) : placefinder_t(welt, radius) {}

		virtual bool is_tile_ok(koord pos, koord d, climate_bits cl) const
		{
			const planquadrat_t* plan = welt->access(pos + d);

			// Hajo: can't build here
			if (plan == NULL) return false;

			const grund_t* gr = plan->get_kartenboden();
			if(  ((1 << welt->get_climate( gr->get_pos().get_2d() )) & cl) == 0  ) {
				return false;
			}

			uint8 obj_idx;
			uint8 obj_count = gr->obj_count();

			for (obj_idx = 0; obj_idx < obj_count; obj_idx++) {
				obj_t *obj = gr->obj_bei(obj_idx);

				if (obj->get_owner() != NULL &&
				    obj->get_owner() != welt->get_public_player()) {
					/* XXX player-owned roads okay to remove? */
					/* XXX player-owned trams/electrification okay if is_boundary_tile()? */
					return false;
				}

				gebaeude_t *gb = obj_cast<gebaeude_t>(obj);

				if (gb && gb->get_typ() == building_desc_t::unknown) {
					return false;
				}
			}

			if (gr->get_grund_hang() != slope_t::flat) {
				return false;
			}

			if (gr->get_typ() == grund_t::fundament) {
				sint8 new_hgt;
				const uint8 new_slope = welt->recalc_natural_slope(gr->get_pos().get_2d(),new_hgt);

				if (new_slope != slope_t::flat) {
					return false;
				}
			}

			if (gr->hat_wege() && !gr->hat_weg(road_wt)) {
				return false;
			}

			if (gr->ist_bruecke() || gr->ist_tunnel()) {
				return false;
			}

			if (gr->kann_alle_obj_entfernen(NULL) != NULL) {
				return false;
			}

			if (is_boundary_tile(d)) {
			} else {
				/* XXX necessary? */
				if (gr->hat_weg(tram_wt)) {
					return false;
				}
			}

			return true;
		}
};


/**
 * townhall_placefinder_t:
 *
 * 22-Dec-02: Hajo: added safety checks for gr != 0 and plan != 0
 *
 * @author V. Meyer
 */
class townhall_placefinder_t : public placefinder_t {
	public:
		townhall_placefinder_t(karte_t* welt, uint8 dir_) : placefinder_t(welt), dir(dir_) {}

		virtual bool is_tile_ok(koord pos, koord d, climate_bits cl) const
		{
			const grund_t* gr = welt->lookup_kartenboden(pos + d);
			if (gr == NULL  ||  gr->get_grund_hang() != slope_t::flat) return false;

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
				((dir & ribi_t::east)!=0  &&  d.x == b - 1)) {
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


// this function adds houses to the city house list
// Please note: this is called during loading, on *every tile*.
// It's therefore not OK to recalc city borders in here.
void stadt_t::add_gebaeude_to_stadt(gebaeude_t* gb, bool ordered, bool do_not_add_to_world_list, bool do_not_update_stats)
{
	if (gb != NULL)
	{
		const building_tile_desc_t* tile = gb->get_tile();
		koord size = tile->get_desc()->get_size(tile->get_layout());
		const koord pos = gb->get_pos().get_2d() - tile->get_offset();
		koord k;

		// add all tiles
		for (k.y = 0; k.y < size.y; k.y++) {
			for (k.x = 0; k.x < size.x; k.x++) {
				if (gebaeude_t* const add_gb = obj_cast<gebaeude_t>(welt->lookup_kartenboden(pos + k)->first_obj())) {
					if(add_gb->get_tile()->get_desc()!=gb->get_tile()->get_desc()) {
						dbg->warning("stadt_t::add_gebaeude_to_stadt()", "two buildings \"%s\" and \"%s\" at (%i,%i), which might lead to problems", add_gb->get_tile()->get_desc()->get_name(), gb->get_tile()->get_desc()->get_name(), pos.x + k.x, pos.y + k.y);
						buildings.remove(add_gb);
						welt->remove_building_from_world_list(add_gb);
					}
					else
					{
						add_building_to_list(add_gb, ordered, do_not_add_to_world_list, do_not_update_stats);
					}
					add_gb->set_stadt(this);
					if (add_gb->get_tile()->get_desc()->is_townhall()) {
						has_townhall = true;
					}
				}
			}
		}
	}
}

void stadt_t::update_city_stats_with_building(gebaeude_t* building, bool remove)
{
	if(!building || building != building->get_first_tile())
	{
		return;
	}

	if(remove)
	{
		city_history_month[0][HIST_CITICENS] -= building->get_adjusted_population();
		city_history_year[0][HIST_CITICENS]  -= building->get_adjusted_population();

		city_history_month[0][HIST_JOBS] -= building->get_adjusted_jobs();
		city_history_year[0][HIST_JOBS]  -= building->get_adjusted_jobs();

		city_history_month[0][HIST_VISITOR_DEMAND] -= building->get_adjusted_visitor_demand();
		city_history_year[0][HIST_VISITOR_DEMAND]  -= building->get_adjusted_visitor_demand();
	}
	else
	{
		city_history_month[0][HIST_CITICENS] += building->get_adjusted_population();
		city_history_year[0][HIST_CITICENS]  += building->get_adjusted_population();

		city_history_month[0][HIST_JOBS] += building->get_adjusted_jobs();
		city_history_year[0][HIST_JOBS]  += building->get_adjusted_jobs();

		city_history_month[0][HIST_VISITOR_DEMAND] += building->get_adjusted_visitor_demand();
		city_history_year[0][HIST_VISITOR_DEMAND]  += building->get_adjusted_visitor_demand();
	}
}

// this function removes houses from the city house list
void stadt_t::remove_gebaeude_from_stadt(gebaeude_t* gb, bool map_generation, bool original_pos)
{
	if (!map_generation)
	{
		if(original_pos) {
			update_city_stats_with_building(gb, true);
		}
		welt->remove_building_from_world_list(gb);
	}
	buildings.remove(gb);

	gb->set_stadt(NULL);
	reset_city_borders();
}

/**
 * This function transfers a house from another city to this one
 * It currently only works on 1-tile buildings
 * It is also currently unused
 * @author neroden
 */
#if 0
bool stadt_t::take_citybuilding_from(stadt_t* old_city, gebaeude_t* gb)
{
 	if (gb == NULL) {
		return false;
	}
	if (old_city == NULL) {
		return false;
	}

	const building_desc_t::btype alt_typ = gb->get_buildingtyp();
	if (  alt_typ == building_desc_t::unknown  ) {
		return false; // only transfer res, com, ind
	}
	if (  gb->get_tile()->get_desc()->get_x()*gb->get_tile()->get_desc()->get_y() !=1  ) {
		return false; // too big
	}

	// Now we know we can transfer the building
	old_city->buildings.remove(gb);
	gb->set_stadt(NULL);
	old_city->reset_city_borders();

	add_building_to_list(add_gb);

	gb->set_stadt(this);
}
#endif

/**
 * Expand the city in a particular direction
 * North, south, east, and west are the only choices
Return true if it is OK for the city to expand to these borders,
 * and false if it is not OK
 *
 * It is not OK if it would overlap another city
 */
bool stadt_t::enlarge_city_borders(ribi_t::ribi direction) {
	koord new_lo, new_ur;
	koord test_first, test_stop, test_increment;
	switch (direction) {
		case ribi_t::north:
			// North
			new_lo = lo + koord(0, -1);
			new_ur = ur;
			test_first = koord(new_lo.x, new_lo.y);
			test_stop = koord(new_ur.x + 1, new_lo.y);
			test_increment = koord(1,0);
			break;
		case ribi_t::south:
			// South
			new_lo = lo;
			new_ur = ur + koord(0, 1);
			test_first = koord(new_lo.x, new_ur.y);
			test_stop = koord(new_ur.x + 1, new_ur.y);
			test_increment = koord(1,0);
			break;
		case ribi_t::east:
			// East
			new_lo = lo;
			new_ur = ur + koord(1, 0);
			test_first = koord(new_ur.x, new_lo.y);
			test_stop = koord(new_ur.x, new_ur.y + 1);
			test_increment = koord(0,1);
			break;
		case ribi_t::west:
			// West
			new_lo = lo + koord(-1, 0);
			new_ur = ur;
			test_first = koord(new_lo.x, new_lo.y);
			test_stop = koord(new_lo.x, new_ur.y + 1);
			test_increment = koord(0,1);
			break;
		default:
			// This is not allowed
			return false;
			break;
	}
	if (  !welt->is_within_limits(new_lo) || !welt->is_within_limits(new_ur)  ) {
		// Expansion would take us outside the map
		// Note that due to the square nature of the limits, we only need to test
		// opposite corners
		return false;
	}
	// Now check a row along that side to see if it's safe to expand
	for (koord test = test_first; test != test_stop; test = test + test_increment) {
		stadt_t* found_city = welt->access(test)->get_city();
		if (found_city && found_city != this) {
			// We'd be expanding into another city.  Don't!
			return false;
		}
	}
	// OK, it's safe to expand in this direction.  Do so.
	lo = new_lo;
	ur = new_ur;
	// Mark the tiles as owned by this city.
	for (koord test = test_first; test != test_stop; test = test + test_increment) {
		planquadrat_t* pl = welt->access(test);
		pl->set_city(this);
	}
	return true;
}

/**
 * Enlarge city limits.
 * Attempts to expand by one tile on one (random) side.
 * Refuses to expand off the map or into another city.
 * Returns false in the case of failure to expand.
 */
bool stadt_t::enlarge_city_borders() {
	// First, pick a direction, randomly.
	int offset_i = simrand(4, "stadt_t::enlarge_city_borders()");
	// We will try all four directions if necessary,
	// but start with a random choice.
	for (int i = 0; i < 4 ; i++) {
		ribi_t::ribi direction;
		switch (  (i + offset_i) % 4 ) {
			case 0:
				direction = ribi_t::north;
				break;
			case 1:
			default:
				direction = ribi_t::south;
				break;
			case 2:
				direction = ribi_t::east;
				break;
			case 3:
				direction = ribi_t::west;
				break;
		}
		if (enlarge_city_borders(direction)) {
			return true;
		}
		// otherwise try the next direction
	}
	// no directions worked
	return false;
}


bool stadt_t::is_within_city_limits(koord k) const
{
	return lo.x <= k.x  &&  ur.x >= k.x  &&  lo.y <= k.y  &&  ur.y >= k.y;
}


void stadt_t::check_city_tiles(bool del)
{
	// ur = SE corner
	// lo = NW corner
	// x = W - E
	// y = N - S
	const sint16 limit_west = lo.x;
	const sint16 limit_east = ur.x;
	const sint16 limit_north = lo.y;
	const sint16 limit_south = ur.y;

	for(int x = limit_west; x <= limit_east; x++)
	{
		for(int y = limit_north; y <= limit_south; y++)
		{
			const koord k(x, y);
			// This can be called with garbage data, especially during loading
			if(  welt->is_within_limits(k)  ) {
				planquadrat_t* plan = welt->access(k);
				if(!del)
				{
					plan->set_city(this);
				}
				else
				{
					if(plan->get_city() == this)
					{
						plan->set_city(NULL);
					}
				}
			}
		}
	}
}

/**
 * Reset city borders to be exactly large enough
 * to contain all the houses of the city (including the townhall, monuments, etc.)
 * and the townhall road
 */
void stadt_t::reset_city_borders()
{
	// Unmark all city tiles
	check_city_tiles(true);

	koord new_lo = pos;
	koord new_ur = pos;
	koord const& thr = get_townhall_road();
	// often thr is still (0,0) and corrupts this check.
	if (thr.x && thr.y)
	{
		if (new_lo.x > thr.x) {
			new_lo.x = thr.x;
		}
		if (new_lo.y > thr.y) {
			new_lo.y = thr.y;
		}
		if (new_ur.x < thr.x) {
			new_ur.x = thr.x;
		}
		if (new_ur.y < thr.y) {
			new_ur.y = thr.y;
		}
	}

	for (
			weighted_vector_tpl<gebaeude_t*>::const_iterator i = buildings.begin();
			i != buildings.end(); ++i) {
		gebaeude_t* gb = *i;
		if (gb->get_tile()->get_desc()->get_type() != building_desc_t::headquarters) {
			// Not an HQ
			koord gb_pos = gb->get_pos().get_2d();
			if (new_lo.x > gb_pos.x) {
				new_lo.x = gb_pos.x;
			}
			if (new_lo.y > gb_pos.y) {
				new_lo.y = gb_pos.y;
			}
			if (new_ur.x < gb_pos.x) {
				new_ur.x = gb_pos.x;
			}
			if (new_ur.y < gb_pos.y) {
				new_ur.y = gb_pos.y;
			}
		}
	}
	lo = new_lo;
	ur = new_ur;
	// Remark all city tiles
	check_city_tiles(false);
}

stadt_t::~stadt_t()
{
	// close info win
	destroy_win((ptrdiff_t)this);

	check_city_tiles(true);

	welt->remove_queued_city(this);

	// Remove references to this city from factories.
	FOR(vector_tpl<fabrik_t*>, factory, city_factories)
	{
		factory->clear_city();
	}

	if(  reliefkarte_t::get_karte()->get_city() == this  ) {
		reliefkarte_t::get_karte()->set_city(NULL);
	}

	// only if there is still a world left to delete from
	if(!welt->is_destroying() && welt->get_size().x > 1)
	{
		welt->lookup_kartenboden(pos)->set_text(NULL);

		// remove city info and houses
		while(!buildings.empty())
		{
			gebaeude_t* const gb = buildings.pop_back();
			assert(  gb!=NULL  &&  !buildings.is_contained(gb)  );

			if(gb->get_tile()->get_desc()->get_type() == building_desc_t::headquarters)
			{
				stadt_t *city = welt->find_nearest_city(gb->get_pos().get_2d());
				gb->set_stadt( city );
				if(city)
				{
					city->buildings.append_unique(gb, gb->get_adjusted_visitor_demand());
				}
			}
			else
			{
				gb->set_stadt( this );
				hausbauer_t::remove(welt->get_public_player(), gb, false);
			}
		}
		// Remove substations
		FOR(vector_tpl<senke_t*>, sub, substations)
		{
			sub->city = NULL;
		}

		const weighted_vector_tpl<stadt_t*>& cities = welt->get_cities();
		FOR(weighted_vector_tpl<stadt_t*>, const i, cities)
		{
			i->remove_connected_city(this);
		}

		// Find all buildings in the world that refer to this as their city and set their city to NULL
		welt->remove_all_building_references_to_city(this);

		check_city_tiles(true);
	}
	// avoid the bookkeeping if world gets destroyed
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


stadt_t::stadt_t(player_t* player, koord pos, sint32 citizens) :
	buildings(16),
	pax_destinations_old(welt->get_size()),
	pax_destinations_new(welt->get_size())
{
	assert(welt->is_within_limits(pos));

	pax_destinations_new_change = 0;
	next_growth_step = 0;
//	has_low_density = false;
	has_townhall = false;

	stadtinfo_options = 3;	// citizen and growth

	owner = player;

	this->pos = pos;

	bev = 0;
	arb = 0;
	won = 0;

	lo = ur = pos;

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

	/* get a unique cityname */
	char                          const* n       = "simcity";
	weighted_vector_tpl<stadt_t*> const& staedte = welt->get_cities();

	const vector_tpl<char*>& city_names = translator::get_city_name_list();

	// make sure we do only ONE random call regardless of how many names are available (to avoid desyncs in network games)
	if(  const uint32 count = city_names.get_count()  ) {
		uint32 idx = simrand( count, "stadt_t::stadt_t()" );
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
		simrand(5, "stadt_t::stadt_t() dummy");
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

	outgoing_private_cars = 0;
	incoming_private_cars = 0;

	calc_traffic_level();

	check_road_connexions = false;
	welt->add_queued_city(this);

	number_of_cars = 0;
}

stadt_t::stadt_t(loadsave_t* file) :
	buildings(16),
	pax_destinations_old(welt->get_size()),
	pax_destinations_new(welt->get_size())
{

	//step_count = 0;
	//next_step = 0;
	//step_interval = 1;
	next_growth_step = 0;
	//has_low_density = false;
	has_townhall = false;

	unsupplied_city_growth = 0;
	stadtinfo_options = 3;

	// These things are not yet saved as part of the city's history,
	// as doing so would require reversioning saved games.

	incoming_private_cars = 0;
	outgoing_private_cars = 0;

	rdwr(file);

	calc_traffic_level();

	check_road_connexions = false;
}


void stadt_t::rdwr(loadsave_t* file)
{
	sint32 owner_n;

	if (file->is_saving()) {
		owner_n = welt->sp2num(owner);
	}
	file->rdwr_str(name);
	// DBG_DEBUG("stadt_t::rdwr", "city'%s'", name);
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

	if(  file->get_version()>=112009  ) {
		// Must record the partial (less than 1 citizen) growth factor
		// Otherwise we will get network desyncs
		// Also allows accumulation of small growth factors
		file->rdwr_longlong(unsupplied_city_growth);
	}
	else if( file->is_loading()  ) {
		unsupplied_city_growth = 0;
	}
	// old values zentrum_namen_cnt : aussen_namen_cnt
	if(file->get_version()<99018) {
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

	}

	const int adapted_max_city_history = file->get_extended_version() < 12 ? MAX_CITY_HISTORY + 1 : MAX_CITY_HISTORY;

	// we probably need to load/save the city history
	if (file->get_version() < 86000)
	{
		//DBG_DEBUG("stadt_t::rdwr()", "is old version: No history!");
	}
	else if(file->get_version() < 99016)
	{
		// 86.00.0 introduced city history
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++)
		{
			file->rdwr_longlong(city_history_year[year][0]);
			file->rdwr_longlong(city_history_year[year][3]);
			file->rdwr_longlong(city_history_year[year][4]);
			for (uint hist_type = 6; hist_type < 9; hist_type++)
			{
				if(hist_type == HIST_PAS_WALKED)
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
					city_history_year[year][hist_type] = 0;
					continue;
				}
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++)
		{
			file->rdwr_longlong(city_history_month[month][0]);
			file->rdwr_longlong(city_history_month[month][3]);
			file->rdwr_longlong(city_history_month[month][4]);
			for (uint hist_type = 6; hist_type < 9; hist_type++)
			{
				if(hist_type == HIST_PAS_WALKED)
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
					city_history_month[month][hist_type] = 0;
					continue;
				}
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
	else if (file->get_extended_version() == 0)
	{
		// 99.17.0 extended city history
		// Extended version 3 extended it further, so skip the last step.
		// For extended versions *before* 3, power history was treated as congestion
		// (they are now separate), so that must be handled differently.
		if (file->get_version() <= 120000)
		{
			for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++)
			{
				for (uint hist_type = 0; hist_type < 14; hist_type++)
				{
					if (hist_type == HIST_PAS_WALKED || hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND)
					{
						// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
						//city_history_year[year][hist_type] = 0;
						continue;
					}
					file->rdwr_longlong(city_history_year[year][hist_type]);
				}
			}
			for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++)
			{
				for (uint hist_type = 0; hist_type < 14; hist_type++)
				{
					if (hist_type == HIST_PAS_WALKED || hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND)
					{
						// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
						//city_history_month[month][hist_type] = 0;
						continue;
					}
					file->rdwr_longlong(city_history_month[month][hist_type]);
				}
			}
			// save button settings for this town
			file->rdwr_long(stadtinfo_options);
		}
		else
		{
			// 120,001 with walking (direct connections) recored seperately
			sint64 dummy_hist_mail_walked = 0L;
			for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++)
			{
				for (uint hist_type = 0; hist_type < 14; hist_type++)
				{
					switch (hist_type)
					{
						case HIST_JOBS:
						case HIST_VISITOR_DEMAND:
							// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
							//city_history_year[year][hist_type] = 0;
							continue;

						case HIST_MAIL_GENERATED:
							file->rdwr_longlong(dummy_hist_mail_walked);
							break;
					}
					file->rdwr_longlong(city_history_year[year][hist_type]);
				}
			}
			for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++)
			{
				for (uint hist_type = 0; hist_type < 14; hist_type++)
				{
					switch (hist_type)
					{
						case HIST_JOBS:
						case HIST_VISITOR_DEMAND:
							// Versions earlier than 111.1 Ex 10.8 did not record walking passengers, and versions earlier than 12 did not record jobs or visitor demand.
							//city_history_year[year][hist_type] = 0;
							continue;

						case HIST_MAIL_GENERATED:
							file->rdwr_longlong(dummy_hist_mail_walked);
							break;
					}
					file->rdwr_longlong(city_history_year[month][hist_type]);
				}
			}
			// save button settings for this town
			file->rdwr_long(stadtinfo_options);
		}
	}
	else if(file->get_extended_version() > 0 && file->get_extended_version() < 3)
	{
		// Move congestion history to the correct place (shares with power received).

		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++)
		{
			for (uint hist_type = 0; hist_type < adapted_max_city_history - 2; hist_type++)
			{
				if(hist_type == HIST_POWER_RECIEVED)
				{
					city_history_year[year][HIST_POWER_RECIEVED] = 0;
					hist_type = HIST_CONGESTION;
				}
				if(hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND)
				{
					city_history_year[year][hist_type] = 0;
				}
				if(hist_type == HIST_PAS_WALKED)
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers.
					city_history_year[year][hist_type] = 0;
					continue;
				}
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++)
		{
			for (uint hist_type = 0; hist_type < adapted_max_city_history - 2; hist_type++)
			{
				if(hist_type == HIST_POWER_RECIEVED)
				{
					city_history_month[month][HIST_POWER_RECIEVED] = 0;
					hist_type = HIST_CONGESTION;
				}
				if(hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND)
				{
					city_history_month[month][hist_type] = 0;
				}
				if(hist_type == HIST_PAS_WALKED)
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers.
					city_history_month[month][hist_type] = 0;
					continue;
				}
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long(stadtinfo_options);
	}
	else if(file->get_extended_version() >= 3)
	{
		for(uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++)
		{
			for(uint hist_type = 0; hist_type < adapted_max_city_history; hist_type++)
			{
				if(hist_type == HIST_PAS_WALKED && (file->get_extended_version() < 10 || file->get_version() < 111001))
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers.
					city_history_year[year][hist_type] = 0;
					continue;
				}

				if(file->get_extended_version() < 12 && (hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND))
				{
					city_history_year[year][hist_type] = 0;
					continue;
				}

				if(file->get_extended_version() < 12 && (hist_type == LEGACY_HIST_CAR_OWNERSHIP))
				{
					sint64 car_ownership_history = welt->get_finance_history_year(0, karte_t::WORLD_CAR_OWNERSHIP);
					file->rdwr_longlong(car_ownership_history);
					welt->set_car_ownership_history_year(year, car_ownership_history);
					city_history_year[year][hist_type] = 0;
					continue;
				}

				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for(uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++)
		{
			for(uint hist_type = 0; hist_type < adapted_max_city_history; hist_type++)
			{
				if(hist_type == HIST_PAS_WALKED && (file->get_extended_version() < 10 || file->get_version() < 111001))
				{
					// Versions earlier than 111.1 Ex 10.8 did not record walking passengers.
					city_history_month[month][hist_type] = 0;
					continue;
				}
				if(file->get_extended_version() < 12 && (hist_type == HIST_JOBS || hist_type == HIST_VISITOR_DEMAND))
				{
					city_history_month[month][hist_type] = 0;
					continue;
				}

				if(file->get_extended_version() < 12 && (hist_type == LEGACY_HIST_CAR_OWNERSHIP))
				{
					sint64 car_ownership_history = welt->get_finance_history_month(0, karte_t::WORLD_CAR_OWNERSHIP);
					file->rdwr_longlong(car_ownership_history);
					welt->set_car_ownership_history_month(month, car_ownership_history);
					city_history_month[month][hist_type] = 0;
					continue;
				}

				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long(stadtinfo_options);
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 26)
	{
		// This load/save block used to be further down, but has been mvoed here
		// because it is necessary to load outgoing_private_cars before the city
		// growth factors.

		file->rdwr_bool(check_road_connexions);

		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(outgoing_private_cars);
		file->rdwr_long(incoming_private_cars);
	}

	if(file->is_loading())
	{
		// These will be set later when buildings are added.
		city_history_month[0][HIST_CITICENS] = 0;
		city_history_year[0][HIST_CITICENS] = 0;
	}

	// differential history
	if (file->get_version() <= 120000 || (file->get_extended_version() > 0 && file->get_extended_version() < 25)) {
		if (file->is_loading()) {
			// Initalize differential statistics assuming a differential of 0.
			city_growth_get_factors(city_growth_factor_previous, 0);
		}
	}
	else if (file->get_extended_version() == 0) {
		if (file->is_loading()) {
			// Initalize differential statistics assuming a differential of 0.
			city_growth_get_factors(city_growth_factor_previous, 0);
		}

		// load/save differential statistics.
		for (uint32 i = 0; i < 3; i++) {
			file->rdwr_longlong(city_growth_factor_previous[i].demand);
			file->rdwr_longlong(city_growth_factor_previous[i].supplied);
		}
	}
	else {
		// load/save differential statistics.
		for (uint32 i = 0; i < GROWTH_FACTOR_NUMBER; i++) {
			file->rdwr_longlong(city_growth_factor_previous[i].demand);
			file->rdwr_longlong(city_growth_factor_previous[i].supplied);
		}
	}

	if(file->get_version()>99014  &&  file->get_version()<99016) {
		sint32 dummy = 0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}

	// since 102.2 there are static cities
	if(file->get_version()>102001 ) {
		file->rdwr_bool(allow_citygrowth);
	}
	else if(  file->is_loading()  ) {
		allow_citygrowth = true;
	}
	// save townhall road position
	if(file->get_version()>102002 && file->get_extended_version() != 7 ) {
		townhall_road.rdwr(file);
	}
	else if(  file->is_loading()  ) {
		townhall_road = koord::invalid;
	}

	if(file->get_version() >= 110005 && file->get_extended_version() < 12)
	{
		// Old "factory_entry_t" code - deprecated, but must skip to the correct
		// position in old saved game files. NOTE: There is *no* way to save in
		// a version compatible with older saved games with the factory entry
		// code stripped out.
		uint32 entry_count = 0;
		for(int i = 0; i < 2; i ++)
		{
			// This must be done twice, as the routine was
			// called once for mail and once for passengers.
			file->rdwr_long(entry_count);
			if(file->is_loading())
			{
				for(uint32 e = 0; e < entry_count; ++e)
				{
					koord factory_pos = koord::invalid;
					factory_pos.rdwr(file);
					uint32 dummy = 0;
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
				}
			}
			uint32 total_generated = 0;
			file->rdwr_long(total_generated);
		}
	}

	// data related to target factories
	//target_factories_pax.rdwr( file );
	//target_factories_mail.rdwr( file );

	if(file->get_extended_version() >=9 && file->get_version() >= 110000 && file->get_extended_version() < 13 && file->get_extended_revision() < 26)
	{
		// This load/save block has been moved upwards because it is necessary to load outgoing_private_cars before setting the growth factors, which is done above.
		file->rdwr_bool(check_road_connexions);
		if(file->get_extended_version() < 11)
		{
			// Was private_car_update_month
			uint8 dummy;
			file->rdwr_byte(dummy);
		}

		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(outgoing_private_cars);
		file->rdwr_long(incoming_private_cars);
	}

	if(file->is_saving() && file->get_extended_version() >=9 && file->get_version() >= 110000)
	{
		uint32 time;
		koord k;
		uint32 count;

		count = connected_cities.get_count();
		file->rdwr_long(count);
		FOR(connexion_map, const& city_iter, connected_cities)
		{
			time = city_iter.value;
			if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
			{
				file->rdwr_long(time);
			}
			else
			{
				uint16 t;
				if (time == UINT32_MAX_VALUE)
				{
					t = 65535;
				}
				else
				{
					t = (uint16)time;
				}
				file->rdwr_short(t);
			}
			k = city_iter.key;
			k.rdwr(file);
		}

		count = connected_industries.get_count();
		file->rdwr_long(count);
		FOR(connexion_map, const& industry_iter, connected_industries)
		{
			time = industry_iter.value;
			if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
			{
				file->rdwr_long(time);
			}
			else
			{
				uint16 t;
				if (time == UINT32_MAX_VALUE)
				{
					t = 65535;
				}
				else
				{
					t = (uint16)time;
				}
				file->rdwr_short(t);
			}
			k = industry_iter.key;
			k.rdwr(file);
		}

		count = connected_attractions.get_count();
		file->rdwr_long(count);
		FOR(connexion_map, const& attraction_iter, connected_attractions)
		{
			time = attraction_iter.value;
			if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
			{
				file->rdwr_long(time);
			}
			else
			{
				uint16 t;
				if (time == UINT32_MAX_VALUE)
				{
					t = 65535;
				}
				else
				{
					t = (uint16)time;
				}
				file->rdwr_short(t);
			}
			k = attraction_iter.key;
			k.rdwr(file);
		}

		file->rdwr_long(number_of_cars);
	}

	if(file->is_loading())
	{
		// We have to be rather careful about this.  City borders are no longer strictly determined
		// by building layout, they are their own thing.  But when loading old files, shrink to fit...
		if (file->get_extended_version() < 12) {
			// 08-Jan-03: Due to some bugs in the special buildings/town hall
			// placement code, li,re,ob,un could've gotten irregular values
			// If a game is loaded, the game might suffer from such an mistake
			// and we need to correct it here.
			DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);

			reset_city_borders();
		}

		connected_cities.clear();
		connected_industries.clear();
		connected_attractions.clear();
		if(file->get_extended_version() >=9 && file->get_version() >= 110000)
		{
			uint32 time;
			koord k;
			uint32 count;

			// Cities
			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
				{
					file->rdwr_long(time);
				}
				else
				{
					uint16 t;
					file->rdwr_short(t);
					if (t == 65535)
					{
						time = UINT32_MAX_VALUE;
					}
					else
					{
						time = (uint32)t;
					}
				}

				k.rdwr(file);
				connected_cities.put(k, time);
			}

			// Industries

			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
				{
					file->rdwr_long(time);
				}
				else
				{
					uint16 t;
					file->rdwr_short(t);
					if (t == 65535)
					{
						time = UINT32_MAX_VALUE;
					}
					else
					{
						time = (uint32)t;
					}
				}
				k.rdwr(file);
				connected_industries.put(k, time);
			}

			// Attractions

			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
				{
					file->rdwr_long(time);
				}
				else
				{
					uint16 t;
					file->rdwr_short(t);
					if (t == 65535)
					{
						time = UINT32_MAX_VALUE;
					}
					else
					{
						time = (uint32)t;
					}
				}
				k.rdwr(file);
				connected_attractions.put(k, time);
			}

			file->rdwr_long(number_of_cars);
		}

		else
		{
			check_road_connexions = false;
		}
	}

	if(file->get_extended_version() >= 12 && file->get_extended_version() < 13)
	{
		// Was waschtum
		uint32 dummy;
		file->rdwr_long(dummy);
	}

	if(file->is_loading())
	{
		// These are recalculated when the buildings are added to the city, so reset these to avoid
		// doubling these figures for every load/save cycle.
		city_history_month[0][HIST_CITICENS] = 0;
		city_history_year[0][HIST_CITICENS] = 0;

		city_history_month[0][HIST_JOBS] = 0;
		city_history_year[0][HIST_JOBS] = 0;

		city_history_month[0][HIST_VISITOR_DEMAND] = 0;
		city_history_year[0][HIST_VISITOR_DEMAND] = 0;
	}
}


/**
 * Wird am Ende der Laderoutine aufgerufen, wenn die Welt geladen ist
 * und nur noch die Datenstrukturenneu verknuepft werden muessen.
 * @author Hj. Malthaner
 */
void stadt_t::finish_rd()
{
	//step_count = 0;
	//next_step = 0;
	next_growth_step = 0;

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
	//init_pax_destinations();

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
	reset_city_borders();

	//next_step = 0;
	next_growth_step = 0;

	if(check_road_connexions)
	{
		welt->add_queued_city(this);
	}
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
	sparse_tpl<uint8> pax_destinations_temp(koord(welt->get_size().x, welt->get_size().y));

	uint8 color;
	koord pos;
	for( uint16 i = 0; i < pax_destinations_new.get_data_count(); i++ ) {
		pax_destinations_new.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set(pos.y, pos.x, color);
	}
	swap<uint8>( pax_destinations_temp, pax_destinations_new );

	pax_destinations_temp.clear();
	for( uint16 i = 0; i < pax_destinations_old.get_data_count(); i++ ) {
		pax_destinations_old.get_nonzero(i, pos, color);
		assert( color != 0 );
		pax_destinations_temp.set(pos.y, pos.x, color);
	}
	pax_destinations_new_change ++;
	swap<uint8>( pax_destinations_temp, pax_destinations_old );

	vector_tpl<koord> k_list(connected_cities.get_count());
	vector_tpl<uint16> f_list(connected_cities.get_count());

	for (connexion_map::iterator iter = connected_cities.begin(); iter != connected_cities.end(); )
	{
		koord k = iter->key;
		uint16 f = iter->value;
		iter = connected_cities.erase(iter);
		k.rotate90(y_size);
		if(connected_cities.is_contained(k))
		{
			uint16 f_2 = connected_cities.remove(k);
			koord k_2 = k;
			k_2.rotate90(y_size);
			assert(k_2 != koord::invalid);
			k_list.append(k_2);
			f_list.append(f_2);
		}
		assert(k != koord::invalid);
		k_list.append(k);
		f_list.append(f);
	}
	connected_cities.clear();

	for(uint32 j = 0; j < k_list.get_count(); j ++)
	{
		connected_cities.put(k_list[j], f_list[j]);
	}

	k_list.clear();
	f_list.clear();
	for (connexion_map::iterator iter = connected_industries.begin(); iter != connected_industries.end(); )
	{
		koord k = iter->key;
		uint16 f = iter->value;
		iter = connected_industries.erase(iter);
		k.rotate90(y_size);
		if(connected_industries.is_contained(k))
		{
			uint16 f_2 = connected_industries.remove(k);
			koord k_2 = k;
			k_2.rotate90(y_size);
			assert(k_2 != koord::invalid);
			k_list.append(k_2);
			f_list.append(f_2);
		}
		assert(k != koord::invalid);
		k_list.append(k);
		f_list.append(f);
	}
	connected_industries.clear();

	for(uint32 m = 0; m < k_list.get_count(); m ++)
	{
		connected_industries.put(k_list[m], f_list[m]);
	}

	k_list.clear();
	f_list.clear();
	for (connexion_map::iterator iter = connected_attractions.begin(); iter != connected_attractions.end(); )
	{
		koord k = iter->key;
		uint16 f = iter->value;
		iter = connected_attractions.erase(iter);
		k.rotate90(y_size);
		if(connected_attractions.is_contained(k))
		{
			uint16 f_2 = connected_attractions.remove(k);
			koord k_2 = k;
			k_2.rotate90(y_size);
			assert(k_2 != koord::invalid);
			k_list.append(k_2);
			f_list.append(f_2);
		}
		assert(k != koord::invalid);
		k_list.append(k);
		f_list.append(f);
	}
	connected_attractions.clear();

	for(uint32 n = 0; n < k_list.get_count(); n ++)
	{
		connected_attractions.put(k_list[n], f_list[n]);
	}
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


/* show city info dialogue
 * @author prissi
 */
void stadt_t::show_info()
{
	create_win( new city_info_t(this), w_info, (ptrdiff_t)this );
}

/* change size of city
 * @author prissi */
void stadt_t::change_size( sint64 delta_citizen, bool new_town, bool map_generation)
{
	DBG_MESSAGE("stadt_t::change_size()", "%i + %i", bev, delta_citizen);
	if(  delta_citizen > 0  ) {
		unsupplied_city_growth += delta_citizen * CITYGROWTH_PER_CITICEN;
		step_grow_city(new_town, map_generation);
	}
	if(  delta_citizen < 0  ) {
		if(  bev > -delta_citizen  ) {
			bev += (sint32)delta_citizen;
		}
		else {
//				remove_city();
			bev = 1;
		}
		step_grow_city(new_town, map_generation);
	}
	DBG_MESSAGE("stadt_t::change_size()", "%i+%i", bev, delta_citizen);
}


void stadt_t::step(uint32 delta_t)
{
	// is it time for the next step?
	next_growth_step += delta_t;

	while(stadt_t::city_growth_step < next_growth_step) {
		calc_growth();
		step_grow_city();
		next_growth_step -= stadt_t::city_growth_step;
	}

	// update history (might be changed due to construction/destroying of houses)

	city_history_month[0][HIST_GROWTH] = city_history_month[0][HIST_CITICENS] - city_history_month[1][HIST_CITICENS];	// growth
	city_history_year[0][HIST_GROWTH] = city_history_year[0][HIST_CITICENS] - city_history_year[1][HIST_CITICENS];

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
	for (int hist_type = HIST_GROWTH; hist_type < MAX_CITY_HISTORY; hist_type++) {
		city_history_month[0][hist_type] = 0;
	}

	city_history_month[0][HIST_BUILDING] = buildings.get_count();
	city_history_month[0][HIST_GOODS_NEEDED] = 0;

	// Congestion figures for the year should be an average of the last 12 months.
	uint16 total_congestion = 0;
	for(int i = 0; i < 12; i ++)
	{
		total_congestion += city_history_month[i][HIST_CONGESTION];
	}

	city_history_year[0][HIST_CONGESTION] = total_congestion / 12;

	// need to roll year too?
	if (welt->get_last_month() == 0)
	{
		for (int i = MAX_CITY_HISTORY_YEARS - 1; i > 0; i--)
		{
			for (int hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++)
			{
				city_history_year[i][hist_type] = city_history_year[i - 1][hist_type];
			}
		}
		// init this year
		for (int hist_type = HIST_GROWTH; hist_type < MAX_CITY_HISTORY; hist_type++)
		{
			city_history_year[0][hist_type] = 0;
		}
		city_history_year[0][HIST_BUILDING] = buildings.get_count();
		city_history_year[0][HIST_GOODS_NEEDED] = 0;

		for(weighted_vector_tpl<gebaeude_t *>::const_iterator i = buildings.begin(), end = buildings.end(); i != end; ++i)
		{
			(*i)->new_year();
		}

	}
	outgoing_private_cars = 0;
}

void stadt_t::check_all_private_car_routes()
{
	const planquadrat_t* plan = welt->access(townhall_road);
	if(plan->get_city() != this)
	{
		// This sometimes happens shortly after the map rotating. Return here to avoid crashing.
		dbg->error("void stadt_t::check_all_private_car_routes()", "Townhall road does not register as being in its origin city - cannot check private car routes");
		return;
	}
	const uint32 depth = welt->get_max_road_check_depth();
	const grund_t* gr = plan->get_kartenboden();
	const koord3d origin = gr ? gr->get_pos() : koord3d::invalid;

	connected_cities.clear();
	connected_industries.clear();
	connected_attractions.clear();

	// This will find the fastest route from the townhall road to *all* other townhall roads.
	route_t private_car_route;
	road_vehicle_t checker;
	private_car_destination_finder_t finder(welt, &checker, this);
	private_car_route.find_route(welt, origin, &finder, welt->get_citycar_speed_average(), ribi_t::all, 1, 1, 1, depth, false, route_t::private_car_checker);

	check_road_connexions = false;
}

void stadt_t::calc_traffic_level()
{
	settings_t const& s = welt->get_settings();

	// We need to calculate the traffic level here, as this determines the vehicle occupancy, which is necessary for the calculation of congestion.
	// Manual assignment of traffic level modifiers, since I could not find a suitable mathematical formula.
	switch(s.get_traffic_level())
	{
	case 0:
		traffic_level = 0;
		break;

	case 1:
		traffic_level = 5;
		break;

	case 2:
		traffic_level = 10;
		break;

	case 3:
		traffic_level = 20;
		break;

	case 4:
		traffic_level = 25;
		break;

	case 5:
		traffic_level = 50;
		break;

	case 6:
		traffic_level = 100;
		break;

	case 7:
		traffic_level = 200;
		break;

	case 8:
		traffic_level = 250;
		break;

	case 9:
		traffic_level = 333;
		break;

	case 10:
		traffic_level = 500;
		break;

	case 11:
		traffic_level = 613; // Average car occupancy of 1.6 = 0.6125.
		break;

	case 12:
		traffic_level = 667;
		break;

	case 13:
		traffic_level = 750;
		break;

	case 14:
		traffic_level = 833;
		break;

	case 15:
		traffic_level = 909;
		break;

	case 16:
	default:
		traffic_level = 1000;
	};
}

void stadt_t::new_month(bool check) //"New month" (Google)
{
	swap<uint8>( pax_destinations_old, pax_destinations_new );
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;

	if(bev == 0)
	{
		bev ++;
	}

	roll_history();

	calc_traffic_level();

	// Calculate the level of congestion.
	// Used in determining growth and passenger preferences.
	// Old system:
	// From observations in game: anything < 2, not very congested.
	// Anything > 4, very congested.
	// For new system, see http://www.tomtom.com/lib/doc/congestionindex/2013-0322-TomTom-CongestionIndex-2012-Annual-EUR-mi.pdf
	// @author: jamespetts

	settings_t const& s = welt->get_settings();

	uint16 congestion_density_factor = s.get_congestion_density_factor();

	if(congestion_density_factor < 32)
	{
		// Old method - congestion density factor
		const uint32 city_size = (ur.x - lo.x + 1) * (ur.y - lo.y + 1);
		uint32 cars_per_tile_thousandths = (city_history_month[1][HIST_CITYCARS] * 1000) / city_size;
		const uint32 population_density = (city_history_month[1][HIST_CITICENS] * 10) / city_size;
		congestion_density_factor *= 100;

		uint32 cars_per_tile_base = 800;

		cars_per_tile_thousandths = welt->calc_adjusted_monthly_figure(cars_per_tile_thousandths);
		cars_per_tile_base = welt->calc_adjusted_monthly_figure(cars_per_tile_base);
		congestion_density_factor = welt->calc_adjusted_monthly_figure(congestion_density_factor);

		uint32 congestion = 0;
		if(cars_per_tile_thousandths > cars_per_tile_base)
		{
			if(congestion_density_factor == 0)
			{
				congestion = (cars_per_tile_thousandths -= cars_per_tile_base) / 30;
			}
			else
			{
				congestion = (((cars_per_tile_thousandths -= cars_per_tile_base) / 45) * population_density) / congestion_density_factor;
			}
		}
		city_history_month[0][HIST_CONGESTION] = congestion;
	}

	else // Congestion density factor > 32:  new system
	{
		// Based on TomTom congestion index system
		// See http://www.tomtom.com/lib/doc/congestionindex/2013-0322-TomTom-CongestionIndex-2012-Annual-EUR-mi.pdf

		// First - check the length of the road network in the city.
		uint32 road_tiles = 0;
		for(sint16 j = lo.y; j <= ur.y; ++j)
		{
			for(sint16 i = lo.x; i <= ur.x; ++i)
			{
				const koord k(i, j);
				const grund_t *const gr = welt->lookup_kartenboden(k);
				if(gr && gr->get_weg(road_wt))
				{
					road_tiles ++;
				}
			}
		}
		uint32 road_hectometers = (road_tiles * (uint32)s.get_meters_per_tile()) / 10;
		if (road_hectometers == 0) {
			// Avoid divide by zero errors
			road_hectometers = 1;
		}

		// Second - get the number of car trips per hour
		const sint64 seconds_per_month = welt->ticks_to_seconds(welt->ticks_per_world_month);
		// Add incoming private cars as these are no longer tracked in the city history statistics to make them clearer.
		const sint64 trips_per_hour = ((city_history_month[1][HIST_CITYCARS] + incoming_private_cars) * 3600l) / seconds_per_month;

		// Third - combine the information, multiplying by a ratio based on
		// congestion_density_factor == 141 is the ideal factor based on the 2012 TomTom congestion index for British cities
		// (Average: range is 70 (London) to 227 (Newcastle/Sunderland).
		// Further reduce this by the traffic_level factor to adjust for occupancy rates (permille).
		const sint64 adjusted_ratio = ((sint64)traffic_level * congestion_density_factor) / 1000l;
		city_history_month[0][HIST_CONGESTION] = (trips_per_hour * adjusted_ratio) / (sint64)road_hectometers;
	}

	// Clearing these will force recalculation as necessary.
	// Cannot do this too often, as it severely impacts on performance.
	if(check)
	{
		check_road_connexions = true;
	}

	incoming_private_cars = 0;
}

void stadt_t::calc_growth()
{
	// now iterate over all factories to get the ratio of producing version non-producing factories
	// we use the incoming storage as a measure and we will only look for end consumers (power stations, markets)

	FOR(const vector_tpl<fabrik_t*>, const& fab, welt->get_fab_list())
	{
		if(fab && fab->get_city() == this && fab->get_lieferziele().empty() && !fab->get_suppliers().empty())
		{
			// consumer => check for it storage
			const factory_desc_t *const desc = fab->get_desc();
			for(  int i=0;  i<desc->get_supplier_count();  i++  )
			{
				city_history_month[0][HIST_GOODS_NEEDED] ++;
				city_history_year[0][HIST_GOODS_NEEDED] ++;
				if(  fab->input_vorrat_an( desc->get_supplier(i)->get_input_type() )>0  )
				{
					city_history_month[0][HIST_GOODS_RECIEVED] ++;
					city_history_year[0][HIST_GOODS_RECIEVED] ++;
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

	settings_t const& s = welt->get_settings();

	// smaller towns should grow slower to have villages for a longer time
	sint32 const weight_factor =
		get_city_population() < s.get_city_threshold_size() ? welt->get_settings().get_growthfactor_small()  :
		get_city_population() < s.get_capital_threshold_size() ? welt->get_settings().get_growthfactor_medium() :
		welt->get_settings().get_growthfactor_large();

	// now compute the growth for this step
	sint32 growth_factor = weight_factor > 0 ? total_supply_percentage / weight_factor : 0;

	// Congestion adversely impacts on growth. At 100% congestion, there will be no growth.
	if(city_history_month[0][HIST_CONGESTION] > 0)
	{
		const uint32 congestion_factor = city_history_month[0][HIST_CONGESTION];
		growth_factor -= (congestion_factor * growth_factor) / 100;
	}

	// Scale up growth to have a larger fractional component. This allows small growth units to accumulate in the case of long months.
	sint64 new_unsupplied_city_growth = (sint64)growth_factor * (CITYGROWTH_PER_CITICEN / 16ll);

	// Growth is scaled down by month length.
	// The result is that ~ the same monthly growth will occur independent of month length.
	new_unsupplied_city_growth = welt->inverse_scale_with_month_length( new_unsupplied_city_growth );

	// Add the computed growth to the growth accumulator.
	// Future growth scale factors can be applied here.
	unsupplied_city_growth += new_unsupplied_city_growth;
}

void stadt_t::city_growth_get_factors(city_growth_factor_t(&factors)[GROWTH_FACTOR_NUMBER], uint32 const month) const
{
	// optimize view of history for convenience
	sint64 const (&h)[MAX_CITY_HISTORY] = city_history_month[month];

	// go through each index one at a time
	uint32 index = 0;

	// passenger growth factors
	factors[index].demand = h[HIST_PAS_GENERATED];
	factors[index++].supplied = h[HIST_PAS_TRANSPORTED] + h[HIST_PAS_WALKED] + (h[HIST_CITYCARS] - outgoing_private_cars);

	// mail growth factors
	factors[index].demand = h[HIST_MAIL_GENERATED];
	factors[index++].supplied = h[HIST_MAIL_TRANSPORTED];

	// goods growth factors
	factors[index].demand = h[HIST_GOODS_NEEDED];
	factors[index++].supplied = h[HIST_GOODS_RECIEVED];

	// Electricity growth factors
	factors[index].demand = h[HIST_POWER_NEEDED];
	factors[index++].supplied = h[HIST_POWER_RECIEVED];
}


sint32 stadt_t::city_growth_base(uint32 const rprec, uint32 const cprec)
{
	// Resolve constant references.
	settings_t const & s = welt->get_settings();
	const sint32 electricity_multiplier = s.get_electricity_multiplier();
	const sint32 electricity_proportion = (get_electricity_consumption(welt->get_timeline_year_month()) * electricity_multiplier / 100);
	sint32 const weight[GROWTH_FACTOR_NUMBER] = { s.get_passenger_multiplier(), s.get_mail_multiplier(),
	    s.get_goods_multiplier(), electricity_proportion };

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
		sint64 const had = growthfactors[i].demand - city_growth_factor_previous[i].demand;
		city_growth_factor_previous[i].demand = growthfactors[i].demand;
		sint64 const got = growthfactors[i].supplied - city_growth_factor_previous[i].supplied;
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
		sint64 const had = growthfactors[i].demand - city_growth_factor_previous[i].demand;
		city_growth_factor_previous[i].demand = -had;
		sint64 const got = growthfactors[i].supplied - city_growth_factor_previous[i].supplied;
		city_growth_factor_previous[i].supplied = -got;
	}
}


// does constructions ...
void stadt_t::step_grow_city(bool new_town, bool map_generation)
{
	// Do not try to expand further after we failed.
	bool failure = false;

	// Try harder to build if this is a new town
	int num_tries = new_town ? 60 : 30;

	// since we use internally a finer value ...
	const sint64 growth_steps = unsupplied_city_growth / CITYGROWTH_PER_CITICEN;
	if(  growth_steps > 0  ) {
		unsupplied_city_growth %= CITYGROWTH_PER_CITICEN;
	}

	// Hajo: let city grow in steps of 1
	// @author prissi: No growth without development
	for(  sint64 n = 0;  n < growth_steps;  n++  ) {
		bev++;

		if (!failure) {
			int i;

			for (i = 0; i < num_tries && bev * 2 > won + arb + 100; i++) {
				build(new_town, map_generation);
			}

			failure = i == num_tries;
		}

		check_bau_spezial(new_town);
		check_bau_townhall(new_town);
		check_bau_factory(new_town); // add industry? (not during creation)
		INT_CHECK("simcity 2241");
	}
}


uint32 stadt_t::check_road_connexion_to(stadt_t* city) const
{
	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		const uint16 journey_time_per_tile = city == this ? welt->get_generic_road_time_per_tile_city() : welt->get_generic_road_time_per_tile_intercity();
		// With this setting, we add congestion factoring at a later stage.
		return journey_time_per_tile;
	}

	if(connected_cities.is_contained(city->get_pos()))
	{
		const uint16 journey_time_per_tile = connected_cities.get(city->get_pos());
		if(city != this || journey_time_per_tile < UINT32_MAX_VALUE)
		{
			return journey_time_per_tile;
		}
	}

	if(city == this)
	{
		// Should always be possible to travel within the city.
		const grund_t* gr = welt->lookup_kartenboden(townhall_road);
		const weg_t* road = gr ? gr->get_weg(road_wt) : NULL;
		const uint32 journey_time_per_tile = road ? road->get_desc() == welt->get_city_road() ? welt->get_generic_road_time_per_tile_city() : welt->calc_generic_road_time_per_tile(road->get_desc()) : welt->get_generic_road_time_per_tile_city();
		// The below is not network safe.
		//connected_cities.put(pos, journey_time_per_tile);
		return journey_time_per_tile;
	}
	else
	{
		return UINT32_MAX_VALUE;
	}
}

uint32 stadt_t::check_road_connexion_to(const fabrik_t* industry) const
{
	stadt_t* city = industry->get_city();

	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		// With this setting, we add congestion factoring at a later stage.
		return city && city == this ? welt->get_generic_road_time_per_tile_city() : welt->get_generic_road_time_per_tile_intercity();
	}

	if(connected_industries.is_contained(industry->get_pos().get_2d()))
	{
		return connected_industries.get(industry->get_pos().get_2d());
	}

	if(city)
	{
		// If an industry is in a city, presume that it is connected
		// if the city is connected. Do not presume the converse.
		const uint32 time_to_city = check_road_connexion_to(city);
		if(time_to_city < UINT32_MAX_VALUE)
		{
			return time_to_city;
		}
	}

	return UINT32_MAX_VALUE;
}


uint32 stadt_t::check_road_connexion_to(const gebaeude_t* attraction) const
{
	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		// 1km/h = 0.06 minutes per meter. To get 100ths of minutes per meter, therefore, multiply the speed by 6.
		// With this setting, we add congestion factoring at a later stage.
		return welt->get_generic_road_time_per_tile_intercity() * 6;
	}

	const koord pos = attraction->get_pos().get_2d();
	if(connected_attractions.is_contained(pos))
	{
		return connected_attractions.get(pos);
	}
	// If this attraction is in a city, assume it to be connected to the same extent as the rest of the city.
	else if (attraction->get_stadt())
	{
		return check_road_connexion_to(attraction->get_stadt());
	}
	/*else
	{
		// get_city() is very CPU intensive, so call it only if necessary
		stadt_t* city = welt->get_city(pos);
		if (city)
		{
			return check_road_connexion_to(city);
		}
	}*/

	return UINT32_MAX_VALUE;
}

void stadt_t::add_road_connexion(uint32 journey_time_per_tile, const stadt_t* city)
{
	connected_cities.set(city->get_pos(), journey_time_per_tile);
}

void stadt_t::add_road_connexion(uint32 journey_time_per_tile, const fabrik_t* industry)
{
	connected_industries.set(industry->get_pos().get_2d(), journey_time_per_tile);
}

void stadt_t::add_road_connexion(uint32 journey_time_per_tile, const gebaeude_t* attraction)
{
	const koord3d attraction_pos = attraction->get_pos();
	connected_attractions.set(attraction_pos.get_2d(), journey_time_per_tile);

	// Add all tiles of an attraction here.
	if(!attraction->get_tile() || attraction_pos == koord3d::invalid)
	{
		return;
	}
	const building_desc_t *bdsc = attraction->get_tile()->get_desc();
	const koord attraction_size = bdsc->get_size(attraction->get_tile()->get_layout());
	koord k;

	for(k.y = 0; k.y < attraction_size.y; k.y ++)
	{
		for(k.x = 0; k.x < attraction_size.x; k.x ++)
		{
			koord3d k_3d = koord3d(k, 0) + attraction_pos;
			grund_t *gr = welt->lookup(k_3d);
			if(gr)
			{
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if(gb_part && gb_part->get_tile()->get_desc() == bdsc)
				{
					connected_attractions.set(gb_part->get_pos().get_2d(), journey_time_per_tile);
				}
			}
		}
	}
}

void stadt_t::set_private_car_trip(int passengers, stadt_t* destination_town)
{
	if(destination_town == NULL || (destination_town->get_pos().x == pos.x && destination_town->get_pos().y == pos.y))
	{
		// Destination town is not set - so going to a factory or tourist attraction,
		// or origin and destination towns are the same.
		// Count as a local or incoming trip
		city_history_year[0][HIST_CITYCARS] += passengers;
		city_history_month[0][HIST_CITYCARS] += passengers;
	}
	else
	{
		// Inter-city trip
		city_history_year[0][HIST_CITYCARS] += passengers;
		city_history_month[0][HIST_CITYCARS] += passengers;

		// Also add private car trips to the *destination*.
		destination_town->set_private_car_trips(passengers);

		// And mark the trip as outgoing for growth calculations
		// (Now deprecated)
		outgoing_private_cars += passengers;
	}
	welt->buche(passengers, karte_t::WORLD_CITYCARS);
}


/**
 * returns a random and uniformly distributed point within city borders
 * @author Hj. Malthaner
 */
koord stadt_t::get_zufallspunkt(uint32 min_distance, uint32 max_distance, koord origin) const
{
	if(!buildings.empty())
	{
		if(origin == koord::invalid)
		{
			origin = this->get_pos();
		}
		koord nearest_miss = koord::invalid;
		uint32 nearest_miss_difference = 2147483647; // uint32 max.
		for(uint32 i = 0; i < 24; i++)
		{
			gebaeude_t* const gb = pick_any_weighted(buildings);

			koord k = gb->get_pos().get_2d();
			if(!welt->is_within_limits(k))
			{
				// this building should not be in this list, since it has been already deleted!
				dbg->error("stadt_t::get_zufallspunkt()", "illegal building in city list of %s: %p removing!", this->get_name(), gb);
				const_cast<stadt_t*>(this)->buildings.remove(gb);
				welt->remove_building_from_world_list(gb);
				k = koord(0, 0);
			}
			const uint32 distance = shortest_distance(k, origin);
			if(distance <= max_distance && distance >= min_distance)
			{
				return k;
			}

			uint32 difference;
			if(distance > max_distance)
			{
				difference = distance - max_distance;
			}
			else // distance < min_distance
			{
				difference = min_distance - distance;
			}
			if(difference < nearest_miss_difference)
			{
				nearest_miss_difference = difference;
				nearest_miss = k;
			}
		}
		return nearest_miss;
	}
	// might happen on slow computers during creation of new cities or start of map
	return koord(0,0);
}


uint16 stadt_t::get_max_dimension()
{
	const uint16 x = ur.x - lo.x;
	const uint16 y = ur.y - lo.y;
	return max(x,y);
}


void stadt_t::merke_passagier_ziel(koord k, uint8 color)
{
	vector_tpl<koord> building_list;
	building_list.append(k);
	const grund_t* gr = welt->lookup_kartenboden(k);
	if(gr)
	{
		const gebaeude_t* gb = gr->find<gebaeude_t>();
		if(gb)
		{
			const building_tile_desc_t* tile = gb->get_tile();
			const building_desc_t *bdsc = tile->get_desc();
			koord size = bdsc->get_size(tile->get_layout());

			if(size != koord(1,1))
			{
				// Only add more tiles for multi-tiled buildings.
				// Single tiled buildings have already had their tile added.

				const koord3d pos = gb->get_pos() - koord3d(tile->get_offset(), 0);
				koord k;

				for(k.y = 0; k.y < size.y; k.y ++)
				{
					for(k.x = 0; k.x < size.x; k.x ++)
					{
						koord3d k_3d = koord3d(k, 0) + pos;
						grund_t *gr = welt->lookup(k_3d);
						if(gr)
						{
							gebaeude_t *gb_part = gr->find<gebaeude_t>();
							// There may be buildings with holes.
							if(gb_part && gb_part->get_tile()->get_desc() == bdsc && k_3d.get_2d() != k)
							{
								building_list.append(k_3d.get_2d());
							}
						}
					}
				}
			}
		}
	}

	FOR(vector_tpl<koord>, const& position, building_list)
	{
		pax_destinations_new.set(position, color);
	}

	pax_destinations_new_change ++;
}


/**
 * building_place_with_road_finder:
 * Search for a free location using the function find_place().
 * added: Minimum distance between monuments
 * @author V. Meyer/prissi
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
			const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_ausflugsziele();
			int dist = welt->get_size().x * welt->get_size().y;
			FOR(  weighted_vector_tpl<gebaeude_t*>, const i, attractions  ) {
				int const d = koord_distance(i->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			FOR(  weighted_vector_tpl<stadt_t *>, const city, welt->get_cities() ) {
				int const d = koord_distance(city->get_pos(), pos);
				if(  d < dist  ) {
					dist = d;
				}
			}
			return dist;
		}

		virtual bool is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const
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
					if (	0 <= x  &&  x < w-1  &&  0 <= y  &&  y < h-1) {
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
	const building_desc_t* desc = hausbauer_t::get_special(has_townhall ? bev : 0, building_desc_t::attraction_city, welt->get_timeline_year_month(), (bev == 0) || !has_townhall, welt->get_climate(pos));
	if (desc != NULL) {
		if (simrand(100, "void stadt_t::check_bau_spezial") < (uint)desc->get_distribution_weight()) {
			// build was immer es ist

			bool big_city = buildings.get_count() >= 10;
			bool is_rotate = desc->get_all_layouts() > 1;
			sint16 radius = koord_distance( get_rechtsunten(), get_linksoben() )/2 + 10;
			// find place
			koord best_pos = building_place_with_road_finder(welt, radius, big_city).find_place(pos, desc->get_x(), desc->get_y(), desc->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				int rotate = 0;
				if (desc->get_all_layouts() > 1) {
					rotate = (simrand(20, "void stadt_t::check_bau_spezial") & 2) + is_rotate;
				}
				gebaeude_t* gb = hausbauer_t::build(owner, welt->lookup_kartenboden(best_pos)->get_pos(), rotate, desc);
				gb->access_first_tile()->set_stadt(this);
				add_building_to_list(gb->access_first_tile());
				// tell the player, if not during initialization
				if (!new_town) {
					cbuffer_t buf;
					buf.printf( translator::translate("To attract more tourists\n%s built\na %s\nwith the aid of\n%i tax payers."), get_name(), make_single_line_string(translator::translate(desc->get_name()), 2), city_history_month[0][HIST_CITICENS]);
					welt->get_message()->add_message(buf, best_pos + koord(1, 1), message_t::city, CITY_KI, desc->get_tile(0)->get_background(0, 0, 0));
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

				grund_t::road_network_plan_t road_tiles;

				for (int i = 0; i < total_size.y; i++) {
					road_tiles.set(best_pos + koord(0,i), true);
					road_tiles.set(best_pos + koord(total_size.x-1,i), true);
				}
				for (int i = 1; i < total_size.x-1; i++) {
					road_tiles.set(best_pos + koord(i,0), true);
					road_tiles.set(best_pos + koord(i,total_size.y-1), true);
				}
				for (int i = 1; i < total_size.x-1; i++) {
					for (int j = 1; j < total_size.y-1; j++) {
						road_tiles.set(best_pos + koord(i,j), false);
					}
				}

				bool ok=false;

				FOR(grund_t::road_network_plan_t, i, road_tiles) {
					if (i.value == true) {
						ok = ok || welt->access(i.key)->get_kartenboden()->hat_weg(road_wt);
					}
				}

				if (!ok) {
					return;
				}

				ok = grund_t::fixup_road_network_plan(road_tiles);

				if (ok) {
					// build roads around the monument
					sint16 h=welt->lookup_kartenboden(best_pos)->get_hoehe();
					FOR(grund_t::road_network_plan_t, i, road_tiles) {
						koord k = i.key;
						grund_t *gr = welt->lookup_kartenboden(k);
						if (!i.value) {
							gr->weg_entfernen(road_wt, true);
						}
					}

					FOR(grund_t::road_network_plan_t, i, road_tiles) {
						koord k = i.key;
						const grund_t *gr = welt->lookup_kartenboden(k);
						if (i.value) {
							if(gr->get_hoehe()==h  &&  gr->get_grund_hang()==0) {
								gebaeude_t *gb = gr->find<gebaeude_t>();
								if (gb) {
									hausbauer_t::remove(NULL, gb, false);
								}
								if (gr->hat_weg(road_wt)) {
									continue;
								}

								bool success = build_road(k, NULL, true, false);

								assert(success);
							}
						}
					}

					// and then build it
					gebaeude_t* gb = hausbauer_t::build(owner, welt->lookup_kartenboden(best_pos + koord(1, 1))->get_pos(), 0, desc);
					hausbauer_t::monument_erected(desc);
					add_gebaeude_to_stadt(gb);
					reset_city_borders();
					// tell the player, if not during initialization
					if (!new_town) {
						cbuffer_t buf;
						buf.printf( translator::translate("With a big festival\n%s built\na new monument.\n%i citizen rejoiced."), get_name(), get_einwohner() );
						welt->get_message()->add_message(buf, best_pos + koord(1, 1), message_t::city, CITY_KI, desc->get_tile(0)->get_background(0, 0, 0));
					}
				}
			}
		}
	}
}


void stadt_t::check_bau_townhall(bool new_town)
{
	const building_desc_t* desc = hausbauer_t::get_special( bev, building_desc_t::townhall, welt->get_timeline_year_month(), bev == 0, welt->get_climate(pos) );
	if(desc != NULL) {
		grund_t* gr = welt->lookup_kartenboden(pos);
		gebaeude_t* gb = obj_cast<gebaeude_t>(gr->first_obj());
		bool neugruendung = !has_townhall || !gb || !gb->is_townhall();
		bool umziehen = !neugruendung;
		koord alte_str(koord::invalid);
		koord best_pos(pos);
		koord k;
		int old_layout(0);

		DBG_MESSAGE("check_bau_townhall()", "bev=%d, new=%d name=%s", bev, neugruendung, name.c_str());

		if(  umziehen  ) {

			const building_desc_t* desc_alt = gb->get_tile()->get_desc();
			if (desc_alt->get_level() == desc->get_level()) {
				DBG_MESSAGE("check_bau_townhall()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}
			old_layout = gb->get_tile()->get_layout();
			const sint8 old_z = gb->get_pos().z;
			koord pos_alt = best_pos = gr->get_pos().get_2d() - gb->get_tile()->get_offset();
			// guess layout for broken townhall's
			if(desc_alt->get_x() != desc_alt->get_y()  &&  desc_alt->get_all_layouts()==1) {
				// test all layouts
				koord corner_offset(desc_alt->get_x()-1, desc_alt->get_y()-1);
				for(uint8 test_layout = 0; test_layout<4; test_layout++) {
					// is there a part of our townhall in this corner
					grund_t *gr0 = welt->lookup_kartenboden(pos + corner_offset);
					gebaeude_t const* const gb0 = gr0 ? obj_cast<gebaeude_t>(gr0->first_obj()) : 0;
					if (gb0  &&  gb0->is_townhall()  &&  gb0->get_tile()->get_desc()==desc_alt  &&  gb0->get_stadt()==this) {
						old_layout = test_layout;
						pos_alt = best_pos = gr->get_pos().get_2d() + koord(test_layout%3!=0 ? corner_offset.x : 0, test_layout&2 ? corner_offset.y : 0);
						break;
					}
					corner_offset = koord(-corner_offset.y, corner_offset.x);
				}
			}
			koord groesse_alt = desc_alt->get_size(old_layout);

			// do we need to move
			if(  old_layout<=desc->get_all_layouts()  &&  desc->get_x(old_layout) <= groesse_alt.x  &&  desc->get_y(old_layout) <= groesse_alt.y  ) {
				// no, the size is ok
				// still need to check whether the existing townhall is not broken in some way
				umziehen = false;
				for (k.y = 0; k.y < groesse_alt.y; k.y++) {
					for (k.x = 0; k.x < groesse_alt.x; k.x++) {
						// for buildings with holes the hole could be on a different height ->gr==NULL
						bool ok = false;
						if (grund_t *gr = welt->lookup_kartenboden(k + pos)) {
							if (gebaeude_t *gb_part = gr->find<gebaeude_t>()) {
								// there may be buildings with holes, so we only remove our building!
								if (gb_part->get_tile() == desc_alt->get_tile(old_layout, k.x, k.y)) {
									ok = true;
								}
							}
						}
						umziehen |= !ok;
					}
				}
				if (!umziehen) {
					// correct position if new townhall is smaller than old
					if (old_layout == 0) {
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
					koord k = pos + (old_layout==0 ? koord(0, desc_alt->get_y()) : koord(desc_alt->get_x(),0) );
					if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
						alte_str = k;
					}
					else {
						k = pos - (old_layout==0 ? koord(0, desc_alt->get_y()) : koord(desc_alt->get_x(),0) );
						if (welt->lookup_kartenboden(k)->hat_weg(road_wt)) {
							alte_str = k;
						}
					}
				}
			}
			// remove old townhall
			if(  gb  ) {
				DBG_MESSAGE("stadt_t::check_bau_townhall()", "delete townhall at (%s)", pos_alt.get_str());
				hausbauer_t::remove(NULL, gb, false);
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
							build_city_building(pos, new_town, false);
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
		int layout = umziehen || neugruendung ? simrand(desc->get_all_layouts(), "void stadt_t::check_bau_townhall") : old_layout % desc->get_all_layouts();
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
		gebaeude_t* new_gb = hausbauer_t::build(owner, welt->lookup_kartenboden(best_pos + offset)->get_pos(), layout, desc);
		DBG_MESSAGE("new townhall", "use layout=%i", layout);
		add_gebaeude_to_stadt(new_gb);
		// sets has_townhall to true
		reset_city_borders();
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
				bauigel.set_overtaking_mode(twoway_mode);
				bauigel.build();
			}
			else {
				build_road(best_pos + road0, NULL, true, false);
			}
			townhall_road = best_pos + road0;
		}
		if (umziehen  &&  alte_str != koord::invalid) {
			// build street from former City Hall to new one.
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


/* eventually adds a new industry
 * so with growing number of inhabitants the industry grows
 * @date 12.1.05
 * @author prissi
 */
void stadt_t::check_bau_factory(bool new_town)
{
	uint32 const inc = welt->get_settings().get_industry_increase_every();
	if (!new_town && inc > 0 && (uint32)bev %inc == 0)
	{
		uint32 div = bev / inc;
		for (uint8 i = 0; i < 8; i++)
		{
			if (div == (1u<<i) && welt->get_actual_industry_density() < welt->get_target_industry_density())
			{
				// Only add an industry if there is a need for it: if the actual industry density is less than the target density.
				// @author: jamespetts
				DBG_MESSAGE("stadt_t::check_bau_factory", "adding new industry at %i inhabitants.", get_einwohner());
				factory_builder_t::increase_industry_density( true, true );
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
			switch (t) {
				case building_desc_t::city_res: i = 0; break;
				case building_desc_t::city_com: i = 1; break;
				case building_desc_t::city_ind: i = 2; break;
				default:;
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
	koord( 0,  1), // "south" -- lower left
	koord( 1,  0), // "east" -- lower right
	koord( 0, -1), // "north" -- upper left
	koord(-1,  0), // "west" -- upper right
	// now the diagonals
	koord(-1, -1),
	koord( 1, -1),
	koord(-1,  1),
	koord( 1,  1)
};

// The following tables are indexed by a bitfield of "street directions":
// S == 0001 (1)
// E == 0010 (2)
// N == 0100 (4)
// W == 1000 (8)

// This gives a bitfield saying which neighbors are *interesting* for purposes of layout --
// whose layout we should attempt to copy
// We don't bother if there's only one nearby street
// This is the variant when we have no corner layouts
static int interesting_neighbors_no_corners[] = {
		// How to read this:
	15, // 0000 no streets: all interesting
	0,  // 0001    S:
	5,	// 0010   E :
	12,	// 0011   ES: N and W are interesting
	0,	// 0100  N  :
	10,	// 0101  N S: E and W are interesting
	9,	// 0110  NE : S and W are interesting
	8,	// 0111  NES: W is interesting
	0,	// 1000 W   :
	6,	// 1001 W  S: N and E are interesting
	5,	// 1010 W E : N and S are interesting
	4,	// 1011 W ES: N is interesting
	3,	// 1100 WN  : S and E are interesting
	2,	// 1101 WN S: E is interesting
	1,	// 1110 WNE : S is interesting
	0	// 1111 WNES: nothing interesting
};

// This is the variant when we have corner layouts available
static int interesting_neighbors_corners[] = {
		// How to read this:
	15, // 0000 no streets: all interesting
	0,  // 0001    S:
	0,	// 0010   E :
	0,	// 0011   ES:
	0,	// 0100  N  :
	10,	// 0101  N S: E and W are interesting
	0,	// 0110  NE :
	8,	// 0111  NES: W is interesting
	0,	// 1000 W   :
	0,	// 1001 W  S:
	5,	// 1010 W E : N and S are interesting
	4,	// 1011 W ES: N is interesting
	0,	// 1100 WN  :
	2,	// 1101 WN S: E is interesting
	1,	// 1110 WNE : S is interesting
	0	// 1111 WNES: nothing interesting
};

// return layout
//static int const building_layout[] = { 0, 0, 1, 4, 2, 0, 5, 1, 3, 7, 1, 0, 6, 3, 2, 0 };

// This takes a layout (0,1,2,3,4,5,6,7)
// and returns a "streetsdir" style bitfield indicating which ways that layout is facing
static int layout_to_orientations[] = {
	1,  // S
	2,  // E
	4,  // N
	8,  // W
	3,  //SE
	6,  //NE
	12, //NW
	9   //SW
};

void process_city_street(grund_t& gr, const way_desc_t* cr)
{
	weg_t* const weg = gr.get_weg(road_wt);
	if(  weg == NULL  ) {
		return;
	}

	player_t *player = weg->get_owner();

	bool make_public = true;
	if(!weg->is_public_right_of_way())
	{
		const wayobj_t *wo = gr.get_wayobj(road_wt);
		if(wo && wo->get_desc()->is_noise_barrier())
		{
			make_public = false;
		}

		const roadsign_t* rs = gr.find<roadsign_t>();
		if(rs && rs->get_desc()->is_private_way())
		{
			make_public = false;
		}
	}

	// Check whether any changes are needed.
	if((!weg->hat_gehweg() || weg->get_desc() != cr) && make_public == true)
	{
		player_t::add_maintenance(player, -weg->get_desc()->get_maintenance(), road_wt);
		weg->set_gehweg(true);
		weg->set_owner(NULL); // make public
		if(cr->is_at_least_as_good_as(weg->get_desc()))
		{
			weg->set_desc(cr);
		}
	}
	gr.calc_image();
	reliefkarte_t::get_karte()->calc_map_pixel(gr.get_pos().get_2d());
}

/**
 * Get gebaeude_t* for citybuilding at location k
 * Returns NULL if there is no citybuilding at that location
 */
const gebaeude_t* stadt_t::get_citybuilding_at(const koord k) const {
	grund_t* gr = welt->lookup_kartenboden(k);
	if (gr && gr->get_typ() == grund_t::fundament && gr->obj_bei(0)->get_typ() == obj_t::gebaeude) {
		// We have a building as a neighbor...
		gebaeude_t const* const gb = obj_cast<gebaeude_t>(gr->first_obj());
		if (gb != NULL) {
			return gb;
		}
	}
	return NULL;
}


/**
 * Returns best layout (orientation) for a new city building h at location k.
 * This needs to know the nearby street directions.
 */
int stadt_t::get_best_layout(const building_desc_t* h, const koord & k) const {
	// Return value is a layout, which is a direction to face:
	//  0, 1, 2, 3, 4, 5, 6, 7
	//  S, E, N, W,SE,NE,NW,SW

	assert(h != NULL);

	// Streetdirs is a bitfield of "street directions":
	// S == 0001 (1)
	// E == 0010 (2)
	// N == 0100 (4)
	// W == 1000 (8)
	int streetdirs = 0;
	int flat_streetdirs = 0;
	for (int i = 0; i < 4; i++) {
		const grund_t* gr = welt->lookup_kartenboden(k + neighbors[i]);
		if (gr == NULL) {
			// No ground, skip this neighbor
			continue;
		}
		const strasse_t* weg = (const strasse_t*)gr->get_weg(road_wt);
		if (weg != NULL) {
			// We found a road... (yes, it is OK to face a road with a different hang)
			// update directions (SENW)
			streetdirs += (1 << i);
			if (gr->get_weg_hang() == slope_t::flat) {
				// Will get flat bridge ends as well as flat ground
				flat_streetdirs += (1 << i);
			}
		}
	}
	// Prefer flat streetdirs.  (Yes, this includes facing flat bridge ends.)
	// If there are any, forget the other directions.
	// But if there aren't,... use the other directions.
	if (flat_streetdirs != 0) {
		streetdirs = flat_streetdirs;
	}

	bool has_corner_layouts = (h->get_all_layouts() == 8);
	const int* interesting_neighbors;
	if (has_corner_layouts) {
		interesting_neighbors = interesting_neighbors_corners;
	} else {
		interesting_neighbors = interesting_neighbors_no_corners;
	}

	// Check the neighbors to collect desirable orientations.
	// Please note that this short-circuits quickly (doing nothing) for
	// cases where it is not worth checking neighbors.
	uint8 dirs_nbr_all = 0;
	uint8 dirs_nbr = 0;
	for (int i = 0; i < 4; i++) {
		if ( interesting_neighbors[streetdirs] & (1 << i) ) {
			const gebaeude_t* neighbor_gb = get_citybuilding_at(k + neighbors[i]);
			if (neighbor_gb != NULL) {
				// We have a building as a neighbor...
				const uint8 neighbor_layout = neighbor_gb->get_tile()->get_layout();
				assert (neighbor_layout <= 7);
				// Convert corner (and other) layouts to bitmaps of cardinal direction orientations
				uint8 neighbor_orientations = layout_to_orientations[neighbor_layout];
				if (streetdirs) {
					// Filter by nearby street directions (unless there are no nearby streets)
					 neighbor_orientations &= streetdirs;
				}
				// Collect it in dirs_nbr_all:
				dirs_nbr_all |= neighbor_orientations;
				// Is it a member of the same cluster as this building?
				if (h->get_clusters() & neighbor_gb->get_tile()->get_desc()->get_clusters() ) {
					// If so collect it in dirs_nbr:
					dirs_nbr |= neighbor_orientations;
				}
			}
		}
	}
	// If we have a matching dirs_nbr, use that.
	// If it's 0, use dirs_nbr_all instead.
	if (dirs_nbr == 0) {
		dirs_nbr = dirs_nbr_all;
	}
	// If neighbor_orientations is also zero, we'll use a default choice.

	// The compiler will reorder the cases in the switch, presumably.
	// These are sorted according to internal logic
	switch (streetdirs) {
		// First the easy cases: only one reasonable direction
		case 15: // NSEW (surrounded by roads): default to south.
			return 0;
		case 1: // S
			return 0;
		case 2: // E
			return 1;
		case 4: // N
			return 2;
		case 8: // W
			return 3;
		// Now the corners: corner layout, or best neighbor, or default
		case 3: // SE
			if (has_corner_layouts) {
				return 4;
			} else if (dirs_nbr & 1) {
				return 0; // S-facing neighbor (preferred)
			} else if (dirs_nbr & 2) {
				return 1; // E-facing neighbor
			} else {
				return 0; // no match, default S-facing
			}
			break;
		case 6: // NE
			if (has_corner_layouts) {
				return 5;
			} else if (dirs_nbr & 2) {
				return 1; // E-facing neighbor (preferred)
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor
			} else {
				return 1; // no match, default E-facing
			}
			break;
		case 12: // NW
			if (has_corner_layouts) {
				return 6;
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor (preferred)
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor
			} else {
				return 2; // no match, default N-facing
			}
			break;
		case 9: // SW
			if (has_corner_layouts) {
				return 7;
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor (preferred)
			} else if (dirs_nbr & 1) {
				return 0; // S-facing neighbor
			} else {
				return 3; // no match, default W-facing
			}
			break;
		// Now the "sandwiched": best neighbor or default
		case 5: // NS
			if (dirs_nbr & 1) {
				return 0; // S-facing neighbor
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor
			} else {
				return 0; // no match, default S-facing
			}
			break;
		case 10: // EW
			if (dirs_nbr & 2) {
				return 1; // E-facing neighbor
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor
			} else {
				return 1; // no match, default E-facing
			}
			break;
		// Now the "three-sided".  Get the best corner, or the best match, or face the third side.
		case 7: // NES
			if (has_corner_layouts) {
				if (dirs_nbr & 1) {
					return 4; // S-facing neighbor, face SE
				} else if (dirs_nbr & 4) {
					return 5; // N-facing neighbor, face NE
				} else {
					return 4; // no match, face SE by default
				}
			} else if (dirs_nbr & 1) {
				return 0; // S-facing neighbor
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor
			} else {
				return 1; // face E
			}
			break;
		case 11: // WES
			if (has_corner_layouts) {
				if (dirs_nbr & 2) {
					return 4; // E-facing neighbor, face SE
				} else if (dirs_nbr & 8) {
					return 7; // W-facing neighbor, face SW
				} else {
					return 4; // no match, face SE by default
				}
			} else if (dirs_nbr & 2) {
				return 1; // E-facing neighbor
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor
			} else {
				return 0; // face S
			}
			break;
		case 13: // WNS
			if (has_corner_layouts) {
				if (dirs_nbr & 1) {
					return 7; // S-facing neighbor, face SW
				} else if (dirs_nbr & 4) {
					return 6; // N-facing neighbor, face NW
				} else {
					return 7; // no match, face SW by default
				}
			} else if (dirs_nbr & 1) {
				return 0; // S-facing neighbor
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor
			} else {
				return 3; // face W
			}
			break;
		case 14: // WNE
			if (has_corner_layouts) {
				if (dirs_nbr & 2) {
					return 5; // E-facing neighbor, face NE
				} else if (dirs_nbr & 8) {
					return 6; // W-facing neighbor, face NW
				} else {
					return 5; // no match, face NE by default
				}
			} else if (dirs_nbr & 2) {
				return 1; // E-facing neighbor
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor
			} else {
				return 2; // face N
			}
			break;
		// Now the most annoying case: no streets
		// This is probably a dead-end corner.  (I suggest not allowing this in construction rules.)
		// This should be done with more care by looking at the corner-edge streets
		// but that would be even more work to implement!
		// As it is this can give strange results when the neighbor is picked up
		// from the "back side of the fence".
		case 0:
			if (dirs_nbr & 1) {
				return 0; // S-facing neighbor
			} else if (dirs_nbr & 2) {
				return 1; // E-facing neighbor
			} else if (dirs_nbr & 4) {
				return 2; // N-facing neighbor
			} else if (dirs_nbr & 8) {
				return 3; // W-facing neighbor
			} else {
				return 0; //default to S
			}
			break;
		default:
			assert(false); // we should never get here
			return 0; // default to south
	}
}

/*
 * get available building sizes to renovate
 *
 * Tiles must be nature or a city building.
 * Tile height must be same. If it is slope, it is treated as it is raised.
 */
void stadt_t::get_available_building_size(const koord k, vector_tpl<koord> &sizes) const {
	sizes.clear();
	const uint8 LEN_LIM = building_desc_t::get_city_building_max_size();
	const grund_t* gr_origin = welt->lookup_kartenboden(k);
	assert(gr_origin);
	const gebaeude_t* gb_origin = gr_origin->get_building();
	assert(gb_origin);
	const koord dim_origin = gb_origin->get_tile()->get_desc()->get_size();
	const uint8 layout_origin = gb_origin->get_tile()->get_layout();
	for(uint8 w=(layout_origin&1)?dim_origin.y:dim_origin.x; w<=LEN_LIM; w++) {
		for(uint8 h=(layout_origin&1)?dim_origin.x:dim_origin.y; h<=LEN_LIM; h++) {
			bool check_continue = true;
			sint8 height = -100;
			for(uint8 x=0; x<w; x++) {
				if(!check_continue) {
					break;
				}
				for(uint8 y=0; y<h; y++) {
					const koord p = k + koord(x,y);
					grund_t* gr = welt->lookup_kartenboden(p);
					// the tile must be nature or a city building.
					if(  !gr  ||  !(gr->ist_natur()  ||  (gr->get_building()  &&  gr->get_building()->is_city_building()))) {
						check_continue = false;
						break;
					}
					// the tile must be in the same height as others.
					sint8 tile_height;
					const slope_t::type hang = welt->recalc_natural_slope(p, tile_height);
					if(hang!=slope_t::flat) {
						tile_height ++;
					}
					if(height!=-100  &&  height!=tile_height) {
						check_continue = false;
						break;
					}
					height = tile_height;
					// buildings in the area must not be in the outside of the area.
					const sint8 x_off = x==0 ? -1 : (x==w-1 ? 1 : 0);
					const sint8 y_off = y==0 ? -1 : (y==h-1 ? 1 : 0);
					const gebaeude_t* gb = gr->get_building();
					if(gb  &&  (x==0  ||  y==0  ||  x==w-1  ||  y==h-1)) {
						const grund_t* neighbor_gr = welt->lookup_kartenboden(k+koord(x_off,y_off));
						if(neighbor_gr) {
							const gebaeude_t* neighbor_gb = neighbor_gr->get_building();
							if(gb==neighbor_gb) {
								check_continue = false;
								break;
							}
						}
					}
				}
			}
			if(check_continue) {
				koord s(w,h);
				sizes.append(s);
			}
		}
	}
}


void stadt_t::build_city_building(const koord k, bool new_town, bool map_generation)
{
	grund_t* gr = welt->lookup_kartenboden(k);
	if (!gr)
	{
		return;
	}
	const koord3d pos(gr->get_pos());

	// Not building on ways (this was actually tested before be the cityrules), btu you can construct manually
	if(  !gr->ist_natur() ) {
		return;
	}
	// test ownership of all objects that can block construction
	for (uint8 i = 0; i < gr->obj_count(); i++) {
		obj_t *const obj = gr->obj_bei(i);
		if (obj->is_deletable(NULL) != NULL  &&  obj->get_typ() != obj_t::pillar) {
			return;
		}
	}
	// Refuse to build on a slope, when there is a groudn right on top of it (=> the house would sit on the bridge then!)
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
	const climate cl = welt->get_climate_at_height(welt->max_hgt(k));

	// Run through orthogonal neighbors (only) looking for which cluster to build
	// This is a bitmap -- up to 32 clustering types are allowed.
	uint32 neighbor_building_clusters = 0;
	for (int i = 0; i < 4; i++) {
		const gebaeude_t* neighbor_gb = get_citybuilding_at(k + neighbors[i]);
		if (neighbor_gb) {
			// We have a building as a neighbor...
			neighbor_building_clusters |= neighbor_gb->get_tile()->get_desc()->get_clusters();
		}
	}

	bool worker_shortage = false;
	bool job_shortage = false;

	// This is a temporary system intended to prevent an imbalance between jobs and population
	// arising until the completely new town growth algorithm is implemented.
	const sint64 world_jobs = welt->get_finance_history_month(0, karte_t::WORLD_JOBS); 
	const sint64 monthly_job_demand_global = welt->calc_monthly_job_demand();
	if ((world_jobs * 100l) > (monthly_job_demand_global * 110l))
	{
		worker_shortage = true;
	}
	else if ((monthly_job_demand_global  * 100l) > (world_jobs * 110l))
	{
		job_shortage = true;
	}

	// Find a house to build
	const koord size_single(1,1);
	building_desc_t::btype want_to_have = building_desc_t::unknown;
	const building_desc_t* h = NULL;

	if (!worker_shortage && (sum_commercial > sum_industrial  &&  sum_commercial > sum_residential)) {
		h = hausbauer_t::get_commercial(0, size_single, current_month, cl, new_town, neighbor_building_clusters);
		if (h != NULL) {
			want_to_have = building_desc_t::city_com;
		}
	}

	if (!worker_shortage && (h == NULL  &&  sum_industrial > sum_residential  &&  sum_industrial > sum_commercial)) {
		h = hausbauer_t::get_industrial(0, size_single, current_month, cl, new_town, neighbor_building_clusters);
		if (h != NULL) {
			want_to_have = building_desc_t::city_ind;
		}
	}

	if (h == NULL  &&  ((sum_residential > sum_industrial  &&  sum_residential > sum_commercial) || worker_shortage)) {
		if (!job_shortage)
		{
			h = hausbauer_t::get_residential(0, size_single, current_month, cl, new_town, neighbor_building_clusters);
		}
		if (h != NULL) {
			want_to_have = building_desc_t::city_res;
		}
	}

	if (h == NULL) {
		// Found no suitable building.  Return!
		return;
	}
//	if (h->get_clusters() == 0) {
//		// This is a non-clustering building.  Do not allow it next to an identical building.
//		// (This avoids "boring cities", supposedly.)
//		for (int i = 0; i < 8; i++) {
//			// Go through the neighbors *again*...
//			const gebaeude_t* neighbor_gb = get_citybuilding_at(k + neighbors[i]);
//			if (neighbor_gb != NULL && neighbor_gb->get_tile()->get_desc() == h) {
//				// Fail.  Get a different building.
//				return;
//			}
//		}
//	}

	// we have something to built here ...
	if (h != NULL) {
		// check for pavement
		for (int i = 0; i < 8; i++) {
			// Neighbors goes through these in 'preferred' order, orthogonal first
			gr = welt->lookup_kartenboden(k + neighbors[i]);
			if(gr && gr->get_weg_hang() == gr->get_grund_hang())
			{
				process_city_street(*gr, welt->get_city_road());
			}
		}

		int layout = get_best_layout(h, k);

		gebaeude_t* gb = hausbauer_t::build(NULL, pos, layout, h);
		add_gebaeude_to_stadt(gb, false, map_generation, map_generation);
		reset_city_borders();

		switch(want_to_have) {
			case building_desc_t::city_res:   won += h->get_level() * 10; break;
			case building_desc_t::city_com:   arb +=  h->get_level() * 20; break;
			case building_desc_t::city_ind: arb +=  h->get_level() * 20; break;
			default: break;
		}
	}
}


bool stadt_t::renovate_city_building(gebaeude_t* gb, bool map_generation)
{
	const building_desc_t::btype alt_typ = gb->get_tile()->get_desc()->get_type();
	if (!gb->is_city_building()) {
		return false; // only renovate res, com, ind
	}

	// Now we are sure that this is a city building
	const int level = gb->get_tile()->get_desc()->get_level();
	const koord k = gb->get_pos().get_2d();

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
	const climate cl = welt->get_climate_at_height(gb->get_pos().z);

	// Run through orthogonal neighbors (only) looking for which cluster to build
	// This is a bitmap -- up to 32 clustering types are allowed.
	vector_tpl<koord> orthogonal_neighbors;
	const building_desc_t* gb_desc = gb->get_tile()->get_desc();
	for(sint8 x=0; x<gb_desc->get_size().x; x++) {
		orthogonal_neighbors.append(k+koord(x,-1));
		orthogonal_neighbors.append(k+koord(x,gb_desc->get_size().y));
	}
	for(sint8 y=0; y<gb_desc->get_size().y; y++) {
		orthogonal_neighbors.append(k+koord(-1,y));
		orthogonal_neighbors.append(k+koord(gb_desc->get_size().x,y));
	}
	uint32 neighbor_building_clusters = 0;
	for (uint16 i=0; i<orthogonal_neighbors.get_count(); i++) {
		const gebaeude_t* neighbor_gb = get_citybuilding_at(orthogonal_neighbors[i]);
		if (neighbor_gb) {
			// We have a building as a neighbor...
			neighbor_building_clusters |= neighbor_gb->get_tile()->get_desc()->get_clusters();
		}
	}

	building_desc_t::btype want_to_have = building_desc_t::unknown;
	int sum = 0;

	uint8 max_level = 0; // Unlimited.
	weg_t* way;
	for(int i = 1; i <= narrowgauge_wt; i++)
	{
		grund_t* gr = welt->lookup(gb->get_pos());
		way = gr ? gr->get_weg((waytype_t)i) : NULL;
		if((way && (way_builder_t::bautyp_t)way->get_desc()->get_wtyp() & way_builder_t::elevated_flag) || (gr && gr->ist_bruecke()))
		{
			// Limit this if any elevated way or bridge is found.
			max_level = welt->get_settings().get_max_elevated_way_building_level();
			break;
		}
	}

	// get available building sizes.
	vector_tpl<koord> available_sizes;
	get_available_building_size(k, available_sizes);
	const uint8 size_offset = simrand(available_sizes.get_count(), "bool stadt_t::renovate_city_building");

	// try to build
	const building_desc_t* h = NULL;
	koord selected_dim;
	if (sum_commercial > sum_industrial && sum_commercial > sum_residential) {
		// we must check, if we can really update to higher level ...
		for(uint8 i=0; i<available_sizes.get_count(); i++) {
			const koord dimension = available_sizes[(i+size_offset)%available_sizes.get_count()];
			h = hausbauer_t::get_commercial(k, dimension, current_month, cl, false, neighbor_building_clusters);
			if(  h != NULL  &&  (max_level == 0 || h->get_level() <= max_level)  ) {
				want_to_have = building_desc_t::city_com;
				sum = sum_commercial;
				selected_dim = dimension;
				break;
			}
		}
	}
	// check for industry, also if we wanted com, but there was no com good enough ...
	if(    (sum_industrial > sum_commercial  &&  sum_industrial > sum_residential)
      || (sum_commercial > sum_residential  &&  want_to_have == building_desc_t::unknown)  ) {
		// we must check, if we can really update to higher level ...
		for(uint8 i=0; i<available_sizes.get_count(); i++) {
			const koord dimension = available_sizes[(i+size_offset)%available_sizes.get_count()];
			h = hausbauer_t::get_industrial(k, dimension, current_month, cl, false, neighbor_building_clusters);
			if(  h != NULL  &&  (max_level == 0 || h->get_level() <= max_level)  ) {
				want_to_have = building_desc_t::city_ind;
				sum = sum_industrial;
				selected_dim = dimension;
				break;
			}
		}
	}
	// check for residence
	// (sum_residential > sum_industrial  &&  sum_residential > sum_commercial
	if (  want_to_have == building_desc_t::unknown ) {
		// we must check, if we can really update to higher level ...
		bool found = false;
		for(uint8 i=0; i<available_sizes.get_count(); i++) {
			const koord dimension = available_sizes[(i+size_offset)%available_sizes.get_count()];
			h = hausbauer_t::get_residential(k, dimension, current_month, cl, false, neighbor_building_clusters);
			if(  h != NULL  &&  (max_level == 0 || h->get_level() <= max_level)  ) {
				want_to_have = building_desc_t::city_res;
				sum = sum_residential;
				selected_dim = dimension;
				found = true;
				break;
			}
		}
		if(!found) {
			h = NULL;
		}
	}

	if (h == NULL) {
		// Found no suitable building.  Return!
		return false;
	}

	vector_tpl<koord> surrounding_pos;
	for(sint8 x=-1; x<=h->get_size().x; x++) {
		surrounding_pos.append(k+koord(x,-1));
		surrounding_pos.append(k+koord(x,h->get_size().y));
	}
	for(sint8 y=-1; y<=h->get_size().y; y++) {
		surrounding_pos.append_unique(k+koord(-1,y));
		surrounding_pos.append_unique(k+koord(h->get_size().x,y));
	}

	if (h->get_clusters() == 0) {
		// This is a non-clustering building.  Do not allow it next to an identical building.
		// (This avoids "boring cities", supposedly.)
		for(uint16 i=0; i<surrounding_pos.get_count(); i++) {
			const gebaeude_t* neighbor_gb = get_citybuilding_at(surrounding_pos[i]);
			if (neighbor_gb != NULL && neighbor_gb->get_tile()->get_desc() == h) {
				// Fail.  Return.
				return false;
			}
		}
	}

	// good enough to renovate, and we found a building?
	if (sum > 0 && h != NULL)
	{
//		DBG_MESSAGE("stadt_t::renovate_city_building()", "renovation at %i,%i (%i level) of typ %i to typ %i with desire %i", k.x, k.y, alt_typ, want_to_have, sum);

		for (uint16 i=0; i<surrounding_pos.get_count(); i++) {
			// Neighbors goes through this in a specific order:
			// orthogonal first, then diagonal
			grund_t* gr = welt->lookup_kartenboden(surrounding_pos[i]);
			if (gr == NULL) {
				// No ground, skip this neighbor
				continue;
			}
			weg_t * const weg = gr->get_weg(road_wt);
			if (weg) {
				// Extend the sidewalk
				if (weg->is_public_right_of_way()) {
					strasse_t *str = static_cast<strasse_t *>(weg);
					str->set_gehweg(true);
				} else {
					bool make_public = true;

					const wayobj_t *wo = gr->get_wayobj(road_wt);
					if (wo && wo->get_desc()->is_noise_barrier()) {
						make_public = false;
					}

					const roadsign_t* rs = gr->find<roadsign_t>();
					if (rs && rs->get_desc()->is_private_way()) {
						make_public = false;
					}
					if (make_public) {
						strasse_t *str = static_cast<strasse_t *>(weg);
						str->set_gehweg(true);
						weg->set_public_right_of_way();
					}
				}
				if (gr->get_weg_hang() == gr->get_grund_hang()) {
					process_city_street(*gr, welt->get_city_road());
				}
				gr->calc_image();
				reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
			}
		}

		uint8 layout = h->get_size().x==selected_dim.x ? 0 : 1;
		if(h->get_size().x==1  &&  h->get_size().y==1) {
			layout = get_best_layout(h, k);
		}
		// The building is being replaced.  The surrounding landscape may have changed since it was
		// last built, and the new building should change height along with it, rather than maintain the old
		// height.  So delete and rebuild, even though it's slower.
		// In case of failure, we stock the removed buildings.
		class removed_building {
		public:
			const building_desc_t* desc;
			koord3d pos;
			uint8 layout;
		};
		vector_tpl<removed_building> removed_buildings;
		for(uint8 x=0; x<(layout&1?h->get_size().y:h->get_size().x); x++) {
			for(uint8 y=0; y<(layout&1?h->get_size().x:h->get_size().y); y++) {
				const grund_t* gr = welt->lookup_kartenboden(k+koord(x,y));
				assert(gr);
				const gebaeude_t* bldg = gr->get_building();
				if(bldg) {
					const building_desc_t* desc = bldg->get_tile()->get_desc();
					removed_building rb;
					rb.desc = desc;
					rb.pos = bldg->get_pos();
					rb.layout = bldg->get_tile()->get_layout();
					removed_buildings.append(rb);
					hausbauer_t::remove(NULL, bldg, map_generation);
				}
			}
		}

		koord3d pos = welt->lookup_kartenboden(k)->get_pos();
		gebaeude_t* new_gb = hausbauer_t::build(NULL, pos, layout, h);
		// We *can* skip most of the work in add_gebaeude_to_stadt, because we *just* cleared the location,
		// so it must be valid.  Our borders also should not have changed.

		// Check that all tiles are same height. If it is different, we remove that.
		gebaeude_t* checked_gb = check_tiles_height(new_gb, k, layout, map_generation);
		if(!checked_gb) {
			// height was different. Let's recover.
			for(uint8 j=0; j<removed_buildings.get_count(); j++) {
				const removed_building rb = removed_buildings[j];
				gebaeude_t* g = hausbauer_t::build(NULL, rb.pos, rb.layout, rb.desc);
				g->set_stadt(this);
				add_building_to_list(g, false, map_generation, map_generation);
			}
			return false;
		}

		// culculation of population
		for(uint8 j=0; j<removed_buildings.get_count(); j++) {
			const uint8 level = removed_buildings[j].desc->get_level();
			switch(removed_buildings[j].desc->get_type()) {
				case building_desc_t::city_res: won -= level * 10; break;
				case building_desc_t::city_com: arb -= level * 20; break;
				case building_desc_t::city_ind: arb -= level * 20; break;
				default: break;
			}
		}
		switch(want_to_have) {
			case building_desc_t::city_res:   won += h->get_level() * 10; break;
			case building_desc_t::city_com:   arb +=  h->get_level() * 20; break;
			case building_desc_t::city_ind: arb +=  h->get_level() * 20; break;
			default: break;
		}
		checked_gb->set_stadt(this);
		add_building_to_list(checked_gb, false, map_generation, map_generation);
		return true;
	}
	return false;
}

/*
 * a subroutine of renovate_city_buiding
 * If successfully built, return the pointer of building.
 * If failed, return NULL.
 */
gebaeude_t* stadt_t::check_tiles_height(gebaeude_t* building, koord pos, uint8 layout, bool map_generation) {
	// We check whether all tiles are same height because we sometimes fail to estimate the height.
	const building_desc_t* desc = building->get_tile()->get_desc();
	bool height_check_approved = true;
	sint8 tile_height = -100;
	for(uint8 x=0; x<(layout&1?desc->get_size().y:desc->get_size().x); x++) {
		for(uint8 y=0; y<(layout&1?desc->get_size().x:desc->get_size().y); y++) {
			const grund_t* gr = welt->lookup_kartenboden(pos+koord(x,y));
			if(!gr) {
				dbg->error("stadt_t::check_height_and_rebuild()", "ground not found! pos:%s", (pos+koord(x,y)).get_str());
			}
			if(tile_height!=-100  &&  tile_height!=gr->get_pos().z) {
				// height is different!
				height_check_approved = false;
				break;
			}
			tile_height = gr->get_pos().z;
		}
	}
	if(height_check_approved) {
		// all tiles are same height.
		return building;
	}
	dbg->message("stadt_t::check_tiles_height()", "height is different. we remove building at pos:%s", pos.get_str());
	// height is different.
	// remove all buildings in the area.
	for(uint8 x=0; x<(layout&1?desc->get_size().y:desc->get_size().x); x++) {
		for(uint8 y=0; y<(layout&1?desc->get_size().x:desc->get_size().y); y++) {
			const grund_t* gr = welt->lookup_kartenboden(pos+koord(x,y));
			if(!gr) {
				dbg->error("stadt_t::check_height_and_rebuild()", "ground not found! pos:%s", (pos+koord(x,y)).get_str());
			}
			const gebaeude_t* bldg = gr->get_building();
			if(bldg) {
				if(!bldg->is_city_building()) {
					dbg->error("stadt_t::check_height_and_rebuild()", "building is not a city building! pos:%s", (pos+koord(x,y)).get_str());
				}
				hausbauer_t::remove(NULL, bldg, map_generation);
			}
		}
	}
	return NULL;
}

void stadt_t::add_building_to_list(gebaeude_t* building, bool ordered, bool do_not_add_to_world_list, bool do_not_update_stats)
{
	if (!do_not_update_stats)
	{
		update_city_stats_with_building(building, false);
	}

	if(ordered)
	{
		buildings.insert_ordered(building, building->get_tile()->get_desc()->get_level(), compare_gebaeude_pos);
	}
	else
	{
		buildings.append_unique(building, building->get_tile()->get_desc()->get_level());
	}

	// Also add to the world list for passenger generation purposes.
	if (!do_not_add_to_world_list)
	{
		// Do not add builings one by one to the world list as the map is being generated;
		// rather, add them all at the end. This reduces duplication and therefore saves
		// CPU time, as buildings are upgraded many times during map generation.

		// Likewise, when loading, do not add the building to the world list here, as, when
		// loading multi-threadedly, these all have to be added with an insertion sort, which
		// can make loading network games very slow; instead, these are now added single-
		// threadedly when the game is loading.
		welt->add_building_to_world_list(building, ordered);
	}
}

void stadt_t::add_all_buildings_to_world_list()
{
	for(weighted_vector_tpl<gebaeude_t*>::const_iterator i = buildings.begin(); i != buildings.end(); ++i)
	{
		gebaeude_t* building = *i;
		update_city_stats_with_building(building, false);
		welt->add_building_to_world_list(building);
	}
}

void stadt_t::generate_private_cars(koord pos, uint32 journey_tenths_of_minutes, koord target, uint8 number_of_passengers)
{
#ifdef FORBID_SYNC_OBJECTS
	return;
#endif
#ifdef FORBID_PRIVATE_CARS
	return;
#endif
	if (!env_t::networkmode)
	{
		// This cannot work with network mode
		// and multi-therading.
		welt->inc_rands(28);
	}
	// Account for (1) the number of passengers; and (2) the occupancy level.
	const uint32 round_up = simrand(2, "void stadt_t::generate_private_cars") == 1 ? 900 : 0;
	const sint32 number_of_trips = ((((sint32)number_of_passengers) * traffic_level) + round_up) / 1000;

	koord k;
	for(sint32 i = 0; i < number_of_trips; i++)
	{
		for (k.y = pos.y - 1; k.y <= pos.y + 1; k.y++)
		{
			for (k.x = pos.x - 1; k.x <= pos.x + 1; k.x++)
			{
				if (welt->is_within_limits(k))
				{
					grund_t* gr = welt->lookup_kartenboden(k);
					const weg_t* weg = gr->get_weg(road_wt);

					if (weg != NULL &&
					    (weg->is_public_right_of_way() || weg->get_owner() == NULL || weg->get_owner()->allows_access_to(1))
						&& (gr->get_weg_ribi_unmasked(road_wt) == ribi_t::northsouth ||
					       gr->get_weg_ribi_unmasked(road_wt) == ribi_t::eastwest))
					{
						if (!private_car_t::list_empty())
						{
							private_car_t* vt = new private_car_t(gr, target);
							const sint32 time_to_live = ((sint32)journey_tenths_of_minutes * 136584) / (sint32)welt->get_settings().get_meters_per_tile();
							vt->set_time_to_life(time_to_live);
							//gr->obj_add(vt);
#ifdef MULTI_THREAD
							karte_t::private_cars_added_threaded[karte_t::passenger_generation_thread_number].append(vt);
#else
							welt->sync.add(vt);
#endif
						}
						goto outer_loop;
					}
				}
			}
		}
		outer_loop:;
	}
}


/**
 * Build a bridge for the city
 * bd == startirng ground
 * zv == direction of construction (must be N, S, E, or W)
 */
bool stadt_t::build_bridge(grund_t* bd, ribi_t::ribi direction, bool map_generation) {
	koord3d k3d = bd->get_pos();
	koord k = k3d.get_2d();
	koord zv = koord(direction);

	const bridge_desc_t *bridge = bridge_builder_t::find_bridge(road_wt, welt->get_city_road()->get_topspeed(), welt->get_timeline_year_month(), welt->get_city_road()->get_max_axle_load() * 2 );
	if(  bridge==NULL  ) {
		// does not have a bridge available ...
		return false;
	}
	/*
	 * We want to discourage city construction of bridges.
	 * Make a simrand call and refuse to construct a bridge some of the time.
	 * "bridge_success_percentage" is the percent of the time when bridges should *succeed*.
	 * --neroden
	 */
	if(  simrand(100, "stadt_t::build_road() (bridge check)") >= bridge_success_percentage  ) {
		return false;
	}

	// Check whether the bridge needs to be high enough for ships to pass underneath
	// Unfortunately, this does not work where the river sides are sloped, as they usually are.
	bool high_bridge = false;
	const grund_t* gr = welt->lookup_kartenboden(k + zv);
	const weg_t* underlying_way = gr ? gr->get_weg(water_wt) : NULL;
	if (underlying_way && (underlying_way->is_public_right_of_way() || underlying_way->get_owner()))
	{
		high_bridge = true;
	}

	const char *err = NULL;
	sint8 bridge_height;
	// Prefer "non-AI bridge"
	koord3d end = bridge_builder_t::find_end_pos(NULL, k3d, zv, bridge, err, bridge_height, false, 0, high_bridge);
	if(err && *err || koord_distance(k, end.get_2d()) > 3  ) {
		// allow "AI bridge"
		end = bridge_builder_t::find_end_pos(NULL, k3d, zv, bridge, err, bridge_height, true, 0, high_bridge);
	}
	if(  err && *err || koord_distance(k, end.get_2d()) > 3  ) {
		// no bridge short enough
		return false;
	}
	// Bridge looks OK, but check the end
	const grund_t* past_end = welt->lookup_kartenboden( (end+zv).get_2d() );
	if (past_end == NULL) {
		// No bridges to nowhere
		return false;
	}
	bool successfully_built_past_end = false;
	// Build a road past the end of the future bridge (even if it has no connections yet)
	// This may fail, in which case we shouldn't build the bridge
	successfully_built_past_end = build_road( (end+zv).get_2d(), NULL, true, map_generation);

	if (!successfully_built_past_end) {
		return false;
	}

	// OK, build the bridge
	bridge_builder_t::build_bridge(NULL, k3d, end, zv, bridge_height, bridge, welt->get_city_road());
	// Now connect the bridge to the road we built
	// (Is there an easier way?)
	build_road( (end+zv).get_2d(), NULL, false, map_generation );

	// Attempt to expand the city repeatedly in the bridge direction
	bool reached_end_plus_2=false;
	bool reached_end_plus_1=false;
	bool reached_end=false;
	for (koord k_new = k; k_new != end.get_2d() + zv + zv + zv; k_new += zv) {
		bool expanded_successfully = false;
		planquadrat_t const* pl = welt->access(k_new);
		if (!pl) {
			break;
		}
		stadt_t const* tile_city = pl->get_city();
		if (tile_city) {
			if (tile_city == this) {
				// Already expanded this far.
				expanded_successfully = true;
			} else {
				// Oops, we ran into another city.
				break;
			}
		} else {
			// not part of a city, see if we can expand
			expanded_successfully = enlarge_city_borders(direction);
		}
		if (!expanded_successfully) {
			// If we didn't expand this far, don't expand further
			break;
		}
		if (expanded_successfully) {
			if (k_new == end.get_2d() + zv + zv) {
				reached_end_plus_2=true;
			} else if (k_new == end.get_2d() + zv) {
				reached_end_plus_1=true;
			} else if (k_new == end.get_2d()) {
				reached_end=true;
			}
		}
	}

	// try to build a house near the bridge end
	// Orthogonal only.  Prefer facing onto bridge.
	// Build only if we successfully expanded the city onto the location.
	koord right_side = koord(ribi_t::rotate90(direction));
	koord left_side = koord(ribi_t::rotate90l(direction));
	vector_tpl<koord> appropriate_locs(5);
	if (reached_end) {
		appropriate_locs.append(end.get_2d()+right_side);
		appropriate_locs.append(end.get_2d()+left_side);
	}
	if (reached_end_plus_2) {
		appropriate_locs.append(end.get_2d()+zv+zv);
	}
	if (reached_end_plus_1) {
		appropriate_locs.append(end.get_2d()+zv+right_side);
		appropriate_locs.append(end.get_2d()+zv+left_side);
	}
	uint32 old_count = buildings.get_count();
	for(uint8 i=0; i<appropriate_locs.get_count(); i++) {
		planquadrat_t const* pl = welt->access(appropriate_locs[i]);
		if (pl) {
			stadt_t const* tile_city = pl->get_city();
			if (tile_city && tile_city == this) {
				build_city_building(appropriate_locs[i], true, map_generation);
				if (buildings.get_count() != old_count) {
					// Successful construction.
					// Fix city limits.
					reset_city_borders();
					break;
				}
			}
		}
	}
	return true;
}


/**
 * baut ein Stueck Strasse
 *
 * @param k         Bauposition
 *
 * @author Hj. Malthaner, V. Meyer
 */
bool stadt_t::build_road(const koord k, player_t* player_, bool forced, bool map_generation)
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
			welt->set_water_hgt(k, bd->get_hoehe()-1);
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
		if (ribi_t::nsew[r] & allowed_dir) {
			// now we have to check for several problems ...
			grund_t* bd2;
			if(bd->get_neighbour(bd2, invalid_wt, ribi_t::nsew[r])) {
				if(bd2->get_typ()==grund_t::fundament  ||  bd2->get_typ()==grund_t::wasser) {
					// not connecting to a building of course ...
				}
				else if (!bd2->ist_karten_boden()) {
					// do not connect to elevated ways / bridges
				}
				else if (bd2->get_typ()==grund_t::tunnelboden  &&  ribi_t::nsew[r]!=ribi_type(bd2->get_grund_hang())) {
					// not the correct slope
				}
				else if (bd2->get_typ()==grund_t::brueckenboden
					&&  (bd2->get_grund_hang()==slope_t::flat  ?  ribi_t::nsew[r]!=ribi_type(bd2->get_weg_hang())
					                                           :  ribi_t::backward(ribi_t::nsew[r])!=ribi_type(bd2->get_grund_hang()))) {
					// not the correct slope
				}
				else if(bd2->hat_weg(road_wt)) {
					const gebaeude_t* gb = bd2->find<gebaeude_t>();
					if(gb) {
						uint8 layouts = gb->get_tile()->get_desc()->get_all_layouts();
						// nothing to connect
						if(layouts==4) {
							// single way
							if(ribi_t::nsew[r]==ribi_t::backward(ribi_t::layout_to_ribi[gb->get_tile()->get_layout()])) {
								// allowed ...
								connection_roads |= ribi_t::nsew[r];
							}
						}
						else if(layouts==2 || layouts==8 || layouts==16) {
							// through way
							if((ribi_t::doubles( ribi_t::layout_to_ribi[gb->get_tile()->get_layout() & 1] )&ribi_t::nsew[r])!=0) {
								// allowed ...
								connection_roads |= ribi_t::nsew[r];
							}
						}
						else {
							dbg->error("stadt_t::build_road()", "building on road with not directions at %i,%i?!?", k.x, k.y );
						}
					}
					else if(bd2->get_depot() || bd2->get_signalbox()) {
						// do not enter depots or signalboxes
					}
					else {
						// check slopes
						way_builder_t bauer( NULL );
						bauer.init_builder( way_builder_t::strasse | way_builder_t::terraform_flag, welt->get_city_road() );
						if(  bauer.check_slope( bd, bd2 )  ) {
							// allowed ...
							connection_roads |= ribi_t::nsew[r];
						}
					}
				}
			}
		}
	}

	if (connection_roads != ribi_t::none || forced) {
		grund_t::road_network_plan_t road_tiles;
		while (bd->would_create_excessive_roads(road_tiles)) {
			if (!bd->remove_excessive_roads(road_tiles)) {
				return false;
			}
		}

		FOR(grund_t::road_network_plan_t, i, road_tiles) {
			koord k = i.key;
			grund_t *gr = welt->lookup_kartenboden(k);
			if (!i.value) {
				gr->weg_entfernen(road_wt, true);
			}
		}
	}

	// now add the ribis to the other ways (if there)
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsew[r] & connection_roads) {
			grund_t* bd2 = welt->lookup_kartenboden(k + koord::nsew[r]);
			weg_t* w2 = bd2->get_weg(road_wt);
			// The following code was added by Philip on the 30th of August 2014, but this causes roads to become disconnected when the city grows.
			/*
			const roadsign_t* rs = bd2->find<roadsign_t>();
			wayobj_t *wo = bd2->get_wayobj(road_wt);
			if ((rs && rs->get_desc()->is_private_way()) ||
			    (w2 && !w2->is_public_right_of_way()) ||
			    (wo && wo->get_desc()->is_noise_barrier())) {
				connection_roads &= ~ribi_t::nsew[r];
			} else {
				w2->ribi_add(ribi_t::backward(ribi_t::nsew[r]));
				bd2->calc_image();
				bd2->set_flag( grund_t::dirty );
			}
			*/
			// In Philip's code, the following three lines were deleted.
			w2->ribi_add(ribi_t::backward(ribi_t::nsew[r]));
			bd2->calc_image();
			bd2->set_flag( grund_t::dirty );
		}
	}

	if (connection_roads != ribi_t::none || forced) {
		if (bd->weg_erweitern(road_wt, connection_roads)) {
			weg_t *weg = bd->get_weg(road_wt);
			weg->set_desc(welt->get_city_road());
			if (weg->is_public_right_of_way()) {
				strasse_t *str = static_cast<strasse_t *>(weg);
				str->set_gehweg(true);
			} else {
				bool make_public = true;

				const wayobj_t *wo = bd->get_wayobj(road_wt);
				if (wo && wo->get_desc()->is_noise_barrier()) {
					make_public = false;
				}

				const roadsign_t* rs = bd->find<roadsign_t>();
				if (rs && rs->get_desc()->is_private_way()) {
					make_public = false;
				}
				if (make_public) {
					strasse_t *str = static_cast<strasse_t *>(weg);
					str->set_gehweg(true);
					weg->set_public_right_of_way();
				}
			}
		} else {
			weg_t *weg = new strasse_t();
			welt->set_recheck_road_connexions();
			// Hajo: city roads should not belong to any player => so we can ignore any construction costs ...
			weg->set_desc(welt->get_city_road());
			strasse_t *str = static_cast<strasse_t *>(weg);
			str->set_gehweg(true);
			str->set_overtaking_mode(twoway_mode);
			weg->set_public_right_of_way();
			bd->neuen_weg_bauen(weg, connection_roads, player_);
			bd->calc_image();
		}
		// check to bridge a river, railway, etc.
		if(ribi_t::is_single(connection_roads)) {
			ribi_t::ribi direction = ribi_t::backward(connection_roads);
			koord zv = koord(direction);
			grund_t *bd_next = welt->lookup_kartenboden( k + zv );
			if(  bd_next &&
			     (bd_next->is_water() || bd_next->hat_weg(water_wt) || bd_next->hat_weg(track_wt) || bd_next->hat_weg(narrowgauge_wt) ||
			       bd_next->hat_weg(monorail_wt) || bd_next->hat_weg(maglev_wt) || (bd_next->hat_weg(road_wt) && !bd_next->get_weg(road_wt)->is_public_right_of_way()))) {
				// There is a river, a canal, railway, a private road, or a lake in the way. Build a bridge.
				build_bridge(bd, direction, map_generation);
			}
		}
		return true;
	}

	return false;
}


/**
 * Enlarge a city by building another building or extending a road.
 */
void stadt_t::build(bool new_town, bool map_generation)
{
	if(welt->get_settings().get_quick_city_growth())
	{
		// Old system (from Standard) - faster but less accurate.

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
			if (maybe_build_road(k, map_generation)) {
				INT_CHECK("simcity 5095");
				return;
			}
			// not good for road => test for house

			// since only a single location is checked, we can stop after we have found a positive rule
			best_haus.reset(k);
			const uint32 num_house_rules = house_rules.get_count();
			uint32 offset = simrand(num_house_rules, "void stadt_t::build");	// start with random rule
			for(  uint32 i = 0;  i < num_house_rules  &&  !best_haus.found();  i++  ) {
				uint32 rule = ( i+offset ) % num_house_rules;
				bewerte_haus(k, 8 + house_rules[rule]->distribution_weight, *house_rules[rule]);
			}
			// one rule applied?
			if(  best_haus.found()  ) {
				build_city_building(best_haus.get_pos(), new_town, map_generation);
				INT_CHECK("simcity 5112");
				return;
			}

		}
	}

	// renovation
	koord c( (ur.x + lo.x)/2 , (ur.y + lo.y)/2);
	uint32 maxdist(koord_distance(ur,c));
	if (maxdist < 10) {maxdist = 10;}
	uint32 was_renovated=0;
	uint32 try_nr = 0;
	if (  !buildings.empty()  &&  simrand(100, "void stadt_t::build") <= renovation_percentage  ) {
		while (was_renovated < renovations_count && try_nr++ < renovations_try) { // trial and errors parameters
			// try to find a non-player owned building
			gebaeude_t* const gb = pick_any(buildings);
			const uint32 dist(koord_distance(c, gb->get_pos()));
			const uint32 distance_rate = 100 - (dist * 100) / maxdist;
			if(  player_t::check_owner(gb->get_owner(),NULL)  && simrand(100, "void stadt_t::build") < distance_rate) {
				if(renovate_city_building(gb, map_generation)) {
					was_renovated++;
				}
			}
		}
		INT_CHECK("simcity 5134");
	}
	if (was_renovated) {
		return;
	}
	if (welt->get_settings().get_quick_city_growth()) {
		return;
	}

	int num_enlarge_tries = 4;
	do {

		// firstly, determine all potential candidate coordinates
		vector_tpl<koord> candidates( (ur.x - lo.x + 1) * (ur.y - lo.y + 1) );
		for(  sint16 j=lo.y;  j<=ur.y;  ++j  ) {
			for(  sint16 i=lo.x;  i<=ur.x;  ++i  ) {
				const koord k(i, j);
				// do not build on any border tile
				if(  !welt->is_within_limits( k+koord(1,1) )  ||  k.x<=0  ||  k.y<=0  ) {
					continue;
				}

				// checks only make sense on empty ground
				const grund_t *const gr = welt->lookup_kartenboden(k);
				if(  gr==NULL  ||  !gr->ist_natur()  ) {
					continue;
				}

				// a potential candidate coordinate
				candidates.append(k);
			}
		}

		// loop until all candidates are exhausted or until we find a suitable location to build road or city building
		while(  candidates.get_count()>0  ) {
			const uint32 idx = simrand( candidates.get_count(), "void stadt_t::build" );
			const koord k = candidates[idx];

			if (maybe_build_road(k, map_generation)) {
				INT_CHECK("simcity 5095");
				return;
			}
			// not good for road => test for house

			// we can stop after we have found a positive rule
			best_haus.reset(k);
			const uint32 num_house_rules = house_rules.get_count();
			uint32 offset = simrand(num_house_rules, "void stadt_t::build");	// start with random rule
			for (uint32 i = 0; i < num_house_rules  &&  !best_haus.found(); i++) {
				uint32 rule = ( i+offset ) % num_house_rules;
				bewerte_haus(k, 8 + house_rules[rule]->distribution_weight, *house_rules[rule]);
			}
			// one rule applied?
			if (best_haus.found()) {
				build_city_building(best_haus.get_pos(), new_town, map_generation);
				INT_CHECK("simcity 5192");
				return;
			}

			candidates.remove_at(idx, false);
		}
		// Oooh.  We tried every candidate location and we couldn't build.
		// (Admittedly, this may be because percentage-distribution_weight rules told us not to.)
		// Anyway, if this happened, enlarge the city limits and try again.
		bool could_enlarge = enlarge_city_borders();
		if (!could_enlarge) {
			// Oh boy.  It's not possible to enlarge.  Seriously?
			// I guess we'd better try merging this city into a neighbor (not implemented yet).
			num_enlarge_tries = 0;
		} else {
			num_enlarge_tries--;
		}
	} while (num_enlarge_tries > 0);
	return;
}

// find suitable places for cities
vector_tpl<koord>* stadt_t::random_place(const karte_t* wl, const vector_tpl<sint32> *sizes_list, sint16 old_x, sint16 old_y)
{
	unsigned number_of_clusters = env_t::number_of_clusters;
	unsigned cluster_size = env_t::cluster_size;
	const int grid_step = 8;
	const double distance_scale = 1.0/(grid_step * 2);
	const double population_scale = 1.0/1024;
	double water_charge = sqrt(env_t::cities_like_water/100.0); // should be from 0 to 1.0
	double water_part;
	double terrain_part;
	if (!env_t::cities_ignore_height) {
		terrain_part = 1.0 - water_charge; water_part = water_charge;
	}
	else{
		terrain_part = 0.0; water_part = 1.0;
	}


	double one_population_charge = 1.0 + wl->get_settings().get_city_isolation_factor()/10.0; // should be > 1.0
	double clustering = 2.0 + cluster_size/100.0; // should be > 2.0


	vector_tpl<koord>* result = new vector_tpl<koord>(sizes_list->get_count());

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
	unsigned int weight_max;
	// unsigned long here -- from weighted_vector_tpl.h(weight field type)
	if ( list->get_count() == 0 || (std::numeric_limits<unsigned long>::max)()/ list->get_count() > 65535) {
		weight_max = 65535;
	}
	else {
		weight_max = (std::numeric_limits<unsigned long>::max)()/list->get_count();
	}

	koord wl_size = welt->get_size();
	// max 1 city from each square can be built
	// each entry represents a cell of grid_step length and width
	const int xmax = wl_size.x/grid_step + 1;
	const int ymax = wl_size.y/grid_step + 1;

	array2d_tpl< vector_tpl<koord> > places(xmax, ymax);
	while (!list->empty()) {
		const koord k = list->remove_first();
		places.at( k.x/grid_step, k.y/grid_step).append(k);
	}

	/* Water
	 */
	array2d_tpl<double> water_field(xmax, ymax);
	//calculate distance to nearest river/sea
	array2d_tpl<double> water_distance(xmax, ymax);
	for (int y = 0; y < ymax; y++) {
		for (int x = 0; x < xmax; x++) {
			water_distance.at(x,y) = (std::numeric_limits<double>::max)();
		}
	}
	koord pos;
	//skip edges -- they treated as water, we don't want it
	for( pos.y = 2; pos.y < wl_size.y-2; pos.y++) {
		for (pos.x = 2; pos.x < wl_size.x-2; pos.x++ ) {
			koord my_grid_pos(pos.x/grid_step, pos.y/grid_step);
			grund_t *gr = wl->lookup_kartenboden(pos);
			if ( gr->get_hoehe() <= wl->get_groundwater()  || ( gr->hat_weg(water_wt) && gr->get_max_speed() )  ) {
				koord dpos;
				for ( dpos.y = -4; dpos.y < 5; dpos.y++) {
					for ( dpos.x = -4; dpos.x < 5 ; dpos.x++) {
						koord neighbour_grid_pos = my_grid_pos + dpos;
					if ( neighbour_grid_pos.x >= 0 && neighbour_grid_pos.y >= 0 &&
						 neighbour_grid_pos.x < xmax && neighbour_grid_pos.y < ymax  ) {
							koord neighbour_center(neighbour_grid_pos.x*grid_step + grid_step/2, neighbour_grid_pos.y*grid_step + grid_step/2);
							double distance =  koord_distance(pos,neighbour_center) * distance_scale;
							if ( water_distance.at(neighbour_grid_pos) > distance ) {
								water_distance.at(neighbour_grid_pos) = distance;
							}
						}
					}
				}
			}
		}
	}

	//now calculate water attraction field
	for (int y = 0; y < ymax; y++) {
		for (int x = 0; x < xmax; x++) {
			double distance = water_distance.at(x, y);
			double f;
			//we want city near water, but not too near
			if ( distance <= 1.0/4.0) {
				f = -1.0;
			}
			else {
				f = water_charge/(distance*distance)-water_charge;
			}
			water_field.at(x,y) = f;
		}
	}
#ifdef DEBUG_WEIGHTMAPS
	dbg_weightmap(water_field, places, weight_max, "water_", 0);
#endif

	/* Terrain
	 */
	array2d_tpl<double> terrain_field(xmax, ymax);
	for (int y = 0; y < ymax; y++) {
		for (int x = 0; x < xmax; x++) {
			terrain_field.at(x,y) = 0.0;
		}
	}

	for ( pos.y = 1; pos.y < wl_size.y; pos.y++) {
		for (pos.x = 1; pos.x < wl_size.x; pos.x++) {
			double f;
			if (env_t::cities_ignore_height) {
				f = 0.0;
			}
			else {
				int weight;
				const sint16 height_above_water = wl->lookup_hgt(pos) - wl->get_groundwater();
				switch(height_above_water)
				{
					case 1: weight = 24; break;
					case 2: weight = 22; break;
					case 3: weight = 16; break;
					case 4: weight = 12; break;
					case 5: weight = 10; break;
					case 6: weight = 9; break;
					case 7: weight = 8; break;
					case 8: weight = 7; break;
					case 9: weight = 6; break;
					case 10: weight = 5; break;
					case 11: weight = 4; break;
					case 12: weight = 3; break;
					case 13: weight = 3; break;
					case 14: weight = 2; break;
					case 15: weight = 2; break;
					default: weight = 1;
				}
				f = weight/12.0 - 1.0;
			}
			koord grid_pos(pos.x/grid_step, pos.y/grid_step);
			terrain_field.at(grid_pos) += f/(grid_step*grid_step);
		}
	}
#ifdef DEBUG_WEIGHTMAPS
	dbg_weightmap(terrain_field, places, weight_max, "terrain_", 0);
#endif



	weighted_vector_tpl<koord> index_to_places(xmax*ymax);
	array2d_tpl<double> isolation_field(xmax, ymax);
	array2d_tpl<bool> cluster_field(xmax, ymax);
	for (int y = 0; y < ymax; y++) {
		for (int x = 0; x < xmax; x++) {
			isolation_field.at(x,y) = 0.0;
			cluster_field.at(x,y) = (number_of_clusters == 0);
		}
	}
	array2d_tpl<double> total_field(xmax, ymax);

	for (unsigned int city_nr = 0; city_nr < sizes_list->get_count(); city_nr++) {
		//calculate summary field
		double population = (*sizes_list)[city_nr] * population_scale;
		if (population < 1.0) { population = 1.0; };
		double population_charge = sqrt( population * one_population_charge);

		for (int y = 0; y < ymax; y++) {
			for (int x = 0; x < xmax; x++) {
				double f = water_part * water_field.at(x,y) + terrain_part*terrain_field.at(x,y)- isolation_field.at(x,y) * population_charge;
				if(city_nr >= number_of_clusters && !cluster_field.at(x,y)) {
					f = -1.0;
				}
				total_field.at(x,y) = f;
			}
		}
#ifdef DEBUG_WEIGHTMAPS
		dbg_weightmap(total_field, places, weight_max, "total_", city_nr);
#endif
		// translate field to weigthed vector
		index_to_places.clear();
		for(int y=0; y<ymax; y++) {
			for(int x=0; x<xmax; x++) {
				if (places.at(x,y).empty()) continue; // (*)
				double f= total_field.at(x,y);
				if ( f > 1.0 ) {
					f = 1.0;
				}
				else if ( f < -1.0 ) {
					f = -1.0;
				}
				int weight(weight_max*(f + 1.0) /2.0);
				if (weight) {
					index_to_places.append( koord(x,y), weight);
				}
			}
		}

		if (index_to_places.empty() ) {
			if(city_nr < sizes_list->get_count() - 1) {
				char buf[256];
				if(number_of_clusters > 0) {
					sprintf(buf, /*256,*/ translator::translate("City generation: only %i cities could be placed inside clusters.\n"), city_nr);
					wl->get_message()->add_message(buf,koord::invalid,message_t::city,COL_GROWTH);
					for (int y = 0; y < ymax; y++) {
						for (int x = 0; x < xmax; x++) {
							cluster_field.at(x,y) = true;
						}
					}
					number_of_clusters = 0;
					city_nr--;
					continue;
				}
				sprintf(buf, /*256,*/ translator::translate("City generation: not enough places found for cities. Only %i cities generated.\n"), city_nr);
				wl->get_message()->add_message(buf,koord::invalid,message_t::city,COL_GROWTH);
				dbg->warning("stadt_t::random_place()", "Not enough places found for cities.");
			}
			break;
		}

		// find a random cell
		const uint32 weight = simrand(index_to_places.get_sum_weight(), "vector_tpl<koord>* stadt_t::random_place");
		const koord ip = index_to_places.at_weight(weight);
		// get random place in the cell
		const uint32 j = simrand(places.at(ip).get_count(), "vector_tpl<koord>* stadt_t::random_place");
		// places.at(ip) can't be empty (see (*) above )
		const koord k = places.at(ip)[j];
		places.at(ip).remove_at(j);
		result->append(k);

		// now update fields
		for (int y = 0; y < ymax; y++) {
			for (int x = 0; x < xmax; x++) {
				const koord central_pos(x * grid_step + grid_step/2, y * grid_step+grid_step/2);
				if (central_pos == k) {
					isolation_field.at(x,y) = 1.0;
				}
				else
				{
					const double distance = shortest_distance(k, central_pos) * distance_scale;
					isolation_field.at(x,y) += population_charge/(distance*distance);
					if (city_nr < number_of_clusters && distance < clustering*population_charge) {
						cluster_field.at(x,y) = true;
					}
				}
			}
		}
	}
	delete list;
	return result;
}

uint32 stadt_t::get_power_demand() const
{
	// Assumed 1kW for every 1 unit of population + 1 job + 4 visitor demand units at 100% in electricity.tab
	const uint32 electricity_per_unit = (uint32)get_electricity_consumption(welt->get_timeline_year_month());
	// The weird order of operations is designed for greater precision.
	// Really, POWER_TO_MW should come last.

	return (((city_history_month[0][HIST_CITICENS] + city_history_month[0][HIST_JOBS] + (city_history_month[0][HIST_VISITOR_DEMAND] / 4)) << POWER_TO_MW) * electricity_per_unit) / 100000;
}

void stadt_t::add_substation(senke_t* substation)
{
	substations.append_unique(substation);
}

void stadt_t::remove_substation(senke_t* substation)
{
	substations.remove(substation);
}

private_car_destination_finder_t::private_car_destination_finder_t(karte_t* w, road_vehicle_t* m, stadt_t* o)
{
	welt = w;
	master = m;
	origin_city = o;
	last_tile_speed = 0;
	last_tile_cost_diagonal = 0;
	last_tile_cost_straight = 0;
	last_city = NULL;
	meters_per_tile_x100 = welt->get_settings().get_meters_per_tile() * 100; // For 100ths of a minute
}

bool private_car_destination_finder_t::check_next_tile(const grund_t* gr) const
{
	// Check to see whether the road prohibits private cars
	if(gr)
	{
		const strasse_t* const str = (strasse_t*)gr->get_weg(road_wt);
		if(str)
		{
			const player_t *player = str->get_owner();
			if(player != NULL && player->get_player_nr() != 1 && !player->allows_access_to(1))
			{
				// Private cas should have the same restrictions as to the roads on which to travel
				// as players' vehicles.
				return false;
			}

			if(str->has_sign())
			{
				const roadsign_t* rs = gr->find<roadsign_t>();
				const roadsign_desc_t* rs_desc = rs->get_desc();
				if(rs_desc->get_min_speed() > welt->get_citycar_speed_average() || (rs_desc->is_private_way() && (rs->get_player_mask() & 2) == 0))
				{
					return false;
				}
			}
		}
	}
	return master->check_next_tile(gr);
}

ribi_t::ribi private_car_destination_finder_t::get_ribi(const grund_t* gr) const
{
	return master->get_ribi(gr);
}

bool private_car_destination_finder_t:: is_target(const grund_t* gr, const grund_t*)
{
	if(!gr)
	{
		return false;
	}

	const koord k = gr->get_pos().get_2d();
	const stadt_t* city = welt->access(k)->get_city();

	if(city && city != origin_city && city->get_townhall_road() == k)
	{
		// We use a different system for determining travel speeds in the current city.

		return true;
	}

	const strasse_t* str = (strasse_t*)gr->get_weg(road_wt);
	if(str->connected_buildings.get_count() > 0)
	{
		return true;
	}

	return false;
}

int private_car_destination_finder_t::get_cost(const grund_t* gr, sint32 max_speed, koord from_pos)
{
	const weg_t *w = gr->get_weg(road_wt);
	if(!w)
	{
		return 0xFFFF;
	}

	const sint32 max_tile_speed = w->get_max_speed(); // This returns speed in km/h.
	const planquadrat_t* plan = welt->access_nocheck(gr->get_pos().get_2d());
	const stadt_t* city = plan->get_city();
	const bool is_diagonal = w->is_diagonal();

	if(city == last_city && max_tile_speed == last_tile_speed)
	{
		// Need not redo the whole calculation if nothing has changed.
		if(is_diagonal && last_tile_cost_diagonal > 0)
		{
			return last_tile_cost_diagonal;
		}
		else if(last_tile_cost_straight > 0)
		{
			return last_tile_cost_straight;
		}
	}

	last_city = city;
	last_tile_speed = max_tile_speed;

	sint32 speed = min(max_speed, max_tile_speed);
#ifndef FORBID_CONGESTION_EFFECTS
	if(city)
	{
		// If this is in a city, take account of congestion when calculating
		// the speed.

		// Congestion here is assumed to be on the percentage basis: i.e. the percentage of extra time that
		// a journey takes owing to congestion. This is the measure used by the TomTom congestion index,
		// compiled by the satellite navigation company of that name, which provides useful research data.
		// See: http://www.tomtom.com/lib/doc/congestionindex/2012-0704-TomTom%20Congestion-index-2012Q1europe-mi.pdf

		const sint32 congestion = (sint32)city->get_congestion() + 100;
		speed = (speed * 100) / congestion;
		speed = max(4, speed);
	}
#endif
	// Time = distance / speed
	int mpt;

	if(is_diagonal)
	{
		// Diagonals are a *shorter* distance.
		mpt = ((int)meters_per_tile_x100 * 5) / 7;
	}
	else
	{
		mpt = (int)meters_per_tile_x100;
	}

	// T = d / (1000 / h)
	// T = d / (h * 1000)
	// T = d / ((h / 60) * (1000 / 60)
	// m == h / 60
	// T = d / (m * 16.67)
	// (m / 100)
	// T = d / ((m / 100) * (16.67 / 100)
	// T = d / ((m / 100) * 0.167)
	// T = (d * 100) / (m * 16.67) -- 100THS OF A MINUTE PER TILE

	const int cost = mpt / ((speed * 167) / 10);

	if(is_diagonal)
	{
		last_tile_cost_diagonal = cost;
	}
	else
	{
		last_tile_cost_straight = cost;
	}
	return cost;
}

void stadt_t::remove_connected_city(stadt_t* city)
{
	if(city)
	{
		connected_cities.remove(city->get_pos());
	}
}


void stadt_t::remove_connected_industry(fabrik_t* fab)
{
	connected_industries.remove(fab->get_pos().get_2d());
}

void stadt_t::remove_connected_attraction(gebaeude_t* attraction)
{
	connected_attractions.remove(attraction->get_pos().get_2d());
}

double stadt_t::get_land_area() const
{
	const uint16 x_dimension = ur.x - lo.x;
	const uint16 y_dimension = ur.y - lo.y;
	const uint16 meters_per_tile = welt->get_settings().get_meters_per_tile();
	const uint16 x_dimension_meters = x_dimension * meters_per_tile;
	const uint16 y_dimension_meters = y_dimension * meters_per_tile;
	const uint32 area_square_meters = (uint32)x_dimension_meters * (uint32)y_dimension_meters;
	const double area_square_km = (double)area_square_meters / 1000000.0;
	return area_square_km;
}

void stadt_t::add_city_factory(fabrik_t *fab)
{
	update_city_stats_with_building(fab->get_building()->access_first_tile(), false);
	city_factories.append_unique(fab);
}

void stadt_t::remove_city_factory(fabrik_t *fab)
{
	update_city_stats_with_building(fab->get_building()->access_first_tile(), true);
	city_factories.remove(fab);
}
