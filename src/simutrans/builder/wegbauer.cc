/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>

#include "../simdebug.h"
#include "../world/simworld.h"
#include "../tool/simtool.h"
#include "../simmesg.h"
#include "../simintr.h"
#include "../player/simplay.h"
#include "../world/simplan.h"
#include "../obj/depot.h"

#include "wegbauer.h"
#include "brueckenbauer.h"
#include "tunnelbauer.h"

#include "../descriptor/way_desc.h"
#include "../descriptor/tunnel_desc.h"
#include "../descriptor/building_desc.h"
#include "../descriptor/crossing_desc.h"

#include "../obj/way/strasse.h"
#include "../obj/way/schiene.h"
#include "../obj/way/monorail.h"
#include "../obj/way/maglev.h"
#include "../obj/way/narrowgauge.h"
#include "../obj/way/kanal.h"
#include "../obj/way/runway.h"
#include "../ground/brueckenboden.h"
#include "../ground/monorailboden.h"
#include "../ground/tunnelboden.h"
#include "../ground/grund.h"
#include "../ground/wasser.h"

#include "../dataobj/environment.h"
#include "../dataobj/route.h"
#include "../dataobj/marker.h"
#include "../dataobj/translator.h"
#include "../dataobj/scenario.h"
#include "../dataobj/pakset_manager.h"

#include "../utils/simrandom.h"

// binary heap, since we only need insert and pop
#include "../tpl/binary_heap_tpl.h" // fastest

#include "../obj/field.h"
#include "../obj/gebaeude.h"
#include "../obj/bruecke.h"
#include "../obj/tunnel.h"
#include "../obj/crossing.h"
#include "../obj/leitung2.h"
#include "../obj/groundobj.h"
#include "../obj/wayobj.h"

#include "../vehicle/simtestdriver.h"

#include "../tpl/array_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

#include "../gui/minimap.h" // for debugging
#include "../gui/tool_selector.h"
#include "../gui/messagebox.h"

#ifdef DEBUG_ROUTES
#include "../sys/simsys.h"
#endif

// built bridges automatically
//#define AUTOMATIC_BRIDGES

// built tunnels automatically
//#define AUTOMATIC_TUNNELS

// lookup also return route and take the better of the two
#define REVERSE_CALC_ROUTE_TOO

karte_ptr_t way_builder_t::welt;

const way_desc_t *way_builder_t::leitung_desc = NULL;

static stringhashtable_tpl <const way_desc_t *> desc_table;


static void set_default(way_desc_t const*& def, waytype_t const wtyp, systemtype_t const system_type = type_flat, sint32 const speed_limit = 1)
{
	def = way_builder_t::weg_search(wtyp, speed_limit, 0, system_type);
	if (def == NULL) {
		def = way_builder_t::weg_search(wtyp, 1, 0, type_all);
	}
}


bool way_builder_t::successfully_loaded()
{
	// some defaults to avoid hardcoded values
	set_default(strasse_t::default_strasse,         road_wt,        type_flat, 50);
	if(  strasse_t::default_strasse == NULL ) {
		dbg->fatal( "way_builder_t::successfully_loaded()", "No road found at all!" );
	}

	set_default(schiene_t::default_schiene,         track_wt,       type_flat, 80);
	set_default(monorail_t::default_monorail,       monorail_wt,    type_elevated); // Only elevated?
	set_default(maglev_t::default_maglev,           maglev_wt,      type_elevated); // Only elevated?
	set_default(narrowgauge_t::default_narrowgauge, narrowgauge_wt);
	set_default(kanal_t::default_kanal,             water_wt,       type_all); // Also find hidden rivers.
	set_default(runway_t::default_runway,           air_wt,         type_runway);
	set_default(way_builder_t::leitung_desc,          powerline_wt);

	return true;
}


bool way_builder_t::register_desc(way_desc_t *desc)
{
	if(  const way_desc_t *old_desc = desc_table.remove(desc->get_name())  ) {
		pakset_manager_t::doubled( "way", desc->get_name() );
		tool_t::general_tool.remove( old_desc->get_builder() );
		delete old_desc->get_builder();
		delete old_desc;
	}

	if(  desc->get_cursor()->get_image_id(1)!=IMG_EMPTY  ) {
		// add the tool
		tool_build_way_t *tool = new tool_build_way_t();
		tool->set_icon( desc->get_cursor()->get_image_id(1) );
		tool->cursor = desc->get_cursor()->get_image_id(0);
		tool->set_default_param(desc->get_name());
		tool_t::general_tool.append( tool );
		desc->set_builder( tool );
	}
	else {
		desc->set_builder( NULL );
	}
	desc_table.put(desc->get_name(), desc);
	return true;
}


const vector_tpl<const way_desc_t *>&  way_builder_t::get_way_list(const waytype_t wtyp, systemtype_t styp)
{
	static vector_tpl<const way_desc_t *> dummy;
	dummy.clear();
	const uint16 time = welt->get_timeline_year_month();
	for(auto const& i : desc_table) {
		way_desc_t const* const test = i.value;
		if( test->get_wtyp()==wtyp  &&  test->get_styp()== styp  &&  test->is_available(time)  &&  test->get_builder() ) {
			dummy.append(test);
		}
	}
	return dummy;
}


/**
 * Finds a way with a given speed limit for a given waytype
 * It finds:
 *  - the slowest way, as fast as speed limit
 *  - if no way faster than speed limit, the fastest way.
 * The timeline is also respected.
 */
const way_desc_t* way_builder_t::weg_search(const waytype_t wtyp, const sint32 speed_limit, const uint16 time, const systemtype_t system_type)
{
	const way_desc_t* best = NULL;
	bool best_allowed = false; // Does the best way fulfil the timeline?
	for(auto const& i : desc_table) {
		way_desc_t const* const test = i.value;
		if(  ((test->get_wtyp()==wtyp  &&
			(test->get_styp()==system_type  ||  system_type==type_all))  ||  (test->get_wtyp()==track_wt  &&  test->get_styp()==type_tram  &&  wtyp==tram_wt))
			&&  test->get_cursor()->get_image_id(1)!=IMG_EMPTY  ) {
			bool test_allowed = test->get_intro_year_month()<=time  &&  time<test->get_retire_year_month();
				if(  !best_allowed  ||  time==0  ||  test_allowed  ) {
					if(  best==NULL  ||
						( best->get_topspeed() <  test->get_topspeed()  &&  test->get_topspeed() <=     speed_limit  )    || // closer to desired speed (from the low end)
						(     speed_limit      <  best->get_topspeed()  &&  test->get_topspeed() <   best->get_topspeed()) || // respects speed_limit better
						( time!=0  &&  !best_allowed  &&  test_allowed)                                                       // current choice is actually not really allowed, timewise
						) {
							best = test;
							best_allowed = test_allowed;
					}
				}
		}
	}
	return best;
}



const way_desc_t *way_builder_t::get_earliest_way(const waytype_t wtyp)
{
	const way_desc_t *desc = NULL;
	for(auto const& i : desc_table) {
		way_desc_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  (desc==NULL  ||  test->get_intro_year_month()<desc->get_intro_year_month())  ) {
			desc = test;
		}
	}
	return desc;
}



const way_desc_t *way_builder_t::get_latest_way(const waytype_t wtyp)
{
	const way_desc_t *desc = NULL;
	for(auto const& i : desc_table) {
		way_desc_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  (desc==NULL  ||  test->get_retire_year_month()>desc->get_retire_year_month())  ) {
			desc = test;
		}
	}
	return desc;
}


// ture if the way is available with timely
bool way_builder_t::waytype_available( const waytype_t wtyp, uint16 time )
{
	if(  time==0  ) {
		return true;
	}

	for(auto const& i : desc_table) {
		way_desc_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  test->get_intro_year_month()<=time  &&  test->get_retire_year_month()>time  ) {
			return true;
		}
	}
	return false;
}



const way_desc_t * way_builder_t::get_desc(const char * way_name, const uint16 time)
{
//DBG_MESSAGE("way_builder_t::get_desc","return desc for %s in (%i)",way_name, time/12);
	const way_desc_t *desc = desc_table.get(way_name);
	if(  desc  &&  desc->is_available(time)  ) {
		return desc;
	}
	return NULL;
}



// generates timeline message
void way_builder_t::new_month()
{
	const uint16 current_month = welt->get_timeline_year_month();
	if(current_month!=0) {
		// check, what changed
		slist_tpl <const way_desc_t *> matching;
		for(auto const& i : desc_table) {
			way_desc_t const* const desc = i.value;
			cbuffer_t buf;

			const uint16 intro_month = desc->get_intro_year_month();
			if(intro_month == current_month) {
				buf.printf( translator::translate("way %s now available:\n"), translator::translate(desc->get_name()) );
				welt->get_message()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,desc->get_image_id(5,0));
			}

			const uint16 retire_month = desc->get_retire_year_month();
			if(retire_month == current_month) {
				buf.printf( translator::translate("way %s cannot longer used:\n"), translator::translate(desc->get_name()) );
				welt->get_message()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,desc->get_image_id(5,0));
			}
		}

	}
}


static bool compare_ways(const way_desc_t* a, const way_desc_t* b)
{
	int cmp = a->get_topspeed() - b->get_topspeed();
	if(cmp==0) {
		cmp = (int)a->get_intro_year_month() - (int)b->get_intro_year_month();
	}
	if(cmp==0) {
		cmp = strcmp(a->get_name(), b->get_name());
	}
	return cmp<0;
}


/**
 * Fill menu with icons of given waytype, return number of added entries
 */
void way_builder_t::fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, const systemtype_t styp, sint16 /*ok_sound*/)
{
	// check if scenario forbids this
	const waytype_t rwtyp = wtyp!=track_wt  || styp!=type_tram  ? wtyp : tram_wt;
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_WAY | GENERAL_TOOL, rwtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();

	// list of matching types (sorted by speed)
	vector_tpl<const way_desc_t*> matching;

	for(auto const& i : desc_table) {
		way_desc_t const* const desc = i.value;
		if (  desc->get_styp()==styp &&  desc->get_wtyp()==wtyp  &&  desc->get_builder()  &&  desc->is_available(time)  ) {
				matching.append(desc);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_ways);

	// now add sorted ways ...
	for(way_desc_t const* const i : matching) {
		tool_selector->add_tool_selector(i->get_builder());
	}
}


/** allow for railroad crossing
 */
bool way_builder_t::check_crossing(const koord zv, const grund_t *bd, waytype_t wtyp0, const player_t *player) const
{
	const waytype_t wtyp = wtyp0==tram_wt ? track_wt : wtyp0;
	// nothing to cross here
	if (!bd->hat_wege()) {
		return true;
	}
	// no triple crossings please
	if (bd->has_two_ways() && !bd->hat_weg(wtyp)) {
		return false;
	}
	const weg_t *w = bd->get_weg_nr(0);
	// index of our wtype at the tile (must exists due to triple-crossing-check above)
	const uint8 iwtyp = w->get_waytype() != wtyp;
	// get the other way
	if(iwtyp==0) {
		w = bd->get_weg_nr(1);
		// no other way here
		if (w==NULL) {
			return true;
		}
	}
	// special case: tram track on road
	if ( (wtyp==road_wt  &&  w->get_waytype()==track_wt  &&  w->get_desc()->get_styp()==type_tram)  ||
		     (wtyp0==tram_wt  &&  w->get_waytype()==road_wt) ) {
		return true;
	}
	// right owner of the other way
	// exception: allow if we want to build road and road already exists, since this is passable for us
	if(!check_owner(w->get_owner(),player)  &&  ! (wtyp==road_wt  &&  bd->has_two_ways()) ) {
		return false;
	}
	// check for existing crossing
	crossing_t *cr = bd->find<crossing_t>();
	if (cr) {
		// index of the waytype in ns-direction at the crossing
		const uint8 ns_way = cr->get_dir();
		// only cross with the right direction
		return (ns_way==iwtyp ? ribi_t::is_straight_ns(ribi_type(zv)) : ribi_t::is_straight_ew(ribi_type(zv)));
	}
	// no crossings in tunnels
	if((bautyp & tunnel_flag)!=0  || bd->ist_tunnel()) {
		return false;
	}
	// no crossings on elevated ways
	if((bautyp & elevated_flag)!=0  ||  bd->get_typ()==grund_t::monorailboden) {
		return false;
	}
	// crossing available and ribis ok
	if(crossing_logic_t::get_crossing(wtyp, w->get_waytype(), desc ? desc->get_topspeed() : 0, w->get_max_speed(), welt->get_timeline_year_month())!=NULL) {
		ribi_t::ribi w_ribi = w->get_ribi_unmasked();
		// it is our way we want to cross: can we built a crossing here?
		// both ways must be straight and no ends
		return  ribi_t::is_straight(w_ribi)
					&&  !ribi_t::is_single(w_ribi)
					&&  ribi_t::is_straight(ribi_type(zv))
				&&  (w_ribi&ribi_type(zv))==0;
	}
	// cannot build crossing here
	return false;
}


/** crossing of powerlines, or no powerline
 */
bool way_builder_t::check_powerline(const koord zv, const grund_t *bd) const
{
	if(zv==koord(0,0)) {
		return true;
	}
	leitung_t* lt = bd->find<leitung_t>();
	if(lt!=NULL) {
		ribi_t::ribi lt_ribi = lt->get_ribi();
		// it is our way we want to cross: can we built a crossing here?
		// both ways must be straight and no ends
		return
		  ribi_t::is_straight(lt_ribi)
		  &&  !ribi_t::is_single(lt_ribi)
		  &&  ribi_t::is_straight(ribi_type(zv))
		  &&  (lt_ribi&ribi_type(zv))==0
		  &&  !bd->ist_tunnel();
	}
	// check for transformer
	if (bd->find<pumpe_t>() != NULL || bd->find<senke_t>()  != NULL) {
		return false;
	}
	// ok, there is not high power transmission stuff going on here
	return true;
}


// allowed slope?
bool way_builder_t::check_slope( const grund_t *from, const grund_t *to )
{
	const koord from_pos=from->get_pos().get_2d();
	const koord to_pos=to->get_pos().get_2d();
	const koord zv=to_pos-from_pos;

	if(  !desc->has_double_slopes()
		&&  (    (from->get_weg_hang()  &&  !is_one_high(from->get_weg_hang()))
		      ||   (to->get_weg_hang()  &&    !is_one_high(to->get_weg_hang()))  )  ) {
		return false;
	}

	if(from==to) {
		if(!slope_t::is_way(from->get_weg_hang())) {
			return false;
		}
	}
	else {
		if(from->get_weg_hang()  &&  ribi_t::doubles(ribi_type(from->get_weg_hang()))!=ribi_t::doubles(ribi_type(zv))) {
			return false;
		}
		if(to->get_weg_hang()  &&  ribi_t::doubles(ribi_type(to->get_weg_hang()))!=ribi_t::doubles(ribi_type(zv))) {
			return false;
		}
	}

	return true;
}


// allowed owner?
bool way_builder_t::check_owner( const player_t *player1, const player_t *player2 ) const
{
	// unowned, mine or public property or superuser ... ?
	return player1==NULL  ||  player1==player2  ||  player1==welt->get_public_player()  ||  player2==welt->get_public_player();
}


/* do not go through depots, station buildings etc. ...
 * direction results from layout
 */
bool way_builder_t::check_building( const grund_t *to, const koord dir ) const
{
	if(dir==koord(0,0)) {
		return true;
	}
	if(to->obj_count()<=0) {
		return true;
	}

	// first find all kind of buildings
	gebaeude_t *gb = to->find<gebaeude_t>();
	if(gb==NULL) {
		// but depots might be overlooked ...
		depot_t* depot = to->get_depot();
		// no road to tram depot and vice-versa
		if (depot) {
			if ( (waytype_t)(bautyp&bautyp_mask) != depot->get_waytype() ) {
				return false;
			}
		}
		gb = depot;
	}
	// check, if we may enter
	if(gb) {
		// now check for directions
		uint8 layouts = gb->get_tile()->get_desc()->get_all_layouts();
		uint8 layout = gb->get_tile()->get_layout();
		ribi_t::ribi r = ribi_type(dir);
		if(  layouts&1  ) {
			return false;
		}
		if(  layouts==4  ) {
			return  r == ribi_t::layout_to_ribi[layout];
		}
		return ribi_t::is_straight( r | ribi_t::doubles(ribi_t::layout_to_ribi[layout&1]) );
	}
	return true;
}


/** This is the core routine for the way search
 * it will check
 * A) allowed step
 * B) if allowed, calculate the cost for the step from from to to
 */
bool way_builder_t::is_allowed_step(const grund_t *from, const grund_t *to, sint32 *costs, bool is_upperlayer ) const
{
	const koord from_pos=from->get_pos().get_2d();
	const koord to_pos=to->get_pos().get_2d();
	const koord zv=to_pos-from_pos;
	// fake empty elevated tiles
	static monorailboden_t to_dummy(koord3d::invalid, slope_t::flat);
	static monorailboden_t from_dummy(koord3d::invalid, slope_t::flat);

	if(bautyp==luft  &&  (from->get_grund_hang()+to->get_grund_hang()!=0  ||  (from->hat_wege()  &&  from->hat_weg(air_wt)==0)  ||  (to->hat_wege()  &&  to->hat_weg(air_wt)==0))) {
		// absolutely no slopes for runways, neither other ways
		return false;
	}

	bool to_flat = false; // to tile will be flattened
	if(from==to) {
		if((bautyp&tunnel_flag)  &&  !slope_t::is_way(from->get_weg_hang())) {
			return false;
		}
	}
	else {
		// check slopes
		bool ok_slope = from->get_weg_hang() == slope_t::flat  ||  ribi_t::doubles(ribi_type(from->get_weg_hang()))==ribi_t::doubles(ribi_type(zv));
		ok_slope &= to->get_weg_hang() == slope_t::flat  ||  ribi_t::doubles(ribi_type(to->get_weg_hang()))==ribi_t::doubles(ribi_type(zv));

		// try terraforming
		if (!ok_slope) {
			uint8 dummy,to_slope;
			if (  (bautyp & terraform_flag) != 0  &&  from->ist_natur()  &&  to->ist_natur()  &&  check_terraforming(from,to,&dummy,&to_slope) ) {
				to_flat = to_slope == slope_t::flat;
			}
			else {
				// slopes not ok and no terraforming possible
				return false;
			}
		}
	}

	// ok, slopes are ok
	bool ok = true;

	// check scenario conditions
	if (welt->get_scenario()->is_work_allowed_here(player_builder, (bautyp&tunnel_flag ? TOOL_BUILD_TUNNEL : TOOL_BUILD_WAY)|GENERAL_TOOL, bautyp&bautyp_mask, to->get_pos()) != NULL) {
		return false;
	}

	// universal check for elevated things ...
	if(bautyp&elevated_flag) {
		if(  is_upperlayer  ) {
			if(  (to->get_typ() != grund_t::monorailboden ||  to->get_weg_nr(0)->get_desc()->get_wtyp()!=desc->get_wtyp()  ||  !check_owner(to->obj_bei(0)->get_owner(),player_builder) )  ||  (from->get_typ() != grund_t::monorailboden ||  from->get_weg_nr(0)->get_desc()->get_wtyp()!=desc->get_wtyp()  ||  !check_owner(from->obj_bei(0)->get_owner(),player_builder) )  ) {
				return false;
			}
		}
		else {
			if(  to->hat_weg(air_wt)  ||  welt->lookup_hgt( to_pos ) < welt->get_water_hgt( to_pos )  ||  !check_powerline( zv, to )  ||  (!to->ist_karten_boden()  &&  to->get_typ() != grund_t::monorailboden)  ||  to->get_typ() == grund_t::brueckenboden  ||  to->get_typ() == grund_t::tunnelboden  ) {
				// no suitable ground below!
				return false;
			}
			gebaeude_t *gb = to->find<gebaeude_t>();
			if(gb==NULL) {
				// but depots might be overlooked ...
				gb = to->get_depot();
			}
			if(gb) {
				// no halt => citybuilding => do not touch
				// also check for too high buildings ...
				if(!check_owner(gb->get_owner(),player_builder)  ||  gb->get_tile()->get_background(0,1,0)!=IMG_EMPTY) {
					return false;
				}
				// building above houses is expensive ... avoid it!
				*costs += 4;
			}
			// absolutely nothing allowed here for set which want double clearance
			if(  welt->get_settings().get_way_height_clearance()==2  &&  welt->lookup( to->get_pos()+koord3d(0,0,1) )  ) {
				return false;
			}
			// up to now 'to' and 'from' referred to the ground one height step below the elevated way
			// now get the grounds at the right height
			koord3d pos = to->get_pos() + koord3d( 0, 0, welt->get_settings().get_way_height_clearance() );
			grund_t *to2 = welt->lookup(pos);
			if(to2) {
				if(to2->get_weg_nr(0)) {
					// already an elevated ground here => it will have always a way object, that indicates ownership
					ok = to2->get_typ()==grund_t::monorailboden  &&  check_owner(to2->obj_bei(0)->get_owner(),player_builder);
					ok &= to2->get_weg_nr(0)->get_desc()->get_wtyp()==desc->get_wtyp();
				}
				else {
					ok = to2->find<leitung_t>()==NULL;
				}
				if (!ok) {
					return false;
				}
				to = to2;
			}
			else {
				// simulate empty elevated tile
				to_dummy.set_pos(pos);
				to_dummy.set_grund_hang(to->get_grund_hang());
				to = &to_dummy;
			}

			pos = from->get_pos() + koord3d( 0, 0, env_t::pak_height_conversion_factor );
			grund_t *from2 = welt->lookup(pos);
			if(from2) {
				from = from2;
			}
			else {
				// simulate empty elevated tile
				from_dummy.set_pos(pos);
				from_dummy.set_grund_hang(from->get_grund_hang());
				from = &from_dummy;
			}
			// now 'from' and 'to' point to grounds at the right height
		}
	}

	if(  welt->get_settings().get_way_height_clearance()==2  ) {
		// cannot build if conversion factor 2, we aren't powerline and way with maximum speed > 0 or powerline 1 tile below
		grund_t *to2 = welt->lookup( to->get_pos() + koord3d(0, 0, -1) );
		if(  to2 && (((bautyp&bautyp_mask)!=leitung  &&  to2->get_weg_nr(0)  &&  to2->get_weg_nr(0)->get_desc()->get_topspeed()>0) || to2->get_leitung())  ) {
			return false;
		}
		// tile above cannot have way unless we are a way (not powerline) with a maximum speed of 0, or be surface if we are underground
		to2 = welt->lookup( to->get_pos() + koord3d(0, 0, 1) );
		if(  to2  &&  ((to2->get_weg_nr(0)  &&  (desc->get_topspeed()>0  ||  (bautyp&bautyp_mask)==leitung))  ||  (bautyp & tunnel_flag) != 0)  ) {
			return false;
		}
	}

	// universal check for depots/stops/...
	if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
		return false;
	}

	// universal check for bridges: enter bridges in bridge direction
	if( to->get_typ()==grund_t::brueckenboden ) {
		ribi_t::ribi br = ribi_type(zv);
		br  = to->hat_wege()    ? to->get_weg_nr(0)->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::none;
		br |= to->get_leitung() ? to->get_leitung()->get_ribi()          : (ribi_t::ribi)ribi_t::none;
		if(!ribi_t::is_straight(br)) {
			return false;
		}
	}
	if( from->get_typ()==grund_t::brueckenboden ) {
		ribi_t::ribi br = ribi_type(zv);
		br  = from->hat_wege()    ? from->get_weg_nr(0)->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::none;
		br |= from->get_leitung() ? from->get_leitung()->get_ribi()          : (ribi_t::ribi)ribi_t::none;
		if(!ribi_t::is_straight(br)) {
			return false;
		}
	}

	// universal check: do not switch to tunnel through cliffs!
	if( from->get_typ() == grund_t::tunnelboden  &&  to->get_typ() != grund_t::tunnelboden  &&  !from->ist_karten_boden() ) {
		return false;
	}
	if( to->get_typ()==grund_t::tunnelboden  &&  from->get_typ() != grund_t::tunnelboden   &&  !to->ist_karten_boden() ) {
		return false;
	}

	// universal check for crossings
	if (to!=from  &&  (bautyp&bautyp_mask)!=leitung) {
		waytype_t const wtyp = (bautyp == river) ? water_wt : (waytype_t)(bautyp & bautyp_mask);
		if(!check_crossing(zv,to,wtyp,player_builder)  ||  !check_crossing(-zv,from,wtyp,player_builder)) {
			return false;
		}
	}

	// universal check for building under powerlines
	if ((bautyp&bautyp_mask)!=leitung) {
		if (!check_powerline(zv,to)  ||  !check_powerline(-zv,from)) {
			return false;
		}
	}

	bool fundament = to->get_typ()==grund_t::fundament;

	// now check way specific stuff
	settings_t const& s = welt->get_settings();
	switch(bautyp&bautyp_mask) {

		case strasse:
		{
			const weg_t *str=to->get_weg(road_wt);

			// we allow connection to any road
			ok = (str  ||  !fundament)  &&  !to->is_water();
			if(!ok) {
				return false;
			}
			// check for end/start of bridge or tunnel
			// fail if no proper way exists, or the way's ribi are not 0 and are not matching the slope type
			ribi_t::ribi test_ribi = (str ? str->get_ribi_unmasked() : 0) | ribi_type(zv);
			if(to->get_weg_hang()!=to->get_grund_hang()  &&  (str==NULL  ||  !(ribi_t::is_straight(test_ribi) || test_ribi==0 ))) {
				return false;
			}
			// calculate costs
			*costs = str ? 0 : s.way_count_straight;
			if((str==NULL  &&  to->hat_wege())  ||  (str  &&  to->has_two_ways())) {
				*costs += 4; // avoid crossings
			}
			if(to->get_weg_hang()!=0  &&  !to_flat) {
				*costs += s.way_count_slope;
			}
		}
		break;

		case schiene:
		default:
		{
			const weg_t *sch=to->get_weg(desc->get_wtyp());
			// extra check for AI construction (not adding to existing tracks!)
			if((bautyp&bot_flag)!=0  &&  (sch  ||  to->get_halt().is_bound())) {
				return false;
			}
			// ok, regular construction here
			// if no way there: check for right ground type, otherwise check owner
			ok = sch==NULL  ?  (!fundament  &&  !to->is_water())  :  check_owner(sch->get_owner(),player_builder);
			if(!ok) {
				return false;
			}
			// check for end/start of bridge or tunnel
			// fail if no proper way exists, or the way's ribi are not 0 and are not matching the slope type
			ribi_t::ribi test_ribi = (sch ? sch->get_ribi_unmasked() : 0) | ribi_type(zv);
			if(to->get_weg_hang()!=to->get_grund_hang()  &&  (sch==NULL  ||  !(ribi_t::is_straight(test_ribi) || test_ribi==0 ))) {
				return false;
			}
			// calculate costs
			*costs = s.way_count_straight;
			if (!sch) *costs += 1; // only prefer existing rails a little
			if((sch  &&  to->has_two_ways())  ||  (sch==NULL  &&  to->hat_wege())) {
				*costs += 4; // avoid crossings
			}
			if(to->get_weg_hang()!=0  &&  !to_flat) {
				*costs += s.way_count_slope;
			}
		}
		break;

		case schiene_tram: // Dario: Tramway
		{
			const weg_t *sch=to->get_weg(track_wt);
			// roads are checked in check_crossing
			// if no way there: check for right ground type, otherwise check owner
			ok = sch==NULL  ?  (!fundament  &&  !to->is_water())  :  check_owner(sch->get_owner(),player_builder);
			// tram track allowed in road tunnels, but only along existing roads / tracks
			if(from!=to) {
				if(from->ist_tunnel()) {
					const ribi_t::ribi ribi = from->get_weg_ribi_unmasked(road_wt)  |  from->get_weg_ribi_unmasked(track_wt)  |  ribi_t::doubles(ribi_type(from->get_grund_hang()));
					ok = ok && ((ribi & ribi_type(zv))==ribi_type(zv));
				}
				if(to->ist_tunnel()) {
					const ribi_t::ribi ribi = to->get_weg_ribi_unmasked(road_wt)  |  to->get_weg_ribi_unmasked(track_wt)  |  ribi_t::doubles(ribi_type(to->get_grund_hang()));
					ok = ok && ((ribi & ribi_type(-zv))==ribi_type(-zv));
				}
			}
			if(ok) {
				// calculate costs
				*costs = s.way_count_straight;
				if (!to->hat_weg(track_wt)) *costs += 1; // only prefer existing rails a little
				// prefer own track
				if(to->hat_weg(road_wt)) {
					*costs += s.way_count_straight;
				}
				if(to->get_weg_hang()!=0  &&  !to_flat) {
					*costs += s.way_count_slope;
				}
			}
		}
		break;

		case leitung:
			ok = !to->is_water()  &&  (to->get_weg(air_wt)==NULL);
			ok &= !(to->ist_tunnel() && to->hat_wege());
			if(to->get_weg_nr(0)!=NULL) {
				// only 90 deg crossings, only a single way
				ribi_t::ribi w_ribi= to->get_weg_nr(0)->get_ribi_unmasked();
				ok &= ribi_t::is_straight(w_ribi)  &&  !ribi_t::is_single(w_ribi)  &&  ribi_t::is_straight(ribi_type(zv))  &&  (w_ribi&ribi_type(zv))==0;
			}
			if(to->has_two_ways()) {
				// only 90 deg crossings, only for trams ...
				ribi_t::ribi w_ribi= to->get_weg_nr(1)->get_ribi_unmasked();
				ok &= ribi_t::is_straight(w_ribi)  &&  !ribi_t::is_single(w_ribi)  &&  ribi_t::is_straight(ribi_type(zv))  &&  (w_ribi&ribi_type(zv))==0;
			}
			// do not connect to other powerlines
			{
				leitung_t *lt = to->get_leitung();
				ok &= (lt==NULL)  ||  check_owner(player_builder, lt->get_owner());
			}

			if(to->get_typ()!=grund_t::tunnelboden) {
				// only fields are allowed
				if(to->get_typ()==grund_t::fundament) {
					ok &= to->find<field_t>()!=NULL;
				}
				// no bridges and monorails here in the air
				ok &= (welt->access(to_pos)->get_boden_in_hoehe(to->get_pos().z+1)==NULL);
			}

			// calculate costs
			if(ok) {
				*costs = s.way_count_straight;
				if(  !to->get_leitung()  ) {
					// extra malus for not following an existing line or going on ways
					*costs += s.way_count_double_curve + (to->hat_wege() ? 8 : 0); // prefer existing powerlines
				}
			}
		break;

		case wasser:
		{
			const weg_t *canal = to->get_weg(water_wt);
			// if no way there: check for right ground type, otherwise check owner
			ok = canal  ||  !fundament;
			// calculate costs
			if(ok) {
				*costs = to->is_water() ||  canal  ? s.way_count_straight : s.way_count_leaving_road; // prefer water very much
				if(to->get_weg_hang()!=0  &&  !to_flat) {
					*costs += s.way_count_slope * 2;
				}
			}
			break;
		}

		case river:
			if(  to->is_water()  ) {
				ok = true;
				// do not care while in ocean
				*costs = 1;
			}
			else {
				// only downstream
				ok = from->get_pos().z>=to->get_pos().z  &&  (to->hat_weg(water_wt)  ||  !to->hat_wege());
				// calculate costs
				if(ok) {
					// prefer existing rivers:
					*costs = to->hat_weg(water_wt) ? 10 : 10+simrand(s.way_count_90_curve);
					if(to->get_weg_hang()!=0  &&  !to_flat) {
						*costs += s.way_count_slope * 10;
					}
				}
			}
			break;

		case luft: // runway
			{
				const weg_t *w = to->get_weg(air_wt);
				if(  w  &&  w->get_desc()->get_styp()==type_runway  &&  desc->get_styp()!=type_runway  &&  ribi_t::is_single(w->get_ribi_unmasked())  ) {
					// cannot go over the end of a runway with a taxiway
					return false;
				}
				ok = !to->is_water() && (w  ||  !to->hat_wege())  &&  to->find<leitung_t>()==NULL  &&  !fundament;
				// calculate costs
				*costs = s.way_count_straight;
			}
			break;
	}
	return ok;
}

bool way_builder_t::check_terraforming( const grund_t *from, const grund_t *to, uint8* new_from_slope, uint8* new_to_slope) const
{
	// only for normal green tiles
	const slope_t::type from_slope = from->get_weg_hang();
	const slope_t::type to_slope = to->get_weg_hang();
	const sint8 from_hgt = from->get_hoehe();
	const sint8 to_hgt = to->get_hoehe();
	// we may change slope of a tile if it is sloped already
	if(  (from_slope == slope_t::flat  ||  from->get_hoehe() == welt->get_water_hgt( from->get_pos().get_2d() ))
	  &&  (to_slope == slope_t::flat  ||  to->get_hoehe() == welt->get_water_hgt( to->get_pos().get_2d() ))  ) {
		return false;
	}
	else if(  abs( from_hgt - to_hgt ) <= (desc->has_double_slopes() ? 2 : 1)  ) {
		// extra check for double heights
		if(  abs( from_hgt - to_hgt) == 2  &&  (welt->lookup( from->get_pos() - koord3d(0,0,2) ) != NULL  ||  welt->lookup( from->get_pos() + koord3d(0, 0, 2) ) != NULL
		  ||  welt->lookup( to->get_pos() - koord3d(0, 0, 2) ) != NULL  ||  welt->lookup( to->get_pos() + koord3d(0, 0, 2) ) != NULL)  ) {
			return false;
		}
		// monorail above / tunnel below
		if (welt->lookup(from->get_pos() - koord3d(0,0,1))!=NULL  ||  welt->lookup(from->get_pos() + koord3d(0,0,1))!=NULL
			||  welt->lookup(to->get_pos() - koord3d(0,0,1))!=NULL  ||  welt->lookup(to->get_pos() + koord3d(0,0,1))!=NULL) {
				return false;
		}
		// can safely change slope of at least one of the tiles
		if (new_from_slope == NULL) {
			return true;
		}
		// now calculate new slopes
		assert(new_from_slope);
		assert(new_to_slope);
		// direction of way
		const koord dir = (to->get_pos() - from->get_pos()).get_2d();
		sint8 start  = from_hgt * 2;
		sint8 middle = from_hgt * 2;
		sint8 end    = to_hgt * 2;
		// get 3 heights - start (min start from, but should be same), middle (average end from/average start to), end (min end to)
		if(  dir == koord::north  ) {
			start  += corner_sw(from_slope) + corner_se(from_slope);
			middle += corner_ne(from_slope) + corner_nw(from_slope);
			end    += corner_ne(to_slope) + corner_nw(to_slope);
		}
		else if(  dir == koord::east  ) {
			start  += corner_sw(from_slope) + corner_nw(from_slope);
			middle += corner_se(from_slope) + corner_ne(from_slope);
			end    += corner_se(to_slope) + corner_ne(to_slope);
		}
		else if(  dir == koord::south  ) {
			start  += corner_ne(from_slope) + corner_nw(from_slope);
			middle += corner_sw(from_slope) + corner_se(from_slope);
			end    += corner_sw(to_slope) + corner_se(to_slope);
		}
		else if(  dir == koord::west  ) {
			start  += corner_se(from_slope) + corner_ne(from_slope);
			middle += corner_sw(from_slope) + corner_nw(from_slope);
			end    += corner_sw(to_slope) + corner_nw(to_slope);
		}
		// work out intermediate height:
		if(  end == start  ) {
			middle = start;
		}
		else {
			//  end < start   to ist wegbar?assert from ist_wegbar:middle = end + 1
			//  end > start   to ist wegbar?assert from ist_wegbar:middle = end - 1
			if(  !slope_t::is_way( to_slope )  ) {
				middle = (start + end) / 2;
			}
		}
		// prevent middle being invalid
		if(  middle >> 1 > from_hgt + 2  ) {
			middle = (from_hgt + 2) * 2;
		}
		if(  middle >> 1 > to_hgt + 2  ) {
			middle = (to_hgt + 2) * 2;
		}
		if(  middle >> 1 < from_hgt  ) {
			middle = from_hgt * 2;
		}
		if(  middle >> 1 < to_hgt  ) {
			middle = to_hgt * 2;
		}
		const uint8 m_from = (middle >> 1) - from_hgt;
		const uint8 m_to = (middle >> 1) - to_hgt;

		// write middle heights
		if(  dir == koord::north  ) {
			*new_from_slope = encode_corners(corner_sw(from_slope), corner_se(from_slope), m_from, m_from);
			*new_to_slope =   encode_corners(m_to, m_to, corner_ne(to_slope), corner_nw(to_slope));
		}
		else if(  dir == koord::east  ) {
			*new_from_slope = encode_corners(corner_sw(from_slope), m_from, m_from, corner_nw(from_slope));
			*new_to_slope =   encode_corners(m_to, corner_se(to_slope), corner_ne(to_slope), m_to);
		}
		else if(  dir == koord::south  ) {
			*new_from_slope = encode_corners(m_from, m_from, corner_ne(from_slope), corner_nw(from_slope));
			*new_to_slope =   encode_corners(corner_sw(to_slope), corner_se(to_slope), m_to, m_to);
		}
		else if(  dir == koord::west  ) {
			*new_from_slope = encode_corners(m_from, corner_se(from_slope), corner_ne(from_slope), m_from);
			*new_to_slope =   encode_corners(corner_sw(to_slope), m_to, m_to, corner_nw(to_slope));
		}
		return true;
	}
	return false;
}

void way_builder_t::do_terraforming()
{
	uint32 last_terraformed = terraform_index.get_count();

	for(uint32 const i : terraform_index) { // index in route
		grund_t *from = welt->lookup(route[i]);
		uint8 from_slope = from->get_grund_hang();

		grund_t *to = welt->lookup(route[i+1]);
		uint8 to_slope = to->get_grund_hang();
		// calculate new slopes
		check_terraforming(from, to, &from_slope, &to_slope);
		bool changed = false;
		// change slope of from
		if(  from_slope != from->get_grund_hang()  ) {
			if(  from_slope == slope_t::all_up_two  ) {
				from->set_hoehe( from->get_hoehe() + 2 );
				from->set_grund_hang( slope_t::flat );
				route[i].z = from->get_hoehe();
			}
			else if(  from_slope != slope_t::all_up_one  ) {
				// bit of a hack to recognise single height slopes shifted up 1
				if(  from_slope > slope_t::all_up_one  &&  slope_t::is_single( from_slope-slope_t::all_up_one )  ) {
					from->set_hoehe( from->get_hoehe() + 1 );
					from_slope -= slope_t::all_up_one;
					route[i].z = from->get_hoehe();
				}
				from->set_grund_hang( from_slope );
			}
			else {
				from->set_hoehe( from->get_hoehe() + 1 );
				from->set_grund_hang( slope_t::flat );
				route[i].z = from->get_hoehe();
			}
			changed = true;
			if (last_terraformed != i) {
				// charge player
				player_t::book_construction_costs(player_builder, welt->get_settings().cst_set_slope, from->get_pos().get_2d(), ignore_wt);
			}
		}
		// change slope of to
		if(  to_slope != to->get_grund_hang()  ) {
			if(  to_slope == slope_t::all_up_two  ) {
				to->set_hoehe( to->get_hoehe() + 2 );
				to->set_grund_hang( slope_t::flat );
				route[i + 1].z = to->get_hoehe();
			}
			else if(  to_slope != slope_t::all_up_one  ) {
				// bit of a hack to recognise single height slopes shifted up 1
				if(  to_slope > slope_t::all_up_one  &&  slope_t::is_single( to_slope-slope_t::all_up_one )  ) {
					to->set_hoehe( to->get_hoehe() + 1 );
					to_slope -= slope_t::all_up_one;
					route[i + 1].z = to->get_hoehe();
				}
				to->set_grund_hang(to_slope);
			}
			else {
				to->set_hoehe( to->get_hoehe() + 1);
				to->set_grund_hang(slope_t::flat);
				route[i+1].z = to->get_hoehe();
			}
			changed = true;
			// charge player
			player_t::book_construction_costs(player_builder, welt->get_settings().cst_set_slope, to->get_pos().get_2d(), ignore_wt);
			last_terraformed = i+1; // do not pay twice for terraforming one tile
		}
		// recalc slope image of neighbors
		if (changed) {
			for(uint8 j=0; j<2; j++) {
				for(uint8 x=0; x<2; x++) {
					for(uint8 y=0; y<2; y++) {
						grund_t *gr = welt->lookup_kartenboden(route[i+j].get_2d()+koord(x,y));
						if (gr) {
							gr->calc_image();
						}
					}
				}
			}
		}
	}
}

void way_builder_t::check_for_bridge(const grund_t* parent_from, const grund_t* from, const vector_tpl<koord3d> &ziel)
{
	// wrong starting slope or tile already occupied with a way ...
	if (!slope_t::is_way(from->get_grund_hang())) {
		return;
	}

	/*
	 * now check existing ways:
	 * no tunnels/bridges at crossings and no track tunnels/bridges on roads (but road tunnels/bridges on tram are allowed).
	 * (keep in mind, that our waytype isn't currently on the tile and will be built later)
	 */
	const weg_t *way0 = from->get_weg_nr(0);
	const weg_t *way1 = from->get_weg_nr(1);
	if(  way0  ) {
		switch(  bautyp&bautyp_mask  ) {
			case schiene_tram:
			case strasse: {
				const weg_t *other = way1;
				if (  way0->get_waytype() != desc->get_wtyp()  ) {
					if (  way1  ) {
						// two different ways
						return;
					}
					other = way0;
				}
				if (  other  ) {
					if (  (bautyp&bautyp_mask) == strasse  ) {
						if (  other->get_waytype() != track_wt  ||  other->get_desc()->get_styp()!=type_tram  ) {
							// road only on tram
							return;
						}
					}
					else {
						if (  other->get_waytype() != road_wt  ) {
							// tram only on road
							return;
						}
					}
				}
			}
			/* FALLTHROUGH */

			default:
				if (way0->get_waytype()!=desc->get_wtyp()  ||  way1!=NULL) {
					// no other ways allowed
					return;
				}
		}
	}

	const koord zv=from->get_pos().get_2d()-parent_from->get_pos().get_2d();
	const ribi_t::ribi ribi = ribi_type(zv);

	// now check ribis of existing ways
	const ribi_t::ribi wayribi = way0 ? way0->get_ribi_unmasked() | (way1 ? way1->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::none) : (ribi_t::ribi)ribi_t::none;
	if (  wayribi & (~ribi)  ) {
		// curves at bridge start
		return;
	}

	// ok, so now we do a closer investigation
	if(  bridge_desc  && (  ribi_type(from->get_grund_hang()) == ribi_t::backward(ribi_type(zv))  ||  from->get_grund_hang() == 0  )
		&&  bridge_builder_t::can_place_ramp(player_builder, from, desc->get_wtyp(),ribi_t::backward(ribi_type(zv)))  ) {
		// Try a bridge.
		const sint32 cost_difference = desc->get_maintenance() > 0 ? (bridge_desc->get_maintenance() * 4l + 3l) / desc->get_maintenance() : 16;
		// try eight possible lengths ..
		uint32 min_length = 1;
		for (uint8 i = 0; i < 8 && min_length <= welt->get_settings().way_max_bridge_len; ++i) {
			sint8 bridge_height;
			const char *error = NULL;
			koord3d end = bridge_builder_t::find_end_pos( player_builder, from->get_pos(), zv, bridge_desc, error, bridge_height, true, min_length );
			const grund_t* gr_end = welt->lookup(end);
			if(  gr_end == NULL) {
				// no valid end point found
				min_length++;
				continue;
			}
			uint32 length = koord_distance(from->get_pos(), end);
			if(!ziel.is_contained(end)  &&  bridge_builder_t::can_place_ramp(player_builder, gr_end, desc->get_wtyp(), ribi_type(zv))) {
				// If there is a slope on the starting tile, it's taken into account in is_allowed_step, but a bridge will be flat!
				sint8 num_slopes = (from->get_grund_hang() == slope_t::flat) ? 1 : -1;
				// On the end tile, we haven't to subtract way_count_slope, since is_allowed_step isn't called with this tile.
				num_slopes += (gr_end->get_grund_hang() == slope_t::flat) ? 1 : 0;
				next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference + num_slopes*welt->get_settings().way_count_slope, build_straight | build_tunnel_bridge));
				min_length = length+1;
			}
			else {
				break;
			}
		}
		return;
	}

	if(  tunnel_desc  &&  ribi_type(from->get_grund_hang()) == ribi_type(zv)  ) {
		// uphill hang ... may be tunnel?
		const sint32 cost_difference = desc->get_maintenance() > 0 ? (tunnel_desc->get_maintenance() * 4l + 3l) / desc->get_maintenance() : 16;
		koord3d end = tunnel_builder_t::find_end_pos( player_builder, from->get_pos(), zv, tunnel_desc);
		if(  end != koord3d::invalid  &&  !ziel.is_contained(end)  ) {
			uint32 length = koord_distance(from->get_pos(), end);
			next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference, build_straight | build_tunnel_bridge ));
			return;
		}
	}
}


way_builder_t::way_builder_t(player_t* player) : next_gr(32)
{
	player_builder     = player;
	bautyp = strasse;   // kann mit init_builder() gesetzt werden
	maximum = 2000;// CA $ PER TILE

	keep_existing_ways = false;
	keep_existing_city_roads = false;
	keep_existing_faster_ways = false;
	build_sidewalk = false;
}


/**
 * If a way is built on top of another way, should the type
 * of the former way be kept or replaced (true == keep)
 */
void way_builder_t::set_keep_existing_ways(bool yesno)
{
	keep_existing_ways = yesno;
	keep_existing_faster_ways = false;
}


void way_builder_t::set_keep_existing_faster_ways(bool yesno)
{
	keep_existing_ways = false;
	keep_existing_faster_ways = yesno;
}


void way_builder_t::init_builder(bautyp_t wt, const way_desc_t *b, const tunnel_desc_t *tunnel, const bridge_desc_t *br)
{
	bautyp = wt;
	desc = b;
	bridge_desc = br;
	tunnel_desc = tunnel;
	if(wt&tunnel_flag  &&  tunnel==NULL) {
		dbg->fatal("way_builder_t::init_builder()","needs a tunnel description for an underground route!");
	}
	if((wt&bautyp_mask)==luft) {
		wt &= bautyp_mask | bot_flag;
	}
	if(player_builder==NULL) {
		bridge_desc = NULL;
		tunnel_desc = NULL;
	}
	else if(  bautyp != river  ) {
#ifdef AUTOMATIC_BRIDGES
		if(  bridge_desc == NULL  ) {
			bridge_desc = bridge_builder_t::find_bridge(b->get_wtyp(), 25, welt->get_timeline_year_month());
		}
#endif
#ifdef AUTOMATIC_TUNNELS
		if(  tunnel_desc == NULL  ) {
			tunnel_desc = tunnel_builder_t::get_tunnel_desc(b->get_wtyp(), 25, welt->get_timeline_year_month());
		}
#endif
	}
	DBG_MESSAGE("way_builder_t::init_builder()", "setting way type to %d, desc=%s, bridge_desc=%s, tunnel_desc=%s",
			bautyp,
			desc ? desc->get_name() : "NULL",
			bridge_desc ? bridge_desc->get_name() : "NULL",
			tunnel_desc ? tunnel_desc->get_name() : "NULL"
		);
}


void get_mini_maxi( const vector_tpl<koord3d> &ziel, koord3d &mini, koord3d &maxi )
{
	mini = maxi = ziel[0];
	for(koord3d const& current : ziel) {
		if( current.x < mini.x ) {
			mini.x = current.x;
		} else if( current.x > maxi.x ) {
			maxi.x = current.x;
		}
		if( current.y < mini.y ) {
			mini.y = current.y;
		} else if( current.y > maxi.y ) {
			maxi.y = current.y;
		}
		if( current.z < mini.z ) {
			mini.z = current.z;
		} else if( current.z > maxi.z ) {
			maxi.z = current.z;
		}
	}
}


/* this routine uses A* to calculate the best route
 * beware: change the cost and you will mess up the system!
 * (but you can try, look at simuconf.tab)
 */
sint32 way_builder_t::intern_calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel)
{
	// we clear it here probably twice: does not hurt ...
	route.clear();
	terraform_index.clear();

	// check for existing koordinates
	bool has_target_ground = false;
	for(koord3d const& i : ziel) {
		has_target_ground |= welt->lookup(i) != 0;
	}
	if( !has_target_ground ) {
		return -1;
	}

	// calculate the minimal cuboid containing 'ziel'
	koord3d mini, maxi;
	get_mini_maxi( ziel, mini, maxi );

	// memory in static list ...
	if(route_t::nodes==NULL) {
		route_t::MAX_STEP = welt->get_settings().get_max_route_steps(); // may need very much memory => configurable
		route_t::nodes = new route_t::ANode[route_t::MAX_STEP+4+1];
	}

	static binary_heap_tpl <route_t::ANode *> queue;

	// initialize marker field
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);

	// clear the queue (should be empty anyhow)
	queue.clear();

	// some thing for the search
	grund_t *to;
	koord3d gr_pos; // just the last valid pos ...
	route_t::ANode *tmp=NULL;
	uint32 step = 0;
	const grund_t* gr=NULL;

	for(koord3d const& i : start) {
		gr = welt->lookup(i);

		// is valid ground?
		sint32 dummy;
		if( !gr || !is_allowed_step(gr,gr,&dummy) ) {
			// DBG_MESSAGE("way_builder_t::intern_calc_route()","cannot start on (%i,%i,%i)",start.x,start.y,start.z);
			continue;
		}
		tmp = &(route_t::nodes[step]);
		step ++;

		tmp->parent = NULL;
		tmp->gr = gr;
		tmp->f = calc_distance(i, mini, maxi);
		tmp->g = 0;
		tmp->dir = 0;
		tmp->count = 0;

		queue.insert(tmp);
	}

	if( queue.empty() ) {
		// no valid ground to start.
		return -1;
	}

	INT_CHECK("wegbauer 347");

	// get exclusively the tile list
	route_t::GET_NODE();

	// to speed up search, but may not find all shortest ways
	uint32 min_dist = 99999999;

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	do {
		route_t::ANode *test_tmp = queue.pop();

		if(marker.test_and_mark(test_tmp->gr)) {
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		gr = tmp->gr;
		gr_pos = gr->get_pos();

#ifdef DEBUG_ROUTES
DBG_DEBUG("insert to close","(%i,%i,%i)  f=%i",gr->get_pos().x,gr->get_pos().y,gr->get_pos().z,tmp->f);
#endif

		// already there
		if(  ziel.is_contained(gr_pos)  ||  tmp->g>maximum) {
			// we added a target to the closed list: we are finished
			break;
		}

		// the four possible directions plus any additional stuff due to already existing brides plus new ones ...
		next_gr.clear();

		// only one direction allowed ...
		const ribi_t::ribi straight_dir = tmp->parent!=NULL ? ribi_type(gr->get_pos() - tmp->parent->gr->get_pos()) : (ribi_t::ribi)ribi_t::all;

		// test directions
		// .. use only those that are allowed by current slope
		// .. do not go backward
		const ribi_t::ribi slope_dir = (slope_t::is_way_ns(gr->get_weg_hang()) ? ribi_t::northsouth : ribi_t::none) | (slope_t::is_way_ew(gr->get_weg_hang()) ? ribi_t::eastwest : ribi_t::none);
		const ribi_t::ribi test_dir = (tmp->count & build_straight)==0  ?  slope_dir  & ~ribi_t::backward(straight_dir)
		                                                                :  straight_dir;

		// testing all four possible directions
		for(ribi_t::ribi r=1; (r&16)==0; r<<=1) {
			if((r & test_dir)==0) {
				// not allowed to go this direction
				continue;
			}

			bool do_terraform = false;
			const koord zv(r);
			if(!gr->get_neighbour(to,invalid_wt,r)  ||  !check_slope(gr, to)) {
				// slopes do not match
				// terraforming enabled?
				if (bautyp==river  ||  (bautyp & terraform_flag) == 0) {
					continue;
				}
				// check terraforming (but not in curves)
				if (gr->get_grund_hang()==0  ||  (tmp->parent!=NULL  &&  tmp->parent->parent!=NULL  &&  r==straight_dir)) {
					to = welt->lookup_kartenboden(gr->get_pos().get_2d() + zv);
					if (to==NULL  ||  (check_slope(gr, to)  &&  gr->get_vmove(r)!=to->get_vmove(ribi_t::backward(r)))) {
						continue;
					}
					else {
						do_terraform = true;
					}
				}
				else {
					continue;
				}
			}

			// something valid?
			if(marker.is_marked(to)) {
				continue;
			}

			sint32 new_cost = 0;
			bool is_ok = is_allowed_step(gr,to,&new_cost);

			if(is_ok) {
				// now add it to the array ...
				next_gr.append(next_gr_t(to, new_cost, do_terraform ? build_straight | terraform : 0));
			}
			else if(tmp->parent!=NULL  &&  r==straight_dir  &&  (tmp->count & build_tunnel_bridge)==0) {
				// try to build a bridge or tunnel here, since we cannot go here ...
				check_for_bridge(tmp->parent->gr,gr,ziel);
			}
		}

		// now check all valid ones ...
		for(next_gr_t const& r : next_gr) {
			to = r.gr;

			if(  to==NULL) {
				continue;
			}

			// new values for cost g
			uint32 new_g = tmp->g + r.cost;

			settings_t const& s = welt->get_settings();
			// check for curves (usually, one would need the lastlast and the last;
			// if not there, then we could just take the last
			uint8 current_dir;
			if(tmp->parent!=NULL) {
				current_dir = ribi_type( tmp->parent->gr->get_pos(), to->get_pos() );
				if(tmp->dir!=current_dir) {
					new_g += s.way_count_curve;
					if(tmp->parent->dir!=tmp->dir) {
						// discourage double turns
						new_g += s.way_count_double_curve;
					}
					else if(ribi_t::is_perpendicular(tmp->dir,current_dir)) {
						// discourage v turns heavily
						new_g += s.way_count_90_curve;
					}
				}
				else if(bautyp==leitung  &&  ribi_t::is_bend(current_dir)) {
					new_g += s.way_count_double_curve;
				}
				// extra malus leave an existing road after only one tile
				waytype_t const wt = desc->get_wtyp();
				if (tmp->parent->gr->hat_weg(wt) && !gr->hat_weg(wt) && to->hat_weg(wt)) {
					// but only if not straight track
					if(!ribi_t::is_straight(tmp->dir)) {
						new_g += s.way_count_leaving_road;
					}
				}
			}
			else {
				 current_dir = ribi_type( gr->get_pos(), to->get_pos() );
			}

			const uint32 new_dist = calc_distance( to->get_pos(), mini, maxi );

			// special check for kinks at the end
			if(new_dist==0  &&  current_dir!=tmp->dir) {
				// discourage turn on last tile
				new_g += s.way_count_double_curve;
			}

			if (new_dist == 0 && r.flag & terraform) {
				// no terraforming near target
				continue;
			}
			if(new_dist<min_dist) {
				min_dist = new_dist;
			}
			else if(new_dist>min_dist+50) {
				// skip, if too far from current minimum tile
				// will not find some ways, but will be much faster ...
				// also it will avoid too big detours, which is probably also not the way, the builder intended
				continue;
			}


			const uint32 new_f = new_g+new_dist;

			if((step&0x03)==0) {
				INT_CHECK( "wegbauer 1347" );
#ifdef DEBUG_ROUTES
				if((step&1023)==0) {minimap_t::get_instance()->calc_map();}
#endif
			}

			// not in there or taken out => add new
			route_t::ANode *k=&(route_t::nodes[step]);
			step++;

			k->parent = tmp;
			k->gr = to;
			k->g = new_g;
			k->f = new_f;
			k->dir = current_dir;
			// count is unused here, use it as flag-variable instead
			k->count = r.flag;

			queue.insert( k );

#ifdef DEBUG_ROUTES
DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i",to->get_pos().x,to->get_pos().y,to->get_pos().z,k->f);
#endif
		}

	} while (!queue.empty() && step < route_t::MAX_STEP);

#ifdef DEBUG_ROUTES
DBG_DEBUG("way_builder_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u",step,route_t::MAX_STEP,queue.get_count(),tmp->g);
#endif
	INT_CHECK("wegbauer 194");

	route_t::RELEASE_NODE();

	// target reached?
	if(  !ziel.is_contained(gr->get_pos())  ||  step>=route_t::MAX_STEP  ||  tmp->parent==NULL  ||  tmp->g > maximum  ) {
		if (step>=route_t::MAX_STEP) {
			dbg->warning("way_builder_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,route_t::MAX_STEP);
		}
		return -1;
	}
	else {
		const sint32 cost = tmp->g;
		// reached => construct route
		while(tmp != NULL) {
			route.append(tmp->gr->get_pos());
			if (tmp->count & terraform) {
				terraform_index.append(route.get_count()-1);
			}
			tmp = tmp->parent;
		}
		return cost;
	}
}


void way_builder_t::intern_calc_straight_route(const koord3d start, const koord3d ziel)
{
	bool ok = true;
	const koord3d koordup(0, 0, welt->get_settings().get_way_height_clearance());

	sint32 dummy_cost;
	const grund_t *test_bd = welt->lookup(start);
	ok = false;
	if (test_bd  &&  is_allowed_step(test_bd,test_bd,&dummy_cost)  ) {
		//there is a legal ground at the start
		ok = true;
	}
	if (ok  &&  (bautyp&tunnel_flag) && !test_bd->ist_tunnel()) {
		// start tunnelbuilding in tunnels
		return;
	}
	if (bautyp&elevated_flag) {
		test_bd = welt->lookup(start + koordup);
		if (test_bd  &&  is_allowed_step(test_bd,test_bd,&dummy_cost, true)  ) {
			//there is a legal way at the upper layer of start
			ok = true;
		}
	}
	if (!ok) {
		//target is not suitable
		return;
	}
	test_bd = welt->lookup(ziel);
	// we have to reach target height if no tunnel building or (target ground does not exists or is underground).
	// in full underground mode if there is no tunnel under cursor, kartenboden gets selected
	const bool target_3d = (bautyp&tunnel_flag)==0  ||  test_bd==NULL  ||  !test_bd->ist_karten_boden();
	if((bautyp&tunnel_flag)==0) {
		//same thing to the target point
		ok = false;
		if (test_bd  &&  is_allowed_step(test_bd,test_bd,&dummy_cost)  ) {
			//there is a legal ground at the target
			ok = true;
		}
		if (bautyp&elevated_flag) {
			test_bd = welt->lookup(ziel + koordup);
			if (test_bd  &&  is_allowed_step(test_bd,test_bd,&dummy_cost, true)  ) {
				//there is a legal way at the upper layer of the target
				ok = true;
			}
		}
		if(!ok) {
			return;
		}
	}

	koord3d pos=start;

	route.clear();
	route.append(start);
	terraform_index.clear();
	bool check_terraform = start.x==ziel.x  ||  start.y==ziel.y;

	while(pos.get_2d()!=ziel.get_2d()  &&  ok) {

		bool do_terraform = false;
		// shortest way
		ribi_t::ribi diff;
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			diff = (pos.x>ziel.x) ? ribi_t::west : ribi_t::east;
		}
		else {
			diff = (pos.y>ziel.y) ? ribi_t::north : ribi_t::south;
		}
		if(bautyp&tunnel_flag) {
			// create fake tunnel grounds if needed
			bool bd_von_new = false, bd_nach_new = false;
			grund_t *bd_von = welt->lookup(pos);
			if(  bd_von == NULL ) {
				bd_von = new tunnelboden_t(pos, slope_t::flat);
				bd_von_new = true;
			}
			// take care of slopes
			pos.z = bd_von->get_vmove(diff);

			// check next tile
			grund_t *bd_nach = welt->lookup(pos + diff);
			if(  !bd_nach  ) {
				// check for slope down ...
				bd_nach = welt->lookup(pos + diff + koord3d(0,0,-1));
				if(  !bd_nach  ) {
					bd_nach = welt->lookup(pos + diff + koord3d(0,0,-2));
				}
				if(  bd_nach  &&  bd_nach->get_weg_hang() == slope_t::flat  ) {
					// Don't care about _flat_ tunnels below.
					bd_nach = NULL;
				}
			}
			if(  bd_nach == NULL  ){
				bd_nach = new tunnelboden_t(pos + diff, slope_t::flat);
				bd_nach_new = true;
			}
			// check for tunnel and right slope
			ok = ok && bd_nach->ist_tunnel() && bd_nach->get_vmove(ribi_t::backward(diff))==pos.z;
			// all other checks are done here (crossings, stations etc)
			ok = ok && is_allowed_step(bd_von, bd_nach, &dummy_cost);

			// advance position
			pos = bd_nach->get_pos();

			// check new tile: ground must be above tunnel and below sea
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			ok = ok  &&  (gr->get_hoehe() > pos.z)  &&  (!gr->is_water()  ||  (welt->lookup_hgt(pos.get_2d()) > pos.z) );

			if (bd_von_new) {
				delete bd_von;
			}
			if (bd_nach_new) {
				delete bd_nach;
			}
		}
		else {
			grund_t *bd_von = welt->lookup(pos);
			ok = false;
			grund_t *bd_nach = NULL;
			if ( bd_von ) {
				if (bd_von->get_neighbour(bd_nach, invalid_wt, diff) && check_slope(bd_von, bd_nach)) {
					ok = true;
				}
				else {
					// slopes do not match
					// terraforming enabled?  or able to follow upper layer?
					if ((bautyp==river  ||  (bautyp & terraform_flag) == 0)  &&  (bautyp&elevated_flag) == 0  ) {
						break;
					}
					// check terraforming (but not in curves)
					if (check_terraform) {
						bd_nach = welt->lookup_kartenboden(bd_von->get_pos().get_2d() + diff);
						if (bd_nach==NULL  ||  (check_slope(bd_von, bd_nach)  &&  bd_von->get_vmove(diff)!=bd_nach->get_vmove(ribi_t::backward(diff)))) {
							ok = false;
						}
						else {
							do_terraform = true;
							ok = true;
						}
					}
				}
				// allowed ground?
				ok = ok  &&  bd_nach  &&  is_allowed_step(bd_von,bd_nach,&dummy_cost);
				if (ok) {
					pos = bd_nach->get_pos();
				}
			}
			// if failed
			if (!ok  &&  bautyp&elevated_flag) {
				//search following the upper layer
				bd_von = welt->lookup(pos + koordup);
				if(bd_von  &&  bd_von->get_neighbour(bd_nach, invalid_wt, diff)  &&  check_slope(bd_von, bd_nach)  &&  is_allowed_step(bd_von, bd_nach, &dummy_cost, true)  ) {
					ok = true;
					pos = bd_nach->get_pos() - koordup;
				}
			}
			check_terraform = pos.x==ziel.x  ||  pos.y==ziel.y;
		}

		route.append(pos);
		if (do_terraform) {
			terraform_index.append(route.get_count()-2);
		}
		DBG_MESSAGE("way_builder_t::calc_straight_route()","step %s = %i",koord(diff).get_str(),ok);
	}
	ok = ok && ( target_3d ? pos==ziel : pos.get_2d()==ziel.get_2d() );

	// we can built a straight route?
	if(ok) {
DBG_MESSAGE("way_builder_t::intern_calc_straight_route()","found straight route max_n=%i",get_count()-1);
	}
	else {
		route.clear();
		terraform_index.clear();
	}
}

/* this routine uses A* to calculate the best route
 * beware: change the cost and you will mess up the system!
 * (but you can try, look at simuconf.tab)
 */
/* It should not be river/airport/tunnel/bridge/terraformable
 */
sint32 way_builder_t::intern_calc_route_elevated(const koord3d start, const koord3d ziel)
{
	// we clear it here probably twice: does not hurt ...
	route.clear();
	terraform_index.clear();

	const koord3d koordup(0, 0, welt->get_settings().get_way_height_clearance());

	// check for existing koordinates
	bool has_target_ground = welt->lookup(ziel) || welt->lookup(ziel + koordup);
	if( !has_target_ground ) {
		return -1;
	}


	// memory in static list ...
	if(route_t::nodes==NULL) {
		route_t::MAX_STEP = welt->get_settings().get_max_route_steps(); // may need very much memory => configurable
		route_t::nodes = new route_t::ANode[route_t::MAX_STEP+4+1];
	}

	static binary_heap_tpl <route_t::ANode *> queue;

	// initialize marker field
	marker_t& markerbelow = marker_t::instance(welt->get_size().x, welt->get_size().y);
	marker_t& markerabove = marker_t::instance_second(welt->get_size().x, welt->get_size().y);

	// clear the queue (should be empty anyhow)
	queue.clear();

	// some thing for the search
	grund_t *to;
	koord3d gr_pos; // just the last valid pos ...
	route_t::ANode *tmp=NULL;
	uint32 step = 0;
	const grund_t *gr=NULL, *gu = NULL;

	gr = welt->lookup(start);
		// is valid ground?
	sint32 dummy;
	if( gr && is_allowed_step(gr,gr,&dummy) ) {
		// DBG_MESSAGE("way_builder_t::intern_calc_route()","cannot start on (%i,%i,%i)",start.x,start.y,start.z);
		tmp = &(route_t::nodes[step]);
		step ++;
		tmp->parent = NULL;
		tmp->gr = gr;
		tmp->f = calc_distance(start, ziel, ziel);
		tmp->g = 0;
		tmp->dir = 0;

		tmp->count = 0;

		queue.insert(tmp);
	}

	gu = welt->lookup(start + koordup);
	if( gu && is_allowed_step(gu,gu,&dummy, true) ) {
		// DBG_MESSAGE("way_builder_t::intern_calc_route()","cannot start on (%i,%i,%i)",start.x,start.y,start.z);
		tmp = &(route_t::nodes[step]);
		step ++;
		tmp->parent = NULL;
		tmp->gr = gu;
		tmp->f = calc_distance(start, ziel, ziel);
		tmp->g = 0;
		tmp->dir = 0;

		tmp->count = is_upperlayer;

		queue.insert(tmp);
	}


	if( queue.empty() ) {
		// no valid ground to start.
		return -1;
	}

	INT_CHECK("wegbauer 347");

	// get exclusively the tile list
	route_t::GET_NODE();

	// to speed up search, but may not find all shortest ways
	uint32 min_dist = 99999999;

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	do {
		route_t::ANode *test_tmp = queue.pop();

		if(  (test_tmp->count&is_upperlayer?markerabove:markerbelow).test_and_mark(test_tmp->gr)  ) {
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		if(test_tmp->count & is_upperlayer) {
			gu = tmp->gr;
			gr_pos = gu->get_pos() - koordup;
			gr = welt->lookup(gr_pos);
		}
		else {
			gr = tmp->gr;
			gr_pos = gr->get_pos();
			gu = welt->lookup(gr_pos + koordup);
		}

#ifdef DEBUG_ROUTES
DBG_DEBUG("insert to close","(%i,%i,%i)  f=%i",gr->get_pos().x,gr->get_pos().y,gr->get_pos().z,tmp->f);
#endif

		// already there
		if(  ziel == gr_pos  ||  tmp->g>maximum) {
			// we added a target to the closed list: we are finished
			break;
		}

		// the four possible directions plus any additional stuff due to already existing brides plus new ones ...
		next_gr.clear();

		//search following the lower layer
		if(gr) {

			// only one direction allowed ...
			const ribi_t::ribi straight_dir = tmp->parent!=NULL ? ribi_type(gr->get_pos() - tmp->parent->gr->get_pos()) : (ribi_t::ribi)ribi_t::all;

			// test directions
			// .. use only those that are allowed by current slope
			// .. do not go backward
			const ribi_t::ribi slope_dir = (slope_t::is_way_ns(gr->get_weg_hang()) ? ribi_t::northsouth : ribi_t::none) | (slope_t::is_way_ew(gr->get_weg_hang()) ? ribi_t::eastwest : ribi_t::none);
			const ribi_t::ribi test_dir = (tmp->count & build_straight)==0  ?  slope_dir  & ~ribi_t::backward(straight_dir)
		                                                                :  straight_dir;

			// testing all four possible directions
			for(ribi_t::ribi r=1; (r&16)==0; r<<=1) {
				if((r & test_dir)==0) {
					// not allowed to go this direction
					continue;
				}

				const koord zv(r);
				if(!gr->get_neighbour(to,invalid_wt,r)  ||  !check_slope(gr, to)) {
					// slopes do not match
					continue;
				}

				// something valid?
				if(markerbelow.is_marked(to)) {
					continue;
				}

				sint32 new_cost = 0;
				bool is_ok = is_allowed_step(gr,to,&new_cost);

				if(is_ok) {
					// now add it to the array ...
					next_gr.append(next_gr_t(to, new_cost, 0));
				}
			}
		}

		//search following the upper layer
		if(gu) {

			// only one direction allowed ...
			const ribi_t::ribi straight_dir = tmp->parent!=NULL ? ribi_type(gu->get_pos() - tmp->parent->gr->get_pos()) : (ribi_t::ribi)ribi_t::all;

			// test directions
			// .. use only those that are allowed by current slope
			// .. do not go backward
			const ribi_t::ribi slope_dir = (slope_t::is_way_ns(gu->get_weg_hang()) ? ribi_t::northsouth : ribi_t::none) | (slope_t::is_way_ew(gu->get_weg_hang()) ? ribi_t::eastwest : ribi_t::none);
			const ribi_t::ribi test_dir = (tmp->count & build_straight)==0  ?  slope_dir  & ~ribi_t::backward(straight_dir)
		                                                                :  straight_dir;

			// testing all four possible directions
			for(ribi_t::ribi r=1; (r&16)==0; r<<=1) {
				if((r & test_dir)==0) {
					// not allowed to go this direction
					continue;
				}

				const koord zv(r);
				if(!gu->get_neighbour(to,invalid_wt,r)  ||  !check_slope(gu, to)) {
					// slopes do not match
					continue;
				}

				// something valid?
				if(markerabove.is_marked(to)) {
					continue;
				}

				sint32 new_cost = 0;
				bool is_ok = is_allowed_step(gu,to,&new_cost, true);

				if(is_ok) {
					// now add it to the array ...
					next_gr.append(next_gr_t(to, new_cost, is_upperlayer));
				}
			}
		}

		// now check all valid ones ...
		for(next_gr_t const& r : next_gr) {
			to = r.gr;

			if(  to==NULL) {
				continue;
			}

			// new values for cost g
			uint32 new_g = tmp->g + r.cost;

			settings_t const& s = welt->get_settings();
			// check for curves (usually, one would need the lastlast and the last;
			// if not there, then we could just take the last
			uint8 current_dir;
			if(tmp->parent!=NULL) {
				current_dir = ribi_type( tmp->parent->gr->get_pos(), to->get_pos() );
				if(tmp->dir!=current_dir) {
					new_g += s.way_count_curve;
					if(tmp->parent->dir!=tmp->dir) {
						// discourage double turns
						new_g += s.way_count_double_curve;
					}
					else if(ribi_t::is_perpendicular(tmp->dir,current_dir)) {
						// discourage v turns heavily
						new_g += s.way_count_90_curve;
					}
				}
				else if(bautyp==leitung  &&  ribi_t::is_bend(current_dir)) {
					new_g += s.way_count_double_curve;
				}
				// extra malus leave an existing road after only one tile
				waytype_t const wt = desc->get_wtyp();
				if (tmp->parent->gr->hat_weg(wt) && !(gr? gr: gu)->hat_weg(wt) && to->hat_weg(wt)) {
					// but only if not straight track
					if(!ribi_t::is_straight(tmp->dir)) {
						new_g += s.way_count_leaving_road;
					}
				}
			}
			else {
				 current_dir = ribi_type( gr_pos, to->get_pos() );
			}

			const uint32 new_dist = calc_distance( to->get_pos(), ziel, ziel );

			// special check for kinks at the end
			if(new_dist==0  &&  current_dir!=tmp->dir) {
				// discourage turn on last tile
				new_g += s.way_count_double_curve;
			}

			if(new_dist<min_dist) {
				min_dist = new_dist;
			}
			else if(new_dist>min_dist+50) {
				// skip, if too far from current minimum tile
				// will not find some ways, but will be much faster ...
				// also it will avoid too big detours, which is probably also not the way, the builder intended
				continue;
			}


			const uint32 new_f = new_g+new_dist;

			if((step&0x03)==0) {
				INT_CHECK( "wegbauer 1347" );
#ifdef DEBUG_ROUTES
				if((step&1023)==0) {minimap_t::get_instance()->calc_map();}
#endif
			}

			// not in there or taken out => add new
			route_t::ANode *k=&(route_t::nodes[step]);
			step++;

			k->parent = tmp;
			k->gr = to;
			k->g = new_g;
			k->f = new_f;
			k->dir = current_dir;
			// count is unused here, use it as flag-variable instead
			k->count = r.flag;

			queue.insert( k );

#ifdef DEBUG_ROUTES
DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i",to->get_pos().x,to->get_pos().y,to->get_pos().z,k->f);
#endif
		}

	} while (!queue.empty() && step < route_t::MAX_STEP);

#ifdef DEBUG_ROUTES
DBG_DEBUG("way_builder_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u",step,route_t::MAX_STEP,queue.get_count(),tmp->g);
#endif
	INT_CHECK("wegbauer 194");

	route_t::RELEASE_NODE();

	// target reached?
	if(  !(ziel == gr_pos)  ||  step>=route_t::MAX_STEP  ||  tmp->parent==NULL  ||  tmp->g > maximum  ) {
		if (step>=route_t::MAX_STEP) {
			dbg->warning("way_builder_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,route_t::MAX_STEP);
		}
		return -1;
	}
	else {
		const sint32 cost = tmp->g;
		// reached => construct route
		while(tmp != NULL) {
			if(tmp->count & is_upperlayer) {
				route.append(tmp->gr->get_pos() - koordup);
			} else {
				route.append(tmp->gr->get_pos() );
			}
			tmp = tmp->parent;
		}
		return cost;
	}
}


// special for starting/landing runways
bool way_builder_t::intern_calc_route_runways(koord3d start3d, const koord3d ziel3d)
{
	route.clear();
	terraform_index.clear();

	const koord start=start3d.get_2d();
	const koord ziel=ziel3d.get_2d();
	// check for straight line!
	const ribi_t::ribi ribi = ribi_type( start, ziel );
	if(  !ribi_t::is_straight(ribi)  ) {
		// only straight runways!
		return false;
	}
	const ribi_t::ribi ribi_straight = ribi_t::doubles(ribi);

	// not too close to the border?
	if(  !(welt->is_within_limits(start-koord(5,5))  &&  welt->is_within_limits(start+koord(5,5)))  ||
	     !(welt->is_within_limits(ziel-koord(5,5))  &&  welt->is_within_limits(ziel+koord(5,5)))  ) {
		if(player_builder==welt->get_active_player()) {
			create_win( new news_img("Zu nah am Kartenrand"), w_time_delete, magic_none);
			return false;
		}
	}

	// now try begin and endpoint
	const koord zv(ribi);
	// end start
	const grund_t *gr = welt->lookup_kartenboden(start);
	const weg_t *weg = gr->get_weg(air_wt);
	if(weg  &&  !ribi_t::is_straight(weg->get_ribi()|ribi_straight)  ) {
		// cannot connect with curve at the end
		return false;
	}
	if(  weg  &&  weg->get_desc()->get_styp()==type_flat  ) {
		//  could not continue taxiway with runway
		return false;
	}
	// check end
	gr = welt->lookup_kartenboden(ziel);
	weg = gr->get_weg(air_wt);
	if(weg  &&  !ribi_t::is_straight(weg->get_ribi()|ribi_straight)  ) {
		// cannot connect with curve at the end
		return false;
	}
	if(  weg  &&  weg->get_desc()->get_styp()==type_flat  ) {
		//  could not continue taxiway with runway
		return false;
	}
	// now try a straight line with no crossings and no curves at the end
	const int dist=koord_distance( ziel, start );
	grund_t *from = welt->lookup_kartenboden(start);
	for(  int i=0;  i<=dist;  i++  ) {
		grund_t *to = welt->lookup_kartenboden(start+zv*i);
		sint32 dummy;
		if (!is_allowed_step(from, to, &dummy)) {
			return false;
		}
		weg = to->get_weg(air_wt);
		if(  weg  &&  weg->get_desc()->get_styp()==type_runway  &&  (ribi_t::is_threeway(weg->get_ribi_unmasked()|ribi_straight))  &&  (weg->get_ribi_unmasked()|ribi_straight)!=ribi_t::all  ) {
			// only fourway crossings of runways allowed, no threeways => fail
			return false;
		}
		from = to;
	}
	// now we can build here
	route.clear();
	terraform_index.clear();
	route.resize(dist + 2);
	for(  int i=0;  i<=dist;  i++  ) {
		route.append(welt->lookup_kartenboden(start + zv * i)->get_pos());
	}
	return true;
}


/*
 * calc_straight_route (maximum one curve, including diagonals)
 */
void way_builder_t::calc_straight_route(koord3d start, const koord3d ziel)
{
	DBG_MESSAGE("way_builder_t::calc_straight_route()","from %d,%d,%d to %d,%d,%d",start.x,start.y,start.z, ziel.x,ziel.y,ziel.z );
	if(bautyp==luft  &&  desc->get_styp()==type_runway) {
		// these are straight anyway ...
		intern_calc_route_runways(start, ziel);
	}
	else {
		intern_calc_straight_route(start,ziel);
		if (route.empty()) {
			intern_calc_straight_route(ziel,start);
		}
	}
}


void way_builder_t::calc_route(const koord3d &start, const koord3d &ziel)
{
	vector_tpl<koord3d> start_vec(1), ziel_vec(1);
	start_vec.append(start);
	ziel_vec.append(ziel);
	calc_route(start_vec, ziel_vec);
}

/* calc_route
 *
 */
void way_builder_t::calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel)
{
#ifdef DEBUG_ROUTES
uint32 ms = dr_time();
#endif
	INT_CHECK("simbau 740");

	if(bautyp==luft  &&  desc->get_styp()==type_runway) {
		assert( start.get_count() == 1  &&  ziel.get_count() == 1 );
		intern_calc_route_runways(start[0], ziel[0]);
	}
	else if(bautyp==river) {
		assert( start.get_count() == 1  &&  ziel.get_count() == 1 );
		// river only go downwards => start and end are clear ...
		if(  start[0].z > ziel[0].z  ) {
			intern_calc_route( start, ziel );
		}
		else {
			intern_calc_route( ziel, start );
		}
		while (route.get_count()>1  &&  welt->lookup(route[0])->get_grund_hang() == slope_t::flat  &&  welt->lookup(route[1])->is_water()) {
			// remove leading water ...
			route.remove_at(0);
		}
	}
	else {
		keep_existing_city_roads |= (bautyp&bot_flag)!=0;
		sint32 cost2;
		if(desc->get_styp() == type_elevated) {
			cost2 = intern_calc_route_elevated(start[0], ziel[0]);
			INT_CHECK("wegbauer 1165");
			if(cost2 < 0) {
				intern_calc_route_elevated(ziel[0], start[0]);
				return;
			}
		}
		else {
			cost2 = intern_calc_route( start, ziel );
			INT_CHECK("wegbauer 1165");
			if(cost2 < 0) {
				intern_calc_route( ziel, start );
				return;
			}
		}

#ifdef REVERSE_CALC_ROUTE_TOO
		vector_tpl<koord3d> route2(0);
		vector_tpl<uint32> terraform_index2(0);
		swap(route, route2);
		swap(terraform_index, terraform_index2);
		sint32 cost;
		if(desc->get_styp() == type_elevated) {
			cost = intern_calc_route_elevated(start[0], ziel[0]);
		}
		else {
			cost = intern_calc_route( start, ziel );
		}
		INT_CHECK("wegbauer 1165");

		// the cheaper will survive ...
		if(  cost2 < cost  ||  cost < 0  ) {
			swap(route, route2);
			swap(terraform_index, terraform_index2);
		}
#endif
	}
	INT_CHECK("wegbauer 778");
#ifdef DEBUG_ROUTES
DBG_MESSAGE("calc_route::calc_route", "took %u ms", dr_time() - ms );
#endif
}


void way_builder_t::build_tunnel_and_bridges()
{
	if(bridge_desc==NULL  &&  tunnel_desc==NULL) {
		return;
	}
	// check for bridges and tunnels (no tunnel/bridge at last/first tile)
	for(uint32 i=1; i<get_count()-2; i++) {
		koord d = (route[i + 1] - route[i]).get_2d();
		const koord zv(ribi_type(d));

		// ok, here is a gap ...
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			if(d.x*d.y!=0) {
				dbg->error("way_builder_t::build_tunnel_and_bridges()","cannot built a bridge between %s (n=%i, max_n=%i) and %s", route[i].get_str(),i,get_count()-1,route[i+1].get_str());
				continue;
			}

			DBG_MESSAGE("way_builder_t::build_tunnel_and_bridges", "built bridge %p between %s and %s", bridge_desc, route[i].get_str(), route[i + 1].get_str());

			const grund_t* start = welt->lookup(route[i]);
			const grund_t* end   = welt->lookup(route[i + 1]);

			if(start->get_weg_hang()!=start->get_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}
			if(end->get_weg_hang()!=end->get_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}

			if(start->get_grund_hang()==slope_t::flat  ||  start->get_grund_hang()==slope_type(zv*(-1))  ||  start->get_grund_hang()==2*slope_type(zv*(-1))) {
				// code derived from tool/simtool

				sint8 bridge_height = 0;
				const char *error;

				koord3d end = bridge_builder_t::find_end_pos(player_builder, route[i], zv, bridge_desc, error, bridge_height, false, koord_distance(route[i], route[i+1]), false);
				if (end == route[i+1]) {
					bridge_builder_t::build_bridge( player_builder, route[i], route[i+1], zv, bridge_height, bridge_desc, way_builder_t::weg_search(bridge_desc->get_waytype(), bridge_desc->get_topspeed(), welt->get_timeline_year_month(), type_flat));
				}
			}
			else {
				// tunnel
				tunnel_builder_t::build( player_builder, route[i].get_2d(), tunnel_desc, true );
			}
			INT_CHECK( "wegbauer 1584" );
		}
		// Don't build short tunnels/bridges if they block a bridge/tunnel behind!
		else if(  bautyp != leitung  &&  koord_distance(route[i + 2], route[i + 1]) == 1  ) {
			grund_t *gr_i = welt->lookup(route[i]);
			grund_t *gr_i1 = welt->lookup(route[i+1]);
			if(  gr_i->get_weg_hang() != gr_i->get_grund_hang()  ||  gr_i1->get_weg_hang() != gr_i1->get_grund_hang()  ) {
				// Here is already a tunnel or a bridge.
				continue;
			}
			slope_t::type h = gr_i->get_weg_hang();
			waytype_t const wt = desc->get_wtyp();
			if(h!=slope_t::flat  &&  slope_t::opposite(h)==gr_i1->get_weg_hang()) {
				// either a short mountain or a short dip ...
				// now: check ownership
				weg_t *wi = gr_i->get_weg(wt);
				weg_t *wi1 = gr_i1->get_weg(wt);
				if(wi->get_owner()==player_builder  &&  wi1->get_owner()==player_builder) {
					// we are the owner
					if(  h != slope_type(zv)  ) {
						// its a bridge
						if( bridge_desc ) {
							wi->set_ribi(ribi_type(h));
							wi1->set_ribi(ribi_type(slope_t::opposite(h)));
							bridge_builder_t::build( player_builder, route[i], bridge_desc);
						}
					}
					else if( tunnel_desc ) {
						// make a short tunnel
						wi->set_ribi(ribi_type(slope_t::opposite(h)));
						wi1->set_ribi(ribi_type(h));
						tunnel_builder_t::build( player_builder, route[i].get_2d(), tunnel_desc, true );
					}
					INT_CHECK( "wegbauer 1584" );
				}
			}
		}
	}
}


/*
 * returns the amount needed to built this way
 */
sint64 way_builder_t::calc_costs()
{
	sint64 costs=0;
	koord3d offset = koord3d( 0, 0, bautyp & elevated_flag ? welt->get_settings().get_way_height_clearance() : 0 );

	sint32 single_cost;
	sint32 new_speedlimit;

	if( bautyp&tunnel_flag ) {
		assert( tunnel_desc );
		single_cost = tunnel_desc->get_price();
		new_speedlimit = tunnel_desc->get_topspeed();
	}
	else {
		single_cost = desc->get_price();
		new_speedlimit = desc->get_topspeed();
	}

	// calculate costs for terraforming
	uint32 last_terraformed = terraform_index.get_count();

	for(uint32 const i : terraform_index) { // index in route
		grund_t *from = welt->lookup(route[i]);
		uint8 from_slope = from->get_grund_hang();

		grund_t *to = welt->lookup(route[i+1]);
		uint8 to_slope = to->get_grund_hang();
		// calculate new slopes
		check_terraforming(from, to, &from_slope, &to_slope);
		// change slope of from
		if (from_slope != from->get_grund_hang()) {
			if (last_terraformed != i) {
				costs -= welt->get_settings().cst_set_slope;
			}
		}
		// change slope of to
		if (to_slope != to->get_grund_hang()) {
			costs -= welt->get_settings().cst_set_slope;
			last_terraformed = i+1; // do not pay twice for terraforming one tile
		}
	}

	for(uint32 i=0; i<get_count(); i++) {
		sint32 old_speedlimit = -1;
		sint32 replace_cost = 0;

		const grund_t* gr = welt->lookup(route[i] + offset);
		if( gr ) {
			if( bautyp&tunnel_flag ) {
				const tunnel_t *tunnel = gr->find<tunnel_t>();
				assert( tunnel );
				if( tunnel->get_desc() == tunnel_desc ) {
					continue; // Nothing to pay on this tile.
				}
				old_speedlimit = tunnel->get_desc()->get_topspeed();
			}
			else {
				if(  desc->get_wtyp() == powerline_wt  ) {
					if( leitung_t *lt=gr->get_leitung() ) {
						old_speedlimit = lt->get_desc()->get_topspeed();
					}
				}
				else {
					if (weg_t const* const weg = gr->get_weg(desc->get_wtyp())) {
						replace_cost = weg->get_desc()->get_price();
						if( weg->get_desc() == desc ) {
							continue; // Nothing to pay on this tile.
						}
						if(  desc->get_styp() == type_flat  &&  weg->get_desc()->get_styp() == type_tram  &&  gr->get_weg_nr(0)->get_waytype() == road_wt  ) {
							// Don't replace a tram on a road with a normal track.
							continue;
						}
						old_speedlimit = weg->get_desc()->get_topspeed();
					}
					else if (desc->get_wtyp()==water_wt  &&  gr->is_water()) {
						old_speedlimit = new_speedlimit;
					}
				}
			}
			// eventually we have to remove trees
			for(  uint8 i=0;  i<gr->get_top();  i++  ) {
				obj_t *obj = gr->obj_bei(i);
				switch(obj->get_typ()) {
					case obj_t::baum:
						costs -= welt->get_settings().cst_remove_tree;
						break;
					case obj_t::groundobj:
						costs += ((groundobj_t *)obj)->get_desc()->get_price();
						break;
					default: break;
				}
			}
		}
		if(  !keep_existing_faster_ways  ||  old_speedlimit < new_speedlimit  ) {
			costs += max(single_cost, replace_cost);
		}

		// last tile cannot be start of tunnel/bridge
		if(i+1<get_count()) {
			koord d = (route[i + 1] - route[i]).get_2d();
			// ok, here is a gap ... => either bridge or tunnel
			if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {
				koord zv = koord (sgn(d.x), sgn(d.y));
				const grund_t* start = welt->lookup(route[i]);
				const grund_t* end   = welt->lookup(route[i + 1]);
				if(start->get_weg_hang()!=start->get_grund_hang()) {
					// already a bridge/tunnel there ...
					continue;
				}
				if(end->get_weg_hang()!=end->get_grund_hang()) {
					// already a bridge/tunnel there ...
					continue;
				}
				if(start->get_grund_hang()==0  ||  start->get_grund_hang()==slope_type(zv*(-1))) {
					// bridge
					costs += bridge_desc->get_price()*(sint64)(koord_distance(route[i], route[i+1])+1);
					continue;
				}
				else {
					// tunnel
					costs += tunnel_desc->get_price()*(sint64)(koord_distance(route[i], route[i+1])+1);
					continue;
				}
			}
		}
	}
	DBG_MESSAGE("way_builder_t::calc_costs()","construction estimate: %f",costs/100.0);
	return costs;
}


// adds the ground before underground construction
bool way_builder_t::build_tunnel_tile()
{
	sint64 cost = 0;
	for(uint32 i=0; i<get_count(); i++) {

		grund_t* gr = welt->lookup(route[i]);

		const way_desc_t *wb = tunnel_desc->get_way_desc();
		if(wb==NULL) {
			// now we search a matching way for the tunnels top speed
			// ignore timeline to get consistent results
			wb = way_builder_t::weg_search( tunnel_desc->get_waytype(), tunnel_desc->get_topspeed(), 0, type_flat );
		}

		if(gr==NULL) {
			// make new tunnelboden
			tunnelboden_t* tunnel = new tunnelboden_t(route[i], slope_t::flat);
			welt->access(route[i].get_2d())->boden_hinzufuegen(tunnel);
			if(  tunnel_desc->get_waytype() != powerline_wt  ) {
				weg_t *weg = weg_t::alloc(tunnel_desc->get_waytype());
				weg->set_desc( wb );
				tunnel->neuen_weg_bauen(weg, route.get_ribi(i), player_builder);
				tunnel->obj_add(new tunnel_t(route[i], player_builder, tunnel_desc));
				weg->set_max_speed(tunnel_desc->get_topspeed());
				player_t::add_maintenance( player_builder, -weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
			}
			else {
				tunnel->obj_add(new tunnel_t(route[i], player_builder, tunnel_desc));
				leitung_t *lt = new leitung_t(tunnel->get_pos(), player_builder);
				lt->set_desc( wb );
				tunnel->obj_add( lt );
				lt->finish_rd();
			}
			tunnel->calc_image();
			cost -= tunnel_desc->get_price();
			player_t::add_maintenance( player_builder,  tunnel_desc->get_maintenance(), tunnel_desc->get_finance_waytype() );
		}
		else if(  gr->get_typ() == grund_t::tunnelboden  ) {
			// check for extension only ...
			if(  tunnel_desc->get_waytype() != powerline_wt  ) {
				gr->weg_erweitern( tunnel_desc->get_waytype(), route.get_ribi(i) );

				tunnel_t *tunnel = gr->find<tunnel_t>();
				assert( tunnel );
				// take the faster way
				if(  !keep_existing_faster_ways  ||  (tunnel->get_desc()->get_topspeed() < tunnel_desc->get_topspeed())  ) {
					player_t::add_maintenance(player_builder, -tunnel->get_desc()->get_maintenance(), tunnel->get_desc()->get_finance_waytype());
					player_t::add_maintenance(player_builder,  tunnel_desc->get_maintenance(), tunnel->get_desc()->get_finance_waytype() );

					tunnel->set_desc(tunnel_desc);
					weg_t *weg = gr->get_weg(tunnel_desc->get_waytype());
					weg->set_desc(wb);
					weg->set_max_speed(tunnel_desc->get_topspeed());
					// respect max speed of catenary
					wayobj_t const* const wo = gr->get_wayobj(tunnel_desc->get_waytype());
					if (wo  &&  wo->get_desc()->get_topspeed() < weg->get_max_speed()) {
						weg->set_max_speed( wo->get_desc()->get_topspeed() );
					}
					gr->calc_image();
					// respect speed limit of crossing
					weg->count_sign();

					cost -= tunnel_desc->get_price();
				}
			}
			else {
				leitung_t *lt = gr->get_leitung();
				if(!lt) {
					lt = new leitung_t(gr->get_pos(), player_builder);
					lt->set_desc( wb );
					gr->obj_add( lt );
				}
			}
		}
	}
	player_t::book_construction_costs(player_builder, cost, route[0].get_2d(), tunnel_desc->get_waytype());
	return true;
}



void way_builder_t::build_elevated()
{
	for(koord3d & i : route) {
		planquadrat_t* const plan = welt->access(i.get_2d());

		grund_t* const gr0 = plan->get_boden_in_hoehe(i.z);
		i.z += welt->get_settings().get_way_height_clearance();
		grund_t* const gr  = plan->get_boden_in_hoehe(i.z);

		if(gr==NULL) {
			slope_t::type hang = gr0 ? gr0->get_grund_hang() : 0;
			// add new elevated ground
			monorailboden_t* const monorail = new monorailboden_t(i, hang);
			plan->boden_hinzufuegen(monorail);
			monorail->calc_image();
		}
	}
}



void way_builder_t::build_road()
{
	// only public player or cities (sp==NULL) can build cityroads with sidewalk
	bool add_sidewalk = build_sidewalk  &&  (player_builder==NULL  ||  player_builder->is_public_service());

	if(add_sidewalk) {
		player_builder = NULL;
	}

	// init undo
	if(player_builder!=NULL) {
		// intercity roads have no owner, so we must check for an owner
		player_builder->init_undo(road_wt,get_count());
	}

	for(  uint32 i=0;  i<get_count();  i++  ) {
		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}

		const koord k = route[i].get_2d();
		grund_t* gr = welt->lookup(route[i]);
		sint64 cost = 0;

		bool extend = gr->weg_erweitern(road_wt, route.get_short_ribi(i));

		// bridges/tunnels have their own track type and must not upgrade
		if(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden) {
			continue;
		}

		if(extend) {
			weg_t * weg = gr->get_weg(road_wt);

			// keep faster ways or if it is the same way
			if(weg->get_desc()==desc  ||  keep_existing_ways
				||  (keep_existing_city_roads  &&  weg->hat_gehweg())
				||  (keep_existing_faster_ways  &&  weg->get_desc()->get_topspeed()>desc->get_topspeed())
				||  (player_builder!=NULL  &&  weg->is_deletable(player_builder)!=NULL)
				||  (gr->get_typ()==grund_t::monorailboden && (bautyp&elevated_flag)==0)
				||  (gr->has_two_ways()  &&  gr->get_weg_nr(1)->is_deletable(player_builder)!=NULL) // do not replace public roads crossing rails of other players
			) {
				//nothing to be done
			}
			else {
				// we take ownership => we take care to maintain the roads completely ...
				player_t *s = weg->get_owner();
				player_t::add_maintenance(s, -weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
				// cost is the more expensive one, so downgrading is between removing and new building
				cost -= max( weg->get_desc()->get_price(), desc->get_price() );
				weg->set_desc(desc);
				// respect max speed of catenary
				wayobj_t const* const wo = gr->get_wayobj(desc->get_wtyp());
				if (wo  &&  wo->get_desc()->get_topspeed() < weg->get_max_speed()) {
					weg->set_max_speed( wo->get_desc()->get_topspeed() );
				}
				weg->set_gehweg(add_sidewalk);
				player_t::add_maintenance( player_builder, weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
				weg->set_owner(player_builder);
				// respect speed limit of crossing
				weg->count_sign();
			}
		}
		else {
			// make new way
			strasse_t * str = new strasse_t();

			str->set_desc(desc);
			str->set_gehweg(add_sidewalk);
			cost = -gr->neuen_weg_bauen(str, route.get_short_ribi(i), player_builder)-desc->get_price();

			// into UNDO-list, so we can remove it later
			if(player_builder!=NULL) {
				// intercity roads have no owner, so we must check for an owner
				player_builder->add_undo( route[i] );
			}
		}
		gr->calc_image(); // because it may be a crossing ...
		minimap_t::get_instance()->calc_map_pixel(k);
		player_t::book_construction_costs(player_builder, cost, k, road_wt);
	} // for
}


void way_builder_t::build_track()
{
	if(get_count() > 1) {
		// init undo
		player_builder->init_undo(desc->get_wtyp(), get_count());

		// built tracks
		for(  uint32 i=0;  i<get_count();  i++  ) {
			sint64 cost = 0;
			grund_t* gr = welt->lookup(route[i]);
			ribi_t::ribi ribi = route.get_short_ribi(i);

			if(gr->get_typ()==grund_t::wasser) {
				// not building on the sea ...
				continue;
			}

			bool const extend = gr->weg_erweitern(desc->get_wtyp(), ribi);

			// bridges/tunnels have their own track type and must not upgrade
			if((gr->get_typ()==grund_t::brueckenboden ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->get_weg_nr(0)->get_waytype()==desc->get_wtyp()) {
				continue;
			}

			if(extend) {
				weg_t* const weg = gr->get_weg(desc->get_wtyp());
				bool change_desc = true;

				// do not touch fences, tram way etc. if there is already same way with different type
				// keep faster ways or if it is the same way
				if (weg->get_desc() == desc                                                               ||
						(desc->get_styp() == 0 && weg->get_desc()->get_styp() == type_tram  && gr->has_two_ways())     ||
						keep_existing_ways                                                                      ||
						(keep_existing_faster_ways && weg->get_desc()->get_topspeed() > desc->get_topspeed()) ||
						(gr->get_typ() == grund_t::monorailboden && !(bautyp & elevated_flag)  &&  gr->get_weg_nr(0)->get_waytype()==desc->get_wtyp())) {
					//nothing to be done
					change_desc = false;
				}

				// build tram track over crossing -> remove crossing
				if(  gr->has_two_ways()  &&  desc->get_styp()==type_tram  &&  weg->get_desc()->get_styp() != type_tram  ) {
					if(  crossing_t *cr = gr->find<crossing_t>(2)  ) {
						// change to tram track
						cr->mark_image_dirty( cr->get_image(), 0);
						cr->cleanup(player_builder);
						delete cr;
						change_desc = true;
						// tell way we have no crossing any more, restore speed limit
						gr->get_weg_nr(0)->clear_crossing();
						gr->get_weg_nr(0)->set_desc( gr->get_weg_nr(0)->get_desc() );
						gr->get_weg_nr(1)->clear_crossing();
					}
				}

				if(  change_desc  ) {
					// we take ownership => we take care to maintain the roads completely ...
					player_t *s = weg->get_owner();
					player_t::add_maintenance( s, -weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
					// cost is the more expensive one, so downgrading is between removing and new buidling
					cost -= max( weg->get_desc()->get_price(), desc->get_price() );
					weg->set_desc(desc);
					// respect max speed of catenary
					wayobj_t const* const wo = gr->get_wayobj(desc->get_wtyp());
					if (wo  &&  wo->get_desc()->get_topspeed() < weg->get_max_speed()) {
						weg->set_max_speed( wo->get_desc()->get_topspeed() );
					}
					player_t::add_maintenance( player_builder, weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
					weg->set_owner(player_builder);
					// respect speed limit of crossing
					weg->count_sign();
				}
			}
			else {
				weg_t* const sch = weg_t::alloc(desc->get_wtyp());
				sch->set_desc(desc);

				cost = -gr->neuen_weg_bauen(sch, ribi, player_builder)-desc->get_price();

				// connect canals to sea
				if(  desc->get_wtyp() == water_wt  ) {
					// do not connect across slopes
					ribi_t::ribi slope_ribi = ribi_t::all;
					if (gr->get_grund_hang()!=slope_t::flat) {
						slope_ribi = ribi_t::doubles( ribi_type(gr->get_weg_hang()) );
					}

					for(  int j = 0;  j < 4;  j++  ) {
						if (ribi_t::nesw[j] & slope_ribi) {
							grund_t *sea = NULL;
							if (gr->get_neighbour(sea, invalid_wt, ribi_t::nesw[j])  &&  sea->is_water()  ) {
								gr->weg_erweitern( water_wt, ribi_t::nesw[j] );
								sea->calc_image();
							}
						}
					}
				}

				// into UNDO-list, so we can remove it later
				player_builder->add_undo( route[i] );
			}

			gr->calc_image();
			minimap_t::get_instance()->calc_map_pixel( gr->get_pos().get_2d() );
			player_t::book_construction_costs(player_builder, cost, gr->get_pos().get_2d(), desc->get_finance_waytype());

			if((i&3)==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



void way_builder_t::build_powerline()
{
	if(  get_count() < 1  ) {
		return;
	}

	// no undo
	player_builder->init_undo(powerline_wt,get_count());

	for(  uint32 i=0;  i<get_count();  i++  ) {
		grund_t* gr = welt->lookup(route[i]);

		leitung_t* lt = gr->get_leitung();
		bool build_powerline = false;
		// ok, really no lt here ...
		if(lt==NULL) {
			if(gr->ist_natur()) {
				// remove trees etc.
				sint64 cost = gr->remove_trees();
				player_t::book_construction_costs(player_builder, -cost, gr->get_pos().get_2d(), powerline_wt);
			}
			lt = new leitung_t(route[i], player_builder );
			gr->obj_add(lt);

			// into UNDO-list, so we can remove it later
			player_builder->add_undo( route[i] );
			build_powerline = true;
		}
		else {
			// modernize the network
			if(lt->get_typ() == obj_t::leitung  && (!keep_existing_faster_ways  ||  lt->get_desc()->get_topspeed() < desc->get_topspeed())  ) {
				build_powerline = true;
				player_t::add_maintenance( lt->get_owner(),  -lt->get_maintenance(), powerline_wt );
			}
		}
		if (build_powerline) {
			lt->set_desc(desc);
			player_t::book_construction_costs(player_builder, -desc->get_price(), gr->get_pos().get_2d(), powerline_wt);
			// this adds maintenance
			lt->leitung_t::finish_rd();
			minimap_t::get_instance()->calc_map_pixel( gr->get_pos().get_2d() );
		}

		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}
	}
}



// this can drive any river, even a river that has max_speed=0
class fluss_test_driver_t : public test_driver_t
{
	bool check_next_tile(const grund_t* gr) const OVERRIDE { return gr->get_weg_ribi_unmasked(water_wt)!=0; }
	ribi_t::ribi get_ribi(const grund_t* gr) const OVERRIDE { return gr->get_weg_ribi_unmasked(water_wt); }
	waytype_t get_waytype() const OVERRIDE { return invalid_wt; }
	int get_cost(const grund_t *, const weg_t *, const sint32, ribi_t::ribi) const OVERRIDE { return 1; }
	bool is_target(const grund_t *cur,const grund_t *) const OVERRIDE { return cur->is_water()  &&  cur->get_hoehe()<=world()->get_groundwater(); }
};


// make a river
void way_builder_t::build_river()
{
	/* since the constraints of the wayfinder ensures that a river flows always downwards
	 * we can assume that the first tiles are the ocean.
	 * Usually the wayfinder would find either direction!
	 * route.front() tile at the ocean, route.back() the spring of the river
	 */

	uint32 start_n = 0;
	uint32 end_n = get_count();
	// first find coast ...
	for(  uint32 idx=start_n;  idx<get_count();  idx++  ) {
		if(  welt->lookup(route[idx])->get_hoehe() == welt->get_water_hgt(route[idx].get_2d())  ) {
			start_n = idx;
		}
		else {
			break;
		}
	}
	// Do we join an other river?
	for(  uint32 idx=start_n;  idx<get_count();  idx++  ) {
		if( welt->lookup( route[idx] )->get_weg(water_wt)  ) {
			// river or channel
			start_n = idx;
		}
	}
	// No spring on a slope
	while(  end_n>start_n  ) {
		if(  welt->lookup( route[end_n-1] )->get_grund_hang() == slope_t::flat  ) {
			break;
		}
		end_n --;
	}
	if(  start_n == get_count()-1  ) {
		// completely joined another river => nothing to do
		return;
	}

	// first check then lower riverbed
	sint8 start_h = max(route[start_n].z,welt->get_groundwater());
	vector_tpl<bool> lower_tile( end_n );    // This contains the tiles which can be lowered.
	vector_tpl<sint8> lower_tile_h( end_n ); // to this value

	// first: find possible valley
	for( uint32 i=0;  i < end_n;  i++  ) {

		bool can_lower = true;

		if(  route[i].z < start_h   ) {
			// skip all tiles below last sea level
			can_lower = false;
		}

		grund_t *gr = welt->lookup( route[i] );
		if(  gr->is_water()  ) {
			// not lowering the sea or lakes
			start_h = welt->get_water_hgt( route[i].get_2d() );
			can_lower = false;
		}

		if(  gr->hat_weg(water_wt)  ) {
			// not lowering exiting rivers
			start_h = route[i].z;
			can_lower = false;
		}

		// check
		if(  can_lower  ) {
			can_lower = welt->can_flatten_tile( NULL, route[i].get_2d(), max( route[i].z-1, start_h ) );
		}

		lower_tile.append( can_lower );
		lower_tile_h.append( start_h );

		sint8 current_h = route[i].z+slope_t::max_diff(gr->get_grund_hang());
		if(  current_h > start_h + 1  ) {
			start_h++;
		}
	}

	// now lower all tiles
	for(  uint32 i=start_n;  i<end_n;  i++  ) {
		if(  lower_tile[i]  ) {
			if(  !welt->flatten_tile( NULL, route[i].get_2d(), lower_tile_h[i] ) ) {
				// illegal slope encountered, give up ...
				return;
			}
		}
	}

	// and raise all tiles that resulted in slopes on curves
	for( uint32 i = max(end_n, 2) - 2; i > start_n+1;   i-- ) {
		grund_t *gr = welt->lookup_kartenboden( route[i].get_2d() );
		if(  !gr->get_weg_ribi( water_wt )  &&  gr->get_grund_hang()!=slope_t::flat  ) {
			if(  !ribi_t::is_straight(ribi_type(route[i+1].get_2d()-route[i-1].get_2d()))  ) {
				welt->flatten_tile( NULL, route[i].get_2d(), gr->get_hoehe()+1 );
				DBG_MESSAGE( "wrong river slope", route[i].get_str() );
			}
		}
	}

	// now build the river
	uint32 last_common_water_tile = start_n;
	grund_t *gr_first = NULL;
	for(  uint32 i=start_n;  i<end_n;  i++  ) {
		grund_t* gr = welt->lookup_kartenboden(route[i].get_2d());
		if(  gr_first == NULL) {
			gr_first = gr;
		}
		if(  gr->get_typ()!=grund_t::wasser  ) {
			// get direction
			ribi_t::ribi ribi = i<end_n-1 ? route.get_short_ribi(i) : ribi_type(route[i-1]-route[i]);
			bool extend = gr->weg_erweitern(water_wt, ribi);
			if(  !extend  ) {
				weg_t *river=weg_t::alloc(water_wt);
				river->set_desc(desc);
				gr->neuen_weg_bauen(river, ribi, NULL);
			}
			else {
				last_common_water_tile = i;
			}
		}
		else {
			dynamic_cast<wasser_t *>(gr)->recalc_water_neighbours();
			last_common_water_tile = i;
		}
	}
	gr_first->calc_image(); // to calculate ribi of water tiles

	// we will make rivers gradually larger by stepping up their width
	// Since we cannot quickly find out, if a lake has another influx, we just assume all rivers after a lake are navigatable
	if(  env_t::river_types>1  ) {
		// now all rivers will end in the sea (or a lake) and thus there must be a valid route!
		// unless weare the first and there is no lake on the way.
		route_t to_the_sea;
		fluss_test_driver_t river_tester;
		if (to_the_sea.calc_route(welt, welt->lookup_kartenboden(route[last_common_water_tile].get_2d())->get_pos(), welt->lookup_kartenboden(route[0].get_2d())->get_pos(), &river_tester, 0, 0x7FFFFFFF)) {
			for(koord3d const& i : to_the_sea.get_route()) {
				if (weg_t* const w = welt->lookup(i)->get_weg(water_wt)) {
					int type;
					for(  type=env_t::river_types-1;  type>0;  type--  ) {
						// lookup type
						if(  w->get_desc()==desc_table.get(env_t::river_type[type])  ) {
								break;
						}
					}
					// still room to expand
					if(  type>0  ) {
						// thus we enlarge
						w->set_desc( desc_table.get(env_t::river_type[type-1]) );
						w->calc_image();
					}
				}
			}
		}
	}
}



void way_builder_t::build()
{
	if(get_count()<2  ||  get_count() > maximum) {
DBG_MESSAGE("way_builder_t::build()","called, but no valid route.");
		// no valid route here ...
		return;
	}
	DBG_MESSAGE("way_builder_t::build()", "type=%d max_n=%d start=%d,%d end=%d,%d", bautyp, get_count() - 1, route.front().x, route.front().y, route.back().x, route.back().y);

#ifdef DEBUG_ROUTES
uint32 ms = dr_time();
#endif

	if ( (bautyp&terraform_flag)!=0  &&  (bautyp&(tunnel_flag|elevated_flag))==0  &&  bautyp!=river) {
		// do the terraforming
		do_terraforming();
	}
	// first add all new underground tiles ... (and finished if successful)
	if(bautyp&tunnel_flag) {
		build_tunnel_tile();
		return;
	}

	// add elevated ground for elevated tracks
	if(bautyp&elevated_flag) {
		build_elevated();
	}


INT_CHECK("simbau 1072");
	switch(bautyp&bautyp_mask) {
		case wasser:
		case schiene:
		case schiene_tram: // Dario: Tramway
		case monorail:
		case maglev:
		case narrowgauge:
		case luft:
			DBG_MESSAGE("way_builder_t::build", "schiene");
			build_track();
			break;
		case strasse:
			build_road();
			DBG_MESSAGE("way_builder_t::build", "strasse");
			break;
		case leitung:
			build_powerline();
			break;
		case river:
			build_river();
			break;
		default:
			break;
	}

	INT_CHECK("simbau 1087");
	build_tunnel_and_bridges();

	INT_CHECK("simbau 1087");

#ifdef DEBUG_ROUTES
DBG_MESSAGE("way_builder_t::build", "took %u ms", dr_time() - ms );
#endif
}



/*
 * This function calculates the distance of pos to the cuboid
 * spanned up by mini and maxi.
 * The result is already weighted according to
 * welt->get_settings().get_way_count_{straight,slope}().
 */
uint32 way_builder_t::calc_distance( const koord3d &pos, const koord3d &mini, const koord3d &maxi )
{
	uint32 dist = 0;
	if( pos.x < mini.x ) {
		dist += mini.x - pos.x;
	} else if( pos.x > maxi.x ) {
		dist += pos.x - maxi.x;
	}
	if( pos.y < mini.y ) {
		dist += mini.y - pos.y;
	} else if( pos.y > maxi.y ) {
		dist += pos.y - maxi.y;
	}
	settings_t const& s = welt->get_settings();
	dist *= s.way_count_straight;
	if( pos.z < mini.z ) {
		dist += (mini.z - pos.z) * s.way_count_slope;
	} else if( pos.z > maxi.z ) {
		dist += (pos.z - maxi.z) * s.way_count_slope;
	}
	return dist;
}
