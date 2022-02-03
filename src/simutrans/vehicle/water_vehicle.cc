/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "water_vehicle.h"

#include "../simware.h"
#include "../builder/goods_manager.h"
#include "../builder/vehikelbauer.h"
#include "../obj/crossing.h"
#include "../dataobj/schedule.h"
#include "../obj/roadsign.h"
#include "../simconvoi.h"


water_vehicle_t::water_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cn) :
	vehicle_t(pos, desc, player)
{
	cnv = cn;
}


water_vehicle_t::water_vehicle_t(loadsave_t *file, bool is_first, bool is_last) : vehicle_t()
{
	vehicle_t::rdwr_from_convoi(file);

	if(  file->is_loading()  ) {
		static const vehicle_desc_t *last_desc = NULL;

		if(is_first) {
			last_desc = NULL;
		}
		// try to find a matching vehicle
		if(desc==NULL) {
			dbg->warning("water_vehicle_t::water_vehicle_t()", "try to find a fitting vehicle for %s.", !fracht.empty() ? fracht.front().get_name() : "passengers");
			desc = vehicle_builder_t::get_best_matching(water_wt, 0, fracht.empty() ? 0 : 30, 100, 40, !fracht.empty() ? fracht.front().get_desc() : goods_manager_t::passengers, true, last_desc, is_last );
			if(desc) {
				calc_image();
			}
		}
		// update last desc
		if(  desc  ) {
			last_desc = desc;
		}
	}
}


void water_vehicle_t::enter_tile(grund_t* gr)
{
	vehicle_t::enter_tile(gr);

	if(  weg_t *ch = gr->get_weg(water_wt)  ) {
		// we are in a channel, so book statistics
		ch->book(get_total_cargo(), WAY_STAT_GOODS);
		if (leading)  {
			ch->book(1, WAY_STAT_CONVOIS);
		}
	}
}


bool water_vehicle_t::check_next_tile(const grund_t *bd) const
{
	if(  bd->is_water()  ) {
		return true;
	}
	// channel can have more stuff to check
	const weg_t *w = bd->get_weg(water_wt);
#ifdef ENABLE_WATERWAY_SIGNS
	if(  w  &&  w->has_sign()  ) {
		const roadsign_t* rs = bd->find<roadsign_t>();
		if(  rs->get_desc()->get_wtyp()==get_waytype()  ) {
			if(  cnv !=NULL  &&  rs->get_desc()->get_min_speed() > 0  &&  rs->get_desc()->get_min_speed() > cnv->get_min_top_speed()  ) {
				// below speed limit
				return false;
			}
			if(  rs->get_desc()->is_private_way()  &&  (rs->get_player_mask() & (1<<get_player_nr()) ) == 0  ) {
				// private road
				return false;
			}
		}
	}
#endif
	return (w  &&  w->get_max_speed()>0);
}


/** Since slopes are handled different for ships
 */
void water_vehicle_t::calc_friction(const grund_t *gr)
{
	// or a hill?
	if(gr->get_weg_hang()) {
		// hill up or down => in lock => decelerate
		current_friction = 16;
	}
	else {
		// flat track
		current_friction = 1;
	}

	if(previous_direction != direction) {
		// curve: higher friction
		current_friction *= 2;
	}
}


bool water_vehicle_t::can_enter_tile(const grund_t *gr, sint32 &restart_speed, uint8)
{
	restart_speed = -1;

	if(leading) {

		assert(gr);
		if(  gr->get_top()>251  ) {
			// too many ships already here ..
			return false;
		}
		weg_t *w = gr->get_weg(water_wt);
		if(w  &&  w->is_crossing()) {
			// ok, here is a draw/turn-bridge ...
			crossing_t* cr = gr->find<crossing_t>();
			if(!cr->request_crossing(this)) {
				restart_speed = 0;
				return false;
			}
		}
	}
	return true;
}


schedule_t * water_vehicle_t::generate_new_schedule() const
{
	return new ship_schedule_t();
}

