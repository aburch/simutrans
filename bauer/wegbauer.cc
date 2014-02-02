/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Ways (Roads, Railways, etc.)
 *
 * Hj. Malthaner
 */

#include <algorithm>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simmesg.h"
#include "../simintr.h"
#include "../player/simplay.h"
#include "../simplan.h"
#include "../simtools.h"
#include "../simdepot.h"

#include "wegbauer.h"
#include "brueckenbauer.h"
#include "tunnelbauer.h"

#include "../besch/weg_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"

#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/maglev.h"
#include "../boden/wege/narrowgauge.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/runway.h"
#include "../boden/brueckenboden.h"
#include "../boden/monorailboden.h"
#include "../boden/tunnelboden.h"
#include "../boden/grund.h"

#include "../dataobj/environment.h"
#include "../dataobj/route.h"
#include "../dataobj/marker.h"
#include "../dataobj/translator.h"
#include "../dataobj/scenario.h"

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

#include "../ifc/fahrer.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../gui/karte.h"	// for debugging
#include "../gui/werkzeug_waehler.h"
#include "../gui/messagebox.h"

#ifdef DEBUG_ROUTES
#include "../simsys.h"
#endif

// built bridges automatically
//#define AUTOMATIC_BRIDGES

// built tunnels automatically
//#define AUTOMATIC_TUNNELS

// lookup also return route and take the better of the two
#define REVERSE_CALC_ROUTE_TOO

karte_ptr_t wegbauer_t::welt;

const weg_besch_t *wegbauer_t::leitung_besch = NULL;

static stringhashtable_tpl <const weg_besch_t *> alle_wegtypen;


static void set_default(weg_besch_t const*& def, waytype_t const wtyp, sint32 const speed_limit = 1)
{
	def = wegbauer_t::weg_search(wtyp, speed_limit, 0, weg_t::type_flat);
}


static void set_default(weg_besch_t const*& def, waytype_t const wtyp, weg_t::system_type const system_type, sint32 const speed_limit = 1)
{
	set_default(def, wtyp, speed_limit);
	if (def) {
		return;
	}
	def = wegbauer_t::weg_search(wtyp, 1, 0, system_type);
}


bool wegbauer_t::alle_wege_geladen()
{
	// some defaults to avoid hardcoded values
	set_default(strasse_t::default_strasse,         road_wt,        weg_t::type_flat, 50);
	if(  strasse_t::default_strasse == NULL ) {
		dbg->fatal( "wegbauer_t::alle_wege_geladen()", "No road found at all!" );
		return false;
	}

	set_default(schiene_t::default_schiene,         track_wt,       weg_t::type_flat, 80);
	set_default(monorail_t::default_monorail,       monorail_wt,    weg_t::type_elevated); // Only elevated?
	set_default(maglev_t::default_maglev,           maglev_wt,      weg_t::type_elevated); // Only elevated?
	set_default(narrowgauge_t::default_narrowgauge, narrowgauge_wt);
	set_default(kanal_t::default_kanal,             water_wt,       weg_t::type_all); // Also find hidden rivers.
	set_default(runway_t::default_runway,           air_wt);
	set_default(wegbauer_t::leitung_besch,          powerline_wt);

	return true;
}


bool wegbauer_t::register_besch(weg_besch_t *besch)
{
	DBG_DEBUG("wegbauer_t::register_besch()", besch->get_name());
	const weg_besch_t *old_besch = alle_wegtypen.get(besch->get_name());
	if(  old_besch  ) {
		alle_wegtypen.remove(besch->get_name());
		dbg->warning( "wegbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		werkzeug_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// add the tool
		wkz_wegebau_t *wkz = new wkz_wegebau_t();
		wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
		wkz->cursor = besch->get_cursor()->get_bild_nr(0);
		wkz->set_default_param(besch->get_name());
		werkzeug_t::general_tool.append( wkz );
		besch->set_builder( wkz );
	}
	else {
		besch->set_builder( NULL );
	}
	alle_wegtypen.put(besch->get_name(), besch);
	return true;
}

/**
 * Finds a way with a given speed limit for a given waytype
 * It finds:
 *  - the slowest way, as fast as speed limit
 *  - if no way faster than speed limit, the fastest way.
 * The timeline is also respected.
 * @author prissi, gerw
 */
const weg_besch_t* wegbauer_t::weg_search(const waytype_t wtyp, const sint32 speed_limit, const uint16 time, const weg_t::system_type system_type)
{
	const weg_besch_t* best = NULL;
	bool best_allowed = false; // Does the best way fulfil the timeline?
	FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
		weg_besch_t const* const test = i.value;
		if(  ((test->get_wtyp()==wtyp  &&
			(test->get_styp()==system_type  ||  system_type==weg_t::type_all))  ||  (test->get_wtyp()==track_wt  &&  test->get_styp()==weg_t::type_tram  &&  wtyp==tram_wt))
			&&  test->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
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



const weg_besch_t *wegbauer_t::get_earliest_way(const waytype_t wtyp)
{
	const weg_besch_t *besch = NULL;
	FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
		weg_besch_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  (besch==NULL  ||  test->get_intro_year_month()<besch->get_intro_year_month())  ) {
			besch = test;
		}
	}
	return besch;
}



const weg_besch_t *wegbauer_t::get_latest_way(const waytype_t wtyp)
{
	const weg_besch_t *besch = NULL;
	FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
		weg_besch_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  (besch==NULL  ||  test->get_retire_year_month()>besch->get_retire_year_month())  ) {
			besch = test;
		}
	}
	return besch;
}


// ture if the way is available with timely
bool wegbauer_t::waytype_available( const waytype_t wtyp, uint16 time )
{
	if(  time==0  ) {
		return true;
	}

	FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
		weg_besch_t const* const test = i.value;
		if(  test->get_wtyp()==wtyp  &&  test->get_intro_year_month()<=time  &&  test->get_retire_year_month()>time  ) {
			return true;
		}
	}
	return false;
}



const weg_besch_t * wegbauer_t::get_besch(const char * way_name, const uint16 time)
{
//DBG_MESSAGE("wegbauer_t::get_besch","return besch for %s in (%i)",way_name, time/12);
	const weg_besch_t *besch = alle_wegtypen.get(way_name);
	if(  besch  &&  besch->is_available(time)  ) {
		return besch;
	}
	return NULL;
}



// generates timeline message
void wegbauer_t::neuer_monat()
{
	const uint16 current_month = welt->get_timeline_year_month();
	if(current_month!=0) {
		// check, what changed
		slist_tpl <const weg_besch_t *> matching;
		FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
			weg_besch_t const* const besch = i.value;
			cbuffer_t buf;

			const uint16 intro_month = besch->get_intro_year_month();
			if(intro_month == current_month) {
				buf.printf( translator::translate("way %s now available:\n"), translator::translate(besch->get_name()) );
				welt->get_message()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->get_bild_nr(5,0));
			}

			const uint16 retire_month = besch->get_retire_year_month();
			if(retire_month == current_month) {
				buf.printf( translator::translate("way %s cannot longer used:\n"), translator::translate(besch->get_name()) );
				welt->get_message()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->get_bild_nr(5,0));
			}
		}

	}
}


static bool compare_ways(const weg_besch_t* a, const weg_besch_t* b)
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
 * @author Hj. Malthaner/prissi/dariok
 */
void wegbauer_t::fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, const weg_t::system_type styp, sint16 /*ok_sound*/)
{
	// check if scenario forbids this
	const waytype_t rwtyp = wtyp!=track_wt  || styp!=weg_t::type_tram  ? wtyp : tram_wt;
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), WKZ_WEGEBAU | GENERAL_TOOL, rwtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();

	// list of matching types (sorted by speed)
	vector_tpl<const weg_besch_t*> matching;

	FOR(stringhashtable_tpl<weg_besch_t const*>, const& i, alle_wegtypen) {
		weg_besch_t const* const besch = i.value;
		if (  besch->get_styp()==styp &&  besch->get_wtyp()==wtyp  &&  besch->get_builder()  &&  besch->is_available(time)  ) {
				matching.append(besch);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_ways);

	// now add sorted ways ...
	FOR(vector_tpl<weg_besch_t const*>, const i, matching) {
		wzw->add_werkzeug(i->get_builder());
	}
}


/* allow for railroad crossing
 * @author prissi
 */
bool wegbauer_t::check_crossing(const koord zv, const grund_t *bd, waytype_t wtyp0, const spieler_t *sp) const
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
	if ( (wtyp==road_wt  &&  w->get_waytype()==track_wt  &&  w->get_besch()->get_styp()==7)  ||
		     (wtyp0==tram_wt  &&  w->get_waytype()==road_wt) ) {
		return true;
	}
	// right owner of the other way
	if(!check_owner(w->get_besitzer(),sp)) {
		return false;
	}
	// check for existing crossing
	crossing_t *cr = bd->find<crossing_t>();
	if (cr) {
		// index of the waytype in ns-direction at the crossing
		const uint8 ns_way = cr->get_dir();
		// only cross with the right direction
		return (ns_way==iwtyp ? ribi_t::ist_gerade_ns(ribi_typ(zv)) : ribi_t::ist_gerade_ow(ribi_typ(zv)));
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
	if(crossing_logic_t::get_crossing(wtyp, w->get_waytype(), 0, 0, welt->get_timeline_year_month())!=NULL) {
		ribi_t::ribi w_ribi = w->get_ribi_unmasked();
		// it is our way we want to cross: can we built a crossing here?
		// both ways must be straight and no ends
		return  ribi_t::ist_gerade(w_ribi)
					&&  !ribi_t::ist_einfach(w_ribi)
					&&  ribi_t::ist_gerade(ribi_typ(zv))
				&&  (w_ribi&ribi_typ(zv))==0;
	}
	// cannot build crossing here
	return false;
}


/* crossing of powerlines, or no powerline
 * @author prissi
 */
bool wegbauer_t::check_for_leitung(const koord zv, const grund_t *bd) const
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
		  ribi_t::ist_gerade(lt_ribi)
		  &&  !ribi_t::ist_einfach(lt_ribi)
		  &&  ribi_t::ist_gerade(ribi_typ(zv))
		  &&  (lt_ribi&ribi_typ(zv))==0
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
bool wegbauer_t::check_slope( const grund_t *from, const grund_t *to )
{
	const koord from_pos=from->get_pos().get_2d();
	const koord to_pos=to->get_pos().get_2d();
	const koord zv=to_pos-from_pos;

	if(  !besch->has_double_slopes()
	  &&  ((from->get_typ() == grund_t::boden  &&  from->get_weg_hang()  &&  !(from->get_weg_hang() & 7))
	  ||  (to->get_typ() == grund_t::boden  &&  to->get_weg_hang()  &&  !(to->get_weg_hang() & 7)) )  ) {
		return false;
	}

	if(from==to) {
		if(!hang_t::ist_wegbar(from->get_weg_hang())) {
			return false;
		}
	}
	else {
		if(from->get_weg_hang()  &&  ribi_t::doppelt(ribi_typ(from->get_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
			return false;
		}
		if(to->get_weg_hang()  &&  ribi_t::doppelt(ribi_typ(to->get_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
			return false;
		}
	}

	return true;
}


// allowed owner?
bool wegbauer_t::check_owner( const spieler_t *sp1, const spieler_t *sp2 ) const
{
	// unowned, mine or public property or superuser ... ?
	return sp1==NULL  ||  sp1==sp2  ||  sp1==welt->get_spieler(1)  ||  sp2==welt->get_spieler(1);
}


/* do not go through depots, station buildings etc. ...
 * direction results from layout
 */
bool wegbauer_t::check_building( const grund_t *to, const koord dir ) const
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
		uint8 layouts = gb->get_tile()->get_besch()->get_all_layouts();
		uint8 layout = gb->get_tile()->get_layout();
		ribi_t::ribi r = ribi_typ(dir);
		if(  layouts&1  ) {
			return false;
		}
		if(  layouts==4  ) {
			return  r == ribi_t::layout_to_ribi[layout];
		}
		return ribi_t::ist_gerade( r | ribi_t::doppelt(ribi_t::layout_to_ribi[layout&1]) );
	}
	return true;
}


/* This is the core routine for the way search
 * it will check
 * A) allowed step
 * B) if allowed, calculate the cost for the step from from to to
 * @author prissi
 */
bool wegbauer_t::is_allowed_step( const grund_t *from, const grund_t *to, long *costs )
{
	const koord from_pos=from->get_pos().get_2d();
	const koord to_pos=to->get_pos().get_2d();
	const koord zv=to_pos-from_pos;
	// fake empty elevated tiles
	static monorailboden_t to_dummy(koord3d::invalid, hang_t::flach);
	static monorailboden_t from_dummy(koord3d::invalid, hang_t::flach);


	if(bautyp==luft  &&  (from->get_grund_hang()+to->get_grund_hang()!=0  ||  (from->hat_wege()  &&  from->hat_weg(air_wt)==0)  ||  (to->hat_wege()  &&  to->hat_weg(air_wt)==0))) {
		// absolutely no slopes for runways, neither other ways
		return false;
	}

	bool to_flat = false; // to tile will be flattened
	if(from==to) {
		if((bautyp&tunnel_flag)  &&  !hang_t::ist_wegbar(from->get_weg_hang())) {
			return false;
		}
	}
	else {
		// check slopes
		bool ok_slope = from->get_weg_hang() == hang_t::flach  ||  ribi_t::doppelt(ribi_typ(from->get_weg_hang()))==ribi_t::doppelt(ribi_typ(zv));
		ok_slope &= to->get_weg_hang() == hang_t::flach  ||  ribi_t::doppelt(ribi_typ(to->get_weg_hang()))==ribi_t::doppelt(ribi_typ(zv));

		// try terraforming
		if (!ok_slope) {
			uint8 dummy,to_slope;
			if (  (bautyp & terraform_flag) != 0  &&  from->ist_natur()  &&  to->ist_natur()  &&  check_terraforming(from,to,&dummy,&to_slope) ) {
				to_flat = to_slope == hang_t::flach;
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
	if (welt->get_scenario()->is_work_allowed_here(sp, (bautyp&tunnel_flag ? WKZ_TUNNELBAU : WKZ_WEGEBAU)|GENERAL_TOOL, bautyp&bautyp_mask, to->get_pos()) != NULL) {
		return false;
	}

	// universal check for elevated things ...
	if(bautyp&elevated_flag) {
		if(  to->hat_weg(air_wt)  ||  welt->lookup_hgt( to_pos ) < welt->get_water_hgt( to_pos )  ||  !check_for_leitung( zv, to )  ||  (!to->ist_karten_boden()  &&  to->get_typ() != grund_t::monorailboden)  ||  to->get_typ() == grund_t::brueckenboden  ||  to->get_typ() == grund_t::tunnelboden  ) {
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
			if(!check_owner(gb->get_besitzer(),sp)  ||  gb->get_tile()->get_hintergrund(0,1,0)!=IMG_LEER) {
				return false;
			}
			// building above houses is expensive ... avoid it!
			*costs += 4;
		}
		// up to now 'to' and 'from' referred to the ground one height step below the elevated way
		// now get the grounds at the right height
		koord3d pos = to->get_pos() + koord3d( 0, 0, env_t::pak_height_conversion_factor );
		grund_t *to2 = welt->lookup(pos);
		if(to2) {
			if(to2->get_weg_nr(0)) {
				// already an elevated ground here => it will have always a way object, that indicates ownership
				ok = to2->get_typ()==grund_t::monorailboden  &&  check_owner(to2->obj_bei(0)->get_besitzer(),sp);
				ok &= to2->get_weg_nr(0)->get_besch()->get_wtyp()==besch->get_wtyp();
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

	if(  env_t::pak_height_conversion_factor == 2  ) {
		// cannot build if conversion factor 2, we aren't powerline and way with maximum speed > 0 or powerline 1 tile below
		grund_t *to2 = welt->lookup( to->get_pos() + koord3d(0, 0, -1) );
		if(  to2 && (((bautyp&bautyp_mask)!=leitung && to2->get_weg_nr(0) && to2->get_weg_nr(0)->get_besch()->get_topspeed()>0) || to2->get_leitung())  ) {
			return false;
		}
		// tile above cannot have way unless we are a way (not powerline) with a maximum speed of 0, or be surface if we are underground
		to2 = welt->lookup( to->get_pos() + koord3d(0, 0, 1) );
		if(  to2  &&  ((to2->get_weg_nr(0) && (besch->get_topspeed()>0 || (bautyp&bautyp_mask)==leitung))  ||  (bautyp & tunnel_flag) != 0)  ) {
			return false;
		}
	}

	// universal check for depots/stops/...
	if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
		return false;
	}

	// universal check for bridges: enter bridges in bridge direction
	if( to->get_typ()==grund_t::brueckenboden ) {
		weg_t *weg=to->get_weg_nr(0);
		if(weg && !ribi_t::ist_gerade(weg->get_ribi_unmasked()|ribi_typ(zv))) {
			return false;
		}
	}
	if( from->get_typ()==grund_t::brueckenboden ) {
		weg_t *weg=from->get_weg_nr(0);
		if(weg && !ribi_t::ist_gerade(weg->get_ribi_unmasked()|ribi_typ(zv))) {
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
		if(!check_crossing(zv,to,wtyp,sp)  ||  !check_crossing(-zv,from,wtyp,sp)) {
			return false;
		}
	}

	// universal check for building under powerlines
	if ((bautyp&bautyp_mask)!=leitung) {
		if (!check_for_leitung(zv,to)  ||  !check_for_leitung(-zv,from)) {
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
			ok = (str  ||  !fundament)  &&  !to->ist_wasser();
			if(!ok) {
				return false;
			}
			// check for end/start of bridge or tunnel
			// fail if no proper way exists, or the way's ribi are not 0 and are not matching the slope type
			ribi_t::ribi test_ribi = (str ? str->get_ribi_unmasked() : 0) | ribi_typ(zv);
			if(to->get_weg_hang()!=to->get_grund_hang()  &&  (str==NULL  ||  !(ribi_t::ist_gerade(test_ribi) || test_ribi==0 ))) {
				return false;
			}
			// calculate costs
			*costs = str ? 0 : s.way_count_straight;
			if((str==NULL  &&  to->hat_wege())  ||  (str  &&  to->has_two_ways())) {
				*costs += 4;	// avoid crossings
			}
			if(to->get_weg_hang()!=0  &&  !to_flat) {
				*costs += s.way_count_slope;
			}
		}
		break;

		case schiene:
		default:
		{
			const weg_t *sch=to->get_weg(besch->get_wtyp());
			// extra check for AI construction (not adding to existing tracks!)
			if((bautyp&bot_flag)!=0  &&  (sch  ||  to->get_halt().is_bound())) {
				return false;
			}
			// ok, regular construction here
			// if no way there: check for right ground type, otherwise check owner
			ok = sch==NULL  ?  (!fundament  &&  !to->ist_wasser())  :  check_owner(sch->get_besitzer(),sp);
			if(!ok) {
				return false;
			}
			// check for end/start of bridge or tunnel
			// fail if no proper way exists, or the way's ribi are not 0 and are not matching the slope type
			ribi_t::ribi test_ribi = (sch ? sch->get_ribi_unmasked() : 0) | ribi_typ(zv);
			if(to->get_weg_hang()!=to->get_grund_hang()  &&  (sch==NULL  ||  !(ribi_t::ist_gerade(test_ribi) || test_ribi==0 ))) {
				return false;
			}
			// calculate costs
			*costs = s.way_count_straight;
			if (!sch) *costs += 1; // only prefer existing rails a little
			if((sch  &&  to->has_two_ways())  ||  (sch==NULL  &&  to->hat_wege())) {
				*costs += 4;	// avoid crossings
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
			ok = sch==NULL  ?  (!fundament  &&  !to->ist_wasser())  :  check_owner(sch->get_besitzer(),sp);
			// tram track allowed in road tunnels, but only along existing roads / tracks
			if(from!=to) {
				if(from->ist_tunnel()) {
					const ribi_t::ribi ribi = from->get_weg_ribi_unmasked(road_wt)  |  from->get_weg_ribi_unmasked(track_wt)  |  ribi_t::doppelt(ribi_typ(from->get_grund_hang()));
					ok = ok && ((ribi & ribi_typ(zv))==ribi_typ(zv));
				}
				if(to->ist_tunnel()) {
					const ribi_t::ribi ribi = to->get_weg_ribi_unmasked(road_wt)  |  to->get_weg_ribi_unmasked(track_wt)  |  ribi_t::doppelt(ribi_typ(to->get_grund_hang()));
					ok = ok && ((ribi & ribi_typ(-zv))==ribi_typ(-zv));
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
			ok = !to->ist_wasser()  &&  (to->get_weg(air_wt)==NULL);
			ok &= !(to->ist_tunnel() && to->hat_wege());
			if(to->get_weg_nr(0)!=NULL) {
				// only 90 deg crossings, only a single way
				ribi_t::ribi w_ribi= to->get_weg_nr(0)->get_ribi_unmasked();
				ok &= ribi_t::ist_gerade(w_ribi)  &&  !ribi_t::ist_einfach(w_ribi)  &&  ribi_t::ist_gerade(ribi_typ(zv))  &&  (w_ribi&ribi_typ(zv))==0;
			}
			if(to->has_two_ways()) {
				// only 90 deg crossings, only for trams ...
				ribi_t::ribi w_ribi= to->get_weg_nr(1)->get_ribi_unmasked();
				ok &= ribi_t::ist_gerade(w_ribi)  &&  !ribi_t::ist_einfach(w_ribi)  &&  ribi_t::ist_gerade(ribi_typ(zv))  &&  (w_ribi&ribi_typ(zv))==0;
			}
			// do not connect to other powerlines
			{
				leitung_t *lt = to->get_leitung();
				ok &= (lt==NULL)  ||  check_owner(sp, lt->get_besitzer());
			}

			if(to->get_typ()!=grund_t::tunnelboden) {
				// only fields are allowed
				if(to->get_typ()!=grund_t::boden) {
					ok &= to->get_typ() == grund_t::fundament && to->find<field_t>();
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
				*costs = to->ist_wasser() ||  canal  ? s.way_count_straight : s.way_count_leaving_road; // prefer water very much
				if(to->get_weg_hang()!=0  &&  !to_flat) {
					*costs += s.way_count_slope * 2;
				}
			}
			break;
		}

		case river:
			if(  to->ist_wasser()  ) {
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

		case luft: // hsiegeln: runway
			{
				const weg_t *w = to->get_weg(air_wt);
				if(  w  &&  w->get_besch()->get_styp()==1  &&  besch->get_styp()!=1 &&  ribi_t::ist_einfach(w->get_ribi_unmasked())  ) {
					// cannot go over the end of a runway with a taxiway
					return false;
				}
				ok = !to->ist_wasser() && (w  ||  !to->hat_wege())  &&  to->find<leitung_t>()==NULL  &&  !fundament;
				// calculate costs
				*costs = s.way_count_straight;
			}
			break;
	}
	return ok;
}


bool wegbauer_t::check_terraforming( const grund_t *from, const grund_t *to, uint8* new_from_slope, uint8* new_to_slope)
{
	// only for normal green tiles
	const hang_t::typ from_slope = from->get_weg_hang();
	const hang_t::typ to_slope = to->get_weg_hang();
	const sint8 from_hgt = from->get_hoehe();
	const sint8 to_hgt = to->get_hoehe();
	// we may change slope of a tile if it is sloped already
	if(  (from_slope == hang_t::flach  ||  from->get_hoehe() == welt->get_water_hgt( from->get_pos().get_2d() ))
	  &&  (to_slope == hang_t::flach  ||  to->get_hoehe() == welt->get_water_hgt( to->get_pos().get_2d() ))  ) {
		return false;
	}
	else if(  abs( from_hgt - to_hgt ) <= (besch->has_double_slopes() ? 2 : 1)  ) {
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
		if(  dir == koord::nord  ) {
			start  += corner1(from_slope) + corner2(from_slope);
			middle += corner3(from_slope) + corner4(from_slope);
			end    += corner3(to_slope) + corner4(to_slope);
		}
		else if(  dir == koord::ost  ) {
			start  += corner1(from_slope) + corner4(from_slope);
			middle += corner2(from_slope) + corner3(from_slope);
			end    += corner2(to_slope) + corner3(to_slope);
		}
		else if(  dir == koord::sued  ) {
			start  += corner3(from_slope) + corner4(from_slope);
			middle += corner1(from_slope) + corner2(from_slope);
			end    += corner1(to_slope) + corner2(to_slope);
		}
		else if(  dir == koord::west  ) {
			start  += corner2(from_slope) + corner3(from_slope);
			middle += corner1(from_slope) + corner4(from_slope);
			end    += corner1(to_slope) + corner4(to_slope);
		}
		// work out intermediate height:
		if(  end == start  ) {
			middle = start;
		}
		else {
			//  end < start   to ist wegbar?assert from ist_wegbar:middle = end + 1
			//  end > start   to ist wegbar?assert from ist_wegbar:middle = end - 1
			if(  !hang_t::ist_wegbar( to_slope )  ) {
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
		if(  dir == koord::nord  ) {
			*new_from_slope = corner1(from_slope) + corner2(from_slope) * 3 + m_from * 9              + m_from * 27;
			*new_to_slope =   m_to                + m_to * 3                + corner3(to_slope) * 9   + corner4(to_slope) * 27;
		}
		else if(  dir == koord::ost  ) {
			*new_from_slope = corner1(from_slope) + m_from * 3              + m_from * 9              + corner4(from_slope) * 27;
			*new_to_slope =   m_to                + corner2(to_slope) * 3   + corner3(to_slope) * 9   + m_to * 27;
		}
		else if(  dir == koord::sued  ) {
			*new_from_slope = m_from              + m_from * 3              + corner3(from_slope) * 9 + corner4(from_slope) * 27;
			*new_to_slope =   corner1(to_slope)   + corner2(to_slope) * 3   + m_to * 9                + m_to * 27;
		}
		else if(  dir == koord::west  ) {
			*new_from_slope = m_from              + corner2(from_slope) * 3 + corner3(from_slope) * 9 + m_from * 27;
			*new_to_slope =   corner1(to_slope)   + m_to * 3                + m_to * 9                + corner4(to_slope) * 27;
		}
		return true;
	}
	return false;
}

void wegbauer_t::do_terraforming()
{
	uint32 last_terraformed = terraform_index.get_count();

	FOR(vector_tpl<uint32>, const i, terraform_index) { // index in route
		grund_t *from = welt->lookup(route[i]);
		uint8 from_slope = from->get_grund_hang();

		grund_t *to = welt->lookup(route[i+1]);
		uint8 to_slope = to->get_grund_hang();
		// calculate new slopes
		check_terraforming(from, to, &from_slope, &to_slope);
		bool changed = false;
		// change slope of from
		if(  from_slope != from->get_grund_hang()  ) {
			if(  from_slope == hang_t::erhoben  ) {
				from->set_hoehe( from->get_hoehe() + 2 );
				from->set_grund_hang( hang_t::flach );
				route[i].z = from->get_hoehe();
			}
			else if(  from_slope != hang_t::erhoben / 2  ) {
				// bit of a hack to recognise single height slopes shifted up 1
				if(  from_slope > hang_t::erhoben / 2  &&  hang_t::ist_einfach( from_slope-hang_t::erhoben / 2 )  ) {
					from->set_hoehe( from->get_hoehe() + 1 );
					from_slope -= hang_t::erhoben / 2;
					route[i].z = from->get_hoehe();
				}
				from->set_grund_hang( from_slope );
			}
			else {
				from->set_hoehe( from->get_hoehe() + 1 );
				from->set_grund_hang( hang_t::flach );
				route[i].z = from->get_hoehe();
			}
			changed = true;
			if (last_terraformed != i) {
				// charge player
				spieler_t::book_construction_costs(sp, welt->get_settings().cst_set_slope, from->get_pos().get_2d(), ignore_wt);
			}
		}
		// change slope of to
		if(  to_slope != to->get_grund_hang()  ) {
			if(  to_slope == hang_t::erhoben  ) {
				to->set_hoehe( to->get_hoehe() + 2 );
				to->set_grund_hang( hang_t::flach );
				route[i + 1].z = to->get_hoehe();
			}
			else if(  to_slope != hang_t::erhoben / 2  ) {
				// bit of a hack to recognise single height slopes shifted up 1
				if(  to_slope > hang_t::erhoben / 2  &&  hang_t::ist_einfach( to_slope-hang_t::erhoben / 2 )  ) {
					to->set_hoehe( to->get_hoehe() + 1 );
					to_slope -= hang_t::erhoben / 2;
					route[i + 1].z = to->get_hoehe();
				}
				to->set_grund_hang(to_slope);
			}
			else {
				to->set_hoehe( to->get_hoehe() + 1);
				to->set_grund_hang(hang_t::flach);
				route[i+1].z = to->get_hoehe();
			}
			changed = true;
			// charge player
			spieler_t::book_construction_costs(sp, welt->get_settings().cst_set_slope, to->get_pos().get_2d(), ignore_wt);
			last_terraformed = i+1; // do not pay twice for terraforming one tile
		}
		// recalc slope image of neighbors
		if (changed) {
			for(uint8 j=0; j<2; j++) {
				for(uint8 x=0; x<2; x++) {
					for(uint8 y=0; y<2; y++) {
						grund_t *gr = welt->lookup_kartenboden(route[i+j].get_2d()+koord(x,y));
						if (gr) {
							gr->calc_bild();
						}
					}
				}
			}
		}
	}
}

void wegbauer_t::check_for_bridge(const grund_t* parent_from, const grund_t* from, const vector_tpl<koord3d> &ziel)
{
	// wrong starting slope or tile already occupied with a way ...
	if (!hang_t::ist_wegbar(from->get_grund_hang())) {
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
				if (  way0->get_waytype() != besch->get_wtyp()  ) {
					if (  way1  ) {
						// two different ways
						return;
					}
					other = way0;
				}
				if (  other  ) {
					if (  (bautyp&bautyp_mask) == strasse  ) {
						if (  other->get_waytype() != track_wt  ||  other->get_besch()->get_styp()!=weg_t::type_tram  ) {
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

			default:
				if (way0->get_waytype()!=besch->get_wtyp()  ||  way1!=NULL) {
					// no other ways allowed
					return;
				}
		}
	}

	const koord zv=from->get_pos().get_2d()-parent_from->get_pos().get_2d();
	const ribi_t::ribi ribi = ribi_typ(zv);

	// now check ribis of existing ways
	const ribi_t::ribi wayribi = way0 ? way0->get_ribi_unmasked() | (way1 ? way1->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::keine) : (ribi_t::ribi)ribi_t::keine;
	if (  wayribi & (~ribi)  ) {
		// curves at bridge start
		return;
	}

	// ok, so now we do a closer investigation
	if(  bruecke_besch  && (  ribi_typ(from->get_grund_hang()) == ribi_t::rueckwaerts(ribi_typ(zv))  ||  from->get_grund_hang() == 0  )
		&&  brueckenbauer_t::ist_ende_ok(sp, from, besch->get_wtyp(),ribi_t::rueckwaerts(ribi_typ(zv)))  ) {
		// Try a bridge.
		const long cost_difference=besch->get_wartung()>0 ? (bruecke_besch->get_wartung()*4l+3l)/besch->get_wartung() : 16;
		const char *error;
		// try eight possible lengths ..
		koord3d end;
		const grund_t* gr_end;
		uint32 min_length = 1;
		for (uint8 i = 0; i < 8 && min_length <= welt->get_settings().way_max_bridge_len; ++i) {
			end = brueckenbauer_t::finde_ende( sp, from->get_pos(), zv, bruecke_besch, error, true, min_length );
			gr_end = welt->lookup(end);
			uint32 length = koord_distance(from->get_pos(), end);
			if(  gr_end  &&  !error  &&  !ziel.is_contained(end)  &&  brueckenbauer_t::ist_ende_ok(sp, gr_end, besch->get_wtyp(), ribi_typ(zv))  &&  length <= welt->get_settings().way_max_bridge_len  ) {
				// If there is a slope on the starting tile, it's taken into account in is_allowed_step, but a bridge will be flat!
				sint8 num_slopes = (from->get_grund_hang() == hang_t::flach) ? 1 : -1;
				// On the end tile, we haven't to subtract way_count_slope, since is_allowed_step isn't called with this tile.
				num_slopes += (gr_end->get_grund_hang() == hang_t::flach) ? 1 : 0;
				next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference + num_slopes*welt->get_settings().way_count_slope, build_straight | build_tunnel_bridge));
				min_length = length+1;
			}
			else {
				break;
			}
		}
		return;
	}

	if(  tunnel_besch  &&  ribi_typ(from->get_grund_hang()) == ribi_typ(zv)  ) {
		// uphill hang ... may be tunnel?
		const long cost_difference=besch->get_wartung()>0 ? (tunnel_besch->get_wartung()*4l+3l)/besch->get_wartung() : 16;
		koord3d end = tunnelbauer_t::finde_ende( sp, from->get_pos(), zv, tunnel_besch);
		if(  end != koord3d::invalid  &&  !ziel.is_contained(end)  ) {
			uint32 length = koord_distance(from->get_pos(), end);
			next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference, build_straight | build_tunnel_bridge ));
			return;
		}
	}
}


wegbauer_t::wegbauer_t(spieler_t* spl) : next_gr(32)
{
	n      = 0;
	sp     = spl;
	bautyp = strasse;   // kann mit route_fuer() gesetzt werden
	maximum = 2000;// CA $ PER TILE

	keep_existing_ways = false;
	keep_existing_city_roads = false;
	keep_existing_faster_ways = false;
	build_sidewalk = false;
}


/**
 * If a way is built on top of another way, should the type
 * of the former way be kept or replaced (true == keep)
 * @author Hj. Malthaner
 */
void wegbauer_t::set_keep_existing_ways(bool yesno)
{
	keep_existing_ways = yesno;
	keep_existing_faster_ways = false;
}


void wegbauer_t::set_keep_existing_faster_ways(bool yesno)
{
	keep_existing_ways = false;
	keep_existing_faster_ways = yesno;
}


void wegbauer_t::route_fuer(bautyp_t wt, const weg_besch_t *b, const tunnel_besch_t *tunnel, const bruecke_besch_t *br)
{
	bautyp = wt;
	besch = b;
	bruecke_besch = br;
	tunnel_besch = tunnel;
	if(wt&tunnel_flag  &&  tunnel==NULL) {
		dbg->fatal("wegbauer_t::route_fuer()","needs a tunnel description for an underground route!");
	}
	if((wt&bautyp_mask)==luft) {
		wt &= bautyp_mask | bot_flag;
	}
	if(sp==NULL) {
		bruecke_besch = NULL;
		tunnel_besch = NULL;
	}
	else if(  bautyp != river  ) {
#ifdef AUTOMATIC_BRIDGES
		if(  bruecke_besch == NULL  ) {
			bruecke_besch = brueckenbauer_t::find_bridge(b->get_wtyp(), 25, welt->get_timeline_year_month());
		}
#endif
#ifdef AUTOMATIC_TUNNELS
		if(  tunnel_besch == NULL  ) {
			tunnel_besch = tunnelbauer_t::find_tunnel(b->get_wtyp(), 25, welt->get_timeline_year_month());
		}
#endif
	}
  DBG_MESSAGE("wegbauer_t::route_fuer()",
         "setting way type to %d, besch=%s, bruecke_besch=%s, tunnel_besch=%s",
         bautyp,
         besch ? besch->get_name() : "NULL",
         bruecke_besch ? bruecke_besch->get_name() : "NULL",
         tunnel_besch ? tunnel_besch->get_name() : "NULL"
         );
}


void get_mini_maxi( const vector_tpl<koord3d> &ziel, koord3d &mini, koord3d &maxi )
{
	mini = maxi = ziel[0];
	FOR(vector_tpl<koord3d>, const& current, ziel) {
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
long wegbauer_t::intern_calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel)
{
	// we clear it here probably twice: does not hurt ...
	route.clear();
	terraform_index.clear();

	// check for existing koordinates
	bool has_target_ground = false;
	FOR(vector_tpl<koord3d>, const& i, ziel) {
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
	koord3d gr_pos;	// just the last valid pos ...
	route_t::ANode *tmp=NULL;
	uint32 step = 0;
	const grund_t* gr=NULL;

	FOR(vector_tpl<koord3d>, const& i, start) {
		gr = welt->lookup(i);

		// is valid ground?
		long dummy;
		if( !gr || !is_allowed_step(gr,gr,&dummy) ) {
			// DBG_MESSAGE("wegbauer_t::intern_calc_route()","cannot start on (%i,%i,%i)",start.x,start.y,start.z);
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
		const ribi_t::ribi straight_dir = tmp->parent!=NULL ? ribi_typ(gr->get_pos().get_2d()-tmp->parent->gr->get_pos().get_2d()) : (ribi_t::ribi)ribi_t::alle;

		// test directions
		// .. use only those that are allowed by current slope
		// .. do not go backward
		const ribi_t::ribi slope_dir = (hang_t::ist_wegbar_ns(gr->get_weg_hang()) ? ribi_t::nordsued : ribi_t::keine) | (hang_t::ist_wegbar_ow(gr->get_weg_hang()) ? ribi_t::ostwest : ribi_t::keine);
		const ribi_t::ribi test_dir = (tmp->count & build_straight)==0  ?  slope_dir  & ~ribi_t::rueckwaerts(straight_dir)
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
					if (to==NULL  ||  (check_slope(gr, to)  &&  gr->get_vmove(r)!=to->get_vmove(ribi_t::rueckwaerts(r)))) {
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

			long new_cost = 0;
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
		FOR(vector_tpl<next_gr_t>, const& r, next_gr) {
			to = r.gr;

			// new values for cost g
			uint32 new_g = tmp->g + r.cost;

			settings_t const& s = welt->get_settings();
			// check for curves (usually, one would need the lastlast and the last;
			// if not there, then we could just take the last
			uint8 current_dir;
			if(tmp->parent!=NULL) {
				current_dir = ribi_typ( tmp->parent->gr->get_pos().get_2d(), to->get_pos().get_2d() );
				if(tmp->dir!=current_dir) {
					new_g += s.way_count_curve;
					if(tmp->parent->dir!=tmp->dir) {
						// discourage double turns
						new_g += s.way_count_double_curve;
					}
					else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
						// discourage v turns heavily
						new_g += s.way_count_90_curve;
					}
				}
				else if(bautyp==leitung  &&  ribi_t::ist_kurve(current_dir)) {
					new_g += s.way_count_double_curve;
				}
				// extra malus leave an existing road after only one tile
				waytype_t const wt = besch->get_wtyp();
				if (tmp->parent->gr->hat_weg(wt) && !gr->hat_weg(wt) && to->hat_weg(wt)) {
					// but only if not straight track
					if(!ribi_t::ist_gerade(tmp->dir)) {
						new_g += s.way_count_leaving_road;
					}
				}
			}
			else {
				 current_dir = ribi_typ( gr->get_pos().get_2d(), to->get_pos().get_2d() );
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
				if((step&1023)==0) {reliefkarte_t::get_karte()->calc_map();}
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
DBG_DEBUG("wegbauer_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u",step,route_t::MAX_STEP,queue.get_count(),tmp->g);
#endif
	INT_CHECK("wegbauer 194");

	route_t::RELEASE_NODE();

	// target reached?
	if( !ziel.is_contained(gr->get_pos())  ||  step>=route_t::MAX_STEP  ||  tmp->parent==NULL) {
		if (step>=route_t::MAX_STEP) {
			dbg->warning("wegbauer_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,route_t::MAX_STEP);
		}
		return -1;
	}
	else {
		const long cost = tmp->g;
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

	return -1;
}


void wegbauer_t::intern_calc_straight_route(const koord3d start, const koord3d ziel)
{
	bool ok=true;

	const grund_t* test_bd = welt->lookup(start);
	if (test_bd == NULL) {
		// not building
		return;
	}
	long dummy_cost;
	if(!is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// no legal ground to start ...
		return;
	}
	if ((bautyp&tunnel_flag) && !test_bd->ist_tunnel()) {
		// start tunnelbuilding in tunnels
		return;
	}
	test_bd = welt->lookup(ziel);
	if((bautyp&tunnel_flag)==0  &&  test_bd  &&  !is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// ... or to end
		return;
	}
	// we have to reach target height if no tunnel building or (target ground does not exists or is underground).
	// in full underground mode if there is no tunnel under cursor, kartenboden gets selected
	const bool target_3d = (bautyp&tunnel_flag)==0  ||  test_bd==NULL  ||  !test_bd->ist_karten_boden();

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
			diff = (pos.x>ziel.x) ? ribi_t::west : ribi_t::ost;
		}
		else {
			diff = (pos.y>ziel.y) ? ribi_t::nord : ribi_t::sued;
		}
		if(bautyp&tunnel_flag) {
			// create fake tunnel grounds if needed
			bool bd_von_new = false, bd_nach_new = false;
			grund_t *bd_von = welt->lookup(pos);
			if(  bd_von == NULL ) {
				bd_von = new tunnelboden_t(pos, hang_t::flach);
				bd_von_new = true;
			}
			// take care of slopes
			pos.z = bd_von->get_vmove(diff);

			// check next tile
			grund_t *bd_nach = welt->lookup(pos + diff);
			if(  !bd_nach  ) {
				// check tiles one level above and below
				bd_nach = welt->lookup( pos + koord3d(0, 0, -1) );
				if(  env_t::pak_height_conversion_factor == 2  ) {
					if (bd_nach == NULL) {
						bd_nach = welt->lookup( pos + koord3d(0, 0, 1) );
					}
					// tile one level above/below should end on same level
					if (bd_nach  &&  bd_nach->get_vmove(ribi_t::rueckwaerts(diff)) != pos.z) {
						ok = false;
					}
				}
				// check two levels below
				if (bd_nach == NULL) {
					bd_nach = welt->lookup( pos + koord3d(0, 0, -2) );
					if(  bd_nach  &&  env_t::pak_height_conversion_factor == 2  ) {
						// should not end at -1
						if (bd_nach->get_vmove(ribi_t::rueckwaerts(diff)) == pos.z-1) {
							ok = false;
						}
					}
				}
				if(  bd_nach  &&  bd_nach->get_weg_hang() == hang_t::flach  ) {
					// Don't care about _flat_ tunnels below.
					bd_nach = NULL;
				}
			}
			if(  bd_nach == NULL  ){
				bd_nach = new tunnelboden_t(pos + diff, hang_t::flach);
				bd_nach_new = true;
			}
			// check for tunnel and right slope
			ok = ok && bd_nach->ist_tunnel() && bd_nach->get_vmove(ribi_t::rueckwaerts(diff))==pos.z;
			// all other checks are done here (crossings, stations etc)
			ok = ok && is_allowed_step(bd_von, bd_nach, &dummy_cost);

			// advance position
			pos = bd_nach->get_pos();

			// check new tile: ground must be above tunnel and below sea
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			ok = ok  &&  (gr->get_hoehe() > pos.z)  &&  (!gr->ist_wasser()  ||  (welt->lookup_hgt(pos.get_2d()) > pos.z) );

			if (bd_von_new) {
				delete bd_von;
			}
			if (bd_nach_new) {
				delete bd_nach;
			}
		}
		else {
			grund_t *bd_von = welt->lookup(pos);
			if (bd_von==NULL) {
				ok = false;
			}
			else
			{
				grund_t *bd_nach = NULL;
				if (!bd_von->get_neighbour(bd_nach, invalid_wt, diff) ||  !check_slope(bd_von, bd_nach)) {
					// slopes do not match
					// terraforming enabled?
					if (bautyp==river  ||  (bautyp & terraform_flag) == 0) {
						break;
					}
					// check terraforming (but not in curves)
					ok = false;
					if (check_terraform) {
						bd_nach = welt->lookup_kartenboden(bd_von->get_pos().get_2d() + diff);
						if (bd_nach==NULL  ||  (check_slope(bd_von, bd_nach)  &&  bd_von->get_vmove(diff)!=bd_nach->get_vmove(ribi_t::rueckwaerts(diff)))) {
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
			check_terraform = pos.x==ziel.x  ||  pos.y==ziel.y;
		}

		route.append(pos);
		if (do_terraform) {
			terraform_index.append(route.get_count()-2);
		}
		DBG_MESSAGE("wegbauer_t::calc_straight_route()","step %s = %i",koord(diff).get_str(),ok);
	}
	ok = ok && ( target_3d ? pos==ziel : pos.get_2d()==ziel.get_2d() );

	// we can built a straight route?
	if(ok) {
DBG_MESSAGE("wegbauer_t::intern_calc_straight_route()","found straight route max_n=%i",get_count()-1);
	}
	else {
		route.clear();
		terraform_index.clear();
	}
}


// special for starting/landing runways
bool wegbauer_t::intern_calc_route_runways(koord3d start3d, const koord3d ziel3d)
{
	route.clear();
	terraform_index.clear();

	const koord start=start3d.get_2d();
	const koord ziel=ziel3d.get_2d();
	// check for straight line!
	const ribi_t::ribi ribi = ribi_typ( start, ziel );
	if(  !ribi_t::ist_gerade(ribi)  ) {
		// only straight runways!
		return false;
	}
	const ribi_t::ribi ribi_gerade = ribi_t::doppelt(ribi);

	// not too close to the border?
	if(	 !(welt->is_within_limits(start-koord(5,5))  &&  welt->is_within_limits(start+koord(5,5)))  ||
		 !(welt->is_within_limits(ziel-koord(5,5))  &&  welt->is_within_limits(ziel+koord(5,5)))  ) {
		if(sp==welt->get_active_player()) {
			create_win( new news_img("Zu nah am Kartenrand"), w_time_delete, magic_none);
			return false;
		}
	}

	// now try begin and endpoint
	const koord zv(ribi);
	// end start
	const grund_t *gr = welt->lookup_kartenboden(start);
	const weg_t *weg = gr->get_weg(air_wt);
	if(weg  &&  !ribi_t::ist_gerade(weg->get_ribi()|ribi_gerade)  ) {
		// cannot connect with curve at the end
		return false;
	}
	if(  weg  &&  weg->get_besch()->get_styp()==0  ) {
		//  could not continue taxiway with runway
		return false;
	}
	// check end
	gr = welt->lookup_kartenboden(ziel);
	weg = gr->get_weg(air_wt);
	if(weg  &&  !ribi_t::ist_gerade(weg->get_ribi()|ribi_gerade)  ) {
		// cannot connect with curve at the end
		return false;
	}
	if(  weg  &&  weg->get_besch()->get_styp()==0  ) {
		//  could not continue taxiway with runway
		return false;
	}
	// now try a straight line with no crossings and no curves at the end
	const int dist=koord_distance( ziel, start );
	grund_t *from = welt->lookup_kartenboden(start);
	for(  int i=0;  i<=dist;  i++  ) {
		grund_t *to = welt->lookup_kartenboden(start+zv*i);
		long dummy;
		if (!is_allowed_step(from, to, &dummy)) {
			return false;
		}
		weg = to->get_weg(air_wt);
		if(  weg  &&  weg->get_besch()->get_styp()==1  &&  (ribi_t::is_threeway(weg->get_ribi_unmasked()|ribi_gerade))  &&  (weg->get_ribi_unmasked()|ribi_gerade)!=ribi_t::alle  ) {
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
void wegbauer_t::calc_straight_route(koord3d start, const koord3d ziel)
{
	DBG_MESSAGE("wegbauer_t::calc_straight_route()","from %d,%d,%d to %d,%d,%d",start.x,start.y,start.z, ziel.x,ziel.y,ziel.z );
	if(bautyp==luft  &&  besch->get_styp()==1) {
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


void wegbauer_t::calc_route(const koord3d &start, const koord3d &ziel)
{
	vector_tpl<koord3d> start_vec(1), ziel_vec(1);
	start_vec.append(start);
	ziel_vec.append(ziel);
	calc_route(start_vec, ziel_vec);
}

/* calc_route
 *
 */
void wegbauer_t::calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel)
{
#ifdef DEBUG_ROUTES
long ms=dr_time();
#endif
	INT_CHECK("simbau 740");

	if(bautyp==luft  &&  besch->get_styp()==1) {
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
		while (route.get_count()>1  &&  welt->lookup(route[0])->get_grund_hang() == hang_t::flach  &&  welt->lookup(route[1])->ist_wasser()) {
			// remove leading water ...
			route.remove_at(0);
		}
	}
	else {
		keep_existing_city_roads |= (bautyp&bot_flag)!=0;
		long cost2 = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		if(cost2<0) {
			// not successful: try backwards
			intern_calc_route(ziel,start);
			return;
		}

#ifdef REVERSE_CALC_ROUTE_TOO
		vector_tpl<koord3d> route2(0);
		vector_tpl<uint32> terraform_index2(0);
		swap(route, route2);
		swap(terraform_index, terraform_index2);
		long cost = intern_calc_route(ziel, start);
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
DBG_MESSAGE("calc_route::calc_route", "took %i ms",dr_time()-ms);
#endif
}

void
wegbauer_t::baue_tunnel_und_bruecken()
{
	if(bruecke_besch==NULL  &&  tunnel_besch==NULL) {
		return;
	}
	// check for bridges and tunnels (no tunnel/bridge at last/first tile)
	for(uint32 i=1; i<get_count()-2; i++) {
		koord d = (route[i + 1] - route[i]).get_2d();
		koord zv = koord (sgn(d.x), sgn(d.y));

		// ok, here is a gap ...
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			if(d.x*d.y!=0) {
				dbg->error("wegbauer_t::baue_tunnel_und_bruecken()","cannot built a bridge between %s (n=%i, max_n=%i) and %s", route[i].get_str(),i,get_count()-1,route[i+1].get_str());
				continue;
			}

			DBG_MESSAGE("wegbauer_t::baue_tunnel_und_bruecken", "built bridge %p between %s and %s", bruecke_besch, route[i].get_str(), route[i + 1].get_str());

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

			if(start->get_grund_hang()==0  ||  start->get_grund_hang()==hang_typ(zv*(-1))) {
				// bridge here, since the route is saved backwards, we have to build it at the posterior end
				brueckenbauer_t::baue( sp, route[i+1].get_2d(), bruecke_besch);
			}
			else {
				// tunnel
				tunnelbauer_t::baue( sp, route[i].get_2d(), tunnel_besch, true );
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
			hang_t::typ h = gr_i->get_weg_hang();
			waytype_t const wt = besch->get_wtyp();
			if(h!=hang_t::flach  &&  hang_t::gegenueber(h)==gr_i1->get_weg_hang()) {
				// either a short mountain or a short dip ...
				// now: check ownership
				weg_t *wi = gr_i->get_weg(wt);
				weg_t *wi1 = gr_i1->get_weg(wt);
				if(wi->get_besitzer()==sp  &&  wi1->get_besitzer()==sp) {
					// we are the owner
					if(  h != hang_typ(zv)  ) {
						// its a bridge
						if( bruecke_besch ) {
							wi->set_ribi(ribi_typ(h));
							wi1->set_ribi(ribi_typ(hang_t::gegenueber(h)));
							brueckenbauer_t::baue( sp, route[i].get_2d(), bruecke_besch);
						}
					}
					else if( tunnel_besch ) {
						// make a short tunnel
						wi->set_ribi(ribi_typ(hang_t::gegenueber(h)));
						wi1->set_ribi(ribi_typ(h));
						tunnelbauer_t::baue( sp, route[i].get_2d(), tunnel_besch, true );
					}
					INT_CHECK( "wegbauer 1584" );
				}
			}
		}
	}
}


/*
 * returns the amount needed to built this way
 * author prissi
 */
sint64 wegbauer_t::calc_costs()
{
	sint64 costs=0;
	koord3d offset = koord3d( 0, 0, bautyp & elevated_flag ? env_t::pak_height_conversion_factor : 0 );

	sint32 single_cost;
	sint32 new_speedlimit;

	if( bautyp&tunnel_flag ) {
		assert( tunnel_besch );
		single_cost = tunnel_besch->get_preis();
		new_speedlimit = tunnel_besch->get_topspeed();
	}
	else {
		single_cost = besch->get_preis();
		new_speedlimit = besch->get_topspeed();
	}

	// calculate costs for terraforming
	uint32 last_terraformed = terraform_index.get_count();

	FOR(vector_tpl<uint32>, const i, terraform_index) { // index in route
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
				if( tunnel->get_besch() == tunnel_besch ) {
					continue; // Nothing to pay on this tile.
				}
				old_speedlimit = tunnel->get_besch()->get_topspeed();
			}
			else {
				if(  besch->get_wtyp() == powerline_wt  ) {
					if( leitung_t *lt=gr->get_leitung() ) {
						old_speedlimit = lt->get_besch()->get_topspeed();
					}
				}
				else {
					if (weg_t const* const weg = gr->get_weg(besch->get_wtyp())) {
						replace_cost = weg->get_besch()->get_preis();
						if( weg->get_besch() == besch ) {
							continue; // Nothing to pay on this tile.
						}
						if(  besch->get_styp() == 0  &&  weg->get_besch()->get_styp() == 7  &&  gr->get_weg_nr(0)->get_waytype() == road_wt  ) {
							// Don't replace a tram on a road with a normal track.
							continue;
						}
						old_speedlimit = weg->get_besch()->get_topspeed();
					}
					else if (besch->get_wtyp()==water_wt  &&  gr->ist_wasser()) {
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
						costs += ((groundobj_t *)obj)->get_besch()->get_preis();
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
				if(start->get_grund_hang()==0  ||  start->get_grund_hang()==hang_typ(zv*(-1))) {
					// bridge
					costs += bruecke_besch->get_preis()*(sint64)(koord_distance(route[i], route[i+1])+1);
					continue;
				}
				else {
					// tunnel
					costs += tunnel_besch->get_preis()*(sint64)(koord_distance(route[i], route[i+1])+1);
					continue;
				}
			}
		}
	}
	DBG_MESSAGE("wegbauer_t::calc_costs()","construction estimate: %f",costs/100.0);
	return costs;
}


// adds the ground before underground construction
bool wegbauer_t::baue_tunnelboden()
{
	long cost = 0;
	for(uint32 i=0; i<get_count(); i++) {

		grund_t* gr = welt->lookup(route[i]);

		const weg_besch_t *wb = tunnel_besch->get_weg_besch();
		if(wb==NULL) {
			// now we search a matching way for the tunnels top speed
			// ignore timeline to get consistent results
			wb = wegbauer_t::weg_search( tunnel_besch->get_waytype(), tunnel_besch->get_topspeed(), 0, weg_t::type_flat );
		}

		if(gr==NULL) {
			// make new tunnelboden
			tunnelboden_t* tunnel = new tunnelboden_t(route[i], 0);
			welt->access(route[i].get_2d())->boden_hinzufuegen(tunnel);
			if(tunnel_besch->get_waytype()!=powerline_wt) {
				weg_t *weg = weg_t::alloc(tunnel_besch->get_waytype());
				weg->set_besch( wb );
				tunnel->neuen_weg_bauen(weg, route.get_ribi(i), sp);
				tunnel->obj_add(new tunnel_t(route[i], sp, tunnel_besch));
				weg->set_max_speed(tunnel_besch->get_topspeed());
				spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
			} else {
				tunnel->obj_add(new tunnel_t(route[i], sp, tunnel_besch));
				leitung_t *lt = new leitung_t(tunnel->get_pos(), sp);
				lt->set_besch( wb );
				tunnel->obj_add( lt );
				lt->laden_abschliessen();
				spieler_t::add_maintenance( sp, -lt->get_besch()->get_wartung(), powerline_wt);
			}
			tunnel->calc_bild();
			cost -= tunnel_besch->get_preis();
			spieler_t::add_maintenance( sp,  tunnel_besch->get_wartung(), tunnel_besch->get_finance_waytype() );
		}
		else if(gr->get_typ()==grund_t::tunnelboden) {
			// check for extension only ...
			if(tunnel_besch->get_waytype()!=powerline_wt) {
				gr->weg_erweitern( tunnel_besch->get_waytype(), route.get_ribi(i) );

				tunnel_t *tunnel = gr->find<tunnel_t>();
				assert( tunnel );
				// take the faster way
				if(  !keep_existing_faster_ways  ||  (tunnel->get_besch()->get_topspeed() < tunnel_besch->get_topspeed())  ) {
					spieler_t::add_maintenance(sp, -tunnel->get_besch()->get_wartung(), tunnel->get_besch()->get_finance_waytype());
					spieler_t::add_maintenance(sp,  tunnel_besch->get_wartung(), tunnel->get_besch()->get_finance_waytype() );

					tunnel->set_besch(tunnel_besch);
					weg_t *weg = gr->get_weg(tunnel_besch->get_waytype());
					weg->set_besch(wb);
					weg->set_max_speed(tunnel_besch->get_topspeed());
					// respect max speed of catenary
					wayobj_t const* const wo = gr->get_wayobj(tunnel_besch->get_waytype());
					if (wo  &&  wo->get_besch()->get_topspeed() < weg->get_max_speed()) {
						weg->set_max_speed( wo->get_besch()->get_topspeed() );
					}
					gr->calc_bild();

					cost -= tunnel_besch->get_preis();
				}
			} else {
				leitung_t *lt = gr->get_leitung();
				if(!lt) {
					lt = new leitung_t(gr->get_pos(), sp);
					lt->set_besch( wb );
					gr->obj_add( lt );
				} else {
					lt->leitung_t::laden_abschliessen();	// only change powerline aspect
					spieler_t::add_maintenance( sp, -lt->get_besch()->get_wartung(), powerline_wt);
				}
			}
		}
	}
	spieler_t::book_construction_costs(sp, cost, route[0].get_2d(), tunnel_besch->get_waytype());
	return true;
}



void wegbauer_t::baue_elevated()
{
	FOR(koord3d_vector_t, & i, route) {
		planquadrat_t* const plan = welt->access(i.get_2d());

		grund_t* const gr0 = plan->get_boden_in_hoehe(i.z);
		i.z += env_t::pak_height_conversion_factor;
		grund_t* const gr  = plan->get_boden_in_hoehe(i.z);

		if(gr==NULL) {
			hang_t::typ hang = gr0 ? gr0->get_grund_hang() : 0;
			// add new elevated ground
			monorailboden_t* const monorail = new monorailboden_t(i, hang);
			plan->boden_hinzufuegen(monorail);
			monorail->calc_bild();
		}
	}
}



void wegbauer_t::baue_strasse()
{
	// only public player or cities (sp==NULL) can build cityroads with sidewalk
	bool add_sidewalk = build_sidewalk  &&  (sp==NULL  ||  sp->get_player_nr()==1);

	if(add_sidewalk) {
		sp = NULL;
	}

	// init undo
	if(sp!=NULL) {
		// intercity roads have no owner, so we must check for an owner
		sp->init_undo(road_wt,get_count());
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

			// keep faster ways or if it is the same way ... (@author prissi)
			if(weg->get_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_city_roads  &&  weg->hat_gehweg())  ||  (keep_existing_faster_ways  &&  weg->get_besch()->get_topspeed()>besch->get_topspeed())  ||  (sp!=NULL  &&  weg->ist_entfernbar(sp)!=NULL) || (gr->get_typ()==grund_t::monorailboden && (bautyp&elevated_flag)==0)) {
				//nothing to be done
//DBG_MESSAGE("wegbauer_t::baue_strasse()","nothing to do at (%i,%i)",k.x,k.y);
			}
			else {
				// we take ownership => we take care to maintain the roads completely ...
				spieler_t *s = weg->get_besitzer();
				spieler_t::add_maintenance(s, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
				// cost is the more expensive one, so downgrading is between removing and new building
				cost -= max( weg->get_besch()->get_preis(), besch->get_preis() );
				weg->set_besch(besch);
				// respect max speed of catenary
				wayobj_t const* const wo = gr->get_wayobj(besch->get_wtyp());
				if (wo  &&  wo->get_besch()->get_topspeed() < weg->get_max_speed()) {
					weg->set_max_speed( wo->get_besch()->get_topspeed() );
				}
				weg->set_gehweg(add_sidewalk);
				spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
				weg->set_besitzer(sp);
			}
		}
		else {
			// make new way
			strasse_t * str = new strasse_t();

			str->set_besch(besch);
			str->set_gehweg(add_sidewalk);
			cost = -gr->neuen_weg_bauen(str, route.get_short_ribi(i), sp)-besch->get_preis();

			// prissi: into UNDO-list, so we can remove it later
			if(sp!=NULL) {
				// intercity roads have no owner, so we must check for an owner
				sp->add_undo( route[i] );
			}
		}
		gr->calc_bild();	// because it may be a crossing ...
		reliefkarte_t::get_karte()->calc_map_pixel(k);
		spieler_t::book_construction_costs(sp, cost, k, road_wt);
	} // for
}


void wegbauer_t::baue_schiene()
{
	if(get_count() > 1) {
		// init undo
		sp->init_undo(besch->get_wtyp(), get_count());

		// built tracks
		for(  uint32 i=0;  i<get_count();  i++  ) {
			sint64 cost = 0;
			grund_t* gr = welt->lookup(route[i]);
			ribi_t::ribi ribi = route.get_short_ribi(i);

			if(gr->get_typ()==grund_t::wasser) {
				// not building on the sea ...
				continue;
			}

			bool const extend = gr->weg_erweitern(besch->get_wtyp(), ribi);

			// bridges/tunnels have their own track type and must not upgrade
			if((gr->get_typ()==grund_t::brueckenboden ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->get_weg_nr(0)->get_waytype()==besch->get_wtyp()) {
				continue;
			}

			if(extend) {
				weg_t* const weg = gr->get_weg(besch->get_wtyp());
				bool change_besch = true;

				// do not touch fences, tram way etc. if there is already same way with different type
				// keep faster ways or if it is the same way ... (@author prissi)
				if (weg->get_besch() == besch                                                               ||
						(besch->get_styp() == 0 && weg->get_besch()->get_styp() == 7 && gr->has_two_ways())     ||
						keep_existing_ways                                                                      ||
						(keep_existing_faster_ways && weg->get_besch()->get_topspeed() > besch->get_topspeed()) ||
						(gr->get_typ() == grund_t::monorailboden && !(bautyp & elevated_flag)  &&  gr->get_weg_nr(0)->get_waytype()==besch->get_wtyp())) {
					//nothing to be done
					change_besch = false;
				}

				// build tram track over crossing -> remove crossing
				if(  gr->has_two_ways()  &&  besch->get_styp()==7  &&  weg->get_besch()->get_styp() != 7  ) {
					if(  crossing_t *cr = gr->find<crossing_t>(2)  ) {
						// change to tram track
						cr->mark_image_dirty( cr->get_bild(), 0);
						cr->entferne(sp);
						delete cr;
						change_besch = true;
						// tell way we have no crossing any more, restore speed limit
						gr->get_weg_nr(0)->clear_crossing();
						gr->get_weg_nr(0)->set_besch( gr->get_weg_nr(0)->get_besch() );
						gr->get_weg_nr(1)->clear_crossing();
					}
				}

				if(  change_besch  ) {
					// we take ownership => we take care to maintain the roads completely ...
					spieler_t *s = weg->get_besitzer();
					spieler_t::add_maintenance( s, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
					// cost is the more expensive one, so downgrading is between removing and new buidling
					cost -= max( weg->get_besch()->get_preis(), besch->get_preis() );
					weg->set_besch(besch);
					// respect max speed of catenary
					wayobj_t const* const wo = gr->get_wayobj(besch->get_wtyp());
					if (wo  &&  wo->get_besch()->get_topspeed() < weg->get_max_speed()) {
						weg->set_max_speed( wo->get_besch()->get_topspeed() );
					}
					spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
					weg->set_besitzer(sp);
				}
			}
			else {
				weg_t* const sch = weg_t::alloc(besch->get_wtyp());
				sch->set_besch(besch);

				cost = -gr->neuen_weg_bauen(sch, ribi, sp)-besch->get_preis();

				// connect canals to sea
				if(  besch->get_wtyp() == water_wt  ) {
					for(  int j = 0;  j < 4;  j++  ) {
						grund_t *sea = welt->lookup_kartenboden( route[i].get_2d() + koord::nsow[j] );
						if(  sea  &&  sea->ist_wasser()  ) {
							gr->weg_erweitern( water_wt, ribi_t::nsow[j] );
							sea->calc_bild();
						}
					}
				}

				// prissi: into UNDO-list, so we can remove it later
				sp->add_undo( route[i] );
			}

			gr->calc_bild();
			reliefkarte_t::get_karte()->calc_map_pixel( gr->get_pos().get_2d() );
			spieler_t::book_construction_costs(sp, cost, gr->get_pos().get_2d(), besch->get_finance_waytype());

			if((i&3)==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



void wegbauer_t::baue_leitung()
{
	if(  get_count() < 1  ) {
		return;
	}

	// no undo
	sp->init_undo(powerline_wt,get_count());

	for(  uint32 i=0;  i<get_count();  i++  ) {
		grund_t* gr = welt->lookup(route[i]);

		leitung_t* lt = gr->get_leitung();
		bool build_powerline = false;
		// ok, really no lt here ...
		if(lt==NULL) {
			if(gr->ist_natur()) {
				// remove trees etc.
				sint64 cost = gr->remove_trees();
				spieler_t::book_construction_costs(sp, -cost, gr->get_pos().get_2d(), powerline_wt);
			}
			lt = new leitung_t(route[i], sp );
			gr->obj_add(lt);

			// prissi: into UNDO-list, so we can remove it later
			sp->add_undo( route[i] );
			build_powerline = true;
		}
		else {
			// modernize the network
			if( !keep_existing_faster_ways  ||  lt->get_besch()->get_topspeed() < besch->get_topspeed()  ) {
				build_powerline = true;
				spieler_t::add_maintenance( lt->get_besitzer(),  -lt->get_besch()->get_wartung(), powerline_wt );
			}
		}
		if (build_powerline) {
			lt->set_besch(besch);
			spieler_t::book_construction_costs(sp, -besch->get_preis(), gr->get_pos().get_2d(), powerline_wt);
			// this adds maintenance
			lt->leitung_t::laden_abschliessen();
			reliefkarte_t::get_karte()->calc_map_pixel( gr->get_pos().get_2d() );
		}

		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}
	}
}



// this can drive any river, even a river that has max_speed=0
class fluss_fahrer_t : public fahrer_t
{
	bool ist_befahrbar(const grund_t* gr) const { return gr->get_weg_ribi_unmasked(water_wt)!=0; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_weg_ribi_unmasked(water_wt); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_kosten(const grund_t *, const sint32, koord) const { return 1; }
	virtual bool ist_ziel(const grund_t *cur,const grund_t *) const { return cur->ist_wasser()  &&  cur->get_grund_hang()==hang_t::flach; }
};


// make a river
void wegbauer_t::baue_fluss()
{
	/* since the constraints of the wayfinder ensures that a river flows always downwards
	 * we can assume that the first tiles are the ocean.
	 * Usually the wayfinder would find either direction!
	 * route.front() tile at the ocean, route.back() the spring of the river
	 */

	// Do we join an other river?
	uint32 start_n = 0;
	for(  uint32 idx=start_n;  idx<get_count();  idx++  ) {
		if(  welt->lookup(route[idx])->hat_weg(water_wt)  ||  welt->lookup(route[idx])->get_hoehe() == welt->get_water_hgt(route[idx].get_2d())  ) {
			start_n = idx;
		}
	}
	if(  start_n == get_count()-1  ) {
		// completely joined another river => nothing to do
		return;
	}

	// first check then lower riverbed
	const sint8 start_h = route[start_n].z;
	uint32 end_n = get_count();
	uint32 i = start_n;
	while(i<get_count()) {
		// first find all tiles that are on the same level as tile i
		// and check whether we can lower all of them
		// if lowering fails we do not continue river building
		bool ok = true;
		uint32 j;
		for(j=i; j<get_count() &&  ok; j++) {
			// one step higher?
			if (route[j].z > route[i].z) break;
			// check
			ok = welt->can_ebne_planquadrat(NULL, route[j].get_2d(), max(route[j].z-1, start_h));
		}
		// now lower all tiles that have the same height as tile i
		for(uint32 k=i; k<j; k++) {
			welt->ebne_planquadrat(NULL, route[k].get_2d(), max(route[k].z-1, start_h));
		}
		if (!ok) {
			end_n = j;
			break;
		}
		i = j;
	}
	// nothing to built ?
	if (start_n >= end_n-1) {
		return;
	}

	// now build the river
	grund_t *gr_first = NULL;
	for(  uint32 i=start_n;  i<end_n;  i++  ) {
		grund_t* gr = welt->lookup_kartenboden(route[i].get_2d());
		if(  gr_first == NULL) {
			gr_first = gr;
		}
		if(  gr->get_typ()!=grund_t::wasser  ) {
			// get direction
			ribi_t::ribi ribi = i<end_n-1 ? route.get_short_ribi(i) : ribi_typ(route[i-1].get_2d()-route[i].get_2d());
			bool extend = gr->weg_erweitern(water_wt, ribi);
			if(  !extend  ) {
				weg_t *sch=weg_t::alloc(water_wt);
				sch->set_besch(besch);
				gr->neuen_weg_bauen(sch, ribi, NULL);
			}
		}
	}
	gr_first->calc_bild(); // to calculate ribi of water tiles

	// we will make rivers gradually larger by stepping up their width
	if(  env_t::river_types>1  &&  start_n<get_count()) {
		/* since we will stop at the first crossing with an existent river,
		 * we cannot make sure, we have the same destination;
		 * thus we use the routefinder to find the sea
		 */
		route_t to_the_sea;
		fluss_fahrer_t ff;
		if (to_the_sea.find_route(welt, welt->lookup_kartenboden(route[start_n].get_2d())->get_pos(), &ff, 0, ribi_t::alle, 0x7FFFFFFF)) {
			FOR(koord3d_vector_t, const& i, to_the_sea.get_route()) {
				if (weg_t* const w = welt->lookup(i)->get_weg(water_wt)) {
					int type;
					for(  type=env_t::river_types-1;  type>0;  type--  ) {
						// lookup type
						if(  w->get_besch()==alle_wegtypen.get(env_t::river_type[type])  ) {
							break;
						}
					}
					// still room to expand
					if(  type>0  ) {
						// thus we enlarge
						w->set_besch( alle_wegtypen.get(env_t::river_type[type-1]) );
						w->calc_bild();
					}
				}
			}
		}
	}
}



void wegbauer_t::baue()
{
	if(get_count()<2  ||  get_count() > maximum) {
DBG_MESSAGE("wegbauer_t::baue()","called, but no valid route.");
		// no valid route here ...
		return;
	}
	DBG_MESSAGE("wegbauer_t::baue()", "type=%d max_n=%d start=%d,%d end=%d,%d", bautyp, get_count() - 1, route.front().x, route.front().y, route.back().x, route.back().y);

#ifdef DEBUG_ROUTES
long ms=dr_time();
#endif

	if ( (bautyp&terraform_flag)!=0  &&  (bautyp&(tunnel_flag|elevated_flag))==0  &&  bautyp!=river) {
		// do the terraforming
		do_terraforming();
	}
	// first add all new underground tiles ... (and finished if successful)
	if(bautyp&tunnel_flag) {
		baue_tunnelboden();
		return;
	}

	// add elevated ground for elevated tracks
	if(bautyp&elevated_flag) {
		baue_elevated();
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
			DBG_MESSAGE("wegbauer_t::baue", "schiene");
			baue_schiene();
			break;
		case strasse:
			baue_strasse();
			DBG_MESSAGE("wegbauer_t::baue", "strasse");
			break;
		case leitung:
			baue_leitung();
			break;
		case river:
			baue_fluss();
			break;
		default:
			break;
	}

	INT_CHECK("simbau 1087");
	baue_tunnel_und_bruecken();

	INT_CHECK("simbau 1087");

#ifdef DEBUG_ROUTES
DBG_MESSAGE("wegbauer_t::baue", "took %i ms",dr_time()-ms);
#endif
}



/*
 * This function calculates the distance of pos to the cuboid
 * spanned up by mini and maxi.
 * The result is already weighted according to
 * welt->get_settings().get_way_count_{straight,slope}().
 */
uint32 wegbauer_t::calc_distance( const koord3d &pos, const koord3d &mini, const koord3d &maxi )
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
