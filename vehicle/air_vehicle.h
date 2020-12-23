/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_AIR_VEHICLE_H
#define VEHICLE_AIR_VEHICLE_H


#include "vehicle.h"


/**
 * A class for aircrafts. Manages the look of the vehicles
 * and the navigability of tiles.
 * @see vehicle_t
 */
class air_vehicle_t : public vehicle_t
{
public:
	enum flight_state {
		taxiing             = 0,
		departing           = 1,
		flying              = 2,
		landing             = 3,
		looking_for_parking = 4,
		circling            = 5,
		taxiing_to_halt     = 6
	};

private:
	// only used for is_target() (do not need saving)
	ribi_t::ribi approach_dir;

	// only used for route search and approach vectors of get_ribi() (do not need saving)
	koord3d search_start;
	koord3d search_end;

	flight_state state; // functions needed for the search without destination from find_route

	sint16 flying_height;
	sint16 target_height;
	uint32 search_for_stop, touchdown, takeoff;

protected:
	// jumps to next tile and correct the height ...
	void hop(grund_t*) OVERRIDE;

	bool check_next_tile(const grund_t *bd) const OVERRIDE;

	void enter_tile(grund_t*) OVERRIDE;

	bool block_reserver( uint32 start, uint32 end, bool reserve ) const;

	// find a route and reserve the stop position
	bool find_route_to_stop_position();

public:
	air_vehicle_t(loadsave_t *file, bool is_first, bool is_last);
	air_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv); // start and schedule

	// to shift the events around properly
	void get_event_index( flight_state &state_, uint32 &takeoff_, uint32 &stopsearch_, uint32 &landing_ ) { state_ = state; takeoff_ = takeoff; stopsearch_ = search_for_stop; landing_ = touchdown; }
	void set_event_index( flight_state state_, uint32 takeoff_, uint32 stopsearch_, uint32 landing_ ) { state = state_; takeoff = takeoff_; search_for_stop = stopsearch_; touchdown = landing_; }

	// since we are drawing ourselves, we must mark ourselves dirty during deletion
	~air_vehicle_t();

	waytype_t get_waytype() const OVERRIDE { return air_wt; }

	// returns true for the way search to an unknown target.
	bool is_target(const grund_t *,const grund_t *) const OVERRIDE;

	// return valid direction
	ribi_t::ribi get_ribi(const grund_t* ) const OVERRIDE;

	// how expensive to go here (for way search)
	int get_cost(const grund_t *gr, const weg_t *w, const sint32 max_speed, ribi_t::ribi from) const OVERRIDE;

	bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8) OVERRIDE;

	void set_convoi(convoi_t *c) OVERRIDE;

	bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route) OVERRIDE;

	typ get_typ() const OVERRIDE { return air_vehicle; }

	schedule_t *generate_new_schedule() const OVERRIDE;

	void rdwr_from_convoi(loadsave_t *file) OVERRIDE;

	int get_flyingheight() const {return flying_height-get_hoff()-2;}

	// image: when flying empty, on ground the plane
	image_id get_image() const OVERRIDE {return !is_on_ground() ? IMG_EMPTY : image;}

	// image: when flying the shadow, on ground empty
	image_id get_outline_image() const OVERRIDE {return !is_on_ground() ? image : IMG_EMPTY;}

	// shadow has black color (when flying)
	FLAGGED_PIXVAL get_outline_colour() const OVERRIDE {return !is_on_ground() ? TRANSPARENT75_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_BLACK) : 0;}

#ifdef MULTI_THREAD
	// this draws the "real" aircrafts (when flying)
	void display_after(int xpos, int ypos, const sint8 clip_num) const OVERRIDE;

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
	void display_overlay(int xpos, int ypos) const OVERRIDE;
#else
	// this draws the "real" aircrafts (when flying)
	void display_after(int xpos, int ypos, bool dirty) const OVERRIDE;
#endif

	void calc_friction(const grund_t*) OVERRIDE {}

	bool is_on_ground() const { return flying_height==0  &&  !(state==circling  ||  state==flying); }

	const char *is_deletable(const player_t *player) OVERRIDE;

	bool is_flying() const OVERRIDE { return !is_on_ground(); }
};

#endif
