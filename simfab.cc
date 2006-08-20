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
#include "simmem.h"
#include "simcolor.h"
#include "boden/grund.h"
#include "simfab.h"
#include "simcity.h"
#include "simhalt.h"
#include "simskin.h"
#include "simworld.h"
#include "besch/haus_besch.h"
#include "besch/ware_besch.h"
#include "simplay.h"
#include "simtools.h"


#include "simintr.h"

#include "dings/raucher.h"
#include "dings/gebaeude.h"
//#include "dings/leitung2.h"

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
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
	if(welt->ist_in_kartengrenzen(pos)) {
		const grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		if(gr  &&  gr->obj_bei(0)!=NULL  &&  gr->obj_bei(0)->get_fabrik()!=NULL) {
			return gr->obj_bei(0)->get_fabrik();
		}
	}
	// not found
	return 0;
}



void fabrik_t::link_halt(halthandle_t halt)
{
	welt->access(pos.gib_2d())->add_to_haltlist(halt);
}



void fabrik_t::unlink_halt(halthandle_t halt)
{
	planquadrat_t *plan=welt->access(pos.gib_2d());
	if(plan) {
		plan->remove_from_haltlist(welt,halt);
	}
}


void
fabrik_t::add_lieferziel(koord ziel)
{
	lieferziele.append_unique(ziel,4);
	fabrik_t * fab = fabrik_t::gib_fab(welt, ziel);
	if (fab) {
		fab->add_supplier(gib_pos().gib_2d());
	}
}


void
fabrik_t::rem_lieferziel(koord ziel)
{
	lieferziele.remove(ziel);
}


fabrik_t::fabrik_t(karte_t *wl, loadsave_t *file) : lieferziele(0), suppliers(0)
{
	welt = wl;

	eingang = NULL;
	ausgang = NULL;

	besch = NULL;

	abgabe_sum = NULL;
	abgabe_letzt = NULL;

	besitzer_p = NULL;

	rdwr(file);

	delta_sum = 0;
	last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;
}


fabrik_t::fabrik_t(karte_t *wl, koord3d pos, spieler_t *spieler, const fabrik_besch_t *fabesch) : lieferziele(0), suppliers(0)
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

	delta_sum = 0;
    last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;
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

	if(raucher) {
		delete raucher;
	}
	raucher = 0;
}



// must be extended for non-square factories!
bool
fabrik_t::is_fabrik( koord check )
{
	return (check.x>=pos.x   &&  check.x<pos.x+besch->gib_haus()->gib_groesse(rotate).x  &&  check.y>=pos.y  &&  check.y<pos.y+besch->gib_haus()->gib_groesse(rotate).y);
}

bool
fabrik_t::is_fabrik( koord lu, koord rd )
{
	return (rd.x>=pos.x   &&  lu.x<pos.x+besch->gib_haus()->gib_groesse(rotate).x  &&  rd.y>=pos.y  &&  lu.y<pos.y+besch->gib_haus()->gib_groesse(rotate).y);
}


void
fabrik_t::add_arbeiterziel(stadt_t *stadt)
{
	// do not add a city twice!
	if(!arbeiterziele.contains(stadt)) {
		arbeiterziele.insert(stadt);
	}
}


void
fabrik_t::rem_arbeiterziel(stadt_t *stadt)
{
	arbeiterziele.remove(stadt);
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
			raucher = new raucher_t(welt, k, rada);
			raucher->set_flag(ding_t::not_on_map);
		}
		else {
			raucher = NULL;
		}
	}
	else {
		dbg->error("fabrik_t::baue()", "Good pak not available!");
	}
}

bool
fabrik_t::ist_bauplatz(karte_t *welt, koord pos, koord groesse,bool wasser)
{
    bool ok = false;
    if(pos.x > 0 && pos.y > 0 &&
       pos.x+groesse.x < welt->gib_groesse_x() && pos.y+groesse.y < welt->gib_groesse_y() &&
       ( wasser  ||  welt->ist_platz_frei(pos, groesse.x, groesse.y) )&&
       !ist_da_eine(welt,pos-koord(5,5),pos+groesse+koord(3,3))) {

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
fabrik_t::sind_da_welche(karte_t *welt, koord min_pos, koord max_pos)
{
	static vector_tpl <fabrik_t*> fablist(16);
	fablist.clear();

	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			if(welt->ist_in_kartengrenzen(x, y)) {
				const grund_t *gr = welt->lookup(koord(x,y))->gib_kartenboden();
				if(gr->obj_bei(0)!=NULL  &&  gr->obj_bei(0)->get_fabrik()!=NULL) {
					if( fablist.append_unique( gr->obj_bei(0)->get_fabrik(), 4 )  ) {
//DBG_MESSAGE("fabrik_t::sind_da_welche()","appended factory %s at (%i,%i)",gr->obj_bei(0)->get_fabrik()->gib_besch()->gib_name(),x,y);
					}
				}
			}
		}
	}
	return fablist;
}

bool
fabrik_t::ist_da_eine(karte_t *welt, koord min_pos, koord max_pos )
{
	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			if(welt->ist_in_kartengrenzen(x, y)) {
				const grund_t *gr = welt->lookup(koord(x,y))->gib_kartenboden();
				if(gr->obj_bei(0)!=NULL  &&  gr->obj_bei(0)->get_fabrik()!=NULL) {
					return true;
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
		s = besch->gib_name();
	}
	file->rdwr_str(s, "-");
	if(file->is_loading()) {
DBG_DEBUG("fabrik_t::rdwr()","loading factory '%s'",s);
		besch = fabrikbauer_t::gib_fabesch(s);
		guarded_free(const_cast<char *>(s));
		// set ware arrays ...
		if(besch) {
			set_eingang( new vector_tpl<ware_t> (besch->gib_lieferanten()) );
			set_ausgang( new vector_tpl<ware_t> (besch->gib_produkte()) );
		}
		else {
			// save defaults for loading only, factory will be ignored!
			set_eingang( new vector_tpl<ware_t> (16) );
			set_ausgang( new vector_tpl<ware_t> (10) );
		}
	}
	pos.rdwr(file);

	file->rdwr_delim("Bau: ");
	file->rdwr_byte(rotate, "\n");

	// now rebuilt information for recieved goods
	file->rdwr_long(eingang_count, "\n");
	for(i=0; i<eingang_count; i++) {
		ware_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = eingang->at(i).gib_typ()->gib_name();
			dummy.menge = eingang->at(i).menge<<(old_precision_bits-precision_bits);
			dummy.max = eingang->at(i).max<<(old_precision_bits-precision_bits);
		}

		file->rdwr_delim("Ein: ");
		file->rdwr_str(typ, " ");
		file->rdwr_long(dummy.menge, " ");
		file->rdwr_long(dummy.max, "\n");
		if(file->is_loading()) {
			dummy.setze_typ( warenbauer_t::gib_info(typ) );
			guarded_free(const_cast<char *>(typ));

			// Hajo: repair files that have 'insane' values
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			if(dummy.menge < 0) {
				dummy.menge = 0;
			}
			if(dummy.menge > (FAB_MAX_INPUT << precision_bits)) {
				dummy.menge = (FAB_MAX_INPUT << precision_bits);
			}
			eingang->append(dummy,4);
		}
	}

	// now rebuilt information for produced goods
	file->rdwr_long(ausgang_count, "\n");
	for(i=0; i<ausgang_count; i++) {
		ware_t dummy;
		const char *typ = NULL;
		int ab_sum;
		int ab_letzt;

		if(file->is_saving()) {
			typ = ausgang->at(i).gib_typ()->gib_name();
			dummy.menge = ausgang->at(i).menge<<(old_precision_bits-precision_bits);
			dummy.max = ausgang->at(i).max<<(old_precision_bits-precision_bits);;
			ab_sum = abgabe_sum->at(i);
			ab_letzt = abgabe_letzt->at(i);
		}
		file->rdwr_delim("Aus: ");
		file->rdwr_str(typ, " ");
		file->rdwr_long(dummy.menge, " ");
		file->rdwr_long(dummy.max, "\n");
		file->rdwr_long(ab_sum, " ");
		file->rdwr_long(ab_letzt, "\n");

		if(file->is_loading()) {
			abgabe_sum->at(i)  = ab_sum;
			abgabe_letzt->at(i) = ab_letzt;
			dummy.setze_typ( warenbauer_t::gib_info(typ));
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			guarded_free(const_cast<char *>(typ));
			ausgang->append(dummy,4);
		}
	}
	// restore other information
	spieler_n = welt->sp2num(besitzer_p);
	file->rdwr_delim("Bes: ");
	file->rdwr_long(spieler_n, "\n");
	file->rdwr_delim("Prf: ");
	file->rdwr_long(prodbase, "\n");
	file->rdwr_delim("Prb: ");
	file->rdwr_long(prodfaktor, "\n");
	// owner stuff
	if(file->is_loading()) {
		// take care of old files
		if(prodfaktor==1  ||  prodfaktor>16) {
			prodfaktor = 16;
		}
		if(file->get_version() < 86001) {
			if(besch) {
				koord k=besch->gib_haus()->gib_groesse();
DBG_DEBUG("fabrik_t::rdwr()","correction of production by %i",k.x*k.y);
				// since we step from 86.01 per factory, not per tile!
				prodbase *= k.x*k.y*2;
			}
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

	file->rdwr_long(anz_lieferziele, "\n");

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

	if(file->is_loading()  &&  besch) {
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
	// default prodfaktor = 16 => shift 4, default time = 1024 => shift 10, rest precion
	const uint32 max = prodbase * prodfaktor;
	uint32 menge = max >> (18-10+4-fabrik_t::precision_bits);

	if(ausgang->get_count() > produkt) {
		// wenn das lager voller wird, produziert eine Fabrik weniger pro step

		const uint32 maxi = ausgang->get(produkt).max;
		const uint32 actu = ausgang->get(produkt).menge;

		if(actu < maxi) {
			// theoretische Menge pro tick
			menge = (menge*(maxi-actu)) / maxi;
		}
		else {
			// overfull? -> 0
			menge = 0;
		}
	}

	return menge;
}



int fabrik_t::max_produktion() const
{
	// production per month
	return (prodbase * prodfaktor)>>4;
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
	    return eingang->at(index).menge>eingang->at(index).max;
	}
    }
    return -1;  // wird hier nicht verbraucht
}


void
fabrik_t::step(long delta_t)
{
	delta_sum += delta_t;

	if(delta_sum > PRODUCTION_DELTA_T) {

		// produce nothing/consumes nothing ...
		if(eingang->get_count()+ausgang->get_count()==0) {
			delta_sum += PRODUCTION_DELTA_T;
			return;
		}

		INT_CHECK("simfab 558");
//DBG_DEBUG("fabrik_t::step()","%s",besch->gib_name());

		const uint32 ecount = eingang->get_count();
		uint32 index = 0;
		uint32 produkt=0;
		bool is_currently_producing = false;

		if(ausgang->get_count()==0) {
			// consumer only ...
			uint32 menge = produktion(produkt) * (delta_sum/PRODUCTION_DELTA_T);

			// finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 v = (menge*vb) >> 8;

				if((uint32)(eingang->get(index).menge) > v) {
					eingang->at(index).menge -= v;
					is_currently_producing = true;
				}
				else {
					eingang->at(index).menge = 0;
				}
			}
		}
		else {

			// ok, calulate maximum allowed consumption
			uint32 max_menge = 0x7FFFFFFF;
			uint32 consumed_menge = 0;
			for(index = 0; index < ecount; index ++) {
				// verbrauch fuer eine Einheit des Produktes (in 1/256)
				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 n = (eingang->get(index).menge * 256)/vb;

				if(n<max_menge) {
					max_menge = n;    // finde geringsten vorrat
				}
			}

			// produces something
			for(produkt = 0; produkt < ausgang->get_count(); produkt ++) {
				uint32 menge;

				if(ecount>0) {

					// calculate production
					uint32 p_menge = 0;
					for( long i=delta_sum/PRODUCTION_DELTA_T;  i>0;  i--  ) {
						p_menge += produktion(produkt);
					}
					menge = p_menge < max_menge ? p_menge : max_menge;  // production smaller than possible due to consumption
					if(menge>consumed_menge) {
						consumed_menge = menge;
					}

				}
				else {
					// source producer
					menge = 0;
					for( long i=delta_sum/PRODUCTION_DELTA_T;  i>0;  i--  ) {
						menge += produktion(produkt);
					}
				}

				const uint32 pb = besch->gib_produkt(produkt)->gib_faktor();
				const uint32 p = (menge*pb) >> 8;

				// produce
				if(ausgang->at(produkt).menge < ausgang->at(produkt).max) {
					ausgang->at(produkt).menge += p;
					is_currently_producing = true;
				} else {
					ausgang->at(produkt).menge = ausgang->at(produkt).max-1;
				}
			}

			// and finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 v = (consumed_menge*vb) >> 8;

				if((uint32)(eingang->get(index).menge) > v) {
					eingang->at(index).menge -= v;
					is_currently_producing = true;
				}
				else {
					eingang->at(index).menge = 0;
				}
			}
		}



		// Zeituhr zurücksetzen
		while(  delta_sum>PRODUCTION_DELTA_T) {
			delta_sum -= PRODUCTION_DELTA_T;
		}

		// distribute, if there are more than 10 waiting ...
		for(uint32 produkt = 0; produkt < ausgang->get_count(); produkt ++) {
			if(ausgang->at(produkt).menge > (10<<precision_bits)) {

				verteile_waren(produkt);
				INT_CHECK("simfab 636");
			}
		}
		recalc_factory_status();

		if(raucher  &&  is_currently_producing) {
			raucher->step(delta_sum);
		}
	}

	// to distribute to all target equally, we use this counter, for the factory, to try first
	last_lieferziel_start ++;

}


/**
 * Die erzeugten waren auf die Haltestellen verteilen
 * @author Hj. Malthaner
 */
void fabrik_t::verteile_waren(const uint32 produkt)
{
	// wohin liefern ?
	if(lieferziele.get_count()==0) {
		return;
	}

	// not connected?
	minivec_tpl<halthandle_t> &haltlist = welt->access(pos.gib_2d())->get_haltlist();
	if(haltlist.get_count()==0) {
		return;
	}

	slist_tpl<halthandle_t> halt_ok;
	slist_tpl<ware_t> ware_ok;
	bool still_overflow = true;

	// ok, first send everything away
	for(  unsigned i=0;  i<haltlist.get_count();  i++  ) {
		halthandle_t halt = haltlist.get(i);

		// Über alle Ziele iterieren
		for(uint32 n=0; n<lieferziele.get_count(); n++) {

			// prissi: this way, the halt, that is tried first, will change. As a result, if all destinations are empty, it will be spread evenly
			const koord lieferziel = lieferziele.get((n+last_lieferziel_start)%lieferziele.get_count());

			fabrik_t * ziel_fab = gib_fab(welt, lieferziel);
			int vorrat;

			if(ziel_fab &&	(vorrat=ziel_fab->verbraucht(ausgang->at(produkt).gib_typ()))>= 0) {

				ware_t ware (ausgang->at(produkt).gib_typ());
				ware.menge = 1;
				ware.setze_zielpos( lieferziel );

				unsigned w;
				// find the index in the target factory
				for( w=0;  w<ziel_fab->gib_eingang()->get_count()  &&  ziel_fab->gib_eingang()->at(w).gib_typ()!=ware.gib_typ();  w++  )
					;

				// Station can only store up to 128 units of goods per square
				if(halt->gib_ware_summe(ware.gib_typ()) <halt->get_capacity()) {
					// ok, still enough space
					halt->suche_route(ware);

//DBG_MESSAGE("verteile_waren()","searched for route for %s with result %i,%i",translator::translate(ware.gib_name()),ware.gib_ziel().x,ware.gib_ziel().y);
					if(ware.gib_ziel() != koord::invalid) {
						// if only overflown factories found => deliver to first
						// else deliver to non-overflown factory
						bool overflown = ziel_fab->gib_eingang()->at(w).menge >= ziel_fab->gib_eingang()->at(w).max;

						if(!umgebung_t::just_in_time) {

							// distribution also to overflowing factories
							if(still_overflow  &&  !overflown) {
								// not overflowing factory found
								still_overflow = false;
								halt_ok.clear();
								ware_ok.clear();
							}
							if(still_overflow  ||  !overflown) {
								halt_ok.insert(halt);
								ware_ok.insert(ware);
							}
						}
						else {

							// only distribute to no-overflowed factories
							if(!overflown) {
								halt_ok.insert(halt);
								ware_ok.insert(ware);
							}
						}
					}

				}
				else {
					// Station too full, notify player
					halt->bescheid_station_voll();
				}
			}
		}
	}

	// Auswertung der Ergebnisse
	if(!halt_ok.is_empty()) {

		// prissi: distribute goods to factory, that has not an overflowing input storage
		// if all have, then distribute evenly
		const int menge = (ausgang->at(produkt).menge >> precision_bits);

//DBG_MESSAGE("verteile_waren()","halts %i",halt_ok.count());

		// Hajo: distribute goods to station that has the least amount stored
		if(menge > 1) {
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
//DBG_MESSAGE("verteile_waren()","best_amount %i %s",best_amount,translator::translate(ware.gib_name()));
			}

			ausgang->at(produkt).menge -= menge << precision_bits;
			best_halt->starte_mit_route(best_ware);
		}

	}
}



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


// static !
unsigned fabrik_t::status_to_color[5] = {COL_RED, COL_ORANGE, COL_GREEN, COL_YELLOW, COL_WHITE };

#define FL_WARE_NULL           1
#define FL_WARE_ALLENULL       2
#define FL_WARE_LIMIT          4
#define FL_WARE_ALLELIMIT      8
#define FL_WARE_UEBER75        16
#define FL_WARE_ALLEUEBER75    32
#define FL_WARE_HATWAS         64

/* returns the status of the current factory, as well as output */
void fabrik_t::recalc_factory_status()
{
	unsigned long warenlager;
	char status_ein;
	char status_aus;

	int haltcount=welt->access(pos.gib_2d())->get_haltlist().get_count();

	// set bits for input
	warenlager = 0;
	status_ein = FL_WARE_ALLELIMIT;
	for (unsigned int j=0;j<eingang->get_count();j++) {

		if (eingang->at(j).menge >= eingang->at(j).max ) {
			status_ein |= FL_WARE_LIMIT;
		}
		else {
			status_ein &= ~FL_WARE_ALLELIMIT;
		}
		warenlager += eingang->at(j).menge;
		status_ein |= FL_WARE_HATWAS;

	}
	warenlager >>= fabrik_t::precision_bits;
	if(warenlager==0) {
		status_ein |= FL_WARE_ALLENULL;
	}
	total_input = warenlager;

	// set bits for output
	warenlager = 0;
	status_aus = FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL;
	for (unsigned int j=0;j<ausgang->get_count();j++)
	{
		if (ausgang->at(j).menge > 0) {

			status_aus &= ~FL_WARE_ALLENULL;
			if (ausgang->at(j).menge >= 0.75*ausgang->at(j).max ) {
				status_aus |= FL_WARE_UEBER75;
			}
			else {
				status_aus &= ~FL_WARE_ALLEUEBER75;
			}
			warenlager += ausgang->at(j).menge;
			status_aus &= ~FL_WARE_ALLENULL;
		}
		else {
			// menge = 0
			status_aus &= ~FL_WARE_ALLEUEBER75;
		}
	}
	warenlager >>= fabrik_t::precision_bits;
	total_output = warenlager;

	// now calculate status bar
	if(status_ein==(FL_WARE_ALLELIMIT|FL_WARE_ALLENULL)) {
		// does not consume anything, should just produce

		if(status_aus==(FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL)) {
			// does also not produce anything
			status = nothing;
		}
		else if(status_aus&FL_WARE_ALLEUEBER75  ||  status_aus&FL_WARE_UEBER75) {
			status = inactive;	// not connected?
			if(haltcount>0) {
				if(status_aus&FL_WARE_ALLEUEBER75) {
					status = bad;	// connect => needs better service
				}
				else {
					status = medium;	// connect => needs better service for at least one product
				}
			}
		}
		else {
			status = good;
		}
	}
	else if(status_aus==(FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL)) {
		// does not produce anything, just consumes

		if(status_ein&FL_WARE_ALLELIMIT) {
			// we assume not served
			status = bad;
		}
		else if(status_ein&FL_WARE_LIMIT) {
			// served, but still one at limit
			status = medium;
		}
		else if(status_ein&FL_WARE_ALLENULL) {
			status = inactive;	// assume not served
			if(haltcount>0) {
				// there is a halt => needs better service
				status = bad;
			}
		}
		else {
			status = good;
		}
	}
	else {
		// produces and consumes
		if((status_ein&FL_WARE_ALLELIMIT)!=0  &&  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = bad;
		}
		else if((status_ein&FL_WARE_ALLELIMIT)!=0  ||  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = medium;
		}
		else if((status_ein&FL_WARE_ALLENULL)!=0  &&  (status_aus&FL_WARE_ALLENULL)!=0) {
			// not producing
			status = inactive;
		}
		else if(haltcount>0  &&  ((status_ein&FL_WARE_ALLENULL)!=0  ||  (status_aus&FL_WARE_ALLENULL)!=0)) {
			// not producing but out of supply
			status = medium;
		}
		else {
			status = good;
		}
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

      ding_t * dt = welt->lookup(lieferziel)->gib_kartenboden()->obj_bei(0);
      if(dt) {
	fabrik_t *fab = dt->get_fabrik();

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

      ding_t * dt = welt->lookup(supplier)->gib_kartenboden()->obj_bei(0);
      if(dt) {
	fabrik_t *fab = dt->get_fabrik();

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
#ifdef DEBUG
	minivec_tpl<halthandle_t> &haltlist = welt->access(pos.gib_2d())->get_haltlist();
	for(unsigned i=0;  i<haltlist.get_count();  i++  ) {
	     buf.append("\n");
		buf.append(haltlist.at(i)->gib_name());
	}
#endif
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
	suppliers.append_unique(pos,4);
}


void
fabrik_t::rem_supplier(koord pos)
{
	suppliers.remove(pos);
}


// prissi: crossconnect everything possible
void
fabrik_t::add_all_suppliers()
{

	for(int i=0; i < besch->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = besch->gib_lieferant(i);
		const ware_besch_t *ware = lieferant->gib_ware();

		const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
		slist_iterator_tpl <fabrik_t *> iter (list);

		while( iter.next() ) {

			fabrik_t * fab = iter.get_current();

			// connect to an existing one, if this is an producer
			if(fab!=this  &&  fab->vorrat_an(ware) > -1) {
				// add us to this factory
				fab->add_lieferziel(pos.gib_2d());
			}
		}
	}
}
