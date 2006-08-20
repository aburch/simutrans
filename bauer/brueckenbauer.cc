/*
 * simbridge.cpp
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "../simdebug.h"
#include "brueckenbauer.h"

#include "../simworld.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simsfx.h"
#include "../simplay.h"
#include "../simskin.h"
#include "../blockmanager.h"

#include "../boden/boden.h"
#include "../boden/brueckenboden.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_parameter_waehler.h"

#include "../besch/bruecke_besch.h"
#include "../besch/skin_besch.h"

#include "../dings/bruecke.h"
#include "../dings/pillar.h"

#include "../dataobj/translator.h"

#include "../tpl/minivec_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif



static minivec_tpl <const bruecke_besch_t *> bruecken (16);
static stringhashtable_tpl<const bruecke_besch_t *> bruecken_by_name;


/**
 * Registers a new bridge type
 * @author V. Meyer, Hj. Malthaner
 */
void brueckenbauer_t::register_besch(const bruecke_besch_t *besch)
{
  bruecken_by_name.put(besch->gib_name(), besch);
  bruecken.append(besch);
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
    const bruecke_besch_t *besch = bruecken.get(i);

    if(besch && besch->gib_wegtyp() == weg_t::schiene) {
      schiene_da = true;
    }
    if(besch && besch->gib_wegtyp() == weg_t::strasse) {
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
brueckenbauer_t::find_bridge(const weg_t::typ wtyp, const uint32 min_speed,const uint16 time)
{
	const bruecke_besch_t *find_besch=NULL;

	// list of matching types (sorted by speed)
	slist_tpl <const bruecke_besch_t *> matching;

	for(unsigned int i = 0; i < bruecken.get_count(); i++) {
		const bruecke_besch_t *besch = bruecken.get(i);
		if(besch->gib_wegtyp() == wtyp) {
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




/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void
brueckenbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
         const weg_t::typ wtyp,
         const int sound_ok,
         const int sound_ko,
         const uint16 time)
{
	// list of matching types (sorted by speed)
	slist_tpl <const bruecke_besch_t *> matching;

	for(unsigned int i = 0; i < bruecken.get_count(); i++) {
		const bruecke_besch_t *besch = bruecken.get(i);
		if(besch->gib_wegtyp() == wtyp) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				// add int sorted
				unsigned j;
				for( j=0;  j<matching.count();  j++  ) {
					// insert sorted
					if(matching.at(j)->gib_topspeed()>besch->gib_topspeed()) {
						matching.insert(besch,j);
						break;
					}
				}
				if(j==matching.count()) {
					matching.append(besch);
				}
			}
		}
	}

	// now sorted ...
	while(matching.count()>0) {
		const bruecke_besch_t * besch = matching.at(0);
		matching.remove_at(0);
		int icon = besch->gib_cursor()->gib_bild_nr(1);
		char buf[128];

		if(besch->gib_max_length()>0) {
			sprintf(buf, "%s, %d$ (%d$), %dkm/h, %dkm",
				  translator::translate(besch->gib_name()),
				  besch->gib_preis()/100,
				  besch->gib_wartung()/100,
				  besch->gib_topspeed(),
				  besch->gib_max_length());
		}
		else {
			sprintf(buf, "%s, %d$ (%d$), %dkm/h",
				  translator::translate(besch->gib_name()),
				  besch->gib_preis()/100,
				  besch->gib_wartung()/100,
				  besch->gib_topspeed());
		}

		wzw->add_param_tool(baue,
			  (const void*) besch,
			  karte_t::Z_PLAN,
			  sound_ok,
			  sound_ko,
			  icon,
			  besch->gib_cursor()->gib_bild_nr(0),
			  cstring_t(buf));
	}
}




koord3d
brueckenbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, weg_t::typ wegtyp)
{
	const grund_t *gr1; // auf Brückenebene
	const grund_t *gr2; // unter Brückenebene
	do {
		pos = pos + zv;
		if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
			return koord3d::invalid;
		}
		gr1 = welt->lookup(pos + koord3d(0, 0, 16));
		if(gr1 && gr1->gib_grund_hang() == hang_t::flach) {
			//return gr1->gib_pos();        // Ende an Querbrücke
			return koord3d::invalid;
		}
		gr2 = welt->lookup(pos);
		if(gr2) {
			ribi_t::ribi ribi = gr2->gib_weg_ribi_unmasked(wegtyp);

			if(gr2->gib_grund_hang() == hang_t::flach) {
				if(ribi_t::ist_einfach(ribi) && koord(ribi) == zv) {
					// Ende mit Rampe - Endschiene vorhanden
					return pos;
				}
				if(!ribi && gr2->gib_weg(wegtyp)) {
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
					if(!gr2->hat_wege() || gr2->gib_weg(wegtyp)) {
						return pos;
					}
				}
			}
		}
	} while(!gr1 &&                             // keine Brücke im Weg
		(!gr2 || gr2->gib_grund_hang() == hang_t::flach) ); // Boden kommt nicht hoch

	return koord3d::invalid;
}

bool brueckenbauer_t::ist_ende_ok(spieler_t *sp, const grund_t *gr)
{
    if(gr->gib_typ() != grund_t::boden) {
  return false;
    }
    if(gr->ist_uebergang()) {
  return false;
    }
    if(gr->gib_besitzer() != sp && gr->gib_besitzer() != NULL) {
  return false;
    }
    if(gr->gib_halt().is_bound()) {
  return false;
    }
    if(gr->gib_depot()) {
  return false;
    }
    return true;
}

/*
 * Bauen mit der ersten passenden Brücke
*/
int brueckenbauer_t::baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp)
{
    for(int i = 0; i < bruecken.get_count(); i++) {
  const bruecke_besch_t *besch = bruecken.get(i);

  if(besch && besch->gib_wegtyp() == wegtyp) {
      return baue(sp, welt, pos, (long)besch);
  }
    }
    return false;
}


/* built bridge with right top speed
 */
int brueckenbauer_t::baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp,int top_speed)
{
  const bruecke_besch_t *besch=NULL;
  for(int i = 0; i < bruecken.get_count(); i++) {
    if(bruecken.get(i)->gib_wegtyp() == wegtyp) {
      if(besch==NULL
          ||  (besch->gib_topspeed()<top_speed  &&  besch->gib_topspeed()<bruecken.get(i)->gib_topspeed())
          ||  (bruecken.get(i)->gib_topspeed()>=top_speed
            &&  (besch->gib_wartung()>bruecken.get(i)->gib_wartung()
              ||  (besch->gib_wartung()==bruecken.get(i)->gib_wartung()  &&  besch->gib_preis()>bruecken.get(i)->gib_preis()) ) ) ) {
        // cheaper, faster and less mantainance
        besch = bruecken.get(i);
      }
    }
  }
  if(besch) {
    return baue(sp, welt, pos, (long)besch);
  }
  return false;
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
	const weg_t *weg = gr->gib_weg(besch->gib_wegtyp());

	if(!weg || !ist_ende_ok(sp, gr)) {
		if(welt->get_active_player()==sp) {
			if(besch->gib_wegtyp() == weg_t::strasse) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nStraße beginnen!\n"), w_autodelete);
			} else {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nSchiene beginnen!\n"), w_autodelete);
			}
		}
		return false;
	}

	bool hat_oberleitung = (gr->suche_obj(ding_t::oberleitung) != 0);
	if(gr->obj_count() > (hat_oberleitung ? 1 : 0)) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
		return false;
	}

	if(!hang_t::ist_wegbar(gr->gib_grund_hang())) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss an\neinfachem\nHang beginnen!\n"), w_autodelete);
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
		if(besch->gib_wegtyp() == weg_t::strasse) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nStrassenende\nbeginnen!\n"), w_autodelete);
		} else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nSchienenende\nbeginnen!\n"), w_autodelete);
		}
		return false;
	}

	zv = koord(ribi_t::rueckwaerts(ribi));
	// Brückenende suchen
	koord3d end = finde_ende(welt, gr->gib_pos(), zv, besch->gib_wegtyp());

	if(besch->gib_max_length()>0  &&  abs_distance(gr->gib_pos().gib_2d(),end.gib_2d())>besch->gib_max_length()) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bridge is too long for this type!\n"), w_autodelete);
		return false;
	}

	// pruefe ob bruecke auf strasse/schiene endet
	if(!welt->ist_in_kartengrenzen(end.gib_2d()) || !ist_ende_ok(sp, welt->lookup(end))) {
		if(besch->gib_wegtyp() == weg_t::strasse) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nStrassenende enden!\n"), w_autodelete);
		} else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Bruecke muss auf\nSchienenende enden!\n"), w_autodelete);
		}
		return false;
	}

	grund_t * gr_end = welt->lookup(end);
	hat_oberleitung = (gr_end->suche_obj(ding_t::oberleitung) != 0);
	if(gr_end->obj_count() > (hat_oberleitung ? 1 : 0)) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
		return false;
	}
	// Anfang und ende sind geprueft, wir konnen endlich bauen
	return baue_bruecke(welt, sp, gr->gib_pos(), end, zv, besch, weg->gib_besch() );
}



bool brueckenbauer_t::baue_bruecke(karte_t *welt, spieler_t *sp,
                 koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	ribi_t::ribi ribi;
	blockhandle_t bs1;
	weg_t *weg;

	DBG_MESSAGE("brueckenbauer_t::baue()", "build from %d,%d", pos.x, pos.y);
	baue_auffahrt(welt, sp, pos, zv, besch, weg_besch );

	ribi = welt->lookup(pos)->gib_weg_ribi_unmasked(besch->gib_wegtyp());
	if(besch->gib_wegtyp() == weg_t::schiene) {
		bs1 = ((schiene_t *)welt->lookup(pos)->gib_weg(besch->gib_wegtyp()))->gib_blockstrecke();
	}
	pos = pos + zv;

	while(pos.gib_2d() != end.gib_2d()) {
		brueckenboden_t *bruecke = new  brueckenboden_t(welt, pos + koord3d(0, 0, 16), 0, 0);

		if(besch->gib_wegtyp() == weg_t::schiene) {
			weg = new schiene_t(welt,450);
			((schiene_t *)weg)->setze_blockstrecke( bs1 );
		}
		else {
			weg = new strasse_t(welt,450);
		}

		weg->setze_max_speed(besch->gib_topspeed());
		welt->access(pos.gib_2d())->boden_hinzufuegen(bruecke);
		bruecke->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		bruecke->obj_add(new bruecke_t(welt, bruecke->gib_pos(), 0, sp, besch, besch->gib_simple(ribi)));

//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","at (%i,%i)",pos.x,pos.y);
		if(besch->gib_pillar()>0) {
			// make a new pillar here
			if(besch->gib_pillar()==1  ||  (pos.x+pos.y)%besch->gib_pillar()==0) {
				grund_t *gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
//DBG_MESSAGE("bool brueckenbauer_t::baue_bruecke()","h1=%i, h2=%i",pos.z,gr->gib_pos().z);
				int height = (pos.z - gr->gib_pos().z)/16+1;
				while(height-->0) {
					// eventual more than one part needed, if it is too high ...
					gr->obj_add( new pillar_t(welt,gr->gib_pos(),sp,besch,besch->gib_pillar(ribi), height_scaling(height*16) ) );
				}
			}
		}

		pos = pos + zv;
	}

	baue_auffahrt(welt, sp, pos, -zv, besch, weg_besch);
	return true;
}

void
brueckenbauer_t::baue_auffahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch)
{
	grund_t *alter_boden = welt->lookup(end);
	weg_t *weg;
	ribi_t::ribi ribi_alt = alter_boden->gib_weg_ribi_unmasked(besch->gib_wegtyp());
	ribi_t::ribi ribi_neu;
	brueckenboden_t *bruecke;
	int weg_hang = 0;
	hang_t::typ grund_hang = alter_boden->gib_grund_hang();
	bruecke_besch_t::img_t img;
	int yoff;

	ribi_neu = ribi_typ(zv);
	if(grund_hang == hang_t::flach) {
		weg_hang = hang_typ(zv);    // nordhang - suedrampe
	}
	bruecke = new brueckenboden_t(welt, end, grund_hang, weg_hang);

	weg_t *alter_weg = alter_boden->gib_weg(besch->gib_wegtyp());
	ding_t *sig = NULL;

	if(besch->gib_wegtyp() == weg_t::schiene) {
		weg = new schiene_t(welt);
		if(!alter_weg) {
			weg->setze_besch(weg_besch);
			sp->buche(weg_besch->gib_preis(), alter_boden->gib_pos().gib_2d(), COST_CONSTRUCTION);
		} else {
			weg->setze_besch(alter_weg->gib_besch());
			blockhandle_t bs = ((schiene_t *)alter_weg)->gib_blockstrecke();
			((schiene_t *)weg)->setze_blockstrecke( bs );
			sig = (ding_t *)alter_boden->suche_obj(ding_t::signal);
			if(sig) { // Signal aufheben - kommt auf den neuen Boden!
				alter_boden->obj_remove(sig, sp);
			}
		}
	}
	else {
		weg = new strasse_t(welt);
		if(!alter_weg) {
			weg->setze_besch(weg_besch);
			sp->buche(weg_besch->gib_preis(), alter_boden->gib_pos().gib_2d(), COST_CONSTRUCTION);
		}
		else {
			weg->setze_besch(alter_weg->gib_besch());
		}
	}
	weg->setze_max_speed( besch->gib_topspeed() );

	welt->access(end.gib_2d())->kartenboden_setzen( bruecke, false );
	bruecke->neuen_weg_bauen(weg, ribi_alt | ribi_neu, sp);

	if(sp!=NULL) {
		// no undo possible anymore
		sp->init_undo(besch->gib_wegtyp(),0);
	}
	if(besch->gib_wegtyp() == weg_t::schiene) {
		blockmanager::gib_manager()->neue_schiene(welt, bruecke, sig);
	}
	if(bruecke->gib_grund_hang() == hang_t::flach) {
		yoff = 0;
		img = besch->gib_rampe(ribi_neu);
	}
	else {
		yoff = -16;
		img = besch->gib_start(ribi_neu);
	}
	bruecke->obj_add(new bruecke_t(welt, end, yoff, sp, besch, img));
}




const char *
brueckenbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d pos, weg_t::typ wegtyp)
{
	blockmanager *bm = blockmanager::gib_manager();

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
		else {
			part_list.insert(pos);
		}
		// Alle Brückenteile auf Entfernbarkeit prüfen!
		msg = from->kann_alle_obj_entfernen(sp);

		if(msg != NULL) {
			return "Die Brücke ist nicht frei!\n";
		}

		// Nachbarn raussuchen
		for(int r = 0; r < 4; r++) {
			if((zv == koord::invalid || zv == koord::nsow[r]) &&
			from->get_neighbour(to, wegtyp, koord::nsow[r]) &&
			!marker.ist_markiert(to)) {
				tmp_list.insert(to->gib_pos());
				marker.markiere(to);
			}
		}
	} while(!tmp_list.is_empty());

	// Jetzt geht es ans löschen der Brücke
	while(!part_list.is_empty()) {
		pos = part_list.remove_first();

		grund_t *gr = welt->lookup(pos);

		if(wegtyp == weg_t::schiene) {
			bm->entferne_schiene(welt, pos);
		}
		gr->weg_entfernen(wegtyp, false);
		gr->obj_loesche_alle(sp);

		welt->access(pos.gib_2d())->boden_entfernen(gr);
		delete gr;

		//  sp->buche(besch->gib_preis(), pos.gib_2d(), COST_CONSTRUCTION);

		// finally delete all pillars (if there)
		gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
		pillar_t *p;
		while((p=dynamic_cast<pillar_t *>(gr->suche_obj(ding_t::pillar)))!=0) {
			gr->obj_remove(p,sp);
			delete p;
		}
	}

	// Und die Brückenenden am Schluß
	while(!end_list.is_empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		ding_t *sig = NULL;
		ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp);

		if(gr->gib_grund_hang() != hang_t::flach) {
			ribi &= ~ribi_typ(hang_t::gegenueber(gr->gib_grund_hang()));
		}
		else {
			ribi &= ~ribi_typ(gr->gib_weg_hang());
		}
		const weg_besch_t *weg_besch=gr->gib_weg(wegtyp)->gib_besch();
		if(wegtyp == weg_t::schiene) {
			sig = gr->suche_obj(ding_t::signal);
			if(sig) { // Signal aufheben - kommt auf den neuen Boden!
				gr->obj_remove(sig, sp);
				((schiene_t *)gr->gib_weg(weg_t::schiene))->gib_blockstrecke()->entferne_signal((signal_t *)sig);
			}
			bm->entferne_schiene(welt, gr->gib_pos());
		}

		gr->weg_entfernen(wegtyp, false);
		gr->obj_loesche_alle(sp);

		gr = new boden_t(welt, pos);
		welt->access(pos.gib_2d())->kartenboden_setzen(gr, false);

		// Neuen Boden wieder mit Weg versehen
		if(wegtyp == weg_t::schiene) {
			weg_t *weg = new schiene_t(welt);
			weg->setze_besch(weg_besch);
			gr->neuen_weg_bauen(weg, ribi, sp);
			bm->neue_schiene(welt, gr, sig);
		}
		else {
			weg_t *weg = new strasse_t(welt);
			weg->setze_besch(weg_besch);
			gr->neuen_weg_bauen(weg, ribi, sp);
		}
		gr->calc_bild();
	}

	welt->setze_dirty();
	return NULL;
}
