/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_runway_h
#define boden_wege_runway_h


#include "weg.h"


/**
 * Klasse für runwayn in Simutrans.
 * speed >250 are for take of (maybe rather use system type in next release?)
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class runway_t : public weg_t
{
public:
	static const weg_besch_t *default_runway;

	/**
	 * File loading constructor.
	 *
	 * @author Hj. Malthaner
	 */
	runway_t(karte_t *welt, loadsave_t *file);

	runway_t(karte_t *welt);

	/**
	 * Calculates the image of this pice of runway
	 */
	virtual void calc_bild(koord3d) { weg_t::calc_bild(); }

	inline waytype_t gib_waytype() const {return air_wt;}

	void rdwr(loadsave_t *file);
};

#endif
