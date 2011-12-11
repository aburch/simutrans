#include "gui_ding_view_t.h"
#include "../../vehicle/simvehikel.h"


ding_view_t::ding_view_t(ding_t const* d, koord const size) :
	world_view_t(d->get_welt()),
	ding(d)
{
	set_groesse(size);
}


void ding_view_t::set_groesse(koord size)
{
	sint16 max_dy_off = 5;
	if(  ding  ) {
		aircraft_t const* const plane = ding_cast<aircraft_t>(ding);
		if(  plane  ) {
			max_dy_off = 11;
		}
	}

	gui_komponente_t::set_groesse(size);
	world_view_t::calc_offsets(size, max_dy_off);
}


koord3d ding_view_t::get_location()
{
	return ding->get_pos();
}
