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
#include "../simgraph.h"
#include "../simwin.h"
#include "../simhalt.h"
#include "../besch/sound_besch.h"
#include "../player/simplay.h"
#include "../simskin.h"
#include "../simtypes.h"

#include "../boden/boden.h"
#include "../boden/brueckenboden.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/kanal.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_waehler.h"
#include "../gui/karte.h"

#include "../besch/bruecke_besch.h"
#include "../besch/skin_besch.h"

#include "../dings/bruecke.h"
#include "../dings/crossing.h"
#include "../dings/leitung2.h"
#include "../dings/pillar.h"
#include "../dings/signal.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"


static stringhashtable_tpl<bruecke_besch_t *> bruecken_by_name;


/**
 * Registers a new bridge type
 * @author V. Meyer, Hj. Malthaner
 */
void brueckenbauer_t::register_besch(bruecke_besch_t *besch)
{
	// avoid duplicates with same name
	if(  bruecken_by_name.remove(besch->get_name())  ) {
		dbg->warning( "brueckenbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
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



bool brueckenbauer_t::laden_erfolgreich()
{
	bool strasse_da = false;
	bool schiene_da = false;

	stringhashtable_iterator_tpl<bruecke_besch_t *>iter(bruecken_by_name);
	while(  iter.next()  ) {
		const bruecke_besch_t* besch = iter.get_current_value();

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
 * Find a matchin bridge
 * @author Hj. Malthaner
 */
const bruecke_besch_t *brueckenbauer_t::find_bridge(const waytype_t wtyp, const sint32 min_speed, const uint16 time)
{
	const bruecke_besch_t *find_besch=NULL;

	stringhashtable_iterator_tpl<bruecke_besch_t *>iter(bruecken_by_name);
	while(  iter.next()  ) {
		const bruecke_besch_t* besch = iter.get_current_value();
		if(besch->get_waytype() == wtyp) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				if(find_besch==NULL  ||
					(find_besch->get_topspeed()<min_speed  &&  find_besch->get_topspeed()<besch->get_topspeed())  ||
					(besch->get_topspeed()>=min_speed  &&  besch->get_wartung()<find_besch->get_wartung())
				) {
					find_besch = besch;
				}
			}
		}
	}
	return find_besch;
}


static bool compare_bridges(const bruecke_besch_t* a, const bruecke_besch_t* b)
{
	return a->get_topspeed() < b->get_topspeed();
}


/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */

void brueckenbauer_t::fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, sint16 /*sound_ok*/, const karte_t *welt)
{
	static stringhashtable_tpl<wkz_brueckenbau_t *> bruecken_tool;

	const uint16 time = welt->get_timeline_year_month();
	vector_tpl<const bruecke_besch_t*> matching(bruecken_by_name.get_count());

	// list of matching types (sorted by speed)
	stringhashtable_iterator_tpl<bruecke_besch_t *>iter(bruecken_by_name);
	while(  iter.next()  ) {
		const bruecke_besch_t* b = iter.get_current_value();
		if (b->get_waytype() == wtyp && (
					time == 0 ||
					(b->get_intro_year_month() <= time && time < b->get_retire_year_month())
				)) {
			matching.insert_ordered( b, compare_bridges);
		}
	}

	// now sorted ...
	for (vector_tpl<const bruecke_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		wzw->add_werkzeug( (*i)->get_builder() );
	}
}



koord3d brueckenbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, const bruecke_besch_t *besch, const char *&error_msg, bool ai_bridge, uint32 min_length )
{
	const grund_t *gr1; // on the level of the bridge
	const grund_t *gr2; // the level under the bridge
	waytype_t wegtyp = besch->get_waytype();
	error_msg = NULL;
	uint16 length = 0;
	do {
		length ++;
		pos = pos + zv;

		// test max length
		if(besch->get_max_length()>0  &&  length > besch->get_max_length()) {
			error_msg = "Bridge is too long for this type!\n";
			return koord3d::invalid;
		}

		if(!welt->ist_in_kartengrenzen(pos.get_2d())) {
			error_msg = "Bridge is too long for this type!\n";
			return koord3d::invalid;
		}
		// check for height
		sint16 height = pos.z -welt->lookup_kartenboden(pos.get_2d())->get_hoehe();
		if(besch->get_max_height()!=0  &&  height>besch->get_max_height()) {
			error_msg = "bridge is too high for its type!";
			return koord3d::invalid;
		}
		// and if ground is above bridge / double slopes
		if (height < -Z_TILE_STEP) {
			break; // to trigger the right error message
		}
		gr1 = welt->lookup(pos + koord3d(0, 0, Z_TILE_STEP));
		if(  gr1  &&  gr1->get_weg_hang()==hang_t::flach  &&  length>=min_length) {
			if(  gr1->get_typ()==grund_t::boden  ) {
				// on slope ok, but not on other bridges
				if(  gr1->has_two_ways()  ) {
					// crossing => then we must be sure we have our way in our direction
					const weg_t* weg = gr1->get_weg(wegtyp);
					if(  weg  &&  ribi_t::ist_gerade(ribi_typ(zv)|weg->get_ribi_unmasked())  ) {
						// way goes in our direction already
						return gr1->get_pos();
					}
				}
				else {
					// no or one way
					const weg_t* weg = gr1->get_weg_nr(0);
					if(  weg==NULL  ||  weg->get_waytype()==wegtyp
						|| (crossing_logic_t::get_crossing( wegtyp, weg->get_waytype(), besch->get_topspeed(), weg->get_besch()->get_topspeed(), welt->get_timeline_year_month())  &&  (ribi_typ(zv)&weg->get_ribi_unmasked())==0)
					) {
						return gr1->get_pos();
					}
				}
			}
			else if(  gr1->get_typ()==grund_t::monorailboden  ) {
				// check if we can connect to elevated way
				const weg_t* weg = gr1->get_weg_nr(0);
				if(  weg==NULL  ||  weg->get_waytype()==wegtyp
//					|| (crossing_logic_t::get_crossing(wegtyp, weg->get_waytype()))
//					|| (wegtyp==powerline_wt)
				) {
					return gr1->get_pos();
				}
			}
		}
		gr2 = welt->lookup(pos);
		if(gr2  &&  (gr2->get_typ()==grund_t::boden  ||  gr2->get_typ()==grund_t::monorailboden)) {
			ribi_t::ribi ribi = ribi_t::keine;
			if(wegtyp != powerline_wt) {
				if(gr2->has_two_ways()) {
					if (gr2->ist_uebergang()  ||  wegtyp!=road_wt) {
						// full ribi -> build no bridge here
						ribi = 15;
					}
					else {
						// road/tram on the tile, we have to check both ribis.
						ribi = gr2->get_weg_nr(0)->get_ribi_unmasked() | gr2->get_weg_nr(1)->get_ribi_unmasked();
					}
				}
				else {
					ribi = gr2->get_weg_ribi_unmasked(wegtyp);
				}
			} else if (leitung_t *lt = gr2->find<leitung_t>()) {
				ribi = lt->get_ribi();
			}
			if(gr2->get_grund_hang()==hang_t::flach) {
				if(!gr2->get_halt().is_bound()  &&  gr2->get_depot()==NULL  &&  gr2->get_typ()==grund_t::boden  &&  length>=min_length) {
					if(  ai_bridge  &&  !gr2->hat_wege()  &&  !gr2->get_leitung()  ) {
						return pos;
					}
					if(ribi_t::ist_einfach(ribi) && koord(ribi) == zv) {
						// end with ramp, end way is already built
						return pos;
					}
					if(ribi==ribi_t::keine && wegtyp!=powerline_wt && gr2->hat_weg(wegtyp)) {
						// end with ramp, end way is already built but ribi's are missing
						return pos;
					}
					if (ribi == ribi_t::keine && wegtyp == powerline_wt && gr2->find<leitung_t>()) {
						// end with ramp, end way is already built but ribi's are missing - for powerlines
						return pos;
					}
					if (ribi==ribi_t::keine && min_length>0 && !gr2->hat_wege() && gr2->find<leitung_t>()==NULL) {
						// end has no ways and powerlines but min-length is specified
						return pos;
					}
				}
			}
			else {
				if(length < min_length) {
					return koord3d::invalid;
				}
				if((wegtyp != powerline_wt) == (gr2->get_leitung() != NULL)) {
					// end would be on slope with powerline if building way bridge (and vice versa)
					error_msg =  "Tile not empty.";
					return koord3d::invalid;
				}
				if(ribi_t::ist_einfach(ribi)  &&  koord(ribi) == zv) {
					// end on slope with way
					return pos;
				}
				if(!ribi  &&  gr2->get_grund_hang()==hang_typ(zv)) {
					// end on slope, way (or ribi) is missing
					// check if another way is blocking us
					if(wegtyp!=powerline_wt && (!gr2->hat_wege() || gr2->hat_weg(wegtyp))) {
						return pos;
					}
					if (wegtyp == powerline_wt && (!gr2->hat_wege() || gr2->find<leitung_t>())) {
						return pos;
					}
				}
			}
		}
	} while(  !gr1  &&  // no bridge is crossing
		(!gr2 || (gr2->get_grund_hang()==hang_t::flach  &&  gr2->get_weg_hang()==hang_t::flach)  ||  gr2->get_hoehe()<pos.z )  &&  // ground stays below bridge
		(!ai_bridge || length <= welt->get_settings().way_max_bridge_len) // not too long in case of AI
		);

	error_msg = "A bridge must start on a way!";
	return koord3d::invalid;
}


bool brueckenbauer_t::ist_ende_ok(spieler_t *sp, const grund_t *gr)
{
	if(gr->get_typ()!=grund_t::boden  &&  gr->get_typ()!=grund_t::monorailboden) {
		return false;
	}
	if(gr->ist_uebergang()) {
		return false;
	}
	ding_t *d=gr->obj_bei(0);
	if (d != NULL) {
		if (d->ist_entfernbar(sp)!=NULL) {
			return false;
		}
	}
	if(gr->get_halt().is_bound()) {
		return false;
	}
	if(gr->get_depot()) {
		return false;
	}
	if(gr->find<senke_t>()!=NULL  ||  gr->find<pumpe_t>()!=NULL) {
		return false;
	}
	return true;
}


const char *brueckenbauer_t::baue( karte_t *welt, spieler_t *sp, koord pos, const bruecke_besch_t *besch)
{
	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(  !(gr  &&  besch)  ) {
		return "";
	}

	DBG_MESSAGE("brueckenbauer_t::baue()", "called on %d,%d for bridge type '%s'",
	pos.x, pos.y, besch->get_name());

	koord zv;
	ribi_t::ribi ribi = ribi_t::keine;
	const weg_t *weg = gr->get_weg(besch->get_waytype());
	leitung_t *lt = gr->find<leitung_t>();

	if(besch->get_waytype()==powerline_wt) {
		if (gr->hat_wege()) {
			return "Tile not empty.";
		}
		if(lt) {
			ribi = lt->get_ribi();
		}
	}
	else {
		if (lt) {
			return "Tile not empty.";
		}
		if(  gr->has_two_ways()  &&  !gr->ist_uebergang()  ) {
			// If road and tram, we have to check both ribis.
			ribi = gr->get_weg_nr(0)->get_ribi_unmasked() | gr->get_weg_nr(1)->get_ribi_unmasked();

			if(  besch->get_waytype()  !=  road_wt  ) {
				// only road bridges allowed here.
				ribi = 0;
			}
		}
		else if (weg) {
			ribi = weg->get_ribi_unmasked();
		}
	}

	if((!lt && !weg) || !ist_ende_ok(sp, gr)) {
		DBG_MESSAGE("brueckenbauer_t::baue()", "no way %x found",besch->get_waytype());
		return "A bridge must start on a way!";
	}

	if(gr->kann_alle_obj_entfernen(sp)) {
		return "Tile not empty.";
	}

	if(!hang_t::ist_wegbar(gr->get_grund_hang())) {
		return "Bruecke muss an\neinfachem\nHang beginnen!\n";
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
	koord3d end = finde_ende(welt, gr->get_pos(), zv, besch, msg );

	// found something?
	if(msg!=NULL) {
DBG_MESSAGE("brueckenbauer_t::baue()", "end not ok");
		return msg;
	}

	// check ownership
	grund_t * gr_end = welt->lookup(end);
	if(gr_end->kann_alle_obj_entfernen(sp)) {
		return "Tile not empty.";
	}

	if(sp && !sp->can_afford(besch->get_preis()))
	{
		return "That would exceed\nyour credit limit.";
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

	// Anfang und ende sind geprueft, wir konnen endlich bauen
	// "Beginning and end are approved, we can finally build" (Google)
	if(besch->get_waytype()==powerline_wt) {
		baue_bruecke(welt, sp, gr->get_pos(), end, zv, besch, lt->get_besch() );
	}
	else {
		baue_bruecke(welt, sp, gr->get_pos(), end, zv, besch, weg->get_besch() );
	}
	if(besch->get_waytype() == road_wt)
	{
		welt->set_recheck_road_connexions();
	}
	return NULL;
}


void brueckenbauer_t::baue_bruecke(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	ribi_t::ribi ribi = 0;
	weg_t *weg=NULL;	// =NULL to keep compiler happy

	DBG_MESSAGE("brueckenbauer_t::baue()", "build from %s", pos.get_str() );
	baue_auffahrt(welt, sp, pos, zv, besch);
	if(besch->get_waytype() != powerline_wt) {
		ribi = welt->lookup(pos)->get_weg_ribi_unmasked(besch->get_waytype());
	} else {
		ribi = ribi_typ(zv);
	}
	pos = pos + zv;

	while(pos.get_2d()!=end.get_2d()) {
		brueckenboden_t *bruecke = new brueckenboden_t(welt, pos + koord3d(0, 0, Z_TILE_STEP), 0, 0);
		welt->access(pos.get_2d())->boden_hinzufuegen(bruecke);
		if(besch->get_waytype() != powerline_wt) {
			weg = weg_t::alloc(besch->get_waytype());
			weg->set_besch(weg_besch);
			bruecke->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		}
		else {
			leitung_t *lt = new leitung_t(welt, bruecke->get_pos(), sp);
			bruecke->obj_add( lt );
			lt->laden_abschliessen();
		}
		bruecke_t *br = new bruecke_t(welt, bruecke->get_pos(), sp, besch, besch->get_simple(ribi));
		bruecke->obj_add(br);
		bruecke->calc_bild();
		br->laden_abschliessen();
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","at (%i,%i)",pos.x,pos.y);
		if(besch->get_pillar()>0) {
			// make a new pillar here
			if(besch->get_pillar()==1  ||  (pos.x*zv.x+pos.y*zv.y)%besch->get_pillar()==0) {
				grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","h1=%i, h2=%i",pos.z,gr->get_pos().z);
				sint16 height = (pos.z - gr->get_pos().z)/Z_TILE_STEP+1;
				while(height-->0) {
					if(height_scaling(TILE_HEIGHT_STEP*height)<=127) {
						// eventual more than one part needed, if it is too high ...
						gr->obj_add( new pillar_t(welt,gr->get_pos(),sp,besch,besch->get_pillar(ribi), height_scaling(TILE_HEIGHT_STEP*height)) );
					}
				}
			}
		}

		pos = pos + zv;
	}

	// must determine end tile: on a slope => likely need auffahrt
	bool need_auffahrt = pos.z == welt->lookup(end)->get_vmove(ribi_typ(-zv));
	if(need_auffahrt) {  //"Need ramp" (Google)
		if (weg_t const* const w = welt->lookup(end)->get_weg(weg_besch->get_wtyp())) {
			need_auffahrt &= w->get_besch()->get_styp()!=weg_besch_t::elevated;
		}
	}

	if(need_auffahrt) {
		// not ending at a bridge
		baue_auffahrt(welt, sp, pos, -zv, besch);
	}
	else {
		// ending on a slope/elevated way
		grund_t *gr=welt->lookup(end);
		if(besch->get_waytype() != powerline_wt) {
			// just connect to existing way
			ribi = ribi_t::rueckwaerts( ribi_typ(zv) );
			if(  !gr->weg_erweitern( besch->get_waytype(), ribi )  ) {
				// builds new way
				weg = weg_t::alloc( besch->get_waytype() );
				weg->set_besch( weg_besch );
				spieler_t::accounting( sp, -gr->neuen_weg_bauen( weg, ribi, sp ) -weg->get_besch()->get_preis(), end.get_2d(), COST_CONSTRUCTION);
			}
			gr->calc_bild();
		}
		else {
			leitung_t *lt = gr->get_leitung();
			if(  lt==NULL  ) {
				lt = new leitung_t( welt, end, sp );
				spieler_t::accounting(sp, -weg_besch->get_preis(), gr->get_pos().get_2d(), COST_CONSTRUCTION);
				gr->obj_add(lt);
				lt->set_besch(weg_besch);
				lt->laden_abschliessen();
			}
			lt->calc_neighbourhood();
		}
	}
}

void brueckenbauer_t::baue_auffahrt(karte_t* welt, spieler_t* sp, koord3d end, koord zv, const bruecke_besch_t* besch)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi_neu;
	brueckenboden_t *bruecke;
	int weg_hang = 0;
	hang_t::typ grund_hang = alter_boden->get_grund_hang();
	bruecke_besch_t::img_t img;

	ribi_neu = ribi_typ(zv);
	if(grund_hang == hang_t::flach) {
		weg_hang = hang_typ(zv);    // nordhang - suedrampe
	}

	bruecke = new brueckenboden_t(welt, end, grund_hang, weg_hang);
	// add the ramp
	if(bruecke->get_grund_hang() == hang_t::flach) {
		img = besch->get_rampe(ribi_neu);
	}
	else {
		img = besch->get_start(ribi_neu);
	}

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
			spieler_t::accounting(sp, -bruecke->neuen_weg_bauen( weg, ribi_neu, sp ), end.get_2d(), COST_CONSTRUCTION);
		}
		weg->set_max_speed( besch->get_topspeed() );
		weg->set_max_weight( besch->get_max_weight() );
		//weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());
		weg->add_way_constraints(besch->get_way_constraints());
	} else {

		leitung_t *lt = bruecke->get_leitung();
		if(!lt) {
			lt = new leitung_t(welt, bruecke->get_pos(), sp); //"leading" (Google)
			bruecke->obj_add( lt );
		}
		else {
			// remove maintenance - it will be added in leitung_t::laden_abschliessen
			spieler_t::add_maintenance( sp, -lt->get_besch()->get_wartung());
		}
		// connect to neighbor tiles and networks, add maintenance
		lt->laden_abschliessen();
	}
	bruecke_t *br = new bruecke_t(welt, end, sp, besch, img);
	bruecke->obj_add( br );
	br->laden_abschliessen();
	bruecke->calc_bild();
}




const char *brueckenbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wegtyp)
{
	marker_t    marker(welt->get_groesse_x(),welt->get_groesse_y());
	slist_tpl<koord3d> end_list;
	slist_tpl<koord3d> part_list;
	slist_tpl<koord3d> tmp_list;
	const char    *msg;

	tmp_list.insert(pos);
	marker.markiere(welt->lookup(pos));
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
				zv = koord(hang_t::gegenueber(from->get_grund_hang()));
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
			if(  (zv == koord::invalid  ||  zv == koord::nsow[r])  &&  from->get_neighbour(to, delete_wegtyp, koord::nsow[r])  &&  !marker.ist_markiert(to)  &&  to->ist_bruecke()  ) {
				if(  wegtyp != powerline_wt  ||  (to->find<bruecke_t>()  &&  to->find<bruecke_t>()->get_besch()->get_waytype() == powerline_wt)  ) {
					tmp_list.insert(to->get_pos());
					marker.markiere(to);
				}
			}
		}
	} while (!tmp_list.empty());

	// now delete the bridge
	while (!part_list.empty()) {
		pos = part_list.remove_first();

		grund_t *gr = welt->lookup(pos);

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
		ding_t *p;
		while ((p = gr->find<pillar_t>()) != 0) {
			p->entferne(p->get_besitzer());
			delete p;
		}
		// refresh map
		reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
	}
	// finally delete the bridge ends
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		if(wegtyp==powerline_wt) {
			ding_t *br;
			while ((br = gr->find<bruecke_t>()) != 0) {
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
				lt = new leitung_t(welt, gr->get_pos(), old_owner);
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

			// removes single signals, bridge head, pedestrians, stops, changes catenary etc
			weg_t *weg=gr->get_weg_nr(1);
			if(weg) {
				gr->remove_everything_from_way(sp,weg->get_waytype(),bridge_ribi);
			}
			gr->remove_everything_from_way(sp,wegtyp,bridge_ribi);	// removes stop and signals correctly

			// corrects the ways
			weg=gr->get_weg(wegtyp);
			if(weg) {
				// may fail, if this was the last tile
				weg->set_besch(weg->get_besch());
				weg->set_ribi( ribi );
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(welt, pos, gr->get_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.get_2d())->kartenboden_setzen( gr_new );

		if(  wegtyp == powerline_wt  ) {
			gr_new->get_leitung()->calc_neighbourhood(); // Recalc the image. calc_bild() doesn't do the right job...
		}
	}

	welt->set_dirty();
	return NULL;
}
