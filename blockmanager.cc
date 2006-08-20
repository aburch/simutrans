/*
 * blockmanager.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "simdebug.h"
#include "simworld.h"
#include "simplay.h"
#include "simvehikel.h"
#include "dings/signal.h"
#include "dings/tunnel.h"
#include "dings/oberleitung.h"
#include "boden/grund.h"
#include "boden/wege/schiene.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "tpl/slist_tpl.h"
#include "tpl/array_tpl.h"

#include "blockmanager.h"
#include "find_block_way.h"

//------------------------- blockmanager ----------------------------

array_tpl<koord3d> &
blockmanager::finde_nachbarn(const karte_t *welt, const koord3d pos,
                             const ribi_t::ribi ribi, int &index)
{
	static array_tpl<koord3d> nb(4);

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos);
	const weg_t::typ way_type=from->gib_weg(weg_t::schiene) ? weg_t::schiene : weg_t::monorail;
	grund_t *to;

	index = 0;
	for(int r = 0; r < 4; r++) {
		if(
			(ribi & ribi_t::nsow[r])  &&
			from->get_neighbour(to, way_type, koord::nsow[r])
//			((to->gib_weg_ribi_unmasked(weg_t::schiene)|to->gib_weg_ribi_unmasked(weg_t::monorail)) & ribi_t::rueckwaerts(ribi_t::nsow[r]))
		) {
			nb.at(index++) = to->gib_pos();
		}
	}

	return nb;
}


blockmanager * blockmanager::single_instance = NULL;



blockmanager::blockmanager() : marker(0,0)
{
}

void blockmanager::setze_welt_groesse(int w,int h)
{
    marker.init(w,h);
    repair_blocks.clear();
}


blockmanager * blockmanager::gib_manager()
{
    if(single_instance == NULL) {
        single_instance = new blockmanager();
    }

    return single_instance;
}


void blockmanager::neue_schiene(karte_t *welt, grund_t *gr, ding_t *sig)
{
    schiene_t *sch = get_block_way(gr);
    ribi_t::ribi ribi = sch->gib_ribi_unmasked();
    int anzahl;

    if(sig) {
	if(sch->gib_blockstrecke().is_bound()) {
	    sch->gib_blockstrecke()->entferne_signal((signal_t *)sig);
	}
	gr->obj_pri_add(sig, ((signal_t *)sig)->gib_richtung() & ribi_t::suedost ? PRI_HOCH : 0);
	ribi &= ~((signal_t *)sig)->gib_richtung();
    }
    array_tpl<koord3d>  &nb = finde_nachbarn(welt, gr->gib_pos(), ribi, anzahl);
    blockhandle_t bs1, bs2;
    schiene_t *nachbar_weg;

    switch(anzahl) {
    case 0:
        bs1 = blockstrecke_t::create(welt);
        strecken.insert(bs1);
        sch->setze_blockstrecke( bs1 );
	break;
    case 1:
        nachbar_weg = get_block_way(welt->lookup(nb.at(0)));
        bs1 = nachbar_weg->gib_blockstrecke();
        sch->setze_blockstrecke( bs1 );
	break;
    case 2:
        nachbar_weg = get_block_way(welt->lookup(nb.at(0)));
        bs1 = nachbar_weg->gib_blockstrecke();
        sch->setze_blockstrecke( bs1 );
        nachbar_weg = get_block_way(welt->lookup(nb.at(1)));
        bs2 = nachbar_weg->gib_blockstrecke();
        if(bs1 != bs2) {
            vereinige(welt, bs1, bs2);
        }
	break;
    default:
        dbg->fatal("blockmanager::neues_schienen_stueck()","unknown case %d in bockmanager::neue_schiene()",anzahl);
    }
    if(sig) {
	sch->gib_blockstrecke()->add_signal((signal_t *)sig);
	if(((signal_t *)sig)->ist_blockiert()) {
	    sch->setze_ribi_maske(((signal_t *)sig)->gib_richtung());
	}
    }
}


void blockmanager::schiene_erweitern(karte_t *welt, grund_t *gr)
{
    schiene_t *sch = get_block_way(gr);
    ribi_t::ribi ribi = sch->gib_ribi_unmasked();
    int anzahl;
    array_tpl<koord3d>  &nb = finde_nachbarn(welt, gr->gib_pos(), ribi, anzahl);
    blockhandle_t bs1, bs2;
    schiene_t *nachbar_weg;
    signal_t *sig;

    // falls wir nachbarn haben
    if(anzahl > 0) {
	// blockstrecke vom feld
        bs1 = sch->gib_blockstrecke();

        assert(bs1.is_bound());

	// Aufpassen bei Schienenbau direkt neben dem Signal
	sig = bs1->gib_signal_bei(gr->gib_pos());

        // nachbarn verbinden
        for(int i=0; i<anzahl; i++) {
	        nachbar_weg = get_block_way(welt->lookup(nb.at(i)));
	        bs2 = nachbar_weg->gib_blockstrecke();

            if(bs1 != bs2 &&
		(!sig || sig->gib_richtung() != ribi_typ(gr->gib_pos().gib_2d(), nb.at(i).gib_2d()))) {
		vereinige(welt, bs1, bs2);
            }
        }
    }
}



blockhandle_t
blockmanager::finde_blockstrecke(karte_t * welt, koord3d pos)
{
	// sanity check
	blockhandle_t bs;
	if(welt != NULL && welt->lookup(pos) != NULL) {
		schiene_t *sch = dynamic_cast<schiene_t*>(welt->lookup(pos)->gib_weg_nr(0));
		if(!sch) {
			sch = dynamic_cast<schiene_t*>(welt->lookup(pos)->gib_weg_nr(1));
		}
		if(sch) {
			bs = sch->gib_blockstrecke();
		}
		else {
			dbg->error("blockmanager::finde_blockstrecke()","not found at %i,%i,%i", pos.x, pos.y, pos.z);
		}
	}
	DBG_MESSAGE("blockmanager::finde_blockstrecke()","found rail block %ld at %d,%d", bs.get_id(), pos.x, pos.y);
	return bs;
}



void
blockmanager::vereinige(karte_t *, blockhandle_t  bs1, blockhandle_t  bs2)
{
	DBG_MESSAGE("blockmanager::vereinige()","joining rail blocks %ld and %ld.", bs1.get_id(), bs2.get_id());

	assert(bs1.is_bound());

	// prüfe, ob nur eine strecke
	if(bs1 == bs2) {
		return;
	}

	if(!bs2.is_bound()) {
		return;
	}

	slist_iterator_tpl <weg_t *> iter (weg_t::gib_alle_wege());
	while(iter.next()) {
		schiene_t *sch = dynamic_cast<schiene_t *>(iter.get_current());
		if(sch) {
                if(sch) {
                    blockhandle_t bs = sch->gib_blockstrecke();
                    assert(bs.is_bound());
                    if(bs == bs2) {
                        sch->setze_blockstrecke(bs1);
                    }
                }
		}
	}

	// vereinige signale
	bs2->verdrahte_signale_neu();
	strecken.remove( bs2 );

	// pruefe, ob strecke frei
	bs1->vereinige_vehikel_counter(bs2);

DBG_MESSAGE("blockmanager::vereinige()", "deleting rail block %ld", bs2.get_id());
	blockstrecke_t::destroy( bs2 );

DBG_MESSAGE("blockmanager::vereinige()", "%d rail blocks left after join", strecken.count());
}


/**
 * entfernt ein signal aus dem blockstreckennetz.
 */
bool
blockmanager::entferne_signal(karte_t *welt, koord3d pos)
{
	// partner des signals suchen
	grund_t * gr = welt->lookup(pos);
	schiene_t *sch=get_block_way(gr);

	blockhandle_t bs0 = sch->gib_blockstrecke();

	signal_t *sig = dynamic_cast <signal_t *> (gr->suche_obj(ding_t::signal));
	// prissi: to delete a presignal, we must use also this routine!
	if(sig==NULL) {
		sig = dynamic_cast <signal_t *> (gr->suche_obj(ding_t::presignal));
	}
	// prissi: to delete a choosesignal, we must use also this routine!
	if(sig==NULL) {
		sig = dynamic_cast <signal_t *> (gr->suche_obj(ding_t::choosesignal));
	}

	// look at all four corners
	int anzahl;
	array_tpl<koord3d> &nb = finde_nachbarn(welt, pos, sch->gib_ribi_unmasked(), anzahl);

	for(int i=0; i<anzahl; i++) {
		blockhandle_t bs=get_block_way(welt->lookup(nb.at(i)))->gib_blockstrecke();
		if(bs != bs0) {
			bs->loesche_signal_bei(nb.get(i));
		}
	}

	for(int i=0; i<anzahl; i++) {
		schiene_t *nachbar_weg = get_block_way(welt->lookup(nb.at(i)));
		blockhandle_t bs = nachbar_weg->gib_blockstrecke();

		if(bs != bs0) {
			block_ersetzer bes (welt, bs);
			bes.neu = bs0;
			marker.unmarkiere_alle();
			traversiere_netz(welt, nb.get(i), &bes);

			bs->verdrahte_signale_neu();
			strecken.remove(bs);
			blockstrecke_t::destroy( bs );
		}
	}

	bs0->loesche_signal_bei(pos);

	pruefer_ob_strecke_frei *pr = new pruefer_ob_strecke_frei(welt, bs0);
	marker.unmarkiere_alle();
	traversiere_netz(welt, pos, pr);
	bs0->setze_belegung( pr->count );
	delete pr;

	return true;
}


bool
blockmanager::entferne_schiene(karte_t *welt, koord3d pos)
{
    schiene_t *sch=get_block_way(welt->lookup(pos));
    int i;

    if(sch) {
        blockhandle_t bs0 = sch->gib_blockstrecke();
        const ribi_t::ribi ribi = sch->gib_ribi();
        int anzahl;

        array_tpl<koord3d> &nb = finde_nachbarn(welt, pos, ribi, anzahl);

        // wenn hier signale sind, muessen sie entfernt werden
        // zuerst das signal an diesem fleck

        if( bs0->loesche_signal_bei(pos) ) {
            // wenn es ein signal zu loeschen gab,
            // dann alle angrenzenden, das kann eigentlich nur
            // ein einziges sein, da signale mit min. 1 feld
            // abstand gebaut werden müssen
            for(int i=0; i<anzahl; i++) {
                schiene_t *nachbar_weg = get_block_way(welt->lookup(nb.at(i)));
                blockhandle_t bs = nachbar_weg->gib_blockstrecke();
                bs->loesche_signal_bei(nb.at(i));
            }
        }

//	printf("Anzahl %d\n", anzahl);

        if(anzahl == 0) {
            // das war das letze stueck einer schiene
            // die blockstrecke wird aufgelöst
            strecken.remove( bs0 );
            blockstrecke_t::destroy( bs0 );
            sch->setze_blockstrecke(blockhandle_t());
        } else if(anzahl == 1) {
            // das war ende einer schiene,
            // nichts weiter zu tun
            sch->setze_blockstrecke(blockhandle_t());
        } else {
            // es gibt jetzt offene enden
            // dafuer muessen neue Blockstrecken angelegt werden

            // max. 4 offene enden
            array_tpl< blockhandle_t > bs(4);
            bs.at(0) = bs0;

            block_ersetzer *bes = new block_ersetzer(welt, bs.at(0));
            sch->setze_blockstrecke(blockhandle_t());

            // max 3 davon ducrh neue bs ersetzen
            for(i=1; i<anzahl; i++) {
                bes->neu = bs.at(i) = blockstrecke_t::create(welt);
                marker.unmarkiere_alle();
                traversiere_netz(welt, nb.at(i), bes);
            }

            delete bes;
            bes = NULL;

            // signale neu verdrahten
            bs0->verdrahte_signale_neu();

            // suchen welche bs uebrig geblieben sind
            // nicht mehr vorhandene bs wegwerfen

            for(i=0; i<anzahl; i++) {
                bool exists = false;

                for(int j=0; j<anzahl; j++) {
                    schiene_t *nachbar_weg = get_block_way(welt->lookup(nb.at(j)));
                    if(bs.at(i) == nachbar_weg->gib_blockstrecke()) {
                        exists = true;
                        break;
                    }
                }

                if(!exists) {
                    strecken.remove(bs.at(i));
                    blockstrecke_t::destroy( bs.at(i) );
                }
            }

            // noch vorhande neue bs einfügen
            for(i=1; i<anzahl; i++) {
                if(bs.at(i).is_bound()) {
                    strecken.insert( bs.at(i) );
                }
            }

            // alle bs pruefen, ob sie frei sind
            for(i=0; i<anzahl; i++) {
                schiene_t *nachbar_weg = get_block_way(welt->lookup(nb.at(i)));
                blockhandle_t bs = nachbar_weg->gib_blockstrecke();
                pruefer_ob_strecke_frei *pr = new pruefer_ob_strecke_frei(welt, bs);
                marker.unmarkiere_alle();
                traversiere_netz(welt, nb.at(i), pr);
                bs->setze_belegung( pr->count );
                delete pr;
            }
        }
//	reset_nachbar_ribis(welt, pos, ribi);
    }
    return sch != NULL;
}



const char *
blockmanager::baue_neues_signal(karte_t *welt, spieler_t *sp, koord3d pos, koord3d pos2, schiene_t *sch, ribi_t::ribi dir, ding_t::typ type)
{
    DBG_MESSAGE("blockmanager::baue_neues_signal()", "build a new signal between %d,%d and %d,%d", pos.x, pos.y, pos2.x, pos2.y);

    blockhandle_t bs1 = sch->gib_blockstrecke();
    blockhandle_t bs2 = blockstrecke_t::create(welt);

    // zweite blockstrecke erzeugen
    marker.unmarkiere_alle();
    schiene_t *sch2 = get_block_way(welt->lookup(pos2));

    sch2->ribi_rem(ribi_t::rueckwaerts(dir));
    block_ersetzer *bes = new block_ersetzer(welt, bs1);
    bes->neu = bs2;
    traversiere_netz(welt, pos2, bes);
    strecken.insert( bs2 );
	delete bes;

    // signale neu verdrahten
    bs1->verdrahte_signale_neu();

	// war es eine ringstrecke ?
	if(sch->gib_blockstrecke() == bs2) {
		// dann wird bs1 nicht mehr verwendet
		strecken.remove(bs1);
		blockstrecke_t::destroy(bs1);
		bs1 = bs2;
		dbg->warning("blockmanager::baue_neues_signal()", "circular rail block detected.");
	}

	sch2->ribi_add(ribi_t::rueckwaerts(dir));
	signal_t *sig1=NULL, *sig2=NULL;	// to keep compiler happy ...
	switch(type) {
		case ding_t::signal:
			sig1 = new signal_t(welt, pos, dir);
			sig2 = new signal_t(welt, pos2, ribi_t::rueckwaerts(dir));
			break;
		case ding_t::presignal:
			sig1 = new presignal_t(welt, pos, dir);
			sig2 = new presignal_t(welt, pos2, ribi_t::rueckwaerts(dir));
			break;
		case ding_t::choosesignal:
			sig1 = new choosesignal_t(welt, pos, dir);
			sig2 = new choosesignal_t(welt, pos2, ribi_t::rueckwaerts(dir));
			break;
		default:
			dbg->fatal("blockmanager::baue_neues_signal()","illegal signal type %d used!",type);
	}

	bs1->add_signal(sig1);
	bs2->add_signal(sig2);

	welt->lookup(pos)->obj_add(sig1);
	welt->lookup(pos2)->obj_pri_add(sig2, PRI_HOCH);

	sig1->setze_blockiert( false );
	sig2->setze_blockiert( false );

	// jetzt muss man noch prüfen welche der beiden bs belegt ist
	// da bs1 jetzt evtl (Ringstrecke!) nicht mehr exitiert müssen
	// sicherheitshalber beide strecken geprüft werden
	pruefer_ob_strecke_frei pr1(welt, bs1);
	marker.unmarkiere_alle();
	traversiere_netz(welt, pos, &pr1);
	bs1->setze_belegung( pr1.count );

	pruefer_ob_strecke_frei pr2(welt, bs2);
	marker.unmarkiere_alle();
	traversiere_netz(welt, pos2, &pr2);
	bs2->setze_belegung( pr2.count );

	if(sp != NULL) {
		sp->buche(umgebung_t::cst_signal, pos.gib_2d(), COST_CONSTRUCTION);
	}
	return NULL;
}

const char *
blockmanager::baue_andere_signale(koord3d pos1, koord3d pos2,
                                  schiene_t *sch1, schiene_t *sch2,
                                  ribi_t::ribi ribi)
{
    const ribi_t::ribi rueck = ribi_t::rueckwaerts(ribi);

    signal_t *sig1 = sch1->gib_blockstrecke()->gib_signal_bei(pos1);
    signal_t *sig2 = sch2->gib_blockstrecke()->gib_signal_bei(pos2);

	// prissi: todo: we should check, if there was a change to a presignal requested!

    DBG_MESSAGE("blockmanager::baue_andere_signale()",
                 "between %d,%d and %d,%d",
                 pos1.x, pos1.y, pos2.x, pos2.y);


    if(sig1 == NULL || sig2 == NULL) {
        // irgendwie ist hier ein signal abhanden gekommen
        // wir brechen sicherheitshalber ab

        dbg->warning("blockmanager::baue_andere_signale()",
                     "Need a pair of signals!");

        return "Zu nah an einem\nanderen Signal!\n";
    }

    bool b1 = sig1->ist_blockiert();
    bool b2 = sig2->ist_blockiert();

    if(b1 == false && b2 == false) {
        sig1->setze_blockiert( false );
        sig2->setze_blockiert( true );

        sch1->setze_ribi_maske(ribi_t::keine);
        sch2->setze_ribi_maske(rueck);

    } else if(b1 == false && b2 == true) {
        sig1->setze_blockiert( true );
        sig2->setze_blockiert( false );

        sch1->setze_ribi_maske(ribi);
        sch2->setze_ribi_maske(ribi_t::keine);

    } else {
        // hier fallen zwei fälle zusammen
        // 1.) beide sig blockiert
        // 2.) b1==true && b2 == false

        sig1->setze_blockiert( false );
        sig2->setze_blockiert( false );

        sch1->setze_ribi_maske(ribi_t::keine);
        sch2->setze_ribi_maske(ribi_t::keine);
    }

    return NULL;
}

const char *
blockmanager::neues_signal(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir, ding_t::typ type)
{
    schiene_t *sch = get_block_way(welt->lookup(pos));
    if(sch) {
        const ribi_t::ribi ribi = sch->gib_ribi();

        // pruefe ob dir gültig
        if(dir!=ribi_t::nord && dir != ribi_t::west) {
            dbg->warning("blockmanager::neues_signal()", "invalid direction %d!", dir);
            return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
        }

        koord3d pos2 ( pos+koord(dir) );

        // pruefe ob pos2 in der karte liegt

        if(welt->lookup(pos2)==NULL) {
            // kein boden
            dbg->warning("blockmanager::neues_signal()", "no ground on square!");
            return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
        }

        // pruefe ob an pos2 geeignete schienen sind
        schiene_t *sch2 = get_block_way(welt->lookup(pos2));
        if(!sch2) {
            // keine schiene
            dbg->warning("blockmanager::neues_signal()", "no railroad track on square!");
            return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
        }

        // pruefe ob schon signale da

        signal_t *sig1 = sch->gib_blockstrecke()->gib_signal_bei(pos);
        signal_t *sig2 = sch2->gib_blockstrecke()->gib_signal_bei(pos2);

        if(sig1 != NULL && sig2 != NULL) {
            // es gibt signale

            return baue_andere_signale(pos, pos2, sch, sch2, dir);
        } else if(sig1 == NULL && sig2 == NULL) {
            // es gibt noch keine Signale

            if(!(dir & ribi)) {
                dbg->warning("blockmanger_t::neues_signal()", "direction mismatch: dir=%d, ribi=%d!", dir, ribi);
                return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
            }
            return baue_neues_signal(welt, sp, pos, pos2, sch, dir, type);

        } else {
            // ein signal muss verlorengegangen sein
            return "Zu nah an einem\nanderen Signal!\n";
        }
    }
    return NULL;
}


bool blockmanager::entferne_signal(signal_t *sig, blockhandle_t bs)
{
    bool ok = false;

    if(bs.is_bound()) {
        ok = bs->entferne_signal( sig );
    }

    if(!ok) {
        // das signal muss zu einer Blockstrecke gehört haben
        // offensichtlich ist bs falsch ermittelt, deshalb pruefen
        // wir hier alle Blockstrecken

        dbg->warning("blockmanager::entferne_signal()",
                     "signal %p was not registered by rail block %ld when removed!", sig, bs.get_id());

        slist_iterator_tpl<blockhandle_t > iter ( strecken );

        while(iter.next() && !ok) {
            blockhandle_t bs = iter.get_current();
            ok = bs->entferne_signal( sig );
        }

        if(!ok) {
            dbg->error("blockmanager::entferne_signal()",
                       "signal %p was not registered at any rail block when removed!", sig);
        }
    }
    return ok;
}



/* if an unmarked block is found with this blockhandle, it will given a new id to repair the blocks
 * return true, if something was repaired
 */
bool
blockmanager::repair_identical_marked_blocks(karte_t *welt, blockhandle_t bs)
{
	// now iterate over all ways with this type
	// there should be no blockstrecke left with this id!
	slist_iterator_tpl <weg_t *> iter (weg_t::gib_alle_wege());
	while(iter.next()) {
		grund_t *gr = welt->lookup(iter.get_current()->gib_pos());
		if(!marker.ist_markiert(gr)) {
			schiene_t *test_sch = get_block_way(gr);
			if(test_sch  &&  test_sch->gib_blockstrecke()==bs) {
				// found unconnected block here! Have to give it a new number an must start again
				blockhandle_t bs_neu = blockstrecke_t::create(welt);
				marker.unmarkiere_alle();
				block_ersetzer *bes = new block_ersetzer(welt, bs);
				bes->neu = bs_neu;
				traversiere_netz(welt, gr->gib_pos(), bes);
				strecken.insert( bs_neu );
				// redistribute signals
				bs->verdrahte_signale_neu();
				bs_neu->verdrahte_signale_neu();
				delete bes;
				// now correct the counter
				pruefer_ob_strecke_frei pr(welt, bs_neu);
				marker.unmarkiere_alle();
				traversiere_netz(welt, gr->gib_pos(), &pr);
				bs_neu->setze_belegung( pr.count );
				// we must now also recorrect the original block
DBG_MESSAGE("blockmanager::repair_identical_marked_blocks()", "block id %d was doubled, new id is %d.",bs.get_id(), bs_neu.get_id() );
				return true;
			}
		}
	}
	// nothing found => all blocks seem ok
	return false;
}



void
blockmanager::pruefe_blockstrecke(karte_t *welt, koord3d k)
{
	const grund_t *gr = welt->lookup(k);
	if(gr) {
		schiene_t *sch = get_block_way(gr);
		if(sch) {
			blockhandle_t bs = sch->gib_blockstrecke();
			if(bs->gib_signale().count()==0) {
				// no signals in this block?!? then there is an error
DBG_MESSAGE("blockmanager::pruefe_blockstrecke()", "block id %d no signals => try replace", bs.get_id());
				// first replace block (will recalc all signals)
				marker.unmarkiere_alle();
				marker.unmarkiere_alle();
				check_block_borders *cbb = new check_block_borders(welt, bs);
				traversiere_netz(welt, gr->gib_pos(), cbb);
				// repair signals
				bs->verdrahte_signale_neu();
				slist_iterator_tpl<blockhandle_t> changed_bs_iter (cbb->changed_bs);
				while(changed_bs_iter.next()) {
					changed_bs_iter.get_current()->verdrahte_signale_neu();
				}
				delete cbb;
			}
			// now find out if there are other blocks with double ids
			int count=0;
			do {
				bs = sch->gib_blockstrecke();
				pruefer_ob_strecke_frei pr(welt, bs);
				marker.unmarkiere_alle();
				traversiere_netz(welt, k, &pr);
				count = pr.count;
			} while(repair_identical_marked_blocks(welt,bs));
DBG_MESSAGE("blockmanager::pruefe_blockstrecke()", "block id %d setting waggon counter to %d waggons", bs.get_id(), count);
			bs->setze_belegung( count );
		}
	}
}


void blockmanager::setze_tracktyp(karte_t *welt, koord3d k, uint8 styp)
{
    const grund_t *gr = welt->lookup(k);
    if(gr) {
        schiene_t *sch = get_block_way(gr);
        if(sch) {
            blockhandle_t bs = sch->gib_blockstrecke();
            tracktyp_ersetzer pr (welt, bs, styp);
            marker.unmarkiere_alle();
            traversiere_netz(welt, k, &pr);
        }
    }
}


void
blockmanager::traversiere_netz(const karte_t *welt, const koord3d start, koord_beobachter *cmd)
{
	weg_t::typ wegtype;
	grund_t *test_gr = welt->lookup(start);
	if(dynamic_cast<schiene_t *>(test_gr->gib_weg_nr(0))) {
		wegtype = test_gr->gib_weg_nr(0)->gib_typ();
	}
	else 	if(dynamic_cast<schiene_t *>(test_gr->gib_weg_nr(1))) {
		wegtype = test_gr->gib_weg_nr(1)->gib_typ();
	}
	else {
		dbg->error("blockmanager::traversiere_netz()","unknown waytype!");
		return;	// nothing found
	}
//	DBG_MESSAGE("blockmanager::traversiere_netz()","waytype %i",wegtype);

	// die Berechnung erfolgt durch eine Breitensuche fuer Graphen
	slist_tpl <koord3d> queue;         // Warteschlange fuer Breitensuche
	koord3d tmp (start);
	queue.append( tmp );        // init queue mit erstem feld

	// Breitensuche
	do {
		tmp = queue.remove_first();
		marker.markiere(welt->lookup(tmp));  // betretene Felder markieren

		if(cmd->neue_koord(tmp) == false) {    // noch nicht fertig ?
			// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
			grund_t *from = welt->lookup(tmp);
			grund_t *to;

			for(int r=0; r<4; r++) {
				if(from->get_neighbour(to, wegtype, koord::nsow[r])) {
					if(cmd->ist_uebergang_ok(tmp, to->gib_pos())) {
						if(!marker.ist_markiert(to)) {
//DBG_MESSAGE("blockmanager::traversiere_netz()","to (%d,%d,%d)", to->gib_pos().x, to->gib_pos().y, to->gib_pos().z);
							queue.append( to->gib_pos() );
						} else {
							// nochmal angefasst
							cmd->wieder_koord(to->gib_pos());
						}
					}
				}
			}
		}
	} while(queue.count());
}


void
blockmanager::rdwr(karte_t *welt, loadsave_t *file)
{
	int count;

	if(file->is_loading()) {
		// alle strecken aufräumen
		delete_all_blocks();
	}
	else {
		count = strecken.count();
	}
	file->rdwr_long(count, "\n");

	if(file->is_loading()) {
		for(int i=0; i<count; i++) {
			strecken.append(blockstrecke_t::create(welt, file));
		}
	}
	else {
		slist_iterator_tpl<blockhandle_t > s_iter ( strecken );
		while(s_iter.next()) {
			if(s_iter.get_current().is_bound()) {
				s_iter.get_current()->rdwr(file);
			}
			else {
				dbg->warning("blockmanager::rdwr()","skipping unused railblock during save!");
			}
		}
	}
}

void
blockmanager::laden_abschliessen()
{
	DBG_MESSAGE("blockmanager::laden_abschliessen()","for %i blocks",strecken.count() );
	// first call laden_abschliessen for all
	slist_iterator_tpl< blockhandle_t > finish_iter ( strecken );
	while(finish_iter.next()) {
		finish_iter.get_current()->laden_abschliessen();
	}

	// now repair all blocks that had invalid ids during loading
	slist_iterator_tpl <blockhandle_t> repair_iter (repair_blocks);
	while(repair_iter.next()) {
		blockhandle_t repair_bs=repair_iter.get_current();
		DBG_MESSAGE("blockmanager::laden_abschliessen()","repair block id %d",repair_bs.get_id() );
		// find the first way which this id ...
		slist_iterator_tpl <weg_t *> iter (weg_t::gib_alle_wege());
		while(iter.next()) {
			schiene_t *sch = dynamic_cast<schiene_t *>(iter.get_current());
			if(sch  &&  repair_bs==sch->gib_blockstrecke()) {
				pruefe_blockstrecke( repair_bs->gib_welt(), sch->gib_pos() );
				break;	// check next repair block
	  		}
		}
	}
	repair_blocks.clear();

	// finally remove all unused blocks ...

	// first copy them
	slist_iterator_tpl <blockhandle_t> all_iter (strecken);
	while(all_iter.next()) {
		repair_blocks.append( all_iter.get_current() );
	}

	// then delete all existing ones ...
	slist_iterator_tpl <weg_t *> iter (weg_t::gib_alle_wege());
	while(iter.next()) {
		schiene_t *sch = dynamic_cast<schiene_t *>(iter.get_current());
		if(sch) {
			repair_blocks.remove( sch->gib_blockstrecke() );
  		}
	}

	// the remaining are broken ...
	while(repair_blocks.count()>0) {
		blockhandle_t remove_bs = repair_blocks.remove_first();
		DBG_MESSAGE("blockmanager::laden_abschliessen()","remove empty block id %d",remove_bs.get_id() );
		strecken.remove(remove_bs);
		blockstrecke_t *bs_ptr=remove_bs.detach();
		delete bs_ptr;
	}

	// now all blocks are hopefully ok (but we did not check for double id for all blocks)
	// but this would take to long, so we left this to the player
}



void
blockmanager::delete_all_blocks()
{
	while(strecken.count() > 0) {
		blockhandle_t bs = strecken.at(0);
		strecken.remove(bs);
		blockstrecke_t::destroy( bs );
	}
	strecken.clear();
}


// ------------ innere klassen des blockmanagers ------------------


blockmanager::block_ersetzer::block_ersetzer(karte_t *welt,
                                             blockhandle_t alt)
{
     this->welt = welt;
     this->alt = alt;

     DBG_MESSAGE("blockmanager::block_ersetzer::block_ersetzer()","replacing rail block %ld", alt.get_id());
}

bool
blockmanager::block_ersetzer::neue_koord(koord3d pos)
{
	bool ok = true;   // assume we break traversal here
	schiene_t *sch = get_block_way(welt->lookup(pos));
	if(sch) {
		if(sch->gib_blockstrecke() == alt) {
//DBG_MESSAGE("blockmanager::block_ersetzer::neue_koord()","on %d %d: %d -> %d", pos.x, pos.y, alt.get_id(), neu.get_id());
			sch->setze_blockstrecke( neu );
			ok = false;   // do not stop traversal yet
		}
	}
	return ok;
}

bool
blockmanager::block_ersetzer::ist_uebergang_ok(koord3d pos1, koord3d pos2)
{
    // Sollte der pos2 nicht zu alt gehören, so bricht erst neue_koord() ab.
    signal_t *sig = alt->gib_signal_bei(pos2);

    // Nicht okay, falls Signale zwischen pos1 und pos2 den Weg versperren
    return !(sig && pos2.gib_2d() + koord(sig->gib_richtung()) == pos1.gib_2d());
}


void
blockmanager::block_ersetzer::wieder_koord(koord3d )
{
}



blockmanager::tracktyp_ersetzer::tracktyp_ersetzer(karte_t *welt, blockhandle_t alt, uint8 is_electric)
{
     this->welt = welt;
     this->alt = alt;
     this->electric = is_electric;
     DBG_MESSAGE("blockmanager::tracktyp_ersetzer::tracktyp_ersetzer()","replacing rail block %ld to electric track type %d",alt.get_id(), is_electric);
}

bool
blockmanager::tracktyp_ersetzer::neue_koord(koord3d pos)
{
    bool ok = true;   // assume we break traversal here

    grund_t *gr = welt->lookup(pos);
    schiene_t *weg = get_block_way(gr);
    if(weg) {
        if(weg->gib_blockstrecke() == alt) {
//	    DBG_MESSAGE("blockmanager::tracktyp_ersetzer::neue_koord()",
//                 "on %d %d: %d -> %d", pos.x, pos.y, alt.get_id(), neu.get_id());
            weg->setze_elektrisch( electric != 0);
	    gr->calc_bild();         // rail looks different now


	    // Hajo: there might be some special tasks to
	    // depending on the type of the track

	    // electrified tracks need overhead power lines

	    if(electric) {
	      oberleitung_t * obl = dynamic_cast<oberleitung_t *> (gr->suche_obj(ding_t::oberleitung));

	      if(obl == 0) {
		spieler_t * sp = gr->gib_besitzer();

		obl = new oberleitung_t(welt, pos, sp);
		gr->obj_add(obl);

		if(sp) {
		  sp->buche(umgebung_t::cst_third_rail,pos.gib_2d(),COST_CONSTRUCTION);
		} else {
		  dbg->error("blockmanager::tracktyp_ersetzer::neue_koord","Owner of track is NULL");
		}
	      }

	      obl->calc_bild();
	    }


            ok = false;   // do not stop traversal yet
	}
    }

//    DBG_MESSAGE("blockmanager::tracktyp_ersetzer::neue_koord()",
//                 "called on %d %d, result %d", pos.x, pos.y, ok);


    return ok;

}

bool
blockmanager::tracktyp_ersetzer::ist_uebergang_ok(koord3d pos1, koord3d pos2)
{
    // Sollte der pos2 nicht zu alt gehören, so bricht erst neue_koord() ab.
    signal_t *sig = alt->gib_signal_bei(pos2);

    // Nicht okay, falls Signale zwischen pos1 und pos2 den Weg versperren
    return !(sig && pos2.gib_2d() + koord(sig->gib_richtung()) == pos1.gib_2d());
}


void
blockmanager::tracktyp_ersetzer::wieder_koord(koord3d )
{
}






blockmanager::pruefer_ob_strecke_frei::pruefer_ob_strecke_frei(karte_t *w, blockhandle_t b)
{
    welt = w;
    bs = b;
    count = 0;
}

bool blockmanager::pruefer_ob_strecke_frei::neue_koord(koord3d k)
{
    const grund_t *gr = welt->lookup(k);
    if(gr) {
        schiene_t *weg = get_block_way(gr);

        if(weg && weg->gib_blockstrecke() == bs) {

            for(int i=0; i<gr->gib_top(); i++) {
                const ding_t *dt = gr->obj_bei(i);

                if(dt != NULL) {
                    ding_t::typ typ = dt->gib_typ();

//		    printf("Typ %d, %s bei %d, %d\n", typ, dt->name(), k.x, k.y);

                    if(typ == ding_t::waggon ||  typ == ding_t::monorailwaggon) {
                        count ++;
                    }
                }
            }

//	    printf("count %d\n", count);

            return false;

        } else {
            // das war eine fremde blockstrecke
            // hier geht's nicht weiter
            return true;
        }
    } else {
        // keine schien, hier geht's nicht weiter
        return true;
    }
}

bool blockmanager::pruefer_ob_strecke_frei::ist_uebergang_ok(koord3d /*pos1*/, koord3d /*pos2*/)
{
    return true;
}

void blockmanager::pruefer_ob_strecke_frei::wieder_koord(koord3d )
{
}



/* tries to correct all invalid signal borders ... */
blockmanager::check_block_borders::check_block_borders(karte_t *w, blockhandle_t b)
{
    welt = w;
    bs = b;
    changed_bs.clear();
}

/* replace the block */
bool
blockmanager::check_block_borders::neue_koord(koord3d pos)
{
	bool ok = true;   // assume we break traversal here
	schiene_t *sch = get_block_way(welt->lookup(pos));
	if(sch) {
		blockhandle_t alt=sch->gib_blockstrecke();
		if(alt!=bs) {
			if(!changed_bs.contains(alt)) {
				changed_bs.append(alt);
DBG_MESSAGE("blockmanager::check_block_borders::neue_koord()","on %d %d: %d -> %d", pos.x, pos.y, alt.get_id(), bs.get_id());
			}
			sch->setze_blockstrecke(bs);
		}
		ok = false;   // do not stop traversal yet
	}
	return ok;
}

/* stop only at a new signal */
bool
blockmanager::check_block_borders::ist_uebergang_ok(koord3d pos1, koord3d)
{
	grund_t *gr=welt->lookup(pos1);
	if(gr  &&  (gr->suche_obj(ding_t::signal)  ||  gr->suche_obj(ding_t::presignal)  ||  gr->suche_obj(ding_t::choosesignal))) {
		return false;
	}
	return true;
}


void
blockmanager::check_block_borders::wieder_koord(koord3d )
{
}
