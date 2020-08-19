/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_MONORAIL_H
#define BODEN_WEGE_MONORAIL_H


#include "schiene.h"


/**
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 */
class monorail_t : public schiene_t
{
public:
	static const way_desc_t *default_monorail;

	monorail_t() : schiene_t(monorail_wt) { set_desc(default_monorail); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	monorail_t(loadsave_t *file);

	//virtual waytype_t get_waytype() const {return monorail_wt;}

	void rdwr(loadsave_t *file);
};

#endif
