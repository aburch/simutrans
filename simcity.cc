/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
#include "simwin.h"
#include "simworld.h"
#include "simware.h"
#include "player/simplay.h"
#include "simplan.h"
#include "simimg.h"
#include "simtools.h"
#include "simhalt.h"
#include "simfab.h"
#include "simcity.h"
#include "simmesg.h"
#include "simcolor.h"

#include "gui/karte.h"
#include "gui/stadt_info.h"

#include "besch/haus_besch.h"
#include "besch/stadtauto_besch.h"

#include "simintr.h"
#include "simdebug.h"

#include "dings/gebaeude.h"

#include "dataobj/translator.h"
#include "dataobj/einstellungen.h"
#include "dataobj/loadsave.h"
#include "dataobj/tabfile.h"
#include "dataobj/umgebung.h"

#include "dings/roadsign.h"
#include "dings/leitung2.h"

#include "sucher/bauplatz_sucher.h"
#include "bauer/warenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/fabrikbauer.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"
#ifdef DEBUG_WEIGHTMAPS
#include "utils/dbg_weightmap.h"
#endif

#include "tpl/minivec_tpl.h"

karte_t* stadt_t::welt = NULL; // one is enough ...

// Private car ownership information.
// @author: jamespetts
// (But much of this code is adapted from the speed bonus code,
// written by Prissi). 

class car_ownership_record_t 
{
public:
	sint64 year;
	sint16 ownership_percent;
	car_ownership_record_t( sint64 y = 0, sint16 ownership = 0 ) 
	{
		year = y*12;
		ownership_percent = ownership;
	};
};

static sint16 default_car_ownership_percent = 25;

static vector_tpl<car_ownership_record_t> car_ownership[1];

void stadt_t::privatecar_init(const std::string &objfilename)
{
	tabfile_t ownership_file;
	// first take user data, then user global data
	if (!ownership_file.open((objfilename+"config/privatecar.tab").c_str()))
	{
		dbg->message("stadt_t::privatecar_init()", "Error opening config/privatecar.tab.\nWill use default value." );
		return;
	}

	tabfileobj_t contents;
	ownership_file.read(contents);

	/* init the values from line with the form year, proportion, year, proportion
	 * must be increasing order!
	 */
	int *tracks = contents.get_ints("car_ownership");
	if((tracks[0]&1)==1) 
	{
		dbg->message("stadt_t::privatecar_init()", "Ill formed line in config/privatecar.tab.\nWill use default value. Format is year,ownership percentage[ year,ownership percentage]!" );
		car_ownership->clear();
		return;
	}
	car_ownership[0].resize( tracks[0]/2 );
	for(  int i=1;  i<tracks[0];  i+=2  ) 
	{
		car_ownership_record_t c( tracks[i], tracks[i+1] );
		car_ownership[0].append( c );
	}
	delete [] tracks;
}


/**
* Reads/writes private car ownership data from/to a savegame
* called from karte_t::speichern and karte_t::laden
* only written for networkgames
* @author jamespetts
*/
void stadt_t::privatecar_rdwr(loadsave_t *file)
{
	if(file->get_experimental_version() < 9)
	{
		 return;
	}

	if(file->is_saving())
	{
		uint32 count = car_ownership[0].get_count();
		file->rdwr_long(count);
		ITERATE(car_ownership[0], i)
		{
			
			file->rdwr_longlong(car_ownership[0].get_element(i).year);
			file->rdwr_short(car_ownership[0].get_element(i).ownership_percent);
		}	
	}

	else
	{
		car_ownership->clear();
		uint32 counter;
		file->rdwr_long(counter);
		sint64 year = 0;
		uint16 ownership_percent = 0;
		for(uint32 c = 0; c < counter; c ++)
		{
			file->rdwr_longlong(year);
			file->rdwr_short(ownership_percent);
			car_ownership_record_t cow(year / 12, ownership_percent);
			car_ownership[0].append( cow );
		}
	}
}

sint16 stadt_t::get_private_car_ownership(sint32 monthyear)
{

	if(monthyear == 0) 
	{
		return default_car_ownership_percent;
	}

	// ok, now lets see if we have data for this
	if(car_ownership->get_count()) 
	{
		uint i=0;
		while(  i<car_ownership->get_count()  &&  monthyear>=car_ownership[0][i].year  ) {
			i++;
		}
		if(  i==car_ownership->get_count()  ) 
		{
			// maxspeed already?
			return car_ownership[0][i-1].ownership_percent;
		}
		else if(i==0) 
		{
			// minspeed below
			return car_ownership[0][0].ownership_percent;
		}
		else 
		{
			// interpolate linear
			const sint32 delta_ownership_percent = car_ownership[0][i].ownership_percent - car_ownership[0][i-1].ownership_percent;
			const sint64 delta_years = car_ownership[0][i].year - car_ownership[0][i-1].year;
			return ( (delta_ownership_percent*(monthyear-car_ownership[0][i-1].year)) / delta_years ) + car_ownership[0][i-1].ownership_percent;
		}
	}
	else
	{
		return default_car_ownership_percent;
	}
}

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
* called from karte_t::speichern and karte_t::laden
* only written for network games
* @author jamespetts
*/
void stadt_t::electricity_consumption_rdwr(loadsave_t *file)
{
	if(file->get_experimental_version() < 9)
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
		while(  i<electricity_consumption->get_count()  &&  monthyear>=electricity_consumption[0][i].year  ) 
		{
			i++;
		}
		if(  i==electricity_consumption->get_count()  ) 
		{
			// past final year
			return electricity_consumption[0][i-1].consumption_percent;
		}
		else if(i==0) 
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
const uint32 stadt_t::step_bau_interval = 21000;


/*
 * chance to do renovation instead new building (in percent)
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
//		flag |= gr->get_typ() == grund_t::fundament ? (gr->obj_bei(0)->get_typ()==ding_t::gebaeude ? rule_is_house :0) : rule_no_house;
//		if (gr->ist_natur() && gr->kann_alle_obj_entfernen(NULL)== NULL) {
//			flag |= rule_is_natur;
//		}
//		flag |= gr->is_halt() ? rule_is_stop : rule_no_stop;
//		flag |= hang_t::ist_wegbar(gr->get_grund_hang()) ? rule_good_slope : rule_bad_slope;
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
//bool stadt_t::bewerte_loc(const koord pos, rule_t &regel, uint16 rotation)
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
//						if (gr->get_typ() != grund_t::fundament  ||  gr->obj_bei(0)->get_typ()!=ding_t::gebaeude) return false;
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
// 						if (!hang_t::ist_wegbar(gr->get_grund_hang())) return false;
// 						break;
//
// 					case rule_good_slope:
// 						// road may be buildable
// 						if (hang_t::ist_wegbar(gr->get_grund_hang())) return false;
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
bool stadt_t::bewerte_loc(const koord pos, rule_t &regel, int rotation)
{
	//printf("Test for (%s) in rotation %d\n", pos.get_str(), rotation);
	koord k;

	for(uint32 i=0; i<regel.rule.get_count(); i++){
		rule_entry_t &r = regel.rule[i];
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
	//printf("Success\n");
	return true;
}


/**
 * Check rule in all transformations at given position
 * prissi: but the rules should explicitly forbid building then?!?
 * @author Hj. Malthaner
 */

sint32 stadt_t::bewerte_pos(const koord pos, rule_t &regel)

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


void stadt_t::bewerte_strasse(koord k, sint32 rd, rule_t &regel)
{
	if (simrand(rd, "void stadt_t::bewerte_strasse") == 0) {
		best_strasse.check(k, bewerte_pos(k, regel));
	}
}


void stadt_t::bewerte_haus(koord k, sint32 rd, rule_t &regel)
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

	renovation_percentage = (uint32)contents.get_int("renovation_percentage", renovation_percentage);
	renovations_count = (uint32)contents.get_int("renovations_count", renovations_count);
	renovations_try   = (uint32)contents.get_int("renovations_try", renovations_try);

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

	house_rules.clear();
	for (uint32 i = 0; i < num_house_rules; i++) {
		house_rules.append(new rule_t());
		sprintf(buf, "house_%u.chance", i + 1);
		house_rules[i]->chance = contents.get_int(buf, 0);

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
		printf("House-Rule %d: chance %d\n",i,house_rules[i]->chance);
		for(uint32 j=0; j< house_rules[i]->rule.get_count(); j++)
			printf("House-Rule %d: Pos (%d,%d) Flag %d\n",i,house_rules[i]->rule[j].x,house_rules[i]->rule[j].y,house_rules[i]->rule[j].flag);
	}

	road_rules.clear();
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
		printf("Road-Rule %d: chance %d\n",i,road_rules[i]->chance);
		for(uint32 j=0; j< road_rules[i]->rule.get_count(); j++)
			printf("Road-Rule %d: Pos (%d,%d) Flag %d\n",i,road_rules[i]->rule[j].x,road_rules[i]->rule[j].y,road_rules[i]->rule[j].flag);
		
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
	if(file->get_experimental_version() < 9)
	{
		 return;
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
		house_rules.clear();
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
		road_rules.clear();
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
				if (gebaeude_t* const add_gb = ding_cast<gebaeude_t>(welt->lookup_kartenboden(pos + k)->first_obj())) {
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

bool stadt_t::is_within_city_limits(koord k) const
{
	const sint16 li_gr = lo.x - 2;
	const sint16 re_gr = ur.x + 2;
	const sint16 ob_gr = lo.y - 2;
	const sint16 un_gr = ur.y + 2;

	bool inside = li_gr < k.x  &&  re_gr > k.x  &&  ob_gr < k.y  &&  un_gr > k.y;
	return inside;
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
	//assert( factory );
}


const stadt_t::factory_entry_t* stadt_t::factory_set_t::get_entry(const fabrik_t *const factory) const
{
	for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
		if(  entries[e].factory==factory  ) {
			return &entries[e];
		}
	}
	return NULL;	// not found
}


stadt_t::factory_entry_t* stadt_t::factory_set_t::get_random_entry()
{
	if(  total_remaining>0  ) {
		sint32 weight = simrand(total_remaining, "stadt_t::factory_set_t::get_random_entry()");
		for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
			factory_entry_t &entry = entries[e];
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
			for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
				factory_entry_t &entry = entries[e];
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
	for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
		factory_entry_t &entry = entries[e];
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
	for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
		entries[e].new_month();
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
	for(  uint32 e=0;  e<entries.get_count();  ++e  ) {
		entries[e].resolve_factory();
	}
}


stadt_t::~stadt_t()
{
	// close info win
	destroy_win((long)this);

	// Empty the list of city cars
	current_cars.clear();

	// Remove references to this city from factories.
	ITERATE(city_factories, i)
	{
		city_factories[i]->clear_city();
	}

	delete finder;
	delete private_car_route;

	if(  reliefkarte_t::get_karte()->get_city() == this  ) {
		reliefkarte_t::get_karte()->set_city(NULL);
	}

	// only if there is still a world left to delete from
	if(welt->get_groesse_x()>1) 
	{

		welt->lookup_kartenboden(pos)->set_text(NULL);

		// remove city info and houses
		while (!buildings.empty()) 
		{
			// old buildings are not where they think they are, so we ask for map floor
			gebaeude_t* const gb = buildings.front();
			buildings.remove(gb);
			assert(  gb!=NULL  &&  !buildings.is_contained(gb)  );
			if(gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::firmensitz)
			{
				stadt_t *city = welt->suche_naechste_stadt(gb->get_pos().get_2d());
				gb->set_stadt( city );
				if(city) 
				{
					city->buildings.append(gb,gb->get_passagier_level(),16);
				}
			}
			else 
			{
				gb->set_stadt( NULL );
				hausbauer_t::remove(welt,welt->get_spieler(1),gb);
			}
		}
		// Remove substations
		ITERATE(substations, i)
		{
			substations[i]->city = NULL;
		}
		
		if(!welt->get_is_shutting_down())
		{
			koordhashtable_iterator_tpl<koord, uint16> iter(connected_cities);
			while(iter.next())
			{
				if(iter.get_current_key() == pos)
				{
					continue;
				}
				remove_connected_city(welt->get_city(iter.get_current_key()));
			}
		}
	}
	free( (void *)name );
}


stadt_t::stadt_t(spieler_t* sp, koord pos, sint32 citizens) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
{
	welt = sp->get_welt();
	assert(welt->ist_in_kartengrenzen(pos));

	if(welt->get_settings().get_quick_city_growth())
	{
		// If "quick_city_growth" is enabled, the renovation percentage
		// needs to be lower to make up for it.
		renovation_percentage /= 3;
	}

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
	int start_cont = simrand(name_list_count, "stadt_t::stadt_t");

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

	outgoing_private_cars = 0;
	incoming_private_cars = 0;

	city_history_month[0][HIST_CAR_OWNERSHIP] = get_private_car_ownership(welt->get_timeline_year_month());
	city_history_year[0][HIST_CAR_OWNERSHIP] = get_private_car_ownership(welt->get_timeline_year_month());

	calc_internal_passengers();

	finder = new road_destination_finder_t(welt, new automobil_t(welt));
	private_car_route = new route_t();
	check_road_connexions = false;

	private_car_update_month = welt->step_next_private_car_update_month();

	number_of_cars = 0;

}


void stadt_t::calc_internal_passengers()
{
	//const uint32 median_town_size =welt->get_settings().get_mittlere_einwohnerzahl();
	const uint32 capital_threshold =welt->get_settings().get_capital_threshold_size();
	const uint32 city_threshold =welt->get_settings().get_city_threshold_size();
	uint16 internal_passenger_percentage;
	//if(city_history_month[0][HIST_CITICENS] >= (median_town_size * 2.5F))
	if(city_history_month[0][HIST_CITICENS] >= capital_threshold)
	{
		internal_passenger_percentage = 85;
	}
	//else if(city_history_month[0][HIST_CITICENS] <= median_town_size >> 1)
	else if(city_history_month[0][HIST_CITICENS] <= city_threshold)
	{
		internal_passenger_percentage = 33;
	}
	else
	{
		//float proportion = ((float)city_history_month[0][HIST_CITICENS] - (float)(median_town_size / 2.0F)) / (float)(median_town_size * 2.5F);
		uint16 proportion = ((city_history_month[0][HIST_CITICENS] - city_threshold) * 100) / capital_threshold;
		internal_passenger_percentage = (((proportion * 52) /100) + 33) / 100;
	}

	adjusted_passenger_routing_local_chance = (welt->get_settings().get_passenger_routing_local_chance() * internal_passenger_percentage) / 100;
}



stadt_t::stadt_t(karte_t* wl, loadsave_t* file) :
	buildings(16),
	pax_destinations_old(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE)),
	pax_destinations_new(koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE))
{
	welt = wl;
	
	if(welt->get_settings().get_quick_city_growth())
	{
		// If "quick_city_growth" is enabled, the renovation percentage
		// needs to be lower to make up for it.
		renovation_percentage /= 3;
	}

	step_count = 0;
	next_step = 0;
	step_interval = 1;
	next_bau_step = 0;
	has_low_density = false;

	wachstum = 0;
	name = NULL;
	stadtinfo_options = 3;

	// These things are not yet saved as part of the city's history,
	// as doing so would require reversioning saved games.
	
	incoming_private_cars = 0;
	outgoing_private_cars = 0;

	rdwr(file);

	calc_internal_passengers();

	finder = new road_destination_finder_t(welt, new automobil_t(welt));
	private_car_route = new route_t();
	check_road_connexions = false;
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
	else if (file->get_experimental_version() == 0)
	{
		// 99.17.0 extended city history
		// Experimental version 3 extended it further, so skip the last step.
		// For experimental versions *before* 3, power history was treated as congestion
		// (they are now separate), so that must be handled differently.
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY - 3; hist_type++) 
			{
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY - 3; hist_type++) 
			{
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long( stadtinfo_options);
	}
	else if(file->get_experimental_version() > 0 && (file->get_experimental_version() < 3|| file->get_experimental_version() == 0))
	{
		// Move congestion history to the correct place (shares with power received).
		
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY - 2; hist_type++) 
			{
				if(hist_type == HIST_POWER_RECIEVED)
				{
					city_history_year[year][HIST_POWER_RECIEVED] = 0;
					hist_type = HIST_CONGESTION;
				}
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY - 2; hist_type++) 
			{
				if(hist_type == HIST_POWER_RECIEVED)
				{
					city_history_month[month][HIST_POWER_RECIEVED] = 0;
					hist_type = HIST_CONGESTION;
				}
				file->rdwr_longlong(city_history_month[month][hist_type]);
			}
		}
		// save button settings for this town
		file->rdwr_long( stadtinfo_options);
	}
	else if(file->get_experimental_version() >= 3)
	{
		for (uint year = 0; year < MAX_CITY_HISTORY_YEARS; year++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) 
			{
				file->rdwr_longlong(city_history_year[year][hist_type]);
			}
		}
		for (uint month = 0; month < MAX_CITY_HISTORY_MONTHS; month++) 
		{
			for (uint hist_type = 0; hist_type < MAX_CITY_HISTORY; hist_type++) 
			{
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
	if(file->get_version()>102001 ) {
		file->rdwr_bool(allow_citygrowth);
	}
	else if(  file->is_loading()  ) {
		allow_citygrowth = true;
	}
	// save townhall road position
	if(file->get_version()>102002 && file->get_experimental_version() != 7 ) {
		townhall_road.rdwr(file);
	}
	else if(  file->is_loading()  ) {
		townhall_road = koord::invalid;
	}

	// data related to target factories
	target_factories_pax.rdwr( file );
	target_factories_mail.rdwr( file );

	if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{
		file->rdwr_bool(check_road_connexions);
		file->rdwr_byte(private_car_update_month);

		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(outgoing_private_cars);
		file->rdwr_long(incoming_private_cars);
	}

	if(file->is_saving() && file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{		
		uint16 time;
		koord k;
		uint32 count;

		count = connected_cities.get_count();
		file->rdwr_long(count);
		koordhashtable_iterator_tpl<koord, uint16> city_iter(connected_cities);
		while(city_iter.next())
		{
			time = city_iter.get_current_value();
			file->rdwr_short(time);
			k = city_iter.get_current_key();
			k.rdwr(file);
		}

		count = connected_industries.get_count();
		file->rdwr_long(count);
		koordhashtable_iterator_tpl<koord, uint16> industry_iter(connected_industries);
		while(industry_iter.next())
		{
			time = industry_iter.get_current_value();
			file->rdwr_short(time);
			k = industry_iter.get_current_key();
			k.rdwr(file);
		}

		count = connected_attractions.get_count();
		file->rdwr_long(count);
		koordhashtable_iterator_tpl<koord, uint16> attraction_iter(connected_attractions);
		while(attraction_iter.next())
		{
			time = attraction_iter.get_current_value();
			file->rdwr_short(time);
			k = attraction_iter.get_current_key();
			k.rdwr(file);
		}

		file->rdwr_long(number_of_cars);
	}

	if(file->is_loading()) 
	{
		// 08-Jan-03: Due to some bugs in the special buildings/town hall
		// placement code, li,re,ob,un could've gotten irregular values
		// If a game is loaded, the game might suffer from such an mistake
		// and we need to correct it here.
		DBG_MESSAGE("stadt_t::rdwr()", "borders (%i,%i) -> (%i,%i)", lo.x, lo.y, ur.x, ur.y);

		// recalculate borders
		recalc_city_size();	

		connected_cities.clear();
		connected_industries.clear();
		connected_attractions.clear();
		if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
		{		
			uint16 time;
			koord k;
			uint32 count;

			// Cities
			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				file->rdwr_short(time);
				k.rdwr(file);
				connected_cities.put(k, time);
			}

			// Industries
			
			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				file->rdwr_short(time);
				k.rdwr(file);
				connected_industries.put(k, time);
			}

			// Attractions
			
			file->rdwr_long(count);
			for(uint32 x = 0; x < count; x ++)
			{
				file->rdwr_short(time);
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
	if(name==NULL) {
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

	vector_tpl<koord> k_list(connected_cities.get_count());
	vector_tpl<uint16> f_list(connected_cities.get_count());
	koordhashtable_iterator_tpl<koord, uint16> iter1(connected_cities);
	while(iter1.next())
	{
		koord k = iter1.get_current_key();
		uint16 f  = connected_cities.remove(k);
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
	koordhashtable_iterator_tpl<koord, uint16> iter2(connected_industries);
	while(iter2.next())
	{
		koord k = iter2.get_current_key();
		uint16 f  = connected_industries.remove(k);
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
	koordhashtable_iterator_tpl<koord, uint16> iter3(connected_attractions);
	while(iter3.next())
	{
		koord k = iter3.get_current_key();
		uint16 f  = connected_attractions.remove(k);
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
	if(name==NULL  ||  strcmp(name,new_name)) {
		free( (void *)name );
		name = strdup( new_name );
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gr->set_text( new_name );
	}
	stadt_info_t *win = dynamic_cast<stadt_info_t*>(win_get_magic((long)this));
	if (win) {
		win->update_data();
	}
}



/* show city info dialoge
 * @author prissi
 */
void stadt_t::zeige_info(void)
{
	create_win( new stadt_info_t(this), w_info, (long)this );
}


/* calculates the factories which belongs to certain cities */
void stadt_t::verbinde_fabriken()
{
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "search factories near %s (center at %i,%i)", get_name(), pos.x, pos.y);
	assert( target_factories_pax.get_entries().empty() );
	assert( target_factories_mail.get_entries().empty() );

	ITERATE(welt->get_fab_list(), i)
	{
		fabrik_t* fab = welt->get_fab_list()[i];
		const uint32 count = fab->get_target_cities().get_count();
		settings_t const& s = welt->get_settings();
		if (count < s.get_factory_worker_maximum_towns() && accurate_distance(fab->get_pos().get_2d(), pos) < s.get_factory_worker_radius()) 
		{
			fab->add_target_city(this);
			if(fab->get_target_cities().get_count() >=welt->get_settings().get_factory_worker_maximum_towns())
			{
				break;
			}
		}
	}
	DBG_MESSAGE("stadt_t::verbinde_fabriken()", "is connected with %i/%i factories (total demand=%i/%i) for pax/mail.",
		target_factories_pax.get_entries().get_count(), target_factories_mail.get_entries().get_count(), target_factories_pax.total_demand, target_factories_mail.total_demand);
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
//				remove_city();
			bev = 1;
		}
		step_bau();
	}
	if(bev == 0)
	{ 
		// Cities will experience uncontrollable growth if bev == 0
		bev = 1;
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
		//outgoing_private_cars = 0;
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

	// Congestion figures for the year should be an average of the last 12 months.
	uint16 total_congestion = 0;
	for(int i = 0; i < 12; i ++)
	{
		total_congestion += city_history_month[i][HIST_CONGESTION];
	}
	
	city_history_year[0][HIST_CONGESTION] = total_congestion / 12;

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
	outgoing_private_cars = 0;
}


void stadt_t::neuer_monat(bool check) //"New month" (Google)
{
	swap<uint8>( pax_destinations_old, pax_destinations_new );
	pax_destinations_new.clear();
	pax_destinations_new_change = 0;

	if(bev == 0)
	{
		bev ++;
	}
	calc_internal_passengers();

	roll_history();
	target_factories_pax.new_month();
	target_factories_mail.new_month();
	settings_t const& s = welt->get_settings();
	target_factories_pax.recalc_generation_ratio( s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_PAS_GENERATED);
	target_factories_mail.recalc_generation_ratio(s.get_factory_worker_percentage(), *city_history_month, MAX_CITY_HISTORY, HIST_MAIL_GENERATED);
	update_target_cities();

	// Calculate the level of congestion.
	// Used in determining growth and passenger preferences.
	// From observations in game: anything < 2, not very congested.
	// Anything > 4, very congested.
	// @author: jamespetts
	
	const uint16 city_size = (ur.x - lo.x) * (ur.y - lo.y);
	const uint16 cars_per_tile_thousandths = (city_history_month[1][HIST_CITYCARS] * 1000) / city_size;
	const uint16 population_density = (city_history_month[1][HIST_CITICENS] * 10) / city_size;
	if(cars_per_tile_thousandths <= 400)
	{
		city_history_month[0][HIST_CONGESTION] = 0;
	}
	else
	{
		const uint8 congestion_density_factor =welt->get_settings().get_congestion_density_factor();
		const uint16 percentage = congestion_density_factor > 0 ? ((((cars_per_tile_thousandths - 400) / 45) * population_density) / congestion_density_factor) / 10 : (cars_per_tile_thousandths - 400) / 30;
		city_history_month[0][HIST_CONGESTION] = percentage;
	}

	city_history_month[0][HIST_CAR_OWNERSHIP] = get_private_car_ownership(welt->get_timeline_year_month());
	sint64 car_ownership_sum = 0;
	for(uint8 months = 0; months < MAX_CITY_HISTORY_MONTHS; months ++)
	{
		car_ownership_sum += city_history_month[months][HIST_CAR_OWNERSHIP];
	}
	city_history_year[0][HIST_CAR_OWNERSHIP] = car_ownership_sum / MAX_CITY_HISTORY_MONTHS;

	// Clearing these will force recalculation as necessary.
	// Cannot do this too often, as it severely impacts on performance.
	check_road_connexions = check;

	if(check_road_connexions && private_car_update_month == welt->get_current_month())
	{
		connected_cities.clear();
		connected_industries.clear();
		connected_attractions.clear();
		check_road_connexions = false;
	}
	
	if (!stadtauto_t::list_empty()) 
	{
		// Spawn citycars
		
		// Citycars now used as an accurate measure of actual traffic level, not just the number of cars generated
		// graphically on the map. Thus, the "traffic level" setting no longer has an effect on the city cars graph,
		// but still affects the number of actual cars generated. 
		
		// Divide by a factor because number of cars drawn on screen should be a fraction of actual traffic, or else
		// everywhere will become completely clogged with traffic. Linear rather than logorithmic scaling so
		// that the player can have a better idea visually of the amount of traffic.

		sint32 old_number_of_cars = number_of_cars;

#ifdef DESTINATION_CITYCARS 	
		//Manual assignment of traffic level modifiers, since I could not find a suitable mathematical formula.
		sint32 traffic_level;
		switch(welt->get_settings().get_verkehr_level())
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
			traffic_level = 670; 
			break;

		case 13:
			traffic_level = 750;
			break;

		case 14:
			traffic_level = 850;
			break;

		case 15:
			traffic_level = 900;
			break;

		case 16:
		default:
			traffic_level = 1000;
		};
		
		// Subtract incoming trips and cars already generated to prevent double counting.
		number_of_cars = ((city_history_month[1][HIST_CITYCARS] * traffic_level) / 1000) - (sint32)incoming_private_cars - (sint32)current_cars.get_count();
		incoming_private_cars = 0;
#else
		uint16 number_of_cars = ((city_history_month[1][HIST_CITYCARS] *welt->get_settings().get_verkehr_level()) / 16) / 64;
#endif

		while(!current_cars.empty() && number_of_cars < 0)
		{
			//Make sure that there are not too many cars on the roads. 
			stadtauto_t* car = current_cars.remove_first();
			car->kill();
			number_of_cars++;
		}
		koord k;
		koord pos = get_zufallspunkt();
		int retry_count = 5;
		while(old_number_of_cars > 0 && retry_count > 0)
		{
			for (k.y = pos.y - 3; k.y < pos.y + 3; k.y++) 
			{
				for (k.x = pos.x - 3; k.x < pos.x + 3; k.x++)
				{
					if(number_of_cars == 0)
					{
						return;
					}

					grund_t* gr = welt->lookup_kartenboden(k);
					if (gr != NULL && gr->get_weg(road_wt) && ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt)) && gr->find<stadtauto_t>() == NULL)
					{
						slist_tpl<stadtauto_t*> *car_list = &current_cars;
						stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), koord::invalid, car_list);
						gr->obj_add(vt);
						welt->sync_add(vt);
						current_cars.append(vt);
						number_of_cars--;
						old_number_of_cars--;
					}
				}
			}
			retry_count --;
		}
	}
}

sint32 stadt_t::get_outstanding_cars()
{
	return number_of_cars - current_cars.get_count();
}

void stadt_t::add_car(stadtauto_t* car)
{
	current_cars.append(car);
}


void stadt_t::calc_growth()
{
	// now iterate over all factories to get the ratio of producing version nonproducing factories
	// we use the incoming storage as a measure und we will only look for end consumers (power stations, markets)
	for(  vector_tpl<factory_entry_t>::const_iterator iter = target_factories_pax.get_entries().begin(), end = target_factories_pax.get_entries().end();  iter!=end;  ++iter  )
	{
		fabrik_t *const fab = iter->factory;
		if(fab && fab->get_lieferziele().empty() && !fab->get_suppliers().empty()) 
		{
			// consumer => check for it storage
			const fabrik_besch_t *const besch = fab->get_besch();
			for(  int i=0;  i<besch->get_lieferanten();  i++  ) 
			{
				city_history_month[0][HIST_GOODS_NEEDED] ++;
				city_history_year[0][HIST_GOODS_NEEDED] ++;
				if(  fab->input_vorrat_an( besch->get_lieferant(i)->get_ware() )>0  ) 
				{
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
	 * passenger transport 40%, mail 16%, goods 24%, and electricity 20% (by default: varies)
	 *
	 * Congestion detracts from growth, but towns can now grow as a result of private car
	 * transport as well as public transport: if private car ownership is high enough.
	 * (@author: jamespetts)
	 */

	sint64     const(& h)[MAX_CITY_HISTORY] = city_history_month[0];
	settings_t const&  s           = welt->get_settings();

	const uint8 electricity_multiplier = 20;
	// const uint8 electricity_multiplier =welt->get_settings().get_electricity_multiplier();
	const uint8 electricity_proportion = (get_electricity_consumption(welt->get_timeline_year_month()) * electricity_multiplier / 100);
	const uint8 mail_proportion = 100 - (s.get_passenger_multiplier() + electricity_proportion + s.get_goods_multiplier());

	const sint32 pas = ((city_history_month[0][HIST_PAS_TRANSPORTED] + (city_history_month[0][HIST_CITYCARS] - outgoing_private_cars)) * (s.get_passenger_multiplier()<<6)) / (city_history_month[0][HIST_PAS_GENERATED] + 1);
	const sint32 mail = (city_history_month[0][HIST_MAIL_TRANSPORTED] * (mail_proportion)<<6) / (city_history_month[0][HIST_MAIL_GENERATED] + 1);
	const sint32 electricity = city_history_month[0][HIST_POWER_NEEDED] == 0 ? 0 : (city_history_month[0][HIST_POWER_RECIEVED] * (electricity_proportion<<6)) / (city_history_month[0][HIST_POWER_NEEDED]);
	const sint32 goods = city_history_month[0][HIST_GOODS_NEEDED]==0 ? 0 : (city_history_month[0][HIST_GOODS_RECIEVED] * (s.get_goods_multiplier()<<6)) / (city_history_month[0][HIST_GOODS_NEEDED]);

	// smaller towns should growth slower to have villages for a longer time
	sint32 weight_factor = s.get_growthfactor_large();
	if(bev < s.get_city_threshold_size()) {
		weight_factor = s.get_growthfactor_small();
	}
	else if(bev <s.get_capital_threshold_size()) {
		weight_factor = s.get_growthfactor_medium();
	}

	// now give the growth for this step
	sint32 growth_factor = weight_factor > 0 ? (pas+mail+electricity+goods) / weight_factor : 0;
	
	//Congestion adversely impacts on growth. At 100% congestion, there will be no growth. 
	if(city_history_month[0][HIST_CONGESTION] > 0)
	{
		const uint32 congestion_factor = city_history_month[0][HIST_CONGESTION];
		growth_factor -= (congestion_factor * growth_factor) / 100;
	}
	
	wachstum += growth_factor;
}


// does constructions ...
void stadt_t::step_bau()
{
	bool new_town = (bev == 0);
	if (new_town) {
		bev = (wachstum >> 4);
		bool need_building = true;
		uint32 buildings_count = buildings.get_count();
		uint32 try_nr = 0;
		while (need_building && try_nr < 1000) {
			baue(false); // it update won
			if ( buildings_count != buildings.get_count() ) {
				if(buildings[buildings_count]->get_haustyp() == gebaeude_t::wohnung) {
					need_building = false;
				}
			}
			try_nr++;
			buildings_count = buildings.get_count();
		}
		bev = 0;
	}
	// since we use internall a finer value ...
	const int growth_step = (wachstum >> 4);
	wachstum &= 0x0F;

	// Hajo: let city grow in steps of 1
	// @author prissi: No growth without development
	for (int n = 0; n < growth_step; n++) {
		bev++; // Hajo: bevoelkerung wachsen lassen

		for (int i = 0; i < 30 && bev * 2 > won + arb + 100; i++) {
			// For some reason, this does not work: it only builds one
			// type of old building rather than all types. TODO: Either
			// fix this, or wait for Prissi to do it properly. 
			//baue(new_town);
			baue(false);
		}

		check_bau_spezial(new_town);
		check_bau_rathaus(new_town);
		check_bau_factory(new_town); // add industry? (not during creation)
		INT_CHECK("simcity 2241");
	}
}

uint16 stadt_t::check_road_connexion_to(stadt_t* city)
{
	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		return city == this ? welt->get_generic_road_speed_city() : welt->get_generic_road_speed_intercity();
	}

	if(connected_cities.is_contained(city->get_pos()))
	{
		return connected_cities.get(city->get_pos());
	}
	else if(city == this)
	{
		const koord3d pos3d(townhall_road, welt->lookup_hgt(townhall_road));
		const weg_t* road = welt->lookup(pos3d)->get_weg(road_wt);
		const uint16 journey_time_per_tile = road ? road->get_besch() == welt->get_city_road() ? welt->get_generic_road_speed_city() : welt->calc_generic_road_speed(road->get_besch()) : welt->get_generic_road_speed_city();
		connected_cities.put(pos, journey_time_per_tile);
		return journey_time_per_tile;
	}
	else
	{
		const koord destination_road = city->get_townhall_road();
		const koord3d desintation_koord(destination_road.x, destination_road.y, welt->lookup_hgt(destination_road));
		const uint16 journey_time_per_tile = check_road_connexion(desintation_koord);
		connected_cities.put(city->get_pos(), journey_time_per_tile);
		city->add_road_connexion(journey_time_per_tile, this);
		if(journey_time_per_tile == 65535)
		{
			// We know that, if this city is not connected to any given city, then every city
			// to which this city is connected must likewise not be connected. So, avoid
			// unnecessary recalculation by propogating this now.
			koordhashtable_iterator_tpl<koord, uint16> iter(connected_cities);
			while(iter.next())
			{
				welt->get_city(iter.get_current_key())->add_road_connexion(65535, city);
			}
		}
		return journey_time_per_tile;
	}
}

uint16 stadt_t::check_road_connexion_to(const fabrik_t* industry)
{
	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		return industry->get_city() && industry->get_city() == this ? welt->get_generic_road_speed_city() : welt->get_generic_road_speed_intercity();
	}
	
	if(connected_industries.is_contained(industry->get_pos().get_2d()))
	{
		return connected_industries.get(industry->get_pos().get_2d());
	}
	if(industry->get_city())
	{
		// If an industry is in a city, presume that it is connected
		// if the city is connected. Do not presume the converse.
		const uint16 time_to_city = check_road_connexion_to(industry->get_city());
		if(time_to_city < 65335)
		{
			return time_to_city;
		}
	}

	vector_tpl<koord> industry_tiles;
	industry->get_tile_list(industry_tiles);
	weg_t* road = NULL;
	ITERATE(industry_tiles, n)
	{
		const koord pos = industry_tiles.get_element(n);
		grund_t *gr;
		for(uint8 i = 0; i < 16; i ++)
		{
			koord3d pos3d(pos + pos.second_neighbours[i], welt->lookup_hgt(pos + pos.second_neighbours[i]));
			gr = welt->lookup(pos3d);
			if(!gr)
			{
				pos3d.z ++;
				gr = welt->lookup(pos3d);
				if(!gr)
				{
					continue;
				}
			}
			road = gr->get_weg(road_wt);
			if(road != NULL)
			{
				const koord3d destination = road->get_pos();
				const uint16 journey_time_per_tile = check_road_connexion(destination);
				connected_industries.put(industry->get_pos().get_2d(), journey_time_per_tile);
				if(journey_time_per_tile == 65535)
				{
					// We know that, if this city is not connected to any given industry, then every city
					// to which this city is connected must likewise not be connected. So, avoid
					// unnecessary recalculation by propogating this now.
					koordhashtable_iterator_tpl<koord, uint16> iter(connected_cities);
					while(iter.next())
					{
						welt->get_city(iter.get_current_key())->set_no_connexion_to_industry(industry);
					}
				}
				return journey_time_per_tile;
			}
		}
	}

	// No road connecting to industry - no connexion at all.
	// We should therefore set *every* city to register this
	// industry as unconnected.
	
	const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
	for(weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j) 
	{
		(*j)->set_no_connexion_to_industry(industry);
	}
	return 65335;

}

uint16 stadt_t::check_road_connexion_to(const gebaeude_t* attraction)
{
	if(welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		return welt->get_generic_road_speed_intercity();
	}
	
	if(connected_attractions.is_contained(attraction->get_pos().get_2d()))
	{
		return connected_attractions.get(attraction->get_pos().get_2d());
	}
	const koord pos = attraction->get_pos().get_2d();
	grund_t *gr;
	weg_t* road = NULL;
	for(uint8 i = 0; i < 16; i ++)
	{
		koord3d pos3d(pos + pos.second_neighbours[i], welt->lookup_hgt(pos + pos.second_neighbours[i]));
		gr = welt->lookup(pos3d);
		if(!gr)
		{
			pos3d.z ++;
			gr = welt->lookup(pos3d);
			if(!gr)
			{
				continue;
			}
		}
		road = gr->get_weg(road_wt);
		if(road != NULL)
		{
			break;
		}
	}
	if(road == NULL)
	{
		// No road connecting to attraction - no connexion at all.
		// We should therefore set *every* city to register this
		// industry as unconnected.
		
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		for(weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j) 
		{
			(*j)->set_no_connexion_to_attraction(attraction);
		}
		return 65535;
	}
	const koord3d destination = road->get_pos();
	const uint16 journey_time_per_tile = check_road_connexion(destination);
	connected_attractions.put(attraction->get_pos().get_2d(), journey_time_per_tile);
	if(journey_time_per_tile == 65535)
	{
		// We know that, if this city is not connected to any given industry, then every city
		// to which this city is connected must likewise not be connected. So, avoid
		// unnecessary recalculation by propogating this now.
		koordhashtable_iterator_tpl<koord, uint16> iter(connected_cities);
		while(iter.next())
		{
			welt->get_city(iter.get_current_key())->set_no_connexion_to_attraction(attraction);
		}
	}
	return journey_time_per_tile;
}

uint16 stadt_t::check_road_connexion(koord3d dest)
{
	const koord3d origin(townhall_road.x, townhall_road.y, welt->lookup_hgt(townhall_road));
	private_car_route->clear();
	finder->set_destination(dest);
	const uint32 depth = welt->get_max_road_check_depth();
	// Must use calc_route rather than find_route, or else this will be *far* too slow: only calc_route uses A*.
	if(!private_car_route->calc_route(welt, origin, dest, finder, welt->get_citycar_speed_average(), 0, depth))
	{
		return 65535;
	}
	koord3d pos;
	const sint32 vehicle_speed_average = welt->get_citycar_speed_average();
	sint32 top_speed;
	sint32 speed_sum = 0;
	uint32 count = 0;
	weg_t* road;
	ITERATE_PTR(private_car_route,i)
	{
		pos = private_car_route->position_bei(i);
		road = welt->lookup(pos)->get_weg(road_wt);
		top_speed = road->get_max_speed();
		speed_sum += min(top_speed, vehicle_speed_average);
		count += road->is_diagonal() ? 7 : 10; //Use precalculated numbers to avoid division here.
	}
	const sint32 speed_average = (speed_sum * 100) / (count * 13); // was (float)(speed_sum / ((float)count / 10.0F))  / 1.3F;
	const uint32 journey_distance_m = private_car_route->get_count() *welt->get_settings().get_meters_per_tile();
	const uint16 journey_time = (6 * journey_distance_m) / (10 * speed_average); // *Tenths* of minutes: hence *0.6, not *0.06.
	const uint16 straight_line_distance_tiles = accurate_distance(origin.get_2d(), dest.get_2d());
	return journey_time / (straight_line_distance_tiles == 0 ? 1 : straight_line_distance_tiles);
}

void stadt_t::add_road_connexion(uint16 journey_time_per_tile, stadt_t* origin_city)
{
	
	if(this == NULL)
	{
		return;
	}
	connected_cities.set(origin_city->get_pos(), journey_time_per_tile);
}

void stadt_t::set_no_connexion_to_industry(const fabrik_t* unconnected_industry)
{
	if(this == NULL)
	{
		return;
	}
	connected_industries.set(unconnected_industry->get_pos().get_2d(), 65535);
}

void stadt_t::set_no_connexion_to_attraction(const gebaeude_t* unconnected_attraction)
{
	if(this == NULL)
	{
		return;
	}
	connected_attractions.set(unconnected_attraction->get_pos().get_2d(), 65535);
}


/* this creates passengers and mail for everything is is therefore one of the CPU hogs of the machine
 * think trice, before applying optimisation here ...
 */
void stadt_t::step_passagiere()
{
	settings_t const& s = welt->get_settings();

	//@author: jamespetts
	// Passenger routing and generation metrics.	
	const uint32 local_passengers_min_distance = s.get_local_passengers_min_distance();
	const uint32 local_passengers_max_distance = s.get_local_passengers_max_distance();
	const uint32 midrange_passengers_min_distance = s.get_midrange_passengers_min_distance();
	const uint32 midrange_passengers_max_distance = s.get_midrange_passengers_max_distance();
	const uint32 longdistance_passengers_min_distance = s.get_longdistance_passengers_min_distance();
	const uint32 longdistance_passengers_max_distance = s.get_longdistance_passengers_max_distance();

	const uint16 min_local_tolerance = s.get_min_local_tolerance();
	const uint16 max_local_tolerance = max(0, s.get_max_local_tolerance() - min_local_tolerance);
	const uint16 min_midrange_tolerance = s.get_min_midrange_tolerance();
	const uint16 max_midrange_tolerance = max( 0, s.get_max_midrange_tolerance() - min_midrange_tolerance);
	const uint16 min_longdistance_tolerance = s.get_min_longdistance_tolerance();
	const uint16 max_longdistance_tolerance = max(0, s.get_max_longdistance_tolerance() - min_longdistance_tolerance);

	const uint8 passenger_packet_size = s.get_passenger_routing_packet_size();
	const uint8 passenger_routing_local_chance = s.get_passenger_routing_local_chance();
	const uint8 passenger_routing_midrange_chance = s.get_passenger_routing_midrange_chance();
	const uint8 always_prefer_car_percent = s.get_always_prefer_car_percent();

	//	DBG_MESSAGE("stadt_t::step_passagiere()", "%s step_passagiere called (%d,%d - %d,%d)\n", name, li, ob, re, un);
	//	long t0 = get_current_time_millis();

	// post oder pax erzeugen ?
	// "post or generate pax"
	const ware_besch_t *const wtyp = (simrand(400, "void stadt_t::step_passagiere() (mail or passengers?"))<300 ? warenbauer_t::passagiere : warenbauer_t::post;
	const int history_type = (wtyp == warenbauer_t::passagiere) ? HIST_PAS_TRANSPORTED : HIST_MAIL_TRANSPORTED;
	factory_set_t &target_factories = (  wtyp==warenbauer_t::passagiere ? target_factories_pax : target_factories_mail );

	// restart at first buiulding?
	if (step_count >= buildings.get_count()) 
	{
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

	
	// Hajo: track number of generated passengers.
	city_history_year[0][history_type+1] += num_pax;
	city_history_month[0][history_type+1] += num_pax;
			
	// create pedestrians in the near area?
	if (s.get_random_pedestrians() && wtyp == warenbauer_t::passagiere) {
		haltestelle_t::erzeuge_fussgaenger(welt, gb->get_pos(), num_pax);
	}

	// suitable start search
	const koord origin_pos = gb->get_pos().get_2d();
	const planquadrat_t *const plan = welt->lookup(origin_pos);
	const halthandle_t *const halt_list = plan->get_haltlist();

	minivec_tpl<halthandle_t> start_halts(plan->get_haltlist_count());
	for (int h = plan->get_haltlist_count() - 1; h >= 0; h--) 
	{
		halthandle_t halt = halt_list[h];
		if (halt->is_enabled(wtyp)  &&  !halt->is_overcrowded(wtyp->get_catg_index())) 
		{
			start_halts.append(halt);
		}
	}

	INT_CHECK( "simcity 2794" );

	// Check whether this batch of passengers has access to a private car each.
	// Check run in batches to save computational effort.
	const sint16 private_car_percent = wtyp == warenbauer_t::passagiere ? get_private_car_ownership(welt->get_timeline_year_month()) : 0; 
	// Only passengers have private cars
	const bool has_private_car = private_car_percent > 0 ? simrand(100, "void stadt_t::step_passagiere() (has private car?)") <= (uint16)private_car_percent : false;
	
	// Record the most useful set of information about why passengers cannot reach their chosen destination:
	//  Too slow > overcrowded > no route. Tiebreaker: higher destination preference.
	koord best_bad_destination;
	uint8 best_bad_start_halt;
	bool too_slow_already_set;

	//Only continue if there are suitable start halts nearby, or the passengers have their own car.
	if(!start_halts.empty() || has_private_car)
	{		
		// Add 1 because the simuconf.tab setting is for maximum *alternative* destinations, whereas we need maximum *actual* desintations
		// const uint8 max_destinations = has_private_car ? 1 : (welt->get_settings().get_max_alternative_destinations() < 16 ?welt->get_settings().get_max_alternative_destinations() : 15) + 1;
		// Passengers with a private car will not tolerate second best destinations,
		// and will use their private car to get to their first choice destination
		// regardless of whether they might go to other destinations by public transport.

		// Now that private cars also have a journey time tolerance and check whether the roads are connected, passengers *might*
		// not be able to get to their destination by private car either, so the above is redundant. 
		const uint8 max_destinations = (s.get_max_alternative_destinations() < 16 ? s.get_max_alternative_destinations() : 15) + 1;

		// Find passenger destination
		for(  int pax_routed=0, pax_left_to_do=0;  pax_routed<num_pax;  pax_routed+=pax_left_to_do  ) 
		{
			
			/* number of passengers that want to travel
			* Hajo: for efficiency we try to route not every
			* single pax, but packets. If possible, we do 7 passengers at a time
			* the last packet might have less then 7 pax
			* Number now not fixed at 7, but set in simuconf.tab (@author: jamespetts)
			*/

			pax_left_to_do = min(passenger_packet_size, num_pax - pax_routed);

			// search target for the passenger
			pax_return_type will_return;

			const uint8 destination_count = simrand(max_destinations, "void stadt_t::step_passagiere() (number of destinations?)") + 1;

			// Split passengers: between local, midrange and long-distance
			// according to the percentages set in simuconf.tab.
			// Note: a random town will be found if there are no towns within range.
			const uint8 passenger_routing_choice = simrand(100, "void stadt_t::step_passagiere() (passenger routing choice?)");
			const journey_distance_type range = 
				passenger_routing_choice <= passenger_routing_local_chance ? 
				local :
			passenger_routing_choice <= (passenger_routing_local_chance + passenger_routing_midrange_chance) ? 
				midrange : longdistance;
			const uint16 tolerance = 
				wtyp != warenbauer_t::passagiere ? 
				0 : 
				range == local ? 
					simrand(max_local_tolerance, "void stadt_t::step_passagiere() (local tolerance?)") + min_local_tolerance : 
				range == midrange ? 
					simrand(max_midrange_tolerance, "void stadt_t::step_passagiere() (midrange tolerance?)") + min_midrange_tolerance : 
				simrand(max_longdistance_tolerance, "void stadt_t::step_passagiere() (longdistance tolerance?)") + min_longdistance_tolerance;
			destination destinations[16];
			for(int destinations_assigned = 0; destinations_assigned <= destination_count; destinations_assigned ++)
			{				
				if(range == local)
				{
					//Local - a designated proportion will automatically go to destinations within the town.
					if(passenger_routing_choice <= adjusted_passenger_routing_local_chance)
					{
						// Will always be a destination in the current town.
						destinations[destinations_assigned] = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, 0, local_passengers_max_distance, origin_pos);	
					}
					else
					{
						destinations[destinations_assigned] = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, local_passengers_min_distance, local_passengers_max_distance, origin_pos);
					}
				}
				else if(range == midrange)
				{
					//Medium
					destinations[destinations_assigned] = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, midrange_passengers_min_distance, midrange_passengers_max_distance, origin_pos);
				}
				else
				//else if(range == longdistance)
				{
					//Long distance
					destinations[destinations_assigned] = find_destination(target_factories, city_history_month[0][history_type+1], &will_return, longdistance_passengers_min_distance, longdistance_passengers_max_distance, origin_pos); 
				}
			}
			
			INT_CHECK( "simcity 3118" );

			uint8 current_destination = 0;

			route_status route_good = no_route;
			
			const uint16 max_walking_distance = s.get_max_walking_distance();
			uint16 car_minutes = 65535;

			best_bad_destination = destinations[0].location;
			best_bad_start_halt = 0;
			too_slow_already_set = false;

			while(route_good != good && route_good != private_car_only && current_destination < destination_count)
			{			
				// Dario: Check if there's a stop near destination
				const planquadrat_t* dest_plan = welt->lookup(destinations[current_destination].location);
				const halthandle_t* dest_list = dest_plan->get_haltlist();

				// Knightly : we can avoid duplicated efforts by building destination halt list here at the same time
				minivec_tpl<halthandle_t> destination_list(dest_plan->get_haltlist_count());
				
				halthandle_t start_halt;
				
				// Check whether the destination is within walking distance first.
				// @author: jamespetts, December 2009
				if(accurate_distance(destinations[current_destination].location, origin_pos) <= max_walking_distance)
				{
					// Passengers will always walk if they are close enough.
					route_good = can_walk;
				}

				if(route_good != can_walk)
				{
					for (int h = dest_plan->get_haltlist_count() - 1; h >= 0; h--) 
					{
						halthandle_t halt = dest_list[h];
						if (halt->is_enabled(wtyp)) 
						{
							destination_list.append(halt);
							ITERATE(start_halts, i)
							{
								if(halt == start_halts[i])
								{
									route_good = can_walk;
									start_halt = start_halts[i];
									// Mail does not walk, but people delivering it do.
									break;
								}
							}
						}
					}
				}

				if (route_good == can_walk) 
				{
					if(destinations[current_destination].factory_entry)
					{
						register_factory_passenger_generation(&pax_left_to_do, wtyp, target_factories, destinations[current_destination].factory_entry);
						// workers who walk to the factory or customers who walk to the consumer store
						destinations[current_destination].factory_entry->factory->book_stat( pax_left_to_do, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
						destinations[current_destination].factory_entry->factory->liefere_an(wtyp, pax_left_to_do);
					}

					// so we have happy passengers
	
					// Passengers who can walk to their destination may be happy about it,
					// but they are not happy *because* the player has made them happy. 
					// Therefore, they should not show on the station's happy graph.
					// @author: jamespetts, December 2009
					
					//start_halt->add_pax_happy(pax_left_to_do);

					merke_passagier_ziel(destinations[0].location, COL_DARK_YELLOW);
					if (s.get_random_pedestrians() && wtyp == warenbauer_t::passagiere) 
					{
						if(!start_halts.empty() && !start_halt.is_bound())
						{
							start_halt = start_halts[0];
						}
						if(start_halt.is_bound())
						{
							haltestelle_t::erzeuge_fussgaenger(welt, start_halt->get_basis_pos3d(), pax_left_to_do);
						}
					}
					
					// They should show that they have been transported, however, since
					// these figures are used for city growth calculations.
					city_history_year[0][history_type] += pax_left_to_do;
					city_history_month[0][history_type] += pax_left_to_do;
					break;
				}

				// ok, they are not in walking distance
				// Check whether public transport can be used.
				ware_t pax(wtyp); //Journey start information needs to be added later.
				pax.set_zielpos(destinations[current_destination].location);
				pax.menge = pax_left_to_do;
				pax.to_factory = ( destinations[current_destination].factory_entry ? 1 : 0 );
				//"Menge" = volume (Google)

				// now, finally search a route; this consumes most of the time

				uint16 best_journey_time = 65535;
				uint8 best_start_halt = 0;
				
				ITERATE(start_halts,i)
				{
					halthandle_t current_halt = start_halts[i];

					uint16 current_journey_time = current_halt->find_route(&destination_list, pax, best_journey_time);
					if(current_journey_time < best_journey_time)
					{
						best_journey_time = current_journey_time;
						best_start_halt = i;
					}
					if(pax.get_ziel().is_bound())
					{
						route_good = good;
					}
				}
				
				// Check first whether the best route is outside
				// the passengers' tolerance.

				if(route_good == good && tolerance > 0 && best_journey_time > tolerance)
				{
					route_good = too_slow;
					
					if(!too_slow_already_set)
					{
						best_bad_destination = destinations[current_destination].location;
						best_bad_start_halt = best_start_halt;
						too_slow_already_set = true;
					}
				}
				else
				{
					// All passengers will use the quickest route.
					if(start_halts.get_count() > 0)
					{
						start_halt = start_halts[best_start_halt];
					}
				}

				INT_CHECK("simcity.cc 2993");

				if(has_private_car) 
				{
					const uint32 straight_line_distance = accurate_distance(origin_pos, destinations[current_destination].location);
					uint16 time_per_tile = 65535;
					switch(destinations[current_destination].type)
					{
					case 1:
						//Town
						time_per_tile = check_road_connexion_to(destinations[current_destination].object.town);
						break;
					case FACTORY_PAX:
						time_per_tile = check_road_connexion_to(destinations[current_destination].object.industry);
						break;
					case TOURIST_PAX:
						time_per_tile = check_road_connexion_to(destinations[current_destination].object.attraction);
						break;
					default:
						//Some error - this should not be reached.
						dbg->error("simcity.cc", "Incorrect destination type detected");
					};
					
					if(time_per_tile < 65535)
					{
						// *Tenths* of minutes used here.
						car_minutes =  time_per_tile * straight_line_distance;
					}
					else
					{
						car_minutes = 65535;
					}
				}		
				
				if(car_minutes <= tolerance)
				{
					if(route_good != good)
					{
						// The passengers can get to their destination by car but not by public transport.
						// Therefore, they will certainly use their car. 
						route_good = private_car_only;
					}
									
					else
					{
						// The passengers can get to their destination both by car and public transport.
						// Choose which they prefer.

						// Now, decide whether passengers would prefer to use their private cars,
						// even though they can travel by public transport.
						const uint32 distance = accurate_distance(destinations[current_destination].location, origin_pos);			
						
						//Weighted random.
						const uint16 private_car_chance = (uint16)simrand(100, "void stadt_t::step_passagiere() (private car chance?)");
						if(private_car_chance <= always_prefer_car_percent)
						{
							route_good = private_car_only;
						}
						else
						{
							// The basic preference for using a private car if available.
							uint16 car_preference = s.get_base_car_preference_percent();
										
							// Firstly, congestion. Drivers will turn to public transport if the origin or destination towns are congested.

							//Average congestion of origin and destination towns, and, at the same time, reduce factor.
							uint16 congestion_total;
							if(destinations[current_destination].type == 1 && destinations[current_destination].object.town != NULL)
							{
								// Destination type is town and the destination town object can be found.
								congestion_total = (city_history_month[0][HIST_CONGESTION] + destinations[current_destination].object.town->get_congestion()) / 4;
							}
							else
							{
								congestion_total = (city_history_month[0][HIST_CONGESTION] * 100) / 133;
							}
							car_preference -= congestion_total;

							// Secondly, adjust for service quality of the public transport.
							// Compare best journey speed on public transport with the 
							// likely private car journey time.

							INT_CHECK( "simcity 3004" );

							// Journey times of private transport as a percentage of player journey times.
							uint32 car_journey_time_percent = (100 * car_minutes) / best_journey_time;
							// Set a limit to how much this affects things.
							car_journey_time_percent = max(car_journey_time_percent, 15);
							car_journey_time_percent = min(car_journey_time_percent, 250);
							// Modify the preference by how much better or worse that the car journey time is than the player's.
							car_preference = (car_preference * 100) / car_journey_time_percent;
						
							// If identical, no adjustment.

							//Thirdly, the number of unhappy passengers at the start station compared with the number of happy passengers.
							const uint16 unhappy_percentage = start_halt->get_unhappy_percentage(0);
						
							if(unhappy_percentage > 80)
							{
								car_preference = (car_preference * 100) / unhappy_percentage;
							}
						
							//Finally, determine whether the private car is used.
							if(private_car_chance <= car_preference)
							{
								// Private cars chosen by preference even though public transport is possible.
								route_good = private_car_only;
							}
						}
					}
				}
					
				if(route_good == good)
				{
					// Passengers can and will use public transport.
					if(destinations[current_destination].factory_entry)
					{
						register_factory_passenger_generation(&pax_left_to_do, wtyp, target_factories, destinations[current_destination].factory_entry);
						destinations[current_destination].factory_entry->factory->book_stat( pax.menge, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
					}
					pax.arrival_time = welt->get_zeit_ms();
					pax.set_origin(start_halt);
					start_halt->starte_mit_route(pax);
					merke_passagier_ziel(destinations[current_destination].location, COL_YELLOW);
				}

				else if(route_good == private_car_only)
				{
					// Passengers have either chosen to use their car or have no other choice.
					if(destinations[current_destination].factory_entry)
					{
						register_factory_passenger_generation(&pax_left_to_do, wtyp, target_factories, destinations[current_destination].factory_entry);
						destinations[current_destination].factory_entry->factory->book_stat( pax.menge, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
					}
					stadt_t* const destination_town = destinations[current_destination].type == 1 ? destinations[current_destination].object.town : NULL;
					set_private_car_trip(num_pax, destination_town);
					merke_passagier_ziel(destinations[current_destination].location, COL_TURQUOISE);
#ifdef DESTINATION_CITYCARS
					erzeuge_verkehrsteilnehmer(origin_pos, step_count, destinations[current_destination].location);
#endif
				}


				// send them also back
				// (Calculate a return journey)
				if(will_return != no_return && (route_good == good || route_good == private_car_only))
				{
					// this comes most of the times for free and balances also the amounts!
					halthandle_t ret_halt = pax.get_ziel();
					bool return_in_private_car = (route_good == private_car_only) || !ret_halt.is_bound();

					if(!return_in_private_car)
					{
						// we just have to ensure that the ware can be delivered at this station
						bool found = false;
						for (uint i = 0; i < plan->get_haltlist_count(); i++) 
						{
							halthandle_t test_halt = halt_list[i];

							if(test_halt->is_enabled(wtyp) && (start_halt == test_halt || test_halt->get_connexions(wtyp->get_catg_index())->access(start_halt) != NULL))
							{
								found = true;
								start_halt = test_halt;
								break;
							}
						}

						// now try to add them to the target halt
						if(!ret_halt->is_overcrowded(wtyp->get_catg_index())) 
						{
							// prissi: not overcrowded and can recieve => add them
							if(found) 
							{
								ware_t return_pax(wtyp, ret_halt);
								return_pax.to_factory = 0;
								if(  will_return != city_return  &&  wtyp==warenbauer_t::post  ) 
								{
								// attractions/factory generate more mail than they recieve
									return_pax.menge = pax_left_to_do * 3;
								}
								else 
								{
									// use normal amount for return pas/mail
									return_pax.menge = pax_left_to_do;
								}
								return_pax.set_zielpos(origin_pos);
								return_pax.set_ziel(start_halt);
								if(ret_halt->find_route(return_pax) != 65535)
								{
									return_pax.arrival_time = welt->get_zeit_ms();
									ret_halt->starte_mit_route(return_pax);
									merke_passagier_ziel(destinations[current_destination].location, COL_YELLOW);
									//ret_halt->add_pax_happy(pax_left_to_do); 
									// Experimental 7.2 and onwards does this when passengers reach their destination
								}
							}
							else 
							{
								// no route back
								if(car_minutes < 65535)
								{
									// This assumes that the journey time in both directions is identical: but 
									// this may not be so if there are one-way routes.
									return_in_private_car = true;
								}
								else
								{	
									ret_halt->add_pax_no_route(pax_left_to_do);
									merke_passagier_ziel(destinations[current_destination].location, COL_DARK_ORANGE);
								}
							}
						}
						else
						{
							// Return halt crowded. Either return by car or mark unhappy.
							if(car_minutes < 65535)
							{
								return_in_private_car = true;
							}
							else
							{
								ret_halt->add_pax_unhappy(pax_left_to_do);
								merke_passagier_ziel(destinations[current_destination].location, COL_RED);
							}
						}
					}
					
					if(return_in_private_car)
					{
						if(car_minutes < 65535)
						{
							// Do not check tolerance, as they must come back!
							stadt_t* const destination_town = destinations[0].type == 1 ? destinations[0].object.town : NULL;
							set_private_car_trip(num_pax, destination_town);
							merke_passagier_ziel(destinations[current_destination].location, COL_TURQUOISE);

#ifdef DESTINATION_CITYCARS
							//citycars with destination
							erzeuge_verkehrsteilnehmer(origin_pos, step_count, destinations[0].location);
#endif

						}
						else
						{
							if(ret_halt.is_bound())
							{
								ret_halt->add_pax_no_route(pax_left_to_do);
							}
							merke_passagier_ziel(destinations[current_destination].location, COL_DARK_ORANGE);
						}
					}
				} // Returning passengers

				INT_CHECK( "simcity 3128" );
				current_destination ++;
			} // While loop (route_good)

			if(route_good != good && route_good != private_car_only && route_good != can_walk)
			{
				if(destinations[0].factory_entry)
				{
					register_factory_passenger_generation(&pax_left_to_do, wtyp, target_factories, destinations[0].factory_entry);
				}
				if(start_halts.get_count() > 0)
				{
					// Passengers/mail cannot reach their destination at all, either by public transport or private car,
					// *but* there are some stops in their locality. Record their inability to get where they are going
					// at those local stops accordingly. 
					halthandle_t start_halt = start_halts[best_bad_start_halt]; //If there is no route, it does not matter where passengers express their unhappiness.
					if(!has_private_car || car_minutes > tolerance)
					{
						// If the passengers are able to use a private car, then they should not record as "no route"						
						if(too_slow_already_set)
						{
							if(start_halt.is_bound())
							{
								start_halt->add_pax_too_slow(pax_left_to_do);
							}
							merke_passagier_ziel(best_bad_destination, COL_LIGHT_PURPLE);
						}
						else
						{
							if(start_halt.is_bound())
							{
								start_halt->add_pax_no_route(pax_left_to_do);
							}
							merke_passagier_ziel(destinations[0].location, COL_DARK_ORANGE);
						}
					}
				}
			}
			
		} // For loop (passenger/mail packets)
	
	} // If statement (are some start halts, or passengers have private car)

	else 
	{
		// The unhappy passengers will be added to all crowded stops
		// however, there might be no stop too
		// NOTE: Because of the conditional statement in the original loop,
		// reaching this code means that passengers must not be able to use
		// a private car for this journey.

		// all passengers without suitable start:
		// fake one ride to get a proper display of destinations (although there may be more) ...
		pax_return_type will_return;

		//const koord ziel = finde_passagier_ziel(&will_return);
		destination destination_now;
		destination_now = find_destination(target_factories, city_history_month[0][history_type+1], &will_return);
		sint32 amount = min(passenger_packet_size, num_pax);
		if(destination_now.factory_entry)
		{
			register_factory_passenger_generation(&amount, wtyp, target_factories, destination_now.factory_entry);
		}

		// First, check whether the passengers can *walk*. Just because
		// they do not have a start halt does not mean that they cannot
		// walk to their destination!
		const uint32 tile_distance = accurate_distance(origin_pos, destination_now.location);
		if(tile_distance < s.get_max_walking_distance())
		{
			// Passengers will walk to their destination if it is within the specified range.
			// (Default: 1.5km)
			// workers who walk to the factory or customers who walk to the consumer store
			if(destination_now.factory_entry)
			{
				destination_now.factory_entry->factory->book_stat( amount, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED ) );
				destination_now.factory_entry->factory->liefere_an(wtyp, amount);
			}
			merke_passagier_ziel(destination_now.location, COL_DARK_YELLOW);
			city_history_year[0][history_type] += num_pax;
			city_history_month[0][history_type] += num_pax;
		}
		else
		{
			// If the passengers cannot walk, they will be unhappy.

			bool crowded_halts = false;
			for(  uint h=0;  h<plan->get_haltlist_count(); h++  ) 
			{
				halthandle_t halt = halt_list[h];
				if (halt->is_enabled(wtyp)) 
				{
					halt->add_pax_unhappy(num_pax);
					crowded_halts = true;
				}
			}
				
			// Only show as being overcrowded if there are, in fact, potentially suitable but overcrowded stops.
			if(crowded_halts)
			{
				merke_passagier_ziel(destination_now.location, COL_RED);		
			}

			else
			{
				merke_passagier_ziel(destination_now.location, COL_DARK_ORANGE);
			}
			// we do not show no route for destination stop!
		}
	}

	//	long t1 = get_current_time_millis();
	//	DBG_MESSAGE("stadt_t::step_passagiere()", "Zeit für Passagierstep: %ld ms\n", (long)(t1 - t0));
}

void stadt_t::register_factory_passenger_generation(int* pax_left_to_do, const ware_besch_t *const wtyp, factory_set_t &target_factories, factory_entry_t* &factory_entry)
{
	if( welt->get_settings().get_factory_enforce_demand()  ) 
	{
		// ensure no more than remaining amount
		*pax_left_to_do = min(*pax_left_to_do, factory_entry->remaining);
		factory_entry->remaining -= *pax_left_to_do;
		target_factories.total_remaining -= *pax_left_to_do;
	}
	target_factories.total_generated += *pax_left_to_do;
	factory_entry->factory->book_stat( *pax_left_to_do, ( wtyp==warenbauer_t::passagiere ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED ) );
}

void stadt_t::set_private_car_trip(int passengers, stadt_t* destination_town)
{
	if(destination_town == NULL || (destination_town->get_pos().x == pos.x && destination_town->get_pos().y == pos.y))
	{
		// Destination town is not set - so going to a factory or tourist attraction.
		// Or origin and destination towns are the same.
		// Count as a local trip
		city_history_year[0][HIST_CITYCARS] += passengers;
		city_history_month[0][HIST_CITYCARS] += passengers;
	}
	else
	{
		//Inter-city trip
		city_history_year[0][HIST_CITYCARS] += passengers;
		city_history_month[0][HIST_CITYCARS] += passengers;
		
		//Also add private car trips to the *destination*.
		destination_town->set_private_car_trips(passengers);

		//And mark the trip as outgoing for growth calculations
		outgoing_private_cars += passengers;
	}
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
		koord k = koord::invalid;
		uint32 distance = 0;
		uint8 counter = 0;
		while (counter++ < 16 && (k == koord::invalid || distance > max_distance || distance < min_distance))
		{
			gebaeude_t* const gb = pick_any_weighted(buildings);
			k = gb->get_pos().get_2d();
			if(!welt->ist_in_kartengrenzen(k)) 
			{
				// this building should not be in this list, since it has been already deleted!
				dbg->error("stadt_t::get_zufallspunkt()", "illegal building in city list of %s: %p removing!", this->get_name(), gb);
				const_cast<stadt_t*>(this)->buildings.remove(gb);
				k = koord(0, 0);
			}
			distance = accurate_distance(k, origin);
		}
		return k;
	}
	// might happen on slow computers during creation of new cities or start of map
	return koord(0,0);
}


void stadt_t::add_target_city(stadt_t *const city)
{
	assert( city != NULL );
	target_cities.insert_ordered(
		target_city_t( city, shortest_distance( this->get_pos(), city->get_pos() ) ),
		max( city->get_einwohner(), 1 ),
		target_city_t::less_than,
		64u
	);
}


void stadt_t::update_target_city(stadt_t *const city)
{
	assert( city != NULL );
	target_cities.update( target_city_t( city, 0 ), max( city->get_einwohner(), 1 ) );
}


void stadt_t::update_target_cities()
{
	for(  uint32 c=0;  c<target_cities.get_count();  ++c  ) {
		target_cities.update_at( c, max( target_cities[c].city->get_einwohner(), 1 ) );
	}
}


void stadt_t::recalc_target_cities()
{
	target_cities.clear();
	const weighted_vector_tpl<stadt_t *> &cities = welt->get_staedte();
	for(  uint32 c=0;  c<cities.get_count();  ++c  ) {
		add_target_city( cities[c] );
	}
}


void stadt_t::add_target_attraction(gebaeude_t *const attraction)
{
	assert( attraction != NULL );
	target_attractions.append(
		attraction,
		weight_by_distance( attraction->get_passagier_level() << 4, shortest_distance( this->get_pos(), attraction->get_pos().get_2d() ) ),
		64u
	);
}


void stadt_t::recalc_target_attractions()
{
	target_attractions.clear();
	const weighted_vector_tpl<gebaeude_t *> &attractions = welt->get_ausflugsziele();
	for(  uint32 a=0;  a<attractions.get_count();  ++a  ) {
		add_target_attraction( attractions[a] );
	}
}


weighted_vector_tpl<uint32> stadt_t::distances;


void stadt_t::init_distances(const uint32 max_distance)
{
	if(  distances.get_count()<max_distance  ) {
		// expand the list
		distances.resize(max_distance);
		for(  uint32 d=distances.get_count()+1u;  d<=max_distance;  ++d  ) {
			distances.append(d, (1u << 20) / d);
		}
	}
	else if(  distances.get_count()>max_distance  ) {
		// shorten the list
		for(  uint32 d=distances.get_count()-1u;  d>=max_distance;  --d  ) {
			distances.remove_at(d);
		}
	}
}


/* this function generates a random target for passenger/mail
 * changing this strongly affects selection of targets and thus game strategy
 */
stadt_t::destination stadt_t::find_destination(factory_set_t &target_factories, const sint64 generated, pax_return_type* will_return, uint32 min_distance, uint32 max_distance, koord origin)
{
	const int rand = simrand(100, "stadt_t::destination stadt_t::find_destination (init)");
	destination current_destination;
	current_destination.object.town = NULL;
	current_destination.type = 1;
	if(origin == koord::invalid)
	{
		origin = this->get_pos();
	}

	// Note: unlike in Standard, this must remain random in Experimental, as passengers will only travel to industries within range, and will have multiple
	// destinations to which they will travel if they cannot get to the factories.
	if(rand < welt->get_settings().get_factory_worker_percentage() && target_factories.total_remaining > 0 && (sint64)target_factories.generation_ratio > ((sint64)(target_factories.total_generated*100) << RATIO_BITS) / (generated + 1))
	{
		factory_entry_t* entry = target_factories.get_random_entry();
		while(entry->factory == NULL)
		{
			entry = target_factories.get_random_entry();
		} 
		*will_return = factory_return;	// worker will return
		current_destination.type = FACTORY_PAX;
		uint8 counter = 0;
		while(counter ++ < 32 && (accurate_distance(origin, entry->factory->get_pos().get_2d()) > max_distance || accurate_distance(origin, entry->factory->get_pos().get_2d()) < min_distance))
		{
			entry = target_factories.get_random_entry();
			while(entry->factory == NULL)
			{
				entry = target_factories.get_random_entry();
			} 
		}
		current_destination.location = entry->factory->get_pos().get_2d();
		current_destination.object.industry = entry->factory;
		current_destination.factory_entry = entry;
		return current_destination;
	} 
	
	else if(rand <welt->get_settings().get_tourist_percentage() +welt->get_settings().get_factory_worker_percentage()) 
	{ 		
		*will_return = tourist_return;	// tourists will return
		const gebaeude_t* gb = welt->get_random_ausflugsziel();
		current_destination.type = TOURIST_PAX;
		uint8 counter = 0;
		while(counter ++ < 32 && (accurate_distance(origin, gb->get_pos().get_2d()) > max_distance || accurate_distance(origin, gb->get_pos().get_2d()) < min_distance))
		{
			gb =  welt->get_random_ausflugsziel();
		}
		current_destination.location = gb->get_pos().get_2d();
		current_destination.object.attraction = gb;
		return current_destination;
	}

	else 
	{
		stadt_t* zielstadt;

		if(max_distance == 0)
		{
			// A proportion of local passengers will always travel *within* the town.
			// This enables the town finding routine to be skipped.
			zielstadt = this;
		}

		else
		{
			const uint32 weight = welt->get_town_list_weight();
			const uint32 number_of_towns = welt->get_staedte().get_count();
			uint32 town_step = weight / number_of_towns;
			uint32 random = simrand(weight, "stadt_t::destination stadt_t::finde_passagier_ziel (town)");
			uint32 distance = 0;
			const uint16 max_x = max((origin.x - ur.x), (origin.x - lo.x));
			const uint16 max_y = max((origin.y - ur.y), (origin.y - lo.y));
			const uint16 max_internal_distance = max(max_x, max_y);
			const uint8 max_count = 96;
			for(uint8 i = 0; i < max_count; i ++)
			{
				zielstadt = welt->get_town_at(random);
				// Add max_internal_distnace here, as the destination building might be *closer* than the town hall.
				
				if(zielstadt == this)
				{
					distance = accurate_distance(origin, zielstadt->get_pos()) + max_internal_distance; 
				}
				else
				{
					distance = accurate_distance(origin, zielstadt->get_pos()) + max(max_internal_distance, zielstadt->get_max_dimension());
				}
				
				if(distance <= max_distance && distance >= min_distance)
				{
					break;
				}

				random += town_step;
				if(random > weight)
				{
					random = 0;
				}
				/*if(i == 16 || i == 32 || i == 64)
				{
					// Necessary to modulate the destinations to avoid repeatedly hitting the same towns.
					town_step -= 128;
				}*/

				/*if(i == 8 || i == 24 || i == 48)
				{
					town_step += 64;
				}*/

				// This is almost never hit, even on very big maps. It is therefore an unnecessary check.
				/*if(i == max_count && distance > max_distance && min_distance < max_internal_distance)
				{
					zielstadt = this;
				}*/
			}
		}

		// long distance traveller? => then we return
		// zielstadt = "Destination city"
		*will_return = (this != zielstadt) ? city_return : no_return;
		current_destination.location = zielstadt->get_zufallspunkt(min_distance, max_distance, origin); //"random dot"
		current_destination.object.town = zielstadt;
		return current_destination;
	}
}

uint16 stadt_t::get_max_dimension()
{
	const uint16 x = ur.x - lo.x;
	const uint16 y = ur.y - lo.y;
	return max(x,y);
}


void stadt_t::merke_passagier_ziel(koord k, uint8 color)
{
	const koord p = koord(
		((k.x * PAX_DESTINATIONS_SIZE) / welt->get_groesse_x()) & (PAX_DESTINATIONS_SIZE-1),
		((k.y * PAX_DESTINATIONS_SIZE) / welt->get_groesse_y()) & (PAX_DESTINATIONS_SIZE-1)
	);


	const uint8 existing_colour = pax_destinations_new.get(p);
	if(color != COL_DARK_ORANGE || existing_colour == 0)
	{
		// "No route" is less useful than more specific indications to that same destination.
		// Therefore, do not over-write anything else with "no route" within the same month.
		pax_destinations_new_change ++;
		pax_destinations_new.set(p, color);
	}
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
			const grund_t* bd = welt->lookup_kartenboden(koord(x, y));
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
		if (simrand(100, "void stadt_t::check_bau_spezial") < (uint)besch->get_chance()) {
			// baue was immer es ist
			int rotate = 0;
			bool is_rotate = besch->get_all_layouts() > 1;
			koord best_pos = bauplatz_mit_strasse_sucher_t(welt).suche_platz(pos, besch->get_b(), besch->get_h(), besch->get_allowed_climate_bits(), &is_rotate);

			if (best_pos != koord::invalid) {
				// then built it
				if (besch->get_all_layouts() > 1) {
					rotate = (simrand(20, "void stadt_t::check_bau_spezial") & 2) + is_rotate;
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

		DBG_MESSAGE("check_bau_rathaus()", "bev=%d, new=%d name=%s", bev, neugruendung, name);

		if(  bev!=0  ) {

			const haus_besch_t* besch_alt = gb->get_tile()->get_besch();
			if (besch_alt->get_level() == besch->get_level()) {
				DBG_MESSAGE("check_bau_rathaus()", "town hall already ok.");
				return; // Rathaus ist schon okay
			}
			old_layout = gb->get_tile()->get_layout();
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
			if (old_layout<=besch->get_all_layouts()  &&  besch->get_b(old_layout) <= groesse_alt.x  &&  besch->get_h(old_layout) <= groesse_alt.y) {
				// no, the size is ok
				// correct position if new townhall is smaller than old
				if (old_layout == 0) {
					best_pos.y -= besch->get_h(old_layout) - groesse_alt.y;
				}
				else if (old_layout == 1) {
					best_pos.x -= besch->get_b(old_layout) - groesse_alt.x;
				}
				umziehen = false;
			} else {
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
			if (gb) {
				DBG_MESSAGE("stadt_t::check_bau_rathaus()", "delete townhall at (%s)", pos_alt.get_str());
				hausbauer_t::remove(welt, NULL, gb);
			}

			// replace old space by normal houses level 0 (must be 1x1!)
			if (umziehen) {
				for (k.x = 0; k.x < groesse_alt.x; k.x++) {
					for (k.y = 0; k.y < groesse_alt.y; k.y++) {
						// we iterate over all tiles, since the townhalls are allowed sizes bigger than 1x1
						const koord pos = pos_alt + k;
						gr = welt->lookup_kartenboden(pos);
						if (gr  &&  gr->ist_natur() &&  gr->kann_alle_obj_entfernen(NULL) == NULL  &&
							  (  gr->get_grund_hang() == hang_t::flach  ||  welt->lookup(koord3d(k, welt->max_hgt(k))) == NULL  ) ) {
							DBG_MESSAGE("stadt_t::check_bau_rathaus()", "fill empty spot at (%s)", pos.get_str());
							baue_gebaeude(pos, new_town);
						}
					}
				}
			}
			else {
				// make tiles flat, hausbauer_t::remove could have set some natural slopes
				for (k.x = 0; k.x < besch->get_b(old_layout); k.x++) {
					for (k.y = 0; k.y < besch->get_h(old_layout); k.y++) {
						gr = welt->lookup_kartenboden(best_pos + k);
						if (gr  &&  gr->ist_natur()) {
							gr->set_grund_hang(hang_t::flach);
						}
					}
				}
			}
		}

		// Now built the new townhall (remember old orientation)
		int layout = umziehen || neugruendung ? simrand(besch->get_all_layouts(), "void stadt_t::check_bau_rathaus") : old_layout % besch->get_all_layouts();
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
			buf.printf( translator::translate("%s wasted\nyour money with a\nnew townhall\nwhen it reached\n%i inhabitants."), name, get_einwohner() );
			welt->get_message()->add_message(buf, best_pos, message_t::city, CITY_KI, besch->get_tile(layout, 0, 0)->get_hintergrund(0, 0, 0));
		}
		else {
			welt->lookup_kartenboden(best_pos)->set_text( name );
		}

		if (neugruendung || umziehen) {
			// build the road in front of the townhall
			if (road0!=road1) {
				wegbauer_t bauigel(welt, NULL);
				bauigel.route_fuer(wegbauer_t::strasse, welt->get_city_road(), NULL, NULL);
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
			//  "Street from the former City Hall as the new move." (Google)
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
			// Knightly : update the links between this city and other cities as well as attractions
			const weighted_vector_tpl<stadt_t *> &cities = welt->get_staedte();
			if(  cities.is_contained(this)  ) {
				// update only if this city has already been added to the world
				for(  uint32 c=0;  c<cities.get_count();  ++c  ) {
					cities[c]->remove_target_city(this);
					cities[c]->add_target_city(this);
				}
				recalc_target_cities();
				recalc_target_attractions();
			}
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
				fabrikbauer_t::increase_industry_density( welt, true, true );
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


void stadt_t::baue_gebaeude(const koord k, bool new_town)
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
			h = hausbauer_t::get_gewerbe(0, current_month, cl, new_town);
			if (h != NULL) {
				arb += h->get_level() * 20;
			}
		}

		if (h == NULL  &&  sum_industrie > sum_gewerbe  &&  sum_industrie > sum_wohnung) {
			h = hausbauer_t::get_industrie(0, current_month, cl, new_town);
			if (h != NULL) {
				arb += h->get_level() * 20;
			}
		}

		if (h == NULL  &&  sum_wohnung > sum_industrie  &&  sum_wohnung > sum_gewerbe) {
			h = hausbauer_t::get_wohnhaus(0, current_month, cl, new_town);
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
						// if not current city road standard, then replace it
						if (weg->get_besch() != welt->get_city_road()) {
							spieler_t *sp = weg->get_besitzer();
							if (sp == NULL  ||  !gr->get_depot()) {
								spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());

								weg->set_besitzer(NULL); // make public
								weg->set_besch(welt->get_city_road());
								weg->set_gehweg(true);
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


void stadt_t::erzeuge_verkehrsteilnehmer(koord pos, sint32 /*level*/, koord target)
{
	const int verkehr_level =welt->get_settings().get_verkehr_level();
	//if (verkehr_level > 0 && level % (17 - verkehr_level) == 0) {
	if((sint32)current_cars.get_count() < number_of_cars)
	{
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
							stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), target, &current_cars);
							gr->obj_add(vt);
							welt->sync_add(vt);
							current_cars.append(vt);
						}
						return;
					}
				}
			}
		}
	}
}


bool stadt_t::renoviere_gebaeude(gebaeude_t* gb)
{
	const gebaeude_t::typ alt_typ = gb->get_haustyp();
	if (alt_typ == gebaeude_t::unbekannt) {
		return false; // only renovate res, com, ind
	}

	if (gb->get_tile()->get_besch()->get_b()*gb->get_tile()->get_besch()->get_h()!=1) {
		return false; // too big ...
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
	bool return_value = false;
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
			grund_t* gr = welt->lookup_kartenboden(k + neighbours[i]);
			if (gr != NULL && gr->get_weg_hang() == gr->get_grund_hang()) {
				if (weg_t* const weg = gr->get_weg(road_wt)) {
					if (i < 4) {
						// update directions (SENW)
						streetdir += (1 << i);
					}
					// if not current city road standard, then replace it
					if (weg->get_besch() != welt->get_city_road()) {
						spieler_t *sp = weg->get_besitzer();
						if (sp == NULL  ||  !gr->get_depot()) {
							spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());

							weg->set_besitzer(NULL); // make public
							weg->set_besch(welt->get_city_road());
							weg->set_gehweg(true);
						}
					}
					gr->calc_bild();
					reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
				} else if (gr->get_typ() == grund_t::fundament) {
					// do not renovate, if the building is already in a neighbour tile
					gebaeude_t const* const gb = ding_cast<gebaeude_t>(gr->first_obj());
					if (gb != NULL && gb->get_tile()->get_besch() == h) {
						return return_value; //it will return false
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
		gb->set_tile( h->get_tile(gebaeude_layout[streetdir], 0, 0) );
		welt->lookup_kartenboden(k)->calc_bild();
		update_gebaeude_from_stadt(gb);
		return_value = true;

		switch (will_haben) {
			case gebaeude_t::wohnung:   won += h->get_level() * 10; break;
			case gebaeude_t::gewerbe:   arb += h->get_level() * 20; break;
			case gebaeude_t::industrie: arb += h->get_level() * 20; break;
			default: break;
		}
	}
	return return_value;
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

	// determine now, in which directions we can connect to another road
	ribi_t::ribi connection_roads = ribi_t::keine;
	// add ribi's to connection_roads if possible
	for (int r = 0; r < 4; r++) {
		if (ribi_t::nsow[r] & allowed_dir) {
			// now we have to check for several problems ...
			grund_t* bd2;
			if(bd->get_neighbour(bd2, invalid_wt, koord::nsow[r])) {
				if(bd2->get_typ()==grund_t::fundament) {
					// not connecting to a building of course ...
				} else if (bd2->get_typ()!=grund_t::boden  &&  ribi_t::nsow[r]!=ribi_typ(bd2->get_grund_hang())) {
					// not the same slope => tunnel or bridge
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
							if((ribi_t::doppelt( ribi_t::layout_to_ribi[gb->get_tile()->get_layout()] )&ribi_t::nsow[r])!=0) {
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
		return true;
	}

	return false;
}


// will check a single random pos in the city, then baue will be called
void stadt_t::baue(bool new_town)
{
	if(welt->get_settings().get_quick_city_growth())
	{
		// Old system (from Standard) - faster but less accurate.

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
			uint32 offset = simrand(num_road_rules, "void stadt_t::baue");	// start with random rule
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
			offset = simrand(num_house_rules, "void stadt_t::baue");	// start with random rule
			for (uint32 i = 0; i < num_house_rules  &&  !best_haus.found(); i++) {
				uint32 rule = ( i+offset ) % num_house_rules;
				bewerte_haus(k, 8 + house_rules[rule]->chance, *house_rules[rule]);
			}
			// one rule applied?
			if (best_haus.found()) {
				baue_gebaeude(best_haus.get_pos(), new_town);
				INT_CHECK("simcity 1163");
				return;
			}
		}
	}

	// renovation (only done when nothing matches a certain location)
	koord c( (ur.x + lo.x)/2 , (ur.y + lo.y)/2);
	double maxdist(koord_distance(ur,c));
	if (maxdist < 10) {maxdist = 10;}
	int was_renovated=0;
	int try_nr = 0;
	if (!buildings.empty() && simrand(100, "void stadt_t::baue") <= renovation_percentage  ) {
		while (was_renovated < renovations_count && try_nr++ < renovations_try) { // trial an errors parameters
			// try to find a public owned building
			gebaeude_t* const gb = pick_any(buildings);
			double dist(koord_distance(c, gb->get_pos()));
			uint32 distance_rate = uint32(100 * (1.0 - dist/maxdist));
			if(  spieler_t::check_owner(gb->get_besitzer(),NULL)  && simrand(100, "void stadt_t::baue") < distance_rate) {
				if(renoviere_gebaeude(gb)) { was_renovated++;}
			}
		}
		INT_CHECK("simcity 3746");
	}
	if(!was_renovated && !welt->get_settings().get_quick_city_growth())
	{
		// firstly, determine all potential candidate coordinates
		vector_tpl<koord> candidates( (ur.x - lo.x + 1) * (ur.y - lo.y + 1) );
		for(  sint16 j=lo.y;  j<=ur.y;  ++j  ) {
			for(  sint16 i=lo.x;  i<=ur.x;  ++i  ) {
				const koord k(i, j);
				// do not build on any border tile
				if(  !welt->ist_in_kartengrenzen( k+koord(1,1) )  ||  k.x<=0  ||  k.y<=0  ) {
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
			const uint32 idx = simrand( candidates.get_count(), "void stadt_t::baue" );
			const koord k = candidates[idx];

			// we can stop after we have found a positive rule
			best_strasse.reset(k);
			const uint32 num_road_rules = road_rules.get_count();
			uint32 offset = simrand(num_road_rules, "void stadt_t::baue");	// start with random rule
			for (uint32 i = 0; i < num_road_rules  &&  !best_strasse.found(); i++) {
				uint32 rule = ( i+offset ) % num_road_rules;
				bewerte_strasse(k, 8 + road_rules[rule]->chance, *road_rules[rule]);
			}
			// ok => then built road
			if (best_strasse.found()) {
				baue_strasse(best_strasse.get_pos(), NULL, false);
				INT_CHECK("simcity 3787");
				return;
			}

			// not good for road => test for house

			// we can stop after we have found a positive rule
			best_haus.reset(k);
			const uint32 num_house_rules = house_rules.get_count();
			offset = simrand(num_house_rules, "void stadt_t::baue");	// start with random rule
			for (uint32 i = 0; i < num_house_rules  &&  !best_haus.found(); i++) {
				uint32 rule = ( i+offset ) % num_house_rules;
				bewerte_haus(k, 8 + house_rules[rule]->chance, *house_rules[rule]);
			}
			// one rule applied?
			if (best_haus.found()) {
				baue_gebaeude(best_haus.get_pos(), new_town);
				INT_CHECK("simcity 3804");
				return;
			}

			candidates.remove_at(idx, false);
		}
	}
}

vector_tpl<koord>* stadt_t::random_place(const karte_t* wl, const vector_tpl<sint32> *sizes_list, sint16 old_x, sint16 old_y)
{
	unsigned number_of_clusters = umgebung_t::number_of_clusters;
	unsigned cluster_size = umgebung_t::cluster_size;
	const int grid_step = 8;
	const double distance_scale = 1.0/(grid_step * 2);
	const double population_scale = 1.0/1024;
	double water_charge = sqrt(umgebung_t::cities_like_water/100.0); // should be from 0 to 1.0
	double water_part;
	double terrain_part;
	if (!umgebung_t::cities_ignore_height) {
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
		if (hausbauer_t::get_special(0, haus_besch_t::rathaus, wl->get_timeline_year_month(), false, (climate)i)) {
			cl |= (1 << i);
		}
	}
	DBG_DEBUG("karte_t::init()", "get random places in climates %x", cl);
	// search at least places which are 5x5 squares large
	slist_tpl<koord>* list = wl->finde_plaetze( 5, 5, (climate_bits)cl, old_x, old_y);
	DBG_DEBUG("karte_t::init()", "found %i places", list->get_count());
	unsigned int weight_max;
	// unsigned long here -- from weighted_vector_tpl.h(weight field type)
	if ( list->get_count() == 0 || (std::numeric_limits<unsigned long>::max)()/ list->get_count() > 65535) {
		weight_max = 65535;
	}
	else {
		weight_max = (std::numeric_limits<unsigned long>::max)()/list->get_count();
	}

	// max 1 city from each square can be built
	// each entry represents a cell of grid_step length and width
	const int xmax = wl->get_groesse_x()/grid_step + 1;
	const int ymax = wl->get_groesse_y()/grid_step + 1;
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
	for( pos.y = 2; pos.y < wl->get_groesse_y()-2; pos.y++) {
		for (pos.x = 2; pos.x < wl->get_groesse_x()-2; pos.x++ ) {
			koord my_grid_pos(pos.x/grid_step, pos.y/grid_step);
			grund_t *gr = wl->lookup_kartenboden(pos);
			if ( gr->get_hoehe() <= wl->get_grundwasser()  || ( gr->hat_weg(water_wt) && gr->get_max_speed() )  ) {
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

	for ( pos.y = 1; pos.y < wl->get_groesse_y(); pos.y++) {
		for (pos.x = 1; pos.x < wl->get_groesse_x(); pos.x++) {
			double f;
			if (umgebung_t::cities_ignore_height) {
				f = 0.0;
			}
			else {
				int weight;
				const sint16 height_above_water = wl->lookup_hgt(pos) - wl->get_grundwasser();
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
					const double distance = accurate_distance(k, central_pos) * distance_scale;
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
	// The 'magic number' in here is the actual amount of electricity consumed per citizen per month at '100%' in electricity.tab
	//uint16 electricity_per_citizen = 0.02F * get_electricity_consumption(welt->get_timeline_year_month(); 
	uint16 electricity_per_citizen = (get_electricity_consumption(welt->get_timeline_year_month()) * 100) / 5; 
	// The weird order of operations is designed for greater precision.
	// Really, POWER_TO_MW should come last.

	return ((city_history_month[0][HIST_CITICENS] << POWER_TO_MW) * electricity_per_citizen) / 100000;
}

void stadt_t::add_substation(senke_t* substation)
{ 
	substations.append(substation); 
}

void stadt_t::remove_substation(senke_t* substation)
{ 
	substations.remove(substation); 
}

bool road_destination_finder_t::ist_befahrbar( const grund_t* gr ) const
{ 
	// Check to see whether the road prohibits private cars
	if(welt->lookup(gr->get_pos())->get_weg(road_wt) && welt->lookup(gr->get_pos())->get_weg(road_wt)->has_sign())
	{
		const roadsign_besch_t* rs_besch = gr->find<roadsign_t>()->get_besch();
		if(rs_besch->is_private_way())
		{
			return false;
		}
	}
	return master->ist_befahrbar(gr);
}

bool road_destination_finder_t::ist_ziel( const grund_t* gr, const grund_t* ) const 
{ 
	return gr->get_pos() == dest;
}

ribi_t::ribi road_destination_finder_t::get_ribi( const grund_t* gr) const
{ 
	return master->get_ribi(gr); 
}

int road_destination_finder_t::get_kosten( const grund_t* gr, sint32 max_speed, koord from_pos) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(road_wt);
	if(!w) {
		return 0xFFFF;
	}

	// max_speed?
	uint32 max_tile_speed = w->get_max_speed();

	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	if(w->is_diagonal())
	{
		// Diagonals are a *shorter* distance.
		costs /= 1.4;
	}

	return costs;
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
