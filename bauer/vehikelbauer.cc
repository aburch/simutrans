/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include "../player/simplay.h"
#include "../simdebug.h"
#include "../utils/simrandom.h"
#include "../simtypes.h"

#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "../descriptor/vehicle_desc.h"
#include "../gui/depot_frame.h"

#include "vehikelbauer.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../vehicle/air_vehicle.h"
#include "../vehicle/rail_vehicle.h"
#include "../vehicle/road_vehicle.h"
#include "../vehicle/water_vehicle.h"


const char* vehicle_builder_t::engine_type_names[9] =
{
	"unknown",
	"steam",
	"diesel",
	"electric",
	"bio",
	"sail",
	"fuel_cell",
	"hydrogene",
	"battery"
};

const char *vehicle_builder_t::vehicle_sort_by[vehicle_builder_t::sb_length] =
{
	"Fracht",
	"Vehicle Name",
	"Capacity",
	"Price",
	"Cost",
	"Cost per unit",
	"Max. speed",
	"Vehicle Power",
	"Weight",
	"Intro. date",
	"Retire date"
};

static stringhashtable_tpl<const vehicle_desc_t*> name_fahrzeuge;

// index 0 aur, 1...8 at normal waytype index
#define GET_WAYTYPE_INDEX(wt) ((int)(wt)>8 ? 0 : (wt))
static slist_tpl<const vehicle_desc_t*> typ_fahrzeuge[vehicle_builder_t::sb_length][9];
static uint8 tmp_sort_idx;

class bonus_record_t {
public:
	sint32 year;
	sint32 speed;
	bonus_record_t( sint32 y=0, sint32 kmh=0 ) {
		year = y*12;
		speed = kmh;
	}
	void rdwr(loadsave_t *file) {
		file->rdwr_long(year);
		file->rdwr_long(speed);
	}
};

// speed bonus
static vector_tpl<bonus_record_t>speedbonus[8];

static sint32 default_speedbonus[8] =
{
	60,  // road
	80,  // track
	35,  // water
	350, // air
	80,  // monorail
	200, // maglev
	60,  // tram
	60   // narrowgauge
};

bool vehicle_builder_t::speedbonus_init(const std::string &objfilename)
{
	tabfile_t bonusconf;
	// first take user data, then user global data
	if (!bonusconf.open((objfilename+"config/speedbonus.tab").c_str())) {
		dbg->warning("vehicle_builder_t::speedbonus_init()", "Can't read speedbonus.tab" );
		return false;
	}

	tabfileobj_t contents;
	bonusconf.read(contents);

	/* init the values from line with the form year, speed, year, speed
	 * must be increasing order!
	 */
	for(  int j=0;  j<8;  j++  ) {
		vector_tpl<int> tracks = contents.get_ints(weg_t::waytype_to_string(j==3?air_wt:(waytype_t)(j+1)));

		if((tracks.get_count() % 2) != 0) {
			dbg->warning( "vehicle_builder_t::speedbonus_init()", "Ill formed line in speedbonus.tab\nFormat is year,speed[,year,speed]!" );
			tracks.pop_back();
		}

		speedbonus[j].resize( tracks.get_count()/2 );
		for(  uint32 i=0;  i<tracks.get_count();  i+=2  ) {
			bonus_record_t b( tracks[i], tracks[i+1] );
			speedbonus[j].append( b );
		}
	}

	return true;
}


sint32 vehicle_builder_t::get_speedbonus( sint32 monthyear, waytype_t wt )
{
	const int typ = wt==air_wt ? 3 : (wt-1)&7;

	if(  monthyear==0  ) {
		return default_speedbonus[typ];
	}

	// ok, now lets see if we have data for this
	vector_tpl<bonus_record_t> const& b = speedbonus[typ];
	if (!b.empty()) {
		uint i=0;
		while (i < b.get_count() && monthyear >= b[i].year) {
			i++;
		}
		if (i == b.get_count()) {
			// maxspeed already?
			return b[i - 1].speed;
		}
		else if(i==0) {
			// minspeed below
			return b[0].speed;
		}
		else {
			// interpolate linear
			sint32 const delta_speed = b[i].speed - b[i - 1].speed;
			sint32 const delta_years = b[i].year  - b[i - 1].year;
			return delta_speed * (monthyear - b[i - 1].year) / delta_years + b[i - 1].speed;
		}
	}
	else {
		sint32 speed_sum = 0;
		sint32 num_averages = 0;
		// needs to do it the old way => iterate over all vehicles with this type ...
		FOR(slist_tpl<vehicle_desc_t const*>, const info, typ_fahrzeuge[0][GET_WAYTYPE_INDEX(wt)]) {
			if(  info->get_power()>0  &&  info->is_available(monthyear)  ) {
				speed_sum += info->get_topspeed();
				num_averages ++;
			}
		}
		if(  num_averages>0  ) {
			return speed_sum / num_averages;
		}
	}

	// no vehicles => return default
	return default_speedbonus[typ];
}


void vehicle_builder_t::rdwr_speedbonus(loadsave_t *file)
{
	for(  int j=0;  j<8;  j++  ) {
		uint32 count = speedbonus[j].get_count();
		file->rdwr_long(count);
		if (file->is_loading()) {
			speedbonus[j].clear();
			speedbonus[j].resize(count);
		}
		for(uint32 i=0; i<count; i++) {
			if (file->is_loading()) {
				speedbonus[j].append(bonus_record_t());
			}
			speedbonus[j][i].rdwr(file);
		}
	}
}


vehicle_t* vehicle_builder_t::build(koord3d k, player_t* player, convoi_t* cnv, const vehicle_desc_t* vb )
{
	vehicle_t* v;
	switch (vb->get_waytype()) {
		case road_wt:     v = new road_vehicle_t(      k, vb, player, cnv); break;
		case monorail_wt: v = new monorail_vehicle_t(k, vb, player, cnv); break;
		case track_wt:
		case tram_wt:     v = new rail_vehicle_t(         k, vb, player, cnv); break;
		case water_wt:    v = new water_vehicle_t(         k, vb, player, cnv); break;
		case air_wt:      v = new air_vehicle_t(       k, vb, player, cnv); break;
		case maglev_wt:   v = new maglev_vehicle_t(  k, vb, player, cnv); break;
		case narrowgauge_wt:v = new narrowgauge_vehicle_t(k, vb, player, cnv); break;

		default:
			dbg->fatal("vehicle_builder_t::build()", "cannot built a vehicle with waytype %i", vb->get_waytype());
	}

	player->book_new_vehicle(-(sint64)vb->get_price(), k.get_2d(), vb->get_waytype() );

	return v;
}



bool vehicle_builder_t::register_desc(const vehicle_desc_t *desc)
{
	// register waytype list
	const int wt_idx = GET_WAYTYPE_INDEX( desc->get_waytype() );

	// first hashtable
	vehicle_desc_t const *old_desc = name_fahrzeuge.get( desc->get_name() );
	if(  old_desc  ) {
		dbg->doubled( "vehicle", desc->get_name() );
		name_fahrzeuge.remove( desc->get_name() );
	}
	name_fahrzeuge.put(desc->get_name(), desc);

	// now add it to sorter (may be more than once!)
	for(  int sort_idx = 0;  sort_idx < vehicle_builder_t::sb_length;  sort_idx++  ) {
		if(  old_desc  ) {
			typ_fahrzeuge[sort_idx][wt_idx].remove(old_desc);
		}
		typ_fahrzeuge[sort_idx][wt_idx].append(desc);
	}
	// we cannot delete old_desc, since then xref-resolving will crash

	return true;
}



// compare funcions to sort vehicle in the list
static int compare_freight(const vehicle_desc_t* a, const vehicle_desc_t* b)
{
	int cmp = a->get_freight_type()->get_catg() - b->get_freight_type()->get_catg();
	if (cmp != 0) return cmp;
	if (a->get_freight_type()->get_catg() == 0) {
		cmp = a->get_freight_type()->get_index() - b->get_freight_type()->get_index();
	}
	return cmp;
}
static int compare_capacity(const vehicle_desc_t* a, const vehicle_desc_t* b) { return a->get_capacity() - b->get_capacity(); }
static int compare_engine(const vehicle_desc_t* a, const vehicle_desc_t* b) {
	return (a->get_capacity() + a->get_power() == 0 ? (uint8)vehicle_desc_t::steam : a->get_engine_type()) - (b->get_capacity() + b->get_power() == 0 ? (uint8)vehicle_desc_t::steam : b->get_engine_type());
}
static int compare_price(const vehicle_desc_t* a, const vehicle_desc_t* b) { return a->get_price() - b->get_price(); }
static int compare_cost(const vehicle_desc_t* a, const vehicle_desc_t* b) { return a->get_running_cost() - b->get_running_cost(); }
static int compare_cost_per_unit(const vehicle_desc_t* a, const vehicle_desc_t* b) { return a->get_running_cost()*b->get_capacity() - b->get_running_cost()*a->get_capacity(); }
static int compare_topspeed(const vehicle_desc_t* a, const vehicle_desc_t* b) {return a->get_topspeed() - b->get_topspeed();}
static int compare_power(const vehicle_desc_t* a, const vehicle_desc_t* b) {return (a->get_power() == 0 ? 0x7FFFFFF : a->get_power()) - (b->get_power() == 0 ? 0x7FFFFFF : b->get_power());}
static int compare_weight(const vehicle_desc_t* a, const vehicle_desc_t* b) {return a->get_weight() - b->get_weight();}
static int compare_intro_year_month(const vehicle_desc_t* a, const vehicle_desc_t* b) {return a->get_intro_year_month() - b->get_intro_year_month();}
static int compare_retire_year_month(const vehicle_desc_t* a, const vehicle_desc_t* b) {return a->get_retire_year_month() - b->get_retire_year_month();}



// default compare function with mode parameter
bool vehicle_builder_t::compare_vehicles(const vehicle_desc_t* a, const vehicle_desc_t* b, sort_mode_t mode )
{
	int cmp = 0;
	switch (mode) {
		case sb_freight:
			cmp = compare_freight(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_name:
			cmp = strcmp(translator::translate(a->get_name()), translator::translate(b->get_name()));
			if (cmp != 0) return cmp < 0;
			break;
		case sb_price:
			cmp = compare_price(a, b);
			if (cmp != 0) return cmp < 0;
			// fall-through
		case sb_cost:
			cmp = compare_cost(a, b);
			if (cmp != 0) return cmp < 0;
			cmp = compare_price(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_cost_per_unit:
			cmp = compare_cost_per_unit(a,b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_speed:
			cmp = compare_topspeed(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_weight:
			cmp = compare_weight(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_power:
			cmp = compare_power(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		case sb_intro_date:
			cmp = compare_intro_year_month(a, b);
			if (cmp != 0) return cmp < 0;
			// fall-through
		case sb_retire_date:
			cmp = compare_retire_year_month(a, b);
			if (cmp != 0) return cmp < 0;
			cmp = compare_intro_year_month(a, b);
			if (cmp != 0) return cmp < 0;
			break;
		default: ;
	}
	// Sort by:
	//  1. cargo category
	//  2. cargo (if special freight)
	//  3. engine_type
	//  4. speed
	//  5. power
	//  6. intro date
	//  7. name
	cmp = compare_freight(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = compare_capacity(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = compare_engine(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = compare_topspeed(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = compare_power(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = compare_intro_year_month(a, b);
	if (cmp != 0) return cmp < 0;
	cmp = strcmp(translator::translate(a->get_name()), translator::translate(b->get_name()));
	return cmp < 0;
}


static bool compare( const vehicle_desc_t* a, const vehicle_desc_t* b )
{
	return vehicle_builder_t::compare_vehicles( a, b, (vehicle_builder_t::sort_mode_t)tmp_sort_idx );
}


bool vehicle_builder_t::successfully_loaded()
{
	// first: check for bonus tables
	DBG_MESSAGE("vehicle_builder_t::sort_lists()","called");
	for (  int sort_idx = 0; sort_idx < vehicle_builder_t::sb_length; sort_idx++  ) {
		for(  int wt_idx=0;  wt_idx<9;  wt_idx++  ) {
			tmp_sort_idx = sort_idx;
			slist_tpl<const vehicle_desc_t*>& typ_liste = typ_fahrzeuge[sort_idx][wt_idx];
			uint count = typ_liste.get_count();
			if (count == 0) {
				continue;
			}
			const vehicle_desc_t** const tmp     = new const vehicle_desc_t*[count];
			const vehicle_desc_t** const tmp_end = tmp + count;
			for(  const vehicle_desc_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
				*tmpptr = typ_liste.remove_first();
			}
			std::sort(tmp, tmp_end, compare);
			for(  const vehicle_desc_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
				typ_liste.append(*tmpptr);
			}
			delete [] tmp;
		}
	}
	return true;
}



const vehicle_desc_t *vehicle_builder_t::get_info(const char *name)
{
	return name_fahrzeuge.get(name);
}


slist_tpl<vehicle_desc_t const*> const & vehicle_builder_t::get_info(waytype_t const typ, uint8 sortkey)
{
	return typ_fahrzeuge[sortkey][GET_WAYTYPE_INDEX(typ)];
}


/**
 * extended search for vehicles for AI
 * checks also timeline and constraints
 * tries to get best with but adds a little random action
 */
const vehicle_desc_t *vehicle_builder_t::vehicle_search( waytype_t wt, const uint16 month_now, const uint32 target_weight, const sint32 target_speed, const goods_desc_t * target_freight, bool include_electric, bool not_obsolete )
{
	const vehicle_desc_t *desc = NULL;
	sint32 desc_index = -100000;

	if(  target_freight==NULL  &&  target_weight==0  ) {
		// no power, no freight => no vehicle to search
		return NULL;
	}

	FOR(slist_tpl<vehicle_desc_t const*>, const test_desc, typ_fahrzeuge[0][GET_WAYTYPE_INDEX(wt)]) {
		// no constricts allow for rail vehicles concerning following engines
		if(wt==track_wt  &&  !test_desc->can_follow_any()  ) {
			continue;
		}
		// do not buy incomplete vehicles
		if(wt==road_wt && !test_desc->can_lead(NULL)) {
			continue;
		}

		// engine, but not allowed to lead a convoi, or no power at all or no electrics allowed
		if(target_weight) {
			if(test_desc->get_power()==0  ||  !test_desc->can_follow(NULL)  ||  (!include_electric  &&  test_desc->get_engine_type()==vehicle_desc_t::electric) ) {
				continue;
			}
		}

		// check for waytype too
		if(test_desc->get_waytype()!=wt  ||  test_desc->is_future(month_now)  ) {
			continue;
		}

		if(  not_obsolete  &&  test_desc->is_retired(month_now)  ) {
			// not using vintage cars here!
			continue;
		}

		uint32 power = (test_desc->get_power()*test_desc->get_gear())/64;
		if(target_freight) {
			// this is either a railcar/trailer or a truck/boat/plane
			if(  test_desc->get_capacity()==0  ||  !test_desc->get_freight_type()->is_interchangeable(target_freight)  ) {
				continue;
			}

			sint32 difference=0; // smaller is better
			// assign this vehicle if we have not found one yet, or we only found one too weak
			if(  desc!=NULL  ) {
				// is it cheaper to run? (this is most important)
				difference += (desc->get_capacity()*1000)/(1+desc->get_running_cost()) < (test_desc->get_capacity()*1000)/(1+test_desc->get_running_cost()) ? -20 : 20;
				if(  target_weight>0  ) {
					// is it stronger?
					difference += (desc->get_power()*desc->get_gear())/64 < power ? -10 : 10;
				}
				// is it faster? (although we support only up to 120km/h for goods)
				difference += (desc->get_topspeed() < test_desc->get_topspeed())? -10 : 10;
				// is it cheaper? (not so important)
				difference += (desc->get_price() > test_desc->get_price())? -5 : 5;
				// add some malus for obsolete vehicles
				if(test_desc->is_retired(month_now)) {
					difference += 5;
				}
			}
			// ok, final check
			if(  desc==NULL  ||  difference<(int)simrand(25)    ) {
				// then we want this vehicle!
				desc = test_desc;
				DBG_MESSAGE( "vehicle_builder_t::vehicle_search","Found car %s",desc->get_name());
			}
		}

		else {
			// engine/tugboat/truck for trailer
			if(  test_desc->get_capacity()!=0  ||  !test_desc->can_follow(NULL)  ) {
				continue;
			}
			// finally, we might be able to use this vehicle
			sint32 speed = test_desc->get_topspeed();
			uint32 max_weight = power/( (speed*speed)/2500 + 1 );

			// we found a useful engine
			sint32 current_index = (power * 100) / (1 + test_desc->get_running_cost()) + test_desc->get_topspeed() - test_desc->get_weight() / 1000 - (sint32)(test_desc->get_price() / 25000);
			// too slow?
			if(speed < target_speed) {
				current_index -= 250;
			}
			// too weak to reach full speed?
			if(  max_weight < target_weight+test_desc->get_weight()/1000  ) {
				current_index += max_weight - (sint32)(target_weight+test_desc->get_weight()/1000);
			}
			current_index += simrand(100);
			if(  current_index > desc_index  ) {
				// then we want this vehicle!
				desc = test_desc;
				desc_index = current_index;
				DBG_MESSAGE( "vehicle_builder_t::vehicle_search","Found engine %s",desc->get_name());
			}
		}
	}
	// no vehicle found!
	if(  desc==NULL  ) {
		DBG_MESSAGE( "vehicle_builder_t::vehicle_search()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	}
	return desc;
}



/**
 * extended search for vehicles for replacement on load time
 * tries to get best match (no random action)
 * if prev_desc==NULL, then the convoi must be able to lead a convoi
 */
const vehicle_desc_t *vehicle_builder_t::get_best_matching( waytype_t wt, const uint16 month_now, const uint32 target_weight, const uint32 target_power, const sint32 target_speed, const goods_desc_t * target_freight, bool not_obsolete, const vehicle_desc_t *prev_veh, bool is_last )
{
	const vehicle_desc_t *desc = NULL;
	sint32 desc_index = -100000;

	FOR(slist_tpl<vehicle_desc_t const*>, const test_desc, typ_fahrzeuge[0][GET_WAYTYPE_INDEX(wt)]) {
		if(target_power>0  &&  test_desc->get_power()==0) {
			continue;
		}

		// will test for first (prev_veh==NULL) or matching following vehicle
		if(!test_desc->can_follow(prev_veh)) {
			continue;
		}

		// not allowed as last vehicle
		if(is_last  &&  test_desc->get_trailer_count()>0  &&  test_desc->get_trailer(0)!=NULL  ) {
			continue;
		}

		// not allowed as non-last vehicle
		if(!is_last  &&  test_desc->get_trailer_count()==1  &&  test_desc->get_trailer(0)==NULL  ) {
			continue;
		}

		// check for waytype too
		if(test_desc->get_waytype()!=wt  ||  test_desc->is_future(month_now)  ) {
			continue;
		}

		// ignore vehicles that need electrification
		if(test_desc->get_power()>0  &&  test_desc->get_engine_type()==vehicle_desc_t::electric) {
			continue;
		}

		// likely tender => replace with some engine ...
		if(target_freight==0  &&  target_weight==0) {
			if(  test_desc->get_capacity()!=0  ) {
				continue;
			}
		}

		if(  not_obsolete  &&  test_desc->is_retired(month_now)  ) {
			// not using vintage cars here!
			continue;
		}

		uint32 power = (test_desc->get_power()*test_desc->get_gear())/64;
		if(target_freight) {
			// this is either a railcar/trailer or a truck/boat/plane
			if(  test_desc->get_capacity()==0  ||  !test_desc->get_freight_type()->is_interchangeable(target_freight)  ) {
				continue;
			}

			sint32 difference=0; // smaller is better
			// assign this vehicle if we have not found one yet, or we only found one too weak
			if(  desc!=NULL  ) {
				// is it cheaper to run? (this is most important)
				difference += (desc->get_capacity()*1000)/(1+desc->get_running_cost()) < (test_desc->get_capacity()*1000)/(1+test_desc->get_running_cost()) ? -20 : 20;
				if(  target_weight>0  ) {
					// is it stronger?
					difference += (desc->get_power()*desc->get_gear())/64 < power ? -10 : 10;
				}
				// is it faster? (although we support only up to 120km/h for goods)
				difference += (desc->get_topspeed() < test_desc->get_topspeed())? -10 : 10;
				// is it cheaper? (not so important)
				difference += (desc->get_price() > test_desc->get_price())? -5 : 5;
				// add some malus for obsolete vehicles
				if(test_desc->is_retired(month_now)) {
					difference += 5;
				}
			}
			// ok, final check
			if(  desc==NULL  ||  difference<12    ) {
				// then we want this vehicle!
				desc = test_desc;
				DBG_MESSAGE( "vehicle_builder_t::get_best_matching","Found car %s",desc->get_name());
			}
		}
		else {
			// finally, we might be able to use this vehicle
			sint32 speed = test_desc->get_topspeed();
			uint32 max_weight = power/( (speed*speed)/2500 + 1 );

			// we found a useful engine
			sint32 current_index = (power * 100) / (1 + test_desc->get_running_cost()) + test_desc->get_topspeed() - (sint16)test_desc->get_weight() / 1000 - (sint32)(test_desc->get_price() / 25000);
			// too slow?
			if(speed < target_speed) {
				current_index -= 250;
			}
			// too weak to reach full speed?
			if(  max_weight < target_weight+test_desc->get_weight()/1000  ) {
				current_index += max_weight - (sint32)(target_weight+test_desc->get_weight()/1000);
			}
			current_index += 50;
			if(  current_index > desc_index  ) {
				// then we want this vehicle!
				desc = test_desc;
				desc_index = current_index;
				DBG_MESSAGE( "vehicle_builder_t::get_best_matching","Found engine %s",desc->get_name());
			}
		}
	}
	// no vehicle found!
	if(  desc==NULL  ) {
		DBG_MESSAGE( "vehicle_builder_t::get_best_matching()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	}
	return desc;
}

sint32 vehicle_builder_t::get_fastest_vehicle_speed(waytype_t wt, uint16 const month_now, bool const use_timeline, bool const allow_obsolete)
{
	sint32 fastest_speed = 0;

	FOR(slist_tpl<vehicle_desc_t const*>, const vehicle_descriptor, typ_fahrzeuge[0][GET_WAYTYPE_INDEX(wt)]) {
		if (vehicle_descriptor->get_power() == 0 ||
			(use_timeline && (
				vehicle_descriptor->is_future(month_now) ||
				(!allow_obsolete && vehicle_descriptor->is_retired(month_now))))) {
			continue;
		}

		sint32 const speed = vehicle_descriptor->get_topspeed();
		if (fastest_speed < speed) {
			fastest_speed = speed;
		}
	}

	return fastest_speed;
}
