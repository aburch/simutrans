/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_monorail_h
#define boden_wege_monorail_h


#include "schiene.h"


/**
 * Class for monorail tracks, derived from schiene.
 * Monorail trains can drive on this tracks.
 * Each track belongs to a section block
 *
 * @author Hj. Malthaner
 */
class monorail_t : public schiene_t
{
public:
	static const way_desc_t *default_monorail;

	monorail_t() : schiene_t() { set_desc(default_monorail); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	monorail_t(loadsave_t *file);

	waytype_t get_waytype() const OVERRIDE {return monorail_wt;}

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
