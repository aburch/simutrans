/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simconvoi.h"
#include "vehicle/simvehicle.h"
#include "gui/simwin.h"
#include "player/simplay.h"
#include "player/finance.h"
#include "simworld.h"
#include "simcity.h"
#include "simdepot.h"
#include "simline.h"
#include "simlinemgmt.h"
#include "simmenu.h"
#include "path_explorer.h"

#include "gui/depot_frame.h"
#include "gui/messagebox.h"

#include "dataobj/schedule.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "bauer/hausbauer.h"
#include "obj/gebaeude.h"

#include "bauer/vehikelbauer.h"

#include "descriptor/building_desc.h"

#include "utils/cbuffer_t.h"


slist_tpl<depot_t *> depot_t::all_depots;


#ifdef INLINE_OBJ_TYPE
depot_t::depot_t(obj_t::typ type, loadsave_t *file) : gebaeude_t(type)
#else
depot_t::depot_t(loadsave_t *file) : gebaeude_t()
#endif
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


#ifdef INLINE_OBJ_TYPE
depot_t::depot_t(obj_t::typ type, koord3d pos, player_t *player, const building_tile_desc_t *t) :
    gebaeude_t(type, pos, player, t)
#else
depot_t::depot_t(koord3d pos, player_t *player, const building_tile_desc_t *t) :
    gebaeude_t(pos, player, t)
#endif
{
	all_depots.append(this);
	selected_filter = VEHICLE_FILTER_RELEVANT;
	last_selected_line = linehandle_t();
	command_pending = false;
	add_to_world_list();
}


depot_t::~depot_t()
{
	destroy_win((ptrdiff_t)this);
	all_depots.remove(this);
	const grund_t* gr = welt->lookup(get_pos());
	if(gr)
	{
		welt->remove_building_from_world_list(this);
		// No need to remove this from the city statistics here, as this will be done by the gebaeude_t parent object.
	}
}


// finds the next/previous depot relative to the current position
depot_t *depot_t::find_depot( koord3d start, const obj_t::typ depot_type, const player_t *player, bool forward)
{
	depot_t *found = NULL;
	koord3d found_pos = forward ? koord3d(welt->get_size().x+1,welt->get_size().y+1,welt->get_groundwater()) : koord3d(-1,-1,-1);
	uint32 found_hash = forward ? 0x7FFFFFF : -1;
	uint32 start_hash = start.x + (8192 * start.y);
	FOR(slist_tpl<depot_t*>, const d, all_depots) {
		if(d->get_typ()==depot_type  &&  d->get_owner()==player) {
			// ok, the right type of depot
			const koord3d pos = d->get_pos();
			if(pos==start) {
				// ignore the start point
				continue;
			}
			sint32 hash = (pos.x + (8192 * pos.y));
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
		return convoi_t::max_vehicle;
	}
	if (wt==air_wt) {
		return 1;
	}
	return convoi_t::max_rail_vehicle;
}

// again needed for server
void depot_t::call_depot_tool( char tool, convoihandle_t cnv, const char *extra, uint16 livery_scheme_index)
{
	// call depot tool
	tool_t *tmp_tool = create_tool( TOOL_CHANGE_DEPOT | SIMPLE_TOOL );
	cbuffer_t buf;
	buf.printf( "%c,%s,%hu,%hu", tool, get_pos().get_str(), cnv.get_id(), livery_scheme_index );
	if(  extra  ) {
		buf.append( "," );
		buf.append( extra );
	}
	tmp_tool->set_default_param(buf);
	welt->set_tool( tmp_tool, get_owner() );
	// since init always returns false, it is safe to delete immediately
	delete tmp_tool;
}


/* this is called on two occasions:
 * first a convoy reaches the depot during its journey (or on emergency stop)
 * second during loading a convoi is stored in a depot => only store it again
 */
void depot_t::convoi_arrived(convoihandle_t acnv, bool fpl_adjust)
{
	if(fpl_adjust) {
		// Volker: remove depot from schedule
		schedule_t *schedule = acnv->get_schedule();
		for(  int i=0;  i<schedule->get_count();  i++  ) {
			// only if convoi found
			if(schedule->entries[i].pos==get_pos()) {
				schedule->set_current_stop( i );
				schedule->remove();
				acnv->set_schedule(schedule);
			}
		}

		if(acnv->get_line().is_bound())
		{
			acnv->unset_line();
			acnv->unregister_stops();
		}
		else
		{
			acnv->unregister_stops();
		}
#ifdef MULTI_THREAD
		world()->await_path_explorer();
#endif
		path_explorer_t::refresh_all_categories(false);
	}

	// Clean up the vehicles -- get rid of freight, etc.  Do even when loading, just in case.
	for(unsigned i=0; i<acnv->get_vehicle_count(); i++) {
		vehicle_t *v = acnv->get_vehicle(i);
		// Hajo: reset vehicle data
		v->discard_cargo();
		v->set_pos( koord3d::invalid );
		v->set_leading( i==0 );
		v->set_last( i+1==acnv->get_vehicle_count() );
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


void depot_t::show_info()
{
	create_win( new depot_frame_t(this), w_info, (ptrdiff_t)this );
}


vehicle_t* depot_t::buy_vehicle(const vehicle_desc_t* info, uint16 livery_scheme_index)
{
	DBG_DEBUG("depot_t::buy_vehicle()", info->get_name());
	vehicle_t* veh = vehicle_builder_t::build(get_pos(), get_owner(), NULL, info, false, livery_scheme_index); //"owner" = "owner" (Google)
	DBG_DEBUG("depot_t::buy_vehicle()", "vehiclebauer %p", veh);
	vehicles.append(veh);
	DBG_DEBUG("depot_t::buy_vehicle()", "appended %i vehicle", vehicles.get_count());
	return veh;
}

void depot_t::upgrade_vehicle(convoihandle_t cnv, const vehicle_desc_t* vb)
{
	if(!cnv.is_bound())
	{
		return;
	}

	for(uint16 i = 0; i < cnv->get_vehicle_count(); i ++)
	{
		for(uint8 c = 0; c < cnv->get_vehicle(i)->get_desc()->get_upgrades_count(); c ++)
		{
			if(cnv->get_vehicle(i)->get_desc()->get_upgrades(c) == vb)
			{
				vehicle_t* new_veh = vehicle_builder_t::build(get_pos(), get_owner(), NULL, vb, true, cnv->get_livery_scheme_index());
				cnv->upgrade_vehicle(i, new_veh);
				if(cnv->get_vehicle(i)->get_desc()->get_trailer_count() == 1 && cnv->get_vehicle(i)->get_desc()->get_power() != 0)
				{
					// We need to upgrade tenders, too.
					const vehicle_desc_t* new_desc = new_veh->get_desc()->get_trailer(0);
					if (new_desc)
					{
						vehicle_t* new_veh_2 = vehicle_builder_t::build(get_pos(), get_owner(), NULL, new_desc, true);
						cnv->upgrade_vehicle(i + 1, new_veh_2);
						// The above assumes that tenders are free, which they are in Pak128.Britain, the cost being built into the locomotive.
						// The below ought work more accurately, but does not work properly, for some reason.

						/*if(cnv->get_vehicle(i + 1)->get_desc()->get_upgrades_count() >= c && cnv->get_vehicle(i + 1)->get_desc()->get_upgrades_count() > 0)
						{
							cnv->get_vehicle(i + 1)->set_desc(cnv->get_vehicle(i + 1)->get_desc()->get_upgrades(c));
						}
						else if(cnv->get_vehicle(i + 1)->get_desc()->get_upgrades_count() > 0)
						{
							cnv->get_vehicle(i + 1)->set_desc(cnv->get_vehicle(i + 1)->get_desc()->get_upgrades(0));
						}*/
					}
				}
				//Check whether this is a Garrett type vehicle (this is code for the exceptional case where a Garrett is upgraded to another Garrett)
				if(cnv->get_vehicle(0)->get_desc()->get_power() == 0 && cnv->get_vehicle(0)->get_desc()->get_total_capacity() == 0)
				{
					// Possible Garrett
					const uint8 count = cnv->get_vehicle(0)->get_desc()->get_trailer_count();
					if(count > 0 && cnv->get_vehicle(1)->get_desc()->get_power() > 0 && cnv->get_vehicle(1)->get_desc()->get_trailer_count() > 0)
					{
						// Garrett detected - need to upgrade all three vehicles.
						vehicle_t* new_veh_2 = vehicle_builder_t::build(get_pos(), get_owner(), NULL, new_veh->get_desc()->get_trailer(0), true);
						cnv->upgrade_vehicle(i + 1, new_veh_2);
						vehicle_t* new_veh_3 = vehicle_builder_t::build(get_pos(), get_owner(), NULL, new_veh->get_desc()->get_trailer(0), true);
						cnv->upgrade_vehicle(i + 2, new_veh_3);
					}
				}
				// Only upgrade one at a time.
				// Could add UI feature in future to allow upgrading all at once.
				return;
			}
		}
	}
}


void depot_t::append_vehicle(convoihandle_t &cnv, vehicle_t* veh, bool infront, bool local_execution)
{
	/* create  a new convoi, if necessary */
	if (!cnv.is_bound()) {
		cnv = add_convoi( local_execution );
	}
	veh->set_pos(get_pos());
	cnv->add_vehicle(veh, infront);
	vehicles.remove(veh);
}


void depot_t::remove_vehicle(convoihandle_t cnv, int ipos)
{
	vehicle_t* veh = cnv->remove_vehicle_bei( ipos );
	if(  veh  ) {
		vehicles.append( veh );
	}
}


void depot_t::remove_vehicles_to_end(convoihandle_t cnv, int ipos)
{
	while(  vehicle_t* veh = cnv->remove_vehicle_bei( ipos )  ) {
		vehicles.append( veh );
	}
}


void depot_t::sell_vehicle(vehicle_t* veh)
{
	vehicles.remove(veh);
	sint64 cost = veh->calc_sale_value();
	get_owner()->book_new_vehicle(cost, get_pos().get_2d(), get_waytype() );
	DBG_MESSAGE("depot_t::sell_vehicle()", "this=%p sells %p", this, veh);
	veh->before_delete();
	delete veh;
}


// returns the indest of the old/newest vehicle in a list
//@author: isidoro
vehicle_t* depot_t::find_oldest_newest(const vehicle_desc_t* desc, bool old, vector_tpl<vehicle_t*> *avoid)
{
	vehicle_t* found_veh = NULL;
	FOR(slist_tpl<vehicle_t*>, const veh, vehicles)
	{
		if(veh != NULL && veh->get_desc() == desc)
		{
			// joy of XOR, finally a line where I could use it!
			if(avoid == NULL || (!avoid->is_contained(veh) && (found_veh == NULL ||
					old ^ (found_veh->get_purchase_time() > veh->get_purchase_time())))) // Used when replacing to avoid specifying the same vehicle twice
			{
				found_veh = veh;
			}
		}
	}
	return found_veh;
}


convoihandle_t depot_t::add_convoi(bool local_execution)
{
	convoi_t* new_cnv = new convoi_t(get_owner());
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
	slist_tpl<vehicle_t*> veh_tmp_list;

	for(  int i = 0;  i < cnv->get_vehicle_count();  i++  ) {
		const vehicle_desc_t* const vb = cnv->get_vehicle(i)->get_desc();
		if(  vb  ) {
			// search storage for matching vehicle
			vehicle_t* veh = NULL;
			for(  slist_tpl<vehicle_t*>::iterator i = vehicles.begin();  i != vehicles.end();  ++i  ) {
				if(  (*i)->get_desc() == vb  ) {
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
		old_cnv->get_vehicle_count() > 0  &&  get_waytype() == old_cnv->front()->get_desc()->get_waytype() )
	{
		convoihandle_t new_cnv = add_convoi( false );
		new_cnv->set_name(old_cnv->get_internal_name());
		new_cnv->set_livery_scheme_index(old_cnv->get_livery_scheme_index());
		int vehicle_count = old_cnv->get_vehicle_count();
		for (int i = 0; i < vehicle_count; i++)
		{
			const vehicle_desc_t * info = old_cnv->get_vehicle(i)->get_desc();
			if (info != NULL)
			{
				// search in depot for an existing vehicle of correct type
				vehicle_t* new_vehicle = get_oldest_vehicle(info);
				if(new_vehicle == NULL)
				{
					// no vehicle of correct type in depot, must buy it:
					//first test affordability.
					sint64 total_price = info->get_value();
					if(!get_owner()->can_afford(total_price))
					{
						create_win( new news_img(NOTICE_INSUFFICIENT_FUNDS), w_time_delete, magic_none);
						if(!new_cnv.is_bound())
						{
							return new_cnv;
						}

						if(new_cnv->get_vehicle_count() == 0)
						{
							disassemble_convoi(new_cnv, true);
							return convoihandle_t();
						}

						break; // ... and what happens with the first few vehicles, if you: return convoihandle_t();

					}
					// buy new vehicle
					new_vehicle = vehicle_builder_t::build(get_pos(), get_owner(), NULL, info, false, old_cnv->get_livery_scheme_index());
				}

				// Reassign the classes:
				uint8 classes_to_check = info->get_number_of_classes();
				for (int j = 0; j < classes_to_check; j++)
				{
					if (old_cnv->get_vehicle(i)->get_reassigned_class(j) != j)
					{
						new_vehicle->set_class_reassignment(j, old_cnv->get_vehicle(i)->get_reassigned_class(j));
					}
				}
				// append new vehicle
				append_vehicle(new_cnv, new_vehicle, false, local_execution);
			}
		}
		if (old_cnv->get_line().is_bound())
		{
			if(!new_cnv.is_bound())
			{
				new_cnv = add_convoi(local_execution);
			}
			new_cnv->set_line(old_cnv->get_line());
			new_cnv->get_schedule()->set_current_stop( old_cnv->get_schedule()->get_current_stop() );
		}
		else
		{
			if (old_cnv->get_schedule() != NULL)
			{
				if(!new_cnv.is_bound())
				{
					new_cnv = add_convoi(local_execution);
				}
				new_cnv->set_schedule(old_cnv->get_schedule()->copy());
			}
		}
		if (new_cnv.is_bound())
		{
			new_cnv->set_name(old_cnv->get_internal_name());

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
			while(  vehicle_t* const v = cnv->remove_vehicle_bei(0)  ) {
				v->discard_cargo();
				v->set_leading(false);
				v->set_last(false);
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
		if(!cnv->get_schedule()->is_editing_finished()) {
			// close the schedule window
			destroy_win((ptrdiff_t)cnv->get_schedule());
		}
	}

	// convoi not in depot anymore, maybe user double-clicked on start-button
	if(!convois.is_contained(cnv)) {
		return false;
	}

	if (cnv.is_bound() && cnv->front()->get_owner() == NULL)
	{
		// It it not clear why the version for this on loading does not work.
		cnv->front()->set_owner(cnv->get_owner());
		cnv->back()->set_owner(cnv->get_owner());
	}
	if (cnv.is_bound() && cnv->get_schedule() && !cnv->get_schedule()->empty()) {
		// if next schedule entry is this depot => advance to next entry
		const koord3d& cur_pos = cnv->get_schedule()->get_current_entry().pos;
		if (cur_pos == get_pos()) {
			cnv->get_schedule()->advance();
		}

		bool convoy_unpowered = cnv->get_sum_power() == 0 || cnv->calc_max_speed(cnv->get_weight_summary()) == 0;

		if(convoy_unpowered)
		{
			// HACK: Not sure what is causing the basic problem with cnv->get_sum_power() reporting 0 with some very large aircraft (currently only 747s).

			bool power = false;
			bool speed = false;
			vector_tpl<const vehicle_desc_t*> vehicle_types;
			const uint8 number_of_vehicles = cnv->get_vehicle_count();
			for(uint8 i = 0; i < number_of_vehicles; i++)
			{
				if(cnv->get_vehicle(i)->get_desc()->get_power())
				{
					power = true;
				}
				vehicle_types.append((cnv->get_vehicle(i)->get_desc()));
			}
			if(power)
			{
				potential_convoy_t convoy(vehicle_types);
				const vehicle_summary_t &vsum = convoy.get_vehicle_summary();
				const sint32 friction = convoy.get_current_friction();
				//const double rolling_resistance = convoy.get_resistance_summary().to_double();
				//const uint32 number_of_vehicles = vehicle_types.get_count();
				const uint32 max_speed = convoy.calc_max_speed(weight_summary_t(vsum.weight, friction));
				speed = max_speed;
			}

			convoy_unpowered = !(power && speed);
		}

		// check if convoy is complete
		if(convoy_unpowered || !cnv->pruefe_alle())
		{
			if (local_execution)
			{
				create_win( new news_img("Diese Zusammenstellung kann nicht fahren!\n"), w_time_delete, magic_none);
			}
		}
		else if(  !cnv->front()->calc_route(this->get_pos(), cur_pos, cnv->get_min_top_speed(), cnv->has_tall_vehicles(), cnv->access_route())) {
			// no route to go ...
			if(local_execution) {
				static cbuffer_t buf;
				buf.clear();
				buf.printf( translator::translate("Vehicle %s can't find a route!"), cnv->get_name() );
				create_win( new news_img(buf), w_time_delete, magic_none);
			}
		}
		else if (!(cnv->front()->get_desc()->get_basic_constraint_prev(cnv->front()->is_reversed()) & vehicle_desc_t::can_be_head)) {
			// Is there a cab at the front end of convoy?
			create_win(new news_img("Cannot start: no cab at the front of the convoy."), w_time_delete, magic_none);
		}
		else {
			// convoi can start now
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
		} else if (!cnv->get_schedule()->is_editing_finished()) {
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

	rdwr_vehicle(vehicles, file);
	if (file->get_version() < 81033) {
		// waggons are stored extra, just add them to vehicles
		assert(file->is_loading());
		rdwr_vehicle(vehicles, file);
	}
}


void depot_t::rdwr_vehicle(slist_tpl<vehicle_t *> &list, loadsave_t *file)
{
// read/write vehicles in the depot, which are not part of a convoi.

	sint32 count;

	if(file->is_saving()) {
		count = list.get_count();
		DBG_MESSAGE("depot_t::vehicle_laden()","saving %d vehicles",count);
	}
	file->rdwr_long(count);

	if(file->is_loading()) {

		// no house definition for this => use a normal hut ...
		if(  this->get_tile()==NULL  ) {
			dbg->error( "depot_t::rdwr()", "tile for depot not found!" );
			set_tile( (*hausbauer_t::get_citybuilding_list( building_desc_t::city_res ))[0]->get_tile(0), true );
		}

		DBG_MESSAGE("depot_t::vehicle_laden()","loading %d vehicles",count);
		for (int i = 0; i < count; i++) {
			obj_t::typ typ = (obj_t::typ)file->rd_obj_id();

			vehicle_t* v = NULL;
			const bool first = false;
			const bool last = false;

			switch (typ) {
			case old_road_vehicle:
			case road_vehicle: v = new road_vehicle_t(file, first, last);    break;
			case old_waggon:
			case rail_vehicle:    v = new rail_vehicle_t(file, first, last);       break;
			case old_schiff:
			case water_vehicle:    v = new water_vehicle_t(file, first, last);       break;
			case old_aircraft:
			case air_vehicle: v = new air_vehicle_t(file, first, last);  break;
			case old_monorail_vehicle:
			case monorail_vehicle: v = new monorail_rail_vehicle_t(file, first, last);  break;
			case maglev_vehicle:   v = new maglev_rail_vehicle_t(file, first, last);  break;
			case narrowgauge_vehicle: v = new narrowgauge_rail_vehicle_t(file, first, last);  break;
			default:
				dbg->fatal("depot_t::vehicle_laden()", "invalid vehicle type $%X", typ);
			}
			const vehicle_desc_t* desc = v->get_desc();
			if (desc) {
				DBG_MESSAGE("depot_t::vehicle_laden()", "loaded %s", desc->get_name());
				list.insert(v);
			}
			else {
				dbg->error("depot_t::vehicle_laden()", "vehicle has no desc => ignored");
			}

			if (v->get_owner() == NULL)
			{
				// Vehicles should not be unowned in depots; assume that the owner of the depot owns the vehicle.
				v->set_owner(get_owner());
			}
		}
	}
	else {
		FOR(slist_tpl<vehicle_t*>, const v, list) {
			file->wr_obj_id(v->get_typ());
			v->rdwr_from_convoi(file);
		}
	}
}

/**
 * @return NULL when OK, otherwise an error message
 * @author Hj. Malthaner
 */
const char * depot_t:: is_deletable(const player_t *player)
{
	if(player!=get_owner()  &&  player!=welt->get_public_player()) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}
	if (!vehicles.empty()) {
		return "There are still vehicles\nstored in this depot!\n";
	}

	FOR(slist_tpl<convoihandle_t>, const c, convois) {
		if (c->get_vehicle_count() > 0) {
			return "There are still vehicles\nstored in this depot!\n";
		}
	}
	return NULL;
}


slist_tpl<vehicle_desc_t*> const & depot_t::get_vehicle_type()
{
	return vehicle_builder_t::get_info(get_waytype());
}


vehicle_t* depot_t::get_oldest_vehicle(const vehicle_desc_t* desc)
{
	vehicle_t* oldest_veh = NULL;
	FOR(slist_tpl<vehicle_t*>, const veh, get_vehicle_list()) {
		if (veh->get_desc() == desc) {
			if (oldest_veh == NULL ||
					oldest_veh->get_purchase_time() > veh->get_purchase_time()) {
				oldest_veh = veh;
			}
		}
	}
	return oldest_veh;
}


// true if already stored here
bool depot_t::is_contained(const vehicle_desc_t *info)
{
	FOR(slist_tpl<vehicle_t*>, const v, get_vehicle_list()) {
		if (v->get_desc() == info) {
			return true;
		}
	}
	return false;
}

void depot_t::update_win()
{
	depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)this ));
	if(depot_frame) {
		depot_frame->build_vehicle_lists();
	}
}

/**
 * The player must pay monthly fixed maintenance costs for the vehicles in the depot.
 * This method is called by the world (karte_t) once per month.
 * @author Bernd Gabriel
 * @date 27.06.2009
 */
void depot_t::new_month()
{
	sint64 fixed_cost_costs = 0;
	if (vehicle_count() > 0)
	{
		FOR(slist_tpl<vehicle_t*>, const v, get_vehicle_list())
		{
			fixed_cost_costs += v->get_desc()->get_fixed_cost(welt);
		}
	}
	if (fixed_cost_costs)
	{
		get_owner()->book_vehicle_maintenance( -fixed_cost_costs, get_waytype() );
	}

	const planquadrat_t* tile = welt->access(get_pos().get_2d());
	stadt_t* city = tile ? tile->get_city() : NULL;
	if(city && get_stadt() == NULL)
	{
		// The depot has joined a city by dint of growth.
		set_stadt(city);
		city->update_city_stats_with_building(this, false);
	}
	// Cannot check for city shrinkage without risking
	// a crash when cities are deleted.

	// since vehicles may have become obsolete
	update_all_win();
}

void depot_t::update_all_win()
{
	FOR(slist_tpl<depot_t*>, const d, all_depots) {
		d->update_win();
	}
}

/**
 * Is this depot suitable for this vehicle?
 * Must be same waytype, same owner, suitable traction, etc.
 * @param test_vehicle -- must not be NULL
 * @param traction_types
 *   - 0 if we don't want to filter by traction type
 *   - a bitmask of possible traction types; we need only match one
 */
bool depot_t::is_suitable_for( const vehicle_t * test_vehicle, const uint16 traction_types /* = 0 */ ) const {
	assert(test_vehicle != NULL);

	// Owner must be the same
	if (  this->get_owner() != test_vehicle->get_owner()  ) {
		return false;
	}

	// Right type of vehicle?  No trams in train depots, etc...
	const waytype_t my_waytype = this->get_wegtyp();
	// Subtle point here: the vehicle waytype is 'train' for trams,
	// but the vehicle desc waytype is tram.
	// Change this if we want to allow trams into train depots and vice versa.
	const waytype_t vehicle_waytype = test_vehicle->get_desc()->get_waytype();
	if (  vehicle_waytype != my_waytype  ) {
		 return false;
	}

	if (traction_types != 0 ) {
		// If traction types were specified, then *one* of them must match
		// *one* of the types supported by this depot
		if ( ! (traction_types & this->get_tile()->get_desc()->get_enabled()) ) {
			return false;
		}
	}
	// Passed all the tests
	return true;
}

void depot_t::add_to_world_list(bool)
{
	welt->add_building_to_world_list(this);
	const planquadrat_t* tile = welt->access(get_pos().get_2d());
	stadt_t* city = tile ? tile->get_city() : NULL;
	if(city)
	{
		set_stadt(city);
		city->update_city_stats_with_building(this, false);
	}
}

