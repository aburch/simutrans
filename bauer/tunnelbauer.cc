
/*
 * simtunnel.cpp
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"

#include "tunnelbauer.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../blockmanager.h"


#include "../besch/tunnel_besch.h"
#include "../besch/spezial_obj_tpl.h"

#include "../boden/boden.h"
#include "../boden/tunnelboden.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"

#include "../dataobj/umgebung.h"

#include "../dings/tunnel.h"

#include "../gui/messagebox.h"

#include "wegbauer.h"


const tunnel_besch_t *tunnelbauer_t::strassentunnel = NULL;
const tunnel_besch_t *tunnelbauer_t::schienentunnel = NULL;

static spezial_obj_tpl<tunnel_besch_t> tunnel_objekte[] = {
    { &tunnelbauer_t::strassentunnel, "RoadTunnel" },
    { &tunnelbauer_t::schienentunnel, "RailTunnel" },
    { NULL, NULL }
};


bool tunnelbauer_t::laden_erfolgreich()
{
    warne_ungeladene(tunnel_objekte, 2);
    return true;
}

bool tunnelbauer_t::register_besch(const tunnel_besch_t *besch)
{
    return ::register_besch(tunnel_objekte, besch);
}


koord3d
tunnelbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, weg_t::typ wegtyp)
{
    const grund_t *gr;

    while(true) {
  pos = pos + zv;
  if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
      return koord3d::invalid;
  }
  gr = welt->lookup(pos);

  if(gr) {
      if(gr->ist_tunnel()) {  // Anderer Tunnel läuft quer
    return koord3d::invalid;
      }
      ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp);

      if(ribi && koord(ribi) == zv) {
    // Ende am Hang - Endschiene vorhanden
    return pos;
      }
      if(!ribi && gr->gib_grund_hang() == hang_typ(-zv)) {
    // Ende am Hang - Endschiene fehlt oder hat keine ribis
    // Wir prüfen noch, ob uns dort ein anderer Weg stört
    if(!gr->hat_wege() || gr->gib_weg(wegtyp)) {
        return pos;
    }
      }
      return koord3d::invalid;  // Was im Weg (schräger Hang oder so)
        }
  // Alles frei - weitersuchen
    }
}

int tunnelbauer_t::baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp)
{
    DBG_MESSAGE("tunnelbauer_t::baue()", "called on %d,%d", pos.x, pos.y);

    if(!welt->ist_in_kartengrenzen(pos)) {
  return false;
    }
	const grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	koord zv;
	const weg_t *weg = gr->gib_weg(wegtyp);

	if(!weg || gr->gib_typ() != grund_t::boden) {
		if(welt->get_active_player()==sp) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tunnel must start on single way!"), w_autodelete);
		}
		return false;
	}
	if(!hang_t::ist_einfach(gr->gib_grund_hang())) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Tunnel muss an\neinfachem\nHang beginnen!\n"), w_autodelete);
		return false;
	}
	if(weg->gib_ribi_unmasked() & ~ribi_t::rueckwaerts(ribi_typ(gr->gib_grund_hang()))) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tunnel must end on single way!"), w_autodelete);
		return false;
	}
    zv = koord(gr->gib_grund_hang());

    // Tunnelende suchen
    koord3d end = finde_ende(welt, gr->gib_pos(), zv, wegtyp);

    // pruefe ob Tunnel auf strasse/schiene endet
    if(!welt->ist_in_kartengrenzen(end.gib_2d())) {
  if(wegtyp == weg_t::strasse) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Tunnel muss auf\nStrassenende\nenden!\n"), w_autodelete);
  } else {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,"Tunnel muss auf\nSchienenende\nenden!\n"), w_autodelete);
  }
      return false;
    }

    // Anfang und ende sind geprueft, wir konnen endlich bauen
    return baue_tunnel(welt, sp, gr->gib_pos(), end, zv, wegtyp);
}



bool tunnelbauer_t::baue_tunnel(karte_t *welt, spieler_t *sp, koord3d start, koord3d end, koord zv, weg_t::typ wegtyp)
{
	ribi_t::ribi ribi;
	blockhandle_t bs1;
	const tunnel_besch_t *besch;
	weg_t *weg;
	koord3d pos = start;
	int cost = 0;
	const weg_besch_t *einfahrt_besch;

DBG_MESSAGE("tunnelbauer_t::baue()","build from (%d,%d)", pos.x, pos.y);

	// get a way for enty/exit
	weg = welt->lookup(start)->gib_weg(wegtyp);
	if(weg==NULL) {
		weg = welt->lookup(end)->gib_weg(wegtyp);
	}
	if(weg==NULL) {
		einfahrt_besch = wegbauer_t::weg_search(wegtyp,450);
	}
	else {
		einfahrt_besch = weg->gib_besch();
	}

	baue_einfahrt(welt, sp, pos, zv, wegtyp, einfahrt_besch, cost);
	ribi = welt->lookup(pos)->gib_weg_ribi_unmasked(wegtyp);
	if(wegtyp == weg_t::schiene) {
		besch = schienentunnel;
		bs1 = ((schiene_t *)welt->lookup(pos)->gib_weg(wegtyp))->gib_blockstrecke();
	}
	else {
		besch = strassentunnel;
	}
	pos = pos + zv;

	while(pos.gib_2d()!=end.gib_2d()) {
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);

		if(wegtyp == weg_t::schiene) {
			weg = new schiene_t(welt,450);
			((schiene_t *)weg)->setze_blockstrecke( bs1 );
		}
		else {
			weg = new strasse_t(welt,450);
		}

		welt->access(pos.gib_2d())->boden_hinzufuegen(tunnel);
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
// why on earth put a tunnel object here!?!
//		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		cost += umgebung_t::cst_tunnel;

		pos = pos + zv;
	}
	baue_einfahrt(welt, sp, pos, -zv, wegtyp, einfahrt_besch, cost);

	sp->buche(cost, start.gib_2d(), COST_CONSTRUCTION);
	return true;
}



void
tunnelbauer_t::baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, weg_t::typ wegtyp, const weg_besch_t *weg_besch, int &cost)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi = alter_boden->gib_weg_ribi_unmasked(wegtyp) | ribi_typ(zv);
	weg_t *alter_weg = alter_boden->gib_weg(wegtyp);

	tunnelboden_t *tunnel = new tunnelboden_t(welt, end, alter_boden->gib_grund_hang());
	ding_t *sig = NULL;
	const tunnel_besch_t *besch;
	weg_t *weg;

DBG_MESSAGE("tunnelbauer_t::baue_einfahrt()","at end (%d,%d) for %s", end.x, end.y, weg_besch->gib_name());

	// rail tunnel
	if(wegtyp == weg_t::schiene) {
		weg = new schiene_t(welt);
		besch = schienentunnel;
		if(alter_weg!=NULL) {
			blockhandle_t bs = ((schiene_t *)alter_weg)->gib_blockstrecke();
			((schiene_t *)weg)->setze_blockstrecke( bs );
			sig = (ding_t *)alter_boden->suche_obj(ding_t::signal);
			if(sig) { // Signal aufheben - kommt auf den neuen Boden!
				alter_boden->obj_remove(sig, sp);
			}
		}
	}
	// or road tunnel
	else {
		weg = new strasse_t(welt);
		besch = strassentunnel;
	}

	if(alter_weg) {
		weg->setze_besch(alter_weg->gib_besch());
	}
	else {
		weg->setze_besch(weg_besch);
		cost += weg_besch->gib_preis();
	}

	tunnel->neuen_weg_bauen(weg, ribi, sp);
	tunnel->obj_add(new tunnel_t(welt, end, sp, besch));
	if(wegtyp == weg_t::schiene) {
		blockmanager::gib_manager()->neue_schiene(welt, tunnel, sig);
	}
	welt->access(end.gib_2d())->kartenboden_setzen( tunnel, false );
	tunnel->calc_bild();

	cost += umgebung_t::cst_tunnel;
	// no undo possible anymore
	if(sp!=NULL) {
		sp->init_undo(wegtyp,0);
	}
}




const char *
tunnelbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d start, weg_t::typ wegtyp)
{
    blockmanager  *bm = blockmanager::gib_manager();
    marker_t    marker(welt->gib_groesse_x(),welt->gib_groesse_y());
    slist_tpl<koord3d>  end_list;
    slist_tpl<koord3d>  part_list;
    slist_tpl<koord3d>  tmp_list;
    const char    *msg;
    koord3d   pos = start;
    int     cost = 0;
    const weg_besch_t* weg_besch;

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
      zv = koord(from->gib_grund_hang());
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
    !marker.ist_markiert(to)) {
    tmp_list.insert(to->gib_pos());
    marker.markiere(to);
      }
  }
    } while(!tmp_list.is_empty());

    // Jetzt geht es ans löschen der Tunnel
    while(!part_list.is_empty()) {
  pos = part_list.remove_first();

  grund_t *gr = welt->lookup(pos);

  if(wegtyp == weg_t::schiene) {
      bm->entferne_schiene(welt, pos);
  }
  gr->weg_entfernen(wegtyp, false);
  gr->obj_loesche_alle(sp);
  cost += umgebung_t::cst_tunnel;

  welt->access(pos.gib_2d())->boden_entfernen(gr);

  delete gr;
    }

		// Und die Tunnelenden am Schluß
		while(!end_list.is_empty()) {
			pos = end_list.remove_first();

			grund_t *gr = welt->lookup(pos);
			ding_t *sig = NULL;
			ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp) &~ribi_typ(gr->gib_grund_hang());

			if(wegtyp == weg_t::schiene) {
				sig = gr->suche_obj(ding_t::signal);
				if(sig) { // Signal aufheben - kommt auf den neuen Boden!
					gr->obj_remove(sig, sp);
					((schiene_t *)gr->gib_weg(weg_t::schiene))->gib_blockstrecke()->entferne_signal((signal_t *)sig);
				}
				bm->entferne_schiene(welt, gr->gib_pos());
			}
			weg_besch = gr->gib_weg(wegtyp)->gib_besch();
			gr->weg_entfernen(wegtyp, false);
			gr->obj_loesche_alle(sp);
			cost += umgebung_t::cst_tunnel;

			gr = new boden_t(welt, pos);
			welt->access(pos.gib_2d())->kartenboden_setzen(gr, false);

			// Neuen Boden wieder mit Weg versehen
			if(wegtyp==weg_t::schiene) {
				weg_t *weg = new schiene_t(welt);
				weg->setze_besch( weg_besch );
				gr->neuen_weg_bauen( weg, ribi, sp );
				bm->neue_schiene(welt, gr, sig);
			}
			else {
				weg_t *weg = new strasse_t(welt);
				weg->setze_besch( weg_besch );
				gr->neuen_weg_bauen( weg, ribi, sp );
			}
		gr->calc_bild();
	}
	welt->setze_dirty();
	sp->buche(cost, start.gib_2d(), COST_CONSTRUCTION);
	return NULL;
}
