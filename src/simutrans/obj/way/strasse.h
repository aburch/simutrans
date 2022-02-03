/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_STRASSE_H
#define BODEN_WEGE_STRASSE_H


#include "weg.h"

/**
 * Cars are able to drive on roads.
 */
class strasse_t : public weg_t
{
public:
	static const way_desc_t *default_strasse;

	strasse_t(loadsave_t *file);
	strasse_t();

	inline waytype_t get_waytype() const OVERRIDE {return road_wt;}

	void set_gehweg(bool janein);

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
