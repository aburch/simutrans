/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Strassen- und Schienenbau
 *
 * Hj. Malthaner
 */

#include <algorithm>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simmesg.h"
#include "../player/simplay.h"
#include "../simplan.h"
#include "../simtools.h"
#include "../simdepot.h"

#include "wegbauer.h"
#include "brueckenbauer.h"
#include "tunnelbauer.h"

#include "../besch/skin_besch.h"
#include "../besch/weg_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/spezial_obj_tpl.h"

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

#include "../dataobj/umgebung.h"
#include "../dataobj/route.h"
#include "../dataobj/translator.h"

// sorted heap, since we only need insert and pop
#include "../tpl/binary_heap_tpl.h" // fastest

#include "../dings/field.h"
#include "../dings/gebaeude.h"
#include "../dings/bruecke.h"
#include "../dings/tunnel.h"
#include "../dings/crossing.h"
#include "../dings/leitung2.h"
#include "../dings/groundobj.h"

#include "../vehicle/movingobj.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../gui/messagebox.h"
#include "../gui/karte.h"	// for debugging
#include "../gui/werkzeug_waehler.h"


#ifdef DEBUG_ROUTES
#include "../simsys.h"
#endif

// built bridges automatically
//#define AUTOMATIC_BRIDGES

// built tunnels automatically
//#define AUTOMATIC_TUNNELS

// lookup also return route and take the better of the two
#define REVERSE_CALC_ROUTE_TOO

const weg_besch_t *wegbauer_t::leitung_besch = NULL;

static stringhashtable_tpl <const weg_besch_t *> alle_wegtypen;


bool wegbauer_t::alle_wege_geladen()
{
	// some defaults to avoid hardcoded values
	strasse_t::default_strasse = wegbauer_t::weg_search(road_wt,50,0,weg_t::type_flat);
	if(strasse_t::default_strasse==NULL) {
		strasse_t::default_strasse = wegbauer_t::weg_search(road_wt,1,0,weg_t::type_flat);
	}
	schiene_t::default_schiene = wegbauer_t::weg_search(track_wt,80,0,weg_t::type_flat);
	if(schiene_t::default_schiene==NULL) {
		schiene_t::default_schiene = wegbauer_t::weg_search(track_wt,1,0,weg_t::type_flat);
	}
	monorail_t::default_monorail = wegbauer_t::weg_search(monorail_wt,1,0,weg_t::type_flat);
	if(monorail_t::default_monorail==NULL) {
		// only elevated???
		monorail_t::default_monorail = wegbauer_t::weg_search(monorail_wt,1,0,weg_t::type_elevated);
	}
	maglev_t::default_maglev = wegbauer_t::weg_search(maglev_wt,1,0,weg_t::type_flat);
	narrowgauge_t::default_narrowgauge = wegbauer_t::weg_search(narrowgauge_wt,1,0,weg_t::type_flat);
	kanal_t::default_kanal = wegbauer_t::weg_search(water_wt,1,0,weg_t::type_flat);
	runway_t::default_runway = wegbauer_t::weg_search(air_wt,1,0,weg_t::type_flat);
	wegbauer_t::leitung_besch = wegbauer_t::weg_search(powerline_wt,1,0,weg_t::type_flat);
	return true;
}


bool wegbauer_t::register_besch(const weg_besch_t *besch)
{
	DBG_DEBUG("wegbauer_t::register_besch()", besch->get_name());
	if(  alle_wegtypen.remove(besch->get_name())  ) {
		dbg->warning( "wegbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
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
const weg_besch_t* wegbauer_t::weg_search(const waytype_t wtyp, const uint32 speed_limit, const uint16 time, const weg_t::system_type system_type)
{
	const weg_besch_t* best = NULL;
	bool best_allowed = false; // Does the best way fulfill the timeline?
	for(  stringhashtable_iterator_tpl<const weg_besch_t*> iter(alle_wegtypen); iter.next();  ) {
		const weg_besch_t* const test = iter.get_current_value();
		if(  ((test->get_wtyp()==wtyp  &&
			(test->get_styp()==system_type  ||  system_type==weg_t::type_all))  ||  (test->get_wtyp()==track_wt  &&  test->get_styp()==weg_t::type_tram  &&  wtyp==tram_wt))
			&&  test->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
				bool test_allowed = test->get_intro_year_month()<=time  &&  time<test->get_retire_year_month();
				if(  !best_allowed  ||  time==0  ||  test_allowed  ) {
					if(  best==NULL  ||
						( best->get_topspeed() <      speed_limit       &&      speed_limit      <= test->get_topspeed()) || // test is faster than speed_limit
						( best->get_topspeed() <  test->get_topspeed()  &&  test->get_topspeed() <=     speed_limit  )    || // closer to desired speed (from the low end)
						(     speed_limit      <= test->get_topspeed()  &&  test->get_topspeed() <  best->get_topspeed()) || // closer to desired speed (from the top end)
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



const weg_besch_t * wegbauer_t::get_besch(const char * way_name,const uint16 time)
{
//DBG_MESSAGE("wegbauer_t::get_besch","return besch for %s in (%i)",way_name, time/12);
	const weg_besch_t *besch = alle_wegtypen.get(way_name);
	if(besch  &&  (time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))  ) {
		return besch;
	}
	return NULL;
}



// generates timeline message
void wegbauer_t::neuer_monat(karte_t *welt)
{
	const uint16 current_month = welt->get_timeline_year_month();
	if(current_month!=0) {
		// check, what changed
		slist_tpl <const weg_besch_t *> matching;
		stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);
		while(iter.next()) {
			const weg_besch_t * besch = iter.get_current_value();
			char	buf[256];

			const uint16 intro_month = besch->get_intro_year_month();
			if(intro_month == current_month) {
				sprintf(buf,
					translator::translate("way %s now available:\n"),
					translator::translate(besch->get_name()));
					welt->get_message()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->get_bild_nr(5,0));
			}

			const uint16 retire_month = besch->get_retire_year_month();
			if(retire_month == current_month) {
				sprintf(buf,
					translator::translate("way %s cannot longer used:\n"),
					translator::translate(besch->get_name()));
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
void wegbauer_t::fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, const weg_t::system_type styp, karte_t *welt)
{
	static stringhashtable_tpl<wkz_wegebau_t *> way_tool;
	const uint16 time = welt->get_timeline_year_month();

	// list of matching types (sorted by speed)
	vector_tpl<const weg_besch_t*> matching;

	stringhashtable_iterator_tpl<const weg_besch_t*> iter(alle_wegtypen);
	while(iter.next()) {
		const weg_besch_t* besch = iter.get_current_value();
		if (besch->get_styp() == styp &&
				besch->get_wtyp() == wtyp &&
				besch->get_cursor()->get_bild_nr(1) != IMG_LEER && (
					time == 0 ||
					(besch->get_intro_year_month() <= time && time < besch->get_retire_year_month())
				)) {
			matching.append(besch);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_ways);

	// now add sorted ways ...
	for (vector_tpl<const weg_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		const weg_besch_t* besch = *i;
		wkz_wegebau_t *wkz = way_tool.get(besch->get_name());
		if(wkz==NULL) {
			// not yet in hashtable
			wkz = new wkz_wegebau_t();
			wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
			wkz->cursor = besch->get_cursor()->get_bild_nr(0);
			wkz->default_param = besch->get_name();
			way_tool.put(besch->get_name(),wkz);
		}
		wzw->add_werkzeug( (werkzeug_t*)wkz );
	}
}




/* allow for railroad crossing
 * @author prissi
 */
bool
wegbauer_t::check_crossing(const koord zv, const grund_t *bd, waytype_t wtyp, const spieler_t *sp) const
{
	const weg_t *w = bd->get_weg_nr(0);
	if(  w  &&  w->get_waytype() == wtyp  ) {
		if(  ribi_t::doppelt(w->get_ribi_unmasked()) == ribi_t::doppelt(ribi_typ(zv))  ) {
			return true;
		}
		w = bd->get_weg_nr(1);
	}
	if(w  &&  !bd->get_halt().is_bound()  &&  check_owner(w->get_besitzer(),sp)  &&  crossing_logic_t::get_crossing(wtyp,w->get_waytype())!=NULL) {
		ribi_t::ribi w_ribi = w->get_ribi_unmasked();
		// it is our way we want to cross: can we built a crossing here?
		// both ways must be straight and no ends
		return ribi_t::ist_gerade(w_ribi)
					&&  !ribi_t::ist_einfach(w_ribi)
					&&  ribi_t::ist_gerade(ribi_typ(zv))
					&&  (w_ribi&ribi_typ(zv))==0;
	}
	if(  !w  ) {
		return true;
	}
	// nothing to cross here
	return false;
}


/* crossing of powerlines, or no powerline
 * @author prissi
 */
bool
wegbauer_t::check_for_leitung(const koord zv, const grund_t *bd) const
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
		  &&  (lt_ribi&ribi_typ(zv))==0;
	}
	// check for transformer
	if (bd->find<pumpe_t>() != NULL || bd->find<senke_t>()  != NULL) {
		return false;
	}
	// ok, there is not high power transmission stuff going on here
	return true;
}



// allowed slope? (not used ... since get_neightbour and some further check does this even better)
bool wegbauer_t::check_slope( const grund_t *from, const grund_t *to )
{
	const koord from_pos=from->get_pos().get_2d();
	const koord to_pos=to->get_pos().get_2d();
	const sint8 ribi=ribi_typ( from_pos, to_pos );

	if(from==to) {
		// this may happen, if the starting position is tested!
		return hang_t::ist_wegbar(from->get_grund_hang());
	}

	// check for valid slopes
	if(from->get_weg_hang()!=0) {
		if(!hang_t::ist_wegbar(from->get_weg_hang())) {
			// not valid
			return false;
		}
		const sint8 hang_ribi=ribi_typ(from->get_weg_hang());
		if(ribi!=hang_ribi  &&  ribi!=ribi_t::rueckwaerts(hang_ribi)) {
			// not down or up ...
			return false;
		}
	}
	// ok, now check destination hang
	if(to->get_weg_hang()!=0) {
		if(!hang_t::ist_wegbar(to->get_weg_hang())) {
			// not valid
			return false;
		}
		const sint8 hang_ribi=ribi_typ(to->get_weg_hang());
		if(ribi!=hang_ribi  &&  ribi!=ribi_t::rueckwaerts(hang_ribi)) {
			// not down or up ...
			return false;
		}
	}

	// now check offsets before changing the slope ...
	const sint8 slope_this = from->get_weg_hang();
	const sint16 hgt=from->get_hoehe()/Z_TILE_STEP;
	const sint16 to_hgt=to->get_hoehe()/Z_TILE_STEP;
	const sint8 to_slope=to->get_weg_hang();

	if(ribi==ribi_t::west) {
#ifndef DOUBLE_GROUNDS
		const sint8 diff_from_ground_1 = to_hgt+((to_slope/2)%2)-hgt-(slope_this%2);
		const sint8 diff_from_ground_2 = to_hgt+((to_slope/4)%2)-hgt-(slope_this/8);
#else
		const sint8 diff_from_ground_1 = to_hgt+((to_slope/3)%3)-hgt-(slope_this%3);
		const sint8 diff_from_ground_2 = to_hgt+((to_slope/9)%3)-hgt-(slope_this/27);
#endif
		if(diff_from_ground_1!=0  ||  diff_from_ground_2!=0) {
			return false;
		}
	}

	if(ribi==ribi_t::ost) {
#ifndef DOUBLE_GROUNDS
		const sint8 diff_from_ground_1 = to_hgt-((slope_this/2)%2)-hgt+(to_slope%2);
		const sint8 diff_from_ground_2 = to_hgt-((slope_this/4)%2)-hgt+(to_slope/8);
#else
		const sint8 diff_from_ground_1 = to_hgt-((slope_this/3)%3)-hgt+(to_slope%3);
		const sint8 diff_from_ground_2 = to_hgt-((slope_this/9)%3)-hgt+(to_slope/27);
#endif
		if(diff_from_ground_1!=0  ||  diff_from_ground_2!=0) {
			return false;
		}
	}

	if(ribi==ribi_t::nord) {
#ifndef DOUBLE_GROUNDS
		const sint8 diff_from_ground_1 = to_hgt+(to_slope%2)-hgt-(slope_this/8);
		const sint8 diff_from_ground_2 = to_hgt+((to_slope/2)%2)-hgt-((slope_this/4)%2);
#else
		const sint8 diff_from_ground_1 = to_hgt+(to_slope%3)-hgt-(slope_this/27);
		const sint8 diff_from_ground_2 = to_hgt+((to_slope/3)%3)-hgt-((slope_this/9)%3);
#endif
		if(diff_from_ground_1!=0  ||  diff_from_ground_2!=0) {
			return false;
		}
	}

	if(ribi==ribi_t::sued) {
#ifndef DOUBLE_GROUNDS
		const sint8 diff_from_ground_1 = to_hgt-(slope_this%2)-hgt+(to_slope/8);
		const sint8 diff_from_ground_2 = to_hgt-((slope_this/2)%2)-hgt+((to_slope/4)%2);
#else
		const sint8 diff_from_ground_1 = to_hgt-(slope_this%3)-hgt+(to_slope/27);
		const sint8 diff_from_ground_2 = to_hgt-((slope_this/3)%3)-hgt+((to_slope/9)%3);
#endif
		if(diff_from_ground_1!=0  ||  diff_from_ground_2!=0) {
			return false;
		}
	}

	return true;
}



// allowed owner?
bool wegbauer_t::check_owner( const spieler_t *sp1, const spieler_t *sp2 ) const
{
	// unowned, mine or public property?
	return sp1==NULL  ||  sp1==sp2  ||  sp1==welt->get_spieler(1);
}



/* do not go through depots, station buildings etc. ...
 * direction results from layout
 */
static bool check_building( const grund_t *to, const koord dir )
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
		depot_t *dp = to->get_depot();
		if(dp!=NULL) {
			gb = dynamic_cast<gebaeude_t *>(dp);
		}
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

	if(bautyp==luft  &&  (from->get_grund_hang()+to->get_grund_hang()!=0  ||  (from->hat_wege()  &&  from->hat_weg(air_wt)==0))) {
		// absolutely no slopes for runways, neither other ways
		return false;
	}

	if(from==to) {
		if((bautyp&tunnel_flag)  &&  !hang_t::ist_wegbar(from->get_weg_hang())) {
//			DBG_MESSAGE("wrong slopes at","%i,%i ribi1=%d",from_pos.x,from_pos.y,ribi_typ(from->get_weg_hang()));
			return false;
		}
	}
	else {
		if(from->get_weg_hang()  &&  ribi_t::doppelt(ribi_typ(from->get_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
//			DBG_MESSAGE("wrong slopes between","%i,%i and %i,%i, ribi1=%d, ribi2=%d",from_pos.x,from_pos.y,to_pos.x,to_pos.y,ribi_typ(from->get_weg_hang()),ribi_typ(zv));
			return false;
		}
		if(to->get_weg_hang()  &&  ribi_t::doppelt(ribi_typ(to->get_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
//			DBG_MESSAGE("wrong slopes between","%i,%i and %i,%i, ribi1=%d, ribi2=%d",from_pos.x,from_pos.y,to_pos.x,to_pos.y,ribi_typ(to->get_weg_hang()),ribi_typ(zv));
			return false;
		}
	}

	// ok, slopes are ok
	bool ok = to->ist_natur()  &&  !to->ist_wasser();
	bool fundament = to->get_typ()==grund_t::fundament;
	const gebaeude_t* gb = to->find<gebaeude_t>();

	// no crossings to halt
	if(to!=from  &&  bautyp!=leitung  &&  (bautyp&elevated_flag)==0) {
		static koord gb_to_zv[4] = { koord::sued, koord::ost, koord::nord, koord::west };
		if(gb  &&  (gb->get_besitzer()==sp  ||  to->get_halt().is_bound())) {

			// terminal imposes stronger direction checks
			if(gb->get_tile()->get_besch()->get_all_layouts()==4) {
				if(zv!=gb_to_zv[(gb->get_tile()->get_layout()+2)&3]) {
//DBG_MESSAGE("cannot go","from %i,%i, to %i, %i due to 4 way stop",from_pos.x,from_pos.y,to_pos.x,to_pos.y);
					return false;
				}
			}
			else {
				// through station
				if( !ribi_t::ist_gerade( ribi_typ(zv)|ribi_typ(gb_to_zv[gb->get_tile()->get_layout()&1]) ) ) {
					return false;
				}
			}
		}

		// no crossings from halt
		const gebaeude_t* from_gb = from->find<gebaeude_t>();
		if(from_gb  &&  (from_gb->get_besitzer()==sp  ||  from->get_halt().is_bound())) {
			// terminal imposes stronger direction checks
			if(from_gb->get_tile()->get_besch()->get_all_layouts()==4) {
				if(zv!=gb_to_zv[from_gb->get_tile()->get_layout()]) {
//DBG_MESSAGE("cannot go","from %i,%i, to %i, %i due to 4 way stop",from_pos.x,from_pos.y,to_pos.x,to_pos.y);
					return false;
				}
			}
			else {
				// through station
				if( !ribi_t::ist_gerade( ribi_typ(zv)|ribi_typ(gb_to_zv[from_gb->get_tile()->get_layout()&1]) ) ) {
					return false;
				}
			}
		}
	}

	// universal check for elevated things ...
	if(bautyp&elevated_flag) {
		if(to->hat_weg(air_wt)  ||  to->ist_wasser()  ||  !check_for_leitung(zv,to)  || (!to->ist_karten_boden() && to->get_typ()!=grund_t::monorailboden) ||  to->get_typ()==grund_t::brueckenboden  ||  to->get_typ()==grund_t::tunnelboden) {
			// no suitable ground below!
			return false;
		}
		//sint16 height = welt->lookup(to->get_pos().get_2d())->get_kartenboden()->get_hoehe()+Z_TILE_STEP;
		sint16 height = to->get_hoehe()+Z_TILE_STEP;
		grund_t *to2 = welt->lookup(koord3d(to->get_pos().get_2d(),height));
		if(to2) {
			if(to2->get_weg_nr(0)) {
				// already an elevated ground here => it will has always a way object, that indicates ownership
				ok = to2->get_typ()==grund_t::monorailboden  &&  check_owner(to2->obj_bei(0)->get_besitzer(),sp);
				ok &= to2->get_weg_nr(0)->get_besch()->get_wtyp()==besch->get_wtyp();
			}
			else {
				ok = to2->find<leitung_t>()==NULL;
			}
			if(!ok) {
DBG_MESSAGE("wegbauer_t::is_allowed_step()","wrong ground already there!");
				return false;
			}
			if(  !check_building( to2, -zv )  ) {
				return false;
			}
		}
		if(gb) {
			// no halt => citybuilding => do not touch
			if(!check_owner(gb->get_besitzer(),sp)  ||  gb->get_tile()->get_hintergrund(0,1,0)!=IMG_LEER) {  // also check for too high buildings ...
				return false;
			}
			// building above houses is expensive ... avoid it!
			*costs += 4;
		}
		ok = true;
	}

	if((bautyp&tunnel_flag)==0) {
		// not jumping to other lanes on bridges
		if( to->get_typ()==grund_t::brueckenboden ) {
			weg_t *weg=to->get_weg_nr(0);
			if(weg && !ribi_t::ist_gerade(weg->get_ribi_unmasked()|ribi_typ(zv))) {
				return false;
			}
		}
		// Do not switch to tunnel through cliffs!
		if( from->get_typ() == grund_t::tunnelboden  &&  to->get_typ() != grund_t::tunnelboden  &&  !from->ist_karten_boden() ) {
			return false;
		}
		if( to->get_typ()==grund_t::tunnelboden  &&  from->get_typ() != grund_t::tunnelboden   &&  !to->ist_karten_boden() ) {
			return false;
		}
	}

	// no check way specific stuff
	switch(bautyp&bautyp_mask) {

		case strasse:
		{
			const weg_t *str=to->get_weg(road_wt);
			// we allow connection to any road
			ok =	(str  ||  !fundament)  &&  !to->ist_wasser()  &&  check_for_leitung(zv,to);
			if(!ok) {
				return false;
			}
			ok = !to->hat_wege()  ||  check_crossing(zv,to,road_wt,sp);
			if(!ok) {
				const weg_t *sch=to->get_weg(track_wt);
				if(sch  &&  sch->get_besch()->get_styp()==7) {
					ok = true;
				}
			}
			if(ok) {
				const weg_t *from_str=from->get_weg(road_wt);
				// check for end/start of bridge
				if(to->get_weg_hang()!=to->get_grund_hang()  &&  (from_str==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|from_str->get_ribi_unmasked()))) {
					return false;
				}
				// check for depots/stops/...
				if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
					return false;
				}
				// calculate costs
				*costs = str ? 0 : welt->get_einstellungen()->way_count_straight;
				if((str==NULL  &&  to->hat_wege())  ||  (str  &&  to->has_two_ways())) {
					*costs += 4;	// avoid crossings
				}
				if(to->get_weg_hang()!=0) {
					*costs += welt->get_einstellungen()->way_count_slope;
				}
			}
		}
		break;

		case schiene:
		{
			const weg_t *sch=to->get_weg(track_wt);
			// extra check for AI construction (not adding to existing tracks!)
			if((bautyp&bot_flag)!=0  &&  (gb  ||  sch  ||  to->get_halt().is_bound())) {
				return false;
			}
			// ok, regular construction here
			if((bautyp&elevated_flag)==0) {
				ok =	!fundament  &&  !to->ist_wasser()  &&  check_crossing(zv,to,track_wt,sp)  &&
				  (!sch  ||  check_owner(sch->get_besitzer(),sp))  &&
					check_for_leitung(zv,to);
			}
			if(ok) {
				// check for end/start of bridge
				if(to->get_weg_hang()!=to->get_grund_hang()  &&  (sch==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|sch->get_ribi_unmasked()))) {
					return false;
				}
				// check for depots/stops/...
				if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
					return false;
				}
				// calculate costs
				*costs = sch ? welt->get_einstellungen()->way_count_straight : welt->get_einstellungen()->way_count_straight+1;	// only prefer existing rails a little
				if((sch  &&  to->has_two_ways())  ||  (sch==NULL  &&  to->hat_wege())) {
					*costs += 4;	// avoid crossings
				}
				if(to->get_weg_hang()!=0) {
					*costs += welt->get_einstellungen()->way_count_slope;
				}
			}
		}
		break;

		// like tram, but checks for bridges too
		// will not connect to any other ways
		case monorail:
		default:
		{
			const weg_t *sch=to->get_weg(besch->get_wtyp());
			// extra check for AI construction (not adding to existing tracks!)
			if(bautyp&bot_flag  &&  (gb  ||  sch  ||  to->get_halt().is_bound())) {
				return false;
			}
			if((bautyp&elevated_flag)==0) {
				// classical monorail
				ok =	!fundament  &&  !to->ist_wasser()  &&  check_crossing(zv,to,besch->get_wtyp(),sp)  &&
				  (!sch  ||  check_owner(sch->get_besitzer(),sp))  &&
					check_for_leitung(zv,to)  &&  !to->get_depot();
				// check for end/start of bridge
				if(to->get_weg_hang()!=to->get_grund_hang()  &&  (sch==NULL  ||  ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->get_ribi_unmasked()))) {
					return false;
				}
				// check for depots/stops/...
				if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
					return false;
				}
			}
			// calculate costs
			if(ok) {
				*costs = welt->get_einstellungen()->way_count_straight;
				if(!to->hat_wege()) {
					*costs += welt->get_einstellungen()->way_count_straight;
				}
				if(to->get_weg_hang()!=0) {
					*costs += welt->get_einstellungen()->way_count_slope;
				}
			}
		}
		break;

		case schiene_tram: // Dario: Tramway
		{
			ok =	(ok ||
							(to->hat_weg(track_wt)  &&  check_owner(to->get_weg(track_wt)->get_besitzer(),sp))  ||
							(to->hat_weg(road_wt)  &&  check_owner(to->get_weg(road_wt)->get_besitzer(),sp)  &&  to->get_weg_nr(1)==NULL))
					 &&  check_for_leitung(zv,to);
			if(ok) {
				// check for depots/stops/...
				if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
					return false;
				}
				// calculate costs
				*costs = to->hat_weg(track_wt) ? welt->get_einstellungen()->way_count_straight : welt->get_einstellungen()->way_count_straight+1;	// only prefer existing rails a little
				// perfer own track
				if(to->hat_weg(road_wt)) {
					*costs += welt->get_einstellungen()->way_count_straight;
				}
				if(to->get_weg_hang()!=0) {
					*costs += welt->get_einstellungen()->way_count_slope;
				}
			}
		}
		break;

		case leitung:
			ok = !to->ist_wasser()  &&  (to->get_weg(air_wt)==NULL);
			if(to->get_weg_nr(0)!=NULL) {
				// only 90 deg crossings, only a signle way
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
			// only fields are allowed
			if(to->get_typ()!=grund_t::boden) {
				ok &= to->get_typ() == grund_t::fundament && to->find<field_t>();
			}
			// no bridges and monorails here in the air
			ok &= (welt->lookup(to_pos)->get_boden_in_hoehe(to->get_pos().z+Z_TILE_STEP)==NULL);
			// calculate costs
			if(ok) {
				*costs = welt->get_einstellungen()->way_count_straight;
				if(  !to->get_leitung()  ) {
					// extra malus for not following an existing line or going on ways
					*costs += welt->get_einstellungen()->way_count_double_curve + to->hat_wege() ? 8 : 0; // prefer existing powerlines
				}
			}
		break;

		case wasser:
			ok = (ok  ||  to->ist_wasser()  ||  (to->hat_weg(water_wt)  &&  check_owner(to->get_weg(water_wt)->get_besitzer(),sp)))  &&  check_for_leitung(zv,to);
			// calculate costs
			if(ok) {
				// check for depots/stops/...
				if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
					return false;
				}
				*costs = to->ist_wasser()  ||  to->hat_weg(water_wt) ? welt->get_einstellungen()->way_count_straight : welt->get_einstellungen()->way_count_leaving_road;	// prefer water very much ...
				if(to->get_weg_hang()!=0) {
					*costs += welt->get_einstellungen()->way_count_slope*2;
				}
			}
			break;

		case river:
			if(  to->ist_wasser()  ) {
				ok = true;
				// do not care while in ocean
				*costs = 1;
			}
			else {
				// only downstream
				ok = from->get_pos().z>=to->get_pos().z  &&  check_for_leitung(zv,to)  &&  (to->hat_weg(water_wt)  ||  !to->hat_wege());
				// calculate costs
				if(ok) {
					// check for depots/stops/...
					if(  !check_building( from, zv )  ||  !check_building( to, -zv )  ) {
						return false;
					}
					// prefer existing rivers:
					*costs = to->hat_weg(water_wt) ? 10 : 10+simrand(welt->get_einstellungen()->way_count_90_curve);
					if(to->get_weg_hang()!=0) {
						*costs += welt->get_einstellungen()->way_count_slope*10;
					}
				}
			}
			break;

		case luft: // hsiegeln: runway
			ok = !to->ist_wasser() && (to->hat_weg(air_wt) || !to->hat_wege())  &&  to->find<leitung_t>()==NULL  &&  !fundament  &&  check_building( from, zv )  &&  check_building( to, -zv );
			// calculate costs
			*costs = welt->get_einstellungen()->way_count_straight;
			break;
	}
	return ok;
}


void wegbauer_t::check_for_bridge(const grund_t* parent_from, const grund_t* from, const vector_tpl<koord3d> &ziel)
{
	// wrong starting slope or tile already occupied with a way ...
	if (!hang_t::ist_wegbar(from->get_grund_hang())) {
		return;
	}

	/*
	 * now check existing ways:
	 * no tunnels/bridges at crossings and no track tunnels/bridges on roads (bot road tunnels/bridges on tram are allowed).
	 * (keep in mind, that our waytype isn't currently on the tile and will be built later)
	 */
	if(  from->has_two_ways()  ) {
		if(  from->ist_uebergang()  ||  besch->get_wtyp() != road_wt  ) {
			return;
		}
	}
	else if(  from->hat_wege()  ) {
		// ground has (currently) exact one way
		weg_t *w = from->get_weg_nr(0);
		if(  w->get_besch()->get_styp() != weg_t::type_tram  &&  w->get_besch() != besch  ) {
			return;
		}
	}

	const koord zv=from->get_pos().get_2d()-parent_from->get_pos().get_2d();

	// ok, so now we do a closer investigation
	if(  bruecke_besch && (  ribi_typ(from->get_grund_hang()) == ribi_t::rueckwaerts(ribi_typ(zv))  ||  from->get_grund_hang() == 0  )
			&&  brueckenbauer_t::ist_ende_ok(sp, from)  &&  !from->get_leitung()  ) {
		// Try a bridge.
		const long cost_difference=besch->get_wartung()>0 ? (bruecke_besch->get_wartung()*4l+3l)/besch->get_wartung() : 16;
		const char *error;
		// add long bridge (i==0) and shortest possible bridge (i==1)
		koord3d end;
		const grund_t* gr_end;
		for( uint8 i = 0; i < 2; i++ ) {
			end = brueckenbauer_t::finde_ende( welt, from->get_pos(), zv, bruecke_besch, error, i!=0 );
			gr_end = welt->lookup(end);
			if(  error == NULL  &&  !ziel.is_contained(end)  &&  brueckenbauer_t::ist_ende_ok(sp, gr_end) ) {
				uint32 length = koord_distance(from->get_pos(), end);
				// If there is a slope on the starting tile, it's taken into account in is_allowed_step, but a bridge will be flat!
				sint8 num_slopes = (from->get_grund_hang() == hang_t::flach) ? 1 : -1;
				// On the end tile, we haven't to subtract way_count_slope, since is_allowed_step isn't called with this tile.
				num_slopes += (gr_end->get_grund_hang() == hang_t::flach) ? 1 : 0;
				next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference + num_slopes*welt->get_einstellungen()->way_count_slope ));
			}
		}
		return;
	}

	if(  tunnel_besch  &&  ribi_typ(from->get_grund_hang()) == ribi_typ(zv)  ) {
	// uphill hang ... may be tunnel?
		const long cost_difference=besch->get_wartung()>0 ? (tunnel_besch->get_wartung()*4l+3l)/besch->get_wartung() : 16;
		koord3d end = tunnelbauer_t::finde_ende( welt, from->get_pos(), zv, besch->get_wtyp());
		if(  end != koord3d::invalid  &&  !ziel.is_contained(end)  ) {
			uint32 length = koord_distance(from->get_pos(), end);
			next_gr.append(next_gr_t(welt->lookup(end), length * cost_difference ));
			return;
		}
	}
}


wegbauer_t::wegbauer_t(karte_t* wl, spieler_t* spl) : next_gr(32)
{
	n      = 0;
	max_n  = -1;
	sp     = spl;
	welt   = wl;
	bautyp = strasse;   // kann mit route_fuer() gesetzt werden
	maximum = 2000;// CA $ PER TILE

	keep_existing_ways = false;
	keep_existing_city_roads = false;
	keep_existing_faster_ways = false;
}


/**
 * If a way is built on top of another way, should the type
 * of the former way be kept or replced (true == keep)
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



void
wegbauer_t::route_fuer(enum bautyp_t wt, const weg_besch_t *b, const tunnel_besch_t *tunnel, const bruecke_besch_t *br)
{
	bautyp = wt;
	besch = b;
	bruecke_besch = br;
	tunnel_besch = tunnel;
	if(wt&tunnel_flag  &&  tunnel==NULL) {
		dbg->fatal("wegbauer_t::route_fuer()","needs a tunnel description for an underground route!");
	}
	if((wt&bautyp_mask)==luft) {
		wt = (bautyp_t)(wt&(bautyp_mask|bot_flag));
	}
	if(sp==NULL) {
		bruecke_besch = NULL;
		tunnel_besch = NULL;
	}
	else if(  bautyp != river  ) {
#ifdef AUTOMATIC_BRIDGES
		if(  bruecke_besch == NULL  ) {
			bruecke_besch = brueckenbauer_t::find_bridge((waytype_t)b->get_wtyp(),25,welt->get_timeline_year_month());
		}
#endif
#ifdef AUTOMATIC_TUNNELS
		if(  tunnel_besch == NULL  ) {
			tunnel_besch = tunnelbauer_t::find_tunnel((waytype_t)b->get_wtyp(),25,welt->get_timeline_year_month());
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
	for( uint32 i = 1; i < ziel.get_count(); i++ ) {
		const koord3d &current = ziel[i];
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
long
wegbauer_t::intern_calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel)
{
	// we assume fail
	max_n = -1;

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// check for existing koordinates
	bool has_target_ground = false;
	for( uint32 i = 0; i < ziel.get_count(); i++ ) {
		has_target_ground |= welt->lookup(ziel[i]) != NULL;
	}
	if( !has_target_ground ) {
		return -1;
	}

	// calculate the minimal cuboid containing 'ziel'
	koord3d mini, maxi;
	get_mini_maxi( ziel, mini, maxi );

	// memory in static list ...
	if(route_t::nodes==NULL) {
		route_t::MAX_STEP = welt->get_einstellungen()->get_max_route_steps();	// may need very much memory => configurable
		route_t::nodes = new route_t::ANode[route_t::MAX_STEP+4+1];
	}

	// there are several variant for mantaining the open list
	// however, only binary heap and HOT queue with binary heap are worth considering
#ifdef tpl_HOT_queue_tpl_h
	//static HOT_queue_tpl <route_t::ANode *> queue;
#else
#ifdef tpl_binary_heap_tpl_h
	static binary_heap_tpl <route_t::ANode *> queue;
#else
#ifdef tpl_sorted_heap_tpl_h
	//static sorted_heap_tpl <route_t::ANode *> queue;
#else
	//static prioqueue_tpl <route_t::ANode *> queue;
#endif
#endif
#endif

	// nothing in lists
	welt->unmarkiere_alle();

	// clear the queue (should be empty anyhow)
	queue.clear();

	// some thing for the search
	grund_t *to;
	koord3d gr_pos;	// just the last valid pos ...
	route_t::ANode *tmp;
	uint32 step = 0;
	const grund_t* gr;

	for( uint32 i = 0; i < start.get_count(); i++ ) {
		gr = welt->lookup(start[i]);

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
		tmp->f = calc_distance(start[i], mini, maxi);
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

		if(welt->ist_markiert(test_tmp->gr)) {
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		gr = tmp->gr;
		welt->markiere(gr);
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
		const koord bridge_nsow=tmp->parent!=NULL ? gr->get_pos().get_2d()-tmp->parent->gr->get_pos().get_2d() : koord::invalid;

		// testing all four possible directions
		for(int r=0; r<4; r++) {

			to = NULL;
			if(!gr->get_neighbour(to,invalid_wt,koord::nsow[r])) {
				continue;
			}

			// something valid?
			if(to==NULL  ||   welt->ist_markiert(to)) {
				continue;
			}

			long new_cost = 0;
			bool is_ok = is_allowed_step(gr,to,&new_cost);

			// we check here for 180° turns and the end of bridges ...
			if(is_ok) {
				if(tmp->parent) {

					// no 180 deg turns ...
					if(tmp->parent->gr==to) {
						continue;
					}

					// ok, check if previous was tunnel or bridge (i.e. there is a gap)
					const koord parent_pos=tmp->parent->gr->get_pos().get_2d();
					const koord to_pos=to->get_pos().get_2d();
					// distance>1
					if(abs(parent_pos.x-gr_pos.x)>1  ||   abs(parent_pos.y-gr_pos.y)>1) {
						if(ribi_typ(parent_pos,to_pos)!=ribi_typ(gr_pos.get_2d(),to_pos)) {
							// not a straight line
							continue;
						}
					}
				}
				// now add it to the array ...
				next_gr.append(next_gr_t(to, new_cost));
			}
			else if(tmp->parent!=NULL  &&  bridge_nsow==koord::nsow[r]) {
				// try to build a bridge or tunnel here, since we cannot go here ...
				check_for_bridge(tmp->parent->gr,gr,ziel);
			}
		}

		// now check all valid ones ...
		for(unsigned r=0; r<next_gr.get_count(); r++) {

			to = next_gr[r].gr;
			if(welt->ist_markiert(to)) {
				continue;
			}

			// new values for cost g
			uint32 new_g = tmp->g + next_gr[r].cost;

			// check for curves (usually, one would need the lastlast and the last;
			// if not there, then we could just take the last
			uint8 current_dir;
			if(tmp->parent!=NULL) {
				current_dir = ribi_typ( tmp->parent->gr->get_pos().get_2d(), to->get_pos().get_2d() );
				if(tmp->dir!=current_dir) {
					new_g += welt->get_einstellungen()->way_count_curve;
					if(tmp->parent->dir!=tmp->dir) {
						// discourage double turns
						new_g += welt->get_einstellungen()->way_count_double_curve;
					}
					else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
						// discourage v turns heavily
						new_g += welt->get_einstellungen()->way_count_90_curve;
					}
				}
				else if(bautyp==leitung  &&  ribi_t::ist_kurve(current_dir)) {
					new_g += welt->get_einstellungen()->way_count_double_curve;
				}
				// extra malus leave an existing road after only one tile
				if(tmp->parent->gr->hat_weg((waytype_t)besch->get_wtyp())  &&
					!gr->hat_weg((waytype_t)besch->get_wtyp())  &&
					to->hat_weg((waytype_t)besch->get_wtyp()) ) {
					// but only if not straight track
					if(!ribi_t::ist_gerade(tmp->dir)) {
						new_g += welt->get_einstellungen()->way_count_leaving_road;
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
				new_g += welt->get_einstellungen()->way_count_double_curve;
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

//DBG_DEBUG("reached","%i,%i",tmp->pos.x,tmp->pos.y);
	// target reached?
	if( !ziel.is_contained(gr->get_pos())  || step >=route_t::MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("wegbauer_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,route_t::MAX_STEP);
		return -1;
	}
	else {
		const long cost = tmp->g;
		// reached => construct route
		while(tmp != NULL) {
			route.append(tmp->gr->get_pos());
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->parent;
		}

		// maybe here bridges should be optimized ...
		max_n = route.get_count() - 1;
		return cost;
	}

	return -1;
}



void
wegbauer_t::intern_calc_straight_route(const koord3d start, const koord3d ziel)
{
	bool ok=true;
	max_n = -1;

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
	test_bd = welt->lookup(ziel);
	if((bautyp&tunnel_flag)==0  &&  test_bd  &&  !is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// ... or to end
		return;
	}

	koord3d pos=start;

	route.clear();
	route.append(start);

	while(pos.get_2d()!=ziel.get_2d()  &&  ok) {

		// shortest way
		koord diff;
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
		}
		else {
			diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
		}
		if(bautyp&tunnel_flag) {
#ifdef ONLY_TUNNELS_BELOW_GROUND
			// ground must be above tunnel
			ok &= (welt->lookup(pos.get_2d())->get_kartenboden()->get_hoehe() > pos.z);
#endif
			grund_t *bd_von = welt->lookup(pos);
			if(  bd_von  ) {
				ok = bd_von->get_typ() == grund_t::tunnelboden  &&  bd_von->get_weg_nr(0)->get_waytype() == besch->get_wtyp();
				// if we have a slope, we must adjust height correspondingly
				if(  bd_von->get_weg_hang()!=hang_t::flach  ) {
					if(  ribi_typ(bd_von->get_weg_hang())==ribi_typ(diff)  ) {
						pos.z += 1;
					}
				}
			}
			else {
				// check for slope down ...
				bd_von = welt->lookup(pos+koord3d(0,0,-1));
				if(  bd_von  ) {
					ok = bd_von->get_typ() == grund_t::tunnelboden  &&  bd_von->get_weg_nr(0)->get_waytype() == besch->get_wtyp()  &&  ribi_typ(bd_von->get_weg_hang())==ribi_t::rueckwaerts(ribi_typ(diff));
					if(  ok  ) {
						route[route.get_count()-1].z -= 1;
						pos.z -= 1;
					}
				}
			}
			// check for halt or crossing ...
			if(ok  &&  bd_von  &&  (bd_von->is_halt()  ||  bd_von->has_two_ways())) {
				// then only single dir is ok ...
				ribi_t::ribi haltribi = bd_von->get_weg_ribi_unmasked( (waytype_t)(bautyp&(~wegbauer_t::tunnel_flag)) );
				haltribi = ribi_t::doppelt(haltribi);
				ribi_t::ribi diffribi = ribi_t::doppelt( ribi_typ(diff) );
				ok = (haltribi==diffribi);
			}
			// and the same for the last tile
			if(  ok  &&  pos.get_2d()+diff==ziel.get_2d()  ) {
				grund_t *bd_von = welt->lookup(koord3d(ziel.get_2d(),pos.z));
				if(  bd_von  ) {
					ok = bd_von->get_typ() == grund_t::tunnelboden  &&  bd_von->get_weg_nr(0)->get_waytype() == besch->get_wtyp();
				}
				// check for halt or crossing ...
				if(ok  &&  bd_von  &&  (bd_von->is_halt()  ||  bd_von->has_two_ways())) {
					// then only single dir is ok ...
					ribi_t::ribi haltribi = bd_von->get_weg_ribi_unmasked( (waytype_t)(bautyp&(~wegbauer_t::tunnel_flag)) );
					haltribi = ribi_t::doppelt(haltribi);
					ribi_t::ribi diffribi = ribi_t::doppelt( ribi_typ(diff) );
					ok = (haltribi==diffribi);
				}
			}
			pos += diff;
		}
		else {
			grund_t *bd_von = welt->lookup(pos);
			if (bd_von==NULL) {
				ok = false;
			}
			else
			{
				grund_t *bd_nach = NULL;
				ok = ok  &&  bd_von->get_neighbour(bd_nach, invalid_wt, diff);

				// allowed ground?
				ok = ok  &&  bd_nach  &&  is_allowed_step(bd_von,bd_nach,&dummy_cost);
				if (ok) {
					pos = bd_nach->get_pos();
				}
			}
		}

		route.append(pos);
DBG_MESSAGE("wegbauer_t::calc_straight_route()","step %i,%i = %i",diff.x,diff.y,ok);
	}
	ok = ok && (bautyp&tunnel_flag ? pos.get_2d()==ziel.get_2d() : pos==ziel);

	// we can built a straight route?
	if(ok) {
		max_n = route.get_count() - 1;
DBG_MESSAGE("wegbauer_t::intern_calc_straight_route()","found straight route max_n=%i",max_n);
	}
}



// special for starting/landing runways
bool
wegbauer_t::intern_calc_route_runways(koord3d start3d, const koord3d ziel3d)
{
	const koord start=start3d.get_2d();
	const koord ziel=ziel3d.get_2d();
	// assume, we will fail
	max_n = -1;
	// check for straight line!
	const ribi_t::ribi ribi = ribi_typ( start, ziel );
	if(!ribi_t::ist_gerade(ribi)) {
		// only straight runways!
		return false;
	}
	// not too close to the border?
	if(	!(welt->ist_in_kartengrenzen(start-koord(5,5))  &&  welt->ist_in_kartengrenzen(start+koord(5,5)))  ||
		!(welt->ist_in_kartengrenzen(ziel-koord(5,5))  &&  welt->ist_in_kartengrenzen(ziel+koord(5,5)))  ) {
		if(sp==welt->get_active_player()) {
			create_win( new news_img("Zu nah am Kartenrand"), w_time_delete, magic_none);
			return false;
		}
	}


	// now try begin and endpoint
	const koord zv(ribi);
	// end start
	const grund_t *gr = welt->lookup(start)->get_kartenboden();
	const weg_t *weg=gr->get_weg(air_wt);
	if(weg  &&  (weg->get_besch()->get_styp()==0  ||  ribi_t::ist_kurve(weg->get_ribi()|ribi))) {
		// cannot connect to taxiway at the start and no curve possible
		return false;
	}
	// check end
	gr = welt->lookup(ziel)->get_kartenboden();
	weg=gr->get_weg(air_wt);
	if(weg  &&  (weg->get_besch()->get_styp()==1  ||  ribi_t::ist_kurve(weg->get_ribi()|ribi))) {
		// cannot connect to taxiway at the end and no curve at the end
		return false;
	}
	// now try a straight line with no crossings and no curves at the end
	const int dist=koord_distance( ziel, start );
	for(  int i=0;  i<=dist;  i++  ) {
		gr=welt->lookup(start+zv*i)->get_kartenboden();
		// no slopes!
		if(gr->get_grund_hang()!=0) {
			return false;
		}
		if(gr->hat_wege()  &&  !gr->hat_weg(air_wt)) {
			// cannot cross another way
			return false;
		}
	}
	// now we can build here
	route.clear();
	route.resize(dist + 2);
	for(  int i=0;  i<=dist;  i++  ) {
		route.append(welt->lookup(start + zv * i)->get_kartenboden()->get_pos());
	}
	max_n = dist;
	return true;
}




/* calc_straight_route (maximum one curve, including diagonals)
 *
 */
void
wegbauer_t::calc_straight_route(koord3d start, const koord3d ziel)
{
	DBG_MESSAGE("wegbauer_t::calc_straight_route()","from %d,%d,%d to %d,%d,%d",start.x,start.y,start.z, ziel.x,ziel.y,ziel.z );
	if(bautyp==luft  &&  besch->get_topspeed()>=250) {
		// these are straight anyway ...
		intern_calc_route_runways(start, ziel);
	}
	else {
		intern_calc_straight_route(start,ziel);
		if(max_n==-1) {
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
		// river only go downards => start and end are clear ...
		if(  start[0].z > ziel[0].z  ) {
			intern_calc_route( start, ziel );
		}
		else {
			intern_calc_route( ziel, start );
		}
		while(  route.get_count()>0  &&  welt->lookup(route[0])->get_grund_hang()==hang_t::flach  ) {
			// remove leading water ...
			route.remove_at(0);
		}
		// now this is the true length of the river
		max_n = route.get_count()-1;
	}
	else {
		keep_existing_city_roads |= (bautyp&bot_flag)!=0;
		long cost2 = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		if(cost2<0) {
			// not sucessful: try backwards
			intern_calc_route(ziel,start);
			return;
		}

#ifdef REVERSE_CALC_ROUTE_TOO
		vector_tpl<koord3d> route2(0);
		swap(route, route2);
		long cost = intern_calc_route(ziel, start);
		INT_CHECK("wegbauer 1165");

		// the cheaper will survive ...
		if(  cost2 < cost  ||  cost < 0  ) {
			swap(route, route2);
		}
#endif
		max_n = route.get_count() - 1;

	}
	INT_CHECK("wegbauer 778");
#ifdef DEBUG_ROUTES
DBG_MESSAGE("calc_route::calc_route", "took %i ms",dr_time()-ms);
#endif
}



ribi_t::ribi
wegbauer_t::calc_ribi(int step)
{
	ribi_t::ribi ribi = ribi_t::keine;

	if(step < max_n) {
		// check distance, only neighbours
		const koord zv = (route[step + 1] - route[step]).get_2d();
		if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
			ribi |= ribi_typ(zv);
		}
	}
	if(step > 0) {
		// check distance, only neighbours
		const koord zv = (route[step - 1] - route[step]).get_2d();
		if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
			ribi |= ribi_typ(zv);
		}
	}
	return ribi;
}



void
wegbauer_t::baue_tunnel_und_bruecken()
{
	if(bruecke_besch==NULL  &&  tunnel_besch==NULL) {
		return;
	}
	// check for bridges and tunnels
	for(int i=1; i<max_n-1; i++) {
		koord d = (route[i + 1] - route[i]).get_2d();
		koord zv = koord (sgn(d.x), sgn(d.y));

		// ok, here is a gap ...
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			if(d.x*d.y!=0) {
				dbg->error("wegbauer_t::baue_tunnel_und_bruecken()","cannot built a bridge between %s (n=%i, max_n=%i) and %s", route[i].get_str(),i,max_n,route[i+1].get_str());
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
				brueckenbauer_t::baue( welt, sp, route[i+1].get_2d(), bruecke_besch);
			}
			else {
				// tunnel
				tunnelbauer_t::baue( welt, sp, route[i].get_2d(), tunnel_besch );
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
			waytype_t wt = (waytype_t)(besch->get_wtyp());
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
							brueckenbauer_t::baue( welt, sp, route[i].get_2d(), bruecke_besch);
						}
					}
					else if( tunnel_besch ) {
						// make a short tunnel
						wi->set_ribi(ribi_typ(hang_t::gegenueber(h)));
						wi1->set_ribi(ribi_typ(h));
						tunnelbauer_t::baue( welt, sp, route[i].get_2d(), tunnel_besch );
					}
					INT_CHECK( "wegbauer 1584" );
				}
			}
		}
	}
}



/* returns the amount needed to built this way
 * author prissi
 */
sint64 wegbauer_t::calc_costs()
{
	sint64 costs=0;

	// construct city road?
	const weg_besch_t *cityroad = welt->get_city_road();

	for(int i=0; i<=max_n; i++) {

		// cost for normal way (also build at bridge starts / tunnel entrances), calculates the cost of removing trees.
		const grund_t* gr = welt->lookup(route[i]);
		if(gr) {
			const weg_t *weg=gr->get_weg((waytype_t)besch->get_wtyp());
			// keep faster ways or if it is the same way ... (@author prissi)
			if(weg!=NULL  &&  (weg->get_besch()==besch  ||  (besch->get_styp()==0 && weg->get_besch()->get_styp()==7 && gr->get_weg_nr(0)!=weg) || keep_existing_ways  ||  (keep_existing_faster_ways  &&  weg->get_besch()->get_topspeed()>besch->get_topspeed()) || (gr->get_typ()==grund_t::monorailboden && (bautyp&elevated_flag)==0) ) ){
					//nothing to be done
			}
			else if(besch->get_wtyp()!=powerline_wt  ||  gr->get_leitung()==NULL) {
				costs += besch->get_preis();
				// eventually we have to remove trees
				for(  uint8 i=0;  i<gr->get_top();  i++  ) {
					ding_t *dt = gr->obj_bei(i);
					switch(dt->get_typ()) {
						case ding_t::baum:
							costs -= welt->get_einstellungen()->cst_remove_tree;
							break;
						case ding_t::groundobj:
							costs += ((groundobj_t *)dt)->get_besch()->get_preis();
							break;

						default: break;
					}
				}
			}
		}

		// last tile cannot be start of tunnel/bridge
		if(i<max_n) {
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
					costs += bruecke_besch->get_preis()*(koord_distance(route[i], route[i+1])+1);
					continue;
				}
				else {
					// tunnel
					costs += tunnel_besch->get_preis()*(koord_distance(route[i], route[i+1])+1);
					continue;
				}
			}
		}

		// check next tile
	}
	DBG_MESSAGE("wegbauer_t::calc_costs()","construction estimate: %f",costs/100.0);
	return costs;
}



// adds the ground before underground construction
bool
wegbauer_t::baue_tunnelboden()
{
	long cost = 0;
	for(int i=0; i<=max_n; i++) {

		grund_t* gr = welt->lookup(route[i]);

		const weg_besch_t *wb = tunnel_besch->get_weg_besch();
		if(wb==NULL) {
			// now we seach a matchin wy for the tunnels top speed
			wb = wegbauer_t::weg_search( tunnel_besch->get_waytype(), tunnel_besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat );
		}

		if(gr==NULL) {
			// make new tunnelboden
			tunnelboden_t* tunnel = new tunnelboden_t(welt, route[i], 0);
			weg_t *weg = weg_t::alloc(tunnel_besch->get_waytype());
			weg->set_besch( wb );
			welt->access(route[i].get_2d())->boden_hinzufuegen(tunnel);
			tunnel->neuen_weg_bauen(weg, calc_ribi(i), sp);
			tunnel->obj_add(new tunnel_t(welt, route[i], sp, tunnel_besch));
			weg->set_max_speed(tunnel_besch->get_topspeed());
			tunnel->calc_bild();
			cost -= tunnel_besch->get_preis();
			spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());
			spieler_t::add_maintenance( sp,  tunnel_besch->get_wartung() );
		}
		else if(gr->get_typ()==grund_t::tunnelboden) {
			// check for extension only ...
			gr->weg_erweitern( tunnel_besch->get_waytype(), calc_ribi(i) );
			weg_t *weg = gr->get_weg(tunnel_besch->get_waytype());
			// take the faster way
			if(  !keep_existing_faster_ways  ||  (weg->get_max_speed() < tunnel_besch->get_topspeed())  ) {
				tunnel_t *tunnel = gr->find<tunnel_t>();
				if( tunnel->get_besch() == tunnel_besch ) {
					continue;
				}
				spieler_t::add_maintenance(sp, -tunnel->get_besch()->get_wartung());
				spieler_t::add_maintenance(sp,  tunnel_besch->get_wartung() );

				tunnel->set_besch(tunnel_besch);
				weg->set_besch(wb);
				weg->set_max_speed(tunnel_besch->get_topspeed());
				gr->calc_bild();

				cost -= tunnel_besch->get_preis();
			}
		}
	}
	spieler_t::accounting(sp, cost, route[0].get_2d(), COST_CONSTRUCTION);
	return true;
}



void
wegbauer_t::baue_elevated()
{
	for(  sint32 i=0;  i<=max_n;  i++  ) {

		planquadrat_t* plan = welt->access(route[i].get_2d());

		grund_t* gr0= plan->get_boden_in_hoehe(route[i].z);
		route[i].z += Z_TILE_STEP;
		grund_t* gr = plan->get_boden_in_hoehe(route[i].z);

		if(gr==NULL) {
			hang_t::typ hang = gr0 ? gr0->get_grund_hang() : 0;
			// add new elevated ground
			monorailboden_t* monorail = new monorailboden_t(welt, route[i], hang);
			plan->boden_hinzufuegen(monorail);
			monorail->calc_bild();
		}
	}
}



void wegbauer_t::baue_strasse()
{
	// construct city road?
	const weg_besch_t *cityroad = get_besch("city_road");
	bool add_sidewalk = besch==cityroad;

	if(add_sidewalk) {
		sp = NULL;
	}

	// init undo
	if(sp!=NULL) {
		// intercity roads have no owner, so we must check for an owner
		sp->init_undo(road_wt,max_n);
	}

	for(  sint32 i=0;  i<=max_n;  i++  ) {
		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}

		const koord k = route[i].get_2d();
		grund_t* gr = welt->lookup(route[i]);
		sint64 cost = 0;

		bool extend = gr->weg_erweitern(road_wt, calc_ribi(i));

		// bridges/tunnels have their own track type and must not upgrade
		if(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden) {
			continue;
		}

		if(extend) {
			weg_t * weg = gr->get_weg(road_wt);

			// keep faster ways or if it is the same way ... (@author prissi)
			if(weg->get_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_city_roads  &&  weg->hat_gehweg())  ||  (keep_existing_faster_ways  &&  weg->get_besch()->get_topspeed()>besch->get_topspeed())  ||  (sp!=NULL  &&  !spieler_t::check_owner( sp, weg->get_besitzer())) || (gr->get_typ()==grund_t::monorailboden && (bautyp&elevated_flag)==0)) {
				//nothing to be done
//DBG_MESSAGE("wegbauer_t::baue_strasse()","nothing to do at (%i,%i)",k.x,k.y);
			}
			else {
				// we take ownership => we take care to maintain the roads completely ...
				spieler_t *s = weg->get_besitzer();
				spieler_t::add_maintenance(s, -weg->get_besch()->get_wartung());
				weg->set_besch(besch);
				spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung());
				weg->set_besitzer(sp);
				cost -= besch->get_preis();
			}
		}
		else {
			// make new way
			strasse_t * str = new strasse_t(welt);

			str->set_besch(besch);
			str->set_gehweg(add_sidewalk);
			cost = -gr->neuen_weg_bauen(str, calc_ribi(i), sp)-besch->get_preis();

			// prissi: into UNDO-list, so wie can remove it later
			if(sp!=NULL) {
				// intercity raods have no owner, so we must check for an owner
				sp->add_undo( route[i] );
			}
		}
		gr->calc_bild();	// because it may be a crossing ...
		reliefkarte_t::get_karte()->calc_map_pixel(k);
		spieler_t::accounting(sp, cost, k, COST_CONSTRUCTION);
	} // for
}



void wegbauer_t::baue_schiene()
{
	if(max_n >= 1) {
		// init undo
		sp->init_undo((waytype_t)besch->get_wtyp(),max_n);

		// built tracks
		for(  sint32 i=0;  i<=max_n;  i++  ) {
			sint64 cost = 0;
			grund_t* gr = welt->lookup(route[i]);
			ribi_t::ribi ribi = calc_ribi(i);

			if(gr->get_typ()==grund_t::wasser) {
				// not building on the sea ...
				continue;
			}

			bool extend = gr->weg_erweitern((waytype_t)besch->get_wtyp(), ribi);

			// bridges/tunnels have their own track type and must not upgrade
			if((gr->get_typ()==grund_t::brueckenboden ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->get_weg_nr(0)->get_waytype()==besch->get_wtyp()) {
				continue;
			}

			if(extend) {
				weg_t * weg = gr->get_weg((waytype_t)besch->get_wtyp());

				// keep faster ways or if it is the same way ... (@author prissi)
				if(weg->get_besch()==besch  ||  (besch->get_styp()==0 && weg->get_besch()->get_styp()==7 && gr->get_weg_nr(0)!=weg) || keep_existing_ways  ||  (keep_existing_faster_ways  &&  weg->get_besch()->get_topspeed()>besch->get_topspeed()) || (gr->get_typ()==grund_t::monorailboden && (bautyp&elevated_flag)==0) ) {
					//nothing to be done
					cost = 0;
				}
				else {
					// we take ownership => we take care to maintain the roads completely ...
					spieler_t *s = weg->get_besitzer();
					spieler_t::add_maintenance( s, -weg->get_besch()->get_wartung());
					weg->set_besch(besch);
					spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung());
					weg->set_besitzer(sp);
					cost -= besch->get_preis();
				}
			}
			else {
				weg_t *sch=weg_t::alloc((waytype_t)besch->get_wtyp());
				sch->set_besch(besch);
				if(besch->get_wtyp()==water_wt  &&  gr->get_hoehe()==welt->get_grundwasser()) {
					ribi = ribi_t::doppelt(ribi);
				}
				cost = -gr->neuen_weg_bauen(sch, ribi, sp)-besch->get_preis();

				// prissi: into UNDO-list, so wie can remove it later
				sp->add_undo( route[i] );
			}

			gr->calc_bild();
			reliefkarte_t::get_karte()->calc_map_pixel( gr->get_pos().get_2d() );
			spieler_t::accounting(sp, cost, gr->get_pos().get_2d(), COST_CONSTRUCTION);

			if((i&3)==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



void
wegbauer_t::baue_leitung()
{
	if(  max_n<=0  ) {
		return;
	}

	// no undo
	sp->init_undo(powerline_wt,max_n);

	for(  sint32 i=0;  i<=max_n;  i++  ) {
		grund_t* gr = welt->lookup(route[i]);

		leitung_t* lt = gr->get_leitung();
		// ok, really no lt here ...
		if(lt==NULL) {
			if(gr->ist_natur()) {
				// remove trees etc.
				gr->obj_loesche_alle(sp);
			}
			lt = new leitung_t( welt, route[i], sp );
			spieler_t::accounting(sp, -leitung_besch->get_preis(), gr->get_pos().get_2d(), COST_CONSTRUCTION);
			gr->obj_add(lt);

			// prissi: into UNDO-list, so wie can remove it later
			sp->add_undo( route[i] );
		}
		else {
			spieler_t::add_maintenance( lt->get_besitzer(),  -wegbauer_t::leitung_besch->get_wartung() );
		}
		lt->laden_abschliessen();

		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}
	}
}



// this can drive any river, even a river that has max_speed=0
class fluss_fahrer_t : fahrer_t
{
	bool ist_befahrbar(const grund_t* gr) const { return gr->get_weg_ribi_unmasked(water_wt)!=0; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_weg_ribi_unmasked(water_wt); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_kosten(const grund_t *,const uint32) const { return 1; }
	virtual bool ist_ziel(const grund_t *cur,const grund_t *) const { return cur->ist_wasser()  &&  cur->get_grund_hang()==hang_t::flach; }
};


// make a river
void
wegbauer_t::baue_fluss()
{
	/* since the contraits of the wayfinder ensures that a river flows always downwards
	 * we can assume that the first tiles are the ocean.
	 * Usually the wayfinder would find either direction!
	 */

	// Do we join an other river?
	sint32 start_n = 0;
	for(  sint32 idx=start_n;  idx<=max_n;  idx++  ) {
		if(  welt->lookup(route[idx])->hat_weg(water_wt)  ) {
			start_n = idx;
		}
	}
	if(  start_n>= max_n  ) {
		// completly joined another river => nothing to do
		return;
	}

	// first lower riverbed
	const sint8 start_h = route[start_n].z;
	for(  sint32 idx=start_n;  idx<=max_n;  idx++  ) {
		koord3d pos = route[idx];
		if(pos.z < start_h){
			// do not handle both joining and water ...
			continue;
		}
		if(  !welt->ebne_planquadrat( NULL, pos.get_2d(), max(pos.z-1, start_h) )  ) {
			dbg->message( "wegbauer_t::baue_fluss()","lowering tile %s failed.", pos.get_str() );
		}
	}

	// now build the river
	for(  sint32 i=start_n;  i<=max_n;  i++  ) {
		grund_t* gr = welt->lookup_kartenboden(route[i].get_2d());
		if(  gr->get_typ()!=grund_t::wasser  ) {
			// get direction
			ribi_t::ribi ribi = calc_ribi(i);
			bool extend = gr->weg_erweitern(water_wt, ribi);
			if(  !extend  ) {
				weg_t *sch=weg_t::alloc(water_wt);
				sch->set_besch(besch);
				gr->neuen_weg_bauen(sch, ribi, NULL);
			}
		}
	}

	// we will make rivers gradually larger by stepping up their width
	if(  umgebung_t::river_types>1  ) {
		/* since we will stop at the first crossing with an existent river,
		 * we cannot make sure, we have the same destination;
		 * thus we use the routefinder to find the sea
		 */
		route_t to_the_sea;
		fluss_fahrer_t ff;
		if(  to_the_sea.find_route( welt, welt->lookup_kartenboden(route[start_n].get_2d())->get_pos(), (fahrer_t *)&ff, 0, ribi_t::alle, 0x7FFFFFFF )  ) {
			for(  uint32 idx=0;  idx<to_the_sea.get_max_n();  idx++  ) {
				weg_t* w = welt->lookup(to_the_sea.get_route()[idx])->get_weg(water_wt);
				if(w) {
					int type;
					for(  type=umgebung_t::river_types-1;  type>0;  type--  ) {
						// llokup type
						if(  w->get_besch()==alle_wegtypen.get(umgebung_t::river_type[type])  ) {
							break;
						}
					}
					// still room to expand
					if(  type>0  ) {
						// thus we enlarge
						w->set_besch( alle_wegtypen.get(umgebung_t::river_type[type-1]) );
						w->calc_bild();
					}
				}
			}
		}
	}
}



void
wegbauer_t::baue()
{
	if(max_n<0  ||  max_n>(sint32)maximum) {
DBG_MESSAGE("wegbauer_t::baue()","called, but no valid route.");
		// no valid route here ...
		return;
	}
	DBG_MESSAGE("wegbauer_t::baue()", "type=%d max_n=%d start=%d,%d end=%d,%d", bautyp, max_n, route[0].x, route[0].y, route[max_n].x, route[max_n].y);

#ifdef DEBUG_ROUTES
long ms=dr_time();
#endif

	// first add all new underground tiles ... (and finished if sucessful)
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
 * welt->get_einstellungen()->get_way_count_{straight,slope}().
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
	dist *= welt->get_einstellungen()->way_count_straight;
	if( pos.z < mini.z ) {
		dist += (mini.z - pos.z) * welt->get_einstellungen()->way_count_slope;
	} else if( pos.z > maxi.z ) {
		dist += (pos.z - maxi.z) * welt->get_einstellungen()->way_count_slope;
	}
	return dist;
}
