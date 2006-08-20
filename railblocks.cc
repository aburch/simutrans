/*
 * railblocks.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * railblocks.cc
 *
 * Implentierung der Blockstreckenverwaltung
 *
 * Hj. Malthaner
 */

#include <stdio.h>

#include "simdebug.h"
#include "simworld.h"

#include "railblocks.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "dings/signal.h"
#include "boden/wege/schiene.h"
#include "boden/grund.h"
#include "utils/cbuffer_t.h"

//#define DEBUG 1


/**
 * Rail block factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
blockhandle_t blockstrecke_t::create(karte_t *welt)
{
    blockstrecke_t * p = new blockstrecke_t(welt);
    blockhandle_t   h(p);

    return h;
}


/**
 * Rail block factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
blockhandle_t blockstrecke_t::create(karte_t *welt, loadsave_t *file)
{
    blockstrecke_t * p = new blockstrecke_t(welt, file);
    blockhandle_t   h(p);

    return h;
}


/**
 * Rail block destruction method.
 * @author Hj. Malthaner
 */
void blockstrecke_t::destroy(blockhandle_t &bs)
{
    blockstrecke_t * p = bs.detach();
    delete p;
}


blockstrecke_t::blockstrecke_t(karte_t *w, loadsave_t *file)
{
    welt = w;

    rdwr(file);
}

void blockstrecke_t::laden_abschliessen()
{

    slist_iterator_tpl<signal_t *> iter( signale );

    while(iter.next()) {
	signal_t *sig = iter.get_current();

	assert(welt->lookup( sig->gib_pos() ) != NULL);
	assert(welt->lookup( sig->gib_pos() )->gib_weg(weg_t::schiene) != NULL);
	ribi_t::ribi dir = sig->gib_richtung();

	if(dir == ribi_t::nord || dir == ribi_t::ost) {
	    welt->lookup(sig->gib_pos())->obj_add(sig);
	} else {
	    welt->lookup(sig->gib_pos())->obj_pri_add(sig, PRI_HOCH);
	}
    }
}


blockstrecke_t::blockstrecke_t(karte_t *w)
{
    v_rein = v_raus = 0;
    welt = w;
}


blockstrecke_t::~blockstrecke_t()
{
//    printf("destruktor::blockstrecke %p\n", this);

    verdrahte_signale_neu();

    // alle signale aufräumen

    while(signale.count() > 0) {
        signal_t * sig = signale.at(0);

	signale.remove(sig);
        delete sig;
    }

    signale.clear();
}

void blockstrecke_t::verdrahte_signale_neu()
{
    slist_tpl<signal_t *> kill_list;

    slist_iterator_tpl<signal_t *> iter( signale );

    while(iter.next()) {
	signal_t *sig = iter.get_current();

	schiene_t *sch = dynamic_cast<schiene_t*> (welt->lookup(sig->gib_pos())->gib_weg(weg_t::schiene));

	if(sch == NULL) {
	    dbg->error("blockstrecke_t::verdrahte_signale_neu()", "signal on square without railroad track, skipping signal!");
	    continue;
	}

	if(sch->gib_blockstrecke().operator->() != this) {
	    // signal neu verdrahten

	    sch->gib_blockstrecke()->add_signal(sig);
	    kill_list.insert( sig );
	}
    }

    slist_iterator_tpl<signal_t *> killer( kill_list );

    while(killer.next()) {
	signal_t *sig = killer.get_current();
        signale.remove( sig );
    }

//    DBG_MESSAGE("blockstrecke_t::verdrahte_signale_neu()",
//		 "rail block %p has now %d signals", this, signale.count());
}

void blockstrecke_t::add_signal(signal_t *sig)
{
    assert(sig != NULL);

    if(!signale.contains( sig )) {

	signale.insert(sig);
	sig->setze_zustand(ist_frei() ? signal_t::gruen : signal_t::rot);

    } else {
	dbg->warning("blockstrecke_t::add_signal()",
		     "Signal %p already attached to rail block %p.",
		     sig, this);
    }

    dbg->warning("blockstrecke_t::add_signal()",
		 "rail block %p has after adding signal %p at %d,%d  %d signals",
		 this, sig, sig->gib_pos().x, sig->gib_pos().y,
		 signale.count());
}


void blockstrecke_t::add_signale(slist_tpl<signal_t *> &signale)
{
    slist_iterator_tpl<signal_t *> iter( signale );

    while(iter.next()) {
	add_signal(iter.get_current());
    }
}


bool blockstrecke_t::loesche_signal_bei(koord3d k)
{
    signal_t * sig = gib_signal_bei( k );

    if(sig) {

        DBG_MESSAGE("blockstrecke_t::loesche_signal_bei()",
		     "rail block %p removes signal %p at %hd,%hd",
		     this, sig, k.x, k.y);

	// der Destruktor entfernt das Signal aus allen Listen
	delete sig;
    } else {
	// check for orphan signal

	if(welt->lookup(k)) {
	    sig = dynamic_cast<signal_t *>(welt->lookup(k)->suche_obj(ding_t::signal));

	    if(sig) {
		delete sig;

		dbg->warning("blockstrecke_t::loesche_signal_bei()",
		             "orphan signal %p detected at %hd,%hd",
			     sig, k.x, k.y);
	    }
	}
    }

    // gab's ueberhaupt was zu loeschen ?
    return (sig != NULL);
}


bool blockstrecke_t::entferne_signal(signal_t *sig)
{
    return signale.remove( sig );
}

signal_t * blockstrecke_t::gib_signal_bei(koord3d k)
{
    signal_t *sig = NULL;

    slist_iterator_tpl<signal_t *> iter( signale );

    while(iter.next()) {
	signal_t *tmp = iter.get_current();

	if(tmp->gib_pos() == k) {
	    sig = tmp;
	    break;
	}
    }

#ifdef DEBUG
    DBG_MESSAGE("blockstrecke_t::gib_signal_bei()",
		 "found signal %p at %d,%d.\n", sig, k.x, k.y);
#endif

    return sig;
}


void blockstrecke_t::schalte_signale()
{
    const signal_t::signalzustand zustand =
        ist_frei() ? signal_t::gruen : signal_t::rot;

    slist_iterator_tpl<signal_t *> iter( signale );

    while(iter.next()) {
	signal_t *sig = iter.get_current();

//	printf("blockstrecke_t::schalte_signale: schalte signal %p\n", sig);

	sig->setze_zustand( zustand );
    }
}


void blockstrecke_t::betrete(vehikel_basis_t *)
{
    v_rein ++;
    schalte_signale();

//    printf("   betrete %p: zustand  %d, %d\n", this, v_rein, v_raus);
}


void blockstrecke_t::verlasse(vehikel_basis_t *)
{

    v_raus ++;
    schalte_signale();

    if(v_raus == v_rein) {
	// vermeide Zaehlerueberlauf
	v_raus = v_rein = 0;
    }

//    printf("   verlasse %p: zustand %d, %d\n", this, v_rein, v_raus);
}

void blockstrecke_t::setze_belegung(int count)
{
    v_raus = 0;
    v_rein = count;

    schalte_signale();
}



void
blockstrecke_t::vereinige_vehikel_counter(blockhandle_t bs)
{
    v_rein += bs->v_rein;
    v_raus += bs->v_raus;

    schalte_signale();

//    printf("   vereinige %p, %p: zustand %d, %d\n", this, bs, v_rein, v_raus);

}


void
blockstrecke_t::rdwr(loadsave_t *file)
{
    int count;
    short int typ = ding_t::signal;

    // signale laden
    if(file->is_saving()) {
        count = signale.count();
    }
    file->rdwr_delim("S ");
    file->rdwr_int(count, "\n");

    if(file->is_saving()) {
        slist_iterator_tpl<signal_t*> s_iter ( signale );
        while(s_iter.next()) {
	    s_iter.get_current()->rdwr(file, true);
        }
    }
    else {
        for(int i=0; i<count; i++) {
	    // objectdescriptor überlesen
	    typ=file->rd_obj_id();
	    signal_t * sig = NULL;
	    // currently we only have two signal types, so this line of code should be ok
	    // if we introduce more types, we should change this code to a "switch-case" block
	  	sig = typ == ding_t::signal ? new signal_t(welt, file) : new presignal_t(welt, file);
	    signale.insert( sig );
        }
    }
    file->rdwr_int(v_rein, " ");
    file->rdwr_int(v_raus, "\n");
}


void blockstrecke_t::info(cbuffer_t & buf) const
{
  buf.append(v_rein-v_raus);
  buf.append(" ");
  buf.append(translator::translate("Wagen im Block"));
}
