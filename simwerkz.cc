/*
 * Tools for the players
 *
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "simdebug.h"
#include "simevent.h"
#include "simcity.h"
#include "simtools.h"
#include "simmesg.h"
#include "simwin.h"

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/schiene.h"
#include "boden/tunnelboden.h"
#include "boden/monorailboden.h"

#include "simdepot.h"
#include "simfab.h"
#include "simimg.h"
#include "simintr.h"
#include "simhalt.h"
#include "simskin.h"

#include "besch/grund_besch.h"
#include "besch/haus_besch.h"
#include "besch/roadsign_besch.h"
#include "besch/tunnel_besch.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/simpeople.h"

#include "gui/line_management_gui.h"
#include "gui/werkzeug_waehler.h"
#include "gui/station_building_select.h"
#include "gui/karte.h"	// to update map after construction of new industry
#include "gui/depot_frame.h"
#include "gui/fahrplan_gui.h"
#include "gui/player_frame_t.h"
#include "gui/schedule_list.h"
#include "gui/signal_spacing.h"
#include "gui/stadt_info.h"
#include "gui/trafficlight_info.h"
#include "gui/privatesign_info.h"
#include "gui/messagebox.h"

#include "dings/zeiger.h"
#include "dings/bruecke.h"
#include "dings/tunnel.h"
#include "dings/groundobj.h"
#include "dings/signal.h"
#include "dings/crossing.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/leitung2.h"
#include "dings/baum.h"
#include "dings/field.h"
#include "dings/label.h"

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/scenario.h"
#include "dataobj/network_cmd_ingame.h" // for dragging raise / lower tools

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "besch/weg_besch.h"
#include "besch/roadsign_besch.h"

#include "tpl/vector_tpl.h"

#include "utils/memory_rw.h"
#include "utils/simstring.h"

#include "simwerkz.h"
#include "player/finance.h"


#define is_scenario()  welt->get_scenario()->is_scripted()

#define CHECK_FUNDS() \
	/* do not allow, if out of money */ \
	if(  !welt->get_settings().is_freeplay()  &&  sp->get_player_nr()!=1  &&  !sp->has_money_or_assets() ) {\
		return "Out of funds";\
	}\


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
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price_maintenance(karte_t *welt, const char *tip, sint64 price, sint64 maintenance)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n,
		welt->ticks_per_world_month_shift>=18 ?
		(double)(maintenance << (welt->ticks_per_world_month_shift - 18)) / 100.0 :
		(double)(maintenance >> (18 - welt->ticks_per_world_month_shift)) / 100.0
	);
	strcat( werkzeug_t::toolstr, ")" );
	return werkzeug_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 */
static char const* tooltip_with_price_maintenance_level(karte_t* const welt, char const* const tip, sint64 const price, sint64 const maintenance, uint32 const level, uint8 const enables)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n,
		welt->ticks_per_world_month_shift>=18 ?
		(double)(maintenance << (welt->ticks_per_world_month_shift - 18)) / 100.0 :
		(double)(maintenance >> (18 - welt->ticks_per_world_month_shift)) / 100.0
			);
	strcat( werkzeug_t::toolstr, ")" );
	n = strlen(werkzeug_t::toolstr);

	if((enables&7)!=0) {
		n += sprintf( werkzeug_t::toolstr+n, ", %d", level*32 );
		if(enables&1) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Passagiere") );
		}
		if(enables&2) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Post") );
		}
		if(enables&4) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Fracht") );
		}
	} else if (!welt->get_settings().is_separate_halt_capacities()) {
		n += sprintf( werkzeug_t::toolstr+n, ", %s %d", translator::translate("Storage capacity"), level*32 );
	}

	return werkzeug_t::toolstr;
}



/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	// any other ground with a valid stop here?
	if(  const planquadrat_t *plan = welt->lookup(pos.get_2d())  ) {
		halthandle_t halt = plan->get_halt();
		if(  halt.is_bound()  ) {
			return (sp==halt->get_besitzer()  ||  halt->get_besitzer()->get_player_nr()==1) ? halt : halthandle_t();
		}
	}

	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos);
	if(  bd==NULL  ) {
		bd = welt->lookup_kartenboden(pos.get_2d());
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(  bd->hat_wege()  ) {
		ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
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

	// first try to connect to our own following ways
	for(  int i=0;  i<iAnzahl;  i++ ) {
		if(  const planquadrat_t *plan = welt->lookup(pos.get_2d()+next_try_dir[i])  ) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  sp==halt->get_besitzer()  ) {
				return halt;
			}
		}
	}

	// now just search all neighbours
	for(  sint16 y=-1;  y<=h;  y++  ) {
		if(  const planquadrat_t *plan = welt->lookup(pos.get_2d()+koord(-1,y))  ) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  sp==halt->get_besitzer()  ) {
				return halt;
			}
		}
		if(  const planquadrat_t *plan = welt->lookup(pos.get_2d()+koord(b,y))  ) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  sp==halt->get_besitzer()  ) {
				return halt;
			}
		}
	}
	for(  sint16 x=0;  x<b;  x++  ) {
		const planquadrat_t *plan = welt->lookup( pos.get_2d()+koord(x,-1) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  sp==halt->get_besitzer()  ) {
				return halt;
			}
		}
		plan = welt->lookup( pos.get_2d()+koord(x,h) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  sp==halt->get_besitzer()  ) {
				return halt;
			}
		}
	}

#if AUTOJOIN_PUBLIC
	// now search everything for public stops
	for(  int i=0;  i<8;  i++ ) {
		const planquadrat_t *plan = welt->lookup(pos.get_2d()+koord::neighbours[i]);
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(  halt.is_bound()  &&  welt->get_spieler(1)==halt->get_besitzer()  ) {
				return halt;
			}
		}
	}
#endif

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
}


// converts a 2d koord to a suitable ground pointer
static grund_t *wkz_intern_koord_to_weg_grund(spieler_t *sp, karte_t *welt, koord3d pos, waytype_t wt)
{
	// check for valid ground
	grund_t *gr=welt->lookup(pos);
	if (gr==NULL) {
		return NULL;
	}

	if(  wt==powerline_wt  &&  gr->get_leitung()  ) {
		// check for ownership
		if(gr->get_leitung()->ist_entfernbar(sp)!=NULL) {
			return NULL;
		}
		// ok
		else {
			return gr;
		}
	}

	// tram
	if(wt==tram_wt) {
		weg_t *way = gr->get_weg(track_wt);
		if (way && way->get_besch()->get_styp() == weg_t::type_tram &&  way->ist_entfernbar(sp)==NULL) {
			return gr;
		}
		else {
			return NULL;
		}
	}


	// has some rail or monorail?
	if(  !gr->hat_weg(wt)  ) {
		return NULL;
	}
	// check for ownership
	if(gr->get_weg(wt)->ist_entfernbar(sp)!=NULL){
		return NULL;
	}
	// ok, now we have a valid ground
	return gr;
}



/****************************************** now the actual tools **************************************/

// werkzeuge
const char *wkz_abfrage_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	if(gr) {
		DBG_MESSAGE("wkz_abfrage()","checking map square %s", pos.get_str());

		if(  umgebung_t::single_info  ) {

			int old_count = win_get_open_count();

			if(  is_ctrl_pressed()  ) {
				// reverse order
				for(int n=0;  n<gr->get_top();  n++  ) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar  &&  dt->get_typ()!=ding_t::label) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						dt->zeige_info();
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}

				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->zeige_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->zeige_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}

			}
			else {

				// show halt and labels first ...
				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->zeige_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}
				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->zeige_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				for (size_t n = gr->get_top(); n-- != 0;) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar  &&  dt->get_typ()!=ding_t::label) {
						DBG_MESSAGE("wkz_abfrage()", "index %u", (unsigned)n);
						dt->zeige_info();
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}
			}

			// no window yet opened -> try ground info
			gr->zeige_info();
		}
		else {
			// lowest (less interesting) first
			gr->zeige_info();
			for(int n=0; n<gr->get_top();  n++  ) {
				ding_t *dt = gr->obj_bei(n);
				if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar) {
					dt->zeige_info();
				}
			}
		}

		if(gr->get_depot()  &&  gr->get_depot()->get_besitzer()==sp) {
			int old_count = win_get_open_count();
			gr->get_depot()->zeige_info();
			// did some new window open?
			if(umgebung_t::single_info  &&  old_count!=win_get_open_count()) {
				return NULL;
			}
		}
	}
	return NULL;
}


/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool wkz_remover_t::wkz_remover_intern(spieler_t *sp, karte_t *welt, koord3d pos, const char *&msg)
{
DBG_MESSAGE("wkz_remover_intern()","at (%s)", pos.get_str());

	grund_t *gr = welt->lookup(pos);
	if (!gr) {
		msg = "";
		return false;
	}

	// check if there is something to remove from here ...
	if(gr->get_top()==0  ) {
		msg = "";
		return false;
	}

	// marker?
	label_t* l = gr->find<label_t>();
	if(l) {
		msg = l->ist_entfernbar(sp);
		if(msg==NULL) {
			delete l;
			return true;
		}
		else if(  gr->get_top()==1  ) {
			// only complain if this is the last object on this tile ...
			return false;
		}
		msg = NULL;
		// not deletable: skip it
	}

	// citycar? (we allow always)
	stadtauto_t* citycar = gr->find<stadtauto_t>();
	if(citycar) {
		delete citycar;
		return true;
	}

	// pedestrians?
	fussgaenger_t* fussgaenger = gr->find<fussgaenger_t>();
	if (fussgaenger) {
		delete fussgaenger;
		return true;
	}

	// prissi: check powerline (can cross ground of another player)
	leitung_t* lt = gr->get_leitung();
	if(lt!=NULL  &&  lt->ist_entfernbar(sp)==NULL) {
		if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
			bruecke_t* br = gr->find<bruecke_t>();
			if(  br == NULL  ) {
				// no bridge? most likely transformer on a former bridge tile...
				grund_t *gr_new = new boden_t(welt, pos, gr->get_grund_hang());
				gr_new->take_obj_from( gr );
				welt->access(pos.get_2d())->kartenboden_setzen( gr_new );
				gr = gr_new;
			}
			else {
				if (br->get_besch()->get_waytype()==powerline_wt) {
					msg = brueckenbauer_t::remove(welt, sp, gr->get_pos(), powerline_wt );
					return msg == NULL;
				}
			}
		}
		if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
			if (gr->find<tunnel_t>()->get_besch()->get_waytype()==powerline_wt) {
				msg = tunnelbauer_t::remove(welt, sp, gr->get_pos(), powerline_wt );
				return msg == NULL;
			}
		}
		if(  gr->ist_im_tunnel()  ) {
			lt->entferne(sp);
			delete lt;
			// now everything gone?
			if(  gr->get_top() == 1  ) {
				// delete tunnel too
				tunnel_t *t = gr->find<tunnel_t>();
				t->entferne(sp);
				delete t;
			}
			// unmark kartenboden (is marked during underground mode deletion)
			welt->lookup_kartenboden(pos.get_2d())->clear_flag(grund_t::marked);
			// remove upper or lower ground
			welt->access(pos.get_2d())->boden_entfernen(gr);
			delete gr;
		}
		else {
			lt->entferne(sp);
			delete lt;
		}
		return true;
	}

	// check for signal
	roadsign_t* rs = gr->find<signal_t>();
	if (rs == NULL) rs = gr->find<roadsign_t>();
	if(rs!=NULL) {
		msg = rs->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
DBG_MESSAGE("wkz_remover()",  "removing roadsign at (%s)", pos.get_str());
		weg_t *weg = gr->get_weg(rs->get_besch()->get_wtyp());
		if(  weg==NULL  &&  rs->get_besch()->get_wtyp()==tram_wt  ) {
			weg = gr->get_weg(track_wt);
		}
		rs->entferne(sp);
		delete rs;
		assert( weg );
		weg->count_sign();
		return true;
	}

	// check stations
	halthandle_t halt = gr->get_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::get_fab(welt,pos.get_2d())==NULL) {
		// halt and not a factory (oil rig etc.)
		const spieler_t* owner = halt->get_besitzer();
		if(  spieler_t::check_owner( owner, sp )  ) {
			return haltestelle_t::remove(welt, sp, gr->get_pos());
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
		depot_t *dep = gr->get_depot();
		if( dep ) {
			dep->update_win();
		}
		return true;
	}

DBG_MESSAGE("wkz_remover()", "check tunnel/bridge");

	// beginning/end of bridge?
	if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing bridge from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		bruecke_t* br = gr->find<bruecke_t>();
		msg = brueckenbauer_t::remove(welt, sp, gr->get_pos(), br->get_besch()->get_waytype());
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing tunnel  from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		msg = tunnelbauer_t::remove(welt, sp, gr->get_pos(), gr->get_weg_nr(0)->get_waytype());
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
			koord pos = gr->get_pos().get_2d();
			sint8 dummy;
			welt->access(pos)->boden_ersetzen( gr, new boden_t(welt, gr->get_pos(), welt->recalc_natural_slope(pos,dummy) ) );
			welt->lookup_kartenboden(pos)->calc_bild();
			welt->lookup_kartenboden(pos)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// depots
	depot_t* dep = gr->get_depot();
	if (dep) {
		msg = dep->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		dep->entferne(sp);
		delete dep;
		return true;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(gb != NULL) {
		msg = gb->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		if(!gb->get_tile()->get_besch()->can_rotate()  &&  welt->cannot_save()) {
			msg = "Not possible in this rotation!";
			return false;
		}
		DBG_MESSAGE("wkz_remover()",  "removing building" );

		// remove town? (when removing townhall)
		if(gb->ist_rathaus()) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos.get_2d());
			if(!welt->rem_stadt( stadt )) {
				msg = "Das Feld gehoert\neinem anderen Spieler\n";
				return false;
			}
		}
		else {
			// townhall is also removed during town removal
			hausbauer_t::remove( welt, sp, gb );
		}
		return true;
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
	// do not delete pointers - they may come from players on other clients
	zeiger_t *zeiger = gr->find<zeiger_t>();
	if(zeiger) {
		gr->obj_remove(zeiger);
	}
	// do not delete other players label
	label_t *label = gr->find<label_t>();
	if(label) {
		gr->obj_remove(label);
	}

	// remove all other stuff (clouds, ...)
	bool return_ok = false;
	uint8 num_obj = gr->obj_count();
	if(num_obj>0) {
		msg = gr->kann_alle_obj_entfernen(sp);
		return_ok = (msg==NULL  &&  !(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->obj_loesche_alle(sp));
		DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
	}

	if(lt) {
		DBG_MESSAGE("wkz_remover()",  "add again powerline");
		gr->obj_add(lt);
	}
	if(cr) {
		gr->obj_add(cr);
	}
	if(zeiger) {
		gr->obj_add(zeiger);
	}
	if(label) {
		gr->obj_add(label);
	}

	// could not delete everything
	if(msg) {
		return false;
	}
	if(return_ok) {
		// no sound
		msg = "";
		return true;
	}

	// ok, now we remove every object that should be removed - one by one.
	// the following objects will be removed together
DBG_MESSAGE("wkz_remover()", "removing way");

	waytype_t wt = ignore_wt;
	if(gr->get_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w = gr->get_weg_nr(1);
		if(gr->get_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(  w  &&  w->get_waytype()==water_wt  ) {
			// remove the other way first
			w = NULL;
		}
		if(w==NULL  ||  w->ist_entfernbar(sp)!=NULL) {
			w = gr->get_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(w->ist_entfernbar(sp)!=NULL){
				msg = w->ist_entfernbar(sp);
				return false;
			}
		}
		wt = w->get_besch()->get_finance_waytype();
		long cost_sum = gr->weg_entfernen(w->get_waytype(), true);
		spieler_t::book_construction_costs(sp, -cost_sum, pos.get_2d(), wt);
	}
	else {
		// remove ways and tunnel
		if(  weg_t *weg = gr->get_weg_nr(0)  ) {
			gr->remove_everything_from_way(sp, weg->get_waytype(), ribi_t::keine);
		}
		// delete tunnel here - if there is lonely tunnel without way
		if(  gr->get_top()==1  ) {
			tunnel_t *t = gr->find<tunnel_t>();
			t->entferne(sp);
			delete t;
		}
	}

	// remove empty tile
	if(  !gr->ist_karten_boden()  &&  gr->get_top()==0  ) {
		// unmark kartenboden (is marked during underground mode deletion)
		welt->lookup_kartenboden(pos.get_2d())->clear_flag(grund_t::marked);
		// remove upper or lower ground
		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;
	}

	return true;
}



const char *wkz_remover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
	const char *fail = NULL;
	if(!wkz_remover_intern(sp, welt, pos, fail)) {
		return fail;
	}

	// must recalc neighbourhood for slopes etc.
	if(pos.x>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::west)->calc_bild();
	}
	if(pos.y>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::nord)->calc_bild();
	}

	if(pos.x<welt->get_size().x-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::ost)->calc_bild();
	}
	if(pos.y<welt->get_size().y-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::sued)->calc_bild();
	}

	return NULL;
}



const char *wkz_raise_lower_base_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	CHECK_FUNDS();

	const char *result = NULL;
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = get_drag_height(welt,pos.get_2d());
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		if (umgebung_t::networkmode) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(sp, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( welt, sp, pos );
		}
		default_param = NULL;
	}
	return result;
}


bool wkz_raise_lower_base_t::drag(karte_t *welt, koord pos, sint16 height, int &n)
{
	// dragging may be going up or down!
	while(welt->lookup_hgt(pos)<height) {
		int diff = welt->grid_raise(pos);
		if(diff==0) break;
		n += diff;
	}
	// when going down need to check here we will not be going below sea level
	// cannot rely on check within lower as water height can be recalculated
	while(  height >= welt->get_water_hgt(pos)  &&  welt->lookup_hgt(pos) > height  ) {
		int diff = welt->grid_lower(pos);
		if(  diff == 0  ) {
			break;
		}
		n += diff;
	}
	return height == welt->lookup_hgt(pos);
}


bool wkz_raise_lower_base_t::check_dragging()
{
	// reset dragging
	if(  is_dragging  &&  strempty(default_param)  ) {
		is_dragging = false;
		return false;
	}
	return true;
}


sint16 wkz_raise_t::get_drag_height(karte_t *welt, koord pos)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(pos);

	return  gr->get_hoehe(welt->get_corner_to_operate(pos)) + 1;
}


const char *wkz_raise_t::check_pos( karte_t *welt, spieler_t *, koord3d k )
{
	// check for underground mode
	if (is_dragging  &&  drag_height-1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	if (! welt->is_within_grid_limits(k.get_2d())) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(welt,k.get_2d());
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}


const char *wkz_raise_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if (!check_dragging()) {
		return NULL;
	}

	bool ok = false;
	koord pos = k.get_2d();

	CHECK_FUNDS();

	if(welt->is_within_grid_limits(pos)) {

		const sint8 hgt = (sint8) get_drag_height(welt,k.get_2d());

		if(hgt <= welt->get_maximumheight()) {

			int n = 0;	// tiles changed
			if (!strempty(default_param)) {
				// called by dragging or by AI
				ok = drag(welt, pos, atoi(default_param), n);
			}
			else {
				n = welt->grid_raise(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::book_construction_costs(sp, welt->get_settings().cst_alter_land * n, pos, ignore_wt);
			}
			return !ok ? "Tile not empty." : (n ? NULL : "");
		}
		else {
			// no mountains higher than welt->get_maximumheight() ...
			return "Maximum tile height difference reached.";
		}
	}
	return "Zu nah am Kartenrand";
}


sint16 wkz_lower_t::get_drag_height(karte_t *welt, koord pos)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(pos);

	return  gr->get_hoehe(welt->get_corner_to_operate(pos)) - 1;
}


const char *wkz_lower_t::check_pos( karte_t *welt, spieler_t *, koord3d k )
{
	// check for underground mode
	if (is_dragging  &&  drag_height+1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	if (! welt->is_within_grid_limits(k.get_2d())) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(welt,k.get_2d());
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}


const char *wkz_lower_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if (!check_dragging()) {
		return NULL;
	}

	bool ok = false;
	koord pos = k.get_2d();

	CHECK_FUNDS();

	if(welt->is_within_grid_limits(pos)) {
		const sint8 hgt = (sint8) get_drag_height(welt,k.get_2d());

		if(  hgt >= welt->get_water_hgt( k.get_2d() )  ) {
			int n = 0; // tiles changed
			if (!strempty(default_param)) {
				// called by dragging or by AI
				ok = drag(welt, pos, atoi(default_param), n);
			}
			else {
				n = welt->grid_lower(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::book_construction_costs(sp, welt->get_settings().cst_alter_land * n, pos, ignore_wt);
			}
			return !ok ? "Tile not empty." : (n ? NULL : "");
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}


const char *wkz_setslope_t::check_pos( karte_t *welt, spieler_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  ) {
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else {
		return "";
	}
	return NULL;
}

const char *wkz_restoreslope_t::check_pos( karte_t *welt, spieler_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  ) {
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else {
		return "";
	}
	return NULL;
}

/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
const char *wkz_setslope_t::wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int new_slope )
{
	if(  !grund_besch_t::double_grounds  ) {
		// translate old single slope parameter to new double slope
		if(  0 < new_slope  &&  new_slope < ALL_UP_SLOPE_SINGLE  ) {
			new_slope = scorner1(new_slope) + scorner2(new_slope) * 3 + scorner3(new_slope) * 9 + scorner4(new_slope) * 27;
		}
		else {
			switch(  new_slope  ) {
				case ALL_UP_SLOPE_SINGLE:   new_slope = ALL_UP_SLOPE;   break;
				case ALL_DOWN_SLOPE_SINGLE: new_slope = ALL_DOWN_SLOPE; break;
				case RESTORE_SLOPE:
				case RESTORE_SLOPE_SINGLE:  new_slope = RESTORE_SLOPE;  break;
				default:
					return ""; // invalid parameter
			}
		}
	}

	bool ok = false;

	grund_t *gr1 = welt->lookup(pos);
	if(  gr1  ) {
		const uint8 max_hdiff = grund_besch_t::double_grounds ?  2 : 1;

		// at least a pixel away from the border?
		if(  pos.z < welt->get_water_hgt( pos.get_2d() )  &&  !gr1->ist_tunnel()  ) {
			return "Maximum tile height difference reached.";
		}

		if(  new_slope==RESTORE_SLOPE  &&  !(gr1->get_typ()==grund_t::boden  ||  gr1->get_typ()==grund_t::wasser)  ) {
			return "No suitable ground!";
		}

		// finally: empty enough
		if(  gr1->get_grund_hang()!=gr1->get_weg_hang()  ||  gr1->get_halt().is_bound()  ||  gr1->kann_alle_obj_entfernen(sp)  ||
				   gr1->find<gebaeude_t>()  ||  gr1->get_depot()  ||  (gr1->get_leitung() && gr1->hat_wege())  ||  gr1->get_weg(air_wt)  ||  gr1->find<label_t>()  ||  gr1->get_typ()==grund_t::brueckenboden) {
			return "Tile not empty.";
		}

		if(  !welt->is_within_limits(pos.get_2d()+koord(1,1))  ||  !welt->is_within_limits(pos.get_2d()+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
		}

		// slopes may affect the position and the total height!
		koord3d new_pos = pos;

		if(  gr1->hat_wege() || gr1->get_leitung() ) {
			// check the resulting slope
			ribi_t::ribi ribis = 0;
			if( gr1->hat_wege()) {
				ribis |= gr1->get_weg_nr(0)->get_ribi_unmasked();
				if(  gr1->get_weg_nr(1)  ) {
					ribis |= gr1->get_weg_nr(1)->get_ribi_unmasked();
				}
			}
			if( gr1->get_leitung()) {
				ribis |= gr1->get_leitung()->get_ribi();
			}

			if(  new_slope==RESTORE_SLOPE  ||  !ribi_t::ist_einfach(ribis)  ||  (new_slope<hang_t::erhoben  &&  ribi_t::rueckwaerts(ribi_typ(new_slope))!=ribis)  ) {
				// has the wrong tilt
				return "Tile not empty.";
			}
			/* new things getting tricky:
			 * A single way on an allup or down slope will result in
			 * a slope with the way as hinge.
			 */
			if(  new_slope==ALL_UP_SLOPE  ) {
				if(  gr1->get_weg_hang()==hang_t::flach  ) {
					new_slope = hang_typ(ribis);
				}
				else if(  gr1->get_weg_hang() == hang_typ(ribis)  ) {
					// check that weg_besch supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_besch()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_besch()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_besch()->has_double_slopes())  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ(ribis) * 2;
				}
				else if(  gr1->get_weg_hang() == hang_typ( ribi_t::rueckwaerts(ribis) ) * 2  ) {
					new_pos.z++;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) );
				}
				else if(  gr1->get_weg_hang() != hang_typ( ribi_t::rueckwaerts(ribis) )  ) {
					return "Maximum tile height difference reached.";
				}
			}
			else if(  new_slope==ALL_DOWN_SLOPE  ) {
				if(  gr1->get_grund_hang()==hang_typ(ribis)  ) {
					// do not lower tiles to sea
					if(  pos.z == welt->get_water_hgt( pos.get_2d() )  &&  !gr1->ist_tunnel()  ) {
						return "Tile not empty.";
					}
				}
				else if(  gr1->get_grund_hang() == hang_typ(ribis) * 2  ) {
					if(  pos.z == welt->get_water_hgt( pos.get_2d() )  &&  !gr1->ist_tunnel()  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ(ribis);
				}
				else if(  gr1->get_grund_hang() == hang_t::flach  ) {
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) );
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
				}
				else if(  gr1->get_grund_hang() == hang_typ( ribi_t::rueckwaerts(ribis) )  ) {
					// check that weg_besch supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_besch()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_besch()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_besch()->has_double_slopes())  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) ) * 2;
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
				}
				else {
					return "Maximum tile height difference reached.";
				}
			}
		}

		if(  new_slope == ALL_DOWN_SLOPE  ||  new_slope == RESTORE_SLOPE  ) {
			if(  new_slope == RESTORE_SLOPE  ) {
				// prissi: special action: set to natural slope
				sint8 min_hgt;
				new_slope = welt->recalc_natural_slope( pos.get_2d(), min_hgt );
				new_pos = koord3d( pos.get_2d(), min_hgt );
				DBG_MESSAGE("natural_slope","%i",new_slope);
			}
			else {
				new_slope = hang_t::flach;
				// is more intuitive: if there is a slope, first downgrade it
				if(  gr1->get_grund_hang() == 0  ) {
					new_pos.z--;
				}
			}

			// now prevent being lowered below neighbouring water
			sint8 water_table = (welt->get_water_hgt( pos.get_2d() ) >= (gr1->get_hoehe() + gr1->get_grund_hang() ? 1 : 0)) ? welt->get_water_hgt( pos.get_2d() ) : welt->get_grundwasser() - 4;
			sint8 min_neighbour_height = gr1->get_hoehe();

			for(  sint16 i = 0 ;  i < 8 ;  i++  ) {
				const koord neighbour = pos.get_2d() + koord::neighbours[i];

				if(  welt->is_within_grid_limits( neighbour )  ) {
					grund_t *gr2 = welt->lookup_kartenboden( neighbour );
					if(  gr2  &&  (welt->get_water_hgt( neighbour ) >= (gr2->get_hoehe() + (gr2->get_grund_hang() ? 1 : 0)))  ) {
						water_table = max( water_table, welt->get_water_hgt( neighbour ) );
					}
					if(  gr2  &&  gr2->get_hoehe() < min_neighbour_height  ) {
						min_neighbour_height = gr2->get_hoehe();
					}
				}
			}

			if(  water_table>new_pos.z  ||  (water_table == new_pos.z  &&  min_neighbour_height < new_pos.z)  ) {
				// do not lower tiles when it will be below water level
				return "Tile not empty.";
			}
			welt->set_water_hgt( pos.get_2d(), water_table );
		}
		else if(  new_slope == ALL_UP_SLOPE  ) {
			new_slope = hang_t::flach;
			new_pos.z++;
		}

		// already some ground here (tunnel, bridge, monorail?)
		if(  new_pos.z != pos.z  &&  welt->lookup(new_pos) != NULL  ) {
			return "Tile not empty.";
		}
		// check for grounds above / below
		if(  new_pos.z >= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, 1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 2) );
			}
			if(  !gr2  &&  umgebung_t::pak_height_conversion_factor  ==  2  &&  (gr1->hat_wege()  ||  gr1->get_leitung())  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  gr2->get_pos().z - new_pos.z + hang_t::diff( gr2->get_weg_hang(), new_slope ) < umgebung_t::pak_height_conversion_factor  ) {
				return "Tile not empty.";
			}
		}
		if(  new_pos.z <= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, -1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -2) );
			}
			if(  !gr2  &&  umgebung_t::pak_height_conversion_factor == 2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  new_pos.z - gr2->get_pos().z + hang_t::diff( new_slope, gr2->get_weg_hang() ) < umgebung_t::pak_height_conversion_factor  ) {
				return "Tile not empty.";
			}
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z;
		// maximum difference
		const sint8 test_hgt = hgt+(new_slope!=0);

		if(  gr1->get_typ()==grund_t::boden  ) {
			// first left side
			const grund_t *grleft=welt->lookup_kartenboden(pos.get_2d()+koord(-1,0));
			if(grleft) {
				const sint16 left_hgt=grleft->get_hoehe() + (new_slope==ALL_DOWN_SLOPE && grleft->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(left_hgt-test_hgt);
				if(  diff_from_ground > 2 * max_hdiff  ) {
					return "Maximum tile height difference reached.";
				}
			}

			// right side
			const grund_t *grright=welt->lookup_kartenboden(pos.get_2d()+koord(1,0));
			if(grright) {
				const sint16 right_hgt=grright->get_hoehe()  + (new_slope==ALL_DOWN_SLOPE && grright->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(right_hgt-test_hgt);
				if(  diff_from_ground > 2 * max_hdiff  ) {
					return "Maximum tile height difference reached.";
				}
			}

			const grund_t *grback=welt->lookup_kartenboden(pos.get_2d()+koord(0,-1));
			if(grback) {
				const sint16 back_hgt=grback->get_hoehe()  + (new_slope==ALL_DOWN_SLOPE && grback->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(back_hgt-test_hgt);
				if(  diff_from_ground > 2 * max_hdiff  ) {
					return "Maximum tile height difference reached.";
				}
			}

			const grund_t *grfront=welt->lookup_kartenboden(pos.get_2d()+koord(0,1));
			if(grfront) {
				const sint16 front_hgt=grfront->get_hoehe()  + (new_slope==ALL_DOWN_SLOPE && grfront->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(front_hgt-test_hgt);
				if(  diff_from_ground > 2 * max_hdiff  ) {
					return "Maximum tile height difference reached.";
				}
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=pos);
		bool slope_changed = new_slope!=gr1->get_grund_hang();
		ok |= slope_changed;

		if(ok) {
			if(  gr1->kann_alle_obj_entfernen(sp)  ) {
				// not empty ...
				return "Tile not empty.";
			}
			// check way ownership
			if(gr1->hat_wege()) {
				if(gr1->get_weg_nr(0)->ist_entfernbar(sp)!=NULL) {
					return "Tile not empty.";
				}
				if(gr1->has_two_ways()  &&  gr1->get_weg_nr(1)->ist_entfernbar(sp)!=NULL) {
					return "Tile not empty.";
				}
			}

			// ok, was sucess
			if(  !gr1->ist_wasser()  &&  new_slope == 0  &&  new_pos.z == welt->get_water_hgt( pos.get_2d() )  &&  gr1->get_typ() != grund_t::tunnelboden  ) {
				// now water
				gr1->obj_loesche_alle(sp);
				welt->access(pos.get_2d())->kartenboden_setzen( new wasser_t(welt,new_pos) );
				gr1 = welt->lookup_kartenboden(new_pos.get_2d());
			}
			else if(  gr1->ist_wasser()  &&  (new_pos.z > welt->get_water_hgt( pos.get_2d() )  ||  new_slope != 0)  ) {
				// build underwater hill first
				if(  !welt->ebne_planquadrat( sp, pos.get_2d(), welt->get_water_hgt( pos.get_2d() ), false, true )  ) {
					return "Tile not empty.";
				}
				gr1->obj_loesche_alle(sp);
				welt->access(pos.get_2d())->kartenboden_setzen( new boden_t(welt,new_pos,new_slope) );
				gr1 = welt->lookup_kartenboden(new_pos.get_2d());
			}
			else {
				gr1->set_grund_hang(new_slope);
				gr1->set_pos(new_pos);
				gr1->clear_flag(grund_t::marked);
				gr1->set_flag(grund_t::dirty);
				// update new positions if changed
				if(  new_pos!=pos  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						gr1->obj_bei(i)->set_pos( new_pos );
					}
				}
				// correct tree offsets if slope has changed
				if(  slope_changed  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						baum_t *tree = ding_cast<baum_t>(gr1->obj_bei(i));
						if (tree) {
							tree->recalc_off();
						}
					}
				}
				if(  !gr1->ist_karten_boden()  ) {
					gr1->calc_bild();
				}
			}

			if(  gr1->ist_karten_boden()  ) {
				if(  new_slope!=hang_t::flach  ) {
					// no lakes on slopes ...
					groundobj_t *d = gr1->find<groundobj_t>();
					if(  d  &&  d->get_besch()->get_phases()!=16  ) {
						d->entferne(sp);
						delete d;
					}
					// connect canals to sea
					if(  gr1->get_hoehe() == welt->get_water_hgt( pos.get_2d() )  &&  gr1->hat_weg(water_wt)  ) {
						grund_t *sea = welt->lookup_kartenboden(new_pos.get_2d() - koord( ribi_typ(new_slope ) ));
						if (sea  &&  sea->ist_wasser()) {
							gr1->weg_erweitern(water_wt, ribi_t::rueckwaerts(ribi_typ(new_slope)));
							sea->calc_bild();
						}
					}
				}
				// recalc slope walls on neightbours
				for(int y=-1; y<=1; y++) {
					for(int x=-1; x<=1; x++) {
						grund_t *gr = welt->lookup_kartenboden(pos.get_2d()+koord(x,y));
						gr->calc_bild();
					}
				}
				// corect the grid height
				if(  gr1->ist_wasser()  ) {
					sint8 grid_hgt = min( welt->get_water_hgt( pos.get_2d() ), welt->lookup_hgt( pos.get_2d() ) );
					welt->set_grid_hgt(pos.get_2d(), grid_hgt );
				}
				else {
					welt->set_grid_hgt(pos.get_2d(), gr1->get_hoehe()+ corner4(gr1->get_grund_hang()) );
				}
				reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
				welt->calc_climate( pos.get_2d(), true );
			}
			settings_t const& s = welt->get_settings();
			spieler_t::book_construction_costs(sp, new_slope == RESTORE_SLOPE ? s.cst_alter_land : s.cst_set_slope, pos.get_2d(), ignore_wt);
		}

	}
	return ok ? NULL : "";
}



// set marker
const char *wkz_marker_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(welt->is_within_limits(pos.get_2d())) {
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		if (gr) {
			if(!gr->get_text()) {
				const ding_t* thing = gr->obj_bei(0);
				if(thing == NULL  ||  thing->get_besitzer() == sp  ||  (spieler_t::check_owner(thing->get_besitzer(), sp)  &&  (thing->get_typ() != ding_t::gebaeude))) {
					gr->obj_add(new label_t(welt, gr->get_pos(), sp, "\0"));
					if (is_local_execution()) {
						gr->find<label_t>()->zeige_info();
					}
					return "";
				}
			}
		}
	}
	return "Das Feld gehoert\neinem anderen Spieler\n";
}



// show/repair blocks
bool wkz_clear_reservation_t::init( karte_t *welt, spieler_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = true;
		welt->set_dirty();
	}
	return true;
}

bool wkz_clear_reservation_t::exit( karte_t *welt, spieler_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = false;
		welt->set_dirty();
	}
	return true;
}

const char *wkz_clear_reservation_t::work( karte_t *welt, spieler_t *, koord3d k )
{
	grund_t *gr = welt->lookup(k);
	if(gr) {
		for(unsigned wnr=0;  wnr<2;  wnr++  ) {

			schiene_t const* const w = ding_cast<schiene_t>(gr->get_weg_nr(wnr));
			// is this a reserved track?
			if(w!=NULL  &&  w->is_reserved()) {
				/* now we do a very crude procedure:
				 * - we search all ways for reservations of this convoi and remove them
				 * - we set the convoi state to ROUTING_1; it must rereserve its ways then
				 */
				const waytype_t waytype = w->get_waytype();
				const convoihandle_t cnv = w->get_reserved_convoi();
				if(cnv->get_state()==convoi_t::DRIVING) {
					// reset driving state
					cnv->suche_neue_route();
				}
				FOR(slist_tpl<weg_t*>, const w, weg_t::get_alle_wege()) {
					if (w->get_waytype() == waytype) {
						schiene_t* const sch = ding_cast<schiene_t>(w);
						if (sch->get_reserved_convoi() == cnv) {
							vehikel_t& v = *cnv->front();
							if (!gr->suche_obj(v.get_typ())) {
								// force free
								sch->unreserve(&v);
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
const char* wkz_transformer_t::get_tooltip(const spieler_t *sp) const
{
	settings_t const& s = sp->get_welt()->get_settings();
	sprintf(toolstr, "%s, %ld$ (%ld$)", translator::translate("Build drain"), (long)(s.cst_transformer / -100), (long)(s.cst_maintain_transformer << (sp->get_welt()->ticks_per_world_month_shift - 18)) / -100);
	return toolstr;
}

image_id wkz_transformer_t::get_icon(spieler_t* const sp) const
{
	return wegbauer_t::waytype_available( powerline_wt, sp->get_welt()->get_timeline_year_month() ) ? icon : IMG_LEER;
}

bool wkz_transformer_t::init( karte_t *welt, spieler_t *)
{
	return wegbauer_t::waytype_available( powerline_wt, welt->get_timeline_year_month() );
}


const char *wkz_transformer_t::check_pos( karte_t *welt, spieler_t *, koord3d pos )
{
	if(grund_t::underground_mode == grund_t::ugm_all  &&  umgebung_t::networkmode) {
		// clients cannot guess at which height transformer should be build
		return "Cannot built this station/building\nin underground mode here.";
	}
	if(grund_t::underground_mode == grund_t::ugm_level) {
		// only above or directly under surface
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		return (gr->get_pos() == pos  ||  gr->get_hoehe() == grund_t::underground_level+1) ? NULL : "";
	}
	return NULL;
}


const char *wkz_transformer_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	DBG_MESSAGE("wkz_transformer_t()","called on %d,%d", k.x, k.y);

	grund_t *gr = welt->lookup_kartenboden(k.get_2d());
	if(  !welt->get_settings().get_allow_underground_transformers()  &&  k.z!=gr->get_hoehe()  ) {
		// no underground transformers allowed
		return "Cannot built this station/building\nin underground mode here.";
	}

	bool underground = false;
	fabrik_t *fab = NULL;
	// full underground mode: coordinate is on ground, adjust it to one level below ground
	// not possible in network mode!
	if (!umgebung_t::networkmode  &&  grund_t::underground_mode == grund_t::ugm_all) {
		k = gr->get_pos() - koord3d(0,0,1);
	}
	// search for factory
	// must be independent of network mode
	if (gr->get_pos().z <= k.z) {
		fab = leitung_t::suche_fab_4(k.get_2d());
	}
	else if (gr->get_pos().z == k.z+1) {
		fab = fabrik_t::get_fab(welt, k.get_2d());
		underground = true;
	}

	if( !fab  ) {
		return "Transformer only next to factory!";
	}
	if(  fab->is_transformer_connected()  ) {
		return "Only one transformer per factory!";
	}

	// underground: first build tunnel tile	at coordinate k
	if(underground) {
		if(gr->ist_wasser()) {
			return "Transformer only next to factory!";
		}

		if(welt->lookup(k)) {
			return "Tile not empty.";
		}

		const tunnel_besch_t *tunnel_besch = tunnelbauer_t::find_tunnel(powerline_wt, 0, 0);
		if(  tunnel_besch==NULL  ) {
			return "Cannot built this station/building\nin underground mode here.";
		}

		tunnelboden_t* tunnel = new tunnelboden_t(welt, k, 0);
		welt->access(k.get_2d())->boden_hinzufuegen(tunnel);
		tunnel->obj_add(new tunnel_t(welt, k, sp, tunnel_besch));
		spieler_t::add_maintenance( sp, tunnel_besch->get_wartung(), tunnel_besch->get_finance_waytype() );
		gr = tunnel;
	}
	else {
		// above ground: check for clear tile
		if(gr->get_grund_hang()!=0  ||  !gr->ist_natur()) {
			return "Transformer only on flat bare land!";
		}
		// remove everything on that spot
		if(const char *fail = gr->kann_alle_obj_entfernen(sp)) {
			return fail;
		}
		gr->obj_loesche_alle(sp);
	}
	// transformer will be build on tile pointed to by gr

	// build source or drain depending on factory type
	if(fab->get_besch()->is_electricity_producer()) {
		pumpe_t *p = new pumpe_t(welt, gr->get_pos(), sp);
		gr->obj_add( p );
		p->laden_abschliessen();
	}
	else {
		senke_t *s = new senke_t(welt, gr->get_pos(), sp);
		gr->obj_add(s);
		s->laden_abschliessen();
	}

	return NULL;	// ok
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
const char *wkz_add_city_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	CHECK_FUNDS();

	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->get_grund_hang() == 0  &&
			hausbauer_t::get_special( 0, haus_besch_t::rathaus, welt->get_timeline_year_month(), 0, welt->get_climate( pos.get_2d() ) ) != NULL  ) {

			gebaeude_t const* const gb = ding_cast<gebaeude_t>(gr->first_obj());
			if(gb && gb->ist_rathaus()) {
				dbg->warning("wkz_add_city()", "Already a city here");
				return "Tile not empty.";
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int const citizens = (int)(welt->get_settings().get_mittlere_einwohnerzahl() * 0.9);
				//  stadt_t *stadt = new stadt_t(welt, welt->get_spieler(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citicens
				stadt_t* stadt = new stadt_t(welt->get_spieler(1), pos.get_2d(), citizens / 10);

				welt->add_stadt(stadt);
				stadt->laden_abschliessen();
				stadt->verbinde_fabriken();

				spieler_t::book_construction_costs(sp, welt->get_settings().cst_found_city, pos.get_2d(), ignore_wt);
				reliefkarte_t::get_karte()->calc_map();
				return NULL;
			}
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}

// buy a house
const char *wkz_buy_house_t::work( karte_t *welt, spieler_t *sp, koord3d pos)
{
	if ( sp == welt->get_spieler(1) ) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(!gr  ||  gr->hat_wege()  ||  gr->get_halt().is_bound()) {
		return "";
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(  gb== NULL  ||  gb->get_haustyp()==gebaeude_t::unbekannt  ||  !spieler_t::check_owner(gb->get_besitzer(),sp)  ) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	if(  gb->get_besitzer()==sp  ) {
		// I bought this already ...
		return "";
	}

	spieler_t *old_owner = gb->get_besitzer();
	const haus_tile_besch_t *tile  = gb->get_tile();
	const haus_besch_t * hb = tile->get_besch();
	koord size = hb->get_groesse( tile->get_layout() );

	koord k;
	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if(  gb_part  &&  gb_part->get_tile()->get_besch()==hb  &&  spieler_t::check_owner(gb_part->get_besitzer(),sp)  ) {
					sint32 const maint = welt->get_settings().maint_building * hb->get_level();
					spieler_t::add_maintenance(old_owner, -maint, gb->get_waytype());
					spieler_t::add_maintenance(sp,        +maint, gb->get_waytype());
					gb->set_besitzer(sp);
					spieler_t::book_construction_costs(sp, -maint, k + pos.get_2d(), gb->get_waytype());
				}
			}
		}
	}
	return NULL;
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
	stadt_t *city = welt->suche_naechste_stadt(pos.get_2d());
	if(city!=NULL) {
		city->change_size( atoi(default_param) );
		// Knightly : update the links from other cities to this city
		FOR(weighted_vector_tpl<stadt_t*>, const c, welt->get_staedte()) {
			c->remove_target_city(city);
			c->add_target_city(city);
		}
		return NULL;
	}
	return "";
}


/* change climate
 * @author kieron
 */
uint8 wkz_set_climate_t::is_valid_pos(karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d &)
{
	// do really nothing ...
	return 2;
}

/*bool wkz_set_climate_t::init(karte_t *, spieler_t *)
{
	cursor = werkzeug_t::general_tool[WKZ_SETSLOPE]->cursor;
	return true;
}*/


void wkz_set_climate_t::mark_tiles(karte_t *welt, spieler_t *, const koord3d &start, const koord3d &end)
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord pos;
	for(  pos.x = k1.x;  pos.x <= k2.x;  pos.x++  ) {
		for(  pos.y = k1.y;  pos.y <= k2.y;  pos.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( pos );

			zeiger_t *marker = new zeiger_t( welt, gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner1(grund_hang), corner1(weg_hang) ) + 3 * max( corner2(grund_hang), corner2(weg_hang) ) + 9 * max( corner3(grund_hang), corner3(weg_hang) ) + 27 * max( corner4(grund_hang), corner4(weg_hang) );
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_bild( grund_besch_t::marker->get_bild( grund_hang % 27 ) );
			marker->set_bild( grund_besch_t::marker->get_bild( back_hang ) );

			marker->mark_image_dirty( marker->get_bild(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *wkz_set_climate_t::do_work( karte_t *welt, spieler_t *, const koord3d &start, const koord3d &end )
{
	climate cl = (climate) atoi(default_param);
	koord k1, k2;
	if(  end == koord3d::invalid  ) {
		k1.x = k2.x = start.x;
		k1.y = k2.y = start.y;
	}
	else {
		k1.x = start.x < end.x ? start.x : end.x;
		k1.y = start.y < end.y ? start.y : end.y;
		k2.x = start.x + end.x - k1.x;
		k2.y = start.y + end.y - k1.y;
	}
	koord pos;
	for(  pos.x = k1.x;  pos.x <= k2.x;  pos.x++  ) {
		for(  pos.y = k1.y;  pos.y <= k2.y;  pos.y++  ) {
			grund_t *gr=welt->lookup_kartenboden(pos);
			if(  gr  ) {
				if(  cl != water_climate  ) {
					bool ok = true;
					if(  gr->ist_wasser()  ) {
						ok = welt->get_water_hgt(pos) == welt->lookup_hgt(pos)  &&  welt->is_plan_height_changeable( pos.x, pos.y );
						if(  ok  ) {
							gr->obj_loesche_alle( NULL );
							welt->set_water_hgt( pos, welt->lookup_hgt(pos) - 1 );
							welt->access(pos)->correct_water(welt);
						}
					}
					if(  ok  ) {
						welt->set_climate( pos, cl, true );
						reliefkarte_t::get_karte()->calc_map_pixel( gr->get_pos().get_2d() );
					}
				}
				else if(  !gr->ist_wasser()  &&  gr->get_grund_hang() == hang_t::flach  &&  welt->is_plan_height_changeable( pos.x, pos.y )  ) {
					bool ok = true;
					for(  int i = 0 ;  i < 8;  i++  ) {
						grund_t *gr2 = welt->lookup_kartenboden( pos + koord::neighbours[i] );
						if(  gr2  &&  ok  ) {
							ok = gr2->get_pos().z >= gr->get_pos().z;
						}
					}
					if(  ok  ) {
						gr->obj_loesche_alle( NULL );
						welt->set_water_hgt( pos, gr->get_pos().z );
						welt->access(pos)->correct_water(welt);
						welt->set_climate( pos, water_climate, true );
						reliefkarte_t::get_karte()->calc_map_pixel( gr->get_pos().get_2d() );
					}
				}

			}
		}
	}
	return NULL;
}


/* change water height
 * @author kieron
 */
bool wkz_change_water_height_t::init( karte_t *, spieler_t * )
{
	cursor = atoi(default_param) > 0 ? werkzeug_t::general_tool[WKZ_RAISE_LAND]->cursor : werkzeug_t::general_tool[WKZ_LOWER_LAND]->cursor;
	return true;
}


const char *wkz_change_water_height_t::work( karte_t *welt, spieler_t *, koord3d pos )
{
	if(  pos == koord3d::invalid  ) {
		return "Cannot alter water";
	}

	// calculate new height to use:
	bool raising = atoi(default_param) > 0;
	koord k = pos.get_2d();
	sint8 new_water_height;
	grund_t *gr = welt->lookup_kartenboden(k);

	if(  gr->ist_wasser()  ) {
		// if currently water, raise = +1, lower = -1
		new_water_height = gr->get_hoehe() + (raising ? 1 : -1);
	}
	// if not water then raise = set water height to ground height, lower = error
	else if(  raising  ) {
		hang_t::typ slope = gr->get_grund_hang();
		new_water_height = gr->get_hoehe() + max( max( corner1(slope), corner2(slope) ),max( corner3(slope), corner4(slope) ) );
	}
	else {
		return "Cannot alter water";
	}
	if(  new_water_height < welt->get_grundwasser() - 3  ) {
		return "Cannot alter water";
	}
	sint8 test_height = max( new_water_height, gr->get_hoehe() );

	// make a list of tiles to change
	// cannot use a recursive method as stack is not large enough!

	sint8 *from_dir = new sint8[welt->get_size().x * welt->get_size().y];
	sint8 *stage = new sint8[welt->get_size().x * welt->get_size().y];
	memset( from_dir, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
	memset( stage, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
#define array_koord(px,py) (px + py * welt->get_size().x)
	stage[array_koord(k.x,k.y)]=0;
	do {
		// firstly we must be able to change ground height
		bool ok = welt->is_plan_height_changeable( k.x, k.y )  &&  k.x > 0  &&  k.y > 0  &&  k.x < welt->get_size().x - 1  &&  k.y < welt->get_size().y - 1;
		const planquadrat_t *plan = welt->lookup(k);

		// next there cannot be any other grounds above this tile
		sint8 h = plan->get_kartenboden()->get_hoehe() + 1;
		while(  ok  &&  h < 32  ) {
			if(  plan->get_boden_in_hoehe(h)  ) {
				ok = false;
			}
			h++;
		}

		if(  !ok  ) {
			delete [] from_dir;
			delete [] stage;
			return "Cannot alter water";
		}
		for(  int i = stage[array_koord(k.x,k.y)];  i < 8;  i++  ) {
			koord k_neighbour = k + koord::neighbours[i];
			grund_t *gr2 = welt->lookup_kartenboden(k_neighbour);
			if(  gr2  ) {
				sint8 neighbour_height = gr2->get_hoehe();

				// move onto this tile if it hasn't been processed yet
				bool ok = stage[array_koord(k_neighbour.x, k_neighbour.y)] == -1  &&  neighbour_height <= test_height;

				if(  raising  ) {
					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) < new_water_height;
				}
				else {
					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) > new_water_height;
				}

				if(  ok  ) {
					//move on to next tile
					from_dir[array_koord(k_neighbour.x,k_neighbour.y)] = i;
					stage[array_koord(k_neighbour.x,k_neighbour.y)] = 0;
					stage[array_koord(k.x,k.y)] = i;
					k = k_neighbour;
					break;
				}
			}
			//return back to previous tile
			if(  i==7  ) {
				k = k - koord::neighbours[from_dir[array_koord(k.x,k.y)]];
			}
		}
	} while(  from_dir[array_koord(k.x,k.y)] != -1  );

	delete [] from_dir;

	// loop over map to find marked tiles
	for(  int y = 1;  y<welt->get_size().y - 1;  y++  ) {
		for(  int x = 1;  x<welt->get_size().x - 1;  x++  ) {
			if(  stage[array_koord(x,y)] > -1  ) {
				// calculate new height, slope and climate and set water height
				grund_t *gr2 =welt->lookup_kartenboden( koord( x, y ) );

				// remove any objects on this tile
				gr2->obj_loesche_alle( NULL );

				const sint8 h0 = gr2->get_hoehe();
				const sint8 min_grid_hgt = welt->min_hgt( koord( x, y ) );

				sint8 h0_nw, h0_ne, h0_se, h0_sw;

				// if min grid height here is less than ground height it will be because either we are partially or totally water
				if(  h0 > min_grid_hgt  ) {
					h0_nw = welt->lookup_hgt( koord( x, y ) );
					h0_ne = welt->lookup_hgt( koord( x + 1, y ) );
					h0_se = welt->lookup_hgt( koord( x + 1, y + 1 ) );
					h0_sw = welt->lookup_hgt( koord( x, y + 1 ) );
					if(  !gr2->ist_wasser()  ) {
						// partially water - while this appears to be a single height slope actually it is a double height slope half underwater
						const sint8 water_hgt = welt->get_water_hgt( koord( x, y ) );
						h0_nw >= water_hgt ? h0_nw = h0 + corner4( gr2->get_grund_hang() ) : 0;
						h0_ne >= water_hgt ? h0_ne = h0 + corner3( gr2->get_grund_hang() ) : 0;
						h0_se >= water_hgt ? h0_se = h0 + corner2( gr2->get_grund_hang() ) : 0;
						h0_sw >= water_hgt ? h0_sw = h0 + corner1( gr2->get_grund_hang() ) : 0;
					}
				}
				else {
					// fully land
					h0_nw = h0 + corner4( gr2->get_grund_hang() );
					h0_ne = h0 + corner3( gr2->get_grund_hang() );
					h0_se = h0 + corner2( gr2->get_grund_hang() );
					h0_sw = h0 + corner1( gr2->get_grund_hang() );
				}


				const sint8 hneu_nw = max( new_water_height, h0_nw );
				const sint8 hneu_ne = max( new_water_height, h0_ne );
				const sint8 hneu_se = max( new_water_height, h0_se );
				const sint8 hneu_sw = max( new_water_height, h0_sw );
				const sint8 hneu = min( min( hneu_nw, hneu_ne ), min( hneu_se, hneu_sw ) );

				gr2->set_hoehe( hneu );

				const uint8 sneu = (hneu_sw - hneu > 2 ? 2 : hneu_sw - hneu) + ((hneu_se - hneu > 2 ? 2 : hneu_se-hneu) * 3) + ((hneu_ne - hneu > 2 ? 2 : hneu_ne - hneu) * 9) + ((hneu_nw - hneu > 2 ? 2 : hneu_nw - hneu) * 27);
				gr2->set_grund_hang( sneu );

				welt->set_water_hgt( koord( x, y ), new_water_height );
				welt->access( koord( x, y ) )->correct_water( welt );
				welt->calc_climate( koord( x, y ), true );
			}
		}
	}

	delete [] stage;

	return NULL;
}


char const* wkz_plant_tree_t::move(karte_t* const welt, spieler_t* const sp, uint16 const b, koord3d const pos)
{
	if (b==0) {
		return NULL;
	}
	if (umgebung_t::networkmode) {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(sp, this, pos, welt->get_steps(), welt->get_map_counter(), false);
		network_send_server(nwc);
		return NULL;
	}
	else {
		return work( welt, sp, pos );
	}
}


const char *wkz_plant_tree_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		const baum_besch_t *besch = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL  ||  strlen(default_param)==0) {
			besch = baum_t::random_tree_for_climate( welt->get_climate( pos.get_2d() ) );
		}
		else {
			// parse default_param: bbbesch_nr b=1 ignore climate b=1 random age
			check_climates = default_param[0]=='0';
			random_age = default_param[1]=='1';
			besch = baum_t::find_tree(default_param+3);
		}
		if(besch  &&  baum_t::plant_tree_on_coordinate( welt, pos.get_2d(), besch, check_climates, random_age )  ) {
			spieler_t::book_construction_costs(sp, welt->get_settings().cst_remove_tree, pos.get_2d(), ignore_wt);
			return NULL;
		}
		return "";
	}
	return NULL;
}



/* the following routines add waypoints/halts to a schedule
 * because we do not like to stop at AIs stop, but we still want to force the truck to use AI roads
 * So if there is a halt, then it must be either public or ours!
 * @author prissi
 */
static const char *wkz_fahrplan_insert_aux(karte_t *welt, spieler_t *sp, koord3d pos, schedule_t *fpl, bool append)
{
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_insert_aux()","Schedule is (null), doing nothing");
		return 0;
	}
	grund_t *bd = welt->lookup(pos);
	if (bd) {
		// now just for error messages, we're assuming a valid ground
		// check for right way type
		if(!fpl->ist_halt_erlaubt(bd)) {
			return fpl->fehlermeldung();
		}
		// and check for ownership
		if(  !bd->is_halt()  ) {
			weg_t *w = bd->get_weg( fpl->get_waytype() );
			if(  w==NULL  &&  fpl->get_waytype()==tram_wt  ) {
				w = bd->get_weg( track_wt );
			}
			if(  w!=NULL  &&  w->get_besitzer()!=welt->get_spieler(1)  &&  !spieler_t::check_owner(w->get_besitzer(),sp)  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  bd->get_depot()  &&  !spieler_t::check_owner( bd->get_depot()->get_besitzer(), sp )  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
		if(  bd->is_halt()  &&  !spieler_t::check_owner( sp, bd->get_halt()->get_besitzer()) ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, now we have a valid ground
		if(append) {
			fpl->append(bd);
		}
		else {
			fpl->insert(bd);
		}
	}
	return NULL;
}

const char *wkz_fahrplan_add_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k, (schedule_t*)const_cast<char *>(default_param), true );
}

const char *wkz_fahrplan_ins_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k, (schedule_t*)const_cast<char *>(default_param), false );
}


/* way construction */
const weg_besch_t *wkz_wegebau_t::defaults[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
karte_t *wkz_wegebau_t::welt = NULL;	// for default city road

const weg_besch_t *wkz_wegebau_t::get_besch( uint16 timeline_year_month, bool remember ) const
{
	const weg_besch_t *besch = default_param ? wegbauer_t::get_besch(default_param,0) :NULL;
	if(  besch==NULL  &&  default_param  ) {
		waytype_t wt = (waytype_t)atoi(default_param);
		besch = defaults[wt&63];
		if(besch==NULL) {
			// search fastest way.
			besch = wegbauer_t::weg_search(wt, 0xffffffff, timeline_year_month, weg_t::type_flat);
		}
	}
	assert(besch);
	if(  remember  ) {
		if(  besch->get_styp() == weg_t::type_tram  ) {
			defaults[ tram_wt ] = besch;
		}
		else {
			defaults[besch->get_wtyp()&63] = besch;
		}
	}
	return besch;
}

image_id wkz_wegebau_t::get_icon(spieler_t *) const
{
	const weg_besch_t *besch = wegbauer_t::get_besch(default_param,0);
	const bool is_tram = besch ? (besch->get_wtyp()==tram_wt) || (besch->get_styp() == weg_t::type_tram) : false;
	return (grund_t::underground_mode==grund_t::ugm_all && !is_tram ) ? IMG_LEER : icon;
}

const char* wkz_wegebau_t::get_tooltip(const spieler_t *sp) const
{
	const weg_besch_t *besch = get_besch(sp->get_welt()->get_timeline_year_month(),false);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed() );
	return toolstr;
}

// default ways are not intialized synchronously for different clients
// always return the name of a way, never the string containing the waytype
const char* wkz_wegebau_t::get_default_param(spieler_t *sp) const
{
	if (sp==NULL) {
		return default_param;
	}
	if (besch) {
		return besch->get_name();
	}
	else {
		if (default_param == NULL) {
			// no chance to guess anything sensible
			return NULL;
		}
		const weg_besch_t* test_besch = get_besch(0, false);
		if (test_besch) {
			return test_besch->get_name();
		}
		else {
			return default_param;
		}
	}
}

bool wkz_wegebau_t::is_selected( const karte_t *welt ) const
{
	werkzeug_t const* const tool = welt->get_werkzeug(welt->get_active_player_nr());
	if (tool->get_id() != get_id()) {
		return false;
	}
	wkz_wegebau_t const* const selected = dynamic_cast<wkz_wegebau_t const*>(tool);
	return (selected  &&  selected->get_besch(welt->get_timeline_year_month(),false) == get_besch(welt->get_timeline_year_month(),false));
}

bool wkz_wegebau_t::init( karte_t *welt, spieler_t *sp )
{
	this->welt = welt;
	two_click_werkzeug_t::init( welt, sp );
	if( ok_sound == NO_SOUND ) {
		ok_sound = SFX_CASH;
	}

	// now get current besch
	besch = get_besch( welt->get_timeline_year_month(), is_local_execution() );
	if(  besch  &&  besch->get_cursor()->get_bild_nr(0) != IMG_LEER  ) {
		cursor = besch->get_cursor()->get_bild_nr(0);
	}
	if(  besch  &&  welt->get_timeline_year_month()  &&  sp!=NULL  &&  sp!=welt->get_spieler(1)  ) {
		if(  welt->get_timeline_year_month() < besch->get_intro_year_month()  ||  welt->get_timeline_year_month() >= besch->get_retire_year_month()  ) {
			// non available way => fail
			return false;
		}
	}
	return besch!=NULL;
}

waytype_t wkz_wegebau_t::get_waytype() const
{
	const weg_besch_t *besch = get_besch( spieler_t::get_welt()->get_timeline_year_month(), false );
	waytype_t wt = besch ? besch->get_wtyp() : invalid_wt;
	if (wt == track_wt  &&  besch->get_styp()==7) {
		wt = tram_wt;
	}
	return wt;
}

uint8 wkz_wegebau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	error = NULL;
	grund_t *gr=welt->lookup(pos);
	if(gr  &&  hang_t::ist_wegbar(gr->get_weg_hang())) {
		// ignore tunnel tiles (except road tunnel for tram track building ..)
		if(  gr->get_typ() == grund_t::tunnelboden  &&  !gr->ist_karten_boden()  && !(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7  && gr->hat_weg(road_wt)) ) {
			return 0;
		}
		bool const elevated = besch->get_styp() == 1  &&  besch->get_wtyp() != air_wt;
		// ignore water
		if(  besch->get_wtyp() != water_wt  &&  gr->get_typ() == grund_t::wasser  ) {
			if(  !elevated  ||  welt->lookup_hgt( gr->get_pos().get_2d() ) < welt->get_water_hgt( pos.get_2d() )  ) {
				return 0;
			}
			// here either channel or elevated way over not too deep water
		}
		// elevated ways have to check tile above
		if(  elevated  ) {
			gr = welt->lookup( pos + koord3d( 0, 0, umgebung_t::pak_height_conversion_factor ) );
			if(  gr == NULL  ) {
				return 2;
			}
			if(  gr->get_typ() != grund_t::monorailboden  ) {
				return 0;
			}
		}
		// test if way already exists on the way and if we are allowed to connect
		weg_t *way = gr->get_weg(besch->get_wtyp());
		if(  way  ) {
			// allow to connect to any road
			if(  besch->get_wtyp() == road_wt  ||  besch->get_wtyp() == water_wt  ) {
				return 2;
			}
			error = way->ist_entfernbar(sp);
			return error==NULL ? 2 : 0;
		}
		// check for ownership but ignore moving things
		if(sp!=NULL) {
			for(uint8 i=0; i<gr->obj_count(); i++) {
				ding_t* dt = gr->obj_bei(i);
				if (!dt->is_moving()  &&  dt->ist_entfernbar(sp)!=NULL) {
					error =  dt->ist_entfernbar(sp); // "Das Feld gehoert\neinem anderen Spieler\n";
					return 0;
				}
			}
		}
	}
	else {
		return 0;
	}
	return 2;
}

void wkz_wegebau_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end )
{
	// recalc type of construction
	wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->get_wtyp();
	if(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7) {
		bautyp = wegbauer_t::schiene_tram;
	}
	// elevated track?
	if(besch->get_styp()==1  &&  besch->get_wtyp()!=air_wt) {
		bautyp |= wegbauer_t::elevated_flag;
	}

	bauigel.route_fuer(bautyp, besch);
	if(  is_ctrl_pressed()  ) {
		bauigel.set_keep_existing_ways( false );
	}
	else {
		bauigel.set_keep_existing_faster_ways( true );
	}
	if(  is_ctrl_pressed()  ||  (umgebung_t::straight_way_without_control  &&  !umgebung_t::networkmode)  ) {
		DBG_MESSAGE("wkz_wegebau()", "try straight route");
		bauigel.calc_straight_route(start,end);
	}
	else {
		bauigel.calc_route(start,end);
	}
	DBG_MESSAGE("wkz_wegebau()", "builder found route with %d squares length.", bauigel.get_count());
}

const char *wkz_wegebau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		return NULL;
	}
	return "";
}

void wkz_wegebau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end );

	uint8 offset = (besch->get_styp() == 1  &&  besch->get_wtyp() != air_wt) ? umgebung_t::pak_height_conversion_factor : 0;

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++   ) {
			koord3d pos = bauigel.get_route()[j] + koord3d(0,0,offset);
			grund_t *gr = welt->lookup( pos );
			if( !gr ) {
				gr = new monorailboden_t(welt, pos, 0);
				// should only be here when elevated/monorail, therefore will be at height offset above ground
				gr->set_grund_hang( welt->lookup( pos - koord3d( 0, 0, offset ) )->get_grund_hang() );
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(besch->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t( welt, pos, NULL );
			if(gr->get_weg_hang()) {
				way->set_bild( besch->get_hang_bild_nr(gr->get_weg_hang(),0) );
			}
			else if(besch->get_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  besch->has_diagonal_bild()) {
				way->set_bild( besch->get_diagonal_bild_nr(zeige,0) );
			}
			else {
				way->set_bild( besch->get_bild_nr(zeige,0) );
			}
			gr->obj_add( way );
			way->set_yoff(-gr->get_weg_yoff() );
			marked.insert( way );
			way->mark_image_dirty( way->get_bild(), 0 );
		}
	}
}


/* city road construction */
const weg_besch_t *wkz_build_cityroad::get_besch(uint16,bool) const
{
	return welt->get_city_road();
}

const char *wkz_build_cityroad::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	bauigel.set_build_sidewalk(true);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		return NULL;
	}
	return "";
}


/* bridge construction */
const char* wkz_brueckenbau_t::get_tooltip(const spieler_t *sp) const
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	if(besch->get_max_length()>0) {
		n += sprintf(toolstr+n, ", %dkm", besch->get_max_length());
	}
	return toolstr;
}

waytype_t wkz_brueckenbau_t::get_waytype() const
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	return besch ? besch->get_waytype() : invalid_wt;
}

const char *wkz_brueckenbau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	if (end==koord3d::invalid) {
		return brueckenbauer_t::baue( welt, sp, start.get_2d(), besch );
	}
	else {
		const koord zv(ribi_typ(end-start));
		brueckenbauer_t::baue_bruecke( welt, sp, start, end, zv, besch, wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat));
		return NULL; // all checks are performed before building.
	}
}

void wkz_brueckenbau_t::rdwr_custom_data(uint8 player_nr, memory_rw_t *packet)
{
	two_click_werkzeug_t::rdwr_custom_data(player_nr, packet);
	uint8 i = ribi;
	packet->rdwr_byte(i);
	ribi = (ribi_t::ribi)i;
}

void wkz_brueckenbau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	const ribi_t::ribi ribi_mark = ribi_typ(end-start);
	const koord zv(ribi_mark);
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	sint64 costs = 0;
	// start
	grund_t *gr = welt->lookup(start);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const hang_t::typ slope = gr->get_grund_hang();
	const uint8 max_height = slope ? ((slope & 7) ? 1 : 2) : (besch->has_double_ramp()?2:1);

	zeiger_t *way = new zeiger_t( welt, start, sp );
	const bruecke_besch_t::img_t img0 = besch->get_end( slope, slope, hang_typ(zv)*max_height );

	gr->obj_add( way );
	way->set_bild( besch->get_hintergrund( img0, 0 ) );
	way->set_after_bild( besch->get_vordergrund( img0, 0 ) );

	if(  gr->get_grund_hang() != 0  ) {
		way->set_yoff( -TILE_HEIGHT_STEP * max_height );
	}
	// eventually we have to remove trees on start tile
	if (besch->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			ding_t *dt = gr->obj_bei(i);
			switch(dt->get_typ()) {
				case ding_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case ding_t::groundobj:
					costs += ((groundobj_t *)dt)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	marked.insert( way );
	way->mark_image_dirty( way->get_bild(), 0 );
	// loop
	koord3d pos( start + zv + koord3d( 0, 0, max_height ) );
	while (pos.get_2d()!=end.get_2d()) {
		grund_t *gr = welt->lookup( pos );
		if( !gr ) {
			gr = new monorailboden_t(welt, pos, 0);
			gr->set_grund_hang( 0 );
			welt->access(pos.get_2d())->boden_hinzufuegen(gr);
		}
		zeiger_t *way = new zeiger_t( welt, pos, sp );
		gr->obj_add( way );
		grund_t *kb = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - kb->get_pos().z;
		way->set_bild(besch->get_hintergrund(besch->get_simple(ribi_mark,height-hang_t::height(kb->get_grund_hang())),0));
		way->set_after_bild(besch->get_vordergrund(besch->get_simple(ribi_mark,height-hang_t::height(kb->get_grund_hang())), 0));
		marked.insert( way );
		way->mark_image_dirty( way->get_bild(), 0 );
		pos = pos + zv;
	}
	costs += besch->get_preis() * koord_distance(start, pos);
	// end
	gr = welt->lookup(end);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const hang_t::typ end_slope = gr->get_grund_hang();
	const uint8 end_max_height = end_slope ? ((end_slope & 7) ? 1 : 2) : (pos.z-end.z);

	if(  gr->ist_karten_boden()  &&  end.z + end_max_height == start.z + max_height  ) {
		zeiger_t *way = new zeiger_t( welt, end, sp );
		const bruecke_besch_t::img_t img1 = besch->get_end( end_slope, end_slope, end_slope?0:(pos.z-end.z)*hang_typ(-zv) );
		gr->obj_add( way );
		way->set_bild(besch->get_hintergrund(img1, 0));
		way->set_after_bild(besch->get_vordergrund(img1, 0));
		if(  gr->get_grund_hang() != 0  ) {
			way->set_yoff( -TILE_HEIGHT_STEP * end_max_height );
		}
		marked.insert( way );
		way->mark_image_dirty( way->get_bild(), 0 );
		costs += besch->get_preis();
	}
	else {
		if (besch->get_waytype() == powerline_wt  ? !gr->find<leitung_t>() : !gr->hat_weg(besch->get_waytype())) {
			const weg_besch_t *weg_besch = wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat);
			costs += weg_besch->get_preis();
		}
	}
	// eventually we have to remove trees on end tile
	if (besch->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			ding_t *dt = gr->obj_bei(i);
			switch(dt->get_typ()) {
				case ding_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case ding_t::groundobj:
					costs += ((groundobj_t *)dt)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs ) );
}

uint8 wkz_brueckenbau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d &start )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	const waytype_t wt = besch->get_waytype();

	error = NULL;
	grund_t *gr = welt->lookup(pos);
	if (gr==NULL  || !brueckenbauer_t::ist_ende_ok(sp,gr) || !hang_t::ist_wegbar(gr->get_grund_hang()) ) {
		return 0;
	}

	if(  welt->lookup( pos + koord3d(0, 0, 1))  ||  (umgebung_t::pak_height_conversion_factor == 2  &&  welt->lookup( pos + koord3d(0, 0, 2) ))  ) {
		return 0;
	}

	if (is_first_click()) {
		// first click
		if (!gr->ist_karten_boden()) {
			return 0;
		}
		ribi_t::ribi rw = ribi_t::keine;
		if (wt==powerline_wt) {
			if (gr->hat_wege()) {
				return 0;
			}
			if (gr->find<leitung_t>()) {
				rw |= gr->find<leitung_t>()->get_ribi();
			}
		}
		else {
			if (gr->find<leitung_t>()) {
				return 0;
			}
			if(wt!=road_wt) {
			// only road bridges can have other ways on it (ie trams)
				if(gr->has_two_ways()  ||  (gr->hat_wege() && gr->get_weg_nr(0)->get_waytype()!=wt) ) {
					return 0;
				}
				if(gr->hat_wege()){
					rw |= gr->get_weg_nr(0)->get_ribi_unmasked();
				}
			}
			else {
				// If road and tram, we have to check both ribis.
				for(int i=0;i<2;i++) {
					const weg_t *w = gr->get_weg_nr(i);
					if (w) {
						if (w->get_waytype()!=road_wt  &&  (w->get_waytype()!=track_wt  ||  w->get_besch()->get_styp()!=tram_wt)) {
							return 0;
						}
						rw |= w->get_ribi_unmasked();
					}
					else break;
				}
			}
		}
		// ribi from slope
		rw |= ribi_typ(gr->get_grund_hang());
		if (rw!=ribi_t::keine && !ribi_t::ist_einfach(rw)) {
			return 0;
		}
		// determine possible directions
		ribi = ribi_t::rueckwaerts(rw);
		return (ribi!=ribi_t::keine ? 2 : 0) | (ribi_t::ist_einfach(ribi) ? 1 : 0);
	}
	else {
		// second click

		// get initial height of bridge from start tile
		// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
		// single height -> height is 1
		// double height -> height is 2
		const hang_t::typ slope = gr->get_grund_hang();
		const uint8 max_height = slope ? ((slope & 7) ? 1 : 2) : umgebung_t::pak_height_conversion_factor;
		const hang_t::typ start_slope = welt->lookup(start)->get_grund_hang();
		const uint8 start_max_height = start_slope ? ((start_slope & 7) ? 1 : 2) : umgebung_t::pak_height_conversion_factor;
		const uint8 height_difference = abs( start.z + start_max_height - pos.z - max_height );
		if(  height_difference>1  &&  besch->get_hintergrund(bruecke_besch_t::N_Start2, 0) == IMG_LEER  ) {
			return 0;
		}

		// dragging in the right direction?
		ribi_t::ribi test = ribi_typ(pos - start);
		if (!ribi_t::ist_einfach(test)  ||  ((test & (~ribi))!=0) ) {
			return 0;
		}

		// check whether we can build a bridge here
		const char *error = NULL;
		koord3d end = brueckenbauer_t::finde_ende(welt, sp, start, koord(test), besch, error, false, koord_distance(start, pos));
		if (end!=pos) {
			return 0;
		}
		return 2;
	}
}


/* more difficult, since this builds also underground ways */
const char* wkz_tunnelbau_t::get_tooltip(const spieler_t *sp) const
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	return toolstr;
}

waytype_t wkz_tunnelbau_t::get_waytype() const
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	return besch ? besch->get_waytype() : invalid_wt;
}

const char *wkz_tunnelbau_t::check_pos( karte_t *welt, spieler_t *sp, koord3d pos)
{
	if (grund_t::underground_mode == grund_t::ugm_all) {
		return NULL;
	}
	else {
		return two_click_werkzeug_t::check_pos(welt, sp, pos);
	}
}

void wkz_tunnelbau_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end, karte_t *)
{
	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	wegbauer_t::bautyp_t bt = (wegbauer_t::bautyp_t)(besch->get_waytype());

	const weg_besch_t *wb = besch->get_weg_besch();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), 0, weg_t::type_flat );
	}

	bauigel.route_fuer(bt | wegbauer_t::tunnel_flag, wb, besch);
	bauigel.set_keep_existing_faster_ways( !is_ctrl_pressed() );
	// wegbauer tries to find route to 3d coordinate if no ground at end exists or is not kartenboden
	bauigel.calc_straight_route(start,end);
}

const char *wkz_tunnelbau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	if( end == koord3d::invalid ) {
		// Build tunnel mouths
		if (welt->lookup_kartenboden(start.get_2d())->get_hoehe() == start.z) {
			const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
			return tunnelbauer_t::baue( welt, sp, start.get_2d(), besch, !is_ctrl_pressed() );
		}
		else {
			return "";
		}
	}
	else {
		// Build tunnels
		wegbauer_t bauigel(welt, sp);
		calc_route( bauigel, start, end, welt );
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);
		welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);
		return NULL;
	}
}

uint8 wkz_tunnelbau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	if(  !is_first_click()  ) {
		error = NULL;
		// All pos are valid for the second click!
		return 2;
	}
	// search for ground
	// start needs valid tile!
	grund_t *gr = welt->lookup(pos);
	if(  gr  ) {
		if( gr->hat_wege() ) {
			const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
			// use the check_owner routine of wegbauer_t (not spieler_t!), needs an instance
			weg_t *w = gr->get_weg_nr(0);
			if(  w==NULL  ||  w->get_besch()->get_wtyp()!=besch->get_waytype()  ) {
				error = "No suitable ground!";
				return 0;
			}
			wegbauer_t bauigel(welt, sp);
			if(!bauigel.check_owner( w->get_besitzer(), sp )) {
				error = "Das Feld gehoert\neinem anderen Spieler\n";
				return 0;
			}
		}
	}
	else {
		error = "No suitable ground!";
		return 0;
	}
	// if starting tile is tunnel .. build underground tracks
	error = NULL;
	if(gr->ist_tunnel()) {
		return 2;
	}
	// .. otherwise build tunnel mouths (and tunnel behind)
	else {
		return 1;
	}
}

void wkz_tunnelbau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end, welt );

	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	// now we search a matching way for the tunnels top speed
	const weg_besch_t *wb = besch->get_weg_besch();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), 0, weg_t::type_flat );
	}

	welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++  ) {
			koord3d pos = bauigel.get_route()[j];
			grund_t *gr = welt->lookup(pos);
			if( !gr ) {
				// We need to create a dummy ground.
				gr = new tunnelboden_t(welt, pos, 0);
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(wb->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t( welt, pos, sp );
			if(gr->get_weg_hang()) {
				way->set_bild( wb->get_hang_bild_nr(gr->get_weg_hang(),0) );
			}
			else if(wb->get_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  wb->has_diagonal_bild()) {
				way->set_bild( wb->get_diagonal_bild_nr(zeige,0) );
			}
			else {
				way->set_bild( wb->get_bild_nr(zeige,0) );
			}
			gr->obj_add( way );
			marked.insert( way );
			way->mark_image_dirty( way->get_bild(), 0 );
		}
		welt->lookup(end)->set_flag(grund_t::marked);
	}
}


/* removes a way like a driving car ... */
char const* wkz_wayremover_t::get_tooltip(spieler_t const*) const
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

image_id wkz_wayremover_t::get_icon(spieler_t *sp) const
{
	if(  default_param  &&  wegbauer_t::waytype_available( (waytype_t)atoi(default_param), sp->get_welt()->get_timeline_year_month() )  ) {
		return icon;
	}
	return IMG_LEER;
}

waytype_t wkz_wayremover_t::get_waytype() const
{
	return default_param ? (waytype_t)atoi(default_param) : invalid_wt;
}

class electron_t : public fahrer_t {
	bool ist_befahrbar(const grund_t* gr) const { return gr->get_leitung()!=NULL; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_leitung()->get_ribi(); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_kosten(const grund_t *, const sint32, koord) const { return 1; }
	virtual bool ist_ziel(const grund_t *,const grund_t *) const { return false; }
};

class scenario_checker_t : public fahrer_t {
public:
	fahrer_t *other;
	scenario_t *scenario;
	uint16 id;
	spieler_t *sp;
	~scenario_checker_t() { delete other; }

	/**
	 * checks for active scenario,
	 * @returns scenario_checker_t if scenario active, the supplied test_driver otherwise
	 */
	static fahrer_t* apply(fahrer_t *test_driver, spieler_t *sp, werkzeug_t *wkz) {
		karte_t *welt = sp->get_welt();
		if (is_scenario()) {
			scenario_checker_t *td2 = new scenario_checker_t();
			td2->other = test_driver;
			td2->scenario = welt->get_scenario();
			td2->id = wkz->get_id();
			td2->sp = sp;
			return td2;
		}
		return test_driver;
	}
private:
	bool ist_befahrbar(const grund_t* gr) const { return other->ist_befahrbar(gr)  &&  scenario->is_work_allowed_here(sp, id, other->get_waytype(), gr->get_pos())==NULL;}
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return other->get_ribi(gr); }
	virtual waytype_t get_waytype() const { return other->get_waytype(); }
	virtual int get_kosten(const grund_t *gr, const sint32 c, koord p) const { return other->get_kosten(gr,c,p); }
	virtual bool ist_ziel(const grund_t *gr,const grund_t *gr2) const { return other->ist_ziel(gr,gr2); }
};

void wkz_wayremover_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
	if( can_built ) {
		FOR(vector_tpl<koord3d>, const& pos, verbindung.get_route()) {
			zeiger_t *marker = new zeiger_t( welt, pos, NULL );
			marker->set_bild( cursor );
			marker->mark_image_dirty( marker->get_bild(), 0 );
			marked.insert( marker );
			welt->lookup(pos)->obj_add( marker );
		}
	}
}

uint8 wkz_wayremover_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	// search for starting ground
	waytype_t wt = (waytype_t)atoi(default_param);
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos,wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return 0;
	}
	// do not remove ground from depot
	if(gr->get_depot()) {
		error = "No suitable ground!";
		return 0;
	}
	if(is_scenario()) {
		error = welt->get_scenario()->is_work_allowed_here(sp, get_id(), wt, pos);
		if (error) {
			dbg->warning("wkz_wayremover_t::is_within_limits()", error);
			return 0;
		}
	}
	error = NULL;
	return 2;
}

bool wkz_wayremover_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	waytype_t wt = (waytype_t)atoi(default_param);
	if (wt == tram_wt) {
		wt = track_wt;
	}
	karte_t *welt = sp->get_welt();

	if(  start == end  ) {
		verbindung.clear();
		grund_t *gr=welt->lookup(start);
		if(  gr  &&  (wt!=powerline_wt ? gr->get_weg(wt)!=NULL : gr->get_leitung()!=NULL) ) {
			verbindung.append( start );
		}
	}
	else {
		// get a default vehikel
		fahrer_t* test_driver;
		if(  wt!=powerline_wt  ) {
			vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
			vehikel_t *driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
			driver->set_flag( ding_t::not_on_map );
			test_driver = driver;
		}
		else {
			test_driver = new electron_t();
		}
		test_driver = scenario_checker_t::apply(test_driver, sp, this);

		verbindung.calc_route(sp->get_welt(), start, end, test_driver, 0, 0);
		delete test_driver;
	}
	DBG_MESSAGE("wkz_wayremover()","route with %d tile found",verbindung.get_count());

	calc_route_error = NULL;
	bool can_delete = start == end  ||  verbindung.get_count()>1;
	if(  can_delete  ) {
		// found a route => check if I can delete anything on it
		FOR(koord3d_vector_t, const& i, verbindung.get_route()) {
			if (!can_delete) break;
			grund_t const* const gr = welt->lookup(i);
			if(  wt!=powerline_wt  ) {
				// no way found
				if(  gr==NULL  ||  gr->get_weg(wt)==NULL  ) {
					can_delete = false;
					break;
				}
				// check all if we want to delete the first on a no-ground tile
				bool check_all = !gr->ist_karten_boden()  &&  gr->has_two_ways()  &&  gr->get_weg_nr(0)->get_waytype()==wt;
				// we have to do a fine check
				for( uint i=0;  i<gr->get_top()  &&  can_delete;  i++  ) {
					ding_t *d = gr->obj_bei(i);
					const uint8 type = d->get_typ();
					// ignore pillars, powerlines
					if (type == ding_t::pillar  ||  type==ding_t::leitung) {
						continue;
					}
					// ignore flying aircraft
					if (type == ding_t::aircraft  &&  !(static_cast<aircraft_t*>(d)->is_on_ground())) {
						continue;
					}
					const waytype_t ding_wt = d->get_waytype();
					// way-related things
					if (ding_wt != invalid_wt) {
						// check this thing if it has the same waytype or if we want to remove the whole bridge/tunnel tile
						// special case: stations - take care not to produce station without any way
						const bool lonely_station = type==ding_t::gebaeude  &&  !gr->has_two_ways();
						if (check_all ||  ding_wt == wt  ||  lonely_station) {
							can_delete = (calc_route_error = d->ist_entfernbar(sp)) == NULL;
						}
					}
					// all other stuff
					else {
						can_delete = (calc_route_error = d->ist_entfernbar(sp)) == NULL;
					}
				}
			}
			else {
				// for powerline: only a ground and a powerline to remove
				if(  gr==NULL  ||  gr->get_leitung()==NULL  ||  (calc_route_error = gr->get_leitung()->ist_entfernbar(sp))!=NULL  ) {
					can_delete = false;
					break;
				}
			}
		}
	}
	DBG_MESSAGE("wkz_wayremover()", "route search returned %d", can_delete);

	return can_delete;
}

const char *wkz_wayremover_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	waytype_t wt = (waytype_t)atoi(default_param);
	route_t verbindung;
	if( !calc_route( verbindung, sp, start, end )  ) {
		DBG_MESSAGE("wkz_wayremover()","no route found");
		if (calc_route_error  &&  *calc_route_error) {
			return calc_route_error;
		}
		else {
			return "Ways not connected";
		}
	}
	bool can_delete = true;	// assume success

	// if successful => delete everything
	for( uint32 i=0;  i<verbindung.get_count();  i++  ) {

		grund_t *gr=welt->lookup(verbindung.position_bei(i));

		// ground can be missing after deleting a bridge ...
		if(gr  &&  !gr->ist_wasser()) {

			if(gr->ist_bruecke()) {
				if(gr->find<bruecke_t>()->get_besch()->get_waytype()==wt) {
					if(gr->ist_karten_boden()) {
						const char *err = NULL;
						err = brueckenbauer_t::remove(welt,sp,verbindung.position_bei(i),wt);
						if(err) {
							return err;
						}
						gr = welt->lookup(verbindung.position_bei(i));
					}
					else {
						// do not remove asphalt from a bridge ...
						continue;
					}
				}
			}

			// now the tricky part: delete just part of a way (or everything, if possible)
			// calculate remaining directions
			ribi_t::ribi rem = 15 ^ ( verbindung.get_route().get_ribi(i) );
			// if start=end tile then delete every direction
			if(  verbindung.get_count() <= 1  ) {
				rem = 0;
			}

			if(  wt!=powerline_wt  ) {
				if(!gr->get_flag(grund_t::is_kartenboden)  &&  (gr->get_typ()==grund_t::tunnelboden  ||  gr->get_typ()==grund_t::monorailboden)  &&  gr->get_weg_nr(0)->get_waytype()==wt) {
					can_delete &= gr->remove_everything_from_way(sp,wt,rem);
					if(can_delete  &&  gr->get_weg(wt)==NULL) {
						if(gr->get_weg_nr(0)!=0) {
							gr->remove_everything_from_way(sp,gr->get_weg_nr(0)->get_waytype(),ribi_t::keine);
						}
						gr->obj_loesche_alle(sp);
						gr->mark_image_dirty();
						if (gr->is_visible() && gr->get_typ()==grund_t::tunnelboden && i>0) { // visibility test does not influence execution
							grund_t *bd = welt->access(verbindung.position_bei(i-1).get_2d())->get_kartenboden();
							bd->calc_bild();
							bd->set_flag(grund_t::dirty);
						}
						// delete tunnel ground too, if empty
						welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
						delete gr;
					}
				}
				else {
					can_delete &= gr->remove_everything_from_way(sp,wt,rem);
				}
			}
			else {
				leitung_t *lt = gr->get_leitung();
				if(  lt  &&  (rem&lt->get_ribi())==0  ) {
					// remove only single connections
					lt->entferne(sp);
					delete lt;
					// delete tunnel ground too, if empty
					if (gr->get_typ()==grund_t::tunnelboden) {
						gr->obj_loesche_alle(sp);
						gr->mark_image_dirty();
						if (!gr->get_flag(grund_t::is_kartenboden)) {
							welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
							delete gr;
						}
						else {
							grund_t *gr_new = new boden_t(welt, gr->get_pos(), gr->get_grund_hang());
							welt->access(gr->get_pos().get_2d())->boden_ersetzen(gr, gr_new);
							gr_new->calc_bild();
						}
					}
				}
				// otherwise it is a crossing ...
			}
		}
		// ok, now everything removed ...
	}

	// return success
	return can_delete ? NULL : "";
}



/* add catenary during construction */
const way_obj_besch_t *wkz_wayobj_t::default_electric = NULL;

const char* wkz_wayobj_t::get_tooltip(const spieler_t *sp) const
{
	if(  build  ) {
		const way_obj_besch_t *besch = get_besch(sp->get_welt());
		if(besch) {
			tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
			size_t n= strlen(toolstr);
			if (besch->get_own_wtyp()==overheadlines_wt) {
				// only overheadlines impose topspeed
				sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
			}
			return toolstr;
		}
		return NULL;
	}
	else {
		waytype_t wt = (waytype_t)atoi( default_param );
		sprintf( toolstr, translator::translate("Remove wayobj %s"), translator::translate(weg_t::waytype_to_string(wt)) );
		return toolstr;
	}
}

const way_obj_besch_t *wkz_wayobj_t::get_besch( const karte_t* welt ) const
{
	const way_obj_besch_t *besch = default_param ? wayobj_t::find_besch(default_param) : NULL;
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = wayobj_t::wayobj_search( track_wt, overheadlines_wt, welt->get_timeline_year_month() );
		}
	}
	return besch;
}

waytype_t wkz_wayobj_t::get_waytype() const
{
	if(  build  ) {
		const way_obj_besch_t *besch = get_besch(spieler_t::get_welt());
		return besch ? besch->get_wtyp() : invalid_wt;
	}
	else {
		return default_param ? (waytype_t)atoi( default_param ) : invalid_wt;
	}
}

bool wkz_wayobj_t::is_selected( const karte_t *welt ) const
{
	const wkz_wayobj_t *selected = dynamic_cast<const wkz_wayobj_t *>(welt->get_werkzeug(welt->get_active_player_nr()));
	return (selected  &&  selected->build==build  &&  selected->get_besch(welt) == get_besch(welt));
}

bool wkz_wayobj_t::init( karte_t *welt, spieler_t *sp )
{
	two_click_werkzeug_t::init( welt, sp );

	if( build ) {
		besch = get_besch(welt);
		if( besch ) {
			cursor = besch->get_cursor()->get_bild_nr(0);
			wt = besch->get_wtyp();
			default_electric = besch;
		}
		return besch!=NULL;
	}
	else {
		besch = NULL;
		wt = (waytype_t)atoi( default_param );
		return wt != 0;
	}
}

bool wkz_wayobj_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t remover_besch( wt, 500, vehikel_besch_t::diesel );
	vehikel_t* test_vehicle = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
	test_vehicle->set_flag( ding_t::not_on_map );
	fahrer_t* test_driver = scenario_checker_t::apply(test_vehicle, sp, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(sp->get_welt(), start, to, test_driver, 0, 0);
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

uint8 wkz_wayobj_t::is_valid_pos( karte_t * welt, spieler_t * sp, const koord3d& pos, const char *&error, const koord3d & )
{
	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp, welt, pos, wt );
	if(  gr == NULL  ) {
		DBG_MESSAGE("wkz_wayobj_t::is_within_limits()", "no ground on %s",pos.get_str());
		// wrong ground or not this way here => exit
		return 0;
	}
	error = NULL;
	return 2;
}

void wkz_wayobj_t::mark_tiles( karte_t * welt, spieler_t * sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
	if( can_built ) {
		sint32 cost_estimate = 0;

		for( uint32 j = 0; j < verbindung.get_count(); j++ ) {
			koord3d pos = verbindung.position_bei(j);
			grund_t *gr = welt->lookup(pos);

			ribi_t::ribi show = verbindung.get_route().get_ribi(j);
			// Search a matching catenary on gr.
			wayobj_t *wayobj = gr->get_wayobj( wt );
			if( build ) {
				cost_estimate += besch->get_preis();
				if( wayobj ) {
					show = show | wayobj->get_dir();
					// Already a catenary here -> costs only, if new catenary is faster
					if(  wayobj->get_besch()->get_topspeed() >= besch->get_topspeed()  ) {
						cost_estimate -= besch->get_preis();
					}
				}
			}
			else if( wayobj ) {
				cost_estimate += wayobj->get_besch()->get_preis();
			}

			zeiger_t *way_obj = NULL;
			if( build ) {
				way_obj = new zeiger_t( welt, pos, NULL );
				if(  gr->get_weg_hang()  ) {
					way_obj->set_after_bild( besch->get_front_slope_image_id(gr->get_weg_hang()) );
					way_obj->set_bild( besch->get_back_slope_image_id(gr->get_weg_hang()) );
				}
				else if(  ribi_t::ist_kurve(show)  &&  besch->has_diagonal_bild()  ) {
					way_obj->set_after_bild( besch->get_front_diagonal_image_id(show) );
					way_obj->set_bild( besch->get_back_diagonal_image_id(show) );
				}
				else {
					way_obj->set_after_bild( besch->get_front_image_id(show) );
					way_obj->set_bild( besch->get_back_image_id(show) );
				}
			}
			else {
				if( gr->get_wayobj( wt ) ) {
					way_obj = new zeiger_t( welt, pos, NULL );
					way_obj->set_bild( cursor ); //skinverwaltung_t::bauigelsymbol->get_bild_nr(0));
				}
			}
			if( way_obj ) {
				way_obj->mark_image_dirty( way_obj->get_bild(), 0 );
				gr->obj_add( way_obj );
				marked.insert( way_obj );
			}
		}
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -cost_estimate ) );
	}
}

const char *wkz_wayobj_t::do_work( karte_t * welt, spieler_t * sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
	DBG_MESSAGE("wkz_wayobj_t::work()","route search returned %d",can_built);

	if(!can_built) {
		return "Ways not connected";
	}

	// built wayobj ...
	koord3d_vector_t const& r = verbindung.get_route();
	for(uint32 i=0;  i<verbindung.get_count();  i++  ) {
		if( build ) {
			wayobj_t::extend_wayobj_t(welt, r[i], sp, r.get_ribi(i), besch);
		}
		else {
			if (wayobj_t* const wo = welt->lookup(r[i])->find<wayobj_t>()) {
				const char *err = wo->ist_entfernbar( sp );
				if( !err ) {
					wo->entferne( sp );
					delete wo;
				}
			}
		}
	}

	// Update depots (new electric tab?). Depots can only be on first and last tile.
	for(  uint8 j = 0;  j < 2;  j++  ) {
		uint8 i = j==0 ? 0 : verbindung.get_count()-1;
		depot_t *dep = welt->lookup( verbindung.position_bei(i) )->get_depot();
		if( dep ) {
			dep->update_win();
		}
	}

	return NULL;
}


/* build all kind of station extension buildings */
const char *wkz_station_t::wkz_station_building_aux(karte_t *welt, spieler_t *sp, bool extend_public_halt, koord3d k, const haus_besch_t *besch, sint8 rotation )
{
	// need kartenboden
	if (welt->lookup_kartenboden(k.get_2d())->get_hoehe() != k.z) {
		return "";
	}
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);

	// Player sp pays for the construction
	// but we try to extend stations of Player new_owner that may be the public player
	spieler_t *new_owner = extend_public_halt ? welt->get_spieler(1) : sp;

	koord offsets;
	halthandle_t halt;
	const char *msg = "Tile not empty.";

	if(  rotation==-1  ) {
		//no predefined rotation

		int best_halt = 0;
		int any_halt = 0;

		// find valid rotations (since halt extensions are symmetric, we need to check only two)
		bool any_ok = false;
		for( int r=0;  r<2;  r++  ) {
			koord testsize = besch->get_groesse(r);
			for(  sint8 j=3;  j>=0;  j-- ) {
				bool ok = true;
				koord offset(((j&1)^1)*(testsize.x-1),((j>>1)&1)*(testsize.y-1));
				if(welt->square_is_free(pos-offset, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())) {
					// first we must check over/under halt
					halthandle_t last_halt;
					for(  sint16 x=0;  x<testsize.x;  x++  ) {
						for(  sint16 y=0;  y<testsize.y;  y++  ) {
							const planquadrat_t *pl = welt->lookup( pos-offset+koord(x,y) );
							halthandle_t test_halt = pl->get_halt();
							if(test_halt.is_bound()) {
								if(!spieler_t::check_owner( new_owner, test_halt->get_besitzer())) {
									// there is another player's halt
									ok = false;
									msg = "Das Feld gehoert\neinem anderen Spieler\n";
								}
								else if(!last_halt.is_bound()) {
									last_halt = test_halt;
								}
								else if(last_halt != test_halt) {
									// there are several halts
									ok = false;
									msg = "Several halts found.";
								}
							}
						}
					}
					if(!ok) {
						continue;
					}
					// well, at least this is theoretical possible here
					any_ok = true;
					if(rotation==-1) {
						// we can build it. reserve this one
						// This needs to build a building at under/over a halt.
						rotation = r;
						offsets = offset;
					}
					koord test_start = pos-offset;
					// find all surrounding tiles with a stop
					int neighbour_halt_n = 0, neighbour_halt_s = 0, neighbour_halt_e = 0, neighbour_halt_w = 0;
					int best_halt_n = 0, best_halt_s = 0, best_halt_e = 0, best_halt_w = 0;
					// test also diagonal corners (that is why from -1 to size!)
					for(  sint16 y=-1;  y<=testsize.y;  y++  ) {
						// left (for all tiles, even bridges)
						const planquadrat_t *pl = welt->lookup( test_start+koord(-1,y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_w ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_w ++;
									}
								}
							}
						}
						pl = welt->lookup( test_start+koord(testsize.x,y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_e ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_e ++;
									}
								}
							}
						}
					}
					// corners were already checked, but to get correct numbers, we must check them again here
					for(  sint16 x=-1;  x<=testsize.x;  x++  ) {
						// upper and lower
						const planquadrat_t *pl = welt->lookup( test_start+koord(x,-1) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_n ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_n ++;
									}
								}
							}
						}
						pl = welt->lookup( test_start+koord(x,testsize.y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_s ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_s ++;
									}
								}
							}
						}
					}
					// now find out, if this offset/rotation is better ... (i.e. matches more fitting buildings)
					if(  r==0  ) {
						// r=0 is either facing south or north
						if(  best_halt_n>best_halt  ||  (best_halt==0  &&  neighbour_halt_n>any_halt)  ) {
							best_halt = best_halt_n;
							any_halt = neighbour_halt_n;
							rotation = 0;
							offsets = offset;
						}
						if(  best_halt_s>best_halt  ||  (best_halt==0  &&  neighbour_halt_s>any_halt)  ) {
							best_halt = best_halt_s;
							any_halt = neighbour_halt_s;
							rotation = 2;
							offsets = offset;
						}
					}
					else {
						// r=1 is either facing east or west
						if(  best_halt_w>best_halt  ||  (best_halt==0  &&  neighbour_halt_w>any_halt)  ) {
							best_halt = best_halt_w;
							any_halt = neighbour_halt_w;
							rotation = 1;
							offsets = offset;
						}
						if(  best_halt_e>best_halt  ||  (best_halt==0  &&  neighbour_halt_e>any_halt)  ) {
							best_halt = best_halt_e;
							any_halt = neighbour_halt_e;
							rotation = 3;
							offsets = offset;
						}
					}
				}
			}
		}

		// no suitable ground here ...
		if(  !any_ok  ) {
			return msg;
		}
		// check over/under halt again
		for(  sint16 x=0;  x<besch->get_b(rotation);  x++  ) {
			for(  sint16 y=0;  y<besch->get_h(rotation);  y++  ) {
				const planquadrat_t *pl = welt->lookup( pos-offsets+koord(x,y) );
				halthandle_t test_halt = pl->get_halt();
				if( test_halt.is_bound()  &&  spieler_t::check_owner( new_owner, test_halt->get_besitzer()) ) {
					halt = test_halt;
					break;
				}
			}
		}
		// is there no halt to connect?
		if(  !halt.is_bound()  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}
	else {
		// rotation was pre-slected; just search for stop now
		assert(  rotation < besch->get_all_layouts()  );
		koord testsize = besch->get_groesse(rotation);
		offsets = koord(0,0);

		if(  !welt->square_is_free(pos, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())  ) {
			return "Tile not empty.";
		}
		// check over/under halt
		for(  sint16 x=0;  x<testsize.x;  x++  ) {
			for(  sint16 y=0;  y<testsize.y;  y++  ) {
				const planquadrat_t *pl = welt->lookup(pos+koord(x,y));
				halthandle_t test_halt = pl->get_halt();
				if(test_halt.is_bound()) {
					if(!spieler_t::check_owner( new_owner, test_halt->get_besitzer())) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					else if(!halt.is_bound()) {
						halt = test_halt;
					}
					else if(halt != test_halt) {
						 return "Several halts found.";
					}
				}
			}
		}
		if(!halt.is_bound()) {
			halt = suche_nahe_haltestelle(new_owner, welt, welt->lookup_kartenboden(pos)->get_pos(), besch->get_b(rotation), besch->get_h(rotation) );
			// is there no halt to connect?
			if(  !halt.is_bound()  ) {
				return "Post muss neben\nHaltestelle\nliegen!\n";
			}
		}
	}

	if(  rotation>besch->get_all_layouts()  ) {
		rotation %= besch->get_all_layouts();
	}

	hausbauer_t::baue(welt, halt->get_besitzer(), k-offsets, rotation, besch, &halt);

	settings_t const& s      = welt->get_settings();
	sint32     const  factor = besch->get_level() * besch->get_b() * besch->get_h();
	sint64            cost   = s.cst_multiply_post * factor;
	if(sp!=halt->get_besitzer()  &&  halt->get_besitzer()==welt->get_spieler(1)) {
		// public stops are expensive!
		cost -= (s.maint_building * factor * 60);
	}
	// difficult to distinguish correctly most suitable waytype
	spieler_t::book_construction_costs(sp,  cost, pos, besch->get_finance_waytype());
	halt->recalc_station_type();

	return NULL;
}

/* build a dock either small or large */
const char *wkz_station_t::wkz_station_dock_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch)
{
	// the cursor cannot be outside the map from here on
	koord pos = k.get_2d();
	grund_t *gr = welt->lookup_kartenboden(pos);
	if (gr->get_hoehe()!= k.z) {
		return "";
	}
	hang_t::typ hang = gr->get_grund_hang();
	// first get the size
	int len = besch->get_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;
	halthandle_t halt;

	// check, if we can build here ...
	if(!hang_t::ist_einfach(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->is_within_limits(pos-dx*i)) {
				// need at least a single tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			// search for nearby stops
			halthandle_t test_halt = welt->lookup(pos-dx*i)->get_halt();
			if(test_halt.is_bound()) {
				if(!spieler_t::check_owner( sp, test_halt->get_besitzer())) {
					return "Das Feld gehoert\neinem anderen Spieler\n";
				}
				else if(!halt.is_bound()) {
					halt = test_halt;
				}
				else if(halt != test_halt) {
						return "Several halts found.";
				}
			}
			// check whether we can build something
			const grund_t *gr=welt->lookup_kartenboden(pos-dx*i);
			if (const char *msg = gr->kann_alle_obj_entfernen(sp)) {
				return msg;
			}

			if (i==0) {
				// start tile on slope near water
				if(gr->hat_wege()  ||  gr->get_typ()!=grund_t::boden  ||  gr->is_halt()) {
					return "Tile not empty.";
				}
			}
			else {
				// all other tiles in water
				if (!gr->ist_wasser()  ||  gr->find<gebaeude_t>()  ||  gr->get_depot()  ||  gr->is_halt()) {
					return "Tile not empty.";
				}
			}
		}
	}

	// remove everything from tile
	gr->obj_loesche_alle(sp);

DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);
	int layout = 0;
	koord3d bau_pos = welt->lookup_kartenboden(pos)->get_pos();
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
			bau_pos = welt->lookup_kartenboden(last_pos)->get_pos();
			break;
		case hang_t::west:
			layout = 3;
			dx2 = koord::nord;
			bau_pos = welt->lookup_kartenboden(last_pos)->get_pos();
			break;
	}

	// handle 16 layouts
	bool change_layout = false;
	if(besch->get_all_layouts()==16) {
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
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->get_halt()->get_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFD,0,0), false );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(pos-dx2);
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->get_halt()->get_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFB,0,0), false );
				}
			}
		}
	}

//DBG_MESSAGE("wkz_dockbau()","search for stop");
	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(sp, welt, welt->lookup_kartenboden(pos)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu) {
		if(  welt->lookup( (koord)pos )->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, really new stop on this tile then
		halt = haltestelle_t::create(welt, pos, sp);
	}
	hausbauer_t::baue(welt, halt->get_besitzer(), bau_pos, layout, besch, &halt);
	sint64 costs = welt->get_settings().cst_multiply_dock * besch->get_level();
	if(  sp!=halt->get_besitzer()  ) {
		// public stops are expensive!
		costs -= (welt->get_settings().maint_building * besch->get_level() * 60);
	}
	for(  int i=0;  i<=len;  i++  ) {
		koord p=pos-dx*i;
		spieler_t::book_construction_costs(sp,  costs, p, water_wt);
	}

	halt->recalc_station_type();
	if(  umgebung_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* const name = halt->create_name(pos, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

// build all types of stops but sea harbours
const char *wkz_station_t::wkz_station_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name )
{
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->get_name(), pos.x, pos.y, wegtype);
	const char *p_error=(besch->get_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	// underground is checked in work(); if underground only simple stations are allowed
	// get valid ground
	grund_t *bd = wkz_intern_koord_to_weg_grund(sp, welt, k, wegtype);

	if(  !bd  ||  bd->get_weg_hang()!=hang_t::flach  ) {
		// only flat tiles, only one stop per map square
		return "No suitable way on the ground!";
	}

	if(  bd->ist_tunnel()  &&  bd->ist_karten_boden()  ) {
		// do not build on tunnel entries
		return "No suitable way on the ground!";
	}

	if(  bd->get_depot()  ) {
		// not on depots
		return "No suitable ground!";
	}

	if(  bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0  ) {
		return "Flugzeughalt muss auf\nRunway liegen!\n";
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi  ribi=ribi_t::dir_invalid;
	if(  besch->get_all_layouts()==2  ||  besch->get_all_layouts()==8  ||  besch->get_all_layouts()==16  ) {
		// through station
		if(  bd->has_two_ways()  ) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked()  |  bd->get_weg_nr(1)->get_ribi_unmasked();
		}
		else if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// not straight: sorry cannot build here ...
		if(  !ribi_t::ist_gerade(ribi)  ) {
			return p_error;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(  besch->get_all_layouts()==4  ) {
		// terminal station
		if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// sorry cannot build here ... (not a terminal tile)
		if(  !ribi_t::ist_einfach(ribi)  ) {
			return p_error;
		}

		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	}
	else {
		// something wrong with station number of layouts
		dbg->fatal( "wkz_station_t::wkz_station_aux", "%s has wrong number of layouts (must be 2,4,8,16!)", besch->get_name() );
		return p_error;
	}

	if(  besch->get_all_layouts() == 8  ||  besch->get_all_layouts() == 16  ) {
		// through station - complex layout
		// bits
		// 1 = north south/east west (as simple layout)
		// 2 = use far end image  \ can be combined
		// 3 = use near end image / to use both end image
		// 4 = platform face - 0 = far, 1 = near

		// bit 1 has already been set

//		ribi_t::ribi next_halt = ribi_t::keine;
		ribi_t::ribi next_own = ribi_t::keine;

		sint8 offset = bd->get_hoehe()+bd->get_weg_yoff()/TILE_HEIGHT_STEP;

		grund_t *gr;
		sint32 neighbour_layout[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
		for(  unsigned i=0;  i<4;  i++  ) {
			// oriented buildings here - get neighbouring layouts
			const planquadrat_t *plan = welt->lookup(pos+koord::nsow[i]);
			if(  plan  &&  plan->get_halt().is_bound()  ) {
				// ok, here is a halt at least
//				next_halt |= ribi_t::nsow[i];
				gr = welt->lookup(koord3d(pos+koord::nsow[i],offset));
				if(!gr) {
					// check whether bridge end tile
					grund_t * gr_tmp = welt->lookup(koord3d(pos+koord::nsow[i],offset-1));
					if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
						gr = gr_tmp;
					}
				}
				if(gr) {
					// check, if there is an oriented stop
					const gebaeude_t* gb = gr->find<gebaeude_t>();
					if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4  &&  gb->get_tile()->get_besch()->get_utyp()>haus_besch_t::hafen) {
						next_own |= ribi_t::nsow[i];
						neighbour_layout[ribi_t::nsow[i]] = gb->get_tile()->get_layout();
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
		layout &= (besch->get_all_layouts()-1);
	}

	halthandle_t old_halt = bd->get_halt();
	uint16 old_level = 0;

	halthandle_t halt;

	if(  old_halt.is_bound()  ) {
		gebaeude_t* gb = bd->find<gebaeude_t>();
		const haus_besch_t *old_besch = gb->get_tile()->get_besch();
		old_level = old_besch->get_level();
		if(  old_besch == besch  ) {
			// already has the same station
			return NULL;
		}
		if(  old_besch->get_level() >= besch->get_level()  &&  !is_ctrl_pressed()  ) {
			return "Upgrade must have\na higher level";
		}
		gb->entferne( NULL );
		delete gb;
		halt = old_halt;
	}
	else {
		halt = suche_nahe_haltestelle(sp,welt,bd->get_pos());
	}

	// seems everything ok, lets build
	bool neu = !halt.is_bound();

	if(neu) {
		if(  welt->lookup( pos )->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		halt = haltestelle_t::create(welt, pos, sp);
	}
	hausbauer_t::neues_gebaeude( welt, halt->get_besitzer(), bd->get_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* const name = halt->create_name(pos, type_name);
		halt->set_name(name);
		free(name);
	}

	sint64 old_cost = old_level * cost;
	cost *= besch->get_level()*besch->get_b()*besch->get_h();
	cost -= old_cost/2;
	if(  sp!=halt->get_besitzer()  ) {
		// public stops are expensive!
		cost -= (welt->get_settings().maint_building * besch->get_level() * besch->get_b() * besch->get_h() * 60);
	}
	spieler_t::book_construction_costs(sp,  cost, pos, wegtype);
	if(  umgebung_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}
	return NULL;
}

// gives the description and sets the rotation value
const haus_besch_t *wkz_station_t::get_besch( sint8 &rotation ) const
{
	char *haus = strdup( default_param );
	const haus_tile_besch_t *ht = NULL;
	if(  haus  ) {
		char *p = strrchr( haus, ',' );
		if(  p  ) {
			*p++ = 0;
			rotation = atoi( p );
		}
		else {
			rotation = -1;
		}
		ht = hausbauer_t::find_tile(haus,0);
		free( haus );
	}
	if(  ht==NULL  ) {
		return NULL;
	}
	return ht->get_besch();
}

bool wkz_station_t::init( karte_t *welt, spieler_t * )
{
	sint8 rotation = -1;
	const haus_besch_t *hb = get_besch( rotation );
	if(  hb==NULL  ) {
		return false;
	}
	cursor = hb->get_cursor()->get_bild_nr(0);
	if(  !is_local_execution()  ) {
		// do not change cursor
		return true;
	}
	if(  hb->get_utyp()==haus_besch_t::generic_extension  &&  hb->get_all_layouts()>1  ) {
		if(  is_ctrl_pressed()  &&  rotation==-1  ) {
			// call station dialog instead
			destroy_win( magic_station_building_select );
			create_win( new station_building_select_t(welt, hb), w_info, magic_station_building_select);
			// we do not activate building yet; else uncomment the return statement
			return false;
		}
		else if(  rotation>=0  ) {
			// rotation is already fixed
			cursor_area = koord( hb->get_b(rotation), hb->get_h(rotation) );
			cursor_centered = false;
		}
		else {
			goto set_area_cov;
		}
	}
	else {
set_area_cov:
		uint16 const cov = welt->get_settings().get_station_coverage() * 2 + 1;
		cursor_area = koord(cov, cov);
		cursor_centered = true;
	}
	return true;
}


image_id wkz_station_t::get_icon( spieler_t * ) const
{
	sint8 dummy;
	const haus_besch_t *besch=get_besch(dummy);
	if(  grund_t::underground_mode==grund_t::ugm_all  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
		if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
			return IMG_LEER;
		}
		if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_underground()) {
			return IMG_LEER;
		}
	}
	if(  grund_t::underground_mode==grund_t::ugm_none  ) {
		if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_aboveground()) {
			return IMG_LEER;
		}
	}
	return icon;
}


const char* wkz_station_t::get_tooltip(const spieler_t *sp) const
{
	sint8               dummy;
	karte_t&            welt     = *sp->get_welt();
	settings_t   const& settings = welt.get_settings();
	haus_besch_t const& besch    = *get_besch(dummy);
	uint32              level    = besch.get_level();
	sint64              price;
	switch (besch.get_utyp()) {
		case haus_besch_t::generic_stop:
			switch (besch.get_extra()) {
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case tram_wt:
				case narrowgauge_wt: price = settings.cst_multiply_station;     break;
				case road_wt:        price = settings.cst_multiply_roadstop;    break;
				case water_wt:       price = settings.cst_multiply_dock;        break;
				case air_wt:         price = settings.cst_multiply_airterminal; break;
				case ignore_wt:      price = settings.cst_multiply_post;        break;
				default:             goto invalid;
			}
			break;

		case haus_besch_t::generic_extension: price = settings.cst_multiply_post; goto scale_by_size;
		case haus_besch_t::hafen:             price = settings.cst_multiply_dock; goto scale_by_size;
scale_by_size:
			level = level * besch.get_groesse().x * besch.get_groesse().y;
			break;

		default:
invalid:
			return "Illegal description";
	}
	return tooltip_with_price_maintenance_level(&welt, besch.get_name(), price * level, settings.maint_building * level, level, besch.get_enabled());
}

waytype_t wkz_station_t::get_waytype() const
{
	sint8 dummy;
	haus_besch_t const* besch = get_besch(dummy);
	switch (besch->get_utyp()) {
		case haus_besch_t::generic_stop:
			return (waytype_t)besch->get_extra();
		case haus_besch_t::hafen:
			return water_wt;
		case haus_besch_t::generic_extension:
		default:
			return invalid_wt;
	}
}


const char *wkz_station_t::check_pos( karte_t *welt, spieler_t*,  koord3d pos )
{
	if(  grund_t *gr = welt->lookup( pos )  ) {
		sint8 rotation;
		const haus_besch_t *besch = get_besch(rotation);
		if(  grund_t *bd = welt->lookup_kartenboden( pos.get_2d() )  ) {
			const bool underground = bd->get_hoehe()>gr->get_hoehe();
			if(  underground  ) {
				// in underground mode, buildings will be done invisible above ground => disallow such confusion
				if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
					return "Cannot built this station/building\nin underground mode here.";
				}
				if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_underground()) {
					return "Cannot built this station/building\nin underground mode here.";
				}
			}
			else if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_aboveground()) {
				return "This station/building\ncan only be built underground.";
			}
			return NULL;
		}
	}
	// no ground here???
	return "Missing ground (fatal!)";
}


const char *wkz_station_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	CHECK_FUNDS();

	const char *result = NULL;
	if(  buttonstate==1  ) {
		const planquadrat_t *plan = welt->lookup(pos.get_2d());
		if(!plan) {
			return "";
		}

		// ownership allowed?
		halthandle_t halt = plan->get_halt();
		if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->get_besitzer())) {
			return "";
		}

		if(  umgebung_t::networkmode  ) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(sp, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( welt, sp, pos );
		}
	}
	return result;
}


const char *wkz_station_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(!plan) {
		return "";
	}

	// ownership allowed?
	halthandle_t halt = plan->get_halt();
	if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->get_besitzer())) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	// check underground / above ground
	if (const char* msg = check_pos(welt, sp, pos)) {
		return msg;
	}

	sint8 rotation = 0;
	const haus_besch_t *besch=get_besch(rotation);
	const char *msg = NULL;
	switch (besch->get_utyp()) {
		case haus_besch_t::hafen:
			msg = wkz_station_t::wkz_station_dock_aux(welt, sp, pos, besch );
			break;
		case haus_besch_t::hafen_geb:
		case haus_besch_t::generic_extension:
			msg = wkz_station_t::wkz_station_building_aux(welt, sp, false, pos, besch, rotation );
			if (msg) {
				// try to build near a public halt
				msg = wkz_station_t::wkz_station_building_aux(welt, sp, true, pos, besch, rotation );
			}
			break;
		case haus_besch_t::generic_stop: {
			settings_t const& s = welt->get_settings();
			switch(besch->get_extra()) {
				case road_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, road_wt, s.cst_multiply_roadstop, "H");
					break;
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case narrowgauge_wt:
				case tram_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, (waytype_t)besch->get_extra(), s.cst_multiply_station, "BF");
					break;
				case water_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, water_wt, s.cst_multiply_dock, "Dock");
					break;
				case air_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, air_wt, s.cst_multiply_airterminal, "Airport");
					break;
			}
			break;
		}

		default:
			dbg->warning("wkz_station_t::work()","tool called for illegal besch \"%\"", default_param );
			msg = "Illegal station tool";
	}
	return msg;
}


char const* wkz_roadsign_t::get_tooltip(spieler_t const*) const
{
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch) {
		return tooltip_with_price( besch->get_name(), -besch->get_preis() );
	}
	return NULL;
}

void wkz_roadsign_t::draw_after(karte_t *welt, koord pos, bool dirty) const
{
	if(  icon!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( icon, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		char level_str[16];
		sprintf(level_str, "%i", signal[welt->get_active_player_nr()].spacing);
		display_proportional( pos.x+4, pos.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
	}
}

const char* wkz_roadsign_t::check_pos_intern(karte_t *welt, spieler_t *sp, koord3d pos)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	if (besch==NULL) {
		// read data from string
		read_default_param(sp);
	}
	if (besch==NULL) {
		return error;
	}
	// search for starting ground
	grund_t *gr = wkz_intern_koord_to_weg_grund(sp, welt, pos, besch->get_wtyp());
	if(gr) {
		// get the sign direction
		weg_t *weg = gr->get_weg( besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		signal_t *s = gr->find<signal_t>();
		if(s  &&  s->get_besch()!=besch) {
			// only one sign per tile
			return error;
		}
		if(besch->is_signal()  &&  gr->find<roadsign_t>())  {
			// only one sign per tile
			return error;
		}

		ribi_t::ribi dir = weg->get_ribi_unmasked();

		// no signals on switches
		if(  ribi_t::is_threeway(dir)  &&  besch->is_signal_type()  ) {
			return error;
		}

		if(  besch->is_private_way()  &&  !ribi_t::ist_gerade(dir)  ) {
			// only on straight tiles ...
			return error;
		}

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			error = NULL;
		}
	}
	return error;
}

char wkz_roadsign_t::toolstring[256];

const char* wkz_roadsign_t::get_default_param(spieler_t *sp) const
{
	if (besch  &&  sp) {
		signal_info const& s = signal[sp->get_player_nr()];
		sprintf(toolstring, "%s,%d,%d,%d", besch->get_name(), s.spacing, s.remove_intermediate, s.replace_other);
		return toolstring;
	}
	else {
		return default_param;
	}
}

waytype_t wkz_roadsign_t::get_waytype() const
{
	return besch ? besch->get_wtyp() : invalid_wt;
}

// read variables from default_param if cmd comes from network
// default_param: sign_name,signal_spacing,remove,replace
// if the static variable toolstring is the default_param then reset default_param to name of signal
void wkz_roadsign_t::read_default_param(spieler_t * sp)
{
	char name[256]="";
	uint32 i;
	for(i=0; default_param[i]!=0  &&  default_param[i]!=','; i++) {
		name[i]=default_param[i];
	}
	name[i]=0;
	besch = roadsign_t::find_besch(name);

	if (default_param[i]) {
		signal_info& s = signal[sp->get_player_nr()];
		int i_signal_spacing              = s.spacing;
		int i_remove_intermediate_signals = s.remove_intermediate;
		int i_replace_other_signals       = s.replace_other;
		sscanf(default_param+i, ",%d,%d,%d", &i_signal_spacing, &i_remove_intermediate_signals, &i_replace_other_signals);
		s.spacing             = (uint8)i_signal_spacing;
		s.remove_intermediate = i_remove_intermediate_signals != 0;
		s.replace_other       = i_replace_other_signals       != 0;
	}
	if (default_param==toolstring) {
		default_param = besch->get_name();
	}
}

bool wkz_roadsign_t::init( karte_t *welt, spieler_t * sp)
{
	// read data from string
	read_default_param(sp);

	if (is_ctrl_pressed()  &&  is_local_execution()) {
		create_win(new signal_spacing_frame_t(sp, this), w_info, (ptrdiff_t)this);
	}
	return two_click_werkzeug_t::init(welt, sp);
}

bool wkz_roadsign_t::exit( karte_t *welt, spieler_t *sp )
{
	destroy_win((ptrdiff_t)this);
	return two_click_werkzeug_t::exit(welt,sp);
}

uint8 wkz_roadsign_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d &start)
{
	// first click
	if (start==koord3d::invalid) {
		error = check_pos_intern(welt, sp, pos);
		return (error==NULL ? 3 : 0);
	}
	// second click
	else {
		error = NULL;
		return 2;
	}
}


bool wkz_roadsign_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t rs_besch( besch->get_wtyp(), 500, vehikel_besch_t::diesel );
	vehikel_t* test_vehicle = vehikelbauer_t::baue(start, sp, NULL, &rs_besch);
	test_vehicle->set_flag(ding_t::not_on_map);
	fahrer_t* test_driver = scenario_checker_t::apply(test_vehicle, sp, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(sp->get_welt(), start, to, test_driver, 0, 0);
		// prevent building of many signals if start and to are adjacent
		// but the step start->to is now allowed
		if (can_built  &&  koord_distance(start, to)==1  &&  verbindung.get_count()>2) {
			grund_t *gr, *grto = sp->get_welt()->lookup(to);
			if(  sp->get_welt()->lookup(start)->get_neighbour(gr, besch->get_wtyp(), ribi_typ(to-start) )  &&  gr==grto) {
				can_built = false;
			}
		}
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

void wkz_roadsign_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &ziel )
{
	route_t route;
	if (!calc_route(route, sp, start, ziel)) {
		return;
	}
	signal_info const& s              = signal[sp->get_player_nr()];
	uint8       const  signal_density = 2 * s.spacing;      // measured in half tiles (straight track count as 2, diagonal as 1, since sqrt(1/2) = 1/2 ;)
	uint8              next_signal    = signal_density + 1; // to place a sign asap
	sint32             cost           = 0;
	directions.clear();
	// dummy roadsign to get images for preview
	roadsign_t *dummy_rs;
	if (besch->is_signal_type()) {
		dummy_rs = new signal_t(welt, sp, koord3d::invalid, ribi_t::keine, besch);
	}
	else {
		dummy_rs = new roadsign_t(welt, sp, koord3d::invalid, ribi_t::keine, besch);
	}
	dummy_rs->set_flag(ding_t::not_on_map);

	bool single_ribi = besch->is_signal_type() || besch->is_single_way() || besch->is_choose_sign();
	for(  uint16 i = 0;  i < route.get_count();  i++  ) {
		grund_t* gr = welt->lookup( route.position_bei(i) );

		weg_t *weg = gr->get_weg(besch->get_wtyp());
		ribi_t::ribi ribi=weg->get_ribi_unmasked(); // set full ribi when signal is on a crossing.
		if(  single_ribi  &&  ribi_t::is_twoway(weg->get_ribi_unmasked()) ) {
			if(i < route.get_count()-1  ) {
				ribi -= ribi_typ(route.position_bei(i), route.position_bei(i+1));
			}
			else if(i>0) {
				ribi -= ribi_typ(route.position_bei(i-1), route.position_bei(i));
			}
		}

		roadsign_t *rs = gr->find<signal_t>();
		if (rs==NULL) {
			rs = gr->find<roadsign_t>();
		}

		// check owner .. other signals...
		bool straight = (i == 0)  ||  (i == route.get_count()-1)  ||  ribi_t::ist_gerade(ribi_typ(route.position_bei(i-1), route.position_bei(i+1)));
		next_signal += straight ? 2 : 1;
		if(  next_signal >= signal_density  ) {
			// can we place signal here?
			if (check_pos_intern(welt, sp, route.position_bei(i))==NULL  ||
					(s.replace_other && rs && !rs->ist_entfernbar(sp))) {
				zeiger_t* zeiger = new zeiger_t(welt, gr->get_pos(), sp );
				marked.append(zeiger);
				zeiger->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0) );
				gr->obj_add( zeiger );
				directions.append(ribi /* !=0 -> place sign*/);
				next_signal = 0;
				dummy_rs->set_pos(gr->get_pos());
				dummy_rs->set_dir(ribi); // calls calc_bild()
				zeiger->set_after_bild(dummy_rs->get_after_bild());
				zeiger->set_bild(dummy_rs->get_bild());
				// as set_dir also modifies the ribi-mask of the way we have to reset it here...
				dummy_rs->set_dir(rs ? rs->get_dir() : (ribi_t::ribi)ribi_t::keine);
				cost += rs ? (rs->get_besch()==besch ? 0  : besch->get_preis()+rs->get_besch()->get_preis()) : besch->get_preis();
			}
		} else if (s.remove_intermediate && rs && !rs->ist_entfernbar(sp)) {
				zeiger_t* zeiger = new zeiger_t(welt, gr->get_pos(), sp );
				marked.append(zeiger);
				zeiger->set_bild( werkzeug_t::general_tool[WKZ_REMOVER]->cursor );
				gr->obj_add( zeiger );
				directions.append(ribi_t::keine /*remove sign*/);
				cost += rs->get_besch()->get_preis();
		}
	}
	delete dummy_rs;
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", cost ) );
}

const char *wkz_roadsign_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end)
{
	// read data from string
	read_default_param(sp);
	// single click ->place signal
	if( end == koord3d::invalid  ||  start == end ) {
		grund_t *gr = welt->lookup(start);
		return place_sign_intern( welt, sp, gr );
	}
	// mark tiles to calculate positions of signals
	mark_tiles(welt, sp, start, end);
	// only search the marked tiles
	uint32 j=0;
	FOR(slist_tpl<zeiger_t*>, const i, marked) {
		grund_t* const gr = welt->lookup(i->get_pos());
		weg_t *weg = gr->get_weg(besch->get_wtyp());
		ribi_t::ribi dir = directions[j++];
		if (dir) {
			// try to place signal
			const char* error_text =  place_sign_intern( welt, sp, gr );
			if(  error_text  ) {
				if (signal[sp->get_player_nr()].replace_other) {
					roadsign_t* rs = gr->find<signal_t>();
					if(rs == NULL) rs = gr->find<roadsign_t>();
					if(  rs != NULL  &&  rs->ist_entfernbar(sp) == NULL  ) {
						rs->entferne(sp);
						delete rs;
						error_text =  place_sign_intern( welt, sp, gr );
					}
				}
			}
			if(  error_text  ) {
				return error_text;
			}
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			assert(rs);
			rs->set_dir(dir);
		}
		else {
			// Place no signal -> remove existing signal
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			if(  rs != NULL  &&  rs->ist_entfernbar(sp) == NULL  ) {
				rs->entferne(sp);
				delete rs;
			};
		}
		weg->count_sign();
		gr->calc_bild();
	}
	cleanup(true);
	directions.clear();
	return NULL;
}

/*
 * Called by the GUI (gui/signal_spacing.*)
 */
void wkz_roadsign_t::set_values( spieler_t *sp, uint8 spacing, bool remove, bool replace )
{
	signal_info& s = signal[sp->get_player_nr()];
	s.spacing             = spacing;
	s.remove_intermediate = remove;
	s.replace_other       = replace;
}


void wkz_roadsign_t::get_values( spieler_t *sp, uint8 &spacing, bool &remove, bool &replace )
{
	signal_info const& s = signal[sp->get_player_nr()];
	spacing = s.spacing;
	remove  = s.remove_intermediate;
	replace = s.replace_other;
}


const char *wkz_roadsign_t::place_sign_intern( karte_t *welt, spieler_t *sp, grund_t* gr, const roadsign_besch_t*)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	// search for starting ground
	if(gr) {
		// get the sign direction
		weg_t *weg = gr->get_weg( besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		roadsign_t *s = gr->find<signal_t>();
		if(s==NULL) {
			s = gr->find<roadsign_t>();
		}
		if(s  &&  s->get_besch()!=besch) {
			// only one sign per tile
			return error;
		}
		ribi_t::ribi dir = weg->get_ribi_unmasked();

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
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
					rs = new signal_t(welt, sp, gr->get_pos(), dir, besch);
					DBG_MESSAGE("wkz_roadsign()", "new signal, dir is %i", dir);
					goto built_sign;
				}
			} else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// reverse only if single way sign
					if (besch->is_single_way() || besch->is_choose_sign()) {
						dir = ~rs->get_dir() & weg->get_ribi_unmasked();
						rs->set_dir(dir);
						DBG_MESSAGE("wkz_roadsign()", "reverse ribi %i", dir);
					}
				}
				else {
					// add a new roadsign at position zero!
					// if single way, we need to reduce the allowed ribi to one
					if (besch->is_single_way() || besch->is_choose_sign()) {
						for(  int i=0;  i<4;  i++  ) {
							if ((dir & ribi_t::nsow[i]) != 0) {
								dir = ribi_t::nsow[i];
								break;
							}
						}
					}
					DBG_MESSAGE("wkz_roadsign()", "new roadsign, dir is %i", dir);
					rs = new roadsign_t(welt, sp, gr->get_pos(), dir, besch);
built_sign:
					gr->obj_add(rs);
					rs->laden_abschliessen();	// to make them visible
					weg->count_sign();
					spieler_t::book_construction_costs(sp, -besch->get_preis(), gr->get_pos().get_2d(), weg->get_waytype());
				}
			}
			error = NULL;
		}
	}
	return error;
}



// built all types of depots
const char *wkz_depot_t::wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost)
{
	if(welt->is_within_limits(pos.get_2d())) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos.get_2d());
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
		if(besch->get_extra()==air_wt  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0) {
			return "Cannot built depot here!";
		}

		const char *p=bd->kann_alle_obj_entfernen(sp);
		if(p) {
			return p;
		}

		// avoid building over a stop
		if(bd->is_halt()  ||  bd->get_depot()!=NULL) {
			return "Cannot built depot here!";
		}

		ribi_t::ribi ribi;
		if(bd->ist_wasser()) {
			// assume one orientation with water
			ribi = ribi_t::sued;
		}
		else {
			ribi = bd->get_weg_ribi_unmasked(wegtype);
		}

		if(ribi_t::ist_einfach(ribi)  &&  bd->get_weg_hang()==0) {

			int layout = 0;
			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude( welt, sp, bd->get_pos(), layout, besch );
			spieler_t::book_construction_costs(sp, cost, pos.get_2d(), besch->get_finance_waytype());
			if(is_local_execution()  &&  sp == welt->get_active_player()) {
				welt->set_werkzeug( general_tool[WKZ_ABFRAGE], sp );
			}

			return NULL;
		}
		return "Cannot built depot here!";
	}
	return "";
}

image_id wkz_depot_t::get_icon(spieler_t *sp) const
{
	if(  sp  &&  sp->get_player_nr()!=1  ) {
		const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
		const uint16 time = sp->get_welt()->get_timeline_year_month();
		if(  time==0  ||  (besch->get_intro_year_month() <= time  &&  time < besch->get_retire_year_month())  ) {
			return besch->get_cursor()->get_bild_nr(1);
		}
	}
	return IMG_LEER;
}

bool wkz_depot_t::init( karte_t *welt, spieler_t *sp )
{
	haus_besch_t const& besch    = *hausbauer_t::find_tile(default_param, 0)->get_besch();
	// no depots for player 1
	if(sp!=welt->get_spieler(1)) {
		cursor = besch.get_cursor()->get_bild_nr(0);
		return true;
	}
	return false;
}

const char* wkz_depot_t::get_tooltip(const spieler_t *sp) const
{
	karte_t&            welt     = *sp->get_welt();
	settings_t   const& settings = welt.get_settings();
	haus_besch_t const& besch    = *hausbauer_t::find_tile(default_param, 0)->get_besch();
	char         const* tip;
	sint64              price;
	switch (besch.get_extra()) {
		case road_wt:        tip = "Build road depot";        price = settings.cst_depot_road; break;
		case track_wt:       tip = "Build train depot";       price = settings.cst_depot_rail; break;
		case monorail_wt:    tip = "Build monorail depot";    price = settings.cst_depot_rail; break;
		case maglev_wt:      tip = "Build maglev depot";      price = settings.cst_depot_rail; break;
		case narrowgauge_wt: tip = "Build narrowgauge depot"; price = settings.cst_depot_rail; break;
		case tram_wt:        tip = "Build tram depot";        price = settings.cst_depot_rail; break;
		case water_wt:       tip = "Build ship depot";        price = settings.cst_depot_ship; break;
		case air_wt:         tip = "Build air depot";         price = settings.cst_depot_air;  break;
		default:             return 0;
	}
	return tooltip_with_price_maintenance(&welt, tip, price, settings.maint_building * besch.get_level());
}

waytype_t wkz_depot_t::get_waytype() const
{
	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	return (waytype_t)besch->get_extra();
}

const char *wkz_depot_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if(sp==welt->get_spieler(1)) {
		// no depots for player 1
		return 0;
	}

	haus_besch_t const* const besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	settings_t   const&       s     = welt->get_settings();
	switch(besch->get_extra()) {
		case road_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, road_wt, s.cst_depot_road);
		case track_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, track_wt, s.cst_depot_rail);
		case monorail_wt:
			{
				// since it needs also a foundation, this is slightly more complex ...
				char const* const err = wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, monorail_wt, s.cst_depot_rail);
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(k.get_2d());
					if(hausbauer_t::elevated_foundation_besch  &&  k.z-bd->get_pos().z==1  &&  bd->ist_natur()) {
						hausbauer_t::baue( welt, sp, bd->get_pos(), 0, hausbauer_t::elevated_foundation_besch );
					}
				}
				return err;
			}
		case tram_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, track_wt, s.cst_depot_rail);
		case water_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, water_wt, s.cst_depot_ship);
		case air_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, air_wt, s.cst_depot_air);
		case maglev_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, maglev_wt, s.cst_depot_rail);
		case narrowgauge_wt:
			return wkz_depot_t::wkz_depot_aux(welt, sp, k, besch, narrowgauge_wt, s.cst_depot_rail);
		default:
			dbg->warning("wkz_depot()","called with unknown besch %s",besch->get_name() );
			return "Unknown depot object";
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
bool wkz_build_haus_t::init( karte_t *, spieler_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile!=NULL) {
			int rotation = (default_param[1]-'0') % tile->get_besch()->get_all_layouts();
			cursor_area = tile->get_besch()->get_groesse(rotation);
		}
	}
	return true;
}

const char *wkz_build_haus_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	// Parsing parameter (if there)
	const haus_besch_t *besch = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile) {
			besch = tile->get_besch();
		}
	}
	else {
		besch = hausbauer_t::waehle_sehenswuerdigkeit( welt->get_timeline_year_month(), false, welt->get_climate( pos.get_2d() ) );
	}

	if(besch==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % besch->get_all_layouts() : simrand(besch->get_all_layouts());
	koord size = besch->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : besch->get_allowed_climate_bits();

	bool hat_platz = welt->square_is_free( pos.get_2d(), besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	if(!hat_platz  &&  size.y!=size.x  &&  besch->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
		// try other rotation too ...
		rotation = (rotation+1) % besch->get_all_layouts();
		hat_platz = welt->square_is_free( pos.get_2d(), besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	}

	// Platz gefunden ...
	if(hat_platz) {
		spieler_t *gb_sp = besch->get_typ()!=gebaeude_t::unbekannt ? NULL : welt->get_spieler(1);
		gebaeude_t *gb = hausbauer_t::baue(welt, gb_sp, gr->get_pos(), rotation, besch);
		if(gb) {
			// building successfull
			if(  besch->get_utyp()!=haus_besch_t::attraction_land  &&  besch->get_utyp()!=haus_besch_t::attraction_city  ) {
				stadt_t *city = welt->suche_naechste_stadt( pos.get_2d() );
				if(city) {
					city->add_gebaeude_to_stadt(gb);
				}
			}
			spieler_t::book_construction_costs(sp, welt->get_settings().cst_multiply_remove_haus * besch->get_level() * size.x * size.y, pos.get_2d(), gb->get_waytype());
			return NULL;
		}
	}
	return "No suitable ground!";
}



// show industry size in cursor (in known)
bool wkz_build_industries_land_t::init( karte_t *, spieler_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
	}
	return true;
}

/* builds a (if param=NULL random) industry chain starting here *
 * the parameter string is a follow:
 * 1#34,oelfeld
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * next number is production value
 * finally industry name
 */
const char *wkz_build_industries_land_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( k.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_haus()->get_all_layouts() : simrand(fab->get_haus()->get_all_layouts()-1);
	koord size = fab->get_haus()->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->get_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->square_is_free( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->square_is_free( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			initial_prod = atol(default_param+2)>>(welt->ticks_per_world_month_shift-18);
		}

		koord3d k = gr->get_pos();
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, initial_prod, rotation, &k, welt->get_spieler(1), 10000 );

		if(anzahl>0) {
			// at least one factory has been built
			welt->change_world_position( k );
			spieler_t::book_construction_costs(sp, anzahl * welt->get_settings().cst_multiply_found_industry, k.get_2d(), ignore_wt);

			// crossconnect all?
			if (welt->get_settings().is_crossconnect_factories()) {
				FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
					f->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}


// show industry size in cursor (in known)
bool wkz_build_industries_city_t::init( karte_t *, spieler_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
	}
	return true;
}

/* builds a industry chain in the next town
 * defaukt_param see previous function
 */
const char *wkz_build_industries_city_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( k.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}

	// eventually adjust production
	sint32 initial_prod = -1;
	if (!strempty(default_param)) {
		initial_prod = atol(default_param+2)>>(welt->ticks_per_world_month_shift-18);
	}

	k = gr->get_pos();
	int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, initial_prod, 0, &k, welt->get_spieler(1), 10000 );
	if(anzahl>0) {
		// at least one factory has been built
		welt->change_world_position( k );

		// crossconnect all?
		if (welt->get_settings().is_crossconnect_factories()) {
			FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
				f->add_all_suppliers();
			}
		}
		// ain't going to be cheap
		spieler_t::book_construction_costs(sp, anzahl * welt->get_settings().cst_multiply_found_industry, k.get_2d(), ignore_wt);
		return NULL;
	}
	return "No suitable ground!";
}



// show industry size in cursor (must be known!)
bool wkz_build_factory_t::init( karte_t *, spieler_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
		return true;
	}
	return true;
}

/* builds an industry next to the cursor (default_param see above) */
const char *wkz_build_factory_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( k.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_haus()->get_all_layouts() : simrand(fab->get_haus()->get_all_layouts());
	koord size = fab->get_haus()->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->get_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->square_is_free( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->square_is_free( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			initial_prod = atol(default_param+2)>>(welt->ticks_per_world_month_shift-18);
		}

		fabrik_t *f = fabrikbauer_t::baue_fabrik(welt, NULL, fab, initial_prod, rotation, gr->get_pos(), welt->get_spieler(1));
		if(f) {
			// at least one factory has been built
			welt->change_world_position( k );
			spieler_t::book_construction_costs(sp, welt->get_settings().cst_multiply_found_industry, k.get_2d(), ignore_wt);

			// crossconnect all?
			if (welt->get_settings().is_crossconnect_factories()) {
				FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
					f->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}



/**	link tool: links products of factory one with factory two (if possible)
 */
image_id wkz_link_factory_t::get_marker_image()
{
	return cursor;
}


uint8 wkz_link_factory_t::is_valid_pos( karte_t *welt, spieler_t *, const koord3d &pos, const char *&error, const koord3d & )
{
	fabrik_t *fab = fabrik_t::get_fab( welt, pos.get_2d() );
	if (fab == NULL) {
		error = "";
		return 0;
	}
	return 2;
}


const char *wkz_link_factory_t::do_work( karte_t *welt, spieler_t *, const koord3d &start, const koord3d &pos )
{
	fabrik_t *last_fab = fabrik_t::get_fab( welt, start.get_2d() );
	fabrik_t *fab = fabrik_t::get_fab( welt, pos.get_2d() );

	if(fab!=NULL  &&  last_fab!=NULL  &&  last_fab!=fab) {
		// It's a factory
		if(!is_ctrl_pressed()) {
			if(fab->add_supplier(last_fab) || last_fab->add_supplier(fab)) {
				//ok! they are connected
				return NULL;
			}
		}
		else {
			// remove connections
			fab->rem_supplier(last_fab->get_pos().get_2d());
			fab->rem_lieferziel(last_fab->get_pos().get_2d());
			last_fab->rem_supplier(fab->get_pos().get_2d());
			last_fab->rem_lieferziel(fab->get_pos().get_2d());
			return NULL;
		}
	}
	return "";
}


/* builds company headquarter
 * @author prissi
 */
const haus_besch_t *wkz_headquarter_t::next_level( const spieler_t *sp ) const
{
	return hausbauer_t::get_headquarter(sp->get_headquarter_level(), sp->get_welt()->get_timeline_year_month());
}

const char* wkz_headquarter_t::get_tooltip(const spieler_t *sp) const
{
	if (haus_besch_t const* const besch = next_level(sp)) {
		settings_t  const& s      = sp->get_welt()->get_settings();
		char const* const  tip    = sp->get_headquarter_level() == 0 ? "build HQ" : "upgrade HQ";
		sint64      const  factor = (sint64)besch->get_level() * besch->get_b() * besch->get_h();
		return tooltip_with_price_maintenance(sp->get_welt(), tip, factor * s.cst_multiply_headquarter, factor * s.maint_building);
	}
	return NULL;
}

bool wkz_headquarter_t::init( karte_t *, spieler_t *sp )
{
	// do no use this, if there is no next level to build ...
	const haus_besch_t *besch = next_level(sp);
	if (besch) {
		if (is_local_execution()) {
			const int rotation = 0;
			cursor_area = besch->get_groesse(rotation);
		}
		return true;
	}
	return false;
}


const char *wkz_headquarter_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	bool ok=false;
	bool built = false;
DBG_MESSAGE("wkz_headquarter()", "building headquarter at (%d,%d)", pos.x, pos.y);

	const haus_besch_t* besch = next_level(sp);
	if(besch==NULL) {
		// no further headquarter level
		dbg->message( "wkz_headquarter()", "Already at maximum level!" );
		return "";
	}

	koord size = besch->get_groesse();
	sint64 const cost = welt->get_settings().cst_multiply_headquarter * besch->get_level() * size.x * size.y;
	if(  -cost > sp->get_finance()->get_account_balance()  ) {
		return "Not enough money!";
	}

	if(welt->is_within_limits(pos.get_2d())) {
		// check for underground ..
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		if (!gr) {
			return "";
		}
		gebaeude_t *hq = NULL;
		// check for current head quarter
		koord previous = sp->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr_hq = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr_hq->find<gebaeude_t>();
			// check if upgrade should be built at same place as current one
			gebaeude_t *gb = gr->find<gebaeude_t>();
			if (gb  &&  gb->get_besitzer()==sp  &&  prev_hq->get_tile()->get_besch()==gb->get_tile()->get_besch()) {
				const haus_besch_t* prev_besch = prev_hq->get_tile()->get_besch();
				// check if sizes fit
				uint8 prev_layout = prev_hq->get_tile()->get_layout();
				uint8 layout =  prev_layout % besch->get_all_layouts();
				koord size = besch->get_groesse(layout);
				if (prev_besch->get_groesse(prev_layout) == size) {
					// check for same tile structure
					ok = true;
					for (sint16 x=0; x<size.x  &&  ok; x++) {
						for (sint16 y=0; y<size.y  &&  ok; y++) {
							ok = (prev_besch->get_tile(prev_layout, x, y)==NULL)==(besch->get_tile(layout, x, y)==NULL);
						}
					}
					hq = gb;
					if (ok) {
						// upgrade the tiles
						koord pos_hq = pos.get_2d() - gb->get_tile()->get_offset();
						for (sint16 x=0; x<size.x; x++) {
							for (sint16 y=0; y<size.y; y++) {
								if (const haus_tile_besch_t *tile = besch->get_tile(layout, x, y)) {
									if (grund_t *gr = welt->lookup_kartenboden(pos_hq + koord(x,y))) {
										if (gebaeude_t *gb = gr->find<gebaeude_t>()) {
											if (gb  &&  gb->get_besitzer()==sp  &&  prev_besch==gb->get_tile()->get_besch()) {
												gb->set_tile( tile, true );
											}
										}
									}
								}
							}
						}
						built = true;
					}

				}
			}
			// did not upgrade old one, need to remove it
			if (!built) {
				sp->add_headquarter( prev_hq->get_tile()->get_besch()->get_extra(), koord::invalid );
				// remove previous one
				hausbauer_t::remove( welt, sp, prev_hq );
				// resize cursor
				init(welt, sp);
			}
		}


		// build new one
		if (!built) {
			int rotate = 0;

			if(welt->square_is_free(pos.get_2d(), size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
				ok = true;
			}
			if(!ok  &&  besch->get_all_layouts()>1  &&  size.y != size.x  &&  welt->square_is_free(pos.get_2d(), size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
				rotate = 1;
				ok = true;
			}

			if(ok) {
				// then built it
				hq = hausbauer_t::baue(welt, sp, welt->lookup_kartenboden(pos.get_2d())->get_pos(), rotate, besch, NULL);
				stadt_t *city = welt->suche_naechste_stadt( pos.get_2d() );
				if(city) {
					city->add_gebaeude_to_stadt( hq );
				}
				built = true;
			}
			else {
				return "No suitable ground!";
			}
		}


		if (built) {
			// sometimes those are not correct after rotation ...
			sp->add_headquarter(besch->get_extra()+1, hq->get_pos().get_2d()-hq->get_tile()->get_offset() );
			spieler_t::book_construction_costs(sp,  cost, pos.get_2d(), ignore_wt);
			// tell the world of it ...
			cbuffer_t buf;
			buf.printf( translator::translate("%s s\nheadquarter now\nat (%i,%i)."), sp->get_name(), pos.x, pos.y );
			welt->get_message()->add_message( buf, pos.get_2d(), message_t::ai, PLAYER_FLAG|sp->get_player_nr(), hq->get_tile()->get_hintergrund(0,0,0) );
			// reset to query tool, since costly relocations should be avoided
			if(is_local_execution()  &&  sp == welt->get_active_player()) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], sp );
			}
			return NULL;
		}
	}
	return "";
}

const char *wkz_lock_game_t::work( karte_t *welt, spieler_t *, koord3d )
{
	// tool can never be executed in network mode
	if (umgebung_t::networkmode) {
		return "";
	}
	// as the result depends on the local locked state of public player
	if (welt->get_spieler(1)->is_locked() || !welt->get_settings().get_allow_player_change()) {
		return "Only public player can lock games!";
	}
	welt->clear_player_password_hashes();
	if(  !welt->get_spieler(1)->is_locked() ) {
		return "In order to lock the game, you have to protect the public player by password!";
	}
	welt->get_settings().set_allow_player_change(false);
	destroy_all_win( true );
	welt->switch_active_player( 0, true );
	welt->set_werkzeug( general_tool[WKZ_ABFRAGE], welt->get_spieler(0) );
	return NULL;
}


const char *wkz_add_citycar_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if( stadtauto_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = wkz_intern_koord_to_weg_grund( sp, welt, k, road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt))  &&  gr->find<stadtauto_t>() == NULL) {
		// add citycar
		stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), koord::invalid);
		gr->obj_add(vt);
		welt->sync_add(vt);
		return NULL;
	}
	return "";
}


uint8 wkz_forest_t::is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & )
{
	// do really nothing ...
	return 2;
}


void wkz_forest_t::mark_tiles( karte_t *welt, spieler_t *, const koord3d &start, const koord3d &end )
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord pos;
	for(  pos.x = k1.x;  pos.x <= k2.x;  pos.x++  ) {
		for(  pos.y = k1.y;  pos.y <= k2.y;  pos.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( pos );

			zeiger_t *marker = new zeiger_t( welt, gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner1(grund_hang), corner1(weg_hang)) +
					3 * max( corner2(grund_hang), corner2(weg_hang)) +
					9 * max( corner3(grund_hang), corner3(weg_hang)) +
					27 * max( corner4(grund_hang), corner4(weg_hang));
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_bild( grund_besch_t::marker->get_bild( grund_hang % 27 ) );
			marker->set_bild( grund_besch_t::marker->get_bild( back_hang ) );

			marker->mark_image_dirty( marker->get_bild(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *wkz_forest_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	koord wh, nw;
	wh.x = abs(end.x-start.x)+1;
	wh.y = abs(end.y-start.y)+1;
	nw.x = min(start.x, end.x)+(wh.x/2);
	nw.y = min(start.y, end.y)+(wh.y/2);

	sint64 costs = baum_t::create_forest( welt, nw, wh );
	spieler_t::book_construction_costs(sp, costs * welt->get_settings().cst_remove_tree, end.get_2d(), ignore_wt);

	return NULL;
}


image_id wkz_stop_moving_t::get_marker_image()
{
	return cursor;
}


void wkz_stop_moving_t::read_start_position(karte_t *welt, spieler_t *sp, const koord3d &pos)
{
	waytype[0] = invalid_wt;
	waytype[1] = invalid_wt;
	last_halt = halthandle_t();

	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		return;
	}
	// now assign waytypes
	if(bd->ist_wasser()) {
		waytype[0] = water_wt;
	}
	else {
		waytype[0] = bd->get_weg_nr(0)->get_waytype();
		if(bd->get_weg_nr(1)) {
			waytype[1] = bd->get_weg_nr(1)->get_waytype();
		}
	}
	// .. and halt
	last_halt = haltestelle_t::get_halt(welt,pos,sp);
}


uint8 wkz_stop_moving_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d &start)
{
	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		error = "";
		return 0;
	}
	// check halt ownership
	halthandle_t h = haltestelle_t::get_halt(welt,pos,sp);
	if(  h.is_bound()  &&  !spieler_t::check_owner( sp, h->get_besitzer() )  ) {
		error = "Das Feld gehoert\neinem anderen Spieler\n";
		return 0;
	}
	// check for halt on the tile
	if(  h.is_bound()  &&  !(bd->is_halt()  ||  (h->get_station_type()&haltestelle_t::dock  &&  bd->ist_wasser())  )  ) {
		error = "No suitable ground!";
		return 0;
	}

	if (start==koord3d::invalid) {
		// check for existing ways
		if (bd->ist_wasser()  ||  bd->hat_wege()) {
			return 2;
		}
		else {
			error = "No suitable ground!";
			return 0;
		}
	}
	else {
		// read conditions at start point
		read_start_position(welt, sp, start);
		// check halts vs waypoints
		if(h.is_bound() ^ last_halt.is_bound()) {
			error = "Can only move from halt to halt or waypoint to waypoint.";
			return 0;
		}
		// check waytypes
		if(  (waytype[0] == water_wt  &&  bd->ist_wasser())  ||  bd->hat_weg(waytype[0])  ||  bd->hat_weg(waytype[1])  ) {
			// ok
			return 2;
		}
		else {
			error = "No suitable ground!";
			return 0;
		}
	}
}

const char *wkz_stop_moving_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &last_pos, const koord3d &pos)
{
	// read conditions at start point
	read_start_position(welt, sp, last_pos);

	// second click
	grund_t *bd = welt->lookup(pos);
	halthandle_t h = haltestelle_t::get_halt(welt,pos,sp);

	if (bd) {
		const halthandle_t new_halt = h;
		// depending on the waytype we simply build replacements lists
		// in the worst case we have to iterate over all tiles twice ...
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
					uint8 ribi = welt->lookup(last_pos)->get_weg_ribi_unmasked(wt);
					koord delta = ribi_t::ist_gerade_ns(ribi) ? koord(0,1) : koord(1,0);
					koord3d start_pos=last_pos;
					while(ribi&12) {
						koord3d test_pos = start_pos+delta;
						grund_t *gr = welt->lookup(test_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
					// now add all of them
					while(ribi&3) {
						koord3d test_pos = start_pos-delta;
						grund_t *gr = welt->lookup(test_pos);
						old_platform.append(start_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
				}
			}

			// first, check convoi without line
			FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
				// check line and owner
				if(!cnv->get_line().is_bound()  &&  cnv->get_besitzer()==sp) {
					schedule_t *fpl = cnv->get_schedule();
					// check waytype
					if(fpl  &&  fpl->ist_halt_erlaubt(bd)) {
						bool updated = false;
						FOR(minivec_tpl<linieneintrag_t>, & k, fpl->eintrag) {
							if ((catch_all_halt && haltestelle_t::get_halt(welt, k.pos, cnv->get_besitzer()) == last_halt) ||
									old_platform.is_contained(k.pos)) {
								k.pos   = pos;
								updated = true;
							}
						}
						if(updated) {
							fpl->cleanup();
							// Knightly : remove lineless convoy from old stop
							if(  last_halt.is_bound()  ) {
								last_halt->remove_convoy(cnv);
							}
							// Knightly : register lineless convoy at new stop
							if(  new_halt.is_bound()  ) {
								new_halt->add_convoy(cnv);
							}
							if(  !fpl->ist_abgeschlossen()  ) {
								// schedule is not owned by schedule window ...
								// ... thus we can set this schedule
								cnv->set_schedule(fpl);
								// otherwise the schedule window will reset it
							}
						}
					}
				}
			}
			// next, check lines serving old_halt (no owner check needed for own lines ...
			vector_tpl<linehandle_t>lines;
			sp->simlinemgmt.get_lines(simline_t::line,&lines);
			FOR(vector_tpl<linehandle_t>, const line, lines) {
				schedule_t *fpl = line->get_schedule();
				// check waytype
				if(fpl->ist_halt_erlaubt(bd)) {
					bool updated = false;
					FOR(minivec_tpl<linieneintrag_t>, & k, fpl->eintrag) {
						// ok!
						if ((catch_all_halt && haltestelle_t::get_halt(welt, k.pos, line->get_besitzer()) == last_halt) ||
								old_platform.is_contained(k.pos)) {
							k.pos   = pos;
							updated = true;
						}
					}
					// update line
					if(updated) {
						fpl->cleanup();
						// remove line from old stop is needed at here
						if(last_halt.is_bound()) {
							last_halt->remove_line(line);
						}
						sp->simlinemgmt.update_line(line);
					}
				}
			}
		}
		// since factory connections may have changed
		welt->set_schedule_counter();
	}
	return NULL;
}


char const* wkz_daynight_level_t::get_tooltip(spieler_t const*) const
{
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  ||  default_param[0]=='-') {
			sprintf(toolstr, "%s %s",
			translator::translate("1LIGHT_CHOOSE"),
			&default_param[0]);
			return toolstr;
		}
		else {
			return translator::translate("Toggle day/night view");
		}
	}
	else {
		return "";
	}
}

bool wkz_daynight_level_t::init( karte_t *, spieler_t * ) {
	if(grund_t::underground_mode==grund_t::ugm_all  ||  umgebung_t::night_shift) {
		return false;
	}
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  &&  umgebung_t::daynight_level > 0) {
			// '+': fade in one level
			umgebung_t::daynight_level = umgebung_t::daynight_level-1;
		}
		else if (default_param[0]=='-') {
			// '-': fade out one level
			umgebung_t::daynight_level = umgebung_t::daynight_level+1;
		}
		else {
			// number: toggle number/0. 4 or 5 is good for night
			const sint8 level = atoi(default_param);
			umgebung_t::daynight_level = (umgebung_t::daynight_level==0) ? level : 0;
		}
	}
	return false;
}



/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
bool wkz_make_stop_public_t::init( karte_t *, spieler_t * )
{
	win_set_static_tooltip( NULL );
	return true;
}

const char* wkz_make_stop_public_t::get_tooltip(const spieler_t *sp) const
{
	sint32 const cost = (sp->get_welt()->get_settings().maint_building * 60);
	sprintf(toolstr, translator::translate("make stop public (or join with public stop next) costs %i per tile and level"), cost);
	return toolstr;
}

const char *wkz_make_stop_public_t::move( karte_t *welt, spieler_t *sp, uint16, koord3d p )
{
	win_set_static_tooltip( NULL );
	const planquadrat_t *pl = welt->lookup(p.get_2d());
	if(pl!=NULL) {
		halthandle_t halt = pl->get_halt();
		if(  halt.is_bound()  &&  spieler_t::check_owner(halt->get_besitzer(),sp)  &&  halt->get_besitzer()!=welt->get_spieler(1) ) {
			sint64 costs = halt->calc_maintenance();
			// set only tooltip if it costs (us)
			if(costs>0) {
				win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs*60 ) );
			}
		}
		else if(  const grund_t *gr = welt->lookup(p)  ) {
			if(  gr->get_typ()==grund_t::brueckenboden  ||  gr->get_grund_hang()!=hang_t::flach  ) {
				// not making ways public on bridges or slopes
				return "No suitable ground!";
			}
			weg_t *w = gr->get_weg_nr(0);
			// no need for action if already player(1) => XOR ...
			if(  !(w  &&  (  (w->get_besitzer()==sp)  ^  (sp==welt->get_spieler(1))  ))  ) {
				w = gr->get_weg_nr(1);
				if(  !(w  &&  (  (w->get_besitzer()==sp)  ^  (sp==welt->get_spieler(1))  ))  ) {
					w = NULL;
				}
			}
			if(  w  ) {
				// no public way with signs
				if(  w->has_sign()  ) {
					return "No suitable ground!";
				}
				sint64 costs = w->get_besch()->get_wartung();
				if(  gr->ist_im_tunnel()  ) {
					tunnel_t *t = gr->find<tunnel_t>();
					costs = t->get_besch()->get_wartung();
				}
				// set only tooltip if it costs (us)
				if(costs>0) {
					win_set_static_tooltip( tooltip_with_price("Building costs estimates", -costs*60 ) );
				}

			}
		}
		else {
			// boing ...
			return "";
		}
	}
	return NULL;
}

const char *wkz_make_stop_public_t::work( karte_t *welt, spieler_t *sp, koord3d p )
{
	const planquadrat_t *pl = welt->lookup(p.get_2d());
	if(  !pl  ||  !pl->get_halt().is_bound()  ||  pl->get_halt()->get_besitzer()==welt->get_spieler(1)  ) {
		weg_t *w = NULL;
		//convert a way here, if there is no halt or already public halt
		if(  const grund_t *gr = welt->lookup(p)  ) {
			if(  gr->get_typ()==grund_t::brueckenboden  ||  gr->get_grund_hang()!=hang_t::flach  ) {
				// not making ways public on bridges or slopes
				return "No suitable ground!";
			}
			w = gr->get_weg_nr(0);
			// no need for action if already player(1) => XOR ...
			if(  !(w  &&  (  (w->get_besitzer()==sp)  ^  (sp==welt->get_spieler(1))  ))  ) {
				w = gr->get_weg_nr(1);
				if(  !(w  &&  (  (w->get_besitzer()==sp)  ^  (sp==welt->get_spieler(1))  ))  ) {
					w = NULL;
				}
			}
			if(  w  ) {
				// no public way with signs
				if(  w->has_sign()  ) {
					return "No suitable ground!";
				}
				// change maintenance and ownership
				sint64 costs = w->get_besch()->get_wartung();
				if(  gr->ist_im_tunnel()  ) {
					tunnel_t *t = gr->find<tunnel_t>();
					costs = t->get_besch()->get_wartung();
					t->set_besitzer( welt->get_spieler(1) );
				}
				spieler_t::add_maintenance( w->get_besitzer(), -costs, w->get_besch()->get_finance_waytype() );
				spieler_t::book_construction_costs(   w->get_besitzer(), -costs*60, gr->get_pos().get_2d(), w->get_besch()->get_finance_waytype());
				spieler_t::book_construction_costs( welt->get_spieler(1), costs*60, koord::invalid, w->get_besch()->get_finance_waytype());
				w->set_besitzer( welt->get_spieler(1) );
				w->set_flag(ding_t::dirty);
				spieler_t::add_maintenance( welt->get_spieler(1), costs, w->get_besch()->get_finance_waytype() );
				// now search for wayobjects
				for(  uint8 i=1;  i<gr->get_top();  i++  ) {
					if(  wayobj_t *wo = ding_cast<wayobj_t>(gr->obj_bei(i))  ) {
						costs = wo->get_besch()->get_wartung();
						spieler_t::add_maintenance( wo->get_besitzer(), -costs, w->get_besch()->get_finance_waytype() );
						spieler_t::book_construction_costs(wo->get_besitzer(), -costs*60, gr->get_pos().get_2d(), w->get_waytype());
						wo->set_besitzer( welt->get_spieler(1) );
						wo->set_flag(ding_t::dirty);
						spieler_t::add_maintenance( welt->get_spieler(1), costs, w->get_besch()->get_finance_waytype() );
						spieler_t::book_construction_costs( welt->get_spieler(1), costs*60, koord::invalid, w->get_waytype());
					}
				}
				// and add message
				if(  sp->get_player_nr()!=1  &&  umgebung_t::networkmode  ) {
					cbuffer_t buf;
					buf.printf( translator::translate("(%s) now public way."), w->get_pos().get_str() );
					welt->get_message()->add_message( buf, w->get_pos().get_2d(), message_t::ai, PLAYER_FLAG|sp->get_player_nr(), IMG_LEER );
				}
			}
		}
		if(  w==NULL  ) {
			return "No stop here!";
		}
	}
	else {
		halthandle_t halt = pl->get_halt();
		if(  !(spieler_t::check_owner(halt->get_besitzer(),sp)  ||  halt->get_besitzer()==welt->get_spieler(1))  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		else {
			halt->make_public_and_join(sp);
		}
	}
	return NULL;
}



bool wkz_show_trees_t::init( karte_t *welt, spieler_t * )
{
	umgebung_t::hide_trees = !umgebung_t::hide_trees;
	baum_t::recalc_outline_color();
	welt->set_dirty();
	return false;
}


sint8 wkz_show_underground_t::save_underground_level = -128;

bool wkz_show_underground_t::init( karte_t *welt, spieler_t * )
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	sint8 old_underground_level = grund_t::underground_level;

	// map needs update?
	bool ok = true;
	// need an extra click?
	bool needs_click = false;

	// default default-param = U for backward compatibility
	if (default_param == NULL) {
		default_param = strdup("U");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				needs_click = true;
				ok = false;
			}
			else {
				ok = false;
			}
			break;
		// decrease slice level
		case 'D':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				if(  grund_t::underground_level>welt->get_grundwasser()-5  ) {
					grund_t::underground_level --;
				}
			}
			else {
				ok = false;
			}
			break;
		// increase slice level
		case 'I':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				if(  grund_t::underground_level<20  ) {
					grund_t::underground_level ++;
				}
			}
			else {
				ok = false;
			}
			break;

		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				// switch to normal or full-underground
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				grund_t::set_underground_mode( grund_t::ugm_level, zpos.z);
			}
			else {
				ok = false;
			}
			break;

		//  switch between full underground or normal/sliced view
		case 'U':
			if (grund_t::underground_mode==grund_t::ugm_all) {
				// check if the old level is valid then switch back to sliced view
				if (-128<save_underground_level && save_underground_level<127) {
					grund_t::set_underground_mode(grund_t::ugm_level, save_underground_level);
				}
				else {
					grund_t::set_underground_mode(grund_t::ugm_none, 0);
				}
			}
			else {
				grund_t::set_underground_mode( grund_t::ugm_all, 0);
			}
			break;

		default:
			ok = false;
			dbg->error( "wkz_show_underground_t::init()", "Unknown command string \"%s\"", default_param );

	}

	// move zeiger back
	welt->get_zeiger()->change_pos( zpos);

	if (ok) {
		save_underground_level = old_underground_level;

		// renew toolbar
		werkzeug_t::update_toolbars(welt);

		// recalc all images on map
		welt->update_map();
	}
	return needs_click;
}

const char *wkz_show_underground_t::work( karte_t *welt, spieler_t *sp, koord3d pos)
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	save_underground_level = grund_t::underground_level;
	grund_t::set_underground_mode( grund_t::ugm_level, pos.z);

	// move zeiger back
	welt->get_zeiger()->change_pos( zpos);

	// renew toolbar
	werkzeug_t::update_toolbars(welt);

	// recalc all images on map
	welt->update_map();

	if(sp == welt->get_active_player()) {
		welt->set_werkzeug( general_tool[WKZ_ABFRAGE], sp );
	}

	return NULL;
}


char const* wkz_show_underground_t::get_tooltip(spieler_t const*) const
{
	// no default-param == U for backward compatibility
	if(  default_param == NULL  ) {
		return translator::translate("underground mode");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return translator::translate("sliced underground mode");
		// decrease slice level
		case 'D':
			return translator::translate("decrease underground view level");
		// increase slice level
		case 'I':
			return translator::translate("increase underground view level");
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return translator::translate("sliced underground mode");
		//  switch between full underground or normal/sliced view
		case 'U':
		default:
			return translator::translate("underground mode");
	}
}

bool wkz_show_underground_t::is_selected(const karte_t *) const
{
	// default default-param = U for backward compatibility
	if(  default_param == NULL  ) {
		return grund_t::underground_mode==grund_t::ugm_all;
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return grund_t::underground_mode==grund_t::ugm_level;
		// decrease slice level
		case 'D':
			return false;
		// increase slice level
		case 'I':
			return false;
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return grund_t::underground_mode==grund_t::ugm_level;
		//  switch between full underground or normal/sliced view
		case 'U':
			return grund_t::underground_mode==grund_t::ugm_all;
	}
	return false;
}

void wkz_show_underground_t::draw_after(karte_t *welt, koord pos, bool dirty) const
{
	if(  icon!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( icon, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		// additionall show level in sliced mode
		if(  default_param!=NULL  &&  grund_t::underground_mode==grund_t::ugm_level  ) {
			char level_str[16];
			sprintf( level_str, "%i", grund_t::underground_level );
			display_proportional( pos.x+4, pos.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
		}
	}
}


bool wkz_rotate90_t::init( karte_t *welt, spieler_t * )
{
	if(  !umgebung_t::networkmode  ) {
		welt->rotate90();
		welt->update_map();
	}
	return false;
}


bool wkz_quit_t::init( karte_t *welt, spieler_t * )
{
	destroy_all_win( true );
	welt->stop( true );
	return false;
}


bool wkz_screenshot_t::init( karte_t *, spieler_t * )
{
	if(  is_ctrl_pressed()  ) {
		if(  const gui_frame_t * topwin = win_get_top()  ) {
			const koord pos = win_get_pos(topwin);
			const koord groesse = topwin->get_fenstergroesse();
			display_snapshot( pos.x, pos.y, groesse.x, groesse.y );
		}
		else {
			display_snapshot( 0, 0, display_get_width(), display_get_height() );
		}
	}
	else {
		display_snapshot( 0, 0, display_get_width(), display_get_height() );
	}
	create_win( new news_img("Screenshot\ngespeichert.\n"), w_time_delete, magic_none);
	return false;
}


bool wkz_undo_t::init( karte_t *, spieler_t *sp )
{
	if(!sp->undo()  &&  is_local_execution()) {
		create_win( new news_img("UNDO failed!"), w_time_delete, magic_none);
	}
	return false;
}


bool wkz_increase_industry_t::init( karte_t *welt, spieler_t * )
{
	fabrikbauer_t::increase_industry_density( welt, false );
	return false;
}


bool wkz_zoom_in_t::init( karte_t *welt, spieler_t * )
{
	win_change_zoom_factor(true);
	welt->set_dirty();
	return false;
}


bool wkz_zoom_out_t::init( karte_t *welt, spieler_t * )
{
	win_change_zoom_factor(false);
	welt->set_dirty();
	return false;
}

/************************* internal tools, only need for networking ***************/

static bool scenario_check_schedule(karte_t *welt, spieler_t *sp, schedule_t *schedule, bool local)
{
	if (!is_scenario()) {
		return true;
	}
	const char* err = welt->get_scenario()->is_schedule_allowed(sp, schedule);
	if (err) {
		if (*err  &&  local) {
			create_win( new news_img(err), w_time_delete, magic_none);
		}
		return false;
	}
	return true;
}


/* Handles all action of convois in depots. Needs a default param:
 * [function],[convoi_id],addition stuff
 * following simple command exists:
 * 'x' : self destruct
 * 'f' : open the schedule window
 * 'g' : apply a schedule
 * 'n' : toggle 'no load'
 * 'w' : toggle withdraw
 * 's' : change state to [number] (and maybe set open schedule flag)
 * 'l' : apply new line [number]
 */
bool wkz_change_convoi_t::init( karte_t *welt, spieler_t *sp )
{
	char tool=0;
	uint16 convoi_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi", &tool, &convoi_id );

	// skip to the commands ...
	for(  int z = 2;  *p  &&  z>0;  p++  ) {
		if(  *p==','  ) {
			z--;
		}
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );
	// double click on remove button will send two such commands
	// the first will delete the convoi, the second should not trigger an assertion
	// catch such commands here
	if( !cnv.is_bound()) {
#if DEBUG>=4
		if (is_local_execution()) {
			create_win( new news_img("Convoy already deleted!"), w_time_delete, magic_none);
		}
#endif
		dbg->warning("wkz_change_convoi_t::init", "no convoy with id=%d found", convoi_id);
		return false;
	}
	// ownership check for network games
	if (cnv.is_bound()  &&  umgebung_t::networkmode  &&  !spieler_t::check_owner(cnv->get_besitzer(), sp)) {
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'x': // self destruction ...
			if(cnv.is_bound()) {
				if (cnv->get_state()==convoi_t::INITIAL) {
					// delete cnv in depot
					if (grund_t *gr = welt->lookup(cnv->get_pos())) {
						if (depot_t *dep = gr->get_depot()) {
							dep->disassemble_convoi(cnv, true);
							return false;
						}
					}
				}
				cnv->self_destruct();
			}
			return false;

		case 'f': // open schedule
			{
				// we open the window only when executed on the same client that triggered the tool
				// but the all clients must call the function anyway
				cnv->open_schedule_window( is_local_execution() );
			}
			break;

		case 'g': // change schedule
			{
				schedule_t *fpl = cnv->create_schedule()->copy();
				fpl->eingabe_abschliessen();
				if (fpl->sscanf_schedule( p )  &&  scenario_check_schedule(welt, sp, fpl, is_local_execution())) {
					cnv->set_schedule( fpl );
				}
				else {
					// could not read schedule, do not assign
					delete fpl;
				}
			}
			break;

		case 'l': // change line
			{
				// read out id and new aktuell index
				uint16 id=0, aktuell=0;
				int count=sscanf( p, "%hi,%hi", &id, &aktuell );
				linehandle_t l;
				l.set_id( id );
				if(  l.is_bound()  ) {
					// sanity check for right line-type (compare schedule types ..)
					schedule_t *fpl = cnv->create_schedule();
					if(  fpl  &&  l->get_schedule()  &&  fpl->get_type()!=l->get_schedule()->get_type()  ) {
						dbg->warning("wkz_change_convoi_t::init", "types of convoi and line do not match");
						return false;
					}
					if(  count==1 ) {
						// aktuell was not supplied -> take it from line schedule
						aktuell = l->get_schedule()->get_aktuell();
					}
					cnv->set_line( l );
					cnv->get_schedule()->set_aktuell((uint8)aktuell);
					cnv->get_schedule()->eingabe_abschliessen();
				}
			}
			break;

		case 'n': // change no_load
			cnv->set_no_load( !cnv->get_no_load() );
			if(  !cnv->get_no_load()  ) {
				cnv->set_withdraw( false );
			}
			break;

		case 's': // change state
			{
				int new_state = atoi(p);
				if(  new_state>0  ) {
					cnv->set_state( new_state );
					if(  new_state==convoi_t::FAHRPLANEINGABE  ) {
						cnv->get_schedule()->eingabe_beginnen();
					}
				}
			}
			break;

		case 'w': // change withdraw
			cnv->set_withdraw( !cnv->get_withdraw() );
			cnv->set_no_load( cnv->get_withdraw() );
			break;
	}

	if(  cnv->in_depot()  &&  (tool=='g'  ||  tool=='l')  ) {
		const grund_t *const ground = welt->lookup( cnv->get_home_depot() );
		if(  ground  ) {
			const depot_t *const depot = ground->get_depot();
			if(  depot  ) {
				depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
				if(  frame  ) {
					frame->update_data();
				}
			}
		}
	}


	return false;	// no related work tool ...
}



/* Handles all action of lines. Needs a default param:
 * [function],[line_id],addition stuff
 * following simple command exists:
 * 'g' : apply new schedule to line [schedule follows]
 */
bool wkz_change_line_t::init( karte_t *welt, spieler_t *sp )
{
	uint16 line_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}

	char tool=*p++;
	while(  *p  &&  *p++!=','  ) {
	}
	if(  *p==0  ) {
		dbg->warning( "wkz_change_line_t::init()", "too short command \"%s\"", default_param );
		return false;
	}

	line_id = atoi(p);
	while(  *p  &&  *p++!=','  ) {
	}

	linehandle_t line;
	line.set_id( line_id );

	// ownership check for network games
	if (line.is_bound()  &&  umgebung_t::networkmode  && !spieler_t::check_owner(line->get_besitzer(), sp)) {
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'c': // create line, next paraemter line type and magic of schedule window (only right window gets updated)
			{
				line = sp->simlinemgmt.create_line( atoi(p), sp );
				while(  *p  &&  *p++!=','  ) {
				}
				long t;
				sscanf( p, "%ld", &t );
				while(  *p  &&  *p++!=','  ) {
				}

				// no need to check schedule for scenario conditions, as schedule is only copied
				line->get_schedule()->sscanf_schedule( p );
				if (is_local_execution()) {
					fahrplan_gui_t *fg = dynamic_cast<fahrplan_gui_t *>(win_get_magic((ptrdiff_t)t));
					if(  fg  ) {
						fg->init_line_selector();
					}
					schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+sp->get_player_nr()));
					if(  sl  ) {
						sl->show_lineinfo( line );
					}
					// no schedule window open => then open one
					if(  fg==NULL  ) {
						create_win( new line_management_gui_t(line, sp), w_info, (ptrdiff_t)line.get_rep() );
					}
				}
			}
			break;

		case 'd':	// delete line
			{
				if (line.is_bound()  &&  line->count_convoys()==0) {
					// close a schedule window, if still active
					gui_frame_t *w = win_get_magic( (ptrdiff_t)line.get_rep() );
					if(w) {
						destroy_win( w );
					}
					sp->simlinemgmt.delete_line(line);
				}
			}
			break;

		case 'g': // change schedule
			{
				if (line.is_bound()) {
					schedule_t *fpl = line->get_schedule()->copy();
					if (fpl->sscanf_schedule( p )  &&  scenario_check_schedule(welt, sp, fpl, is_local_execution()) ) {
						fpl->eingabe_abschliessen();
						line->set_schedule( fpl );
						line->get_besitzer()->simlinemgmt.update_line(line);
					}
					else {
						// could not read schedule, do not assign
						delete fpl;
					}
				}
			}
			break;

		case 'u':	// unite all lineless convois with similar schedules
			{
				array_tpl<vector_tpl<convoihandle_t> > cnvs(welt->convoys().get_count());
				uint32 max_cnvs=0;
				FOR(vector_tpl<convoihandle_t>, cnv, welt->convoys()) {
					// only check lineless convoys
					if(  !cnv->get_line().is_bound()  ) {
						bool found = false;
						// check, if already matches existing convois schedule
						for(  uint32 i=0;  i<max_cnvs  &&  !found;  i++  ) {
							FOR(vector_tpl<convoihandle_t>, cnvcomp, cnvs[i] ) {
								if(  cnvcomp->get_schedule()->matches( welt, cnv->get_schedule() )  ) {
									found = true;
									cnvs[i].append( cnv );
									break;
								}
							}
						}
						// not added: then may be new line for this?
						if(  !found  ) {
							cnvs[max_cnvs++].append( cnv );
						}
					}
				}
				// now we have an array of one or more lineless convois
				for(  uint32 i=0;  i<max_cnvs;  i++  ) {
					// if there is more than one convois => new line
					if(  cnvs[i].get_count()>1  ) {
						line = sp->simlinemgmt.create_line( cnvs[i][0]->get_schedule()->get_type(), sp, cnvs[i][0]->get_schedule() );
						FOR(vector_tpl<convoihandle_t>, cnv, cnvs[i] ) {
							line->add_convoy( cnv );
							cnv->set_line( line );
						}
					}
				}
			}
			break;

		case 'w': // change widthdraw
			{
				if (line.is_bound()) {
					line->set_withdraw( atoi(p) );
				}
			}
			break;
	}
	return false;
}



/* Handles all action of convois in depots. Needs a default param:
 * [function],[depot_pos_3d],[convoi_id],addition stuff
 * following simple command exists:
 * 'l' : creates a new line (convoi_id might be invalid) (+printf'd initial schedule)
 * 'b' : starts the convoi
 * 'B' : starts all convoys
 * 'c' : copies this convoi
 * 'd' : dissassembles convoi
 * 's' : sells convoi
 * 'a' : appends a vehicle (+vehikel_name) uses the oldest
 * 'i' : inserts a vehicle in front (+vehikel_name) uses the oldest
 * 's' : sells a vehikel (+vehikel_name) uses the newest
 * 'r' : removes a vehikel (+number in convoi)
 * 'R' : removes all vehikels including (+number in convoi) to end
 */
bool wkz_change_depot_t::init( karte_t *welt, spieler_t *sp )
{
	char tool=0;
	koord3d pos = koord3d::invalid;
	sint16 z;
	uint16 convoi_id;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi,%hi,%hi,%hi", &tool, &pos.x, &pos.y, &z, &convoi_id );
	pos.z = (sint8)z;

	// skip to the commands ...
	z = 5;
	while(  *p  &&  z>0  ) {
		if(  *p==','  ) {
			z--;
		}
		p++;
	}

	grund_t *gr = welt->lookup(pos);
	depot_t *depot = gr ? gr->get_depot() : NULL;
	if(  depot==NULL  ){
		dbg->warning("wkz_change_depot_t::init", "no depot found at (%s)", pos.get_str());
		return false;
	}
	if(  !spieler_t::check_owner( depot->get_besitzer(), sp)  ) {
		dbg->warning("wkz_change_depot_t::init", "depot at (%s) belongs to another player", pos.get_str());
		return false;
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );

	// ok now do our stuff
	switch(  tool  ) {
		case 'l': { // create line schedule window
			linehandle_t selected_line = depot->get_besitzer()->simlinemgmt.create_line(depot->get_line_type(),depot->get_besitzer());
			// no need to check schedule for scenario conditions, as schedule is only copied
			selected_line->get_schedule()->sscanf_schedule( p );

			depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)depot ));
			if(  is_local_execution()  ) {
				if(  welt->get_active_player()==sp  &&  depot_frame  ) {
					create_win( new line_management_gui_t( selected_line, depot->get_besitzer() ), w_info, (ptrdiff_t)selected_line.get_rep() );
				}
			}

			if(  depot_frame  ) {
				if(  is_local_execution()  ) {
					depot_frame->set_selected_line( selected_line );
					depot_frame->apply_line();
				}
				depot_frame->update_data();
			}

			schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic( magic_line_management_t + sp->get_player_nr() ));
			if(  sl  ) {
				sl->update_data( selected_line );
			}
			DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
			break;
		}
		case 'b': { // start a convoi from the depot
			if(  cnv.is_bound()  ) {
				depot->start_convoi(cnv, is_local_execution());
			}
			break;
		}
		case 'B': { // start all convoys
			depot->start_all_convoys();
			break;
		}
		case 'd':   // disassemble convoi
		case 'v': { // sell convoi
			depot->disassemble_convoi( cnv, tool=='v' );
			break;
		}
		case 'c': { // copy this convoi
			if(  cnv.is_bound()  ) {
				if(  convoihandle_t::is_exhausted()  ) {
					if(  is_local_execution()  ) {
						create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none );
					}
					return false;
				}
				depot->copy_convoi( cnv, is_local_execution() );
			}
			break;
		}
		case 'a':   // append a vehicle
		case 'i':   // insert a vehicle in front
		case 's':   // sells a vehicle
		case 'r':   // removes a vehicle (assumes a valid depot)
		case 'R': { // removes all vehicles to end (assumes a valid depot)
			if(  tool=='r'  ||  tool=='R'  ) {
				// test may fail after double-click on the button:
				// two remove cmds are sent, only the first will remove, the second should not trigger assertion failure
				if ( cnv.is_bound() ) {
					int start_nr = atoi(p);
					int nr = start_nr;

					// find end
					while(nr<cnv->get_vehikel_anzahl()) {
						const vehikel_besch_t *info = cnv->get_vehikel(nr)->get_besch();
						nr ++;
						if(info->get_nachfolger_count()!=1) {
							break;
						}
					}
					// now remove the vehicles
					if(  cnv->get_vehikel_anzahl()==nr-start_nr  ||  (tool=='R'  &&  start_nr==0)  ) {
						depot->disassemble_convoi(cnv, false);
					}
					else if(  tool=='R'  ) {
						depot->remove_vehicles_to_end( cnv, start_nr );
					}
					else {
						for(  int i=start_nr;  i<nr;  i++  ) {
							depot->remove_vehicle(cnv, start_nr);
						}
					}
				}
			}
			else {
				// create and append it
				const vehikel_besch_t *info = vehikelbauer_t::get_info( p );
				// we have a valid vehicle there => now check for details
				if(  info  ) {
					// we buy/sell all vehicles together!
					slist_tpl<const vehikel_besch_t *>new_vehicle_info;
					const vehikel_besch_t *start_info = info;

					if(tool!='a') {
						// start of composition
						while (info->get_vorgaenger_count() == 1 && info->get_vorgaenger(0) != NULL  &&  !new_vehicle_info.is_contained(info)) {
							info = info->get_vorgaenger(0);
							new_vehicle_info.insert(info);
						}
						info = start_info;
					}
					while(info) {
						new_vehicle_info.append( info );
						if(info->get_nachfolger_count()!=1  ||  (tool=='i'  &&  info==start_info)  ||  new_vehicle_info.is_contained(info->get_nachfolger(0))) {
							break;
						}
						info = info->get_nachfolger(0);
					}
					// now we have a valid composition together
					if(  tool=='s'  ) {
						while(new_vehicle_info.get_count()) {
							// We sell the newest vehicle - gives most money back.
							vehikel_t* veh = depot->find_oldest_newest(new_vehicle_info.remove_first(), false);
							if(veh != NULL) {
								depot->sell_vehicle(veh);
							}
						}
					}
					else {
						// append/insert into convoi; create one if needed
						depot->clear_command_pending();
						if(!cnv.is_bound()) {
							if(  convoihandle_t::is_exhausted()  ) {
								if (is_local_execution()) {
									create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none);
								}
								return false;
							}
							// create a new convoi
							cnv = depot->add_convoi( is_local_execution() );
							cnv->set_name(new_vehicle_info.front()->get_name());
						}

						// now we have a valid cnv
						if(cnv->get_vehikel_anzahl()+new_vehicle_info.get_count() <= depot->get_max_convoi_length()) {

							for(  unsigned i=0;  i<new_vehicle_info.get_count();  i++  ) {
								// insert/append needs reverse order
								unsigned nr = (tool=='i') ? new_vehicle_info.get_count()-i-1 : i;
								// We add the oldest vehicle - newer stay for selling
								const vehikel_besch_t* vb = new_vehicle_info.at(nr);
								vehikel_t* veh = depot->find_oldest_newest(vb, true);
								if (veh == NULL) {
									// nothing there => we buy it
									veh = depot->buy_vehicle(vb);
								}
								depot->append_vehicle( cnv, veh, tool=='i', is_local_execution() );
							}
						}
					}
				}
			}
			depot->update_win();
			break;
		}
	}
	return false;
}


/* Handles all player stuff default_param:
 * [function],[player_id],[state]
 * following command exists:
 * 'a' : activate/deactivate player (depends on state)
 * 'n' : create player at id of type state
 * 'f' : activates/deactivates freeplay
 * 'c' : change player color
 */
bool wkz_change_player_t::init( karte_t *welt, spieler_t *sp)
{
	if(  default_param==NULL  ) {
		dbg->warning( "wkz_change_player_t::init()", "nothing to do!" );
		return false;
	}

	char tool=0;
	int id=0;
	int state=0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%i,%i", &tool, &id, &state );

	spieler_t *player = welt->get_spieler(id);

	// ok now do our stuff
	switch(  tool  ) {
		case 'a': // activate/deactivate AI
			if(  player  &&  player->get_ai_id()!=spieler_t::HUMAN  &&  (sp==welt->get_spieler(1)  ||  !umgebung_t::networkmode)  ) {
				player->set_active(state);
				welt->get_settings().set_player_active(id, player->is_active());
			}
			break;
		case 'c': // change player color
			if(  player  &&  player==sp  ) {
				int c1, c2, dummy;
				sscanf( p, "%c,%i,%i,%i", &tool, &dummy, &c1, &c2 );
				player->set_player_color( c1, c2 );
			}
			break;
		case 'n': // WAS: new player with type state
		case 'f': // WAS: activate/deactivate freeplay
			dbg->error( "wkz_change_player_t::init()", "deprecated command called" );
			break;
	}

	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if (playerwin) {
		playerwin->update_data();
	}

	return false;
}


/* Sets traffic light phases via default_param:
 * [pos],[ns_flag],[ticks]
 */
bool wkz_change_traffic_light_t::init( karte_t *welt, spieler_t *sp )
{
	koord3d pos;
	sint16 z, ns, ticks;
	if(  5!=sscanf( default_param, "%hi,%hi,%hi,%hi,%hi", &pos.x, &pos.y, &z, &ns, &ticks )  ) {
		return false;
	}
	pos.z = (sint8)z;
	if(  grund_t *gr = welt->lookup(pos)  ) {
		if( roadsign_t *rs = gr->find<roadsign_t>()  ) {
			if(  (  rs->get_besch()->is_traffic_light()  ||  rs->get_besch()->is_private_way()  )  &&  spieler_t::check_owner(rs->get_besitzer(),sp)  ) {
				if(  ns == 1  ) {
					rs->set_ticks_ns( (uint8)ticks );
				}
				else if(  ns == 0  ) {
					rs->set_ticks_ow( (uint8)ticks );
				}
				else if(  ns == 2  ) {
					rs->set_ticks_offset( (uint8)ticks );
				}
				// update the window
				if(  rs->get_besch()->is_traffic_light()  ) {
					trafficlight_info_t* trafficlight_win = (trafficlight_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
				else {
					privatesign_info_t* trafficlight_win = (privatesign_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
			}
		}
	}
	return false;
}


/**
 * change city:
 * g[x],[y],[allow_city_growth]
 */
bool wkz_change_city_t::init( karte_t *welt, spieler_t *sp )
{
	if (sp != welt->get_spieler(1)) {
		return false;
	}
	koord pos;
	sint16 allow_growth;
	if(  3!=sscanf( default_param, "g%hi,%hi,%hi", &pos.x, &pos.y, &allow_growth )  ) {
		return false;
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if (gr) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if (gb) {
			stadt_t *st = gb->get_stadt();
			if (st) {
				st->set_citygrowth_yesno(allow_growth);
				stadt_info_t *stinfo = dynamic_cast<stadt_info_t*>(win_get_magic((ptrdiff_t)st));
				if (stinfo) {
					stinfo->update_data();
				}
			}
		}
	}
	return false;
}



/* Handles renaming of ingame entities. Needs a default param:
 * [object='c|h|l|m|t|p|f'][id|pos],[name]
 * c=convoi, h=halt, l=line,  m=marker, t=town, p=player, f=factory
 * in case of marker / factory, id is a pos3d string
 */
bool wkz_rename_t::init(karte_t* const welt, spieler_t *sp)
{
	uint16 id = 0;
	koord3d pos = koord3d::invalid;

	// skip the rest of the command
	const char *p = default_param;
	const char what = *p++;
	switch(  what  ) {
		case 'h':
		case 'l':
		case 'c':
		case 't':
		case 'p':
			id = atoi(p);
			while(  *p>0  &&  *p++!=','  ) {
			}
			break;
		case 'm':
		case 'f':
			if(  3!=sscanf( p, "%hi,%hi,%hi", &pos.x, &pos.y, &id )  ) {
				dbg->error( "wkz_rename_t::init", "no position given for marker/factory! (%s)", default_param );
				return false;
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			pos.z = (sint8)id;
			id = 0;
			break;
		default:
			dbg->error( "wkz_rename_t::init", "illegal request! (%s)", default_param );
			return false;
	}

	// now for action ...
	switch(  what  ) {
		case 'h':
		{
			halthandle_t halt;
			halt.set_id( id );
			if(  halt.is_bound()  &&  (!umgebung_t::networkmode  ||  spieler_t::check_owner(halt->get_besitzer(), sp))  ) {
				halt->set_name( p );
				return false;
			}
			break;
		}

		case 'l':
		{
			linehandle_t line;
			line.set_id( id );
			if(  line.is_bound()  &&  (!umgebung_t::networkmode  ||  spieler_t::check_owner(line->get_besitzer(), sp))  ) {
				line->set_name( p );

				schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+sp->get_player_nr()));
				if(  sl  ) {
					sl->update_data( line );
				}
				return false;
			}
			break;
		}

		case 'c':
		{
			convoihandle_t cnv;
			cnv.set_id( id );
			if(  cnv.is_bound()  &&  (!umgebung_t::networkmode  ||  spieler_t::check_owner(cnv->get_besitzer(), sp))  ) {
				//  set name without ID
				cnv->set_name( p, false );
				return false;
			}
			break;
		}

		case 't':
		{
			if(  sp == welt->get_spieler(1)  &&   id<welt->get_staedte().get_count()  ) {
				welt->get_staedte()[id]->set_name( p );
				return false;
			}
			break;
		}

		case 'm':
			if(  grund_t *gr = welt->lookup(pos)  ) {
				label_t *label = gr->find<label_t>();
				if (label  &&  (!umgebung_t::networkmode  ||  spieler_t::check_owner(label->get_besitzer(), sp))  ) {
					gr->set_text(p);
				}
				return false;
			}
			break;

		case 'p': {
			spieler_t *other = welt->get_spieler((uint8)id);
			if(  other  &&  other == sp  ) {
				other->set_name(p);
				return false;
			}
		}

		case 'f':
		{
			if(  sp == welt->get_spieler(1)) {
				if(  grund_t *gr = welt->lookup(pos)  ) {
					if(  gebaeude_t* gb = gr->find<gebaeude_t>()  ) {
						if (  fabrik_t *fab = gb->get_fabrik()  ) {
							fab->set_name(p);
							return false;
						}
					}
				}
			}
		}
	}
	// we are only getting here, if we could not process this request
	dbg->warning( "wkz_rename_t::init", "could not perform (%s)", default_param );
	return false;
}


/*
 * Add a message to the message queue
 */
bool wkz_add_message_t::init (karte_t* welt, spieler_t*)
{
	if (  *default_param  ) {
		// Local message, not stored by server
		welt->get_message()->add_message( default_param, koord::invalid, message_t::general | message_t::local_flag, COL_BLACK, IMG_LEER );
	}
	return false;
}
