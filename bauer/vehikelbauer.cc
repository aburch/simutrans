/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../simdebug.h"
#include "../utils/simrandom.h"
#include "../simtypes.h"

#include "../simconvoi.h"

#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/livery_scheme.h"

#include "../descriptor/vehicle_desc.h"

#include "vehikelbauer.h"

#include "../tpl/stringhashtable_tpl.h"

static stringhashtable_tpl< vehicle_desc_t*> name_fahrzeuge;

// index 0 aur, 1...8 at normal waytype index
#define GET_WAYTYPE_INDEX(wt) ((int)(wt)>8 ? 0 : (wt))
static slist_tpl<vehicle_desc_t*> typ_fahrzeuge[9];


void vehicle_builder_t::rdwr_speedbonus(loadsave_t *file)
{
	// Former speed bonus settings, now deprecated.
	// This is necessary to load old saved games that
	// contained speed bonus data.
	for(uint32 j = 0; j < 8; j++)
	{
		uint32 count = 1;
		file->rdwr_long(count);

		for(uint32 i=0; i<count; i++)
		{
			sint64 dummy = 0;
			uint32 dummy_2 = 0;
			file->rdwr_longlong(dummy);
			file->rdwr_long(dummy_2);
		}
	}
}


vehicle_t* vehicle_builder_t::build(koord3d k, player_t* player, convoi_t* cnv, const vehicle_desc_t* vb, bool upgrade, uint16 livery_scheme_index )
{
	vehicle_t* v;
	static karte_ptr_t welt;
	switch (vb->get_waytype()) {
		case road_wt:     v = new road_vehicle_t(      k, vb, player, cnv); break;
		case monorail_wt: v = new monorail_rail_vehicle_t(k, vb, player, cnv); break;
		case track_wt:
		case tram_wt:     v = new rail_vehicle_t(         k, vb, player, cnv); break;
		case water_wt:    v = new water_vehicle_t(         k, vb, player, cnv); break;
		case air_wt:      v = new air_vehicle_t(       k, vb, player, cnv); break;
		case maglev_wt:   v = new maglev_rail_vehicle_t(  k, vb, player, cnv); break;
		case narrowgauge_wt:v = new narrowgauge_rail_vehicle_t(k, vb, player, cnv); break;

		default:
			dbg->fatal("vehicle_builder_t::build()", "cannot built a vehicle with waytype %i", vb->get_waytype());
	}

	if(cnv)
	{
		livery_scheme_index = cnv->get_livery_scheme_index();
	}

	if(livery_scheme_index >= welt->get_settings().get_livery_schemes()->get_count())
	{
		// To prevent errors when loading a game with fewer livery schemes than that just played.
		livery_scheme_index = 0;
	}

	const livery_scheme_t* const scheme = welt->get_settings().get_livery_scheme(livery_scheme_index);
	uint16 date = welt->get_timeline_year_month();
	if(scheme)
	{
		const char* livery = scheme->get_latest_available_livery(date, vb);
		if(livery)
		{
			v->set_current_livery(livery);
		}
		else
		{
			bool found = false;
			for(uint32 j = 0; j < welt->get_settings().get_livery_schemes()->get_count(); j ++)
			{
				const livery_scheme_t* const new_scheme = welt->get_settings().get_livery_scheme(j);
				const char* new_livery = new_scheme->get_latest_available_livery(date, vb);
				if(new_livery)
				{
					v->set_current_livery(new_livery);
					found = true;
					break;
				}
			}
			if(!found)
			{
				v->set_current_livery("default");
			}
		}
	}
	else
	{
		v->set_current_livery("default");
	}

	sint64 price = 0;

	if(upgrade)
	{
		price = vb->get_upgrade_price();
	}
	else
	{
		price = vb->get_value();
	}
	player->book_new_vehicle(-price, k.get_2d(), vb->get_waytype() );

	return v;
}



bool vehicle_builder_t::register_desc(vehicle_desc_t *desc)
{
	// register waytype list
	const int idx = GET_WAYTYPE_INDEX( desc->get_waytype() );
	vehicle_desc_t *old_desc = name_fahrzeuge.get( desc->get_name() );
	if(  old_desc  ) {
		dbg->warning( "vehicle_builder_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
		name_fahrzeuge.remove( desc->get_name() );
		typ_fahrzeuge[idx].remove(old_desc);
	}
	name_fahrzeuge.put(desc->get_name(), desc);
	typ_fahrzeuge[idx].append(desc);
	return true;
}


static bool compare_vehicle_desc(const vehicle_desc_t* a, const vehicle_desc_t* b)
{
	// Sort by:
	//  1. cargo category
	//  2. cargo (if special freight)
	//  3. engine_type
	//  4. speed
	//  5. power
	//  6. intro date
	//  7. name
	int cmp = a->get_freight_type()->get_catg() - b->get_freight_type()->get_catg();
	if (cmp == 0) {
		if (a->get_freight_type()->get_catg() == 0) {
			cmp = a->get_freight_type()->get_index() - b->get_freight_type()->get_index();
		}
		if (cmp == 0) {
			cmp = a->get_total_capacity() - b->get_total_capacity();
			if (cmp == 0) {
				// to handle tender correctly
				uint8 b_engine = (a->get_total_capacity() + a->get_power() == 0 ? (uint8)vehicle_desc_t::steam : a->get_engine_type());
				uint8 a_engine = (b->get_total_capacity() + b->get_power() == 0 ? (uint8)vehicle_desc_t::steam : b->get_engine_type());
				cmp = b_engine - a_engine;
				if (cmp == 0) {
					cmp = a->get_topspeed() - b->get_topspeed();
					if (cmp == 0) {
						// put tender at the end of the list ...
						int b_power = (a->get_power() == 0 ? 0x7FFFFFF : a->get_power());
						int a_power = (b->get_power() == 0 ? 0x7FFFFFF : b->get_power());
						cmp = b_power - a_power;
						if (cmp == 0) {
							cmp = a->get_intro_year_month() - b->get_intro_year_month();
							if (cmp == 0) {
								cmp = strcmp(a->get_name(), b->get_name());
							}
						}
					}
				}
			}
		}
	}
	return cmp < 0;
}


bool vehicle_builder_t::successfully_loaded()
{
	// first: check for bonus tables
	DBG_MESSAGE("vehicle_builder_t::sort_lists()","called");
	for(  int wt_idx=0;  wt_idx<9;  wt_idx++  ) {
		slist_tpl<vehicle_desc_t*>& typ_liste = typ_fahrzeuge[wt_idx];
		uint count = typ_liste.get_count();
		if (count == 0) {
			continue;
		}
		vehicle_desc_t** const tmp     = new vehicle_desc_t*[count];
		vehicle_desc_t** const tmp_end = tmp + count;
		for(  vehicle_desc_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
			*tmpptr = typ_liste.remove_first();
		}
		std::sort(tmp, tmp_end, compare_vehicle_desc);
		for(  vehicle_desc_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
			typ_liste.append(*tmpptr);

			(*tmpptr)->fix_number_of_classes();
		}
		delete [] tmp;
	}


	return true;
}



const vehicle_desc_t *vehicle_builder_t::get_info(const char *name)
{
	return name_fahrzeuge.get(name);
}

slist_tpl<vehicle_desc_t*> const & vehicle_builder_t::get_info(waytype_t typ)
{
	return typ_fahrzeuge[GET_WAYTYPE_INDEX(typ)];
}

/* extended search for vehicles for KI *
 * checks also timeline and constraints
 * tries to get best with but adds a little random action
 * @author prissi
 */
const vehicle_desc_t *vehicle_builder_t::vehicle_search( waytype_t wt, const uint16 month_now, const uint32 target_weight, const sint32 target_speed, const goods_desc_t * target_freight, bool include_electric, bool not_obsolete )
{
	if(  (target_freight!=NULL  ||  target_weight!=0)  &&  !typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)].empty()  )
	{
		struct best_t {
			uint32 power;
			uint16 payload_per_maintenance;
			long index;
		} best, test;

		best.power = 0;
		best.payload_per_maintenance = 0;
		best.index = -100000;

		const vehicle_desc_t *desc = NULL;
		FOR(slist_tpl<vehicle_desc_t *>, const test_desc, typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)])
		{
			// no constricts allow for rail vehicles concerning following engines
			if(wt==track_wt  &&  !test_desc->can_follow_any()  )
			{
				continue;
			}
			// do not buy incomplete vehicles
			if(wt==road_wt && !test_desc->can_lead(NULL))
			{
				continue;
			}

			// engine, but not allowed to lead a convoi, or no power at all or no electrics allowed
			if(target_weight)
			{
				if(test_desc->get_power()==0  ||  !test_desc->can_follow(NULL)  ||  (!include_electric  &&  test_desc->get_engine_type()==vehicle_desc_t::electric) )
				{
					continue;
				}
			}

			// check for wegetype/too new
			if(test_desc->get_waytype()!=wt  ||  test_desc->is_future(month_now)  )
			{
				continue;
			}

			if(  not_obsolete  &&  test_desc->is_retired(month_now)  )
			{
				// not using vintage cars here!
				continue;
			}

			test.power = (test_desc->get_power() * test_desc->get_gear()) / 64;
			if(target_freight)
			{
				// this is either a railcar/trailer or a truck/boat/plane
				if(  test_desc->get_total_capacity()==0  ||  !test_desc->get_freight_type()->is_interchangeable(target_freight)  )
				{
					continue;
				}

				test.index = -100000;
				test.payload_per_maintenance = test_desc->get_total_capacity() / max(test_desc->get_running_cost(), 1);

				sint32 difference=0;	// smaller is better
				// assign this vehicle if we have not found one yet, or we only found one too weak
				if(  desc!=NULL  ) {
					// is it cheaper to run? (this is most important)
					difference += best.payload_per_maintenance < test.payload_per_maintenance ? -20 : 20;
					if(  target_weight>0  ) {
						// is it strongerer?
						difference += best.power < test.power ? -10 : 10;
					}
					// it is faster? (although we support only up to 120km/h for goods)
					difference += (desc->get_topspeed() < test_desc->get_topspeed())? -10 : 10;
					// it is cheaper? (not so important)
					difference += (desc->get_value() > test_desc->get_value())? -5 : 5;
					// add some malus for obsolete vehicles
					if(test_desc->is_retired(month_now)) {
						difference += 5;
					}
				}
				// ok, final check
				if(  desc==NULL  ||  difference<(int)simrand(25, "vehicle_builder_t::vehicle_search")    ) {
					// then we want this vehicle!
					desc = test_desc;
					best = test;
					DBG_MESSAGE( "vehicle_builder_t::vehicle_search","Found car %s", desc ? desc->get_name() : "null");
				}
			}
			else {
				// engine/tugboat/truck for trailer
				if(  test_desc->get_total_capacity()!=0  ||  !test_desc->can_follow(NULL)  ) {
					continue;
				}
				// finally, we might be able to use this vehicle
				sint32 speed = test_desc->get_topspeed();
				uint32 max_weight = test.power/( (speed*speed)/2500 + 1 );

				// we found a useful engine
				test.index = (test.power*100)/max(test_desc->get_running_cost(), 1) + test_desc->get_topspeed() - (sint16)test_desc->get_weight() - (sint32)(test_desc->get_value()/25000);
				// too slow?
				if(speed < target_speed) {
					test.index -= 250;
				}
				// too weak to to reach full speed?
				if(  max_weight < target_weight+test_desc->get_weight()  ) {
					test.index += max_weight - (sint32)(target_weight+test_desc->get_weight());
				}
				test.index += simrand(100, "vehicle_builder_t::vehicle_search");
				if(  test.index > best.index  ) {
					// then we want this vehicle!
					desc = test_desc;
					best = test;
					DBG_MESSAGE( "vehicle_builder_t::vehicle_search","Found engine %s",desc->get_name());
				}
			}
		}
		if (desc)
		{
			return desc;
		}
	}
	// no vehicle found!
	DBG_MESSAGE( "vehicle_builder_t::vehicle_search()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	return NULL;
}



/* extended search for vehicles for replacement on load time
 * tries to get best match (no random action)
 * if prev_desc==NULL, then the convoi must be able to lead a convoi
 * @author prissi
 */
const vehicle_desc_t *vehicle_builder_t::get_best_matching( waytype_t wt, const uint16 month_now, const uint32 target_weight, const uint32 target_power, const sint32 target_speed, const goods_desc_t * target_freight, bool not_obsolete, const vehicle_desc_t *prev_veh, bool is_last )
{
	const vehicle_desc_t *desc = NULL;
	sint32 desc_index =- 100000;

	if(  !typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)].empty()  )
	{
		FOR(slist_tpl<vehicle_desc_t *>, const test_desc, typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)])
		{
			if(target_power>0  &&  test_desc->get_power()==0)
			{
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
				if(  test_desc->get_total_capacity()!=0  ) {
					continue;
				}
			}

			if(  not_obsolete  &&  test_desc->is_retired(month_now)  ) {
				// not using vintage cars here!
				continue;
			}

			uint32 power = (test_desc->get_power()*test_desc->get_gear())/64;
			if(target_freight)
			{
				// this is either a railcar/trailer or a truck/boat/plane
				if(  test_desc->get_total_capacity()==0  ||  !test_desc->get_freight_type()->is_interchangeable(target_freight)  ) {
					continue;
				}

				sint32 difference=0;	// smaller is better
				// assign this vehicle, if we have none found one yet, or we found only a too week one
				if(  desc!=NULL  )
				{
					// it is cheaper to run? (this is most important)
					difference += (desc->get_total_capacity()*1000)/(1+desc->get_running_cost()) < (test_desc->get_total_capacity()*1000)/(1+test_desc->get_running_cost()) ? -20 : 20;
					if(  target_weight>0  )
					{
						// it is strongere?
						difference += (desc->get_power()*desc->get_gear())/64 < power ? -10 : 10;
					}

					sint32 difference=0;	// smaller is better
					// it is faster? (although we support only up to 120km/h for goods)
					difference += (desc->get_topspeed() < test_desc->get_topspeed())? -10 : 10;
					// it is cheaper? (not so important)
					difference += (desc->get_value() > test_desc->get_value())? -5 : 5;
					// add some malus for obsolete vehicles
					if(test_desc->is_retired(month_now))
					{
						difference += 5;
					}
				}
				// ok, final check
				if(  desc==NULL  ||  difference<12    )
				{
					// then we want this vehicle!
					desc = test_desc;
					DBG_MESSAGE( "vehicle_builder_t::get_best_matching","Found car %s", desc ? desc->get_name() : "null");
				}
			}
			else {
				// finally, we might be able to use this vehicle
				sint32 speed = test_desc->get_topspeed();
				uint32 max_weight = power/( (speed*speed)/2500 + 1 );

				// we found a useful engine
				sint32 current_index = (power * 100) / (1 + test_desc->get_running_cost()) + test_desc->get_topspeed() - (sint16)test_desc->get_weight() - (sint32)(test_desc->get_value()/25000);
				// too slow?
				if(speed < target_speed) {
					current_index -= 250;
				}
				// too weak to to reach full speed?
				if(  max_weight < target_weight+test_desc->get_weight()  ) {
					current_index += max_weight - (sint32)(target_weight+test_desc->get_weight());
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
	}
	// no vehicle found!
	if(  desc==NULL  ) {
		DBG_MESSAGE( "vehicle_builder_t::get_best_matching()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	}
	return desc;
}
