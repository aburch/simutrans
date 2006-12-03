/*
 * simbau.cc
 *
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Strassen- und Schienenbau
 *
 * Hj. Malthaner
 */

#include "../simtime.h"	// testing speed
#include "../gui/messagebox.h"

#include "../simdebug.h"
#include "wegbauer.h"

#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/runway.h"
#include "../boden/brueckenboden.h"
#include "../boden/monorailboden.h"
#include "../boden/tunnelboden.h"
#include "../boden/grund.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/route.h"
// sorted heap, since we only need insert and pop
#include "../tpl/binary_heap_tpl.h" // fastest

#include "../simworld.h"
#include "../simwerkz.h"
#include "../simmesg.h"
#include "../simplay.h"
#include "../simplan.h"
#include "../simdepot.h"
//#include "../blockmanager.h"

#include "../dings/gebaeude.h"
#include "../dings/bruecke.h"
#include "../dings/tunnel.h"

#include "../simintr.h"

#include "hausbauer.h"
#include "tunnelbauer.h"
#include "brueckenbauer.h"

#include "../besch/weg_besch.h"
#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/spezial_obj_tpl.h"


#include "../tpl/stringhashtable_tpl.h"

#include "../dings/leitung2.h"

#include "../gui/karte.h"	// for debugging


// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"


// built bridges automatically
//#define AUTOMATIC_BRIDGES 1

slist_tpl<const kreuzung_besch_t *> wegbauer_t::kreuzungen;

const weg_besch_t *wegbauer_t::leitung_besch = NULL;

static spezial_obj_tpl<weg_besch_t> spezial_objekte[] = {
    { &wegbauer_t::leitung_besch, "Powerline" },
    { NULL, NULL }
};


static stringhashtable_tpl <const weg_besch_t *> alle_wegtypen;


bool wegbauer_t::alle_wege_geladen()
{
	// some defaults to avoid hardcoded values
	strasse_t::default_strasse = wegbauer_t::weg_search(road_wt,50,0,weg_t::type_flat);
	schiene_t::default_schiene = wegbauer_t::weg_search(track_wt,80,0,weg_t::type_flat);
	monorail_t::default_monorail = wegbauer_t::weg_search(monorail_wt,120,0,weg_t::type_flat);
	kanal_t::default_kanal = wegbauer_t::weg_search(water_wt,20,0,weg_t::type_flat);
	runway_t::default_runway = wegbauer_t::weg_search(air_wt,20,0,weg_t::type_flat);
	return ::alles_geladen(spezial_objekte + 1);
}


bool wegbauer_t::alle_kreuzungen_geladen()
{
	if(!gib_kreuzung(track_wt, road_wt) ||
	!gib_kreuzung(road_wt, track_wt))
	{
		ERROR("wegbauer_t::alles_geladen()", "at least one crossing not found");
		return false;
	}
	return true;
}


bool wegbauer_t::register_besch(const weg_besch_t *besch)
{
  DBG_DEBUG("wegbauer_t::register_besch()", besch->gib_name());
  alle_wegtypen.put(besch->gib_name(), besch);
  return ::register_besch(spezial_objekte, besch);
}


bool wegbauer_t::register_besch(const kreuzung_besch_t *besch)
{
    kreuzungen.append(besch);
    return true;
}





/**
 * Finds a way with a given speed limit for a given waytype
 * @author prissi
 */
const weg_besch_t *  wegbauer_t::weg_search(const waytype_t wtyp,const uint32 speed_limit,const uint16 time, const weg_t::system_type system_type)
{
	stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);

	const weg_besch_t * besch=NULL;
//DBG_MESSAGE("wegbauer_t::weg_search","Search cheapest for limit %i, year=%i, month=%i",speed_limit, time/12, time%12+1);
	while(iter.next()) {
		const weg_besch_t *test_weg = iter.get_current_value();


		if(  ( (test_weg->gib_wtyp()==wtyp  &&  system_type==system_type)
			||  (wtyp==tram_wt  &&  test_weg->gib_wtyp()==track_wt  &&  test_weg->gib_styp()==weg_t::type_tram)
			)  &&  iter.get_current_value()->gib_cursor()->gib_bild_nr(1) != IMG_LEER) {

			if(  besch==NULL  ||  (besch->gib_topspeed()<speed_limit  &&  besch->gib_topspeed()<iter.get_current_value()->gib_topspeed()) ||  (iter.get_current_value()->gib_topspeed()>=speed_limit  &&  iter.get_current_value()->gib_wartung()<besch->gib_wartung())  ) {
				if(time==0  ||  (test_weg->get_intro_year_month()<=time  &&  test_weg->get_retire_year_month()>time)) {
					besch = test_weg;
//DBG_MESSAGE("wegbauer_t::weg_search","Found weg %s, limit %i",besch->gib_name(),besch->gib_topspeed());
				}
			}
		}
	}
	return besch;
}



/**
 * Tries to look up description for way, described by way type,
 * system type and construction type
 * @author Hj. Malthaner
 */
const weg_besch_t * wegbauer_t::gib_besch(const char * way_name,const uint16 time)
{
//DBG_MESSAGE("wegbauer_t::gib_besch","return besch for %s in (%i)",way_name, time/12);
	const weg_besch_t *besch = alle_wegtypen.get(way_name);
	if(time==0  ||  besch==NULL  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
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
					translator::translate("way %s now available\n"),
					translator::translate(besch->gib_name()));
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->gib_bild_nr(5,0));
			}

			const uint16 retire_month =besch->get_retire_year_month();
			if(retire_month == current_month) {
				sprintf(buf,
					translator::translate("way %s cannot longer used:\n"),
					translator::translate(besch->gib_name()));
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->gib_bild_nr(5,0));
			}
		}

	}
}



const kreuzung_besch_t *wegbauer_t::gib_kreuzung(const waytype_t ns, const waytype_t ow)
{
	slist_iterator_tpl<const kreuzung_besch_t *> iter(kreuzungen);

	while(iter.next()) {
		if(iter.get_current()->gib_wegtyp_ns()==ns  &&  iter.get_current()->gib_wegtyp_ow()==ow) {
			return iter.get_current();
		}
	}
	return NULL;
}


/**
 * Fill menu with icons of given waytype, return number of added entries
 * @author Hj. Malthaner/prissi/dariok
 */
void wegbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	const waytype_t wtyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	const int sound_ok,
	const int sound_ko,
	const uint16 time,
	const weg_t::system_type styp)
{
	// list of matching types (sorted by speed)
	slist_tpl <const weg_besch_t *> matching;

	stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);
	while(iter.next()) {
		const weg_besch_t * besch = iter.get_current_value();

		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			if(besch->gib_styp()==styp) {
				 // DarioK: load only if the given styp maches!
				if(besch->gib_wtyp()==wtyp &&  besch->gib_cursor()->gib_bild_nr(1)!=IMG_LEER) {

					// this should avoid tram-tracks to be loaded into rail-menu
					unsigned i;
					for( i=0;  i<matching.count();  i++  ) {
						// insert sorted
						if(matching.at(i)->gib_topspeed()>besch->gib_topspeed()) {
							matching.insert(besch,i);
							break;
						}
					}
					if(i==matching.count()) {
						matching.append(besch);
					}

				}
			}
		}
	}

	const sint32 shift_maintanance = (karte_t::ticks_bits_per_tag-18);
	// now sorted ...
	while (!matching.empty()) {
		const weg_besch_t* besch = matching.front();
		matching.remove_at(0);

		char buf[128];

		sprintf(buf, "%s, %ld$ (%ld$), %dkm/h",
			translator::translate(besch->gib_name()),
			besch->gib_preis()/100l,
			(besch->gib_wartung()<<shift_maintanance)/100l,
			besch->gib_topspeed());

		wzw->add_param_tool(werkzeug,
			(const void *)besch,
			karte_t::Z_PLAN,
			sound_ok,
			sound_ko,
			besch->gib_cursor()->gib_bild_nr(1),
			besch->gib_cursor()->gib_bild_nr(0),
			buf );
	}
}




/* allow for railroad crossing
 * @author prissi
 */
bool
wegbauer_t::check_crossing(const koord zv, const grund_t *bd,waytype_t wtyp) const
{
  if(bd->hat_weg(wtyp)  &&  bd->gib_halt()==NULL) {
	ribi_t::ribi w_ribi = bd->gib_weg_ribi_unmasked(wtyp);
    // it is our way we want to cross: can we built a crossing here?
    // both ways must be straight and no ends
    return
      ribi_t::ist_gerade(w_ribi)
      &&  !ribi_t::ist_einfach(w_ribi)
      &&  ribi_t::ist_gerade(ribi_typ(zv))
      &&  (w_ribi&ribi_typ(zv))==0;
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
	leitung_t *lt=dynamic_cast<leitung_t *> (bd->suche_obj(ding_t::leitung));
	if(lt!=NULL) {//  &&  (bd->gib_besitzer()==0  ||  bd->gib_besitzer()==sp)) {
		ribi_t::ribi lt_ribi = lt->gib_ribi();
	    // it is our way we want to cross: can we built a crossing here?
	    // both ways must be straight and no ends
	    return
	      ribi_t::ist_gerade(lt_ribi)
	      &&  !ribi_t::ist_einfach(lt_ribi)
	      &&  ribi_t::ist_gerade(ribi_typ(zv))
	      &&  (lt_ribi&ribi_typ(zv))==0;
	}
	// check for transformer
	if(bd->suche_obj(ding_t::pumpe)!=NULL  ||  bd->suche_obj(ding_t::senke)!=NULL) {
		return false;
	}
	// ok, there is not high power transmission stuff going on here
	return true;
}



// allowed slope? (not used ... since get_neightbour and some further check does this even better)
bool wegbauer_t::check_slope( const grund_t *from, const grund_t *to )
{
	const koord from_pos=from->gib_pos().gib_2d();
	const koord to_pos=to->gib_pos().gib_2d();
	const sint8 ribi=ribi_typ( from_pos, to_pos );

	if(from==to) {
		// this may happen, if the starting position is tested!
		return hang_t::ist_wegbar(from->gib_grund_hang());
	}

	// check for valid slopes
	if(from->gib_weg_hang()!=0) {
		if(!hang_t::ist_wegbar(from->gib_weg_hang())) {
			// not valid
			return false;
		}
		const sint8 hang_ribi=ribi_typ(from->gib_weg_hang());
		if(ribi!=hang_ribi  &&  ribi!=ribi_t::rueckwaerts(hang_ribi)) {
			// not down or up ...
			return false;
		}
	}
	// ok, now check destination hang
	if(to->gib_weg_hang()!=0) {
		if(!hang_t::ist_wegbar(to->gib_weg_hang())) {
			// not valid
			return false;
		}
		const sint8 hang_ribi=ribi_typ(to->gib_weg_hang());
		if(ribi!=hang_ribi  &&  ribi!=ribi_t::rueckwaerts(hang_ribi)) {
			// not down or up ...
			return false;
		}
	}

	// now check offsets before changing the slope ...
	const sint8 slope_this = from->gib_weg_hang();
	const sint16 hgt=from->gib_hoehe()/Z_TILE_STEP;
	const sint16 to_hgt=to->gib_hoehe()/Z_TILE_STEP;
	const sint8 to_slope=to->gib_weg_hang();

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
bool wegbauer_t::check_owner( const spieler_t *sp1, const spieler_t *sp2 )
{
	// unowned, mine or public property?
	return sp1==NULL  ||  sp1==sp2  ||  sp1==welt->gib_spieler(1);
}



/* This is the core routine for the way search
 * it will check
 * A) allowed step
 * B) if allowed, calculate the cost for the step from from to to
 * @author prissi
 */
bool wegbauer_t::is_allowed_step( const grund_t *from, const grund_t *to, long *costs )
{
	const koord from_pos=from->gib_pos().gib_2d();
	const koord to_pos=to->gib_pos().gib_2d();
	const koord zv=to_pos-from_pos;

	if(bautyp==luft  &&  (from->gib_grund_hang()+to->gib_grund_hang()!=0  ||  (from->hat_wege()  &&  from->hat_weg(air_wt)==0))) {
		// absolutely no slopes for runways, neither other ways
		return false;
	}

	if(from==to) {
		if(!hang_t::ist_wegbar(from->gib_weg_hang())) {
//			DBG_MESSAGE("wrong slopes at","%i,%i ribi1=%d",from_pos.x,from_pos.y,ribi_typ(from->gib_weg_hang()));
			return false;
		}
	}
	else {
		if(from->gib_weg_hang()  &&  ribi_t::doppelt(ribi_typ(from->gib_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
//			DBG_MESSAGE("wrong slopes between","%i,%i and %i,%i, ribi1=%d, ribi2=%d",from_pos.x,from_pos.y,to_pos.x,to_pos.y,ribi_typ(from->gib_weg_hang()),ribi_typ(zv));
			return false;
		}
		if(to->gib_weg_hang()  &&  ribi_t::doppelt(ribi_typ(to->gib_weg_hang()))!=ribi_t::doppelt(ribi_typ(zv))) {
//			DBG_MESSAGE("wrong slopes between","%i,%i and %i,%i, ribi1=%d, ribi2=%d",from_pos.x,from_pos.y,to_pos.x,to_pos.y,ribi_typ(to->gib_weg_hang()),ribi_typ(zv));
			return false;
		}
	}

	// ok, slopes are ok
	bool ok = to->ist_natur()  &&  !to->ist_wasser();
	bool fundament= to->gib_typ()==grund_t::fundament;
	const gebaeude_t *gb=dynamic_cast<const gebaeude_t *>(to->suche_obj(ding_t::gebaeude));

	// no crossings to halt
	if(to!=from  &&  bautyp!=leitung  &&  (bautyp&elevated_flag)==0) {
		static koord gb_to_zv[4] = { koord::sued, koord::ost, koord::nord, koord::west };
		if(gb  &&  (gb->gib_besitzer()==sp  ||  to->gib_halt().is_bound())) {

			// terminal imposes stronger direction checks
			if(gb->gib_tile()->gib_besch()->gib_all_layouts()==4) {
				if(zv!=gb_to_zv[(gb->gib_tile()->gib_layout()+2)&3]) {
//DBG_MESSAGE("cannot go","from %i,%i, to %i, %i due to 4 way stop",from_pos.x,from_pos.y,to_pos.x,to_pos.y);
					return false;
				}
			}
			else {
				// through station
				if( !ribi_t::ist_gerade( ribi_typ(zv)|ribi_typ(gb_to_zv[gb->gib_tile()->gib_layout()]) ) ) {
					return false;
				}
			}
		}

		// no crossings from halt
		const gebaeude_t *from_gb=dynamic_cast<const gebaeude_t *>(from->suche_obj(ding_t::gebaeude));
		if(from_gb  &&  (from_gb->gib_besitzer()==sp  ||  from->gib_halt().is_bound())) {
			// terminal imposes stronger direction checks
			if(from_gb->gib_tile()->gib_besch()->gib_all_layouts()==4) {
				if(zv!=gb_to_zv[from_gb->gib_tile()->gib_layout()]) {
//DBG_MESSAGE("cannot go","from %i,%i, to %i, %i due to 4 way stop",from_pos.x,from_pos.y,to_pos.x,to_pos.y);
					return false;
				}
			}
			else {
				// through station
				if( !ribi_t::ist_gerade( ribi_typ(zv)|ribi_typ(gb_to_zv[from_gb->gib_tile()->gib_layout()]) ) ) {
					return false;
				}
			}
		}
	}

	// universal check for elevated things ...
	if(bautyp&elevated_flag) {
		if(to->hat_weg(air_wt)  ||  to->ist_wasser()  ||  !check_for_leitung(zv,to)  ||  !to->ist_karten_boden()  ||  to->gib_typ()==grund_t::brueckenboden  ||  to->gib_typ()==grund_t::tunnelboden) {
			// no suitable ground below!
			return false;
		}
		sint16 height = welt->lookup(to->gib_pos().gib_2d())->gib_kartenboden()->gib_hoehe()+Z_TILE_STEP;
		grund_t *to2 = welt->lookup(koord3d(to->gib_pos().gib_2d(),height));
		if(to2) {
			ok = to2->gib_typ()==grund_t::monorailboden  &&  check_owner(to2->gib_besitzer(),sp);
			ok &= to2->gib_weg_nr(0)==NULL  ||  to2->gib_weg_nr(0)->gib_besch()->gib_wtyp()==besch->gib_wtyp();
			if(!ok) {
DBG_MESSAGE("wegbauer_t::is_allowed_step()","wrong ground already there!");
				return false;
			}
			// now we have a halt => check for direction
			if(to2->gib_halt().is_bound()  &&  !ribi_t::ist_gerade(ribi_typ(zv)|to->gib_weg_ribi_unmasked((waytype_t)besch->gib_wtyp()))  ) {
				// no crossings on stops
				return false;
			}
		}
		if(gb) {
			// no halt => citybuilding => do not touch
			if(!check_owner(gb->gib_besitzer(),sp)  ||  gb->gib_tile()->gib_hintergrund(0,1,0)!=IMG_LEER) {  // also check for too high buildings ...
				return false;
			}
			// building above houses is expensive ... avoid it!
			*costs += 4;
		}
		ok = true;
	}

	// not jumping to other lanes on bridges or tunnels
	if(to->gib_typ()==grund_t::brueckenboden  ||  to->gib_typ()==grund_t::tunnelboden) {
		weg_t *weg=to->gib_weg_nr(0);
		if(!ribi_t::ist_gerade(weg->gib_ribi_unmasked()|ribi_typ(zv))) {
			return false;
		}
	}

	// no check way specific stuff
	switch(bautyp&bautyp_mask) {

		case strasse:
		{
			const weg_t *str=to->gib_weg(road_wt);
			const weg_t *sch=to->gib_weg(track_wt);
			ok =	(str  ||  !fundament)  &&  ((!to->hat_wege()  ||  sch==NULL)  ||  (check_crossing(zv,to,track_wt)  ||  sch->gib_besch()->gib_styp()==7)) &&
					!to->ist_wasser()  &&  (str!=NULL  ||  check_owner(to->gib_besitzer(),sp))  &&
					check_for_leitung(zv,to);
			if(ok) {
				const weg_t *from_str=from->gib_weg(road_wt);
				// check for end/start of bridge
				if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (from_str==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|from_str->gib_ribi_unmasked()))) {
					return false;
				}
				// do not go through depots ...
				if(to->gib_depot()  &&  (str==NULL  ||  ribi_typ(to_pos,from_pos)!=str->gib_ribi_unmasked())) {
					return false;
				}
				if(from->gib_depot()) {
					return false;
				}
				// calculate costs
				*costs = str ? 0 : 5;
				*costs += from_str ? 0 : 5;
				if(to->hat_weg(track_wt)) {
					*costs += 4;	// avoid crossings
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case schiene:
		{
			const weg_t *sch=to->gib_weg(track_wt);
			// extra check for AI construction (not adding to existing tracks!)
			if((bautyp&bot_flag)!=0  &&  (gb  ||  sch  ||  to->gib_halt().is_bound())) {
				return false;
			}
			ok =	!fundament  &&  !to->ist_wasser()  &&  (!to->hat_wege()  ||  sch  ||  check_crossing(zv,to,road_wt))  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to);
			if(ok) {
				// check for end/start of bridge
				if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (sch==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|sch->gib_ribi_unmasked()))) {
					return false;
				}
				// do not go through depots ...
				if(to->gib_depot()) {
					if(sch==NULL  ||  ribi_typ(to_pos,from_pos)!=sch->gib_ribi_unmasked()) {
						return false;
					}
				}
				if(from->gib_depot()) {
					return false;
				}
				if(gb  &&  (bautyp&elevated_flag)==0) {
					// no halt => citybuilding => do not touch
					if(sch==NULL  ||  !check_owner(gb->gib_besitzer(),sp)) {
						return false;
					}
					// now we have a halt => check for direction
					if(sch  &&  ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->gib_ribi_unmasked())) {
						// no crossings on stops
						return false;
					}
				}
				// calculate costs
				*costs = sch ? 3 : 4;	// only prefer existing rails a little
				if(to->hat_weg(road_wt)) {
					*costs += 4;	// avoid crossings
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		// like tram, but checks for bridges too
		// will not connect to any other ways
		case monorail:
		default:
		{
			const weg_t *sch=to->gib_weg((waytype_t)besch->gib_wtyp());
			// extra check for AI construction (not adding to existing tracks!)
			if(bautyp&bot_flag  &&  (gb  ||  sch  ||  to->gib_halt().is_bound())) {
				return false;
			}
			if((bautyp&elevated_flag)==0) {
				// classical monorail
				ok =	!to->ist_wasser()  &&  !fundament  &&
					(to->hat_wege()==0  ||  sch)  &&  (from->hat_wege()==0  ||  from->hat_weg((waytype_t)besch->gib_wtyp()))
					&&  check_owner(to->gib_besitzer(),sp) && check_for_leitung(zv,to)  && !to->gib_depot();
				// check for end/start of bridge
				if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (sch==NULL  ||  ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->gib_ribi_unmasked()))) {
					return false;
				}
				// do not go through depots ...
				if(to->gib_depot()) {
					if(sch==NULL  ||  ribi_typ(to_pos,from_pos)!=sch->gib_ribi_unmasked()) {
						return false;
					}
				}
				if(from->gib_depot()) {
					return false;
				}
				if(gb  &&  (bautyp&elevated_flag)==0) {
					// no halt => citybuilding => do not touch
					if(sch==NULL  ||  !check_owner(gb->gib_besitzer(),sp)) {
						return false;
					}
					// now we have a halt => check for direction
					if(ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->gib_ribi_unmasked())  ) {
						// no crossings on stops
						return false;
					}
				}
			}
			// calculate costs
			if(ok) {
				*costs = 4;
				// perfer ontop of ways
				if(to->hat_wege()) {
					*costs = 3;
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case schiene_tram: // Dario: Tramway
		{
			ok =	(ok || to->hat_weg(track_wt)  || to->hat_weg(road_wt)) &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to)  &&
					!to->gib_depot();
			// missing: check for crossings on halt!

			// calculate costs
			if(ok) {
				*costs = to->hat_weg(track_wt) ? 2 : 4;	// only prefer existing rails a little
				// perfer own track
				if(to->hat_weg(road_wt)) {
					*costs += 2;
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case leitung:
			ok = !fundament &&  !to->ist_wasser()  &&  to->gib_weg(air_wt)==NULL;
			if(to->gib_weg_nr(0)!=NULL) {
				// only 90 deg crossings, only a signle way
				ribi_t::ribi w_ribi= to->gib_weg_nr(0)->gib_ribi_unmasked();
				ok &= ribi_t::ist_gerade(w_ribi)  &&  !ribi_t::ist_einfach(w_ribi)  &&  ribi_t::ist_gerade(ribi_typ(zv))  &&  (w_ribi&ribi_typ(zv))==0;
			}
			if(to->gib_weg_nr(1)!=NULL) {
				// only 90 deg crossings, only for trams ...
				ribi_t::ribi w_ribi= to->gib_weg_nr(1)->gib_ribi_unmasked();
				ok &= ribi_t::ist_gerade(w_ribi)  &&  !ribi_t::ist_einfach(w_ribi)  &&  ribi_t::ist_gerade(ribi_typ(zv))  &&  (w_ribi&ribi_typ(zv))==0;
			}
			// no bridges and monorails here in the air
			ok &= welt->lookup(to_pos)->gib_boden_in_hoehe(to->gib_pos().z+Z_TILE_STEP)==NULL;
			// calculate costs
			if(ok) {
				*costs = to->hat_wege() ? 8 : 1;
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		break;

		case wasser:
			ok = (ok  ||  to->ist_wasser()  ||  to->hat_weg(water_wt))  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to);
			// calculate costs
			if(ok) {
				*costs = to->ist_wasser()  ||  to->hat_weg(water_wt) ? 2 : 10;	// prefer water very much ...
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope*2;
				}
			}
			break;

	case luft: // hsiegeln: runway
		ok = !to->ist_wasser()  &&  (to->hat_weg(air_wt)  || !to->hat_wege())  &&
				  to->suche_obj(ding_t::leitung)==NULL   &&  !to->gib_depot()  &&  !fundament;
			// calculate costs
		*costs = 4;
		break;
	}
	return ok;
}


void wegbauer_t::check_for_bridge(const grund_t* parent_from, const grund_t* from, koord3d ziel)
{
	// bridge not enabled or wrong starting slope or tile already occupied with a way ...
	if (!hang_t::ist_wegbar(from->gib_grund_hang())) return;

	// since we were allowed to go there, it is ok ...
	if(from->gib_grund_hang()!=from->gib_weg_hang()) {
//		DBG_MESSAGE("wegbauer_t::check_for_bridge()","has bridge starting");
		// now find the end ...
		grund_t *gr=welt->lookup(from->gib_pos());
		grund_t *to=gr;
		waytype_t wegtyp=(waytype_t)from->gib_weg_nr(0)->gib_besch()->gib_wtyp();
		koord zv = koord::invalid;
		if(from->ist_karten_boden()) {
			// Der Grund ist Brückenanfang/-ende - hier darf nur in
			// eine Richtung getestet werden.
			if(from->gib_grund_hang() != hang_t::flach) {
				zv = koord(hang_t::gegenueber(from->gib_grund_hang()));
			}
			else {
				zv = koord(from->gib_weg_hang());
			}
		}
		while(gr) {
			gr->get_neighbour(to, wegtyp, zv);
			if(to  &&  to->ist_karten_boden()) {
				next_gr_t ngt={to,7};
				next_gr.append(ngt,16);
				return;
			}
			gr = to;
		}
		return;
	}

	if(parent_from==NULL  ||  bruecke_besch==NULL  ||  (from->gib_grund_hang()==0  &&  from->hat_wege())) {
		return;
	}

	const koord zv=from->gib_pos().gib_2d()-parent_from->gib_pos().gib_2d();
	const koord to_pos=from->gib_pos().gib_2d()+zv;
	bool has_reason_for_bridge=false;

	// ok, so now we do a closer investigation
	grund_t *gr, *gr2;
	long internal_cost;
	const long cost_difference=besch->gib_wartung()>0 ? (bruecke_besch->gib_wartung()*4l+3l)/besch->gib_wartung() : 16;

	if(from->gib_grund_hang()==0) {
		// try bridge on even ground
		int max_lenght=bruecke_besch->gib_max_length()>0 ? bruecke_besch->gib_max_length()-2 : umgebung_t::way_max_bridge_len;
		max_lenght = min( max_lenght, umgebung_t::way_max_bridge_len );

		for(int i=0;  i<max_lenght;  i++ ) {
			// not on map or already something there => fail
			if (!welt->ist_in_kartengrenzen(to_pos + zv * (i + 1))) return;
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			if (gr2->gib_pos() == ziel) return;
			// something in the way?
			if (welt->lookup(from->gib_pos() + zv * i + koord3d(0, 0, Z_TILE_STEP))) return;
			// powerline in the way?
			if (gr->gib_pos().z == from->gib_pos().z && gr->suche_obj(ding_t::leitung)) return;
			// some artificial tiles here?!?
			if (gr->gib_pos().z > from->gib_pos().z) return;
			// check for runways ...
			if (gr->hat_weg(air_wt)) return;
			// check, if we need really a bridge
			if(i<=2  &&  !has_reason_for_bridge) {
				long dummy;
				has_reason_for_bridge = !is_allowed_step(gr, gr2, &dummy );
			}
			// non-manable slope?
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
#ifndef DOUBLE_GROUNDS
				if (!hang_t::ist_wegbar(gr->gib_grund_hang()) || ribi_typ(zv) != ribi_typ(gr->gib_grund_hang())) return;
#else
				if (hang_t::ist_wegbar(gr->gib_grund_hang()) || !hang_t::ist_einfach(gr->gib_grund_hang()) || ribi_typ(zv) != ribi_typ(gr->gib_grund_hang())) return;
#endif
			}
			// make sure, the bridge is not too short
			if(i==0) {
				continue;
			}
			// here is a hang
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
				// ok, qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					// we return the koord3d AFTER we came down!
					if(has_reason_for_bridge) {
						next_gr_t ngt={gr,i*cost_difference+umgebung_t::way_count_slope};
						next_gr.append(ngt,16);
					}
					return;
				}
			}
			// check for flat landing
			if(gr->gib_pos().z==from->gib_pos().z) {
				// ok, height qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  !gr2->hat_wege()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					// we return the koord3d AFTER we came down!
					if(has_reason_for_bridge) {
						next_gr_t ngt={gr,i*cost_difference+umgebung_t::way_count_slope*2};
						next_gr.append(ngt,16);
						return;
					}
				}
			}
		}
		return;
	}

	// downhill hang ...
	if(ribi_typ(from->gib_grund_hang())==ribi_t::rueckwaerts(ribi_typ(zv))) {
		// try bridge

		int max_lenght=bruecke_besch->gib_max_length()>0 ? bruecke_besch->gib_max_length()-2 : umgebung_t::way_max_bridge_len;
		max_lenght = min( max_lenght, umgebung_t::way_max_bridge_len );

		for(int i=1;  i<max_lenght;  i++ ) {
			// not on map or already something there => fail
			if (!welt->ist_in_kartengrenzen(to_pos + zv * (i + 1))) return;
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			// must leave one field empty ...
			if (gr2->gib_pos() == ziel) return;
			// something in the way?
			if (welt->lookup(from->gib_pos() + zv * i)) return;
			// powerline in the way?
			if (gr->gib_pos().z == from->gib_pos().z + Z_TILE_STEP && gr->suche_obj(ding_t::leitung)) return;
			// some articial tiles here?!?
			if (gr->gib_pos().z > from->gib_pos().z) return;
			// check for runways ...
			if (gr->hat_weg(air_wt)) return;
			// non-manable slope?
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
#ifndef DOUBLE_GROUNDS
				if (!hang_t::ist_wegbar(gr->gib_grund_hang()) || ribi_typ(zv) != ribi_typ(gr->gib_grund_hang())) return;
#else
				if (hang_t::ist_wegbar(gr->gib_grund_hang()) || !hang_t::ist_einfach(gr->gib_grund_hang()) || ribi_typ(zv) != ribi_typ(gr->gib_grund_hang())) return;
#endif
			}
			// make sure, the bridge is not too short
			if(i==1) {
				continue;
			}
			// here is the uphill hang
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
				// ok, qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					// we return the koord3d AFTER we came down!
					// this make always sense, since it is a counter slope
					next_gr_t ngt={gr,i*cost_difference};
					next_gr.append(ngt,16);
					return;
				}
			}
			// check, if we need really a bridge
			if(!has_reason_for_bridge) {
				long dummy;
				has_reason_for_bridge = !is_allowed_step(gr, gr2, &dummy );
			}
			// check for flat landing
			if(gr->gib_pos().z==from->gib_pos().z  &&  has_reason_for_bridge) {
				// ok, height qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  !gr2->hat_wege()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					if(has_reason_for_bridge) {
						next_gr_t ngt={gr,i*cost_difference+umgebung_t::way_count_slope};
						next_gr.append(ngt,16);
					}
				}
			}
		}
		return;
	}

	// uphill hang ... may be tunnel?
	if(ribi_typ(from->gib_grund_hang())==ribi_typ(zv)) {

		for(unsigned i=0;  i<(unsigned)umgebung_t::way_max_bridge_len;  i++ ) {
			// not on map or already something there => fail
			if (!welt->ist_in_kartengrenzen(to_pos + zv * (i + 1))) return;
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			// here is a hang
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
				if(ribi_t::rueckwaerts(ribi_typ(zv))!=ribi_typ(gr->gib_grund_hang())  ||  !hang_t::ist_einfach(gr->gib_grund_hang())) {
					// non, manable slope
					return;
				}
				// ok, qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					// we return the koord3d AFTER we came down!
					next_gr_t ngt={gr,i*umgebung_t::way_count_tunnel };
					next_gr.append(ngt,16);
					return;
				}
			}
		}
	}
}


wegbauer_t::wegbauer_t(karte_t* wl, spieler_t* spl) : next_gr(32), route(0)
{
	n      = 0;
	max_n  = -1;
	baubaer= false;
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
#if AUTOMATIC_BRIDGES
  else if(bruecke_besch==NULL) {
  	bruecke_besch = brueckenbauer_t::find_bridge((waytype_t)b->gib_wtyp(),25,0);
  }
#endif
  DBG_MESSAGE("wegbauer_t::route_fuer()",
         "setting way type to %d, besch=%s, bruecke_besch=%s",
         bautyp,
         besch ? besch->gib_name() : "NULL",
         bruecke_besch ? bruecke_besch->gib_name() : "NULL"
         );
}



koord *
get_next_koord(koord gr_pos, koord ziel)
{
	static koord next_koord[4];
	if( abs(gr_pos.x-ziel.x)>abs(gr_pos.y-ziel.y) ) {
		next_koord[0] = (ziel.x>gr_pos.x) ? koord::ost : koord::west;
		next_koord[1] = (ziel.y>gr_pos.y) ? koord::sued : koord::nord;
	}
	else {
		next_koord[0] = (ziel.y>gr_pos.y) ? koord::sued : koord::nord;
		next_koord[1] = (ziel.x>gr_pos.x) ? koord::ost : koord::west;
	}
	next_koord[2] = koord(  -next_koord[1].x, -next_koord[1].y );
	next_koord[3] = koord(  -next_koord[0].x, -next_koord[0].y );
	return next_koord;
}



/* this routine uses A* to calculate the best route
 * beware: change the cost and you will mess up the system!
 * (but you can try, look at simuconf.tab)
 */
long
wegbauer_t::intern_calc_route(const koord3d start3d, const koord3d ziel3d)
{
	// we assume fail
	max_n = -1;

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// check for existing koordinates
	if(welt->lookup(start3d)==NULL  ||  welt->lookup(ziel3d)==NULL) {
		return -1;
	}

	// some thing for the search
	const grund_t *gr;
	grund_t *to;

	koord3d gr_pos;	// just the last valid pos ...

	// is valid ground?
	long dummy;
	gr = welt->lookup(start3d);
	if(!is_allowed_step(gr,gr,&dummy)) {
		DBG_MESSAGE("wegbauer_t::intern_calc_route()","cannot start on (%i,%i,%i)",start3d.x,start3d.y,start3d.z);
		return false;
	}

	// memory in static list ...
	if(route_t::nodes==NULL) {
		route_t::MAX_STEP = umgebung_t::max_route_steps;	// may need very much memory => configurable
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

	// get exclusively the tile list
	route_t::GET_NODE();

	uint32 step = 0;
	route_t::ANode *tmp = &(route_t::nodes[step]);
	step ++;

	tmp->parent = NULL;
	tmp->gr = welt->lookup(start3d);
	tmp->f = route_t::calc_distance(start3d,ziel3d);
	tmp->g = 0;
	tmp->dir = 0;
	tmp->count = 0;

	// clear the queue (should be empty anyhow)
	queue.clear();
	queue.insert(tmp);

	INT_CHECK("wegbauer 347");

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
		gr_pos = gr->gib_pos();

#ifdef DEBUG_ROUTES
DBG_DEBUG("insert to close","(%i,%i,%i)  f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);
#endif

		// already there
		if(gr_pos==ziel3d  ||  tmp->g>maximum) {
			// we added a target to the closed list: we are finished
			break;
		}

		// the four possible directions plus any additional stuff due to already existing brides plus new ones ...
		next_gr.clear();

		// only one direction allowed ...
		const koord bridge_nsow=tmp->parent!=NULL ? gr->gib_pos().gib_2d()-tmp->parent->gr->gib_pos().gib_2d() : koord::invalid;

		const koord *next_koord = get_next_koord(gr->gib_pos().gib_2d(),ziel3d.gib_2d());

		// testing all four possible directions
		for(int r=0; r<4; r++) {

			to = NULL;
			if(!gr->get_neighbour(to,invalid_wt,next_koord[r])) {
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
					const koord parent_pos=tmp->parent->gr->gib_pos().gib_2d();
					const koord to_pos=to->gib_pos().gib_2d();
					// distance>1
					if(abs(parent_pos.x-gr_pos.x)>1  ||   abs(parent_pos.y-gr_pos.y)>1) {
						if(ribi_typ(parent_pos,to_pos)!=ribi_typ(gr_pos.gib_2d(),to_pos)) {
							// not a straight line
							continue;
						}
					}
				}
				// now add it to the array ...
				next_gr_t nt={to,new_cost};
				next_gr.append(nt);
			}
			else if(tmp->parent!=NULL  &&  !gr->hat_wege()  &&  bridge_nsow==koord::nsow[r]) {
				// try to build a bridge or tunnel here, since we cannot go here ...
				check_for_bridge(tmp->parent->gr,gr,ziel3d);
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
				current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				if(tmp->dir!=current_dir) {
					new_g += umgebung_t::way_count_curve;
					if(tmp->parent->dir!=tmp->dir) {
						// discourage double turns
						new_g += umgebung_t::way_count_double_curve;
					}
					else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
						// discourage v turns heavily
						new_g += umgebung_t::way_count_90_curve;
					}
				}
			}
			else {
				 current_dir = ribi_typ( gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
			}

			const uint32 new_dist = route_t::calc_distance( to->gib_pos(), ziel3d );

			if(new_dist<min_dist) {
				min_dist = new_dist;
			}
			else if(new_dist>min_dist+50) {
				// skip, if too far from current minimum tile
				// will not find some ways, but will be much faster ...
				// also it will avoid too big detours, which is probably also not, way the builder intended
				continue;
			}


			const uint32 new_f = new_g+new_dist;

			if((step&0x03)==0) {
				INT_CHECK( "wegbauer 1347" );
#ifdef DEBUG_ROUTES
				if((step&1023)==0) {reliefkarte_t::gib_karte()->calc_map();}
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
DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z,k->f);
#endif
		}

	} while (!queue.empty() && step < route_t::MAX_STEP && gr->gib_pos() != ziel3d);

#ifdef DEBUG_ROUTES
DBG_DEBUG("wegbauer_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u",step,route_t::MAX_STEP,queue.count(),tmp->g);
#endif
	INT_CHECK("wegbauer 194");

	route_t::RELEASE_NODE();

//DBG_DEBUG("reached","%i,%i",tmp->pos.x,tmp->pos.y);
	// target reached?
	if(gr->gib_pos()!=ziel3d  || step >=route_t::MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("wegbauer_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,route_t::MAX_STEP);
		return -1;
	}
	else {
		const long cost = tmp->g;
		// reached => construct route
		while(tmp != NULL) {
			route.append(tmp->gr->gib_pos(), 256);
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

	if(welt->lookup(start)==NULL) {
		// not building
		return;
	}
	long dummy_cost;
	grund_t *test_bd = welt->lookup(start);
	if(!is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// no legal ground to start ...
		return;
	}
	test_bd = welt->lookup(ziel);
	if((bautyp&tunnel_flag)==0  &&  test_bd  &&  !is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// ... or to end
		return;
	}

	koord pos=start.gib_2d();
	while(pos!=ziel.gib_2d()  &&  ok) {

		// shortest way
		koord diff;
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
		}
		else {
			diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
		}

		grund_t *bd_von = welt->lookup(pos)->gib_kartenboden();
		if(bautyp&tunnel_flag) {
			grund_t *bd_von = welt->lookup(koord3d(pos,start.z));
			ok = (bd_von==NULL)  ||  bd_von->gib_typ()==grund_t::tunnelboden;
		}
		else {
			grund_t *bd_nach = NULL;
			ok = ok  &&  bd_von->get_neighbour(bd_nach, invalid_wt, diff);

			// allowed ground?
			ok = ok  &&  bd_von  &&  (!bd_nach->ist_bruecke())  &&  is_allowed_step(bd_von,bd_nach,&dummy_cost);
		}
DBG_MESSAGE("wegbauer_t::calc_straight_route()","step %i,%i = %i",diff.x,diff.y,ok);
		pos += diff;
	}
	route.clear();
	// we can built a straight route?
	if(ok) {
		route.append(start, 16);
		pos = start.gib_2d();
		while(pos!=ziel.gib_2d()) {
			// shortest way
			koord diff;
			if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
				diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
			}
			else {
				diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
			}
			pos += diff;
			if(bautyp&tunnel_flag) {
				route.append(koord3d(pos, start.z), 16);
			}
			else {
				route.append(welt->lookup(pos)->gib_kartenboden()->gib_pos(), 16);
			}
		}
		max_n = route.get_count() - 1;
DBG_MESSAGE("wegbauer_t::intern_calc_straight_route()","found straight route max_n=%i",max_n);
	}
}



// special for starting/landing runways
bool
wegbauer_t::intern_calc_route_runways(koord3d start3d, const koord3d ziel3d)
{
	const koord start=start3d.gib_2d();
	const koord ziel=ziel3d.gib_2d();
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
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Zu nah am Kartenrand"), w_autodelete);
			return false;
		}
	}


	// now try begin and endpoint
	const koord zv(ribi);
	// end start
	const grund_t *gr = welt->lookup(start)->gib_kartenboden();
	const weg_t *weg=gr->gib_weg(air_wt);
	if(weg  &&  (weg->gib_besch()->gib_styp()==0  ||  ribi_t::ist_kurve(weg->gib_ribi()|ribi))) {
		// cannot connect to taxiway at the start and no curve possible
		return false;
	}
	// check end
	gr = welt->lookup(ziel)->gib_kartenboden();
	weg=gr->gib_weg(air_wt);
	if(weg  &&  (weg->gib_besch()->gib_styp()==1  ||  ribi_t::ist_kurve(weg->gib_ribi()|ribi))) {
		// cannot connect to taxiway at the end and no curve at the end
		return false;
	}
	// now try a straight line with no crossings and no curves at the end
	const int dist=abs(ziel.x-start.x)+abs(ziel.y-start.y);
	for(  int i=0;  i<=dist;  i++  ) {
		gr=welt->lookup(start+zv*i)->gib_kartenboden();
		// no slopes!
		if(gr->gib_grund_hang()!=0) {
			return false;
		}
		if(gr->hat_wege()  &&  !gr->hat_weg(air_wt)) {
			// cannot cross another way
			return false;
		}
	}
	// now we can built here ...
	route.clear();
	route.resize(dist + 2);
	for(  int i=0;  i<=dist;  i++  ) {
		route.append(welt->lookup(start + zv * i)->gib_kartenboden()->gib_pos());
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
	if(bautyp==luft  &&  besch->gib_topspeed()>=250) {
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



/* calc_route
 *
 */
void
wegbauer_t::calc_route(koord3d start, koord3d ziel)
{
long ms=get_current_time_millis();
	INT_CHECK("simbau 740");

	if(bautyp==luft  &&  besch->gib_styp()==1) {
		intern_calc_route_runways(start, ziel);
	}
	else {
		// we need start and target for on plain ground and never on monorails!
		grund_t *gr=welt->lookup(start);
		if(gr  &&  gr->gib_typ()==grund_t::monorailboden) {
			start.z -= Z_TILE_STEP;
		}
		gr=welt->lookup(ziel);
		if(gr  &&  gr->gib_typ()==grund_t::monorailboden) {
			ziel.z -= Z_TILE_STEP;
		}

		keep_existing_city_roads |= (bautyp&bot_flag)!=0;
		long cost2 = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		if(cost2<0) {
			// not sucessful: try backwards
			intern_calc_route(ziel,start);
			return;
		}

		vector_tpl<koord3d> route2(0);
		swap(route, route2);
		long cost = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		// the ceaper will survive ...
		if(cost2<cost  &&  cost>0) {
			swap(route, route2);
		}
		max_n = route.get_count() - 1;

	}
	INT_CHECK("wegbauer 778");
DBG_MESSAGE("calc_route::clac_route", "took %i ms",get_current_time_millis()-ms);
}



ribi_t::ribi
wegbauer_t::calc_ribi(int step)
{
	ribi_t::ribi ribi = ribi_t::keine;

	if(step < max_n) {
		// check distance, only neighbours
		const koord zv = (route[step + 1] - route[step]).gib_2d();
		if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
			ribi |= ribi_typ(zv);
		}
	}
	if(step > 0) {
		// check distance, only neighbours
		const koord zv = (route[step - 1] - route[step]).gib_2d();
		if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
			ribi |= ribi_typ(zv);
		}
	}
	return ribi;
}



void
wegbauer_t::baue_tunnel_und_bruecken()
{
	// tunnel pruefen
	for(int i=1; i<max_n-1; i++) {
		koord d = (route[i + 1] - route[i]).gib_2d();

		// ok, here is a gap ...
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			if(d.x*d.y!=0) {
				koord3d start = route[i];
				koord3d end   = route[i + 1];
				dbg->error("wegbauer_t::baue_tunnel_und_bruecken()","cannot built a bridge between %i,%i,%i (n=%i, max_n=%i) and %i,%i,%i",start.x,start.y,start.z,i,max_n,end.x,end.y,end.z);
				continue;
			}

			koord zv = koord (sgn(d.x), sgn(d.y));

			DBG_MESSAGE("wegbauer_t::baue_tunnel_und_bruecken", "built bridge %p between %i,%i,%i and %i,%i,%i", bruecke_besch, route[i].x, route[i].y, route[i].z, route[i + 1].x, route[i + 1].y, route[i + 1].z);

			const grund_t* start = welt->lookup(route[i]);
			const grund_t* end   = welt->lookup(route[i + 1]);

			if(start->gib_weg_hang()!=start->gib_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}
			if(end->gib_weg_hang()!=end->gib_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}

			if(start->gib_grund_hang()==0  ||  start->gib_grund_hang()==hang_typ(zv*(-1))) {
				// bridge here
DBG_MESSAGE("wegbauer_t::baue_tunnel_und_bruecken","built bridge %p",bruecke_besch);
				INT_CHECK( "wegbauer 1584" );
				brueckenbauer_t::baue(sp, welt, route[i].gib_2d(), (value_t)bruecke_besch);
//				brueckenbauer_t::baue_bruecke(welt, sp, route[i], route[i + 1], zv, bruecke_besch);
				INT_CHECK( "wegbauer 1584" );
			}
			else {
				// tunnel
				INT_CHECK( "wegbauer 1584" );
				tunnelbauer_t::baue(sp, welt, route[i].gib_2d(), tunnel_besch );
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



/* returns the amount needed to built this way
 * author prissi
 */
long
wegbauer_t::calc_costs()
{
	long costs=0;

	// construct city road?
	const weg_besch_t *cityroad = gib_besch("city_road");

	for(int i=1; i<max_n-1; i++) {
		koord d = (route[i + 1] - route[i]).gib_2d();

		// ok, here is a gap ... => either bridge or tunnel
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			koord zv = koord (sgn(d.x), sgn(d.y));

			const grund_t* start = welt->lookup(route[i]);
			const grund_t* end   = welt->lookup(route[i + 1]);

			if(start->gib_weg_hang()!=start->gib_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}
			if(end->gib_weg_hang()!=end->gib_grund_hang()) {
				// already a bridge/tunnel there ...
				continue;
			}

			if(start->gib_grund_hang()==0  ||  start->gib_grund_hang()==hang_typ(zv*(-1))) {
				// bridge
				koord pos = route[i].gib_2d();
				while (route[i + 1].gib_2d() != pos) {
					pos += zv;
					costs += bruecke_besch->gib_preis();
				}
			}
			else {
				// tunnel
				// bridge
				koord pos = route[i].gib_2d();
				while (route[i + 1].gib_2d() != pos) {
					pos += zv;
					costs -= umgebung_t::cst_tunnel;
				}
			}
		}
		else {
			// no gap => normal way
			const grund_t* gr = welt->lookup(route[i]);
			if(gr) {
				const weg_t *weg=gr->gib_weg((waytype_t)besch->gib_wtyp());
				// keep faster ways or if it is the same way ... (@author prissi)
				if(weg!=NULL  &&  (weg->gib_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_city_roads  && weg->gib_besch()==cityroad)  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed()))  ) {
						//nothing to be done
				}
				else {
					costs += besch->gib_preis();
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

		if(gr==NULL) {
			// make new tunnelboden
			tunnelboden_t* tunnel = new tunnelboden_t(welt, route[i], 0);
			weg_t *weg = weg_t::alloc(tunnel_besch->gib_waytype());
			welt->access(route[i].gib_2d())->boden_hinzufuegen(tunnel);
			tunnel->neuen_weg_bauen(weg, calc_ribi(i), sp);
			tunnel->obj_add(new tunnel_t(welt, route[i], sp, tunnel_besch));
			weg->setze_max_speed(tunnel_besch->gib_topspeed());
			cost -= tunnel_besch->gib_preis();
			sp->add_maintenance(-weg->gib_besch()->gib_wartung());
			sp->add_maintenance( tunnel_besch->gib_wartung() );
		}
		else if(gr->gib_typ()==grund_t::tunnelboden) {
			// check for extension only ...
			gr->weg_erweitern(tunnel_besch->gib_waytype(),calc_ribi(i));
		}
	}
	if(cost && sp) {
		sp->buche(cost, route[0].gib_2d(), COST_CONSTRUCTION);
	}
	return true;
}



void
wegbauer_t::baue_elevated()
{
	for(int i=0; i<=max_n; i++) {

		planquadrat_t* plan = welt->access(route[i].gib_2d());
		route[i].z = plan->gib_kartenboden()->gib_pos().z + Z_TILE_STEP;
		grund_t* gr = plan->gib_boden_in_hoehe(route[i].z);
		if(gr==NULL) {
			// add new elevated ground
			monorailboden_t* monorail = new monorailboden_t(welt, route[i], plan->gib_kartenboden()->gib_grund_hang());
			plan->boden_hinzufuegen(monorail);
		}
	}
}



void
wegbauer_t::baue_strasse()
{
	// construct city road?
	const weg_besch_t *cityroad = gib_besch("city_road");
	bool add_sidewalk = besch==cityroad;

	if(add_sidewalk) {
		sp = NULL;
	}

	// init undo
	if(sp!=NULL) {
		// intercity roads have no owner, so we must check for an owner
		sp->init_undo(road_wt,max_n);
	}

	for(int i=0; i<=max_n; i++) {
		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}

		const koord k = route[i].gib_2d();
		grund_t* gr = welt->lookup(route[i]);
		long cost = 0;

		bool extend = gr->weg_erweitern(road_wt, calc_ribi(i));

		// bridges/tunnels have their own track type and must not upgrade
		if(gr->gib_typ()==grund_t::brueckenboden  ||  gr->gib_typ()==grund_t::tunnelboden) {
			continue;
		}

		if(extend) {
			weg_t * weg = gr->gib_weg(road_wt);

			// keep faster ways or if it is the same way ... (@author prissi)
			if(weg->gib_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_city_roads  && weg->gib_besch()==cityroad)  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed())  ) {
				//nothing to be done
//DBG_MESSAGE("wegbauer_t::baue_strasse()","nothing to do at (%i,%i)",k.x,k.y);
			}
			else {
				// we take ownership => we take care to maintain the roads completely ...
				gr->setze_besitzer( NULL );	// this way the maitenace will be correct
				weg->setze_besch(besch);
				gr->setze_besitzer( sp );
				gr->calc_bild();
				cost -= besch->gib_preis();
			}
		}
		else {
			// make new way
			strasse_t * str = new strasse_t(welt);

			str->setze_besch(besch);
			str->setze_gehweg(add_sidewalk);
			cost = -gr->neuen_weg_bauen(str, calc_ribi(i), sp)-besch->gib_preis();

			// prissi: into UNDO-list, so wie can remove it later
			if(sp!=NULL) {
				// intercity raods have no owner, so we must check for an owner
				sp->add_undo( position_bei(i));
			}
		}

		if(cost && sp) {
			sp->buche(cost, k, COST_CONSTRUCTION);
		}
	} // for
}



void
wegbauer_t::baue_schiene()
{
	int i;
	if(max_n >= 1) {
		// init undo
		sp->init_undo((waytype_t)besch->gib_wtyp(),max_n);

		// built tracks
		for(i=0; i<=max_n; i++) {
			int cost = 0;
			grund_t* gr = welt->lookup(route[i]);
			ribi_t::ribi ribi = calc_ribi(i);

			bool extend = gr->weg_erweitern((waytype_t)besch->gib_wtyp(), ribi);

			// bridges/tunnels have their own track type and must not upgrade
			if((gr->gib_typ()==grund_t::brueckenboden  ||  gr->gib_typ()==grund_t::tunnelboden)  &&  gr->gib_weg_nr(0)->gib_waytype()==besch->gib_wtyp()) {
				continue;
			}

			if(extend) {
				weg_t * weg = gr->gib_weg((waytype_t)besch->gib_wtyp());

				// keep faster ways or if it is the same way ... (@author prissi)
				if(weg->gib_besch()==besch  ||  (weg->gib_besch()->gib_styp()==7  &&  gr->gib_weg_nr(0)!=weg)  ||  keep_existing_ways  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed())  ) {
					//nothing to be done
					cost = 0;
				}
				else {
					// we take ownershipe => we take care to maintain the roads completely ...
					gr->setze_besitzer( NULL );	// no costs
					weg->setze_besch(besch);
					gr->setze_besitzer( sp );	// all to us now
					gr->calc_bild();
					cost = -besch->gib_preis();
				}
			}
			else {
				weg_t *sch=weg_t::alloc((waytype_t)besch->gib_wtyp());
				sch->setze_besch(besch);
				cost = -gr->neuen_weg_bauen(sch, ribi, sp)-besch->gib_preis();
				// prissi: into UNDO-list, so wie can remove it later
				sp->add_undo( position_bei(i) );
			}

			if(cost  &&  sp) {
				sp->buche(cost, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
			}

			gr->calc_bild();

			if((i&3)==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



void
wegbauer_t::baue_leitung()
{
	int i;
	for(i=0; i<=max_n; i++) {
		grund_t* gr = welt->lookup(route[i]);

		leitung_t *lt = dynamic_cast <leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(lt==0) {
			lt = (leitung_t *)gr->suche_obj(ding_t::pumpe);
		}
		if(lt==0) {
			lt = (leitung_t *)gr->suche_obj(ding_t::senke);
		}
		// ok, really no lt here ...
		if(lt == 0) {
			lt = new leitung_t(welt, gr->gib_pos(), sp);
			if(gr->ist_natur()) {
				// remove trees etc.
				gr->obj_loesche_alle(sp);
			}
			if(sp) {
				sp->buche(-leitung_besch->gib_preis(), gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
			}
			gr->obj_add(lt);
		}
		lt->calc_neighbourhood();
		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
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

// test!
long ms=get_current_time_millis();

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
	}

	INT_CHECK("simbau 1087");
	baue_tunnel_und_bruecken();

	INT_CHECK("simbau 1087");

DBG_MESSAGE("wegbauer_t::baue", "took %i ms",get_current_time_millis()-ms);
}
