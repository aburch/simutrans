/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Fahrzeuge in der Welt von Simutrans
 *
 * 01.11.99  getrennt von simdings.cc
 *
 * Hansjoerg Malthaner, Nov. 1999
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "simvehikel.h"

#include "boden/grund.h"
#include "boden/wege/runway.h"
#include "boden/wege/kanal.h"
#include "boden/wege/schiene.h"
#include "boden/wege/monorail.h"
#include "boden/wege/strasse.h"

#include "bauer/warenbauer.h"

#include "simworld.h"
#include "simdebug.h"

#include "simplay.h"
#include "simware.h"
#include "simhalt.h"
#include "simconvoi.h"
#include "simsound.h"

#include "simimg.h"
#include "simcolor.h"
#include "simgraph.h"
#include "simio.h"
#include "simmem.h"

#include "simline.h"

#include "simintr.h"

#include "dings/wolke.h"
#include "dings/signal.h"
#include "dings/roadsign.h"
#include "dings/crossing.h"

#include "gui/karte.h"

#include "besch/ware_besch.h"
#include "besch/skin_besch.h"
#include "besch/roadsign_besch.h"

#include "dataobj/fahrplan.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "utils/simstring.h"
#include "utils/cbuffer_t.h"


#include "bauer/vehikelbauer.h"


static const uint8 offset_array[8] = {
//dir_sued, dir_west, dir_suedwest, dir_suedost, dir_nord, dir_ost, dir_nordost, dir_nordwest
0, 0, 0, 0, 1, 1, 1, 1
};


vehikel_basis_t::vehikel_basis_t(karte_t *welt):
    ding_t(welt)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
}



vehikel_basis_t::vehikel_basis_t(karte_t *welt, koord3d pos):
    ding_t(welt, pos)
{
	bild = IMG_LEER;
	set_flag( ding_t::is_vehicle );
	pos_next = pos;
}



void
vehikel_basis_t::verlasse_feld()
{
	// first: release crossing
	grund_t *gr = welt->lookup(gib_pos());
	if(gr  &&  gr->ist_uebergang()) {
		gr->find<crossing_t>(2)->release_crossing(this);
	}

	// then remove from ground (or search whole map, if failed)
	if (gr==NULL  ||  !gr->obj_remove(this)) {
		// was not removed (not found?)

		dbg->error("vehikel_basis_t::verlasse_feld()","'typ %i' %p could not be removed from %d %d", gib_typ(), this, gib_pos().x, gib_pos().y);
		DBG_MESSAGE("vehikel_basis_t::verlasse_feld()","checking all plan squares");

		gr = welt->lookup( gib_pos().gib_2d() )->gib_boden_von_obj(this);
		if(gr) {
			gr->obj_remove(this);
			dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",gib_typ(), this, gib_pos().x, gib_pos().y);
		}

		koord k;
		bool ok = false;

		for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
				grund_t *gr = welt->lookup( k )->gib_boden_von_obj(this);
				if(gr && gr->obj_remove(this)) {
					dbg->warning("vehikel_basis_t::verlasse_feld()","removed vehicle typ %i (%p) from %d %d",gib_name(), this, k.x, k.y);
					ok = true;
				}
			}
		}

		if(!ok) {
			dbg->error("vehikel_basis_t::verlasse_feld()","'%s' %p was not found on any map sqaure!",gib_name(), this);
		}
	}
}


void vehikel_basis_t::betrete_feld()
{
	grund_t *gr=welt->lookup(gib_pos());
	if(!gr) {
		dbg->error("vehikel_basis_t::betrete_feld()","'%s' new position (%i,%i,%i)!",gib_name(), gib_pos().x, gib_pos().y, gib_pos().z );
		gr = welt->lookup_kartenboden(gib_pos().gib_2d());
		setze_pos( gr->gib_pos() );
	}
	gr->obj_add(this);
}


/**
 * Checks if this vehicle must change the square upon next move
 * @author Hj. Malthaner
 */
inline bool is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff )
{
    const sint8 y_off_2 = 2*neu_yoff;
    const sint8 c_plus  = y_off_2 + neu_xoff;
    const sint8 c_minus = y_off_2 - neu_xoff;

    return ! (c_plus < 32 && c_minus < 32 && c_plus > -32 && c_minus > -32);
}


void vehikel_basis_t::fahre()
{
	const sint8 neu_xoff = gib_xoff() + gib_dx();
	const sint8 neu_yoff = gib_yoff() + gib_dy();

	// want to go to next field and want to step
	if(is_about_to_hop(neu_xoff,neu_yoff)) {

//		assert(abs(neu_xoff)==16  &&  abs(neu_yoff)==8);

		if( !hop_check() ) {
			// red signal etc ...
			return;
		}

		if(!get_flag(ding_t::dirty)) {
			// old position needs redraw
			mark_image_dirty(gib_bild(),hoff);
		}

		hop();

		// may ended on a slope ...
		use_calc_height = true;
		hoff = 0;

		setze_xoff( (neu_xoff < 0) ? 16 : -16 );
		setze_yoff( (neu_yoff < 0) ? 8 : -8 );
	}
	else {
		// driving on the same tile

		if(!get_flag(ding_t::dirty)) {
			// old position needs redraw
			mark_image_dirty(gib_bild(),hoff);
		}

		setze_xoff( neu_xoff );
		setze_yoff( neu_yoff );
	}
	set_flag(ding_t::dirty);
}



ribi_t::ribi
vehikel_basis_t::calc_richtung(koord start, koord ende, sint8 &dx, sint8 &dy) const
{
	ribi_t::ribi richtung = ribi_t::keine;

	const sint8 di = ende.x - start.x;
	const sint8 dj = ende.y - start.y;

	if(dj < 0 && di == 0) {
		richtung = ribi_t::nord;
		dx = 2;
		dy = -1;
	} else if(dj > 0 && di == 0) {
		richtung = ribi_t::sued;
		dx = -2;
		dy = 1;
	} else if(di < 0 && dj == 0) {
		richtung = ribi_t::west;
		dx = -2;
		dy = -1;
	} else if(di >0 && dj == 0) {
		richtung = ribi_t::ost;
		dx = 2;
		dy = 1;
	} else if(di > 0 && dj > 0) {
		richtung = ribi_t::suedost;
		dx = 0;
		dy = 2;
	} else if(di < 0 && dj < 0) {
		richtung = ribi_t::nordwest;
		dx = 0;
		dy = -2;
	} else if(di > 0 && dj < 0) {
		richtung = ribi_t::nordost;
		dx = 4;
		dy = 0;
	} else {
		richtung = ribi_t::suedwest;
		dx = -4;
		dy = 0;
	}
	return richtung;
}



ribi_t::ribi
vehikel_basis_t::calc_richtung(koord start, koord ende) const
{
	ribi_t::ribi richtung = ribi_t::keine;

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



// this routine calculates the new height
// beware of bridges, tunnels, slopes, ...
int
vehikel_basis_t::calc_height()
{
	int hoff = 0;
	use_calc_height = false;	// assume, we are only needed after next hop

	grund_t *gr = welt->lookup(gib_pos());
	if(gr->ist_tunnel()) {
		hoff = 0;
		if(!gr->ist_im_tunnel()) {
			use_calc_height = true;
			// need hiding?
			switch(gr->gib_grund_hang()) {
			case 3:	// nordhang
				if(vehikel_basis_t::gib_yoff()>-7) {
					setze_bild(IMG_LEER);
				}
				else {
					calc_bild();
				}
				break;
			case 6:	// westhang
				if(vehikel_basis_t::gib_xoff()>-12) {
					setze_bild(IMG_LEER);
				}
				else {
					calc_bild();
				}
				break;
			case 9:	// osthang
				if(vehikel_basis_t::gib_xoff()<6) {
					setze_bild(IMG_LEER);
				}
				else {
					calc_bild();
				}
				break;
			case 12:    // suedhang
				if(vehikel_basis_t::gib_yoff()<7) {
					setze_bild(IMG_LEER);
				}
				else {
					calc_bild();
				}
				break;
			}
		}
	}
	else {
		switch(gr->gib_weg_hang()) {
			case 3:	// nordhang
			case 6:	// westhang
				hoff = -vehikel_basis_t::gib_yoff() - 8;
				use_calc_height = true;
				break;
			case 9:	// osthang
			case 12:    // suedhang
				hoff = vehikel_basis_t::gib_yoff() - 8;
				use_calc_height = true;
				break;
			case 0:
				hoff = -gr->gib_weg_yoff();
				break;
		}
	}

	// recalculate friction
	hoff = height_scaling(hoff);

	return hoff;
}



void
vehikel_t::setze_convoi(convoi_t *c)
{
	/* cnv can have three values:
	 * NULL: not previously assigned
	 * 1 (only during loading): convoi wants to reserve the whole route
	 * other: previous convoi (in this case, currently always c==cnv)
	 *
	 * if c is NULL, then the vehicle is removed from the convoi
	 * (the waggon_t::setze_convoi etc. routines must then remove a
	 *  possibly pending reservation of stops/tracks)
	 */
	assert(  c==NULL  ||  cnv==NULL  ||  cnv==(convoi_t *)1  ||  c==cnv);
	cnv = c;
	if(cnv) {
		// we need to reestablish the finish flag after loading
		if(ist_erstes) {
			check_for_finish = (route_index>cnv->get_route()->gib_max_n())  ||  (gib_pos()==cnv->get_route()->position_bei(route_index));
		}
		// some convois were saved with broken coordinates
		if(!welt->lookup(pos_prev)) {
			pos_prev = welt->lookup_kartenboden(pos_prev.gib_2d())->gib_pos();
		}
		if(pos_next!=koord3d::invalid  &&  route_index<cnv->get_route()->gib_max_n()  &&  (welt->lookup(pos_next)==NULL  ||  welt->lookup(pos_next)->gib_weg(gib_waytype())==NULL)) {
			pos_next = cnv->get_route()->position_bei(route_index+1);
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
	if(halt->is_enabled( gib_fracht_typ() )) {
		if (!fracht.empty()) {

			slist_iterator_tpl<ware_t> iter (fracht);
			while(iter.next()) {
				const ware_t& tmp = iter.get_current();

				halthandle_t end_halt = tmp.gib_ziel();
				halthandle_t via_halt = tmp.gib_zwischenziel();

				// probleme mit fehlerhafter ware
				// vielleicht wurde zwischendurch die
				// Zielhaltestelle entfernt ?
				if(!end_halt.is_bound() || !via_halt.is_bound()) {
					DBG_MESSAGE("vehikel_t::entladen()", "destination of %d %s is no longer reachable",tmp.menge,translator::translate(tmp.gib_name()));
					kill_queue.insert(tmp);
				} else if(end_halt==halt || via_halt==halt) {

					//		    printf("Liefere %d %s nach %s via %s an %s\n",
					//                           tmp->menge,
					//			   tmp->name(),
					//			   end_halt->gib_name(),
					//			   via_halt->gib_name(),
					//			   halt->gib_name());

					// hier sollte nur ordentliche ware verabeitet werden
					int menge = halt->liefere_an(tmp);
					sum_menge += menge;

					kill_queue.insert(tmp);

					INT_CHECK("simvehikel 937");
				}
			}
		}
	}

	slist_iterator_tpl<ware_t> iter (kill_queue);
	while( iter.next() ) {
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
bool vehikel_t::load_freight(halthandle_t halt)
{
	const bool ok = halt->gibt_ab(besch->gib_ware());
	fahrplan_t *fpl = cnv->gib_fahrplan();
	if( ok ) {

		while(total_freight < besch->gib_zuladung()) {
			const uint16 hinein = besch->gib_zuladung() - total_freight;

			ware_t ware = halt->hole_ab(besch->gib_ware(), hinein, fpl);
			if(ware.menge==0) {
				// now empty, but usually, we can get it here ...
				return ok;
			}

			slist_iterator_tpl<ware_t> iter (fracht);

			// could this be joined with existing freight?
			while(iter.next()) {
				ware_t &tmp = iter.access_current();

				// for pax: join according next stop
				// for all others we *must* use target coordinates
				if(ware.same_destination(tmp)) {
					tmp.menge += ware.menge;
					total_freight += ware.menge;
					ware.menge = 0;
					break;
				}
			}

			// if != 0 we could not joi it to existing => load it
			if(ware.menge != 0) {
				fracht.insert(ware);
				total_freight += ware.menge;
			}

			INT_CHECK("simvehikel 876");
		}
	}
	return ok;
}



void
vehikel_t::setze_offsets(int x, int y)
{
	setze_xoff( x );
	setze_yoff( y );
}


/**
 * Remove freight that no longer can reach it's destination
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
			fahrplan_t *fpl = cnv->gib_fahrplan();

			const ware_t& tmp = iter.get_current();
			bool found = false;

			for (int i = 0; i < fpl->maxi(); i++) {
				if (haltestelle_t::gib_halt(welt, fpl->eintrag[i].pos.gib_2d()) == tmp.gib_zwischenziel()) {
					found = true;
					break;
				}
			}

			if (!found) {
				kill_queue.insert(tmp);
			}
			else {
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
	if(besch->gib_sound() >= 0) {
		struct sound_info info;
		info.index = besch->gib_sound();
		info.volume = 255;
		info.pri = 0;
		welt->play_sound_area_clipped(gib_pos().gib_2d(), info);
	}
}


/**
 * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
 * der Convoi eine neue Route ermittelt
 * @author Hj. Malthaner
 */
void vehikel_t::neue_fahrt(uint16 start_route_index, bool recalc)
{
	route_index = start_route_index+1;
	check_for_finish = false;
	use_calc_height = true;

	if(welt->ist_in_kartengrenzen(gib_pos().gib_2d())) {
		// mark the region after the image as dirty
		// better not try to twist your brain to follow the retransformation ...
		mark_image_dirty( gib_bild(), hoff );
	}

	if(!recalc) {
		// always set pos_next
		// otherwise a convoi with somehow a wrong pos_next will next continue
		pos_next = cnv->get_route()->position_bei(route_index);
		assert( gib_pos() == cnv->get_route()->position_bei(start_route_index) );
	}
	else {

		// recalc directions
		pos_next = cnv->get_route()->position_bei(route_index);
		pos_prev = gib_pos();
		setze_pos( cnv->get_route()->position_bei(start_route_index) );

		alte_fahrtrichtung = fahrtrichtung;
		fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
		hoff = 0;

		const sint8 li = -16;
		const sint8 re = 16;
		const sint8 ob = -8;
		const sint8 un = 8;

		sint8 yoff;
		if (dy < 0) {
			yoff = un;
		}
		else {
			yoff = ob;
		}

		sint8 xoff;
		if (dx < 0) {
			xoff = re;
		}
		else {
			xoff = li;
		}

		setze_xoff( xoff );
		setze_yoff( yoff );

	  calc_bild();
	}
}



vehikel_t::vehikel_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp) :
	vehikel_basis_t(sp->get_welt(), pos)
{
	this->besch = besch;

	setze_besitzer( sp );
	insta_zeit = welt->get_current_month();
	cnv = NULL;
	speed_limit = SPEED_UNLIMITED;

	route_index = 1;

	rauchen = true;
	fahrtrichtung = ribi_t::keine;

	current_friction = 4;
	total_freight = 0;
	sum_weight = besch->gib_gewicht();

	ist_erstes = ist_letztes = false;
	check_for_finish = false;
	use_calc_height = true;
	alte_fahrtrichtung = fahrtrichtung = ribi_t::keine;
	target_halt = halthandle_t();
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
}


bool
vehikel_t::hop_check()
{
	// the leading vehicle will do all the checks
	if(ist_erstes) {
		// now check, if we can go here
		const grund_t *bd = welt->lookup(pos_next);
		if(bd==NULL  ||  !ist_befahrbar(bd)) {
			// weg not existent (likely destroyed)
			cnv->suche_neue_route();
			return false;
		}
		// check for oneway sign etc.
		if(air_wt!=gib_waytype()  &&  route_index<cnv->get_route()->gib_max_n()) {
			uint8 dir=gib_ribi(bd);
			koord3d nextnext_pos=cnv->get_route()->position_bei(route_index+1);
			uint8 new_dir = ribi_typ(nextnext_pos.gib_2d()-pos_next.gib_2d());
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
	return true;//route_index<=cnv->get_route()->gib_max_n();
}



void
vehikel_t::verlasse_feld()
{
	vehikel_basis_t::verlasse_feld();
#ifndef DEBUG_ROUTES
	if(ist_letztes  &&  reliefkarte_t::is_visible) {
			reliefkarte_t::gib_karte()->calc_map_pixel(gib_pos().gib_2d());
	}
#endif
}




/* this routine add a vehicle to a tile and will insert it in the correct sort order to prevent overlaps
 * @author prissi
 */
void
vehikel_t::betrete_feld()
{
	if(ist_erstes  &&  reliefkarte_t::is_visible) {
		reliefkarte_t::gib_karte()->setze_relief_farbe(gib_pos().gib_2d(), VEHIKEL_KENN);
	}

	vehikel_basis_t::betrete_feld();
}


void
vehikel_t::hop()
{
	// Fahrtkosten
	cnv->add_running_cost(-besch->gib_betriebskosten());

	verlasse_feld();

	pos_prev = gib_pos();
	setze_pos( pos_next );  // naechstes Feld
	if(route_index<cnv->get_route()->gib_max_n()) {
		route_index ++;
		pos_next = cnv->get_route()->position_bei(route_index);
	}
	else {
		check_for_finish = true;
	}
	alte_fahrtrichtung = fahrtrichtung;

	// this is a required hack for aircrafts! Aircrafts can turn on a single square, and this confuses the previous calculation!
	// author: hsiegeln
	if (pos_prev.gib_2d()==pos_next.gib_2d()) {
		fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
DBG_MESSAGE("vehikel_t::hop()","reverse dir at route index %d",route_index);
	}
	else {
		if(pos_next!=gib_pos()) {
			fahrtrichtung = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d(), dx, dy);
		}
		else if(  (  check_for_finish  &&  welt->lookup(pos_next)  &&  ribi_t::ist_gerade(welt->lookup(pos_next)->gib_weg_ribi_unmasked(gib_waytype()))  )  ||  welt->lookup(pos_next)->is_halt()) {
			// allow diagonal stops at waypoints on diagonal tracks but avoid them on halts and at straight tracks...
			fahrtrichtung = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d(), dx, dy);
		}
	}
	calc_bild();

	betrete_feld();
	grund_t *gr = welt->lookup(gib_pos());
	const weg_t * weg = gr->gib_weg(gib_waytype());
	if(weg) {
		speed_limit = kmh_to_speed( weg->gib_max_speed() );
		if(weg->is_crossing()) {
			gr->find<crossing_t>(2)->add_to_crossing(this);
		}
	}
	else {
		speed_limit = SPEED_UNLIMITED;
	}

	calc_akt_speed(gr);
}



/* calculates the current friction coefficient based on the curent track
 * falt, slope, curve ...
 * @author prissi, HJ
 */
void
vehikel_t::calc_akt_speed(const grund_t *gr) //,const int h_alt, const int h_neu)
{
	// assume straigh flat track
	current_friction = 1;

	// or a hill?
	const hang_t::typ hang = gr->gib_weg_hang();
	if(hang!=hang_t::flach) {
		if(ribi_typ(hang)==fahrtrichtung) {
			// hill up, since height offsets are negative: heavy deccelerate
			current_friction = 24;
		}
		else {
			// hill down: accelrate
			current_friction = -12;
		}
	}

	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) {
		current_friction = 8;
	}

	if(ist_erstes) {
		sint32 akt_speed = speed_limit;

		// break at the end of the route
		uint32 brake_speed_soll = akt_speed;
		switch(cnv->get_next_stop_index()+1-route_index) {
			case 3: brake_speed_soll = kmh_to_speed(200); break;
			case 2: brake_speed_soll = kmh_to_speed(100); break;
			case 1: brake_speed_soll = kmh_to_speed(50); break;
			case 0: brake_speed_soll = kmh_to_speed(25); break;
			default: break;
		}
		if(brake_speed_soll<akt_speed) {
			akt_speed = brake_speed_soll;
		}
		// speed is limited anyway in the convoi
		cnv->setze_akt_speed_soll( akt_speed );
	}
}



void
vehikel_t::rauche()
{
	// raucht ueberhaupt ?
	if(rauchen && besch->gib_rauch()) {

		bool smoke = besch->get_engine_type()==vehikel_besch_t::steam;

		if(!smoke) {
			// Hajo: only produce smoke when heavily accelerating
			//       or steam engine
			int akt_speed = kmh_to_speed(besch->gib_geschw());
			if(akt_speed > speed_limit) {
				akt_speed = speed_limit;
			}

			smoke = (cnv->gib_akt_speed() < (akt_speed*7)>>3);
		}

		if(smoke) {
			grund_t * gr = welt->lookup( gib_pos() );
			// nicht im tunnel ?
			if(gr && !gr->ist_im_tunnel() ) {
				wolke_t *abgas =  new wolke_t(welt, gib_pos(), gib_xoff(), gib_yoff(), besch->gib_rauch()->gib_bild_nr(0), true );

				if( !gr->obj_add(abgas) ) {
					delete abgas;
				} else {
					welt->sync_add( abgas );
				}
			}
		}
	}
}



/* the only difference to the normal driving routine
 * that is used also by citycars without a route is the check for end
 * this is marked by repeating the same koord3d twice
 */
void
vehikel_t::fahre()
{
	// target mark: same coordinate twice (stems from very old ages, I think)
	if(ist_erstes  &&  check_for_finish) {
		// check a vehicle leanght ahead for a tile change
		// for south/east going vehicles, we must add half a tile
		// this is also the correct value for diagonals
		const sint8 iterations = (fahrtrichtung==ribi_t::nord  || fahrtrichtung==ribi_t::west) ? 8 : 1;

		const sint8 neu_xoff = gib_xoff() + gib_dx()*iterations;
		const sint8 neu_yoff = gib_yoff() + gib_dy()*iterations;

		// want to go to next field and want to step
		if(is_about_to_hop(neu_xoff,neu_yoff)) {
			// so we are there yet?
			cnv->ziel_erreicht();
			return;
		}
	}

	vehikel_basis_t::fahre();
}



/**
 * Payment is done per hop. It iterates all goods and calculates
 * the income for the last hop. This method must be called upon
 * every stop.
 * @return income total for last hop
 * @author Hj. Malthaner
 */
sint64 vehikel_t::calc_gewinn(koord3d start, koord3d end) const
{
	const long dist = abs(end.x - start.x) + abs(end.y - start.y);

	const sint32 ref_speed = welt->get_average_speed( gib_waytype() );
	const sint32 speed_base = (100*speed_to_kmh(cnv->gib_min_top_speed()))/ref_speed-100;

	sint64 value = 0;
	slist_iterator_tpl <ware_t> iter (fracht);

	while( iter.next() ) {
		const ware_t & ware = iter.get_current();

		const sint32 grundwert128 = ware.gib_typ()->gib_preis()<<7;
		const sint32 grundwert_bonus = (ware.gib_typ()->gib_preis()*(1000+speed_base*ware.gib_typ()->gib_speed_bonus()));
		const sint64 price = (sint64)(grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus) * (sint64)dist * (sint64)ware.menge;

		// sum up new price
		value += price;
	}

  // Hajo: Rounded value, in cents
  // prissi: Why on earth 1/3???
  return (value+1500ll)/3000ll;
}


const char *vehikel_t::gib_fracht_mass() const
{
    return gib_fracht_typ()->gib_mass();
}


/**
 * Berechnet Gesamtgewicht der transportierten Fracht in KG
 * @author Hj. Malthaner
 */
uint32 vehikel_t::gib_fracht_gewicht() const
{
	uint32 weight = 0;

	slist_iterator_tpl<ware_t> iter(fracht);
	while(iter.next()) {
		weight +=
			iter.get_current().menge *
			iter.get_current().gib_typ()->gib_weight_per_unit();
	}
	return weight;
}


const char * vehikel_t::gib_fracht_name() const
{
    return gib_fracht_typ()->gib_name();
}


void vehikel_t::gib_fracht_info(cbuffer_t & buf)
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

			halthandle_t halt = ware.gib_ziel();
			if(halt.is_bound()) {
				name = halt->gib_name();
			}

			buf.append("   ");
			buf.append(ware.menge);
			buf.append(translator::translate(ware.gib_mass()));
			buf.append(" ");
			buf.append(translator::translate(ware.gib_name()));
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
vehikel_t::beladen(koord , halthandle_t halt)
{
	const bool ok= load_freight(halt);
	sum_weight =  (gib_fracht_gewicht()+499)/1000 + besch->gib_gewicht();

	calc_bild();
	return ok;
}


/**
 * fahrzeug an haltestelle entladen
 * @author Hj. Malthaner
 */
bool vehikel_t::entladen(koord, halthandle_t halt)
{
	// printf("Vehikel %p entladen\n", this);
	uint16 menge = unload_freight(halt);
	if(menge>0) {
		// add delivered goods to statistics
		cnv->book(menge, CONVOI_TRANSPORTED_GOODS);
		// add delivered goods to halt's statistics
		halt->book(menge, HALT_ARRIVED);
		return true;
	}
	return false;
}


void vehikel_t::sync_step()
{
	setze_yoff( gib_yoff() - hoff );
	fahre();
	if(use_calc_height) {
		hoff = calc_height();
	}
	setze_yoff( gib_yoff() + hoff );
}



/**
 * Ermittelt fahrtrichtung
 * @author Hj. Malthaner
 */
ribi_t::ribi
vehikel_t::richtung()
{
  ribi_t::ribi neu = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d());
  if(neu == ribi_t::keine) {
    // sonst ausrichtung des Vehikels beibehalten
    return fahrtrichtung;
  } else {
    return neu;
  }
}


void
vehikel_t::calc_bild()
{
	image_id old_bild=gib_bild();
	if (fracht.empty()) {
		setze_bild(besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()),NULL));
	} else {
		setze_bild(besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()), fracht.front().gib_typ()));
	}
	if(old_bild!=gib_bild()) {
		set_flag(ding_t::dirty);
	}
}


void
vehikel_t::rdwr(loadsave_t *file)
{
	int fracht_count = 0;

	if(file->is_saving()) {
		fracht_count = fracht.count();
		// we try to have one freight count to guess the right freight
		// when no besch is given
		if(fracht_count==0  &&  besch->gib_ware()!=warenbauer_t::nichts  &&  besch->gib_zuladung()>0) {
			fracht_count = 1;
		}
	}

	ding_t::rdwr(file);

	if(file->get_version()<86006) {
		// parameter werden in der deklarierten reihenfolge gespeichert
		sint32 l;
		file->rdwr_long(insta_zeit, "\n");
		file->rdwr_long(l, " ");
		dx = (sint8)l;
		file->rdwr_long(l, "\n");
		dy = (sint8)l;
		file->rdwr_long(l, "\n");
		hoff = (sint8)l;
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_delim("Wre: ");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_long(l, "\n");
		route_index = (uint16)l;
		insta_zeit = (insta_zeit >> welt->ticks_bits_per_tag) + umgebung_t::starting_year;
DBG_MESSAGE("vehicle_t::rdwr()","bought at %i/%i.",(insta_zeit%12)+1,insta_zeit/12);
	}
	else {
		// prissi: changed several data types to save runtime memory
		file->rdwr_long(insta_zeit, "\n");
		file->rdwr_byte(dx, " ");
		file->rdwr_byte(dy, "\n");
		sint16 dummy16=hoff;
		file->rdwr_short(dummy16, "\n");
		hoff = (sint8)dummy16;
		file->rdwr_long(speed_limit, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_enum(alte_fahrtrichtung, "\n");
		file->rdwr_delim("Wre: ");
		file->rdwr_long(fracht_count, " ");
		file->rdwr_short(route_index, "\n");
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

	const char *s = NULL;
	if(file->is_saving()) {
		s = besch->gib_name();
	}
	file->rdwr_str(s, " ");
	if(file->is_loading()) {
		besch = vehikelbauer_t::gib_info(s);
		if(besch==NULL) {
			besch = vehikelbauer_t::gib_info(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			dbg->warning("vehikel_t::rdwr()","no vehicle pak for '%s' search for something similar", s);
		}
		guarded_free(const_cast<char *>(s));
	}

	if(file->is_saving()) {
		if (fracht.empty()  &&  fracht_count>0) {
			// create dummy freight for savegame compatibility
			ware_t ware( besch->gib_ware() );
			ware.menge = 0;
			ware.setze_ziel( halthandle_t() );
			ware.setze_zwischenziel( halthandle_t() );
			ware.setze_zielpos( gib_pos().gib_2d() );
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

	// skip first last info (the convoi will know this better than we!
	if(file->get_version()<88007) {
		bool dummy = 0;
		file->rdwr_bool(dummy, " ");
		file->rdwr_bool(dummy, " ");
	}

	if(file->is_loading()) {
		ist_erstes = ist_letztes = false;	// dummy, will be setz by convoi afterwards
		if(besch) {
			calc_bild();
			// full weight after loading
			sum_weight =  (gib_fracht_gewicht()+499)/1000 + besch->gib_gewicht();
		}
		// recalc total freight
		total_freight = 0;
		slist_iterator_tpl<ware_t> iter(fracht);
		while(iter.next()) {
			total_freight += iter.get_current().menge;
		}
	}
}


uint32 vehikel_t::calc_restwert() const
{
	// after 20 year, it has only half value
    return (uint32)((double)besch->gib_preis() * pow(0.997, (int)(welt->get_current_month() - gib_insta_zeit())));
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


/**
 * debug info into buffer
 * @author Hj. Malthaner
 */
char * vehikel_t::debug_info(char *buf) const
{
  buf += sprintf(buf, "ist_erstes = %d, ist_letztes = %d\n",	 ist_erstes, ist_letztes);
  return buf;
}


/**
 * Debug info nach stderr
 * @author Hj. Malthaner
 * @date 26-Aug-03
 */
void vehikel_t::dump() const
{
  char buf[16000];
  debug_info(buf);
  fprintf(stderr, buf);
}



const char *
vehikel_t::ist_entfernbar(const spieler_t *)
{
	return "Fahrzeuge koennen so nicht entfernt werden";
}


vehikel_t::~vehikel_t()
{
	if(welt->lookup(gib_pos())) {
		// remove vehicle's marker from the reliefmap
		reliefkarte_t::gib_karte()->calc_map_pixel(gib_pos().gib_2d());
	}
}


// this routine will display a tooltip for lost, on depot order and stucked vehicles
void
vehikel_t::display_after(int xpos, int ypos, bool is_gobal) const
{
	if(is_gobal  &&  cnv  &&  ist_erstes) {

		COLOR_VAL color;
		char tooltip_text[1024];
		tooltip_text[0] = 0;

		// now find out what has happend
		switch(cnv->get_state()) {
			case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			case convoi_t::CAN_START_TWO_MONTHS:
				sprintf( tooltip_text, translator::translate("Vehicle %s is stucked!"), cnv->gib_name() );
				color = COL_ORANGE;
				break;
			case convoi_t::ROUTING_2:
				if(cnv->hat_keine_route()) {
					sprintf( tooltip_text, translator::translate("Vehicle %s can't find a route!"), cnv->gib_name() );
					color = COL_RED;
				}
		}

		// something to show?
		if(tooltip_text[0]) {
			const int width = proportional_string_width(tooltip_text)+7;
			const int raster_width = get_tile_raster_width();
			xpos += tile_raster_scale_x(gib_xoff(), raster_width);
			ypos += tile_raster_scale_y(gib_yoff(), raster_width);
			if(ypos>LINESPACE+32  &&  ypos+LINESPACE<display_gib_clip_wh().yy) {
				display_ddd_proportional( xpos, ypos, width, 0, color, COL_BLACK, tooltip_text, true);
			}
		}
	}
}



/*--------------------------- Fahrdings ------------------------------*/


automobil_t::automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
	cnv = cn;
}



automobil_t::automobil_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
	rdwr(file, true);
}


// ned to reset halt reservation (if there was one)
bool automobil_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
	assert(cnv);
	// free target reservation
	if(ist_erstes  &&  alte_fahrtrichtung!=ribi_t::keine  &&  cnv  &&  target_halt.is_bound() ) {
		route_t *rt=cnv->get_route();
		grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));
		if(target) {
			target_halt->unreserve_position(target,cnv->self);
		}
	}
	target_halt = halthandle_t();	// no block reserved
	return route->calc_route(welt, start, ziel, this, max_speed );
}



bool
automobil_t::ist_befahrbar(const grund_t *bd) const
{
	strasse_t *str=(strasse_t *)bd->gib_weg(road_wt);
	if(str==NULL  ||  ((cnv!=NULL ? cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric)  &&  !str->is_electrified()) ) {
		return false;
	}
	// check for signs
	if(str->has_sign()) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(rs!=NULL) {
			if(rs->gib_besch()->gib_min_speed()>0  &&  rs->gib_besch()->gib_min_speed()>kmh_to_speed(gib_besch()->gib_geschw())) {
				return false;
			}
			// do not search further for a free stop beyond here
			if(target_halt.is_bound()  &&  cnv->is_waiting()  &&  rs->gib_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA) {
				return false;
			}
		}
	}
	return true;
}



// how expensive to go here (for way search)
// author prissi
int
automobil_t::gib_kosten(const grund_t *gr,const uint32 max_speed) const
{
	// first favor faster ways
	const weg_t *w=gr->gib_weg(road_wt);
	if(!w) {
		return 0xFFFF;
	}

	// max_speed?
	uint32 max_tile_speed = w->gib_max_speed();

	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	// assume all traffic (and even road signs etc.) is not good ...
	costs += gr->gib_top();

	// effect of hang ...
	if(gr->gib_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 15;
	}
	return costs;
}


// this routine is called by find_route, to determined if we reached a destination
bool automobil_t::ist_ziel(const grund_t *gr,const grund_t *) const
{
	//  just check, if we reached a free stop position of this halt
	if(gr->is_halt()  &&  gr->gib_halt()==target_halt  &&  target_halt->is_reservable((grund_t *)gr,cnv->self)) {
//DBG_MESSAGE("is_target()","success at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
		return true;
	}
	return false;
}


bool
automobil_t::ist_weg_frei(int &restart_speed)
{
	const grund_t *gr = welt->lookup(pos_next);
	if(gr==NULL) {
		cnv->suche_neue_route();
		return false;
	}

	// check for traffic lights (only relevant for the first car in a convoi)
	if(ist_erstes) {
		// pruefe auf Schienenkreuzung
		strasse_t *str=(strasse_t *)gr->gib_weg(road_wt);

		if(str==NULL  ||  gr->gib_top()>200) {
			// too many cars here or no street
			return false;
		}

		// first: check roadsigns
		if(str->has_sign()) {
			const roadsign_t* rs = gr->find<roadsign_t>();
			if(rs) {
				// since at the corner, our direct may be diagonal, we make it straight
				const uint8 richtung = ribi_typ(gib_pos().gib_2d(),pos_next.gib_2d());

				if(rs->gib_besch()->is_traffic_light()  &&  (rs->get_dir()&richtung)==0) {
					fahrtrichtung = richtung;
					calc_bild();
					// wait here
					restart_speed = 16;
					return false;
				}
				// check, if we reached a choose point
				else if(rs->is_free_route(richtung)) {
					route_t *rt=cnv->get_route();
					grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));

					// is our target occupied?
					if(target) {
						target_halt = target->gib_halt();
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
							target_halt->reserve_position(welt->lookup(target_rt.position_bei(target_rt.gib_max_n())),cnv->self);
	//DBG_MESSAGE("automobil_t::ist_weg_frei()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.gib_max_n()).x,target_rt.position_bei(target_rt.gib_max_n()).y, target_rt.position_bei(target_rt.gib_max_n()).z );
							rt->remove_koord_from(route_index);
							rt->append( &target_rt );
						}
					}
				}
			}
		}

		// calculate new direction
		const uint8 next_fahrtrichtung = calc_richtung(pos_prev.gib_2d(), pos_next.gib_2d());
		const uint8 next_90fahrtrichtung = route_index<cnv->get_route()->gib_max_n() ? this->calc_richtung(gib_pos().gib_2d(), cnv->get_route()->position_bei(route_index+1).gib_2d()) : calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d());;
		bool frei = true;

		// suche vehikel
		const uint8 top = gr->gib_top();
		for(  uint8 pos=0;  pos<top  && frei;  pos++ ) {
			ding_t *dt = gr->obj_bei(pos);
			if(dt) {
				uint8 other_fahrtrichtung=255;

				// check for car
				if(dt->gib_typ()==ding_t::automobil) {
					automobil_t *at = (automobil_t *)dt;
					// check, if this is not ourselves
					if(at->cnv!=cnv) {
						other_fahrtrichtung = at->gib_fahrtrichtung();
					}
				}

				// check for city car
				if(dt->gib_typ()==ding_t::verkehr) {
					vehikel_basis_t *v = (vehikel_basis_t *)dt;
					other_fahrtrichtung = v->gib_fahrtrichtung();
				}

				// ok, there is another car ...
				if(other_fahrtrichtung!=255) {

					if(other_fahrtrichtung==next_fahrtrichtung  ||  other_fahrtrichtung==next_90fahrtrichtung  ||  other_fahrtrichtung==gib_fahrtrichtung() ) {
						// this goes into the same direction as we, so stopp and save a restart speed
						frei = false;
						restart_speed = 0;
						if(cnv  &&  cnv->get_state()==convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH) {
							if( ((vehikel_basis_t *)dt)->is_stuck() ) {
								cnv->reset_waiting();
							}
						}

					} else if(ribi_t::ist_orthogonal(other_fahrtrichtung,gib_fahrtrichtung() )) {

						// there is a car orthogonal to us
						frei = false;
						restart_speed = 0;
					}
				}
			}
		}

		if(frei  &&  str->is_crossing()) {
			// ok, crossing ahead
			crossing_t* cr = gr->find<crossing_t>(2);
			frei = cr->request_crossing(this);
		}

		return frei;
	}

	return true;
}



void
automobil_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	const int cargo = gib_fracht_menge();
	weg_t *str=welt->lookup( gib_pos() )->gib_weg(road_wt);
	str->book(cargo, WAY_STAT_GOODS);
	if (ist_erstes)  {
		str->book(1, WAY_STAT_CONVOIS);
	}
}




fahrplan_t * automobil_t::erzeuge_neuen_fahrplan() const
{
  return new autofahrplan_t();
}


void
automobil_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
automobil_t::rdwr(loadsave_t *file, bool force)
{
	assert(force);

	if(file->is_saving() && cnv != NULL && !force) {
		file->wr_obj_id(-1);
	}
	else {
		vehikel_t::rdwr(file);

		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			const ware_besch_t* w = (!fracht.empty() ? fracht.front().gib_typ() : warenbauer_t::passagiere);
			DBG_MESSAGE("automobil_t::rdwr()","try to find a fitting vehicle for %s.",  w->gib_name() );
			besch = vehikelbauer_t::vehikel_search(road_wt, 0, ist_erstes?50:0, speed_to_kmh(speed_limit), w );
			if(besch) {
				DBG_MESSAGE("automobil_t::rdwr()","replaced by %s",besch->gib_name());
				// still wrong load ...
				calc_bild();
			}
			if (!fracht.empty() && fracht.front().menge == 0) {
				// this was only there to find a matching vehicle
				fracht.remove_first();
			}
		}
	}
}


void
automobil_t::setze_convoi(convoi_t *c)
{
	DBG_MESSAGE("automobil_t::setze_convoi()","%p",c);
	if(c!=NULL) {
		bool target=(bool)cnv;	// only during loadtype: cnv==1 indicates, that the convoi did reserve a stop
		vehikel_t::setze_convoi(c);
		if(target  &&  ist_erstes) {
			// reintitialize the target halt
			route_t *rt=cnv->get_route();
			grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));
			target_halt = target->gib_halt();
			if(target_halt.is_bound()) {
				target_halt->reserve_position(target,cnv->self);
			}
		}
	}
	else {
		if(cnv  &&  ist_erstes  &&  target_halt.is_bound()) {
			target_halt->unreserve_position(welt->lookup(cnv->get_route()->position_bei(cnv->get_route()->gib_max_n())),cnv->self);
			target_halt = halthandle_t();
		}
		cnv = NULL;
	}
}



/* from now on rail vehicles (and other vehicles using blocks) */

waggon_t::waggon_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
    rdwr(file, true);
}


waggon_t::waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
    cnv = cn;
}


waggon_t::~waggon_t()
{
	if(cnv  &&  ist_erstes  &&  route_index<cnv->get_route()->gib_max_n()+1) {
		// free als reserved blocks
		block_reserver( cnv->get_route(), cnv->gib_vehikel(cnv->gib_vehikel_anzahl()-1)->gib_route_index(), target_halt.is_bound()?1000:1, false );
	}
	grund_t *gr = welt->lookup(gib_pos());
	if(gr) {
		schiene_t * sch = (schiene_t *)gr->gib_weg(gib_waytype());
		if(sch) {
			sch->unreserve(this);
		}
	}
}



void
waggon_t::setze_convoi(convoi_t *c)
{
	if(c!=cnv) {
		DBG_MESSAGE("waggon_t::setze_convoi()","new=%p old=%p",c,cnv);
		if(ist_erstes) {
			if(cnv!=NULL  &&  cnv!=(convoi_t *)1) {
				// free route from old convoi
				if(route_index<cnv->get_route()->gib_max_n()+1) {
					block_reserver( cnv->get_route(), cnv->gib_vehikel(cnv->gib_vehikel_anzahl()-1)->gib_route_index(), 1000, false );
					target_halt = halthandle_t();
				}
			}
			else {
				// eventually reserve new route
				if(c  &&  c->get_state()==convoi_t::DRIVING) {
DBG_MESSAGE("waggon_t::setze_convoi()","new route %p, route_index %i",c->get_route(),route_index);
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
				if(c->get_state()>=convoi_t::WAITING_FOR_CLEARANCE) {
DBG_MESSAGE("waggon_t::setze_convoi()","new route %p, route_index %i",c->get_route(),route_index);
					// find about next signal after loading
					uint16 next_signal_index=65535;
					route_t *route=c->get_route();

					if(gib_pos()==route->position_bei(route->gib_max_n())) {
						// we are there, were we should go? Usually this is an error during autosave
						c->suche_neue_route();
					}
					else {
						for(  uint16 i=max(route_index,1)-1;  i<=route->gib_max_n();  i++) {
							schiene_t * sch = (schiene_t *) welt->lookup(route->position_bei(i))->gib_weg(gib_waytype());
							if(sch==NULL) {
								break;
							}
							if(sch->has_signal()) {
								next_signal_index = i+1;
								break;
							}
						}
						c->set_next_stop_index( next_signal_index );
					}
				}
			}
		}
		vehikel_t::setze_convoi(c);
	}
}


// need to reset halt reservation (if there was one)
bool waggon_t::calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route)
{
	if(ist_erstes  &&  route_index<cnv->get_route()->gib_max_n()) {
		// free all reserved blocks
		block_reserver( cnv->get_route(), cnv->gib_vehikel(cnv->gib_vehikel_anzahl()-1)->gib_route_index(), target_halt.is_bound()?1000:1, false );
	}
	target_halt = halthandle_t();	// no block reserved
	return route->calc_route(welt, start, ziel, this, max_speed );
}



bool
waggon_t::ist_befahrbar(const grund_t *bd) const
{
	const schiene_t * sch = dynamic_cast<const schiene_t *> (bd->gib_weg(gib_waytype()));

	// Hajo: diesel and steam engines can use electrifed track as well.
	// also allow driving on foreign tracks ...
	const bool needs_no_electric = !(cnv!=NULL ? cnv->needs_electrification() : besch->get_engine_type()==vehikel_besch_t::electric);
	bool ok = (sch!=0)  &&  (needs_no_electric  ||  sch->is_electrified());

	if(!ok  ||  !target_halt.is_bound()  ||  !cnv->is_waiting()) {
		return ok;
	}
	else {
		// we are searching a stop here:
		// ok, we can go where we already are ...
		if(bd->gib_pos()==gib_pos()) {
			return true;
		}
		// we cannot pass an end of choose area
		if(sch->has_sign()) {
			const roadsign_t* rs = bd->find<roadsign_t>();
			if(rs->gib_besch()->gib_wtyp()==gib_waytype()  &&  rs->gib_besch()->get_flags()&roadsign_besch_t::END_OF_CHOOSE_AREA) {
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
waggon_t::gib_kosten(const grund_t *gr,const uint32 max_speed) const
{
	// first favor faster ways
	const weg_t *w=gr->gib_weg(gib_waytype());
	uint32 max_tile_speed = w ? w->gib_max_speed() : 999;
	// add cost for going (with maximum speed, cost is 1)
	int costs = (max_speed<=max_tile_speed) ? 1 :  (max_speed*4)/(max_tile_speed*4);

	// effect of hang ...
	if(gr->gib_weg_hang()!=0) {
		// we do not need to check whether up or down, since everything that goes up must go down
		// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
		costs += 25;
	}

	return costs;
}



signal_t *
waggon_t::ist_blockwechsel(koord3d k2) const
{
	const schiene_t * sch1 = (const schiene_t *) welt->lookup( k2 )->gib_weg(gib_waytype());
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
	const schiene_t * sch1 = (const schiene_t *) gr->gib_weg(gib_waytype());
	// first check blocks, if we can go there
	if(sch1->can_reserve(cnv->self)) {
		//  just check, if we reached a free stop position of this halt
		if(gr->is_halt()  &&  gr->gib_halt()==target_halt) {
			// now we must check the precessor ...
			if(prev_gr!=NULL) {
				const koord dir=gr->gib_pos().gib_2d()-prev_gr->gib_pos().gib_2d();
				grund_t *to;
				if(!gr->get_neighbour(to,gib_waytype(),dir)  ||  !(to->gib_halt()==target_halt)) {
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
	if(ist_erstes  &&  (cnv->get_state()==convoi_t::CAN_START  ||  cnv->get_state()==convoi_t::CAN_START_ONE_MONTH)) {
		// reserve first block at the start until the next signal
		uint16 n=block_reserver(cnv->get_route(), max(route_index,1)-1, 0, true);
		if(n==0) {
//DBG_MESSAGE("cannot","start at index %i",route_index);
			restart_speed = 0;
			return false;
		}
		cnv->set_next_stop_index(n);
		return true;
	}

	const grund_t *gr = welt->lookup(pos_next);
	if(gr->gib_top()>200) {
		// too many objects here
		return false;
	}

	if(!welt->lookup( pos_next )->hat_weg(gib_waytype())) {
		return false;
	}
	if(!ist_erstes) {
		restart_speed = -1;
		return true;
	}

	uint16 next_block=cnv->get_next_stop_index()-1;
	if(next_block<=route_index+3) {
		route_t *rt=cnv->get_route();
		koord3d block_pos=rt->position_bei(next_block);
		signal_t *sig = ist_blockwechsel(block_pos);
		if(sig) {
			// action depend on the next signal
			uint16 next_stop = 0;
			const roadsign_besch_t *sig_besch=sig->gib_besch();

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
				short fahrplan_index = cnv->gib_fahrplan()->get_aktuell();
				int count = 0;
				route_t target_rt;
				koord3d cur_pos = rt->position_bei(next_block+1);
				do {
					// search for route
					if(!target_rt.calc_route( welt, cur_pos , cnv->gib_fahrplan()->eintrag[fahrplan_index].pos, this, speed_to_kmh(cnv->gib_min_top_speed()))) {
						exit_loop = true;
					}
					else {
						// check tiles of route until we find signal or reserved track
						for (int i = 0; i<=target_rt.gib_max_n(); i++) {
							koord3d pos = target_rt.position_bei(i);
							grund_t *gr = welt->lookup(pos);
							schiene_t * sch1 = gr ? (schiene_t *)gr->gib_weg(gib_waytype()) : NULL;
							if(sch1 && sch1->has_signal()) {
								next_stop = block_reserver(cnv->get_route(),next_block+1,0,true);
								if(next_stop > 0) {
									// we should be able to always get reservation to next station - since we've already checked
									// that tiles are clear. No harm in sanity check though...
									restart_speed = -1;
									cnv->set_next_stop_index(next_stop);
									sig->setze_zustand(roadsign_t::gruen);
									return true;
								}
								exit_loop = true;
							} else if(sch1 && sch1->get_reserved_convoi()!=convoihandle_t()) {
								// for some reason sch1->can_reserve() crashed from time to time so we use get_reserved_convoi instead...
								exit_loop = true;
							}
						}
					}
					target_rt.clear();
					cur_pos = cnv->gib_fahrplan()->eintrag[fahrplan_index].pos;
					fahrplan_index ++;
					if(fahrplan_index >= cnv->gib_fahrplan()->maxi()) {
						fahrplan_index = 0;
					}
					count++;
				} while(count < cnv->gib_fahrplan()->maxi() && exit_loop == false); // stop after we've looped round schedule...
				// we can't go
				sig->setze_zustand(roadsign_t::rot);
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
				target=welt->lookup(rt->position_bei(rt->gib_max_n()));

				if(target) {
					target_halt = target->gib_halt();
				}
			}
			next_stop = block_reserver(cnv->get_route(),next_block+1,(target_halt.is_bound()?1000:sig_besch->is_pre_signal()),true);

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
				const int richtung = ribi_typ(gib_pos().gib_2d(),pos_next.gib_2d());	// to avoid confusion at diagonals
#ifdef MAX_CHOOSE_BLOCK_TILES
				if(!target_rt.find_route( welt, rt->position_bei(next_block), this, speed_to_kmh(cnv->gib_min_top_speed()), richtung, MAX_CHOOSE_BLOCK_TILES )) {
#else
				if(!target_rt.find_route( welt, rt->position_bei(next_block), this, speed_to_kmh(cnv->gib_min_top_speed()), richtung, welt->gib_groesse_x()+welt->gib_groesse_y() )) {
#endif
					// nothing empty or not route with less than MAX_CHOOSE_BLOCK_TILES tiles
					target_halt = halthandle_t();
				}
				else {
					// try to alloc the whole route
					rt->remove_koord_from(next_block);
					rt->append( &target_rt );
					next_stop = block_reserver(rt,next_block,1000,true);
				}
				// reserved route to target (or not)
			}
			// next signal can be passed
			if(next_stop!=0) {
				restart_speed = -1;
				cnv->set_next_stop_index(next_stop);
				sig->setze_zustand(roadsign_t::gruen);
			}
			else {
				// cannot be passed
				sig->setze_zustand(roadsign_t::rot);
				if(route_index==next_block+1) {
//DBG_MESSAGE("cannot","continue");
					restart_speed = 0;
					return false;
				}
			}
		}
		else {
			// end of route?
			if(next_block+1>=cnv->get_route()->gib_max_n()  &&  route_index==next_block+1) {
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
			uint16 next_stop = block_reserver(cnv->get_route(),next_block+1,target_halt.is_bound()?1000:0,true);
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

	if(start_index>route->gib_max_n()) {
		return 0;
	}

	if(route->position_bei(start_index)==gib_pos()) {
		start_index++;
	}

	// find next blocksegment enroute
	uint16 i=start_index;
	uint16 next_signal_index=65535, skip_index=65535;
	uint16 next_crossing_index=65535;
	bool unreserve_now = false;
	for ( ; success  &&  count>=0  &&  i<=route->gib_max_n(); i++) {

		koord3d pos = route->position_bei(i);
		grund_t *gr = welt->lookup(pos);
		schiene_t * sch1 = gr ? (schiene_t *)gr->gib_weg(gib_waytype()) : NULL;
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
					signal->setze_zustand(roadsign_t::rot);
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
				schiene_t * sch1 = (schiene_t *)welt->lookup( route->position_bei(j))->gib_weg(gib_waytype());
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
			signal->setze_zustand(roadsign_t::gruen);
		}
	}

	// if next stop is further away then next crossing, return next crossing
	if(next_signal_index>next_crossing_index) {
		next_signal_index = next_crossing_index;
	}
	// stop at station or signals, not at waypoints
	if(next_signal_index==65535) {
		// find out if stop or waypoint, waypoint: do not brake at waypoints
		grund_t *gr=welt->lookup(route->position_bei(route->gib_max_n()));
		return (gr  &&  gr->is_halt()) ? route->gib_max_n() : 65535;
	}
	return next_signal_index+1;
}



/* beware: we must take unreserve railblocks ... */
void
waggon_t::verlasse_feld()
{
	vehikel_t::verlasse_feld();
	// fix counters
	if(ist_letztes) {
		schiene_t * sch0 = (schiene_t *) welt->lookup( gib_pos() )->gib_weg(gib_waytype());
		if(sch0) {
			sch0->unreserve(this);
			// tell next signal?
			// and swith to red
			if(sch0->has_signal()) {
				signal_t* sig = welt->lookup(gib_pos())->find<signal_t>();
				if(sig) {
					sig->setze_zustand(roadsign_t::rot);
				}
			}
		}
	}
}



void
waggon_t::betrete_feld()
{
	vehikel_t::betrete_feld();

	schiene_t * sch0 = (schiene_t *) welt->lookup(gib_pos())->gib_weg(gib_waytype());
	if(sch0) {
		sch0->reserve(cnv->self);
		// way statistics
		const int cargo = gib_fracht_menge();
		sch0->book(cargo, WAY_STAT_GOODS);
		if(ist_erstes) {
			sch0->book(1, WAY_STAT_CONVOIS);
		}
	}
}



fahrplan_t * waggon_t::erzeuge_neuen_fahrplan() const
{
  return besch->get_waytype()==tram_wt ? new tramfahrplan_t() : new zugfahrplan_t();
}


void
waggon_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
waggon_t::rdwr(loadsave_t *file, bool force)
{
	static const vehikel_besch_t *last_besch;
	assert(force);

	if(file->is_saving()  &&  cnv!=NULL  &&  !force) {
		file->wr_obj_id(-1);
		last_besch = NULL;
	}
	else {
		vehikel_t::rdwr(file);

		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			int power = (ist_erstes || fracht.empty() || fracht.front() == warenbauer_t::nichts) ? 500 : 0;
			const ware_besch_t* w = power ? warenbauer_t::nichts : fracht.front().gib_typ();
			DBG_MESSAGE("waggon_t::rdwr()","try to find a fitting vehicle for %s.", power>0 ? "engine": w->gib_name() );
			if(!ist_erstes  &&  last_besch!=NULL  &&  last_besch->gib_ware()==w  &&
				(
					(power>0  &&  last_besch->gib_leistung()>0)
//					||  (power==0  &&  last_besch->gib_leistung()>=0)
				)
			) {
				// same as previously ...
				besch = last_besch;
			}
			else {
				// we have to search
				last_besch = besch = vehikelbauer_t::vehikel_search(gib_waytype(), 0, ist_erstes ? 500 : power, speed_to_kmh(speed_limit), power>0 ? NULL : w, false );
			}
			if(besch) {
DBG_MESSAGE("waggon_t::rdwr()","replaced by %s",besch->gib_name());
				calc_bild();
			}
			else {
dbg->error("waggon_t::rdwr()","no matching besch found for %s!",w->gib_name());
			}
			if (!fracht.empty() && fracht.front().menge == 0) {
				// this was only there to find a matchin vehicle
				fracht.remove_first();
			}
		}
	}
}



fahrplan_t * monorail_waggon_t::erzeuge_neuen_fahrplan() const
{
	return new monorailfahrplan_t();
}


schiff_t::schiff_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cn) :
	vehikel_t(pos, besch, sp)
{
    cnv = cn;
}

schiff_t::schiff_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
	rdwr(file, true);
}



bool
schiff_t::ist_befahrbar(const grund_t *bd) const
{
	return bd->ist_wasser()  ||  bd->hat_weg(water_wt);
}



/* Since slopes are handled different for ships
 * @author prissi
 */
void
schiff_t::calc_akt_speed(const grund_t *gr)
{
	// even track
	current_friction = 1;
	// or a hill?
	if(gr->gib_weg_hang()) {
		// hill up or down => in lock => deccelarte
		current_friction = 16;
	}

	// curve: higher friction
	if(alte_fahrtrichtung != fahrtrichtung) {
		current_friction = 2;
	}

	if(ist_erstes) {
		// just to accelerate: The actual speed takes care of all vehicles in the convoi
		cnv->setze_akt_speed_soll( speed_limit );
	}
}



bool
schiff_t::ist_weg_frei(int &restart_speed)
{
	restart_speed = -1;

	if(ist_erstes) {
		grund_t *gr = welt->lookup( pos_next );
		if(gr==NULL) {
			cnv->suche_neue_route();
			return false;
		}

		weg_t *w = gr->gib_weg(water_wt);
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



fahrplan_t * schiff_t::erzeuge_neuen_fahrplan() const
{
  return new schifffahrplan_t();
}



void
schiff_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	}
	else {
		rdwr(file, true);
	}
}

void
schiff_t::rdwr(loadsave_t *file, bool force)
{
	assert(force);
	if(file->is_saving() && cnv != NULL && !force) {
		file->wr_obj_id(-1);
	} else {
		vehikel_t::rdwr(file);
		// try to find a matching vehivle
		if(file->is_loading()  &&  besch==NULL) {
			DBG_MESSAGE("schiff_t::rdwr()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().gib_name() : "passagiere");
			besch = vehikelbauer_t::vehikel_search(water_wt, 0, 100, 40, !fracht.empty() ? fracht.front().gib_typ() : warenbauer_t::passagiere);
			if(besch) {
				calc_bild();
			}
		}
	}
}


/**** from here on planes ***/


// this routine is called by find_route, to determined if we reached a destination
bool aircraft_t::ist_ziel(const grund_t *gr,const grund_t *) const
{
	if(state!=looking_for_parking) {

		// search for the end of the runway
		const weg_t *w=gr->gib_weg(air_wt);
		if(w  &&  w->gib_besch()->gib_styp()==1) {

//DBG_MESSAGE("aircraft_t::ist_ziel()","testing at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
			// ok here is a runway
			ribi_t::ribi ribi= w->gib_ribi_unmasked();
//DBG_MESSAGE("aircraft_t::ist_ziel()","ribi=%i target_ribi=%i",ribi,approach_dir);
			if(ribi_t::ist_einfach(ribi)  &&  (ribi&approach_dir)!=0) {
				// pointing in our direction
				// here we should check for length, but we assume everything is ok
//DBG_MESSAGE("aircraft_t::ist_ziel()","success at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
				return true;
			}
		}
	}
	else {
		// otherwise we just check, if we reached a free stop position of this halt
		if(gr->gib_halt()==target_halt  &&  target_halt->is_reservable((grund_t *)gr,cnv->self)) {
			return true;
		}
	}
//DBG_MESSAGE("is_target()","failed at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
	return false;
}



// for flying thingies, everywhere is good ...
// another function only called during route searching
ribi_t::ribi
aircraft_t::gib_ribi(const grund_t *gr) const
{
	switch(state) {
		case taxiing:
		case looking_for_parking:
			return gr->gib_weg_ribi(air_wt);

		case taxiing_to_halt:
		{
			// we must invert all one way signs here, since we start from the target position here!
			weg_t *w = gr->gib_weg(air_wt);
			if(w) {
				ribi_t::ribi r = w->gib_ribi_unmasked();
				ribi_t::ribi mask = w->gib_ribi_maske();
				return (mask) ? (r & ~ribi_t::rueckwaerts(mask)) : r;
			}
			return ribi_t::keine;
		}

		case landing:
		case departing:
		{
			ribi_t::ribi dir = gr->gib_weg_ribi(air_wt);
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
aircraft_t::gib_kosten(const grund_t *gr,const uint32 ) const
{
	// first favor faster ways
	const weg_t *w=gr->gib_weg(air_wt);
	int costs = 0;

	if(state==flying) {
		if(w==NULL) {
			costs += 1;
		}
		else {
			if(w->gib_besch()->gib_styp()==0) {
				costs += 25;
			}
		}
	}
	else {
		// only, if not flying ...
		assert(w);

		if(w->gib_besch()->gib_styp()==0) {
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
//DBG_MESSAGE("ist_befahrbar()","at %i,%i",bd->gib_pos().x,bd->gib_pos().y);
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
	grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));

//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","can approach? (cnv %i)",cnv->self.get_id());

	target_halt = target ? target->gib_halt() : halthandle_t();
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
		target_halt->reserve_position(welt->lookup(target_rt.position_bei(target_rt.gib_max_n())),cnv->self);
//DBG_MESSAGE("aircraft_t::find_route_to_stop_position()","found free stop near %i,%i,%i",target_rt.position_bei(target_rt.gib_max_n()).x,target_rt.position_bei(target_rt.gib_max_n()).y, target_rt.position_bei(target_rt.gib_max_n()).z );
		rt->remove_koord_from(suchen);
		rt->append( &target_rt );
		return true;
	}
}





bool
aircraft_t::ist_weg_frei(int & restart_speed)
{
	restart_speed = -1;

	grund_t *gr = welt->lookup( pos_next );
	if(gr==NULL) {
		cnv->suche_neue_route();
		return false;
	}

	if(gr->gib_top()>200) {
		// too many objects here
		return false;
	}

	if(route_index==takeoff  &&  state==taxiing) {
		// stop shortly at the end of the runway
		state = departing;
		restart_speed = 0;
		return false;
	}

//DBG_MESSAGE("aircraft_t::ist_weg_frei()","index %i<>%i",route_index,touchdown);

	// check for another circle ...
	if(route_index==(touchdown-4)  &&  !target_halt.is_bound()) {
		// circle slowly next round
		cnv->setze_akt_speed_soll( kmh_to_speed(besch->gib_geschw())/2 );
		route_index -= 16;
		state = flying;
		return true;
	}

	if(route_index==touchdown-16-3  &&  state!=flying2) {
		// if we fail, we will wait in a step, much more simulation friendly
		// and the route finder is not reentrant!
		if(!cnv->is_waiting()) {
			return false;
		}

		if(find_route_to_stop_position()) {
			// stop reservation successful
			route_t *rt=cnv->get_route();
			route_index += 16;
			pos_next==rt->position_bei(route_index);
			state = flying2;
		}
		else {
			// circle slowly
			cnv->setze_akt_speed_soll( kmh_to_speed(besch->gib_geschw())/2 );
			route_t *rt=cnv->get_route();
			pos_next==rt->position_bei(route_index);
			state = flying2;
//DBG_MESSAGE("aircraft_t::ist_weg_frei()","%i circles from idx %i",cnv->self.get_id(),route_index );
		}
		return true;
	}

	if(route_index==suchen  &&  state==landing) {
		// stop shortly at the end of the runway
		state = taxiing;
		restart_speed = 0;
		return false;
	}

	if(state==looking_for_parking) {
		state = taxiing;
	}

	if (state == taxiing && gr->is_halt() && gr->find<aircraft_t>()) {
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
		const short landehoehe=height_scaling(cnv->get_route()->position_bei(touchdown).z)+(touchdown-route_index);
		if(landehoehe*TILE_HEIGHT_STEP/Z_TILE_STEP<flughoehe) {
			state = landing;
			target_height = height_scaling(cnv->get_route()->position_bei(touchdown).z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
		}
	}
	else {
		weg_t *w=welt->lookup(gib_pos())->gib_weg(air_wt);
		if(w) {
			const int cargo = gib_fracht_menge();
			w->book(cargo, WAY_STAT_GOODS);
			if (ist_erstes) {
				w->book(1, WAY_STAT_CONVOIS);
			}
		}
	}
}




aircraft_t::aircraft_t(karte_t *welt, loadsave_t *file) : vehikel_t(welt)
{
	rdwr(file, true);
	old_x = old_y = -1;
	old_bild = IMG_LEER;
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
aircraft_t::setze_convoi(convoi_t *c)
{
	DBG_MESSAGE("automobil_t::setze_convoi()","%p",c);
	if(c!=NULL) {
		bool target=(bool)cnv;
		vehikel_t::setze_convoi(c);
		if(target  &&  ist_erstes) {
			// reintitialize the target halt
			route_t *rt=cnv->get_route();
			grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));
			target_halt = target->gib_halt();
			if(target_halt.is_bound()) {
				target_halt->reserve_position(target,cnv->self);
			}
		}
	}
	else {
		if(ist_erstes  &&  target_halt.is_bound()) {
			target_halt->unreserve_position(welt->lookup(cnv->get_route()->position_bei(cnv->get_route()->gib_max_n())),cnv->self);
			target_halt = halthandle_t();
		}
		cnv = NULL;
	}
}




fahrplan_t * aircraft_t::erzeuge_neuen_fahrplan() const
{
	return new airfahrplan_t();
}



void
aircraft_t::rdwr(loadsave_t *file)
{
	if(file->is_saving() && cnv != NULL) {
		file->wr_obj_id(-1);
	} else {
		rdwr(file, true);
	}
}

void
aircraft_t::rdwr(loadsave_t *file, bool force)
{
	// make sure, we are not called by loading via dingliste directly from a tile!
	assert(force);

	if(file->is_saving() && cnv != NULL && !force) {
		file->wr_obj_id(-1);
	}
	else {
		vehikel_t::rdwr(file);
		file->rdwr_enum(state, " ");
		file->rdwr_short(flughoehe, " ");
		file->rdwr_short(target_height, "\n");
		file->rdwr_long(suchen," ");
		file->rdwr_long(touchdown," ");
		file->rdwr_long(takeoff,"\n");
	}
}



#ifdef USE_DIFFERENT_WIND
// well lots of code to make sure, we have at least two diffrent directions for the runway search
uint8
aircraft_t::get_approach_ribi( koord3d start, koord3d ziel )
{
	uint8 dir = ribi_typ( (koord)((ziel-start).gib_2d()) );	// reverse
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

	// free target reservation
	if(ist_erstes  &&  alte_fahrtrichtung!=ribi_t::keine  &&  cnv  &&  target_halt.is_bound() ) {
		route_t *rt=cnv->get_route();
		grund_t *target=welt->lookup(rt->position_bei(rt->gib_max_n()));
		if(target) {
			target_halt->unreserve_position(target,cnv->self);
		}
	}
	target_halt = halthandle_t();	// no block reserved

	const weg_t *w=welt->lookup(start)->gib_weg(air_wt);
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

	if(start_in_the_air  ||  (w->gib_besch()->gib_styp()==1  &&  ribi_t::ist_einfach(w->gib_ribi())) ) {
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
		search_start = route->position_bei( route->gib_max_n() );
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
		search_end = end_route.position_bei( end_route.gib_max_n() );
	}
//DBG_MESSAGE("aircraft_t::calc_route()","ziel now %i,%i,%i",search_end.x,search_end.y,search_end.z);

	// create target route
	if(!start_in_the_air) {
		takeoff = route->gib_max_n();
		koord start_dir(welt->lookup(search_start)->gib_weg_ribi(air_wt));
		if(start_dir!=koord(0,0)) {
			// add the start
			const grund_t *gr=NULL;
			int endi = 4;
			// now add all runway + 3 ...
			for( int i=1;  i<endi;  i++  ) {
				if(!welt->ist_in_kartengrenzen(search_start.gib_2d()+(start_dir*i)) ) {
					break;
				}
				gr = welt->lookup_kartenboden(search_start.gib_2d()+(start_dir*i));
				if(gr->hat_weg(air_wt)) {
					endi ++;
				}
				route->append(gr->gib_pos());
			}
			// out of map
			if(gr==NULL) {
				dbg->error("aircraft_t::calc_route()","out of map!");
				return false;
			}
			// need some extra step to avoid 180° turns
			if( start_dir.x!=0  &&  sgn(start_dir.x)==sgn(search_end.x-search_start.x)  ) {
				route->append( welt->lookup(gr->gib_pos().gib_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->gib_kartenboden()->gib_pos() );
				route->append( welt->lookup(gr->gib_pos().gib_2d()+koord(0,(search_end.y>search_start.y) ? 2 : -2 ) )->gib_kartenboden()->gib_pos() );
			}
			else if( start_dir.y!=0  &&  sgn(start_dir.y)==sgn(search_end.y-search_start.y)  ) {
				route->append( welt->lookup(gr->gib_pos().gib_2d()+koord((search_end.x>search_start.x) ? 1 : -1 ,0) )->gib_kartenboden()->gib_pos() );
				route->append( welt->lookup(gr->gib_pos().gib_2d()+koord((search_end.x>search_start.x) ? 2 : -2 ,0) )->gib_kartenboden()->gib_pos() );
			}
		}
		else {
			// init with startpos
			dbg->error("aircraft_t::calc_route()","Invalid route calculation: start is on a single direction field ...");
		}
		state = taxiing;
		flughoehe = gib_pos().z;
	}
	else {
		// init with current pos (in air ... )
		route->clear();
		route->append( gib_pos() );
		state = flying;
		cnv->setze_akt_speed_soll( vehikel_t::SPEED_UNLIMITED );
		flughoehe = gib_pos().z+48;
		takeoff = 0;
	}
	target_height = gib_pos().z+48;

//DBG_MESSAGE("aircraft_t::calc_route()","take off ok");

	koord3d landing_start=search_end;
	if(!end_in_air) {
		// now find way to start of landing pos
		koord end_dir(welt->lookup(search_end)->gib_weg_ribi(air_wt));
		if(end_dir!=koord(0,0)) {
			// add the start
			const grund_t *gr;
			int endi = 3;
			// now add all runway + 3 ...
			for( int i=0;  i<endi;  i++  ) {
				if(!welt->ist_in_kartengrenzen(search_end.gib_2d()+(end_dir*i)) ) {
					break;
				}
				gr=welt->lookup_kartenboden(search_end.gib_2d()+(end_dir*i));
				if(gr->hat_weg(air_wt)) {
					endi ++;
				}
				landing_start = gr->gib_pos();
			}
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
		switch(welt->lookup(search_end)->gib_weg_ribi(air_wt)) {
			case ribi_t::nord: offset = 0; break;
			case ribi_t::west: offset = 4; break;
			case ribi_t::sued: offset = 8; break;
			case ribi_t::ost: offset = 12; break;
		}

		// now make a curve
		koord circlepos=landing_start.gib_2d();
		static const koord circle_koord[16]={ koord(0,1), koord(0,1), koord(1,0), koord(0,1), koord(1,0), koord(1,0), koord(0,-1), koord(1,0), koord(0,-1), koord(0,-1), koord(-1,0), koord(0,-1), koord(-1,0), koord(-1,0), koord(0,1), koord(-1,0) };

		// circle to the left
		route->append( welt->lookup_kartenboden(circlepos)->gib_pos() );
		for(  int  i=0;  i<16;  i++  ) {
			circlepos += circle_koord[(offset+i+16)%16];
			if(welt->ist_in_kartengrenzen(circlepos)) {
				route->append( welt->lookup_kartenboden(circlepos)->gib_pos() );
			}
			else {
				// could only happen during loading old savegames;
				// in new versions it should not possible to build a runway here
				route->clear();
				dbg->error("aircraft_t::calc_route()","airport too close to the edge! (Cannot go to %i,%i!)",circlepos.x,circlepos.y);
				return false;
			}
		}

		touchdown = route->gib_max_n()+3;
		route->append_straight_route(welt,search_end);

		// now the route rearch point (+1, since it will check before entering the tile ...)
		suchen = route->gib_max_n();

		// now we just append the rest
		for( int i=end_route.gib_max_n()-1;  i>=0;  i--  ) {
			route->append(end_route.position_bei(i));
		}
	}

//DBG_MESSAGE("aircraft_t::calc_route()","departing=%i  touchdown=%i   suchen=%i   total=%i  state=%i",takeoff, touchdown, suchen, route->gib_max_n(), state );
	return true;
}



// well, the heihgt is of course most important for an aircraft ...
int aircraft_t::calc_height()
{
	const sint16 h_cur = height_scaling(gib_pos().z)*TILE_HEIGHT_STEP/Z_TILE_STEP;

	switch( state ) {
		case departing:
			{
				flughoehe = 0;
				current_friction = 14;
				speed_limit = SPEED_UNLIMITED;

				// take off, when a) end of runway or b) last tile of runway or c) fast enough
				weg_t *weg=welt->lookup(gib_pos())->gib_weg(air_wt);
				if((weg==NULL  ||  weg->gib_besch()->gib_styp()!=1)  ||  cnv->gib_akt_speed()>kmh_to_speed(besch->gib_geschw())/3 ) {
					state = flying;
					current_friction = 16;
					target_height = h_cur+TILE_HEIGHT_STEP*3;
				}
			}
			break;

		case flying:
		case flying2:
		{
			const sint16 h_next=height_scaling(pos_next.z)*TILE_HEIGHT_STEP/Z_TILE_STEP;

			speed_limit = SPEED_UNLIMITED;

			// did we have to change our flight height?
			if(target_height-h_next>TILE_HEIGHT_STEP*5) {
				// sinken
				target_height -= TILE_HEIGHT_STEP*2;
			}
			else if(target_height-h_next<TILE_HEIGHT_STEP*2) {
				// steigen
				target_height += TILE_HEIGHT_STEP*2;
			}
			// now change flight level if required
			if(flughoehe<target_height) {
				flughoehe ++;
			}
			else if(flughoehe>target_height) {
				flughoehe --;
			}
			else {
				// after reaching flight level, friction will be constant
				current_friction = 1;
			}
			flughoehe -= h_cur;
		}
		break;

		case landing:
		{
			if(flughoehe>target_height) {
				// still decenting
				flughoehe--;
				speed_limit = kmh_to_speed(besch->gib_geschw())/2;
				current_friction = 64;
				flughoehe -= h_cur;
			}
			else {
				// touchdown!
				flughoehe = 0;
				// all planes taxi with same speed
				speed_limit = kmh_to_speed(60);
				current_friction = 16;
			}
		}
		break;

	default:
		speed_limit = kmh_to_speed(60);
		current_friction = 16;//(alte_fahrtrichtung != fahrtrichtung) ? 512 : 128;
		flughoehe = 0;
		break;
	}

	cnv->setze_akt_speed_soll( speed_limit );

	// this calculation is no really working, since it assumes straight ways ...
	// but for a shadow it is good enough
	return vehikel_basis_t::calc_height();
}



// needed for shadows ...
void aircraft_t::sync_step()
{
	setze_yoff( gib_yoff() - hoff );
	if(!get_flag(ding_t::dirty)) {
		// airplane and shadow
		mark_image_dirty( bild, gib_yoff()-flughoehe-hoff-2 );
		mark_image_dirty( bild, hoff );
	}
	flughoehe += height_scaling(gib_pos().z)*TILE_HEIGHT_STEP/Z_TILE_STEP;
	fahre();
	hoff = calc_height();
	setze_yoff( gib_yoff() + hoff );
}



// this routine will display the shadow
void
aircraft_t::display_after(int xpos, int ypos, bool /*reset_dirty*/) const
{
	if(bild != IMG_LEER) {
		const int raster_width = get_tile_raster_width();
		xpos += tile_raster_scale_x(gib_xoff(), raster_width);
		ypos += tile_raster_scale_y(gib_yoff()-flughoehe-hoff-2, raster_width);

		// will be dirty
		display_color_img(bild, xpos, ypos, get_player_nr(), true, get_flag(ding_t::dirty) );
	}
}
