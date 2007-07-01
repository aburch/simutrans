/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include <string.h>

#include "../simdebug.h"
#include "brueckenbauer.h"

#include "../simworld.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simhalt.h"
#include "../besch/sound_besch.h"
#include "../simplay.h"
#include "../simskin.h"

#include "../boden/boden.h"
#include "../boden/brueckenboden.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/kanal.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_parameter_waehler.h"

#include "../besch/bruecke_besch.h"
#include "../besch/skin_besch.h"

#include "../dings/bruecke.h"
#include "../dings/pillar.h"
#include "../dings/signal.h"

#include "../dataobj/translator.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"

static vector_tpl<const bruecke_besch_t*> bruecken(16);
static stringhashtable_tpl<const bruecke_besch_t *> bruecken_by_name;


/**
 * Registers a new bridge type
 * @author V. Meyer, Hj. Malthaner
 */
void brueckenbauer_t::register_besch(const bruecke_besch_t *besch)
{
  bruecken_by_name.put(besch->gib_name(), besch);
  bruecken.push_back(besch);
}



const bruecke_besch_t *brueckenbauer_t::gib_besch(const char *name)
{
  return bruecken_by_name.get(name);
}



bool brueckenbauer_t::laden_erfolgreich()
{
	bool strasse_da = false;
	bool schiene_da = false;

	for(unsigned int i = 0; i < bruecken.get_count(); i++) {
		const bruecke_besch_t* besch = bruecken[i];

		if(besch && besch->gib_waytype() == track_wt) {
			schiene_da = true;
		}
		if(besch && besch->gib_waytype() == road_wt) {
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



/**
 * Find a matchin bridge
 * @author Hj. Malthaner
 */
const bruecke_besch_t *
brueckenbauer_t::find_bridge(const waytype_t wtyp, const uint32 min_speed,const uint16 time)
{
	const bruecke_besch_t *find_besch=NULL;

	for(unsigned int i = 0; i < bruecken.get_count(); i++) {
		const bruecke_besch_t* besch = bruecken[i];
		if(besch->gib_waytype() == wtyp) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				if(find_besch==NULL  ||
					(find_besch->gib_topspeed()<min_speed  &&  find_besch->gib_topspeed()<besch->gib_topspeed())  ||
					(besch->gib_topspeed()>=min_speed  &&  besch->gib_wartung()<find_besch->gib_wartung())
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
	return a->gib_topspeed() < b->gib_topspeed();
}


/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void
brueckenbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
         const waytype_t wtyp,
         const int sound_ok,
         const int sound_ko,
         const karte_t *welt)
{
	const uint16 time = welt->get_timeline_year_month();

	// list of matching types (sorted by speed)
	vector_tpl<const bruecke_besch_t*> matching;
	for (vector_tpl<const bruecke_besch_t*>::const_iterator i = bruecken.begin(), end = bruecken.end(); i != end; ++i) {
		const bruecke_besch_t* b = *i;
		if (b->gib_waytype() == wtyp && (
					time == 0 ||
					(b->get_intro_year_month() <= time && time < b->get_retire_year_month())
				)) {
			matching.push_back(b);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_bridges);

	const sint32 shift_maintanance = (welt->ticks_bits_per_tag-18);	// same costs per intervall => correct display

	// now sorted ...
	for (vector_tpl<const bruecke_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		const bruecke_besch_t* besch = *i;
		char buf[256];

		if(besch->gib_max_length()>0) {
			sprintf(buf, "%s, %d$ (%d$), %dkm/h, %dkm",
				  translator::translate(besch->gib_name()),
				  besch->gib_preis()/100,
				  (besch->gib_wartung()<<shift_maintanance)/100,
				  besch->gib_topspeed(),
				  besch->gib_max_length());
		}
		else {
			sprintf(buf, "%s, %d$ (%d$), %dkm/h",
				  translator::translate(besch->gib_name()),
				  besch->gib_preis()/100,
				  (besch->gib_wartung()<<shift_maintanance)/100,
				  besch->gib_topspeed());
		}

		wzw->add_param_tool(baue,
			  (const void*) besch,
			  karte_t::Z_PLAN,
			  sound_ok,
			  sound_ko,
			  besch->gib_cursor()->gib_bild_nr(1),
			  besch->gib_cursor()->gib_bild_nr(0),
			  cstring_t(buf));
	}
}




koord3d
brueckenbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, waytype_t wegtyp)
{
	const grund_t *gr1; // auf Brückenebene
	const grund_t *gr2; // unter Brückenebene
	do {
		pos = pos + zv;
		if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
			return koord3d::invalid;
		}
		gr1 = welt->lookup(pos + koord3d(0, 0, Z_TILE_STEP));
		if(gr1 && gr1->gib_weg_hang()==hang_t::flach) {
			return gr1->gib_pos();        // Ende an Querbrücke
		}
		gr2 = welt->lookup(pos);
		if(gr2) {
			ribi_t::ribi ribi = gr2->gib_weg_ribi_unmasked(wegtyp);

			if(gr2->gib_grund_hang() == hang_t::flach) {
				if(ribi_t::ist_einfach(ribi) && koord(ribi) == zv) {
					// Ende mit Rampe - Endschiene vorhanden
					return pos;
				}
				if(!ribi && gr2->hat_weg(wegtyp)) {
					// Ende mit Rampe - Endschiene hat keine ribis
					return pos;
				}
			}
			else {
				if(ribi_t::ist_einfach(ribi) && koord(ribi) == zv) {
					// Ende am Hang - Endschiene vorhanden
					return pos;
				}
				if(!ribi && gr2->gib_grund_hang() == hang_typ(zv)) {
					// Ende am Hang - Endschiene fehlt oder hat keine ribis
					// Wir prüfen noch, ob uns dort ein anderer Weg stört
					if(!gr2->hat_wege() || gr2->hat_weg(wegtyp)) {
						return pos;
					}
				}
			}
		}
	} while(!gr1 &&                             // keine Brücke im Weg
		(!gr2 || gr2->gib_grund_hang()==hang_t::flach  ||  gr2->gib_hoehe()<pos.z ) ); // Boden kommt nicht hoch

	return koord3d::invalid;
}



bool brueckenbauer_t::ist_ende_ok(spieler_t *sp, const grund_t *gr)
{
	if(gr->gib_typ()!=grund_t::boden  &&  gr->gib_typ()!=grund_t::monorailboden) {
		return false;
	}
	if(gr->ist_uebergang()) {
		return false;
	}
	ding_t *d=gr->obj_bei(0);
	if (d != NULL) {
		const spieler_t* owner = d->gib_besitzer();
		if (owner != sp && owner != NULL) {
			return false;
		}
	}
	if(gr->gib_halt().is_bound()) {
		return false;
	}
	if(gr->gib_depot()) {
		return false;
	}
	return true;
}


int
brueckenbauer_t::baue(spieler_t *sp, karte_t *welt, koord pos, value_t param)
{
	const bruecke_besch_t *besch = (const bruecke_besch_t *)param.p;

	if(!besch) {
DBG_MESSAGE("brueckenbauer_t::baue()", "no description for bridge type");
		return false;
	}
DBG_MESSAGE("brueckenbauer_t::baue()", "called on %d,%d for bridge type '%s'",
	pos.x, pos.y, besch->gib_name());

	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}

	const grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	koord zv;
	ribi_t::ribi ribi = ribi_t::keine;
	const weg_t *weg = gr->gib_weg(besch->gib_waytype());

	if(!weg || !ist_ende_ok(sp, gr)) {
DBG_MESSAGE("brueckenbauer_t::baue()", "no way %x found",besch->gib_waytype());
		if(welt->get_active_player()==sp) {
			create_win(-1, -1, MESG_WAIT, new news_img("A bridge must start on a way!"), w_autodelete);
		}
		return false;
	}

	if(gr->kann_alle_obj_entfernen(sp)) {
		create_win(-1, -1, MESG_WAIT, new news_img("Tile not empty."), w_autodelete);
		return false;
	}

	if(!hang_t::ist_wegbar(gr->gib_grund_hang())) {
		create_win(-1, -1, MESG_WAIT, new news_img("Bruecke muss an\neinfachem\nHang beginnen!\n"), w_autodelete);
		return false;
	}

	if(gr->gib_grund_hang() == hang_t::flach) {
		ribi = weg->gib_ribi_unmasked();
		if(!ribi_t::ist_einfach(ribi)) {
			ribi = 0;
		}
	}
	else {
		ribi = ribi_typ(gr->gib_grund_hang());
		if(weg->gib_ribi_unmasked() & ~ribi) {
			ribi = 0;
		}
	}
	if(!ribi) {
		create_win(-1, -1, MESG_WAIT, new news_img("A bridge must start on a way!"), w_autodelete);
		return false;
	}

	zv = koord(ribi_t::rueckwaerts(ribi));
	// Brückenende suchen
	koord3d end = finde_ende(welt, gr->gib_pos(), zv, besch->gib_waytype());

	if(besch->gib_max_length()>0  &&  abs_distance(gr->gib_pos().gib_2d(),end.gib_2d())>(unsigned)besch->gib_max_length()) {
		create_win(-1, -1, MESG_WAIT, new news_img("Bridge is too long for this type!\n"), w_autodelete);
		return false;
	}

	// pruefe ob bruecke auf strasse/schiene endet
	if(!welt->ist_in_kartengrenzen(end.gib_2d()) || !ist_ende_ok(sp, welt->lookup(end))) {
DBG_MESSAGE("brueckenbauer_t::baue()", "end not ok");
		create_win(-1, -1, MESG_WAIT, new news_img("A bridge must end on a way!"), w_autodelete);
		return false;
	}

	grund_t * gr_end = welt->lookup(end);
	if(gr_end->kann_alle_obj_entfernen(sp)) {
		create_win(-1, -1, MESG_WAIT, new news_img("Tile not empty."), w_autodelete);
		return false;
	}
	// Anfang und ende sind geprueft, wir konnen endlich bauen
	return baue_bruecke(welt, sp, gr->gib_pos(), end, zv, besch, weg->gib_besch() );
}



bool brueckenbauer_t::baue_bruecke(karte_t *welt, spieler_t *sp,
                 koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	ribi_t::ribi ribi;
	weg_t *weg=NULL;	// =NULL to keep compiler happy

	DBG_MESSAGE("brueckenbauer_t::baue()", "build from %d,%d", pos.x, pos.y);
	baue_auffahrt(welt, sp, pos, zv, besch, weg_besch );

	ribi = welt->lookup(pos)->gib_weg_ribi_unmasked(besch->gib_waytype());
	pos = pos + zv;

	while(pos.gib_2d()!=end.gib_2d()) {
		brueckenboden_t *bruecke = new  brueckenboden_t(welt, pos + koord3d(0, 0, Z_TILE_STEP), 0, 0);
		weg = weg_t::alloc(besch->gib_waytype());
		weg->setze_besch(weg_besch);
		weg->setze_max_speed(besch->gib_topspeed());
		welt->access(pos.gib_2d())->boden_hinzufuegen(bruecke);
		bruecke->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);

		sp->add_maintenance( -weg_besch->gib_wartung() );
		sp->add_maintenance( besch->gib_wartung() );

		bruecke->obj_add(new bruecke_t(welt, bruecke->gib_pos(), sp, besch, besch->gib_simple(ribi)));
		bruecke->calc_bild();

//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","at (%i,%i)",pos.x,pos.y);
		if(besch->gib_pillar()>0) {
			// make a new pillar here
			if(besch->gib_pillar()==1  ||  (pos.x*zv.x+pos.y*zv.y)%besch->gib_pillar()==0) {
				grund_t *gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","h1=%i, h2=%i",pos.z,gr->gib_pos().z);
				sint8 height = (pos.z - gr->gib_pos().z)/Z_TILE_STEP+1;
				while(height-->0) {
					// can overflow, did never happened apparently
					if(height_scaling(TILE_HEIGHT_STEP*height)<=127) {
						// eventual more than one part needed, if it is too high ...
						gr->obj_add( new pillar_t(welt,gr->gib_pos(),sp,besch,besch->gib_pillar(ribi), height_scaling(TILE_HEIGHT_STEP*height)) );
					}
				}
			}
		}

		pos = pos + zv;
	}

	if(pos.z==end.z) {
		// not ending at a bridge
		baue_auffahrt(welt, sp, pos, -zv, besch, weg_besch);
	}
	else {
		// just connect to existing way
		grund_t *gr=welt->lookup(end);
		gr->weg_erweitern(besch->gib_waytype(),ribi_t::doppelt(ribi));
	}
	return true;
}

void brueckenbauer_t::baue_auffahrt(karte_t* welt, spieler_t* sp, koord3d end, koord zv, const bruecke_besch_t* besch, const weg_besch_t*)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi_neu;
	brueckenboden_t *bruecke;
	int weg_hang = 0;
	hang_t::typ grund_hang = alter_boden->gib_grund_hang();
	bruecke_besch_t::img_t img;

	ribi_neu = ribi_typ(zv);
	if(grund_hang == hang_t::flach) {
		weg_hang = hang_typ(zv);    // nordhang - suedrampe
	}

	bruecke = new brueckenboden_t(welt, end, grund_hang, weg_hang);
	// add the ramp
	if(bruecke->gib_grund_hang() == hang_t::flach) {
		img = besch->gib_rampe(ribi_neu);
	}
	else {
		img = besch->gib_start(ribi_neu);
	}

	weg_t *weg=alter_boden->gib_weg( besch->gib_waytype() );
	// take care of everything on that tile ...
	bruecke->take_obj_from( alter_boden );
	welt->access(end.gib_2d())->kartenboden_setzen( bruecke );
	if(weg) {
		// has already a way
		bruecke->weg_erweitern(besch->gib_waytype(), ribi_t::doppelt(ribi_neu));
	}
	else {
		// needs still one
		weg = weg_t::alloc( besch->gib_waytype() );
		bruecke->neuen_weg_bauen( weg, ribi_t::doppelt(ribi_neu), sp );
	}
	weg->setze_max_speed( besch->gib_topspeed() );
	bruecke_t *br = new bruecke_t(welt, end, sp, besch, img);
	bruecke->obj_add( br );
	br->laden_abschliessen();
	bruecke->calc_bild();
}




const char *
brueckenbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wegtyp)
{
	marker_t    marker(welt->gib_groesse_x(),welt->gib_groesse_y());
	slist_tpl<koord3d> end_list;
	slist_tpl<koord3d> part_list;
	slist_tpl<koord3d> tmp_list;
	const char    *msg;

	// Erstmal das ganze Außmaß der Brücke bestimmen und sehen,
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
			// Der Grund ist Brückenanfang/-ende - hier darf nur in
			// eine Richtung getestet werden.
			if(from->gib_grund_hang() != hang_t::flach) {
				zv = koord(hang_t::gegenueber(from->gib_grund_hang()));
			}
			else {
				zv = koord(from->gib_weg_hang());
			}
			end_list.insert(pos);
		}
		else if(from->ist_bruecke()) {
			part_list.insert(pos);
		}
		// Alle Brückenteile auf Entfernbarkeit prüfen!
		msg = from->kann_alle_obj_entfernen(sp);

		if(msg != NULL  ||  (from->gib_halt().is_bound()  &&  from->gib_halt()->gib_besitzer()!=sp)) {
			return "Die Bruecke ist nicht frei!\n";
		}

		// Nachbarn raussuchen
		for(int r = 0; r < 4; r++) {
			if((zv == koord::invalid || zv == koord::nsow[r]) &&  	from->get_neighbour(to, wegtyp, koord::nsow[r]) &&  !marker.ist_markiert(to) &&  to->ist_bruecke()) {
				tmp_list.insert(to->gib_pos());
				marker.markiere(to);
			}
		}
	} while (!tmp_list.empty());

	// Jetzt geht es ans löschen der Brücke
	while (!part_list.empty()) {
		pos = part_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// we may have a second way here ...
		gr->obj_loesche_alle(sp);
		welt->access(pos.gib_2d())->boden_entfernen(gr);
		delete gr;

		// finally delete all pillars (if there)
		gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
		ding_t *p;
		while ((p = gr->find<pillar_t>()) != 0) {
			p->entferne(sp);
			delete p;
		}
	}

	// Und die Brückenenden am Schluß
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp);

		if(gr->gib_grund_hang() != hang_t::flach) {
			ribi &= ~ribi_typ(hang_t::gegenueber(gr->gib_grund_hang()));
		}
		else {
			ribi &= ~ribi_typ(gr->gib_weg_hang());
		}

		// removes single signals, bridge head, pedestrians, stops, changes catenary etc
		gr->remove_everything_from_way(sp,wegtyp,ribi);	// removes stop and signals correctly

		// corrects the ways
		weg_t *weg=gr->gib_weg_nr(0);
		if(weg) {
			// may fail, if this was the last tile
			weg->setze_besch(weg->gib_besch());
			weg->setze_ribi( ribi );
			if(gr->gib_weg_nr(1)) {
				gr->gib_weg_nr(1)->setze_ribi( ribi );
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(welt, pos, gr->gib_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.gib_2d())->kartenboden_setzen(gr_new );
	}

	welt->setze_dirty();
	return NULL;
}
