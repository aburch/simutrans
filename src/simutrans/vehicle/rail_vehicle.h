/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_RAIL_VEHICLE_H
#define VEHICLE_RAIL_VEHICLE_H


#include "vehicle.h"


/**
 * A class for rail vehicles (trains). Manages the look of the vehicles
 * and the navigability of tiles.
 * @see vehicle_t
 */
class rail_vehicle_t : public vehicle_t
{
protected:
	bool check_next_tile(const grund_t *bd) const OVERRIDE;

	void enter_tile(grund_t*) OVERRIDE;

	bool is_signal_clear(route_t::index_t start_index, sint32 &restart_speed);
	bool is_pre_signal_clear(signal_t *sig, route_t::index_t start_index, sint32 &restart_speed);
	bool is_priority_signal_clear(signal_t *sig, route_t::index_t start_index, sint32 &restart_speed);
	bool is_longblock_signal_clear(signal_t *sig, route_t::index_t start_index, sint32 &restart_speed);
	bool is_choose_signal_clear(signal_t *sig, route_t::index_t start_index, sint32 &restart_speed);

public:
	waytype_t get_waytype() const OVERRIDE { return track_wt; }

	// since we might need to un-reserve previously used blocks, we must do this before calculation a new route
	bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route) OVERRIDE;

	// how expensive to go here (for way search)
	int get_cost(const grund_t *gr, const weg_t *w, const sint32 max_speed, ribi_t::ribi from) const OVERRIDE;

	uint32 get_cost_upslope() const OVERRIDE { return 25; }

	// returns true for the way search to an unknown target.
	bool is_target(const grund_t *,const grund_t *) const OVERRIDE;

	// handles all block stuff and route choosing ...
	bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8) OVERRIDE;

	// reserves or un-reserves all blocks and returns the handle to the next block (if there)
	// returns true on successful reservation
	bool block_reserver(const route_t *route, route_t::index_t start_index, route_t::index_t &next_signal, route_t::index_t &next_crossing, int signal_count, bool reserve, bool force_unreserve ) const;

	void leave_tile() OVERRIDE;

	typ get_typ() const OVERRIDE { return rail_vehicle; }

	rail_vehicle_t(loadsave_t *file, bool is_first, bool is_last);
	rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t *cnv);
	~rail_vehicle_t();

	void set_convoi(convoi_t *c) OVERRIDE;

	schedule_t * generate_new_schedule() const OVERRIDE;
};



/**
 * very similar to normal railroad, so we can implement it here completely
 * @see vehicle_t
 */
class monorail_vehicle_t : public rail_vehicle_t
{
public:
	waytype_t get_waytype() const OVERRIDE { return monorail_wt; }

	// all handled by rail_vehicle_t
	monorail_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : rail_vehicle_t(file,is_first, is_last) {}
	monorail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const OVERRIDE { return monorail_vehicle; }

	schedule_t * generate_new_schedule() const OVERRIDE;
};



/**
 * very similar to normal railroad, so we can implement it here completely
 * @see vehicle_t
 */
class maglev_vehicle_t : public rail_vehicle_t
{
public:
	waytype_t get_waytype() const OVERRIDE { return maglev_wt; }

	// all handled by rail_vehicle_t
	maglev_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : rail_vehicle_t(file, is_first, is_last) {}
	maglev_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const OVERRIDE { return maglev_vehicle; }

	schedule_t * generate_new_schedule() const OVERRIDE;
};



/**
 * very similar to normal railroad, so we can implement it here completely
 * @see vehicle_t
 */
class narrowgauge_vehicle_t : public rail_vehicle_t
{
public:
	waytype_t get_waytype() const OVERRIDE { return narrowgauge_wt; }

	// all handled by rail_vehicle_t
	narrowgauge_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : rail_vehicle_t(file, is_first, is_last) {}
	narrowgauge_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const OVERRIDE { return narrowgauge_vehicle; }

	schedule_t * generate_new_schedule() const OVERRIDE;
};


#endif
