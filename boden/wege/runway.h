/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_RUNWAY_H
#define BODEN_WEGE_RUNWAY_H


#include "schiene.h"


/**
 * Class for aiport runaway in Simutrans.
 * speed >250 are for take of (maybe rather use system type in next release?)
 */
class runway_t : public schiene_t
{
public:
	static const way_desc_t *default_runway;

	/**
	 * File loading constructor.
	 */
	runway_t(loadsave_t *file);

	runway_t();

	//inline waytype_t get_waytype() const OVERRIDE {return air_wt;}

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
