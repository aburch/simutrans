/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "../simdebug.h"
#include "../tool/simtool.h"
#include "brueckenbauer.h"
#include "wegbauer.h"

#include "../world/simworld.h"
#include "../simhalt.h"
#include "../obj/depot.h"
#include "../player/simplay.h"
#include "../simtypes.h"

#include "../ground/boden.h"
#include "../ground/brueckenboden.h"

#include "../gui/tool_selector.h"
#include "../gui/minimap.h"

#include "../descriptor/bridge_desc.h"

#include "../dataobj/marker.h"
#include "../dataobj/scenario.h"
#include "../obj/bruecke.h"
#include "../obj/leitung2.h"
#include "../obj/pillar.h"
#include "../obj/signal.h"
#include "../dataobj/crossing_logic.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"

karte_ptr_t bridge_builder_t::welt;


/// All bridges hashed by name
static stringhashtable_tpl<const bridge_desc_t *> desc_table;


void bridge_builder_t::register_desc(bridge_desc_t *desc)
{
	// avoid duplicates with same name
	if(  const bridge_desc_t *old_desc = desc_table.remove(desc->get_name())  ) {
		dbg->doubled( "bridge", desc->get_name() );
		tool_t::general_tool.remove( old_desc->get_builder() );
		delete old_desc->get_builder();
		delete old_desc;
	}

	// add the tool
	tool_build_bridge_t *tool = new tool_build_bridge_t();
	tool->set_icon( desc->get_cursor()->get_image_id(1) );
	tool->cursor = desc->get_cursor()->get_image_id(0);
	tool->set_default_param(desc->get_name());
	tool_t::general_tool.append( tool );
	desc->set_builder( tool );
	desc_table.put(desc->get_name(), desc);
}


const bridge_desc_t *bridge_builder_t::get_desc(const char *name)
{
	return  (name ? desc_table.get(name) : NULL);
}


const bridge_desc_t *bridge_builder_t::find_bridge(const waytype_t wtyp, const sint32 min_speed, const uint16 time)
{
	const bridge_desc_t *find_desc=NULL;

	FOR(stringhashtable_tpl<bridge_desc_t const*>, const& i, desc_table) {
		bridge_desc_t const* const desc = i.value;
		if(  desc->get_waytype()==wtyp  &&  desc->is_available(time)  ) {
			if(  find_desc==NULL  ||
				(find_desc->get_topspeed()<min_speed  &&  find_desc->get_topspeed()<desc->get_topspeed())  ||
				(desc->get_topspeed()>=min_speed  &&  desc->get_maintenance()<find_desc->get_maintenance())
			) {
				find_desc = desc;
			}
		}
	}
	return find_desc;
}


/**
 * Compares the maximum speed of two bridges.
 * @param a the first bridge.
 * @param b the second bridge.
 * @return true, if the speed of the second bridge is greater.
 */
static bool compare_bridges(const bridge_desc_t* a, const bridge_desc_t* b)
{
	return a->get_topspeed() < b->get_topspeed();
}


void bridge_builder_t::fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_BRIDGE | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();
	vector_tpl<const bridge_desc_t*> matching(desc_table.get_count());

	// list of matching types (sorted by speed)
	FOR(stringhashtable_tpl<bridge_desc_t const*>, const& i, desc_table) {
		bridge_desc_t const* const b = i.value;
		if (  b->get_waytype()==wtyp  &&  b->is_available(time)  ) {
			matching.insert_ordered( b, compare_bridges);
		}
	}

	// now sorted ...
	FOR(vector_tpl<bridge_desc_t const*>, const i, matching) {
		tool_selector->add_tool_selector(i->get_builder());
	}
}


const vector_tpl<const bridge_desc_t *>&  bridge_builder_t::get_available_bridges(const waytype_t wtyp)
{
	static vector_tpl<const bridge_desc_t *> dummy;
	dummy.clear();
	const uint16 time = welt->get_timeline_year_month();
	FOR(stringhashtable_tpl<bridge_desc_t const*>, const& i, desc_table) {
		bridge_desc_t const* const b = i.value;
		if (  b->get_waytype()==wtyp  &&  b->is_available(time)  ) {
			dummy.append(b);
		}
	}
	return dummy;
}


inline bool ribi_check( ribi_t::ribi ribi, ribi_t::ribi check_ribi )
{
	// either check for single (if nothing given) otherwise ensure exact match
	return check_ribi ? ribi == check_ribi : ribi_t::is_single( ribi );
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

	if(  !slope_t::is_way(gr->get_weg_hang())  ) {
		// it's not a straight slope
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
			if(  gr->get_weg(wt)  &&  ribi_check(ribi, check_ribi)  ) {
				// ok too
				return NULL;
			}
			return "A bridge must start on a way!";
		}

		if(  w->get_waytype() != wt  ) {
			// now check for perpendicular and crossing
			if(  (ribi_t::doubles(ribi) ^ ribi_t::doubles(check_ribi) ) == ribi_t::all  &&  crossing_logic_t::get_crossing(wt, w->get_waytype(), 0, 0, welt->get_timeline_year_month()  )  ) {
				return NULL;
			}
			return "A bridge must start on a way!";
		}

		// same waytype, check direction
		if(  w->get_waytype() == wt   &&  ribi_check(ribi, check_ribi ) ) {
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
	// something here which we cannot remove => fail too
	if(  obj_t *obj=gr->obj_bei(0)  ) {
		if(  const char *err_msg = obj->is_deletable(player)  ) {
			return err_msg;
		}
	}
	return ""; // could end here but need not end here
}

bool bridge_builder_t::is_blocked(koord3d pos, ribi_t::ribi check_ribi, const char *&error_msg)
{
	/* can't build directly above or below a way if height clearance == 2 */
	// take slopes on grounds below into accout
	const sint8 clearance = welt->get_settings().get_way_height_clearance()-1;
	for(int dz = -clearance -2; dz <= clearance; dz++) {
		grund_t *gr2 = NULL;
		if (dz != 0 && (gr2 = welt->lookup(pos + koord3d(0,0,dz)))) {

			const slope_t::type slope = gr2->get_weg_hang();
			if (dz < -clearance) {
				if (dz + slope_t::max_diff(slope) + gr2->get_weg_yoff() / TILE_HEIGHT_STEP < -clearance ) {
					// too far below
					continue;
				}
			}

			if (dz + slope_t::max_diff(slope) == 0  &&  gr2->ist_karten_boden()  &&  ribi_type(gr2->get_grund_hang())==check_ribi) {
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

	return false;
}

// return true if bridge can be connected to monorail tile
bool bridge_builder_t::is_monorail_junction(koord3d pos, player_t *player, const bridge_desc_t *desc, const char *&error_msg)
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
				if(  w->get_waytype() == desc->get_waytype()  ) {
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
// 2 to suppress warnings when h=start_height+x  and min_bridge_height=start_height
#define height_okay2(h) (((h) > max_height) ? false : height_okay_array[h-min_bridge_height])

koord3d bridge_builder_t::find_end_pos(player_t *player, koord3d pos, const koord zv, const bridge_desc_t *desc, const char *&error_msg, sint8 &bridge_height, bool ai_bridge, uint32 min_length, bool high_bridge )
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
	const slope_t::type slope = gr2->get_weg_hang();
	const sint8 start_height = gr2->get_hoehe() + slope_t::max_diff(slope);
	sint8 min_bridge_height = start_height; /* was  + (slope==0) */
	sint8 min_height = start_height - (1+desc->has_double_ramp()) + (slope==0);
	sint8 max_height = start_height + (slope || gr2->ist_tunnel() ? 0 : (1+desc->has_double_ramp()));

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
		min_height = max_height - (1+desc->has_double_ramp());
		min_bridge_height = start_height;
	}
	bool height_okay_array[256];
	for (int i = 0; i < max_height+1 - min_bridge_height; i++) {
		height_okay_array[i] = true;
	}

	if(  slope_t::max_diff(slope)==2  &&  !desc->has_double_start()  ) {
		error_msg = "Cannot build on a double slope!";
		return koord3d::invalid;
	}

	scenario_t *scen = welt->get_scenario();
	waytype_t wegtyp = desc->get_waytype();
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
		if(  desc->get_max_length() > 0  &&  length > desc->get_max_length()  ) {
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

		if(  gr->get_hoehe() == max_height-1  &&  desc->get_waytype() != powerline_wt  &&  gr->get_leitung()  ) {
			error_msg = "Tile not empty.";
			return koord3d::invalid;
		}

		// check for height
		if(  desc->get_max_height()  &&  max_height-gr->get_hoehe() > desc->get_max_height()  ) {
			error_msg = "bridge is too high for its type!";
			return koord3d::invalid;
		}

		if(  gr->hat_weg(air_wt)  &&  gr->get_styp(air_wt)==type_runway  ) {
			error_msg = "No bridges over runways!";
			return koord3d::invalid;
		}

		bool abort = true;
		for(sint8 z = min_bridge_height; z <= max_height; z++) {
			if(height_okay(z)) {
				if(is_blocked(koord3d(pos.get_2d(), z), ribi_type(zv), error_msg)) {
					height_okay_array[z-min_bridge_height] = false;

					// connect to suitable monorail tiles if possible
					if(is_monorail_junction(koord3d(pos.get_2d(), z), player, desc, error_msg)) {
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

		const slope_t::type end_slope = gr->get_weg_hang();
		const sint16 hang_height = gr->get_hoehe() + slope_t::max_diff(end_slope) + gr->get_weg_yoff()/TILE_HEIGHT_STEP;

		if(hang_height > max_height) {
			error_msg = "Cannot connect to the\ncenter of a double slope!";
			return koord3d::invalid;
		}
		if(hang_height < min_height) {
			continue;
		}
		// now check for end of bridge conditions
		if(length >= min_length && slope_t::is_way(end_slope) &&
		   (gr->get_typ()==grund_t::boden || gr->get_typ()==grund_t::tunnelboden)) {
			bool finish = false;
			if(height_okay(hang_height) && end_slope == slope_t::flat &&
			   (hang_height == max_height || ai_bridge || min_length)) {
				/* now we have a flat tile below */
				error_msg = check_tile( gr, player, desc->get_waytype(), ribi_type(zv) );

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
				if(  slope_t::max_diff(end_slope)==2   &&   !desc->has_double_start()  ) {
					// cannot end on a double slope if we do not have the matching ramp
					error_msg = "Cannot build on a double slope!";
				}
				else {
					// first check slope
					if(  ribi_type(end_slope) == ribi_type(zv)  ) {
						// slope matches
						error_msg = check_tile( gr, player, desc->get_waytype(), ribi_type(zv) );
						if(  !error_msg  ||  !*error_msg  ) {
							// success
							bridge_height = hang_height - start_height;

							return gr->get_pos();
						}
					}
				}
			}
			else if (end_slope == slope_t::flat) {
				if ((hang_height+1 >= start_height && height_okay(hang_height+1)) ||
				    (hang_height+2 >= start_height && height_okay(hang_height+2))) {
					/* now we have a flat tile below */
					error_msg = check_tile( gr, player, desc->get_waytype(), ribi_type(zv) );

					if(  hang_height < max_height  &&  (  gr->has_two_ways()  ||  (  gr->get_weg_nr(0)  &&  (gr->get_weg_nr(0)->get_waytype() != desc->get_waytype()  ||  gr->get_weg_ribi_unmasked(desc->get_waytype())!=ribi_type(zv)  )  )  )  ) {
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

		const slope_t::type end_ground_slope = gr->get_grund_hang();
		const sint16 hang_ground_height = gr->get_hoehe()+slope_t::max_diff(end_ground_slope);
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


bool bridge_builder_t::is_start_of_bridge( const grund_t *gr )
{
	if(  gr->ist_bruecke()  ) {
		if(  gr->ist_karten_boden()  ) {
			// ramp => true
			return true;
		}
		// now check for end of rampless bridges
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked( gr->get_leitung() ? powerline_wt : gr->get_weg_nr(0)->get_waytype() );
		for(  int i=0;  i<4;  i++  ) {
			if(  ribi_t::nesw[i] & ribi  ) {
				grund_t *to = welt->lookup( gr->get_pos()+koord(ribi_t::nesw[i]) );
				if(  to  &&  !to->ist_bruecke()  ) {
					return true;
				}
			}
		}
	}
	return false;
}


bool bridge_builder_t::can_place_ramp(player_t *player, const grund_t *gr, waytype_t wt, ribi_t::ribi r )
{
	// bridges can only start or end above ground
	if(  gr->get_typ()!=grund_t::boden  &&  gr->get_typ()!=grund_t::monorailboden  &&
	     gr->get_typ()!=grund_t::tunnelboden  ) {
		return false;
	}
	// wrong slope
	if(!slope_t::is_way(gr->get_grund_hang())) {
		return false;
	}
	// blocked from above
	koord3d pos = gr->get_pos();
	if(  welt->lookup( pos + koord3d(0, 0, 1))  ||  (welt->get_settings().get_way_height_clearance()==2  &&  welt->lookup( pos + koord3d(0, 0, 2) ))  ) {
		return false;
	}
	// check for ribis and crossings
	bool test_ribi = r != ribi_t::none;
	if (wt==powerline_wt) {
		if (gr->hat_wege()) {
			return false;
		}
		if (test_ribi  &&  gr->find<leitung_t>()  &&  (gr->find<leitung_t>()->get_ribi() & ~r) != 0) {
			return false;
		}
	}
	else {
		if (gr->find<leitung_t>()) {
			return false;
		}

		if(wt!=road_wt) {
			// only road bridges can have other ways on it (ie trams)
			if(gr->has_two_ways()  ||  (gr->hat_wege()  && gr->get_weg(wt) == NULL) ) {
				return false;
			}
			if (test_ribi  &&  (gr->get_weg_ribi_unmasked(wt) & ~r) != 0) {
				return false;
			}
		}
		else {
			// If road and tram, we have to check both ribis.
			ribi_t::ribi ribi = ribi_t::none;
			for(int i=0;i<2;i++) {
				if (const weg_t *w = gr->get_weg_nr(i)) {
					if (w->get_waytype()!=road_wt  &&  !w->get_desc()->is_tram()) {
						return false;
					}
					ribi |= w->get_ribi_unmasked();
				}
				else break;
			}
			if (test_ribi  &&  (ribi & ~r) != 0) {
				return false;
			}
		}
	}
	// ribi from slope
	if (test_ribi  &&  gr->get_grund_hang()  &&  (ribi_type(gr->get_grund_hang()) & ~r) != 0) {
		return false;
	}
	// check everything else
	const char *error_msg = check_tile( gr, player, wt, r );
	return (error_msg == NULL  ||  *error_msg == 0  );
}


const char *bridge_builder_t::build( player_t *player, const koord3d pos, const bridge_desc_t *desc)
{
	const grund_t *gr = welt->lookup(pos);
	if(  !(gr  &&  desc)  ) {
		return "";
	}

	DBG_MESSAGE("bridge_builder_t::build()", "called on %d,%d for bridge type '%s'", pos.x, pos.y, desc->get_name());

	koord zv;
	ribi_t::ribi ribi = ribi_t::none;
	const weg_t *weg = gr->get_weg(desc->get_waytype());
	leitung_t *lt = gr->find<leitung_t>();

	if(  desc->get_waytype()==powerline_wt  ) {
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

	if(  !can_place_ramp(player, gr,desc->get_waytype(),ribi)  ) {
		DBG_MESSAGE( "bridge_builder_t::build()", "no way %x found", desc->get_waytype() );
		return "A bridge must start on a way!";
	}

	if(  gr->kann_alle_obj_entfernen(player)  ) {
		return "Tile not empty.";
	}

	if(gr->get_weg_hang() == slope_t::flat) {
		if(!ribi_t::is_single(ribi)) {
			ribi = 0;
		}
	}
	else {
		ribi_t::ribi hang_ribi = ribi_type(gr->get_weg_hang());
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

	zv = koord(ribi_t::backward(ribi));
	// search for suitable bridge end tile
	const char *msg;
	sint8 bridge_height;
	koord3d end = find_end_pos(player, gr->get_pos(), zv, desc, msg, bridge_height );

	// found something?
	if(  koord3d::invalid == end  ) {
DBG_MESSAGE("bridge_builder_t::build()", "end not ok");
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
	const way_desc_t* way_desc;
	if (weg) {
		way_desc = weg->get_desc();
	}
	else if (lt) {
		way_desc = lt->get_desc();
	}
	else {
		way_desc = way_builder_t::weg_search(desc->get_waytype(), desc->get_topspeed(), welt->get_timeline_year_month(), type_flat);
	}

	// Start and end have been checked, we can start to build eventually
	build_bridge(player, gr->get_pos(), end, zv, bridge_height, desc, way_desc );

	return NULL;
}


void bridge_builder_t::build_bridge(player_t *player, const koord3d start, const koord3d end, koord zv, sint8 bridge_height, const bridge_desc_t *desc, const way_desc_t *way_desc)
{
	ribi_t::ribi ribi = ribi_type(zv);

	DBG_MESSAGE("bridge_builder_t::build()", "build from %s", start.get_str() );

	grund_t *start_gr = welt->lookup( start );
	const slope_t::type slope = start_gr->get_weg_hang();

	// get initial height of bridge from start tile
	uint8 add_height = 0;

	// end tile height depends on whether slope matches direction...
	slope_t::type end_slope = welt->lookup(end)->get_weg_hang();
	sint8 end_slope_height = end.z;
	if(  end_slope != slope_type(zv) && end_slope != slope_type(zv)*2  ) {
		end_slope_height += slope_t::max_diff(end_slope);
	}

	if(  slope!=slope_t::flat  ||  bridge_height != 0  ) {
		// needs a ramp to start on ground
		add_height = slope!=slope_t::flat ? slope_t::max_diff(slope) : bridge_height;
		build_ramp( player, start, ribi, slope!=slope_t::flat ? slope_t::flat : slope_type(zv)*add_height, desc );
		if(  desc->get_waytype() != powerline_wt  ) {
			ribi = welt->lookup(start)->get_weg_ribi_unmasked(desc->get_waytype());
		}
	}
	else {
		// start at cliff
		if(  desc->get_waytype() == powerline_wt  ) {
			leitung_t *lt = start_gr->get_leitung();
			if(!lt) {
				lt = new leitung_t(start_gr->get_pos(), player);
				lt->set_desc( way_desc );
				start_gr->obj_add( lt );
				lt->finish_rd();
			}
		}
		else if(  !start_gr->weg_erweitern( desc->get_waytype(), ribi )  ) {
			// builds new way
			weg_t * const weg = weg_t::alloc( desc->get_waytype() );
			weg->set_desc( way_desc );
			player_t::book_construction_costs( player, -start_gr->neuen_weg_bauen( weg, ribi, player ) -weg->get_desc()->get_price(), end.get_2d(), weg->get_waytype());
		}
		start_gr->calc_image();
	}

	koord3d pos = start+koord3d( zv.x, zv.y, add_height );

	// update limits
	if(  welt->min_height > pos.z  ) {
		welt->min_height = pos.z;
	}
	else if(  welt->max_height < pos.z  ) {
		welt->max_height = pos.z;
	}

	while(  pos.get_2d() != end.get_2d()  ) {
		brueckenboden_t *bruecke = new brueckenboden_t( pos, slope_t::flat, slope_t::flat );
		welt->access(pos.get_2d())->boden_hinzufuegen(bruecke);
		if(  desc->get_waytype() != powerline_wt  ) {
			weg_t * const weg = weg_t::alloc(desc->get_waytype());
			weg->set_desc(way_desc);
			bruecke->neuen_weg_bauen(weg, ribi_t::doubles(ribi), player);
		}
		else {
			leitung_t *lt = new leitung_t(bruecke->get_pos(), player);
			bruecke->obj_add( lt );
			lt->finish_rd();
		}
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - gr->get_pos().z;
		bruecke_t *br = new bruecke_t(bruecke->get_pos(), player, desc, desc->get_straight(ribi,height-slope_t::max_diff(gr->get_grund_hang())));
		bruecke->obj_add(br);
		bruecke->calc_image();
		br->finish_rd();
//DBG_MESSAGE("bool bridge_builder_t::build_bridge()","at (%i,%i)",pos.x,pos.y);
		if(desc->get_pillar()>0) {
			// make a new pillar here
			if(desc->get_pillar()==1  ||  (pos.x*zv.x+pos.y*zv.y)%desc->get_pillar()==0) {
//DBG_MESSAGE("bool bridge_builder_t::build_bridge()","h1=%i, h2=%i",pos.z,gr->get_pos().z);
				while(height-->0) {
					if( TILE_HEIGHT_STEP*height <= 127) {
						// eventual more than one part needed, if it is too high ...
						gr->obj_add( new pillar_t(gr->get_pos(),player,desc,desc->get_pillar(ribi), TILE_HEIGHT_STEP*height) );
					}
				}
			}
		}

		pos = pos + zv;
	}

	// must determine end tile: on a slope => likely need auffahrt
	bool need_auffahrt = pos.z != end_slope_height;
	if(  need_auffahrt  ) {
		if(  weg_t const* const w = welt->lookup(end)->get_weg( way_desc->get_wtyp() )  ) {
			need_auffahrt &= w->get_desc()->get_styp() != type_elevated;
		}
	}

	grund_t *gr = welt->lookup(end);
	if(  need_auffahrt  ) {
		// not ending at a bridge
		build_ramp(player, end, ribi_type(-zv), gr->get_weg_hang()?0:slope_type(-zv)*(pos.z-end.z), desc);
	}
	else {
		// ending on a slope/elevated way
		if(desc->get_waytype() != powerline_wt) {
			// just connect to existing way
			ribi = ribi_t::backward( ribi_type(zv) );
			if(  !gr->weg_erweitern( desc->get_waytype(), ribi )  ) {
				// builds new way
				weg_t * const weg = weg_t::alloc( desc->get_waytype() );
				weg->set_desc( way_desc );
				player_t::book_construction_costs( player, -gr->neuen_weg_bauen( weg, ribi, player ) -weg->get_desc()->get_price(), end.get_2d(), weg->get_waytype());
			}
			gr->calc_image();
		}
		else {
			leitung_t *lt = gr->get_leitung();
			if(  lt==NULL  ) {
				lt = new leitung_t(end, player );
				player_t::book_construction_costs(player, -way_desc->get_price(), gr->get_pos().get_2d(), powerline_wt);
				gr->obj_add(lt);
				lt->set_desc(way_desc);
				lt->finish_rd();
			}
			lt->calc_neighbourhood();
		}
	}

	// if start or end are single way, and next tile is not, try to connect
	if(  desc->get_waytype() != powerline_wt  &&  player  ) {
		koord3d endtiles[] = {start, end};
		for(uint i=0; i<lengthof(endtiles); i++) {
			koord3d pos = endtiles[i];
			if(  grund_t *gr = welt->lookup(pos)  ) {
				ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(desc->get_waytype());
				grund_t *to = NULL;
				if(  ribi_t::is_single(ribi)  &&  gr->get_neighbour(to, invalid_wt, ribi_t::backward(ribi))) {
					// connect to open sea, calc_image will recompute ribi at to.
					if (desc->get_waytype() == water_wt  &&  to->is_water()) {
						gr->weg_erweitern(water_wt, ribi_t::backward(ribi));
						to->calc_image();
						continue;
					}
					// only single tile under bridge => try to connect to next tile
					way_builder_t bauigel(player);
					bauigel.set_keep_existing_ways(true);
					bauigel.set_keep_city_roads(true);
					bauigel.set_maximum(20);
					bauigel.init_builder( (way_builder_t::bautyp_t)desc->get_waytype(), way_desc, NULL, NULL );
					bauigel.calc_route( pos, to->get_pos() );
					if(  bauigel.get_count() == 2  ) {
						bauigel.build();
					}
				}
			}
		}
	}
}


void bridge_builder_t::build_ramp(player_t* player, koord3d end, ribi_t::ribi ribi_neu, slope_t::type weg_hang, const bridge_desc_t* desc)
{
	assert(weg_hang <= slope_t::max_number);

	grund_t *alter_boden = welt->lookup(end);
	brueckenboden_t *bruecke;
	slope_t::type grund_hang = alter_boden->get_grund_hang();
	bridge_desc_t::img_t img;

	bruecke = new brueckenboden_t(end, grund_hang, weg_hang);
	// add the ramp
	img = desc->get_end( bruecke->get_grund_hang(), grund_hang, weg_hang );

	weg_t *weg = NULL;
	if(desc->get_waytype() != powerline_wt) {
		weg = alter_boden->get_weg( desc->get_waytype() );
	}
	// take care of everything on that tile ...
	bruecke->take_obj_from( alter_boden );
	welt->access(end.get_2d())->kartenboden_setzen( bruecke );
	if(desc->get_waytype() != powerline_wt) {
		if(  !bruecke->weg_erweitern( desc->get_waytype(), ribi_neu)  ) {
			// needs still one
			weg = weg_t::alloc( desc->get_waytype() );
			player_t::book_construction_costs(player, -bruecke->neuen_weg_bauen( weg, ribi_neu, player ), end.get_2d(), desc->get_waytype());
		}
		weg->set_max_speed( desc->get_topspeed() );
	}
	else {
		leitung_t *lt = bruecke->get_leitung();
		if(!lt) {
			lt = new leitung_t(bruecke->get_pos(), player);
			bruecke->obj_add( lt );
		}
		else {
			// remove maintenance - it will be added in leitung_t::finish_rd
			player_t::add_maintenance( player, -lt->get_desc()->get_maintenance(), powerline_wt);
		}
		// connect to neighbor tiles and networks, add maintenance
		lt->finish_rd();
	}
	bruecke_t *br = new bruecke_t(end, player, desc, img);
	bruecke->obj_add( br );
	br->finish_rd();
	bruecke->calc_image();
}


const char *bridge_builder_t::remove(player_t *player, koord3d pos_start, waytype_t wegtyp)
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

		// weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(  pos != pos_start  &&  from->ist_karten_boden()  ) {
			// gr is start/end - test only one direction
			if(  from->get_grund_hang() != slope_t::flat  ) {
				zv = -koord(from->get_grund_hang());
			}
			else if(  from->get_weg_hang() != slope_t::flat  ) {
				zv = koord(from->get_weg_hang());
			}
			end_list.insert(pos);
		}
		else if(  pos == pos_start  ) {
			if(  from->ist_karten_boden()  ) {
				// gr is start/end - test only one direction
				if(  from->get_grund_hang() != slope_t::flat  ) {
					zv = -koord(from->get_grund_hang());
				}
				else if(  from->get_weg_hang() != slope_t::flat  ) {
					zv = koord(from->get_weg_hang());
				}
				end_list.insert(pos);
			}
			else {
				ribi_t::ribi r = wegtyp==powerline_wt ? from->get_leitung()->get_ribi() : from->get_weg_nr(0)->get_ribi_unmasked();
				ribi_t::ribi dir1 = r & ribi_t::northeast;
				ribi_t::ribi dir2 = r & ribi_t::southwest;

				grund_t *to;
				// test if we are at the end of a bridge:
				// 1. test direction must be single or zero
				bool is_end1 = (dir1 == ribi_t::none);
				// 2. if single direction: test for neighbor in that direction
				//    there must be no neighbor or no bridge
				if (ribi_t::is_single(dir1)) {
					is_end1 = !from->get_neighbour(to, delete_wegtyp, dir1)  ||  !to->ist_bruecke();
				}
				// now do the same for the reverse direction
				bool is_end2 = (dir2 == ribi_t::none);
				if (ribi_t::is_single(dir2)) {
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
			if(  (zv == koord::invalid  ||  zv == koord::nesw[r])  &&  from->get_neighbour(to, delete_wegtyp, ribi_t::nesw[r])  &&  !marker.is_marked(to)  &&  to->ist_bruecke()  ) {
				if(  wegtyp != powerline_wt  ||  (to->find<bruecke_t>()  &&  to->find<bruecke_t>()->get_desc()->get_waytype() == powerline_wt)  ) {
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
				if(  bridge_ribi & ribi_t::nesw[i]  ) {
					grund_t *prev;
					// if we have a ramp, then only check the higher end!
					if(  gr->get_neighbour( prev, wegtyp, ribi_t::nesw[i])  &&  !prev->ist_bruecke()  &&  !end_list.is_contained(prev->get_pos())   ) {
						if(  weg_t *w = prev->get_weg( wegtyp )  ) {
							// now remove ribi (or full way)
							w->set_ribi( (~ribi_t::backward( ribi_t::nesw[i] )) & w->get_ribi_unmasked() );
							if(  w->get_ribi_unmasked() == 0  ) {
								// nowthing left => then remove completel
								prev->remove_everything_from_way( player, wegtyp, bridge_ribi ); // removes stop and signals correctly
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

		gr->remove_everything_from_way(player,wegtyp,ribi_t::none); // removes stop and signals correctly
		// remove also the second way on the bridge
		if(gr->get_weg_nr(0)!=0) {
			gr->remove_everything_from_way(player,gr->get_weg_nr(0)->get_waytype(),ribi_t::none);
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
		minimap_t::get_instance()->calc_map_pixel(pos.get_2d());
	}

	// finally delete the bridge ends (all are kartenboden)
	while(  !end_list.empty()  ) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);

		// starts on slope or elevated way, or it consist only of the ramp
		ribi_t::ribi bridge_ribi = gr->get_weg_ribi_unmasked( wegtyp );
		for(  uint i = 0;  i < 4;  i++  ) {
			if(  bridge_ribi & ribi_t::nesw[i]  ) {
				grund_t *prev;
				// if we have a ramp, then only check the higher end!
				if(  gr->get_neighbour( prev, wegtyp, ribi_t::nesw[i])  &&  prev->get_hoehe() > gr->get_hoehe()  ) {
					if(  weg_t *w = prev->get_weg( wegtyp )  ) {
						// now remove ribi (or full way)
						w->set_ribi( (~ribi_t::backward( ribi_t::nesw[i] )) & w->get_ribi_unmasked() );
						if(  w->get_ribi_unmasked() == 0  ) {
							// nothing left => then remove completly
							prev->remove_everything_from_way( player, wegtyp, bridge_ribi ); // removes stop and signals correctly
							prev->weg_entfernen( wegtyp, true );
							if(  prev->get_typ() == grund_t::monorailboden  ) {
								welt->access( prev->get_pos().get_2d() )->boden_entfernen( prev );
								delete prev;
							}
							else if(  prev->get_typ() == grund_t::tunnelboden  &&  !prev->hat_wege()  ) {
								grund_t* gr_new = new boden_t(prev->get_pos(), prev->get_grund_hang());
								welt->access(prev->get_pos().get_2d())->kartenboden_setzen(gr_new);
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

			if(gr->get_weg_hang() != slope_t::flat) {
				bridge_ribi = ~ribi_t::backward(ribi_type(gr->get_weg_hang()));
			}
			else {
				bridge_ribi = ~ribi_type(gr->get_weg_hang());
			}
			bridge_ribi &= ~(ribi_t::backward(~bridge_ribi));
			ribi &= bridge_ribi;

			bruecke_t *br = gr->find<bruecke_t>();
			br->cleanup(player);
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
			gr->remove_everything_from_way(player,wegtyp,bridge_ribi); // removes stop and signals correctly

			// corrects the ways
			weg = gr->get_weg(wegtyp);
			if(  weg  ) {
				// needs checks, since this fails if it was the last tile
				weg->set_desc( weg->get_desc() );
				weg->set_ribi( ribi );
				if(  slope_t::max_diff(gr->get_weg_hang())>=2  &&  !weg->get_desc()->has_double_slopes()  ) {
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
