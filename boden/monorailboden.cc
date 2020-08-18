#include "monorailboden.h"

#include "../simworld.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "wege/weg.h"


monorailboden_t::monorailboden_t(koord3d pos,slope_t::type slope) : grund_t(pos)
{
	this->slope = slope;
}


void monorailboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		// save slope locally
		if(file->get_version()>88005) {
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
