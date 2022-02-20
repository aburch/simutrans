/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_WAY_MONORAIL_H
#define OBJ_WAY_MONORAIL_H


#include "schiene.h"


/**
 * Class for monorail tracks, derived from schiene.
 * Monorail trains can drive on this tracks.
 * Each track belongs to a section block
 */
class monorail_t : public schiene_t
{
public:
	static const way_desc_t *default_monorail;

	monorail_t() : schiene_t() { set_desc(default_monorail); }

	/**
	 * File loading constructor.
	 */
	monorail_t(loadsave_t *file);

	waytype_t get_waytype() const OVERRIDE {return monorail_wt;}

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
