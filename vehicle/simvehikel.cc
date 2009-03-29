/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * All moving stuff (vehikel_basis_t) and all player vehicle (derived from vehikel_t)
 *
 * 01.11.99  getrennt von simdings.cc
 *
 * Hansjoerg Malthaner, Nov. 1999
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../boden/grund.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/strasse.h"

#include "../bauer/warenbauer.h"

#include "../simworld.h"
#include "../simdebug.h"

#include "../player/simplay.h"
#include "../simfab.h"
#include "../simware.h"
#include "../simhalt.h"
#include "../simsound.h"

#include "../simimg.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simio.h"
#include "../simmem.h"

#include "../simline.h"

#include "../simintr.h"

#include "../dings/wolke.h"
#include "../dings/signal.h"
#include "../dings/roadsign.h"
#include "../dings/crossing.h"
#include "../dings/zeiger.h"

#include "../gui/karte.h"

#include "../besch/stadtauto_besch.h"
#include "../besch/ware_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/roadsign_besch.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"


#include "../bauer/vehikelbauer.h"

#include "simvehikel.h"
#include "simverkehr.h"

/* get dx and dy from dir (just to remind you)
 * any vehikel (including city cars and pedestrians)
 * will go this distance per sync step.
 * (however, the real dirs are only calculated during display, these are the old ones)
 */
sint8 vehikel_basis_t::dxdy[ 8*2 ] = {
	-2, 1,	// s
	-2, -1,	// w
	-4, 0,	// sw
	0, 2,	//se
	2, -1, // n
	2, 1,	// e
	4, 0,	// ne
	0, -2	// nw
};



uint8 vehikel_basis_t::old_diagonal_length = 127;
uint8 vehikel_basis_t::diagonal_length = 180;
uint16 vehikel_basis_t::diagonal_multiplier = 724;

// set only once, before loading!
void vehikel_basis_t::set_diagonal_multiplier( uint32 multiplier, uint32 old_diagonal_multiplier )
{
	diagonal_multiplier = (uint16)multiplier;
	diagonal_length = (uint8)(130560u/diagonal_multiplier);
	old_diagonal_length = (uint8)(130560u/old_diagonal_multiplier);
}


// if true, convoi, must restart!
bool vehikel_basis_t::need_realignment()
{
	return old_diagonal_length!=diagonal_length  &&  (fahrtrichtung&0x0A)!=0;
}

// [0]=xoff [1]=yoff
sint8 vehikel_basis_t::overtaking_base_offsets[8][2];

// recalc offsets for overtaking
void vehikel_basis_t::set_overtaking_offsets( bool driving_on_the_left )
{
	sint8 sign = driving_on_the_left ? -1 : 1;
	// a tile has the internal size of
	const sint8 XOFF=12;
	const sint8 YOFF=6;

	overtaking_base_offsets[0][0] = sign * XOFF;
	overtaking_base_offsets[1][0] = -sign * XOFF;
	overtaking_base_offsets[2][0] = 0;
	overtaking_base_offsets[3][0] = sign * XOFF;
	overtaking_base_offsets[4][0] = -sign * XOFF;
	overtaking_base_offsets[5][0] = sign * XOFF;
	overtaking_base_offsets[6][0] = 0;
	overtaking_base_offsets[7][0] = sign * (-XOFF-YOFF);

	overtaking_base_offsets[0][1] = sign * YOFF;
	overtaking_base_offsets[1][1] = sign * YOFF;
	overtaking_base_offsets[2][1] = sign * YOFF;
	overtaking_base_offsets[3][1] = 0;
	overtaking_base_offsets[4][1] = -sign * YOFF;
	overtaking_base_offsets[5][1] = -sign * YOFF;
	overtaking_base_offsets[6][1] = -sign * YOFF;
	overtaking_base_offsets[7][1] = 0;
}



/**
 * Checks if this vehicle must change the square upon next move
 * @author Hj. Malthaner
 */
bool vehikel_basis_t::is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const
{
	const sint8 y_off_2 = 2*neu_yoff;
	const sint8 c_plus  = y_off_2 + neu_xoff;
	const sint8 c_minus = y_off_2 - neu_xoff;

	return ! (c_plus < TILE_STEPS*2  &&  c_minus < TILE_STEPS*2  &&  c_plus > -TILE_STEPS*2  &&  c_minus > -TILE_STEPS*2);
}



vehikel_basis_t::vehikel_basis_t(karte_t *welt):
	ding_t(welt)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
	steps = 0;
	steps_next = 255;
	use_calc_height = true;
}



vehikel_basis_t::vehikel_basis_t(karte_t *welt, koord3d pos):
	ding_t(welt, pos)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
	pos_next = pos;
	steps = 0;
	steps_next = 255;
	use_calc_height = true;
}



void vehikel_basis_t::rotate90()
{
	koord3d pos_cur = get_pos();
	pos_cur.rotate90( welt->get_groesse_y()-1 );
	set_pos( pos_cur );
	// directions are counterclockwise to ribis!
	fahrtrichtung = ribi_t::rotate90( fahrtrichtung );
	pos_next.rotate90( welt->get_groesse_y()-1 );
	// new offsets: very tricky ...
	sint8 new_dx = -dy*2;
	dy = dx/2;
	dx = new_dx;
	// new pos + step offsets (only possible, since we know the height!
	sint8 neu_yoff = get_xoff()/2;
	set_xoff( -get_yoff()*2 );
	set_yoff( neu_yoff );
}



void
vehikel_basis_t::verlasse_feld() //"leave field" (Google)
{
	// first: release crossing
	grund_t *gr = welt->lookup(get_pos());
	if(gr  &&  gr->ist_uebergang()) {
		crossing_t *cr = gr->find<crossing_t>(2);
		grund_t *gr2 = welt->lookup(pos_next);
		if(gr2==NULL  ||  gr2==gr  ||  !gr2->ist_uebergang()  ||  cr->get_logic()!=gr2->find<crossing_t>(2)->get_logic()) {
			cr->release_crossing(this);
		}
	}

	// then remove from ground (or search whole map, if failed)
	if(gr==NULL  ||  !gr->obj_remove(this)) {

		// was not removed (not found?)
		dbg->error("vehikel_basis_t::verlasse_feld()","'typ %i' %p could not be removed from %d %d", get_typ(), this, get_pos().x, get_pos().y);
		DBG_MESSAGE("vehikel_basis_t::verlasse_feld()","checking all plan squares");

		// check, whether it is on another height ...
		if(welt->ist_in_kartengrenzen( get_pos().get_2d() )) {
			gr = welt->lookup( get_pos().get_2d() )->get_boden_von_obj(this);
			if(gr) {
				gr->obj_remove(this);
				dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",get_typ(), this, get_pos().x, get_pos().y);
			}
			return;
		}

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->get_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->get_groesse_x(); k.x++) {
				grund_t *gr = welt->lookup( k )->get_boden_von_obj(this);
				if(gr && gr->obj_remove(this)) {
					dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",get_name(), this, k.x, k.y);
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehikel_basis_t::verlasse_feld()","'%s' %p was not found on any map sqaure!",get_name(), this);
		}
	}
}


void vehikel_basis_t::betrete_feld()
{
	grund_t *gr=welt->lookup(get_pos());
	if(!gr) {
		dbg->error("vehikel_basis_t::betrete_feld()","'%s' new position (%i,%i,%i)!",get_name(), get_pos().x, get_pos().y, get_pos().z );
		gr = welt->lookup_kartenboden(get_pos().get_2d());
		set_pos( gr->get_pos() );
	}
	gr->obj_add(this);
}



/* THE routine for moving vehicles
 * it will drive on as log as it can
 * @return the distance actually travelled
 */
uint32 vehikel_basis_t::fahre_basis(uint32 distance)
{
	koord3d pos_prev;

	uint32 steps_to_do = distance>>12;
	if(steps_to_do==0) {
		// ok, we will not move in this steps
		return 0;
	}
	// ok, so moving ...
	if(!get_flag(ding_t::dirty)) {
		mark_image_dirty(get_bild(),hoff);
		set_flag(ding_t::dirty);
	}
	steps_to_do += steps;

	if(steps_to_do>steps_next) {

		sint32 steps_done = - steps;
		bool has_hopped = false;

		// first we hop steps ...
		while(steps_to_do>steps_next  &&  hop_check()) {
			steps_to_do -= steps_next+1;
			steps_done += steps_next+1;
			pos_prev = get_pos();
			hop();
			use_calc_height = true;
			has_hopped = true;
		}

		if(steps_next==0) {
			// only needed for aircrafts, which can turn on the same tile
			// the indicate the turn with this here
			steps_next = 255;
			steps_to_do = 255;
			steps_done -= 255;
		}

		if(steps_to_do>steps_next) {
			// could not go as far as we wanted => stop at end of tile
			steps_to_do = steps_next;
		}
		steps = steps_to_do;

		steps_done += steps;
		distance = steps_done<<12;

		if(has_hopped) {
			set_xoff( (dx<0) ? TILE_STEPS : -TILE_STEPS );
			set_yoff( (dy<0) ? TILE_STEPS/2 : -TILE_STEPS/2 );
			if(dx*dy==0) {
				if(dx==0) {
					if(dy>0) {
						set_xoff( pos_prev.x!=get_pos().x ? -TILE_STEPS : TILE_STEPS );
					}
					else {
						set_xoff( pos_prev.x!=get_pos().x ? TILE_STEPS : -TILE_STEPS );
					}
				}
				else {
					if(dx>0) {
						set_yoff( pos_prev.y!=get_pos().y ? TILE_STEPS/2 : -TILE_STEPS/2 );
					}
					else {
						set_yoff( pos_prev.y!=get_pos().y ? -TILE_STEPS/2 : TILE_STEPS/2 );
					}
				}
			}
		}
	}
	else {
		distance &= 0xFFFFF000;
		steps = steps_to_do;
	}

	if(use_calc_height) {
		hoff = calc_height();
	}
	// remaining steps
	set_flag(ding_t::dirty);
	return distance;
}



// to make smaller steps than the tile granularity, we have to use this trick
void vehikel_basis_t::get_screen_offset( int &xoff, int &yoff ) const
{
	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)steps*(uint16)get_tile_raster_width();
	if(dx*dy) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}
	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (hoff*(sint16)get_tile_raster_width())/(4*16);
}



// calcs new direction and applies it to the vehicles
ribi_t::ribi
vehikel_basis_t::calc_set_richtung(koord start, koord ende)
{
	ribi_t::ribi richtung = ribi_t::keine; //"richtung" = direction (Google); "keine" = none (Google)

	const sint8 di = ende.x - start.x;
	const sint8 dj = ende.y - start.y;

	if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
		dx = 2;
		dy = -1;
		steps_next = 255;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
		dx = -2;
		dy = 1;
		steps_next = 255;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
		dx = -2;
		dy = -1;
		steps_next = 255;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
		dx = 2;
		dy = 1;
		steps_next = 255;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
		dx = 0;
		dy = 2;
		steps_next = diagonal_length;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
		dx = 0;
		dy = -2;
		steps_next = diagonal_length;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
		dx = 4;
		dy = 0;
		steps_next = diagonal_length;
	} else {
		richtung = ribi_t::suedwest;
		dx = -4;
		dy = 0;
		steps_next = diagonal_length;
	}
	// we could artificially make diagonals shorter: but this would break existing game behaviour
	return richtung;
}

ribi_t::ribi
vehikel_basis_t::calc_check_richtung(koord start, koord ende)
//Dry run version of calc_set_richtung.
{
	ribi_t::ribi richtung = ribi_t::keine; //"richtung" = direction (Google); "keine" = none (Google)

	const sint8 di = ende.x - start.x;
	const sint8 dj = ende.y - start.y;

	if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
	} else {
		richtung = ribi_t::suedwest;
	}
	return richtung;
}


ribi_t::ribi
vehikel_basis_t::calc_richtung(koord start, koord ende) const
{/*
	static ribi_t::ribi didj_richtung[9] =
	{
		ribi_t::nordwest, ribi_t::nord, ribi_t::nordost,	// dy<0
		ribi_t::west, ribi_t::keine, ribi_t::ost,	// dy==0
		ribi_t::suedwest, ribi_t::sued, ribi_t::suedost	// dy>0
	};

	uint8 di = 0x80 & (ende.x - start.x);
	uint8 dj = 0x80 & (ende.y - start.y);
	di ++ ;	// 0=(di<0), 1=(di==0), 2=(di>0)
	dj ++ ;
	return richtung[di+(3*dj)];
*/
	ribi_t::ribi richtung;
	const sint8 di = (ende.x - start.x);
	const sint8 dj = (ende.y - start.y);
	if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
	} else {
		richtung = ribi_t::suedwest;
	}
	return richtung;

}



// this routine calculates the new height
// beware of bridges, tunnels, slopes, ...
sint16 vehikel_basis_t::calc_height()
{
	sint16 hoff = 0;
	use_calc_height = false;	// assume, we are only needed after next hop

	grund_t *gr = welt->lookup(get_pos());
	if(gr==NULL) {
		// slope changed below a moving thing?!?
		return 0;
	}
	else if(gr->ist_tunnel()) {
		hoff = 0;
		if(!gr->ist_im_tunnel()) {
			use_calc_height = true;
			// need hiding? One of the few uses of XOR: not half driven XOR exiting => not hide!
			ribi_t::ribi hang_ribi = ribi_typ( gr->get_grund_hang() );
			if((steps<(steps_next/2))  ^  ((hang_ribi&fahrtrichtung)!=0)  ) {
				set_bild(IMG_LEER);
			}
			else {
				calc_bild();
			}
		}
	}
	else {
		// will not work great with ways, but is very short!
		hang_t::typ hang = gr->get_weg_hang();
		if(hang) {
			ribi_t::ribi hang_ribi = ribi_typ(hang);
			if(  ribi_t::doppelt(hang_ribi)  ==  ribi_t::doppelt(fahrtrichtung)) {
				sint16 h_end = hang_ribi & ribi_t::rueckwaerts(fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				sint16 h_start = (hang_ribi & fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				hoff = (h_start*(sint16)(uint16)steps + h_end*(256-steps)) >> 8;
			}
			else {
				// only for shadows and movingobjs ...
				sint16 h_end = hang_ribi & ribi_t::rueckwaerts(fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				sint16 h_start = (hang_ribi & fahrtrichtung) ? -TILE_HEIGHT_STEP : 0;
				hoff = ((h_start*(sint16)(uint16)steps + h_end*(256-steps)) >> 9) - TILE_HEIGHT_STEP/2;
			}
			use_calc_height = true;	// we need to recalc again next time
		}
		else {
			hoff = -gr->get_weg_yoff();
		}
	}
	return hoff;
}



/* true, if one could pass through this field
 * also used for citycars, thus defined here
 */
vehikel_basis_t *vehikel_basis_t::no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_fahrtrichtung, const uint8 next_fahrtrichtung, const uint8 next_90fahrtrichtung )
{
	// suche vehikel
	const uint8 top = gr->get_top();
	for(  uint8 pos=1;  pos<top;  pos++ ) {
		vehikel_basis_t *v = (vehikel_basis_t *)gr->obj_bei(pos);
		if(v->is_moving()) {
			uint8 other_fahrtrichtung=255;

			// check for car
			if(v->get_typ()==ding_t::automobil) {
				automobil_t *at = (automobil_t *)v;
				// ignore ourself
				if(cnv==at->get_convoi()) {
					continue;
				}
				other_fahrtrichtung = at->get_fahrtrichtung();
			}
			// check for city car
			else if(v->get_waytype()==road_wt  &&  v->get_typ()!=ding_t::fussgaenger) {
				other_fahrtrichtung = v->get_fahrtrichtung();
			}

			// ok, there is another car ...
			if(other_fahrtrichtung!=255) {

				if(other_fahrtrichtung==next_fahrtrichtung  ||  other_fahrtrichtung==next_90fahrtrichtung  ||  other_fahrtrichtung==current_fahrtrichtung ) {
					// this goes into the same direction as we, so stopp and save a restart speed
					return v;

				} else if(ribi_t::ist_orthogonal(other_fahrtrichtung,current_fahrtrichtung)) {

					// there is a car orthogonal to us
					return v;
				}
			}
		}
	}

	// way is free
	return NULL;
}


sint16
vehikel_t::compare_directions(sint16 first_direction, sint16 second_direction)
{
	//Returns difference between two directions in degrees
	sint16 difference = 0;
	if(first_direction > 360 || second_direction > 360 || first_direction < 0 || second_direction < 0) return 0; 
	//If directions > 360, this means that they are not supposed to be checked.
	if(first_direction > 180 && second_direction < 180)
	{
		sint16 tmp = first_direction - second_direction;
		if(tmp > 180) difference = 360 - tmp;
		else difference = tmp;
	}
	else if(first_direction < 180 && second_direction > 180)
	{
		sint16 tmp = second_direction - first_direction;
		if(tmp > 180) difference = 360 - tmp;
		else difference = tmp;
	}
	else if(first_direction > second_direction)
	{
		difference = first_direction - second_direction;
	}
	else
	{
		difference = second_direction - first_direction;
	}
	return difference;
}

void
vehikel_t::rotate90()
{
	vehikel_basis_t::rotate90();
	alte_fahrtrichtung = ribi_t::rotate90( alte_fahrtrichtung );
	pos_prev.rotate90( welt->get_groesse_y()-1 );
	last_stop_pos.rotate90( welt->get_groesse_y()-1 );
	// now rotate the freight
	slist_iterator_tpl<ware_t> iter (fracht);
	while(iter.next()) {
		ware_t& tmp = iter.access_current();
		koord k = tmp.get_zielpos();
		k.rotate90( welt->get_groesse_y()-1 );
		tmp.set_zielpos( k );
	}
}



void vehikel_t::set_convoi(convoi_t *c)
{
	/* cnv can have three values:
	 * NULL: not previously assigned
	 * 1 (only during loading): convoi wants to reserve the whole route
	 * other: previous convoi (in this case, currently always c==cnv)
	 *
	 * if c is NULL, then the vehicle is removed from the convoi
	 * (the waggon_t::set_convoi etc. routines must then remove a
	 *  possibly pending reservation of stops/tracks)
	 */
	assert(  c==NULL  ||  cnv==NULL  ||  cnv==(convoi_t *)1  ||  c==cnv);
	cnv = c;
	if(cnv) {
		// we need to reestablish the finish flag after loading
		if(ist_erstes) {
			check_for_finish = cnv->get_route()->empty()  ||  (route_index>cnv->get_route()->get_max_n())  ||  (get_pos()==cnv->get_route()->position_bei(route_index));
		}
		// some convois were saved with broken coordinates
		if(!welt->lookup(pos_prev)) {
			dbg->error("vehikel_t::set_convoi()","pos_prev is illegal of convoi %i at %s", cnv->self.get_id(), get_pos().get_str() );
			if(welt->lookup_kartenboden(pos_prev.get_2d())) { //"Kartenboden" = "map ground" (Babelfish)
				pos_prev = welt->lookup_kartenboden(pos_prev.get_2d())->get_pos();
			}
			else {
				pos_prev = get_pos();
			}
		}
		if(pos_next!=koord3d::invalid  &&  !cnv->get_route()->empty()  &&  route_index<cnv->get_route()->get_max_n()  &&  (welt->lookup(pos_next)==NULL  ||  welt->lookup(pos_next)->get_weg(get_waytype())==NULL)) {
			pos_next = cnv->get_route()->position_bei(route_index+1u);
		}
		// just correct freight deistinations
		slist_iterator_tpl <ware_t> iter (fracht);
		while(iter.next()) {
			iter.access_current().laden_abschliessen(welt);
		}
	}
}



/**
 * Unload freight to halt
 * @return sum of unloaded goods
 * @author Hj. Malthaner
 */
uint16
vehikel_t::unload_freight(halthandle_t halt)
{
	assert(halt.is_bound());
	uint16 sum_menge = 0;

	slist_tpl<ware_t> kill_queue;
	if(halt->is_enabled( get_fracht_typ() )) 
	{
		if (!fracht.empty())
		{

			slist_iterator_tpl<ware_t> iter (fracht);
			while(iter.next()) 
			{
				const ware_t& tmp = iter.get_current();

				halthandle_t end_halt = tmp.get_ziel();
				halthandle_t via_halt = tmp.get_zwischenziel();

				// probleme mit fehlerhafter ware
				// vielleicht wurde zwischendurch die
				// Zielhaltestelle entfernt ?
				if(!end_halt.is_bound() || !via_halt.is_bound()) 
				{
					DBG_MESSAGE("vehikel_t::entladen()", "destination of %d %s is no longer reachable",tmp.menge,translator::translate(tmp.get_name()));
					kill_queue.insert(tmp);
				} 

				else if(end_halt==halt || via_halt==halt) 
				{

					//		    printf("Liefere %d %s nach %s via %s an %s\n",
					//                           tmp->menge,
					//			   tmp->name(),
					//			   end_halt->get_name(),
					//			   via_halt->get_name(),
					//			   halt->get_name());

					// hier sollte nur ordentliche ware verabeitet werden
					// "here only tidy commodity should be processed" (Babelfish) 
					int menge = halt->liefere_an(tmp); //"supply" (Babelfish)
					sum_menge += menge;

					// Calculates the revenue for each packet under the new
					// revenue model. 
					// @author: jamespetts
					cnv->calc_revenue(iter.access_current());

					// book delivered goods to destination
					if(end_halt==halt) 
					{
						// pax is always index 1
						const int categorie = tmp.get_index()>1 ? 2 : tmp.get_index();
						get_besitzer()->buche( menge, (player_cost)(COST_TRANSPORTED_PAS+categorie) );
					}

					kill_queue.insert(tmp);

					INT_CHECK("simvehikel 937");
				}
			}
		}
	}

	slist_iterator_tpl<ware_t> iter (kill_queue);
	while( iter.next() ) 
	{
		total_freight -= iter.get_current().menge;
		bool ok = fracht.remove(iter.get_current());
		assert(ok);
	}

	return sum_menge;
}


/**
 * Load freight from halt
 * @return loading successful?
 * @author Hj. Malthaner
 */
bool vehikel_t::load_freight(halthandle_t halt, bool overcrowd)
{
	const bool ok = halt->gibt_ab(besch->get_ware());
	schedule_t *fpl = cnv->get_schedule();
	if( ok ) 
	{

		while(total_freight < besch->get_zuladung() + (overcrowd ? besch->get_overcrowded_capacity() : 0)) //"Payload" (Google)
		{
			// Modified to allow overcrowding.
			// @author: jamespetts
			const uint16 hinein = (besch->get_zuladung() - total_freight) + (overcrowd ? besch->get_overcrowded_capacity() : 0); 
			//hinein = inside (Google)

			ware_t ware = halt->hole_ab(besch->get_ware(), hinein, fpl);
			
			if(ware.menge==0) 
			{
				// now empty, but usually, we can get it here ...
				return ok;
			}

			slist_iterator_tpl<ware_t> iter (fracht);

			// could this be joined with existing freight?
			while(iter.next()) 
			{
				ware_t &tmp = iter.access_current();

				/*
				 *	OLD SYSTEM - did not take account of origins, etc.
				 * 
				 * // for pax: join according next stop
				 * // for all others we *must* use target coordinates
				 * if(ware.same_destination(tmp)) {
				 */

				// New system: only merges if origins are alike.
				// @author: jamespetts
				if(ware.can_merge_with(tmp))
				{
					tmp.menge += ware.menge;
					total_freight += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not join it to existing => load it
			if(ware.menge != 0) 
			{
				fracht.insert(ware);
				total_freight += ware.menge;
			}

			INT_CHECK("simvehikel 876");
		}
	}
	return ok;
}



void
vehikel_t::set_offsets(int x, int y)
{
	set_xoff( x );
	set_yoff( y );
}


/**
 * Remove freight that no longer can reach its destination
 * i.e. becuase of a changed schedule
 * @author Hj. Malthaner
 */
void vehikel_t::remove_stale_freight()
{
	DBG_DEBUG("vehikel_t::remove_stale_freight()", "called");

	// and now check every piece of ware on board,
	// if its target is somewhere on
	// the new schedule, if not -> remove
	slist_tpl<ware_t> kill_queue;
	total_freight = 0;

	if (!fracht.empty()) {
		slist_iterator_tpl<ware_t> iter (fracht);
		while(iter.next()) {
			schedule_t *fpl = cnv->get_schedule();

			ware_t& tmp = iter.access_current();
			bool found = false;

			for (int i = 0; i < fpl->get_count(); i++) {
				if (haltestelle_t::get_halt( welt, fpl->eintrag[i].pos ) == tmp.get_zwischenziel()) {
					found = true;
					break;
				}
			}

			if (!found) {
				kill_queue.insert(tmp);
			}
			else {
				// since we need to point at factory (0,0), we recheck this too
				koord k = tmp.get_zielpos();
				fabrik_t *fab = fabrik_t::get_fab( welt, k );
				tmp.set_zielpos( fab ? fab->get_pos().get_2d() : k );

				total_freight += tmp.menge;
			}
		}

		slist_iterator_tpl<ware_t> killer (kill_queue);
		while(killer.next()) {
			fracht.remove(killer.get_current());
		}
	}
}


void
vehikel_t::play_sound() const
{
	if(  besch->get_sound() >= 0  &&  !welt->is_fast_forward()  ) {
		struct sound_info info;
		info.index = besch->get_sound();
		info.volume = 255;
		info.pri = 0;
		welt->play_sound_area_clipped(get_pos().get_2d(), info);
	}
}


/**
 * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
 * der Convoi eine neue Route ermittelt
 *
 * Fahrzeiug prepares for new journey before - will be called 
 * if the Convoi a new route determined
 * @author Hj. Malthaner
 */
void vehikel_t::neue_fahrt(uint16 start_route_index, bool recalc)
{
	route_index = start_route_index+1;
	check_for_finish = false;
	use_calc_height = true;

	if(welt->ist_in_kartengrenzen(get_pos().get_2d())) { //"Is in map border" (Babelfish)
		// mark the region after the image as dirty
		// better not try to twist your brain to follow the retransformation ...
		mark_image_dirty( get_bild(), hoff );
	}

	if(!recalc) {
		// always set pos_next
		// otherwise a convoi with somehow a wrong pos_next will next continue
		pos_next = cnv->get_route()->position_bei(route_index);
		assert( get_pos() == cnv->get_route()->position_bei(start_route_index) );
	}
	else {

		// recalc directions
		pos_next = cnv->get_route()->position_bei(route_index);
		pos_prev = get_pos();
		set_pos( cnv->get_route()->position_bei(start_route_index) );

		alte_fahrtrichtung = fahrtrichtung; //1
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		hoff = 0;
		steps = 0;

		set_xoff( (dx<0) ? TILE_STEPS : -TILE_STEPS );
		set_yoff( (dy<0) ? TILE_STEPS/2 : -TILE_STEPS/2 );

		calc_bild();
		
		if(alte_fahrtrichtung != fahrtrichtung)
		{
			pre_corner_direction.clear();
		}
	}

	steps_next = ribi_t::ist_einfach(fahrtrichtung) ? 255 : diagonal_length;
}



vehikel_t::vehikel_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp) :
	vehikel_basis_t(sp->get_welt(), pos)
{
	this->besch = besch;

	set_besitzer( sp );
	insta_zeit = welt->get_current_month();
	cnv = NULL;
	speed_limit = SPEED_UNLIMITED;

	route_index = 1;

	rauchen = true;
	fahrtrichtung = ribi_t::keine;

	current_friction = 4;
	total_freight = 0;
	sum_weight = besch->get_gewicht();

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;
	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
	target_halt = halthandle_t();

	//@author: jamespetts 
#ifdef debug_corners	 
	current_corner = 0;
#endif
	 direction_steps = 4;
	 //local_bonus_supplement = 0;
	 is_overweight = false;
	 reversed = false;
}


vehikel_t::vehikel_t(karte_t *welt) :
	vehikel_basis_t(welt)
{
	rauchen = true;

	besch = NULL;
	cnv = NULL;

	route_index = 1;
	current_friction = 4;
	sum_weight = 10;
	total_freight = 0;

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;
	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;

	//@author: jamespetts 
#ifdef debug_corners 
	 current_corner = 0;
#endif
	 direction_steps = 4;
	 //local_bonus_supplement = 0;
	 is_overweight = false;
	 reversed = false;
}



bool vehikel_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
	return route->calc_route(welt, start, ziel, this, max_speed);
}



bool vehikel_t::hop_check()
{
	// the leading vehicle will do all the checks
	if(ist_erstes) {
		if(check_for_finish) {
			// so we are there yet?
			cnv->ziel_erreicht();
			if(cnv->get_state()==convoi_t::INITIAL) {
				// to avoid crashes with airplanes
				use_calc_height = false;
			}
			return false;
		}

		// now check, if we can go here
		const grund_t *bd = welt->lookup(pos_next);
		if(bd==NULL  ||  !ist_befahrbar(bd)  ||  cnv->get_route()->empty()) {
			// weg not existent (likely destroyed) or no route ...
			cnv->suche_neue_route();
			return false;
		}

		// check for oneway sign etc.
		if(air_wt!=get_waytype()  &&  route_index<cnv->get_route()->get_max_n()) {
			uint8 dir = get_ribi(bd);
			koord3d nextnext_pos = cnv->get_route()->position_bei(route_index+1);
			uint8 new_dir = ribi_typ(nextnext_pos.get_2d()-pos_next.get_2d());
			if((dir&new_dir)==0) {
				// new one way sign here?
				cnv->suche_neue_route();
				return false;
			}
		}

		int restart_speed = -1;
		// ist_weg_frei() berechnet auch die Geschwindigkeit
		// mit der spaeter weitergefahren wird
		if(!ist_weg_frei(restart_speed)) {

			// convoi anhalten, wenn strecke nicht frei
			cnv->warten_bis_weg_frei(restart_speed);

			// nicht weiterfahren
			return false;
		}
	}
	return true;
}



void
vehikel_t::verlasse_feld()
{
	vehikel_basis_t::verlasse_feld();
#ifndef DEBUG_ROUTES
	if(ist_letztes  &&  reliefkarte_t::is_visible) {
			reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}
#endif
}




/* this routine add a vehicle to a tile and will insert it in the correct sort order to prevent overlaps
 * @author prissi
 */
void
vehikel_t::betrete_feld()
{
	vehikel_basis_t::betrete_feld();
	if(ist_erstes  &&  reliefkarte_t::is_visible  ) {
		reliefkarte_t::get_karte()->calc_map_pixel( get_pos().get_2d() );  //"Set relief colour" (Babelfish)
	}
}


void
vehikel_t::hop()
{
	// Fahrtkosten
	// "Travel costs" (Babelfish)
	uint16 costs = besch->get_betriebskosten(welt);
	if(costs != base_costs)
	{
		// Recalculate base costs only if necessary
		// With this formula, no need to initialise base 
		// costs or diagonal costs!
		base_costs = costs;
		diagonal_costs = (costs * diagonal_length) / 255;
	}
	if(steps_next != 255)
	{
		costs = diagonal_costs;
	}
	
	cnv->add_running_cost(-costs);	
	
	verlasse_feld(); //"Verlasse" = "leave" (Babelfish)

	pos_prev = get_pos();
	set_pos( pos_next );  // naechstes Feld ("next field" - Babelfish)
	if(route_index<cnv->get_route()->get_max_n()) {
		route_index ++;
		pos_next = cnv->get_route()->position_bei(route_index);
	}
	else {
		check_for_finish = true;
	}
	alte_fahrtrichtung = fahrtrichtung; //2

	// this is a required hack for aircraft: aircraft can turn on a single square, and this confuses the previous calculation.
	// author: hsiegeln
	if(!check_for_finish  &&  pos_prev.get_2d()==pos_next.get_2d()) {
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next.get_2d() );
		steps_next = 0;
	}
	else {
		if(pos_next!=get_pos()) {
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::ist_gerade(welt->lookup(pos_next)->get_weg_ribi_unmasked(get_waytype()))  )  ||  welt->lookup(pos_next)->is_halt()) {
			// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
			fahrtrichtung = calc_set_richtung( pos_prev.get_2d(), pos_next.get_2d() );
		}
	}
	calc_bild(); //Calculate image

	betrete_feld(); //"Enter field" (Google)
	grund_t *gr;
	gr = welt->lookup(pos_next);

	const weg_t * weg = gr->get_weg(get_waytype());
	if(weg) {
		speed_limit = kmh_to_speed( weg->get_max_speed() );
		// Weight limit needed for GUI flag
		uint32 weight_limit = (weg->get_max_weight());	
		// Necessary to prevent division by zero exceptions if
		// weight limit is set to 0 in the file.
		if(weight_limit < 1)
		{
			weight_limit = 1;
		}
		is_overweight = (cnv->get_heaviest_vehicle() > weight_limit); 

		if(alte_fahrtrichtung != fahrtrichtung)
		{
			pre_corner_direction.add_to_tail(get_direction_degrees(ribi_t::get_dir(alte_fahrtrichtung)));
		}
		else
		{
			pre_corner_direction.add_to_tail(999);
		}

		speed_limit = calc_modified_speed_limit(&(cnv->get_route()->position_bei(route_index)), fahrtrichtung, (alte_fahrtrichtung != fahrtrichtung));
		if(weg->is_crossing()) {
			gr->find<crossing_t>(2)->add_to_crossing(this);
			}
	}
	else {
		speed_limit = SPEED_UNLIMITED;
	}

	if(check_for_finish & ist_erstes) {
		if(  fahrtrichtung==ribi_t::nord  || fahrtrichtung==ribi_t::west ) {
			steps_next = (steps_next/2)+1;
		}
	}
	
	calc_akt_speed(gr);

	sint8 trim_size = pre_corner_direction.get_count() - direction_steps;
	pre_corner_direction.trim_from_head((trim_size >= 0) ? trim_size : 0);

}

/* Calculates the modified speed limit of the current way,
 * taking into account the curve and weight limit.
 * @author: jamespetts
 */
uint32
vehikel_t::calc_modified_speed_limit(const koord3d *position, ribi_t::ribi current_direction, bool is_corner)
{
	grund_t *g;
	g = welt->lookup(*position);
	waytype_t waytype = get_waytype();
	const weg_t *w = g->get_weg(waytype);
	uint32 base_limit;
	uint16 weight_limit;
	static bool is_tilting = besch->get_tilting();
	if(w != NULL)
	{
		base_limit = kmh_to_speed(w->get_max_speed());
		weight_limit = w->get_max_weight();
	}
	else
	{
		//Necessary to prevent access violations in some cases of loading saved games.
		base_limit = kmh_to_speed(200);
		weight_limit = 999;
	}
	uint32 overweight_speed_limit = base_limit;
	uint32 corner_speed_limit = base_limit;
	uint32 new_limit = base_limit;
	uint32 heaviest_vehicle = cnv->get_heaviest_vehicle();

	//Reduce speed for overweight vehicles

	if(heaviest_vehicle > weight_limit)
	{
		if(heaviest_vehicle / weight_limit <= 1.1)
		{
			//Overweight by up to 10% - reduce speed limit to a third.
			overweight_speed_limit = base_limit / 3;
		}
		else if(heaviest_vehicle / weight_limit > 1.1)
		{
			//Overweight by more than 10% - reduce speed limit by a factor of 10.
			overweight_speed_limit = base_limit / 10;
		}
	}

	// Cornering settings. Vehicles must slow to take corners.
	static uint32 max_corner_limit = welt->get_einstellungen()->get_max_corner_limit(waytype);
	static uint32 min_corner_limit = welt->get_einstellungen()->get_min_corner_limit(waytype);
	static float max_corner_adjustment_factor = welt->get_einstellungen()->get_max_corner_adjustment_factor(waytype);
	static float min_corner_adjustment_factor = welt->get_einstellungen()->get_min_corner_adjustment_factor(waytype);
	static uint8 min_direction_steps = welt->get_einstellungen()->get_min_direction_steps(waytype);
	static uint8 max_direction_steps = welt->get_einstellungen()->get_max_direction_steps(waytype);
	
#ifndef debug_corners	
	if(is_corner)
	{
#endif
 		sint16 direction_difference = 0;
		sint16 direction = get_direction_degrees(ribi_t::get_dir(current_direction));
		const koord3d *current_tile = position;
		const koord3d *previous_tile = &cnv->get_route()->position_bei(route_index - 1);
		ribi_t::ribi old_direction = current_direction;
		if(previous_tile != NULL)
		{
			old_direction = calc_check_richtung(previous_tile->get_2d(), position->get_2d());
		}
	
		float limit_adjustment_factor = 1;
		
		if(base_limit > max_corner_limit)
		{
			limit_adjustment_factor = max_corner_adjustment_factor;
			direction_steps = max_direction_steps;
		}
		else if(base_limit < min_corner_limit)
		{
			limit_adjustment_factor = min_corner_adjustment_factor;
			direction_steps = min_direction_steps;
		}
		else
		{
			//Smooth the difference
			float tmp1 = base_limit - min_corner_limit;
			float tmp2 = max_corner_limit - min_corner_limit;
			float proportion = tmp1 / tmp2;
			limit_adjustment_factor = ((max_corner_adjustment_factor - min_corner_adjustment_factor) * proportion) + min_corner_adjustment_factor;
			direction_steps = ((max_direction_steps - min_direction_steps) * proportion) + min_direction_steps; 
		}
		
		uint16 tmp;

		for(int i = (pre_corner_direction.get_count() >= direction_steps) ? direction_steps - 1 : pre_corner_direction.get_count() - 1; i > 0; i --)
		{
			tmp = vehikel_t::compare_directions(direction, pre_corner_direction.get_element(i));
			if(tmp > direction_difference)
			{
				direction_difference = tmp;
			}
		}

#ifdef debug_corners
		current_corner = direction_difference;
#endif
		if(direction_difference == 0 && current_direction != old_direction)
		{
			//Fallback code in case the checking the histories did not work properly (for example, if the histories were cleared recently) 
			direction_difference = compare_directions(get_direction_degrees(ribi_t::get_dir(current_direction)), get_direction_degrees(ribi_t::get_dir(old_direction)));
		}

		//Smoothing code: slightly smoothed corners benefit.		
		if(direction_difference > compare_directions(direction, get_direction_degrees(ribi_t::get_dir(old_direction)) && limit_adjustment_factor < 0.8))
		{
			limit_adjustment_factor += 0.15;
			if(limit_adjustment_factor >= 1)
			{
				//But there is a limit to the benefit of smoothness.
				limit_adjustment_factor = 0.97;
			}
		}

		//Tilting only makes a difference on faster track and on smoothed corners.
		if(is_tilting && base_limit > kmh_to_speed(160) && compare_directions(direction, get_direction_degrees(ribi_t::get_dir(old_direction)) <= 45))
		{	
			// Tilting trains can take corners faster
			limit_adjustment_factor += 0.30;
			if(limit_adjustment_factor > 1)
			{
				//But cannot go faster on a corner than on the straight!
				limit_adjustment_factor = 1;
			}
		}

		switch(direction_difference)
		{
			case 0 :
				
				//If we are here, there *must* be a curve, since we have already checked that.
				//If this is 0, this is an error, and we will assume a 45 degree bend.
#ifndef debug_corners
				corner_speed_limit = (base_limit * limit_adjustment_factor); 
#else
				corner_speed_limit = base_limit;
#endif
				break;

			case 45 :
				
				corner_speed_limit = (base_limit * limit_adjustment_factor);
				break;

			case 90 :
				corner_speed_limit = (base_limit * (limit_adjustment_factor * 0.5));
				break;
					
			case 135 :

				corner_speed_limit = (base_limit * (limit_adjustment_factor * 0.35));
				break;

			case 180 :

				corner_speed_limit = (base_limit * (limit_adjustment_factor * 0.25));
				break;
				
			default :
				//treat as 45 degree bend if something has gone wrong in the calculations.
				//There *must* be a curve here, as the original bool flag was triggered.
				corner_speed_limit = (base_limit * limit_adjustment_factor);
		}
#ifndef debug_corners
	}
#endif
	
	//Overweight penalty not to be made cumulative to cornering penalty

	if(corner_speed_limit < overweight_speed_limit)
	{
		new_limit = corner_speed_limit;
	}
	else
	{
		new_limit = overweight_speed_limit;
	}

	//Speed limit must never be 0.
	if(new_limit > 0)
	{
		return new_limit;
	}
	else
	{
		return kmh_to_speed(10);
	}
}


/* calculates the current friction coefficient based on the curent track
 * flat, slope, (curve)...
 * @author prissi, HJ
 */
void
vehikel_t::calc_akt_speed(const grund_t *gr) //,const int h_alt, const int h_neu)
{

	// assume straight flat track
	current_friction = 1;
	
	//The level (if any) of additional friction to apply around corners.
	waytype_t waytype = get_waytype();
	static uint8 curve_friction_factor = welt->get_einstellungen()->get_curve_friction_factor(waytype);

	// Old method - not realistic. Now uses modified speed limit. Preserved optionally.
	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) { //"Old direction != direction"	
		current_friction += curve_friction_factor;
	}

	// or a hill?
	// Cumulative drag for hills: @author: jamespetts
	const hang_t::typ hang = gr->get_weg_hang();
	if(hang!=hang_t::flach) 
	{
		if(ribi_typ(hang) == fahrtrichtung)
		{
			//Uphill
			hill_up ++;
			hill_down = 0;

			switch(hill_up)
			{
			case 0:
				break;

			case 1:
				current_friction += 18;
				break;

			case 2:
				current_friction += 25;
				break;

			case 3:
				current_friction += 32;
				break;

			case 4:
				current_friction += 38;
				break;

			case 5:
			default:
				current_friction += 45;
				break;

			};
		}

		else
		{
			//Downhill
			hill_down ++;
			hill_up = 0;
	
			switch(hill_down)
			{
			case 0:
				break;

			case 1:
				current_friction -= 10;
				break;

			case 2:
				current_friction -= 13;
				break;

			case 3:
			default:
				current_friction -= 15;
			};
		}
	}

	else
	{
		//No hill at all - reset hill count.
		hill_up = 0;
		hill_down = 0;
	}

	if(ist_erstes) { //"Is the first" (Google)
		uint32 akt_speed = speed_limit;

		uint32 tiles_left = cnv->get_next_stop_index()+1-route_index;
		if(tiles_left<4) {
			// break at the end of stations/in front of signals
			uint32 brake_speed_soll = akt_speed;

			if(check_for_finish) {
				// for the half last tile to stop in stations only
				akt_speed = kmh_to_speed(25);
			}
			else {
				switch(tiles_left) {
					case 3: brake_speed_soll = kmh_to_speed(200); break;
					case 2: brake_speed_soll = kmh_to_speed(100); break;
					case 1: brake_speed_soll = kmh_to_speed(50); break;
					case 0: assert(1);
					default: break;
				}
			}
			if(brake_speed_soll<(uint32)akt_speed) {
				akt_speed = brake_speed_soll;
			}
		}
		// speed is limited anyway in the convoi
		cnv->set_akt_speed_soll( akt_speed );
	}
}

vehikel_t::direction_degrees vehikel_t::get_direction_degrees(ribi_t::dir direction)
{
	switch(direction)
	{
	case ribi_t::dir_nord :
		return vehikel_t::North;
	case ribi_t::dir_nordost :
		return vehikel_t::Northeast;
	case ribi_t::dir_ost :
		return vehikel_t::East;
	case ribi_t::dir_suedost :
		return vehikel_t::Southeast;
	case ribi_t::dir_sued :
		return vehikel_t::South;
	case ribi_t::dir_suedwest :
		return vehikel_t::Southwest;
	case ribi_t::dir_west :
		return vehikel_t::West;
	case ribi_t::dir_nordwest :
		return vehikel_t::Northwest;
	};
	return vehikel_t::North;
}

void
vehikel_t::rauche()
{
	// raucht ueberhaupt ?
	if(rauchen && besch->get_rauch()) {

		bool smoke = besch->get_engine_type()==vehikel_besch_t::steam;

		if(!smoke) {
			// Hajo: only produce smoke when heavily accelerating
			//       or steam engine
			uint32 akt_speed = kmh_to_speed(besch->get_geschw());
			if(akt_speed > speed_limit) {
				akt_speed = speed_limit;
			}

			smoke = (cnv->get_akt_speed() < (sint32)((akt_speed*7u)>>3));
		}

		if(smoke) {
			grund_t * gr = welt->lookup( get_pos() );
			// nicht im tunnel ?
			if(gr && !gr->ist_im_tunnel() ) {
				wolke_t *abgas =  new wolke_t(welt, get_pos(), get_xoff()+((dx*(sint16)((uint16)steps*TILE_STEPS))>>8), get_yoff()+((dy*(sint16)((uint16)steps*TILE_STEPS))>>8)+hoff, besch->get_rauch() );

				if( !gr->obj_add(abgas) ) {
					delete abgas;
				} else {
					welt->sync_add( abgas );
				}
			}
		}
	}
}



/**
 * Payment is done per hop. It iterates all goods and calculates
 * the income for the last hop. This method must be called upon
 * every stop.
 * @return income total for last hop
 * @author Hj. Malthaner
 */
/*sint64 vehikel_t::calc_gewinn(koord start, koord end, convoi_t *cnv) const 
{
	//According to Google translations: 
	//Calculate profit ("gewinn")
	//"Menge" = "volume"
	//"zwischenziel" = "Between target"
	//"Fracht" = "Freight"
	//"Grundwert" = "Fundamental value"
	//"Preis" = "Price"

	// may happen when waiting in station
	if(start==end  ||  fracht.get_count()==0) {
		return 0;
	}

	//const long dist = abs(end.x - start.x) + abs(end.y - start.y);
	const sint32 ref_speed = welt->get_average_speed( get_besch()->get_waytype() );
	const sint32 speed_base = (100*speed_to_kmh(cnv->get_min_top_speed()))/ref_speed-100;

	sint64 value = 0;
	slist_tpl<ware_t> kill_queue;
	slist_iterator_tpl <ware_t> iter (fracht);

	while( iter.next() ) 
	{

		const ware_t &ware = iter.get_current();

		if(ware.menge==0  ||  !ware.get_zwischenziel().is_bound()) 
		{
			continue;
		}

		// now only use the real gain in difference for the revenue (may as well be negative!)
		const koord zwpos = ware.get_zwischenziel()->get_basis_pos();
		const long dist = abs(zwpos.x - start.x) + abs(zwpos.y - start.y) - (abs(end.x - zwpos.x) + abs(end.y - zwpos.y));

		const ware_besch_t* goods = ware.get_besch();
		const sint32 grundwert128 = goods->get_preis()<<7;

		
		 // Calculates the speed bonus, taking into account: (
		 // (1) the local bonus; and
		 // (2) the speed bonus distance settings. 
		 // @author: jamespetts
		 

		const uint16 base_bonus = goods->get_speed_bonus();
		uint16 adjusted_bonus = 0;
		uint16 local_bonus_supplement = (welt->get_einstellungen()->get_local_bonus_multiplier() / 100) * base_bonus;
		
		if(dist <= welt->get_einstellungen()->get_min_bonus_max_distance())
		{
			//If distance of journey too low, disapply speed bonus entirely, but apply local bonus supplement instead.
			adjusted_bonus = local_bonus_supplement;
		}
		
		else if(dist > welt->get_einstellungen()->get_min_bonus_max_distance() && dist < welt->get_einstellungen()->get_max_bonus_min_distance())
		{
			//Apply proportionate mix of local bonus supplement and conventional speed bonus. 
			//The effect of this code is that, if the local supplement is set to 1, a 100% speed bonus is applied until max_bonus_min_distance is reached.
			//For that reason, it is better not to set local supplement to 1. 
			float tmp = (welt->get_einstellungen()->get_max_bonus_min_distance() - welt->get_einstellungen()->get_min_bonus_max_distance());
			float proportion_distance = dist / tmp;
			uint16 intermediate_bonus = base_bonus * proportion_distance;
			uint16 intermediate_local_supplement = local_bonus_supplement * (1 / proportion_distance);
			adjusted_bonus = intermediate_bonus + intermediate_local_supplement;
		}

		else if(dist >= welt->get_einstellungen()->get_max_bonus_min_distance())
		{
			//Apply full speed bonus in conventional way.
			adjusted_bonus = base_bonus;
		}

		//const sint32 grundwert128 = ware.get_besch()->get_preis()<<7;	// bonus price will be always at least 0.128 of the real price
		const sint32 grundwert_bonus = (ware.get_besch()->get_preis()*(1000 + speed_base * adjusted_bonus));
		sint64 price = (sint64)(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus) * (sint64)dist * (sint64)ware.menge;

		//Apply the catering bonus, if applicable.
		if(cnv->get_catering_level(ware.get_besch()->get_catg_index()) > 0)
		{
			float catering_bonus = 1;
			//TODO: Add code for calculating catering bonus
			value *= catering_bonus;
		}


		// sum up new price
		value += price;
	}

	// Hajo: Rounded value, in cents
	// prissi: Why on earth 1/3???
	return (value+1500ll)/3000ll;
}*/


const char *vehikel_t::get_fracht_mass() const
{
	return get_fracht_typ()->get_mass();
}


/**
 * Berechnet Gesamtgewicht der transportierten Fracht in KG
 * "Calculated total weight of cargo transported in KG"  (Translated by Google)
 * @author Hj. Malthaner
 */
uint32 vehikel_t::get_fracht_gewicht() const
{
	uint32 weight = 0;

	slist_iterator_tpl<ware_t> iter(fracht);
	while(iter.next()) {
		weight +=
			iter.get_current().menge *
			iter.get_current().get_besch()->get_weight_per_unit();
	}
	return weight;
}


const char *vehikel_t::get_fracht_name() const
{
	return get_fracht_typ()->get_name();
}


void vehikel_t::get_fracht_info(cbuffer_t & buf)
{
	if (fracht.empty()) {
		buf.append("  ");
		buf.append(translator::translate("leer"));
		buf.append("\n");
	} else {

		slist_iterator_tpl<ware_t> iter (fracht);

		while(iter.next()) {
			const ware_t& ware = iter.get_current();
			const char * name = "Error in Routing";

			halthandle_t halt = ware.get_ziel();
			if(halt.is_bound()) {
				name = halt->get_name();
			}

			buf.append("   ");
			buf.append(ware.menge);
			buf.append(translator::translate(ware.get_mass()));
			buf.append(" ");
			buf.append(translator::translate(ware.get_name()));
			buf.append(" > ");
			buf.append(name);
			buf.append("\n");
		}
	}
}


void
vehikel_t::loesche_fracht()
{
	fracht.clear();
}


bool
vehikel_t::beladen(koord, halthandle_t halt, bool overcrowd)
{
	bool ok = true;
	if(halt.is_bound()) 
	{
		ok = load_freight(halt, overcrowd);
	}
	sum_weight = (get_fracht_gewicht()+499)/1000 + besch->get_gewicht();
	calc_bild();
	return ok;
}


/**
 * fahrzeug an haltestelle entladen
 * "Vehicle to stop discharged" (translated by Google)
 * @author Hj. Malthaner
 */
bool vehikel_t::entladen(koord, halthandle_t halt)
{
	// printf("Vehikel %p entladen\n", this);
	uint16 menge = unload_freight(halt);
	if(menge > 0) 
	{
		// add delivered goods to statistics
		cnv->book(menge, CONVOI_TRANSPORTED_GOODS);
		// add delivered goods to halt's statistics
		halt->book(menge, HALT_ARRIVED);
		return true;
	}
	return false;
}


/**
 * Ermittelt fahrtrichtung
 * "Determined Direction" (translated by Google)
 * @author Hj. Malthaner
 */
ribi_t::ribi vehikel_t::richtung()
{
	ribi_t::ribi neu = calc_richtung(pos_prev.get_2d(), pos_next.get_2d());
	// nothing => use old direct further on
	return (neu == ribi_t::keine) ? fahrtrichtung : neu;
}


void
vehikel_t::calc_bild() //"Bild" = "picture" (Google)
{
	image_id old_bild=get_bild();
	if (fracht.empty()) 
	{
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_direction_of_travel()),NULL)); 
	}
	else 
	{
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_direction_of_travel()), fracht.front().get_besch()));
	}
	if(old_bild!=get_bild()) {
		set_flag(ding_t::dirty);
	}
}

ribi_t::ribi
vehikel_t::get_direction_of_travel()
{
	ribi_t::ribi dir = get_fahrtrichtung();
	if(reversed)
	{
		switch(dir)
		{
		case ribi_t::nord:
			dir = ribi_t::sued;
			break;

		case ribi_t::ost:
			dir = ribi_t::west;
			break;

		case ribi_t::nordost:
			dir = ribi_t::suedwest;
			break;

		case ribi_t::sued:
			dir = ribi_t::nord;
			break;

		case ribi_t::suedost:
			dir = ribi_t::nordwest;
			break;

		case ribi_t::west:
			dir = ribi_t::ost;
			break;

		case ribi_t::nordwest:
			dir = ribi_t::suedost;
			break;

		case ribi_t::suedwest:
			dir = ribi_t::nordost;
			break;
		};
	}
	return dir;
}

void vehikel_t::set_reversed(bool value)
{
	if(besch->is_bidirectional() || (cnv != NULL && cnv->get_reversable()))
	{
		reversed = value;
	}
}

uint16 vehikel_t::get_overcrowding() const
{
	return total_freight - besch->get_zuladung() > 0 ? total_freight - besch->get_zuladung() : 0;
}

uint8 vehikel_t::get_comfort() const
{
	if(besch->get_comfort() == 0)
	{
		return 0;
	}
	else if(total_freight <= get_fracht_max())
	{
		// Not overcrowded - return base level
		return besch->get_comfort();
	}

	// Else
	// Overcrowded - adjust comfort. Standing passengers
	// are very uncomfortable (no more than 10).
	const uint8 standing_comfort = 10 < besch->get_comfort() - 5 ? 10 : besch->get_comfort() / 2;
	uint16 passenger_count = 0;
	slist_iterator_tpl<ware_t> iter(fracht);
	while(iter.next()) 
	{
		ware_t ware = iter.get_current();
		if(ware.get_catg() == 0)
		{
			passenger_count += ware.menge;
		}
	}
	const uint16 total_seated_passengers = passenger_count < get_fracht_max() ? passenger_count : get_fracht_max();
	const uint16 total_standing_passengers = passenger_count > total_seated_passengers ? passenger_count - total_seated_passengers : 0;
	// Avoid division if we can
	if(total_seated_passengers < 1)
	{
		return besch->get_comfort();
	}
	// Else
	// Average comfort of seated and standing
	return ((total_seated_passengers * besch->get_comfort()) + (total_standing_passengers * standing_comfort)) / passenger_count;
}

void vehikel_t::rdwr(loadsave_t *file)
{
	// this is only called from dingliste => we save nothing ...
	assert(  file->is_saving()  );
}



void vehikel_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t r( file, "vehikel_t" );

	sint32 fracht_count = 0;

	if(file->is_saving()) {
		fracht_count = fracht.get_count();
		// we try to have one freight count to guess the right freight
		// when no besch is given
		if(fracht_count==0  &&  besch->get_ware()!=warenbauer_t::nichts  &&  besch->get_zuladung()>0) {
			fracht_count = 1;
		}
	}

	ding_t::rdwr(file);

	// since ding_t does no longer save positions
	if(  file->get_version()>=101000  ) {
		koord3d pos = get_pos();
		pos.rdwr(file);
		set_pos(pos);
	}

	if(file->get_version()<86006) {
		// parameter werden in der deklarierten reihenfolge gespeichert
		// "Parameters are declared in the order saved" (Translated by Google)
		sint32 l;
		file->rdwr_long(insta_zeit, "\n");
		file->rdwr_long(l, " ");
		dx = (sint8)l;
		file->rdwr_long(l, "\n");
		dy = (sint8)l;
		file->rdwr_long(l, "\n");
		hoff = (sint8)(l*TILE_HEIGHT_STEP/16);
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_long(l, "\n");
		route_index = (uint16)l;
		insta_zeit = (insta_zeit >> welt->ticks_bits_per_tag) + welt->get_einstellungen()->get_starting_year();
DBG_MESSAGE("vehicle_t::rdwr_from_convoi()","bought at %i/%i.",(insta_zeit%12)+1,insta_zeit/12);
	}
	else {
		// prissi: changed several data types to save runtime memory
		file->rdwr_long(insta_zeit, "\n");
		if(file->get_version()<99018) {
			file->rdwr_byte(dx, " ");
			file->rdwr_byte(dy, "\n");
		}
		else {
			file->rdwr_byte(steps, " ");
			file->rdwr_byte(steps_next, "\n");
			if(steps_next==old_diagonal_length  &&  file->is_loading()) {
				// reset diagonal length (convoi will be resetted anyway, if game diagonal is different)
				steps_next = diagonal_length;
			}
		}
		sint16 dummy16 = ((16*(sint16)hoff)/TILE_HEIGHT_STEP);
		file->rdwr_short(dummy16, "\n");
		hoff = (sint8)((TILE_HEIGHT_STEP*(sint16)dummy16)/16);
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_short(route_index, "\n");
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];
	}

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx=get_xoff(), ddy=get_yoff()-hoff;
		sint8 i=1;
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];

		while(  !is_about_to_hop(ddx+dx*i,ddy+dy*i )  &&  i<16 ) {
			i++;
		}
		i--;
		set_xoff( ddx-(16-i)*dx );
		set_yoff( ddy-(16-i)*dy );
		if(file->is_loading()) {
			if(dx*dy) {
				steps = min( 255, 255-(i*16) );
				steps_next = 255;
			}
			else {
				// will be corrected anyway, if in a convoi
				steps = min( diagonal_length, diagonal_length-(uint8)(((uint16)i*(uint16)diagonal_length)/8) );
				steps_next = diagonal_length;
			}
		}
	}

	// information about the target halt
	if(file->get_version()>=88007) {
		bool target_info;
		if(file->is_loading()) {
			file->rdwr_bool(target_info," ");
			cnv = (convoi_t *)target_info;	// will be checked during convoi reassignement
		}
		else {
			target_info = target_halt.is_bound();
			file->rdwr_bool(target_info, " ");
		}
	}
	else {
		if(file->is_loading()) {
			cnv = NULL;	// no reservation too
		}
	}
	pos_prev.rdwr(file);

	if(file->get_version()<=99004) {
		koord3d dummy;
		dummy.rdwr(file);	// current pos (is already saved as ding => ignore)
	}
	pos_next.rdwr(file);

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s,256);
		besch = vehikelbauer_t::get_info(s);
		if(besch==NULL) {
			besch = vehikelbauer_t::get_info(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			dbg->warning("vehikel_t::rdwr_from_convoi()","no vehicle pak for '%s' search for something similar", s);
		}
	}

	if(file->is_saving()) {
		if (fracht.empty()  &&  fracht_count>0) {
			// create dummy freight for savegame compatibility
			ware_t ware( besch->get_ware() );
			ware.menge = 0;
			ware.set_ziel( halthandle_t() );
			ware.set_zwischenziel( halthandle_t() );
			ware.set_zielpos( get_pos().get_2d() );
			ware.rdwr(welt,file);
		}
		else {
			slist_iterator_tpl<ware_t> iter(fracht);
			while(iter.next()) {
				ware_t ware = iter.get_current();
				ware.rdwr(welt,file);
			}
		}
	}
	else {
		for(int i=0; i<fracht_count; i++) {
			ware_t ware(welt,file);
			if(besch==NULL  ||  ware.menge>0) {	// also add, of the besch is unknown to find matching replacement
				fracht.insert(ware);
			}
		}
	}

	// skip first last info (the convoi will know this better than we!)
	if(file->get_version()<88007) {
		bool dummy = 0;
		file->rdwr_bool(dummy, " ");
		file->rdwr_bool(dummy, " ");
	}

	// koordinate of the last stop
	if(file->get_version()>=99015) {
		last_stop_pos.rdwr(file);
	}

	if(file->is_loading()) {
		ist_erstes = ist_letztes = false;	// dummy, will be setz by convoi afterwards
		if(besch) {
			calc_bild();
			// full weight after loading
			sum_weight = (get_fracht_gewicht()+499)/1000 + besch->get_gewicht();
		}
		// recalc total freight
		total_freight = 0;
		slist_iterator_tpl<ware_t> iter(fracht);
		while(iter.next()) {
			total_freight += iter.get_current().menge;
		}
	}

	if(file->get_experimental_version() >= 1)
	{
		file->rdwr_bool(reversed, "");
	}
	else
	{
		reversed = false;
	}
}


uint32 vehikel_t::calc_restwert() const
{
	// after 20 year, it has only half value
	return (uint32)((double)besch->get_preis() * pow(0.997, (int)(welt->get_current_month() - get_insta_zeit())));
}




void
vehikel_t::zeige_info()
{
	if(cnv != NULL) {
		cnv->zeige_info();
	} else {
		dbg->warning("vehikel_t::zeige_info()","cnv is null, can't open convoi window!");
	}
}


void vehikel_t::info(cbuffer_t & buf) const
{
	if(cnv) {
		cnv->info(buf);
	}
}


const char *
vehikel_t::ist_entfernbar(const spieler_t *)
{
	return "Vehicles cannot be removed";
}

bool vehikel_t::check_way_constraints(const weg_t *way) const
{
	// Permissive way constraints
	// If vehicle has it, way must have it.
	uint8 my_or = besch->get_permissive_constraints() | way->get_way_constraints_permissive();
	bool permissive = way->get_way_constraints_permissive() ^ my_or;

	// Prohibitive way constraints
	// If way has it, vehicle must have it.
	my_or = besch->get_prohibitive_constraints() | way->get_way_constraints_prohibitive();
	bool prohibitive = besch->get_prohibitive_constraints() ^ my_or;

	return (!permissive) && (!prohibitive);
}


vehikel_t::~vehikel_t()
{
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		// remove vehicle's marker from the reliefmap
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());
	}
}


// this routine will display a tooltip for lost, on depot order and stucked vehicles
void vehikel_t::display_after(int xpos, int ypos, bool is_gobal) const
{
	if(is_gobal  &&  cnv  &&  ist_erstes) {

		COLOR_VAL color = COL_GREEN; // not used, but stop compiler warning about uninitialized
		char tooltip_text[1024];
		tooltip_text[0] = 0;
		uint8 state = umgebung_t::show_vehicle_states;
		if(  state==1  ) {
			// only show when mouse over vehicle
			if(  welt->get_zeiger()->get_pos()==get_pos()  ) {
				state = 2;
			}
			else {
				state = 0;
			}
		}

		// now find out what has happend
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::WAITING_FOR_CLEARANCE:
			case convoi_t::CAN_START:
			case convoi_t::CAN_START_ONE_MONTH:
				if(  state>=2  ) {
					sprintf( tooltip_text, translator::translate("Waiting for clearance!") );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::LOADING:
				if(  state>=1  ) {
					sprintf( tooltip_text, translator::translate("Loading (%i->%i%%)!"), cnv->get_loading_level(), cnv->get_loading_limit() );
					color = COL_YELLOW;
				}
				break;

			case convoi_t::FAHRPLANEINGABE:
//			case convoi_t::ROUTING_1:
				if(  state>=2  ) {
					sprintf( tooltip_text, translator::translate("Schedule changing!") );
					color = COL_LIGHT_YELLOW;
				}
				break;

			case convoi_t::DRIVING:
				if(  state>=1  ) {
					grund_t *gr = welt->lookup( cnv->get_route()->position_bei( cnv->get_route()->get_max_n() ) );
					if(  gr  &&  gr->get_depot()  ) {
						sprintf( tooltip_text, translator::translate("go home") );
						color = COL_GREEN;
					}
					else if(  cnv->get_no_load()  ) {
						sprintf( tooltip_text, translator::translate("no load") );
						color = COL_GREEN;
					}
				}
				break;

			case convoi_t::LEAVING_DEPOT:
				if(  state>=2  ) {
					sprintf( tooltip_text, translator::translate("Leaving depot!") );
					color = COL_GREEN;
				}
				break;

			case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			case convoi_t::CAN_START_TWO_MONTHS:
				sprintf( tooltip_text, translator::translate("clf_chk_stucked") );
				color = COL_ORANGE;
				break;

			case convoi_t::NO_ROUTE:
				sprintf( tooltip_text, translator::translate("clf_chk_noroute") );
				color = COL_RED;
				break;
		}
		if(is_overweight)
		{
			sprintf(tooltip_text, translator::translate("Vehicle %s is too heavy for this route: speed limited."), cnv->get_name());
			color = COL_YELLOW;
		}
#ifdef debug_corners
			sprintf(tooltip_text, translator::translate("CORNER: %i"), current_corner);
			color = COL_GREEN;
#endif
		

		// something to show?
		if(  tooltip_text[0]  ) {
			const int width = proportional_string_width(tooltip_text)+7;
			const int raster_width = get_tile_raster_width();
			get_screen_offset( xpos, ypos );
			xpos += tile_raster_scale_x(get_xoff(), raster_width);
			ypos += tile_raster_scale_y(get_yoff(), raster_width);
			if(ypos>LINESPACE+32  &&  ypos+LINESPACE<display_get_clip_wh().yy) {
				display_ddd_proportional( xpos, ypos, width, 0, color, COL_BLACK, tooltip_text, true);
			}
		}
	}
}



/*--------------------------- Fahrdings ------------------------------*/
//(Translated by Babelfish as "driving thing")


automobil_t::automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
}



automobil_t::automobil_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			const ware_besch_t* w = (!fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere);
			dbg->warning("automobil_t::automobil_t()","try to find a fitting vehicle for %s.",  w->get_name() );
			besch = vehikelbauer_t::get_best_matching(road_wt, 0, (fracht.empty() ? 0 : 50), is_first?50:0, speed_to_kmh(speed_limit), w, false, true, last_besch, is_last );
			if(besch) {
				DBG_MESSAGE("automobil_t::automobil_t()","replaced by %s",besch->get_name());
				// still wrong load ...
				calc_bild();
			}
			if(!fracht.empty()  &&  fracht.front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht.remove_first();
			}
		}
		if(  besch  ) {
			last_besch = besch;
		}
	}
}



// ned to reset halt reservation (if there was one)
bool automobil_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
	assert(cnv);
	// free target reservation
	if(ist_erstes  &&  alte_fahrtrichtung!=ribi_t::keine  &&  cnv  &&  target_halt.is_bound() ) {
		route_t *rt=cnv->get_route();
		grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n())); //"bei" = "at" (Google)
		if(target) {
			target_halt->unreserve_position(target,cnv->self);
		}
	}
	target_halt = halthandle_t();	// no block reserved
	return route->calc_route(welt, start, ziel, this, max_speed );
}



bool automobil_t::ist_befahrbar(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->get_weg(road_wt);
	if(str==NULL) {
		return false;
	}
	bool electric = cnv!=NULL  ?  cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric;
	if(electric  &&  !str->is_electrified()) {
		return false;
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			if(rs->get_besch()->get_min_speed()>0  &&  rs->get_besch()->get_min_speed()>kmh_to_speed(get_besch()->get_geschw())) {
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound()  &&  cnv->is_waiting()  &&  rs->get_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA) {
				return false;
			}
		}
	}
	const strasse_t* way = str;
	return check_way_constraints(way);
}



// how expensive to go here (for way search)
// author prissi
int
automobil_t::get_kosten(const grund_t *gr,const uint32 max_speed) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(road_wt);
	if(!w) {
		return 0xFFFF;
	}

	// max_speed?
	uint32 max_tile_speed = w->get_max_speed();

	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	// assume all traffic (and even road signs etc.) is not good ...
	costs += gr->get_top();

	// effect of hang ...
	if(gr->get_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 15;
	}

	//@author: jamespetts
	// Strongly prefer routes for which the vehicle is not overweight.
	uint16 weight_limit = w->get_max_weight();
	if(vehikel_t::get_gesamtgewicht() > weight_limit)
	{
		costs += 40;
	}

	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool automobil_t::ist_ziel(const grund_t *gr, const grund_t *) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
//DBG_MESSAGE("is_target()","success at %i,%i",gr->get_pos().x,gr->get_pos().y);
		return true;
	}
	return false;
}



// to make smaller steps than the tile granularity, we have to use this trick
void automobil_t::get_screen_offset( int &xoff, int &yoff ) const
{
	vehikel_basis_t::get_screen_offset( xoff, yoff );

	// eventually shift position to take care of overtaking
	if(cnv) {
		const int raster_width = get_tile_raster_width();
		if(  cnv->is_overtaking()  ) {
			xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width);
			yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width);
		}
		else if(  cnv->is_overtaken()  ) {
			xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width)/5;
			yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width)/5;
		}
	}
}



bool automobil_t::ist_weg_frei(int &restart_speed)
{
	const grund_t *gr = welt->lookup(pos_next);

	// check for traffic lights (only relevant for the first car in a convoi)
	if(ist_erstes) {
		// pruefe auf Schienenkreuzung
		strasse_t *str=(strasse_t *)gr->get_weg(road_wt);

		if(str==NULL  ||  gr->get_top()>250) {
			// too many cars here or no street
			return false;
		}

		// first: check roadsigns
		if(str->has_sign()) {
			const roadsign_t* rs = gr->find<roadsign_t>();
			if(rs) {
				// since at the corner, our direction may be diagonal, we make it straight
				const uint8 richtung = ribi_typ(get_pos().get_2d(),pos_next.get_2d());

				if(rs->get_besch()->is_traffic_light()  &&  (rs->get_dir()&richtung)==0) {
					fahrtrichtung = richtung;
					calc_bild();
					// wait here
					restart_speed = 16;
					return false;
				}
				// check, if we reached a choose point
				else if(rs->is_free_route(richtung)) {
					route_t *rt=cnv->get_route();
					grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n()));

					// is our target occupied?
					if(target) {
						target_halt = target->get_halt();
						if (target_halt.is_bound() && !target_halt->reserve_position(target, cnv->self)) {

							// if we fail, we will wait in a step, much more simulation friendly
							if(!cnv->is_waiting()) {
								restart_speed = -1;
								target_halt = halthandle_t();
								return false;
							}

							// check if there is a free position
							// this is much faster than waysearch
							if(!target_halt->find_free_position(road_wt,cnv->self,ding_t::automobil)) {
								restart_speed = 0;
								target_halt = halthandle_t();
	//DBG_MESSAGE("automobil_t::ist_weg_frei()","cnv=%d nothing free found!",cnv->self.get_id());
								return false;
							}

							// now it make sense to search a route
							route_t target_rt;
							if(!target_rt.find_route( welt, pos_next, this, 50, richtung, 33 )) {
								// nothing empty or not route with less than 33 tiles
								target_halt = halthandle_t();
								restart_speed = 0;
								return false;
							}
							// now reserve our choice ...
							target_halt->reserve_position(welt->lookup(target_rt.position_bei(target_rt.get_max_n())),cnv->self);
	//DBG_MESSAGE("automobil_t::ist_weg_frei()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.get_max_n()).x,target_rt.position_bei(target_rt.get_max_n()).y, target_rt.position_bei(target_rt.get_max_n()).z );
							rt->remove_koord_from(route_index);
							rt->append( &target_rt );
						}
					}
				}
			}
		}

		vehikel_basis_t *dt = NULL;

		// calculate new direction
		const uint8 next_fahrtrichtung = route_index<cnv->get_route()->get_max_n() ? this->calc_richtung(get_pos().get_2d(), cnv->get_route()->position_bei(route_index+1).get_2d()) : calc_richtung(get_pos().get_2d(), pos_next.get_2d());

		// way should be clear for overtaking: we checked previously
		if(  !cnv->is_overtaking()  ) {
			const uint8 next_90fahrtrichtung = route_index<cnv->get_route()->get_max_n() ? this->calc_richtung(get_pos().get_2d(), cnv->get_route()->position_bei(route_index+1).get_2d()) : calc_richtung(get_pos().get_2d(), pos_next.get_2d());
			dt = no_cars_blocking( gr, cnv, get_fahrtrichtung(), next_fahrtrichtung, next_90fahrtrichtung );

			// do not block intersections
			if(dt==NULL  &&  ribi_t::is_threeway(str->get_ribi_unmasked())  &&  route_index+1u<cnv->get_route()->get_max_n()) {
				// we have to test also next field
				const grund_t *gr = welt->lookup( cnv->get_route()->position_bei(route_index+1u) );
				if(gr) {
					const uint8 nextnext_fahrtrichtung = this->calc_richtung(cnv->get_route()->position_bei(route_index).get_2d(), cnv->get_route()->position_bei(route_index+2).get_2d());
					const uint8 nextnext_90fahrtrichtung = this->calc_richtung(cnv->get_route()->position_bei(route_index+1).get_2d(), cnv->get_route()->position_bei(route_index+2).get_2d());
					dt = no_cars_blocking( gr, cnv, next_fahrtrichtung, nextnext_fahrtrichtung, nextnext_90fahrtrichtung );
				}
			}
		}

		// do not block crossings
		if(dt==NULL  &&  str->is_crossing()) {
			// ok, reserve way crossing
			crossing_t* cr = gr->find<crossing_t>(2);
			if(  !cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
			// no further check, when already entered a crossing (to allow leaving it)
			grund_t *gr_here = welt->lookup(get_pos());
			if(gr_here  &&  gr_here->ist_uebergang()) {
				return true;
			}
			// can cross, but can we leave?
			uint32 test_index = route_index+1u;
			while(test_index+1u<cnv->get_route()->get_max_n()) {
				const grund_t *test = welt->lookup(cnv->get_route()->position_bei(test_index));
				if(!test) {
					break;
				}
				// test next field after crossing
				const uint8 nextnext_fahrtrichtung = this->calc_richtung(cnv->get_route()->position_bei(test_index-1).get_2d(), cnv->get_route()->position_bei(test_index+1).get_2d());
				const uint8 nextnext_90fahrtrichtung = this->calc_richtung(cnv->get_route()->position_bei(test_index).get_2d(), cnv->get_route()->position_bei(test_index+1).get_2d());
				dt = no_cars_blocking( test, cnv, next_fahrtrichtung, nextnext_fahrtrichtung, nextnext_90fahrtrichtung );
				if(dt) {
					// take care of warning messages
					break;
				}
				if(!test->ist_uebergang()) {
					// ok, left the crossing; check is successful
					return true;
				}
				test_index ++;
			}
		}

		// stuck message ...
		if(dt) {
			if(  dt->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehikel is stuck too
				restart_speed = 0;
				cnv->set_tiles_overtaking(0);
				cnv->reset_waiting();
			}
			else {
				// we might be able to overtake this one ...
				overtaker_t *over = dt->get_overtaker();
				if(over  &&  !over->is_overtaken()) {
					if(  over->is_overtaking()  ) {
						// otherwise we would stop every time being overtaken
						return true;
					}
					// not overtaking/being overtake: we need to make a more thourough test!
					if(  dt->get_typ()==ding_t::automobil  ) {
						convoi_t *ocnv = static_cast<automobil_t *>(dt)->get_convoi();
						if(  cnv->can_overtake( ocnv, ocnv->get_min_top_speed(), ocnv->get_length()*16, diagonal_length)  ) {
							return true;
						}
					}
					else if(  dt->get_typ()==ding_t::verkehr  ) {
						stadtauto_t *caut = static_cast<stadtauto_t *>(dt);
						if(  cnv->can_overtake(caut, caut->get_besch()->get_geschw(), 256, diagonal_length)  ) {
							return true;
						}
					}
				}
				// we have to wait ...
				restart_speed = 0;
				cnv->set_tiles_overtaking(0);
			}
		}

		return dt==NULL;
	}

	return true;
}



void automobil_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	const int cargo = get_fracht_menge();
	weg_t *str = welt->lookup( get_pos() )->get_weg(road_wt);
	str->book(cargo, WAY_STAT_GOODS);
	if (ist_erstes)  {
		str->book(1, WAY_STAT_CONVOIS);
		cnv->update_tiles_overtaking();
	}
}




schedule_t * automobil_t::erzeuge_neuen_fahrplan() const
{
  return new autofahrplan_t();
}



void automobil_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("automobil_t::set_convoi()","%p",c);
	if(c!=NULL) {
		bool target=(bool)cnv;	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
		vehikel_t::set_convoi(c);
		if(target  &&  ist_erstes  &&  c->get_route()->empty()) {
			// reintitialize the target halt
			route_t *rt=cnv->get_route();
			grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n()));
			target_halt = target->get_halt();
			if(target_halt.is_bound()) {
				target_halt->reserve_position(target,cnv->self);
			}
		}
	}
	else {
		if(cnv  &&  ist_erstes  &&  target_halt.is_bound()) {
			target_halt->unreserve_position(welt->lookup(cnv->get_route()->position_bei(cnv->get_route()->get_max_n())),cnv->self);
			target_halt = halthandle_t();
		}
		cnv = NULL;
	}
}



/* from now on rail vehicles (and other vehicles using blocks) */

waggon_t::waggon_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	vehikel_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehicle
		if(besch==NULL) {
			int power = (is_first || fracht.empty() || fracht.front() == warenbauer_t::nichts) ? 500 : 0;
			const ware_besch_t* w = fracht.empty() ? warenbauer_t::nichts : fracht.front().get_besch();
			dbg->warning("waggon_t::waggon_t()","try to find a fitting vehicle for %s.", power>0 ? "engine": w->get_name() );
			if(last_besch!=NULL  &&  last_besch->can_follow(last_besch)  &&  last_besch->get_ware()==w  &&  (!is_last  ||  last_besch->get_nachfolger(0)==NULL)) {
				// same as previously ...
				besch = last_besch;
			}
			else {
				// we have to search
				besch = vehikelbauer_t::get_best_matching(get_waytype(), 0, w!=warenbauer_t::nichts?5000:0, power, speed_to_kmh(speed_limit), w, false, false, last_besch, is_last );
			}
			if(besch) {
DBG_MESSAGE("waggon_t::waggon_t()","replaced by %s",besch->get_name());
				calc_bild();
			}
			else {
				dbg->error("waggon_t::waggon_t()","no matching besch found for %s!",w->get_name());
			}
			if (!fracht.empty() && fracht.front().menge == 0) {
				// this was only there to find a matchin vehicle
				fracht.remove_first();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}


waggon_t::waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
    cnv = cn;
}


waggon_t::~waggon_t()
{
	if(cnv  &&  ist_erstes  &&  !cnv->get_route()->empty()  &&  route_index<=cnv->get_route()->get_max_n()) {
		// free all reserved blocks
		block_reserver( cnv->get_route(), cnv->get_vehikel(cnv->get_vehikel_anzahl()-1)->get_route_index(), target_halt.is_bound()?100000:1, false );
	}
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		schiene_t * sch = (schiene_t *)gr->get_weg(get_waytype());
		if(sch) {
			sch->unreserve(this);
		}
	}
}



void
waggon_t::set_convoi(convoi_t *c)
{
	if(c!=cnv) {
		DBG_MESSAGE("waggon_t::set_convoi()","new=%p old=%p",c,cnv);
		if(ist_erstes) {
			if(cnv!=NULL  &&  cnv!=(convoi_t *)1) {
				// free route from old convoi
				if(!cnv->get_route()->empty()  &&  route_index+1u<cnv->get_route()->get_max_n()) {
					block_reserver( cnv->get_route(), cnv->get_vehikel(cnv->get_vehikel_anzahl()-1)->get_route_index(), 100000, false );
					target_halt = halthandle_t();
				}
			}
			else {
				assert(c!=NULL);
				// eventually reserve new route
				if(  c->get_state()==convoi_t::DRIVING  || c->get_state()==convoi_t::LEAVING_DEPOT  ) {
					if(route_index>c->get_route()->get_max_n()) {
						c->suche_neue_route();
						dbg->warning("waggon_t::set_convoi()", "convoi %i had a too high route index! (%i of max %i)", c->self.get_id(), route_index, c->get_route()->get_max_n() );
					}
					else {
						long num_index = cnv==(convoi_t *)1 ? 1001 : 0; 	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
						// rereserve next block, if needed
						cnv = c;
						uint16 n = block_reserver( c->get_route(), route_index, num_index, true );
						if(n) {
							c->set_next_stop_index( n );
						}
						else {
							c->warten_bis_weg_frei(-1);
						}
					}
				}
				if(c->get_state()>=convoi_t::WAITING_FOR_CLEARANCE) {
//	DBG_MESSAGE("waggon_t::set_convoi()","new route %p, route_index %i",c->get_route(),route_index);
					// find about next signal after loading
					uint16 next_signal_index=65535;	
					route_t *route=c->get_route();

					if(route->empty()  ||  get_pos()==route->position_bei(route->get_max_n())) {
						// we are there, were we should go? Usually this is an error during autosave
						c->suche_neue_route();
					}
					else {
						for(  uint16 i=max(route_index,1)-1;  i<=route->get_max_n();  i++) {
							schiene_t * sch = (schiene_t *) welt->lookup(route->position_bei(i))->get_weg(get_waytype());
							if(sch==NULL) {
								break;
							}
							if(sch->has_signal()) {
								next_signal_index = i+1;
								break;
							}
							if(sch->is_crossing()) {
								next_signal_index = i+1;
								break;
							}
						}
						c->set_next_stop_index( next_signal_index );
					}
				}
			}
		}
		vehikel_t::set_convoi(c);
	}
}


// need to reset halt reservation (if there was one)
bool waggon_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
	if(ist_erstes  &&  route_index<cnv->get_route()->get_max_n()+1) {
		// free all reserved blocks
		block_reserver( cnv->get_route(), cnv->get_vehikel(cnv->get_vehikel_anzahl()-1)->get_route_index(), target_halt.is_bound()?100000:1, false );
	}
	target_halt = halthandle_t();	// no block reserved
	return route->calc_route(welt, start, ziel, this, max_speed );
}



bool
waggon_t::ist_befahrbar(const grund_t *bd) const
{
	const schiene_t * sch = dynamic_cast<const schiene_t *> (bd->get_weg(get_waytype()));

	// Hajo: diesel and steam engines can use electrifed track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric);
	bool ok = (sch!=0)  &&  (needs_no_electric  ||  sch->is_electrified());

	if(!ok  ||  !target_halt.is_bound()  ||  !cnv->is_waiting() || !check_way_constraints(sch)) {
		return ok;
	}
	else {
		// we are searching a stop here:
		// ok, we can go where we already are ...
		if(bd->get_pos()==get_pos()) {
			return true;
		}
		// we cannot pass an end of choose area
		if(sch->has_sign()) {
			const roadsign_t* rs = bd->find<roadsign_t>();
			if(rs->get_besch()->get_wtyp()==get_waytype()  &&  rs->get_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA) {
				return false;
			}
		}
		// but we can only use empty blocks ...
		// now check, if we could enter here
		return sch->can_reserve(cnv->self);
	}
}



// how expensive to go here (for way search)
// author prissi
int
waggon_t::get_kosten(const grund_t *gr,const uint32 max_speed) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(get_waytype());
	uint32 max_tile_speed = w ? w->get_max_speed() : 999;
	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	// effect of hang ...
	if(gr->get_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 25;
	}

	//@author: jamespetts
	// Strongly prefer routes for which the vehicle is not overweight.
	uint16 weight_limit = w->get_max_weight();
	if(vehikel_t::get_gesamtgewicht() > weight_limit)
	{
		costs += 40;
	}

	return costs;
}



signal_t *
waggon_t::ist_blockwechsel(koord3d k2) const
{
	const schiene_t * sch1 = (const schiene_t *) welt->lookup( k2 )->get_weg(get_waytype());
	if(sch1  &&  sch1->has_signal()) {
		// a signal for us
		return welt->lookup(k2)->find<signal_t>();
	}
	return NULL;
}



// this routine is called by find_route, to determined if we reached a destination
bool
waggon_t::ist_ziel(const grund_t *gr,const grund_t *prev_gr) const
{
	const schiene_t * sch1 = (const schiene_t *) gr->get_weg(get_waytype());
	// first check blocks, if we can go there
	if(sch1->can_reserve(cnv->self)) {
		//  just check, if we reached a free stop position of this halt
		if(gr->is_halt()  &&  gr->get_halt()==target_halt) {
			// now we must check the precessor ...
			if(prev_gr!=NULL) {
				const koord dir=gr->get_pos().get_2d()-prev_gr->get_pos().get_2d();
				grund_t *to;
				if(!gr->get_neighbour(to,get_waytype(),dir)  ||  !(to->get_halt()==target_halt)) {
					return true;
				}
			}
		}
	}
	return false;
}


bool
waggon_t::ist_weg_frei(int & restart_speed)
{
	if(ist_erstes  &&  (cnv->get_state()==convoi_t::CAN_START  ||  cnv->get_state()==convoi_t::CAN_START_ONE_MONTH  ||  cnv->get_state()==convoi_t::CAN_START_TWO_MONTHS)) {
		// reserve first block at the start until the next signal
		weg_t *w = welt->lookup( get_pos() )->get_weg(get_waytype());
		if(  w->has_signal()  ) {
			// we have to use the signal routine below
			cnv->set_next_stop_index( max(route_index,1) );
		}
		else {
			// free track => reserve up to next signal
			uint16 n=block_reserver(cnv->get_route(), max(route_index,1)-1, 0, true);
			if(n==0) {
	//DBG_MESSAGE("cannot","start at index %i",route_index);
				restart_speed = 0;
				return false;
			}
			cnv->set_next_stop_index(n);
			return true;
		}
	}

	const grund_t *gr = welt->lookup(pos_next);
	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	weg_t *w = gr->get_weg(get_waytype());
	if(w==NULL) {
		return false;
	}

	uint16 next_block=cnv->get_next_stop_index()-1;
	if(next_block<=route_index+3) {
		route_t *rt=cnv->get_route();
		koord3d block_pos=rt->position_bei(next_block);
		signal_t *sig = ist_blockwechsel(block_pos);
		if(sig) {
			// action depend on the next signal
			uint16 next_stop = 0;
			const roadsign_besch_t *sig_besch=sig->get_besch();

			// if signal is single line then calculate route to next signal and check it is clear
			if(sig_besch->is_longblock_signal()) {
				if(!cnv->is_waiting()) {
					// do not stop before the signal ...
					if(route_index<=next_block) {
						restart_speed = -1;
						return true;
					}
					restart_speed = -1;
					return false;
				}

				bool exit_loop = false;
				short fahrplan_index = cnv->get_schedule()->get_aktuell();
				int count = 0;
				route_t target_rt;
				koord3d cur_pos = rt->position_bei(next_block+1);
				// next tile is end of schedule => must start with next leg of schedule
				if(count==0  &&  next_block+1u>=rt->get_max_n()) {
					fahrplan_index ++;
					if(fahrplan_index >= cnv->get_schedule()->get_count()) {
						fahrplan_index = 0;
					}
					count++;
				}
				// now search
				do {
					// search for route
					if(!target_rt.calc_route( welt, cur_pos, cnv->get_schedule()->eintrag[fahrplan_index].pos, this, speed_to_kmh(cnv->get_min_top_speed()))) {
						exit_loop = true;
					}
					else {
						// check tiles of route until we find signal or reserved track
						for(  uint i = count==0 ? next_block+1u : 0u; i<=target_rt.get_max_n(); i++) {
							koord3d pos = target_rt.position_bei(i);
							grund_t *gr = welt->lookup(pos);
							schiene_t * sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;
							if(  sch1  &&  sch1->has_signal()  ) {
								next_stop = block_reserver(cnv->get_route(),next_block+1u,0,true);
								if(next_stop > 0) {
									// we should be able to always get reservation to next station - since we've already checked
									// that tiles are clear. No harm in sanity check though...
									restart_speed = -1;
									cnv->set_next_stop_index(next_stop);
									sig->set_zustand(roadsign_t::gruen);
									return true;
								}
								exit_loop = true;
								break;
							} else if(sch1  &&  !sch1->can_reserve(cnv->self)) {
								exit_loop = true;
								break;
							}
						}
					}

					if(!exit_loop) {
						target_rt.clear();
						cur_pos = cnv->get_schedule()->eintrag[fahrplan_index].pos;
						fahrplan_index ++;
						if(fahrplan_index >= cnv->get_schedule()->get_count()) {
							fahrplan_index = 0;
						}
						count++;
					}

				} while(count < cnv->get_schedule()->get_count()  &&  exit_loop == false); // stop after we've looped round schedule...
				// we can't go
				sig->set_zustand(roadsign_t::rot);
				if(route_index==next_block+1) {
					restart_speed = 0;
					return false;
				}
				restart_speed = -1;
				return true;
			}
			else if(sig_besch->is_free_route()) {
				grund_t *target=NULL;

				// choose signal here
				target=welt->lookup(rt->position_bei(rt->get_max_n()));

				if(target) {
					target_halt = target->get_halt();
				}
			}
			next_stop = block_reserver(cnv->get_route(),next_block+1,(target_halt.is_bound()?100000:sig_besch->is_pre_signal()),true);

			if(next_stop==0  &&  target_halt.is_bound()  &&  sig_besch->is_free_route()) {

				// no free route to target!
				// note: any old reservations should be invalid after the block reserver call.
				//           We can now start freshly all over

				// if we fail, we will wait in a step, much more simulation friendly
				// thus we ensure correct convoi state!
				if(!cnv->is_waiting()) {
					// do not stop before the signal ...
					if(route_index<=next_block) {
						restart_speed = -1;
						return true;
					}
					restart_speed = -1;
					return false;
				}

				// now it make sense to search a route
				route_t target_rt;
				const int richtung = ribi_typ(get_pos().get_2d(),pos_next.get_2d());	// to avoid confusion at diagonals
#ifdef MAX_CHOOSE_BLOCK_TILES
				if(!target_rt.find_route( welt, rt->position_bei(next_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, MAX_CHOOSE_BLOCK_TILES )) {
#else
				if(!target_rt.find_route( welt, rt->position_bei(next_block), this, speed_to_kmh(cnv->get_min_top_speed()), richtung, welt->get_groesse_x()+welt->get_groesse_y() )) {
#endif
					// nothing empty or not route with less than MAX_CHOOSE_BLOCK_TILES tiles
					target_halt = halthandle_t();
				}
				else {
					// try to alloc the whole route
					rt->remove_koord_from(next_block);
					rt->append( &target_rt );
					next_stop = block_reserver(rt,next_block,100000,true);
				}
				// reserved route to target (or not)
			}
			// next signal can be passed
			if(next_stop!=0) {
				restart_speed = -1;
				cnv->set_next_stop_index(next_stop);
				sig->set_zustand(roadsign_t::gruen);
			}
			else {
				// cannot be passed
				sig->set_zustand(roadsign_t::rot);
				if(route_index==next_block+1) {
//DBG_MESSAGE("cannot","continue");
					restart_speed = 0;
					return false;
				}
			}
		}
		else {
			// end of route?
			if(  next_block+1u >= cnv->get_route()->get_max_n()  &&  route_index == next_block+1u  ) {
				// we can always continue, if there would be a route ...
				return true;
			}
			// Is a crossing?
			crossing_t* cr = welt->lookup(block_pos)->find<crossing_t>(2);
			if(cr) {
				// ok, here is a draw/turnbridge ...
				bool ok = cr->request_crossing(this);
				if(!ok) {
					// cannot pass, will brake ...
					if(route_index==next_block) {
						restart_speed = 0;
						return false;
					}
					restart_speed = -1;
					return true;
				}
				//  drive on ...
			}
			// not a signal (anymore) but we will still stop anyway
			uint16 next_stop = block_reserver(cnv->get_route(),next_block+1,target_halt.is_bound()?100000:0,true);
			if(next_stop!=0) {
				// can pass the non-existing signal ..
				restart_speed = -1;
				cnv->set_next_stop_index(next_stop);
			}
			else if(route_index==next_block+1) {
				// no free route
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}



/*
 * reserves or unreserves all blocks and returns the handle to the next block (if there)
 * if count is larger than 1, (and defined) maximum MAX_CHOOSE_BLOCK_TILES tiles will be checked
 * (freeing or reserving a choose signal path)
 * return the last checked block
 * @author prissi
 */
uint16
waggon_t::block_reserver(const route_t *route, uint16 start_index, int count, bool reserve) const
{
	bool success=true;
#ifdef MAX_CHOOSE_BLOCK_TILES
	int max_tiles=2*MAX_CHOOSE_BLOCK_TILES; // max tiles to check for choosesignals
#endif
	slist_tpl<grund_t *> signs;	// switch all signals on their way too ...

	if(start_index>route->get_max_n()) {
		return 0;
	}

	if(route->position_bei(start_index)==get_pos()  &&  reserve) {
		start_index++;
	}

	// find next blocksegment enroute
	uint16 i=start_index;
	uint16 next_signal_index=65535, skip_index=65535;
	uint16 next_crossing_index=65535;
	bool unreserve_now = false;
	for ( ; success  &&  count>=0  &&  i<=route->get_max_n(); i++) {

		koord3d pos = route->position_bei(i);
		grund_t *gr = welt->lookup(pos);
		schiene_t * sch1 = gr ? (schiene_t *)gr->get_weg(get_waytype()) : NULL;
		if(sch1==NULL  &&  reserve) {
			// reserve until the end of track
			break;
		}
		// we unreserve also nonexisting tiles! (may happen during deletion)

#ifdef MAX_CHOOSE_BLOCK_TILES
		max_tiles--;
		if(max_tiles<0  &&   count>1) {
			break;
		}
#endif
		if(reserve) {
			if(sch1->has_signal()) {
				if(count) {
					signs.append(gr);
				}
				count --;
				next_signal_index = i;
			}
			if(!sch1->reserve(cnv->self)) {
				success = false;
			}
			if(next_crossing_index==65535  &&  sch1->is_crossing()) {
				next_crossing_index = i;
			}
		}
		else if(sch1) {
			if(!sch1->unreserve(cnv->self)) {
				if(unreserve_now) {
					// reached an reserved or free track => finished
					return 65535;
				}
			}
			else {
				// unreserve from here (only used during sale, since there might be reserved tiles not freed)
				unreserve_now = true;
			}
			if(sch1->has_signal()) {
				signal_t* signal = gr->find<signal_t>();
				if(signal) {
					signal->set_zustand(roadsign_t::rot);
				}
			}
			if(sch1->is_crossing()) {
				gr->find<crossing_t>()->release_crossing(this);
			}
		}
	}

	if(!reserve) {
		return 65535;
	}
	// here we go only with reserve

//DBG_MESSAGE("block_reserver()","signals at %i, sucess=%d",next_signal_index,success);

	// free, in case of unreserve or no sucess in reservation
	if(!success) {
		// free reservation
		for ( int j=start_index; j<i; j++) {
			if(i!=skip_index) {
				schiene_t * sch1 = (schiene_t *)welt->lookup( route->position_bei(j))->get_weg(get_waytype());
				sch1->unreserve(cnv->self);
			}
		}
		return 0;
	}

	// ok, switch everything green ...
	slist_iterator_tpl<grund_t *> iter(signs);
	while(iter.next()) {
		signal_t* signal = iter.get_current()->find<signal_t>();
		if(signal) {
			signal->set_zustand(roadsign_t::gruen);
		}
	}

	// if next stop is further away then next crossing, return next crossing
	if(next_signal_index>next_crossing_index) {
		next_signal_index = next_crossing_index;
	}
	// stop at station or signals, not at waypoints
	if(next_signal_index==65535) {
		// find out if stop or waypoint, waypoint: do not brake at waypoints
		grund_t *gr=welt->lookup(route->position_bei(route->get_max_n()));
		return (gr  &&  gr->is_halt()) ? route->get_max_n()+1 : 65535;
	}
	return next_signal_index+1;
}



/* beware: we must unreserve railblocks ... */
void
waggon_t::verlasse_feld()
{
	vehikel_t::verlasse_feld();
	// fix counters
	if(ist_letztes) {
		grund_t *gr = welt->lookup( get_pos() );
		if(gr) {
			schiene_t *sch0 = (schiene_t *) gr->get_weg(get_waytype());
			if(sch0) {
				sch0->unreserve(this);
				// tell next signal?
				// and swith to red
				if(sch0->has_signal()) {
					signal_t* sig = welt->lookup(get_pos())->find<signal_t>();
					if(sig) {
						sig->set_zustand(roadsign_t::rot);
					}
				}
			}
		}
	}
}



void
waggon_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	schiene_t * sch0 = (schiene_t *) welt->lookup(get_pos())->get_weg(get_waytype());
	if(sch0) {
		// way statistics
		const int cargo = get_fracht_menge();
		sch0->book(cargo, WAY_STAT_GOODS);
		if(ist_erstes) {
			sch0->book(1, WAY_STAT_CONVOIS);
			sch0->reserve(cnv->self);
		}
	}
}



schedule_t * waggon_t::erzeuge_neuen_fahrplan() const
{
	return besch->get_waytype()==tram_wt ? new tramfahrplan_t() : new zugfahrplan_t();
}


schedule_t * monorail_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new monorailfahrplan_t();
}


schedule_t * maglev_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new maglevfahrplan_t();
}


schedule_t * narrowgauge_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new narrowgaugefahrplan_t();
}


schiff_t::schiff_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
}

schiff_t::schiff_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	vehikel_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			dbg->warning("schiff_t::schiff_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passagiere");
			besch = vehikelbauer_t::get_best_matching(water_wt, 0, fracht.empty() ? 0 : 30, 100, 40, !fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere, false, true, last_besch, is_last );
			if(besch) {
				calc_bild();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}



bool
schiff_t::ist_befahrbar(const grund_t *bd) const
{
	if(  bd->ist_wasser()  ) {
		return true;
	}
	const weg_t *w = bd->get_weg(water_wt);
	return (w  &&  w->get_max_speed()>0);
}



/* Since slopes are handled different for ships
 * @author prissi
 */
void
schiff_t::calc_akt_speed(const grund_t *gr)
{
	// or a hill?
	if(gr->get_weg_hang()) {
		// hill up or down => in lock => deccelarte
		current_friction = 16;
	}
	else {
		// flat track
		current_friction = 1;
	}

	if(alte_fahrtrichtung != fahrtrichtung) {
		// curve: higher friction
		current_friction *= 2;
	}

	if(ist_erstes) {
		// just to accelerate: The actual speed takes care of all vehicles in the convoi
		cnv->set_akt_speed_soll( speed_limit );
	}
}



bool
schiff_t::ist_weg_frei(int &restart_speed)
{
	restart_speed = -1;

	if(ist_erstes) {
		grund_t *gr = welt->lookup( pos_next );

		weg_t *w = gr->get_weg(water_wt);
		if(w  &&  w->is_crossing()) {
			// ok, here is a draw/turnbridge ...
			crossing_t* cr = gr->find<crossing_t>();
			if(!cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}



schedule_t * schiff_t::erzeuge_neuen_fahrplan() const
{
  return new schifffahrplan_t();
}



/**** from here on planes ***/


// this routine is called by find_route, to determined if we reached a destination
bool aircraft_t::ist_ziel(const grund_t *gr,const grund_t *) const
{
	if(state!=looking_for_parking) {

		// search for the end of the runway
		const weg_t *w=gr->get_weg(air_wt);
		if(w  &&  w->get_besch()->get_styp()==1) {

//DBG_MESSAGE("aircraft_t::ist_ziel()","testing at %i,%i",gr->get_pos().x,gr->get_pos().y);
			// ok here is a runway
			ribi_t::ribi ribi= w->get_ribi_unmasked();
//DBG_MESSAGE("aircraft_t::ist_ziel()","ribi=%i target_ribi=%i",ribi,approach_dir);
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&approach_dir)!=0) {
				// pointing in our direction
				// here we should check for length, but we assume everything is ok
//DBG_MESSAGE("aircraft_t::ist_ziel()","success at %i,%i",gr->get_pos().x,gr->get_pos().y);
				return true;
			}
		}
	}
	else {
		// otherwise we just check, if we reached a free stop position of this halt
		if(gr->get_halt()==target_halt  &&  target_halt->is_reservable(gr,cnv->self)) {
			return true;
		}
	}
//DBG_MESSAGE("is_target()","failed at %i,%i",gr->get_pos().x,gr->get_pos().y);
	return false;
}



// for flying thingies, everywhere is good ...
// another function only called during route searching
ribi_t::ribi
aircraft_t::get_ribi(const grund_t *gr) const
{
	switch(state) {
		case taxiing:
		case looking_for_parking:
			return gr->get_weg_ribi(air_wt);

		case taxiing_to_halt:
		{
			// we must invert all one way signs here, since we start from the target position here!
			weg_t *w = gr->get_weg(air_wt);
			if(w) {
				ribi_t::ribi r = w->get_ribi_unmasked();
				ribi_t::ribi mask = w->get_ribi_maske();
				return (mask) ? (r & ~ribi_t::rueckwaerts(mask)) : r;
			}
			return ribi_t::keine;
		}

		case landing:
		case departing:
		{
			ribi_t::ribi dir = gr->get_weg_ribi(air_wt);
			if(dir==0) {
				return ribi_t::alle;
			}
			return dir;
		}

		case flying:
		case flying2:
			return ribi_t::alle;
	}
	return ribi_t::keine;
}



// how expensive to go here (for way search)
// author prissi
int
aircraft_t::get_kosten(const grund_t *gr,const uint32 ) const
{
	// first favor faster ways
	const weg_t *w=gr->get_weg(air_wt);
	int costs = 0;

	if(state==flying) {
		if(w==NULL) {
			costs += 1;
		}
		else {
			if(w->get_besch()->get_styp()==0) {
				costs += 25;
			}
		}
	}
	else {
		// only, if not flying ...
		assert(w);

		if(w->get_besch()->get_styp()==0) {
			costs += 3;
		}
		else {
			costs += 2;
		}
	}

	return costs;
}



// whether the ground is drivable or not depends on the current state of the airplane
bool
aircraft_t::ist_befahrbar(const grund_t *bd) const
{
	switch (state) {
		case taxiing:
		case taxiing_to_halt:
		case looking_for_parking:
//DBG_MESSAGE("ist_befahrbar()","at %i,%i",bd->get_pos().x,bd->get_pos().y);
			return bd->hat_weg(air_wt);

		case landing:
		case departing:
		case flying:
		case flying2:
		{
//DBG_MESSAGE("aircraft_t::ist_befahrbar()","(cnv %i) in idx %i",cnv->self.get_id(),route_index );
			// prissi: here a height check could avoid too height montains
			return true;
		}
	}
	return false;
}



/* finds a free stop, calculates a route and reserve the position
 * else return false
 */
bool
aircraft_t::find_route_to_stop_position()
{
	if(target_halt.is_bound()) {
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","bound! (cnv %i)",cnv->self.get_id());
		return true;	// already searched with success
	}

	// check for skipping circle
	route_t *rt=cnv->get_route();
	grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n()));

//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","can approach? (cnv %i)",cnv->self.get_id());

	target_halt = target ? target->get_halt() : halthandle_t();
	if(!target_halt.is_bound()) {
		return true;	// no halt to search
	}

	// then: check if the search point is still on a runway (otherwise just proceed)
	target = welt->lookup(rt->position_bei(suchen));
	if(target==NULL  ||  !target->hat_weg(air_wt)) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no runway found at %i,%i,%i",rt->position_bei(suchen).x,rt->position_bei(suchen).y,rt->position_bei(suchen).z);
		return true;	// no runway any more ...
	}

	// is our target occupied?
//	DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","state %i",state);
	if(!target_halt->find_free_position(air_wt,cnv->self,ding_t::aircraft)  ) {
		target_halt = halthandle_t();
		DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","no free prosition found!");
		return false;
	}
	else {
		// calculate route to free position:

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not reentrant!
		if(!cnv->is_waiting()) {
			target_halt = halthandle_t();
			return false;
		}

		// now search a route
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","some free: find route from index %i",suchen);
		route_t target_rt;
		flight_state prev_state = state;
		state = looking_for_parking;
		if(!target_rt.find_route( welt, rt->position_bei(suchen), this, 500, ribi_t::alle, 100 )) {
DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","found no route to free one");
			// circle slowly another round ...
			target_halt = halthandle_t();
			state = prev_state;
			return false;
		}
		state = prev_state;

		// now reserve our choice ...
		target_halt->reserve_position(welt->lookup(target_rt.position_bei(target_rt.get_max_n())),cnv->self);
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.get_max_n()).x,target_rt.position_bei(target_rt.get_max_n()).y, target_rt.position_bei(target_rt.get_max_n()).z );
		rt->remove_koord_from(suchen);
		rt->append( &target_rt );
		return true;
	}
}


/* reserves runways (reserve true) or removes the reservation
 * finishes when reaching end tile or leaving the ground (end of runway)
 * @return true if the reservation is successfull
 */
bool aircraft_t::block_reserver( uint32 start, uint32 end, bool reserve )
{
	bool start_now = false;
	bool success = true;
	uint32 i;

	route_t *route = cnv->get_route();
	if(route->empty()) {
		return false;
	}

	for(  uint32 i=start;  success  &&  i<end  &&  i<=route->get_max_n();  i++) {

		grund_t *gr = welt->lookup(route->position_bei(i));
		runway_t * sch1 = gr ? (runway_t *)gr->get_weg(air_wt) : NULL;
		if(sch1==NULL) {
			if(reserve) {
				if(!start_now) {
					// touched down here
					start = i;
				}
				else {
					// most likely left the ground here ...
					end = i;
					break;
				}
			}
		}
		else {
			// we unreserve also nonexisting tiles! (may happen during deletion)
			if(reserve) {
				start_now = true;
				if(!sch1->reserve(cnv->self)) {
					// unsuccessful => must unreserve all
					success = false;
					end = i;
					break;
				}
				// end of runway?
				if(i>start  &&  ribi_t::ist_einfach(sch1->get_ribi_unmasked())  ) {
					return true;
				}
			}
			else if(!sch1->unreserve(cnv->self)) {
				if(start_now) {
					// reached an reserved or free track => finished
					return true;
				}
			}
			else {
				// unreserve from here (only used during sale, since there might be reserved tiles not freed)
				start_now = true;
			}
		}
	}

	// unreserve if not successful
	if(!success  &&  reserve) {
		for(  i=start;  i<end;  i++  ) {
			runway_t * sch1 = (runway_t *)welt->lookup(route->position_bei(i))->get_weg(air_wt);
			if(sch1) {
				sch1->unreserve(cnv->self);
			}
		}
	}
	return success;
}



// handles all the decisions on the ground an in the air
bool aircraft_t::ist_weg_frei(int & restart_speed)
{
	restart_speed = -1;

	grund_t *gr = welt->lookup( pos_next );
	if(gr==NULL) {
		cnv->suche_neue_route();
		return false;
	}

	if(gr->get_top()>250) {
		// too many objects here
		return false;
	}

	if(route_index<takeoff  &&  route_index>1  &&  takeoff<cnv->get_route()->get_max_n()) {
		if(route_index>1  &&  gr->suche_obj(ding_t::aircraft)) {
			// check, if tile occupied, if not on stop
			restart_speed = 0;
			return false;
		}
		// need to reserve runway?
		runway_t *rw = (runway_t *)gr->get_weg(air_wt);
		if(rw==NULL) {
			cnv->suche_neue_route();
			return false;
		}
		// next tile a runway => then reserve
		if(rw->get_besch()->get_styp()==1) {
			// try to reserve the runway
			if(!block_reserver(takeoff,takeoff+100,true)) {
				// runway already blocked ...
				restart_speed = 0;
				return false;
			}
		}
		return true;
	}

	if(route_index==takeoff  &&  state==taxiing) {
		// try to reserve the runway if not already done
		if(route_index==2  &&  !block_reserver(takeoff,takeoff+100,true)) {
			// runway blocked, wait at start of runway
			restart_speed = 0;
			return false;
		}
		// stop shortly at the end of the runway
		state = departing;
		restart_speed = 0;
		return false;
	}

//DBG_MESSAGE("aircraft_t::ist_weg_frei()","index %i<>%i",route_index,touchdown);

	// check for another circle ...
	if(route_index==(touchdown-3)) {
		if(state!=flying2  &&  !block_reserver( touchdown-1, suchen, true )) {
			// circle slowly next round
			cnv->set_akt_speed_soll( kmh_to_speed(besch->get_geschw())/2 );
			state = flying;
			route_index -= 16;
		}
		return true;
	}

	if(route_index==touchdown-16-3  &&  state!=flying2) {
		// just check, if the end of runway ist free; we will wait there
		if(block_reserver( touchdown-1, suchen, true )) {
			route_index += 16;
			state = flying2;
		}
		return true;
	}

	if(route_index==suchen  &&  state==landing  &&  !target_halt.is_bound()) {

		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not reentrant!
		if(!cnv->is_waiting()) {
			return false;
		}

		// nothing free here?
		if(find_route_to_stop_position()) {
			// stop reservation successful
			route_t *rt=cnv->get_route();
			pos_next==rt->position_bei(route_index);
			block_reserver( touchdown-1, suchen+1, false );
			state = taxiing;
			return true;
		}
		restart_speed = 0;
		return false;
	}

	if(state==looking_for_parking) {
		state = taxiing;
	}

	if(state == taxiing  &&  gr->is_halt()  &&  gr->find<aircraft_t>()) {
		// the next step is a parking position. We do not enter, if occupied!
		restart_speed = 0;
		return false;
	}

	return true;
}



// this must also change the internal modes for the calculation
void
aircraft_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	if((state==flying2  ||  state==flying)  &&  route_index+6u>=touchdown) {
		const sint16 landehoehe = height_scaling(cnv->get_route()->position_bei(touchdown).z)+(touchdown-route_index);
		const sint16 current_height = height_scaling(get_pos().z) + (flughoehe*Z_TILE_STEP)/TILE_HEIGHT_STEP;
		if(landehoehe<=current_height) {
			state = landing;
			target_height = height_scaling((sint16)cnv->get_route()->position_bei(touchdown).z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
		}
	}
	else {
		runway_t *w=(runway_t *)welt->lookup(get_pos())->get_weg(air_wt);
		if(w) {
			const int cargo = get_fracht_menge();
			w->book(cargo, WAY_STAT_GOODS);
			if (ist_erstes) {
				w->book(1, WAY_STAT_CONVOIS);
			}
		}
	}
}



aircraft_t::aircraft_t(karte_t *welt, loadsave_t *file, bool is_first, bool is_last) : vehikel_t(welt)
{
	rdwr_from_convoi(file);
	old_x = old_y = -1;
	old_bild = IMG_LEER;

	if(  file->is_loading()  ) {
		static const vehikel_besch_t *last_besch = NULL;

		if(is_first) {
			last_besch = NULL;
		}
		// try to find a matching vehivle
		if(besch==NULL) {
			dbg->warning("aircraft_t::aircraft_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passagiere");
			besch = vehikelbauer_t::get_best_matching(air_wt, 0, 101, 1000, 800, !fracht.empty() ? fracht.front().get_besch() : warenbauer_t::passagiere, false, true, last_besch, is_last );
			if(besch) {
				calc_bild();
			}
		}
		// update last besch
		if(  besch  ) {
			last_besch = besch;
		}
	}
}


aircraft_t::aircraft_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
	state = taxiing;
	old_x = old_y = -1;
	old_bild = IMG_LEER;
}



aircraft_t::~aircraft_t()
{
	// mark aircraft (after_image) dirty, since we have no "real" image
	mark_image_dirty( bild, -flughoehe-hoff-2 );
	mark_image_dirty( bild, 0 );
}



void
aircraft_t::set_convoi(convoi_t *c)
{
	DBG_MESSAGE("aircraft_t::set_convoi()","%p",c);
	if(ist_erstes  &&  (unsigned long)cnv > 1) {
		// free stop reservation
		if(target_halt.is_bound()) {
			target_halt->unreserve_position(welt->lookup(cnv->get_route()->position_bei(cnv->get_route()->get_max_n())),cnv->self);
			target_halt = halthandle_t();
		}
		if(!cnv->get_route()->empty()) {
			// free runway reservation
			if(route_index>=takeoff  &&  route_index<touchdown-4  &&  state!=flying) {
				block_reserver( takeoff, takeoff+100, false );
			}
			else if(route_index>=touchdown-1  &&  state!=taxiing) {
				block_reserver( touchdown-1, suchen, false );
			}
		}
	}
	// maybe need to restore state?
	if(c!=NULL) {
		bool target=(bool)cnv;
		vehikel_t::set_convoi(c);
		if(ist_erstes) {
			if(target) {
				// reintitialize the target halt
				route_t *rt=cnv->get_route();
				grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n()));
				target_halt = target->get_halt();
				if(target_halt.is_bound()) {
					target_halt->reserve_position(target,cnv->self);
				}
			}
			// restore reservation
			if(route_index>=takeoff  &&  route_index<touchdown-21  &&  state!=flying) {
				block_reserver( takeoff, takeoff+100, true );
			}
			else if(route_index>=touchdown-1  &&  state!=taxiing) {
				block_reserver( touchdown-1, suchen, true );
			}
		}
	}
	else {
		vehikel_t::set_convoi(NULL);
	}
}




schedule_t * aircraft_t::erzeuge_neuen_fahrplan() const
{
	return new airfahrplan_t();
}



void aircraft_t::rdwr_from_convoi(loadsave_t *file)
{
	xml_tag_t t( file, "aircraft_t" );

	vehikel_t::rdwr_from_convoi(file);

	file->rdwr_enum(state, " ");
	file->rdwr_short(flughoehe, " ");
	flughoehe &= ~(TILE_HEIGHT_STEP-1);
	file->rdwr_short(target_height, "\n");
	file->rdwr_long(suchen," ");
	file->rdwr_long(touchdown," ");
	file->rdwr_long(takeoff,"\n");
}



#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two diffrent directions for the runway search
uint8
aircraft_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_typ( (koord)((ziel-start).get_2d()) );	// reverse
	// make sure, there are at last two directions to choose, or you might en up with not route
	if(ribi_t::ist_einfach(dir)) {
		dir |= (dir<<1);
		if(dir>16) {
			dir += 1;
		}
	}
	return dir&0x0F;
}
#endif


// main routine: searches the new route in up to three steps
// must also take care of stops under traveling and the like
bool aircraft_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
//DBG_MESSAGE("aircraft_t::calc_route()","search route from %i,%i,%i to %i,%i,%i",start.x,start.y,start.z,ziel.x,ziel.y,ziel.z);

	if(ist_erstes  &&  cnv) {
		// free target reservation
		if(  target_halt.is_bound() ) {
			route_t *rt=cnv->get_route();
			grund_t *target=welt->lookup(rt->position_bei(rt->get_max_n()));
			if(target) {
				target_halt->unreserve_position(target,cnv->self);
			}
		}
		// free runway reservation
		if(route_index>=takeoff  &&  route_index<touchdown-21  &&  state!=flying) {
			block_reserver( takeoff, takeoff+100, false );
		}
		else if(route_index>=touchdown-1  &&  state!=taxiing) {
			block_reserver( touchdown-1, suchen, false );
		}
	}
	target_halt = halthandle_t();	// no block reserved

	const weg_t *w=welt->lookup(start)->get_weg(air_wt);
	bool start_in_the_air = (w==NULL);
	bool end_in_air=false;

	suchen = takeoff = touchdown = 0x7ffffffful;
	if(!start_in_the_air) {

		// see, if we find a direct route within 150 boxes: We are finished
		state = taxiing;
		if(route->calc_route( welt, start, ziel, this, max_speed, 600 )) {
			// ok, we can taxi to our location
			return true;
		}
	}

	if(start_in_the_air  ||  (w->get_besch()->get_styp()==1  &&  ribi_t::ist_einfach(w->get_ribi())) ) {
		// we start here, if we are in the air or at the end of a runway
		search_start = start;
		start_in_the_air = true;
		route->clear();
//DBG_MESSAGE("aircraft_t::calc_route()","start in air at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}
	else {
		// not found and we are not on the takeoff tile (where the route search will fail too) => we try to calculate a complete route, starting with the way to the runway

		// second: find start runway end
		state = taxiing;
#ifdef USE_DIFFERENT_WIND
		approach_dir = get_approach_ribi( ziel, start );	// reverse
//DBG_MESSAGE("aircraft_t::calc_route()","search runway start near %i,%i,%i with corner in %x",start.x,start.y,start.z, approach_dir);
#else
		approach_dir = ribi_t::nordost;	// reverse
		DBG_MESSAGE("aircraft_t::calc_route()","search runway start near %i,%i,%i",start.x,start.y,start.z);
#endif
		if(!route->find_route( welt, start, this, max_speed, ribi_t::alle, 100 )) {
			DBG_MESSAGE("aircraft_t::calc_route()","failed");
			return false;
		}
		// save the route
		search_start = route->position_bei( route->get_max_n() );
//DBG_MESSAGE("aircraft_t::calc_route()","start at ground at %i,%i,%i",search_start.x,search_start.y,search_start.z);
	}

	// second: find target runway end
	state = taxiing_to_halt;	// only used for search
#ifdef USE_DIFFERENT_WIND
	approach_dir = get_approach_ribi( start, ziel );	// reverse
//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z,approach_dir);
#else
	approach_dir = ribi_t::suedwest;	// reverse
//DBG_MESSAGE("aircraft_t::calc_route()","search runway target near %i,%i,%i in corners %x",ziel.x,ziel.y,ziel.z);
#endif
	route_t end_route;

	if(!end_route.find_route( welt, ziel, this, max_speed, ribi_t::alle, 100 )) {
		// well, probably this is a waypoint
		end_in_air = true;
		search_end = ziel;
	}
	else {
		// save target route
		search_end = end_route.position_bei( end_route.get_max_n() );
	}
//DBG_MESSAGE("aircraft_t::calc_route()","ziel now %i,%i,%i",search_end.x,search_end.y,search_end.z);

	// create target route
	if(!start_in_the_air) {
		takeoff = route->get_max_n();
		koord start_dir(welt->lookup(search_start)->get_weg_ribi(air_wt));
		if(start_dir!=koord(0,0)) {
			// add the start
			ribi_t::ribi start_ribi = ribi_t::rueckwaerts(ribi_typ(start_dir));
			const grund_t *gr=NULL;
			// add the start
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->ist_in_kartengrenzen(search_start.get_2d()+(start_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_start.get_2d()+(start_dir*endi));
				if(over<3  ||  (gr->get_weg_ribi(air_wt)&start_ribi)==0) {
					over --;
				}
				endi ++;
				route->append(gr->get_pos());
			} while(  over>0  );
			// out of map
			if(gr==NULL) {
				dbg->error("aircraft_t::calc_route()","out of map!");
				return false;
			}
			// need some extra step to avoid 180° turns
			if( start_dir.x!=0  &&  sgn(start_dir.x)==sgn(search_end.x-search_start.x)  ) {
				route->append( welt->lookup(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->get_kartenboden()->get_pos() );
				route->append( welt->lookup(gr->get_pos().get_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->get_kartenboden()->get_pos() );
			}
			else if( start_dir.y!=0  &&  sgn(start_dir.y)==sgn(search_end.y-search_start.y)  ) {
				route->append( welt->lookup(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 1 : -1 ,0) )->get_kartenboden()->get_pos() );
				route->append( welt->lookup(gr->get_pos().get_2d()+koord((search_end.x>search_start.x) ? 2 : -2 ,0) )->get_kartenboden()->get_pos() );
			}
		}
		else {
			// init with startpos
			dbg->error("aircraft_t::calc_route()","Invalid route calculation: start is on a single direction field ...");
		}
		state = taxiing;
		flughoehe = 0;
		target_height = ((sint16)get_pos().z*TILE_HEIGHT_STEP)/Z_TILE_STEP;
	}
	else {
		// init with current pos (in air ... )
		route->clear();
		route->append( get_pos() );
		state = flying;
		cnv->set_akt_speed_soll( vehikel_t::SPEED_UNLIMITED );
		if(flughoehe==0) {
			flughoehe = 3*TILE_HEIGHT_STEP;
		}
		takeoff = 0;
		target_height = (((sint16)get_pos().z+3)*TILE_HEIGHT_STEP)/Z_TILE_STEP;
	}

//DBG_MESSAGE("aircraft_t::calc_route()","take off ok");

	koord3d landing_start=search_end;
	if(!end_in_air) {
		// now find way to start of landing pos
		ribi_t::ribi end_ribi = welt->lookup(search_end)->get_weg_ribi(air_wt);
		koord end_dir(end_ribi);
		end_ribi = ribi_t::rueckwaerts(end_ribi);
		if(end_dir!=koord(0,0)) {
			// add the start
			const grund_t *gr;
			int endi = 1;
			int over = 3;
			// now add all runway + 3 ...
			do {
				if(!welt->ist_in_kartengrenzen(search_end.get_2d()+(end_dir*endi)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_end.get_2d()+(end_dir*endi));
				if(over<3  ||  (gr->get_weg_ribi(air_wt)&end_ribi)==0) {
					over --;
				}
				endi ++;
				landing_start = gr->get_pos();
			} while(  over>0  );
		}
	}
	else {
		suchen = touchdown = 0x7FFFFFFFul;
	}

	// just some straight routes ...
	if(!route->append_straight_route(welt,landing_start)) {
		// should never fail ...
		dbg->error( "aircraft_t::calc_route()", "No straight route found!" );
		return false;
	}

	if(!end_in_air) {

		// find starting direction
		int offset = 0;
		switch(welt->lookup(search_end)->get_weg_ribi(air_wt)) {
			case ribi_t::nord: offset = 0; break;
			case ribi_t::west: offset = 4; break;
			case ribi_t::sued: offset = 8; break;
			case ribi_t::ost: offset = 12; break;
		}

		// now make a curve
		koord circlepos=landing_start.get_2d();
		static const koord circle_koord[16]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		route->append( welt->lookup_kartenboden(circlepos)->get_pos() );
		for(  int  i=0;  i<16;  i++  ) {
			circlepos += circle_koord[(offset+i+16)%16];
			if(welt->ist_in_kartengrenzen(circlepos)) {
				route->append( welt->lookup_kartenboden(circlepos)->get_pos() );
			}
			else {
				// could only happen during loading old savegames;
				// in new versions it should not possible to build a runway here
				route->clear();
				dbg->error("aircraft_t::calc_route()","airport too close to the edge! (Cannot go to %i,%i!)",circlepos.x,circlepos.y);
				return false;
			}
		}

		touchdown = route->get_max_n()+3;
		route->append_straight_route(welt,search_end);

		// now the route rearch point (+1, since it will check before entering the tile ...)
		suchen = route->get_max_n();

		// now we just append the rest
		for( int i=end_route.get_max_n()-1;  i>=0;  i--  ) {
			route->append(end_route.position_bei(i));
		}
	}

//DBG_MESSAGE("aircraft_t::calc_route()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route->get_max_n(), state );
	return true;
}



void
aircraft_t::hop()
{
	if(!get_flag(ding_t::dirty)) {
		mark_image_dirty( bild, get_yoff()-flughoehe-hoff-2 );
	}

	uint32 new_speed_limit = SPEED_UNLIMITED;
	sint32 new_friction = 0;

	// take care of inflight height ...
	const sint16 h_cur = height_scaling((sint16)get_pos().z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
	const sint16 h_next = height_scaling((sint16)pos_next.z)*TILE_HEIGHT_STEP/Z_TILE_STEP;

	switch(state) {
		case departing:
			{
				flughoehe = 0;
				target_height = h_cur;
				new_friction = max( 0, 25-route_index-takeoff )*4;

				// take off, when a) end of runway or b) last tile of runway or c) fast enough
				weg_t *weg=welt->lookup(get_pos())->get_weg(air_wt);
				if(
					(weg==NULL  ||  // end of runway (broken runway)
					 weg->get_besch()->get_styp()!=1  ||  // end of runway (gras now ... )
					 (route_index>takeoff+1  &&  ribi_t::ist_einfach(weg->get_ribi_unmasked())) )  ||  // single ribi at end of runway
					 cnv->get_akt_speed()>kmh_to_speed(besch->get_geschw())/3 // fast enough
				){
					state = flying;
					new_friction = 16;
					block_reserver( takeoff, takeoff+100, false );
					target_height = h_cur+TILE_HEIGHT_STEP*3;
				}
			}
			break;

		case flying2:
			new_speed_limit = kmh_to_speed(besch->get_geschw())/2;
			new_friction = 64;
		case flying:
			// since we are at a tile border, round up to the nearest value
			flughoehe += h_cur;
			if(flughoehe<target_height) {
				flughoehe = (flughoehe+TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
			}
			else if(flughoehe>target_height) {
				flughoehe = (flughoehe-TILE_HEIGHT_STEP);
			}
			flughoehe -= h_next;
			// did we have to change our flight height?
			if(target_height-h_next > TILE_HEIGHT_STEP*5) {
				// sinken
				target_height -= TILE_HEIGHT_STEP*2;
			}
			else if(target_height-h_next < TILE_HEIGHT_STEP*2) {
				// steigen
				target_height += TILE_HEIGHT_STEP*2;
			}
			break;

		case landing:
			flughoehe += h_cur;
			if(flughoehe>target_height) {
				// still decenting
				flughoehe = (flughoehe-TILE_HEIGHT_STEP) & ~(TILE_HEIGHT_STEP-1);
				flughoehe -= h_next;
				new_speed_limit = kmh_to_speed(besch->get_geschw())/2;
				new_friction = 64;
			}
			else {
				// touchdown!
				flughoehe = 0;
				target_height = h_next;
				// all planes taxi with same speed
				new_speed_limit = kmh_to_speed(60);
				new_friction = 16;
			}
			break;

		default:
			new_speed_limit = kmh_to_speed(60);
			new_friction = 16;
			flughoehe = 0;
			target_height = h_next;
			break;
	}

	// hop to next tile
	vehikel_t::hop();

	// and change flight height
	cnv->set_akt_speed_soll( new_speed_limit );
	current_friction = new_friction;
}



// this routine will display the shadow
void
aircraft_t::display_after(int xpos_org, int ypos_org, bool /*reset_dirty*/) const
{
	if(bild != IMG_LEER) {
		int xpos = xpos_org, ypos = ypos_org;

		const int raster_width = get_tile_raster_width();
		sint16 current_flughohe = flughoehe;
		const sint16 target = target_height - ((sint16)get_pos().z*TILE_HEIGHT_STEP)/Z_TILE_STEP;
		if(  current_flughohe < target  ) {
			current_flughohe += (steps*TILE_HEIGHT_STEP) >> 8;
		}
		else if(  current_flughohe > target  ) {
			current_flughohe -= (steps*TILE_HEIGHT_STEP) >> 8;
		}

		ypos += tile_raster_scale_y(get_yoff()-current_flughohe-hoff-2, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		get_screen_offset( xpos, ypos );

		// will be dirty
		display_color_img(bild, xpos, ypos, get_player_nr(), true, true/*get_flag(ding_t::dirty)*/ );

		vehikel_t::display_after( xpos_org, ypos_org-tile_raster_scale_y(current_flughohe-hoff-2, raster_width), true );
	}
}
