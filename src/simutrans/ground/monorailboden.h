/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GROUND_MONORAILBODEN_H
#define GROUND_MONORAILBODEN_H


#include "grund.h"

class monorailboden_t : public grund_t
{
protected:
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	monorailboden_t(loadsave_t *file, koord pos) : grund_t( koord3d(pos,0) ) { rdwr(file); }
	monorailboden_t(koord3d pos, slope_t::type slope);

public:
	/// @copydoc grund_t::rdwr
	void rdwr(loadsave_t *file) OVERRIDE;

	/// @copydoc grund_t::get_name
	const char *get_name() const OVERRIDE { return "Monorailboden"; }

	/// @coypdoc grund_t::get_typ
	typ get_typ() const OVERRIDE { return monorailboden; }
};

#endif
