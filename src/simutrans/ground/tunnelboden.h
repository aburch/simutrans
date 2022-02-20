/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GROUND_TUNNELBODEN_H
#define GROUND_TUNNELBODEN_H


#include "boden.h"


class tunnelboden_t : public boden_t
{
protected:
	/// @copydoc boden_t::calc_image_internal
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	tunnelboden_t(loadsave_t *file, koord pos );
	tunnelboden_t(koord3d pos, slope_t::type slope_type) : boden_t(pos, slope_type) {}

public:
	/// @copydoc boden_t::rdwr
	void rdwr(loadsave_t *file) OVERRIDE;

	/// @copydoc boden_t::get_weg_hang
	slope_t::type get_weg_hang() const OVERRIDE { return ist_karten_boden() ? (slope_t::type)slope_t::flat : get_grund_hang(); }

	/// @copydoc boden_t::get_name
	const char *get_name() const OVERRIDE { return "Tunnelboden"; }

	/// @copydoc boden_t::get_typ
	typ get_typ() const OVERRIDE { return tunnelboden; }

	/// @copydoc boden_t::info
	void info(cbuffer_t & buf) const OVERRIDE;
};

#endif
