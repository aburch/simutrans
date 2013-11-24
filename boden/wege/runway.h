/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_runway_h
#define boden_wege_runway_h


#include "schiene.h"


/**
 * Klasse für runwayn in Simutrans.
 * speed >250 are for take of (maybe rather use system type in next release?)
 *
 * @author Hj. Malthaner
 */
class runway_t : public schiene_t
{
public:
	static const weg_besch_t *default_runway;

	/**
	 * File loading constructor.
	 *
	 * @author Hj. Malthaner
	 */
	runway_t(loadsave_t *file);

	runway_t();

	inline waytype_t get_waytype() const {return air_wt;}

	void rdwr(loadsave_t *file);
};

#endif
