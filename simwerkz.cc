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
#include "simpeople.h"

#include "bauer/fabrikbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/kanal.h"
#include "boden/tunnelboden.h"

#include "simvehikel.h"
#include "simverkehr.h"
#include "simworld.h"
#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "simintr.h"
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
#include "gui/factorylist_frame_t.h"
#include "gui/curiositylist_frame_t.h"

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
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, int b = 1, int h = 1)
{
	// here below?
	if(sp->is_my_halt(pos.gib_2d())) {
		return welt->lookup(pos.gib_2d())->gib_halt();
	}

	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos);
	if(bd==NULL) {
		bd = welt->lookup(pos.gib_2d())->gib_kartenboden();
	}

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

	halthandle_t halt=halthandle_t();

	// first try to connect to our own
	for(  int i=0;  i<iAnzahl;  i++ ) {
		const koord try_pos = pos.gib_2d()+next_try_dir[i];
		if(sp->is_my_halt(try_pos)) {
			return welt->lookup(try_pos)->gib_halt();
		}
	}

	// now just search everything
	koord k=pos.gib_2d();
	for(k.x=pos.x-1; k.x<=pos.x+b; k.x++) {
		for(k.y=pos.y-1; k.y<=pos.y+h; k.y++) {
			if(sp->is_my_halt(k)) {
				return welt->lookup(k)->gib_halt();
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
	DBG_MESSAGE("wkz_abfrage()","checking map square %d,%d", pos.x, pos.y);
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {

		const planquadrat_t *plan = welt->lookup(pos);

		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			grund_t *gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );

			if(gr) {
				// remember top window
				const gui_fenster_t *old_top = win_get_top();

				for(int n=0; n<gr->gib_top(); n++) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  (dt->gib_typ()!=ding_t::oberleitung  ||  dt->gib_typ()!=ding_t::pillar)) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						gr->obj_bei(n)->zeige_info();
						// did some new window open?
						if(umgebung_t::single_info  &&  old_top!=win_get_top()  &&  !gr->ist_wasser()) {
							return true;
						}
					}
				}

				if(gr->gib_depot()) {
					gr->gib_depot()->zeige_info();
					return true;
				}

				gr->zeige_info();
				ok = true;
			}
		}
	}
	return ok;
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
					if(welt->ist_in_kartengrenzen(pos.x+i,pos.y+j)  &&  welt->lookup(pos+koord(i,j))->gib_kartenboden()) {
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
                     halthandle_t halt, koord3d pos)
{
DBG_MESSAGE("entferne_haltestelle()","removing segment from %d,%d,%d", pos.x, pos.y, pos.z);

	grund_t *bd = welt->lookup(pos);

// Alle objekte entfernen
//	bd->obj_loesche_alle(sp);
	halt->rem_grund(bd);
	// remove station building?
	gebaeude_t *gb=dynamic_cast<gebaeude_t *>(bd->suche_obj(ding_t::gebaeude));
	if(gb) {
		bd->obj_remove(gb,sp);
		delete gb;
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("entferne_haltestelle()","remove last");
		// all deleted?
		sp->halt_remove( halt );
DBG_DEBUG("entferne_haltestelle()","destroy");
		haltestelle_t::destroy( halt );
	}
	else {
DBG_DEBUG("entferne_haltestelle()","not last");
		// may have been changed ... (due to post office/dock/railways station deletion)
		halt->recalc_station_type();
	}
DBG_DEBUG("entferne_haltestelle()","recalc bild");
	bd->calc_bild();

DBG_DEBUG("entferne_haltestelle()","reset city way owner");
	weg_t *weg = bd->gib_weg(weg_t::strasse);
	if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
		// Stadtstrassen sollten entfernbar sein, deshalb
		// dürfen sie keinen Besitzer haben.
		bd->setze_besitzer( NULL );
	}
	bd->setze_text( NULL );

	if(bd->gib_typ() == boden_t::fundament) {
		// Post/building: remove fundation
		welt->access(pos.gib_2d())->kartenboden_setzen(new boden_t(welt, pos), false);
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

	grund_t *gr=0;
      const bool backwards = (event_get_last_control_shift()==2);
    	// remove lower ground first with CNTRL
     	for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
     		if(gr->ist_im_tunnel()) {
     			// do not remove tunnel ground
     			gr = 0;
     			continue;
     		}
		// ok, something to remove from here ...
		if(gr->obj_count()>0  ||  (gr->hat_wege()  &&  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL))) {
			break;
		}
	}
	// everything removed, nothing left ...
	if(gr==NULL) {
		return true;
	}

	// prissi: Leitung prüfen (can cross ground of another player)
	leitung_t *lt=dynamic_cast<leitung_t *>(gr->suche_obj(ding_t::leitung));
	if(lt!=NULL  &&  lt->gib_besitzer()==sp) {
		gr->obj_remove(lt,sp);
		delete lt;
		return true;
	}

	// check for signal
	if(gr->suche_obj(ding_t::signal) != NULL  ||  gr->suche_obj(ding_t::presignal) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing signal %d,%d",  pos.x, pos.y);
		if(gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL) {
			blockmanager *bm = blockmanager::gib_manager();
			const bool ok = bm->entferne_signal(welt, gr->gib_pos());
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Ambiguous signal combination.\nTry removing by clicking the\nother side of the signal.\n"), w_autodelete);
			}
			return ok;
		}
		else {
			msg = "Das Feld gehoert\neinem anderen Spieler\n";
			return false;
		}
	}

	if(gr->suche_obj(ding_t::roadsign) != NULL) {
DBG_MESSAGE("wkz_remover()",  "removing roadsign %d,%d",  pos.x, pos.y);
		roadsign_t *rs = dynamic_cast<roadsign_t *>(gr->suche_obj(ding_t::roadsign));
		if(rs->gib_besitzer()==sp  ||  rs->gib_besitzer()==NULL) {
			rs->entferne(sp);
			delete rs;
			return true;
		}
		else {
			msg = "Das Feld gehoert\neinem anderen Spieler\n";
			return false;
		}
	}

	// Haltestelle prüfen
	halthandle_t halt = gr->gib_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if( halt.is_bound()  &&  (halt->gib_besitzer()==sp  ||  halt->gib_besitzer()==welt->gib_spieler(1))) {
		entferne_haltestelle(welt, sp, halt, gr->gib_pos());
		return true;
	}

	// citycar? (we allow always)
	if(gr->suche_obj(ding_t::verkehr)) {
		stadtauto_t *citycar = dynamic_cast<stadtauto_t *>(gr->suche_obj(ding_t::verkehr));
		delete citycar;
		return true;
	}

	// pedestrians?
	if(gr->suche_obj(ding_t::fussgaenger)) {
		fussgaenger_t *fussgaenger = dynamic_cast<fussgaenger_t *>(gr->suche_obj(ding_t::fussgaenger));
		delete fussgaenger;
		return true;
	}

DBG_MESSAGE("wkz_remover()", "check tunnel/bridge");

	// beginning/end of bridge?
	if(gr->ist_bruecke()) {
		if(gr==plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()",  "removing bridge from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
			weg_t *weg = gr->gib_weg(weg_t::schiene);
			if(!weg  ||  weg->gib_besch()->gib_styp()!=1) {
				if(!weg) {
					weg = gr->gib_weg(weg_t::strasse);
				}
				msg = brueckenbauer_t::remove(welt, sp, gr->gib_pos(), weg->gib_typ());
			}
		}
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()) {
		if(gr==plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()",  "removing tunnel  from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
			weg_t *weg = gr->gib_weg(weg_t::schiene);

			if(!weg) {
				weg = gr->gib_weg(weg_t::strasse);
			}
			msg = tunnelbauer_t::remove(welt, sp, gr->gib_pos(), weg->gib_typ());
		}
		return msg == NULL;
	}

#if 0
	// Depot prüfen
	depot_t * dp = gr->gib_depot();
	if(dp) {
DBG_MESSAGE("wkz_remover()", "removing %s %p from %d,%d",	dp->gib_name(), dp, pos.x, pos.y);
		gr->obj_remove(dp, sp);
		dp->entferne(sp);
		delete dp;
		return true;
	}
#endif

	msg = gr->kann_alle_obj_entfernen(sp);
	if( msg ) {
		return false;
	}

#if 1
	// since buildings can have more than one tile, we must handle them together
	gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
	if(gb) {
		DBG_MESSAGE("wkz_remover()",  "removing building" );
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

				if(welt->access(fab->gib_pos().gib_2d())->get_haltlist().get_count()!=0) {
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
				const weighted_vector_tpl<stadt_t *> *stadt = welt->gib_staedte();
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

DBG_MESSAGE("wkz_remover()", "removing building: cleanup");
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(k+pos)->gib_kartenboden();
					gr->obj_loesche_alle(sp);
					if(gr->gib_typ()==grund_t::fundament) {
						welt->access(k+pos)->kartenboden_setzen(new boden_t(welt, gr->gib_pos()), false);
					}
				}
			}

		}
		welt->rem_fab((fabrik_t*)1);
		return true;
	}
#endif

	// remove everything else ...
	if(gr->obj_count()>0  &&  !gr->ist_bruecke()) {
DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
		gr->obj_loesche_alle(sp);
		return true;
	}

	// prüfen, ob boden entfernbar
	if(gr->gib_besitzer() != NULL && gr->gib_besitzer() != sp) {
		msg = "Das Feld gehoert\neinem anderen Spieler\n";
		return false;
	}

	// ok, now we removed every object, that should be removed one by one.
	// the following objects will be removed together
	if( gr->gib_weg(weg_t::schiene) != NULL ) {
		DBG_MESSAGE("wkz_remover()",  "removing block" );
		if(!blockmanager::gib_manager()->entferne_schiene(welt, gr->gib_pos())) {
			return false;
		}
	}

DBG_MESSAGE("wkz_remover()", "removing way");
	/*
	* Eigentlich müssen wir hier noch verhindern, daß ein Bahnhofsgebäude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst lässt sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	int cost_sum = gr->weg_entfernen(weg_t::schiene, true);
	if(cost_sum>0  &&  gr->gib_weg(weg_t::strasse)) {
		// remove only railway track
		return true;
	}
	cost_sum += gr->weg_entfernen(weg_t::strasse, true);
	cost_sum += gr->weg_entfernen(weg_t::wasser, true);
	cost_sum += gr->weg_entfernen(weg_t::luft, true);

	if(cost_sum > 0) {
		sp->buche(cost_sum * CST_STRASSE, pos, COST_CONSTRUCTION);
	}
DBG_MESSAGE("wkz_remover()", "check ground");

	// now labels on water
	// replacements of ground will give a grass tile
	// => 	don't do this on water ...
	if(!gr->ist_wasser()  &&  plan->gib_kartenboden()==gr) {
		bool label = gr->gib_besitzer() && welt->gib_label_list().contains(pos);

		plan->kartenboden_setzen(new boden_t(welt, gr->gib_pos()), false);
		if(label) {
			plan->gib_kartenboden()->setze_besitzer(sp);
		}
	}

	if(gr!=plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()", "removing ground");
		// remove upper or lower ground
		plan->boden_entfernen(gr);
//		delete gr;
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
const weg_besch_t *default_track=NULL;
const weg_besch_t *default_road=NULL;

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
		}
		else if(besch->gib_styp() == 1) {
			bautyp = wegbauer_t::schiene_monorail;
		}
		else {
			bautyp = wegbauer_t::schiene;
		}
		default_track = besch;
	}
	if(besch->gib_wtyp() == weg_t::powerline) {
		bautyp = wegbauer_t::leitung;
	}
	if(besch->gib_wtyp() == weg_t::strasse) {
		default_road = besch;
	}
	if(besch->gib_wtyp() == weg_t::wasser) {
		bautyp = wegbauer_t::wasser;
	}
	if(besch->gib_wtyp() == weg_t::luft) {
		bautyp = wegbauer_t::luft;
	}

	if(pos==INIT || pos == EXIT) {  // init strassenbau
		erster = true;

		if(bauer != NULL) {
			delete bauer;
			bauer = NULL;
		}
	}
	else {
		const planquadrat_t *plan = welt->lookup(pos);

		// Hajo: check if it's safe to build here
		if(plan == NULL) {
			return false;
		}
		else {
			grund_t *gr = welt->lookup(pos)->gib_kartenboden();
			if(gr == NULL ||  gr->ist_wasser() ||  gr->kann_alle_obj_entfernen(sp)) {
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
		}
		else {
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
			if(event_get_last_control_shift()==2) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
				bauigel.set_keep_existing_ways(false);
				bauigel.calc_straight_route(start,ziel);
			}
			else {
				bauigel.set_keep_existing_faster_ways(true);
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
int wkz_station_building_aux(spieler_t *sp, karte_t *welt, koord pos, const haus_besch_t * besch)
{
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);
	const bool is_post=(besch->get_enabled()&2)!=0;

	if(welt->ist_in_kartengrenzen(pos)) {
		koord size = besch->gib_groesse();
		koord3d k=welt->lookup(pos)->gib_kartenboden()->gib_pos();
		int rotate = 0;

		bool hat_platz = false;
		bool slope_check = size.x+size.y>2;
		halthandle_t halt;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
			halt = suche_nahe_haltestelle(sp, welt, k, size.x, size.y);
			hat_platz = halt.is_bound();
		}

		if(!hat_platz  &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
			halthandle_t halt2 = suche_nahe_haltestelle(sp, welt, k, size.y, size.x);
			hat_platz = halt2.is_bound();

			if(hat_platz) {
				halt = halt2;
				rotate = 1;
			}
		}

		// is there already a halt to connect?
		if(hat_platz) {
			if(is_post  &&  halt->get_post_enabled()) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Station already\nhas a post office!\n"), w_autodelete);
			}
			else {
				hausbauer_t::baue(welt, halt->gib_besitzer(), k, rotate, besch, true, &halt);
				sp->buche(CST_POST, pos, COST_CONSTRUCTION * size.x * size.y);
			}
			halt->recalc_station_type();
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Post muss neben\nHaltestelle\nliegen!\n"), w_autodelete);
//			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
		}
		return true;
	}
	return false;
}


int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	wkz_station_building_aux(sp, welt, pos, (const haus_besch_t *)value.p);
	return true;
}


#ifdef LAGER_NOT_IN_USE
int
wkz_lagerhaus(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_lagerhaus()", "building storage shed on square %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  bool can_build = (welt->lookup(pos)->gib_boden()->kann_alle_obj_entfernen(sp) == NULL);

  if( can_build ) {
      halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);

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
    koord3d pos,
    bool nordsued,
    const haus_besch_t * besch)
{
	grund_t *bd = welt->lookup(pos);

	if(bd->kann_alle_obj_entfernen(sp)==NULL && bd->gib_weg_hang()==0) {
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);
		bool neu = !halt.is_bound();

		if(neu) {
			DBG_MESSAGE("wkz_bahnhof_aux()", "founding new station");
			halt = sp->halt_add(pos.gib_2d());
		}
		else {
DBG_MESSAGE("wkz_bahnhof_aux()", "new segment for station");
		}

		hausbauer_t::neues_gebaeude(welt, halt->gib_besitzer(), pos, nordsued ? 0 : 1, besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos.gib_2d());
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos.gib_2d(), "BF", count);
			bd->setze_text( name );
		}


	// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("haltestelle_t::add_grund()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);
		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 836" );
		}

		sp->buche(neu ? CST_BAHNHOF : CST_BAHNHOF/2, pos.gib_2d(), COST_CONSTRUCTION);
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

		const planquadrat_t *plan=welt->lookup(pos);
		grund_t *bd = NULL;
	      const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			bd=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// ignore tunnel
	     		if(bd->ist_im_tunnel()) {
	     			bd = 0;
	     			continue;
	     		}
	     		// now just for error messages, we assing a valid ground
	     		// and check for ownership
			if(sp!=welt->gib_spieler(1)  &&  (bd->gib_besitzer()!=sp)) {
				continue;
			}
			// check for rail
			if(bd->gib_weg(weg_t::schiene)==NULL) {
				continue;
			}
			// and finally check for the presence of a stop
			if(bd->gib_halt().is_bound()) {
				continue;
			}
			// ok, now we have a track, where we can build!
			break;
		}

		// these error messages should now only appear, if there is not ground suited
		if(bd->gib_weg(weg_t::schiene) == NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Bahnhof kann\nnur auf Schienen\ngebaut werden!\n"), w_autodelete);
			return false;
		}

		if(sp!=welt->gib_spieler(1)  &&  bd->gib_besitzer()!=sp) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Die Schiene\ngehoert einem\nanderen Spieler!\n"), w_autodelete);
			return false;
		}

		const bool hat_oberleitung = (bd->suche_obj(ding_t::oberleitung) != 0);
		const bool is_bridge = (bd->suche_obj(ding_t::bruecke) != 0);

		if(bd->obj_count() > hat_oberleitung+is_bridge) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			return false;
		}

		const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);
		const haus_besch_t * besch = (const haus_besch_t *) value.p;

		if(ribi_t::ist_gerade_ns(ribi)) {
			return wkz_bahnhof_aux(sp, welt, bd->gib_pos(), true, besch);
		} else if(ribi_t::ist_gerade_ow(ribi)) {
			return wkz_bahnhof_aux(sp, welt, bd->gib_pos(), false, besch);
		} else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nBahnhof ge-\nbaut werden!\n"), w_autodelete);
		}
	}
	return false;
}

static int
wkz_bushalt_aux(spieler_t *sp, karte_t *welt, koord3d pos, ribi_t::ribi ribi,const haus_besch_t * besch)
{
	grund_t *bd = welt->lookup(pos);

	if((sp==welt->gib_spieler(1)  ||  bd->gib_besitzer() == sp || bd->gib_besitzer() == 0) &&
		bd->gib_weg_hang() == 0 &&
		(!bd->gib_halt().is_bound()  ||  bd->obj_bei(0)==0  ||  bd->obj_bei(0)->gib_typ()!=ding_t::gebaeude)
	) {
DBG_MESSAGE("wkz_bushalt_aux()", "searching near stops");
		halthandle_t halt = suche_nahe_haltestelle(sp,welt,pos);
DBG_MESSAGE("wkz_bushalt_aux()", "");
		bool neu = !halt.is_bound();

		if(neu) {
			halt = sp->halt_add(pos.gib_2d());
DBG_MESSAGE("wkz_bushalt_aux()", "founding new station");
		}
		else {
DBG_MESSAGE("wkz_bushalt_aux()", "new segment for station");
		}

		bd->setze_besitzer(halt->gib_besitzer());
		hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), (ribi & ribi_t::nordsued)?0 :1, besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos.gib_2d());
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos.gib_2d(), "H", count);
			bd->setze_text( name );
		}

		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("wkz_bushalt_aux()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);

		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 930" );
		}

		sp->buche(CST_BUSHALT, pos.gib_2d(), COST_CONSTRUCTION);
	}
	return true;
}

int
wkz_bushalt(spieler_t *sp, karte_t *welt, koord pos,value_t value)
{
DBG_MESSAGE("wkz_bushalt()", "building bus stop on square %d,%d", pos.x, pos.y);

	const planquadrat_t *plan=welt->lookup(pos);
	if(plan) {

		grund_t *bd = NULL;
		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			bd=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// ignore tunnel
	     		if(bd->ist_im_tunnel()) {
	     			bd = NULL;
	     			continue;
	     		}
	     		// now just for error messages, we assing a valid ground
	     		// and check for ownership
			if(sp!=welt->gib_spieler(1)  &&  (bd->gib_besitzer()!=sp)) {
				continue;
			}
			// check for rail
			if(bd->gib_weg(weg_t::strasse)==NULL) {
				continue;
			}
			// and finally check for the presence of a stop
			if(bd->gib_halt().is_bound()) {
				continue;
			}
			// ok, now we have a track, where we can build!
			break;
		}

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
			return wkz_bushalt_aux(sp, welt, bd->gib_pos(), ribi, (const haus_besch_t *) value.p);
		}

		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann keine\nHaltestelle ge-\nbaut werden!\n"), w_autodelete);
	}
  return false;
}


#if 0
static int
wkz_airport_aux(spieler_t *sp, karte_t *welt, koord pos, ribi_t::ribi ribi, const haus_besch_t * besch)
{
	// handles construction of all(!) airport buildings
	// name must contain building subtype!
	// possible values are:
	// Tower, Stop, Terminal, Post, Hangar

	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	if((bd->gib_besitzer() == sp || bd->gib_besitzer() == 0) &&
	 bd->gib_grund_hang() == 0 &&
	 bd->gib_halt() == 0) {
		bd->setze_besitzer(sp);
		halthandle_t halt = suche_nahe_haltestelle(sp,welt,koord3d(pos.x, pos.y, 0));

		int layout = 0;
		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}

		// Stops and Hangars can only be build on runways. If the ribi is set to !=ribi_t::keine we build on a runway
		if (ribi != ribi_t::keine) {
			if (strstr(besch->gib_name(), "Stop")) {
				// build parking position
				if(!halt.is_bound()) {
						halt = sp->halt_add(pos);
						stadt_t *stadt = welt->suche_naechste_stadt(pos);
						const int count = sp->get_haltcount();
						  const char *name = stadt->haltestellenname(pos, "Airport", count);
						bd->setze_text( name );
					}
				hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
				sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
				return true;
			} else if (strstr(besch->gib_name(), "Hangar") && ribi_t::ist_einfach(ribi)) {
				// build hangar
				gebaeude_t * gb = new hangar_t(welt, bd->gib_pos(), sp, besch->gib_tile(layout, 0, 0));
		    bd->obj_pri_add(gb, PRI_DEPOT);
	      gb->setze_sync( true );
	      sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
	      return true;
			}
		} else {
			// put here everything that must not be built on runways. For example: Tower, Terminal, Freight/Mail
			if (strstr(besch->gib_name(), "Tower")) {
				// build Tower
				if (halt.is_bound()) {
					hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
					sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
	      			halt->recalc_station_type();
					return true;
				}
			} else if (strstr(besch->gib_name(), "Terminal")) {
				// get layout from neighbouring parking position - terminal will face closest parking position!
				ribi_t::ribi bauribi = ribi_t::keine;
				for (int i=0; i<4; i++) {
					koord dir = koord::nsow[i];
					grund_t *baugrund = welt->lookup(pos+dir)->gib_kartenboden();
					if (baugrund->gib_halt().is_bound() && (baugrund->gib_weg_ribi_unmasked(weg_t::luft) != ribi_t::keine)) {
						bauribi = ribi_typ(dir);
					}
				}
				switch (bauribi) {
					case ribi_t::keine:
					case ribi_t::sued: layout = 0; break;
					case ribi_t::ost:   layout = 1;    break;
					case ribi_t::nord:  layout = 2;    break;
					case ribi_t::west:  layout = 3;    break;
				}
				// build Terminal
				if (halt.is_bound()) {
					hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
					sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
			      	halt->recalc_station_type();
					return true;
				}
			} else if (strstr(besch->gib_name(), "Post")) {
				// build Post/Freight
				if (halt.is_bound()) {
					hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
			      	halt->recalc_station_type();
					sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
					return true;
				}
			}
		}
	}
	return false;
}
#else
// prissi: only terminals here for the moment
static int
wkz_airport_aux(spieler_t *sp, karte_t *welt, koord pos, ribi_t::ribi ribi, const haus_besch_t * besch)
{
	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	if((bd->gib_besitzer() == sp || bd->gib_besitzer() == 0) && bd->gib_grund_hang() == 0) {
		bd->setze_besitzer(sp);
		halthandle_t halt = suche_nahe_haltestelle(sp,welt,koord3d(pos.x, pos.y, 0));

		int layout = 0;
		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}

			if (besch->gib_utyp()==hausbauer_t::airport) {
				// build parking position
				if(!halt.is_bound()) {
						halt = sp->halt_add(pos);
						stadt_t *stadt = welt->suche_naechste_stadt(pos);
						const int count = sp->get_haltcount();
						  const char *name = stadt->haltestellenname(pos, "Airport", count);
						bd->setze_text( name );
					}
				hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
      			halt->recalc_station_type();
				sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
				return true;
			} else {
				// build hangar
				gebaeude_t * gb = new airdepot_t(welt, bd->gib_pos(), sp, besch->gib_tile(layout, 0, 0));
				bd->obj_pri_add(gb, PRI_DEPOT);
				gb->setze_sync( true );
				sp->buche(CST_TERMINAL, pos, COST_CONSTRUCTION);
				return true;
			}
	}
	return false;
}
#endif



int
wkz_airport(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	dbg->message("wkz_airport()", "building on square %d,%d", pos.x, pos.y);

	if(welt->lookup(pos)) {
		grund_t *bd = welt->lookup(pos)->gib_kartenboden();

		if(bd->suche_obj(ding_t::airdepot)) {
	    create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
	    return false;
		}

		ribi_t::ribi ribi;
		ribi = bd->gib_weg_ribi_unmasked(weg_t::luft);

	  if(ribi_t::ist_einfach(ribi)) {
	      if (wkz_airport_aux(sp, welt, pos, ribi, (const haus_besch_t *) value.p)) {
	      	return true;
	      }
		}
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann dieses\nFlughafengebaeude nicht\ngebaut werden!\n"), w_autodelete);

	}
  return false;
}




/* build a river dock either small or large */
int
wkz_kaibau(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	const haus_besch_t *besch=(const haus_besch_t *)value.p;
	bool ok = false;

	// docks können nicht direkt am kartenrand gebaut werden,
	// da die schiffe dort nicht fahren können

	if(pos == INIT || pos == EXIT) {
		// init und exit ignorieren
		return true;
	}

	const planquadrat_t *plan=welt->lookup(pos);
	const weg_t *w=0;
	if(plan  &&  !welt->get_slope(pos)) {
		grund_t *gr=plan->gib_kartenboden();
		if(gr->ist_wasser()  ||  (w=gr->gib_weg(weg_t::wasser))==NULL  ||  !ribi_t::ist_gerade(w->gib_ribi_unmasked()) ) {
			// cannot built here
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Zu nah am Kartenrand"), w_autodelete);
			return false;
		}

DBG_MESSAGE("wkz_kaibau()","search for stop");
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, gr->gib_pos() );
		bool neu = !halt.is_bound();

		if(neu) { // neues dock
DBG_MESSAGE("wkz_kaibau()","spawn new halt");
			halt = sp->halt_add(pos);
		}

DBG_MESSAGE("wkz_kaibau()","remove ground below slope");
		// remove old ground below slope

DBG_MESSAGE("wkz_kaibau()","finally errect it");
		hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), gr->gib_pos(), (w->gib_ribi_unmasked() & ribi_t::nordsued)?0 :1, besch, &halt);

DBG_MESSAGE("wkz_kaibau()","clean up");
		// Kosten
		sp->buche(CST_DOCK, pos, COST_CONSTRUCTION);
		halt->recalc_station_type();

		if(neu) {
DBG_MESSAGE("wkz_kaibau()","create new name");
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "Dock", count);
DBG_MESSAGE("wkz_kaibau()","basis pos (%i,%i)",halt->gib_basis_pos().x,halt->gib_basis_pos().y);
			gr->setze_text( name );
		}

DBG_MESSAGE("wkz_kaibau()","rebuilt destination");
		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("haltestelle_t::add_grund()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);
		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 1080" );
		}

		ok = true;
	}
	else {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Zu nah am Kartenrand"), w_autodelete);
	}

	return ok;
}



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

DBG_MESSAGE("wkz_dockbau()","search for stop");
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, welt->lookup(pos)->gib_kartenboden()->gib_pos() );
		bool neu = !halt.is_bound();

		if(neu) { // neues dock
DBG_MESSAGE("wkz_dockbau()","spawn new halt");
			halt = sp->halt_add(pos);
		}

DBG_MESSAGE("wkz_dockbau()","remove ground below slope");
		// remove old ground below slope
		welt->access(pos)->kartenboden_setzen(new boden_t(welt, welt->lookup(pos)->gib_kartenboden()->gib_pos()), false);

DBG_MESSAGE("wkz_dockbau()","finally errect it");
		hausbauer_t::baue(welt, halt->gib_besitzer(), bau_pos, layout,besch, 0, &halt);

DBG_MESSAGE("wkz_dockbau()","clean up");
		for(int i=0;  i<=len;  i++ ) {
			koord p=pos-dx*i;
			grund_t *gr = welt->lookup(p)->gib_kartenboden();
			// Kosten
			sp->buche(CST_DOCK, p, COST_CONSTRUCTION);
			// dock-land anbindung gehört auch zur haltestelle
//			halt->add_grund(gr);
		}
DBG_MESSAGE("wkz_dockbau()","recalc station type");
		halt->recalc_station_type();

		if(neu) {
DBG_MESSAGE("wkz_dockbau()","create new name");
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "Dock", count);
DBG_MESSAGE("wkz_dockbau()","basis pos (%i,%i)",halt->gib_basis_pos().x,halt->gib_basis_pos().y);
			welt->lookup(halt->gib_basis_pos())->gib_kartenboden()->setze_text( name );
		}

DBG_MESSAGE("wkz_dockbau()","rebuilt destination");
		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("haltestelle_t::add_grund()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);
		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 1080" );
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
		int fab_name_len = strlen( fab->gib_besch()->gib_name() );
		if(fab_name_len>11   &&  (strcmp(fab->gib_besch()->gib_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(fab->gib_besch()->gib_name()+fab_name_len-11, "Power Plant")==0)) {
			gr->obj_add( new pumpe_t(welt, gr->gib_pos(), sp) );
		}
		else {
			gr->obj_add(new senke_t(welt, gr->gib_pos(), sp));
		}
	}
	return true;
}


static int
wkz_frachthof_aux(spieler_t *sp, karte_t *welt, koord pos, ribi_t::ribi dir, const haus_besch_t * besch)
{
DBG_MESSAGE("wkz_frachthof_aux()",     "called on %d,%d", pos.x, pos.y);
	grund_t *bd = welt->lookup(pos)->gib_kartenboden();

	if(bd->kann_alle_obj_entfernen(sp) == NULL && bd->gib_grund_hang() == 0) {
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, bd->gib_pos() );
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

		int layout = 0;
		switch(dir) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
		hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			const int count = sp->get_haltcount();
			const char *name = stadt->haltestellenname(pos, "H", count);

			bd->setze_text( name );
		}

		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("haltestelle_t::add_grund()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);
		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 1187" );
		}

		sp->buche(CST_FRACHTHOF, pos, COST_CONSTRUCTION);
	}
	return true;
}

int
wkz_frachthof(spieler_t *sp, karte_t *welt, koord pos, value_t value)
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
      return wkz_frachthof_aux(sp, welt, pos, ribi,(const haus_besch_t *) value.p);
  }
  create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nFrachthof ge-\nbaut werden!\n"), w_autodelete);
    }
    return false;
}



// builds signal
int wkz_signal_aux(spieler_t *sp, karte_t *welt, koord pos,bool presignal)
{
	DBG_MESSAGE("wkz_signal_aux()","called on %d,%d for a %ssignal", pos.x, pos.y, presignal?"pre":"");
	if(welt->ist_in_kartengrenzen(pos)) {

		blockmanager * bm = blockmanager::gib_manager();
		const char * error = "Hier kann kein\npreSignal aufge-\nstellt werden!\n";

		const planquadrat_t *plan=welt->lookup(pos);
		const bool backwards = (event_get_last_control_shift()==2);
		const grund_t *gr=0;
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// =0 is gound, all other are either bridges or tunnel
	     		if(gr->ist_im_tunnel()) {
	     			gr = NULL;
	     			continue;
	     		}
			if(gr->gib_weg(weg_t::schiene)!=NULL  &&  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL)) {
				// found ground
				break;
			}
			gr = NULL;
		}

		if(gr) {
			error = bm->neues_signal(welt, sp, gr->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung(), presignal);
		}
		if(error != NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
		}
		return error == NULL;
	} else {
		return false;
	}
}



int wkz_signale(spieler_t *sp, karte_t *welt, koord pos)
{
	return wkz_signal_aux(sp,welt,pos,false);
}



int wkz_presignals(spieler_t *sp, karte_t *welt, koord pos)
{
	return wkz_signal_aux(sp,welt,pos,true);
}



int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", pos.x, pos.y);
	const roadsign_besch_t * besch = (const roadsign_besch_t *) (lParam.p);

	if(welt->ist_in_kartengrenzen(pos)) {

		grund_t *gr = NULL;
		weg_t *weg = NULL;
		const planquadrat_t *plan = welt->lookup(pos);
		const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";

		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// =0 is gound, all other are either bridges or tunnel
	     		if(gr->ist_im_tunnel()) {
	     			gr = NULL;
	     			continue;
	     		}
			weg = gr->gib_weg(weg_t::strasse);
			if(  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL)  &&  weg!=NULL) {
				// found ground
				break;
			}
			gr = NULL;
		}

		if(gr) {

			// get the sign dirction
			ribi_t::ribi dir = weg->gib_ribi_unmasked();
			if(ribi_t::ist_gerade(dir)  ||  (besch->is_traffic_light()  &&  ribi_t::ist_kreuzung(dir))) {

				// if single way, we need to reduce the allowed ribi to one
DBG_MESSAGE("wkz_roadsign()","dir is %i", dir);
				if(besch->is_single_way()  ||  besch->is_free_route()) {
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
					if(besch->is_single_way()  ||  besch->is_free_route()) {
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
					sp->buche( CST_ROADSIGN, pos, COST_CONSTRUCTION);
				}
				error = NULL;
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



int wkz_bahndepot(spieler_t *sp, karte_t *welt, koord pos)
{
	DBG_MESSAGE("wkz_bahndepot()","called on %d,%d", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd = welt->lookup(pos)->gib_kartenboden();

		const bool hat_oberleitung = (bd->suche_obj(ding_t::oberleitung) != 0);

		if(bd->obj_count() > (hat_oberleitung ? 1 : 0)) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			return false;
		}

		const ribi_t::ribi ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);

		if(ribi_t::ist_einfach(ribi) &&	welt->get_slope(pos) == 0 &&	bd->kann_alle_obj_entfernen(sp) == NULL) {
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

int wkz_monoraildepot(spieler_t *sp, karte_t *welt, koord pos)
{
DBG_MESSAGE("wkz_monoraildepot()","called on %d,%d", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd = welt->lookup(pos)->gib_kartenboden();

		grund_t *bd2 = welt->lookup(bd->gib_pos()+koord3d(0,0,16));	// monorail?
		if(!bd2) {
			// no monorail here ...
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nBahndepot ge-\nbaut werden!\n"), w_autodelete);
			return false;
		}

		if(bd2->obj_count() > 0) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Es ist ein\nObjekt im Weg!\n"), w_autodelete);
			return false;
		}

		const ribi_t::ribi ribi = bd2->gib_weg_ribi_unmasked(weg_t::schiene);
		if(ribi_t::ist_einfach(ribi) &&	welt->get_slope(pos) == 0 &&	bd2->kann_alle_obj_entfernen(sp) == NULL) {
			int layout = 0;

			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude( welt, sp, bd2->gib_pos(), layout,hausbauer_t::monorail_depot_besch);
			hausbauer_t::baue( welt, sp, bd->gib_pos(), layout,hausbauer_t::monorail_foundation_besch, true );

			sp->buche(CST_BAHNDEPOT, pos, COST_CONSTRUCTION);
			return true;
		}
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Hier kann kein\nBahndepot ge-\nbaut werden!\n"), w_autodelete);
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
	DBG_MESSAGE("wkz_fahrplan_add()", "Insert coordinate to schedule.");

	// haben wir einen Fahrplan ?
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_add()","Schedule is (null), doing nothing");
		return false;
	}

	if(pos!=INIT  &&  pos!=EXIT  &&  welt->ist_in_kartengrenzen(pos)) {

		const planquadrat_t *pl = welt->lookup(pos);
		const bool backwards=event_get_last_control_shift();
		const grund_t *bd=0;
		// search all grounds for match
		for(  unsigned cnt=0;  cnt<pl->gib_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->gib_boden_count()-1-cnt : cnt;
			// ignore tunnel
	     		if(pl->gib_boden_bei(i)->ist_im_tunnel()) {
	     			continue;
	     		}
	     		// now just for error messages, we assing a valid ground
	     		// and check for ownership
	     		bd = pl->gib_boden_bei(i);
			if(sp!=welt->gib_spieler(1)  &&  bd->gib_besitzer()!=sp  &&  bd->gib_besitzer()!=NULL) {
				bd = 0;
				continue;
			}
DBG_MESSAGE("wkz_fahrplan_add()", "insert pos check (%i,%i,%i)", bd->gib_pos().x, bd->gib_pos().y, bd->gib_pos().z );
			// check for rail
			if(!fpl->ist_halt_erlaubt(bd)) {
				bd = 0;
				continue;
			}
DBG_MESSAGE("wkz_fahrplan_add()", "insert pos ok (%i,%i,%i)", bd->gib_pos().x, bd->gib_pos().y, bd->gib_pos().z );
			// ok, now we have a valid ground
			break;
		}

DBG_MESSAGE("wkz_fahrplan_add()", "insert pos (%i,%i,%i)", bd->gib_pos().x, bd->gib_pos().y, bd->gib_pos().z );
		if(bd) {
			// no halt; ownership not checked here!
			fpl->append(welt, bd );
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Das Feld gehoert\neinem anderen Spieler\n"), w_autodelete);
		}
	}

	return true;
}



int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	fahrplan_t *fpl=(fahrplan_t *)f.p;
	DBG_MESSAGE("wkz_fahrplan_ins()", "Insert coordinate to schedule.");

	// haben wir einen Fahrplan ?
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_ins()","Schedule is (null), doing nothing");
		return false;
	}

	if(pos!=INIT  &&  pos!=EXIT  &&  welt->ist_in_kartengrenzen(pos)) {

		const planquadrat_t *pl = welt->lookup(pos);
		const bool backwards=event_get_last_control_shift()==2;
		const grund_t *bd=0;
		// search all grounds for match
		for(  unsigned cnt=0;  cnt<pl->gib_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->gib_boden_count()-1-cnt : cnt;
			// ignore tunnel
	     		if(pl->gib_boden_bei(i)->ist_im_tunnel()) {
	     			continue;
	     		}
	     		// now just for error messages, we assing a valid ground
	     		// and check for ownership
	     		bd = pl->gib_boden_bei(i);
			if(sp!=welt->gib_spieler(1)  &&  bd->gib_besitzer()!=sp  &&  bd->gib_besitzer()!=NULL) {
				bd = 0;
				continue;
			}
			// check for rail
			if(!fpl->ist_halt_erlaubt(bd)) {
				bd = 0;
				continue;
			}
			// ok, now we have a valid ground
			break;
		}

		if(bd) {
			// no halt; ownership not checked here!
			fpl->insert(welt, bd );
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Das Feld gehoert\neinem anderen Spieler\n"), w_autodelete);
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
		create_win(0, 0,new goods_frame_t(welt), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}



/* open the list of factories */
int wkz_list_factory_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(0, 0,new factorylist_frame_t(welt), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}


/* open the list of attraction */
int wkz_list_curiosity_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(0, 0,new curiositylist_frame_t(welt), w_autodelete);
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
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month());

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



// protects map from further change
int wkz_lock( spieler_t *, karte_t *welt, koord pos)
{
	if(pos!=INIT  && pos!=EXIT) {
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
		welt->gib_einstellungen()->setze_allow_player_change( false );
		destroy_all_win();
		welt->switch_active_player();
	}
	return false;
}



// setp one year forward
int wkz_step_year( spieler_t *, karte_t *welt, koord k)
{
	if(k == INIT) {
		welt->step_year();
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, 0, 0);
	}
	return false;
}
