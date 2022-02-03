/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "monorailboden.h"

#include "../world/simworld.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../obj/way/weg.h"


monorailboden_t::monorailboden_t(koord3d pos,slope_t::type slope) : grund_t(pos)
{
	this->slope = slope;

	// update limits
	if(  welt->min_height > pos.z  ) {
		welt->min_height = pos.z;
	}
	else if(  welt->max_height < pos.z  ) {
		welt->max_height = pos.z;
	}
}


void monorailboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->is_version_less(88, 9)) {
		// save slope locally
		if(file->is_version_atleast(88, 6)) {
			uint8 sl;
			file->rdwr_byte(sl);
			// convert slopes from old single height saved game
			slope = (scorner_sw(sl) + scorner_se(sl) * 3 + scorner_ne(sl) * 9 + scorner_nw(sl) * 27) * env_t::pak_height_conversion_factor;
		}
		else {
			slope = grund_t::get_grund_hang();
		}
	}
}


void monorailboden_t::calc_image_internal(const bool calc_only_snowline_change)
{
	set_image(IMG_EMPTY);
	clear_back_image();

	weg_t *const weg = get_weg_nr(0);
	if(  weg  ) {
		if(  !calc_only_snowline_change  ) {
			weg->calc_image();
		}
		else {
			weg->check_season(false);
		}
	}
}
