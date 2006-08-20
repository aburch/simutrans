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
#include "../boden/grund.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/route.h"

#include "../simworld.h"
#include "../simwerkz.h"
#include "../simmesg.h"
#include "../simplay.h"
#include "../simplan.h"
#include "../simdepot.h"
#include "../blockmanager.h"

#include "../dings/gebaeude.h"
#include "../dings/bruecke.h"

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
	strasse_t::default_strasse = wegbauer_t::weg_search(weg_t::strasse,50,0);
	schiene_t::default_schiene = wegbauer_t::weg_search(weg_t::schiene,80,0);
	monorail_t::default_monorail = wegbauer_t::weg_search(weg_t::monorail,120,0);
	kanal_t::default_kanal = wegbauer_t::weg_search(weg_t::wasser,20,0);
	runway_t::default_runway = wegbauer_t::weg_search(weg_t::luft,20,0);
	return ::alles_geladen(spezial_objekte + 1);
}


bool wegbauer_t::alle_kreuzungen_geladen()
{
    if(!gib_kreuzung(weg_t::schiene, weg_t::strasse) ||
  !gib_kreuzung(weg_t::strasse, weg_t::schiene))
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
const weg_besch_t *  wegbauer_t::weg_search(const weg_t::typ wtyp,const int speed_limit,const uint16 time)
{
	stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);

	const weg_besch_t * besch=NULL;
DBG_MESSAGE("wegbauer_t::weg_search","Search cheapest for limit %i, year=%i, month=%i",speed_limit, time/12, time%12+1);
	while(iter.next()) {
		const weg_besch_t *test_weg = iter.get_current_value();


		if(
			(test_weg->gib_wtyp()==wtyp
				||  (wtyp==weg_t::schiene_strab  &&  test_weg->gib_wtyp()==weg_t::schiene  &&  test_weg->gib_styp()==7)
			)  &&  iter.get_current_value()->gib_cursor()->gib_bild_nr(1) != -1) {

			if(  besch==NULL  ||  (besch->gib_topspeed()<speed_limit  &&  besch->gib_topspeed()<iter.get_current_value()->gib_topspeed()) ||  (iter.get_current_value()->gib_topspeed()>=speed_limit  &&  iter.get_current_value()->gib_wartung()<besch->gib_wartung())  ) {
				if(time==0  ||  (test_weg->get_intro_year_month()<=time  &&  test_weg->get_retire_year_month()>time)) {
					besch = test_weg;
DBG_MESSAGE("wegbauer_t::weg_search","Found weg %s, limit %i",besch->gib_name(),besch->gib_topspeed());
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
					translator::translate("%s now available:\n%s\n"),
					translator::translate(besch->gib_name()));
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->gib_bild_nr(5));
			}

			const uint16 retire_month =besch->get_retire_year_month();
			if(retire_month == current_month) {
				sprintf(buf,
					translator::translate("%s cannot longer used:\n%s\n"),
					translator::translate(besch->gib_name()));
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,besch->gib_bild_nr(5));
			}
		}

	}
}



const kreuzung_besch_t *wegbauer_t::gib_kreuzung(const weg_t::typ ns, const weg_t::typ ow)
{
    slist_iterator_tpl<const kreuzung_besch_t *> iter(kreuzungen);

    while(iter.next()) {
  if(iter.get_current()->gib_wegtyp_ns() == ns &&
      iter.get_current()->gib_wegtyp_ow() == ow) {
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
	const weg_t::typ wtyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	const int sound_ok,
	const int sound_ko,
	const uint16 time,
	const uint8 styp)
{
	// list of matching types (sorted by speed)
	slist_tpl <const weg_besch_t *> matching;

	stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);
	while(iter.next()) {
		const weg_besch_t * besch = iter.get_current_value();

		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			if(besch->gib_styp()==styp) {
				 // DarioK: load only if the given styp maches!
				if(besch->gib_wtyp()==wtyp &&  besch->gib_cursor()->gib_bild_nr(1)!=-1) {

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
	while(matching.count()>0) {
		const weg_besch_t * besch = matching.at(0);
		matching.remove_at(0);

		char buf[128];

		sprintf(buf, "%s, %d$ (%d$), %dkm/h",
			translator::translate(besch->gib_name()),
			besch->gib_preis()/100,
			(besch->gib_wartung()<<shift_maintanance)/100,
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
wegbauer_t::check_crossing(const koord zv, const grund_t *bd,weg_t::typ wtyp) const
{
  if(bd->gib_weg(wtyp)  &&  bd->gib_halt()==NULL) {
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
	const sint16 hgt=from->gib_hoehe()/16;
	const sint16 to_hgt=to->gib_hoehe()/16;
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

	if(bautyp==luft  &&  (from->gib_grund_hang()+to->gib_grund_hang()!=0  ||  (from->hat_wege()  &&  from->gib_weg(weg_t::luft)==0))) {
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
	if(to!=from) {
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

	// no check way specific stuff
	switch(bautyp) {

		case strasse:
		case strasse_bot:
		{
			const weg_t *str=to->gib_weg(weg_t::strasse);
			const weg_t *sch=to->gib_weg(weg_t::schiene);
			ok =	(str  ||  !fundament)  &&  (sch==NULL  ||  check_crossing(zv,to,weg_t::schiene)  ||  sch->gib_besch()->gib_styp()==7) &&  !to->ist_wasser()  &&
 					!to->gib_weg(weg_t::luft)  &&  !to->gib_weg(weg_t::wasser)  &&    !to->gib_weg(weg_t::monorail)  &&
 					(str  ||  check_owner(to->gib_besitzer(),sp)) &&
					check_for_leitung(zv,to);
			if(ok) {
				const weg_t *from_str=from->gib_weg(weg_t::strasse);
				// check for end/start of bridge
				if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (from_str==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|from_str->gib_ribi_unmasked()))) {
					return false;
				}
				// calculate costs
				*costs = str ? 0 : 5;
				*costs += from_str ? 0 : 5;
				if(to->gib_weg(weg_t::schiene)) {
					*costs += 4;	// avoid crossings
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case elevated_strasse:
			dbg->fatal("wegbauer_t::is_allowed_step()","elevated_strasse not (yet) allowed!");
			break;

		case schiene:
		{
			const weg_t *sch=to->gib_weg(weg_t::schiene);
			ok =	(ok&!fundament || sch  || check_crossing(zv,to,weg_t::strasse)) &&
					!to->gib_weg(weg_t::luft)  &&  !to->gib_weg(weg_t::wasser)  &&    !to->gib_weg(weg_t::monorail)  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to);
			if(ok) {
				// check for end/start of bridge
				if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (sch==NULL  ||  !ribi_t::ist_gerade(ribi_typ(zv)|sch->gib_ribi_unmasked()))) {
					return false;
				}
				if(gb) {
					// no halt => citybuilding => do not touch
					if(sch==NULL  ||  !check_owner(gb->gib_besitzer(),sp)) {
						return false;
					}
					// now we have a halt => check for direction
					if(sch  &&  ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->gib_ribi_unmasked())  ) {
						// no crossings on stops
						return false;
					}
				}
				// calculate costs
				*costs = sch ? 3 : 4;	// only prefer existing rails a little
				if(to->gib_weg(weg_t::strasse)) {
					*costs += 4;	// avoid crossings
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case elevated_schiene:
			dbg->fatal("wegbauer_t::is_allowed_step()","elevated_schiene not (yet) allowed!");
			break;

		// like schiene, but allow for railroad crossings
		// avoid crossings with any (including our) railroad tracks
		case schiene_bot:
		case schiene_bot_bau:
			if(gb  ||   to->gib_halt().is_bound()  ||  to->gib_weg(weg_t::schiene)) {
				return false;
			}
			ok =	(ok&!fundament || check_crossing(zv,to,weg_t::strasse)) &&
					 !to->gib_weg(weg_t::luft)  &&  !to->gib_weg(weg_t::wasser)  &&    !to->gib_weg(weg_t::monorail)  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to)  &&
					!to->gib_depot();
			// calculate costs
			if(ok) {
				*costs = 4;
				if(to->gib_weg(weg_t::strasse)) {
					*costs += 4;	// avoid crossings
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		break;

		// like tram, but checks for bridges too
		case monorail:
		{
			ok =	!to->ist_wasser()  &&  !fundament  &&
					(to->hat_wege()==0  ||  to->gib_weg(weg_t::monorail))  &&  (from->hat_wege()==0  ||  from->gib_weg(weg_t::monorail))
					&&  check_owner(to->gib_besitzer(),sp) && check_for_leitung(zv,to)  && !to->gib_depot();
			const weg_t *sch=to->gib_weg(weg_t::monorail);
			// check for end/start of bridge
			if(to->gib_weg_hang()!=to->gib_grund_hang()  &&  (sch==NULL  ||  ribi_t::ist_kreuzung(ribi_typ(to_pos,from_pos)|sch->gib_ribi_unmasked()))) {
				return false;
			}
			if(gb) {
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

		// like tram, but checks for bridges too
		case elevated_monorail:
		{
			ok =	(ok  || fundament || to->gib_weg(weg_t::schiene)  || to->gib_weg(weg_t::monorail)  || to->gib_weg(weg_t::strasse)) &&  !to->gib_weg(weg_t::luft)  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to);
			if(ok) {
				if(gb) {
					// no halt => citybuilding => do not touch
					if(!check_owner(gb->gib_besitzer(),sp)  ||  gb->gib_tile()->gib_hintergrund(0,1)!=-1) {  // also check for too high buildings ...
						return false;
					}
				}
				const grund_t *bd =  welt->lookup(to_pos)->gib_boden_in_hoehe(to->gib_pos().z+16);
				if(bd  &&  bd->gib_typ()!=grund_t::monorailboden) {
					ok = false;
				}
				if(bd  &&  bd->gib_halt().is_bound()) {
					// now we have a halt => check for direction
					if(!ribi_t::ist_gerade(ribi_typ(zv)|bd->gib_weg_ribi_unmasked(weg_t::monorail))  ) {
						// no crossings on stops
						return false;
					}
				}
				// calculate costs
				*costs = bd?2:4;
				// ontop of city buildings is expensive
				if(to->gib_typ()==grund_t::fundament) {
					*costs += 4;
				}
				// perfer ontop of ways
				if(to->hat_wege()) {
					*costs -= 1;
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case schiene_tram: // Dario: Tramway
		{
			ok =	(ok || to->gib_weg(weg_t::schiene)  || to->gib_weg(weg_t::strasse)) &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to)  &&
					!to->gib_depot();
			// missing: check for crossings on halt!
			// check for end/start of bridge
			if(to->gib_weg_hang()!=to->gib_grund_hang()) {
				return false;
			}
			// calculate costs
			if(ok) {
				*costs = to->gib_weg(weg_t::schiene) ? 2 : 4;	// only prefer existing rails a little
				// perfer own track
				if(to->gib_weg(weg_t::strasse)) {
					*costs += 2;
				}
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		}
		break;

		case leitung:
			ok = (!fundament &&  !to->ist_wasser())  &&  (!to->hat_wege()  ||
						(to->gib_weg(weg_t::wasser)!=NULL  &&  check_crossing(zv,to,weg_t::wasser))  ||
						(to->gib_weg(weg_t::strasse)!=NULL  &&  check_crossing(zv,to,weg_t::strasse))  ||
						(to->gib_weg(weg_t::schiene)!=NULL  &&  check_crossing(zv,to,weg_t::schiene))
					);
			// calculate costs
			if(ok) {
				*costs = to->hat_wege() ? 8 : 1;
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope;
				}
			}
		break;

		case wasser:
			ok = (ok  ||  to->ist_wasser()  ||  to->gib_weg(weg_t::wasser))  &&
					check_owner(to->gib_besitzer(),sp) &&
					check_for_leitung(zv,to);
			// calculate costs
			if(ok) {
				*costs = to->ist_wasser()  ||  to->gib_weg(weg_t::wasser) ? 2 : 10;	// prefer water very much ...
				if(to->gib_grund_hang()!=0) {
					*costs += umgebung_t::way_count_slope*2;
				}
			}
			break;

	case luft: // hsiegeln: runway
		ok = !to->ist_wasser()  &&  (to->gib_weg(weg_t::luft)  || !to->hat_wege())  &&
				  to->suche_obj(ding_t::leitung)==NULL   &&  !to->gib_depot()  &&  !fundament;
			// calculate costs
		*costs = 4;
		break;
	}
	return ok;
}




int wegbauer_t::check_for_bridge( const grund_t *parent_from, const grund_t *from, koord3d ziel )
{
	// bridge not enabled or wrong starting slope or tile already occupied with a way ...
	if(!hang_t::ist_wegbar(from->gib_grund_hang())) {
		return false;
	}

	// since we were allowed to go there, it is ok ...
	if(from->gib_grund_hang()!=from->gib_weg_hang()) {
//		DBG_MESSAGE("wegbauer_t::check_for_bridge()","has bridge starting");
		// now find the end ...
		grund_t *gr=welt->lookup(from->gib_pos());
		grund_t *to=gr;
		weg_t::typ wegtyp=(weg_t::typ)from->gib_weg_nr(0)->gib_besch()->gib_wtyp();
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
				return 1;
			}
			gr = to;
		}
		return 0;
	}

	if(parent_from==NULL  ||  bruecke_besch==NULL  ||  (from->gib_grund_hang()==0  &&  from->hat_wege())) {
		return 0;
	}

	const koord zv=from->gib_pos().gib_2d()-parent_from->gib_pos().gib_2d();
	const koord to_pos=from->gib_pos().gib_2d()+zv;
	bool has_reason_for_bridge=false;

	// ok, so now we do a closer investigation
	const int start_count = next_gr.get_count();
	grund_t *gr, *gr2;
	long internal_cost;
	const long cost_difference=besch->gib_wartung()>0 ? (bruecke_besch->gib_wartung()*4l+3l)/besch->gib_wartung() : 16;

	if(from->gib_grund_hang()==0) {
		// try bridge on even ground
		int max_lenght=bruecke_besch->gib_max_length()>0 ? bruecke_besch->gib_max_length()-2 : umgebung_t::way_max_bridge_len;
		max_lenght = min( max_lenght, umgebung_t::way_max_bridge_len );

		for(int i=0;  i<max_lenght;  i++ ) {
			// not on map or already something there => fail
			if( !welt->ist_in_kartengrenzen(to_pos+zv*(i+1)) ) {
				return next_gr.get_count()-start_count;
			}
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			if(gr2->gib_pos()==ziel) {
				return next_gr.get_count()-start_count;
			}
			if(welt->lookup(from->gib_pos()+zv*i+koord3d(0,0,16))) {
				// something in the way
				return next_gr.get_count()-start_count;
			}
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->suche_obj(ding_t::leitung)) {
				// powerline in the way
				return next_gr.get_count()-start_count;
			}
			// some artificial tiles here?!?
			if(gr->gib_pos().z>from->gib_pos().z) {
				return next_gr.get_count()-start_count;
			}
			// check for runways ...
			if(gr->gib_weg(weg_t::luft)) {
				return next_gr.get_count()-start_count;
			}
			// check, if we need really a bridge
			if(i<=2  &&  !has_reason_for_bridge) {
				long dummy;
				has_reason_for_bridge = !is_allowed_step(gr, gr2, &dummy );
			}
			// non-manable slope?
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
#ifndef DOUBLE_GROUNDS
				if(!hang_t::ist_wegbar(gr->gib_grund_hang())  ||  ribi_typ(zv)!=ribi_typ(gr->gib_grund_hang())) {
#else
				if(hang_t::ist_wegbar(gr->gib_grund_hang())  ||  !hang_t::ist_einfach(gr->gib_grund_hang())  ||  ribi_typ(zv)!=ribi_typ(gr->gib_grund_hang())) {
#endif
					return next_gr.get_count()-start_count;
				}
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
					return next_gr.get_count()-start_count;
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
						return next_gr.get_count()-start_count;
					}
				}
			}
		}
		return next_gr.get_count()-start_count;
	}

	// downhill hang ...
	if(ribi_typ(from->gib_grund_hang())==ribi_t::rueckwaerts(ribi_typ(zv))) {
		// try bridge

		int max_lenght=bruecke_besch->gib_max_length()>0 ? bruecke_besch->gib_max_length()-2 : umgebung_t::way_max_bridge_len;
		max_lenght = min( max_lenght, umgebung_t::way_max_bridge_len );

		for(int i=1;  i<max_lenght;  i++ ) {
			// not on map or already something there => fail
			if(!welt->ist_in_kartengrenzen(to_pos+zv*(i+1)) ) {
				return false;
			}
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			// must leave one field empty ...
			if(gr2->gib_pos()==ziel) {
				return next_gr.get_count()-start_count;
			}
			if(welt->lookup(from->gib_pos()+zv*i)) {
				// something in the way
				return next_gr.get_count()-start_count;
			}
			if(gr->gib_pos().z==from->gib_pos().z+16  &&  gr->suche_obj(ding_t::leitung)) {
				// powerline in the way
				return next_gr.get_count()-start_count;
			}
			// some articial tiles here?!?
			if(gr->gib_pos().z>from->gib_pos().z) {
				return next_gr.get_count()-start_count;
			}
			// check for runways ...
			if(gr->gib_weg(weg_t::luft)) {
				return next_gr.get_count()-start_count;
			}
			// non-manable slope?
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
#ifndef DOUBLE_GROUNDS
				if(!hang_t::ist_wegbar(gr->gib_grund_hang())  ||  ribi_typ(zv)!=ribi_typ(gr->gib_grund_hang())) {
#else
				if(hang_t::ist_wegbar(gr->gib_grund_hang())  ||  !hang_t::ist_einfach(gr->gib_grund_hang())  ||  ribi_typ(zv)!=ribi_typ(gr->gib_grund_hang())) {
#endif
					return next_gr.get_count()-start_count;
				}
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
					return next_gr.get_count()-start_count;
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
		return next_gr.get_count()-start_count;
	}

	// uphill hang ... may be tunnel?
	if(ribi_typ(from->gib_grund_hang())==ribi_typ(zv)) {

		for(unsigned i=0;  i<(unsigned)umgebung_t::way_max_bridge_len;  i++ ) {
			// not on map or already something there => fail
			if(!welt->ist_in_kartengrenzen(to_pos+zv*(i+1)) ) {
				return false;
			}
			gr = welt->lookup(to_pos+zv*i)->gib_kartenboden();
			gr2 = welt->lookup(to_pos+zv*(i+1))->gib_kartenboden();
			// here is a hang
			if(gr->gib_pos().z==from->gib_pos().z  &&  gr->gib_grund_hang()!=0) {
				if(ribi_t::rueckwaerts(ribi_typ(zv))!=ribi_typ(gr->gib_grund_hang())  ||  !hang_t::ist_einfach(gr->gib_grund_hang())) {
					// non, manable slope
					return false;
				}
				// ok, qualify for endpos
				if(!gr->hat_wege()  &&  gr->ist_natur()  &&  !gr->ist_wasser()  &&  is_allowed_step(gr, gr2, &internal_cost )) {
					// ok, here we may end
					// we return the koord3d AFTER we came down!
					next_gr_t ngt={gr,i*umgebung_t::way_count_tunnel };
					next_gr.append(ngt,16);
					return 1;
				}
			}
		}
	}

	return false;
}






wegbauer_t::wegbauer_t(karte_t *wl, spieler_t *spl) : next_gr(32)
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

    route = new vector_tpl<koord3d> (0);
}


wegbauer_t::~wegbauer_t()
{
    delete route;
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
wegbauer_t::route_fuer(enum bautyp wt, const weg_besch_t *b, const bruecke_besch_t *br)
{
  bautyp = wt;
  besch = b;
  bruecke_besch = br;
  if(sp==NULL) {
  	bruecke_besch = NULL;
  }
#if AUTOMATIC_BRIDGES
  else if(bruecke_besch==NULL) {
  	bruecke_besch = brueckenbauer_t::find_bridge((weg_t::typ)b->gib_wtyp(),25,0);
  }
#endif
  DBG_MESSAGE("wegbauer_t::route_fuer()",
         "setting way type to %d, besch=%s, bruecke_besch=%s",
         bautyp,
         besch ? besch->gib_name() : "NULL",
         bruecke_besch ? bruecke_besch->gib_name() : "NULL"
         );
}



/* this routine uses A* to calculate the best route
 * beware: change the cost and you will mess up the system!
 */
long
wegbauer_t::intern_calc_route(const koord start, const koord ziel)
{
	// we assume fail
	max_n = -1;

	// we clear it here probably twice: does not hurt ...
	route->clear();

	// check for existing koordinates
	if(welt->lookup(start)==NULL  ||  welt->lookup(ziel)==NULL) {
		return -1;
	}

	// some thing for the search
	const grund_t *gr;
	grund_t *to;

	const koord3d start3d = welt->lookup(start)->gib_kartenboden()->gib_pos();
	const koord3d ziel3d = welt->lookup(ziel)->gib_kartenboden()->gib_pos();

	koord3d gr_pos;	// just the last valid pos ...

	// memory in static list ...
	const int MAX_STEP = 65530;	// may need very much memory => configurable
	if(route_t::nodes==NULL) {
		route_t::nodes = new route_t::ANode[MAX_STEP+4+1];
	}
	int step = 0;

	INT_CHECK("wegbauer 347");

	// arrays for A*
	static vector_tpl <route_t::ANode *> open = vector_tpl <route_t::ANode *>(0);

	// nothing in lists
	open.clear();
	welt->unmarkiere_alle();

	route_t::ANode *tmp = &(route_t::nodes[step++]);
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start3d);
	tmp->f = route_t::calc_distance(start3d,ziel3d);
	tmp->g = 0;
	tmp->dir = 0;

	// is valid ground?
	long dummy;
	if(!is_allowed_step(tmp->gr,tmp->gr,&dummy)) {
		DBG_MESSAGE("wegbauer_t::intern_calc_route()","cannot start on (%i,%i)",start.x,start.y);
		return false;
	}

	route_t::GET_NODE();

	// assume first tile is valid!

	// start in open
	open.append(tmp,256);

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	do {
		tmp = open.at( open.get_count()-1 );
		open.remove_at( open.get_count()-1 );

		gr = tmp->gr;
		gr_pos = gr->gib_pos();
		welt->markiere(gr);

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);

		// already there
		if(gr_pos==ziel3d  ||  tmp->g>maximum) {
			// we added a target to the closed list: we are finished
			break;
		}

		// the four possible directions plus any additional stuff due to already existing brides plus new ones ...
		next_gr.clear();

		// only one direction allowed ...
		const koord bridge_nsow=tmp->parent!=NULL ? gr->gib_pos().gib_2d()-tmp->parent->gr->gib_pos().gib_2d() : koord::invalid;

		// testing all four possible directions
		for(int r=0; r<4; r++) {

			to = NULL;
//				const planquadrat_t *pl=welt->lookup(gr_pos.gib_2d()+koord::nsow[r]);
			if(!gr->get_neighbour(to,weg_t::invalid,koord::nsow[r])) {
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

			to = next_gr.get(r).gr;

			// new values for cost g
			uint32 new_g = tmp->g + next_gr.get(r).cost;

			// check for curves (usually, one would need the lastlast and the last;
			// if not there, then we could just take the last
			uint8 current_dir;
			if(tmp->parent==NULL) {
				current_dir = ribi_typ( tmp->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				if(tmp->dir!=current_dir) {
					new_g += umgebung_t::way_count_curve;
				}
			}
			else {
				current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				if(tmp->parent->dir!=current_dir  ||  tmp->dir!=current_dir) {
					// is near a curve ...

					if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
						// 180° bend
						new_g +=  umgebung_t::way_count_90_curve;
					}
					else if(ribi_t::ist_orthogonal(tmp->parent->dir,current_dir)) {
						// 90° curve
						new_g +=  umgebung_t::way_count_double_curve;
//DBG_MESSAGE("curve double from","%i,%i -> %i,%i -> %i,%i", tmp->parent->gr->gib_pos().x, tmp->parent->gr->gib_pos().y, tmp->gr->gib_pos().x, tmp->gr->gib_pos().y, to->gib_pos().x, to->gib_pos().y );
					}
					// double curve?
					else if(tmp->dir!=current_dir  &&  tmp->parent->dir!=current_dir  &&  tmp->dir!=tmp->parent->dir) {
						new_g +=  umgebung_t::way_count_double_curve;
					}
					// normal curve?
					else {
						if(tmp->dir!=current_dir) {
							new_g +=  umgebung_t::way_count_curve;
						}
/*
						if(tmp->parent->dir!=current_dir) {
							new_g +=  umgebung_t::way_count_curve;
						}
*/
					}
				}
			}

			const uint32 new_f = new_g+route_t::calc_distance( to->gib_pos(), ziel3d );

			// already in open list and better?
			sint32 index;
			for(  index=open.get_count()-1;  index>=0  &&   open.get(index)->f<=new_f;  index--  ) {
				if(open.get(index)->gr==to) {
					break;
				}
			}

			if(index>=0  &&  open.get(index)->gr==to) {
				// it is already contained in the list
				// and it is lower in f ...
				continue;
			}

			// it may or may not be in the list; but since the arrays are sorted
			// we find out about this during inserting!

			INT_CHECK("wegbauer 161");

			// not in there or taken out => add new
			route_t::ANode *k=&(route_t::nodes[step++]);

			k->parent = tmp;
			k->gr = to;
			k->g = new_g;
			k->f = new_f;
			k->dir = current_dir;

			// insert sorted
			for( index=0;  index<(int)open.get_count()  &&  open.get(index)->f>new_f;  index++  ) {
				if(open.get(index)->gr==to) {
					open.remove_at(index);
					index --;
				}
			}
			// was best f so far => append
			if(index>=(int)open.get_count()) {
				open.append(k,16);
			}
			else {
				open.insert_at(index,k);
			}

//DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i at %i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z,k->f, index);
		}
	} while(open.get_count()>0  &&  step<MAX_STEP  &&  gr->gib_pos()!=ziel3d);

	INT_CHECK("wegbauer 194");

	route_t::RELEASE_NODE();

//DBG_DEBUG("reached","%i,%i",tmp->pos.x,tmp->pos.y);
	// target reached?
	if(gr->gib_pos()!=ziel3d  || step >= MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("wegbauer_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
		return -1;
	}
	else {
		const long cost = tmp->g;
		// reached => construct route
		while(tmp != NULL) {
			route->append( tmp->gr->gib_pos(), 256 );
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->parent;
		}

		// maybe here bridges should be optimized ...
		max_n = route->get_count()-1;
		return cost;
  }

    return -1;
}



void
wegbauer_t::intern_calc_straight_route(const koord start, const koord ziel)
{
	bool ok=true;
	max_n = -1;

	if(!welt->ist_in_kartengrenzen(start)  ||  !welt->ist_in_kartengrenzen(ziel)) {
		// not building
		return;
	}
	long dummy_cost;
	grund_t *test_bd = welt->lookup(start)->gib_kartenboden();
	if(!is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// no legal ground to start ...
		return;
	}
	test_bd = welt->lookup(ziel)->gib_kartenboden();
	if(!is_allowed_step(test_bd,test_bd,&dummy_cost)) {
		// ... or to end
		return;
	}

	koord pos=start;
	while(pos!=ziel  &&  ok) {

		// shortest way
		koord diff;
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
		}
		else {
			diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
		}

		grund_t *bd_von = welt->lookup(pos)->gib_kartenboden();
		grund_t *bd_nach = NULL;
		ok = ok  &&  bd_von->get_neighbour(bd_nach, weg_t::invalid, diff);

		// allowed ground?
		ok = ok  &&  bd_von  &&  (!bd_nach->ist_bruecke())  &&  is_allowed_step(bd_von,bd_nach,&dummy_cost);
DBG_MESSAGE("wegbauer_t::calc_straight_route()","step %i,%i = %i",diff.x,diff.y,ok);
		pos += diff;
	}
	route->clear();
	// we can built a straight route?
	if(ok) {
		pos = start;
		route->append( welt->lookup(pos)->gib_kartenboden()->gib_pos(), 16 );
		while(pos!=ziel) {
			// shortest way
			koord diff;
			if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
				diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
			}
			else {
				diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
			}
			pos += diff;
			route->append( welt->lookup(pos)->gib_kartenboden()->gib_pos(), 16 );
		}
		max_n = route->get_count()-1;
DBG_MESSAGE("wegbauer_t::intern_calc_straight_route()","found straight route max_n=%i",max_n);
	}
}



// special for starting/landing runways
bool
wegbauer_t::intern_calc_route_runways(koord start, const koord ziel)
{
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
	const weg_t *weg=gr->gib_weg(weg_t::luft);
	if(weg  &&  (weg->gib_besch()->gib_styp()==0  ||  ribi_t::ist_kurve(weg->gib_ribi()|ribi))) {
		// cannot connect to taxiway at the start and no curve possible
		return false;
	}
	// check end
	gr = welt->lookup(ziel)->gib_kartenboden();
	weg=gr->gib_weg(weg_t::luft);
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
		if(gr->hat_wege()  &&  gr->gib_weg(weg_t::luft)==0) {
			// cannot cross another way
			return false;
		}
	}
	// now we can built here ...
	route->clear();
	route->resize( dist+2 );
	for(  int i=0;  i<=dist;  i++  ) {
		route->append( welt->lookup(start+zv*i)->gib_kartenboden()->gib_pos() );
	}
	max_n = dist;
	return true;
}




/* calc_straight_route (maximum one curve, including diagonals)
 *
 */
void
wegbauer_t::calc_straight_route(koord start, const koord ziel)
{
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
wegbauer_t::calc_route(koord start, const koord ziel)
{
long ms=get_current_time_millis();
	INT_CHECK("simbau 740");

	if(bautyp==luft  &&  besch->gib_styp()==1) {
		intern_calc_route_runways(start, ziel);
	}
	else {
		keep_existing_city_roads |= (bautyp==strasse_bot);
		long cost2 = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		if(cost2<0) {
			// not sucessful: try backwards
			intern_calc_route(ziel,start);
			return;
		}

		vector_tpl<koord3d> *route2 = route;
		route = new vector_tpl<koord3d>(0);
		long cost = intern_calc_route(start, ziel);
		INT_CHECK("wegbauer 1165");

		// the ceaper will survive ...
		if(cost2<cost) {
			delete route;
			route = route2;
		}
		else {
			delete route2;
		}
		max_n = route->get_count()-1;

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
  const koord zv = (route->at(step+1) - route->at(step)).gib_2d();
  if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
      ribi |= ribi_typ(zv);
  }
    }
    if(step > 0) {
  // check distance, only neighbours
  const koord zv = (route->at(step-1) - route->at(step)).gib_2d();
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
		koord d = (route->at(i+1) - route->at(i)).gib_2d();

		// ok, here is a gap ...
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			if(d.x*d.y!=0) {
				koord3d start=route->at(i), end=route->at(i+1);
				dbg->error("wegbauer_t::baue_tunnel_und_bruecken()","cannot built a bridge between %i,%i,%i (n=%i, max_n=%i) and %i,%i,%i",start.x,start.y,start.z,i,max_n,end.x,end.y,end.z);
				continue;
			}

			koord zv = koord (sgn(d.x), sgn(d.y));

DBG_MESSAGE("wegbauer_t::baue_tunnel_und_bruecken","built bridge %p between %i,%i,%i and %i,%i,%i",bruecke_besch, route->at(i).x, route->at(i).y, route->at(i).z, route->at(i+1).x, route->at(i+1).y, route->at(i+1).z );

			const grund_t *start=welt->lookup(route->at(i));
			const grund_t *end=welt->lookup(route->at(i+1));

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
				brueckenbauer_t::baue(sp, welt, route->at(i).gib_2d(), (value_t)bruecke_besch  );
//				brueckenbauer_t::baue_bruecke(welt, sp, route->at(i), route->at(i+1), zv, bruecke_besch);
				INT_CHECK( "wegbauer 1584" );
			}
			else {
				// tunnel
				INT_CHECK( "wegbauer 1584" );
				tunnelbauer_t::baue(sp, welt, route->at(i).gib_2d(), (weg_t::typ)besch->gib_wtyp()  );
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
		koord d = (route->at(i+1) - route->at(i)).gib_2d();

		// ok, here is a gap ... => either bridge or tunnel
		if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

			koord zv = koord (sgn(d.x), sgn(d.y));

			const grund_t *start=welt->lookup(route->at(i));
			const grund_t *end=welt->lookup(route->at(i+1));

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
				koord pos = route->at(i).gib_2d();
				while(route->at(i+1).gib_2d()!=pos) {
					pos += zv;
					costs += bruecke_besch->gib_preis();
				}
			}
			else {
				// tunnel
				// bridge
				koord pos = route->at(i).gib_2d();
				while(route->at(i+1).gib_2d()!=pos) {
					pos += zv;
					costs -= umgebung_t::cst_tunnel;
				}
			}
		}
		else {
			// no gap => normal way
			const grund_t *gr=welt->lookup(route->at(i));
			const weg_t *weg=gr->gib_weg((weg_t::typ)besch->gib_wtyp());
			// keep faster ways or if it is the same way ... (@author prissi)
			if(weg!=NULL  &&  (weg->gib_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_city_roads  && weg->gib_besch()==cityroad)  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed()))  ) {
					//nothing to be done
			}
			else {
				costs += besch->gib_preis();
			}
		}
		// check next tile
	}
	DBG_MESSAGE("wegbauer_t::calc_costs()","construction estimate: %f",costs/100.0);
	return costs;
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
		sp->init_undo(weg_t::strasse,max_n);
	}

	for(int i=0; i<=max_n; i++) {
		if((i&3)==0) {
			INT_CHECK( "wegbauer 1584" );
		}

		const koord k = route->at(i).gib_2d();
		grund_t *gr = welt->lookup(route->at(i));
		long cost = 0;

		if(gr->weg_erweitern(weg_t::strasse, calc_ribi(i))) {
			weg_t * weg = gr->gib_weg(weg_t::strasse);

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
			gr->neuen_weg_bauen(str, calc_ribi(i), sp);

			// prissi: into UNDO-list, so wie can remove it later
			if(sp!=NULL) {
				// intercity raods have no owner, so we must check for an owner
				sp->add_undo( position_bei(i));
			}

			cost = -besch->gib_preis();
		}

		if(cost && sp) {
			sp->buche(cost, k, COST_CONSTRUCTION);
		}
	} // for
}


void
wegbauer_t::baue_leitung()
{
	int i;
	for(i=0; i<=max_n; i++) {
		const koord k = position_bei(i);
		grund_t *gr = welt->lookup(route->at(i));

		leitung_t *lt = dynamic_cast <leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(lt==0) {
			lt = (leitung_t *)gr->suche_obj(ding_t::pumpe);
		}
		if(lt==0) {
			lt = (leitung_t *)gr->suche_obj(ding_t::senke);
		}
		// ok, really no lt here ...
		if(lt == 0) {
//			DBG_MESSGAE("no powerline %p at (%i,%i)\n",lt,k.x,k.y);
			lt = new leitung_t(welt, gr->gib_pos(), sp);
			if(gr->ist_natur()) {
				// remove trees etc.
				gr->obj_loesche_alle(sp);
			}
			if(sp) {
				sp->buche(-leitung_besch->gib_preis(), k, COST_CONSTRUCTION);
			}
			gr->obj_add(lt);
		}
		lt->calc_neighbourhood();
		if(i&3==0) {
			INT_CHECK( "wegbauer 1584" );
		}
	}
}


void
wegbauer_t::baue_schiene()
{
	int i;
	if(max_n >= 1) {

		// rails have blocks
		blockmanager * bm = blockmanager::gib_manager();

		// init undo
		sp->init_undo(weg_t::schiene,max_n);

		// built tracks
		for(i=0; i<=max_n; i++) {
			int cost = 0;
			grund_t *gr = welt->lookup(route->at(i));
			ribi_t::ribi ribi = calc_ribi(i);

			if(gr->weg_erweitern(weg_t::schiene, ribi)) {
				weg_t * weg = gr->gib_weg(weg_t::schiene);

				// keep faster ways or if it is the same way ... (@author prissi)
				if(weg->gib_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed())  ) {
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

				bm->schiene_erweitern(welt, gr);
			}
			else {
				schiene_t * sch = new schiene_t(welt);
				sch->setze_besch(besch);
				gr->neuen_weg_bauen(sch, ribi, sp);

				// prissi: into UNDO-list, so wie can remove it later
				sp->add_undo( position_bei(i) );

				bm->neue_schiene(welt, gr);
				cost = -besch->gib_preis();
			}

			if(cost  &&  sp) {
				sp->buche(cost, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
			}

			gr->calc_bild();

			if(i&3==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *start = welt->lookup(route->at(0));
		grund_t *end = welt->lookup(route->at(max_n));
		grund_t *to;

		for (i=0; i <= 4; i++) {
			if (start->get_neighbour(to, weg_t::schiene, koord::nsow[i])) {
				to->calc_bild();
			}
			if (end->get_neighbour(to, weg_t::schiene, koord::nsow[i])) {
				to->calc_bild();
			}
			if(i&3==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}



void
wegbauer_t::baue_monorail()
{
	if(max_n >= 1) {

		// rails have blocks
		blockmanager * bm = blockmanager::gib_manager();

		sint16 z_offset=0;
		if(bautyp==elevated_monorail) {
			z_offset = 16;
		}

		// init undo
		sp->init_undo(weg_t::monorail,max_n);

		// built elevated track ... non-trivial
		for(int i=0; i<=max_n; i++) {

			int cost = 0;
			ribi_t::ribi ribi = calc_ribi(i);
			planquadrat_t *plan = welt->access(route->at(i).gib_2d());
			grund_t *monorail = plan->gib_boden_in_hoehe(plan->gib_kartenboden()->gib_pos().z+z_offset);

			// here is already a track => try to connect
			if(monorail) {
				if(monorail->weg_erweitern(weg_t::monorail, ribi)) {
					weg_t *weg=monorail->gib_weg(weg_t::monorail);
					// keep faster ways or if it is the same way ... (@author prissi)
					if(weg->gib_besch()==besch  ||  keep_existing_ways  ||  (keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>besch->gib_topspeed())  ) {
						//nothing to be done
						cost = 0;
					}
					else {
						// we take ownershipe => we take care to maintain the roads completely ...
						monorail->setze_besitzer( NULL );
						weg->setze_besch(besch);
						monorail->setze_besitzer( sp );
						monorail->calc_bild();
						cost = -besch->gib_preis();
					}
					bm->schiene_erweitern(welt, monorail);
					monorail->calc_bild();
				}
				else {
					// must built new way here (is on kartenboden!)
					monorail_t * mono = new monorail_t(welt);
					mono->setze_besch(besch);
					monorail->neuen_weg_bauen(mono, ribi, sp);

					// prissi: into UNDO-list, so wie can remove it later
					sp->add_undo( position_bei(i) );

					bm->neue_schiene(welt, monorail);
					cost = -besch->gib_preis();
				}
			}
			else {
				monorail = new  monorailboden_t( welt, plan->gib_kartenboden()->gib_pos()+koord3d(0, 0, 16) );

				monorail_t *mono = new monorail_t(welt);
				mono->setze_besch(besch);
				mono->setze_max_speed(besch->gib_topspeed());
				plan->boden_hinzufuegen(monorail);
				monorail->neuen_weg_bauen(mono, ribi, sp);

				// prissi: into UNDO-list, so wie can remove it later
	//			sp->add_undo( position_bei(i) );

				bm->neue_schiene(welt, monorail);
				cost = -besch->gib_preis();
				monorail->calc_bild();
			}

			if(cost  &&  sp) {
				sp->buche(cost, route->at(i).gib_2d(), COST_CONSTRUCTION);
			}

			if(i&3==0) {
				INT_CHECK( "wegbauer 1584" );
			}
		}
	}
}




void
wegbauer_t::baue_kanal()
{
	int i;
	if(max_n >= 1) {

		// init undo
		sp->init_undo(weg_t::wasser,max_n);

		// built tracks
		for(i=0; i<=max_n; i++) {
			int cost = 0;
			grund_t *gr = welt->lookup(route->at(i));
			ribi_t::ribi ribi = calc_ribi(i);

			// extended ribi calculation for first and last tile
			if(i==0  ||  i==max_n) {
				// see slope!
				if(gr->gib_hoehe()==welt->gib_grundwasser()) {
					if(i>0) {
						ribi |= ribi_typ( position_bei(max_n-1), position_bei(max_n) );
					}
					else {
						ribi |= ribi_typ( position_bei(1), position_bei(0) );
					}
DBG_MESSAGE("wegbauer_t::baue_kanal()","extend ribi_t at (%i,%i) with %i",route->at(i).x,route->at(i).y, ribi );
				}
			}

			if(!gr->ist_wasser()) {
				if(gr->weg_erweitern(weg_t::wasser, ribi)) {
					weg_t * weg = gr->gib_weg(weg_t::wasser);
					if(weg->gib_besch()!=besch && !keep_existing_ways) {
						// Hajo: den typ des weges aendern, kosten berechnen
						gr->setze_besitzer( NULL );
						weg->setze_besch(besch);
						gr->setze_besitzer( sp );
						gr->calc_bild();
						cost = -besch->gib_preis();
					}

				}
				else {
					kanal_t * w = new kanal_t(welt);
					w->setze_besch(besch);
					gr->neuen_weg_bauen(w, ribi, sp);

					// prissi: into UNDO-list, so wie can remove it later
					sp->add_undo( position_bei(i) );

					cost = -besch->gib_preis();
				}

				if(cost  &&  sp) {
					sp->buche(cost, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
				}

				gr->calc_bild();

				if(i&3==0) {
					INT_CHECK( "wegbauer 1584" );
				}
			}
		}
	}

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *start = welt->lookup(route->at(0));
	grund_t *end = welt->lookup(route->at(max_n));
	grund_t *to;

	for (i=0; i <= 4; i++) {
		if (start->get_neighbour(to, weg_t::wasser, koord::nsow[i])) {
			to->calc_bild();
		}
		if (end->get_neighbour(to, weg_t::wasser, koord::nsow[i])) {
			to->calc_bild();
		}
	}
}



void
wegbauer_t::baue_runway()
{
	int i;

	if(max_n >= 1) {

		// init undo
		sp->init_undo(weg_t::luft,max_n);

		// built tracks
		for(i=0; i<=max_n; i++) {
			int cost = 0;
			grund_t *gr = welt->lookup(route->at(i));
			ribi_t::ribi ribi = calc_ribi(i);
			// extended ribi calculation for first and last tile
			if(i==0  ||  i==max_n) {
				// see slope!
				if(gr->gib_hoehe()==welt->gib_grundwasser()) {
					if(i>0) {
						ribi |= ribi_typ( position_bei(max_n-1), position_bei(max_n) );
					}
					else {
						ribi |= ribi_typ( position_bei(1), position_bei(0) );
					}
DBG_MESSAGE("wegbauer_t::baue_runway()","extend ribi_t at (%i,%i) with %i",route->at(i).x,route->at(i).y, ribi );
				}
			}

			if(!gr->ist_wasser()) {
				if(gr->weg_erweitern(weg_t::luft, ribi)) {
					weg_t * weg = gr->gib_weg(weg_t::luft);
					if(weg->gib_besch()!=besch && !(weg->gib_besch()->gib_topspeed()>besch->gib_topspeed())) {
						if(sp) {
							sp->add_maintenance(besch->gib_wartung() - weg->gib_besch()->gib_wartung());
						}

						weg->setze_besch(besch);
						gr->calc_bild();
						cost = -besch->gib_preis();
					}

				}
				else {
					runway_t * w = new runway_t(welt);
					w->setze_besch(besch);
					gr->neuen_weg_bauen(w, ribi, sp);

					// prissi: into UNDO-list, so wie can remove it later
					sp->add_undo( position_bei(i) );

					cost = -besch->gib_preis();
				}

				if(cost  &&  sp) {
					sp->buche(cost, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
				}

				gr->calc_bild();

				if(i&3==0) {
					INT_CHECK( "wegbauer 1880" );
				}
			}
		}
	}

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *start = welt->lookup(route->at(0));
	grund_t *end = welt->lookup(route->at(max_n));
	grund_t *to;

	for (i=0; i <= 4; i++) {
		if (start->get_neighbour(to, weg_t::luft, koord::nsow[i])) {
			to->calc_bild();
		}
		if (end->get_neighbour(to, weg_t::luft, koord::nsow[i])) {
			to->calc_bild();
		}
	}
}




/* built a coordinate list
 * @author prissi
 */
void
wegbauer_t::baue_strecke( slist_tpl <koord> &list )
{
	max_n = list.count()-1;
	for(  int i=0;  i<=max_n;  i++  ) {
		route->at(i) = welt->lookup(list.at(i))->gib_kartenboden()->gib_pos();
	}
	baue();
	baue_tunnel_und_bruecken();
}



void
wegbauer_t::baue()
{
	if(max_n<0  ||  max_n>(sint32)maximum) {
		DBG_MESSAGE("wegbauer_t::baue()","called, but no valid route.");
		// no valid route here ...
		return;
	}
  DBG_MESSAGE("wegbauer_t::baue()",
         "type=%d max_n=%d start=%d,%d end=%d,%d",
         bautyp, max_n,
         route->at(0).x, route->at(0).y,
         route->at(max_n).x, route->at(max_n).y );
// test!
long ms=get_current_time_millis();

	INT_CHECK("simbau 1072");
  switch(bautyp) {
  	case wasser:
  			baue_kanal();
  			break;
   	case strasse:
   	case elevated_strasse:
   	case strasse_bot:
			baue_strasse();
			DBG_MESSAGE("wegbauer_t::baue", "strasse");
			break;
   	case schiene:
   	case elevated_schiene:
   	case schiene_bot:
   	case schiene_bot_bau:
			DBG_MESSAGE("wegbauer_t::baue", "schiene");
			baue_schiene();
			break;
		case leitung:
			baue_leitung();
			break;
		case schiene_tram: // Dario: Tramway
			DBG_MESSAGE("wegbauer_t::baue", "schiene_tram");
			baue_schiene();
			break;
		case monorail:
		case elevated_monorail:
			DBG_MESSAGE("wegbauer_t::baue", "monorail");
			baue_monorail();
			break;
		case luft:
			DBG_MESSAGE("wegbauer_t::baue", "luft");
			baue_runway();
			break;
  }
 	INT_CHECK("simbau 1087");
	baue_tunnel_und_bruecken();
 	INT_CHECK("simbau 1087");

DBG_MESSAGE("wegbauer_t::baue", "took %i ms",get_current_time_millis()-ms);
}
