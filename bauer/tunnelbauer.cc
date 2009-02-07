/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
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
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/koord3d.h"

#include "../dings/tunnel.h"
#include "../dings/signal.h"
#include "../dings/zeiger.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_waehler.h"

#include "wegbauer.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"


static vector_tpl<tunnel_besch_t*> tunnel(16);
static stringhashtable_tpl<tunnel_besch_t *> tunnel_by_name;



void
tunnelbauer_t::register_besch(tunnel_besch_t *besch)
{
	tunnel_by_name.put(besch->get_name(), besch);
	tunnel.push_back(besch);
}



// now we have to convert old tunnel to new ones ...
bool
tunnelbauer_t::laden_erfolgreich()
{
	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		tunnel_besch_t* besch = *i;
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



const tunnel_besch_t *
tunnelbauer_t::get_besch(const char *name)
{
	return tunnel_by_name.get(name);
}



/**
 * Find a matchin tunnel
 * @author Hj. Malthaner
 */
const tunnel_besch_t *
tunnelbauer_t::find_tunnel(const waytype_t wtyp, const uint32 min_speed,const uint16 time)
{
	const tunnel_besch_t *find_besch=NULL;

	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
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
void tunnelbauer_t::fill_menu(werkzeug_waehler_t* wzw, const waytype_t wtyp, const karte_t* welt)
{
	static stringhashtable_tpl<wkz_tunnelbau_t *> tunnel_tool;

	const uint16 time=welt->get_timeline_year_month();
	vector_tpl<const tunnel_besch_t*> matching;
	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
		if (besch->get_waytype() == wtyp && (
					time == 0 ||
					(besch->get_intro_year_month() <= time && time < besch->get_retire_year_month())
				)) {
			matching.push_back(besch);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_tunnels);

	// now sorted ...
	for (vector_tpl<const tunnel_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
		wkz_tunnelbau_t *wkz = tunnel_tool.get(besch->get_name());
		if(wkz==NULL) {
			// not yet in hashtable
			wkz = new wkz_tunnelbau_t();
			wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
			wkz->cursor = besch->get_cursor()->get_bild_nr(0);
			wkz->default_param = besch->get_name();
			tunnel_tool.put(besch->get_name(),wkz);
		}
		wzw->add_werkzeug( (werkzeug_t*)wkz );
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
		gr = welt->lookup(pos.get_2d())->get_kartenboden();
		if(  gr->get_hoehe() < pos.z  ){
			return koord3d::invalid;
		}
#endif

		gr = welt->lookup(pos);
		if(gr) {
			if(  gr->get_typ() != grund_t::boden  ||  gr->get_grund_hang() != hang_typ(-zv)  ) {
				// must end on boden_t and correct slope
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
		// Alles frei - weitersuchen
	}
}



const char *tunnelbauer_t::baue( karte_t *welt, spieler_t *sp, koord pos, const tunnel_besch_t *besch )
{
	assert( besch  &&  !grund_t::underground_mode );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr==NULL) {
		return "Tunnel must start on single way!";
	}

	koord zv;
	const waytype_t wegtyp = besch->get_waytype();
	const weg_t *weg = gr->get_weg(wegtyp);

	if(!weg || gr->get_typ() != grund_t::boden) {
		return "Tunnel must start on single way!";
	}
	if(!hang_t::ist_einfach(gr->get_grund_hang())) {
		return "Tunnel muss an\neinfachem\nHang beginnen!\n";
	}
	if(weg->get_ribi_unmasked() & ~ribi_t::rueckwaerts(ribi_typ(gr->get_grund_hang()))) {
		return "Tunnel must end on single way!";
	}
	zv = koord(gr->get_grund_hang());

	// Tunnelende suchen
	koord3d end = koord3d::invalid;
	if(event_get_last_control_shift()!=2) {
		end = finde_ende(welt, gr->get_pos(), zv, wegtyp);
	} else {
		end = gr->get_pos()+zv;
		if(welt->lookup(end)  ||  welt->lookup_kartenboden(pos+zv)->get_hoehe()<=end.z) {
			end = koord3d::invalid;
		}
	}

	// pruefe ob Tunnel auf strasse/schiene endet
	if(!welt->ist_in_kartengrenzen(end.get_2d())) {
		return "Tunnel must end on single way!";
	}

	// Anfang und ende sind geprueft, wir konnen endlich bauen
	if(!baue_tunnel(welt, sp, gr->get_pos(), end, zv, besch)) {
		return "Ways not connected";
	}
	return NULL;
}



bool tunnelbauer_t::baue_tunnel(karte_t *welt, spieler_t *sp, koord3d start, koord3d end, koord zv, const tunnel_besch_t *besch)
{
	ribi_t::ribi ribi;
	weg_t *weg;
	koord3d pos = start;
	int cost = 0;
	const weg_besch_t *weg_besch;
	waytype_t wegtyp = besch->get_waytype();

DBG_MESSAGE("tunnelbauer_t::baue()","build from (%d,%d,%d) to (%d,%d,%d) ", pos.x, pos.y, pos.z, end.x, end.y, end.z );

	// get a way for enty/exit
	weg = welt->lookup(start)->get_weg(wegtyp);
	if(weg==NULL) {
		weg = welt->lookup(end)->get_weg(wegtyp);
	}
	if(weg==NULL) {
		dbg->error("tunnelbauer_t::baue_tunnel()","No way found!");
		return false;
	}
	weg_besch = weg->get_besch();

	baue_einfahrt(welt, sp, pos, zv, besch, weg_besch, cost);

	ribi = welt->lookup(pos)->get_weg_ribi_unmasked(wegtyp);
	// don't move on to next tile if only one tile long
	if(end!=start) pos = pos + zv;

	// Now we build the invisible part
	while(pos!=end) {
		assert(pos.get_2d()!=end.get_2d());
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		// use the fastest way
		weg = weg_t::alloc(besch->get_waytype());
		weg->set_besch(weg_besch);
		weg->set_max_speed(besch->get_topspeed());
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp,  -weg->get_besch()->get_wartung() );
		spieler_t::add_maintenance( sp,  besch->get_wartung() );
		cost += besch->get_preis();
		pos = pos + zv;
	}

	// if end is above ground construct an exit
	if(welt->lookup(end.get_2d())->get_kartenboden()->get_pos().z==end.z) {
		baue_einfahrt(welt, sp, pos, -zv, besch, weg_besch, cost);
	} else {
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		// use the fastest way
		weg = weg_t::alloc(besch->get_waytype());
		weg->set_besch(weg_besch);
		weg->set_max_speed(besch->get_topspeed());
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp,  -weg->get_besch()->get_wartung() );
		spieler_t::add_maintenance( sp,  besch->get_wartung() );
		cost += besch->get_preis();
	}

	spieler_t::accounting(sp, -cost, start.get_2d(), COST_CONSTRUCTION);
	return true;
}



void
tunnelbauer_t::baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, int &cost)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi = alter_boden->get_weg_ribi_unmasked(besch->get_waytype()) | ribi_typ(zv);

	tunnelboden_t *tunnel = new tunnelboden_t(welt, end, alter_boden->get_grund_hang());
	tunnel->obj_add(new tunnel_t(welt, end, sp, besch));

	weg_t *weg=alter_boden->get_weg( besch->get_waytype() );
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
		tunnel->neuen_weg_bauen( weg, ribi, sp );
	}
	weg->set_max_speed( besch->get_topspeed() );
	tunnel->calc_bild();

	if(sp!=NULL) {
		spieler_t::add_maintenance( sp,  -weg_besch->get_wartung() );
		spieler_t::add_maintenance( sp,  besch->get_wartung() );
	}
	cost += besch->get_preis();
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

	assert(!end_list.empty());

	// Jetzt geht es ans löschen der Tunnel
	while (!part_list.empty()) {
		pos = part_list.remove_first();
		grund_t *gr = welt->lookup(pos);
		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// we may have a second way here ...
		gr->obj_loesche_alle(sp);
		welt->access(pos.get_2d())->boden_entfernen(gr);
		reliefkarte_t::get_karte()->calc_map_pixel( pos.get_2d() );
		delete gr;
	}

	// Und die Tunnelenden am Schluß
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp);
		if(gr->get_grund_hang()!=hang_t::flach) {
			ribi &= ~ribi_typ(gr->get_grund_hang());
		}
		else {
			ribi &= ~ribi_typ(hang_t::gegenueber(gr->get_weg_hang()));
		}

		// removes single signals, bridge head, pedestrians, stops, changes catenary etc
		gr->remove_everything_from_way(sp,wegtyp,ribi);	// removes stop and signals correctly

		// remove tunnel portals
		tunnel_t *t = gr->find<tunnel_t>();
		if(t) {
			t->entferne(sp);
			delete t;
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
	}
	return NULL;
}
