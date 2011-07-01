/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"

#include "tunnelbauer.h"
#include "../besch/intro_dates.h"

#include "../gui/karte.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../player/simplay.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simevent.h"

#include "../besch/tunnel_besch.h"

#include "../boden/boden.h"
#include "../boden/tunnelboden.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/strasse.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/koord3d.h"

#include "../dings/tunnel.h"
#include "../dings/signal.h"
#include "../dings/zeiger.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_waehler.h"

#include "wegbauer.h"
#include "../tpl/vector_tpl.h"


static stringhashtable_tpl<tunnel_besch_t *> tunnel_by_name;


void tunnelbauer_t::register_besch(tunnel_besch_t *besch)
{
	// avoid duplicates with same name
	if(  tunnel_by_name.remove(besch->get_name())  ) {
		dbg->warning( "tunnelbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	// add the tool
	wkz_tunnelbau_t *wkz = new wkz_tunnelbau_t();
	wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
	wkz->cursor = besch->get_cursor()->get_bild_nr(0);
	wkz->set_default_param( besch->get_name() );
	werkzeug_t::general_tool.append( wkz );
	besch->set_builder( wkz );
	tunnel_by_name.put(besch->get_name(), besch);
}

stringhashtable_tpl <tunnel_besch_t *> * tunnelbauer_t::get_all_tunnels()
{
	return &tunnel_by_name;
}


// now we have to convert old tunnel to new ones ...
bool tunnelbauer_t::laden_erfolgreich()
{
	stringhashtable_iterator_tpl<tunnel_besch_t *>iter(tunnel_by_name);
	while(  iter.next()  ) {
		tunnel_besch_t* besch = iter.get_current_value();

		if(besch->get_topspeed()==0) {
			// old style, need to convert
			if(strcmp(besch->get_name(),"RoadTunnel")==0) {
				besch->wegtyp = (uint8)road_wt;
				besch->topspeed = 120;
				besch->maintenance = 500;
				besch->preis = 200000;
				besch->intro_date = DEFAULT_INTRO_DATE*12;
				besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
			}
			else {
				besch->wegtyp = (uint8)track_wt;
				besch->topspeed = 280;
				besch->maintenance = 500;
				besch->preis = 200000;
				besch->intro_date = DEFAULT_INTRO_DATE*12;
				besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
			}
		}
	}
	return true;
}



const tunnel_besch_t *tunnelbauer_t::get_besch(const char *name)
{
	return tunnel_by_name.get(name);
}



/**
 * Find a matchin tunnel
 * @author Hj. Malthaner
 */
const tunnel_besch_t *tunnelbauer_t::find_tunnel(const waytype_t wtyp, const sint32 min_speed, const uint16 time)
{
	const tunnel_besch_t *find_besch=NULL;

	stringhashtable_iterator_tpl<tunnel_besch_t *>iter(tunnel_by_name);
	while(  iter.next()  ) {
		tunnel_besch_t* besch = iter.get_current_value();

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


static bool compare_tunnels(const tunnel_besch_t* a, const tunnel_besch_t* b)
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
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void tunnelbauer_t::fill_menu(werkzeug_waehler_t* wzw, const waytype_t wtyp, sint16 /*sound_ok*/, const karte_t* welt)
{
	const uint16 time=welt->get_timeline_year_month();
	vector_tpl<const tunnel_besch_t*> matching(tunnel_by_name.get_count());

	stringhashtable_iterator_tpl<tunnel_besch_t *>iter(tunnel_by_name);
	while(  iter.next()  ) {
		tunnel_besch_t* besch = iter.get_current_value();
		if (besch->get_waytype() == wtyp && (
					time == 0 ||
					(besch->get_intro_year_month() <= time && time < besch->get_retire_year_month())
				)) {
			matching.insert_ordered(besch, compare_tunnels);
		}
	}
	// now sorted ...
	for (vector_tpl<const tunnel_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		wzw->add_werkzeug( (*i)->get_builder() );
	}
}



/* now construction stuff */


koord3d
tunnelbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, waytype_t wegtyp)
{
	const grund_t *gr;

	while(true) {
		pos = pos + zv;
		if(!welt->ist_in_kartengrenzen(pos.get_2d())) {
			return koord3d::invalid;
		}

#ifdef ONLY_TUNNELS_BELOW_GROUND
		// check if ground is below tunnel level
		gr = welt->lookup_kartenboden(pos.get_2d());
		if(  gr->get_hoehe() < pos.z  ){
			return koord3d::invalid;
		}
#endif

		gr = welt->lookup(pos);
		if(gr) {
			if(  gr->get_typ() != grund_t::boden  ||  gr->get_grund_hang() != hang_typ(-zv)  ||  gr->is_halt()  ||  gr->get_leitung()) {
				// must end on boden_t and correct slope, not on halts and powerlines
				return koord3d::invalid;
			}
			if(  gr->has_two_ways()  &&  wegtyp != road_wt  ) {
				// Only road tunnels allowed here.
				return koord3d::invalid;
			}

			ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp);
			if(  ribi && koord(ribi) == zv  ) {
				// There is already a way (with correct ribi)
				return pos;
			}
			if(  !ribi  ) {
				// Ende am Hang - Endschiene fehlt oder hat keine ribis
				// Wir prüfen noch, ob uns dort ein anderer Weg stört
				if(  !gr->hat_wege()  ||  gr->hat_weg(wegtyp)  ) {
					return pos;
				}
			}
			return koord3d::invalid;  // Was im Weg (schräger Hang oder so)
		}
		// tunnel slope underneath?
		gr = welt->lookup(pos +koord3d(0,0,-1));
		if (gr && gr->get_grund_hang()!=hang_t::flach) {
			return koord3d::invalid;
		}

		// Alles frei - weitersuchen
	}
}



const char *tunnelbauer_t::baue( karte_t *welt, spieler_t *sp, koord pos, const tunnel_besch_t *besch, bool full_tunnel )
{
	assert( besch );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr==NULL) {
		return "Tunnel must start on single way!";
	}

	koord zv;
	const waytype_t wegtyp = besch->get_waytype();
	const weg_t *weg = gr->get_weg(wegtyp);

	if(  gr->get_typ() != grund_t::boden  ||  gr->is_halt()  ||  gr->get_leitung()) {
		return "Tunnel must start on single way!";
	}
	if(!hang_t::ist_einfach(gr->get_grund_hang())) {
		return "Tunnel muss an\neinfachem\nHang beginnen!\n";
	}
	// If there is a way on this tile, it must have the right ribis.
	if( weg  &&  (weg->get_ribi_unmasked() & ~ribi_t::rueckwaerts(ribi_typ(gr->get_grund_hang())))  ) {
		return "Tunnel must start on single way!";
	}
	if(  gr->has_two_ways()  &&  wegtyp != road_wt  ) {
		return "Tunnel must start on single way!";
	}
	zv = koord(gr->get_grund_hang());

	// Tunnelende suchen
	koord3d end = koord3d::invalid;
	if(full_tunnel) {
		end = finde_ende(welt, gr->get_pos(), zv, wegtyp);
	}
	else {
		end = gr->get_pos()+zv;
		if(welt->lookup(end)  ||  welt->lookup_kartenboden(pos+zv)->get_hoehe()<=end.z) {
			end = koord3d::invalid;
		}
	}

	if(sp && !sp->can_afford(besch->get_preis()))
	{
		return "That would exceed\nyour credit limit.";
	}

	if(!welt->ist_in_kartengrenzen(end.get_2d())) {
		return "Tunnel must start on single way!";
	}

	// check ownership
	if (const grund_t *gr_end = welt->lookup(end)) {
		if (weg_t *weg_end = gr_end->get_weg(wegtyp)) {
			if (weg_end->ist_entfernbar(sp)!=NULL) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
	}

	// Anfang und ende sind geprueft, wir konnen endlich bauen
	// "examine whether the tunnel on road / rail ends" (Google)
	if(!baue_tunnel(welt, sp, gr->get_pos(), end, zv, besch)) {
		return "Ways not connected";
	}

	if(besch->get_waytype() == road_wt)
	{
		welt->set_recheck_road_connexions();
	}
	return NULL;
}



bool tunnelbauer_t::baue_tunnel(karte_t *welt, spieler_t *sp, koord3d start, koord3d end, koord zv, const tunnel_besch_t *besch)
{
	ribi_t::ribi ribi;
	weg_t *weg;
	koord3d pos = start;
	int cost = 0, maint = 0;
	waytype_t wegtyp = besch->get_waytype();

DBG_MESSAGE("tunnelbauer_t::baue()","build from (%d,%d,%d) to (%d,%d,%d) ", pos.x, pos.y, pos.z, end.x, end.y, end.z );

	// now we search a matching way for the tunnels top speed
	const weg_besch_t *weg_besch = besch->get_weg_besch();
	if(weg_besch==NULL) {
		// ignore timeline to get consistent results
		weg_besch = wegbauer_t::weg_search( wegtyp, besch->get_topspeed(), besch->get_max_weight(), 0, weg_t::type_flat );
	}

	baue_einfahrt(welt, sp, pos, zv, besch, NULL, cost, maint);

	ribi = ribi_typ(-zv);
	// don't move on to next tile if only one tile long
	if(  end != start  ) {
		pos = pos + zv;
	}
	// calc new back image for the ground
	if(grund_t::underground_mode) {
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		gr->calc_bild();
		gr->set_flag(grund_t::dirty);
	}

	// Now we build the invisible part
	while(pos!=end) {
		assert(pos.get_2d()!=end.get_2d());
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		weg = weg_t::alloc(besch->get_waytype());
		weg->set_besch(weg_besch);
		weg->set_max_speed(besch->get_topspeed());
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		weg->set_max_weight(besch->get_max_weight());
		//weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());
		weg->add_way_constraints(besch->get_way_constraints());
		
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		tunnel->calc_bild();
		tunnel->set_flag(grund_t::dirty);
		assert(!tunnel->ist_karten_boden());
		maint += besch->get_wartung()-weg->get_besch()->get_wartung(); 
		cost += besch->get_preis();
		pos = pos + zv;
	}

	// if end is above ground construct an exit
	if(welt->lookup_kartenboden(end.get_2d())->get_pos().z==end.z) {
		baue_einfahrt(welt, sp, pos, -zv, besch, weg_besch, cost, maint);
		// calc new back image for the ground
		if (end!=start && grund_t::underground_mode) {
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d()-zv);
			gr->calc_bild();
			gr->set_flag(grund_t::dirty);
		}
	}
	else {
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		weg = weg_t::alloc(besch->get_waytype());
		weg->set_besch(weg_besch);
		weg->set_max_speed(besch->get_topspeed());
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		weg->set_max_weight(besch->get_max_weight());
		tunnel->neuen_weg_bauen(weg, ribi, sp);
		//weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());
		weg->add_way_constraints(besch->get_way_constraints());
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		tunnel->calc_bild();
		tunnel->set_flag(grund_t::dirty);
		assert(!tunnel->ist_karten_boden());
		maint += besch->get_wartung()-weg->get_besch()->get_wartung(); 
		cost += besch->get_preis();
	}

	spieler_t::add_maintenance(sp, maint);
	spieler_t::accounting(sp, -cost, start.get_2d(), COST_CONSTRUCTION);
	return true;
}



const weg_besch_t *tunnelbauer_t::baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, int &cost, int &maint)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi = alter_boden->get_weg_ribi_unmasked(besch->get_waytype()) | ribi_typ(zv);

	tunnelboden_t *tunnel = new tunnelboden_t(welt, end, alter_boden->get_grund_hang());
	tunnel->obj_add(new tunnel_t(welt, end, sp, besch));

	weg_t *weg = alter_boden->get_weg( besch->get_waytype() );
	// take care of everything on that tile ...
	tunnel->take_obj_from( alter_boden );
	welt->access(end.get_2d())->kartenboden_setzen( tunnel );
	if(weg) {
		// has already a way
		tunnel->weg_erweitern(besch->get_waytype(), ribi);
	}
	else {
		// needs still one
		weg = weg_t::alloc( besch->get_waytype() );
		if(  weg_besch  ) {
			weg->set_besch( weg_besch );
		}
		tunnel->neuen_weg_bauen( weg, ribi, sp );
	}
	weg->set_max_speed( besch->get_topspeed() );
	weg->set_max_weight( besch->get_max_weight() );
	//weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());
	weg->add_way_constraints(besch->get_way_constraints());
	tunnel->calc_bild();
	tunnel->set_flag(grund_t::dirty);

	maint += besch->get_wartung() - weg->get_besch()->get_wartung();

	// remove sidewalk
	weg_t *str = tunnel->get_weg( road_wt );
	if( str  &&  str->hat_gehweg()) {
		str->set_gehweg(false);
	}

	tunnel->calc_bild();
	tunnel->set_flag(grund_t::dirty);

	// Auto-connect to a way outside the new tunnel mouth
	grund_t *ground_outside = welt->lookup(end-zv);
	if( !ground_outside ) {
		ground_outside = welt->lookup(end-zv+koord3d(0,0,-1));
		if(  ground_outside  &&  ground_outside->get_grund_hang() != tunnel->get_grund_hang()  ) {
			// Not the correct slope.
			ground_outside = NULL;
		}
	}
	if( ground_outside ) {
		weg_t *way_outside = ground_outside->get_weg( besch->get_waytype() );
		if( way_outside ) {
			// use the check_owner routine of wegbauer_t (not spieler_t!), needs an instance
			wegbauer_t bauigel(welt, sp);
			bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), way_outside->get_besch());
			long dummy;
			if(bauigel.is_allowed_step(tunnel, ground_outside, &dummy)) {
				tunnel->weg_erweitern(besch->get_waytype(), ribi_typ(-zv));
				ground_outside->weg_erweitern(besch->get_waytype(), ribi_typ(zv));
			}
		}
		if (besch->get_waytype()==water_wt  &&  ground_outside->ist_wasser()) {
			// connect to the sea
			tunnel->weg_erweitern(besch->get_waytype(), ribi_typ(-zv));
			ground_outside->calc_bild(); // to recalculate ribis
		}
	}

	cost += besch->get_preis();
	return weg->get_besch();
}




const char *
tunnelbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d start, waytype_t wegtyp)
{
	marker_t    marker(welt->get_groesse_x(),welt->get_groesse_y());
	slist_tpl<koord3d>  end_list;
	slist_tpl<koord3d>  part_list;
	slist_tpl<koord3d>  tmp_list;
	const char    *msg;
	koord3d   pos = start;

	// Erstmal das ganze Außmaß des Tunnels bestimmen und sehen,
	// ob uns was im Weg ist.
	tmp_list.insert(pos);
	marker.markiere(welt->lookup(pos));

	do {
		pos = tmp_list.remove_first();

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(from->ist_karten_boden()) {
			// Der Grund ist Tunnelanfang/-ende - hier darf nur in
			// eine Richtung getestet werden.
			zv = koord(from->get_grund_hang());
			end_list.insert(pos);
		}
		else {
			part_list.insert(pos);
		}
		// Alle Tunnelteile auf Entfernbarkeit prüfen!
		msg = from->kann_alle_obj_entfernen(sp);

		if(msg != NULL) {
		return "Der Tunnel ist nicht frei!\n";
		}
		// Nachbarn raussuchen
		for(int r = 0; r < 4; r++) {
			if((zv == koord::invalid || zv == koord::nsow[r]) &&
				from->get_neighbour(to, wegtyp, koord::nsow[r]) &&
				!marker.ist_markiert(to))
			{
				tmp_list.insert(to->get_pos());
				marker.markiere(to);
			}
		}
	} while (!tmp_list.empty());

	// Jetzt geht es ans löschen der Tunnel
	while (!part_list.empty()) {
		pos = part_list.remove_first();
		grund_t *gr = welt->lookup(pos);
		// remove the second way first in the tunnel
		if(gr->get_weg_nr(1)) {
			gr->remove_everything_from_way(sp,gr->get_weg_nr(1)->get_waytype(),ribi_t::keine);
		}
		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// remove everything else
		gr->obj_loesche_alle(sp);
		gr->mark_image_dirty();
		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;

		reliefkarte_t::get_karte()->calc_map_pixel( pos.get_2d() );
	}

	// Und die Tunnelenden am Schluß
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		ribi_t::ribi mask = gr->get_grund_hang()!=hang_t::flach ? ~ribi_typ(gr->get_grund_hang()) : ~ribi_typ(hang_t::gegenueber(gr->get_weg_hang()));

		// remove the second way first in the tunnel
		if(gr->get_weg_nr(1)) {
			gr->remove_everything_from_way(sp,gr->get_weg_nr(1)->get_waytype(),gr->get_weg_nr(1)->get_ribi_unmasked() & mask);
		}
		// removes single signals, bridge head, pedestrians, stops, changes catenary etc
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp) & mask;

		tunnel_t *t = gr->find<tunnel_t>();
		uint8 broad_type = t->get_broad_type();
		gr->remove_everything_from_way(sp,wegtyp,ribi);	// removes stop and signals correctly

		// remove tunnel portals
		t = gr->find<tunnel_t>();
		if(t) {
			t->entferne(sp);
			delete t;
		}

		if( broad_type ) {
			hang_t::typ hang = gr->get_grund_hang();
			ribi_t::ribi dir = ribi_t::rotate90( ribi_typ( hang ) );
			if( broad_type & 1 ) {
				const grund_t *gr_l = welt->lookup(pos + dir);
				tunnel_t* tunnel_l = gr_l ? gr_l->find<tunnel_t>() : NULL;
				if( tunnel_l ) {
					tunnel_l->calc_bild();
				}
			}
			if( broad_type & 2 ) {
				const grund_t *gr_r = welt->lookup(pos - dir);
				tunnel_t* tunnel_r = gr_r ? gr_r->find<tunnel_t>() : NULL;
				if( tunnel_r ) {
					tunnel_r->calc_bild();
				}
			}
		}

		// corrects the ways
		weg_t *weg=gr->get_weg_nr(0);
		if(weg) {
			// fails if it was preivously the last ribi
			weg->set_besch(weg->get_besch());
			weg->set_ribi( ribi );
			if(gr->get_weg_nr(1)) {
				gr->get_weg_nr(1)->set_ribi( ribi );
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(welt, pos, gr->get_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.get_2d())->kartenboden_setzen(gr_new );

		// recalc image of ground
		grund_t *kb = welt->access(pos.get_2d()+koord(gr_new->get_grund_hang()))->get_kartenboden();
		kb->calc_bild();
		kb->set_flag(grund_t::dirty);
	}
	return NULL;
}
