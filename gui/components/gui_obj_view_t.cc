#include "gui_obj_view_t.h"
#include "../../vehicle/simvehikel.h"


obj_view_t::obj_view_t(obj_t const* d, scr_size const size) :
	world_view_t(),
	obj(d)
{
	set_size(size);
}


void obj_view_t::set_size(scr_size size)
{
	sint16 max_dy_off = 5;
	if(  obj  ) {
		aircraft_t const* const plane = obj_cast<aircraft_t>(obj);
		if(  plane  ) {
			max_dy_off = 11;
		}
	}

	gui_komponente_t::set_size(size);
	world_view_t::calc_offsets(size, max_dy_off);
}


koord3d obj_view_t::get_location()
{
	return obj->get_pos();
}
