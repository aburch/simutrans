/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simconst.h"

#include "../descriptor/ground_desc.h"
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(loadsave_t *file, koord pos) :
	grund_t(koord3d(pos,0))
{
	rdwr(file);
	slope = slope_t::flat;
}


fundament_t::fundament_t(koord3d pos, slope_t::type hang) :
	grund_t(pos)
{
	set_image( IMG_EMPTY );
	if(hang != slope_t::flat) {
		pos = get_pos();
		pos.z += slope_t::max_diff(hang);
		set_pos( pos );
	}
	slope = slope_t::flat;
}


void fundament_t::calc_image_internal(const bool calc_only_snowline_change)
{
	set_image( ground_desc_t::get_ground_tile(this) );

	if(  !calc_only_snowline_change  ) {
		grund_t::calc_back_image( get_disp_height(), slope_t::flat );
	}
}
