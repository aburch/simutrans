/*
 * simwerkz.cc
 *
 * Werkzeuge für den Simutrans-Spieler
 * von Hj. Malthaner
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>

#include "simdebug.h"
#include "simplay.h"
#include "simtools.h"
#include "simevent.h"
#include "simskin.h"
#include "simcity.h"
#include "simcosts.h"

#include "bauer/fabrikbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/dock.h"
#include "boden/tunnelboden.h"

#include "simvehikel.h"
#include "simverkehr.h"
#include "simworld.h"
#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "blockmanager.h"
#include "simhalt.h"

#include "simwerkz.h"

#include "besch/haus_besch.h"
#include "besch/skin_besch.h"

#include "gui/messagebox.h"
#include "gui/label_frame.h"
#include "gui/halt_list_filter_frame.h"
#include "gui/convoi_filter_frame.h"
#include "gui/citylist_frame_t.h"
#include "gui/goods_frame_t.h"

#include "dings/zeiger.h"
#include "dings/tunnel.h"
#include "dings/signal.h"
#include "dings/roadsign.h"
#include "dings/leitung2.h"
#include "dings/baum.h"
#ifdef LAGER_NOT_IN_USE
#include "dings/lagerhaus.h"
#endif

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/fahrplan.h"

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "besch/weg_besch.h"
#include "besch/roadsign_besch.h"

#include "gui/karte.h"	// to update map after construction of new industry
#include "sucher/bauplatz_sucher.h"

// hilfsfunktionen


/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord pos, int b = 1, int h = 1)
{
	halthandle_t halt;
	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(bd->gib_weg(weg_t::schiene) != NULL) {
		ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);
	}
	if(bd->gib_weg(weg_t::strasse) != NULL) {
		ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse);
	}
	if(  ribi_t::nord & ribi ) {
		next_try_dir[iAnzahl++] = koord(0,-1);
	}
	if(  ribi_t::sued & ribi ) {
		next_try_dir[iAnzahl++] = koord(0,1);
	}
	if(  ribi_t::ost & ribi ) {
		next_try_dir[iAnzahl++] = koord(1,0);
	}
	if(  ribi_t::west & ribi ) {
		next_try_dir[iAnzahl++] = koord(-1,0);
	}

	// first try to connect to our own
	for(  int i=0;  i<iAnzahl;  i++ ) {
		halt = sp->is_my_halt(pos+next_try_dir[i]);
		if(  halt.is_bound()  ) {
			return halt;
		}
	}

	// now just search everything
	koord k;
	for(k.x=pos.x-1; k.x<=pos.x+b; k.x++) {
		for(k.y=pos.y-1; k.y<=pos.y+h; k.y++) {
			halt = sp->is_my_halt(k);
			if(halt.is_bound()) {
				return halt;
			}
		}
	}
	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halt;
}


// werkzeuge

int
wkz_abfrage(spieler_t *, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_abfrage()",
     "checking map square %d,%d", pos.x, pos.y);


    if(welt->ist_in_kartengrenzen(pos)) {
  grund_t *gr = welt->lookup(pos)->gib_kartenboden();

//  printf("Information, max %d objects\n", plan->gib_max_index());

  if(gr->gib_halt().is_bound()) {
//      printf("Haltestelle %p\n", plan->gib_boden()->gib_halt());

      gr->zeige_info();

  } else {

      if(gr->gib_top() <= 0) {
    gr->zeige_info();
      } else if(gr->suche_obj(ding_t::oberleitung) &&
          gr->obj_count() == 1) {
        // Hajo: special case of oberleitung which may not
        // disable ground info display
    gr->zeige_info();
      }
        }

  for(int n=0; n<gr->gib_top(); n++) {
      if(gr->obj_bei(n) != NULL) {
          DBG_MESSAGE("wkz_abfrage()", "index %d", n);
    gr->obj_bei(n)->zeige_info();
      }
  }

  return true;
    }

    return false;
}

int
wkz_raise(spieler_t *sp, karte_t *welt, koord pos)
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+16);

	bool ok = false;

	if(welt->ist_in_gittergrenzen(pos)) {
		const int hgt = welt->lookup_hgt(pos);

		if(hgt < 224) {

			int n = welt->raise(pos);
			ok = (n!=0);
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
				return false;
			}
			else {
				sp->buche(CST_BAU*n, pos, COST_CONSTRUCTION);
			}
//DBG_MESSAGE("wkz_raise()", "%d squares changed", n);
		} else {
//DBG_MESSAGE("wkz_raise()", "Maximum height reached");
		}
	}
	return ok;
}

int
wkz_lower(spieler_t *sp, karte_t *welt, koord pos)
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-16);

	bool ok = false;

	if(welt->ist_in_gittergrenzen(pos)) {

		const int hgt = welt->lookup_hgt(pos);

		if(hgt > welt->gib_grundwasser()) {

			int n = welt->lower(pos);
			ok = (n!=0);
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			}
			else {
				sp->buche(CST_BAU*n, pos, COST_CONSTRUCTION);
			}
//      DBG_MESSAGE("wkz_lower()", "%d squares changed", n);

			// update image
			for(int j=-1; j<=1; j++) {
				for(int i=-1; i<=1; i++) {
					if(welt->ist_in_gittergrenzen(pos.x+i,pos.y+j)  &&  welt->lookup(pos+koord(i,j))->gib_kartenboden()) {
						welt->lookup(pos+koord(i,j))->gib_kartenboden()->calc_bild();
					}
				}
			}

		} else {
//      DBG_MESSAGE("wkz_lower()", "Minimum height reached");
		}
	}

	return ok;
}


/*
 * Vorbedingungen:
 *  - für Docks nok
 * welt != NULL
 * sp != NULL
 * halt != NULL
 * pos in Kartengrenzen
 */

static void
entferne_haltestelle(karte_t *welt, spieler_t *sp,
                     halthandle_t halt, koord pos)
{
	DBG_MESSAGE("entferne_haltestelle()","removing segment from %d,%d", pos.x, pos.y);

	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	// Alle objekte entfernen
	bd->obj_loesche_alle(sp);
	halt->rem_grund(bd);

	if(!halt->existiert_in_welt()) {
		// all deleted?
		sp->halt_remove( halt );
		haltestelle_t::destroy( halt );
	}
	else {
		// may have been changed ...
		halt->recalc_station_type();
	}
	bd->calc_bild();

	weg_t *weg = bd->gib_weg(weg_t::strasse);

	if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
		// Stadtstrassen sollten entfernbar sein, deshalb
		// dürfen sie keinen Besitzer haben.
		bd->setze_besitzer( NULL );
	}
	bd->setze_text( NULL );

	if(bd->gib_typ() == boden_t::fundament) {
		// Posthäuschen das Fundament rauben!
		if(halt.is_bound()) {
DBG_MESSAGE("entferne_haltestelle()", "removing post office");
			halt->set_post_enabled(false);
		}
		welt->access(pos)->kartenboden_setzen(new boden_t(welt, bd->gib_pos()), false);
	}
}


static int
wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg)
{
DBG_MESSAGE("wkz_remover_intern()","at (%d,%d)", pos.x, pos.y);

	planquadrat_t *plan = welt->access(pos);

	if(!plan) {
		return false;
	}
	grund_t *gr = plan->gib_kartenboden();

	// stadtauto zum löschen? (we allow always)
	if(gr->suche_obj(ding_t::verkehr)) {
		stadtauto_t *citycar = dynamic_cast<stadtauto_t *>(gr->suche_obj(ding_t::verkehr));
		gr->obj_remove(citycar,welt->gib_spieler(1));
		citycar->~stadtauto_t();
		return true;
	}

	msg = gr->kann_alle_obj_entfernen(sp);
	if( msg ) {
		return false;
	}

	// prissi: Leitung prüfen (can cross ground of another player)
	leitung_t *lt=dynamic_cast<leitung_t *>(gr->suche_obj(ding_t::leitung));
	if(lt!=NULL  &&  lt->gib_besitzer()==sp) {
		gr->obj_remove(lt,sp);
		delete lt;
		return true;
	}

	// prüfen, ob boden entfernbar
	if(gr->gib_besitzer() != NULL && gr->gib_besitzer() != sp) {
		msg = "Das Feld gehoert\neinem anderen Spieler\n";
		return false;
	}

	grund_t *gr_oben = plan->gib_obersten_boden(sp);
	if(gr_oben == NULL) {
		gr_oben = plan->gib_obersten_boden(NULL);
	}

	// dan: Signal auf Brücke prüfen
	if(gr_oben->suche_obj(ding_t::signal) != NULL  ||  gr_oben->suche_obj(ding_t::presignal) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing signal from bridge %d,%d",  pos.x, pos.y);
		blockmanager *bm = blockmanager::gib_manager();
		const bool ok = bm->entferne_signal(welt, gr_oben->gib_pos());
		if(!ok) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Ambiguous signal combination.\nTry removing by clicking the\nother side of the signal.\n"), w_autodelete);
		}
		return ok;
	}
	if(gr_oben->suche_obj(ding_t::roadsign) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing roadsign from bridge %d,%d",  pos.x, pos.y);
		roadsign_t *rs = dynamic_cast<roadsign_t *>(gr_oben->suche_obj(ding_t::roadsign));
//		gr_oben->obj_remove(rs,sp);
		rs->~roadsign_t();
		return true;
	}
    // Signal auf Boden prüfen
	if(gr_oben->suche_obj(ding_t::signal) != NULL  ||  gr_oben->suche_obj(ding_t::presignal) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing signal from %d,%d",  pos.x, pos.y);
		blockmanager *bm = blockmanager::gib_manager();
		const bool ok = bm->entferne_signal(welt, gr->gib_pos());
		if(!ok) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Ambiguous signal\ncombination. Try removing\nfrom the other side of the\nsignal.\n"), w_autodelete);
		}
		return ok;
	}
	if(gr_oben->suche_obj(ding_t::roadsign) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing roadsign %d,%d",  pos.x, pos.y);
		roadsign_t *rs = dynamic_cast<roadsign_t *>(gr_oben->suche_obj(ding_t::roadsign));
//		gr_oben->obj_remove(rs,sp);
		rs->~roadsign_t();
		return true;
	}

	// Brückenanfang prüfen
	if(gr->ist_bruecke()) {
		weg_t *weg = gr->gib_weg(weg_t::schiene);

		if(!weg) {
			weg = gr->gib_weg(weg_t::strasse);
		}
		msg = brueckenbauer_t::remove(welt, sp, gr->gib_pos(), weg->gib_typ());
		return msg == NULL;
	}

	// Tunnelanfang prüfen
	if(gr->ist_tunnel()) {
		weg_t *weg = gr->gib_weg(weg_t::schiene);

		if(!weg) {
			weg = gr->gib_weg(weg_t::strasse);
		}
		msg = tunnelbauer_t::remove(welt, sp, gr->gib_pos(), weg->gib_typ());
		return msg == NULL;
	}

	// Haltestelle prüfen
	halthandle_t halt = sp->is_my_halt( pos);
	if( halt.is_bound() ) {
		entferne_haltestelle(welt, sp, halt, pos);
		return true;
	}

	// Depot prüfen
	depot_t * dp = gr->gib_depot();
	if(dp) {
DBG_MESSAGE("wkz_remover()", "removing %s %p from %d,%d",	dp->gib_name(), dp, pos.x, pos.y);

		gr->obj_remove(dp, sp);
		delete dp;
		return true;
	}

	// ok, now we removed every object, that should be removed one by one.
	// the following objects will be removed together
DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",
	gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
	if( gr->gib_weg(weg_t::schiene) != NULL ) {
		if(!blockmanager::gib_manager()->entferne_schiene(welt, gr->gib_pos())) {
			return false;
		}
	}

#if 1
		// since buildings can have more than one tile, we must handle them together
		gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
		if(gb) {
			const haus_tile_besch_t *tile  = gb->gib_tile();
			koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );

			// get startpos
			koord k=tile->gib_offset();
			if(k != koord(0,0)) {
				return wkz_remover_intern(sp, welt, pos-k, msg);
			}
			else {

				// remove factory?
				if(gb->fabrik()!=NULL) {

					// remove connections ...
					fabrik_t *fab=gb->fabrik();
DBG_MESSAGE("wkz_remover()", "removing factory:  %p");

					if(fab->gib_halt_list().count()!=0) {
						// remove from all stops
						msg = "Das Feld gehoert\neinem anderen Spieler\n";
						return false;
					}


					// delete it from map
DBG_MESSAGE("wkz_remover()", "removing factory from world");
					gb->setze_fab(NULL);
					welt->rem_fab(fab);

DBG_MESSAGE("wkz_remover()", "removing factory:  supplier info change for %i factories",welt->gib_fab_list().count() );
					// remove all links from factories
					slist_iterator_tpl<fabrik_t *> iter (welt->gib_fab_list());
					while(iter.next()) {
						fabrik_t * fab = iter.get_current();
						fab->rem_lieferziel(pos);
						fab->rem_supplier(pos);
DBG_MESSAGE("wkz_remover()", "reconnecting factories");
					}
			welt->rem_fab((fabrik_t*)6);

					// remove from all cities
DBG_MESSAGE("wkz_remover()", "removing factory:  reconnecting towns");
					const vector_tpl<stadt_t *> *stadt = welt->gib_staedte();
					for( unsigned i=0;  i<stadt->get_count();  i++ ) {
						stadt->at(i)->verbinde_fabriken();
					}
welt->rem_fab((fabrik_t*)1);

					// finally delete it
					fab->~fabrik_t();
				}

				// remove town? (when removing townhall)
				if(gb->ist_rathaus()) {
					stadt_t *stadt = welt->suche_naechste_stadt(pos);
					if(!welt->rem_stadt( stadt )) {
						msg = "Das Feld gehoert\neinem anderen Spieler\n";
						return false;
					}
				}

				for(k.y = 0; k.y < size.y; k.y ++) {
					for(k.x = 0; k.x < size.x; k.x ++) {
						if(k!=koord(0,0)) {
							grund_t *gr = welt->lookup(k+pos)->gib_kartenboden();
							gr->obj_loesche_alle(sp);
							welt->access(k+pos)->kartenboden_setzen(new boden_t(welt, gr->gib_pos()), false);
						}
					}
				}
			}
			welt->rem_fab((fabrik_t*)1);
		}
#endif
	gr->obj_loesche_alle(sp);

	/*
	* Eigentlich müssen wir hier noch verhindern, daß ein Bahnhofsgebäude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst lässt sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	int cost_sum = gr->weg_entfernen(weg_t::schiene, true);
	cost_sum += gr->weg_entfernen(weg_t::strasse, true);
	cost_sum += gr->weg_entfernen(weg_t::wasser, true);

	if(cost_sum > 0) {
		sp->buche(cost_sum * CST_STRASSE, pos, COST_CONSTRUCTION);
	}

	// now labels on water
	// replacements of ground will give a grass tile
	// => 	don't do this on water ...
	if(!gr->ist_wasser()) {
		bool label = gr->gib_besitzer() && welt->gib_label_list().contains(pos);

		plan->kartenboden_setzen(new boden_t(welt, gr->gib_pos()), false);
		if(label) {
			plan->gib_kartenboden()->setze_besitzer(sp);
		}
	}
	return true;
}


int
wkz_remover(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
    const char *fail = NULL;

    if(!wkz_remover_intern(sp, welt, pos, fail)) {
  if(fail) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, fail), w_autodelete);
  }
  return false;
    }
    // Nachbarschaft (Bilder) neu berechnen

    if(pos.x>1)
  welt->lookup(pos+koord(-1, 0))->gib_kartenboden()->calc_bild();
    if(pos.y>1)
  welt->lookup(pos+koord::nord)->gib_kartenboden()->calc_bild();

    if(pos.x<welt->gib_groesse_x()-1)
  welt->lookup(pos+koord::ost)->gib_kartenboden()->calc_bild();
    if(pos.y<welt->gib_groesse_y()-1)
  welt->lookup(pos+koord::sued)->gib_kartenboden()->calc_bild();

    return true;
}




/*
 * aufruf mit i == -1, j == -1 initialisiert
 *
 * erster aufruf setzt start
 * zweiter aufruf setzt ende und baut
 */


int
wkz_wegebau(spieler_t *sp, karte_t *welt,  koord pos, value_t lParam)
{
  static bool erster = true;
  static koord start, ziel;
  static zeiger_t *bauer = NULL;


  const weg_besch_t * besch = (const weg_besch_t *) (lParam.p);
  enum wegbauer_t::bautyp bautyp = wegbauer_t::strasse;

  if(besch->gib_wtyp() == weg_t::schiene) {
		if(besch->gib_styp() == 7){ // Dario: Tramway
			bautyp = wegbauer_t::schiene_tram;
		} else {
			bautyp = wegbauer_t::schiene;
		}
  }
  if(besch->gib_wtyp() == weg_t::powerline) {
    bautyp = wegbauer_t::leitung;
  }

  if(pos==INIT || pos == EXIT) {  // init strassenbau
    erster = true;

    if(bauer != NULL) {
      delete bauer;
      bauer = NULL;
    }
  } else {
    const planquadrat_t *plan = welt->lookup(pos);

    // Hajo: check if it's safe to build here
    if(plan == NULL) {
      return false;
    } else {
      grund_t *gr = welt->lookup(pos)->gib_kartenboden();
      if(gr == NULL ||
   gr->ist_wasser() ||
   gr->kann_alle_obj_entfernen(sp)) {

  return false;
      }
    }


    if( erster ) {
      DBG_MESSAGE("wkz_wegebau()", "Setting start to %d,%d",pos.x, pos.y);

      start = pos;
      erster = false;

      // symbol für strassenanfang setzen
      grund_t *gr = welt->lookup(pos)->gib_kartenboden();
      bauer = new zeiger_t(welt, gr->gib_pos(), sp);
      bauer->setze_bild(0, skinverwaltung_t::bauigelsymbol->gib_bild_nr(0));
      gr->obj_pri_add(bauer, PRI_NIEDRIG);
    } else {
      // Hajo: symbol für strassenanfang entfernen
      delete( bauer );
      bauer = NULL;

      wegbauer_t bauigel(welt, sp);

      // Hajo: to test building as disguised AI player
      // wegbauer_t bauigel(welt, welt->gib_spieler(2));


      erster = true;

      DBG_MESSAGE("wkz_wegebau()", "Setting end to %d,%d",pos.x, pos.y);

      ziel = pos;

      // Hajo: to test baubaer mode (only for AI)
      // bauigel.baubaer = true;

      bauigel.route_fuer(bautyp, besch);
      bauigel.set_keep_existing_faster_ways(true);
      if(event_get_last_control_shift()==2) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
      	bauigel.calc_straight_route(start,ziel);
    	 }
      else {
      	bauigel.calc_route(start,ziel);
      }

      // sp->platz1 = start;
      // sp->platz2 = ziel;
      // sp->create_complex_rail_transport();
      // sp->create_complex_road_transport();

      DBG_MESSAGE("wkz_wegebau()", "builder found route with %d sqaures length.", bauigel.max_n);

      bauigel.baue();
    }
  }
  return true;
}


/* build all kind of station buildings
 * @author V. Meyer
 */
int wkz_station_building_aux(spieler_t *sp, karte_t *welt, koord pos, const haus_besch_t * besch, bool is_post)
{
DBG_MESSAGE("wkz_post()", "building mail office/station building on square %d,%d", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {
		koord size = besch->gib_groesse();
		int rotate = 0;

		bool hat_platz = false;
		bool slope_check = size.x+size.y>2;
		halthandle_t halt;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
			hat_platz = true;
			halt = suche_nahe_haltestelle(sp, welt, pos, size.x, size.y);
		}

		if(!hat_platz  &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
			halthandle_t halt2 = suche_nahe_haltestelle(sp, welt, pos, size.y, size.x);
			hat_platz = true;
			if(halt2.is_bound()) {
				if(halt.is_bound() || simrand(2) == 1) {
					halt = halt2;
					rotate = 1;
				}
			}
		}

		// is there already a halt to connect?
		if(halt.is_bound()) {
			if(is_post  &&  halt->get_post_enabled()) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Station already\nhas a post office!\n"), w_autodelete);
			}
			else {
				hausbauer_t::baue(welt, sp, welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotate, besch, true, &halt);
				sp->buche(CST_POST, pos, COST_CONSTRUCTION * size.x * size.y);
			}
		}
		else {
			if(hat_platz) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Post muss neben\nHaltestelle\nliegen!\n"), w_autodelete);
			}
			else {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			}
		}
		return true;
	}
	return false;
}


int wkz_post(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	wkz_station_building_aux(sp, welt, pos, (const haus_besch_t *)value.p, true);
	return true;
}


int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	wkz_station_building_aux(sp, welt, pos, (const haus_besch_t *)value.p, false);
	return true;
}


#ifdef LAGER_NOT_IN_USE
int
wkz_lagerhaus(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_lagerhaus()", "building storage shed on square %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  bool can_build = (welt->lookup(pos)->gib_boden()->kann_alle_obj_entfernen(sp) == NULL);

  halthandle_t halt;

  if( can_build ) {
      halt = suche_nahe_haltestelle(sp, welt, pos);

      if(halt.is_bound()) {
    grund_t *gr = welt->lookup(pos)->gib_boden();
    lagerhaus_t *lager = new lagerhaus_t( welt, gr->gib_pos(), sp );
    lager->setze_name( halt->gib_name() );
    gr->baue_gebaeude( IMG_LAGERHAUS, lager, true );
                gr = NULL;

    halt->add_grund(welt->lookup(pos)->gib_boden());
    halt->setze_lager( lager );

      } else {
    create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Lager muss neben\nHaltestelle\nliegen!\n"), w_autodelete);
      }
  }
  return true;
    } else {
  return false;
    }
}
#endif


static int
wkz_bahnhof_aux(spieler_t *sp,
    karte_t *welt,
    koord pos,
    bool nordsued,
    const haus_besch_t * besch)
{
    grund_t *bd = welt->lookup(pos)->gib_kartenboden();

    if(bd->kann_alle_obj_entfernen(sp) == NULL && bd->gib_grund_hang() == 0) {
      halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);
      bool neu = !halt.is_bound();

      if(neu) {
    halt = sp->halt_add(pos);
//	halt = haltestelle_t::create(welt, pos, sp);
    DBG_MESSAGE("wkz_bahnhof_aux()", "founding new station");
      } else {
    DBG_MESSAGE("wkz_bahnhof_aux()", "new segment for station");
      }
      halt->set_pax_enabled( true );
      halt->set_ware_enabled( true );

      hausbauer_t::neues_gebaeude(welt,
          sp,
          bd->gib_pos(),
          nordsued ? 0 : 1,
          besch,
          &halt);
      halt->recalc_station_type();

      if(neu) {
    stadt_t *stadt = welt->suche_naechste_stadt(pos);
    const int count = sp->get_haltcount();
    const char *name = stadt->haltestellenname(pos, "BF", count);

    bd->setze_text( name );
      }
      sp->buche(neu ? CST_BAHNHOF : CST_BAHNHOF/2,
          pos,
          COST_CONSTRUCTION);
DBG_MESSAGE("wkz_bahnhof_aux()","sucessful built station at (%d,%d)",pos.x, pos.y);
    }
    else {
DBG_MESSAGE("wkz_bahnhof_aux()","can't build a train station segment on %d,%d",pos.x, pos.y);
    }
    return true;
}

int
wkz_bahnhof(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
    DBG_MESSAGE("wkz_bahnhof()", "building rail station segment on square %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  grund_t *bd = welt->lookup(pos)->gib_kartenboden();

  if(bd->gib_weg(weg_t::schiene) == NULL || bd->gib_weg(weg_t::strasse) != NULL ) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Bahnhof kann\nnur auf Schienen\ngebaut werden!\n"), w_autodelete);
      return false;
  }

  if(bd->gib_besitzer() != sp) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Die Schiene\ngehoert einem\nanderen Spieler!\n"), w_autodelete);
      return false;
  }

  const bool hat_oberleitung = (bd->suche_obj(ding_t::oberleitung) != 0);

  if(bd->obj_count() > (hat_oberleitung ? 1 : 0)) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
      return false;
  }

  const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);


  const haus_besch_t * besch = (const haus_besch_t *) value.p;

  if(ribi_t::ist_gerade_ns(ribi)) {
      return wkz_bahnhof_aux(sp, welt, pos, true, besch);
  } else if(ribi_t::ist_gerade_ow(ribi)) {
      return wkz_bahnhof_aux(sp, welt, pos, false, besch);
  } else {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nBahnhof ge-\nbaut werden!\n"), w_autodelete);
  }
    }
    return false;
}

static int
wkz_bushalt_aux(spieler_t *sp, karte_t *welt, koord pos, ribi_t::ribi ribi,const haus_besch_t * besch)
{
	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	if((bd->gib_besitzer() == sp || bd->gib_besitzer() == 0) &&
		// bd->kann_alle_obj_entfernen(sp) == 0 &&
		bd->gib_grund_hang() == 0 &&
		!bd->gib_halt().is_bound()
	) {
		halthandle_t halt = suche_nahe_haltestelle(sp,welt,pos);
		bool neu = !halt.is_bound();

		if(neu) {
			halt = sp->halt_add(pos);
//			halt = haltestelle_t::create(welt, pos, sp);
DBG_MESSAGE("wkz_bushalt_aux()", "founding new station");
		}
		else {
DBG_MESSAGE("wkz_bushalt_aux()", "new segment for station");
		}
		halt->set_pax_enabled( true );

		// bd->obj_loesche_alle(sp);
		bd->setze_besitzer(sp);

		hausbauer_t::neues_gebaeude( welt, sp, bd->gib_pos(), (ribi & ribi_t::nordsued)?0 :1, besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "H", count);

			bd->setze_text( name );
		}
		sp->buche(CST_BUSHALT, pos, COST_CONSTRUCTION);
	}
	return true;
}

int
wkz_bushalt(spieler_t *sp, karte_t *welt, koord pos,value_t value)
{
DBG_MESSAGE("wkz_bushalt()", "building bus stop on square %d,%d", pos.x, pos.y);

	if(welt->lookup(pos)) {

		grund_t *bd = welt->lookup(pos)->gib_kartenboden();
		if(!bd->gib_weg(weg_t::strasse)) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Haltestelle kann\nnur auf Schienen\ngebaut werden!\n"), w_autodelete);
			return false;
		}

		if(bd->suche_obj(ding_t::strassendepot)) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			return false;
		}

		// there is a street and both street and rails (if there) are parallel
		// can only happen for trams, so we can skip rail type checks
		ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse)  |  bd->gib_weg_ribi_unmasked(weg_t::schiene);
		if(ribi_t::ist_gerade(ribi)) {
			return wkz_bushalt_aux(sp, welt, pos, ribi, (const haus_besch_t *) value.p);
		}

		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann keine\nHaltestelle ge-\nbaut werden!\n"), w_autodelete);
	}
  return false;
}



#if 0
static void dock_add_halt_grund_um(karte_t *welt, spieler_t *sp, halthandle_t halt, koord pos)
{
    assert(halt.is_bound());
    koord k;

    for(k.x=pos.x-1; k.x<=pos.x+1; k.x++) {
  for(k.y=pos.y-1; k.y<=pos.y+1; k.y++) {
      if(! halt->ist_da(k)) {
    const planquadrat_t *plan = welt->lookup(k);

    if(plan != NULL) {
        grund_t *gr = plan->gib_kartenboden();

        if(gr->ist_wasser() && !gr->gib_weg(weg_t::wasser)) {

      gr->neuen_weg_bauen(new dock_t(welt), ribi_t::alle, sp);

      halt->add_grund( gr );
        }
    }
      }
  }
    }
}
#endif



/* build a dock either small or large */
int
wkz_dockbau(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	const haus_besch_t *besch=(const haus_besch_t *)value.p;
	bool ok = false;

	// docks können nicht direkt am kartenrand gebaut werden,
	// da die schiffe dort nicht fahren können

	if(pos == INIT || pos == EXIT) {
		// init und exit ignorieren
		return true;
	}
	int hang = welt->get_slope(pos);
	// first get the size
	int len = besch->gib_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;

DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);
	if(hang_t::ist_einfach(hang) &&
		welt->ist_in_kartengrenzen(pos) && welt->ist_in_kartengrenzen(last_pos)  &&
		welt->lookup(pos-dx)->gib_kartenboden()->ist_wasser()  &&
		welt->lookup(last_pos)->gib_kartenboden()->ist_wasser()) {

		int layout = 0;
		koord3d bau_pos = welt->lookup(pos)->gib_kartenboden()->gib_pos();
		switch(hang) {
			case hang_t::sued:
				layout = 0;
				break;
			case hang_t::ost:
				layout = 1;
				break;
			case hang_t::nord:
				layout = 2;
				bau_pos = welt->lookup(last_pos)->gib_kartenboden()->gib_pos();
				break;
			case hang_t::west:
				layout = 3;
				bau_pos = welt->lookup(last_pos)->gib_kartenboden()->gib_pos();
				break;
		}
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);
		bool neu = !halt.is_bound();

		if(neu) { // neues dock
			halt = sp->halt_add(pos);
		}
		halt->set_pax_enabled( true );
		halt->set_ware_enabled( true );

		// remove old ground below slope
		welt->access(pos)->kartenboden_setzen(new boden_t(welt, welt->lookup(pos)->gib_kartenboden()->gib_pos()), false);

		hausbauer_t::baue(welt, sp, bau_pos, layout,besch, 0, &halt);

		for(int i=0;  i<=len;  i++ ) {
			koord p=pos-dx*i;
			grund_t *gr = welt->lookup(p)->gib_kartenboden();
			// Kosten
			sp->buche(CST_DOCK, p, COST_CONSTRUCTION);
			// dock-land anbindung gehört auch zur haltestelle
			halt->add_grund(gr);
		}
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "Dock", count);
			welt->lookup(halt->gib_basis_pos())->gib_kartenboden()->setze_text( name );
		}

		ok = true;
	}
	else {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Zu nah am Kartenrand"), w_autodelete);
	}

	return ok;
}



int
wkz_senke(spieler_t *sp, karte_t *welt, koord pos)
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", pos.x, pos.y);

	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}

	//int top = welt->lookup(pos)->gib_kartenboden()->gib_top();
	int hangtyp = welt->get_slope(pos);

	if(hangtyp == 0  &&
		!welt->lookup(pos)->gib_kartenboden()->ist_wasser()  &&
		welt->lookup(pos)->gib_kartenboden()->kann_alle_obj_entfernen(sp)==NULL)
	{

		fabrik_t *fab=leitung_t::suche_fab_4(pos);
		if(fab==NULL) {
DBG_MESSAGE("wkz_senke()","no factory near (%i,%i)",pos.x, pos.y);
			// need a factory
			return false;
		}
		// remove everything on that spot
		const char *fail = NULL;
		if(!wkz_remover_intern(sp, welt, pos, fail)) {
			if(fail) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, fail), w_autodelete);
			}
		}
		// now decide from the string whether a source or drain is built
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		int fab_name_len = strlen( fab->gib_name() );
		if(fab_name_len>11   &&  (strcmp(fab->gib_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(fab->gib_name()+fab_name_len-11, "Power Plant")==0)) {
			gr->obj_add( new pumpe_t(welt, gr->gib_pos(), sp) );
		}
		else {
			gr->obj_add(new senke_t(welt, gr->gib_pos(), sp));
		}
	}
	return true;
}


static int
wkz_frachthof_aux(spieler_t *sp, karte_t *welt, koord pos, ribi_t::ribi dir)
{
DBG_MESSAGE("wkz_frachthof_aux()",     "called on %d,%d", pos.x, pos.y);
	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	if(bd->kann_alle_obj_entfernen(sp) == NULL && bd->gib_grund_hang() == 0) {
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);
		bool neu = !halt.is_bound();

DBG_MESSAGE("wkz_frachthof_aux()","building loading bay on %d,%d", pos.x, pos.y);
		if(neu) {
			halt = sp->halt_add(pos);
			//	halt = haltestelle_t::create(welt, pos, sp);
DBG_MESSAGE("wkz_frachthof_aux()","Founding new station %s",halt->gib_name());
		}
		else {
DBG_MESSAGE("wkz_frachthof_aux()","Adding building to station %s", halt->gib_name());
		}
		halt->set_ware_enabled( true );

		bd->setze_besitzer(sp);

		int layout = 0;
		switch(dir) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
		hausbauer_t::neues_gebaeude(
				welt, sp, bd->gib_pos(), layout,
				hausbauer_t::frachthof_besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "H", count);

			bd->setze_text( name );
		}
		sp->buche(CST_FRACHTHOF, pos, COST_CONSTRUCTION);
	}
	return true;
}

int
wkz_frachthof(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_frachthof()","called on %d,%d", pos.x, pos.y);


    if(welt->ist_in_kartengrenzen(pos)) {
  const grund_t *bd = welt->lookup(pos)->gib_kartenboden();
  const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse);

  if(!bd->gib_weg(weg_t::strasse)) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Frachthof kann\nnur auf Strasse\ngebaut werden!\n"), w_autodelete);
      return false;
  }

  if(bd->obj_count() > 0) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
      return false;
  }

  if(ribi == ribi_t::nord || ribi == ribi_t::ost || ribi == ribi_t::sued || ribi == ribi_t::west) {
      return wkz_frachthof_aux(sp, welt, pos, ribi);
  }
  create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nFrachthof ge-\nbaut werden!\n"), w_autodelete);
    }
    return false;
}


int wkz_signale(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_signale()",
     "called on %d,%d", pos.x, pos.y);


    if(welt->ist_in_kartengrenzen(pos)) {
  blockmanager * bm = blockmanager::gib_manager();
  const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
  const planquadrat_t *plan = welt->lookup(pos);
  grund_t *gr = plan->gib_kartenboden();

  if(gr->gib_besitzer() == sp) {
      error = bm->neues_signal(welt, sp, gr->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung());
        }
  else {
      slist_tpl<grund_t *> gr_liste;
      unsigned int i;

      for(i = 0; i < plan->gib_boden_count(); i++) {
    if(plan->gib_boden_bei(i)->gib_besitzer() == sp &&
       plan->gib_boden_bei(i)->gib_hoehe() > gr->gib_hoehe()) {
        gr_liste.append(plan->gib_boden_bei(i));
    }
      }
      while(!gr_liste.is_empty() && error != NULL) {
    grund_t *best = gr_liste.at(0);
    for(i = 1; i < gr_liste.count(); i++) {
        if(gr->gib_hoehe() > best->gib_hoehe()) {
      best = gr;
        }
    }
    gr_liste.remove(best);
    error = bm->neues_signal(welt, sp, best->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung());
      }
  }
  if(error != NULL) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
  }
  return error == NULL;
    } else {
  return false;
    }
}

int wkz_presignals(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_presignals()",
     "called on %d,%d", pos.x, pos.y);


    if(welt->ist_in_kartengrenzen(pos)) {
  blockmanager * bm = blockmanager::gib_manager();
  const char * error = "Hier kann kein\npreSignal aufge-\nstellt werden!\n";
  const planquadrat_t *plan = welt->lookup(pos);
  grund_t *gr = plan->gib_kartenboden();

  if(gr->gib_besitzer() == sp) {
      error = bm->neues_signal(welt, sp, gr->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung(), true);
        }
  else {
      slist_tpl<grund_t *> gr_liste;
      unsigned int i;

      for(i = 0; i < plan->gib_boden_count(); i++) {
    if(plan->gib_boden_bei(i)->gib_besitzer() == sp &&
       plan->gib_boden_bei(i)->gib_hoehe() > gr->gib_hoehe()) {
        gr_liste.append(plan->gib_boden_bei(i));
    }
      }
      while(!gr_liste.is_empty() && error != NULL) {
    grund_t *best = gr_liste.at(0);
    for(i = 1; i < gr_liste.count(); i++) {
        if(gr->gib_hoehe() > best->gib_hoehe()) {
      best = gr;
        }
    }
    gr_liste.remove(best);
    error = bm->neues_signal(welt, sp, best->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung(), true);
      }
  }
  if(error != NULL) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
  }
  return error == NULL;
    } else {
  return false;
    }
}


int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", pos.x, pos.y);
	const roadsign_besch_t * besch = (const roadsign_besch_t *) (lParam.p);

	if(welt->ist_in_kartengrenzen(pos)) {

		const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
		const planquadrat_t *plan = welt->lookup(pos);
		grund_t *gr = plan->gib_kartenboden();

		weg_t *weg = gr->gib_weg(weg_t::strasse);
		if(  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL)  &&  weg!=NULL) {

			// get the sign dirction
			ribi_t::ribi dir = weg->gib_ribi_unmasked();
			if(ribi_t::ist_gerade(dir)) {

				// if single way, we need to reduce the allowed ribi to one
DBG_MESSAGE("wkz_roadsign()","dir is %i", dir);
				if(besch->is_single_way()) {
					for( int i=0;  i<=4;  i++) {
						if((dir&ribi_t::nsow[i])!=0) {
							dir = ribi_t::nsow[i];
							break;
						}
					}
DBG_MESSAGE("wkz_roadsign()","single dir is %i", dir);
				}

				// if there is already a sing, we might need to inverse the direction
				roadsign_t *rs = dynamic_cast<roadsign_t *>(gr->suche_obj(ding_t::roadsign));
				if(rs) {
					// revers only if single way sign
					if(besch->is_single_way()) {
DBG_MESSAGE("wkz_roadsign()","reverse ribi %i", ribi_t::rueckwaerts(dir) );
						rs->set_dir( ribi_t::rueckwaerts(rs->get_dir()) );
					}
				}
				else {
					// add a new roadsign at position zero!
					rs = new roadsign_t( welt, gr->gib_pos(), dir, besch );
					ding_t *dt = gr->obj_bei(0);
					if(dt) {
						gr->obj_remove( dt, dt->gib_besitzer() );
						gr->obj_pri_add(rs,0);
						gr->obj_pri_add(dt,0);
					}
					else {
						gr->obj_pri_add(rs,0);
					}
				}
				error = NULL;
			}
		}
#if 0
		// not on bridges yet
		else {
			slist_tpl<grund_t *> gr_liste;
			unsigned int i;

			for(i = 0; i < plan->gib_boden_count(); i++) {
				if(plan->gib_boden_bei(i)->gib_besitzer() == sp &&
					plan->gib_boden_bei(i)->gib_hoehe() > gr->gib_hoehe()  &&
					gr->gib_weg(weg_t::strasse)) {
					gr_liste.append(plan->gib_boden_bei(i));
				}
			}
			while(!gr_liste.is_empty() && error != NULL) {
				grund_t *best = gr_liste.at(0);
				for(i = 1; i < gr_liste.count(); i++) {
					if(gr->gib_hoehe() > best->gib_hoehe()) {
						best = gr;
					}
				}
				gr_liste.remove(best);
				ribi_t::ribi dir = weg->gib_ribi_unmasked();
				if(ribi_t::ist_gerade(dir)) {
					for( int i=0;  i<4;  i++) {
						if(dir&ribi_t::nsow[i]!=0) {
							dir = ribi_t::nsow[i];
							break;
						}
					}
					roadsign_t *rs = dynamic_cast<roadsign_t *>(gr->suche_obj(ding_t::roadsign));
					if(rs!=NULL) {
						rs->set_dir( ribi_t::rueckwaerts(rs->get_dir()) );
					}
					else {
						// add a new roadsign at position zero!
						rs = new roadsign_t( welt, gr->gib_pos(), dir, besch );
						ding_t *dt = gr->obj_bei(0);
						if(dt) {
							gr->obj_remove( dt, dt->gib_besitzer() );
							gr->obj_pri_add(rs,0);
							gr->obj_pri_add(dt,0);
						}
						else {
							gr->obj_pri_add(rs,0);
						}
					}
					error = NULL;
				}
			}
		}
#endif
		if(error != NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
		}
		return error == NULL;
	} else {
		return false;
	}
}



int wkz_bahndepot(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_bahndepot()",
     "called on %d,%d", pos.x, pos.y);


    if(welt->ist_in_kartengrenzen(pos)) {
  grund_t *bd = welt->lookup(pos)->gib_kartenboden();

  const bool hat_oberleitung = (bd->suche_obj(ding_t::oberleitung) != 0);

  if(bd->obj_count() > (hat_oberleitung ? 1 : 0)) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
      return false;
  }

  const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);

  if(ribi_t::ist_einfach(ribi) &&
      welt->get_slope(pos) == 0 &&
      bd->kann_alle_obj_entfernen(sp) == NULL)
  {
      int layout = 0;

      switch(ribi) {
      //case ribi_t::sued:layout = 0;  break;
      case ribi_t::ost:   layout = 1;    break;
      case ribi_t::nord:  layout = 2;    break;
      case ribi_t::west:  layout = 3;    break;
      }
      hausbauer_t::neues_gebaeude( welt, sp, bd->gib_pos(), layout,hausbauer_t::bahn_depot_besch);

      sp->buche(CST_BAHNDEPOT, pos, COST_CONSTRUCTION);
      return true;
  }
  create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nBahndepot ge-\nbaut werden!\n"), w_autodelete);
    }
    return false;
}


int wkz_tramdepot(spieler_t *sp, karte_t *welt, koord pos)
{
DBG_MESSAGE("wkz_tramdepot()","called on %d,%d", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd = welt->lookup(pos)->gib_kartenboden();

		const bool hat_oberleitung = (bd->suche_obj(ding_t::oberleitung) != 0);

		if(bd->obj_count() > (hat_oberleitung ? 1 : 0)) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			return false;
		}

		const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);
		if(ribi_t::ist_einfach(ribi) &&
			welt->get_slope(pos) == 0 &&
			bd->kann_alle_obj_entfernen(sp) == NULL)
		{
			int layout = 0;

			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude( welt, sp, bd->gib_pos(), layout, hausbauer_t::tram_depot_besch);
DBG_MESSAGE("wkz_tramdepot()","ok", layout, hausbauer_t::tram_depot_besch );
			sp->buche(CST_BAHNDEPOT, pos, COST_CONSTRUCTION);
			return true;
		}
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nStrabdepot ge-\nbaut werden!\n"), w_autodelete);
	}
	return false;
}

int wkz_strassendepot(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_strassendepot()",
     "called on %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  grund_t *bd = welt->lookup(pos)->gib_kartenboden();

  if(bd->obj_count() > 0) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
      return false;
  }
  const int ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse);

  if(ribi_t::ist_einfach(ribi) &&
      welt->get_slope(pos) == 0 &&
      bd->kann_alle_obj_entfernen(sp) == NULL)
  {
      int layout = 0;

      switch(ribi) {
      //case ribi_t::sued:layout = 0;  break;
      case ribi_t::ost:   layout = 1;    break;
      case ribi_t::nord:  layout = 2;    break;
      case ribi_t::west:  layout = 3;    break;
      }
      hausbauer_t::neues_gebaeude(
    welt, sp, bd->gib_pos(), layout,
    hausbauer_t::str_depot_besch);

      sp->buche(CST_BAHNDEPOT, pos, COST_CONSTRUCTION);
      return true;

  }
  create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nStraßendepot ge-\nbaut werden!\n"), w_autodelete);
    }
    return false;
}


int wkz_schiffdepot(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	int layout=value.i;
    DBG_MESSAGE("wkz_schiffdepot_aux()",
     "called on %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  grund_t *bd = welt->lookup(pos)->gib_kartenboden();

  if(bd->obj_count() > 0) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
      return false;
  }

  if(bd->ist_wasser() &&
      bd->kann_alle_obj_entfernen(sp) == NULL)
  {
      hausbauer_t::neues_gebaeude(
    welt, sp, bd->gib_pos(), layout,
    hausbauer_t::sch_depot_besch);

      sp->buche(CST_SCHIFFDEPOT, pos, COST_CONSTRUCTION);
      return true;
  }
  create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nSchiffdepot ge-\nbaut werden!\n"), w_autodelete);
    }
    return false;
}



/* the following two routines add points to a schedule
 *  by we do not like to stop at AIs stop, but we want still force the truck to use AI roads
 * So if there is a halt, then it must be either public or our!
 * @autor prissi
 */
int wkz_fahrplan_add(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	fahrplan_t *fpl=(fahrplan_t *)f.p;
	DBG_MESSAGE("wkz_fahrplan_add()", "Add coordinate to schedule.");

	// haben wir einen Fahrplan ?
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_add()","Schedule is (null), doing nothing");
		return false;
	}

	if(pos == INIT) {
		// init
	} else if(pos == EXIT) {
		// exit
	} else {
		// eingabe
		const planquadrat_t *plan = welt->lookup(pos);
		if(plan) {
			const grund_t * gr = plan->gib_kartenboden();

			if(gr  &&
				(!gr->gib_halt().is_bound()  ||
					 (gr->gib_halt()->gib_besitzer()==sp  ||  gr->gib_halt()->gib_besitzer()==NULL  ||  gr->gib_halt()->gib_besitzer()==welt->gib_spieler(1)) )
			) {
				fpl->append(welt, gr->gib_pos());
			}
			else {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Das Feld gehoert\neinem anderen Spieler\n"), w_autodelete);
			}
		}
	}

	return true;
}

int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	fahrplan_t *fpl=(fahrplan_t *)f.p;
	DBG_MESSAGE("wkz_fahrplan_add()", "Insert coordinate to schedule.");

	// haben wir einen Fahrplan ?
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_add()","Schedule is (null), doing nothing");
		return false;
	}

	if(pos == INIT) {
		// init
	} else if(pos == EXIT) {
		// exit
	} else {
		// eingabe
		const planquadrat_t *plan = welt->lookup(pos);
		if(plan) {
			const grund_t * gr = plan->gib_kartenboden();

			if(gr  &&
				(!gr->gib_halt().is_bound()  ||
					 (gr->gib_halt()->gib_besitzer()==sp  ||  gr->gib_halt()->gib_besitzer()==NULL  ||  gr->gib_halt()->gib_besitzer()==welt->gib_spieler(1)) )
			) {
				fpl->insert(welt, gr->gib_pos());
			}
			else {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Das Feld gehoert\neinem anderen Spieler\n"), w_autodelete);
			}
		}
	}

	return true;
}



int wkz_marker(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {
  create_win(-1, -1, -1, new label_frame_t(welt, sp, pos), w_autodelete, magic_label_frame);
  return true;
    }
    return false;
}


int wkz_blocktest(spieler_t *, karte_t *welt, koord pos)
{
    if(welt->lookup(pos)) {
  grund_t *gr = welt->lookup(pos)->gib_kartenboden();
  if(gr) {
            blockmanager::gib_manager()->pruefe_blockstrecke(welt, gr->gib_pos());
  }
  return true;
    }
    return false;
}


int wkz_electrify_block(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->lookup(pos)) {
  grund_t *gr = welt->lookup(pos)->gib_kartenboden();
  if(gr && gr->gib_besitzer() == sp) {
            blockmanager::gib_manager()->setze_tracktyp(welt, gr->gib_pos(),
              true);
  }
  return true;
    }
    return false;
}


/**
 * found a new city
 * @author Hj. Malthaner
 */
int wkz_add_city(spieler_t *sp, karte_t *welt, koord pos)
{
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();

		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->gib_grund_hang() == 0) {

			ding_t *d = gr->obj_bei(0);
			gebaeude_t *gb = dynamic_cast<gebaeude_t *>(d);

			if(gb && gb->ist_rathaus()) {
dbg->warning("wkz_add_city()", "Already a city here");
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int citizens=(int)(welt->gib_einstellungen()->gib_mittlere_einwohnerzahl()*0.9);
				//  stadt_t *stadt = new stadt_t(welt, welt->gib_spieler(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citicens
				stadt_t *stadt = new stadt_t(welt, welt->gib_spieler(1), pos,citizens/10);

				stadt->laden_abschliessen();
				welt->add_stadt(stadt);

				sp->buche(CST_STADT, pos, COST_CONSTRUCTION);
				ok =  true;
			}
		}
	}

	// update an open map
	reliefkarte_t::gib_karte()->set_mode(-1);
	return ok;
}


/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
int wkz_set_slope(spieler_t * sp, karte_t *welt, koord pos, value_t lParam)
{
	const int slope = lParam.i;
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {

		grund_t * gr1 = welt->lookup(pos)->gib_kartenboden();
		if(!gr1->ist_natur()  ||  gr1->suche_obj(ding_t::gebaeude)!=NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			return false;
		}

		if(slope >= 0 && slope < 15) {
			welt->set_slope(pos, slope);
			ok = true;
		}

		if(slope == 16) {
			// Hajo: special action: lower land

			// Precondition: sqaure and all neighbours have same height
			// or neighbours are one level deeper

			ok = true;
			for(int i=0; i<4; i++) {
				const koord k = pos + koord::nsow[i];
				const planquadrat_t *plan = welt->lookup(k);

				if(plan) {
					grund_t * gr = plan->gib_kartenboden();
					if(gr) {
						ok &= gr->gib_pos().z == gr1->gib_pos().z ||
								  gr->gib_pos().z == gr1->gib_pos().z - 16;
					}
				}
			}

			if(ok) {
				welt->set_slope(pos, 0);
				gr1->setze_pos(gr1->gib_pos() - koord3d(0,0,16));
			}
		}


		if(slope == 17) {
			// Hajo: special action: raise land

			// Precondition: sqaure and all neighbours have same height
			// or neighbours are one level higher

			ok = true;
			for(int i=0; i<4; i++) {
				const koord k = pos + koord::nsow[i];
				const planquadrat_t *plan = welt->lookup(k);

				if(plan) {
					grund_t * gr = plan->gib_kartenboden();
					if(gr) {
						ok &= gr->gib_pos().z == gr1->gib_pos().z ||
								  gr->gib_pos().z == gr1->gib_pos().z + 16;
					}
				}
			}

			if(ok) {
				welt->set_slope(pos, 0);
				gr1->setze_pos(gr1->gib_pos() + koord3d(0,0,16));
			}
		}

		if(slope == 18) {
			// prissi: special action: set to natural slope

			const int natural_slope = welt->calc_natural_slope(pos);
			ok = natural_slope!=welt->get_slope(pos);

			if(ok) {
				welt->set_slope(pos, natural_slope);
			}
		}

		// ok, was sucess => need to treat the other tiles
		if(ok) {
			sp->buche(CST_BAU*2, pos, COST_CONSTRUCTION);

			gr1->calc_bild();

			for(int i=0; i<4; i++) {
				const koord k = pos + koord::nsow[i];
				const planquadrat_t *plan = welt->lookup(k);

				if(plan) {
					grund_t *gr = plan->gib_kartenboden();
					if(gr) {
						gr->calc_bild();
					}
				}
			}
		}

	}
	return ok;
}


/**
 * Plant a tree
 * @author Hj. Malthaner
 */
int wkz_pflanze_baum(spieler_t *, karte_t *welt, koord pos)
{
  DBG_MESSAGE("wkz_pflanze_baum()",
         "called on %d,%d", pos.x, pos.y);

  baum_t::plant_tree_on_coordinate(welt,
           pos,
           10);

  return true;
}


// --- Hajo: below are inofficial and/or testing tools


/* Hajo: unused
#include "simverkehr.h"
int wkz_test(spieler_t *, karte_t *welt, koord pos)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		verkehrsteilnehmer_t *vt = new verkehrsteilnehmer_t(welt, pos);
		welt->lookup(pos)->gib_boden()->obj_add( vt );
		welt->sync_add( vt );
	}
	return true;
}
*/



/* builts a random industry chain, either in the next town */
int wkz_build_industries_land(spieler_t *, karte_t *welt, koord pos)
{
	const fabrik_besch_t *info = fabrikbauer_t::get_random_consumer(false);

	koord size = info->gib_haus()->gib_groesse();
	int rotation = 0;

	bool hat_platz = false;
	bool slope_check = size.x+size.y>2;

	if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
		hat_platz = true;
	}

	if(!hat_platz &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
		rotation = 1;
		hat_platz = true;
	}

	if(hat_platz) {
		koord3d k = welt->lookup(pos)->gib_kartenboden()->gib_pos();
		fabrikbauer_t::baue_hierarchie(welt, NULL, info, rotation, &k,welt->gib_spieler(1));

		// crossconnect all?
		if(umgebung_t::crossconnect_factories) {
			const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
			slist_iterator_tpl <fabrik_t *> iter (list);
			while( iter.next() ) {
				iter.get_current()->add_all_suppliers();
			}
		}

		return true;
	}

	return false;
}


/* builts a random industry chain, either in the next town */
int wkz_build_industries_city(spieler_t *, karte_t *welt, koord pos)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {

		const fabrik_besch_t *info = fabrikbauer_t::get_random_consumer(true);
		koord3d pos3d = plan->gib_kartenboden()->gib_pos();
		fabrikbauer_t::baue_hierarchie(welt, NULL, info, false, &pos3d,welt->gib_spieler(1));

		// crossconnect all?
		if(umgebung_t::crossconnect_factories) {
			const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
			slist_iterator_tpl <fabrik_t *> iter (list);
			while( iter.next() ) {
				iter.get_current()->add_all_suppliers();
			}
		}

	}
	return plan != 0;
}


/* open the list of halt */
int wkz_list_halt_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(-1, -1, -1, new halt_list_frame_t(sp), w_autodelete, magic_halt_list_t);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}


/* open the list of vehicle */
int wkz_list_vehicle_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(-1, -1, -1, new convoi_frame_t(sp, welt), w_autodelete, magic_convoi_t);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}


/* open the list of towns */
int wkz_list_town_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(0, 0, new citylist_frame_t(welt), w_info);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}



/* open the list of goods */
int wkz_list_good_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(0, 0,new goods_frame_t(), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}


/* prissi: undo building */
int wkz_undo(spieler_t *sp, karte_t *welt)
{
	if(!sp->undo()) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, translator::translate("UNDO failed!")), w_autodelete);
	}
	return false;
}
// Werkzeuge ende



/* builds company headquarter
 * @author prissi
 */
int wkz_headquarter(spieler_t *sp, karte_t *welt, koord pos)
{
	bool ok=false;

	if(pos==INIT  ||  pos==EXIT) {
		return true;
	}
DBG_MESSAGE("wkz_headquarter()", "building headquarter at (%d,%d)", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {

		int level = sp->get_headquarter_level();
		int besch_nr=-1;
		koord previous = sp->get_headquarter_pos();

		for(unsigned i=0;  i<hausbauer_t::headquarter.count();  i++  ) {
			if(hausbauer_t::headquarter.at(i)->gib_bauzeit()==level) {
				besch_nr = i;
				break;
			}
		}

		if(besch_nr<0) {
			// no further headquarter level
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
			return false;
		}

		const haus_besch_t *besch = hausbauer_t::headquarter.at(besch_nr);
		koord size = besch->gib_groesse();
		int rotate = 0;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, false)) {
			ok = true;
		}
		if(size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, false)) {
			rotate = 1;
			ok = true;
		}

		if(ok) {
			// remove previous one
			if(previous!=koord::invalid) {
				wkz_remover(sp,welt,previous);
			}
			// then built is
			hausbauer_t::baue(welt, sp, welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotate, besch, true, NULL);
			sp->add_headquarter( level+1, pos );
			sp->buche(-1000000*besch->gib_level(), pos, COST_CONSTRUCTION * size.x * size.y);
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
		}
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return ok;
}



/* switch to next player
 * @author prissi
 */
int wkz_switch_player(spieler_t *, karte_t *welt, koord pos)
{
	if(pos==INIT) {
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
		welt->switch_active_player();
	}
	return false;
}



/* change city size
 * @author prissi
 */
int wkz_grow_city(spieler_t *, karte_t *welt, koord pos, value_t lParam)
{
	stadt_t *city = welt->suche_naechste_stadt(pos);

	if(pos!=INIT  && pos!=EXIT  &&  city!=NULL) {
		city->change_size( lParam.i );
	}
	return true;
}


/* built random tourist attraction
 * @author prissi
 */
int wkz_add_attraction(spieler_t *, karte_t *welt, koord pos)
{
	if(pos!=INIT  && pos!=EXIT) {
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit();

		if(attraction==NULL) {
			return false;
		}

		koord size = attraction->gib_groesse();
		int rotation = 0;

		bool hat_platz = false;
		bool slope_check = size.x+size.y>2;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
			hat_platz = true;
		}

		if(!hat_platz &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
			rotation = 1;
			hat_platz = true;
		}

		// Platz gefunden ...
		if(hat_platz) {
			hausbauer_t::baue(welt, welt->gib_spieler(1), welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotation, attraction);
			return true;
		}
	}
	return false;
}
