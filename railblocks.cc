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
#include "convoihandle_t.h"
#include "simconvoi.h"	// for debugging

#include "railblocks.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "dings/signal.h"
#include "boden/wege/schiene.h"
#include "boden/grund.h"
#include "utils/cbuffer_t.h"

//#define DEBUG 1


#include "find_block_way.h"

karte_t *blockstrecke_t::welt=NULL;


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
	cnv_reserved = convoihandle_t();
	rdwr(file);
}



blockstrecke_t::blockstrecke_t(karte_t *w)
{
	v_rein = v_raus = 0;
	cnv_reserved = convoihandle_t();
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
		grund_t *gr=welt->lookup(sig->gib_pos());

		if(gr==NULL) {
			dbg->error("blockstrecke_t::verdrahte_signale_neu()", "signal on illegal square (%i,%i,%i): removing signal!",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z);
			kill_list.insert( sig );
			// we just remove it to avoid crashes
			continue;
		}

		schiene_t *sch = get_block_way( gr );
		if(sch == NULL) {
			dbg->error("blockstrecke_t::verdrahte_signale_neu()", "signal on square (%i,%i,%i) without railroad track: removing signal!",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z);
			gr->obj_remove(sig,sig->gib_besitzer());
			kill_list.insert( sig );
			// we just remove it from the map but do not delete it, to avoid crashes
			continue;
		}

		// check, if this signal is unique ...
		ribi_t::ribi dir = sig->gib_richtung();
		for(int i=0;  i<gr->gib_top()  && sig;  i++  ) {
			ding_t *d=gr->obj_bei(i);
			if(d==sig) {
				// our own is ok ...
				break;
			}
			if(d) {
				// check for signal
				if(d->gib_typ()==ding_t::signal  ||  d->gib_typ()==ding_t::presignal  ||  d->gib_typ()==ding_t::choosesignal) {
					if(dynamic_cast<signal_t *>(d)->gib_richtung()==dir) {
						// already there ...
						dbg->error("blockstrecke_t::verdrahte_signale_neu()", "mutiple signal on square (%i,%i,%i): removing dubletts!",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z);
						kill_list.insert( sig );
						sig = NULL;
					}
				}
			}
		}

		if(sig  &&  sch->gib_blockstrecke().operator->() != this) {
			if(!sch->gib_blockstrecke().is_bound()) {
				dbg->error("blockstrecke_t::verdrahte_signale_neu()","invalid blockstrecke at signal on square (%i,%i,%i): removing!",sig->gib_pos().x,sig->gib_pos().y,sig->gib_pos().z);
			}
			else {
				// signal neu verdrahten
				sch->gib_blockstrecke()->add_signal(sig);
			}
			kill_list.insert( sig );
		  }
	}

	// remove all wrong ones ...
	slist_iterator_tpl<signal_t *> killer( kill_list );
	while(killer.next()) {
		signal_t *sig = killer.get_current();
		signale.remove( sig );
	}

//DBG_MESSAGE("blockstrecke_t::verdrahte_signale_neu()","rail block %p has now %d signals", this, signale.count());
}



void blockstrecke_t::add_signal(signal_t *sig)
{
	assert(sig != NULL);

	if(!signale.contains( sig )) {
		signale.insert(sig);
		sig->setze_zustand(ist_frei() ? signal_t::gruen : signal_t::rot);
	} else {
		dbg->warning("blockstrecke_t::add_signal()","Signal %p already attached to rail block %p.",sig, this);
	}

    dbg->warning("blockstrecke_t::add_signal()",
		 "rail block %p has after adding signal %p at %d,%d  %d signals",
		 this, sig, sig->gib_pos().x, sig->gib_pos().y,
		 signale.count());
}



bool blockstrecke_t::loesche_signal_bei(koord3d k)
{
	bool del=false;
	signal_t * sig = gib_signal_bei( k );
	if(sig) {
DBG_MESSAGE("blockstrecke_t::loesche_signal_bei()","rail block %p removes signal %p at %hd,%hd", this, sig, k.x, k.y);
		// der Destruktor entfernt das Signal aus allen Listen
		delete sig;
		del = true;
	}
	else {
		// check for orphan signal
		if(welt->lookup(k)) {
			choosesignal_t *cs = dynamic_cast<choosesignal_t *>(welt->lookup(k)->suche_obj(ding_t::choosesignal));
			if(cs) {
				dbg->warning("blockstrecke_t::loesche_signal_bei()","orphan choosesignal %p detected at %hd,%hd",sig, k.x, k.y);
				delete cs;
				del = true;
			}
			else {
				presignal_t *presig = dynamic_cast<presignal_t *>(welt->lookup(k)->suche_obj(ding_t::presignal));
				if(presig) {
					dbg->warning("blockstrecke_t::loesche_signal_bei()","orphan presignal %p detected at %hd,%hd",sig, k.x, k.y);
					delete presig;
					del = true;
				}
				else {
					sig = dynamic_cast<signal_t *>(welt->lookup(k)->suche_obj(ding_t::signal));
					if(sig) {
						dbg->warning("blockstrecke_t::loesche_signal_bei()","orphan signal %p detected at %hd,%hd",sig, k.x, k.y);
						delete sig;
						del = true;
					}
				}
			}
		}
	}
	// gab's ueberhaupt was zu loeschen ?
	return del;
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

//	if(sig==NULL) {DBG_MESSAGE("blockstrecke_t::gib_signal_bei()","no signal found at %d,%d.", k.x, k.y);}
	return sig;
}



bool
blockstrecke_t::ist_frei() const
{
	if(v_rein==v_raus) {
		// is empty => check reservation
		if(cnv_reserved.is_bound()) {
			// another convoi wants to enter here already
			return false;
		}
		return true;
	}
	return false;
}



bool
blockstrecke_t::can_reserve_block(convoihandle_t cnv)
{
	if(v_rein==v_raus) {
		// is empty => check reservation
		if(cnv_reserved!=cnv  &&  cnv_reserved.is_bound()) {
			// not our reservation => fail
			return false;
		}
		return true;
	}
	return false;
}



bool
blockstrecke_t::reserve_block(convoihandle_t cnv)
{
	if(v_rein==v_raus) {
		// is empty => check reservation
		if(cnv_reserved==cnv  ||  !cnv_reserved.is_bound()) {
			// our reservation => ok
			cnv_reserved = cnv;
			return true;
		}
	}
	return false;
}



bool
blockstrecke_t::unreserve_block(convoihandle_t cnv)
{
	if(cnv_reserved==cnv) {
		cnv_reserved = convoihandle_t();
		return true;
	}
	return false;
}



void blockstrecke_t::schalte_signale()
{
	const signal_t::signalzustand zustand = ist_frei() ? signal_t::gruen : signal_t::rot;

	slist_iterator_tpl<signal_t *> iter( signale );
	while(iter.next()) {
		signal_t *sig = iter.get_current();
		sig->setze_zustand( zustand );
	}
}



void blockstrecke_t::betrete(vehikel_basis_t *)
{
	v_rein ++;
	if(cnv_reserved.is_bound()) {
		cnv_reserved = convoihandle_t();
	}
	schalte_signale();
//    printf("   betrete %p: zustand  %d, %d\n", this, v_rein, v_raus);
}



void blockstrecke_t::verlasse(vehikel_basis_t *)
{
	v_raus ++;
	schalte_signale();

	if(v_raus >= v_rein) {
		// vermeide Zaehlerueberlauf
		v_raus = v_rein = 0;
	}
//    printf("   verlasse %p: zustand %d, %d\n", this, v_rein, v_raus);
}



void blockstrecke_t::setze_belegung(int count)
{
	v_raus = 0;
	v_rein = count;
	cnv_reserved = convoihandle_t();
	schalte_signale();
}



void
blockstrecke_t::vereinige_vehikel_counter(blockhandle_t bs)
{
	v_rein += bs->v_rein;
	v_raus += bs->v_raus;
	cnv_reserved = convoihandle_t();
	schalte_signale();
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
	file->rdwr_long(count, "\n");

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
			switch(typ) {
				case ding_t::signal:
					sig = new signal_t(welt, file);
					break;
				case ding_t::presignal:
					sig = new presignal_t(welt, file);
					break;
				case ding_t::choosesignal:
					sig = new choosesignal_t(welt, file);
					break;
				default:
					dbg->fatal("blockstrecke_t::rdwr()","ding %d is not a signal!",typ);
			}
			signale.insert( sig );
		}
	}

	// counters
	if(file->get_version()<=88005) {
		// old style
		long dummy = 0;
		file->rdwr_long(dummy, " ");
		file->rdwr_long(dummy, "\n");
		v_rein = 0;
		v_raus = (uint16)-1;	// unreserved block
	}
	else  {
		// new style => 16 Bit counter and a reservation handle
		// not yet reserved (should be different!)
		if(file->is_loading()) {
			v_rein = 0;	// will be set by the new vehicles anyway ...
			// since convois are not yet loaded, read the current number of the convoi
			file->rdwr_short(v_raus, "\n");
DBG_MESSAGE("blockstrecke_t::laden_abschliessen()","v_raus = %i",v_raus );
			cnv_reserved = convoihandle_t();
		}
		else {
			uint16 id = cnv_reserved.is_bound() ? (uint16)welt->gib_convoi_list().index_of(cnv_reserved) : (uint16)-1;
			file->rdwr_short(id, "\n");
		}
	}

	// the v_rein and v_raus are better not saved
	// the counter will be regenerated automatically during reloading!
}



// during loading v_raus actual value should be zero
// however it will temporary contain the number of the last convoi that reseverd that block
void blockstrecke_t::laden_abschliessen()
{
	// reassign reservations
	if(v_raus==(uint16)-1) {
		cnv_reserved = convoihandle_t();
	}
	else {
		cnv_reserved = welt->gib_convoi_list().at(v_raus)->self;
DBG_MESSAGE("blockstrecke_t::laden_abschliessen()","was reserved by %s (%i)",cnv_reserved->gib_name(),v_raus );
	}
	v_raus = 0;

	// add signals to map
	slist_iterator_tpl<signal_t *> iter( signale );
	while(iter.next()) {
		signal_t *sig = iter.get_current();
		grund_t *gr=welt->lookup(sig->gib_pos());

		assert(gr!=NULL);
		assert(get_block_way(welt->lookup(sig->gib_pos()))!=NULL);

		ribi_t::ribi dir = sig->gib_richtung();
		if(dir == ribi_t::nord || dir == ribi_t::ost) {
			gr->obj_add(sig);
		}
		else {
			gr->obj_pri_add(sig, PRI_HOCH);
		}
	}

	// remove wrong signals
	verdrahte_signale_neu();
}



void blockstrecke_t::info(cbuffer_t & buf) const
{
	buf.append(v_rein-v_raus);
	buf.append(" ");
	buf.append(translator::translate("Wagen im Block"));
	if(cnv_reserved.is_bound()) {
		buf.append(translator::translate("(reserved)"));
	}
#if 1
	// debug:
	buf.append("\nnumber of signals ");buf.append(signale.count());
	slist_iterator_tpl<signal_t *> iter( signale );
	while(iter.next()) {
		signal_t *sig = iter.get_current();
		buf.append("\n(");buf.append(sig->gib_pos().x);
		buf.append(",");buf.append(sig->gib_pos().y);
		buf.append(",");buf.append(sig->gib_pos().z);
		buf.append(") ");buf.append((int)sig->gib_richtung());
	}
#endif
}
