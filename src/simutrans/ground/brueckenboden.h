/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_BRUECKENBODEN_H
#define BODEN_BRUECKENBODEN_H


#include "grund.h"


class brueckenboden_t : public grund_t
{
private:
	slope_t::type weg_hang; ///< for e.g. ramps

protected:
	/// @copydoc grund_t::calc_image_internal
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	brueckenboden_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) ) { rdwr(file); }
	brueckenboden_t(koord3d pos, slope_t::type grund_hang, slope_t::type weg_hang);

public:
	/// @copydoc grund_t::rdwr
	void rdwr(loadsave_t *file) OVERRIDE;

	/// @copydoc grund_t::rotate90
	void rotate90() OVERRIDE;

	/// @copydoc grund_t::get_weg_yoff
	sint8 get_weg_yoff() const OVERRIDE;

	/// @copydoc grund_t::get_weg_hang
	slope_t::type get_weg_hang() const OVERRIDE { return weg_hang; }

	/// @copydoc grund_t::get_name
	const char *get_name() const OVERRIDE { return "Brueckenboden"; }

	/// @copydoc grund_t::get_typ
	typ get_typ() const OVERRIDE { return brueckenboden; }

	/// @copydoc grund_t::info
	void info(cbuffer_t & buf) const OVERRIDE;
};

#endif
