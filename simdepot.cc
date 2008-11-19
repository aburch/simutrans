/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simintr.h"

#include "simconvoi.h"
#include "vehicle/simvehikel.h"
#include "simwin.h"
#include "simware.h"
#include "simhalt.h"
#include "player/simplay.h"
#include "simworld.h"
#include "simio.h"
#include "simdepot.h"
#include "simline.h"
#include "simlinemgmt.h"

#include "gui/depot_frame.h"
#include "gui/messagebox.h"

#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "bauer/vehikelbauer.h"

#include "boden/wege/schiene.h"

#include "besch/haus_besch.h"


slist_tpl<depot_t *> depot_t::all_depots;

depot_t::depot_t(karte_t *welt,loadsave_t *file) : gebaeude_t(welt)
{
	rdwr(file);
	if(file->get_version()<88002) {
		setze_yoff(0);
	}
	all_depots.append(this);
}



depot_t::depot_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    gebaeude_t(welt, pos, sp, t)
{
	all_depots.append(this);
}


depot_t::~depot_t()
{
	destroy_win((long)this);
	all_depots.remove(this);
}


// finds the next/previous depot relative to the current position
depot_t *depot_t::find_depot( koord3d start, const ding_t::typ depot_type, const spieler_t *sp, bool forward)
{
	depot_t *found = NULL;
	koord3d found_pos = forward ? koord3d(welt->gib_groesse_x()+1,welt->gib_groesse_y()+1,welt->gib_grundwasser()) : koord3d(-1,-1,-1);
	long found_hash = forward ? 0x7FFFFFF : -1;
	long start_hash = start.x + (8192*start.y);
	slist_iterator_tpl<depot_t *> iter(all_depots);
	while(iter.next()) {
		depot_t *d = iter.access_current();
		if(d->gib_typ()==depot_type  &&  d->gib_besitzer()==sp) {
			// ok, the right type of depot
			const koord3d pos = d->gib_pos();
			if(pos==start) {
				// ignore the start point
				continue;
			}
			long hash = (pos.x+(8192*pos.y));
			if(forward) {
				if(hash>start_hash  ||  (hash==start_hash  &&  pos.z>start.z)) {
				// found a suitable one
					if(hash<found_hash  ||  (hash==found_hash  &&  pos.z<found_pos.z)) {
						// which is closer ...
						found = d;
						found_pos = pos;
						found_hash = hash;
					}
				}
			}
			else {
				// search to start of the map
				if(hash<start_hash  ||  (hash==start_hash  &&  pos.z<start.z)) {
				// found a suitable one
					if(hash>found_hash  ||  (hash==found_hash  &&  pos.z>found_pos.z)) {
						// which is closer ...
						found = d;
						found_pos = pos;
						found_hash = hash;
					}
				}
			}
		}
	}
	return found;
}



/* this is called on two occasions:
 * first a convoy reaches the depot during its journey
 * second during loading a covoi is stored in a depot => only store it again
 */
void
depot_t::convoi_arrived(convoihandle_t acnv, bool fpl_adjust)
{
	if(fpl_adjust) {
		// here a regular convoi arrived

		for(unsigned i=0; i< acnv->gib_vehikel_anzahl(); i++) {
			vehikel_t *v = acnv->gib_vehikel(i);
			if(v) {
				// Hajo: reset vehikel data
				v->loesche_fracht();
			}
			v->setze_pos( koord3d::invalid );
		}
		// Volker: remove depot from schedule
		fahrplan_t *fpl = acnv->gib_fahrplan();
		for(  int i=0;  i<fpl->maxi();  i++  ) {
			// only if convoi found
			if(fpl->eintrag[i].pos==gib_pos()) {
				fpl->aktuell = i;
				fpl->remove();
				acnv->setze_fahrplan(fpl);
				break;
			}
		}
	}
	// this part stores the covoi in the depot
	convois.append(acnv);
	depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (long)this ));
	if(depot_frame) {
		depot_frame->action_triggered(NULL,(long int)0);
	}
	acnv->set_home_depot( gib_pos() );
	DBG_MESSAGE("depot_t::convoi_arrived()", "convoi %d, %p entered depot", acnv.get_id(), acnv.get_rep());
}


void
depot_t::zeige_info()
{
	create_win(20, 20, new depot_frame_t(this), w_info, (long)this);
}


bool depot_t::can_convoi_start(convoihandle_t /*cnv*/) const
{
	return true;
}


vehikel_t* depot_t::buy_vehicle(const vehikel_besch_t* info)
{
	// Offen: prüfen ob noch platz im depot ist???
	DBG_DEBUG("depot_t::buy_vehicle()", info->gib_name());
	vehikel_t* veh = vehikelbauer_t::baue(gib_pos(), gib_besitzer(), NULL, info );
	DBG_DEBUG("depot_t::buy_vehicle()", "vehiclebauer %p", veh);

	vehicles.append(veh);
	DBG_DEBUG("depot_t::buy_vehicle()", "appended %i vehicle", vehicles.count());
	return veh;
}


void depot_t::append_vehicle(convoihandle_t cnv, vehikel_t* veh, bool infront)
{
	/* create  a new convoi, if necessary */
	if (!cnv.is_bound()) {
		cnv = add_convoi();
	}
	veh->setze_pos(gib_pos());
	cnv->add_vehikel(veh, infront);
	vehicles.remove(veh);
}


void depot_t::remove_vehicle(convoihandle_t cnv, int ipos)
{
	vehikel_t* veh = cnv->remove_vehikel_bei(ipos);
	if (veh) {
		vehicles.append(veh);
	}
}


void depot_t::sell_vehicle(vehikel_t* veh)
{
	vehicles.remove(veh);
	gib_besitzer()->buche(veh->calc_restwert(), gib_pos().gib_2d(), COST_NEW_VEHICLE);
	DBG_MESSAGE("depot_t::sell_vehicle()", "this=%p sells %p", this, veh);
	delete veh;
}


convoihandle_t depot_t::add_convoi()
{
	convoi_t* new_cnv = new convoi_t(gib_besitzer());
	new_cnv->set_home_depot(gib_pos());
    convois.append(new_cnv->self);

    return new_cnv->self;
}

convoihandle_t depot_t::copy_convoi(convoihandle_t old_cnv)
{
    if (old_cnv.is_bound()) {
	    convoihandle_t new_cnv = add_convoi();
	    new_cnv->setze_name(old_cnv->gib_internal_name());
			int vehicle_count = old_cnv->gib_vehikel_anzahl();
			for (int i = 0; i<vehicle_count; i++) {
				const vehikel_besch_t * info = old_cnv->gib_vehikel(i)->gib_besch();
				if (info != NULL) {
					// search in depot for an existing vehicle of correct type
					vehikel_t* oldest_vehicle = get_oldest_vehicle(info);
					if (oldest_vehicle != NULL) {
						// append existing vehicle
						append_vehicle(convois.back(), oldest_vehicle, false);
					}
					else {
						// buy new vehicle
						vehikel_t* veh = vehikelbauer_t::baue(gib_pos(), gib_besitzer(), NULL, info );
						veh->setze_pos(gib_pos());
						new_cnv->add_vehikel(veh, false);
					}
				}
			}
			if (old_cnv->get_line().is_bound()) {
				new_cnv->set_line(old_cnv->get_line());
			}
			else {
				if (old_cnv->gib_fahrplan() != NULL) {
					new_cnv->setze_fahrplan(old_cnv->gib_fahrplan()->copy());
				}
			}
		return new_cnv->self;
	}
	return convoihandle_t();
}



bool depot_t::disassemble_convoi(convoihandle_t cnv, bool sell)
{
	if(cnv.is_bound()) {

		if(!sell) {
			// store vehicles in depot
			vehikel_t *v;
			while(  (v=cnv->remove_vehikel_bei(0))!=NULL  ) {
				v->loesche_fracht();
				v->setze_erstes(false);
				v->setze_letztes(false);
				vehicles.append(v);
			}
		}

		// remove from depot lists
		convois.remove(cnv);

		// and remove from welt
		cnv->self_destruct();
		return true;
	}
	return false;
}


bool depot_t::start_convoi(convoihandle_t cnv)
{
	// close schedule window if not yet closed
	if(cnv.is_bound() &&  cnv->gib_fahrplan()!=NULL) {
		if(!cnv->gib_fahrplan()->ist_abgeschlossen()) {
			destroy_win((long)cnv->gib_fahrplan());
			cnv->gib_fahrplan()->cleanup();
			cnv->gib_fahrplan()->eingabe_abschliessen();
			// just recheck if schedules match
			if(  cnv->get_line().is_bound()  &&  !cnv->get_line()->get_fahrplan()->matches( welt, cnv->gib_fahrplan() )  ) {
				cnv->unset_line();
			}
		}
	}

	if(cnv.is_bound() &&  cnv->gib_fahrplan()!=NULL  &&  cnv->gib_fahrplan()->maxi() > 0) {
		// if next schedule entry is this depot => advance to next entry
		const koord3d& cur_pos = cnv->gib_fahrplan()->eintrag[cnv->gib_fahrplan()->aktuell].pos;
		if (cur_pos == gib_pos()) {
			cnv->gib_fahrplan()->aktuell = (cnv->gib_fahrplan()->aktuell+1) % cnv->gib_fahrplan()->maxi();
		}

		// pruefen ob zug vollstaendig
		if(cnv->gib_sum_leistung() == 0 || !cnv->pruefe_alle()) {
			create_win( new news_img("Diese Zusammenstellung kann nicht fahren!\n"), w_time_delete, magic_none);
		} else if (!cnv->gib_vehikel(0)->calc_route(this->gib_pos(), cur_pos, cnv->gib_min_top_speed(), cnv->get_route())) {
			// no route to go ...
			static char buf[256];
			sprintf(buf,translator::translate("Vehicle %s can't find a route!"), cnv->gib_name());
			create_win( new news_img(buf), w_time_delete, magic_none);
		} else if (can_convoi_start(cnv)) {
			// der Convoi kann losdüsen
			welt->sync_add( cnv.get_rep() );
			cnv->start();

			convois.remove(cnv);
			return true;
		}
		else {
			create_win(new news_img("Blockstrecke ist\nbelegt\n"), w_time_delete, magic_none);
		}
	}
	else {
		create_win( new news_img("Noch kein Fahrzeug\nmit Fahrplan\nvorhanden\n"), w_time_delete, magic_none);

		if(!cnv.is_bound()) {
			dbg->warning("depot_t::start_convoi()","No convoi to start!");
		}
		if(cnv.is_bound() && cnv->gib_fahrplan() == NULL) {
			dbg->warning("depot_t::start_convoi()","No schedule for convoi.");
		}
		if (cnv.is_bound() && cnv->gib_fahrplan() != NULL && !cnv->gib_fahrplan()->ist_abgeschlossen()) {
			dbg->warning("depot_t::start_convoi()","Schedule is incomplete/not finished");
		}
	}
	return false;
}



// attention! this will not be used for railway depots! They will be loaded by hand ...
void
depot_t::rdwr(loadsave_t *file)
{
	gebaeude_t::rdwr(file);

	rdwr_vehikel(vehicles, file);
	if (file->get_version() < 81033) {
		// waggons are stored extra, just add them to vehicles
		assert(file->is_loading());
		rdwr_vehikel(vehicles, file);
	}
}



void
depot_t::rdwr_vehikel(slist_tpl<vehikel_t *> &list, loadsave_t *file)
{
	sint32 count;

	if(file->is_saving()) {
		count = list.count();
		DBG_MESSAGE("depot_t::vehikel_laden()","saving %d vehicles",count);
	}
	file->rdwr_long(count, "\n");

	if(file->is_loading()) {
		DBG_MESSAGE("depot_t::vehikel_laden()","loading %d vehicles",count);
		for(int i=0; i<count; i++) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();

			vehikel_t *v = NULL;

			switch( typ ) {
				case old_automobil:
				case automobil: v = new automobil_t(welt, file);    break;
				case old_waggon:
				case waggon:    v = new waggon_t(welt, file);       break;
				case old_schiff:
				case schiff:    v = new schiff_t(welt, file);       break;
				case old_aircraft:
				case aircraft: v = new aircraft_t(welt,file);  break;
				case old_monorailwaggon:
				case monorailwaggon: v = new monorail_waggon_t(welt,file);  break;
				case maglevwaggon:   v = new maglev_waggon_t(welt,file);  break;
				case narrowgaugewaggon: v = new narrowgauge_waggon_t(welt,file);  break;
				default:
					dbg->fatal("depot_t::vehikel_laden()","invalid vehicle type $%X", typ);
			}

			DBG_MESSAGE("depot_t::vehikel_laden()","loaded %s", v->gib_besch()->gib_name());
			list.insert( v );
		}
	}
	else {
		slist_iterator_tpl<vehikel_t *> l_iter ( list);
		while(l_iter.next()) {
			l_iter.get_current()->rdwr( file );
		}
	}
}


/**
 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
 * @author Hj. Malthaner
 */
const char * depot_t::ist_entfernbar(const spieler_t *sp)
{
	if(sp!=gib_besitzer()) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}
	if (!vehicles.empty()) {
		return "There are still vehicles\nstored in this depot!\n";
	}
	slist_iterator_tpl<convoihandle_t> iter(convois);

	while(iter.next()) {
		if(iter.get_current()->gib_vehikel_anzahl() > 0) {
			return "There are still vehicles\nstored in this depot!\n";
		}
	}
	return NULL;
}


slist_tpl<const vehikel_besch_t*>* depot_t::get_vehicle_type()
{
	return vehikelbauer_t::gib_info(get_wegtyp());
}


linehandle_t
depot_t::create_line()
{
	return gib_besitzer()->simlinemgmt.create_line(get_line_type());
}


vehikel_t* depot_t::get_oldest_vehicle(const vehikel_besch_t* besch)
{
	vehikel_t* oldest_veh = NULL;
	slist_iterator_tpl<vehikel_t*> iter(get_vehicle_list());
	while (iter.next()) {
		vehikel_t* veh = iter.get_current();
		if (veh->gib_besch() == besch) {
			if (oldest_veh == NULL ||
					oldest_veh->gib_insta_zeit() > veh->gib_insta_zeit()) {
				oldest_veh = veh;
			}
		}
	}
	return oldest_veh;
}


	/**
	 * sets/gets the line that was selected the last time in the depot-dialog
	 */
void depot_t::set_selected_line(const linehandle_t sel_line)
{
	selected_line = sel_line;
}

linehandle_t depot_t::get_selected_line()
{
	return selected_line;
}

bool bahndepot_t::can_convoi_start(convoihandle_t cnv) const
{
	waytype_t wt=cnv->gib_vehikel(0)->gib_waytype();
	uint32 tiles=0;
	for(uint8 i=0;  i<cnv->gib_vehikel_anzahl();  i++) {
		tiles += cnv->gib_vehikel(i)->gib_besch()->get_length();
	}
	tiles = (tiles+TILE_STEPS-1)/TILE_STEPS;

	schiene_t* sch0 = (schiene_t *)welt->lookup(gib_pos())->gib_weg(wt);
	if(sch0==NULL) {
		// no rail here???
		return false;
	}

	if(!sch0->reserve(cnv)) {
		// could not even reserve first tile ...
		return false;
	}

	// reserve the next segments of the train
	route_t *route=cnv->get_route();
	bool success = true;
	uint32 i;
	for(  i=0;  success  &&  i<tiles  &&  i<=route->gib_max_n();  i++  ) {
		schiene_t * sch1 = (schiene_t *) welt->lookup( route->position_bei(i))->gib_weg(wt);
		if(sch1==NULL) {
			dbg->warning("waggon_t::is_next_block_free()","invalid route");
			success = false;
			break;
		}
		// otherwise we might check one tile too much
		if(!sch1->reserve(cnv)) {
			success = false;
		}
	}

	if(!success  &&  i>0) {
		// free reservation, since we were not sucessfull
		i--;
		sch0->unreserve(cnv);
		for(uint32 j=0; j<i; j++) {
			schiene_t *sch1 = (schiene_t *)(welt->lookup(route->position_bei(j))->gib_weg(wt));
			sch1->unreserve(cnv);
		}
	}
	return  success;
}
