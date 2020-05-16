/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_BODEN_H
#define BODEN_BODEN_H


#include "grund.h"

/**
 * boden_t are nature tiles (maybe with ways, powerlines, trees and beware: harbor buildings)
 *
 * @author Hj. Malthaner
 */

class boden_t : public grund_t
{
protected:
	virtual void calc_image_internal(const bool calc_only_snowline_change);

public:
	boden_t(loadsave_t *file, koord pos );
	boden_t(koord3d pos, slope_t::type slope);

	virtual void rdwr(loadsave_t *file);

	inline bool ist_natur() const { return !hat_wege()  &&  !is_halt(); }

	const char *get_name() const;

	grund_t::typ get_typ() const {return boden;}
};

#endif
