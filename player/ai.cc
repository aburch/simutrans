/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "finance.h"
#include "ai.h"

#include "../simcity.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simintr.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simware.h"

#include "../bauer/brueckenbauer.h"
#include "../bauer/hausbauer.h"
#include "../bauer/tunnelbauer.h"
#include "../bauer/vehikelbauer.h"
#include "../bauer/wegbauer.h"

#include "../descriptor/building_desc.h"

#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"

#include "../obj/zeiger.h"

#include "../utils/cbuffer_t.h"

#include "../vehicle/vehicle.h"


/* The flesh for the place with road for headquarters searcher ... */
bool ai_building_place_with_road_finder::is_road_at(sint16 x, sint16 y) const {
	grund_t *bd = welt->lookup_kartenboden( koord(x,y) );
	return bd && bd->hat_weg(road_wt);
}


bool ai_building_place_with_road_finder::is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const {
	if(building_placefinder_t::is_area_ok(pos, w, h, cl)) {
		// check to not built on a road
		int i, j;
		for(j=pos.x; j<pos.x+w; j++) {
			for(i=pos.y; i<pos.y+h; i++) {
				if(is_road_at(j,i)) {
					return false;
				}
			}
		}
		// now check for road connection
		for(i = pos.y; i < pos.y + h; i++) {
			if(is_road_at(pos.x - 1, i) ||  is_road_at(pos.x + w, i)) {
				return true;
			}
		}
		for(i = pos.x; i < pos.x + w; i++) {
			if(is_road_at(i, pos.y - 1) ||  is_road_at(i, pos.y + h)) {
				return true;
			}
		}
	}
	return false;
}


/************************** and now the "real" helper functions ***************/

/* return the halt on the map ground */
halthandle_t ai_t::get_halt(const koord pos ) const
{
	if(  grund_t *gr = welt->lookup_kartenboden(pos)  ) {
		return haltestelle_t::get_halt( gr->get_pos(), this );
	}
	return halthandle_t();
}


/**
 * returns true,
 * if there is already a connection
 */
bool ai_t::is_connected( const koord start_pos, const koord dest_pos, const goods_desc_t *wtyp ) const
{
	// Dario: Check if there's a stop near destination
	const planquadrat_t* start_plan = welt->access(start_pos);
	const halthandle_t* start_list = start_plan->get_haltlist();
	const uint16 start_halt_count  = start_plan->get_haltlist_count();

	// now try to find a route
	// ok, they are not in walking distance
	ware_t ware(wtyp);
	ware.set_zielpos(dest_pos);
	ware.menge = 1;

	return (start_halt_count != 0)  &&  (haltestelle_t::search_route( start_list, start_halt_count, false, ware ) != haltestelle_t::NO_ROUTE);
}


// calls a general tool just like a player work do
bool ai_t::call_general_tool( int tool, koord k, const char *param )
{
	grund_t *gr = welt->lookup_kartenboden(k);
	koord3d pos = gr ? gr->get_pos() : koord3d::invalid;
	const char *old_param = tool_t::general_tool[tool]->get_default_param();
	tool_t::general_tool[tool]->set_default_param(param);
	const char * err = tool_t::general_tool[tool]->work( this, pos );
	if(err) {
		if(*err) {
			dbg->message("ai_t::call_general_tool()","failed for tool %i at (%s) because of \"%s\"", tool, pos.get_str(), err );
		}
		else {
			dbg->message("ai_t::call_general_tool()","not successful for tool %i at (%s)", tool, pos.get_str() );
		}
	}
	tool_t::general_tool[tool]->set_default_param(old_param);
	return err==0;
}



/* returns ok, of there is a suitable space found
 * only check into two directions, the ones given by dir
 */
bool ai_t::suche_platz(koord pos, koord &size, koord *dirs)
{
	sint16 length = abs( size.x + size.y );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(  gr==NULL  ) {
		return false;
	}

	sint8 start_z = gr->get_hoehe();
	int max_dir = length==0 ? 1 : 2;
	bool place_ok;

	// two rotations
	for(  int dir=0;  dir<max_dir;  dir++  ) {
		place_ok = true;
		for(  sint16 i=0;  i<=length;  i++  ) {
			grund_t *gr = welt->lookup_kartenboden(  pos + (dirs[dir]*i)  );
			if(  gr == NULL
				||  gr->get_halt().is_bound()
				||  !welt->can_flatten_tile(this, pos, start_z )
				||  !gr->ist_natur()
				||  gr->kann_alle_obj_entfernen(this) != NULL
				||  gr->get_hoehe() < welt->get_water_hgt( pos + (dirs[dir] * i) )  ) {
					// something is blocking construction, try next rotation
					place_ok = false;
					break;
			}
		}
		if(  place_ok  ) {
			// apparently we can build this rotation here
			size = dirs[dir]*length;
			return true;
		}
	}
	return false;
}



/** needed renovation due to different sized factories
 * also try "nicest" place first
 */
bool ai_t::suche_platz(koord &start, koord &size, koord target, koord off)
{
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord platz;
	int const cov = welt->get_settings().get_station_coverage();
	int xpos = start.x;
	int ypos = start.y;

	koord dir[2];
	if(  abs(start.x-target.x)<abs(start.y-target.y)  ) {
		dir[0] = koord( 0, sgn(target.y-start.y) );
		dir[1] = koord( sgn(target.x-start.x), 0 );
	}
	else {
		dir[0] = koord( sgn(target.x-start.x), 0 );
		dir[1] = koord( 0, sgn(target.y-start.y) );
	}

	DBG_MESSAGE("ai_t::suche_platz()","at (%i,%i) for size (%i,%i)",xpos,ypos,off.x,off.y);
	int maxy = min( welt->get_size().y, ypos + off.y + cov );
	int maxx = min( welt->get_size().x, xpos + off.x + cov );
	for (int y = max(0,ypos-cov);  y < maxy;  y++) {
		for (int x = max(0,xpos-cov);  x < maxx;  x++) {
			platz = koord(x,y);
			// no water tiles
			if(  welt->lookup_kartenboden(platz)->get_hoehe() <= welt->get_water_hgt(platz)  ) {
				continue;
			}
			// thus now check them
			int current_dist = koord_distance(platz,target);
			if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
				// we will take the shortest route found
				start = platz;
				dist = koord_distance(platz,target);
			}
			else {
				koord test(x,y);
				if(  get_halt(test).is_bound()  ) {
DBG_MESSAGE("ai_t::suche_platz()","Search around stop at (%i,%i)",x,y);

					// we are on a station that belongs to us
					int xneu=x-1, yneu=y-1;
					platz = koord(xneu,y);
					current_dist = koord_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = koord_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					// search on the other side of the station
					xneu = x+1;
					yneu = y+1;
					platz = koord(xneu,y);
					current_dist = koord_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = koord_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}
				}
			}
		}
	}
	// of, success of short than maximum
	if(  dist<0x7FFFFFFF  ) {
		return true;
	}
	return false;
}



void ai_t::clean_marker( koord place, koord size )
{
	koord pos;
	if(size.y<0) {
		place.y += size.y;
		size.y = -size.y;
	}
	if(size.x<0) {
		place.x += size.x;
		size.x = -size.x;
	}
	for(  pos.y=place.y;  pos.y<=place.y+size.y;  pos.y++  ) {
		for(  pos.x=place.x;  pos.x<=place.x+size.x;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			zeiger_t *z = gr->find<zeiger_t>();
			delete z;
		}
	}
}



void ai_t::set_marker( koord place, koord size )
{
	koord pos;
	if(size.y<0) {
		place.y += size.y;
		size.y = -size.y;
	}
	if(size.x<0) {
		place.x += size.x;
		size.x = -size.x;
	}
	for(  pos.y=place.y;  pos.y<=place.y+size.y;  pos.y++  ) {
		for(  pos.x=place.x;  pos.x<=place.x+size.x;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			zeiger_t *z = new zeiger_t(gr->get_pos(), this);
			z->set_image( skinverwaltung_t::belegtzeiger->get_image_id(0) );
			gr->obj_add( z );
		}
	}
}



/* builds headquarters or upgrades one */
bool ai_t::built_update_headquarter()
{
	// find next level
	const building_desc_t* desc = hausbauer_t::get_headquarters(get_headquarter_level(), welt->get_timeline_year_month());
	// is the a suitable one?
	if(desc!=NULL) {
		// cost is negative!
		sint64 const cost = welt->get_settings().cst_multiply_headquarter * desc->get_level() * desc->get_x() * desc->get_y();
		if(  finance->get_account_balance()+cost > finance->get_starting_money() ) {
			// and enough money left ...
			koord place = get_headquarter_pos();
			if(place!=koord::invalid) {
				// remove old hq
				grund_t *gr = welt->lookup_kartenboden(place);
				gebaeude_t *prev_hq = gr->find<gebaeude_t>();
				// other size?
				if(  desc->get_size()!=prev_hq->get_tile()->get_desc()->get_size()  ) {
					// needs new place
					place = koord::invalid;
				}
				else {
					// old positions false after rotation => correct it
					place = prev_hq->get_pos().get_2d() - prev_hq->get_tile()->get_offset();
				}
			}
			// needs new place?
			if(place==koord::invalid) {
				stadt_t *st = NULL;
				FOR(vector_tpl<halthandle_t>, const halt, haltestelle_t::get_alle_haltestellen()) {
					if(  halt->get_owner()==this  ) {
						st = welt->find_nearest_city(halt->get_basis_pos());
						break;
					}
				}
				if(st) {
					bool is_rotate=desc->get_all_layouts()>1;
					place = ai_building_place_with_road_finder(welt).find_place(st->get_pos(), desc->get_x(), desc->get_y(), desc->get_allowed_climate_bits(), &is_rotate);
				}
			}
			const char *err="No suitable ground!";
			if(  place!=koord::invalid  ) {
				err = tool_t::general_tool[TOOL_HEADQUARTER]->work( this, welt->lookup_kartenboden(place)->get_pos() );
				// success
				if(  err==NULL  ) {
					return true;
				}
			}
			// failed
			if(  place==koord::invalid  ||  err!=NULL  ) {
				dbg->warning( "ai_t::built_update_headquarter()", "HQ failed with : %s", translator::translate(err) );
			}
			return false;
		}

	}
	return false;
}



/**
 * Find the last water tile using line algorithm
 * start MUST be on land!
 */
koord ai_t::find_shore(koord start, koord end) const
{
	int x = start.x;
	int y = start.y;
	int xx = end.x;
	int yy = end.y;

	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	koord last = start;
	for (i = 0; i <= steps; i++) {
		koord next(xp>>16,yp>>16);
		if(next!=last) {
			if(!welt->lookup_kartenboden(next)->is_water()) {
				last = next;
			}
		}
		xp += xs;
		yp += ys;
	}
	// should always find something, since it ends in water ...
	return last;
}



bool ai_t::find_harbour(koord &start, koord &size, koord target)
{
	koord shore = find_shore(target,start);
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord k;
	// now find a nice shore next to here
	for(  k.y=max(1,shore.y-5);  k.y<shore.y+6  &&  k.y<welt->get_size().y-2; k.y++  ) {
		for(  k.x=max(1,shore.x-5);  k.x<shore.x+6  &&  k.y<welt->get_size().x-2; k.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(k);
			if(  gr  &&  gr->get_grund_hang() != 0  &&  slope_t::is_way( gr->get_grund_hang() )  &&  gr->ist_natur()  &&  gr->get_hoehe() == welt->get_water_hgt(k)  &&  !gr->is_halt()  ) {
				koord zv = koord(gr->get_grund_hang());
				if(welt->lookup_kartenboden(k-zv)->get_weg_ribi(water_wt)) {
					// next place is also water
					koord dir[2] = { zv, koord(zv.y,zv.x) };
					koord platz = k+zv;
					int current_dist = koord_distance(k,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = k;
						dist = current_dist;
					}
				}
			}
		}
	}
	return (dist<0x7FFFFFFF);
}



bool ai_t::create_simple_road_transport(koord platz1, koord size1, koord platz2, koord size2, const way_desc_t *road_weg )
{
	// sanity check here
	if(road_weg==NULL) {
		DBG_MESSAGE("ai_t::create_simple_road_transport()","called without valid way.");
		return false;
	}

	// remove pointer
	clean_marker(platz1,size1);
	clean_marker(platz2,size2);

	climate c1 = welt->get_climate(platz1);
	climate c2 = welt->get_climate(platz2);

	if(  !(welt->flatten_tile( this, platz1, welt->lookup_kartenboden(platz1)->get_hoehe() )  &&  welt->flatten_tile( this, platz2, welt->lookup_kartenboden(platz2)->get_hoehe() ))  ) {
		// no flat land here?!?
		return false;
	}
	// ensure is land
	grund_t* bd = welt->lookup_kartenboden(platz1);
	if (bd->get_typ() == grund_t::wasser) {
		welt->set_water_hgt(platz1, bd->get_hoehe()-1);
		welt->access(platz1)->correct_water();
		welt->set_climate(platz1, c1, true);
	}
	bd = welt->lookup_kartenboden(platz2);
	if (bd->get_typ() == grund_t::wasser) {
		welt->set_water_hgt(platz2, bd->get_hoehe()-1);
		welt->access(platz2)->correct_water();
		welt->set_climate(platz2, c2, true);
	}

	// is there already a connection?
	// get a default vehicle
	vehicle_desc_t test_desc(road_wt, 25, vehicle_desc_t::diesel );
	vehicle_t* test_driver = vehicle_builder_t::build(welt->lookup_kartenboden(platz1)->get_pos(), this, NULL, &test_desc);
	test_driver->set_flag( obj_t::not_on_map );
	route_t verbindung;
	if(  verbindung.calc_route(welt, welt->lookup_kartenboden(platz1)->get_pos(), welt->lookup_kartenboden(platz2)->get_pos(), test_driver, 0, 0)  &&
		 verbindung.get_count() < 2u*koord_distance(platz1,platz2))  {
DBG_MESSAGE("ai_passenger_t::create_simple_road_transport()","Already connection between %d,%d to %d,%d is only %i",platz1.x, platz1.y, platz2.x, platz2.y, verbindung.get_count() );
		// found something with the nearly same length
		delete test_driver;
		return true;
	}
	delete test_driver;

	// no connection => built one!
	way_builder_t bauigel(this);
	bauigel.init_builder( way_builder_t::strasse, road_weg, tunnel_builder_t::get_tunnel_desc(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()), bridge_builder_t::find_bridge(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()) );

	// we won't destroy cities (and save the money)
	bauigel.set_keep_existing_faster_ways(true);
	bauigel.set_keep_city_roads(true);
	bauigel.set_maximum(10000);

	bauigel.calc_route(welt->lookup_kartenboden(platz1)->get_pos(),welt->lookup_kartenboden(platz2)->get_pos());
	INT_CHECK("ai 501");

	if(  bauigel.calc_costs() > finance->get_netwealth()  ) {
		// too expensive
		return false;
	}

	// now try route with terraforming
	way_builder_t baumaulwurf(this);
	baumaulwurf.init_builder( way_builder_t::strasse|way_builder_t::terraform_flag, road_weg, tunnel_builder_t::get_tunnel_desc(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()), bridge_builder_t::find_bridge(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()) );
	baumaulwurf.set_keep_existing_faster_ways(true);
	baumaulwurf.set_keep_city_roads(true);
	baumaulwurf.set_maximum(10000);
	baumaulwurf.calc_route(welt->lookup_kartenboden(platz1)->get_pos(),welt->lookup_kartenboden(platz2)->get_pos());

	// build with terraforming if shorter and enough money is available
	bool with_tf = (baumaulwurf.get_count() > 2)  &&  (10*baumaulwurf.get_count() < 9*bauigel.get_count()  ||  bauigel.get_count() <= 2);
	if(  with_tf  &&  baumaulwurf.calc_costs() > finance->get_netwealth()  ) {
		// too expensive
		with_tf = false;
	}

	// now build with or without terraforming
	if (with_tf) {
		DBG_MESSAGE("ai_t::create_simple_road_transport()","building not so simple road from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		baumaulwurf.build();
		return true;
	}
	else if(bauigel.get_count() > 2) {
		DBG_MESSAGE("ai_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.build();
		return true;
	}
	// beware: The stop position might have changes!
	DBG_MESSAGE("ai_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d failed",platz1.x, platz1.y, platz2.x, platz2.y);
	return false;
}


/* create new AI */
ai_t::ai_t(uint8 nr) :
	player_t( nr ),
	road_transport(false),
	rail_transport(false),
	ship_transport(false),
	air_transport(false),
	construction_speed(env_t::default_ai_construction_speed)
{
}


void ai_t::rdwr(loadsave_t *file)
{
	player_t::rdwr(file);

	if(  file->is_version_less(111, 1)  ) {
		// do not know about ai_t
		return;
	}

	file->rdwr_long( construction_speed );
	file->rdwr_bool( road_transport );
	file->rdwr_bool( rail_transport );
	file->rdwr_bool( air_transport );
	file->rdwr_bool( ship_transport );
}


const vehicle_desc_t *ai_t::vehicle_search(waytype_t typ, const uint32 target_power, const sint32 target_speed, const goods_desc_t * target_freight, bool include_electric)
{
	bool obsolete_allowed = welt->get_settings().get_allow_buying_obsolete_vehicles();
	return vehicle_builder_t::vehicle_search(typ, welt->get_timeline_year_month(), target_power, target_speed, target_freight, include_electric, !obsolete_allowed);
}
