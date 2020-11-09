/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_BODEN_H
#define BODEN_BODEN_H


#include "grund.h"

/**
 * boden_t are nature tiles (maybe with ways, powerlines, trees and beware: harbor buildings)
 */
class boden_t : public grund_t
{
protected:
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	boden_t(loadsave_t *file, koord pos );
	boden_t(koord3d pos, slope_t::type slope);

	void rdwr(loadsave_t *file) OVERRIDE;

	inline bool ist_natur() const OVERRIDE { return !hat_wege()  &&  !is_halt(); }

	const char *get_name() const OVERRIDE;

	grund_t::typ get_typ() const OVERRIDE {return boden;}
};

#endif
