
/* standard good AI code */

#include "../simfab.h"
#include "../simmenu.h"
#include "../simtypes.h"
#include "../simwerkz.h"
#include "../simunits.h"

#include "finance.h"
#include "simplay.h"

#include "../simhalt.h"
#include "../simintr.h"
#include "../simline.h"
#include "../simmesg.h"
#include "../simtools.h"
#include "../simworld.h"

#include "../bauer/brueckenbauer.h"
#include "../bauer/hausbauer.h"
#include "../bauer/tunnelbauer.h"
#include "../bauer/vehikelbauer.h"
#include "../bauer/wegbauer.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/loadsave.h"

#include "../dings/wayobj.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../vehicle/simvehikel.h"

#include "ai_goods.h"


ai_goods_t::ai_goods_t(karte_t *wl, uint8 nr) : ai_t(wl,nr)
{
	state = NR_INIT;

	freight = NULL;

	root = NULL;
	start = NULL;
	ziel = NULL;

	count = 0;

	rail_engine = NULL;
	rail_vehicle = NULL;
	rail_weg = NULL;
	road_vehicle = NULL;
	ship_vehicle = NULL;
	road_weg = NULL;

	next_construction_steps = welt->get_steps()+ 50;

	road_transport = nr!=7;
	rail_transport = nr>2;
	ship_transport = true;
	air_transport = false;
}




/**
 * Methode fuer jaehrliche Aktionen
 * @author Hj. Malthaner, Owen Rudge, hsiegeln
 */
void ai_goods_t::neues_jahr()
{
	spieler_t::neues_jahr();

	// AI will reconsider the oldest unbuiltable lines again
	uint remove = (uint)max(0,(int)forbidden_connections.get_count()-3);
	while(  remove < forbidden_connections.get_count()  ) {
		forbidden_connections.remove_first();
	}
}



void ai_goods_t::rotate90( const sint16 y_size )
{
	spieler_t::rotate90( y_size );

	// rotate places
	platz1.rotate90( y_size );
	platz2.rotate90( y_size );
	size1.rotate90( 0 );
	size2.rotate90( 0 );
	harbour.rotate90( y_size );
}



/* Activates/deactivates a player
 * @author prissi
 */
bool ai_goods_t::set_active(bool new_state)
{
	// something to change?
	if(automat!=new_state) {

		if(!new_state) {
			// deactivate AI
			automat = false;
			state = NR_INIT;
			start = ziel = NULL;
		}
		else {
			// aktivate AI
			automat = true;
		}
	}
	return automat;
}



/* recursive lookup of a factory tree:
 * sets start and ziel to the next needed supplier
 * start always with the first branch, if there are more goods
 */
bool ai_goods_t::get_factory_tree_lowest_missing( fabrik_t *fab )
{
	// now check for all products (should be changed later for the root)
	for(  int i=0;  i<fab->get_besch()->get_lieferanten();  i++  ) {
		const ware_besch_t *ware = fab->get_besch()->get_lieferant(i)->get_ware();

		// find out how much is there
		const array_tpl<ware_production_t>& eingang = fab->get_eingang();
		uint ware_nr;
		for(  ware_nr=0;  ware_nr<eingang.get_count()  &&  eingang[ware_nr].get_typ()!=ware;  ware_nr++  )
			;
		if(  eingang[ware_nr].menge > eingang[ware_nr].max/4  ) {
			// already enough supplied to
			continue;
		}

		FOR(vector_tpl<koord>, const& q, fab->get_suppliers()) {
			fabrik_t* const qfab = fabrik_t::get_fab(welt, q);
			const fabrik_besch_t* const fb = qfab->get_besch();
			for(  uint qq = 0;  qq < fb->get_produkte();  qq++  ) {
				if(  fb->get_produkt(qq)->get_ware() == ware  &&
					 !is_forbidden(qfab, fab, ware)  &&
					 !is_connected(q, fab->get_pos().get_2d(), ware)  ) {
					// find out how much is there
					const array_tpl<ware_production_t>& ausgang = qfab->get_ausgang();
					uint ware_nr;
					for(ware_nr=0;  ware_nr<ausgang.get_count()  &&  ausgang[ware_nr].get_typ()!=ware;  ware_nr++  )
						;
					// ok, there is no connection and it is not banned, so we if there is enough for us
					if(  ausgang[ware_nr].menge < 1+ausgang[ware_nr].max/8  ) {
						// try better supplying this first
						if(  qfab->get_suppliers().get_count()>0  &&  get_factory_tree_lowest_missing( qfab )) {
							return true;
						}
					}
					start = qfab;
					ziel = fab;
					freight = ware;
					return true;
				}
			}
		}
		// completely supplied???
	}
	return false;
}



/* recursive lookup of a tree and how many factories must be at least connected
 * returns -1, if this tree is can't be completed
 */
int ai_goods_t::get_factory_tree_missing_count( fabrik_t *fab )
{
	int numbers=0;	// how many missing?

	fabrik_besch_t const& d = *fab->get_besch();
	// ok, this is a source ...
	if (d.is_producer_only()) {
		return 0;
	}

	// now check for all
	for (int i = 0; i < d.get_lieferanten(); ++i) {
		ware_besch_t const* const ware = d.get_lieferant(i)->get_ware();

		bool complete = false;	// found at least one factory
		FOR(vector_tpl<koord>, const& q, fab->get_suppliers()) {
			fabrik_t* const qfab = fabrik_t::get_fab(welt, q);
			if(!qfab) {
				dbg->error("fabrik_t::get_fab()","fab %s at %s does not find supplier at %s.", fab->get_name(), fab->get_pos().get_str(), q.get_str());
				continue;
			}
			if( !is_forbidden( qfab, fab, ware ) ) {
				const fabrik_besch_t* const fb = qfab->get_besch();
				for (uint qq = 0; qq < fb->get_produkte(); qq++) {
					if (fb->get_produkt(qq)->get_ware() == ware ) {
						int n = get_factory_tree_missing_count( qfab );
						if(n>=0) {
							complete = true;
							if (!is_connected(q, fab->get_pos().get_2d(), ware)) {
								numbers += 1;
							}
							numbers += n;
						}
					}
				}
			}
		}
		if(!complete) {
			return -1;
		}
	}
	return numbers;
}

void add_neighbourhood( vector_tpl<koord> &list, const uint16 size)
{
	uint32 old_size = list.get_count();
	koord test;
	for( uint32 i = 0; i < old_size; i++ ) {
		for( test.x = -size; test.x < size+1; test.x++ ) {
			for( test.y = -size; test.y < size+1; test.y++ ) {
				list.append_unique( list[i] + test );
			}
		}
	}
}


bool ai_goods_t::suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab, int length )
{
	clean_marker(platz1,size1);
	clean_marker(platz2,size2);

	koord start( qfab->get_pos().get_2d() );
	koord start_size( length, 0 );
	koord ziel( zfab->get_pos().get_2d() );
	koord ziel_size( length, 0 );

	bool ok = false;
	bool has_ziel = false;

	if(qfab->get_besch()->get_platzierung()!=fabrik_besch_t::Wasser) {
		if( length == 0 ) {
			vector_tpl<koord3d> tile_list[2];
			uint16 const cov = welt->get_settings().get_station_coverage();
			koord test;
			for( uint8 i = 0; i < 2; i++ ) {
				fabrik_t *fab =  i==0 ? qfab : zfab;
				vector_tpl<koord> fab_tiles;
				fab->get_tile_list( fab_tiles );
				add_neighbourhood( fab_tiles, cov );
				vector_tpl<koord> one_more( fab_tiles );
				add_neighbourhood( one_more, 1 );
				// Any halts here?
				vector_tpl<koord> halts;
				FOR(vector_tpl<koord>, const& j, one_more) {
					halthandle_t const halt = get_halt(j);
					if(  halt.is_bound()  &&  !halts.is_contained(halt->get_basis_pos())  ) {
						bool halt_connected = halt->get_fab_list().is_contained( fab );
						FOR(  slist_tpl<haltestelle_t::tile_t>, const& i, halt->get_tiles()  ) {
							koord const pos = i.grund->get_pos().get_2d();
							if(  halt_connected  ||  fab_tiles.is_contained(pos)  ) {
								halts.append_unique( pos );
							}
						}
					}
				}
				add_neighbourhood( halts, 1 );
				vector_tpl<koord> *next = &halts;
				for( uint8 k = 0; k < 2; k++ ) {
					// On which tiles we can start?
					FOR(vector_tpl<koord>, const& j, *next) {
						grund_t const* const gr = welt->lookup_kartenboden(j);
						if(  gr  &&  gr->get_grund_hang() == hang_t::flach  &&  !gr->hat_wege()  &&  !gr->get_leitung()  ) {
							tile_list[i].append_unique( gr->get_pos() );
						}
					}
					if( !tile_list[i].empty() ) {
						// Skip, if found tiles beneath halts.
						break;
					}
					next = &fab_tiles;
				}
			}
			// Test which tiles are the best:
			wegbauer_t bauigel(welt, this);
			bauigel.route_fuer( wegbauer_t::strasse, road_weg, tunnelbauer_t::find_tunnel(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,road_weg->get_topspeed(),welt->get_timeline_year_month()) );
			// we won't destroy cities (and save the money)
			bauigel.set_keep_existing_faster_ways(true);
			bauigel.set_keep_city_roads(true);
			bauigel.set_maximum(10000);
			bauigel.calc_route(tile_list[0], tile_list[1]);
			if(  bauigel.get_count() > 2  ) {
				// Sometimes reverse route is the best, so we have to change the koords.
				koord3d_vector_t const& r = bauigel.get_route();
				start = r.front().get_2d();
				ziel  = r.back().get_2d();
				if(  !tile_list[0].is_contained(r.front())  ) {
					sim::swap(start, ziel);
				}
				ok = true;
				has_ziel = true;
			}
		}
		if( !ok ) {
			ok = suche_platz(start, start_size, ziel, qfab->get_besch()->get_haus()->get_groesse(qfab->get_rotate()) );
		}
	}
	else {
		// water factory => find harbour location
		ok = find_harbour(start, start_size, ziel );
	}

	if( ok && !has_ziel ) {
		// found a place, search for target
		ok = suche_platz(ziel, ziel_size, start, zfab->get_besch()->get_haus()->get_groesse(zfab->get_rotate()) );
	}

	INT_CHECK("simplay 1729");

	if( !ok ) {
		// keine Bauplaetze gefunden
		DBG_MESSAGE( "ai_t::suche_platz1_platz2()", "no suitable locations found" );
	}
	else {
		// save places
		platz1 = start;
		size1 = start_size;
		platz2 = ziel;
		size2 = ziel_size;

		DBG_MESSAGE( "ai_t::suche_platz1_platz2()", "platz1=%d,%d platz2=%d,%d", platz1.x, platz1.y, platz2.x, platz2.y );

		// reserve space with marker
		set_marker( platz1, size1 );
		set_marker( platz2, size2 );
	}
	return ok;
}



/* builts dock and ships
 * @author prissi
 */
bool ai_goods_t::create_ship_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
	// pak64 has barges ...
	const vehikel_besch_t *v_second = NULL;
	if(ship_vehicle->get_leistung()==0) {
		v_second = ship_vehicle;
		if(v_second->get_vorgaenger_count()==0  ||  v_second->get_vorgaenger(0)==NULL) {
			// pushed barge?
			if(ship_vehicle->get_nachfolger_count()>0  &&  ship_vehicle->get_nachfolger(0)!=NULL) {
				v_second = ship_vehicle->get_nachfolger(0);
			}
			else {
				return false;
			}
		}
		else {
			ship_vehicle = v_second->get_vorgaenger(0);
		}
	}
	DBG_MESSAGE( "ai_goods_t::create_ship_transport_vehikel()", "for %i ships", anz_vehikel );

	if(  convoihandle_t::is_exhausted()  ) {
		// too many convois => cannot do anything about this ...
		return false;
	}

	// must remove marker
	grund_t* gr = welt->lookup_kartenboden(platz1);
	if (gr) {
		gr->obj_loesche_alle(this);
	}
	// try to built dock
	const haus_besch_t* h = hausbauer_t::get_random_station(haus_besch_t::hafen, water_wt, welt->get_timeline_year_month(), haltestelle_t::WARE);
	if(h==NULL  ||  !call_general_tool(WKZ_STATION, platz1, h->get_name())) {
		return false;
	}

	// sea pos (and not on harbour ... )
	halthandle_t halt = haltestelle_t::get_halt(welt,gr->get_pos(),this);
	koord pos1 = platz1 - koord(gr->get_grund_hang())*h->get_groesse().y;
	koord best_pos = pos1;
	uint16 const cov = welt->get_settings().get_station_coverage();
	for(  int y = pos1.y - cov; y <= pos1.y + cov; ++y  ) {
		for(  int x = pos1.x - cov; x <= pos1.x + cov; ++x  ) {
			koord p(x,y);
			grund_t *gr = welt->lookup_kartenboden(p);
			// check for water tile, do not start in depots
			if(  gr->ist_wasser()  &&  halt == get_halt(p)  &&  gr->get_depot()==NULL  ) {
				if(  koord_distance(best_pos,platz2)<koord_distance(p,platz2)  ) {
					best_pos = p;
				}
			}
		}
	}

	// no stop position found
	if(best_pos==koord::invalid) {
		return false;
	}

	// since 86.01 we use lines for vehicles ...
	schedule_t *fpl=new schifffahrplan_t();
	fpl->append( welt->lookup_kartenboden(best_pos), 0 );
	fpl->append( welt->lookup(qfab->get_pos()), 100 );
	fpl->set_aktuell( 1 );
	fpl->eingabe_abschliessen();
	linehandle_t line=simlinemgmt.create_line(simline_t::shipline,this,fpl);
	delete fpl;

	// now create all vehicles as convois
	for(int i=0;  i<anz_vehikel;  i++) {
		if(  convoihandle_t::is_exhausted()  ) {
			// too many convois => cannot do anything about this ...
			return i>0;
		}
		vehikel_t* v = vehikelbauer_t::baue( qfab->get_pos(), this, NULL, ship_vehicle);
		convoi_t* cnv = new convoi_t(this);
		// V.Meyer: give the new convoi name from first vehicle
		cnv->set_name(v->get_besch()->get_name());
		cnv->add_vehikel( v );

		// two part consist
		if(v_second!=NULL) {
			v = vehikelbauer_t::baue( qfab->get_pos(), this, NULL, v_second );
			cnv->add_vehikel( v );
		}

		welt->sync_add( cnv );
		cnv->set_line(line);
		cnv->start();
	}
	clean_marker(platz1,size1);
	clean_marker(platz2,size2);
	platz1 += koord(welt->lookup_kartenboden(platz1)->get_grund_hang());
	return true;
}



/* changed to use vehicles searched before
 * @author prissi
 */
void ai_goods_t::create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
	const haus_besch_t* fh = hausbauer_t::get_random_station(haus_besch_t::generic_stop, road_wt, welt->get_timeline_year_month(), haltestelle_t::WARE);
	// succeed in frachthof creation
	if(fh  &&  call_general_tool(WKZ_STATION, platz1, fh->get_name())  &&  call_general_tool(WKZ_STATION, platz2, fh->get_name())  ) {
		koord3d pos1 = welt->lookup_kartenboden(platz1)->get_pos();
		koord3d pos2 = welt->lookup_kartenboden(platz2)->get_pos();

		int	start_location=0;
		// sometimes, when factories are very close, we need exakt calculation
		const koord3d& qpos = qfab->get_pos();
		if ((qpos.x - platz1.x) * (qpos.x - platz1.x) + (qpos.y - platz1.y) * (qpos.y - platz1.y) >
				(qpos.x - platz2.x) * (qpos.x - platz2.x) + (qpos.y - platz2.y) * (qpos.y - platz2.y)) {
			start_location = 1;
		}

		// calculate vehicle start position
		koord3d startpos=(start_location==0)?pos1:pos2;
		ribi_t::ribi w_ribi = welt->lookup(startpos)->get_weg_ribi_unmasked(road_wt);
		// now start all vehicle one field before, so they load immediately
		startpos = welt->lookup_kartenboden(koord(startpos.get_2d())+koord(w_ribi))->get_pos();

		// since 86.01 we use lines for road vehicles ...
		schedule_t *fpl=new autofahrplan_t();
		fpl->append(welt->lookup(pos1), start_location == 0 ? 100 : 0);
		fpl->append(welt->lookup(pos2), start_location == 1 ? 100 : 0);
		fpl->set_aktuell( start_location );
		fpl->eingabe_abschliessen();
		linehandle_t line=simlinemgmt.create_line(simline_t::truckline,this,fpl);
		delete fpl;

		// now create all vehicles as convois
		for(int i=0;  i<anz_vehikel;  i++) {
			if(  convoihandle_t::is_exhausted()  ) {
				// too many convois => cannot do anything about this ...
				return;
			}
			vehikel_t* v = vehikelbauer_t::baue(startpos, this, NULL, road_vehicle);
			convoi_t* cnv = new convoi_t(this);
			// V.Meyer: give the new convoi name from first vehicle
			cnv->set_name(v->get_besch()->get_name());
			cnv->add_vehikel( v );

			welt->sync_add( cnv );
			cnv->set_line(line);
			cnv->start();
		}
	}
}



/* now obeys timeline and use "more clever" scheme for vehicle selection *
 * @author prissi
 */
void ai_goods_t::create_rail_transport_vehikel(const koord platz1, const koord platz2, int anz_vehikel, int ladegrad)
{
	schedule_t *fpl;
	if(  convoihandle_t::is_exhausted()  ) {
		// too many convois => cannot do anything about this ...
		return;
	}
	convoi_t* cnv = new convoi_t(this);
	koord3d pos1= welt->lookup_kartenboden(platz1)->get_pos();
	koord3d pos2 = welt->lookup_kartenboden(platz2)->get_pos();

	// probably need to electrify the track?
	if(  rail_engine->get_engine_type()==vehikel_besch_t::electric  ) {
		// we need overhead wires
		const way_obj_besch_t *e = wayobj_t::wayobj_search(track_wt,overheadlines_wt,welt->get_timeline_year_month());
		wkz_wayobj_t wkz;
		wkz.set_default_param(e->get_name());
		wkz.init( welt, this );
		wkz.work( welt, this, welt->lookup_kartenboden(platz1)->get_pos() );
		wkz.work( welt, this, welt->lookup_kartenboden(platz2)->get_pos() );
		wkz.exit( welt, this );
	}

	koord3d start_pos = welt->lookup_kartenboden(pos1.get_2d() + (abs(size1.x)>abs(size1.y) ? koord(size1.x,0) : koord(0,size1.y)))->get_pos();
	vehikel_t* v = vehikelbauer_t::baue( start_pos, this, NULL, rail_engine);

	// V.Meyer: give the new convoi name from first vehicle
	cnv->set_name(rail_engine->get_name());
	cnv->add_vehikel( v );

	DBG_MESSAGE( "ai_goods_t::create_rail_transport_vehikel","for %i cars",anz_vehikel);

	/* now we add cars:
	 * check here also for introduction years
	 */
	for(int i = 0; i < anz_vehikel; i++) {
		// use the vehicle we searched before
		vehikel_t* v = vehikelbauer_t::baue(start_pos, this, NULL, rail_vehicle);
		cnv->add_vehikel( v );
	}

	fpl = cnv->front()->erzeuge_neuen_fahrplan();

	fpl->set_aktuell( 0 );
	fpl->append(welt->lookup(pos1), ladegrad);
	fpl->append(welt->lookup(pos2), 0);
	fpl->eingabe_abschliessen();

	cnv->set_schedule(fpl);
	welt->sync_add( cnv );
	cnv->start();
}



/* built a station
 * Can fail even though check has been done before
 * @author prissi
 */
int ai_goods_t::baue_bahnhof(const koord* p, int anz_vehikel)
{
	int laenge = max(((rail_vehicle->get_length()*anz_vehikel)+rail_engine->get_length()+CARUNITS_PER_TILE-1)/CARUNITS_PER_TILE,1);

	int baulaenge = 0;
	ribi_t::ribi ribi = welt->lookup_kartenboden(*p)->get_weg_ribi(track_wt);
	koord zv ( ribi );
	koord t = *p;
	bool ok = true;

	for(  int i=0;  i<laenge;  i++  ) {
		grund_t *gr = welt->lookup_kartenboden(t);
		ok &= (gr != NULL) &&  !gr->has_two_ways()  &&  gr->get_weg_hang()==hang_t::flach;
		if(!ok) {
			break;
		}
		baulaenge ++;
		t += zv;
	}

	// too short
	if(baulaenge<=1) {
		return 0;
	}

	// to avoid broken stations, they will be always built next to an existing
	bool make_all_bahnhof=false;

	// find a freight train station
	const haus_besch_t* besch = hausbauer_t::get_random_station(haus_besch_t::generic_stop, track_wt, welt->get_timeline_year_month(), haltestelle_t::WARE);
	if(besch==NULL) {
		// no freight station
		return 0;
	}
	koord pos;
	for(  pos=t-zv;  pos!=*p;  pos-=zv ) {
		if(  make_all_bahnhof  ||
			get_halt(pos+koord(-1,-1)).is_bound()  ||
			get_halt(pos+koord(-1, 1)).is_bound()  ||
			get_halt(pos+koord( 1,-1)).is_bound()  ||
			get_halt(pos+koord( 1, 1)).is_bound()
		) {
			// start building, if next to an existing station
			make_all_bahnhof = true;
			call_general_tool( WKZ_STATION, pos, besch->get_name() );
		}
		INT_CHECK("simplay 753");
	}
	// now add the other squares (going backwards)
	for(  pos=*p;  pos!=t;  pos+=zv ) {
		if(  !get_halt(pos).is_bound()  ) {
			call_general_tool( WKZ_STATION, pos, besch->get_name() );
		}
	}

	laenge = min( anz_vehikel, (baulaenge*CARUNITS_PER_TILE - rail_engine->get_length())/rail_vehicle->get_length() );
//DBG_MESSAGE("ai_goods_t::baue_bahnhof","Final station at (%i,%i) with %i tiles for %i cars",p->x,p->y,baulaenge,laenge);
	return laenge;
}



/* built a very simple track with just the minimum effort
 * usually good enough, since it can use road crossings
 * @author prissi
 */
bool ai_goods_t::create_simple_rail_transport()
{
	clean_marker(platz1,size1);
	clean_marker(platz2,size2);

	wegbauer_t bauigel(welt, this);
	bauigel.route_fuer( wegbauer_t::schiene|wegbauer_t::bot_flag, rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()) );
	bauigel.set_keep_existing_ways(false);
	// for stations
	wegbauer_t bauigel1(welt, this);
	bauigel1.route_fuer( wegbauer_t::schiene|wegbauer_t::bot_flag, rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()) );
	bauigel1.set_keep_existing_ways(false);
	wegbauer_t bauigel2(welt, this);
	bauigel2.route_fuer( wegbauer_t::schiene|wegbauer_t::bot_flag, rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()) );
	bauigel2.set_keep_existing_ways(false);

	// first: make plain stations tiles as intended
	sint8 z1 = max( welt->get_grundwasser()+1, welt->lookup_kartenboden(platz1)->get_hoehe() );
	koord k = platz1;
	koord diff1( sgn(size1.x), sgn(size1.y) );
	koord perpend( sgn(size1.y), sgn(size1.x) );
	while(k!=size1+platz1) {
		if(!welt->ebne_planquadrat( this, k, z1 )) {
			return false;
		}
		k += diff1;
	}

	// make the second ones flat ...
	sint8 z2 = max( welt->get_grundwasser()+1, welt->lookup_kartenboden(platz2)->get_hoehe() );
	k = platz2;
	perpend = koord( sgn(size2.y), sgn(size2.x) );
	koord diff2( sgn(size2.x), sgn(size2.y) );
	while(k!=size2+platz2) {
		if(!welt->ebne_planquadrat(this,k,z2)) {
			return false;
		}
		k += diff2;
	}

	bauigel1.calc_route( koord3d(platz1,z1), koord3d(platz1+size1-diff1, z1));
	bauigel2.calc_route( koord3d(platz2,z2), koord3d(platz2+size2-diff2, z2));

	// build immediately, otherwise wegbauer could get confused and connect way to a tile in the middle of the station
	bauigel1.baue();
	bauigel2.baue();

	vector_tpl<koord3d> starttiles, endtiles;
	// now calc the route
	starttiles.append(welt->lookup_kartenboden(platz1 + size1)->get_pos());
	starttiles.append(welt->lookup_kartenboden(platz1 - diff1)->get_pos());
	endtiles.append(welt->lookup_kartenboden(platz2 + size2)->get_pos());
	endtiles.append(welt->lookup_kartenboden(platz2 - diff2)->get_pos());
	bauigel.calc_route( starttiles, endtiles );
	INT_CHECK("ai_goods 672");

	// build only if enough cash available
	bool build_no_tf = (bauigel.get_count() > 4)  &&  (bauigel.calc_costs() <= finance->get_netwealth());

	// now try route with terraforming
	wegbauer_t baumaulwurf(welt, this);
	baumaulwurf.route_fuer( wegbauer_t::schiene|wegbauer_t::bot_flag|wegbauer_t::terraform_flag, rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()) );
	baumaulwurf.set_keep_existing_ways(false);
	baumaulwurf.calc_route( starttiles, endtiles );

	// build with terraforming if shorter and enough money is available
	bool with_tf = (baumaulwurf.get_count() > 4)  &&  (10*baumaulwurf.get_count() < 9*bauigel.get_count()  ||  bauigel.get_count() <= 4);

	// too expensive ?
	with_tf = with_tf  &&  (baumaulwurf.calc_costs() <= finance->get_netwealth());

	// now build with or without terraforming
	if (with_tf) {
		baumaulwurf.baue();
	}
	else if (build_no_tf) {
		bauigel.baue();
	}

	// connect track to station
	if(  with_tf  ||  build_no_tf  ) {
DBG_MESSAGE("ai_goods_t::create_simple_rail_transport()","building simple track from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		// connect to track

		koord3d_vector_t const& r     = with_tf ? baumaulwurf.get_route() : bauigel.get_route();
		koord3d                 tile1 = r.front();
		koord3d                 tile2 = r.back();
		if (!starttiles.is_contained(tile1)) sim::swap(tile1, tile2);
		// No botflag, since we want to connect with the station.
		bauigel.route_fuer( wegbauer_t::schiene, rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->get_geschw(),welt->get_timeline_year_month()) );
		bauigel.calc_straight_route( koord3d(platz1,z1), tile1);
		bauigel.baue();
		bauigel.calc_straight_route( koord3d(platz2,z2), tile2);
		bauigel.baue();
		// If connection is built not at platz1/2, we must alter platz1/2, otherwise baue_bahnhof gets confused.
		if(  tile1.get_2d() != platz1 + size1  ) {
			platz1 = platz1 + size1 - diff1;
			size1 = -size1;
		}
		if(  tile2.get_2d() != platz2 + size2  ) {
			platz2 = platz2 + size2 - diff2;
		}
		return true;
	}
	else {
		// not ok: remove station ...
		k = platz1;
		while(k!=size1+platz1) {
			int cost = -welt->lookup_kartenboden(k)->weg_entfernen( track_wt, true );
			book_construction_costs(this, cost, k, track_wt);
			k += diff1;
		}
		k = platz2;
		while(k!=size2+platz2) {
			int cost = -welt->lookup_kartenboden(k)->weg_entfernen( track_wt, true );
			book_construction_costs(this, cost, k, track_wt);
			k += diff2;
		}
	}
	return false;
}



// the normal length procedure for freigth AI
void ai_goods_t::step()
{
	// needed for schedule of stops ...
	spieler_t::step();

	if(!automat) {
		// I am off ...
		return;
	}

	// one route per month ...
	if(  welt->get_steps() < next_construction_steps  ) {
		return;
	}

	if(  finance->get_netwealth() < (finance->get_starting_money()/8)  ) {
		// nothing to do but to remove unneeded convois to gain some money
		state = CHECK_CONVOI;
	}

	switch(state) {

		case NR_INIT:

			state = NR_SAMMLE_ROUTEN;
			count = 0;
			built_update_headquarter();
			if(root==NULL) {
				// find a tree root to complete
				weighted_vector_tpl<fabrik_t *> start_fabs(20);
				FOR(  slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()  ) {
					// consumer and not completely overcrowded
					if(  fab->get_besch()->is_consumer_only()  &&  fab->get_status() != fabrik_t::bad  ) {
						int missing = get_factory_tree_missing_count( fab );
						if(  missing>0  ) {
							start_fabs.append( fab, 100/(missing+1)+1 );
						}
					}
				}
				if(  !start_fabs.empty()  ) {
					root = pick_any_weighted(start_fabs);
				}
			}
			// still nothing => we have to check convois ...
			if(root==NULL) {
				state = CHECK_CONVOI;
			}
		break;

		/* try to built a network:
		* last target also new target ...
		*/
		case NR_SAMMLE_ROUTEN:
			if(  get_factory_tree_lowest_missing(root)  ) {
				if(  start->get_besch()->get_platzierung()!=fabrik_besch_t::Wasser  ||  vehikel_search( water_wt, 0, 10, freight, false)!=NULL  ) {
					DBG_MESSAGE("ai_goods_t::do_ki", "Consider route from %s (%i,%i) to %s (%i,%i)", start->get_name(), start->get_pos().x, start->get_pos().y, ziel->get_name(), ziel->get_pos().x, ziel->get_pos().y );
					state = NR_BAUE_ROUTE1;
				}
				else {
					// add to impossible connections
					forbidden_connections.append( new fabconnection_t( start, ziel, freight ) );
					state = CHECK_CONVOI;
				}
			}
			else {
				// did all I could do here ...
				root = NULL;
				state = CHECK_CONVOI;
			}
		break;

		// now we need so select the cheapest mean to get maximum profit
		case NR_BAUE_ROUTE1:
		{
			/* if we reached here, we decide to built a route;
			 * the KI just chooses the way to run the operation at maximum profit (minimum loss).
			 * The KI will built also a loosing route; this might be required by future versions to
			 * be able to built a network!
			 * @author prissi
			 */

			/* for the calculation we need:
			 * a suitable car (and engine)
			 * a suitable weg
			 */
			uint32 dist = koord_distance( start->get_pos(), ziel->get_pos() );

			// guess the "optimum" speed (usually a little too low)
			sint32 best_rail_speed = 80;// is ok enough for goods, was: min(60+freight->get_speed_bonus()*5, 140 );
			sint32 best_road_speed = min(60+freight->get_speed_bonus()*5, 130 );

			INT_CHECK("simplay 1265");

			// is rail transport allowed?
			if(rail_transport) {
				// any rail car that transport this good (actually this weg_t the largest)
				rail_vehicle = vehikel_search( track_wt, 0, best_rail_speed,  freight, true);
			}
			rail_engine = NULL;
			rail_weg = NULL;
DBG_MESSAGE("do_ki()","rail vehicle %p",rail_vehicle);

			// is road transport allowed?
			if(road_transport) {
				// any road car that transport this good (actually this returns the largest)
				road_vehicle = vehikel_search( road_wt, 10, best_road_speed, freight, false);
			}
			road_weg = NULL;
DBG_MESSAGE("do_ki()","road vehicle %p",road_vehicle);

			ship_vehicle = NULL;
			if(start->get_besch()->get_platzierung()==fabrik_besch_t::Wasser) {
				// largest ship available
				ship_vehicle = vehikel_search( water_wt, 0, 20, freight, false);
			}

			INT_CHECK("simplay 1265");


			// properly calculate production
			const array_tpl<ware_production_t>& ausgang = start->get_ausgang();
			uint start_ware=0;
			while(  start_ware<ausgang.get_count()  &&  ausgang[start_ware].get_typ()!=freight  ) {
				start_ware++;
			}
			assert(  start_ware<ausgang.get_count()  );
			const int prod = min((uint32)ziel->get_base_production(),
			                 ( start->get_base_production() * start->get_besch()->get_produkt(start_ware)->get_faktor() )/256u - (uint32)(start->get_ausgang()[start_ware].get_stat(1, FAB_GOODS_DELIVERED)) );

DBG_MESSAGE("do_ki()","check railway");
			/* calculate number of cars for railroad */
			count_rail=255;	// no cars yet
			if(  rail_vehicle!=NULL  ) {
				// if our car is faster: well use slower speed to save money
				best_rail_speed = min(51, rail_vehicle->get_geschw());
				// for engine: gues number of cars
				count_rail = (prod*dist) / (rail_vehicle->get_zuladung()*best_rail_speed)+1;
				// assume the engine weight 100 tons for power needed calcualtion
				int total_weight = count_rail*( rail_vehicle->get_zuladung()*freight->get_weight_per_unit() + rail_vehicle->get_gewicht() );
//				long power_needed = (long)(((best_rail_speed*best_rail_speed)/2500.0+1.0)*(100.0+count_rail*(rail_vehicle->get_gewicht()+rail_vehicle->get_zuladung()*freight->get_weight_per_unit()*0.001)));
				rail_engine = vehikel_search( track_wt, total_weight/1000, best_rail_speed, NULL, wayobj_t::default_oberleitung!=NULL);
				if(  rail_engine!=NULL  ) {
					best_rail_speed = min(rail_engine->get_geschw(),rail_vehicle->get_geschw());
					// find cheapest track with that speed (and no monorail/elevated/tram tracks, please)
					rail_weg = wegbauer_t::weg_search( track_wt, best_rail_speed, welt->get_timeline_year_month(),weg_t::type_flat );
					if(  rail_weg!=NULL  ) {
						if(  best_rail_speed>rail_weg->get_topspeed()  ) {
							best_rail_speed = rail_weg->get_topspeed();
						}
						// no train can have more than 15 cars
						count_rail = min( 22, (3*prod*dist) / (rail_vehicle->get_zuladung()*best_rail_speed*2) );
						// if engine too week, reduce number of cars
						if(  count_rail*80*64>(int)(rail_engine->get_leistung()*rail_engine->get_gear())  ) {
							count_rail = rail_engine->get_leistung()*rail_engine->get_gear()/(80*64);
						}
						count_rail = ((count_rail+1)&0x0FE)+1;
DBG_MESSAGE("ai_goods_t::do_ki()","Engine %s guess to need %d rail cars %s for route (%s)", rail_engine->get_name(), count_rail, rail_vehicle->get_name(), rail_weg->get_name() );
					}
				}
				if(  rail_engine==NULL  ||  rail_weg==NULL  ) {
					// no rail transport possible
DBG_MESSAGE("ai_goods_t::do_ki()","No railway possible.");
					rail_vehicle = NULL;
					count_rail = 255;
				}
			}

			INT_CHECK("simplay 1265");

DBG_MESSAGE("do_ki()","check railway");
			/* calculate number of cars for road; much easier */
			count_road=255;	// no cars yet
			if(  road_vehicle!=NULL  ) {
				best_road_speed = road_vehicle->get_geschw();
				// find cheapest road
				road_weg = wegbauer_t::weg_search( road_wt, best_road_speed, welt->get_timeline_year_month(),weg_t::type_flat );
				if(  road_weg!=NULL  ) {
					if(  best_road_speed>road_weg->get_topspeed()  ) {
						best_road_speed = road_weg->get_topspeed();
					}
					// minimum vehicle is 1, maximum vehicle is 48, more just result in congestion
					count_road = min( 254, (prod*dist) / (road_vehicle->get_zuladung()*best_road_speed*2)+2 );
DBG_MESSAGE("ai_goods_t::do_ki()","guess to need %d road cars %s for route %s", count_road, road_vehicle->get_name(), road_weg->get_name() );
				}
				else {
					// no roads there !?!
DBG_MESSAGE("ai_goods_t::do_ki()","No roadway possible.");
				}
			}

			// find the cheapest transport ...
			// assume maximum cost
			int	cost_rail=0x7FFFFFFF, cost_road=0x7FFFFFFF;
			int	income_rail=0, income_road=0;

			// calculate cost for rail
			if(  count_rail<255  ) {
				int freight_price = (freight->get_preis()*rail_vehicle->get_zuladung()*count_rail)/24*((8000+(best_rail_speed-80)*freight->get_speed_bonus())/1000);
				// calculated here, since the above number was based on production
				// only uneven number of cars bigger than 3 makes sense ...
				count_rail = max( 3, count_rail );
				income_rail = (freight_price*best_rail_speed)/(2*dist+count_rail);
				cost_rail = rail_weg->get_wartung() + (((count_rail+1)/2)*300)/dist + ((count_rail*rail_vehicle->get_betriebskosten()+rail_engine->get_betriebskosten())*best_rail_speed)/(2*dist+count_rail);
				DBG_MESSAGE("ai_goods_t::do_ki()","Netto credits per day for rail transport %.2f (income %.2f)",cost_rail/100.0, income_rail/100.0 );
				cost_rail -= income_rail;
			}

			// and calculate cost for road
			if(  count_road<255  ) {
				// for short distance: reduce number of cars
				// calculated here, since the above number was based on production
				count_road = CLIP( (sint32)(dist*15)/best_road_speed, 2, count_road );
				int freight_price = (freight->get_preis()*road_vehicle->get_zuladung()*count_road)/24*((8000+(best_road_speed-80)*freight->get_speed_bonus())/1000);
				cost_road = road_weg->get_wartung() + 300/dist + (count_road*road_vehicle->get_betriebskosten()*best_road_speed)/(2*dist+5);
				income_road = (freight_price*best_road_speed)/(2*dist+5);
				DBG_MESSAGE("ai_goods_t::do_ki()","Netto credits per day and km for road transport %.2f (income %.2f)",cost_road/100.0, income_road/100.0 );
				cost_road -= income_road;
			}

			// check location, if vehicles found
			if(  min(count_road,count_rail)!=255  ) {
				// road or rail?
				int length = 1;
				if(  cost_rail<cost_road  ) {
					length = (rail_engine->get_length() + count_rail*rail_vehicle->get_length()+CARUNITS_PER_TILE-1)/CARUNITS_PER_TILE;
					if(suche_platz1_platz2(start, ziel, length)) {
						state = ship_vehicle ? NR_BAUE_WATER_ROUTE : NR_BAUE_SIMPLE_SCHIENEN_ROUTE;
						next_construction_steps += 10;
					}
				}
				// if state is still NR_BAUE_ROUTE1 then there are no sutiable places
				if(state==NR_BAUE_ROUTE1  &&  (count_road != 255)  &&  suche_platz1_platz2(start, ziel, 0)) {
					// rail was too expensive or not successfull
					count_rail = 255;
					state = ship_vehicle ? NR_BAUE_WATER_ROUTE : NR_BAUE_STRASSEN_ROUTE;
					next_construction_steps += 10;
				}
			}
			// no success at all?
			if(state==NR_BAUE_ROUTE1) {
				// maybe this route is not builtable ... add to forbidden connections
				forbidden_connections.append( new fabconnection_t( start, ziel, freight ) );
				ziel = NULL;	// otherwise it may always try to built the same route!
				state = CHECK_CONVOI;
			}
		}
		break;

		// built a simple ship route
		case NR_BAUE_WATER_ROUTE:
			if(is_connected(start->get_pos().get_2d(), ziel->get_pos().get_2d(), freight)) {
				state = CHECK_CONVOI;
			}
			else {
				// properly calculate production
				const array_tpl<ware_production_t>& ausgang = start->get_ausgang();
				uint start_ware=0;
				while(  start_ware<ausgang.get_count()  &&  ausgang[start_ware].get_typ()!=freight  ) {
					start_ware++;
				}
				const sint32 prod = min( ziel->get_base_production(), (sint32)(start->get_base_production() * start->get_besch()->get_produkt(start_ware)->get_faktor()) - (sint32)(start->get_ausgang()[start_ware].get_stat(1, FAB_GOODS_DELIVERED)) );
				if(prod<0) {
					// too much supplied last time?!? => retry
					state = CHECK_CONVOI;
					break;
				}
				// just remember the position, where the harbour will be built
				harbour=platz1;
				int ships_needed = 1 + (prod*koord_distance(harbour,start->get_pos().get_2d())) / (ship_vehicle->get_zuladung()*max(20,ship_vehicle->get_geschw()));
				if(create_ship_transport_vehikel(start,ships_needed)) {
					if(welt->lookup(harbour)->get_halt()->get_fab_list().is_contained(ziel)) {
						// so close, so we are already connected
						grund_t *gr = welt->lookup_kartenboden(platz2);
						if (gr) gr->obj_loesche_alle(this);
						state = (rail_vehicle  &&  count_rail<255) ? NR_RAIL_SUCCESS : NR_ROAD_SUCCESS;
					}
					else {
						// else we need to built the second part of the route
						state = (rail_vehicle  &&  count_rail<255) ? NR_BAUE_SIMPLE_SCHIENEN_ROUTE : NR_BAUE_STRASSEN_ROUTE;
					}
				}
				else {
					ship_vehicle = NULL;
					state = NR_BAUE_CLEAN_UP;
				}
			}
			break;

		// built a simple railroad
		case NR_BAUE_SIMPLE_SCHIENEN_ROUTE:
			if(is_connected(start->get_pos().get_2d(), ziel->get_pos().get_2d(), freight)) {
				state = ship_vehicle ? NR_BAUE_CLEAN_UP : CHECK_CONVOI;
			}
			else if(create_simple_rail_transport()) {
				sint16 org_count_rail = count_rail;
				count_rail = baue_bahnhof(&platz1, count_rail);
				if(count_rail>=3) {
					count_rail = baue_bahnhof(&platz2, count_rail);
				}
				if(count_rail>=3) {
					if(count_rail<org_count_rail) {
						// rethink engine
						int best_rail_speed = min(51, rail_vehicle->get_geschw());
						// for engine: gues number of cars
						long power_needed=(long)(((best_rail_speed*best_rail_speed)/2500.0+1.0)*(100.0+count_rail*( (rail_vehicle->get_gewicht()+rail_vehicle->get_zuladung()*freight->get_weight_per_unit())*0.001 )));
						const vehikel_besch_t *v=vehikel_search( track_wt, power_needed, best_rail_speed, NULL, false);
						if(v->get_betriebskosten()<rail_engine->get_betriebskosten()) {
							rail_engine = v;
						}
					}
					create_rail_transport_vehikel( platz1, platz2, count_rail, 100 );
					state = NR_RAIL_SUCCESS;
				}
				else {
DBG_MESSAGE("ai_goods_t::step()","remove already constructed rail between %i,%i and %i,%i and try road",platz1.x,platz1.y,platz2.x,platz2.y);
					// no sucess: clean route
					char param[16];
					sprintf( param, "%i", track_wt );
					wkz_wayremover_t wkz;
					wkz.set_default_param(param);
					wkz.init( welt, this );
					wkz.work( welt, this, welt->lookup_kartenboden(platz1)->get_pos() );
					wkz.work( welt, this, welt->lookup_kartenboden(platz2)->get_pos() );
					wkz.exit( welt, this );
					if( (count_road != 255) && suche_platz1_platz2(start, ziel, 0) ) {
						state = NR_BAUE_STRASSEN_ROUTE;
					}
					else {
						state = NR_BAUE_CLEAN_UP;
					}
				}
			}
			else {
				if( (count_road != 255) && suche_platz1_platz2(start, ziel, 0) ) {
					state = NR_BAUE_STRASSEN_ROUTE;
				}
				else {
					state = NR_BAUE_CLEAN_UP;
				}
			}
		break;

		// built a simple road (no bridges, no tunnels)
		case NR_BAUE_STRASSEN_ROUTE:
			if(is_connected(start->get_pos().get_2d(), ziel->get_pos().get_2d(), freight)) {
				state = ship_vehicle ? NR_BAUE_CLEAN_UP : CHECK_CONVOI;
			}
			else if(create_simple_road_transport(platz1,size1,platz2,size2,road_weg)) {
				create_road_transport_vehikel(start, count_road );
				state = NR_ROAD_SUCCESS;
			}
			else {
				state = NR_BAUE_CLEAN_UP;
			}
		break;

		// remove marker etc.
		case NR_BAUE_CLEAN_UP:
		{
			if(start!=NULL  &&  ziel!=NULL) {
				forbidden_connections.append( new fabconnection_t( start, ziel, freight ) );
			}
			if(ship_vehicle) {
				// only here, if we could built ships but no connection
				halthandle_t start_halt = get_halt(harbour);
				if(  start_halt.is_bound()  &&  (start_halt->get_station_type()&haltestelle_t::dock)!=0  ) {
					// delete all ships on this line
					vector_tpl<linehandle_t> lines;
					simlinemgmt.get_lines( simline_t::shipline, &lines );
					if(!lines.empty()) {
						linehandle_t line = lines.back();
						schedule_t *fpl=line->get_schedule();
						if(fpl->get_count()>1  &&  haltestelle_t::get_halt(welt,fpl->eintrag[0].pos,this)==start_halt) {
							while(line->count_convoys()>0) {
								convoihandle_t cnv = line->get_convoy(0);
								cnv->self_destruct();
								if(cnv.is_bound()) {
									cnv->step();
								}
							}
							simlinemgmt.delete_line( line );
						}
					}
				}
				// delete harbour
				call_general_tool( WKZ_REMOVER, harbour, NULL );
			}
			harbour = koord::invalid;
			// otherwise it may always try to built the same route!
			ziel = NULL;
			// schilder aufraeumen
			clean_marker(platz1,size1);
			clean_marker(platz2,size2);
			state = CHECK_CONVOI;
			break;
		}

		// successful rail construction
		case NR_RAIL_SUCCESS:
		{
			// tell the player
			cbuffer_t buf;
			const koord3d& spos = start->get_pos();
			const koord3d& zpos = ziel->get_pos();
			buf.printf( translator::translate("%s\nopened a new railway\nbetween %s\nat (%i,%i) and\n%s at (%i,%i)."), get_name(), translator::translate(start->get_name()), spos.x, spos.y, translator::translate(ziel->get_name()), zpos.x, zpos.y);
			welt->get_message()->add_message(buf, spos.get_2d(), message_t::ai, PLAYER_FLAG|player_nr, rail_engine->get_basis_bild());

			harbour = koord::invalid;
			state = CHECK_CONVOI;
		}
		break;

		// successful rail construction
		case NR_ROAD_SUCCESS:
		{
			// tell the player
			cbuffer_t buf;
			const koord3d& spos = start->get_pos();
			const koord3d& zpos = ziel->get_pos();
			buf.printf( translator::translate("%s\nnow operates\n%i trucks between\n%s at (%i,%i)\nand %s at (%i,%i)."), get_name(), count_road, translator::translate(start->get_name()), spos.x, spos.y, translator::translate(ziel->get_name()), zpos.x, zpos.y);
			welt->get_message()->add_message(buf, spos.get_2d(), message_t::ai, PLAYER_FLAG|player_nr, road_vehicle->get_basis_bild());

			harbour = koord::invalid;
			state = CHECK_CONVOI;
		}
		break;

		// remove stucked vehicles (only from roads!)
		case CHECK_CONVOI:
		{
			next_construction_steps = welt->get_steps() + ((1+finance->get_account_overdrawn())>0)*simrand( ai_t::construction_speed ) + 25;

			for (size_t i = welt->convoys().get_count(); i-- != 0;) {
				convoihandle_t const cnv = welt->convoys()[i];
				if(!cnv.is_bound()  ||  cnv->get_besitzer()!=this) {
					continue;
				}

				if (cnv->front()->get_waytype() == water_wt) {
					// ships will be only deleted together with the connecting convoi
					continue;
				}

				sint64 gewinn = 0;
				for( int j=0;  j<12;  j++  ) {
					gewinn += cnv->get_finance_history( j, convoi_t::CONVOI_PROFIT );
				}

				// apparently we got the toatlly wrong vehicle here ...
				// (but we will delete it only, if we need, because it may be needed for a chain)
				bool delete_this = (finance->get_account_overdrawn() > 0)  &&  (gewinn < -cnv->calc_restwert());

				// check for empty vehicles (likely stucked) that are making no plus and remove them ...
				// take care, that the vehicle is old enough ...
				if (!delete_this && (welt->get_current_month() - cnv->front()->get_insta_zeit()) > 6 && gewinn <= 0) {
					sint64 goods=0;
					// no goods for six months?
					for( int i=0;  i<6;  i ++) {
						goods += cnv->get_finance_history( i, convoi_t::CONVOI_TRANSPORTED_GOODS );
					}
					delete_this = (goods==0);
				}

				// well, then delete this (likely stucked somewhere) or insanely unneeded
				if(delete_this) {
					waytype_t const wt = cnv->front()->get_besch()->get_waytype();
					linehandle_t line = cnv->get_line();
					DBG_MESSAGE("ai_goods_t::do_ki()","%s retires convoi %s!", get_name(), cnv->get_name());

					koord3d start_pos, end_pos;
					schedule_t *fpl = cnv->get_schedule();
					if(fpl  &&  fpl->get_count()>1) {
						start_pos = fpl->eintrag[0].pos;
						end_pos = fpl->eintrag[1].pos;
					}

					cnv->self_destruct();
					if(cnv.is_bound()) {
						cnv->step();	// to really get rid of it
					}

					// last vehicle on that connection (no line => railroad)
					if(  !line.is_bound()  ||  line->count_convoys()==0  ) {
						// check if a conncetion boat must be removed
						halthandle_t start_halt = haltestelle_t::get_halt(welt,start_pos,this);
						if(start_halt.is_bound()  &&  (start_halt->get_station_type()&haltestelle_t::dock)!=0) {
							// delete all ships on this line
							vector_tpl<linehandle_t> lines;
							koord water_stop = koord::invalid;
							simlinemgmt.get_lines( simline_t::shipline, &lines );
							FOR(vector_tpl<linehandle_t>, const line, lines) {
								schedule_t *fpl=line->get_schedule();
								if(fpl->get_count()>1  &&  haltestelle_t::get_halt(welt,fpl->eintrag[0].pos,this)==start_halt) {
									water_stop = koord( (start_pos.x+fpl->eintrag[0].pos.x)/2, (start_pos.y+fpl->eintrag[0].pos.y)/2 );
									while(line->count_convoys()>0) {
										convoihandle_t cnv = line->get_convoy(0);
										cnv->self_destruct();
										if(cnv.is_bound()) {
											cnv->step();
										}
									}
									simlinemgmt.delete_line( line );
								}
							}
							// delete harbour
							call_general_tool( WKZ_REMOVER, water_stop, NULL );
						}
					}

					if(wt==track_wt) {
						char param[16];
						sprintf( param, "%i", track_wt );
						wkz_wayremover_t wkz;
						wkz.set_default_param(param);
						wkz.init( welt, this );
						wkz.work( welt, this, start_pos );
						wkz.work( welt, this, end_pos );
						wkz.exit( welt, this );
					}
					else {
						// last convoi => remove completely<
						if(line.is_bound()  &&  line->count_convoys()==0) {
							simlinemgmt.delete_line( line );

							char param[16];
							sprintf( param, "%i", wt );
							wkz_wayremover_t wkz;
							wkz.set_default_param(param);
							wkz.init( welt, this );
							wkz.work( welt, this, start_pos );
							if(wkz.work( welt, this, end_pos )!=NULL) {
								// cannot remove all => likely some other convois there too
								// remove loading bays and road on start and end, if we cannot remove the whole way
								wkz.work( welt, this, start_pos );
								wkz.work( welt, this, start_pos );
								wkz.work( welt, this, end_pos );
								wkz.work( welt, this, end_pos );
							}
							wkz.exit( welt, this );
						}
					}
					break;
				}
			}
			state = NR_INIT;
		}
		break;

		default:
			dbg->warning("ai_goods_t::step()","Illegal state!", state );
			state = NR_INIT;
	}
}



void ai_goods_t::rdwr(loadsave_t *file)
{
	if(  file->get_version()<102002  ) {
		// due to an error the player was never saved correctly
		spieler_t::rdwr(file);
		return;
	}

	xml_tag_t t( file, "ai_goods_t" );

	ai_t::rdwr(file);

	// then check, if we have to do something or the game is too old ...
	if(file->get_version()<101000) {
		// ignore saving, reinit on loading
		if(  file->is_loading()  ) {
			state = NR_INIT;

			road_vehicle = NULL;
			road_weg = NULL;

			next_construction_steps = welt->get_steps()+simrand(400);
			root = start = ziel = NULL;
		}
		return;
	}

	// now save current state ...
	file->rdwr_enum(state);
	platz1.rdwr( file );
	size1.rdwr( file );
	platz2.rdwr( file );
	size2.rdwr( file );
	file->rdwr_long(count_rail);
	file->rdwr_long(count_road);
	file->rdwr_long(count);
	if(  file->get_version()<111001  ) {
		file->rdwr_bool(road_transport);
		file->rdwr_bool(rail_transport);
		file->rdwr_bool(ship_transport);
		air_transport = false;
	}

	if(file->is_saving()) {
		// save current pointers
		sint32 delta_steps = next_construction_steps-welt->get_steps();
		file->rdwr_long(delta_steps);
		koord3d k3d = root ? root->get_pos() : koord3d::invalid;
		k3d.rdwr(file);
		k3d = start ? start->get_pos() : koord3d::invalid;
		k3d.rdwr(file);
		k3d = ziel ? ziel->get_pos() : koord3d::invalid;
		k3d.rdwr(file);
		// what freight?
		const char *s = freight ? freight->get_name() : NULL;
		file->rdwr_str( s );
		// vehicles besch
		s = rail_engine ? rail_engine->get_name() : NULL;
		file->rdwr_str( s );
		s = rail_vehicle ? rail_vehicle->get_name() : NULL;
		file->rdwr_str( s );
		s = road_vehicle ? road_vehicle->get_name() : NULL;
		file->rdwr_str( s );
		s = ship_vehicle ? ship_vehicle->get_name() : NULL;
		file->rdwr_str( s );
		// ways
		s = rail_weg ? rail_weg->get_name() : NULL;
		file->rdwr_str( s );
		s = road_weg ? road_weg->get_name() : NULL;
		file->rdwr_str( s );
	}
	else {
		// since steps in loaded game == 0
		file->rdwr_long(next_construction_steps);
		// reinit current pointers
		koord3d k3d;
		k3d.rdwr(file);
		root = fabrik_t::get_fab( welt, k3d.get_2d() );
		k3d.rdwr(file);
		start = fabrik_t::get_fab( welt, k3d.get_2d() );
		k3d.rdwr(file);
		ziel = fabrik_t::get_fab( welt, k3d.get_2d() );
		// freight?
		const char *temp=NULL;
		file->rdwr_str( temp );
		freight = temp ? warenbauer_t::get_info(temp) : NULL;
		// vehicles
		file->rdwr_str( temp );
		rail_engine = temp ? vehikelbauer_t::get_info(temp) : NULL;
		file->rdwr_str( temp );
		rail_vehicle = temp ? vehikelbauer_t::get_info(temp) : NULL;
		file->rdwr_str( temp );
		road_vehicle = temp ? vehikelbauer_t::get_info(temp) : NULL;
		file->rdwr_str( temp );
		ship_vehicle = temp ? vehikelbauer_t::get_info(temp) : NULL;
		// ways
		file->rdwr_str( temp );
		rail_weg = temp ? wegbauer_t::get_besch(temp,0) : NULL;
		file->rdwr_str( temp );
		road_weg = temp ? wegbauer_t::get_besch(temp,0) : NULL;
	}

	// finally: forbidden connection list
	sint32 cnt = forbidden_connections.get_count();
	file->rdwr_long(cnt);
	if(file->is_saving()) {
		FOR(slist_tpl<fabconnection_t*>, const fc, forbidden_connections) {
			fc->rdwr(file);
		}
	}
	else {
		forbidden_connections.clear();
		while(  cnt-->0  ) {
			fabconnection_t *fc = new fabconnection_t(0,0,0);
			fc->rdwr(file);
			// @author Bernd Gabriel, Jan 01, 2010: Don't add, if fab or ware no longer in the game.
			if(  fc->fab1  &&  fc->fab2  &&  fc->ware  ) {
				forbidden_connections.append(fc);
			}
			else {
				delete fc;
			}
		}
	}
	// save harbour position
	if(  file->get_version() > 110000  ) {
		harbour.rdwr(file);
	}
}


bool ai_goods_t::is_forbidden( fabrik_t *fab1, fabrik_t *fab2, const ware_besch_t *w ) const
{
	fabconnection_t fc(fab1, fab2, w);
	FOR(slist_tpl<fabconnection_t*>, const test_fc, forbidden_connections) {
		if (fc == (*test_fc)) {
			return true;
		}
	}
	return false;
}


void ai_goods_t::fabconnection_t::rdwr(loadsave_t *file)
{
	koord3d k3d;
	if(file->is_saving()) {
		k3d = fab1->get_pos();
		k3d.rdwr(file);
		k3d = fab2->get_pos();
		k3d.rdwr(file);
		const char *s = ware->get_name();
		file->rdwr_str( s );
	}
	else {
		k3d.rdwr(file);
		fab1 = fabrik_t::get_fab( welt, k3d.get_2d() );
		k3d.rdwr(file);
		fab2 = fabrik_t::get_fab( welt, k3d.get_2d() );
		const char *temp=NULL;
		file->rdwr_str( temp );
		ware = warenbauer_t::get_info(temp);
	}
}


/**
 * Dealing with stucked  or lost vehicles:
 * - delete lost ones
 * - ignore stucked ones
 * @author prissi
 * @date 30-Dec-2008
 */
void ai_goods_t::bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel)
{
	if(  cnv->get_state() == convoi_t::NO_ROUTE  &&  this!=welt->get_active_player()  ) {
			DBG_MESSAGE("ai_passenger_t::bescheid_vehikel_problem","Vehicle %s can't find a route to (%i,%i)!", cnv->get_name(),ziel.x,ziel.y);
			cnv->self_destruct();
			return;
	}
	spieler_t::bescheid_vehikel_problem( cnv, ziel );
}


/**
 * Tells the player that a fabrik_t is going to be deleted.
 * It could also tell, that a fab has been created, but by now the fabrikbauer_t does not.
 * @author Bernd Gabriel, Jan 01, 2010
 */
void ai_goods_t::notify_factory(notification_factory_t flag, const fabrik_t* fab)
{
	switch(flag) {
		// factory is going to be deleted
		case notify_delete:
			if (start==fab  ||  ziel==fab  ||  root==fab) {
				root = NULL;
				start = NULL;
				ziel = NULL;
				// set new state
				if (state == NR_SAMMLE_ROUTEN  ||  state == NR_BAUE_ROUTE1) {
					state = NR_INIT;
				}
				else if (state == NR_BAUE_SIMPLE_SCHIENEN_ROUTE  ||  state == NR_BAUE_STRASSEN_ROUTE  ||  state == NR_BAUE_WATER_ROUTE) {
					state = NR_BAUE_CLEAN_UP;
				}
				else if (state == NR_RAIL_SUCCESS  ||  state == NR_ROAD_SUCCESS ||  state == NR_WATER_SUCCESS) {
					state = CHECK_CONVOI;
				}
				for(  slist_tpl<fabconnection_t *>::iterator i=forbidden_connections.begin();  i!=forbidden_connections.end();  ) {
					fabconnection_t *fc = *i;
					if (fc->fab1 == fab || fc->fab2 == fab)
					{
						i = forbidden_connections.erase(i);
						delete fc;
					}
					else {
						++i;
					}
				}
			}
			break;
		default: ;
	}
}
