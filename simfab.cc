/*
 * simfab.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simfab.cc
 *
 * Fabrikfunktionen und Fabrikbau
 *
 * Hansjörg Malthaner
 *
 *
 * 25.03.00 Anpassung der Lagerkapazitäten: min. 5 normale Lieferungen
 *          sollten an Lager gehalten werden.
 */

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdebug.h"
#include "simio.h"
#include "simimg.h"
#include "boden/grund.h"
#include "boden/wege/dock.h"
#include "simfab.h"
#include "simcity.h"
#include "simhalt.h"
#include "simskin.h"
#include "simworld.h"
#include "besch/haus_besch.h"
#include "besch/ware_besch.h"
#include "simplay.h"
#include "simtools.h"
#include "simmem.h"

#include "simintr.h"

#include "dings/raucher.h"
#include "dings/gebaeude.h"
//#include "dings/leitung2.h"

#include "dataobj/einstellungen.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "besch/skin_besch.h"
#include "besch/fabrik_besch.h"

#include "bauer/warenbauer.h"
#include "bauer/fabrikbauer.h"

#include "tpl/stringhashtable_tpl.h"

#include "utils/cbuffer_t.h"

#include "simgraph.h"
#include "simdisplay.h"

// Fabrik_t


static const int FAB_MAX_INPUT = 15000;


fabrik_t * fabrik_t::gib_fab(const karte_t *welt, const koord pos)
{
  const planquadrat_t *plan = welt->lookup(pos);

  if(plan) {
    const grund_t *gr = plan->gib_kartenboden();

    if(gr) {

      for(int n=0; n<gr->gib_top(); n++) {
	ding_t * dt = gr->obj_bei(n);

	if(dt && dt->fabrik()) {

	  // printf("gib_fab(): Factory at %d,%d is %p\n", pos.x, pos.y, dt->fabrik());

	  // wir haben alle Daten, liefere halt
	  return dt->fabrik();
	}
      }
    }
  }

  // printf("gib_fab(): No Factory at %d,%d\n", pos.x, pos.y);

  // not found
  return 0;
}



void fabrik_t::link_halt(halthandle_t halt)
{
  if(halt_list.contains(halt) == false) {
    halt_list.insert(halt);
    welt->lookup(pos)->add_to_haltlist(halt);
  }
}



void fabrik_t::unlink_halt(halthandle_t halt)
{
  halt_list.remove(halt);
  welt->lookup(pos)->remove_from_haltlist(halt);
}


void
fabrik_t::add_lieferziel(koord ziel)
{
    if(lieferziele.get_count() < max_lieferziele) {
		lieferziele.append_unique(ziel);
		fabrik_t * fab = fabrik_t::gib_fab(welt, ziel);
		if (fab) {
			fab->add_supplier(gib_pos().gib_2d());
		}
    }
}


fabrik_t::fabrik_t(karte_t *wl, loadsave_t *file) : lieferziele(max_lieferziele), suppliers(max_suppliers)
{
    welt = wl;

    eingang = NULL;
    ausgang = NULL;

    abgabe_sum = NULL;
    abgabe_letzt = NULL;

    // fixme: real size isn't known here
    vector_tpl<ware_t> * eingang_tmp = new vector_tpl<ware_t> (16);
    vector_tpl<ware_t> * ausgang_tmp = new vector_tpl<ware_t> (16);

    set_eingang( eingang_tmp );
    set_ausgang( ausgang_tmp );

    // abgabe_sum = new array_tpl<int>(16);
    // abgabe_letzt = new array_tpl<int>(16);

    besitzer_p = NULL;

    rdwr(file);
    aktionszeit = 0;
#ifdef FAB_PAX
    pax_zeit = 0;
    pax_intervall = 262144/besch->gib_pax_level();
#endif
    delta_sum = 0;
}


fabrik_t::fabrik_t(karte_t *wl, koord3d pos, spieler_t *spieler, const fabrik_besch_t *fabesch) : lieferziele(max_lieferziele), suppliers(max_suppliers)
{
    this->pos = pos;
    besch = fabesch;

    welt = wl;

    besitzer_p = spieler;
    prodfaktor = 16;
    prodbase = 0;

    eingang = NULL;
    ausgang = NULL;

    abgabe_sum = NULL;
    abgabe_letzt = NULL;

    aktionszeit = 0;
#ifdef FAB_PAX
    pax_zeit = 0;
    pax_intervall = 262144/besch->gib_pax_level();
#endif
    delta_sum = 0;
}


fabrik_t::~fabrik_t()
{
	// prissi: check added
	if(eingang) {
		delete eingang;
	}
	eingang = 0;

	if(ausgang) {
		delete ausgang;
	}
	ausgang = 0;

	if(abgabe_sum) {
		delete abgabe_sum;
	}
	abgabe_sum = 0;

	if(abgabe_letzt) {
		delete abgabe_letzt;
	}
	abgabe_letzt = 0;
}


void
fabrik_t::add_arbeiterziel(stadt_t *stadt)
{
	// do not add a city twice!
	if(!arbeiterziele.contains(stadt)) {
		arbeiterziele.insert(stadt);
	}
}


/**
 * Baut vier Gebäude für die Fabrik
 *
 * @author Hj. Malthaner
 */
void
fabrik_t::baue(int rotate, bool clear)
{
  if(besch) {
    this->rotate = rotate;
     hausbauer_t::baue(welt, besitzer_p, pos, rotate, besch->gib_haus(), clear, this);

    const rauch_besch_t *rada = besch->gib_rauch();
    if(rada) {
	const koord3d k ( pos + rada->gib_pos_off() );
	welt->lookup(k)->obj_add(new raucher_t(welt, k, rada));
    }
  } else {
    dbg->error("fabrik_t::baue()", "Good pak not available!");
  }
}

bool
fabrik_t::ist_bauplatz(karte_t *welt, koord pos, koord groesse,bool wasser)
{
    bool ok = false;
    if(pos.x > 0 && pos.y > 0 &&
       pos.x+groesse.x < welt->gib_groesse() && pos.y+groesse.y < welt->gib_groesse() &&
       ( wasser  ||  welt->ist_platz_frei(pos, groesse.x, groesse.y) )&&
       !ist_da_eine(welt, pos.x-5, pos.y-5, pos.x+groesse.x+3, pos.y+groesse.y+3)) {

	ok = true;
		// check for water (no shore in sight!)
		if(wasser) {
			for(int y=0;y<groesse.y;y++) {
				for(int x=0;x<groesse.x;x++) {
					const grund_t *gr=welt->lookup(pos+koord(x,y))->gib_kartenboden();
					if(!gr->ist_wasser()  ||  gr->gib_grund_hang()!=hang_t::flach) {
						return false;
					}
				}
			}
		}

    } else {
	ok = false;
    }
//if(welt->ist_in_kartengrenzen(pos) &&  wasser)
//DBG_MESSAGE("fabrik_t::ist_bauplatz()","(%i,%i) is%s water => %s",pos.x,pos.y,welt->lookup(pos)->gib_kartenboden()->ist_wasser()?"":" not",ok?"ok":"error");

    return ok;
}



vector_tpl<fabrik_t *> &
fabrik_t::sind_da_welche(karte_t *welt, int minX, int minY, int maxX, int maxY)
{
    int i,j;
    static vector_tpl <fabrik_t*> fablist(32);

    fablist.clear();

    for(j=minY; j<=maxY; j++) {
	for(i=minX; i<=maxX; i++) {
	    const koord pos(i,j);
	    const planquadrat_t * plan = welt->lookup(pos);
	    if(plan) {
		const grund_t *gr = plan->gib_kartenboden();

		for(int n=0; gr && n<gr->gib_top(); n++) {
		    ding_t *dt = gr->obj_bei(n);

		   if(dt != NULL && dt->fabrik() != NULL) {

			fabrik_t * fab = dt->fabrik();

			// fabrik nur einmal in liste eintragen
			fablist.append_unique( fab );
DBG_MESSAGE("fabrik_t::sind_da_welche()","appended factory %s at (%i,%i)",fab->gib_besch()->gib_name(),i,j);

		    }
		}
	    }
	}
    }

    return fablist;
}

bool
fabrik_t::ist_da_eine(karte_t *welt, int minX, int minY, int maxX, int maxY)
{
    int i,j;

    for(j=minY; j<=maxY; j++) {
	for(i=minX; i<=maxX; i++) {
	    if(welt->ist_in_kartengrenzen(i, j)) {
	        const koord pos(i,j);
		const grund_t *gr = welt->lookup(pos)->gib_kartenboden();

		for(int n=0; n<gr->gib_top(); n++) {

		    if(gr->obj_bei(n) != NULL &&
		       gr->obj_bei(n)->fabrik() != NULL) {

			return true;
		    }
		}
	    }
	}
    }

    return false;
}


void
fabrik_t::rdwr(loadsave_t *file)
{
	int i;
	int spieler_n;
	int eingang_count;
	int ausgang_count;
	int anz_lieferziele;
	const char *s = NULL;

	if(file->is_saving()) {
		eingang_count = eingang->get_count();
		ausgang_count = ausgang->get_count();
		anz_lieferziele = lieferziele.get_count();
		s = gib_name();
	}
	file->rdwr_str(s, "-");
	if(file->is_loading()) {
		besch = fabrikbauer_t::gib_fabesch(s);
		guarded_free(const_cast<char *>(s));
	}
	pos.rdwr(file);

	file->rdwr_delim("Bau: ");
	file->rdwr_char(rotate, "\n");

	// now rebuilt information for recieved goods
	file->rdwr_int(eingang_count, "\n");
	for(i=0; i<eingang_count; i++) {
		ware_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = eingang->at(i).gib_typ()->gib_name();
			dummy.menge = eingang->at(i).menge;
			dummy.max = eingang->at(i).max;
		}

		file->rdwr_delim("Ein: ");
		file->rdwr_str(typ, " ");
		file->rdwr_int(dummy.menge, " ");
		file->rdwr_int(dummy.max, "\n");
		if(file->is_loading()) {
			dummy.setze_typ( warenbauer_t::gib_info(typ) );
			guarded_free(const_cast<char *>(typ));

			// Hajo: repair files that have 'insane' values
			if(dummy.menge < 0) {
				dummy.menge = 0;
			}
			if(dummy.menge > (FAB_MAX_INPUT << precision_bits)) {
				dummy.menge = (FAB_MAX_INPUT << precision_bits);
			}
			eingang->append(dummy);
		}
	}

	// now rebuilt information for produced goods
	file->rdwr_int(ausgang_count, "\n");
	for(i=0; i<ausgang_count; i++) {
		ware_t dummy;
		const char *typ = NULL;
		int ab_sum;
		int ab_letzt;

		if(file->is_saving()) {
			typ = ausgang->at(i).gib_typ()->gib_name();
			dummy.menge = ausgang->at(i).menge;
			dummy.max = ausgang->at(i).max;
			ab_sum = abgabe_sum->at(i);
			ab_letzt = abgabe_letzt->at(i);
		}
		file->rdwr_delim("Aus: ");
		file->rdwr_str(typ, " ");
		file->rdwr_int(dummy.menge, " ");
		file->rdwr_int(dummy.max, "\n");
		file->rdwr_int(ab_sum, " ");
		file->rdwr_int(ab_letzt, "\n");

		if(file->is_loading()) {
			abgabe_sum->at(i)  = ab_sum;
			abgabe_letzt->at(i) = ab_letzt;
			dummy.setze_typ( warenbauer_t::gib_info(typ));
			guarded_free(const_cast<char *>(typ));
			ausgang->append(dummy);
		}
	}
	// restore other information
	spieler_n = welt->sp2num(besitzer_p);
	file->rdwr_delim("Bes: ");
	file->rdwr_int(spieler_n, "\n");
	file->rdwr_delim("Prf: ");
	file->rdwr_int(prodbase, "\n");
	file->rdwr_delim("Prb: ");
	file->rdwr_int(prodfaktor, "\n");
	// owner stuff
	if(file->is_loading()) {
		// take care of old files
		if(prodfaktor==1  ||  prodfaktor>16) {
			prodfaktor = 16;
		}
		if(file->get_version() < 86001) {
			koord k=besch->gib_haus()->gib_groesse();
DBG_DEBUG("fabrik_t::rdwr()","correction of production by %i",k.x*k.y);
			// since we step from 86.01 per factory, not per tile!
			prodbase *= k.x*k.y;
		}
		// Hajo: restore factory owner
		// Due to a omission in Volkers changes, there might be savegames
		// in which factories were saved without an owner. In this case
		// set the owner to the default of player 1
		if(spieler_n == -1) {
			// Use default
			besitzer_p = welt->gib_spieler(1);
		}
		else {
		// Restore owner pointer
		besitzer_p = welt->gib_spieler(spieler_n);
		}
	}

	file->rdwr_int(anz_lieferziele, "\n");

	// connect/save consumer
	if(file->is_loading()) {
		koord k;
		for(int i=0; i<anz_lieferziele; i++) {
			k.rdwr(file);
			add_lieferziel(k);
		}
	}
	else {
		koord k;
		for(int i=0; i<anz_lieferziele; i++) {
			k = lieferziele.get(i);
			k.rdwr(file);
		}
	}

	if(file->is_loading()) {
		baue(rotate, false);
	}
}


/**
 * setzt die Eingangswarentypen
 */
void fabrik_t::set_eingang(vector_tpl<ware_t> * typen)
{
	if(eingang) {
		delete eingang;
	}
	eingang = typen;
}

/**
 * setzt die Ausgangsswarentypen
 */
void fabrik_t::set_ausgang(vector_tpl<ware_t> * typen)
{
	if(ausgang) {
		delete ausgang;
	}

	ausgang = typen;
	if(abgabe_sum) {
		delete abgabe_sum;
	}
	if(abgabe_letzt) {
		delete abgabe_letzt;
	}

	abgabe_sum = new array_tpl<int> (ausgang->get_size());
	abgabe_letzt = new array_tpl<int> (ausgang->get_size());
	for(uint32 j=0; j<ausgang->get_size(); j++) {
		abgabe_sum->at(j) = 0;
		abgabe_letzt->at(j) = 0;
	}
}



/*
 * calculates the produktion per delta_t; this is now PRODUCTION_DELTA_T
 * @author Hj. Malthaner
 */
uint32 fabrik_t::produktion(const uint32 produkt) const
{
//	uint32 menge = (prodbase * prodfaktor) >> (BASEPRODSHIFT+MAX_PRODBASE_SHIFT-precision_bits);
	uint32 menge = (prodbase * prodfaktor*4) >> (BASEPRODSHIFT+MAX_PRODBASE_SHIFT-precision_bits);

	if(ausgang->get_count() > produkt) {
		// wenn das lager voller wird, produziert eine Fabrik weniger pro step

		const uint32 maxi = ausgang->get(produkt).max;
		const uint32 actu = ausgang->get(produkt).menge;

		if(actu < maxi) {
			// P = prod_base * anz_gebaeude * prodfaktor;
			// theoretische Menge pro tick
			menge = (menge*(maxi-actu)) / maxi;
		}
		else {
			// Lager (über)voll? -> 0
			menge = 0;
		}
	}

//	return (menge*PRODUCTION_DELTA_T) >> BASEPRODSHIFT;
	return menge;
}



int fabrik_t::max_produktion() const
{
  // P = prod_base * anz_gebaeude * prodfaktor; (prodfaktor 16=1.0)
  // theoretische Menge pro tick
  const uint32 menge = (prodbase * prodfaktor) >> (BASEPRODSHIFT+MAX_PRODBASE_SHIFT-precision_bits);
//  const koord k = besch->gib_haus()->gib_groesse();
//  const int n = k.x * k.y;
//  return (((menge*welt->ticks_per_tag) >> BASEPRODSHIFT) >> precision_bits)*n;

  // monat mit 1(!) tag
  return (((menge*welt->ticks_per_tag) >> BASEPRODSHIFT) >> precision_bits);
}


int
fabrik_t::vorrat_an(const ware_besch_t *typ)
{
    int menge = -1;

    for(uint32 index = 0; index < ausgang->get_count(); index ++) {

	if(typ == ausgang->at(index).gib_typ()) {
	    menge =  ausgang->at(index).menge >> precision_bits;
	    break;
	}
    }

    return menge;
}

int
fabrik_t::hole_ab(const ware_besch_t *typ, int menge)
{
    for(uint32 index = 0; index < ausgang->get_count(); index ++) {

	if(ausgang->at(index).gib_typ() == typ) {
	    if((ausgang->at(index).menge >> precision_bits) >= menge ) {
		ausgang->at(index).menge -= menge << precision_bits;
	    } else {
		menge = ausgang->at(index).menge >> precision_bits;
		ausgang->at(index).menge = 0;
	    }

	    abgabe_sum->at(index) += menge;

	    return menge;

	}
    }

    // ware "typ" wird hier nicht produziert
    return -1;
}



int
fabrik_t::liefere_an(const ware_besch_t *typ, int menge)
{
	for(uint32 index = 0; index < eingang->get_count(); index ++) {

		if(eingang->at(index).gib_typ() == typ) {

			// Hajo: avoid overflow
			if(eingang->at(index).menge < ((FAB_MAX_INPUT-menge) << precision_bits)) {
				eingang->at(index).menge += menge << precision_bits;
			}

			// sollte maximale lagerkapazitaet pruefen
			return menge;
		}
	}

	// ware "typ" wird hier nicht verbraucht
	return -1;
}

int
fabrik_t::verbraucht(const ware_besch_t *typ)
{
    for(uint32 index = 0; index < eingang->get_count(); index ++) {
	if(eingang->at(index).gib_typ() == typ) {
            // sollte maximale lagerkapazitaet pruefen
	    return 1;
	}
    }
    return -1;  // wird hier nicht verbraucht
}


void
fabrik_t::step(long delta_t)
{
	delta_sum += delta_t;

	if(delta_sum > PRODUCTION_DELTA_T) {
		INT_CHECK("simfab 558");
//DBG_DEBUG("fabrik_t::step()","%s",gib_name());

		const uint32 ecount = eingang->get_count();
		uint32 index = 0;
		uint32 produkt;

		if(ausgang->get_count()==0) {
			// consumer only ...
			uint32 menge = produktion(produkt) * (delta_sum/PRODUCTION_DELTA_T);

			// finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 v = (menge*vb) >> 8;

				if((uint32)(eingang->get(index).menge) > v) {
					eingang->at(index).menge -= v;
				}
				else {
					eingang->at(index).menge = 0;
				}
			}
		}
		else {
			// produces something
			for(produkt = 0; produkt < ausgang->get_count(); produkt ++) {
				uint32 menge = 9999999;

				// consumer?
				if(ecount > 0) {
					// ok, calulate consumption

					// also producer?
					if(ausgang->get_count() > 0) {

						for(index = 0; index < ecount; index ++) {
							// verbrauch fuer eine Einheit des Produktes (in 1/256)
							uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
							uint32 n;

							if((n = (eingang->get(index).menge * 256)/vb) < menge) {
								menge = n;    // finde geringsten vorrat
							}
						}
					}

					// calculate production
					uint32 p_menge = produktion(produkt) * (delta_sum/PRODUCTION_DELTA_T);
					menge = p_menge < menge ? p_menge : menge;  // production smaller than possible due to consumption

					// finally consume stock
					for(index = 0; index<ecount; index ++) {

						const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
						const uint32 v = (menge*vb) >> 8;

						if((uint32)(eingang->get(index).menge) > v) {
							eingang->at(index).menge -= v;
						}
						else {
							eingang->at(index).menge = 0;
						}
					}
				}
				else {
					// source producer
					menge = produktion(produkt) * (delta_sum/PRODUCTION_DELTA_T);
				}
				const uint32 pb = besch->gib_produkt(produkt)->gib_faktor();
				const uint32 p = (menge*pb) >> 8;

				// finally produce
				if(ausgang->at(produkt).menge < ausgang->at(produkt).max) {
					ausgang->at(produkt).menge += p;
				} else {
					ausgang->at(produkt).menge = ausgang->at(produkt).max-1;
				}
			}
		}

		// Zeituhr zurücksetzen
		while(  delta_sum>PRODUCTION_DELTA_T) {
			delta_sum -= PRODUCTION_DELTA_T;
		}
	}

	// verteilung frühestens alle 8 sekunden
	if(welt->gib_zeit_ms() > aktionszeit) {
		aktionszeit = welt->gib_zeit_ms() + 8192;

		for(uint32 produkt = 0; produkt < ausgang->get_count(); produkt ++) {
			if(ausgang->at(produkt).menge > (10<<precision_bits)) {

				verteile_waren(produkt);
				INT_CHECK("simfab 636");
			}
		}
	}

#ifdef FAB_PAX
/* will be now handled by the simcity.cc as returning passengers! */
	// finally passengers
	pax_zeit += delta_t;
	if(pax_zeit>pax_intervall) {
		pax_zeit -= pax_intervall;
		verteile_passagiere();
		INT_CHECK("simfab 643");
	}
#endif
}


/**
 * Die erzeugten waren auf die Haltestellen verteilen
 * @author Hj. Malthaner
 */
void fabrik_t::verteile_waren(const uint32 produkt)
{
	// wohin liefern ?
	if(lieferziele.get_count() == 0) {
		return;
	}

	slist_tpl<halthandle_t> halt_ok;
	slist_tpl<ware_t> ware_ok;
	// Über alle Haltestellen iterieren
	slist_iterator_tpl <halthandle_t> iter (halt_list);

	// ok, first send everything away
	while(iter.next()) {
		halthandle_t halt = iter.get_current();

		// Über alle Ziele iterieren
		for(uint32 n=0; n<lieferziele.get_count(); n++) {
			const koord lieferziel = lieferziele.get(n);

			fabrik_t * ziel_fab = gib_fab(welt, lieferziel);

			if(ziel_fab &&	ziel_fab->verbraucht(ausgang->at(produkt).gib_typ()) >= 0) {

				ware_t ware (ausgang->at(produkt).gib_typ());
				ware.menge = 1;
				ware.setze_zielpos( lieferziel );


				// Station can only store up to 128 units of goods per square
				if(halt->gib_ware_summe(ware.gib_typ()) <(halt->gib_grund_count() << 7)) {
					// ok, still enough space
					halt->suche_route(ware, halt);

					if(ware.gib_ziel() != koord::invalid) {
//	    printf("Starthalt %d/%d %s\n", i+1, list.get_count(), halt->gib_name());
//	    printf("Zielhalt %s\n", haltestelle_t::gib_halt(welt, ware->gib_ziel())->gib_name());

						halt_ok.insert(halt);
						ware_ok.insert(ware);
					}
					else {
// printf("Starthalt %d/%d %s\n", i+1, list.get_count(), halt->gib_name());
// printf("kann %s nicht Ziel %d %d (%d/%d) liefern\n", ware.gib_name(), lieferziel.x, lieferziel.y, n, lieferziele.get_count());
					}

				}
				else {
					// Station too full, notify player
					halt->gib_besitzer()->bescheid_station_voll(halt);
					break;
				}
			}
		}
	}


	// Auswertung der Ergebnisse
	if(!halt_ok.is_empty()) {

		// Hajo: distribute goods to station that has the least amount stored
		const int menge = (ausgang->at(produkt).menge >> precision_bits);

		if(menge > 1) {
			ausgang->at(produkt).menge -= menge << precision_bits;

			slist_iterator_tpl<halthandle_t> iter (halt_ok);
			slist_iterator_tpl<ware_t> ware_iter (ware_ok);

			ware_t best_ware       = ware_ok.at(0);
			halthandle_t best_halt = halt_ok.at(0);
			int best_amount        = 999999;

			while(iter.next() && ware_iter.next()) {
				halthandle_t halt = iter.get_current();
				ware_t ware       = ware_iter.get_current();

				const int amount = halt->gib_ware_fuer_ziel(ware.gib_typ(),ware.gib_ziel());

				if(amount < best_amount) {
					best_ware = ware;
					best_ware.menge = menge;
					best_halt = halt;
					best_amount = amount;
				}
			}

// printf("liefere %d %s an %s\n", menge, ware->name(), halt->gib_name());
			best_halt->liefere_an(best_ware);
		}



		/* Hajo: old code distributed equal amounts to all stations
		const int count = halt_ok.count();
		int menge = (ausgang->at(0).menge >> precision_bits) / count;

		//	printf("Menge %d\n", menge);

		if(menge > 1) {
		ausgang->at(0).menge -= (menge * count) << precision_bits;

		slist_iterator_tpl<halthandle_t> iter (halt_ok);
		slist_iterator_tpl<ware_t> ware_iter (ware_ok);

		while(iter.next() && ware_iter.next()) {
		halthandle_t halt = iter.get_current();
		ware_t ware = ware_iter.get_current();

		ware.menge = menge;

		//		printf("liefere %d %s an %s\n", menge, ware->name(), halt->gib_name());

		halt->liefere_an(ware);
		}
		}
	*/
	}
}


#ifdef FAB_PAX
void
fabrik_t::verteile_passagiere()
{
	slist_iterator_tpl <halthandle_t> iter (halt_list);

	while(iter.next()) {
		halthandle_t halt = iter.get_current();

		// die Arbeiter wollen auch wieder nach hause
		slist_iterator_tpl<stadt_t *> stadt_iter (arbeiterziele);

		while(stadt_iter.next()) {
			stadt_t * stadt = stadt_iter.get_current();

			// Hajo: call simrand() only once, use different
			// bits for different purposes
			const int r = simrand(0xFFFF);

			ware_t pax (((r & 3) == 0) ? warenbauer_t::post : warenbauer_t::passagiere);
			pax.menge = 1;

			const koord ziel = stadt->gib_zufallspunkt();
			pax.setze_zielpos( ziel );

			halt->suche_route(pax, halt);

			if(pax.gib_ziel()!=koord::invalid) {
				if(halt->gib_ware_summe(pax.gib_typ()) > (halt->gib_grund_count() << 7)) {
					halt->add_pax_unhappy(pax.menge);
				}
				else {
					halt->liefere_an(pax);
					halt->add_pax_happy(pax.menge);
				}
			}
			else {
				halt->add_pax_no_route(pax.menge);
			}
		}
	}
}
#endif


void
fabrik_t::neuer_monat()
{
    for(uint32 index = 0; index < ausgang->get_count(); index ++) {
	abgabe_letzt->at(index) = abgabe_sum->at(index);
        abgabe_sum->at(index) = 0;
    }
}


static void info_add_ware_description(cbuffer_t & buf, const ware_t & ware)
{
  const ware_besch_t * type = ware.gib_typ();

  buf.append(" -");
  buf.append(translator::translate(ware.gib_name()));
  buf.append(" ");
  buf.append(ware.menge >> fabrik_t::precision_bits);
  buf.append("/");
  buf.append(ware.max >> fabrik_t::precision_bits);
  buf.append(translator::translate(ware.gib_mass()));

  if(ware.gib_catg() != 0) {
    buf.append(", ");
    buf.append(translator::translate(type->gib_catg_name()));
  }
}


void fabrik_t::info(cbuffer_t & buf)
{
  buf.append("\n");
  buf.append(translator::translate("Produktion"));
  buf.append(":\n ");
  buf.append(translator::translate("Durchsatz"));
  buf.append(" ");
  buf.append(max_produktion());
  buf.append(" ");
  buf.append(translator::translate("units/day"));
  buf.append("\n");


  if(lieferziele.get_count() > 0) {
    buf.append("\n");
    buf.append(translator::translate("Abnehmer"));
    buf.append(":\n");

    for(uint32 i=0; i<lieferziele.get_count(); i++) {
      const koord lieferziel = lieferziele.get(i);

      ding_t * dt = welt->lookup(lieferziel)->gib_kartenboden()->obj_bei(1);
      if(dt) {
	fabrik_t *fab = dt->fabrik();

	if(fab) {
	  buf.append("     ");
	  buf.append(translator::translate(fab->gib_name()));
	  buf.append(" ");
	  buf.append(lieferziel.x);
	  buf.append(",");
	  buf.append(lieferziel.y);
	  buf.append("\n");
	}
      }
    }
  }

  if(suppliers.get_count() > 0) {
    buf.append("\n");
    buf.append(translator::translate("Suppliers"));
    buf.append(":\n");

    for(uint32 i=0; i<suppliers.get_count(); i++) {
      const koord supplier = suppliers.get(i);

      ding_t * dt = welt->lookup(supplier)->gib_kartenboden()->obj_bei(1);
      if(dt) {
	fabrik_t *fab = dt->fabrik();

	if(fab) {
	  buf.append("     ");
	  buf.append(translator::translate(fab->gib_name()));
	  buf.append(" ");
	  buf.append(supplier.x);
	  buf.append(",");
	  buf.append(supplier.y);
	  buf.append("\n");
	}
      }
    }
  }

  if(!arbeiterziele.is_empty()) {
    slist_iterator_tpl<stadt_t *> iter (arbeiterziele);

    buf.append("\n");
    buf.append(translator::translate("Arbeiter aus:"));
    buf.append("\n");

    while(iter.next()) {
      stadt_t *stadt = iter.get_current();

      buf.append("     ");
      buf.append(stadt->gib_name());
      buf.append("\n");

    }
    // give a passenger level for orientation
    int passagier_rate = besch->gib_pax_level();
    buf.append("\n");
    buf.append(translator::translate("Passagierrate"));
    buf.append(": ");
    buf.append(passagier_rate);
    buf.append("\n");

    buf.append(translator::translate("Postrate"));
    buf.append(": ");
    buf.append(passagier_rate/3);
    buf.append("\n");
  }

  if(ausgang->get_count() > 0) {

    buf.append("\n ");
    buf.append(translator::translate("Produktion"));
    buf.append(":\n");

    for(uint32 index = 0; index < ausgang->get_count(); index ++) {
      info_add_ware_description(buf, ausgang->at(index));

      buf.append(", ");
      buf.append((int)(besch->gib_produkt(index)->gib_faktor()*100/256));
      buf.append("%\n");
    }
  }

  if(eingang->get_count() > 0) {

    buf.append("\n ");
    buf.append(translator::translate("Verbrauch"));
    buf.append(":\n");

    for(uint32 index = 0; index < eingang->get_count(); index ++) {

      buf.append(" -");
      buf.append(translator::translate(eingang->at(index).gib_name()));
      buf.append(" ");
      buf.append(eingang->at(index).menge >> precision_bits);
      buf.append("/");
      buf.append(eingang->at(index).max >> precision_bits);
      buf.append(translator::translate(eingang->at(index).gib_mass()));
      buf.append(", ");
      buf.append((int)(besch->gib_lieferant(index)->gib_verbrauch()*100/256));
      buf.append("%\n");
    }
  }

  // debug - alle Haltestellen iterieren
  /*
  slist_iterator_tpl <halthandle_t> iter (halt_list);

  while(iter.next()) {
    halthandle_t halt = iter.get_current();
    buf.append(halt->gib_name());
    buf.append("\n");
  }
  */
}

void fabrik_t::laden_abschliessen()
{
	slist_iterator_tpl<fabrik_t*> fiter ( welt->gib_fab_list() );
	while(fiter.next()) {
		fabrik_t * fab = fiter.get_current();
		koord fab_pos = fab->gib_pos().gib_2d();
		const vector_tpl <koord> &lieferziele = fab->gib_lieferziele();
		for(uint32 i=0; i<lieferziele.get_count(); i++) {
			const koord lieferziel = lieferziele.get(i);
			fabrik_t * fab2 = fabrik_t::gib_fab(welt, lieferziel);
			if (fab2) {
				fab2->add_supplier(fab_pos);
			}
		}
	}
}



void
fabrik_t::add_supplier(koord pos)
{
	if(suppliers.get_count() < max_suppliers) {
		suppliers.append_unique(pos);
	}
}
