/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GROUND_BODEN_H
#define GROUND_BODEN_H


#include "grund.h"

/**
 * boden_t are nature tiles (maybe with ways, powerlines, trees and beware: harbor buildings)
 */
class boden_t : public grund_t
{
protected:
	/// @copydoc grund_t::calc_image_internal
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	boden_t(loadsave_t *file, koord pos );
	boden_t(koord3d pos, slope_t::type slope);

public:
	/// @copydoc grund_t::rdwr
	void rdwr(loadsave_t *file) OVERRIDE;

	/// @copydoc grund_t::ist_natur
	bool ist_natur() const OVERRIDE {
		return (flags & (is_halt_flag | has_way1 | has_way2))==0;
// equivalen to		return !hat_wege()  &&  !is_halt();
	}

	/// @copydoc grund_t::get_name
	const char *get_name() const OVERRIDE;

	/// @copydoc grund_t::get_typ
	grund_t::typ get_typ() const OVERRIDE { return boden; }
};

#endif
