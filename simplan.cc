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
#include "simgraph.h"
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



// deletes also all grounds in this array!
planquadrat_t::~planquadrat_t()
{
	if(ground_size==0) {
		// empty
	}
	else if(ground_size==1) {
		delete data.one;
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			delete data.some[i];
		}
		delete [] data.some;
	}
	if(halt_list_count) {
		delete [] halt_list;
	}
}



grund_t *
planquadrat_t::gib_boden_von_obj(ding_t *obj) const
{
	if(ground_size==1) {
		if(data.one  &&  data.one->obj_ist_da(obj)) {
			return data.one;
		}
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]->obj_ist_da(obj)) {
				return data.some[i];
			}
		}
	}
	return NULL;
}



void planquadrat_t::boden_hinzufuegen(grund_t *bd)
{
	assert(!bd->ist_karten_boden());
	if(ground_size==0) {
		// completely empty
		data.one = bd;
		ground_size = 1;
		return;
	}
	else if(ground_size==1) {
		// needs to convert to array
//	assert(data.one->gib_hoehe()!=bd->gib_hoehe());
		if(data.one->gib_hoehe()==bd->gib_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->gib_name(),bd->gib_pos().x,bd->gib_pos().y,bd->gib_pos().z);
			return;
		}
		grund_t **tmp = new grund_t *[2];
		tmp[0] = data.one;
		tmp[1] = bd;
		data.some = tmp;
		ground_size = 2;
		return;
	}
	else {
		// insert into array
		uint8 i;
		for(i=1;  i<ground_size;  i++) {
			if(data.some[i]->gib_hoehe()>=bd->gib_hoehe()) {
				break;
			}
		}
		if(i<ground_size  &&  data.some[i]->gib_hoehe()==bd->gib_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->gib_name(),bd->gib_pos().x,bd->gib_pos().y,bd->gib_pos().z);
			return;
		}
		bd->set_kartenboden(false);
		// extend array if needed
		grund_t **tmp = new grund_t *[ground_size+1];
		for( uint8 j=0;  j<i;  j++  ) {
			tmp[j] = data.some[j];
		}
		tmp[i] = bd;
		while(i<ground_size) {
			tmp[i+1] = data.some[i];
			i++;
		}
		ground_size ++;
		delete [] data.some;
		data.some = tmp;
		bd->calc_bild();
		reliefkarte_t::gib_karte()->calc_map_pixel(bd->gib_pos().gib_2d());
	}
}



bool planquadrat_t::boden_entfernen(grund_t *bd)
{
	assert(!bd->ist_karten_boden()  &&  ground_size>0);
	if(ground_size==1) {
		ground_size = 0;
		data.one = NULL;
		reliefkarte_t::gib_karte()->calc_map_pixel(bd->gib_pos().gib_2d());
		return true;
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==bd) {
				// found
				while(i<ground_size) {
					data.some[i] = data.some[i+1];
					i++;
				}
				ground_size --;
				// below 2?
				if(ground_size==1) {
					grund_t *tmp=data.some[0];
					delete [] data.some;
					data.one = tmp;
				}
				return true;
			}
		}
	}
	assert(0);
	return false;
}



void
planquadrat_t::kartenboden_setzen(grund_t *bd)
{
	assert(bd);
	grund_t *tmp = gib_kartenboden();

	if(ground_size<=1) {
		data.one = bd;
		ground_size = 1;
	}
	else {
		data.some[0] = bd;
	}
	if( tmp ) {
		// transfer old properties ...
		bd->setze_text(tmp->gib_text());
		// now delete everything
		delete tmp;
	}
	bd->set_kartenboden(true);
	bd->calc_bild();
}



/**
 * Ersetzt Boden alt durch neu, löscht Boden alt.
 * @author Hansjörg Malthaner
 */
void planquadrat_t::boden_ersetzen(grund_t *alt, grund_t *neu)
{
	assert(alt!=NULL  &&  neu!=NULL);
#ifdef COVER_TILES
	if(alt->get_flag(grund_t::is_cover_tile)) {
		neu->set_flag(grund_t::is_cover_tile);
	}
#endif

	if(ground_size==1) {
		assert(data.one==alt);
		data.one = neu;
		neu->set_kartenboden(true);
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==alt) {
				data.some[i] = neu;
				neu->set_kartenboden(i==0);
				break;
			}
		}
	}
	delete alt;
}



void
planquadrat_t::rdwr(karte_t *welt, loadsave_t *file)
{
	if(file->is_saving()) {
		if(ground_size==1) {
			data.one->rdwr(file);
		}
		else {
			for(int i=0; i<ground_size; i++) {
				data.some[i]->rdwr(file);
			}
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
				if(ground_size==0) {
					data.one = gr;
					ground_size = 1;
					gr->set_kartenboden(true);
				}
				else {
					boden_hinzufuegen(gr);
				}
			}
		} while(gr != 0);
	}
}



// start a new month (an change seasons)
void planquadrat_t::check_season(const long month)
{
	if(ground_size==0) {
	}
	else if(ground_size==1) {
		data.one->check_season(month);
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			data.some[i]->check_season(month);
		}
	}
}



void planquadrat_t::abgesenkt(karte_t *welt)
{
	grund_t *gr = gib_kartenboden();
	if(gr) {
		gr->obj_loesche_alle(NULL);
		koord k=gr->gib_pos().gib_2d();

		if(welt->max_hgt(k) <= welt->gib_grundwasser()) {
			kartenboden_setzen(new wasser_t(welt, k));
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
			kartenboden_setzen(new boden_t(welt, gr->gib_pos(), welt->calc_natural_slope(k) ) );
		}
		else {
			gr->calc_bild();
		}
	}
}



void
planquadrat_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 /*raster*/, bool reset_dirty) const
{
	grund_t *gr=gib_kartenboden();
	if(!gr->get_flag(grund_t::draw_as_ding)) {
		gr->display_boden(xpos, ypos, reset_dirty);
	}
}



void
planquadrat_t::display_dinge(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, bool reset_dirty) const
{
	grund_t *gr=gib_kartenboden();
	const bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
	if(gr->get_flag(grund_t::draw_as_ding)) {
		gr->display_boden(xpos, ypos, reset_dirty);
	}
	gr->display_dinge(xpos, ypos, reset_dirty);

	if(ground_size>1) {
		const sint16 h0 = gr->gib_hoehe();
		for(uint8 i=1;  i<ground_size;  i++) {
			gr=data.some[i];
			const sint16 yypos = ypos - tile_raster_scale_y( (gr->gib_hoehe()-h0)*TILE_HEIGHT_STEP/Z_TILE_STEP, raster_tile_width);
			gr->display_boden(xpos, yypos, reset_dirty );
			gr->display_dinge(xpos, yypos, reset_dirty );
		}
	}

	// display station owner boxes
	if(umgebung_t::station_coverage_show  &&  halt_list_count>0) {
		const sint16 r=raster_tile_width/8;
		const sint16 x=xpos+raster_tile_width/2-r;
		const sint16 y=ypos+(raster_tile_width*3)/4-r - (gr->gib_grund_hang()? tile_raster_scale_y(8,raster_tile_width): 0);
		// suitable start search
		for(sint16 h=halt_list_count-1;  h>=0;  h--  ) {
			display_fillbox_wh_clip(x - h * 2, y + h * 2, r, r, PLAYER_FLAG | (halt_list[h]->gib_besitzer()->get_player_color() * 4 + 4), kartenboden_dirty);
		}
	}
}



/**
 * Manche Böden können zu Haltestellen gehören.
 * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
 * @author Hj. Malthaner
 */
void planquadrat_t::setze_halt(halthandle_t halt)
{
#ifdef DEBUG
	if(halt.is_bound()  &&  this_halt.is_bound()  &&  halt!=this_halt) {
		dbg->warning("planquadrat_t::setze_halt()","cannot assign new halt: already bound!" );
	}
#endif
	this_halt = halt;
}



// these functions are private helper functions for halt_list
void planquadrat_t::halt_list_remove( halthandle_t halt )
{
	for( uint8 i=0;  i<halt_list_count;  i++ ) {
		if(halt_list[i]==halt) {
			for( uint8 j=i+1;  j<halt_list_count;  j++  ) {
				halt_list[j-1] = halt_list[j];
			}
			halt_list_count--;
			break;
		}
	}
}

void planquadrat_t::halt_list_insert_at( halthandle_t halt, uint8 pos )
{
	// extend list?
	if((halt_list_count%4)==0) {
		halthandle_t *tmp = new halthandle_t[halt_list_count+4];
		if(halt_list!=NULL) {
			// now insert
			for( uint8 i=0;  i<halt_list_count;  i++ ) {
				tmp[i] = halt_list[i];
			}
			delete [] halt_list;
		}
		halt_list = tmp;
	}
	// now insert
	for( uint8 i=halt_list_count;  i>pos;  i-- ) {
		halt_list[i] = halt_list[i-1];
	}
	halt_list[pos] = halt;
	halt_list_count ++;
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
		const koord pos = gib_kartenboden()->gib_pos().gib_2d();

		// exact position does matter only for passenger/mail transport
		if(sp!=NULL  &&  sp->has_passenger()  &&  halt_list_count>0  ) {
			halt_list_remove(halt);

			// since only the first one gets all the passengers, we want the closest one for passenger transport to be on top
			for(insert_pos=0;  insert_pos<halt_list_count;  insert_pos++) {

				// not a passenger KI or other is farer away
				if (!halt_list[insert_pos]->gib_besitzer()->has_passenger() ||
						abs_distance(halt_list[insert_pos]->get_next_pos(pos), pos) > abs_distance(halt->get_next_pos(pos), pos))
				{
					halt_list_insert_at( halt, insert_pos );
					return;
				}
			}
			// not found
		}
		else {
			// just look, if it is not there ...
			for(insert_pos=0;  insert_pos<halt_list_count;  insert_pos++) {
				if(halt_list[insert_pos]==halt) {
					// do not add twice
					return;
				}
			}
		}

		// first or no passenger or append to the end ...
		halt_list_insert_at( halt, halt_list_count );
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
	const koord pos = gib_kartenboden()->gib_pos().gib_2d();

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
	halt_list_remove(halt);
}



/**
 * true, if this halt is reachable from here
 * @author prissi
 */
bool planquadrat_t::is_connected(halthandle_t halt) const
{
	for( uint8 i=0;  i<halt_list_count;  i++  ) {
		if(halt_list[i]==halt) {
			return true;
		}
	}
	return false;
}
