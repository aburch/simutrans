/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../simdebug.h"
#include "../utils/simrandom.h"  // for simrand
#include "../simtypes.h"

#include "../simconvoi.h"

#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/livery_scheme.h"

#include "../besch/vehikel_besch.h"

#include "vehikelbauer.h"

#include "../tpl/stringhashtable_tpl.h"

static stringhashtable_tpl< vehikel_besch_t*> name_fahrzeuge;

// index 0 aur, 1...8 at normal waytype index
#define GET_WAYTYPE_INDEX(wt) ((int)(wt)>8 ? 0 : (wt))
static slist_tpl<vehikel_besch_t*> typ_fahrzeuge[9];

class bonus_record_t {
public:
	sint64 year;
	sint32 speed;
	bonus_record_t( sint64 y=0, sint32 kmh=0 ) {
		year = y*12;
		speed = kmh;
	};
	void rdwr(loadsave_t *file) {
		file->rdwr_longlong(year);
		file->rdwr_long(speed);
	}
};

// speed bonus
static vector_tpl<bonus_record_t>speedbonus[8];

static sint32 default_speedbonus[8] =
{
	80,	// road
	80,	// track
	35,	// water
	80,	// air
	80,	// monorail
	80,	// maglev
	80,	// tram
	80	// narrowgauge
};

bool vehikelbauer_t::speedbonus_init(const std::string &objfilename)
{
	tabfile_t bonusconf;
	// first take user data, then user global data
	if (!bonusconf.open((objfilename+"config/speedbonus.tab").c_str())) {
		dbg->warning("vehikelbauer_t::speedbonus_init()", "Can't read speedbonus.tab" );
		return false;
	}

	tabfileobj_t contents;
	bonusconf.read(contents);

	/* init the values from line with the form year, speed, year, speed
	 * must be increasing order!
	 */
	for(  int j=0;  j<8;  j++  ) {
		int *tracks = contents.get_ints(weg_t::waytype_to_string(j==3?air_wt:(waytype_t)(j+1)));
		if((tracks[0]&1)==1) {
			dbg->warning( "vehikelbauer_t::speedbonus_init()", "Ill formed line in speedbonus.tab\nFormat is year,speed[,year,speed]!" );
			tracks[0]--;
		}
		speedbonus[j].resize( tracks[0]/2 );
		for(  int i=1;  i<tracks[0];  i+=2  ) {
			bonus_record_t b( tracks[i], tracks[i+1] );
			speedbonus[j].append( b );
		}
		delete [] tracks;
	}

	return true;
}


sint32 vehikelbauer_t::get_speedbonus( sint32 monthyear, waytype_t wt )
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
		const int wtidx = GET_WAYTYPE_INDEX(wt);
		if(  !typ_fahrzeuge[wtidx].empty()  ) {
			FOR(slist_tpl<vehikel_besch_t *>, info, typ_fahrzeuge[wtidx]) 
			{
				if(info->get_leistung()>0  &&  info->is_available(monthyear)) {
					speed_sum += info->get_geschw();
					num_averages ++;
				}
			}
		}
		if(  num_averages>0  ) {
			return speed_sum / num_averages;
		}
	}

	// no vehicles => return default
	return default_speedbonus[typ];
}


void vehikelbauer_t::rdwr_speedbonus(loadsave_t *file)
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


vehicle_t* vehikelbauer_t::baue(koord3d k, player_t* player, convoi_t* cnv, const vehikel_besch_t* vb, bool upgrade, uint16 livery_scheme_index )
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
			dbg->fatal("vehikelbauer_t::baue()", "cannot built a vehicle with waytype %i", vb->get_waytype());
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
		price = vb->get_preis();
	}
	player->book_new_vehicle(-price, k.get_2d(), vb->get_waytype() );

	return v;
}



bool vehikelbauer_t::register_besch(vehikel_besch_t *besch)
{
	// register waytype list
	const int idx = GET_WAYTYPE_INDEX( besch->get_waytype() );
	vehikel_besch_t *old_besch = name_fahrzeuge.get( besch->get_name() );
	if(  old_besch  ) {
		dbg->warning( "vehikelbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		name_fahrzeuge.remove( besch->get_name() );
		typ_fahrzeuge[idx].remove(old_besch);
	}
	name_fahrzeuge.put(besch->get_name(), besch);
	typ_fahrzeuge[idx].append(besch);
	return true;
}


static bool compare_vehikel_besch(const vehikel_besch_t* a, const vehikel_besch_t* b)
{
	// Sort by:
	//  1. cargo category
	//  2. cargo (if special freight)
	//  3. engine_type
	//  4. speed
	//  5. power
	//  6. intro date
	//  7. name
	int cmp = a->get_ware()->get_catg() - b->get_ware()->get_catg();
	if (cmp == 0) {
		if (a->get_ware()->get_catg() == 0) {
			cmp = a->get_ware()->get_index() - b->get_ware()->get_index();
		}
		if (cmp == 0) {
			cmp = a->get_zuladung() - b->get_zuladung();
			if (cmp == 0) {
				// to handle tender correctly
				uint8 b_engine = (a->get_zuladung() + a->get_leistung() == 0 ? (uint8)vehikel_besch_t::steam : a->get_engine_type());
				uint8 a_engine = (b->get_zuladung() + b->get_leistung() == 0 ? (uint8)vehikel_besch_t::steam : b->get_engine_type());
				cmp = b_engine - a_engine;
				if (cmp == 0) {
					cmp = a->get_geschw() - b->get_geschw();
					if (cmp == 0) {
						// put tender at the end of the list ...
						int b_leistung = (a->get_leistung() == 0 ? 0x7FFFFFF : a->get_leistung());
						int a_leistung = (b->get_leistung() == 0 ? 0x7FFFFFF : b->get_leistung());
						cmp = b_leistung - a_leistung;
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


bool vehikelbauer_t::alles_geladen()
{
	// first: check for bonus tables
	DBG_MESSAGE("vehikelbauer_t::sort_lists()","called");
	for(  int wt_idx=0;  wt_idx<9;  wt_idx++  ) {
		slist_tpl<vehikel_besch_t*>& typ_liste = typ_fahrzeuge[wt_idx];
		uint count = typ_liste.get_count();
		if (count == 0) {
			continue;
		}
		vehikel_besch_t** const tmp     = new vehikel_besch_t*[count];
		vehikel_besch_t** const tmp_end = tmp + count;
		for(  vehikel_besch_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
			*tmpptr = typ_liste.remove_first();
		}
		std::sort(tmp, tmp_end, compare_vehikel_besch);
		for(  vehikel_besch_t** tmpptr = tmp;  tmpptr != tmp_end;  tmpptr++  ) {
			typ_liste.append(*tmpptr);
		}
		delete [] tmp;
	}
	return true;
}



const vehikel_besch_t *vehikelbauer_t::get_info(const char *name)
{
	return name_fahrzeuge.get(name);
}

slist_tpl<vehikel_besch_t*>& vehikelbauer_t::get_info(waytype_t typ)
{
	return typ_fahrzeuge[GET_WAYTYPE_INDEX(typ)];
}

/* extended search for vehicles for KI *
 * checks also timeline and constraints
 * tries to get best with but adds a little random action
 * @author prissi
 */
const vehikel_besch_t *vehikelbauer_t::vehikel_search( waytype_t wt, const uint16 month_now, const uint32 target_weight, const sint32 target_speed, const ware_besch_t * target_freight, bool include_electric, bool not_obsolete )
{
	if(  (target_freight!=NULL  ||  target_weight!=0)  &&  !typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)].empty()  ) 
	{
		struct best_t {
			uint32 power;
			uint16 payload_per_maintenance;
			long index;
		} best, test;
		best.index = -100000;

		const vehikel_besch_t *besch = NULL;
		FOR(slist_tpl<vehikel_besch_t *>, const test_besch, typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)]) 
		{
			// no constricts allow for rail vehicles concerning following engines
			if(wt==track_wt  &&  !test_besch->can_follow_any()  ) 
			{
				continue;
			}
			// do not buy incomplete vehicles
			if(wt==road_wt && !test_besch->can_lead(NULL))
			{
				continue;
			}

			// engine, but not allowed to lead a convoi, or no power at all or no electrics allowed
			if(target_weight) 
			{
				if(test_besch->get_leistung()==0  ||  !test_besch->can_follow(NULL)  ||  (!include_electric  &&  test_besch->get_engine_type()==vehikel_besch_t::electric) ) 
				{
					continue;
				}
			}

			// check for wegetype/too new
			if(test_besch->get_waytype()!=wt  ||  test_besch->is_future(month_now)  )
			{
				continue;
			}

			if(  not_obsolete  &&  test_besch->is_retired(month_now)  ) 
			{
				// not using vintage cars here!
				continue;
			}

			test.power = (test_besch->get_leistung() * test_besch->get_gear()) / 64;
			if(target_freight) 
			{
				// this is either a railcar/trailer or a truck/boat/plane
				if(  test_besch->get_zuladung()==0  ||  !test_besch->get_ware()->is_interchangeable(target_freight)  ) 
				{
					continue;
				}

				test.index = -100000;
				test.payload_per_maintenance = test_besch->get_zuladung() / max(test_besch->get_running_cost(), 1);

				sint32 difference=0;	// smaller is better
				// assign this vehicle if we have not found one yet, or we only found one too weak
				if(  besch!=NULL  ) {
					// is it cheaper to run? (this is most important)
					difference += best.payload_per_maintenance < test.payload_per_maintenance ? -20 : 20;
					if(  target_weight>0  ) {
						// is it strongerer?
						difference += best.power < test.power ? -10 : 10;
					}
					// it is faster? (although we support only up to 120km/h for goods)
					difference += (besch->get_geschw() < test_besch->get_geschw())? -10 : 10;
					// it is cheaper? (not so important)
					difference += (besch->get_preis() > test_besch->get_preis())? -5 : 5;
					// add some malus for obsolete vehicles
					if(test_besch->is_retired(month_now)) {
						difference += 5;
					}
				}
				// ok, final check
				if(  besch==NULL  ||  difference<(int)simrand(25, "vehikelbauer_t::vehikel_search")    ) {
					// then we want this vehicle!
					besch = test_besch;
					best = test;
					DBG_MESSAGE( "vehikelbauer_t::vehikel_search","Found car %s", besch ? besch->get_name() : "null");
				}
			}
			else {
				// engine/tugboat/truck for trailer
				if(  test_besch->get_zuladung()!=0  ||  !test_besch->can_follow(NULL)  ) {
					continue;
				}
				// finally, we might be able to use this vehicle
				sint32 speed = test_besch->get_geschw();
				uint32 max_weight = test.power/( (speed*speed)/2500 + 1 );

				// we found a useful engine
				test.index = (test.power*100)/max(test_besch->get_running_cost(), 1) + test_besch->get_geschw() - (sint16)test_besch->get_gewicht() - (sint32)(test_besch->get_preis()/25000);
				// too slow?
				if(speed < target_speed) {
					test.index -= 250;
				}
				// too weak to to reach full speed?
				if(  max_weight < target_weight+test_besch->get_gewicht()  ) {
					test.index += max_weight - (sint32)(target_weight+test_besch->get_gewicht());
				}
				test.index += simrand(100, "vehikelbauer_t::vehikel_search");
				if(  test.index > best.index  ) {
					// then we want this vehicle!
					besch = test_besch;
					best = test;
					DBG_MESSAGE( "vehikelbauer_t::vehikel_search","Found engine %s",besch->get_name());
				}
			}
		}
		if (besch)
		{
			return besch;
		}
	}
	// no vehicle found!
	DBG_MESSAGE( "vehikelbauer_t::vehikel_search()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	return NULL;
}



/* extended search for vehicles for replacement on load time
 * tries to get best match (no random action)
 * if prev_besch==NULL, then the convoi must be able to lead a convoi
 * @author prissi
 */
const vehikel_besch_t *vehikelbauer_t::get_best_matching( waytype_t wt, const uint16 month_now, const uint32 target_weight, const uint32 target_power, const sint32 target_speed, const ware_besch_t * target_freight, bool not_obsolete, const vehikel_besch_t *prev_veh, bool is_last )
{
	const vehikel_besch_t *besch = NULL;
	sint32 besch_index =- 100000;

	if(  !typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)].empty()  ) 
	{
		FOR(slist_tpl<vehikel_besch_t *>, const test_besch, typ_fahrzeuge[GET_WAYTYPE_INDEX(wt)])
		{
			if(target_power>0  &&  test_besch->get_leistung()==0) 
			{
				continue;
			}

			// will test for first (prev_veh==NULL) or matching following vehicle
			if(!test_besch->can_follow(prev_veh)) {
				continue;
			}

			// not allowed as last vehicle
			if(is_last  &&  test_besch->get_nachfolger_count()>0  &&  test_besch->get_nachfolger(0)!=NULL  ) {
				continue;
			}

			// not allowed as non-last vehicle
			if(!is_last  &&  test_besch->get_nachfolger_count()==1  &&  test_besch->get_nachfolger(0)==NULL  ) {
				continue;
			}

			// check for waytype too
			if(test_besch->get_waytype()!=wt  ||  test_besch->is_future(month_now)  ) {
				continue;
			}

			// ignore vehicles that need electrification
			if(test_besch->get_leistung()>0  &&  test_besch->get_engine_type()==vehikel_besch_t::electric) {
				continue;
			}

			// likely tender => replace with some engine ...
			if(target_freight==0  &&  target_weight==0) {
				if(  test_besch->get_zuladung()!=0  ) {
					continue;
				}
			}

			if(  not_obsolete  &&  test_besch->is_retired(month_now)  ) {
				// not using vintage cars here!
				continue;
			}

			uint32 power = (test_besch->get_leistung()*test_besch->get_gear())/64;
			if(target_freight) 
			{
				// this is either a railcar/trailer or a truck/boat/plane
				if(  test_besch->get_zuladung()==0  ||  !test_besch->get_ware()->is_interchangeable(target_freight)  ) {
					continue;
				}

				sint32 difference=0;	// smaller is better
				// assign this vehicle, if we have none found one yet, or we found only a too week one
				if(  besch!=NULL  ) 
				{
					// it is cheaper to run? (this is most important)
					difference += (besch->get_zuladung()*1000)/(1+besch->get_running_cost()) < (test_besch->get_zuladung()*1000)/(1+test_besch->get_running_cost()) ? -20 : 20;
					if(  target_weight>0  ) 
					{
						// it is strongere?
						difference += (besch->get_leistung()*besch->get_gear())/64 < power ? -10 : 10;
					}

					sint32 difference=0;	// smaller is better
					// it is faster? (although we support only up to 120km/h for goods)
					difference += (besch->get_geschw() < test_besch->get_geschw())? -10 : 10;
					// it is cheaper? (not so important)
					difference += (besch->get_preis() > test_besch->get_preis())? -5 : 5;
					// add some malus for obsolete vehicles
					if(test_besch->is_retired(month_now))
					{
						difference += 5;
					}
				}
				// ok, final check
				if(  besch==NULL  ||  difference<12    ) 
				{
					// then we want this vehicle!
					besch = test_besch;
					DBG_MESSAGE( "vehikelbauer_t::get_best_matching","Found car %s", besch ? besch->get_name() : "null");
				}
			}
			else {
				// finally, we might be able to use this vehicle
				sint32 speed = test_besch->get_geschw();
				uint32 max_weight = power/( (speed*speed)/2500 + 1 );

				// we found a useful engine
				sint32 current_index = (power * 100) / (1 + test_besch->get_running_cost()) + test_besch->get_geschw() - (sint16)test_besch->get_gewicht() - (sint32)(test_besch->get_preis()/25000);
				// too slow?
				if(speed < target_speed) {
					current_index -= 250;
				}
				// too weak to to reach full speed?
				if(  max_weight < target_weight+test_besch->get_gewicht()  ) {
					current_index += max_weight - (sint32)(target_weight+test_besch->get_gewicht());
				}
				current_index += 50;
				if(  current_index > besch_index  ) {
					// then we want this vehicle!
					besch = test_besch;
					besch_index = current_index;
					DBG_MESSAGE( "vehikelbauer_t::get_best_matching","Found engine %s",besch->get_name());
				}
			}
		}
	}
	// no vehicle found!
	if(  besch==NULL  ) {
		DBG_MESSAGE( "vehikelbauer_t::get_best_matching()","could not find a suitable vehicle! (speed %i, weight %i)",target_speed,target_weight);
	}
	return besch;
}
