/*
 * Werkzeuge für den Simutrans-Spieler
 * von Hj. Malthaner
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "simdebug.h"
#include "simworld.h"
#include "simplay.h"
#include "simsound.h"
#include "simevent.h"
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

#include "besch/haus_besch.h"
#include "besch/way_obj_besch.h"
#include "besch/skin_besch.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/simpeople.h"

#include "gui/label_frame.h"
#include "gui/werkzeug_waehler.h"
#include "gui/karte.h"	// to update map after construction of new industry

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

#include "dataobj/tabfile.h"
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

#include "sucher/bauplatz_sucher.h"

#include "tpl/vector_tpl.h"

#include "utils/simstring.h"

#include "simwerkz.h"

/****************************************** static helper functions **************************************/

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price(const char * tip, sint64 price)
{
	const int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	return werkzeug_t::toolstr;
}



/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(plan  &&  plan->gib_halt().is_bound()) {
		return plan->gib_halt();
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
		const planquadrat_t *plan = welt->lookup(pos.gib_2d()+next_try_dir[i]);
		if(plan) {
			halthandle_t halt = plan->gib_halt();
			if(halt.is_bound()  &&  (spieler_t::check_owner( sp, halt->gib_besitzer())  ||  welt->gib_spieler(1)==sp)) {
				return halt;
			}
		}
	}

	// now just search everything
	koord k=pos.gib_2d();
	for(k.x=pos.x-1; k.x<=pos.x+b; k.x++) {
		for(k.y=pos.y-1; k.y<=pos.y+h; k.y++) {
			const planquadrat_t *plan = welt->lookup(k);
			if(plan) {
				halthandle_t halt = plan->gib_halt();
				if(halt.is_bound()  &&  (spieler_t::check_owner( sp, halt->gib_besitzer())  ||  welt->gib_spieler(1)==sp)) {
					return halt;
				}
			}
		}
	}

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
}


// converts a 2d koord to a suitable ground pointer
static grund_t *wkz_intern_koord_to_weg_grund(spieler_t *sp, karte_t *welt, koord pos, waytype_t wt)
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
		if(sp!=NULL  &&  !spieler_t::check_owner( sp, gr->gib_weg(wt)->gib_besitzer())){
			gr = NULL;
			continue;
		}
		// ok, now we have a valid ground
		break;
	}
	return gr;
}



/****************************************** now the actual tools **************************************/

// werkzeuge
const char *wkz_abfrage_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(plan) {
		DBG_MESSAGE("wkz_abfrage()","checking map square %d,%d", pos.x, pos.y);

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
							return NULL;
						}
					}
				}

				if(gr->gib_depot()  &&  gr->gib_depot()->gib_besitzer()==sp) {
					gr->gib_depot()->zeige_info();
					return NULL;
				}

				gr->zeige_info();
			}
		}
	}
	return NULL;
}

/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool wkz_remover_t::wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg)
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
		if(gr->gib_top()>0  &&  spieler_t::check_owner( sp, gr->obj_bei(0)->gib_besitzer()) ) {
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
		if(w==NULL  ||  !spieler_t::check_owner( sp, w->gib_besitzer())) {
			w = gr->gib_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(!spieler_t::check_owner( sp, w->gib_besitzer())) {
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



const char *wkz_remover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
	const char *fail = NULL;
	if(!wkz_remover_intern(sp, welt, pos.gib_2d(), fail)) {
		return fail;
	}

	// must recalc neighbourhood for slopes etc.
	if(pos.x>1) {
		welt->lookup_kartenboden(pos.gib_2d()+koord::west)->calc_bild();
	}
	if(pos.y>1) {
		welt->lookup_kartenboden(pos.gib_2d()+koord::nord)->calc_bild();
	}

	if(pos.x<welt->gib_groesse_x()-1) {
		welt->lookup_kartenboden(pos.gib_2d()+koord::ost)->calc_bild();
	}
	if(pos.y<welt->gib_groesse_y()-1) {
		welt->lookup_kartenboden(pos.gib_2d()+koord::sued)->calc_bild();
	}

	return NULL;
}



const char *wkz_raise_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = welt->lookup_hgt(pos.gib_2d())+Z_TILE_STEP;
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		const char *result = work( welt, sp, pos );
		default_param = NULL;
		return result;
	}
	return NULL;
}



const char *wkz_raise_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+Z_TILE_STEP);
	bool ok = false;
	koord pos = k.gib_2d();

	if(welt->ist_in_gittergrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {
		const int hgt = welt->lookup_hgt(pos);

		if(hgt < 14*Z_TILE_STEP) {

			int n = 0;	// tiles changed
			if(default_param) {
				ok = true;
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be goind up or down!
				while(welt->lookup_hgt(pos)<height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = true;
			}
			else {
				if(  is_dragging  ) {
					is_dragging = false;
					return NULL;
				}
				n = welt->raise(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::accounting(sp, umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
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
			return !ok ? "Tile not empty." : NULL;
		}
		else {
			// no mountains heigher than 14 ...
			return "Maximum tile height difference reached.";
		}
	}
	return "Zu nah am Kartenrand";
}



const char *wkz_lower_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = welt->lookup_hgt(pos.gib_2d())-Z_TILE_STEP;
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		const char *result = work( welt, sp, pos );
		default_param = NULL;
		return result;
	}
	return NULL;
}



const char *wkz_lower_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-Z_TILE_STEP);
	bool ok = false;
	koord pos = k.gib_2d();

	if(welt->ist_in_gittergrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {

		const int hgt = welt->lookup_hgt(pos);

		if(hgt > welt->gib_grundwasser()) {

			int n = 0;	// tiles changed
			if(default_param) {
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be goind up or down!
				while(welt->lookup_hgt(pos)<height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = welt->lookup_hgt(pos);
			}
			else {
				if(  is_dragging  ) {
					is_dragging = false;
					return NULL;
				}
				n = welt->lower(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::accounting(sp, umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
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
			return !ok ? "Tile not empty." : NULL;
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}



/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
const char *wkz_setslope_t::wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord pos, int new_slope )
{
	bool ok = false;

	grund_t * gr1 = welt->lookup_kartenboden(pos);
	if(gr1) {
		// at least a pixel away from the border?
		if(welt->min_hgt(pos)<welt->gib_grundwasser()  ) {
			return "Maximum tile height difference reached.";
		}

		// finally: empty
		if (gr1->find<gebaeude_t>() || gr1->hat_wege() || gr1->kann_alle_obj_entfernen(sp)) {
			return "Tile not empty.";
		}

		if(  !welt->ist_in_kartengrenzen(pos+koord(1,1))  ||  !welt->ist_in_kartengrenzen(pos+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
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
				return "Maximum tile height difference reached.";
			}
		}

		// right side
		const grund_t *grright=welt->lookup(pos+koord(1,0))->gib_kartenboden();
		if(grright) {
			const sint16 right_hgt=grright->gib_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground_1 = hgt+corner2(slope_this)-right_hgt;
			const sint8 diff_from_ground_2 = hgt+corner3(slope_this)-right_hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				return "Maximum tile height difference reached.";
			}
		}

		const grund_t *grback=welt->lookup(pos+koord(0,-1))->gib_kartenboden();
		if(grback) {
			const sint16 back_hgt=grback->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=grback->gib_grund_hang();
			const sint8 diff_from_ground_1 = back_hgt+corner1(slope)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+corner2(slope)-hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				return "Maximum tile height difference reached.";
			}
		}

		const grund_t *grfront=welt->lookup(pos+koord(0,1))->gib_kartenboden();
		if(grfront) {
			const sint16 front_hgt=grfront->gib_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground_1 = hgt+corner1(slope_this)-front_hgt;
			const sint8 diff_from_ground_2 = hgt+corner2(slope_this)-front_hgt;
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				return "Maximum tile height difference reached.";
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=gr1->gib_pos());
		ok |= slope_this!=gr1->gib_grund_hang();

		if(ok) {

			// already some ground here (tunnel, bridge, monorail?)
			if(new_pos!=gr1->gib_pos()  &&  welt->lookup(new_pos)!=NULL) {
				return "Tile not empty.";
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
			spieler_t::accounting(sp, new_slope==RESTORE_SLOPE?umgebung_t::cst_alter_land:umgebung_t::cst_set_slope, pos, COST_CONSTRUCTION);
		}

	}
	return ok ? NULL : "";
}



// set marker
const char *wkz_marker_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(welt->ist_in_kartengrenzen(pos.gib_2d())) {
		create_win( new label_frame_t(welt, sp, pos.gib_2d()), w_info, magic_label_frame);
		return NULL;
	}
	return "";
}



// show/repair blocks
bool wkz_clear_reservation_t::init( karte_t *welt, spieler_t * )
{
	schiene_t::show_reservations = true;
	welt->setze_dirty();
	return true;
}

bool wkz_clear_reservation_t::exit( karte_t *welt, spieler_t * )
{
	schiene_t::show_reservations = false;
	welt->setze_dirty();
	return true;
}

const char *wkz_clear_reservation_t::work( karte_t *welt, spieler_t *, koord3d k )
{
	const planquadrat_t *plan = welt->lookup(k.gib_2d());
	if(plan) {
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
	return NULL;
}



// transformer for electricity supply
const char *wkz_transformer_t::get_tooltip( spieler_t *sp )
{
	sprintf(toolstr, "%s, %ld$ (%ld$)", translator::translate("Build drain"), (long)(umgebung_t::cst_transformer/-100l), (long)(umgebung_t::cst_maintain_transformer<<(sp->get_welt()->ticks_bits_per_tag-18))/-100l );
	return toolstr;
}

const char *wkz_transformer_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", k.x, k.y);
	grund_t *gr=welt->lookup_kartenboden(k.gib_2d());
	if(gr  &&  gr->gib_grund_hang()==0  &&  !gr->ist_wasser()  &&  !gr->hat_wege()  &&  gr->kann_alle_obj_entfernen(sp)==NULL) {

		fabrik_t *fab=leitung_t::suche_fab_4(k.gib_2d());
		if(fab==NULL) {
			return "Transformer only next to factory!";
		}
		// remove everything on that spot
		const char *fail = gr->kann_alle_obj_entfernen(sp);
		if(fail) {
			return fail;
		}
		gr->obj_loesche_alle(sp);
		// now decide from the string whether a source or drain is built
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
	return NULL;
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
const char *wkz_add_city_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.gib_2d());
	if(gr) {

		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->gib_grund_hang() == 0  &&
			hausbauer_t::gib_special(0,haus_besch_t::rathaus,0,0,welt->get_climate(gr->gib_hoehe()))!=NULL  ) {

			ding_t *d = gr->first_obj();
			gebaeude_t *gb = dynamic_cast<gebaeude_t *>(d);

			if(gb && gb->ist_rathaus()) {
				dbg->warning("wkz_add_city()", "Already a city here");
				return "Tile not empty!";
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int citizens=(int)(welt->gib_einstellungen()->gib_mittlere_einwohnerzahl()*0.9);
				//  stadt_t *stadt = new stadt_t(welt, welt->gib_spieler(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citicens
				stadt_t* stadt = new stadt_t(welt->gib_spieler(1), pos.gib_2d(), citizens / 10);

				welt->add_stadt(stadt);
				stadt->laden_abschliessen();
				stadt->verbinde_fabriken();

				spieler_t::accounting(sp, umgebung_t::cst_found_city, pos.gib_2d(), COST_CONSTRUCTION);
				reliefkarte_t::gib_karte()->calc_map();
				return NULL;
			}
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}



/* change city size
 * @author prissi
 */
bool wkz_change_city_size_t::init( karte_t *, spieler_t * )
{
	cursor = atoi(default_param)>0 ? werkzeug_t::general_tool[WKZ_RAISE_LAND]->cursor : werkzeug_t::general_tool[WKZ_LOWER_LAND]->cursor;
	return true;
}

const char *wkz_change_city_size_t::work( karte_t *welt, spieler_t *, koord3d pos )
{
	stadt_t *city = welt->suche_naechste_stadt(pos.gib_2d());
	if(city!=NULL) {
		city->change_size( atoi(default_param) );
		return NULL;
	}
	return "";
}



const char *wkz_plant_tree_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(welt->ist_in_kartengrenzen(pos.gib_2d())) {
		const baum_besch_t *besch = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL) {
			besch = baum_t::random_tree_for_climate( welt->get_climate(pos.z) );
		}
		else {
			// parse default_param: bbbesch_nr b=1 ignore climate b=1 randome age
			check_climates = default_param[0]=='0';
			random_age = default_param[1]=='1';
			besch = baum_t::find_tree(default_param+3);
		}
		if(besch  &&  baum_t::plant_tree_on_coordinate( welt, pos.gib_2d(), besch, check_climates, random_age )  ) {
			spieler_t::accounting( sp, umgebung_t::cst_remove_tree, pos.gib_2d(), COST_CONSTRUCTION );
			return NULL;
		}
		return "";
	}
	return NULL;
}



/* the following three routines add waypoints/halts to a schedule
 * because we do not like to stop at AIs stop, but we want still force the truck to use AI roads
 * So if there is a halt, then it must be either public or our!
 * @autor prissi
 */
static const char *wkz_fahrplan_insert_aux(karte_t *welt, spieler_t *sp, koord pos, fahrplan_t *fpl, bool append)
{
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
			if(!bd->is_halt()  &&  bd->obj_count()!=0  &&  !spieler_t::check_owner( sp, bd->obj_bei(0)->gib_besitzer())) {
				bd = 0;
				continue;
			}
			if(bd->is_halt()  &&  !spieler_t::check_owner( sp, bd->gib_halt()->gib_besitzer()) ) {
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
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			else {
				return fpl->fehlermeldung();
			}
		}
	}
	return NULL;
}

const char *wkz_fahrplan_add_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k.gib_2d(), (fahrplan_t *)default_param, true );
}

const char *wkz_fahrplan_ins_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k.gib_2d(), (fahrplan_t *)default_param, false );
}



/* way construction */
const weg_besch_t *wkz_wegebau_t::defaults[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const weg_besch_t * wkz_wegebau_t::get_besch()
{
	const weg_besch_t *besch = wegbauer_t::gib_besch(default_param,0);
	if(besch==NULL) {
		waytype_t wt = (waytype_t)atoi(default_param);
		besch = defaults[wt&63];
		if(besch==NULL) {
			if(wt<=air_wt) {
				weg_t *w = weg_t::alloc(wt);
				besch = w->gib_besch();
				delete w;
			}
			else {
				besch = wegbauer_t::leitung_besch;
			}
		}
	}
	assert(besch);
	defaults[besch->gib_wtyp()&63] = besch;
	return besch;
}

const char *wkz_wegebau_t::get_tooltip(spieler_t *sp)
{
	const weg_besch_t *besch = get_besch();
	sprintf(toolstr, "%s, %ld$ (%ld$), %dkm/h",
		translator::translate(besch->gib_name()),
		besch->gib_preis()/100l,
		(besch->gib_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100l,
		besch->gib_topspeed());
	return toolstr;
}

bool wkz_wegebau_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wegebau_bauer != NULL) {
		wkz_wegebau_bauer->mark_image_dirty( wkz_wegebau_bauer->gib_bild(), 0 );
		delete wkz_wegebau_bauer;
		wkz_wegebau_bauer = NULL;
	}
	// delete old route
	while(!marked.empty()) {
		zeiger_t *z = marked.remove_first();
		z->mark_image_dirty( z->gib_bild(), 0 );
		delete z;
	}
	// now get current besch
	besch = get_besch();
	if(besch  &&  besch->gib_cursor()->gib_bild_nr(0) != IMG_LEER) {
		cursor = besch->gib_cursor()->gib_bild_nr(0);
	}
	win_set_static_tooltip( NULL );
	return besch!=NULL;
}

const char *wkz_wegebau_t::move(karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	// on map?
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(plan == NULL) {
		return "";
	}

	// ignore start==pos
	if(start==pos  &&  buttonstate==0) {
		init(welt,sp);
		return "";
	}

	// recalc type of construction
	wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->gib_wtyp();
	if(besch->gib_wtyp()==track_wt  &&  besch->gib_styp()==7) {
		bautyp = wegbauer_t::schiene_tram;
	}
	// elevated track?
	if(besch->gib_styp()==1  &&  besch->gib_wtyp()!=air_wt) {
		bautyp = (wegbauer_t::bautyp_t)((int)bautyp|(int)wegbauer_t::elevated_flag);
	}

	win_set_static_tooltip( NULL );
	if(buttonstate==1) {
		// delete old route
		while(!marked.empty()) {
			zeiger_t *z = marked.remove_first();
			z->mark_image_dirty( z->gib_bild(), 0 );
			delete z;
		}
		// check for suitable ground
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
				if(sp!=NULL  &&  (gr->obj_count()==0  ||  !spieler_t::check_owner( sp, gr->obj_bei(0)->gib_besitzer()))){
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
		// calc new route (if there)
		if(gr) {
			if(start==koord3d::invalid) {
				welt->show_distance = start = gr->gib_pos();
				wkz_wegebau_bauer = new zeiger_t(welt, start, sp);
				wkz_wegebau_bauer->setze_bild( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0) );
				gr->obj_add(wkz_wegebau_bauer);
			}
			else {
				// calculate route
				wegbauer_t bauigel(welt, sp);
				koord3d ziel = gr->gib_pos();
				display_show_load_pointer(true);
				bauigel.route_fuer(bautyp, besch);
				if(event_get_last_control_shift()==2  ||  grund_t::underground_mode) {
					bauigel.set_keep_existing_ways(false);
					bauigel.calc_straight_route(start,ziel);
				}
				else {
					bauigel.set_keep_existing_faster_ways(true);
					bauigel.calc_route(start,ziel);
				}
				if(bauigel.max_n>0) {
					// make dummy route from bauigel
					for( int j=0;  j<=bauigel.max_n;  j++  ) {
						koord3d pos = bauigel.gib_route_bei(j);
						grund_t *gr = welt->lookup(pos);
						ribi_t::ribi zeige = gr->gib_weg_ribi_unmasked(besch->gib_wtyp());
						if(j>0) {
							zeige |= ribi_typ( bauigel.gib_route_bei(j-1).gib_2d()-pos.gib_2d() );
						}
						if(j<bauigel.max_n) {
							zeige |= ribi_typ( bauigel.gib_route_bei(j+1).gib_2d()-pos.gib_2d() );
						}
						zeiger_t *way = new zeiger_t( welt, pos, sp );
						if(gr->gib_weg_hang()) {
							way->setze_bild( besch->gib_hang_bild_nr(gr->gib_weg_hang(),0) );
						}
						else if(besch->gib_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  besch->has_diagonal_bild()) {
							way->setze_bild( besch->gib_diagonal_bild_nr(zeige,0) );
						}
						else {
							way->setze_bild( besch->gib_bild_nr(zeige,0) );
						}
						welt->lookup(pos)->obj_add( way );
						marked.insert( way );
						way->mark_image_dirty( way->gib_bild(), 0 );
					}
					win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );
				}
				display_show_load_pointer(false);
			}
		}
	}
	return NULL;
}

const char *wkz_wegebau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
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
			if(sp!=NULL  &&  (gr->obj_count()==0  ||  !spieler_t::check_owner( sp, gr->obj_bei(0)->gib_besitzer()))){
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
		return "";
	}

	if(start==koord3d::invalid) {
		welt->show_distance = start = gr->gib_pos();

		DBG_MESSAGE("wkz_wegebau()", "Setting start to %d,%d,%d",start.x, start.y, start.z);

		// symbol für strassenanfang setzen
		wkz_wegebau_bauer = new zeiger_t(welt, start, sp);
		wkz_wegebau_bauer->setze_bild( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0) );
		gr->obj_add(wkz_wegebau_bauer);
	}
	else {
		const koord3d baustart = start;
		wegbauer_t bauigel(welt, sp);
		koord3d ziel = gr->gib_pos();
		DBG_MESSAGE("wkz_wegebau()", "Setting end to %d,%d,%d",ziel.x, ziel.y, ziel.z);

		// remove old pointers
		init(welt,sp);
		display_show_load_pointer(true);

		// recalc type of construction
		wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->gib_wtyp();
		if(besch->gib_wtyp()==track_wt  &&  besch->gib_styp()==7) {
			bautyp = wegbauer_t::schiene_tram;
		}
		// elevated track?
		if(besch->gib_styp()==1  &&  besch->gib_wtyp()!=air_wt) {
			bautyp = (wegbauer_t::bautyp_t)((int)bautyp|(int)wegbauer_t::elevated_flag);
		}

		bauigel.route_fuer(bautyp, besch);
		if(event_get_last_control_shift()==2  ||  grund_t::underground_mode) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
			bauigel.set_keep_existing_ways(false);
			bauigel.calc_straight_route(baustart,ziel);
		}
		else {
			bauigel.set_keep_existing_faster_ways(true);
			bauigel.calc_route(baustart,ziel);
		}
		welt->show_distance = start = koord3d::invalid;

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
	return NULL;
}



/* bridge construction */
const char *wkz_brueckenbau_t::get_tooltip(spieler_t *sp)
{
	const bruecke_besch_t * besch = brueckenbauer_t::gib_besch(default_param);
	int n = sprintf(toolstr, "%s, %d$ (%d$)",
		  translator::translate(besch->gib_name()),
		  besch->gib_preis()/100,
		  (besch->gib_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100);

	if(besch->gib_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->gib_topspeed());
	}
	if(besch->gib_max_length()>0) {
		n += sprintf(toolstr+n, ", %dkm", besch->gib_max_length());
	}
	return toolstr;
}



/* just call the bruckenbauer */
const char *wkz_brueckenbau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	return brueckenbauer_t::baue( welt, sp, pos.gib_2d(), brueckenbauer_t::gib_besch(default_param) );
}



/* more difficult, since this builts also underground ways */
const char *wkz_tunnelbau_t::get_tooltip(spieler_t *sp)
{
	const tunnel_besch_t * besch = tunnelbauer_t::gib_besch(default_param);
	int n = sprintf(toolstr, "%s, %d$ (%d$)",
		  translator::translate(besch->gib_name()),
		  besch->gib_preis()/100,
		  (besch->gib_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100);

	if(besch->gib_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->gib_topspeed());
	}
	return toolstr;
}

bool wkz_tunnelbau_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;
	if(wkz_tunnelbau_bauer != NULL) {
		wkz_tunnelbau_bauer->mark_image_dirty( wkz_tunnelbau_bauer->gib_bild(), 0 );
		delete wkz_tunnelbau_bauer;
		wkz_tunnelbau_bauer = NULL;
	}
	return true;
}

const char *wkz_tunnelbau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
		return "";
	}

	if(  !grund_t::underground_mode  ) {
		init(welt,sp);
	}

	const tunnel_besch_t *besch = tunnelbauer_t::gib_besch(default_param);
	DBG_MESSAGE("tunnelbauer_t::baue()", "called on %d,%d", pos.x, pos.y);

	// in underground mode, new tunnel can be made ...
	if(grund_t::underground_mode) {
		// search for ground
		// start needs valid tile!
		if(start==koord3d::invalid) {
			const planquadrat_t *plan=welt->lookup(pos.gib_2d());
			grund_t *gr=NULL;
			for (uint i = 0; i < plan->gib_boden_count(); i++) {
				if(plan->gib_boden_bei(i)->gib_typ()==grund_t::tunnelboden) {
					if(spieler_t::check_owner( sp, plan->gib_boden_bei(i)->obj_bei(0)->gib_besitzer())) {
						gr = plan->gib_boden_bei(i);
						break;
					}
				}
			}
			if(gr==NULL) {
				return "No suitable ground!";
			}
			welt->show_distance = start = gr->gib_pos();
			// move bulldozer to start ...
			wkz_tunnelbau_bauer = new zeiger_t(welt, start, sp);
			wkz_tunnelbau_bauer->setze_bild( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0));
			gr->obj_add(wkz_tunnelbau_bauer);
			return NULL;
		}
		else {
			// we have a start, now just try to built it ...
			delete wkz_tunnelbau_bauer;
			wkz_tunnelbau_bauer = NULL;

			int bt = besch->gib_waytype()|wegbauer_t::tunnel_flag;
			weg_t *w=weg_t::alloc(besch->gib_waytype());
			const weg_besch_t *wb=w->gib_besch();
			delete w;
			// now try construction
			wegbauer_t bauigel(welt, sp);
			bauigel.route_fuer((wegbauer_t::bautyp_t)bt, wb, besch);
			bauigel.set_keep_existing_ways(false);
			bauigel.calc_straight_route(start,koord3d(pos.gib_2d(),start.z));
			welt->mute_sound(true);
			bauigel.baue();
			welt->mute_sound(false);
			welt->show_distance = start = koord3d::invalid;
			return NULL;
		}
	}
	else {
		return tunnelbauer_t::baue( welt, sp, pos.gib_2d(), besch );
	}
}



/* removes a way like a driving car ... */
const char *wkz_wayremover_t::get_tooltip(spieler_t *)
{
	switch(atoi(default_param)) {
		case road_wt: return translator::translate("remove roads");
		case tram_wt:
		case track_wt: return translator::translate("remove tracks");
		case maglev_wt: return translator::translate("remove maglev tracks");
		case narrowgauge_wt: return translator::translate("remove narrowgauge tracks");
		case monorail_wt: return translator::translate("remove monorails");
		case water_wt: return translator::translate("remove channels");
		case air_wt: return translator::translate("remove airstrips");
		case powerline_wt: return translator::translate("remove powerlines");
	}
	return NULL;
}

bool wkz_wayremover_t::init( karte_t *welt, spieler_t * )
{
	erster = true;
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wayremover_bauer != NULL) {
		wkz_wayremover_bauer->mark_image_dirty( wkz_wayremover_bauer->gib_bild(), 0 );
		delete wkz_wayremover_bauer;
		wkz_wayremover_bauer = NULL;
	}
	return true;
}

const char *wkz_wayremover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	// search for starting ground
	waytype_t wt = (waytype_t)atoi(default_param);
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos.gib_2d(),wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return "";
	}
	// do not remove ground from depot
	if(gr->gib_depot()) {
		return "No suitable ground!";
	}

	if( erster ) {
		// set start position
		erster = false;
		welt->show_distance = start = gr->gib_pos();
		wkz_wayremover_bauer = new zeiger_t(welt, start, sp);
		wkz_wayremover_bauer->setze_bild( cursor );
		gr->obj_add(wkz_wayremover_bauer);
DBG_MESSAGE("wkz_wayremover()", "Setting start to %d,%d,%d",start.x, start.y,start.z);
	}
	else {
DBG_MESSAGE("wkz_wayremover()", "Setting end to %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y,gr->gib_pos().z);

		// remove marker
		wkz_wayremover_bauer->mark_image_dirty( wkz_wayremover_bauer->gib_bild(), 0 );
		delete wkz_wayremover_bauer;
		wkz_wayremover_bauer = NULL;
		erster = true;

		// get a default vehikel
		vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
		vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
		route_t verbindung;
		bool can_delete = verbindung.calc_route(welt, start, gr->gib_pos(), test_driver, 0);
		delete test_driver;
		welt->show_distance = start = koord3d::invalid;
		DBG_MESSAGE("wkz_wayremover()", "route search returned %d", can_delete);

		if(!can_delete) {
			DBG_MESSAGE("wkz_wayremover()","no route found");
			return "Ways not connected";
		}

DBG_MESSAGE("wkz_wayremover()","route with %d tile found",verbindung.gib_max_n());


		// found a route => check if I can delete anything on it
		for( int i=0;  can_delete  &&  i<=verbindung.gib_max_n();  i++  ) {
			grund_t *gr=welt->lookup(verbindung.position_bei(i));
			if(gr) {
				if(gr->gib_weg(wt)==NULL  ||  !spieler_t::check_owner( sp, gr->gib_weg(wt)->gib_besitzer())) {
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
					if(gr->find<bruecke_t>()->gib_besch()->gib_waytype()==wt) {
						if(gr->ist_karten_boden()) {
							const char *err = NULL;
							err = brueckenbauer_t::remove(welt,sp,verbindung.position_bei(i),wt);
							if(err) {
								return err;
							}
						}
						// do not remove asphalt from a bridge ...
						continue;
					}
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
		return can_delete ? NULL : "";
	}
	return NULL;
}



/* add catenary during construction */
const way_obj_besch_t *wkz_wayobj_t::default_electric = NULL;

const char *wkz_wayobj_t::get_tooltip(spieler_t *sp)
{
	const way_obj_besch_t *besch = wayobj_t::find_besch(default_param);
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = default_electric = wayobj_t::wayobj_search(track_wt,overheadlines_wt,sp->get_welt()->get_timeline_year_month());
		}
	}
	if(besch) {
		sprintf(toolstr, "%s, %ld$ (%ld$), %dkm/h",
			translator::translate(besch->gib_name()),
			besch->gib_preis()/100l,
			(besch->gib_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18l))/100l,
			besch->gib_topspeed());
		return toolstr;
	}
	return NULL;
}

bool wkz_wayobj_t::init( karte_t *welt, spieler_t *sp )
{
	const way_obj_besch_t *besch = default_param ? wayobj_t::find_besch(default_param) : NULL;
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = default_electric = wayobj_t::wayobj_search(track_wt,overheadlines_wt,sp->get_welt()->get_timeline_year_month());
		}
	}
	else {
		default_electric = besch;
	}
	erster = true;
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wayobj_bauer != NULL) {
		wkz_wayobj_bauer->mark_image_dirty( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0), 0 );
		delete wkz_wayobj_bauer;
		wkz_wayobj_bauer = NULL;
	}
	return besch!=NULL;
}

const char *wkz_wayobj_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const way_obj_besch_t *besch = wayobj_t::find_besch(default_param);
	if(besch==NULL) {
		besch = default_electric;
	}
	waytype_t wt=besch->gib_wtyp();
	koord3d end;

	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos.gib_2d(),wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return "";
	}

	if( erster ) {
		// set start position
		erster = false;
		welt->show_distance = start = gr->gib_pos();
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
		vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
		vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
		route_t verbindung;
		bool can_built = verbindung.calc_route(welt, start, gr->gib_pos(), test_driver, 0);
		delete test_driver;
		welt->show_distance = start = koord3d::invalid;
		DBG_MESSAGE("wkz_wayremover()","route search returned %d",can_built);

		if(!can_built) {
			DBG_MESSAGE("wkz_wayremover()","no route found");
			return "Ways not connected";
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
		return NULL;
	}
	return "";
}


/* build all kind of station extension buildings */
const char *wkz_station_t::wkz_station_building_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch)
{
	koord pos = k.gib_2d();
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);
	static koord rotate_koords[4]={koord(0,-1),koord(1,0),koord(0,1),koord(-1,0)};

	koord size = besch->gib_groesse();
	int rotate=-1, built_rotate=-1;
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
		// nothing found ...
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
/*
		if((besch->get_enabled()&2)!=0  &&  halt->get_post_enabled()) {
			create_win( new news_img("Station already\nhas a post office!\n"), w_time_delete, magic_none);
		}
*/
		hausbauer_t::baue(welt, halt->gib_besitzer(), k, rotate, besch, &halt);
		sp->buche(umgebung_t::cst_multiply_post*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
		halt->recalc_station_type();
	}
	else {
		return "Post muss neben\nHaltestelle\nliegen!\n";
	}
	return NULL;
}

/* build a dock either small or large */
const char *wkz_station_t::wkz_station_dock_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch)
{
	// the cursor cannot be outside the map from here on
	koord pos = k.gib_2d();
	grund_t *gr = welt->lookup_kartenboden(pos);
	hang_t::typ hang = gr->gib_grund_hang();
	// first get the size
	int len = besch->gib_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;

	// check, if we can built here ...
	if(!hang_t::ist_einfach(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->ist_in_kartengrenzen(pos-dx*i)) {
				// need at least a signle tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			else {
				halthandle_t halt = welt->lookup(pos-dx*i)->gib_halt();
				if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->gib_besitzer())) {
					return "Das Feld gehoert\neinem anderen Spieler\n";
				}
				else {
					const grund_t *gr=welt->lookup(pos-dx*i)->gib_kartenboden();
					const char *msg = gr->kann_alle_obj_entfernen(sp);
					if(msg) {
						return msg;
					}
					else if(i==0  &&  (gr->ist_wasser()  ||  gr->hat_wege()  ||  gr->gib_typ()!=grund_t::boden)  ||  gr->kann_alle_obj_entfernen(sp)!=NULL  ||  gr->is_halt()) {
						return "Tile not empty.";
					}
					else if (i!=0  &&  (!gr->ist_wasser() || gr->find<gebaeude_t>() || gr->gib_depot() || gr->is_halt())) {
						return "Tile not empty.";
					}
				}
			}
		}
	}

	// remove everything from tile
	gr->obj_loesche_alle(sp);

DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);
	int layout = 0;
	koord3d bau_pos = welt->lookup_kartenboden(pos)->gib_pos();
	koord dx2;
	switch(hang) {
		case hang_t::sued:
			layout = 0;
			dx2 = koord::west;
			break;
		case hang_t::ost:
			layout = 1;
			dx2 = koord::nord;
			break;
		case hang_t::nord:
			layout = 2;
			dx2 = koord::west;
			bau_pos = welt->lookup_kartenboden(last_pos)->gib_pos();
			break;
		case hang_t::west:
			layout = 3;
			dx2 = koord::nord;
			bau_pos = welt->lookup_kartenboden(last_pos)->gib_pos();
			break;
	}

	// handle 16 layouts
	bool change_layout = false;
	if(besch->gib_all_layouts()==16) {
		if(  layout<2  ) {
			layout = 15-layout;
		}
		else {
			layout = 9-layout;
		}
		change_layout = true;
	}

	// oriented buildings here - get neighbouring layouts
	gr = welt->lookup_kartenboden(pos+dx2);

	// find out if middle end or start tile
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->gib_halt()->gib_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->gib_tile()->gib_besch()->gib_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->gib_tile()->gib_besch()->gib_all_layouts()==16) {
				sint8 ly = gb->gib_tile()->gib_layout();
				if(ly&0x02) {
					gb->setze_tile( gb->gib_tile()->gib_besch()->gib_tile(ly&0xFD,0,0) );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(pos-dx2);
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->gib_halt()->gib_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->gib_tile()->gib_besch()->gib_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->gib_tile()->gib_besch()->gib_all_layouts()==16) {
				sint8 ly = gb->gib_tile()->gib_layout();
				if(ly&0x04) {
					gb->setze_tile( gb->gib_tile()->gib_besch()->gib_tile(ly&0xFB,0,0) );
				}
			}
		}
	}

//DBG_MESSAGE("wkz_dockbau()","search for stop");
	halthandle_t halt = suche_nahe_haltestelle(sp, welt, welt->lookup_kartenboden(pos)->gib_pos() );
	bool neu = !halt.is_bound();

	if(neu) { // neues dock
		halt = sp->halt_add(pos);
	}
	hausbauer_t::baue(welt, halt->gib_besitzer(), bau_pos, layout, besch, &halt);
	for(int i=0;  i<=len;  i++ ) {
		koord p=pos-dx*i;
		sp->buche(umgebung_t::cst_multiply_dock*besch->gib_level(), p, COST_CONSTRUCTION);
	}

	halt->recalc_station_type();
	if(umgebung_t::station_coverage_show  &&  welt->gib_zeiger()->gib_pos().gib_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* name = halt->create_name(pos, "Dock");
		halt->setze_name( name );
		free(name);
	}
	return NULL;
}

// built all types of stops but sea harbours
const char *wkz_station_t::wkz_station_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name )
{
	koord pos = k.gib_2d();
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->gib_name(), pos.x, pos.y, wegtype);
	const char *p_error=(besch->gib_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	grund_t *bd = wkz_intern_koord_to_weg_grund( sp==welt->gib_spieler(1)?NULL:sp,welt,k.gib_2d(),wegtype);
	if(!bd  &&  track_wt) {
		bd = wkz_intern_koord_to_weg_grund(sp==welt->gib_spieler(1)?NULL:sp,welt,k.gib_2d(),monorail_wt);
	}

	if(!bd  ||  bd->gib_weg_hang()!=hang_t::flach  ||  bd->is_halt()) {
		// only flat tiles, only one stop per map square
		return p_error;
	}

	if(bd->gib_typ()==grund_t::tunnelboden  &&  bd->ist_karten_boden()) {
		// do not build on tunnel entries
		return false;
	}

	if(bd->gib_depot()) {
		return "No suitable ground!";
	}

	if(bd->hat_weg(air_wt)  &&  bd->gib_weg(air_wt)->gib_besch()->gib_styp()!=0) {
		return "No suitable ground!";
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
			return p_error;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(besch->gib_all_layouts()==4) {
		// terminal station
		ribi = bd->gib_weg_ribi_unmasked(wegtype);
		// sorry cannot built here ... (not a terminal tile)
		if(!ribi_t::ist_einfach(ribi)) {
			return p_error;
		}

		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	} else {
		// something wrong with station number of layouts
		dbg->fatal( "wkz_station_t::wkz_station_aux", "%s has wrong number of layouts (must be 2,4,8,16!)", besch->gib_name() );
		return p_error;
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
					if(gb  &&  gb->gib_tile()->gib_besch()->gib_all_layouts()>4  &&  gb->gib_tile()->gib_besch()->gib_utyp()>haus_besch_t::hafen) {
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
	}
	else {
		if(sp==welt->gib_spieler(1)) {
			halt->transfer_to_public_owner();
		}
	}
	hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* name = halt->create_name(pos, type_name);
		halt->setze_name( name );
		free(name);
	}
	sp->buche(cost*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
	if(umgebung_t::station_coverage_show  &&  welt->gib_zeiger()->gib_pos().gib_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}
	return NULL;
}

bool wkz_station_t::init( karte_t *welt, spieler_t * )
{
	cursor = hausbauer_t::find_tile(default_param,0)->gib_besch()->gib_cursor()->gib_bild_nr(0);
	welt->gib_zeiger()->setze_area( koord( welt->gib_einstellungen()->gib_station_coverage()*2+1, welt->gib_einstellungen()->gib_station_coverage()*2+1 ), true );
	return true;
}


image_id wkz_station_t::get_icon( spieler_t * )
{
	if(  grund_t::underground_mode  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
		const haus_besch_t *besch=hausbauer_t::find_tile(default_param,0)->gib_besch();
		if(  besch->gib_utyp()!=haus_besch_t::generic_stop  ||  besch->gib_extra()==air_wt) {
			return IMG_LEER;
		}
	}
	return icon;
}



const char *wkz_station_t::get_tooltip(spieler_t *)
{
	const haus_besch_t *besch=hausbauer_t::find_tile(default_param,0)->gib_besch();
	if(  besch->gib_utyp()==haus_besch_t::generic_stop  ||  besch->gib_utyp()==haus_besch_t::generic_extension  ) {
		switch (besch->gib_extra()) {
			case track_wt:
			case monorail_wt:
			case maglev_wt:
			case tram_wt:
			case narrowgauge_wt:
				return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_station*besch->gib_level() );
			case road_wt:
				return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_roadstop*besch->gib_level() );
			case water_wt:
				return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_dock*besch->gib_level()*besch->gib_groesse().x*besch->gib_groesse().y );
			case air_wt:
				return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_dock*besch->gib_level()*besch->gib_groesse().x*besch->gib_groesse().y );
			case 0:
				return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_post*besch->gib_level()*besch->gib_groesse().x*besch->gib_groesse().y );
		}
	}
	else if(  besch->gib_utyp()==haus_besch_t::hafen  ) {
		return tooltip_with_price( besch->gib_name(), umgebung_t::cst_multiply_dock*besch->gib_level()*besch->gib_groesse().x*besch->gib_groesse().y );
	}
	return "Illegal description";
}

const char *wkz_station_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(!plan) {
		return "";
	}

	// ownership allowed?
	halthandle_t halt = plan->gib_halt();
	if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->gib_besitzer())) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	const haus_besch_t *besch=hausbauer_t::find_tile(default_param,0)->gib_besch();
	const char *msg = NULL;
	switch (besch->gib_utyp()) {
		case haus_besch_t::hafen:
			msg = wkz_station_t::wkz_station_dock_aux(welt, sp, pos, besch );
			break;
		case haus_besch_t::hafen_geb:
		case haus_besch_t::generic_extension:
			msg = wkz_station_t::wkz_station_building_aux(welt, sp, pos, besch );
			break;
		case haus_besch_t::generic_stop:
			switch(besch->gib_extra()) {
				case road_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, road_wt, umgebung_t::cst_multiply_roadstop, "H");
					break;
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case narrowgauge_wt:
				case tram_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, (waytype_t)besch->gib_extra(), umgebung_t::cst_multiply_station, "BF");
					break;
				case water_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, water_wt, umgebung_t::cst_multiply_dock, "Dock");
					break;
				case air_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, air_wt, umgebung_t::cst_multiply_station, "Airport");
					break;
			}

			break;
		default:
			dbg->fatal("wkz_station_t::work()","tool called for illegal besch \"%\"", default_param );
	}

	if(msg==NULL) {
		// no error? => recalc all station connections
		welt->set_schedule_counter();
	}
	return msg;
}


// builds roadsings and signals
const char *wkz_roadsign_t::get_tooltip(spieler_t *)
{
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch) {
		return tooltip_with_price( besch->gib_name(), besch->gib_preis() );
	}
	return NULL;
}

const char *wkz_roadsign_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", k.x, k.y);
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch==NULL) {
		dbg->fatal("wkz_roadsign_t::work()","No roadsign \"%s\"", default_param );
	}
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,k.gib_2d(),besch->gib_wtyp());
	if(gr) {
		// get the sign dirction
		weg_t *weg = gr->gib_weg(besch->gib_wtyp());
		signal_t *s = gr->find<signal_t>();
		if(s  &&  s->gib_besch()!=besch) {
			// only one sign per tile
			return error;
		}
		if(besch->is_signal()  &&  gr->find<roadsign_t>())  {
			// only one sign per tile
			return error;
		}
		ribi_t::ribi dir = weg->gib_ribi_unmasked();

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->gib_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
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
					if(  !spieler_t::check_owner( rs->gib_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
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
					spieler_t::accounting(sp, -besch->gib_preis(), k.gib_2d(), COST_CONSTRUCTION);
				}
			}
			error = NULL;
		}
	}
	return error;
}



// built all types of depots
const char *wkz_depot_t::wkz_depot_aux(karte_t *welt, spieler_t *sp, koord pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos);
			if(!bd->ist_wasser()) {
				bd = NULL;
			}
		}
		if(bd==NULL) {
			bd = wkz_intern_koord_to_weg_grund(sp,welt,pos,wegtype);
		}
		if(!bd  ||  bd->has_two_ways()) {
			return "Cannot built depot here!";
		}

		// no depots on runways!
		if(besch->gib_extra()==air_wt  &&  bd->gib_weg(air_wt)->gib_besch()->gib_styp()!=0) {
			return "Cannot built depot here!";
		}

		const char *p=bd->kann_alle_obj_entfernen(sp);
		if(p) {
			return p;
		}

		// avoid building over a stop
		if(bd->is_halt()  ||  bd->gib_depot()!=NULL) {
			return "Cannot built depot here!";
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
			if(sp == welt->get_active_player()) {
				welt->set_werkzeug( general_tool[WKZ_ABFRAGE] );
			}
			return NULL;
		}
		return "Cannot built depot here!";
	}
	return "";
}

bool wkz_depot_t::init( karte_t *welt, spieler_t *sp )
{
	// no depots for player 1
	if(sp!=welt->gib_spieler(1)) {
		cursor = hausbauer_t::find_tile(default_param,0)->gib_besch()->gib_cursor()->gib_bild_nr(0);
		return true;
	}
	return false;
}

const char *wkz_depot_t::get_tooltip(spieler_t *)
{
	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->gib_besch();
	switch(besch->gib_extra()) {
		case road_wt: return tooltip_with_price( "Build road depot", umgebung_t::cst_depot_road );
		case track_wt: return tooltip_with_price( "Build train depot", umgebung_t::cst_depot_rail );
		case monorail_wt: return tooltip_with_price( "Build monorail depot", umgebung_t::cst_depot_rail );
		case maglev_wt: return tooltip_with_price( "Build maglev depot", umgebung_t::cst_depot_rail );
		case narrowgauge_wt: return tooltip_with_price( "Build narrowgauge depot", umgebung_t::cst_depot_rail );
		case tram_wt: return tooltip_with_price( "Build tram depot", umgebung_t::cst_depot_rail );
		case water_wt: return tooltip_with_price( "Build ship depot", umgebung_t::cst_depot_ship );
		case air_wt: return tooltip_with_price( "Build air depot", umgebung_t::cst_depot_air );
	}
	return NULL;
}

const char *wkz_depot_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if(sp==welt->gib_spieler(1)) {
		// no depots for player 1
		return false;
	}

	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->gib_besch();
	switch(besch->gib_extra()) {
		case road_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, road_wt, umgebung_t::cst_depot_road );
		case track_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, track_wt, umgebung_t::cst_depot_rail );
		case monorail_wt:
			{
				// since it need also a foundations, this is slightly more complex ...
				const char *err = wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, monorail_wt, umgebung_t::cst_depot_rail );
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(k.gib_2d());
					if(bd->ist_natur()) {
						hausbauer_t::baue( welt, sp, bd->gib_pos(), 0, hausbauer_t::elevated_foundation_besch );
					}
				}
				return err;
			}
		case tram_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, track_wt, umgebung_t::cst_depot_rail );
		case water_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, water_wt, umgebung_t::cst_depot_ship );
		case air_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, air_wt, umgebung_t::cst_depot_air );
		case maglev_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, maglev_wt, umgebung_t::cst_depot_rail );
		case narrowgauge_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.gib_2d(), besch, narrowgauge_wt, umgebung_t::cst_depot_rail );
		default:
			dbg->fatal("wkz_depot()","called with unknown besch %s",besch->gib_name() );
			return "";
	}
	return NULL;
}



/* builds (random) tourist attraction and maybe adds it to the next city
 * the parameter string is a follow:
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
bool wkz_build_haus_t::init( karte_t *welt, spieler_t * )
{
	if(default_param) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile!=NULL) {
			int rotation = (default_param[1]-'0') % tile->gib_besch()->gib_all_layouts();
			welt->gib_zeiger()->setze_area( tile->gib_besch()->gib_groesse(rotation), false );
		}
	}
	return true;
}

const char *wkz_build_haus_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.gib_2d());
	if(gr==NULL) {
		return "";
	}

	// Parsing parameter (if there)
	const haus_besch_t *besch = NULL;
	if(default_param) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile) {
			besch = tile->gib_besch();
		}
	}
	else {
		besch = hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),false,welt->get_climate(pos.z));
	}

	if(besch==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % besch->gib_all_layouts() : simrand(besch->gib_all_layouts());
	koord size = besch->gib_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : besch->get_allowed_climate_bits();

	bool hat_platz = welt->ist_platz_frei( pos.gib_2d(), besch->gib_b(rotation), besch->gib_h(rotation), NULL, cl );
	if(!hat_platz  &&  size.y!=size.x  &&  besch->gib_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
		// try other rotation too ...
		rotation = (rotation+1) % besch->gib_all_layouts();
		hat_platz = welt->ist_platz_frei( pos.gib_2d(), besch->gib_b(rotation), besch->gib_h(rotation), NULL, cl );
	}

	// Platz gefunden ...
	if(hat_platz) {
		spieler_t *gb_sp = besch->gib_typ()!=gebaeude_t::unbekannt ? NULL : welt->gib_spieler(1);
		gebaeude_t *gb = hausbauer_t::baue(welt, gb_sp, gr->gib_pos(), rotation, besch);
		if(gb) {
			// building successfull
			if(  besch->gib_utyp()!=haus_besch_t::attraction_land  ) {
				stadt_t *city = welt->suche_naechste_stadt( pos.gib_2d() );
				if(city) {
					city->add_gebaeude_to_stadt(gb);
				}
			}
			spieler_t::accounting(sp, umgebung_t::cst_multiply_remove_haus * besch->gib_level() * size.x * size.y, pos.gib_2d(), COST_CONSTRUCTION);
			return NULL;
		}
	}
	return "No suitable ground!";
}



// show industry size in cursor (in known)
bool wkz_build_industries_land_t::init( karte_t *welt, spieler_t * )
{
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		const fabrik_besch_t *fab = fabrikbauer_t::gib_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts();
		welt->gib_zeiger()->setze_area( fab->gib_haus()->gib_groesse(rotation), false );
	}
	return true;
}

/* builts a (if param=NULL random) industry chain starting here *
 * the parameter string is a follow:
 * 1#34,oelfeld
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * next number is production value
 * finally industry name
 */
const char *wkz_build_industries_land_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.gib_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		fab = fabrikbauer_t::gib_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->gib_hoehe())), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts() : simrand(fab->gib_haus()->gib_all_layouts()-1);
	koord size = fab->gib_haus()->gib_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->gib_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->gib_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( k.gib_2d(), fab->gib_haus()->gib_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_wasser( k.gib_2d(), fab->gib_haus()->gib_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->ist_platz_frei( k.gib_2d(), fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_platz_frei( k.gib_2d(), fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		koord3d k = gr->gib_pos();
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, rotation, &k, welt->gib_spieler(1), 10000 );

		if(anzahl>0) {
			// least one factory has been built
			welt->change_world_position( k.gib_2d(), 0, 0 );
			spieler_t::accounting(sp, anzahl*umgebung_t::cst_multiply_found_industry, k.gib_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param) {
				fabrik_t::gib_fab(welt,k.gib_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
			}

			// crossconnect all?
			if(umgebung_t::crossconnect_factories) {
				const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}


// show industry size in cursor (in known)
bool wkz_build_industries_city_t::init( karte_t *welt, spieler_t * )
{
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		const fabrik_besch_t *fab = fabrikbauer_t::gib_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts();
		welt->gib_zeiger()->setze_area( fab->gib_haus()->gib_groesse(rotation), false );
	}
	return true;
}

/* builts a industry chain in the next town
 * defaukt_param see previous function
 */
const char *wkz_build_industries_city_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.gib_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		fab = fabrikbauer_t::gib_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->gib_hoehe())), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts() : simrand(fab->gib_haus()->gib_all_layouts()-1);
	koord size = fab->gib_haus()->gib_groesse(rotation);

// process ignore climates switch (not possible for chains!)
//	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->gib_haus()->get_allowed_climate_bits();

	k = gr->gib_pos();
	int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, false, &k, welt->gib_spieler(1), 10000 );
	if(anzahl>0) {
		// least one factory has been built
		welt->change_world_position( k.gib_2d(), 0, 0 );

		// eventually adjust production
		if(default_param) {
			fabrik_t::gib_fab(welt,k.gib_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
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
		spieler_t::accounting(sp, anzahl*umgebung_t::cst_multiply_found_industry, k.gib_2d(), COST_CONSTRUCTION );
		return NULL;
	}
	return "No suitable ground!";
}



// show industry size in cursor (must be known!)
bool wkz_build_factory_t::init( karte_t *welt, spieler_t * )
{
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		const fabrik_besch_t *fab = fabrikbauer_t::gib_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts();
		welt->gib_zeiger()->setze_area( fab->gib_haus()->gib_groesse(rotation), false );
		return true;
	}
	return false;
}

/* builts an industry next to the cursor (default_param see above) */
const char *wkz_build_factory_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.gib_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',');
		fab = fabrikbauer_t::gib_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->gib_hoehe())), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->gib_haus()->gib_all_layouts() : simrand(fab->gib_haus()->gib_all_layouts());
	koord size = fab->gib_haus()->gib_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->gib_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->gib_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( k.gib_2d(), fab->gib_haus()->gib_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_wasser( k.gib_2d(), fab->gib_haus()->gib_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->ist_platz_frei( k.gib_2d(), fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->gib_haus()->gib_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->gib_haus()->gib_all_layouts();
			hat_platz = welt->ist_platz_frei( k.gib_2d(), fab->gib_haus()->gib_b(rotation), fab->gib_haus()->gib_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		fabrik_t *f = fabrikbauer_t::baue_fabrik(welt, NULL, fab, rotation, gr->gib_pos(), welt->gib_spieler(1));
		if(f) {
			// least one factory has been built
			welt->change_world_position( k.gib_2d(), 0, 0 );
			spieler_t::accounting(sp, umgebung_t::cst_multiply_found_industry, k.gib_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param) {
				f->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
			}

			// crossconnect all?
			if(umgebung_t::crossconnect_factories) {
				const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}



/**	link tool: links products of factory one with factory two (if possible)
 */
bool wkz_link_factory_t::init( karte_t *, spieler_t * )
{
	last_fab = NULL;
	if(wkz_linkzeiger!=NULL) {
		wkz_linkzeiger->mark_image_dirty( cursor, 0 );
		delete wkz_linkzeiger;
		wkz_linkzeiger = NULL;
	}
	return true;
}

const char *wkz_link_factory_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	fabrik_t *fab = fabrik_t::gib_fab( welt, pos.gib_2d() );
	if(fab!=NULL  &&  last_fab!=fab) {
		// It's a factory
		if(last_fab==NULL) {
			// first click
			grund_t *gr = welt->lookup_kartenboden(pos.gib_2d());
			wkz_linkzeiger = new zeiger_t(welt, gr->gib_pos(), sp);
			wkz_linkzeiger->setze_bild( cursor );
			gr->obj_add(wkz_linkzeiger);
			last_fab = fab;
			return NULL;
		}
		else {
			// second click
			if(fab->add_supplier(last_fab) || last_fab->add_supplier(fab)) {
				//ok! they are connected => remove marker
				init( welt, sp );
				return NULL;
			}
		}
	}
	return "";
}




/* builds company headquarter
 * @author prissi
 */
const haus_besch_t *wkz_headquarter_t::next_level( spieler_t *sp )
{
	// assume no further headquarter level
	const sint16 level = sp->get_headquarter_level();
	for (vector_tpl<const haus_besch_t*>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end(); iter != end; ++iter) {
		if ((*iter)->gib_extra() == level) {
			return *iter;
		}
	}
	return NULL;
}

const char *wkz_headquarter_t::get_tooltip( spieler_t *sp )
{
	const haus_besch_t* besch = next_level(sp);
	if(besch) {
		return tooltip_with_price( sp->get_headquarter_level()==0 ? "build HQ" : "upgrade HQ", umgebung_t::cst_multiply_headquarter*besch->gib_level()*besch->gib_b()*besch->gib_h() );
	}
	return NULL;
}

bool wkz_headquarter_t::init( karte_t *, spieler_t *sp )
{
	// do no use this, if there is no next level to built ...
	return next_level(sp)!=NULL;
}


const char *wkz_headquarter_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	bool ok=false;
DBG_MESSAGE("wkz_headquarter()", "building headquarter at (%d,%d)", pos.x, pos.y);

	const haus_besch_t* besch = next_level(sp);
	if(besch==NULL) {
		// no further headquarter level
		dbg->message( "wkz_headquarter()", "Already at maximum level!" );
		return "";
	}

	if(welt->ist_in_kartengrenzen(pos.gib_2d())) {

		// remove previous one
		koord previous = sp->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr->find<gebaeude_t>();
			sp->add_headquarter( prev_hq->gib_tile()->gib_besch()->gib_extra(), koord::invalid );
			hausbauer_t::remove( welt, sp, prev_hq );
		}

		koord size = besch->gib_groesse();
		int rotate = 0;

		if(welt->ist_platz_frei(pos.gib_2d(), size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
			ok = true;
		}
		if(!ok  &&  besch->gib_all_layouts()>1  &&  size.y != size.x  &&  welt->ist_platz_frei(pos.gib_2d(), size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
			rotate = 1;
			ok = true;
		}

		if(ok) {
			// then built is
			gebaeude_t *hq = hausbauer_t::baue(welt, sp, welt->lookup_kartenboden(pos.gib_2d())->gib_pos(), rotate, besch, NULL);
			stadt_t *city = welt->suche_naechste_stadt( pos.gib_2d() );
			if(city) {
				city->add_gebaeude_to_stadt( hq );
			}
			sp->add_headquarter( besch->gib_extra()+1, pos.gib_2d() );
			sp->buche(umgebung_t::cst_multiply_headquarter * besch->gib_level() * size.x * size.y, pos.gib_2d(), COST_CONSTRUCTION);
			if(sp == welt->get_active_player()) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
			}
			return NULL;
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}

const char *wkz_add_citycar_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if( stadtauto_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = wkz_intern_koord_to_weg_grund( sp, welt, k.gib_2d(), road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->gib_weg_ribi_unmasked(road_wt))  &&  gr->find<stadtauto_t>() == NULL) {
		// add citycar
		stadtauto_t* vt = new stadtauto_t(welt, gr->gib_pos(), koord::invalid);
		gr->obj_add(vt);
		welt->sync_add(vt);
		return NULL;
	}
	return "";
}


bool wkz_forest_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;

	if(marked!=NULL) {
		marked->mark_image_dirty( cursor, 0 );
		delete marked;
		marked = NULL;
	}
	return true;
}

const char *wkz_forest_t::move(karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	// on map?
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());

	if(plan == NULL) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.gib_2d());

	if(buttonstate==1) {
		// delete old area
		if(marked!=NULL) {
			welt->mark_area(nw, wh, false);
		}

		if(start==koord3d::invalid) {
			welt->show_distance = start = gr->gib_pos();
			marked = new zeiger_t(welt, start, sp);
			marked->setze_bild( cursor );
			gr->obj_add(marked);
		}
		else {
			nw = gr->gib_pos();

			wh.x = abs(nw.x-start.x)+1;
			wh.y = abs(nw.y-start.y)+1;
			nw.x = min(start.x, nw.x);
			nw.y = min(start.y, nw.y);

			welt->mark_area(nw, wh, true);
		}
	}
	return NULL;
}

const char *wkz_forest_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t* gr = welt->lookup_kartenboden(pos.gib_2d());
	if(!gr) {
		return "";
	}

	if(start==koord3d::invalid) {
		welt->show_distance = start = gr->gib_pos();

		marked = new zeiger_t(welt, start, sp);
		marked->setze_bild( cursor );
		gr->obj_add(marked);
	}
	else {
		if(marked!=NULL) {
			welt->mark_area(nw, wh, false);
		}

		nw = gr->gib_pos();

		wh.x = abs(nw.x-start.x)+1;
		wh.y = abs(nw.y-start.y)+1;
		nw.x = min(start.x, nw.x)+(wh.x/2);
		nw.y = min(start.y, nw.y)+(wh.y/2);

		// remove old pointers
		init(welt,sp);

		baum_t::create_forest( welt, nw.gib_2d(), wh );

		// then init
		init( welt, sp );
	}
	return NULL;
}



bool wkz_stop_moving_t::init( karte_t *, spieler_t * )
{
	last_pos = koord3d::invalid;
	last_halt = halthandle_t();
	waytype[0] = invalid_wt;
	waytype[1] = invalid_wt;
	if(wkz_linkzeiger!=NULL) {
		wkz_linkzeiger->mark_image_dirty( cursor, 0 );
		delete wkz_linkzeiger;
		wkz_linkzeiger = NULL;
	}
	return true;
}

const char *wkz_stop_moving_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	// now we can start
	const planquadrat_t *pl = welt->lookup(pos.gib_2d());
	if(pl) {
		const bool backwards=event_get_last_control_shift()==2;
		const grund_t *bd=0;
		// search all grounds for match
		halthandle_t h = pl->gib_halt();
		if(last_pos!=koord3d::invalid  &&  (h.is_bound() ^ last_halt.is_bound())) {
			init(welt,sp);
			return "Can only move from halt to halt or waypoint to waypoint.";
		}
		if(h.is_bound()  &&  !spieler_t::check_owner( sp, pl->get_haltlist()[0]->gib_besitzer())) {
			init(welt,sp);
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}

		for(  unsigned cnt=0;  cnt<pl->gib_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->gib_boden_count()-1-cnt : cnt;
			bd = pl->gib_boden_bei(i);
			// ignore tunnel (can be set with Underground mode)
			if(bd->ist_im_tunnel()) {
				bd = 0;
				continue;
			}
#if 0
			// not halt
			if((!bd->is_halt() && !bd->ist_wasser()) || pl->get_haltlist_count()==0) {
				bd = 0;
				continue;
			}
			// and check harbor ownership
			if(bd->ist_wasser()) {
				if(pl->get_haltlist_count()>0) {
					if(spieler_t::check_owner( sp, pl->get_haltlist()[0]->gib_besitzer())) {
						waytype1 = water_wt;
						break;
					}
				}
				bd = 0;
				continue;
			}
#endif
			// must be on a way or in the sea?
			if(!bd->ist_wasser()) {
				weg_t *w1 = bd->gib_weg_nr(0);
				if(  w1==NULL  ||  !spieler_t::check_owner( w1->gib_besitzer(), sp )  ) {
					// fails, if no way
					bd = 0;
					continue;
				}
				weg_t *w2 = bd->gib_weg_nr(1);
				if(  w2  &&  !spieler_t::check_owner( w2->gib_besitzer(), sp )  ) {
					// this only fails, if wrong owner
					bd = 0;
					continue;
				}
			}
			// ok, now we have old_stop
			break;
		}
		if(bd==NULL) {
			// here we failed
			return "";
		}

		if(last_pos == koord3d::invalid) {
			// put cursor
			last_pos = bd->gib_pos();
			last_halt = bd->ist_wasser() ?  haltestelle_t::gib_halt(welt,last_pos) : bd->gib_halt();
			if(bd->ist_wasser()) {
				waytype[0] = water_wt;
			}
			else {
				waytype[0] = bd->gib_weg_nr(0)->gib_waytype();
				if(bd->gib_weg_nr(1)) {
					waytype[1] = bd->gib_weg_nr(1)->gib_waytype();
				}
			}
			grund_t *gr = welt->lookup_kartenboden(last_pos.gib_2d());
			wkz_linkzeiger = new zeiger_t(welt, last_pos, sp);
			wkz_linkzeiger->setze_bild( cursor );
			gr->obj_add(wkz_linkzeiger);
		}
		else {
			// second click
			pos = bd->gib_pos();
			const halthandle_t new_halt = haltestelle_t::gib_halt(welt,pos);
			// depending on the waytype we simply built replacements lists
			// in the wort case we have to iterate over all tiles twice ...
			for(  uint i=0;  i<2;  i++  ) {
				const waytype_t wt = waytype[i];
				slist_tpl <koord3d>old_platform;

				if(bd->ist_wasser()) {
					if(wt!=water_wt) {
						break;
					}
				}
				else if(!bd->hat_weg(wt)) {
					continue;
				}
				// platform, stop or just tile moving?
				const bool catch_all_halt = (wt==water_wt  ||  wt==air_wt)  &&  last_halt.is_bound();
				if(!last_halt.is_bound()) {
					old_platform.append(last_pos);
				}
				else if(!catch_all_halt) {
					// builds a coordinate list
					if(wt==road_wt) {
						old_platform.append(last_pos);
					}
					else {
						// all connected tiles for start pos
						uint8 ribi = welt->lookup(last_pos)->gib_weg_ribi_unmasked(wt);
						koord delta = ribi_t::ist_gerade_ns(ribi) ? koord(0,1) : koord(1,0);
						koord3d start_pos=last_pos;
						while(ribi&12) {
							koord3d test_pos = start_pos+delta;
							grund_t *gr = welt->lookup(test_pos);
							if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->gib_weg_ribi_unmasked(wt))==0) {
								break;
							}
							start_pos = test_pos;
						}
						// now add all of them
						while(ribi&3) {
							koord3d test_pos = start_pos-delta;
							grund_t *gr = welt->lookup(test_pos);
							old_platform.append(start_pos);
							if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->gib_weg_ribi_unmasked(wt))==0) {
								break;
							}
							start_pos = test_pos;
						}
					}
				}

				// first, check convoi without line
				for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
					convoihandle_t cnv = *i;
					// check line and owner
					if(!cnv->get_line().is_bound()  &&  cnv->gib_besitzer()==sp) {
						fahrplan_t *fpl = cnv->gib_fahrplan();
						// check waytype
						if(fpl->ist_halt_erlaubt(bd)) {
							bool updated = false;
							for(  int k=0;  k<fpl->maxi();  k++  ) {
								if(  (catch_all_halt  &&  haltestelle_t::gib_halt(welt,fpl->eintrag[k].pos)==last_halt)  ||  old_platform.contains(fpl->eintrag[k].pos)  ) {
									fpl->eintrag[k].pos = pos;
									updated = true;
								}
							}
							if(updated) {
								fpl->cleanup();
								// set this schedule
								cnv->setze_fahrplan(fpl);
							}
						}
					}
				}
				// next, check lines serving old_halt (no owner check needed for own lines ...
				vector_tpl<linehandle_t>lines;
				sp->simlinemgmt.get_lines(simline_t::line,&lines);
				for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; ++i) {
					linehandle_t line = (*i);
					fahrplan_t *fpl = line->get_fahrplan();
					// check waytype
					if(fpl->ist_halt_erlaubt(bd)) {
						bool updated = false;
						for(  int k=0;  k<fpl->maxi();  k++  ) {
							// ok!
							if(  (catch_all_halt  &&  haltestelle_t::gib_halt(welt,fpl->eintrag[k].pos)==last_halt)  ||  old_platform.contains(fpl->eintrag[k].pos)  ) {
								fpl->eintrag[k].pos = pos;
								updated = true;
							}
						}
						// update line
						if(updated) {
							fpl->cleanup();
							sp->simlinemgmt.update_line(line);
						}
					}
				}
			}
			// since factory connections may have changed
			welt->set_schedule_counter();
			//ok! they are connected => remove marker
			init( welt, sp );
			return NULL;
		}
	}
	return "";

}

