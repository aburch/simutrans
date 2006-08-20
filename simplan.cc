/*
 * simplan.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "simdebug.h"
#include "simdings.h"
#include "simplan.h"
#include "simworld.h"
#include "simdebug.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/fundament.h"
#include "boden/wasser.h"
#include "boden/tunnelboden.h"
#include "boden/brueckenboden.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"



#include "gui/karte.h"

// debug!
// #include "simtime.h"



grund_t *
planquadrat_t::gib_boden_von_obj(ding_t *obj) const
{
    if(this) {
        for(int i = 0; i < boeden.count(); i++) {
            if(boeden.get(i)->obj_ist_da(obj)) {
                return boeden.get(i);
            }
        }
    }
    return NULL;
}


grund_t * planquadrat_t::gib_obersten_boden(spieler_t *sp) const
{
    grund_t *bd = gib_kartenboden();
    int i = boeden.count();
    grund_t *oben;

    if(bd->gib_besitzer() != sp) {
	bd = NULL;
    }
    while(--i) {
	oben = boeden.get(i);
	if(oben->gib_besitzer() == sp) {
	    return !bd || oben->gib_hoehe() > bd->gib_hoehe() ? oben : bd;
	}
    }
    return bd;
}


void planquadrat_t::boden_hinzufuegen(grund_t *bd)
{
    int i;
    if(bd != NULL && !gib_boden_in_hoehe(bd->gib_hoehe())) {
	// boeden[0] ist Kartengrund,
	// danach folgen die Tunnels und Brücken höhensortiert.
	for(i = 1; i < boeden.count(); i++) {
	    if(bd->gib_hoehe() < boeden.get(i)->gib_hoehe())
		break;
	}
        boeden.insert(i, bd);

        bd->calc_bild();
        reliefkarte_t::gib_karte()->recalc_relief_farbe(bd->gib_pos().gib_2d());
    }
}

bool planquadrat_t::boden_entfernen(grund_t *bd)
{
    if(boeden.remove(bd)) {
        reliefkarte_t::gib_karte()->recalc_relief_farbe(bd->gib_pos().gib_2d());
	return true;
    }
    return false;
}

void
planquadrat_t::kartenboden_setzen(grund_t *bd, bool mit_spieler)
{
	if(bd != NULL) {
		grund_t *tmp = gib_kartenboden();

		if(boeden.count() > 0) {
			boeden.at(0) = bd;
		}
		else {
			boeden.append(bd);
		}

		bd->calc_bild();

		if( tmp ) {
			// transfer old properties ...
			bd->setze_text(tmp->gib_text());
			if(mit_spieler) {
				bd->setze_besitzer(tmp->gib_besitzer());
			}
			// prissi: restore halt list
			vector_tpl<halthandle_t> &haltlist=tmp->get_haltlist();
			for(int i=0;i<haltlist.get_count();i++) {
				bd->add_to_haltlist( haltlist.get(i) );
			}
			// now delete everything
			delete tmp;
		}

		reliefkarte_t::gib_karte()->recalc_relief_farbe(bd->gib_pos().gib_2d());
	}
}


/**
 * Ersetzt Boden alt durch neu, löscht Boden alt.
 * @author Hansjörg Malthaner
 */
void planquadrat_t::boden_ersetzen(grund_t *alt, grund_t *neu)
{
    if(alt != NULL && neu != NULL) {
	for(int i=0; i<boeden.count(); i++) {
	    if(boeden.get(i) == alt) {
		grund_t *tmp = boeden.get(i);
		if(i==0) {
			// prissi: restore halt list
			vector_tpl<halthandle_t> &haltlist=tmp->get_haltlist();
			for(int i=0;i<haltlist.get_count();i++) {
				neu->add_to_haltlist( haltlist.get(i) );
			}
		}
		boeden.at(i) = neu;
		delete tmp;
		return;
	    }
	}
    }
}


bool
planquadrat_t::destroy(spieler_t *sp)
{
    while(boeden.count() > 0) {
        grund_t *tmp = boeden.get(boeden.count() - 1);

	assert(tmp);

        tmp->obj_loesche_alle(sp);

        boeden.remove(tmp);
        delete tmp;
    }
    return true;
}

void
planquadrat_t::rdwr(karte_t *welt, loadsave_t *file)
{
    if(file->is_saving()) {
        for(int i = 0; i < boeden.count(); i++) {
            boeden.get(i)->rdwr(file);
        }
        file->wr_obj_id(-1);
    }
    else {
        grund_t *gr;
//DBG_DEBUG("planquadrat_t::rdwr()","Reading boden");
        do {
            grund_t::typ gtyp = (grund_t::typ)file->rd_obj_id();

            switch(gtyp) {
            case -1:			    gr = NULL;				    break;
            case grund_t::boden:	    gr = new boden_t(welt, file);	    break;
            case grund_t::wasser:	    gr = new wasser_t(welt, file);	    break;
            case grund_t::fundament:	    gr = new fundament_t(welt, file);	    break;
            case grund_t::tunnelboden:	    gr = new tunnelboden_t(welt, file);	    break;
            case grund_t::brueckenboden:    gr = new brueckenboden_t(welt, file);   break;
            default:
	        gr = 0; // Hajo: keep compiler happy, fatal() never returns
                dbg->fatal("planquadrat_t::rdwr()",
			   "Error while loading game: "
			   "Unknown ground type '%d'",
			   gtyp);
            }
            if(gr) {
                koord3d tmppos ( gr->gib_pos() );
                if(gib_kartenboden() == NULL) {
                    kartenboden_setzen(gr, false);
                } else {
                    boden_hinzufuegen(gr);
                }
                gr->setze_pos(tmppos);   // setze_boden macht pos kaputt!
            }
        } while(gr != 0);
    }
}


void planquadrat_t::step(const long delta_t, const int steps)
{
    static slist_tpl<ding_t *>loeschen;

    for(unsigned int i = 0; i < boeden.count(); i++) {
        grund_t *gr = boeden.get(i);

		// Hajo: play or don't play ground animation
		// see boden/grund.h for more details
		if(umgebung_t::bodenanimation) {
			gr->step();
		}

		for(int j=0; j<gr->gib_top(); j++) {
			ding_t * ding = gr->obj_bei(j);

			if(ding &&
			    ding->step_frequency>0 &&
			    (steps%ding->step_frequency) == 0) {

				//long T0 = get_current_time_millis();

				if(ding->step(delta_t) == false) {
					loeschen.insert( ding );
				}

				/*
				long T1 = get_current_time_millis();

				if(T1-T0 > 5) {
					koord3d pos = ding->gib_pos();
					printf("ding %s at %d,%d-> %ld ms\n", ding->gib_name(), pos.x, pos.y, T1-T0);
				}
				*/
			}
		}
	}

	while(loeschen.count()) {
		// destruktor entfernt objekt aus der Verwaltung
		delete loeschen.remove_first();
	}
}

void planquadrat_t::abgesenkt(karte_t *welt)
{
    grund_t *gr = gib_kartenboden();

    if(gr) {
        gr->obj_loesche_alle(NULL);
	   gr->sync_height();
        gr->calc_bild();

        koord pos ( gr->gib_pos().gib_2d() );

        if(welt->max_hgt(pos) <= welt->gib_grundwasser()) {
            kartenboden_setzen(new wasser_t(welt, pos), true);
        }
    }
}

void planquadrat_t::angehoben(karte_t *welt)
{
    grund_t *gr = gib_kartenboden();

    if(gr) {
	gr->sync_height();
        gr->calc_bild();

        koord3d pos ( gr->gib_pos() );

        if (welt->max_hgt(pos.gib_2d()) > welt->gib_grundwasser()) {
            gr->obj_loesche_alle(NULL);

            kartenboden_setzen(new boden_t(welt, pos), true);
        }
    }
}



void
planquadrat_t::display_boden(const int xpos, const int ypos, bool dirty) const
{
    if(boeden.count() > 0) {
        boeden.get(0)->display_boden(xpos, ypos, dirty);
    }
}


void
planquadrat_t::display_dinge(const int xpos, const int ypos, bool dirty) const
{
    if(boeden.count() > 0) {
        boeden.get(0)->display_dinge(xpos, ypos, dirty);
    }

    for(int i = 1; i < boeden.count(); i++) {
        boeden.get(i)->display_boden(xpos, ypos, dirty);
        boeden.get(i)->display_dinge(xpos, ypos, dirty);
    }
}



// ende planqudrat_t methoden
