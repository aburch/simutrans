/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_obj_view.h"

#include "../../vehicle/air_vehicle.h"


obj_view_t::obj_view_t(obj_t const* d, scr_size const size) :
	world_view_t(size),
	obj(d)
{
	set_size(size);
}


void obj_view_t::set_size(scr_size size)
{
	sint16 max_dy_off = 5;
	if(  obj  ) {
		air_vehicle_t const* const plane = obj_cast<air_vehicle_t>(obj);
		if(  plane  ) {
			max_dy_off = 11;
		}
	}

	gui_component_t::set_size(size);
	world_view_t::calc_offsets(size, max_dy_off);
}


koord3d obj_view_t::get_location()
{
	return obj->get_pos();
}
