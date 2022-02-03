/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_WATER_VEHICLE_H
#define VEHICLE_WATER_VEHICLE_H


#include "vehicle.h"


/**
 * A class for naval vehicles. Manages the look of the vehicles
 * and the navigability of tiles.
 * @see vehicle_t
 */
class water_vehicle_t : public vehicle_t
{
protected:
	// how expensive to go here (for way search)
	int get_cost(const grund_t *, const weg_t*, const sint32, ribi_t::ribi) const OVERRIDE { return 1; }

	void calc_friction(const grund_t *gr) OVERRIDE;

	bool check_next_tile(const grund_t *bd) const OVERRIDE;

	void enter_tile(grund_t*) OVERRIDE;

public:
	waytype_t get_waytype() const OVERRIDE { return water_wt; }

	bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8) OVERRIDE;

	// returns true for the way search to an unknown target.
	bool is_target(const grund_t *,const grund_t *) const OVERRIDE {return 0;}

	water_vehicle_t(loadsave_t *file, bool is_first, bool is_last);
	water_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv);

	obj_t::typ get_typ() const OVERRIDE { return water_vehicle; }

	schedule_t * generate_new_schedule() const OVERRIDE;
};


#endif
