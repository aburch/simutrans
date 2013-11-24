/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simconvoi.h"
#include "vehicle/simvehikel.h"
#include "gui/simwin.h"
#include "player/simplay.h"
#include "simworld.h"
#include "simdepot.h"
#include "simline.h"
#include "simlinemgmt.h"
#include "simmenu.h"

#include "gui/depot_frame.h"
#include "gui/messagebox.h"

#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "bauer/hausbauer.h"
#include "obj/gebaeude.h"

#include "bauer/vehikelbauer.h"

#include "besch/haus_besch.h"

#include "utils/cbuffer_t.h"


slist_tpl<depot_t *> depot_t::all_depots;

depot_t::depot_t(loadsave_t *file) : gebaeude_t()
{
	rdwr(file);
	if(file->get_version()<88002) {
		set_yoff(0);
	}
	all_depots.append(this);
	selected_filter = VEHICLE_FILTER_RELEVANT;
	last_selected_line = linehandle_t();
	command_pending = false;
}


depot_t::depot_t(koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    gebaeude_t(pos, sp, t)
{
	all_depots.append(this);
	selected_filter = VEHICLE_FILTER_RELEVANT;
	last_selected_line = linehandle_t();
	command_pending = false;
}


depot_t::~depot_t()
{
	destroy_win((ptrdiff_t)this);
	all_depots.remove(this);
}


// finds the next/previous depot relative to the current position
depot_t *depot_t::find_depot( koord3d start, const obj_t::typ depot_type, const spieler_t *sp, bool forward)
{
	depot_t *found = NULL;
	koord3d found_pos = forward ? koord3d(welt->get_size().x+1,welt->get_size().y+1,welt->get_grundwasser()) : koord3d(-1,-1,-1);
	long found_hash = forward ? 0x7FFFFFF : -1;
	long start_hash = start.x + (8192*start.y);
	FOR(slist_tpl<depot_t*>, const d, all_depots) {
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


// again needed for server
void depot_t::call_depot_tool( char tool, convoihandle_t cnv, const char *extra)
{
	// call depot tool
	werkzeug_t *w = create_tool( WKZ_DEPOT_TOOL | SIMPLE_TOOL );
	cbuffer_t buf;
	buf.printf( "%c,%s,%hu", tool, get_pos().get_str(), cnv.get_id() );
	if(  extra  ) {
		buf.append( "," );
		buf.append( extra );
	}
	w->set_default_param(buf);
	welt->set_werkzeug( w, get_besitzer() );
	// since init always returns false, it is safe to delete immediately
	delete w;
}


/* this is called on two occasions:
 * first a convoy reaches the depot during its journey
 * second during loading a convoi is stored in a depot => only store it again
 */
void depot_t::convoi_arrived(convoihandle_t acnv, bool fpl_adjust)
{
	if(fpl_adjust) {
		// here a regular convoi arrived

		for(unsigned i=0; i<acnv->get_vehikel_anzahl(); i++) {
			vehikel_t *v = acnv->get_vehikel(i);
			// Hajo: reset vehikel data
			v->loesche_fracht();
			v->set_pos( koord3d::invalid );
			v->set_erstes( i==0 );
			v->set_letztes( i+1==acnv->get_vehikel_anzahl() );
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
	depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
	if(depot_frame) {
		depot_frame->action_triggered(NULL,(long int)0);
	}
	acnv->set_home_depot( get_pos() );
	DBG_MESSAGE("depot_t::convoi_arrived()", "convoi %d, %p entered depot", acnv.get_id(), acnv.get_rep());
}


void depot_t::zeige_info()
{
	create_win( new depot_frame_t(this), w_info, (ptrdiff_t)this );
}


vehikel_t* depot_t::buy_vehicle(const vehikel_besch_t* info)
{
	DBG_DEBUG("depot_t::buy_vehicle()", info->get_name());
	vehikel_t* veh = vehikelbauer_t::baue(get_pos(), get_besitzer(), NULL, info );
	DBG_DEBUG("depot_t::buy_vehicle()", "vehiclebauer %p", veh);

	vehicles.append(veh);
	DBG_DEBUG("depot_t::buy_vehicle()", "appended %i vehicle", vehicles.get_count());
	return veh;
}


void depot_t::append_vehicle(convoihandle_t cnv, vehikel_t* veh, bool infront, bool local_execution)
{
	/* create  a new convoi, if necessary */
	if (!cnv.is_bound()) {
		cnv = add_convoi( local_execution );
	}
	veh->set_pos(get_pos());
	cnv->add_vehikel(veh, infront);
	vehicles.remove(veh);
}


void depot_t::remove_vehicle(convoihandle_t cnv, int ipos)
{
	vehikel_t* veh = cnv->remove_vehikel_bei( ipos );
	if(  veh  ) {
		vehicles.append( veh );
	}
}


void depot_t::remove_vehicles_to_end(convoihandle_t cnv, int ipos)
{
	while(  vehikel_t* veh = cnv->remove_vehikel_bei( ipos )  ) {
		vehicles.append( veh );
	}
}


void depot_t::sell_vehicle(vehikel_t* veh)
{
	vehicles.remove(veh);
	get_besitzer()->book_new_vehicle((sint64)veh->calc_restwert(), get_pos().get_2d(), get_waytype() );
	DBG_MESSAGE("depot_t::sell_vehicle()", "this=%p sells %p", this, veh);
	delete veh;
}


// returns the indest of the oldest/newest vehicle in a list
vehikel_t* depot_t::find_oldest_newest(const vehikel_besch_t* besch, bool old)
{
	vehikel_t* found_veh = NULL;
	FOR(  slist_tpl<vehikel_t*>,  const veh,  vehicles  ) {
		if(  veh->get_besch() == besch  ) {
			// joy of XOR, finally a line where I could use it!
			if(  found_veh == NULL  ||
					old ^ (found_veh->get_insta_zeit() > veh->get_insta_zeit())  ) {
				found_veh = veh;
			}
		}
	}
	return found_veh;
}


convoihandle_t depot_t::add_convoi(bool local_execution)
{
	convoi_t* new_cnv = new convoi_t(get_besitzer());
	new_cnv->set_home_depot(get_pos());
	convois.append(new_cnv->self);
	depot_frame_t *win = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
	if(  win  &&  local_execution  ) {
		win->activate_convoi( new_cnv->self );
	}
	return new_cnv->self;
}


bool depot_t::check_obsolete_inventory(convoihandle_t cnv)
{
	bool ok = true;
	slist_tpl<vehikel_t*> veh_tmp_list;

	for(  int i = 0;  i < cnv->get_vehikel_anzahl();  i++  ) {
		const vehikel_besch_t* const vb = cnv->get_vehikel(i)->get_besch();
		if(  vb  ) {
			// search storage for matching vehicle
			vehikel_t* veh = NULL;
			for(  slist_tpl<vehikel_t*>::iterator i = vehicles.begin();  i != vehicles.end();  ++i  ) {
				if(  (*i)->get_besch() == vb  ) {
					// found in storage, remove to temp list while searching for next vehicle
					veh = *i;
					vehicles.erase(i);
					veh_tmp_list.append( veh );
					break;
				}
			}
			if(  !veh  ) {
				// need to buy new
				if(  vb->is_retired( welt->get_timeline_year_month() )  ) {
					// is obsolete, return false
					ok = false;
					break;
				}
			}
		}
	}

	// put vehicles back into storage
	vehicles.append_list( veh_tmp_list );

	return ok;
}


convoihandle_t depot_t::copy_convoi(convoihandle_t old_cnv, bool local_execution)
{
	if(  old_cnv.is_bound()  &&  !convoihandle_t::is_exhausted()  &&
		old_cnv->get_vehikel_anzahl() > 0  &&  get_waytype() == old_cnv->front()->get_besch()->get_waytype() ) {

		convoihandle_t new_cnv = add_convoi( false );
		new_cnv->set_name(old_cnv->get_internal_name());
		int vehicle_count = old_cnv->get_vehikel_anzahl();
		for (int i = 0; i<vehicle_count; i++) {
			const vehikel_besch_t * info = old_cnv->get_vehikel(i)->get_besch();
			if (info != NULL) {
				// search in depot for an existing vehicle of correct type
				vehikel_t* oldest_vehicle = get_oldest_vehicle(info);
				if (oldest_vehicle != NULL) {
					// append existing vehicle
					append_vehicle( new_cnv, oldest_vehicle, false, local_execution );
				}
				else {
					// buy new vehicle
					vehikel_t* veh = vehikelbauer_t::baue(get_pos(), get_besitzer(), NULL, info );
					veh->set_pos(get_pos());
					new_cnv->add_vehikel(veh, false);
				}
			}
		}
		if (old_cnv->get_line().is_bound()) {
			new_cnv->set_line(old_cnv->get_line());
			new_cnv->get_schedule()->set_aktuell( old_cnv->get_schedule()->get_aktuell() );
		}
		else {
			if (old_cnv->get_schedule() != NULL) {
				new_cnv->set_schedule(old_cnv->get_schedule()->copy());
			}
		}

		// make this the current selected convoi
		depot_frame_t *win = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
		if(  win  ) {
			if(  local_execution  ) {
				win->activate_convoi( new_cnv );
			}
			else {
				win->update_data();
			}
		}

		return new_cnv;
	}
	return convoihandle_t();
}


bool depot_t::disassemble_convoi(convoihandle_t cnv, bool sell)
{
	if(  cnv.is_bound()  ) {
		if(  !sell  ) {
			// store vehicles in depot
			while(  vehikel_t* const v = cnv->remove_vehikel_bei(0)  ) {
				v->loesche_fracht();
				v->set_erstes(false);
				v->set_letztes(false);
				vehicles.append(v);
			}
		}

		// remove from depot lists
		remove_convoi( cnv );

		// and remove from welt
		cnv->self_destruct();
		return true;
	}
	return false;
}


bool depot_t::start_all_convoys()
{
	uint32 i = 0;
	while(  i < convois.get_count()  ) {
		if(  !start_convoi( convois.at(i), false )  ) {
			i++;
		}
	}
	return (convois.get_count() == 0);
}


bool depot_t::start_convoi(convoihandle_t cnv, bool local_execution)
{
	// close schedule window if not yet closed
	if(cnv.is_bound() &&  cnv->get_schedule()!=NULL) {
		if(!cnv->get_schedule()->ist_abgeschlossen()) {
			// close the schedule window
			destroy_win((ptrdiff_t)cnv->get_schedule());
		}
	}

	// convoi not in depot anymore, maybe user double-clicked on start-button
	if(!convois.is_contained(cnv)) {
		return false;
	}

	if (cnv.is_bound() && cnv->get_schedule() && !cnv->get_schedule()->empty()) {
		// if next schedule entry is this depot => advance to next entry
		const koord3d& cur_pos = cnv->get_schedule()->get_current_eintrag().pos;
		if (cur_pos == get_pos()) {
			cnv->get_schedule()->advance();
		}

		// check if convoi is complete
		if(cnv->get_sum_leistung() == 0 || !cnv->pruefe_alle()) {
			if (local_execution) {
				create_win( new news_img("Diese Zusammenstellung kann nicht fahren!\n"), w_time_delete, magic_none);
			}
		}
		else if(  !cnv->front()->calc_route(this->get_pos(), cur_pos, cnv->get_min_top_speed(), cnv->access_route())  ) {
			// no route to go ...
			if(local_execution) {
				static cbuffer_t buf;
				buf.clear();
				buf.printf( translator::translate("Vehicle %s can't find a route!"), cnv->get_name() );
				create_win( new news_img(buf), w_time_delete, magic_none);
			}
		}
		else {
			// convoi can start now
			welt->sync_add( cnv.get_rep() );
			cnv->start();

			// remove from depot lists
			remove_convoi( cnv );

			return true;
		}
	}
	else {
		if (local_execution) {
			create_win( new news_img("Noch kein Fahrzeug\nmit Fahrplan\nvorhanden\n"), w_time_delete, magic_none);
		}

		if (!cnv.is_bound()) {
			dbg->warning("depot_t::start_convoi()","No convoi to start!");
		} else if (!cnv->get_schedule()) {
			dbg->warning("depot_t::start_convoi()","No schedule for convoi.");
		} else if (!cnv->get_schedule()->ist_abgeschlossen()) {
			dbg->warning("depot_t::start_convoi()","Schedule is incomplete/not finished");
		}
	}
	return false;
}


void depot_t::remove_convoi( convoihandle_t cnv )
{
	depot_frame_t *win = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
	if(  win  ) {
		// get currently selected convoi to restore selection if not removed
		int icnv = win->get_icnv();
		convoihandle_t c = icnv > -1 ? get_convoi( icnv ) : convoihandle_t();

		icnv = convois.index_of( cnv );
		convois.remove( cnv );

		if(  c == cnv  ) {
			// removing currently selected, select next in list or last instead
			c = !convois.empty() ? convois.at( min((uint32)icnv, convois.get_count() - 1) ) : convoihandle_t();
		}
		win->activate_convoi( c );
	}
	else {
		convois.remove( cnv );
	}
}


// attention! this will not be used for railway depots! They will be loaded by hand ...
void depot_t::rdwr(loadsave_t *file)
{
	gebaeude_t::rdwr(file);

	rdwr_vehikel(vehicles, file);
	if (file->get_version() < 81033) {
		// waggons are stored extra, just add them to vehicles
		assert(file->is_loading());
		rdwr_vehikel(vehicles, file);
	}
}


void depot_t::rdwr_vehikel(slist_tpl<vehikel_t *> &list, loadsave_t *file)
{
	sint32 count;

	if(file->is_saving()) {
		count = list.get_count();
		DBG_MESSAGE("depot_t::vehikel_laden()","saving %d vehicles",count);
	}
	file->rdwr_long(count);

	if(file->is_loading()) {

		// no house definition for this => use a normal hut ...
		if(  this->get_tile()==NULL  ) {
			dbg->error( "depot_t::rdwr()", "tile for depot not found!" );
			set_tile( (*hausbauer_t::get_citybuilding_list( gebaeude_t::wohnung ))[0]->get_tile(0), true );
		}

		DBG_MESSAGE("depot_t::vehikel_laden()","loading %d vehicles",count);
		for(int i=0; i<count; i++) {
			obj_t::typ typ = (obj_t::typ)file->rd_obj_id();

			vehikel_t *v = NULL;
			const bool first = false;
			const bool last = false;

			switch( typ ) {
				case old_automobil:
				case automobil: v = new automobil_t(file, first, last);    break;
				case old_waggon:
				case waggon:    v = new waggon_t(file, first, last);       break;
				case old_schiff:
				case schiff:    v = new schiff_t(file, first, last);       break;
				case old_aircraft:
				case aircraft: v = new aircraft_t(file, first, last);  break;
				case old_monorailwaggon:
				case monorailwaggon: v = new monorail_waggon_t(file, first, last);  break;
				case maglevwaggon:   v = new maglev_waggon_t(file, first, last);  break;
				case narrowgaugewaggon: v = new narrowgauge_waggon_t(file, first, last);  break;
				default:
					dbg->fatal("depot_t::vehikel_laden()","invalid vehicle type $%X", typ);
			}
			if(v->get_besch()) {
				DBG_MESSAGE("depot_t::vehikel_laden()","loaded %s", v->get_besch()->get_name());
				list.insert( v );
			}
			else {
				dbg->error("depot_t::vehikel_laden()","vehicle has no besch => ignored");
			}
		}
	}
	else {
		FOR(slist_tpl<vehikel_t*>, const v, list) {
			file->wr_obj_id(v->get_typ());
			v->rdwr_from_convoi(file);
		}
	}
}


/**
 * @return NULL wenn OK, ansonsten eine Fehlermeldung
 * @author Hj. Malthaner
 */
const char * depot_t::ist_entfernbar(const spieler_t *sp)
{
	if(sp!=get_besitzer()  &&  sp!=welt->get_spieler(1)) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}
	if (!vehicles.empty()) {
		return "There are still vehicles\nstored in this depot!\n";
	}

	FOR(slist_tpl<convoihandle_t>, const c, convois) {
		if (c->get_vehikel_anzahl() > 0) {
			return "There are still vehicles\nstored in this depot!\n";
		}
	}
	return NULL;
}


slist_tpl<vehikel_besch_t const*> const& depot_t::get_vehicle_type() const
{
	return vehikelbauer_t::get_info(get_waytype());
}


vehikel_t* depot_t::get_oldest_vehicle(const vehikel_besch_t* besch)
{
	vehikel_t* oldest_veh = NULL;
	FOR(slist_tpl<vehikel_t*>, const veh, get_vehicle_list()) {
		if (veh->get_besch() == besch) {
			if (oldest_veh == NULL ||
					oldest_veh->get_insta_zeit() > veh->get_insta_zeit()) {
				oldest_veh = veh;
			}
		}
	}
	return oldest_veh;
}


void depot_t::update_win()
{
	depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
	if(depot_frame) {
		depot_frame->build_vehicle_lists();
	}
}


void depot_t::neuer_monat()
{
	// since vehicles may have become obsolete
	update_all_win();
}


void depot_t::update_all_win()
{
	FOR(slist_tpl<depot_t*>, const d, all_depots) {
		d->update_win();
	}
}


unsigned bahndepot_t::get_max_convoi_length() const
{
	return convoi_t::max_rail_vehicle;
}
