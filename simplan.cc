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
#include "simconst.h"
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
	if(bd!=NULL  &&  !gib_boden_in_hoehe(bd->gib_hoehe())) {
		// boeden[0] ist Kartengrund,
		// danach folgen die Tunnels und Brücken höhensortiert.
		int i=1;
		while(i<boeden.get_count()) {
			if(bd->gib_hoehe()<boeden.get(i)->gib_hoehe()) {
				break;
			}
			i ++;
		}
		boeden.insert(i, bd);
		if(bd->gib_typ()==grund_t::boden) {
			bd->set_flag(grund_t::is_cover_tile);
		}
		bd->calc_bild();
		reliefkarte_t::gib_karte()->calc_map_pixel(bd->gib_pos().gib_2d());
	}
}



bool planquadrat_t::kartenboden_insert(grund_t *bd)
{
	if(bd!=NULL) {
		int already_there=0;
		// boeden[0] ist Kartengrund,
		// danach folgen die Tunnels und Brücken höhensortiert.
		for(int i=0; i<boeden.get_count(); i++) {
			if(boeden.get(i)->gib_typ()==grund_t::boden) {
				already_there ++;
				if(boeden.get(i)->get_flag(grund_t::is_cover_tile)) {
					return 0;
				}
			}
		}
		boeden.insert(0, bd);
		if(already_there) {
			bd->set_flag(grund_t::is_cover_tile);
			bd->set_kartenboden(false);
		}
		bd->calc_bild();
		reliefkarte_t::gib_karte()->calc_map_pixel(bd->gib_pos().gib_2d());
	}
	return 1;
}



bool planquadrat_t::boden_entfernen(grund_t *bd)
{
    if(boeden.remove(bd)) {
        reliefkarte_t::gib_karte()->calc_map_pixel(bd->gib_pos().gib_2d());
	return true;
    }
    return false;
}

void
planquadrat_t::kartenboden_setzen(grund_t *bd, bool mit_spieler)
{
	if(bd != NULL) {
		grund_t *tmp = gib_kartenboden();

		if(boeden.get_count()>0) {
			boeden.at(0) = bd;
		}
		else {
			boeden.append(bd);
		}
		if( tmp ) {
			// transfer old properties ...
			bd->setze_text(tmp->gib_text());
			if(mit_spieler) {
				bd->setze_besitzer(tmp->gib_besitzer());
			}
			// now delete everything
			delete tmp;
		}
		bd->set_kartenboden(true);
		bd->calc_bild();
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
			if(boeden.get(i)==alt) {
				grund_t *tmp = boeden.get(i);
				neu->set_kartenboden(i==0);
				boeden.at(i) = neu;
				if(tmp->get_flag(grund_t::is_cover_tile)) {
					neu->set_flag(grund_t::is_cover_tile);
				}
				delete tmp;
				return;
			}
		}
	}
}


bool
planquadrat_t::destroy(spieler_t *sp)
{
	while(boeden.get_count()>0) {
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
					dbg->fatal("planquadrat_t::rdwr()","Error while loading game: Unknown ground type '%d'",gtyp);
			}

//			assert(gr==NULL  ||  gtyp==gr->gib_typ());

			// check if we have a matching building here, otherwise set to nothing
			if(gr  &&  gtyp==grund_t::fundament  &&  gr->suche_obj(ding_t::gebaeude)==0) {
				koord3d pos = gr->gib_pos();
				// show normal ground here
				delete gr;
				gr = new boden_t(welt, pos, 0);
DBG_MESSAGE("planquadrat_t::rwdr", "unknown building (or prepare for factory) at %d,%d replaced by normal ground!", pos.x,pos.y);
			}
			// we should also check for ground below factories
			if(gr) {
				if(gib_kartenboden()==NULL) {
					if(boeden.get_count()>0) {
						boeden.at(0) = gr;
					}
					else {
						boeden.append(gr);
					}
					gr->set_kartenboden(true);
				} else {
					boden_hinzufuegen(gr);
				}
			}
		} while(gr != 0);
	}
}



void planquadrat_t::step(const long delta_t, const int steps)
{
	for(unsigned int i = 0; i < boeden.get_count(); i++) {
		grund_t *gr = boeden.get(i);
		if(gr->gib_top()) {
			gr->step(delta_t,steps);
		}
	}
}



// start a new month (an change seasons)
void planquadrat_t::check_season(const long month)
{
	for(unsigned int i = 0; i < boeden.get_count(); i++) {
		grund_t *gr = boeden.get(i);
		gr->check_season(month);
	}
}



void planquadrat_t::abgesenkt(karte_t *welt)
{
	grund_t *gr = gib_kartenboden();
	if(gr) {
		gr->obj_loesche_alle(NULL);
		koord k=gr->gib_pos().gib_2d();

		if(welt->max_hgt(k) <= welt->gib_grundwasser()) {
			kartenboden_setzen(new wasser_t(welt, k), true);
		}
		else {
			gr->setze_pos(koord3d(k,welt->min_hgt(k)));
			gr->setze_grund_hang( welt->calc_natural_slope(k) );
		}
	}
}

void planquadrat_t::angehoben(karte_t *welt)
{
	grund_t *gr = gib_kartenboden();
	if(gr) {
		koord k ( gr->gib_pos().gib_2d() );
		gr->obj_loesche_alle(NULL);
		gr->setze_pos(koord3d(k,welt->min_hgt(k)));
		if (welt->max_hgt(k) > welt->gib_grundwasser()) {
			kartenboden_setzen(new boden_t(welt, gr->gib_pos(), welt->calc_natural_slope(k) ), true);
		}
		else {
			gr->calc_bild();
		}
	}
}



void
planquadrat_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 /*raster*/, bool reset_dirty) const
{
	if(boeden.get_count() > 0) {
#ifdef COVER_TILES
	// cover tiles
		if(boeden.get(0)->get_flag(grund_t::is_cover_tile)) {
			for(uint8 i=boeden.get_count()-1; i>0;  i--) {
				grund_t *gr = boeden.get(i);
				if(gr->gib_typ()==grund_t::boden) {
					gr->display_boden(xpos, ypos, reset_dirty);
					gr->display_dinge(xpos, ypos, reset_dirty);
				}
			}
		}
#endif
		grund_t *gr = boeden.get(0);
		if(!gr->get_flag(grund_t::draw_as_ding)) {
			gr->display_boden(xpos, ypos, reset_dirty);
		}
	}
}


void
planquadrat_t::display_dinge(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, bool reset_dirty) const
{
	if(boeden.get_count()>0) {
#ifdef COVER_TILES
		boeden.get(0)->display_dinge(xpos, ypos, reset_dirty);
		for(uint8 i=1; i<boeden.get_count(); i++) {
			gr = boeden.get(i);
			if(gr->gib_typ()!=grund_t::boden)) {
				gr->display_boden(xpos, ypos, reset_dirty);
				gr->display_dinge(xpos, ypos, reset_dirty);
			}
		}
#else
		// first some action with the kartenboden (i.e. level ground)
		grund_t *gr = boeden.get(0);
		bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
		if(gr->get_flag(grund_t::draw_as_ding)) {
			gr->display_boden(xpos, ypos, reset_dirty );
		}
		gr->display_dinge(xpos, ypos, reset_dirty );

		// display station owner boxes
		if(umgebung_t::station_coverage_show  &&  halt_list.get_count()>0) {
			const sint16 r=raster_tile_width/8;
			const sint16 x=xpos+raster_tile_width/2-r;
			const sint16 y=ypos+(raster_tile_width*3)/4-r - (gr->gib_grund_hang()? tile_raster_scale_y(8,raster_tile_width): 0);
			// suitable start search
			for(sint16 h=halt_list.get_count()-1;  h>=0;  h--  ) {
				display_fillbox_wh_clip( x-h*2, y+h*2, r, r, PLAYER_FLAG|((halt_list.at(h)->gib_besitzer()->get_player_color())*4+4), kartenboden_dirty );
			}
		}

		// the rest
		const sint16 h0 = gr->gib_hoehe();
		for(uint8 i=1; i<boeden.get_count(); i++) {
			gr = gib_boden_bei(i);
			const sint16 yypos = ypos -tile_raster_scale_y( gr->gib_hoehe()-h0, raster_tile_width);
			gr->display_boden(xpos, yypos, reset_dirty );
			gr->display_dinge(xpos, yypos, reset_dirty );
		}
#endif
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
