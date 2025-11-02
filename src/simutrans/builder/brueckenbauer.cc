/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include <string.h>

#include "../dataobj/crossing_logic.h"
#include "../dataobj/marker.h"
#include "../dataobj/scenario.h"

#include "../descriptor/bridge_desc.h"

#include "../ground/boden.h"
#include "../ground/brueckenboden.h"

#include "../gui/minimap.h"
#include "../gui/tool_selector.h"

#include "../obj/bruecke.h"
#include "../obj/depot.h"
#include "../obj/leitung2.h"
#include "../obj/pillar.h"
#include "../obj/signal.h"
#include "../obj/wayobj.h"

#include "../player/simplay.h"

#include "../tool/simtool.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"

#include "../world/simworld.h"

#include "../simachievements.h"
#include "../simdebug.h"
#include "../simhalt.h"
#include "../simtypes.h"

#include "brueckenbauer.h"
#include "wegbauer.h"


karte_ptr_t bridge_builder_t::welt;


/// All bridges hashed by name
static stringhashtable_tpl<const bridge_desc_t *> desc_table;


void bridge_builder_t::register_desc(bridge_desc_t *desc)
{
	// avoid duplicates with same name
	if(  const bridge_desc_t *old_desc = desc_table.remove(desc->get_name())  ) {
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

	for(auto const& i : desc_table) {
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
	bool enable = welt->get_scenario()->is_tool_enabled(welt->get_active_player(), TOOL_BUILD_BRIDGE | GENERAL_TOOL, wtyp, 0);

	const uint16 time = welt->get_timeline_year_month();
	vector_tpl<const bridge_desc_t*> matching(desc_table.get_count());

	// list of matching types (sorted by speed)
	for(auto const& i : desc_table) {
		bridge_desc_t const* const b = i.value;
		if (  b->get_waytype()==wtyp  &&  b->is_available(time)  ) {
			if (welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_BRIDGE | GENERAL_TOOL, wtyp,b->get_name())) {
				matching.insert_ordered(b, compare_bridges);
			}
		}
	}

	// now sorted ...
	for(bridge_desc_t const *i : matching) {
		i->get_builder()->enabled = enable  &&  welt->get_scenario()->is_tool_enabled(welt->get_active_player(), TOOL_BUILD_BRIDGE | GENERAL_TOOL, wtyp, i->get_name());
		tool_selector->add_tool_selector(i->get_builder());
	}
}


const vector_tpl<const bridge_desc_t *>&  bridge_builder_t::get_available_bridges(const waytype_t wtyp)
{
	static vector_tpl<const bridge_desc_t *> dummy;
	dummy.clear();
	const uint16 time = welt->get_timeline_year_month();
	for(auto const& i : desc_table) {
		bridge_desc_t const* const b = i.value;
		if (b->get_waytype()==wtyp  &&  b->is_available(time)  &&  b->get_builder()) {
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

		if(  w->get_removal_error(player) != NULL  ) {
			// not our way
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}

		// now check for direction
		ribi_t::ribi ribi = w->get_ribi_unmasked();
		if(  weg_t *w2 = gr->get_weg_nr(1)  ) {
			if(  w2->get_removal_error(player) != NULL ) {
				// not our way
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  !gr->get_crossing()  ) {
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
			if(  lt->get_removal_error(player) == NULL  &&  ribi_check( lt->get_ribi(), check_ribi )  ) {
				// matching powerline
				return NULL;
			}
			return "A bridge must start on a way!";
		}
	}
	// something here which we cannot remove => fail too
	if(  obj_t *obj=gr->obj_bei(0)  ) {
		if(  const char *err_msg = obj->get_removal_error(player)  ) {
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
			// no through bridges
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


/**
 * checks if a bridge can start (or end) on this tile in priciple
 * @returns either an error_message (must abort bridge building) or NULL
 */
const char* bridge_builder_t::check_start_tile(const player_t* player, const grund_t* gr, ribi_t::ribi bridge_ribi, const bridge_desc_t* desc)
{
	const char* err = "A bridge must start on a way!";
	if (!gr) {
		return err;
	}

	if (gr->get_typ() != grund_t::boden  &&  gr->get_typ() != grund_t::monorailboden) {
		return err;
	}

	// not overbuilt transformers bridges or tunnel
	if (obj_t* obj=gr->obj_bei(0)) {
		switch (obj->get_typ()) {
			case obj_t::bruecke:
			case obj_t::tunnel:
			case obj_t::pumpe:
			case obj_t::senke:
				return err;
			default:
				break;
		}
	}

	if (slope_t::type sl = gr->get_weg_hang()) {
		if (!slope_t::is_way(sl)) {
			// it's not a straight slope
			return "Bruecke muss an\neinfachem\nHang beginnen!\n";
		}

		if (slope_t::max_diff(gr->get_weg_hang()) == 2  &&  !desc->has_double_start()) {
			// cannot handle double slope
			return "Bruecke muss an\neinfachem\nHang beginnen!\n";
		}

		if (bridge_ribi  &&  ribi_type(sl) != bridge_ribi) {
			// wrong slope
			return "Bruecke muss an\neinfachem\nHang beginnen!\n";
		}
	}

	if (gr->is_halt()  ||  gr->get_depot()) {
		// something in the way
		return "Tile not empty.";
	}

	if (const char* err = gr->kann_alle_obj_entfernen(player)) {
		// therse is something not from us blocking construction
		return err;
	}

	if (desc->get_waytype() == powerline_wt) {
		if (leitung_t* lt = gr->get_leitung()) {
			ribi_t::ribi ribi = lt->get_ribi();
			if (ribi_t::is_single(ribi)) {
				// matching powerline
				return NULL;
			}
			return "A bridge must start on a way!";
		}

	}
	// we can build a ramp when there is one (or with tram two) way in our direction and no stations/depot etc.
	else if (gr->hat_wege()  &&  gr->get_typ()!=grund_t::monorailboden) {

		weg_t* w = gr->get_weg(desc->get_waytype());
		if (!w) {
			// another way type is blocking us
			return "A bridge must start on a way!";
		}

		// now check for direction
		ribi_t::ribi ribi = w->get_ribi_unmasked();
		if (gr->get_weg_nr(1)) {
			if (gr->get_crossing()) {
				// cannot start on a crossing
				return "A bridge must start on a way!";
			}
			// we have to check both ribis.
			ribi = gr->get_weg_nr(0)->get_ribi_unmasked() | gr->get_weg_nr(1)->get_ribi_unmasked();
		}
		if (ribi_t::is_single(ribi) || ribi_t::is_straight(ribi | bridge_ribi)) {
			// directions
			return NULL;
		}
		// one or two non-matching ways where I cannot built a crossing
		return "A bridge must start on a way!";
	}

	// test scenario conditions
	if (const char* error_msg = welt->get_scenario()->is_work_allowed_here(player, TOOL_BUILD_BRIDGE | GENERAL_TOOL, desc->get_finance_waytype(), desc->get_name(), gr->get_pos())) {
		return error_msg;
	}

	return NULL;
}





// check the bridge span for clearance in the given height
const char *bridge_builder_t::can_span_bridge(const player_t* player, koord3d start_pos, koord3d end_pos, sint8 height, const bridge_desc_t* desc)
{
	sint16 length = koord_distance(end_pos,start_pos);
	const sint8 clearance = welt->get_settings().get_way_height_clearance();

	if (length == 1  &&  end_pos.z == start_pos.z  &&  height == 0) {
		return "";	// just two tiles next to each other on flat ground => no bridge needed!
	}

	// test max length
	if (desc->get_max_length() > 0  &&  length-1 > desc->get_max_length()) {
		return "Bridge is too long for this type!\n";
	}

	if (start_pos.z < height) {
		// ramp on start tile needs clearance; also we are on ground then
		planquadrat_t* pl = welt->access(start_pos.x,start_pos.y);
		if (weg_t* w = pl->get_boden_in_hoehe(start_pos.z)->get_weg(desc->get_waytype())) {
			ribi_t::ribi r = w->get_ribi_unmasked();
			if (!ribi_t::is_single(r)  ||  ribi_t::is_bend(r|ribi_type(start_pos-end_pos))  ) {
				// not a single way here => cannot start
				return "A bridge must start on a way!";
			}
		}
		for (sint8 z = start_pos.z + 1; z < height + clearance; z++) {
			if (pl->get_boden_in_hoehe(z)) {
				// something is the way
				return "";
			}
		}
	}
	if (end_pos.z < height) {
		// ramp on end tile needs clearance; also we are on ground then
		planquadrat_t* pl = welt->access(end_pos.x, end_pos.y);
		if (weg_t* w = pl->get_boden_in_hoehe(end_pos.z)->get_weg(desc->get_waytype())) {
			ribi_t::ribi r = w->get_ribi_unmasked();
			if (!ribi_t::is_single(r) || ribi_t::is_bend(r | ribi_type(start_pos - end_pos))) {
				// not a single way here => cannot end here
				return "A bridge must start on a way!";
			}
		}
		for (sint8 z = end_pos.z + 1; z < height + clearance; z++) {
			if (pl->get_boden_in_hoehe(z)) {
				// something is the way
				return "";
			}
		}
	}

	koord zv = (end_pos - start_pos).get_2d() / length;
	for (sint16 i = 1; i < length; i++) {
		koord pos = start_pos.get_2d() + zv * i;

		planquadrat_t* pl = welt->access(pos.x, pos.y);
		if (!pl) {
			// outside of map
			return "";
		}

		// test scenario conditions
		if (const char* error_msg = welt->get_scenario()->is_work_allowed_here(player, TOOL_BUILD_BRIDGE | GENERAL_TOOL, desc->get_finance_waytype(), desc->get_name(), koord3d(pos,height))) {
			return error_msg;
		}

		for (int i = 0; i < clearance; i++) {
			if (pl->get_boden_in_hoehe(height + i)) {
				// something is the way
				return "";
			}
		}

		if (pl->get_boden_in_hoehe(height)) {
			// something at this level => stop
			return "";
		}

		// check for height
		grund_t* gr = pl->get_kartenboden();
		if (desc->get_max_height()  &&  height - gr->get_hoehe() > desc->get_max_height()) {
			return "bridge is too high for its type!";
		}

		if (gr->hat_weg(air_wt)  &&  gr->get_styp(air_wt) == type_runway) {
			return "No bridges over runways!";
		}

		for (int h = -clearance - 2; h <= -1; h++) {
			// check for slope and way clearance
			if (grund_t* grcheck = pl->get_boden_in_hoehe(height + h)) {
				sint8 delta_h = slope_t::max_diff(gr->get_grund_hang());
				if (grcheck->hat_wege()) {
					if (delta_h == 0) {
						delta_h += slope_t::max_diff(gr->get_weg_hang());
					}
					delta_h += clearance-1;
				}
				if (h + delta_h >= 0) {
					// not enough clearance
					return "";
				}
			}
		}
	}
	return NULL;
}


// checks if we can build a bridge here
const char* bridge_builder_t::can_build_bridge(const player_t* pl, koord3d start_pos, koord3d end_pos, sint8 &height, const bridge_desc_t* desc, bool try_high_bridge)
{
	const char* err = "A bridge must start on a way!";
	minivec_tpl<sint8>heights;

	koord delta = end_pos.get_2d() - start_pos.get_2d();
	if (delta.x && delta.y) {
		// no diagonal bridges
		return "";
	}
	else if (delta.x+delta.y == 0) {
		// no bridge
		return "";
	}
	delta = delta / abs(delta.x + delta.y); // normalised step difference
	ribi_t::ribi bridge_ribi = ribi_type(delta);

	grund_t* start = welt->lookup(start_pos);
	if (const char* error_msg = check_start_tile(pl, start, ribi_t::reverse_single(bridge_ribi), desc)) {
		return error_msg;
	}
	grund_t* end = welt->lookup(end_pos);
	if (const char* error_msg = check_start_tile(pl, end, bridge_ribi, desc)) {
		return error_msg;
	}

	// now the tiles are in priciple ok = find out the bridge height
	const sint8 max_ramp = 1 + desc->has_double_ramp(); // on flat tile
	const sint8 clearance = welt->get_settings().get_way_height_clearance();

	if (max(start_pos.z, end_pos.z)-min(start_pos.z, end_pos.z) > max_ramp) {
		// too big height difference
		return err;
	}
	slope_t::type start_sl = start->get_grund_hang();
	if (start->get_typ() == grund_t::monorailboden) {
		if (start->get_grund_hang()) {
			// no ramps on elevated ways ...
			return "";
		}
		heights.append(start_pos.z);
	}
	else if (start_sl != slope_t::flat) {
		sint8 start_dz = slope_t::max_diff(start_sl);
		heights.append(start_pos.z + start_dz);	// test this first
	}
	else {
		// start on flat tile => may need a ramp => need clearance
		sint8 max_z = max_ramp+clearance;
		while(welt->lookup(start_pos + koord3d(0, 0, max_z))) {
			if (max_z == clearance) {
				break;
			}
			max_z--;
		}
		max_z -= clearance;	// maximum possible height for a ramp
		for (sint8 i = 0; i <= max_z;  i++) {
			heights.append(start_pos.z+i);
		}
	}

	slope_t::type end_sl = end->get_grund_hang();
	if (end->get_typ() == grund_t::monorailboden) {
		if (!heights.is_contained(end_pos.z)) {
			// not matching heights
			return "";
		}
		heights.clear();
		heights.append(end_pos.z);
	}
	else if (end_sl != slope_t::flat) {
		sint8 end_dz = slope_t::max_diff(end_sl);
		if(!heights.is_contained(end_pos.z + end_dz)) {
			// mismatched heights
			return "";
		}
	}
	else {
		// start on flat tile => may need a ramp => need clearance
		sint8 max_z = max_ramp + clearance;
		while (welt->lookup(start_pos + koord3d(0, 0, max_z))) {
			if (max_z == clearance) {
				break;
			}
			max_z--;
		}
		max_z -= clearance;	// maximum possible height for a ramp
		// end of flat tile => may need a ramp
		for (int i = heights.get_count()-1; i >= 0; i--) {
			if (heights[i] > end_pos.z+max_z  ||  heights[i] < end_pos.z) {
				// not matching height
				heights.remove_at(i);
			}
		}
	}
	// if no heights are left => there is nothing to build for us ...
	if (heights.empty()) {
		return "";
	}

	// as default, we fail construction
	const char* error_msg = "";

	if (koord_distance(start_pos, end_pos) > 0) {
		// we always try to build the lowest bridge possible with the least slopes
		for (int  i = 0; i < heights.get_count(); i++) {
			sint8 h = heights[i];
			error_msg = can_span_bridge(pl, start_pos, end_pos, h, desc);
			if (!error_msg) {
				// found valid height
				height = h;
				return NULL;
			}
		}
	}
	return error_msg ? error_msg: "";
}



// returns the shortest possible bridge with a length of 1 ... min_length
const char *bridge_builder_t::find_end_pos(player_t* player, koord3d &pos, const koord zv, sint8& bridge_height, const bridge_desc_t* desc, uint32 min_length, uint32 max_length, bool also_flat_ends)
{
	grund_t* start = welt->lookup(pos);
	if (check_start_tile(player, start, ribi_t::backward(ribi_type(zv)), desc)) {
		return "";
	}
	const char* error = "Bridge is too long for this type!\n";
	sint8 finish_height = pos.z + slope_t::max_diff(start->get_grund_hang());

	// first find slope in my direction
	for (uint8 i = max(1,min_length); i <= max_length  &&  i <= welt->get_settings().way_max_bridge_len; i++) {
		const grund_t* gr_end = welt->lookup_kartenboden(pos.get_2d() + zv * i);
		if (!gr_end) {
			// not on map any more
			max_length = i-1;  // try shorter bridge then
			break;
		}
		if (gr_end->get_hoehe() + slope_t::max_diff(gr_end->get_grund_hang()) == finish_height) {
			// try to connect to a slope or flat tile ending here
			koord3d end = gr_end->get_pos();
			error = bridge_builder_t::can_build_bridge(player, pos, end, bridge_height, desc, false);
			if (!error) {
				pos = end;
				return NULL;
			}
			max_length = i-1;  // try shorter bridge then
			break;
		}
	}

	if(also_flat_ends) {
		// now try shorter bridge
		for (uint8 i = min_length;  i <= max_length; i++) {
			const grund_t* gr_end = welt->lookup_kartenboden(pos.get_2d() + zv * i);	// must succeed, we tested above
			koord3d end = gr_end->get_pos();
			if (const char* err = bridge_builder_t::can_build_bridge(player, pos, end, bridge_height, desc, false)) {
				if (err && *err) {
					error = err;
				}
			}
			else {
				pos = end;
				return NULL;
			}
		}
	}
	return (error &&  *error) ? error : "Bridge is too long for this type!\n";
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


void bridge_builder_t::build_bridge(player_t *player, const koord3d start, const koord3d end, koord zv, sint8 bridge_height, const bridge_desc_t *desc, const way_desc_t *way_desc)
{
	ribi_t::ribi ribi = ribi_type(zv);

	DBG_MESSAGE("bridge_builder_t::build()", "build from %s", start.get_str() );

	grund_t *start_gr = welt->lookup( start );
	const slope_t::type slope = start_gr->get_weg_hang();

	if(  slope!=slope_t::flat  ||  bridge_height > start.z  ) {
		// needs a ramp to start on ground
		build_ramp( player, start, ribi, slope ? 0 : slope_type(zv) * (bridge_height - start.z), desc );
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

	koord3d pos = koord3d(start.get_2d()+zv, bridge_height);

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
	grund_t* gr = welt->lookup(end);
	if(bridge_height > end.z) {
		// ending at different height => needs ramp
		build_ramp(player, end, ribi_type(-zv), gr->get_weg_hang()?0:slope_type(-zv)*(bridge_height-end.z), desc);
	}
	else {
		// ending on a flat cliff or elevated way
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
	if (desc->get_waytype() != powerline_wt && player) {
		koord3d endtiles[] = { start, end };
		for (uint i = 0; i < lengthof(endtiles); i++) {
			koord3d pos = endtiles[i];
			if (grund_t* gr = welt->lookup(pos)) {
				ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(desc->get_waytype());
				grund_t* to = NULL;
				if (ribi_t::is_single(ribi) && gr->get_neighbour(to, invalid_wt, ribi_t::backward(ribi))) {
					// connect to open sea, calc_image will recompute ribi at to.
					if (desc->get_waytype() == water_wt && to->is_water()) {
						gr->weg_erweitern(water_wt, ribi_t::backward(ribi));
						to->calc_image();
						continue;
					}
					// only single tile under bridge => try to connect to next tile
					way_builder_t bauigel(player);
					bauigel.set_keep_existing_ways(true);
					bauigel.set_keep_city_roads(true);
					bauigel.set_maximum(20);
					bauigel.init_builder((way_builder_t::bautyp_t)desc->get_waytype(), way_desc, NULL, NULL);
					bauigel.calc_route(pos, to->get_pos());
					if (bauigel.get_count() == 2) {
						bauigel.build();
					}
				}
			}
		}
	}
}


void bridge_builder_t::build_ramp(player_t* player, koord3d end, ribi_t::ribi ribi_neu, const slope_t::type weg_hang, const bridge_desc_t* desc)
{
	assert(weg_hang <= slope_t::max_number);

	grund_t* alter_boden = welt->lookup(end);
	brueckenboden_t* bruecke;
	slope_t::type grund_hang = alter_boden->get_grund_hang();
	bridge_desc_t::img_t img;

	bruecke = new brueckenboden_t(end, grund_hang, weg_hang);
	// add the ramp
	img = desc->get_end(bruecke->get_grund_hang(), grund_hang, weg_hang);

	weg_t* weg = NULL;
	if (desc->get_waytype() != powerline_wt) {
		weg = alter_boden->get_weg(desc->get_waytype());
	}
	// take care of everything on that tile ...
	bruecke->take_obj_from(alter_boden);
	welt->access(end.get_2d())->kartenboden_setzen(bruecke);
	if (desc->get_waytype() != powerline_wt) {
		if (!bruecke->weg_erweitern(desc->get_waytype(), ribi_neu)) {
			// needs still one
			weg = weg_t::alloc(desc->get_waytype());
			player_t::book_construction_costs(player, -bruecke->neuen_weg_bauen(weg, ribi_neu, player), end.get_2d(), desc->get_waytype());
		}
		weg->set_max_speed(desc->get_topspeed());
	}
	else {
		leitung_t* lt = bruecke->get_leitung();
		if (!lt) {
			lt = new leitung_t(bruecke->get_pos(), player);
			bruecke->obj_add(lt);
		}
		else {
			// remove maintenance - it will be added in leitung_t::finish_rd
			player_t::add_maintenance(player, -lt->get_desc()->get_maintenance(), powerline_wt);
		}
		// connect to neighbor tiles and networks, add maintenance
		lt->finish_rd();
	}
	bruecke_t* br = new bruecke_t(end, player, desc, img);
	bruecke->obj_add(br);
	br->finish_rd();
	bruecke->calc_image();
}



const char* bridge_builder_t::renovate(player_t* player, koord3d pos_start, waytype_t wegtyp, const bridge_desc_t* desc)
{
	if (wegtyp != desc->get_waytype()) {
		return "";
	}
	slist_tpl<grund_t*> part_list;
	slist_tpl<grund_t*> end_list;
	const char* msg;

	grund_t* start = welt->lookup(pos_start);
	if (!start) {
		// no ground???
		return "A bridge must start on a way!";
	}
	if (!start->find<bruecke_t>()) {
		// need to click on a bridge
		return "A bridge must start on a way!";
	}

	waytype_t check_wegtyp = wegtyp == powerline_wt ? invalid_wt : wegtyp;
	if (start->ist_karten_boden()) {
		end_list.append(start);
	}
	else {
		part_list.append(start);
	}

	if (!player->is_public_service()) {
		bruecke_t* br = start->find<bruecke_t>();
		if (!br->get_owner_nr() == player->get_player_nr()) {
			// since bridges are fully owned by one player, check only one tile
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		if (br->get_desc() == desc) {
			// same bridge already ...
			return NULL;
		}

	}

	// one direction
	grund_t* from = start;
	while (1) {
		ribi_t::ribi r = wegtyp == powerline_wt ? from->get_leitung()->get_ribi() : from->get_weg_nr(0)->get_ribi_unmasked();
		ribi_t::ribi dir = r & ribi_t::northeast;

		grund_t* to;
		if (from->get_neighbour(to, check_wegtyp, dir) && to->ist_bruecke()) {
			if (to->ist_karten_boden()) {
				end_list.append(to);
				break;
			}
			else {
				part_list.append(to);
			}
			from = to;
		}
		else {
			break;
		}
	}
	// other direction
	from = start;
	while (1) {
		ribi_t::ribi r = wegtyp == powerline_wt ? from->get_leitung()->get_ribi() : from->get_weg_nr(0)->get_ribi_unmasked();
		ribi_t::ribi dir = r & ribi_t::southwest;

		grund_t* to;
		if (from->get_neighbour(to, check_wegtyp, dir) && to->ist_bruecke()) {
			if (to->ist_karten_boden()) {
				end_list.append(to);
				break;
			}
			else {
				part_list.insert(part_list.begin(), to);
			}
			from = to;
		}
		else {
			break;
		}
	}

	// Check whether we can replace the ends
	for (grund_t *& gr : end_list) {
		const slope_t::type slope = gr->get_grund_hang();
		if (desc->get_end(slope, slope, gr->get_weg_hang()) == IMG_EMPTY) {
			// wrong starting slope
			return "bridge is too high for its type!";
		}
	}

	// Check whether the bridge would be too long
	if (desc->get_max_length() > 0 && part_list.get_count() > desc->get_max_length()) {
		return "Bridge is too long for this type!\n";
	}

	// Check whether the bridge would be too tall
	if (desc->get_max_height() > 0) {
		for (grund_t *& bg : part_list) {
			// check for height
			grund_t* gr = welt->lookup_kartenboden(bg->get_pos().get_2d());
			if (gr && bg->get_hoehe() - gr->get_hoehe() > desc->get_max_height()) {
				return "bridge is too high for its type!";
			}
		}
	}

	part_list.append_list(end_list);
	const way_desc_t* way_desc = way_builder_t::weg_search(desc->get_waytype(), desc->get_topspeed(), welt->get_timeline_year_month(), type_flat);
	for(grund_t *&gr : part_list) {
		bruecke_t *br = gr->find<bruecke_t>();
		const bridge_desc_t *br_desc = br->get_desc();

		if (wegtyp != powerline_wt) {
			weg_t* weg = gr->get_weg(wegtyp);
			weg->set_owner(player);
			weg->set_desc(way_desc);
			weg->set_max_speed(desc->get_topspeed());
			// respect max speed of catenary
			wayobj_t const* const wo = gr->get_wayobj(desc->get_wtyp());
			if (wo  &&  wo->get_desc()->get_topspeed() < desc->get_topspeed()) {
				weg->set_max_speed(wo->get_desc()->get_topspeed());
			}
		}
		player_t::add_maintenance(br->get_owner(), -br_desc->get_maintenance(), desc->get_finance_waytype());
		br->set_owner(player);
		br->set_desc(desc);
		player_t::add_maintenance(player, desc->get_maintenance(), desc->get_finance_waytype());
		player_t::book_construction_costs(player, -desc->get_price(), br->get_pos().get_2d(), desc->get_waytype());
		gr->calc_image();

		// remove old pillars
		grund_t* gr_bottom = welt->lookup_kartenboden(gr->get_pos().get_2d());
		while (obj_t* const p = gr_bottom->find<pillar_t>()) {
			p->cleanup(p->get_owner());
			delete p;
		}

		// Now for new pillars
		sint16 height = gr->get_hoehe() - gr_bottom->get_pos().z;
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp);
		if (desc->get_pillar() > 0 && !gr->ist_karten_boden()) {
			koord zv((ribi_t::ribi)(ribi & ribi_t::northeast));
			// make a new pillar here
			koord3d pos = gr->get_pos();
			if (desc->get_pillar() == 1  ||  (pos.x * zv.x + pos.y * zv.y) % desc->get_pillar() == 0) {
				//DBG_MESSAGE("bool bridge_builder_t::renovate()","h1=%i, h2=%i",pos.z,gr->get_pos().z);
				while (height-- > 0) {
					if (TILE_HEIGHT_STEP * height <= 127) {
						// eventual more than one part needed, if it is too high ...
						gr_bottom->obj_add(new pillar_t(gr_bottom->get_pos(), player, desc, desc->get_pillar(ribi), TILE_HEIGHT_STEP * height));
					}
				}
			}
		}

	}
	welt->set_dirty();

	return NULL;
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
			simachievements_t::set_achievement(ACH_TOOL_REMOVE_BUSY_BRIDGE);
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
