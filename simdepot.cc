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
		set_yoff(0);
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
	koord3d found_pos = forward ? koord3d(welt->get_groesse_x()+1,welt->get_groesse_y()+1,welt->get_grundwasser()) : koord3d(-1,-1,-1);
	long found_hash = forward ? 0x7FFFFFF : -1;
	long start_hash = start.x + (8192*start.y);
	slist_iterator_tpl<depot_t *> iter(all_depots);
	while(iter.next()) {
		depot_t *d = iter.access_current();
		if(d->get_typ()==depot_type  &&  d->get_besitzer()==sp) {
			// ok, the right type of depot
			const koord3d pos = d->get_pos();
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

unsigned depot_t::get_max_convoy_length(waytype_t wt)
{
	if (wt==road_wt || wt==water_wt) {
		return 4;
	}
	if (wt==air_wt) {
		return 1;
	}
	return convoi_t::max_rail_vehicle;
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

		for(unsigned i=0; i< acnv->get_vehikel_anzahl(); i++) {
			vehikel_t *v = acnv->get_vehikel(i);
			if(v) {
				// Hajo: reset vehikel data
				v->loesche_fracht();
			}
			v->set_pos( koord3d::invalid );
		}
		// Volker: remove depot from schedule
		schedule_t *fpl = acnv->get_schedule();
		for(  int i=0;  i<fpl->get_count();  i++  ) {
			// only if convoi found
			if(fpl->eintrag[i].pos==get_pos()) {
				fpl->set_aktuell( i );
				fpl->remove();
				acnv->set_schedule(fpl);
				break;
			}
		}
	}
	// this part stores the convoi in the depot
	convois.append(acnv);
	depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (long)this ));
	if(depot_frame) {
		depot_frame->action_triggered(NULL,(long int)0);
	}
	acnv->set_home_depot( get_pos() );
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


vehikel_t* depot_t::buy_vehicle(const vehikel_besch_t* info, bool upgrade)
{
	// Offen: prüfen ob noch platz im depot ist???
	// "Hours: check whether there is space in the depot?" (Google)
	DBG_DEBUG("depot_t::buy_vehicle()", info->get_name());
	vehikel_t* veh = vehikelbauer_t::baue(get_pos(), get_besitzer(), NULL, info, upgrade ); //"besitzer" = "owner" (Google)
	DBG_DEBUG("depot_t::buy_vehicle()", "vehiclebauer %p", veh);

	if(!upgrade)
	{
		vehicles.append(veh);
		DBG_DEBUG("depot_t::buy_vehicle()", "appended %i vehicle", vehicles.get_count());
		return veh;
	}
	return NULL;
}


void depot_t::append_vehicle(convoihandle_t cnv, vehikel_t* veh, bool infront)
{
	/* create  a new convoi, if necessary */
	if (!cnv.is_bound()) {
		cnv = add_convoi();
	}
	veh->set_pos(get_pos());
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
	get_besitzer()->buche(veh->calc_restwert(), get_pos().get_2d(), COST_NEW_VEHICLE);
	DBG_MESSAGE("depot_t::sell_vehicle()", "this=%p sells %p", this, veh);
	veh->before_delete();
	delete veh;
}


convoihandle_t depot_t::add_convoi()
{
	convoi_t* new_cnv = new convoi_t(get_besitzer());
	new_cnv->set_home_depot(get_pos());
    convois.append(new_cnv->self);

    return new_cnv->self;
}

convoihandle_t depot_t::copy_convoi(convoihandle_t old_cnv)
{
	if (old_cnv.is_bound()) 
	{
		convoihandle_t new_cnv;
		int vehicle_count = old_cnv->get_vehikel_anzahl();
		bool first_run = true;
		for (int i = 0; i < vehicle_count; i++) 
		{
			const vehikel_besch_t * info = old_cnv->get_vehikel(i)->get_besch();
			if (info != NULL) 
			{
				// search in depot for an existing vehicle of correct type
				vehikel_t* oldest_vehicle = get_oldest_vehicle(info);

				//Dry run first to test affordability.
				sint64 total_price = 0;
					
				if(oldest_vehicle == NULL)
				{
					total_price += info->get_preis();
				}

				if(!get_besitzer()->can_afford(total_price))
				{
					return convoihandle_t();
				}

				if (oldest_vehicle != NULL) 
				{
					// append existing vehicle
					append_vehicle(convois.back(), oldest_vehicle, false);
				}
				else 
				{
					// buy new vehicle
					if(first_run)
					{
						new_cnv = add_convoi();
						new_cnv->set_name(old_cnv->get_name());
						first_run = false;
					}
					vehikel_t* veh = vehikelbauer_t::baue(get_pos(), get_besitzer(), NULL, info );
					veh->set_pos(get_pos());
					new_cnv->add_vehikel(veh, false);
				}
			}
		}
		if (old_cnv->get_line().is_bound()) 
		{
			if(first_run)
			{
				new_cnv = add_convoi();
				new_cnv->set_name(old_cnv->get_name());
				first_run = false;
			}
			new_cnv->set_line(old_cnv->get_line());
		}
		else 
		{
			if (old_cnv->get_schedule() != NULL) 
			{
				if(first_run)
				{
					new_cnv = add_convoi();
					new_cnv->set_name(old_cnv->get_name());
					first_run = false;
				}
				new_cnv->set_schedule(old_cnv->get_schedule()->copy());
			}
		}	
		return new_cnv->self;
	}
	return convoihandle_t();
}



bool depot_t::disassemble_convoi(convoihandle_t cnv, bool sell)
{
	if(cnv.is_bound()) 
	{
		if(!sell)
		{
			// store vehicles in depot
			vehikel_t *v;
			while(  (v=cnv->remove_vehikel_bei(0))!=NULL  ) {
				v->loesche_fracht();
				v->set_erstes(false);
				v->set_letztes(false);
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
	if(cnv.is_bound() &&  cnv->get_schedule()!=NULL) {
		if(!cnv->get_schedule()->ist_abgeschlossen()) {
			destroy_win((long)cnv->get_schedule());
			cnv->get_schedule()->cleanup();
			cnv->get_schedule()->eingabe_abschliessen();
			// just recheck if schedules match
			if(  cnv->get_line().is_bound()  &&  !cnv->get_line()->get_schedule()->matches( welt, cnv->get_schedule() )  ) {
				cnv->unset_line();
			}
		}
	}

	if(cnv.is_bound() &&  cnv->get_schedule()!=NULL  &&  cnv->get_schedule()->get_count() > 0) {
		// if next schedule entry is this depot => advance to next entry
		const koord3d& cur_pos = cnv->get_schedule()->get_current_eintrag().pos;
		if (cur_pos == get_pos()) {
			cnv->get_schedule()->advance();
		}

		// pruefen ob zug vollstaendig
		if(cnv->get_sum_leistung() == 0 || !cnv->pruefe_alle()) {
			create_win( new news_img("Diese Zusammenstellung kann nicht fahren!\n"), w_time_delete, magic_none);
		} else if (!cnv->get_vehikel(0)->calc_route(this->get_pos(), cur_pos, cnv->get_min_top_speed(), cnv->get_route())) {
			// no route to go ...
			static char buf[256];
			sprintf(buf,translator::translate("Vehicle %s can't find a route!"), cnv->get_name());
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
		if(cnv.is_bound() && cnv->get_schedule() == NULL) {
			dbg->warning("depot_t::start_convoi()","No schedule for convoi.");
		}
		if (cnv.is_bound() && cnv->get_schedule() != NULL && !cnv->get_schedule()->ist_abgeschlossen()) {
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
// read/write vehicles in the depot, which are not part of a convoi.

	sint32 count;

	if(file->is_saving()) {
		count = list.get_count();
		DBG_MESSAGE("depot_t::vehikel_laden()","saving %d vehicles",count);
	}
	file->rdwr_long(count, "\n");

	if(file->is_loading()) {
		DBG_MESSAGE("depot_t::vehikel_laden()","loading %d vehicles",count);
		for(int i=0; i<count; i++) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();

			vehikel_t *v = NULL;
			const bool first = false;
			const bool last = false;

			switch( typ ) {
				case old_automobil:
				case automobil: v = new automobil_t(welt, file, first, last);    break;
				case old_waggon:
				case waggon:    v = new waggon_t(welt, file, first, last);       break;
				case old_schiff:
				case schiff:    v = new schiff_t(welt, file, first, last);       break;
				case old_aircraft:
				case aircraft: v = new aircraft_t(welt,file, first, last);  break;
				case old_monorailwaggon:
				case monorailwaggon: v = new monorail_waggon_t(welt,file, first, last);  break;
				case maglevwaggon:   v = new maglev_waggon_t(welt,file, first, last);  break;
				case narrowgaugewaggon: v = new narrowgauge_waggon_t(welt,file, first, last);  break;
				default:
					dbg->fatal("depot_t::vehikel_laden()","invalid vehicle type $%X", typ);
			}
			const vehikel_besch_t *besch = v->get_besch();
			if(besch) {
				DBG_MESSAGE("depot_t::vehikel_laden()","loaded %s", besch->get_name());
				list.insert( v );
				// BG, 06.06.2009: fixed maintenance for vehicles in the depot, which are not part of a convoi
				spieler_t::add_maintenance(get_besitzer(), besch->get_fixed_maintenance(get_welt()), spieler_t::MAINT_VEHICLE);
			}
			else {
				dbg->error("depot_t::vehikel_laden()","vehicle has no besch => ignored");
			}
		}
	}
	else {
		slist_iterator_tpl<vehikel_t *> l_iter ( list);
		while(l_iter.next()) {
			file->wr_obj_id( l_iter.get_current()->get_typ() );
			l_iter.get_current()->rdwr_from_convoi( file );
		}
	}
}

/**
 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
 * @author Hj. Malthaner
 */
const char * depot_t::ist_entfernbar(const spieler_t *sp)
{
	if(sp!=get_besitzer()) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}
	if (!vehicles.empty()) {
		return "There are still vehicles\nstored in this depot!\n";
	}
	slist_iterator_tpl<convoihandle_t> iter(convois);

	while(iter.next()) {
		if(iter.get_current()->get_vehikel_anzahl() > 0) {
			return "There are still vehicles\nstored in this depot!\n";
		}
	}
	return NULL;
}

// returns the indest of the old/newest vehicle in a list
//@author: isidoro
vehikel_t* depot_t::find_oldest_newest(const vehikel_besch_t* besch, bool old)
{
	vehikel_t* found_veh = NULL;
	slist_iterator_tpl<vehikel_t*> iter(get_vehicle_list());
	while (iter.next()) {
		vehikel_t* veh = iter.get_current();
		if (veh != NULL && veh->get_besch() == besch) {
			// joy of XOR, finally a line where I could use it!
			if (found_veh == NULL ||
					old ^ (found_veh->get_insta_zeit() > veh->get_insta_zeit())) {
				found_veh = veh;
			}
		}
	}
	return found_veh;
}


slist_tpl<const vehikel_besch_t*>* depot_t::get_vehicle_type()
{
	return vehikelbauer_t::get_info(get_wegtyp());
}


linehandle_t
depot_t::create_line()
{
	return get_besitzer()->simlinemgmt.create_line(get_line_type(),get_besitzer());
}


vehikel_t* depot_t::get_oldest_vehicle(const vehikel_besch_t* besch)
{
	vehikel_t* oldest_veh = NULL;
	slist_iterator_tpl<vehikel_t*> iter(get_vehicle_list());
	while (iter.next()) {
		vehikel_t* veh = iter.get_current();
		if (veh->get_besch() == besch) {
			if (oldest_veh == NULL ||
					oldest_veh->get_insta_zeit() > veh->get_insta_zeit()) {
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


sint32 depot_t::calc_restwert(const vehikel_besch_t *veh_type)
{
	sint32 wert = 0;

	slist_iterator_tpl<vehikel_t *> iter(get_vehicle_list());
	while(iter.next()) {
		if(iter.get_current()->get_besch() == veh_type) {
			wert += iter.get_current()->calc_restwert();
		}
	}
	return wert;
}


bool bahndepot_t::can_convoi_start(convoihandle_t cnv) const
{
	waytype_t wt=cnv->get_vehikel(0)->get_waytype();
	schiene_t* sch0 = (schiene_t *)welt->lookup(get_pos())->get_weg(wt);
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
	uint16 tiles = cnv->get_tile_length();
	uint32 i;
	for(  i=0;  success  &&  i<tiles  &&  i<=route->get_max_n();  i++  ) {
		schiene_t * sch1 = (schiene_t *) welt->lookup( route->position_bei(i))->get_weg(wt);
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
			schiene_t *sch1 = (schiene_t *)(welt->lookup(route->position_bei(j))->get_weg(wt));
			sch1->unreserve(cnv);
		}
	}
	return  success;
}

// true if already stored here
bool depot_t::is_contained(const vehikel_besch_t *info)
{
	if(vehicle_count()>0) {
		slist_iterator_tpl<vehikel_t *> iter(get_vehicle_list());
		while(iter.next()) {
			if(iter.get_current()->get_besch()==info) {
				return true;
			}
		}
	}
	return false;
}

/**
 * The player must pay monthly fixed maintenance costs for the vehicles in the depot.
 * This method is called by the world (karte_t) once per month.
 * @author Bernd Gabriel
 * @date 27.06.2009
 */
void depot_t::neuer_monat()
{
	uint32 fixed_maintenance_costs = 0;
	if (vehicle_count() > 0) {
		karte_t *world = get_welt();
		slist_iterator_tpl<vehikel_t *> vehicle_iter(vehicles);
		while (vehicle_iter.next()) {
			fixed_maintenance_costs += vehicle_iter.get_current()->get_besch()->get_fixed_maintenance(world);
		}
	}
	if (fixed_maintenance_costs)
	{
		//spieler->add_maintenance(fixed_maintenance_costs, spieler_t::MAINT_VEHICLE);
		get_besitzer()->buche(-(sint32)welt->calc_adjusted_monthly_figure(fixed_maintenance_costs), COST_VEHICLE_RUN);
	}
}
