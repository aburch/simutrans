/*
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
#include "simworld.h"
#include "simplay.h"
#include "simsound.h"
#include "simevent.h"
#include "simmem.h"
#include "simskin.h"
#include "simcity.h"
#include "simtools.h"

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/kanal.h"
#include "boden/tunnelboden.h"

#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "simintr.h"
#include "simhalt.h"

#include "simwerkz.h"

#include "besch/haus_besch.h"
#include "besch/way_obj_besch.h"
#include "besch/skin_besch.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/simpeople.h"

#include "gui/messagebox.h"
#include "gui/label_frame.h"
#include "gui/halt_list_filter_frame.h"
#include "gui/convoi_filter_frame.h"
#include "gui/citylist_frame_t.h"
#include "gui/goods_frame_t.h"
#include "gui/factorylist_frame_t.h"
#include "gui/factory_edit.h"
#include "gui/curiositylist_frame_t.h"

#include "dings/zeiger.h"
#include "dings/bruecke.h"
#include "dings/tunnel.h"
#include "dings/signal.h"
#include "dings/crossing.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/leitung2.h"
#include "dings/baum.h"
#include "dings/field.h"

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/fahrplan.h"
#include "dataobj/route.h"

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
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	// here below?
	halthandle_t halt = haltestelle_t::gib_halt(welt,pos.gib_2d());
	if(halt.is_bound()) {
		// owner must be checked before!!!
		return halt;
	}

	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos);
	if(bd==NULL) {
		bd = welt->lookup_kartenboden(pos.gib_2d());
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(bd->hat_weg(track_wt)) {
		ribi = bd->gib_weg_ribi_unmasked(track_wt);
	}
	if(bd->hat_weg(road_wt)) {
		ribi = bd->gib_weg_ribi_unmasked(road_wt);
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
		halt = haltestelle_t::gib_halt( welt, pos.gib_2d()+next_try_dir[i] );
		if(halt.is_bound()  &&  (sp->check_owner(halt->gib_besitzer())  ||  welt->gib_spieler(1)==sp)) {
			return halt;
		}
	}

	// now just search everything
	koord k=pos.gib_2d();
	for(k.x=pos.x-1; k.x<=pos.x+b; k.x++) {
		for(k.y=pos.y-1; k.y<=pos.y+h; k.y++) {
			halt = haltestelle_t::gib_halt( welt, k );
			if(halt.is_bound()  &&  (sp->check_owner(halt->gib_besitzer())  ||  welt->gib_spieler(1)==sp)) {
				return halt;
			}
		}
	}

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
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

				int old_count = win_get_open_count();
				for(int n=0; n<gr->gib_top(); n++) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  (dt->gib_typ()!=ding_t::wayobj  ||  dt->gib_typ()!=ding_t::pillar)) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						dt->zeige_info();
						// did some new window open?
						if(umgebung_t::single_info  &&  old_count!=win_get_open_count()  &&  !gr->ist_wasser()) {
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



int wkz_raise(spieler_t *sp, karte_t *welt, koord pos)
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+Z_TILE_STEP);
	bool ok = false;
	static bool dragging = false;
	static sint16 drag_height = 0;

	// enable DRAGGING
	if(pos==DRAGGING) {
		pos = welt->gib_zeiger()->gib_pos().gib_2d();
		if(!dragging) {
			drag_height = welt->lookup_hgt(pos)+Z_TILE_STEP;
		}
		dragging = true;
	}
	if(pos==EXIT  ||  pos==INIT) {
		dragging = false;
	}

	if(welt->ist_in_gittergrenzen(pos)) {
		const int hgt = welt->lookup_hgt(pos);

		if(hgt < 14*Z_TILE_STEP) {

			int n = 0;	// tiles changed
			if(dragging) {
				ok = true;
				// dragging may be goind up or down!
				while(welt->lookup_hgt(pos)<drag_height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>drag_height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = true;
			}
			else {
				n = welt->raise(pos);
				ok = (n!=0);
			}
			if(!ok) {
				create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
				return false;
			}
			if(n>0) {
				sp->buche(umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						const planquadrat_t* p = welt->lookup(pos + koord(i, j));
						if (p)  {
							grund_t* g = p->gib_kartenboden();
							if (g) g->calc_bild();
						}
					}
				}
			}
		}
	}
	return ok;
}

int
wkz_lower(spieler_t *sp, karte_t *welt, koord pos)
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-Z_TILE_STEP);
	bool ok = false;
	static bool dragging = false;
	static sint16 drag_height = 0;

	// enable DRAGGING
	if(pos==DRAGGING) {
		pos = welt->gib_zeiger()->gib_pos().gib_2d();
		if(!dragging) {
			drag_height = welt->lookup_hgt(pos)-Z_TILE_STEP;
		}
		dragging = true;
	}
	if(pos==EXIT  ||  pos==INIT) {
		dragging = false;
	}

	if(welt->ist_in_gittergrenzen(pos)) {

		const int hgt = welt->lookup_hgt(pos);

		if(hgt > welt->gib_grundwasser()) {

			int n = 0;	// tiles changed
			if(dragging) {
				ok = true;
				// dragging may be goind up or down!
				while(welt->lookup_hgt(pos)<drag_height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>drag_height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = true;
			}
			else {
				n = welt->lower(pos);
				ok = (n!=0);
			}
			if(!ok) {
				create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
				return false;
			}
			if(n>0) {
				sp->buche(umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						const planquadrat_t* p = welt->lookup(pos + koord(i, j));
						if (p)  {
							grund_t* g = p->gib_kartenboden();
							if (g) g->calc_bild();
						}
					}
				}
			}
		}
	}

	return ok;
}




int
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
		if(gr->gib_top()>0  &&  sp->check_owner(gr->obj_bei(0)->gib_besitzer()) ) {
			break;
		}
	}
	// everything removed, nothing left ...
	if(gr==NULL) {
		return true;
	}

	// citycar? (we allow always)
	stadtauto_t* citycar = gr->find<stadtauto_t>();
	if (citycar) {
		delete citycar;
		return true;
	}

	// pedestrians?
	fussgaenger_t* fussgaenger = gr->find<fussgaenger_t>();
	if (fussgaenger) {
		delete fussgaenger;
		return true;
	}

	// prissi: Leitung prüfen (can cross ground of another player)
	leitung_t* lt = gr->gib_leitung();
	if(lt!=NULL  &&  lt->gib_besitzer()==sp) {
		bool is_leitungsbruecke = false;
		if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
			bruecke_t* br = gr->find<bruecke_t>();
			is_leitungsbruecke = br->gib_besch()->gib_waytype()==powerline_wt;
		}
		if(is_leitungsbruecke) {
			msg = brueckenbauer_t::remove(welt, sp, gr->gib_pos(), powerline_wt );
			return msg == NULL;
		}
		else {
			lt->entferne(sp);
			delete lt;
			return true;
		}
	}

	// check for signal
	roadsign_t* rs = gr->find<signal_t>();
	if (rs == NULL) rs = gr->find<roadsign_t>();
	if(rs!=NULL) {
		msg = rs->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
DBG_MESSAGE("wkz_remover()",  "removing roadsign %d,%d",  pos.x, pos.y);
		weg_t *weg = gr->gib_weg(rs->gib_besch()->gib_wtyp());
		rs->entferne(sp);
		delete rs;
		weg->count_sign();
		return true;
	}

	// Haltestelle prüfen
	halthandle_t halt = plan->gib_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::gib_fab(welt,pos)==NULL) {
		// halt and not a factory (oil rig etc.)
		const spieler_t* owner = halt->gib_besitzer();
		if (owner == sp || owner == welt->gib_spieler(1)) {
			return haltestelle_t::remove(welt, sp, gr->gib_pos(), msg);
		}
	}

	// catenary or something like this
	wayobj_t* wo = gr->find<wayobj_t>();
	if(wo) {
		msg = wo->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		wo->entferne(sp);
		delete wo;
		return true;
	}

DBG_MESSAGE("wkz_remover()", "check tunnel/bridge");

	// beginning/end of bridge?
	if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing bridge from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
		bruecke_t* br = gr->find<bruecke_t>();
		msg = brueckenbauer_t::remove(welt, sp, gr->gib_pos(), br->gib_besch()->gib_waytype());
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing tunnel  from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
		msg = tunnelbauer_t::remove(welt, sp, gr->gib_pos(), gr->gib_weg_nr(0)->gib_waytype());
		return msg == NULL;
	}

	// fields
	field_t* f = gr->find<field_t>();
	if (f) {
		msg = f->ist_entfernbar(sp);
		if(msg==NULL) {
			f->entferne(sp);
			delete f;
			// fields have foundations ...
			koord pos = gr->gib_pos().gib_2d();
			welt->access(pos)->boden_ersetzen( gr, new boden_t(welt, gr->gib_pos(), welt->calc_natural_slope(pos) ) );
			welt->lookup_kartenboden(pos)->calc_bild();
			welt->lookup_kartenboden(pos)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if (gb != NULL) {
		const spieler_t* owner = gb->gib_besitzer();
		if (owner == sp || owner == NULL  ||  sp==welt->gib_spieler(1)) {
			if(!gb->gib_tile()->gib_besch()->can_rotate()  &&  welt->cannot_save()) {
				msg = "Not possible in this rotation!";
				return false;
			}
			DBG_MESSAGE("wkz_remover()",  "removing building" );
			const haus_tile_besch_t *tile  = gb->gib_tile();
			koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );

			// get startpos
			koord k=tile->gib_offset();
			if(k != koord(0,0)) {
				return wkz_remover_intern(sp, welt, pos-k, msg);
			}
			else {
				// remove town? (when removing townhall)
				if(gb->ist_rathaus()) {
					stadt_t *stadt = welt->suche_naechste_stadt(pos);
					if(!welt->rem_stadt( stadt )) {
						msg = "Das Feld gehoert\neinem anderen Spieler\n";
						return false;
					}
				}
				else {
					// townhall is also removed during town removal
					hausbauer_t::remove( welt, sp, gb );
				}
			}
			return true;
		}
	}

	// there is a powerline above this tile, but we do not own it
	// so we take it out and add it later again
	if(lt) {
DBG_MESSAGE("wkz_remover()",  "took out powerline");
		gr->obj_remove(lt);
	}

	// do not delete crossing, so we remove it
	crossing_t *cr = gr->find<crossing_t>(2);
	if(cr) {
		gr->obj_remove(cr);
	}

	// remove all other stuff (clouds ... )
	bool return_ok = false;
	if(gr->obj_count()>0) {
		msg = gr->kann_alle_obj_entfernen(sp);
		return_ok = (msg==NULL  &&  !(gr->gib_typ()==grund_t::brueckenboden  ||  gr->gib_typ()==grund_t::tunnelboden)  &&  gr->obj_loesche_alle(sp));
	DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
	}

	if(lt) {
DBG_MESSAGE("wkz_remover()",  "add again powerline");
		gr->obj_add(lt);
	}
	if(cr) {
		gr->obj_add(cr);
	}

	// could not delete everything
	if(msg) {
		return false;
	}
	if(return_ok) {
		return true;
	}

	// ok, now we remove every object, that should be removed one by one.
	// the following objects will be removed together
DBG_MESSAGE("wkz_remover()", "removing way");

	/*
	* Eigentlich müssen wir hier noch verhindern, daß ein Bahnhofsgebäude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst lässt sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	long cost_sum = 0;
	if(gr->gib_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w=gr->gib_weg_nr(1);
		if(gr->gib_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(w==NULL  ||  !sp->check_owner(w->gib_besitzer())) {
			w = gr->gib_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(!sp->check_owner(w->gib_besitzer())) {
				msg = w->ist_entfernbar(sp);
				return false;
			}
		}
		cost_sum = gr->weg_entfernen(w->gib_waytype(), true);
	}
	else {
		// remove upper ways ...
		if(gr->gib_weg_nr(1)) {
			cost_sum = gr->weg_entfernen(gr->gib_weg_nr(1)->gib_waytype(), true);
		}
		else {
			// delete tunnel here ...
			const tunnel_besch_t* besch = gr->find<tunnel_t>()->gib_besch();
			gr->obj_loesche_alle(sp);
			cost_sum += gr->weg_entfernen(besch->gib_waytype(), true);
			cost_sum += besch->gib_preis();
		}
	}

	if(cost_sum > 0) {
		sp->buche(-cost_sum, pos, COST_CONSTRUCTION);
		if(gr->hat_wege()) {
			return true;
		}
	}
DBG_MESSAGE("wkz_remover()", "check ground");

	if(gr!=plan->gib_kartenboden()  &&  gr->gib_top()==0) {
DBG_MESSAGE("wkz_remover()", "removing ground");
		// unmark kartenboden (is marked during underground mode deletion)
		plan->gib_kartenboden()->clear_flag(grund_t::marked);
		// remove upper or lower ground
		plan->boden_entfernen(gr);
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
			create_win( new news_img(fail), w_time_delete, magic_none);
		}
		return false;
	}

	// Nachbarschaft (Bilder) neu berechnen
	if(pos.x>1)
		welt->lookup_kartenboden(pos+koord::west)->calc_bild();
	if(pos.y>1)
		welt->lookup_kartenboden(pos+koord::nord)->calc_bild();

	if(pos.x<welt->gib_groesse_x()-1)
		welt->lookup_kartenboden(pos+koord::ost)->calc_bild();
	if(pos.y<welt->gib_groesse_y()-1)
		welt->lookup_kartenboden(pos+koord::sued)->calc_bild();

	return true;
}



/*
 * aufruf mit INIT initialisiert
 *
 * erster aufruf setzt start
 * zweiter aufruf setzt ende und baut
 */
const weg_besch_t *default_track=NULL;
const weg_besch_t *default_road=NULL;

koord3d wkz_wegebau_start=koord3d::invalid;

int
wkz_wegebau(spieler_t *sp, karte_t *welt,  koord pos, value_t lParam)
{
	static zeiger_t *wkz_wegebau_bauer = NULL;
	static bool erster = true;
	static koord3d start, ziel;

	const weg_besch_t * besch = (const weg_besch_t *) (lParam.p);
	wegbauer_t::bautyp_t bautyp = wegbauer_t::strasse;
	switch (besch->gib_wtyp()) {
		case road_wt:
			default_road = besch;
			break;

		case track_wt:
			bautyp = (besch->gib_styp() == 7 ? wegbauer_t::schiene_tram : wegbauer_t::schiene);
			default_track = besch;
			break;

		case water_wt:     bautyp = wegbauer_t::wasser;   break;
		case monorail_wt:  bautyp = wegbauer_t::monorail; break;
		case powerline_wt: bautyp = wegbauer_t::leitung;  break;
		case air_wt:       bautyp = wegbauer_t::luft;     break;
	}

	// elevated track?
	if(besch->gib_styp()==1  &&  besch->gib_wtyp()!=air_wt) {
		bautyp = (wegbauer_t::bautyp_t)((int)bautyp|(int)wegbauer_t::elevated_flag);
	}

	if(pos==INIT || pos == EXIT) {  // init strassenbau
		erster = true;

		if(wkz_wegebau_bauer != NULL) {
			wkz_wegebau_bauer->mark_image_dirty( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0), 0 );
			delete wkz_wegebau_bauer;
			wkz_wegebau_bauer = NULL;
		}
		wkz_wegebau_start=koord3d::invalid;
	}
	else {
		const planquadrat_t *plan = welt->lookup(pos);

		// Hajo: check if it's safe to build here
		if(plan == NULL) {
			return false;
		}

		grund_t *gr=NULL;
		if(grund_t::underground_mode) {
			// search all grounds for match
			for( unsigned cnt=0;  cnt<plan->gib_boden_count();  cnt++  ) {
				// with control backwards
				gr = plan->gib_boden_bei(cnt);
				// ignore tunnel
				if(gr->gib_typ()!=grund_t::tunnelboden) {
					gr = NULL;
					continue;
				}
				// check for ownership
				if(sp!=NULL  &&  (gr->obj_count()==0  ||  !sp->check_owner(gr->obj_bei(0)->gib_besitzer()))){
					gr = NULL;
					continue;
				}
			}
		}
		else {
			// normal ground; just check for ownership
			gr = plan->gib_kartenboden();
			if(gr->kann_alle_obj_entfernen(sp)!=NULL  &&  gr->gib_weg((waytype_t)besch->gib_wtyp())==NULL) {
				gr = NULL;
			}
		}

		if(gr==NULL) {
			return false;
		}

		if( erster ) {
			start = gr->gib_pos();

			DBG_MESSAGE("wkz_wegebau()", "Setting start to %d,%d,%d",start.x, start.y, start.z);

			erster = false;

			// symbol für strassenanfang setzen
			wkz_wegebau_bauer = new zeiger_t(welt, start, sp);
			wkz_wegebau_bauer->setze_bild( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0) );
			gr->obj_add(wkz_wegebau_bauer);
			wkz_wegebau_start = start;
		}
		else {
			// Hajo: symbol für strassenanfang entfernen
			wkz_wegebau_bauer->mark_image_dirty( wkz_wegebau_bauer->gib_bild(), 0 );
			delete wkz_wegebau_bauer;
			wkz_wegebau_bauer = NULL;
			wkz_wegebau_start = koord3d::invalid;

			wegbauer_t bauigel(welt, sp);

			// Hajo: to test building as disguised AI player
			// wegbauer_t bauigel(welt, welt->gib_spieler(2));
			erster = true;

			ziel = gr->gib_pos();
			DBG_MESSAGE("wkz_wegebau()", "Setting end to %d,%d,%d",ziel.x, ziel.y, ziel.z);

			// Hajo: to test baubaer mode (only for AI)
			// bauigel.baubaer = true;

			display_show_load_pointer(true);
			bauigel.route_fuer(bautyp, besch);
			if(event_get_last_control_shift()==2  ||  grund_t::underground_mode) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
				bauigel.set_keep_existing_ways(false);
				bauigel.calc_straight_route(start,ziel);
			}
			else {
				bauigel.set_keep_existing_faster_ways(true);
				bauigel.calc_route(start,ziel);
			}

			DBG_MESSAGE("wkz_wegebau()", "builder found route with %d sqaures length.", bauigel.max_n);

			long cost = bauigel.calc_costs();
			welt->mute_sound(true);
			bauigel.baue();
			welt->mute_sound(false);
			if(cost>10000) {
				struct sound_info info;
				info.index = SFX_CASH;
				info.volume = 255;
				info.pri = 0;
				sound_play(info);
			}
			display_show_load_pointer(false);
		}
	}
	return true;
}


// converts a 2d koord to a suitable ground pointer
grund_t *
wkz_intern_koord_to_weg_grund(spieler_t *sp, karte_t *welt, koord pos, waytype_t wt)
{
	const planquadrat_t *plan = welt->lookup(pos);

	// check for valid ground
	if(plan==NULL) {
		return NULL;
	}
	if(wt==tram_wt) {
		wt = track_wt;
	}

	const bool backwards=event_get_last_control_shift()==2;

	grund_t *gr=NULL;
	// search all grounds for match
	for( unsigned cnt=0;  cnt<plan->gib_boden_count();  cnt++  ) {
		// with control backwards
		const unsigned i = (backwards) ? plan->gib_boden_count()-1-cnt : cnt;
		gr = plan->gib_boden_bei(i);
		// ignore tunnel
		if(gr->ist_im_tunnel()) {
			gr = NULL;
			continue;
		}
		// has some rail or monorail?
		if(!gr->hat_weg(wt)) {
			gr = NULL;
			continue;
		}
		// check for ownership
		if(sp!=NULL  &&  !sp->check_owner(gr->gib_weg(wt)->gib_besitzer())){
			gr = NULL;
			continue;
		}
		// ok, now we have a valid ground
		break;
	}
	return gr;
}





/* removes a way like a driving car ... */
int
wkz_wayremover(spieler_t *sp, karte_t *welt,  koord pos, value_t lParam)
{
	static zeiger_t *wkz_wayremover_bauer = NULL;
	waytype_t wt=(waytype_t)(lParam.i);
	static bool erster = true;
	static koord3d start, end;

	if(pos==INIT || pos == EXIT) {  // init strassenbau
		erster = true;

		if(wkz_wayremover_bauer!=NULL) {
			wkz_wayremover_bauer->mark_image_dirty( skinverwaltung_t::killzeiger->gib_bild_nr(0), 0 );
			delete wkz_wayremover_bauer;
			wkz_wayremover_bauer = NULL;
		}
		start = end = koord3d::invalid;
	}
	else {
		// search for starting ground
		grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos,wt);
		if(gr==NULL) {
			DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
			// wrong ground or not this way here => exit
			return false;
		}
		// do not remove ground from depot
		if(gr->gib_depot()) {
			return false;
		}

		if( erster ) {
			// set start position
			erster = false;
			start = gr->gib_pos();
			wkz_wayremover_bauer = new zeiger_t(welt, start, sp);
			wkz_wayremover_bauer->setze_bild( skinverwaltung_t::killzeiger->gib_bild_nr(0));
			gr->obj_add(wkz_wayremover_bauer);
DBG_MESSAGE("wkz_wayremover()", "Setting start to %d,%d,%d",start.x, start.y,start.z);
		}
		else {
DBG_MESSAGE("wkz_wayremover()", "Setting end to %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y,gr->gib_pos().z);

			// remove marker
			wkz_wayremover_bauer->mark_image_dirty( skinverwaltung_t::killzeiger->gib_bild_nr(0), 0 );
			delete wkz_wayremover_bauer;
			wkz_wayremover_bauer = NULL;
			erster = true;

			// get a default vehikel
			vehikel_besch_t remover_besch(wt, 1, vehikel_besch_t::diesel );
			vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
			route_t verbindung;
			bool can_delete = verbindung.calc_route(welt, start, gr->gib_pos(), test_driver, 0);
			delete test_driver;
			DBG_MESSAGE("wkz_wayremover()", "route search returned %d", can_delete);

			if(!can_delete) {
				DBG_MESSAGE("wkz_wayremover()","no route found");
				return false;
			}

DBG_MESSAGE("wkz_wayremover()","route with %d tile found",verbindung.gib_max_n());


			// found a route => check if I can delete anything on it
			for( int i=0;  can_delete  &&  i<=verbindung.gib_max_n();  i++  ) {
				grund_t *gr=welt->lookup(verbindung.position_bei(i));
				if(gr) {
					if(gr->gib_weg(wt)==NULL  ||  !sp->check_owner(gr->gib_weg(wt)->gib_besitzer())) {
						can_delete = false;
					}
					else {
						if(gr->kann_alle_obj_entfernen(sp)!=NULL) {
							// we have to do a fine check
							for( uint i=1;  i<gr->gib_top();  i++  ) {
								uint8 type = gr->obj_bei(i)->gib_typ();
								if(type>=ding_t::automobil  &&  type!=ding_t::aircraft) {
									can_delete = false;
									break;
								}
							}
						}
					}
				}
				else {
					can_delete = false;
				}

			}

			// if successful => delete everything
			for( int i=0;  can_delete  &&  i<=verbindung.gib_max_n();  i++  ) {

				grund_t *gr=welt->lookup(verbindung.position_bei(i));

				// ground can be missing after deleting a bridge ...
				if(gr  &&  !gr->ist_wasser()) {

					if(gr->ist_bruecke()) {
						//  first remove bridge
						brueckenbauer_t::remove(welt,sp,verbindung.position_bei(i),wt);
						// renew ground
						gr=welt->lookup(verbindung.position_bei(i));
					}

					// may be invalid after removing tunnel or bridge
					if(!gr) {
						continue;
					}

					// now the tricky part: delete just part of a way (or everything, if possible)
					// calculated removing directions
					ribi_t::ribi rem = (i>0) ? ribi_typ( verbindung.position_bei(i).gib_2d(), verbindung.position_bei(i-1).gib_2d() ) : 0;
					ribi_t::ribi rem2 = (i<verbindung.gib_max_n()) ? ribi_typ(verbindung.position_bei(i).gib_2d(),verbindung.position_bei(i+1).gib_2d()) : 0;
					rem = ~(rem|rem2);

					if( 0&& (i==0  ||  i==verbindung.gib_max_n()) &&  gr->get_flag(grund_t::is_kartenboden)  &&  gr->gib_typ()==grund_t::tunnelboden)  {
						gr->weg_entfernen( wt, rem );
					}
					else if(!gr->get_flag(grund_t::is_kartenboden)  &&  (gr->gib_typ()==grund_t::tunnelboden  ||  gr->gib_typ()==grund_t::monorailboden)  &&  gr->gib_weg_nr(0)->gib_waytype()==wt) {
						can_delete &= gr->remove_everything_from_way(sp,wt,rem);
						if(can_delete  &&  gr->gib_weg(wt)==NULL) {
							if(gr->gib_weg_nr(0)!=0) {
								gr->remove_everything_from_way(sp,gr->gib_weg_nr(0)->gib_waytype(),ribi_t::alle);
							}
							gr->obj_loesche_alle(sp);
							// delete tunnel ground too, if empty
							welt->access(gr->gib_pos().gib_2d())->boden_entfernen(gr);
						}
					}
					else {
						can_delete &= gr->remove_everything_from_way(sp,wt,rem);
					}
				}
				// ok, now everything removed ...
			}

			// return success
			return can_delete;
		}
	}
	return true;
}



const way_obj_besch_t *default_electric=NULL;

/* add catenary during construction */
int
wkz_wayobj(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	static zeiger_t *wkz_wayobj_bauer = NULL;
	const way_obj_besch_t *besch=(const way_obj_besch_t *)(lParam.p);
	waytype_t wt=(waytype_t)besch->gib_wtyp();
	static bool erster = true;
	static koord3d start, end;

	// save default overheadwires
	if(besch->gib_wtyp()==track_wt  &&  besch->gib_own_wtyp()==overheadlines_wt) {
		default_electric = besch;
	}

	if(pos==INIT || pos == EXIT) {  // init strassenbau
		erster = true;

		if(wkz_wayobj_bauer!=NULL) {
			delete wkz_wayobj_bauer;
			wkz_wayobj_bauer = NULL;
		}
		start = end = koord3d::invalid;
	}
	else {
		// search for starting ground
		grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos,wt);
		if(gr==NULL) {
			DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
			// wrong ground or not this way here => exit
			return false;
		}

		if( erster ) {
			// set start position
			erster = false;
			start = gr->gib_pos();
			wkz_wayobj_bauer = new zeiger_t(welt, start, sp);
			wkz_wayobj_bauer->setze_bild( besch->gib_cursor()->gib_bild_nr(0) );
			gr->obj_add(wkz_wayobj_bauer);
			DBG_MESSAGE("wkz_wayremover()", "Setting start to %d,%d,%d",start.x, start.y,start.z);
		}
		else {
			DBG_MESSAGE("wkz_wayremover()", "Setting end to %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y,gr->gib_pos().z);

			// remove marker
			delete wkz_wayobj_bauer ;
			wkz_wayobj_bauer = NULL;
			erster = true;

			// get a default vehikel
			vehikel_besch_t remover_besch(wt, 1, vehikel_besch_t::diesel );
			vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
			route_t verbindung;
			bool can_built = verbindung.calc_route(welt, start, gr->gib_pos(), test_driver, 0);
			delete test_driver;
			DBG_MESSAGE("wkz_wayremover()","route search returned %d",can_built);

			if(!can_built) {
				DBG_MESSAGE("wkz_wayremover()","no route found");
				return false;
			}

			// built wayobj ...
			koord3d last_pos, pos=verbindung.position_bei(0);
			uint8 dir = ribi_t::alle;
			for(int i=1;  i<=verbindung.gib_max_n();  i++  ) {
				last_pos = pos;
				pos = verbindung.position_bei(i);
				uint8 last_dir = ribi_t::rueckwaerts(dir);
				dir = ribi_typ((pos-last_pos).gib_2d());
				koord3d pos=verbindung.position_bei(i);
				wayobj_t::extend_wayobj_t(welt,last_pos,sp,dir|last_dir,besch);
			}
			// last point
			wayobj_t::extend_wayobj_t(welt,pos,sp,ribi_t::rueckwaerts(dir),besch);
			return true;
		}
	}
	return false;
}


/* build all kind of station buildings
 * @author V. Meyer
 */
static int
wkz_station_building_aux(spieler_t *sp, karte_t *welt, koord pos, const haus_besch_t * besch)
{
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);
	const bool is_post=(besch->get_enabled()&2)!=0;

	if(welt->ist_in_kartengrenzen(pos)) {
		koord size = besch->gib_groesse();
		koord3d k=welt->lookup(pos)->gib_kartenboden()->gib_pos();
		int rotate=-1, built_rotate=-1;
		static koord rotate_koords[4]={koord(0,-1),koord(1,0),koord(0,1),koord(-1,0)};
		halthandle_t halt;

		// get initial rotation
		if(besch->gib_all_layouts()>1) {
//DBG_MESSAGE("wkz_station_building_aux()","building %s; layouts %i",besch->gib_name(),besch->gib_all_layouts());
			uint8 best=0;
			for( int i=0;  i<4;  i++  ) {
				if(welt->ist_in_kartengrenzen(pos+rotate_koords[i])) {
					grund_t *gr=welt->lookup(pos+rotate_koords[i])->gib_kartenboden();
					if(gr->is_halt()) {
//DBG_MESSAGE("wkz_station_building_aux()","test layouts %i",i);
						// get best alignment judging all
						uint8 current=1;
						if(gr->hat_weg(track_wt)) {
							current += 2;
						} else if(gr->hat_weg(road_wt)) {
							current += 1;
						} else if(gr->hat_weg(water_wt)) {
							current += 1;
						} else if(gr->hat_weg(air_wt)) {
							current += 3;
						} else if(gr->gib_typ()==grund_t::fundament) {
							// always samle layout as next station building
							built_rotate = static_cast<gebaeude_t *>(gr->first_obj())->gib_tile()->gib_layout()%besch->gib_all_layouts();
//DBG_MESSAGE("wkz_station_building_aux()","find building with layout %i",built_rotate);
							continue;
						}
						if(current>best) {
							best = current;
							rotate = i;
//DBG_MESSAGE("wkz_station_building_aux()","propose layout %i",i);
						}
					}
				}
			}
		}

		// if a station building is found and nothing else, then mimic the station building alingment
		if(rotate!=-1) {
			built_rotate = rotate;
		}
		else if(built_rotate==-1) {
			// nothing found ... should never happen!!!
dbg->warning("wkz_station_building_aux()","no near building for a station extension found");
			built_rotate = 0;
		}

		// now try to built it (best layout first)
		for( int i=0;  i<besch->gib_all_layouts()  &&  !halt.is_bound();  i++  ) {
			if(((i+built_rotate)&1)!=0) {
				if(welt->ist_platz_frei(pos, size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
					halt = suche_nahe_haltestelle(sp, welt, k, size.y, size.x);
				}
			}
			else {
				if(welt->ist_platz_frei(pos, size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
					halt = suche_nahe_haltestelle(sp, welt, k, size.y, size.x);
				}
			}
			rotate = (built_rotate+i)%besch->gib_all_layouts();
		}

		// is there already a halt to connect?
		if(halt.is_bound()) {
			if(is_post  &&  halt->get_post_enabled()) {
				create_win( new news_img("Station already\nhas a post office!\n"), w_time_delete, magic_none);
			}
			hausbauer_t::baue(welt, halt->gib_besitzer(), k, rotate, besch, &halt);
			sp->buche(umgebung_t::cst_multiply_post*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
			halt->recalc_station_type();
		}
		else {
			create_win( new news_img("Post muss neben\nHaltestelle\nliegen!\n", besch->gib_cursor()->gib_bild_nr(0)), w_time_delete, magic_none);
		}
		return true;
	}
	return false;
}



/* any station extension building here: Alignment automatically! */
int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	if(pos == INIT || pos == EXIT) {
		// init => set area
		welt->gib_zeiger()->setze_area( welt->gib_einstellungen()->gib_station_coverage() );
		return true;
	}
	// are we allowed to built here?
	halthandle_t halt=haltestelle_t::gib_halt(welt,pos);
	if(halt.is_bound()  &&  !sp->check_owner(halt->gib_besitzer())) {
		// we cannot connect to this halt!
		create_win( new news_img("Das Feld gehoert\neinem anderen Spieler\n"), w_time_delete, magic_none);
		return false;
	}
	wkz_station_building_aux(sp, welt, pos, (const haus_besch_t *)value.p);
	return true;
}


/* build a dock either small or large */
int
wkz_dockbau(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	const haus_besch_t *besch=(const haus_besch_t *)value.p;

	if(pos == INIT || pos == EXIT) {
		// init und exit ignorieren
		welt->gib_zeiger()->setze_area( welt->gib_einstellungen()->gib_station_coverage() );
		return true;
	}
	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}
	// the cursor cannot be outside the map from here on
	hang_t::typ hang = welt->lookup(pos)->gib_kartenboden()->gib_grund_hang();
	// first get the size
	int len = besch->gib_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;

	// check, if we can built here ...
	const char *msg=NULL;
	if(!hang_t::ist_einfach(hang)) {
		msg = "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->ist_in_kartengrenzen(pos-dx*i)) {
				// need at least a signle tile to navigate ...
				msg = "Zu nah am Kartenrand";
				break;
			}
			else {
				halthandle_t halt = welt->lookup(pos-dx*i)->gib_halt();
				if(halt.is_bound()  &&  !sp->check_owner(halt->gib_besitzer())) {
					msg = "Das Feld gehoert\neinem anderen Spieler\n";
					break;
				}
				else {
					const grund_t *gr=welt->lookup(pos-dx*i)->gib_kartenboden();
					msg = gr->kann_alle_obj_entfernen(sp);
					if(msg) {
						break;
					}
					else if(i==0  &&  (gr->ist_wasser()  ||  gr->hat_wege()  ||  gr->gib_typ()!=grund_t::boden)  ||  gr->obj_count()>0  ||  gr->is_halt()) {
						msg = "Tile not empty.";
						break;
					}
					else if (i!=0  &&  (!gr->ist_wasser() || gr->find<gebaeude_t>() || gr->gib_depot() || gr->is_halt())) {
						msg = "Tile not empty.";
						break;
					}
				}
			}
		}
	}

	if(msg==NULL) {
DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);

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

//DBG_MESSAGE("wkz_dockbau()","search for stop");
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, welt->lookup(pos)->gib_kartenboden()->gib_pos() );
		bool neu = !halt.is_bound();

		if(neu) { // neues dock
//DBG_MESSAGE("wkz_dockbau()","spawn new halt");
			halt = sp->halt_add(pos);
		}

//DBG_MESSAGE("wkz_dockbau()","finally errect it");
		hausbauer_t::baue(welt, halt->gib_besitzer(), bau_pos, layout, besch, &halt);

//DBG_MESSAGE("wkz_dockbau()","clean up");
		for(int i=0;  i<=len;  i++ ) {
			koord p=pos-dx*i;
			sp->buche(umgebung_t::cst_multiply_dock*besch->gib_level(), p, COST_CONSTRUCTION);
		}
//DBG_MESSAGE("wkz_dockbau()","recalc station type");
		halt->recalc_station_type();
		if(umgebung_t::station_coverage_show  &&  welt->gib_zeiger()->gib_pos().gib_2d()==pos) {
			// since we are larger now ...
			halt->mark_unmark_coverage( true );
		}

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			if(stadt) {
				const int count = sp->get_haltcount();
				char* name = stadt->haltestellenname(pos, "Dock", count);
				halt->setze_name( name );
				free(name);
			}
			else {
				// get a default name
				const int count = sp->get_haltcount();
				char *tmp=new char[256];
				sprintf( tmp, translator::translate("land stop %i %s"), count, translator::translate("Dock") );
				halt->setze_name( tmp );
			}
		}

		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
		// TODO: only when there is this tile in a schedule ...
		welt->set_schedule_counter();
		return true;
	}
	else {
		create_win( new news_img(msg), w_time_delete, magic_none);
		return false;
	}
}



// built all types of stops but sea harbours
bool wkz_halt_aux(spieler_t *sp, karte_t *welt, koord pos, const char *&p_error, const haus_besch_t *besch,waytype_t wegtype,sint64 cost,const char *type_name)
{
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->gib_name(), pos.x, pos.y, wegtype);
	p_error=(besch->gib_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	grund_t *bd = wkz_intern_koord_to_weg_grund(sp==welt->gib_spieler(1)?NULL:sp,welt,pos,wegtype);
	if(!bd  &&  track_wt) {
		bd = wkz_intern_koord_to_weg_grund(sp==welt->gib_spieler(1)?NULL:sp,welt,pos,monorail_wt);
	}

	if(!bd  ||  bd->gib_weg_hang()!=hang_t::flach  ||  bd->is_halt()) {
		// only flat tiles, only one stop per map square
		return false;
	}

	if(bd->gib_typ()==grund_t::tunnelboden  &&  bd->ist_karten_boden()) {
		// do not build on tunnel entries
		return false;
	}

	if(bd->gib_depot()) {
		p_error = "Tile not empty.";
		return false;
	}

	if(bd->hat_weg(air_wt)  &&  bd->gib_weg(air_wt)->gib_besch()->gib_styp()==1) {
		return false;
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi  ribi;
	if(besch->gib_all_layouts()==2 || besch->gib_all_layouts()==8 || besch->gib_all_layouts()==16) {
		// through station
		if(bd->has_two_ways()) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->gib_weg_nr(0)->gib_ribi_unmasked()  |  bd->gib_weg_nr(1)->gib_ribi_unmasked();
		}
		else {
			ribi = bd->gib_weg_ribi_unmasked(wegtype);
		}
		// not straight: sorry cannot built here ...
		if(!ribi_t::ist_gerade(ribi)) {
			return false;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(besch->gib_all_layouts()==4) {
		// terminal station
		ribi = bd->gib_weg_ribi_unmasked(wegtype);
		// sorry cannot built here ... (not a terminal tile)
		if(!ribi_t::ist_einfach(ribi)) {
			return false;
		}

		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	} else {
		// something wrong with station number of layouts
		return false;
	}

	if(besch->gib_all_layouts()==8 || besch->gib_all_layouts()==16) {
		// through station - complex layout
		// bits
		// 1 = north south/east west (as simple layout)
		// 2 = use far end image  \ can be combined
		// 3 = use near end image / to use both end image
		// 4 = platform face - 0 = far, 1 = near

		// bit 1 has already been set

		ribi_t::ribi next_halt = ribi_t::keine;
		ribi_t::ribi next_own = ribi_t::keine;

		sint16 offset = bd->gib_hoehe()+bd->gib_weg_yoff()/TILE_HEIGHT_STEP;

		grund_t *gr;
		sint32 neighbour_layout[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
		for(unsigned i=0;  i<4;  i++) {
			// oriented buildings here - get neighbouring layouts
			const planquadrat_t *plan = welt->lookup(pos+koord::nsow[i]);
			if(plan  &&  plan->gib_halt().is_bound()) {
				// ok, here is a halt at least
				next_halt |= ribi_t::nsow[i];
				gr = welt->lookup(koord3d(pos+koord::nsow[i],offset));
				if(!gr) {
					// check whether bridge end tile
					grund_t * gr_tmp = welt->lookup(koord3d(pos+koord::nsow[i],offset-1));
					if(gr_tmp && gr_tmp->gib_weg_yoff()/TILE_HEIGHT_STEP == 1) {
						gr = gr_tmp;
					}
				}
				if(gr) {
					// check, if there is an oriented stop
					const gebaeude_t* gb = gr->find<gebaeude_t>();
					if(gb  &&  gb->gib_tile()->gib_besch()->gib_all_layouts()>4) {
						next_own |= ribi_t::nsow[i];
						neighbour_layout[ribi_t::nsow[i]] = gb->gib_tile()->gib_layout();
					}
				}
			}
		}

		// now for the details
		ribi_t::ribi senkrecht = ~ribi_t::doppelt(ribi);
		ribi_t::ribi waagerecht = ribi_t::doppelt(ribi);
		if(next_own!=ribi_t::keine) {
			// oriented buildings here
			if(ribi_t::ist_einfach(ribi & next_own)) {
				// only a single next neighbour on the same track
				layout |= neighbour_layout[ribi & next_own] & 8;
			}
			else if(ribi_t::ist_gerade(ribi & next_own)) {
				// two neighbours on the same track, use the north/west one
				layout |= neighbour_layout[ribi & next_own & ribi_t::nordwest] & 8;
			}
			else if(ribi_t::ist_einfach((~ribi) & waagerecht & next_own)) {
				// neighbour across break in track
				layout |= neighbour_layout[(~ribi) & waagerecht & next_own] & 8;
			}
			else {
				// no buildings left and right
				// oriented buildings left and right
				if(neighbour_layout[senkrecht & next_own & ribi_t::nordwest] != -1) {
					// just rotate layout
					layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::nordwest]&8);
				}
				else {
					if(neighbour_layout[senkrecht & next_own & ribi_t::suedost] != -1) {
						layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::suedost]&8);
					}
				}
			}
		}
		// avoid orientation on 8 tiled buildings
		layout &= (besch->gib_all_layouts()-1);
	}

	// seems everything ok, lets build
	halthandle_t halt = suche_nahe_haltestelle(sp,welt,bd->gib_pos());
	bool neu = !halt.is_bound();

	if(neu) {
		halt = sp->halt_add(pos);
DBG_MESSAGE("wkz_halt_aux()", "founding new station");
	}
	else {
DBG_MESSAGE("wkz_halt_aux()", "new segment for station");
		if(sp==welt->gib_spieler(1)) {
			halt->transfer_to_public_owner();
		}
	}

	hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		stadt_t *stadt = welt->suche_naechste_stadt(pos);
		if(stadt) {
			const int count = sp->get_haltcount();
			char* name = stadt->haltestellenname(pos, type_name, count);
			halt->setze_name( name );
			free(name);
		}
		else {
			// get a default name
			const int count = sp->get_haltcount();
			char *tmp=new char[256];
			sprintf( tmp, translator::translate("land stop %i %s"), count,  translator::translate(type_name) );
			halt->setze_name( tmp );
		}
	}

	// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
	// TODO: only when there is this tile in a schedule ...
	welt->set_schedule_counter();

	sp->buche(cost*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
	if(umgebung_t::station_coverage_show  &&  welt->gib_zeiger()->gib_pos().gib_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}
	return true;
}



/* built a halt on a way
 * cannot built sea harbours, since those are really different ...
 * @author prissi
 */
int wkz_halt(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	if(pos==INIT  ||  pos==EXIT) {
		welt->gib_zeiger()->setze_area( welt->gib_einstellungen()->gib_station_coverage() );
		return true;
	}

	if(!welt->ist_in_kartengrenzen(pos)) {
		return pos==DRAGGING;
	}

	bool success = false;
	const char *msg = "Das Feld gehoert\neinem anderen Spieler\n";

	// ownership allowed?
	halthandle_t halt = welt->lookup(pos)->gib_halt();
	if(!halt.is_bound()  ||  sp->check_owner(halt->gib_besitzer())) {

		const haus_besch_t *besch=(const haus_besch_t *)value.p;
		switch (besch->gib_utyp()) {
			case haus_besch_t::bahnhof:
				success = wkz_halt_aux(sp, welt, pos, msg, besch, track_wt,    umgebung_t::cst_multiply_station,     "BF");
				break;
			case haus_besch_t::monorailstop:
				success = wkz_halt_aux(sp, welt, pos, msg, besch, monorail_wt, umgebung_t::cst_multiply_station,     "BF");
				break;
			case haus_besch_t::bushalt:
			case haus_besch_t::ladebucht:
				success = wkz_halt_aux(sp, welt, pos, msg, besch, road_wt,     umgebung_t::cst_multiply_roadstop,    "H");
				break;
			case haus_besch_t::binnenhafen:
				success = wkz_halt_aux(sp, welt, pos, msg, besch, water_wt,    umgebung_t::cst_multiply_dock,        "Dock");
				break;
			case haus_besch_t::airport:
				success = wkz_halt_aux(sp, welt, pos, msg, besch, air_wt,      umgebung_t::cst_multiply_airterminal, "Airport");
				break;
			default:
				DBG_MESSAGE("wkz_halt()", "called with unknown besch %s", besch->gib_name());
				return false;
		}
	}

	// we had an error?
	if(!success) {
		dbg->warning("wkz_halt_aux()", "failed '%s' at (%i,%i)", msg, pos.x, pos.y );
		if(sp==welt->get_active_player()) {
			create_win(new news_img(msg), w_time_delete, magic_none );
		}
	}
	return success;
}



int
wkz_senke(spieler_t *sp, karte_t *welt, koord pos)
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", pos.x, pos.y);

	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}

	grund_t *gr=welt->lookup(pos)->gib_kartenboden();
	if(gr->gib_grund_hang()==0  &&  !gr->ist_wasser()  &&  	!gr->hat_wege()  &&  gr->kann_alle_obj_entfernen(sp)==NULL) {

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
				create_win( new news_img(fail), w_time_delete, magic_none);
			}
		}
		// now decide from the string whether a source or drain is built
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		if(fab->gib_besch()->is_electricity_producer()) {
			pumpe_t *p = new pumpe_t(welt, gr->gib_pos(), sp);
			gr->obj_add( p );
			p->laden_abschliessen();
		}
		else {
			senke_t *s = new senke_t(welt, gr->gib_pos(), sp);
			gr->obj_add(s);
			s->laden_abschliessen();
		}
	}
	return true;
}



int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", pos.x, pos.y);
	const roadsign_besch_t * besch = (const roadsign_besch_t *) (lParam.p);

	if(welt->ist_in_kartengrenzen(pos)) {

		const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
		// search for starting ground
		grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos,besch->gib_wtyp());
		if(gr) {
			// get the sign dirction
			weg_t *weg = gr->gib_weg(besch->gib_wtyp());
			signal_t *s = gr->find<signal_t>();
			if(s  &&  s->gib_besch()!=besch) {
				// only one signe per tile
				create_win( new news_img(error), w_time_delete, magic_none);
				return false;
			}
			if(besch->is_signal()  &&  gr->find<roadsign_t>())  {
				// only one sign per tile
				create_win( new news_img(error), w_time_delete, magic_none);
				return false;
			}
			ribi_t::ribi dir = weg->gib_ribi_unmasked();

			const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

			if(ribi_t::doppelt(dir)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
				roadsign_t* rs;
				if (besch->is_signal_type()) {
					// if there is already a signal, we might need to inverse the direction
					rs = gr->find<signal_t>();
					if (rs) {
						// signals have three options
						ribi_t::ribi sig_dir = rs->get_dir();
						uint8 i = 0;
						if (!ribi_t::is_twoway(sig_dir)) {
							// inverse first dir
							for (; i < 4; i++) {
								if ((dir & ribi_t::nsow[i]) == sig_dir) {
									i++;
									break;
								}
							}
						}
						// find the second dir ...
						for (; i < 4; i++) {
							if ((dir & ribi_t::nsow[i]) != 0) {
								dir = ribi_t::nsow[i];
							}
						}
						// if nothing found, we have two ways again ...
						rs->set_dir(dir);
					} else {
						// add a new signal at position zero!
						rs = new signal_t(welt, sp, gr->gib_pos(), dir, besch);
						DBG_MESSAGE("wkz_roadsign()", "new signal, dir is %i", dir);
						goto built_sign;
					}
				} else {
					// if there is already a sign, we might need to inverse the direction
					rs = gr->find<roadsign_t>();
					if (rs) {
						// reverse only if single way sign
						if (besch->is_single_way() || besch->is_free_route()) {
							dir = ~rs->get_dir() & weg->gib_ribi_unmasked();
							rs->set_dir(dir);
							DBG_MESSAGE("wkz_roadsign()", "reverse ribi %i", dir);
						}
					} else {
						// add a new roadsign at position zero!
						// if single way, we need to reduce the allowed ribi to one
						if (besch->is_single_way() || besch->is_free_route()) {
							for (int i = 0; i <= 4; i++) {
								if ((dir & ribi_t::nsow[i]) != 0) {
									dir = ribi_t::nsow[i];
									break;
								}
							}
						}
						DBG_MESSAGE("wkz_roadsign()", "new roadsign, dir is %i", dir);
						rs = new roadsign_t(welt, sp, gr->gib_pos(), dir, besch);
built_sign:
						gr->obj_add(rs);
						rs->laden_abschliessen();	// to make them visible
						weg->count_sign();
						sp->buche(-besch->gib_preis(), pos, COST_CONSTRUCTION);
					}
				}
				error = NULL;
			}
		}

		if(error != NULL) {
			create_win( new news_img(error), w_time_delete, magic_none);
		}
		return error == NULL;

	} else {

		return false;
	}
}




// built all types of depots
static bool
wkz_depot_aux(spieler_t *sp, karte_t *welt, koord pos, const haus_besch_t *besch,waytype_t wegtype,sint64 cost)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup(pos)->gib_kartenboden();
			if(!bd->ist_wasser()) {
				bd = NULL;
			}
		}
		if(bd==NULL) {
			bd = wkz_intern_koord_to_weg_grund(sp,welt,pos,wegtype);
		}
		if(!bd  ||  bd->has_two_ways()) {
			// no monorail here ...
			create_win( new news_img("Cannot built depot here!"), w_time_delete, magic_none);
			return false;
		}

		const char *p=bd->kann_alle_obj_entfernen(sp);
		if(p) {
			create_win( new news_img(p), w_time_delete, magic_none);
			return false;
		}

		// avoid building over a stop
		if(bd->is_halt()  ||  bd->gib_depot()!=NULL) {
			create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
			return false;
		}

		ribi_t::ribi ribi;
		if(bd->ist_wasser()) {
			// assume one orientation with water
			ribi = ribi_t::sued;
		}
		else {
			ribi = bd->gib_weg_ribi_unmasked(wegtype);
		}

		if(ribi_t::ist_einfach(ribi)  &&  bd->gib_weg_hang()==0) {

			int layout = 0;
			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude( welt, sp, bd->gib_pos(), layout, besch );
			sp->buche(cost, pos, COST_CONSTRUCTION);
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
			return true;
		}
	}
	create_win( new news_img("Cannot built depot here!"), w_time_delete, magic_none);
	return false;
}



/* prissi: universal depot tool */
/* returns fals on success, so one can built something else
 * otherwise will return true to try again ... */
int wkz_depot(spieler_t *sp, karte_t *welt, koord pos,value_t w)
{
	if(sp==welt->gib_spieler(1)) {
		// no depots for player 1
		return false;
	}

	if(pos==INIT  ||  pos==EXIT) {
		return true;
	}

	const haus_besch_t *besch=(const haus_besch_t *)w.p;
	if(hausbauer_t::bahn_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, track_wt, umgebung_t::cst_depot_rail );
	}
	else if(hausbauer_t::monorail_depot_besch==besch) {
		// since it need also a foundations, this is slightly more complex ...
		if( wkz_depot_aux( sp, welt, pos, besch, monorail_wt, umgebung_t::cst_depot_rail ) ) {
			grund_t *bd = welt->lookup(pos)->gib_kartenboden();
			if(bd->ist_natur()) {
				hausbauer_t::baue( welt, sp, bd->gib_pos(), 0, hausbauer_t::monorail_foundation_besch );
			}
		}
		return true;
	}
	else if(hausbauer_t::tram_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, track_wt, umgebung_t::cst_depot_rail );
	}
	else if(hausbauer_t::str_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, road_wt, umgebung_t::cst_depot_road );
	}
	else if(hausbauer_t::sch_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, water_wt, umgebung_t::cst_depot_ship );
	}
	else if(hausbauer_t::air_depot.contains(besch)) {
		return wkz_depot_aux( sp, welt, pos, besch, air_wt, umgebung_t::cst_multiply_airterminal );
	}
	else {
		DBG_MESSAGE("wkz_depot()","called with unknown besch %s",besch->gib_name() );
		return false;
	}
}




/* the following three routines add points to a schedule
 *  by we do not like to stop at AIs stop, but we want still force the truck to use AI roads
 * So if there is a halt, then it must be either public or our!
 * @autor prissi
 */
static int
wkz_fahrplan_insert_aux(spieler_t *sp, karte_t *welt, koord pos, fahrplan_t *fpl, bool append)
{
	if(pos==INIT  &&  pos==EXIT) {
		return false;
	}

	if(fpl == NULL) {
dbg->warning("wkz_fahrplan_insert_aux()","Schedule is (null), doing nothing");
		return false;
	}

	// now we can start
	if(welt->ist_in_kartengrenzen(pos)) {
		bool wrong_owner = false;
		const planquadrat_t *pl = welt->lookup(pos);
		const bool backwards=event_get_last_control_shift()==2;
		const grund_t *bd=0;
		// search all grounds for match
		for(  unsigned cnt=0;  cnt<pl->gib_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->gib_boden_count()-1-cnt : cnt;
			bd = pl->gib_boden_bei(i);
			// ignore tunnel (can be set with Underground mode)
			if(bd->ist_im_tunnel()) {
				bd = 0;
				continue;
			}
			// now just for error messages, we assuming a valid ground
			// and check for ownership
			if(!bd->is_halt()  &&  bd->obj_count()!=0  &&  !sp->check_owner(bd->obj_bei(0)->gib_besitzer())) {
				bd = 0;
				continue;
			}
			if(bd->is_halt()  &&  !sp->check_owner(bd->gib_halt()->gib_besitzer()) ) {
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
			// no halt; ownership not checked here, so we checked before!
			if(append) {
				fpl->append(bd);
			}
			else {
				fpl->insert(bd);
			}
		}
		else {
			// here we failed
			if(wrong_owner) {
				create_win( new news_img("Das Feld gehoert\neinem anderen Spieler\n"), w_time_delete, magic_none);
			}
			else {
				fpl->zeige_fehlermeldung();
			}
			return false;
		}
	}
	return true;
}



int wkz_fahrplan_add(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	return wkz_fahrplan_insert_aux( sp, welt, pos, (fahrplan_t *)f.p, true );
}



int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	return wkz_fahrplan_insert_aux( sp, welt, pos, (fahrplan_t *)f.p, false );
}



int wkz_clear_reservation(spieler_t *sp, karte_t *welt, koord pos)
{
	if(welt->ist_in_kartengrenzen(pos)) {

		const planquadrat_t *plan = welt->lookup(pos);
		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			grund_t *gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );

			if(gr) {

				for(unsigned wnr=0;  wnr<2;  wnr++  ) {

					schiene_t *w = dynamic_cast<schiene_t *>(gr->gib_weg_nr(wnr));
					// is this a reserved track?
					if(w!=NULL  &&  w->is_reserved()) {
						/* now we do a very crude procedure:
						 * - we search all ways for reservations of this convoi and remove them
						 * - we set the convoi state to ROUTING_1; it must rereserve its ways then
						 */
						const waytype_t waytype = w->gib_waytype();
						const convoihandle_t cnv = w->get_reserved_convoi();
						if(cnv->get_state()==convoi_t::DRIVING) {
							// reset driving state
							cnv->suche_neue_route();
						}
						slist_iterator_tpl<weg_t *>iter(weg_t::gib_alle_wege());
						while(iter.next()) {
							if(iter.get_current()->gib_waytype()==waytype) {
								schiene_t *sch = dynamic_cast<schiene_t *>(iter.access_current());
								if(sch->get_reserved_convoi()==cnv  &&  !gr->suche_obj(cnv->gib_vehikel(0)->gib_typ())) {
									// force free
									sch->unreserve( cnv->gib_vehikel(0) );
								}
							}
						}
					}
				}
			}
		}
	}
	return true;
}



int wkz_marker(spieler_t *sp, karte_t *welt, koord pos)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		create_win( new label_frame_t(welt, sp, pos), w_info, magic_label_frame);
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

			ding_t *d = gr->first_obj();
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
				stadt_t* stadt = new stadt_t(welt->gib_spieler(1), pos, citizens / 10);

				welt->add_stadt(stadt);
				stadt->laden_abschliessen();
				stadt->verbinde_fabriken();

				sp->buche(umgebung_t::cst_found_city, pos, COST_CONSTRUCTION);
				ok =  true;
			}
		}
	}

	// update an open map
	reliefkarte_t::gib_karte()->calc_map();
	return ok;
}



/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
int wkz_set_slope(spieler_t * sp, karte_t *welt, koord pos, value_t lParam)
{
	const sint8 new_slope = (sint8)lParam.i;
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {

		grund_t * gr1 = welt->lookup(pos)->gib_kartenboden();

		// at least a pixel away from the border?
		if(welt->min_hgt(pos)<welt->gib_grundwasser()  ||  !welt->ist_in_kartengrenzen(pos+koord(1,1))  ||  !welt->ist_in_kartengrenzen(pos+koord(-1,-1))) {
			create_win( new news_img("Maximum tile height difference reached."), w_time_delete, magic_none);
			return false;
		}

#ifdef COVER_TILES
		// already covered ... ?
		if(gr1->get_flag(grund_t::is_cover_tile)) {
			if(new_slope==RESTORE_SLOPE  &&  gr1->kann_alle_obj_entfernen(sp)) {
				planquadrat_t *plan=welt->access(pos);
				plan->boden_entfernen( plan->gib_boden_bei(1) );
				sp->buche(umgebung_t::cst_alter_land, pos, COST_CONSTRUCTION);
				return true;
			}
			else {
				create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
				return false;
			}
		}

		// special slope: 0 => makes a cover tile if possible
		if(new_slope==0) {
			planquadrat_t *plan=welt->access(pos);
			if(gr1->gib_grund_hang()!=0  ||  welt->max_hgt(pos)<=gr1->gib_hoehe()) {
				create_win( new news_img("Cannot cover this tile."), w_time_delete, magic_none);
				return false;
			}
			plan->kartenboden_insert( new boden_t(welt,koord3d(pos,welt->max_hgt(pos)),0) );
			sp->buche(umgebung_t::cst_alter_land, pos, COST_CONSTRUCTION);
			return true;
		}
#endif

		// finally: empty
		if (gr1->find<gebaeude_t>() || gr1->hat_wege() || gr1->kann_alle_obj_entfernen(sp)) {
			create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
			return false;
		}

		// slopes may affect the position and the total height!
		koord3d new_pos;
		sint8 slope_this;

		if(new_slope == RESTORE_SLOPE) {
			// prissi: special action: set to natural slope
			new_pos=koord3d(pos,welt->min_hgt(pos));
			slope_this = welt->calc_natural_slope(pos);
			DBG_MESSAGE("natural_slope","%i",slope_this);
		}
		else {
			// now check offsets before changing the slope ...
			sint8 change_to_slope=new_slope;
			if(new_slope==ALL_DOWN_SLOPE  &&  gr1->gib_grund_hang()>0) {
				// is more intiutive: if there is a slope, first downgrade it
				change_to_slope = 0;
			}
			slope_this = (change_to_slope>=ALL_UP_SLOPE) ? 0 : change_to_slope;
			new_pos = gr1->gib_pos() + koord3d(0,0,(change_to_slope==ALL_UP_SLOPE?Z_TILE_STEP:(change_to_slope==ALL_DOWN_SLOPE?-Z_TILE_STEP:0)));
#ifdef DOUBLE_GROUNDS
			// if already the same, double the slope
			if(slope_this==gr1->gib_grund_hang()) {
				slope_this *= 2;
			}
#endif
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z/Z_TILE_STEP;

		// first left side
		const grund_t *grleft=welt->lookup(pos+koord(-1,0))->gib_kartenboden();
		if(grleft) {
			const sint16 left_hgt=grleft->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=grleft->gib_grund_hang();
			const sint8 diff_from_ground_1 = left_hgt+corner2(slope)-hgt;
			const sint8 diff_from_ground_2 = left_hgt+corner3(slope)-hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win( new news_img("Maximum tile height difference reached."), w_time_delete, magic_none);
				return false;
			}
		}

		// right side
		const grund_t *grright=welt->lookup(pos+koord(1,0))->gib_kartenboden();
		if(grright) {
			const sint16 right_hgt=grright->gib_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground_1 = hgt+corner2(slope_this)-right_hgt;
			const sint8 diff_from_ground_2 = hgt+corner3(slope_this)-right_hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win( new news_img("Maximum tile height difference reached."), w_time_delete, magic_none);
				return false;
			}
		}

		const grund_t *grback=welt->lookup(pos+koord(0,-1))->gib_kartenboden();
		if(grback) {
			const sint16 back_hgt=grback->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=grback->gib_grund_hang();
			const sint8 diff_from_ground_1 = back_hgt+corner1(slope)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+corner2(slope)-hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win( new news_img("Maximum tile height difference reached."), w_time_delete, magic_none);
				return false;
			}
		}

		const grund_t *grfront=welt->lookup(pos+koord(0,1))->gib_kartenboden();
		if(grfront) {
			const sint16 front_hgt=grfront->gib_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground_1 = hgt+corner1(slope_this)-front_hgt;
			const sint8 diff_from_ground_2 = hgt+corner2(slope_this)-front_hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win( new news_img("Maximum tile height difference reached."), w_time_delete, magic_none);
				return false;
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=gr1->gib_pos());
		ok |= slope_this!=gr1->gib_grund_hang();

		if(ok) {

			// already some ground here (tunnel, bridge, monorail?)
			if(new_pos!=gr1->gib_pos()  &&  welt->lookup(new_pos)!=NULL) {
				create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
				return false;
			}

			// ok, was sucess
			if(!gr1->ist_wasser()  &&  slope_this==0  &&  new_pos.z==welt->gib_grundwasser()) {
				// now water
				welt->access(pos)->kartenboden_setzen( new wasser_t(welt,new_pos) );
			}
			else if(gr1->ist_wasser()  &&  (new_pos.z>welt->gib_grundwasser()  ||  slope_this!=0)) {
				welt->access(pos)->kartenboden_setzen( new boden_t(welt,new_pos,slope_this) );
			}
			else {
				gr1->obj_loesche_alle(sp);
				gr1->setze_grund_hang(slope_this);
				gr1->setze_pos(new_pos);
				gr1->clear_flag(grund_t::marked);
				gr1->set_flag(grund_t::dirty);
			}
			// recalc slope walls on neightbours
			for(int y=-1; y<=1; y++) {
				for(int x=-1; x<=1; x++) {
					grund_t *gr = welt->lookup(pos+koord(x,y))->gib_kartenboden();
					gr->calc_bild();
				}
			}
			sp->buche(new_slope==RESTORE_SLOPE?umgebung_t::cst_alter_land:umgebung_t::cst_set_slope, pos, COST_CONSTRUCTION);
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
	DBG_MESSAGE("wkz_pflanze_baum()", "called on %d,%d", pos.x, pos.y);
	baum_t::plant_tree_on_coordinate(welt, pos, 10);
	return true;
}




/* builts a random industry chain, either in the next town */
int wkz_build_industries_land(spieler_t *sp, karte_t *welt, koord pos, value_t param)
{
	const planquadrat_t* p = welt->lookup(pos);
	if (p == NULL) return false;
	const grund_t* gr = p->gib_kartenboden();
	const build_fab_struct *bfs = (const build_fab_struct *)param.p;
	const fabrik_besch_t *fab = bfs ? bfs->besch : fabrikbauer_t::get_random_consumer( true, (climate_bits)(1<<welt->get_climate(gr->gib_hoehe())), welt->get_timeline_year_month() );
	if(fab==NULL) {
		return false;
	}

	int rotation = bfs ? bfs->rotation % fab->gib_haus()->gib_all_layouts() : simrand(fab->gib_haus()->gib_all_layouts()-1);
	koord size = fab->gib_haus()->gib_groesse(rotation);

	bool hat_platz = false;

	// process ignore climates switch
	climate_bits cl = (bfs!=NULL  &&  bfs->ignore_climates) ? ALL_CLIMATES : fab->gib_haus()->get_allowed_climate_bits();

	if(fab->gib_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( pos, fab->gib_haus()->gib_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  bfs->rotation==255) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_wasser( pos, fab->gib_haus()->gib_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->ist_platz_frei( pos, fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  bfs->rotation==255) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_platz_frei( pos, fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		koord3d k = gr->gib_pos();
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, rotation, &k, welt->gib_spieler(1));

		if(anzahl>0) {
			// least one factory has been built
			welt->change_world_position( k.gib_2d(), 0, 0 );
			sp->buche( anzahl*umgebung_t::cst_multiply_found_industry, pos, COST_CONSTRUCTION );

			if(bfs) {
				// eventually adjust production
				const gebaeude_t *gb=welt->lookup_kartenboden(k.gib_2d())->find<gebaeude_t>();
				gb->get_fabrik()->set_base_production( bfs->production>>(welt->ticks_bits_per_tag-18) );
			}

			// crossconnect all?
			if(umgebung_t::crossconnect_factories) {
				const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
		}

		return true;
	}

	return false;
}


/* builts a random industry chain, either in the next town */
int wkz_build_industries_city(spieler_t *sp, karte_t *welt, koord pos, value_t param)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {

		koord3d pos3d = plan->gib_kartenboden()->gib_pos();
		const build_fab_struct *bfs = (const build_fab_struct *)param.p;
		const fabrik_besch_t *fab = bfs ? bfs->besch : fabrikbauer_t::get_random_consumer( true, (climate_bits)(1<<welt->get_climate(pos3d.z)), welt->get_timeline_year_month() );
		if(fab==NULL) {
			return false;
		}
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, false, &pos3d, welt->gib_spieler(1));

		if(anzahl>0) {
			// least one factory has been built
			welt->change_world_position( pos3d.gib_2d(), 0, 0 );

			if(bfs) {
				// eventually adjust production
				const gebaeude_t *gb=welt->lookup_kartenboden(pos3d.gib_2d())->find<gebaeude_t>();
				gb->get_fabrik()->set_base_production( bfs->production>>(welt->ticks_bits_per_tag-18) );
			}

			// crossconnect all?
			if(umgebung_t::crossconnect_factories) {
				const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
			// ain't going to be cheap
			if(sp) {
				sp->buche( anzahl*umgebung_t::cst_multiply_found_industry, pos, COST_CONSTRUCTION );
			}
		}
	}
	return plan != 0;
}



/*
*	extend tool: build factory
*/
int wkz_build_fab(spieler_t *, karte_t *welt, koord pos, value_t param)
{
	if(welt->ist_in_kartengrenzen(pos)) {

		const build_fab_struct *bfs = (const build_fab_struct *)param.p;

		const planquadrat_t *plan = welt->lookup(pos);
		grund_t *gr=plan->gib_boden_bei(0);
		welt->lookup(pos)->gib_kartenboden()->gib_pos();

		const fabrik_besch_t* fab = bfs->besch;
		int rotation = bfs->rotation % fab->gib_haus()->gib_all_layouts();
		koord size = fab->gib_haus()->gib_groesse(rotation);
		bool hat_platz = false;

		// process ignore climates switch
		climate_bits cl = bfs->ignore_climates ? ALL_CLIMATES : fab->gib_haus()->get_allowed_climate_bits();

		if(fab->gib_platzierung()==fabrik_besch_t::Wasser) {
			// at sea
			hat_platz = welt->ist_wasser( pos, fab->gib_haus()->gib_groesse(rotation) );

			if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  bfs->rotation==255) {
				// try other rotation too ...
				rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
				hat_platz = welt->ist_wasser( pos, fab->gib_haus()->gib_groesse(rotation) );
			}
		}
		else {
			// and on solid ground
			hat_platz = welt->ist_platz_frei( pos, fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );

			if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  bfs->rotation==255) {
				// try other rotation too ...
				rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
				hat_platz = welt->ist_platz_frei( pos, fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );
			}
		}

		// Platz gefunden ...
		if(hat_platz) {
			// ok! build it
			koord3d k = gr->gib_pos();
			fabrikbauer_t::baue_fabrik(welt, NULL, fab, rotation, k, welt->gib_spieler(1));
			const gebaeude_t *gb=gr->find<gebaeude_t>();
			if(gb==NULL) {
				return false;
			}
			gb->get_fabrik()->set_base_production( bfs->production>>(welt->ticks_bits_per_tag-18) );

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
	}
	return false;
}



/* open the list of halt */
int wkz_list_halt_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win( new halt_list_frame_t(sp), w_info, magic_halt_list_t );
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of vehicle */
int wkz_list_vehicle_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win( new convoi_frame_t(sp), w_info, magic_convoi_t );
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of towns */
int wkz_list_town_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win( new citylist_frame_t(welt), w_info, magic_citylist_frame_t );
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}



/* open the list of goods */
int wkz_list_good_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(new goods_frame_t(welt), w_info, magic_goodslist);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}



/* open the list of factories */
int wkz_list_factory_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(new factorylist_frame_t(welt), w_info, magic_factorylist );
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of attraction */
int wkz_list_curiosity_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(new curiositylist_frame_t(welt), w_info, magic_curiositylist );
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* prissi: undo building */
int wkz_undo(spieler_t* sp)
{
	if(!sp->undo()) {
		create_win( new news_img("UNDO failed!"), w_time_delete, magic_none);
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
		koord previous = sp->get_headquarter_pos();
		const haus_besch_t* besch = NULL;

		for(  vector_tpl<const haus_besch_t *>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end();  iter != end;  ++iter  ) {
			if ((*iter)->gib_bauzeit() == sp->get_headquarter_level()) {
				besch = (*iter);
				break;
			}
		}
		if(besch==NULL) {
			// no further headquarter level
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN, NO_SOUND, NO_SOUND);
			return false;
		}

		koord size = besch->gib_groesse();
		int rotate = 0;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
			ok = true;
		}
		if(size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
			rotate = 1;
			ok = true;
		}

		if(ok) {
			// remove previous one
			if(previous!=koord::invalid) {
				wkz_remover(sp,welt,previous);
			}
			// then built is
			gebaeude_t *hq = hausbauer_t::baue(welt, sp, welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotate, besch, NULL);
			stadt_t *city = welt->suche_naechste_stadt( pos );
			if(city) {
				city->add_gebaeude_to_stadt( hq );
			}
			sp->add_headquarter( level+1, pos );
			sp->buche(umgebung_t::cst_multiply_headquarter*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION * size.x * size.y);
		}
		else {
			create_win( new news_img("Tile not empty."), w_time_delete, magic_none);
		}
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return ok;
}



/* switch to next player
 * @author prissi
 */
int wkz_switch_player(spieler_t *, karte_t *welt, koord pos)
{
	if(pos==INIT) {
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
		welt->switch_active_player( welt->get_active_player_nr()+1 );
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
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),welt->get_climate(welt->max_hgt(pos)));

		if(attraction==NULL) {
			return false;
		}

		koord size = attraction->gib_groesse();
		int rotation = 0;

		bool hat_platz = false;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, attraction->get_allowed_climate_bits())) {
			hat_platz = true;
		}

		if(!hat_platz &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, attraction->get_allowed_climate_bits())) {
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
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
		welt->gib_einstellungen()->setze_allow_player_change( false );
		destroy_all_win();
		welt->switch_active_player( 0 );
	}
	return false;
}



// setp one year forward
int wkz_step_year( spieler_t *, karte_t *welt, koord k)
{
	if(k == INIT) {
		welt->step_year();
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}
