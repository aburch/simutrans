/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../simdebug.h"
#include "../simtool.h"
#include "brueckenbauer.h"
#include "wegbauer.h"

#include "../simworld.h"
#include "../simhalt.h"
#include "../simdepot.h"
#include "../player/simplay.h"
#include "../simtypes.h"

#include "../boden/boden.h"
#include "../boden/brueckenboden.h"

#include "../gui/messagebox.h"
#include "../gui/tool_selector.h"
#include "../gui/karte.h"

#include "../besch/bruecke_besch.h"
#include "../besch/haus_besch.h"

#include "../dataobj/marker.h"
#include "../dataobj/scenario.h"
#include "../dataobj/crossing_logic.h"

#include "../obj/bruecke.h"
#include "../obj/leitung2.h"
#include "../obj/pillar.h"
#include "../obj/signal.h"
#include "../obj/wayobj.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"

karte_ptr_t brueckenbauer_t::welt;


/// All bridges hashed by name
static stringhashtable_tpl<bruecke_besch_t *> bruecken_by_name;


void brueckenbauer_t::register_besch(bruecke_besch_t *besch)
{
	// avoid duplicates with same name
	if( const bruecke_besch_t *old_besch = bruecken_by_name.get(besch->get_name()) ) {
		dbg->warning( "brueckenbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		bruecken_by_name.remove(besch->get_name());
		tool_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}
	// add the tool
	tool_build_bridge_t *tool = new tool_build_bridge_t();
	tool->set_icon( besch->get_cursor()->get_bild_nr(1) );
	tool->cursor = besch->get_cursor()->get_bild_nr(0);
	tool->set_default_param(besch->get_name());
	tool_t::general_tool.append( tool );
	besch->set_builder( tool );
	bruecken_by_name.put(besch->get_name(), besch);
}


const bruecke_besch_t *brueckenbauer_t::get_besch(const char *name)
{
	return (name ? bruecken_by_name.get(name) : NULL);
}


// "successfully load" (Babelfish)
bool brueckenbauer_t::laden_erfolgreich()
{
	bool strasse_da = false;
	bool schiene_da = false;

	FOR(stringhashtable_tpl<bruecke_besch_t*>, const& i, bruecken_by_name) {
		bruecke_besch_t const* const besch = i.value;

		if(besch && besch->get_waytype() == track_wt) {
			schiene_da = true;
		}
		if(besch && besch->get_waytype() == road_wt) {
			strasse_da = true;
		}
	}

	if(!schiene_da) {
		DBG_MESSAGE("brueckenbauer_t", "No rail bridge found - feature disabled");
	}

	if(!strasse_da) {
		DBG_MESSAGE("brueckenbauer_t", "No road bridge found - feature disabled");
	}

	return true;
}


stringhashtable_tpl<bruecke_besch_t *> * brueckenbauer_t::get_all_bridges() 
{ 
	return &bruecken_by_name; 
}

/**
 * Find a matching bridge
 * @author Hj. Malthaner
 */
const bruecke_besch_t *brueckenbauer_t::find_bridge(const waytype_t wtyp, const sint32 min_speed, const uint16 time, const uint16 max_weight)
{
	const bruecke_besch_t *find_besch=NULL;

	FOR(stringhashtable_tpl<bruecke_besch_t*>, const& i, bruecken_by_name) {
		bruecke_besch_t const* const besch = i.value;
		if(  besch->get_waytype()==wtyp  &&  besch->is_available(time)  ) {
			if(find_besch==NULL  ||
				((find_besch->get_topspeed()<min_speed  &&  find_besch->get_topspeed()<besch->get_topspeed())  &&
				(find_besch->get_max_weight()<max_weight  &&  find_besch->get_topspeed()<besch->get_max_weight()))  ||
				(besch->get_topspeed()>=min_speed && besch->get_max_weight()>=max_weight  &&  (besch->get_wartung()<find_besch->get_wartung() ||
				(besch->get_wartung()==find_besch->get_wartung() &&  besch->get_preis()<find_besch->get_preis())))
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
	return a->get_topspeed() < b->get_topspeed() && a->get_axle_load() < b->get_axle_load();
}


void brueckenbauer_t::fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_BRIDGE | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();
	vector_tpl<const bruecke_besch_t*> matching(bruecken_by_name.get_count());

	// list of matching types (sorted by speed)
	FOR(stringhashtable_tpl<bruecke_besch_t*>, const& i, bruecken_by_name) {
		bruecke_besch_t const* const b = i.value;
		if (  b->get_waytype()==wtyp  &&  b->is_available(time)  ) {
			matching.insert_ordered( b, compare_bridges);
		}
	}

	// now sorted ...
	FOR(vector_tpl<bruecke_besch_t const*>, const i, matching) {
		tool_selector->add_tool_selector(i->get_builder());
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
const char *check_tile( const grund_t *gr, const player_t *player, waytype_t wt, ribi_t::ribi check_ribi )
{
	static karte_ptr_t welt;
	// not overbuilt transformers
	if(  gr->find<senke_t>()!=NULL  ||  gr->find<pumpe_t>()!=NULL  ) {
		return "A bridge must start on a way!";
	}

	if(  !hang_t::ist_wegbar(gr->get_weg_hang())  ) {
		return "Bruecke muss an\neinfachem\nHang beginnen!\n";
	}

	if(  gr->is_halt()  ||  gr->get_depot() || gr->get_signalbox() ) 
	{
		// something in the way
		return "Tile not empty.";
	}

	if(  wt != powerline_wt  &&  gr->get_leitung()  ) {
		return "Tile not empty.";
	}

	// we can build a ramp when there is one (or with tram two) way in our direction and no stations/depot etc.
	if(  weg_t *w = gr->get_weg_nr(0)  ) {

		if(  w->is_deletable(player) != NULL  ) {
			// not our way
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}

		// now check for direction
		ribi_t::ribi ribi = w->get_ribi_unmasked();
		if(  weg_t *w2 = gr->get_weg_nr(1)  ) {
			if(  w2->is_deletable(player) != NULL ) {
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
			if(  (ribi_t::doppelt(ribi) ^ ribi_t::doppelt(check_ribi) ) == ribi_t::alle  &&  crossing_logic_t::get_crossing(wt, w->get_waytype(), 0, 0, welt->get_timeline_year_month()  )  ) {
				return NULL;
			}
			//return "A bridge must start on a way!";
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
			if(  lt->is_deletable(player) == NULL  &&  ribi_check( lt->get_ribi(), check_ribi )  ) {
				// matching powerline
				return NULL;
			}
			return "A bridge must start on a way!";
		}
	}
	// somethign here which we cannot remove => fail too
	if(  obj_t *obj=gr->obj_bei(0)  ) {
		if(  const char *err_msg = obj->is_deletable(player)  ) {
			return err_msg;
		}
	}
	return "";	// could end here by must not end here
}

bool brueckenbauer_t::is_blocked(koord3d pos, ribi_t::ribi check_ribi, const char *&error_msg)
{
	/* can't build directly above or below a way if height clearance == 2 */
	// take slopes on grounds below into accout
	const sint8 clearance = welt->get_settings().get_way_height_clearance()-1;
	for(int dz = -clearance -2; dz <= clearance; dz++) {
		grund_t *gr2;
		if (dz != 0 && (gr2 = welt->lookup(pos + koord3d(0,0,dz)))) {

			const hang_t::typ slope = gr2->get_weg_hang();
			if (dz < -clearance) {
				if (dz + hang_t::max_diff(slope) < -clearance ) {
					// too far below
					continue;
				}
			}

			if (dz + hang_t::max_diff(slope) == 0  &&  gr2->ist_karten_boden()  &&  ribi_typ(gr2->get_grund_hang())==check_ribi) {
				// potentially connect to this slope
				continue;
			}

			weg_t *w = gr2->get_weg_nr(0);
			if (w && w->get_max_speed() > 0) {
				error_msg = "Bridge blocked by way below or above.";
				return true;
			}
		}
	}

	// first check for elevated ground exactly in our height
	if(  grund_t *gr2 = welt->lookup( pos )  ) {
		if(  gr2->get_typ() != grund_t::boden  && gr2->get_typ() != grund_t::tunnelboden  ) {
			// not through bridges
			// monorail tiles will be checked in is_monorail_junction
			error_msg = "Tile not empty.";
			return true;
		}
	}

	if(grund_t *gr = welt->lookup_kartenboden(pos.get_2d())) 
	{
		if (const gebaeude_t* gb = gr->get_building()) 
		{
			const uint8 max_level = welt->get_settings().get_max_elevated_way_building_level();
			if( gb->get_tile()->get_besch()->get_level() > max_level && !haltestelle_t::get_halt(gb->get_pos(), NULL).is_bound())
			{
				error_msg = "Bridges cannot be built over large buildings.";
				return true;
			}
		}
	}

	return false;
}



// return true if bridge can be connected to monorail tile
bool brueckenbauer_t::is_monorail_junction(koord3d pos, player_t *player, const bruecke_besch_t *besch, const char *&error_msg)
{
	// first check for elevated ground exactly in our height
	if(  grund_t *gr2 = welt->lookup( pos )  ) {
		if(  gr2->get_typ() == grund_t::monorailboden  ) {
			// now check if our way
			if(  weg_t *w = gr2->get_weg_nr(0)  ) {
				if(  !player_t::check_owner(w->get_owner(),player)  ) {
					// not our way
					error_msg = "Das Feld gehoert\neinem anderen Spieler\n";
					return false;
				}
				if(  w->get_waytype() == besch->get_waytype()  ) {
					// ok, all fine
					return true;
				}
			}
			error_msg = "Tile not empty.";
		}
	}

	return false;
}


#define height_okay(h) (((h) < min_bridge_height || (h) > max_height) ? false : height_okay_array[h-min_bridge_height])
// 2 to suppress warnings when h=start_height+x and min_bridge_height=start_height
#define height_okay2(h) (((h) > max_height) ? false : height_okay_array[h-min_bridge_height])

koord3d brueckenbauer_t::finde_ende(player_t *player, koord3d pos, const koord zv, const bruecke_besch_t *besch, const char *&error_msg, sint8 &bridge_height, bool ai_bridge, uint32 min_length, bool high_bridge )
{
	const grund_t *const gr2 = welt->lookup( pos );
	if(  !gr2  ) {
		error_msg = "A bridge must start on a way!";
		return koord3d::invalid;
	}

	// get initial height of bridge from start tile
	// flat -> height is 1 if only shallow ramps, else height 2
	// single height -> height is 1
	// double height -> height is 2
	// if start tile is tunnel mouth then min = max = pos.z = start_height
	const hang_t::typ slope = gr2->get_weg_hang();
	const sint8 start_height = gr2->get_hoehe() + hang_t::max_diff(slope);
	sint8 min_bridge_height = start_height; /* was  + (slope==0) */
	sint8 min_height = start_height - (1+besch->has_double_ramp()) + (slope==0);
	sint8 max_height = start_height + (slope || gr2->ist_tunnel() ? 0 : (1+besch->has_double_ramp()));

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
		min_bridge_height = start_height;
	}
	bool height_okay_array[256];
	for (int i = 0; i < max_height+1 - min_bridge_height; i++) {
		height_okay_array[i] = true;
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
		if(  (error_msg = scen->is_work_allowed_here(player, TOOL_BUILD_BRIDGE|GENERAL_TOOL, wegtyp, pos)) != NULL  ) {
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

		bool abort = true;
		for(sint8 z = min_bridge_height; z <= max_height; z++) {
			if(height_okay(z)) {
				if(is_blocked(koord3d(pos.get_2d(), z), ribi_typ(zv), error_msg)) {
					height_okay_array[z-min_bridge_height] = false;

					// connect to suitable monorail tiles if possible
					if(is_monorail_junction(koord3d(pos.get_2d(), z), player, besch, error_msg)) {
						gr = welt->lookup(koord3d(pos.get_2d(), z));
						bridge_height = z - start_height;
						return gr->get_pos();
					}
				}
				else {
					abort = false;
				}
			}
		}
		if(abort) {
			return koord3d::invalid;
		}

		const hang_t::typ end_slope = gr->get_weg_hang();
		const sint16 hang_height = gr->get_hoehe()+hang_t::max_diff(end_slope);

		if(hang_height > max_height) {
			error_msg = "Cannot connect to the\ncenter of a double slope!";
			return koord3d::invalid;
		}
		if(hang_height < min_height) {
			continue;
		}
		// now check for end of bridge conditions
		if(length >= min_length && hang_t::ist_wegbar(end_slope) &&
		   (gr->get_typ()==grund_t::boden || gr->get_typ()==grund_t::tunnelboden)) {
			bool finish = false;
			if(height_okay(hang_height) && end_slope == hang_t::flach &&
			   (hang_height == max_height || ai_bridge || min_length)) {
				/* now we have a flat tile below */
				error_msg = check_tile( gr, player, besch->get_waytype(), ribi_typ(zv) );

				if(  !error_msg  ||  (!*error_msg) ) {
					// success
					for(sint8 z = hang_height + 3; z <= max_height; z++) {
						height_okay_array[z-min_bridge_height] = false;
					}
					for(sint8 z = min_bridge_height; z < hang_height; z++) {
						height_okay_array[z-min_bridge_height] = false;
					}
					finish = true;
				}
			}
			else if(  height_okay(hang_height) &&
				    (hang_height == max_height || ai_bridge || min_length) ) {
				// here is a slope that ends at the bridge height
				if(  hang_t::max_diff(end_slope)==2   &&   !besch->has_double_start()  ) {
					// cannot end on a double slope if we do not have the matching ramp
					error_msg = "Cannot build on a double slope!";
				}
				else {
					// first check slope
					if(  ribi_typ(end_slope) == ribi_typ(zv)  ) {
						// slope matches
						error_msg = check_tile( gr, player, besch->get_waytype(), ribi_typ(zv) );
						if(  !error_msg  ||  !*error_msg  ) {
							// success
							bridge_height = hang_height - start_height;

							return gr->get_pos();
						}
					}
				}
			}
			else if (end_slope == hang_t::flach) {
				if ((hang_height+1 >= start_height && height_okay(hang_height+1)) ||
				    (hang_height+2 >= start_height && height_okay(hang_height+2))) {
					/* now we have a flat tile below */
					error_msg = check_tile( gr, player, besch->get_waytype(), ribi_typ(zv) );

					if(  hang_height < max_height  &&  (  gr->has_two_ways()  ||  (  gr->get_weg_nr(0)  &&  (gr->get_weg_nr(0)->get_waytype() != besch->get_waytype()  ||  gr->get_weg_ribi_unmasked(besch->get_waytype())!=ribi_typ(zv)  )  )  )  ) {
						// no crossing or curve here (since it will a slope ramp)
						error_msg = "A bridge must start on a way!";
					}
					if (gr->ist_tunnel()) {
						// non-flat bridges should not end in tunnels
						error_msg = "Tile not empty.";
					}
					if(  !error_msg  ) {
						for(sint8 z = hang_height + 3; z <= max_height; z++) {
							height_okay_array[z-min_bridge_height] = false;
						}
						finish = true;
					}
					else if(  *error_msg == 0 ) {
						if(  (ai_bridge  ||  min_length)  ) {
							for(sint8 z = hang_height + 3; z <= max_height; z++) {
								height_okay_array[z-min_bridge_height] = false;
							}
							// in the way, or find shortest and empty => ok
							finish = true;
						}
					}

				}
			}

			if(  finish  ) {
				if(  high_bridge  ) {
					if(  height_okay2( start_height + 2 )  ) {
						bridge_height = 2;
					}
					else if(  height_okay2( start_height + 1 )  ) {
						bridge_height = 1;
					}
					else if(  height_okay2( start_height )  ) {
						bridge_height = 0;
					}
					else {
						assert(false);
					}
				}
				else {
					if(  height_okay2( start_height )  ) {
						bridge_height = 0;
					}
					else if(  height_okay2( start_height + 1 )  ) {
						bridge_height = 1;
					}
					else if(  height_okay2( start_height + 2 )  ) {
						bridge_height = 2;
					}
					else {
						assert(false);
					}
				}
				// there is no point in building flat, one-tile bridges
				if (bridge_height == 0  &&  length == 1) {
					error_msg = "";
					return koord3d::invalid;
				}
				return gr->get_pos();
			}
			// slope, which ends too low => we can continue
		}

		const hang_t::typ end_ground_slope = gr->get_grund_hang();
		const sint16 hang_ground_height = gr->get_hoehe()+hang_t::max_diff(end_ground_slope);
		// sorry, this is in the way
		if(  hang_ground_height >= max_height  ) {
			break;
		}

		for(sint8 z = min_bridge_height; z <= hang_ground_height; z++) {
			if(height_okay(z)) {
				height_okay_array[z-min_bridge_height] = false;
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
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked( gr->get_leitung() ? powerline_wt : gr->get_weg_nr(0)->get_waytype() );
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


bool brueckenbauer_t::ist_ende_ok(player_t *player, const grund_t *gr, waytype_t wt, ribi_t::ribi r )
{
	// bridges can only start or end above ground
	if(  gr->get_typ()!=grund_t::boden  &&  gr->get_typ()!=grund_t::monorailboden  &&
	     gr->get_typ()!=grund_t::tunnelboden  ) {
		return false;
	}
	const char *error_msg = check_tile( gr, player, wt, r );
	return (error_msg == NULL  ||  *error_msg == 0  );
}


const char *brueckenbauer_t::baue( player_t *player, const koord3d pos, const bruecke_besch_t *besch)
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

	if(  !ist_ende_ok(player, gr,besch->get_waytype(),ribi)  ) {
		DBG_MESSAGE( "brueckenbauer_t::baue()", "no way %x found", besch->get_waytype() );
		return "A bridge must start on a way!";
	}

	if(  gr->kann_alle_obj_entfernen(player)  ) {
		return "Tile not empty.";
	}

	if(gr->get_weg_hang() == hang_t::flach) {
		if(!ribi_t::ist_einfach(ribi)) {
			ribi = 0;
		}
	}
	else {
		ribi_t::ribi hang_ribi = ribi_typ(gr->get_weg_hang());
		if(ribi & ~hang_ribi) {
			ribi = 0;
		}
		else {
			// take direction from slope of tile
			ribi = hang_ribi;
		}
	}

	if(!ribi) {
		return "A bridge must start on a way!";
	}

	zv = koord(ribi_t::rueckwaerts(ribi));
	// search for suitable bridge end tile
	const char *msg;
	sint8 bridge_height;
	koord3d end = finde_ende(player, gr->get_pos(), zv, besch, msg, bridge_height );

	// found something?
	if(  koord3d::invalid == end  ) {
DBG_MESSAGE("brueckenbauer_t::baue()", "end not ok");
		return msg;
	}

	// check ownership
	grund_t * gr_end = welt->lookup(end);
	if(gr_end->kann_alle_obj_entfernen(player)) {
		return "Tile not empty.";
	}
	// check way ownership
	if(gr_end->hat_wege()) {
		if(gr_end->get_weg_nr(0)->is_deletable(player)!=NULL) {
			return "Tile not empty.";
		}
		if(gr_end->has_two_ways()  &&  gr_end->get_weg_nr(1)->is_deletable(player)!=NULL) {
			return "Tile not empty.";
		}
	}

	// associated way
	const weg_besch_t* way_besch;
	if (weg) {
		way_besch = weg->get_besch();
	}
	else if (lt) {
		way_besch = lt->get_besch();
	}
	else {
		way_besch = wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat);
	}

	// Start and end have been checked, we can start to build eventually
	baue_bruecke(player, gr->get_pos(), end, zv, bridge_height, besch, way_besch );

	if(besch->get_waytype() == road_wt)
	{
		welt->set_recheck_road_connexions();
	}
	return NULL;
}


void brueckenbauer_t::baue_bruecke(player_t *player, const koord3d start, const koord3d end, koord zv, sint8 bridge_height, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	ribi_t::ribi ribi = ribi_typ(zv);

	DBG_MESSAGE("brueckenbauer_t::baue()", "build from %s", start.get_str() );

	grund_t *start_gr = welt->lookup( start );
	const hang_t::typ slope = start_gr->get_weg_hang();

	// get initial height of bridge from start tile
	uint8 add_height = 0;

	// end tile height depends on whether slope matches direction...
	hang_t::typ end_slope = welt->lookup(end)->get_weg_hang();
	sint8 end_slope_height = end.z;
	if(  end_slope != hang_typ(zv) && end_slope != hang_typ(zv)*2  ) {
		end_slope_height += hang_t::max_diff(end_slope);
	}

	if (slope || bridge_height != 0) {
		// needs a ramp to start on ground
		add_height = slope ?  hang_t::max_diff(slope) : bridge_height;
		baue_auffahrt( player, start, ribi, slope?0:hang_typ(zv)*add_height, besch, weg_besch );
		if(  besch->get_waytype() != powerline_wt  ) {
			ribi = welt->lookup(start)->get_weg_ribi_unmasked(besch->get_waytype());
		}
	} else {
		// start at cliff
		if(  besch->get_waytype() == powerline_wt  ) {
			leitung_t *lt = start_gr->get_leitung();
			if(!lt) {
				lt = new leitung_t(start_gr->get_pos(), player);
				lt->set_besch( weg_besch );
				start_gr->obj_add( lt );
				lt->finish_rd();
			}
		}
		else if(  !start_gr->weg_erweitern( besch->get_waytype(), ribi )  ) {
			// builds new way
			weg_t * const weg = weg_t::alloc( besch->get_waytype() );
			weg->set_besch( weg_besch );
			weg->set_bridge_weight_limit(besch->get_max_weight());
			const hang_t::typ hang = start_gr ? start_gr->get_weg_hang() :  hang_t::flach;
			if(hang != hang_t::flach)
			{
				const uint slope_height = (hang & 7) ? 1 : 2;
				if(slope_height == 1)
				{
					weg->set_max_speed(min(besch->get_topspeed_gradient_1(), weg_besch->get_topspeed_gradient_1()));
				}
				else
				{
					weg->set_max_speed(min(besch->get_topspeed_gradient_2(), weg_besch->get_topspeed_gradient_2()));
				}
			}
			else
			{
				weg->set_max_speed(min(besch->get_topspeed(), weg_besch->get_topspeed()));
			}
			const weg_t* old_way = start_gr ? start_gr->get_weg(weg_besch->get_wtyp()) : NULL;
			const wayobj_t* way_object = old_way ? way_object = start_gr->get_wayobj(besch->get_waytype()) : NULL;
			// Necessary to avoid the "default" way (which might have constraints) setting the constraints here.
			weg->clear_way_constraints();
			weg->add_way_constraints(besch->get_way_constraints());
			if(old_way)
			{		
				if(way_object)
				{
					weg->add_way_constraints(way_object->get_besch()->get_way_constraints());
				}
			}
			player_t::book_construction_costs( player, -start_gr->neuen_weg_bauen( weg, ribi, player ) -weg->get_besch()->get_preis(), end.get_2d(), weg->get_waytype());
		}
		start_gr->calc_image();
	}

	koord3d pos = start+koord3d( zv.x, zv.y, add_height );
	while(  pos.get_2d() != end.get_2d()  ) {
		brueckenboden_t *bruecke = new brueckenboden_t( pos, 0, 0 );
		welt->access(pos.get_2d())->boden_hinzufuegen(bruecke);

		if(besch->get_waytype() != powerline_wt) {
			weg_t * const weg = weg_t::alloc(besch->get_waytype());
			weg->set_besch(weg_besch);
			weg->set_bridge_weight_limit(besch->get_max_weight());
			bruecke->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), player);
			const grund_t* gr = welt->lookup(weg->get_pos());
			const hang_t::typ hang = gr ? gr->get_weg_hang() :  hang_t::flach;
			const weg_t* old_way = gr ? gr->get_weg(weg_besch->get_wtyp()) : NULL;
			const wayobj_t* way_object = old_way ? way_object = gr->get_wayobj(besch->get_waytype()) : NULL;

			if(hang != hang_t::flach) 
			{
				const uint slope_height = (hang & 7) ? 1 : 2;
				if(slope_height == 1)
				{
					weg->set_max_speed(min(besch->get_topspeed_gradient_1(), weg_besch->get_topspeed_gradient_1()));
				}
				else
				{
					weg->set_max_speed(min(besch->get_topspeed_gradient_2(), weg_besch->get_topspeed_gradient_2()));
				}
			}
			else
			{
				weg->set_max_speed(min(besch->get_topspeed(), weg_besch->get_topspeed()));
			}
			// Necessary to avoid the "default" way (which might have constraints) setting the constraints here.
			weg->clear_way_constraints();
			weg->add_way_constraints(besch->get_way_constraints());
			if(old_way)
			{
				const wayobj_t* way_object = gr->get_wayobj(besch->get_waytype());
				if(way_object)
				{
					weg->add_way_constraints(way_object->get_besch()->get_way_constraints());
				}
			}
		}
		else {
			leitung_t *lt = new leitung_t(bruecke->get_pos(), player);
			bruecke->obj_add( lt );
			lt->finish_rd();
		}
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - gr->get_pos().z;
		bruecke_t *br = new bruecke_t(bruecke->get_pos(), player, besch, besch->get_simple(ribi,height-hang_t::max_diff(gr->get_grund_hang())));
		bruecke->obj_add(br);
		bruecke->calc_image();
		br->finish_rd();
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","at (%i,%i)",pos.x,pos.y);
		if(besch->get_pillar()>0) {
			// make a new pillar here
			if(besch->get_pillar()==1  ||  (pos.x*zv.x+pos.y*zv.y)%besch->get_pillar()==0) {
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","h1=%i, h2=%i",pos.z,gr->get_pos().z);
				while(height-->0) {
					if( TILE_HEIGHT_STEP*height <= 127) {
						// eventual more than one part needed, if it is too high ...
						gr->obj_add( new pillar_t(gr->get_pos(),player,besch,besch->get_pillar(ribi), TILE_HEIGHT_STEP*height) );
					}
				}
			}
		}

		pos = pos + zv;
	}

	// must determine end tile: on a slope => likely need auffahrt
	bool need_auffahrt = pos.z != end_slope_height;
	if(  need_auffahrt  ) {
		if(  weg_t const* const w = welt->lookup(end)->get_weg( weg_besch->get_wtyp() )  ) {
			need_auffahrt &= w->get_besch()->get_styp() != weg_besch_t::elevated;
		}
	}

	grund_t *gr=welt->lookup(end);

	if(  need_auffahrt  ) {
		// not ending at a bridge
		baue_auffahrt(player, end, ribi_typ(-zv), gr->get_weg_hang()?0:hang_typ(-zv)*(pos.z-end.z), besch, weg_besch);
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
				weg->set_bridge_weight_limit(besch->get_max_weight());
				const weg_t* old_way = gr ? gr->get_weg(weg_besch->get_wtyp()) : NULL;
				const wayobj_t* way_object = old_way ? way_object = gr->get_wayobj(besch->get_waytype()) : NULL;
				const hang_t::typ hang = gr ? gr->get_weg_hang() :  hang_t::flach;
				if(hang != hang_t::flach) 
				{
					const uint slope_height = (hang & 7) ? 1 : 2;
					if(slope_height == 1)
					{
						weg->set_max_speed(min(besch->get_topspeed_gradient_1(), weg_besch->get_topspeed_gradient_1()));
					}
					else
					{
						weg->set_max_speed(min(besch->get_topspeed_gradient_2(), weg_besch->get_topspeed_gradient_2()));
					}
				}
				else
				{
					weg->set_max_speed(min(besch->get_topspeed(), weg_besch->get_topspeed()));
				}
				// Necessary to avoid the "default" way (which might have constraints) setting the constraints here.
				weg->clear_way_constraints();
				weg->add_way_constraints(besch->get_way_constraints());
				if(old_way)
				{
					const wayobj_t* way_object = gr->get_wayobj(besch->get_waytype());
					if(way_object)
					{
						weg->add_way_constraints(way_object->get_besch()->get_way_constraints());
					}
				}
				player_t::book_construction_costs( player, -gr->neuen_weg_bauen( weg, ribi, player ) -weg->get_besch()->get_preis(), end.get_2d(), weg->get_waytype());
			}
			gr->calc_image();
		}
		else {
			leitung_t *lt = gr->get_leitung();
			if(  lt==NULL  ) {
				lt = new leitung_t(end, player );
				player_t::book_construction_costs(player, -weg_besch->get_preis(), gr->get_pos().get_2d(), powerline_wt);
				gr->obj_add(lt);
				lt->set_besch(weg_besch);
				lt->finish_rd();
			}
			lt->calc_neighbourhood();
		}
	}

	// if start or end are single way, and next tile is not, try to connect
	if(  besch->get_waytype() != powerline_wt  &&  player  ) {
		if(  grund_t *start_gr = welt->lookup(start)  ) {
			ribi_t::ribi ribi = start_gr->get_weg_ribi_unmasked(besch->get_waytype());
			if(  ribi_t::ist_einfach(ribi)  ) {
				// only single tile under start => try to connect to next tile
				koord3d next_to_start = koord3d( start.get_2d()-koord(ribi), start_gr->get_vmove( ribi_t::rueckwaerts(ribi) ) );
				wegbauer_t bauigel(player);
				bauigel.set_keep_existing_ways(true);
				bauigel.set_keep_city_roads(true);
				bauigel.set_maximum(20);
				bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat ), NULL, NULL );
				bauigel.calc_route( start, next_to_start );
				if(  bauigel.get_count() == 2  ) {
					bauigel.baue();
				}
			}
		}
		if(  grund_t *end_gr = welt->lookup(end)  ) {
			ribi_t::ribi ribi = end_gr->get_weg_ribi_unmasked(besch->get_waytype());
			if(  ribi_t::ist_einfach(ribi)  ) {
				// only single tile under start => try to connect to next tile
				koord3d next_to_end = koord3d( end.get_2d()-koord(ribi), end_gr->get_vmove( ribi_t::rueckwaerts(ribi) ) );
				wegbauer_t bauigel(player);
				bauigel.set_keep_existing_ways(true);
				bauigel.set_keep_city_roads(true);
				bauigel.set_maximum(20);
				bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat ), NULL, NULL );
				bauigel.calc_route( end, next_to_end );
				if(  bauigel.get_count() == 2  ) {
					bauigel.baue();
				}
			}
		}
	}
}


void brueckenbauer_t::baue_auffahrt(player_t* player, koord3d end, ribi_t::ribi ribi_neu, hang_t::typ weg_hang, const bruecke_besch_t* besch, const weg_besch_t *weg_besch)
{
	assert(weg_hang >= 0);
	assert(weg_hang < 81);

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
			weg->set_besch(weg_besch); 
			weg->set_bridge_weight_limit(besch->get_max_weight());
			player_t::book_construction_costs(player, -bruecke->neuen_weg_bauen( weg, ribi_neu, player ), end.get_2d(), besch->get_waytype());
		}
		const grund_t* gr = welt->lookup(weg->get_pos());
		const hang_t::typ hang = gr ? gr->get_weg_hang() : hang_t::flach;
		if(hang != hang_t::flach) 
		{
			const uint slope_height = (hang & 7) ? 1 : 2;
			if(slope_height == 1)
			{
			weg->set_max_speed(min(besch->get_topspeed_gradient_1(), weg_besch->get_topspeed_gradient_1()));
			}
			else
			{
				weg->set_max_speed(min(besch->get_topspeed_gradient_2(), weg_besch->get_topspeed_gradient_2()));
			}
		}
		else
		{
			weg->set_max_speed(min(besch->get_topspeed(), weg_besch->get_topspeed()));
		}
		// Necessary to avoid the "default" way (which might have constraints) setting the constraints here.
		const weg_t* old_way = gr ? gr->get_weg(weg->get_besch()->get_wtyp()) : NULL;
		weg->clear_way_constraints();
		weg->add_way_constraints(besch->get_way_constraints());
		if(old_way)
		{
			const wayobj_t* way_object = gr->get_wayobj(besch->get_waytype());
			if(way_object)
			{
				weg->add_way_constraints(way_object->get_besch()->get_way_constraints());
			}
		}
	} else {

		leitung_t *lt = bruecke->get_leitung();
		if(!lt) {
			lt = new leitung_t(bruecke->get_pos(), player);
			bruecke->obj_add( lt );
		}
		else {
			// remove maintenance - it will be added in leitung_t::finish_rd
			player_t::add_maintenance( player, -lt->get_besch()->get_wartung(), powerline_wt);
		}
		// connect to neighbor tiles and networks, add maintenance
		lt->finish_rd();
	}
	bruecke_t *br = new bruecke_t(end, player, besch, img);
	bruecke->obj_add( br );
	br->finish_rd();
	bruecke->calc_image();
}


const char *brueckenbauer_t::remove(player_t *player, koord3d pos_start, waytype_t wegtyp)
{
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);
	slist_tpl<koord3d> end_list;
	slist_tpl<koord3d> part_list;
	slist_tpl<koord3d> tmp_list;
	const char    *msg;

	tmp_list.insert(pos_start);
	marker.mark(welt->lookup(pos_start));
	waytype_t delete_wegtyp = wegtyp==powerline_wt ? invalid_wt : wegtyp;
	koord3d pos;
	do {
		pos = tmp_list.remove_first();

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(pos != pos_start && from->ist_karten_boden()) {
			// gr is start/end - test only one direction
			if(from->get_grund_hang() != hang_t::flach) {
				zv = -koord(from->get_grund_hang());
			}
			else if(from->get_weg_hang() != hang_t::flach) {
				zv = koord(from->get_weg_hang());
			}
			end_list.insert(pos);
		}
		else if(pos == pos_start) {
			if(from->ist_karten_boden()) {
				// gr is start/end - test only one direction
				if(from->get_grund_hang() != hang_t::flach) {
					zv = -koord(from->get_grund_hang());
				}
				else if(from->get_weg_hang() != hang_t::flach) {
					zv = koord(from->get_weg_hang());
				}
				end_list.insert(pos);
			}
			else {
				ribi_t::ribi r = wegtyp==powerline_wt ? from->get_leitung()->get_ribi() : from->get_weg_nr(0)->get_ribi_unmasked();
				ribi_t::ribi dir1 = r & ribi_t::nordost;
				ribi_t::ribi dir2 = r & ribi_t::suedwest;
				bool dir1_ok = false, dir2_ok = false;

				grund_t *to;
				// test if we are at the end of a bridge:
				// 1. test direction must be single or zero
				bool is_end1 = (dir1 == ribi_t::keine);
				// 2. if single direction: test for neighbor in that direction
				//    there must be no neighbor or no bridge
				if (ribi_t::ist_einfach(dir1)) {
					is_end1 = !from->get_neighbour(to, delete_wegtyp, dir1)  ||  !to->ist_bruecke();
				}
				// now do the same for the reverse direction
				bool is_end2 = (dir2 == ribi_t::keine);
				if (ribi_t::ist_einfach(dir2)) {
					is_end2 = !from->get_neighbour(to, delete_wegtyp, dir2)  ||  !to->ist_bruecke();
				}
				
				if(!is_end1 && !is_end2) {
					return "Cannot delete a bridge from its centre";
				}
				
				// if one is an end then go towards the other direction
				zv = koord(is_end1 ? dir2 : dir1);
				part_list.insert(pos);
			}
		}
		else if(from->ist_bruecke()) {
			part_list.insert(pos);
		}
		// can we delete everything there?
		msg = from->kann_alle_obj_entfernen(player);

		if(msg != NULL  ||  (from->get_halt().is_bound()  &&  from->get_halt()->get_owner()!=player)) {
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
								prev->remove_everything_from_way( player, wegtyp, bridge_ribi );	// removes stop and signals correctly
								prev->weg_entfernen( wegtyp, true );
								if(  prev->get_typ() == grund_t::monorailboden  ) {
									welt->access( prev->get_pos().get_2d() )->boden_entfernen( prev );
									delete prev;
								}
							}
							else {
								w->calc_image();
							}
						}
					}
				}
			}
		}
		first = false;

		// first: remove bridge
		bruecke_t *br = gr->find<bruecke_t>();
		br->cleanup(player);
		delete br;

		gr->remove_everything_from_way(player,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// remove also the second way on the bridge
		if(gr->get_weg_nr(0)!=0) {
			gr->remove_everything_from_way(player,gr->get_weg_nr(0)->get_waytype(),ribi_t::keine);
		}

		// we may have a second way/powerline here ...
		gr->obj_loesche_alle(player);
		gr->mark_image_dirty();

		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;

		// finally delete all pillars (if there)
		gr = welt->lookup_kartenboden(pos.get_2d());
		while (obj_t* const p = gr->find<pillar_t>()) {
			p->cleanup(p->get_owner());
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
							prev->remove_everything_from_way( player, wegtyp, bridge_ribi );	// removes stop and signals correctly
							prev->weg_entfernen( wegtyp, true );
							if(  prev->get_typ() == grund_t::monorailboden  ) {
								welt->access( prev->get_pos().get_2d() )->boden_entfernen( prev );
								delete prev;
							}
						}
						else {
							w->calc_image();
						}
					}
				}
			}
		}

		if(wegtyp==powerline_wt) {
			while (obj_t* const br = gr->find<bruecke_t>()) {
				br->cleanup(player);
				delete br;
			}
			leitung_t *lt = gr->get_leitung();
			if (lt) {
				player_t *old_owner = lt->get_owner();
				// first delete powerline to decouple from the bridge powernet
				lt->cleanup(old_owner);
				delete lt;
				// .. now create powerline to create new powernet
				lt = new leitung_t(gr->get_pos(), old_owner);
				lt->finish_rd();
				gr->obj_add(lt);
			}
		}
		else {
			ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp);
			ribi_t::ribi bridge_ribi;

			if(gr->get_weg_hang() != hang_t::flach) {
				bridge_ribi = ~ribi_t::rueckwaerts(ribi_typ(gr->get_weg_hang()));
			}
			else {
				bridge_ribi = ~ribi_typ(gr->get_weg_hang());
			}
			bridge_ribi &= ~(ribi_t::rueckwaerts(~bridge_ribi));
			ribi &= bridge_ribi;

			bruecke_t *br = gr->find<bruecke_t>();
			if(br)
			{
				br->cleanup(player);
			}
			delete br;

			// stops on flag bridges ends with road + track on it
			// are not correctly deleted if ways are kept
			if (gr->is_halt()) {
				haltestelle_t::remove(player, gr->get_pos());
			}

			// depots at bridge ends needs to be deleted as well
			if (depot_t *dep = gr->get_depot()) {
				dep->cleanup(player);
				delete dep;
			}

			// removes single signals, bridge head, pedestrians, stops, changes catenary etc
			weg_t *weg=gr->get_weg_nr(1);
			if(weg) {
				gr->remove_everything_from_way(player,weg->get_waytype(),bridge_ribi);
			}
			gr->remove_everything_from_way(player,wegtyp,bridge_ribi);	// removes stop and signals correctly

			// corrects the ways
			weg = gr->get_weg(wegtyp);
			if(  weg  ) {
				// needs checks, since this fails if it was the last tile
				weg->set_besch( weg->get_besch(), true );
				weg->set_ribi( ribi );
				if(  hang_t::max_diff(gr->get_weg_hang())>=2  &&  !weg->get_besch()->has_double_slopes()  ) {
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
			gr_new->get_leitung()->calc_neighbourhood(); // Recalc the image. calc_image() doesn't do the right job...
		}
	}

	welt->set_dirty();
	return NULL;
}
