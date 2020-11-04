/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef FINDER_BUILDING_PLACEFINDER_H
#define FINDER_BUILDING_PLACEFINDER_H


#include "placefinder.h"
#include "../simworld.h"

/**
 * building_placefinder_t:
 *
 * Search a free site for building using function find_place().
 */
class building_placefinder_t : public placefinder_t {
public:
	building_placefinder_t(karte_t *welt, sint16 radius = -1) : placefinder_t(welt, radius) {}

	bool is_area_ok(koord pos, sint16 b, sint16 h, climate_bits cl, uint16 allowed_regions) const OVERRIDE {
		return welt->square_is_free(pos, b, h, NULL, cl, allowed_regions);
	}
};

#endif
