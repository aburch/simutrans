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
#include "simcosts.h"
#include "dataobj/loadsave.h"

#include "tpl/slist_tpl.h"
#include "tpl/array_tpl.h"

#include "blockmanager.h"

//------------------------- blockmanager ----------------------------

array_tpl<koord3d> &
blockmanager::finde_nachbarn(const karte_t *welt, const koord3d pos,
                             const ribi_t::ribi ribi, int &index)
{
    static array_tpl<koord3d> nb(4);

    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
    grund_t *from = welt->lookup(pos);
    grund_t *to;

    index = 0;

    for(int r = 0; r < 4; r++) {
        if((ribi & ribi_t::nsow[r]) &&
	    from->get_neighbour(to, weg_t::invalid, koord::nsow[r]) &&
	(to->gib_weg_ribi_unmasked(weg_t::schiene) & ribi_t::rueckwaerts(ribi_t::nsow[r]))) {
    	    nb.at(index++) = to->gib_pos();
	}
    }

    return nb;
}


blockmanager * blockmanager::single_instance = NULL;


blockmanager::blockmanager()
{
}

void blockmanager::setze_welt_groesse(int w)
{
    marker.init(w);
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
    schiene_t *sch = (schiene_t *)gr->gib_weg(weg_t::schiene);
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
    weg_t *nachbar_weg;

    switch(anzahl) {
    case 0:
        bs1 = blockstrecke_t::create(welt);
        strecken.insert(bs1);
        sch->setze_blockstrecke( bs1 );
	break;
    case 1:
        nachbar_weg = welt->lookup(nb.at(0))->gib_weg(weg_t::schiene);
        bs1 = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();
        sch->setze_blockstrecke( bs1 );
	break;
    case 2:
        nachbar_weg = welt->lookup(nb.at(0))->gib_weg(weg_t::schiene);
        bs1 = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();
        nachbar_weg = welt->lookup(nb.at(1))->gib_weg(weg_t::schiene);
        bs2 = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();

        sch->setze_blockstrecke( bs1 );
        if(bs1 != bs2) {
            vereinige(welt, bs1, bs2);
        }
	break;
    default:
        dbg->fatal("blockmanager::neues_schienen_stueck()",
                   "unknown case in bockmanager::neue_schiene()");
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
    schiene_t *sch = (schiene_t *)gr->gib_weg(weg_t::schiene);
    ribi_t::ribi ribi = sch->gib_ribi_unmasked();
    int anzahl;
    array_tpl<koord3d>  &nb = finde_nachbarn(welt, gr->gib_pos(), ribi, anzahl);
    blockhandle_t bs1, bs2;
    weg_t *nachbar_weg;
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
	    nachbar_weg = welt->lookup(nb.at(i))->gib_weg(weg_t::schiene);
            bs2 = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();

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
        weg_t *weg = welt->lookup(pos)->gib_weg(weg_t::schiene);
        if(weg) {
            bs = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();
        }
    }

    dbg->message("blockmanager::finde_blockstrecke()",
                 "found rail block %ld at %d,%d", bs.get_id(), pos.x, pos.y);

    return bs;
}

void
blockmanager::vereinige(karte_t *welt,
                        blockhandle_t  bs1,
                        blockhandle_t  bs2)
{
    dbg->message("blockmanager::vereinige()",
                 "joining rail blocks %ld and %ld.",
                 bs1.get_id(), bs2.get_id());

    assert(bs1.is_bound());

    // prüfe, ob nur eine strecke
    if(bs1 == bs2) {
        return;
    }

    if(!bs2.is_bound()) {
        return;
    }

    for(int j=0; j<welt->gib_groesse(); j++) {
        for(int i=0; i<welt->gib_groesse(); i++) {
            const planquadrat_t *plan = welt->lookup(koord(i, j));

            for(unsigned int k = 0; k < plan->gib_boden_count(); k++) {
                weg_t *weg = plan->gib_boden_bei(k)->gib_weg(weg_t::schiene);
                if(weg) {
                    blockhandle_t bs = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();

                    assert(bs.is_bound());

                    if(bs == bs2) {
                        dynamic_cast<schiene_t *>(weg)->setze_blockstrecke(bs1);
                    }
                }
            }
        }
    }


    // vereinige signale

    bs2->verdrahte_signale_neu();

    strecken.remove( bs2 );

    // pruefe, ob strecke frei

    bs1->vereinige_vehikel_counter(bs2);

    dbg->message("blockmanager::vereinige()", "deleting rail block %ld", bs2.get_id());
    blockstrecke_t::destroy( bs2 );

    dbg->message("blockmanager::vereinige()", "%d rail blocks left after join", strecken.count());

}


/**
 * entfernt ein signal aus dem blockstreckennetz.
 */
bool
blockmanager::entferne_signal(karte_t *welt, koord3d pos)
{
    // partner des signals suchen

    grund_t * gr = welt->lookup(pos);
    weg_t *weg = gr->gib_weg(weg_t::schiene);
    blockhandle_t bs0 = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();


    signal_t *sig = dynamic_cast <signal_t *> (gr->suche_obj(ding_t::signal));


    int anzahl;
    array_tpl<koord3d> &nb = finde_nachbarn(welt,
					    pos,
					    sig->gib_richtung(),
					    // weg->gib_ribi_unmasked(),
					    anzahl);


    // Hajo: count signals nearby
    int count = 0;
    for(int i=0; i<anzahl; i++) {
        grund_t *gr = welt->lookup(nb.at(i));

	count += (gr->suche_obj(ding_t::signal) != 0) ? 1 : 0;
    }

    dbg->message("blockmanager::entferne_signal()",
		 "%d neighbours, %d signals found", anzahl, count);


    // Hajo: ambiguous signals ?
    if(count != 1) {
      dbg->warning("blockmanager::entferne_signal()",
		   "ambiguous combination of %d signals found, break.",
		   count);
      return false;
    }


    for(int i=0; i<anzahl; i++) {
        weg_t *nachbar_weg = welt->lookup(nb.at(i))->gib_weg(weg_t::schiene);
        blockhandle_t bs = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();

	bs->loesche_signal_bei(nb.at(i));

        if(bs != bs0) {
            block_ersetzer bes (welt, bs);
            bes.neu = bs0;
            marker.unmarkiere_alle();
            traversiere_netz(welt, nb.at(i), &bes);

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
    weg_t *weg = welt->lookup(pos)->gib_weg(weg_t::schiene);
    int i;

    if(weg) {
        blockhandle_t bs0 = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();

        const ribi_t::ribi ribi = weg->gib_ribi();
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
                weg_t *nachbar_weg = welt->lookup(nb.at(i))->gib_weg(weg_t::schiene);
                blockhandle_t bs = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();

                bs->loesche_signal_bei(nb.at(i));
            }
        }

//	printf("Anzahl %d\n", anzahl);

        if(anzahl == 0) {
            // das war das letze stueck einer schiene
            // die blockstrecke wird aufgelöst

//	    printf("%d Blockstrecken, entferne strecke %p\n", strecken.count(), bs0);
            strecken.remove( bs0 );
//	    printf("%d Blockstrecken\n", strecken.count());

            blockstrecke_t::destroy( bs0 );

            dynamic_cast<schiene_t *>(weg)->setze_blockstrecke(blockhandle_t());
        } else if(anzahl == 1) {
            // das war ende einer schiene,
            // nichts weiter zu tun

            dynamic_cast<schiene_t *>(weg)->setze_blockstrecke(blockhandle_t());
        } else {
            // es gibt jetzt offene enden
            // dafuer muessen neue Blockstrecken angelegt werden

            // max. 4 offene enden
            array_tpl< blockhandle_t > bs(4);
            bs.at(0) = bs0;

            block_ersetzer *bes = new block_ersetzer(welt, bs.at(0));
            dynamic_cast<schiene_t *>(weg)->setze_blockstrecke(blockhandle_t());

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
                    weg_t *nachbar_weg = welt->lookup(nb.at(j))->gib_weg(weg_t::schiene);

                    if(bs.at(i) == dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke()) {
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
                weg_t *nachbar_weg = welt->lookup(nb.at(i))->gib_weg(weg_t::schiene);
                blockhandle_t bs = dynamic_cast<schiene_t *>(nachbar_weg)->gib_blockstrecke();

                pruefer_ob_strecke_frei *pr = new pruefer_ob_strecke_frei(welt, bs);
                marker.unmarkiere_alle();
                traversiere_netz(welt, nb.at(i), pr);
                bs->setze_belegung( pr->count );
                delete pr;
            }
        }
//	reset_nachbar_ribis(welt, pos, ribi);
    }
    return weg != NULL;
}



const char *
blockmanager::baue_neues_signal(karte_t *welt,
                                spieler_t *sp,
                                koord3d pos, koord3d pos2, schiene_t *sch,
                                ribi_t::ribi dir)
{
    dbg->message("blockmanager::baue_neues_signal()", "build a new signal between %d,%d and %d,%d",
           pos.x, pos.y, pos2.x, pos2.y);

    blockhandle_t bs1 = sch->gib_blockstrecke();
    blockhandle_t bs2 = blockstrecke_t::create(welt);


    // zweite blockstrecke erzeugen

    marker.unmarkiere_alle();

    weg_t *weg2 = welt->lookup(pos2)->gib_weg(weg_t::schiene);
    weg2->ribi_rem(ribi_t::rueckwaerts(dir));

    block_ersetzer *bes = new block_ersetzer(welt, bs1);
    bes->neu = bs2;
    traversiere_netz(welt, pos2, bes);

    strecken.insert( bs2 );

    // signale neu verdrahten
    bs1->verdrahte_signale_neu();

    // war es eine ringstrecke ?
    if(sch->gib_blockstrecke() == bs2) {
        // dann wird bs1 nicht mehr verwendet
        strecken.remove(bs1);
        blockstrecke_t::destroy(bs1);

        bs1 = bs2;

        dbg->message("blockmanager::baue_neues_signal()", "circular rail block detected.");
    }

    weg2->ribi_add(ribi_t::rueckwaerts(dir));

    signal_t *sig1 = new signal_t(welt, pos, dir);
    signal_t *sig2 = new signal_t(welt, pos2, ribi_t::rueckwaerts(dir));

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
        sp->buche(CST_SIGNALE, pos.gib_2d(), COST_CONSTRUCTION);
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

    dbg->message("blockmanager::baue_andere_signale()",
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
blockmanager::neues_signal(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir)
{
    weg_t *weg = welt->lookup(pos)->gib_weg(weg_t::schiene);

    if(weg) {
        const ribi_t::ribi ribi = weg->gib_ribi();

        // pruefe ob dir gültig
        if(dir != ribi_t::nord && dir != ribi_t::west) {
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
        weg_t *weg2 = welt->lookup(pos2)->gib_weg(weg_t::schiene);

        if(!weg2) {
            // keine schiene
            dbg->warning("blockmanager::neues_signal()", "no railroad track on square!");
            return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
        }

        // pruefe ob schon signale da

        signal_t *sig1 = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke()->gib_signal_bei(pos);
        signal_t *sig2 = dynamic_cast<schiene_t *>(weg2)->gib_blockstrecke()->gib_signal_bei(pos2);

        if(sig1 != NULL && sig2 != NULL) {
            // es gibt signale

            return baue_andere_signale(pos, pos2, dynamic_cast<schiene_t *>(weg), dynamic_cast<schiene_t *>(weg2), dir);
        } else if(sig1 == NULL && sig2 == NULL) {
            // es gibt noch keine Signale

            if(!(dir & ribi)) {
                dbg->warning("blockmanger_t::neues_signal()", "direction mismatch: dir=%d, ribi=%d!", dir, ribi);
                return "Hier kann kein\nSignal aufge-\nstellt werden!\n";
            }
            return baue_neues_signal(welt, sp, pos, pos2, dynamic_cast<schiene_t *>(weg), dir);

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

void
blockmanager::pruefe_blockstrecke(karte_t *welt, koord3d k)
{
    const grund_t *gr = welt->lookup(k);

    if(gr) {
        weg_t *weg = gr->gib_weg(weg_t::schiene);

        if(weg) {
            blockhandle_t bs = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();

            pruefer_ob_strecke_frei pr (welt, bs);
            marker.unmarkiere_alle();
            traversiere_netz(welt, k, &pr);
            bs->setze_belegung( pr.count );

            // dbg->message("blockmanager::pruefe_blockstrecke()", "setting waggon counter to %d waggons", pr.count);
        }
    }
}


void blockmanager::setze_tracktyp(karte_t *welt, koord3d k, uint8 styp)
{
    const grund_t *gr = welt->lookup(k);

    if(gr) {
        weg_t *weg = gr->gib_weg(weg_t::schiene);

        if(weg) {
            blockhandle_t bs = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();

            tracktyp_ersetzer pr (welt, bs, styp);
            marker.unmarkiere_alle();
            traversiere_netz(welt, k, &pr);
        }
    }
}


void
blockmanager::traversiere_netz(const karte_t *welt,
			       const koord3d start,
                               koord_beobachter *cmd)
{
  // die Berechnung erfolgt durch eine Breitensuche fuer Graphen

  slist_tpl <koord3d> queue;         // Warteschlange fuer Breitensuche

  koord3d tmp (start);

  queue.append( tmp );        // init queue mit erstem feld

//    printf("Start ist %d %d\n", start.x, start.y);
//    printf("pos ist %d %d ziel ist %d %d\n", tmp->pos.x, tmp->pos.y, ziel.x, ziel.y);

  // Breitensuche

  do {
    tmp = queue.remove_first();

    marker.markiere(welt->lookup(tmp));  // betretene Felder markieren

    //printf("Checking koord %d %d\n", tmp->x, tmp->y);

    if(cmd->neue_koord(tmp) == false) {    // noch nicht fertig ?
    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
      grund_t *from = welt->lookup(tmp);
      grund_t *to;

      for(int r=0; r<4; r++) {
	if(from->get_neighbour(to, weg_t::schiene, koord::nsow[r])) {
	  if(cmd->ist_uebergang_ok(tmp, to->gib_pos())) {
	    if(!marker.ist_markiert(to)) {
	      //printf("found koord %d %d\n", nach.gib_pos().x, nach.gib_pos().y);
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
    file->rdwr_int(count, "\n");

    if(file->is_loading()) {
        for(int i=0; i<count; i++) {
            strecken.append(blockstrecke_t::create(welt, file));
        }
    }
    else {
        slist_iterator_tpl<blockhandle_t > s_iter ( strecken );
        while(s_iter.next()) {
            s_iter.get_current()->rdwr(file);
        }
    }
}

void
blockmanager::laden_abschliessen()
{
    slist_iterator_tpl< blockhandle_t > iter ( strecken );

    while(iter.next()) {
        blockhandle_t bs = iter.get_current();
        bs->laden_abschliessen();
    }
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

     dbg->message("blockmanager::block_ersetzer::block_ersetzer()",
                  "replacing rail block %ld", alt.get_id());
}

bool
blockmanager::block_ersetzer::neue_koord(koord3d pos)
{
    bool ok = true;   // assume we break traversal here

    schiene_t *weg = dynamic_cast<schiene_t *>(welt->lookup(pos)->gib_weg(weg_t::schiene));

    if(weg) {
        if(weg->gib_blockstrecke() == alt) {
//	    dbg->message("blockmanager::block_ersetzer::neue_koord()",
//                 "on %d %d: %d -> %d", pos.x, pos.y, alt.get_id(), neu.get_id());
            weg->setze_blockstrecke( neu );
            ok = false;   // do not stop traversal yet
	}
    }

//    dbg->message("blockmanager::block_ersetzer::neue_koord()",
//                 "called on %d %d, result %d", pos.x, pos.y, ok);


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



blockmanager::tracktyp_ersetzer::tracktyp_ersetzer(karte_t *welt,
						   blockhandle_t alt,
						   uint8 is_electric)
{
     this->welt = welt;
     this->alt = alt;
     this->electric = is_electric;

     dbg->message("blockmanager::tracktyp_ersetzer::tracktyp_ersetzer()",
                  "replacing rail block %ld to electric track type %d",
		  alt.get_id(), is_electric);
}

bool
blockmanager::tracktyp_ersetzer::neue_koord(koord3d pos)
{
    bool ok = true;   // assume we break traversal here

    grund_t *gr = welt->lookup(pos);

    schiene_t *weg = dynamic_cast<schiene_t *>(gr->gib_weg(weg_t::schiene));

    if(weg) {
        if(weg->gib_blockstrecke() == alt) {
//	    dbg->message("blockmanager::tracktyp_ersetzer::neue_koord()",
//                 "on %d %d: %d -> %d", pos.x, pos.y, alt.get_id(), neu.get_id());
            weg->setze_elektrisch( electric );
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
		  sp->buche(CST_OBERLEITUNG,
			    pos.gib_2d(),
			    COST_CONSTRUCTION);
		} else {
		  dbg->error("blockmanager::tracktyp_ersetzer::neue_koord",
			     "Owner of track is NULL");
		}
	      }

	      obl->calc_bild();
	    }


            ok = false;   // do not stop traversal yet
	}
    }

//    dbg->message("blockmanager::tracktyp_ersetzer::neue_koord()",
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
        schiene_t *weg = dynamic_cast<schiene_t *>(gr->gib_weg(weg_t::schiene));

        if(weg && weg->gib_blockstrecke() == bs) {

            for(int i=0; i<gr->gib_top(); i++) {
                const ding_t *dt = gr->obj_bei(i);

                if(dt != NULL) {
                    ding_t::typ typ = dt->gib_typ();

//		    printf("Typ %d, %s bei %d, %d\n", typ, dt->name(), k.x, k.y);

                    if(typ == ding_t::waggon ||
		       typ == ding_t::car) {
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
