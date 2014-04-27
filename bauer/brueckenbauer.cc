/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../simdebug.h"
#include "../simwerkz.h"
#include "brueckenbauer.h"
#include "wegbauer.h"

#include "../simworld.h"
#include "../simhalt.h"
#include "../simdepot.h"
#include "../player/simplay.h"
#include "../simtypes.h"

#include "../boden/boden.h"
#include "../boden/brueckenboden.h"

#include "../gui/werkzeug_waehler.h"
#include "../gui/karte.h"

#include "../besch/bruecke_besch.h"

#include "../dataobj/marker.h"
#include "../dataobj/scenario.h"
#include "../obj/bruecke.h"
#include "../obj/leitung2.h"
#include "../obj/pillar.h"
#include "../obj/signal.h"
#include "../dataobj/crossing_logic.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"

karte_ptr_t brueckenbauer_t::welt;


/// All bridges hashed by name
static stringhashtable_tpl<const bruecke_besch_t *> bruecken_by_name;


void brueckenbauer_t::register_besch(bruecke_besch_t *besch)
{
	// avoid duplicates with same name
	if( const bruecke_besch_t *old_besch = bruecken_by_name.get(besch->get_name()) ) {
		dbg->warning( "brueckenbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		bruecken_by_name.remove(besch->get_name());
		werkzeug_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}

	// add the tool
	wkz_brueckenbau_t *wkz = new wkz_brueckenbau_t();
	wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
	wkz->cursor = besch->get_cursor()->get_bild_nr(0);
	wkz->set_default_param(besch->get_name());
	werkzeug_t::general_tool.append( wkz );
	besch->set_builder( wkz );
	bruecken_by_name.put(besch->get_name(), besch);
}


const bruecke_besch_t *brueckenbauer_t::get_besch(const char *name)
{
	return bruecken_by_name.get(name);
}


const bruecke_besch_t *brueckenbauer_t::find_bridge(const waytype_t wtyp, const sint32 min_speed, const uint16 time)
{
	const bruecke_besch_t *find_besch=NULL;

	FOR(stringhashtable_tpl<bruecke_besch_t const*>, const& i, bruecken_by_name) {
		bruecke_besch_t const* const besch = i.value;
		if(  besch->get_waytype()==wtyp  &&  besch->is_available(time)  ) {
			if(  find_besch==NULL  ||
				(find_besch->get_topspeed()<min_speed  &&  find_besch->get_topspeed()<besch->get_topspeed())  ||
				(besch->get_topspeed()>=min_speed  &&  besch->get_wartung()<find_besch->get_wartung())
			) {
				find_besch = besch;
			}
		}
	}
	return find_besch;
}


/**
 * Compares the maximum speed of two bridges.
 * @param a the first bridge.
 * @param b the second bridge.
 * @return true, if the speed of the second bridge is greater.
 */
static bool compare_bridges(const bruecke_besch_t* a, const bruecke_besch_t* b)
{
	return a->get_topspeed() < b->get_topspeed();
}


void brueckenbauer_t::fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), WKZ_BRUECKENBAU | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();
	vector_tpl<const bruecke_besch_t*> matching(bruecken_by_name.get_count());

	// list of matching types (sorted by speed)
	FOR(stringhashtable_tpl<bruecke_besch_t const*>, const& i, bruecken_by_name) {
		bruecke_besch_t const* const b = i.value;
		if (  b->get_waytype()==wtyp  &&  b->is_available(time)  ) {
			matching.insert_ordered( b, compare_bridges);
		}
	}

	// now sorted ...
	FOR(vector_tpl<bruecke_besch_t const*>, const i, matching) {
		wzw->add_werkzeug(i->get_builder());
	}
}



inline bool ribi_check( ribi_t::ribi ribi, ribi_t::ribi check_ribi )
{
	// either check for single (if nothing given) otherwise ensure exact match
	return check_ribi ? ribi == check_ribi : ribi_t::ist_einfach( ribi );
}


/**
 * if check_ribi==0, then any single tile is ok
 * @returns either (1) error_message (must  abort bridge here) (2) "" if it could end here or (3) NULL if it must end here
 */
const char *check_tile( const grund_t *gr, const spieler_t *sp, waytype_t wt, ribi_t::ribi check_ribi )
{
	// not overbuilt transformers
	if(  gr->find<senke_t>()!=NULL  ||  gr->find<pumpe_t>()!=NULL  ) {
		return "A bridge must start on a way!";
	}

	if(  !hang_t::ist_wegbar(gr->get_grund_hang())  ) {
		return "Bruecke muss an\neinfachem\nHang beginnen!\n";
	}

	if(  gr->is_halt()  ||  gr->get_depot()  ) {
		// something in the way
		return "Tile not empty.";
	}

	if(  wt != powerline_wt  &&  gr->get_leitung()  ) {
		return "Tile not empty.";
	}

	// we can build a ramp when there is one (or with tram two) way in our direction and no stations/depot etc.
	if(  weg_t *w = gr->get_weg_nr(0)  ) {

		if(  !spieler_t::check_owner(w->get_besitzer(),sp)  ) {
			// not our way
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}

		// now check for direction
		ribi_t::ribi ribi = w->get_ribi_unmasked();
		if(  weg_t *w2 = gr->get_weg_nr(1)  ) {
			if(  !spieler_t::check_owner(w2->get_besitzer(),sp)  ) {
				// not our way
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  !gr->ist_uebergang()  ) {
				// If road and tram, we have to check both ribis.
				ribi = gr->get_weg_nr(0)->get_ribi_unmasked() | gr->get_weg_nr(1)->get_ribi_unmasked();
			}
			else {
				// else only the straight ones
				ribi = gr->get_weg_ribi_unmasked(wt);
			}
			// same waytype, same direction, no stop or depot or any other stuff */
			if(  gr->get_weg(wt)  &&  ribi_t::doppelt(ribi) == ribi_t::doppelt( check_ribi )  ) {
				// ok too
				return NULL;
			}
			return "A bridge must start on a way!";
		}

		if(  w->get_waytype() != wt  ) {
			// now check for perpendicular and crossing
			if(  (ribi_t::doppelt(ribi) ^ ribi_t::doppelt(check_ribi) ) == ribi_t::alle  &&  crossing_logic_t::get_crossing(wt, w->get_waytype(), 0, 0, 0)  ) {
				return NULL;
			}
			return "A bridge must start on a way!";
		}

		// same waytype, any direction, no stop or depot or any other stuff */
		if(  w->get_waytype() == wt  ) {
			// ok too
			return NULL;
		}

		// one or two non-matching ways where I cannot built a crossing
		return "A bridge must start on a way!";
	}
	else if(  wt == powerline_wt  ) {
		if(  leitung_t *lt = gr->get_leitung()  ) {
			if(  spieler_t::check_owner(lt->get_besitzer(),sp)  &&  ribi_check( lt->get_ribi(), check_ribi )  ) {
				// matching powerline
				return NULL;
			}
			return "A bridge must start on a way!";
		}
	}
	// somethign here which we cannot remove => fail too
	if(  obj_t *obj=gr->obj_bei(0)  ) {
		if(  const char *err_msg = obj->ist_entfernbar(sp)  ) {
			return err_msg;
		}
	}
	return "";	// could end here by must not end here
}


koord3d brueckenbauer_t::finde_ende(spieler_t *sp, koord3d pos, const koord zv, const bruecke_besch_t *besch, const char *&error_msg, bool ai_bridge, uint32 min_length )
{
	const grund_t *gr2 = welt->lookup( pos );
	if(  !gr2  ) {
		error_msg = "A bridge must start on a way!";
		return koord3d::invalid;
	}

	// get initial height of bridge from start tile
	// flat -> height is 1 if only shallow ramps, else height 2
	// single height -> height is 1
	// double height -> height is 2
	const hang_t::typ slope = gr2->get_grund_hang();
	const sint8 start_height = gr2->get_hoehe() + hang_t::max_diff(slope);
	sint8 min_height = start_height - (1+besch->has_double_ramp()) + (slope==0);
	sint8 max_height = start_height + (slope ? 0 : (1+besch->has_double_ramp()));

	// when a bridge starts on an elevated way, only downwards ramps are allowed
	if(  !gr2->ist_karten_boden()  ) {
		// cannot start on a sloped elevated way
		if(  slope  ) {
			error_msg = "A bridge must start on a way!";
			return koord3d::invalid;
		}
		// must be at least an elevated way for starting, no bridge or tunnel
		if(  gr2->get_typ() != grund_t::monorailboden  ) {
			error_msg = "A bridge must start on a way!";
			return koord3d::invalid;
		}
		max_height = start_height;
		min_height = max_height - (1+besch->has_double_ramp());
	}

	if(  hang_t::max_diff(slope)==2  &&  !besch->has_double_start()  ) {
		error_msg = "Cannot build on a double slope!";
		return koord3d::invalid;
	}

	scenario_t *scen = welt->get_scenario();
	waytype_t wegtyp = besch->get_waytype();
	error_msg = NULL;
	uint16 length = 0;

	do {
		length ++;
		pos = pos + zv;

		// test scenario conditions
		if(  (error_msg = scen->is_work_allowed_here(sp, WKZ_BRUECKENBAU|GENERAL_TOOL, wegtyp, pos)) != NULL  ) {
			// fail silent?
			return koord3d::invalid;
		}

		// test max length
		if(  besch->get_max_length() > 0  &&  length > besch->get_max_length()  ) {
			error_msg = "Bridge is too long for this type!\n";
			return koord3d::invalid;
		}

		if(  !welt->is_within_limits(pos.get_2d())  ) {
			error_msg = "Bridge is too long for this type!\n";
			return koord3d::invalid;
		}

		// now so some special checks for grounds
		grund_t *gr = welt->lookup_kartenboden( pos.get_2d() );
		if(  !gr  ) {
			// end of map!
			break;
		}

		if(  gr->get_hoehe() == max_height-1  &&  besch->get_waytype() != powerline_wt  &&  gr->get_leitung()  ) {
			error_msg = "Tile not empty.";
			return koord3d::invalid;
		}

		// check for height
		if(  besch->get_max_height()  &&  max_height-gr->get_hoehe() > besch->get_max_height()  ) {
			error_msg = "bridge is too high for its type!";
			return koord3d::invalid;
		}

		if(  gr->hat_weg(air_wt)  &&  gr->get_styp(air_wt)==1  ) {
			// sytem_type==1 is runway
			error_msg = "No bridges over runways!";
			return koord3d::invalid;
		}

		const hang_t::typ end_slope = gr->get_weg_hang();
		const sint16 hang_height = gr->get_hoehe()+hang_t::max_diff(end_slope);

		// first check for elevated ground exactly in our height
		if(  grund_t *gr2 = welt->lookup( koord3d(pos.get_2d(),max_height) )  ) {
			if(  gr2->get_typ() == grund_t::monorailboden  ) {
				// now check if our way
				if(  weg_t *w = gr2->get_weg_nr(0)  ) {
					if(  !spieler_t::check_owner(w->get_besitzer(),sp)  ) {
						// not our way
						error_msg = "Das Feld gehoert\neinem anderen Spieler\n";
						return koord3d::invalid;
					}
					if(  w->get_waytype() == besch->get_waytype()  ) {
						// ok, all fine
						return gr2->get_pos();
					}
				}
			}
			else if(  gr2->get_typ() != grund_t::boden  ) {
				// not through bridges
				break;
			}
		}

		// now check for end of bridge conditions
		if(  length >= min_length  &&  hang_height <= max_height  &&  hang_height >= min_height  &&  hang_t::ist_wegbar(end_slope)  &&  gr->get_typ()==grund_t::boden  ) {

			if(  end_slope == hang_t::flach  ) {

				/* now we have a flat tile below */
				error_msg = check_tile( gr, sp, besch->get_waytype(), ribi_typ(zv) );

				if(  hang_height < max_height  &&  (  gr->has_two_ways()  ||  (  gr->get_weg_nr(0)  &&  (gr->get_weg_nr(0)->get_waytype() != besch->get_waytype()  ||  gr->get_weg_ribi_unmasked(besch->get_waytype())!=ribi_typ(zv)  )  )  )  ) {
					// no crossing or curve here (since it will a slope ramp)
					error_msg = "A bridge must start on a way!";
				}

				if(  !error_msg  ) {
					// success
					return gr->get_pos();
				}

				if(  *error_msg  ) {
					// real error: next tile or stop when in the way
					if(  hang_height >= max_height  ) {
						// this is an real error
						return koord3d::invalid;
					}
				}
				// from here no real error (only "")
				else if(  hang_height == max_height  ||  (ai_bridge  ||  min_length)  ) {
					// in the way, or find shortest and empty => ok
					return gr->get_pos();
				}
			}
			else if(  hang_height == max_height  ) {

				// here is a slope that ends at the bridge height
				if(  hang_t::max_diff(end_slope)==2   &&   !besch->has_double_start()  ) {
					// cannot end on a double slope if we do not have the matching ramp
					error_msg = "Cannot build on a double slope!";
					return koord3d::invalid;
				}

				// first check slope
				if(  ribi_typ(end_slope) == ribi_typ(zv)  ) {
					// slope matches
					error_msg = check_tile( gr, sp, besch->get_waytype(), ribi_typ(zv) );
					if(  !error_msg  ||  !*error_msg  ) {
						// success
						return gr->get_pos();
					}
					else {
						// this is an real error
						return koord3d::invalid;
					}
				}
				// not matching => then we have to stop here!
				break;
			}
			// slope, which ends too low => we can continue
		}

		// something in the way ...
		if(  hang_height > max_height  ) {
			error_msg = "Cannot connect to the\ncenter of a double slope!";
			return koord3d::invalid;
		}

		// sorry, this is in the way
		if(  hang_height == max_height  ) {
			break;
		}

		// check that we do not cross a too low way
		if(  welt->get_settings().get_way_height_clearance() == 2  &&  gr->get_hoehe() == max_height-1  ) {
			if(  weg_t *w = gr->get_weg_nr(0)  ) {
				// now check if this is not a fence or a river (max_speed == 0)
				if(  w->get_max_speed() > 0  ) {
					error_msg = "Not enough clearance.";
					return koord3d::invalid;
				}
			}
		}

	} while(  !ai_bridge  ||  length <= welt->get_settings().way_max_bridge_len  ); // not too long in case of AI

	error_msg = "A bridge must start on a way!";
	return koord3d::invalid;
}


bool brueckenbauer_t::is_start_of_bridge( const grund_t *gr )
{
	if(  gr->ist_bruecke()  ) {
		if(  gr->ist_karten_boden()  ) {
			// ramp => true
			return true;
		}
		// now check for end of rampless bridges
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked( gr->get_weg_nr(0)->get_waytype() );
		for(  int i=0;  i<4;  i++  ) {
			if(  ribi_t::nsow[i] & ribi  ) {
				grund_t *to = welt->lookup( gr->get_pos()+koord(ribi_t::nsow[i]) );
				if(  to  &&  !to->ist_bruecke()  ) {
					return true;
				}
			}
		}
	}
	return false;
}


bool brueckenbauer_t::ist_ende_ok(spieler_t *sp, const grund_t *gr, waytype_t wt, ribi_t::ribi r )
{
	// bridges can only start or end above ground
	if(  gr->get_typ()!=grund_t::boden  &&  gr->get_typ()!=grund_t::monorailboden  ) {
		return false;
	}
	const char *error_msg = check_tile( gr, sp, wt, r );
	return (error_msg == NULL  ||  *error_msg == 0  );
}


const char *brueckenbauer_t::baue( spieler_t *sp, const koord3d pos, const bruecke_besch_t *besch)
{
	const grund_t *gr = welt->lookup(pos);
	if(  !(gr  &&  besch)  ) {
		return "";
	}

	DBG_MESSAGE("brueckenbauer_t::baue()", "called on %d,%d for bridge type '%s'", pos.x, pos.y, besch->get_name());

	koord zv;
	ribi_t::ribi ribi = ribi_t::keine;
	const weg_t *weg = gr->get_weg(besch->get_waytype());
	leitung_t *lt = gr->find<leitung_t>();

	if(  besch->get_waytype()==powerline_wt  ) {
		if(  gr->hat_wege()  ) {
			return "Tile not empty.";
		}
		if(lt) {
			ribi = lt->get_ribi();
		}
	}
	else {
		if(  lt  ) {
			return "Tile not empty.";
		}
		if(  weg  ) {
			ribi = weg->get_ribi_unmasked();
		}
	}

	if(  (!lt  &&  !weg)  ||  !ist_ende_ok(sp, gr,besch->get_waytype(),ribi)  ) {
		DBG_MESSAGE( "brueckenbauer_t::baue()", "no way %x found", besch->get_waytype() );
		return "A bridge must start on a way!";
	}

	if(  gr->kann_alle_obj_entfernen(sp)  ) {
		return "Tile not empty.";
	}

	if(gr->get_grund_hang() == hang_t::flach) {
		if(!ribi_t::ist_einfach(ribi)) {
			ribi = 0;
		}
	}
	else {
		ribi_t::ribi hang_ribi = ribi_typ(gr->get_grund_hang());
		if(ribi & ~hang_ribi) {
			ribi = 0;
		}
	}

	if(!ribi) {
		return "A bridge must start on a way!";
	}

	zv = koord(ribi_t::rueckwaerts(ribi));
	// search for suitable bridge end tile
	const char *msg;
	koord3d end = finde_ende(sp, gr->get_pos(), zv, besch, msg );

	// found something?
	if(  koord3d::invalid == end  ) {
DBG_MESSAGE("brueckenbauer_t::baue()", "end not ok");
		return msg;
	}

	// check ownership
	grund_t * gr_end = welt->lookup(end);
	if(gr_end->kann_alle_obj_entfernen(sp)) {
		return "Tile not empty.";
	}
	// check way ownership
	if(gr_end->hat_wege()) {
		if(gr_end->get_weg_nr(0)->ist_entfernbar(sp)!=NULL) {
			return "Tile not empty.";
		}
		if(gr_end->has_two_ways()  &&  gr_end->get_weg_nr(1)->ist_entfernbar(sp)!=NULL) {
			return "Tile not empty.";
		}
	}


	// Start and end have been checked, we can start to build eventually
	if(besch->get_waytype()==powerline_wt) {
		baue_bruecke(sp, gr->get_pos(), end, zv, besch, lt->get_besch() );
	}
	else {
		baue_bruecke(sp, gr->get_pos(), end, zv, besch, weg->get_besch() );
	}
	return NULL;
}


void brueckenbauer_t::baue_bruecke(spieler_t *sp, const koord3d start, const koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	ribi_t::ribi ribi = ribi_typ(zv);

	DBG_MESSAGE("brueckenbauer_t::baue()", "build from %s", start.get_str() );

	grund_t *start_gr = welt->lookup( start );
	const hang_t::typ slope = start_gr->get_grund_hang();

	// get initial height of bridge from start tile
	uint8 add_height = 0;

	if(  start_gr->ist_karten_boden()  ) {
		// needs a ramp to start on ground
		add_height = slope ?  hang_t::max_diff(slope) : (1+besch->has_double_ramp());
		baue_auffahrt( sp, start, ribi, slope?0:hang_typ(zv)*add_height, besch );
	}
	if(  besch->get_waytype() != powerline_wt  ) {
		ribi = welt->lookup(start)->get_weg_ribi_unmasked(besch->get_waytype());
	}

	koord3d pos = start+koord3d( zv.x, zv.y, add_height );
	while(  pos.get_2d() != end.get_2d()  ) {
		brueckenboden_t *bruecke = new brueckenboden_t( pos, 0, 0 );
		welt->access(pos.get_2d())->boden_hinzufuegen(bruecke);
		if(besch->get_waytype() != powerline_wt) {
			weg_t * const weg = weg_t::alloc(besch->get_waytype());
			weg->set_besch(weg_besch);
			bruecke->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		}
		else {
			leitung_t *lt = new leitung_t(bruecke->get_pos(), sp);
			bruecke->obj_add( lt );
			lt->laden_abschliessen();
		}
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - gr->get_pos().z;
		bruecke_t *br = new bruecke_t(bruecke->get_pos(), sp, besch, besch->get_simple(ribi,height-hang_t::max_diff(gr->get_grund_hang())));
		bruecke->obj_add(br);
		bruecke->calc_bild();
		br->laden_abschliessen();
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","at (%i,%i)",pos.x,pos.y);
		if(besch->get_pillar()>0) {
			// make a new pillar here
			if(besch->get_pillar()==1  ||  (pos.x*zv.x+pos.y*zv.y)%besch->get_pillar()==0) {
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","h1=%i, h2=%i",pos.z,gr->get_pos().z);
				while(height-->0) {
					if( TILE_HEIGHT_STEP*height <= 127) {
						// eventual more than one part needed, if it is too high ...
						gr->obj_add( new pillar_t(gr->get_pos(),sp,besch,besch->get_pillar(ribi), TILE_HEIGHT_STEP*height) );
					}
				}
			}
		}

		pos = pos + zv;
	}

	// end tile height depends on whether slope matches direction...
	hang_t::typ end_slope = welt->lookup(end)->get_grund_hang();
	sint8 end_slope_height = end.z;
	if(  end_slope != hang_typ(zv) && end_slope != hang_typ(zv)*2  ) {
		end_slope_height += hang_t::max_diff(end_slope);
	}

	// must determine end tile: on a slope => likely need auffahrt
	bool need_auffahrt = pos.z != end_slope_height;
	if(  need_auffahrt  ) {
		if(  weg_t const* const w = welt->lookup(end)->get_weg( weg_besch->get_wtyp() )  ) {
			need_auffahrt &= w->get_besch()->get_styp() != weg_besch_t::elevated;
		}
	}

	grund_t *gr = welt->lookup(end);
	if(  need_auffahrt  ) {
		// not ending at a bridge
		baue_auffahrt(sp, end, ribi_typ(-zv), gr->get_grund_hang()?0:hang_typ(-zv)*(pos.z-end.z), besch);
	}
	else {
		// ending on a slope/elevated way
		if(besch->get_waytype() != powerline_wt) {
			// just connect to existing way
			ribi = ribi_t::rueckwaerts( ribi_typ(zv) );
			if(  !gr->weg_erweitern( besch->get_waytype(), ribi )  ) {
				// builds new way
				weg_t * const weg = weg_t::alloc( besch->get_waytype() );
				weg->set_besch( weg_besch );
				spieler_t::book_construction_costs( sp, -gr->neuen_weg_bauen( weg, ribi, sp ) -weg->get_besch()->get_preis(), end.get_2d(), weg->get_waytype());
			}
			gr->calc_bild();
		}
		else {
			leitung_t *lt = gr->get_leitung();
			if(  lt==NULL  ) {
				lt = new leitung_t(end, sp );
				spieler_t::book_construction_costs(sp, -weg_besch->get_preis(), gr->get_pos().get_2d(), powerline_wt);
				gr->obj_add(lt);
				lt->set_besch(weg_besch);
				lt->laden_abschliessen();
			}
			lt->calc_neighbourhood();
		}
	}

	// if start or end are single way, and next tile is not, try to connect
	if(  besch->get_waytype() != powerline_wt  &&  sp  ) {
		// we need to check start_gr again, because a ramp deletes the old ground
		if(  (start_gr = welt->lookup( start ))  ) {
			if(  !start_gr->ist_karten_boden()  ) {
				// elevated way, just add ribi
				start_gr->weg_erweitern( besch->get_waytype(), ribi_t::rueckwaerts( ribi ) );
				start_gr->calc_bild();
			}
			else {
				ribi_t::ribi start_ribi = start_gr->get_weg_ribi_unmasked( besch->get_waytype() );
				if(  ribi_t::ist_einfach(start_ribi)  ) {
					// only single tile under start => try to connect to next tile
					koord3d next_to_start = koord3d( start.get_2d()-koord(start_ribi), start_gr->get_vmove( ribi_t::rueckwaerts(start_ribi) ) );
					wegbauer_t bauigel(sp);
					bauigel.set_keep_existing_ways(true);
					bauigel.set_keep_city_roads(true);
					bauigel.set_maximum(2);
					bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat ), NULL, NULL );
					bauigel.calc_route( start, next_to_start );
					if(  bauigel.get_count() > 1  ) {
						bauigel.baue();
					}
				}
			}
		}
		if(  grund_t *end_gr = welt->lookup(end)  ) {
			ribi_t::ribi ribi = end_gr->get_weg_ribi_unmasked(besch->get_waytype());
			if(  ribi_t::ist_einfach(ribi)  ) {
				// only single tile under start => try to connect to next tile
				koord3d next_to_end = koord3d( end.get_2d()-koord(ribi), end_gr->get_vmove( ribi_t::rueckwaerts(ribi) ) );
				wegbauer_t bauigel(sp);
				bauigel.set_keep_existing_ways(true);
				bauigel.set_keep_city_roads(true);
				bauigel.set_maximum(2);
				bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat ), NULL, NULL );
				bauigel.calc_route( end, next_to_end );
				if(  bauigel.get_count() > 1  ) {
					bauigel.baue();
				}
			}
		}
	}
}


void brueckenbauer_t::baue_auffahrt(spieler_t* sp, koord3d end, ribi_t::ribi ribi_neu, hang_t::typ weg_hang, const bruecke_besch_t* besch)
{
	grund_t *alter_boden = welt->lookup(end);
	brueckenboden_t *bruecke;
	hang_t::typ grund_hang = alter_boden->get_grund_hang();
	bruecke_besch_t::img_t img;

	bruecke = new brueckenboden_t(end, grund_hang, weg_hang);
	// add the ramp
	img = besch->get_end( bruecke->get_grund_hang(), grund_hang, weg_hang );

	weg_t *weg = NULL;
	if(besch->get_waytype() != powerline_wt) {
		weg = alter_boden->get_weg( besch->get_waytype() );
	}
	// take care of everything on that tile ...
	bruecke->take_obj_from( alter_boden );
	welt->access(end.get_2d())->kartenboden_setzen( bruecke );
	if(besch->get_waytype() != powerline_wt) {
		if(  !bruecke->weg_erweitern( besch->get_waytype(), ribi_neu)  ) {
			// needs still one
			weg = weg_t::alloc( besch->get_waytype() );
			spieler_t::book_construction_costs(sp, -bruecke->neuen_weg_bauen( weg, ribi_neu, sp ), end.get_2d(), besch->get_waytype());
		}
		weg->set_max_speed( besch->get_topspeed() );
	}
	else {
		leitung_t *lt = bruecke->get_leitung();
		if(!lt) {
			lt = new leitung_t(bruecke->get_pos(), sp);
			bruecke->obj_add( lt );
		}
		else {
			// remove maintenance - it will be added in leitung_t::laden_abschliessen
			spieler_t::add_maintenance( sp, -lt->get_besch()->get_wartung(), powerline_wt);
		}
		// connect to neighbor tiles and networks, add maintenance
		lt->laden_abschliessen();
	}
	bruecke_t *br = new bruecke_t(end, sp, besch, img);
	bruecke->obj_add( br );
	br->laden_abschliessen();
	bruecke->calc_bild();
}


const char *brueckenbauer_t::remove(spieler_t *sp, koord3d pos, waytype_t wegtyp)
{
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);
	slist_tpl<koord3d> end_list;
	slist_tpl<koord3d> part_list;
	slist_tpl<koord3d> tmp_list;
	const char    *msg;

	tmp_list.insert(pos);
	marker.mark(welt->lookup(pos));
	waytype_t delete_wegtyp = wegtyp==powerline_wt ? invalid_wt : wegtyp;

	do {
		pos = tmp_list.remove_first();

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(from->ist_karten_boden()) {
			// gr is start/end - test only one direction
			if(from->get_grund_hang() != hang_t::flach) {
				zv = -koord(from->get_grund_hang());
			}
			else {
				zv = koord(from->get_weg_hang());
			}
			end_list.insert(pos);
		}
		else if(from->ist_bruecke()) {
			part_list.insert(pos);
		}
		// can we delete everything there?
		msg = from->kann_alle_obj_entfernen(sp);

		if(msg != NULL  ||  (from->get_halt().is_bound()  &&  from->get_halt()->get_besitzer()!=sp)) {
			return "Die Bruecke ist nicht frei!\n";
		}

		// search neighbors
		for(int r = 0; r < 4; r++) {
			if(  (zv == koord::invalid  ||  zv == koord::nsow[r])  &&  from->get_neighbour(to, delete_wegtyp, ribi_t::nsow[r])  &&  !marker.is_marked(to)  &&  to->ist_bruecke()  ) {
				if(  wegtyp != powerline_wt  ||  (to->find<bruecke_t>()  &&  to->find<bruecke_t>()->get_besch()->get_waytype() == powerline_wt)  ) {
					tmp_list.insert(to->get_pos());
					marker.mark(to);
				}
			}
		}
	} while (!tmp_list.empty());

	// now delete the bridge
	bool first = true;
	while (!part_list.empty()) {
		pos = part_list.remove_first();

		grund_t *gr = welt->lookup(pos);

		// the following code will check if this is the first of last tile, end then tries to remove the superflous ribis from it
		if(  first  ||  end_list.empty()  ) {
			// starts on slope or elevated way, or it consist only of the ramp
			ribi_t::ribi bridge_ribi = gr->get_weg_ribi_unmasked( wegtyp );
			for(  uint i = 0;  i < 4;  i++  ) {
				if(  bridge_ribi & ribi_t::nsow[i]  ) {
					grund_t *prev;
					// if we have a ramp, then only check the higher end!
					if(  gr->get_neighbour( prev, wegtyp, ribi_t::nsow[i])  &&  !prev->ist_bruecke()  &&  !end_list.is_contained(prev->get_pos())   ) {
						if(  weg_t *w = prev->get_weg( wegtyp )  ) {
							// now remove ribi (or full way)
							w->set_ribi( (~ribi_t::rueckwaerts( ribi_t::nsow[i] )) & w->get_ribi_unmasked() );
							if(  w->get_ribi_unmasked() == 0  ) {
								// nowthing left => then remove completel
								prev->remove_everything_from_way( sp, wegtyp, bridge_ribi );	// removes stop and signals correctly
								prev->weg_entfernen( wegtyp, true );
								if(  prev->get_typ() == grund_t::monorailboden  ) {
									welt->access( prev->get_pos().get_2d() )->boden_entfernen( prev );
									delete prev;
								}
							}
							else {
								w->calc_bild();
							}
						}
					}
				}
			}
		}
		first = false;

		// first: remove bridge
		bruecke_t *br = gr->find<bruecke_t>();
		br->entferne(sp);
		delete br;

		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// remove also the second way on the bridge
		if(gr->get_weg_nr(0)!=0) {
			gr->remove_everything_from_way(sp,gr->get_weg_nr(0)->get_waytype(),ribi_t::keine);
		}

		// we may have a second way/powerline here ...
		gr->obj_loesche_alle(sp);
		gr->mark_image_dirty();

		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;

		// finally delete all pillars (if there)
		gr = welt->lookup_kartenboden(pos.get_2d());
		while (obj_t* const p = gr->find<pillar_t>()) {
			p->entferne(p->get_besitzer());
			delete p;
		}
		// refresh map
		reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
	}

	// finally delete the bridge ends (all are kartenboden)
	while(  !end_list.empty()  ) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);

		// starts on slope or elevated way, or it consist only of the ramp
		ribi_t::ribi bridge_ribi = gr->get_weg_ribi_unmasked( wegtyp );
		for(  uint i = 0;  i < 4;  i++  ) {
			if(  bridge_ribi & ribi_t::nsow[i]  ) {
				grund_t *prev;
				// if we have a ramp, then only check the higher end!
				if(  gr->get_neighbour( prev, wegtyp, ribi_t::nsow[i])  &&  prev->get_hoehe() > gr->get_hoehe()  ) {
					if(  weg_t *w = prev->get_weg( wegtyp )  ) {
						// now remove ribi (or full way)
						w->set_ribi( (~ribi_t::rueckwaerts( ribi_t::nsow[i] )) & w->get_ribi_unmasked() );
						if(  w->get_ribi_unmasked() == 0  ) {
							// nowthing left => then remove completel
							prev->remove_everything_from_way( sp, wegtyp, bridge_ribi );	// removes stop and signals correctly
							prev->weg_entfernen( wegtyp, true );
							if(  prev->get_typ() == grund_t::monorailboden  ) {
								welt->access( prev->get_pos().get_2d() )->boden_entfernen( prev );
								delete prev;
							}
						}
						else {
							w->calc_bild();
						}
					}
				}
			}
		}

		if(wegtyp==powerline_wt) {
			while (obj_t* const br = gr->find<bruecke_t>()) {
				br->entferne(sp);
				delete br;
			}
			leitung_t *lt = gr->get_leitung();
			if (lt) {
				spieler_t *old_owner = lt->get_besitzer();
				// first delete powerline to decouple from the bridge powernet
				lt->entferne(old_owner);
				delete lt;
				// .. now create powerline to create new powernet
				lt = new leitung_t(gr->get_pos(), old_owner);
				lt->laden_abschliessen();
				gr->obj_add(lt);
			}
		}
		else {
			ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp);
			ribi_t::ribi bridge_ribi;

			if(gr->get_grund_hang() != hang_t::flach) {
				bridge_ribi = ~ribi_typ(hang_t::gegenueber(gr->get_grund_hang()));
			}
			else {
				bridge_ribi = ~ribi_typ(gr->get_weg_hang());
			}
			ribi &= bridge_ribi;

			bruecke_t *br = gr->find<bruecke_t>();
			br->entferne(sp);
			delete br;

			// stops on flag bridges ends with road + track on it
			// are not correctly deleted if ways are kept
			if (gr->is_halt()) {
				haltestelle_t::remove(sp, gr->get_pos());
			}

			// depots at bridge ends needs to be deleted as well
			if (depot_t *dep = gr->get_depot()) {
				dep->entferne(sp);
				delete dep;
			}

			// removes single signals, bridge head, pedestrians, stops, changes catenary etc
			weg_t *weg=gr->get_weg_nr(1);
			if(weg) {
				gr->remove_everything_from_way(sp,weg->get_waytype(),bridge_ribi);
			}
			gr->remove_everything_from_way(sp,wegtyp,bridge_ribi);	// removes stop and signals correctly

			// corrects the ways
			weg = gr->get_weg(wegtyp);
			if(  weg  ) {
				// needs checks, since this fails if it was the last tile
				weg->set_besch( weg->get_besch() );
				weg->set_ribi( ribi );
				if(  hang_t::max_diff(gr->get_grund_hang())>=2  &&  !weg->get_besch()->has_double_slopes()  ) {
					// remove the way totally, if is is on a double slope
					gr->weg_entfernen( weg->get_waytype(), true );
				}
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(pos, gr->get_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.get_2d())->kartenboden_setzen( gr_new );

		if(  wegtyp == powerline_wt  ) {
			gr_new->get_leitung()->calc_neighbourhood(); // Recalc the image. calc_bild() doesn't do the right job...
		}
	}

	welt->set_dirty();
	return NULL;
}
