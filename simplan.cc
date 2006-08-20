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
#include "simhalt.h"
#include "simplay.h"
#include "simdebug.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/fundament.h"
#include "boden/wasser.h"
#include "boden/tunnelboden.h"
#include "boden/brueckenboden.h"
#include "boden/monorailboden.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "gui/karte.h"

// debug!
// #include "simtime.h"



grund_t *
planquadrat_t::gib_boden_von_obj(ding_t *obj) const
{
    if(this) {
        for(int i = 0; i < boeden.get_count(); i++) {
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
    int i = boeden.get_count();
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
	for(i = 1; i < boeden.get_count(); i++) {
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

		if(boeden.get_count() > 0) {
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
	for(int i=0; i<boeden.get_count(); i++) {
	    if(boeden.get(i) == alt) {
		grund_t *tmp = boeden.get(i);
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
    while(boeden.get_count() > 0) {
        grund_t *tmp = boeden.get(boeden.get_count() - 1);

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
        for(int i = 0; i < boeden.get_count(); i++) {
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
            case grund_t::monorailboden:	    gr = new monorailboden_t(welt, file);	    break;
            default:
	        gr = 0; // Hajo: keep compiler happy, fatal() never returns
                dbg->fatal("planquadrat_t::rdwr()",
			   "Error while loading game: "
			   "Unknown ground type '%d'",
			   gtyp);
            }

		// check if we have a matching building here, otherwise set to nothing
		if(gr  &&  gtyp==grund_t::fundament  &&  gr->suche_obj(ding_t::gebaeude)==0) {
			koord3d pos = gr->gib_pos();
			// show normal ground here
			delete gr;
			DBG_MESSAGE("gebaeude_t::rwdr", "try replace by normal ground!");
			gr = new boden_t(welt, pos);
			DBG_MESSAGE("gebaeude_t::rwdr", "unknown building at %d,%d replace by normal ground!", pos.x,pos.y);
		}
		// we should also check for ground below factories

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

    for(unsigned int i = 0; i < boeden.get_count(); i++) {
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
    if(boeden.get_count() > 0) {
        boeden.get(0)->display_boden(xpos, ypos, dirty);
    }
}


void
planquadrat_t::display_dinge(const int xpos, const int ypos, bool dirty) const
{
    if(boeden.get_count() > 0) {
        boeden.get(0)->display_dinge(xpos, ypos, dirty);
    }

    for(int i = 1; i < boeden.get_count(); i++) {
        boeden.get(i)->display_boden(xpos, ypos, dirty);
        boeden.get(i)->display_dinge(xpos, ypos, dirty);
    }
}



/**
 * Manche Böden können zu Haltestellen gehören.
 * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
 * @author Hj. Malthaner
 */
void planquadrat_t::setze_halt(halthandle_t halt) {
#ifdef DEBUG
	if(halt.is_bound()  &&  this->halt.is_bound()  &&  halt!=this->halt) {
		dbg->warning("planquadrat_t::setze_halt()","cannot assign new halt: already bound!" );
	}
#endif
	this->halt = halt;
}




/* The following functions takes at least 8 bytes of memory per tile but speed up passenger generation *
 * @author prissi
 */
void planquadrat_t::add_to_haltlist(halthandle_t halt)
{
	if(halt.is_bound()) {
		spieler_t *sp=halt->gib_besitzer();

		unsigned insert_pos = 0;
		// quick and dirty way to our 2d koodinates ...
		const koord pos = boeden.at(0)->gib_pos().gib_2d();

		// exact position does matter only for passenger/mail transport
		if(sp!=NULL  &&  sp->has_passenger()  &&  halt_list.get_count()>0  ) {
			halt_list.remove(halt);

			// since only the first one gets all the passengers, we want the closest one for passenger transport to be on top
			for(insert_pos=0;  insert_pos<halt_list.get_count();  insert_pos++) {

				// not a passenger KI or other is farer away
				if(  !halt_list.get(insert_pos)->gib_besitzer()->has_passenger()
				      ||  abs_distance( halt_list.get(insert_pos)->get_next_pos(pos), pos ) > abs_distance( halt->get_next_pos(pos), pos )  )
				{

					halt_list.insert_at( insert_pos, halt );
					return;
				}
			}
			// not found
		}

		// first or no passenger or append to the end ...
		halt_list.append_unique( halt, 2 );
	}
}

/**
 * removes the halt from a ground
 * however this funtion check, whether there is really no other part still reachable
 * @author prissi
 */
void planquadrat_t::remove_from_haltlist(karte_t *welt, halthandle_t halt)
{
	// quick and dirty way to our 2d koodinates ...
	const koord pos = boeden.at(0)->gib_pos().gib_2d();

	for(int y=-umgebung_t::station_coverage_size; y<=umgebung_t::station_coverage_size; y++) {

		for(int x=-umgebung_t::station_coverage_size; x<=umgebung_t::station_coverage_size; x++) {

			koord test_pos = pos+koord(x,y);
			const planquadrat_t *pl = welt->lookup(test_pos);

			if(pl   &&  pl->gib_halt()==halt  ) {
				// still conncected  => do nothing
				return;
			}
		}
	}
	// if we reached here, we are not connected to this halt anymore
	halt_list.remove(halt);
}



// ende planqudrat_t methoden
