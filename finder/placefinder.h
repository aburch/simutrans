/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 *
 * Author: V. Meyer
 */

#ifndef __PLACEFINDER_H
#define __PLACEFINDER_H

#include "../dataobj/koord.h"
class world_t;

/**
 * pos_list_t:
 *
 * Provides a list of all coordinates from (0, 0) sorted by distance in increasing
 * order with x and y in range [-max_xy; max_xy] except for (0, 0) itself.
 * (0, 0) is used as the end marker.
 *
 * @author V. Meyer
 */
class pos_list_t {
	sint16 max_radius;
	sint16 *columns;
	sint16 radius;
	sint16 row;
	sint16 quadrant;

	sint16 find_best_row();

public:
	/**
	* @param max_xy (Maximum value for x and y position)
	*
	* @author V. Meyer
	*/
	pos_list_t(sint16 max_xy);
	virtual ~pos_list_t();

	void restart();
	bool get_pos(koord &k);
	virtual bool get_next_pos(koord &k);
};


/**
 * pos_list_wh_t:
 *
 * Extended version of pos_list_t. Provides surrounding positions
 * for a range of size h by w.
 * (0, 0) is used as the end marker again.
 *
 * @author V. Meyer
 */
class pos_list_wh_t : public pos_list_t {
	sint16 w;
	sint16 h;

	sint16 dx;
	sint16 dy;
public:
	pos_list_wh_t(sint16 max_radius, sint16 w, sint16 h);

	void restart(sint16 w, sint16 h);
	void restart() { pos_list_t::restart(); }

	bool get_next_pos(koord &k);
};

/**
 * @author V. Meyer
 */
class placefinder_t {
protected:
	world_t *welt;
	sint16 w;
	sint16 h;
	sint16 max_radius;

	virtual bool is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const;

	virtual bool is_tile_ok(koord pos, koord d, climate_bits cl) const;

	bool is_boundary_tile(koord d) const;

	placefinder_t(world_t *welt, sint16 _max_radius = - 1) { this->welt = welt; max_radius = _max_radius; }
	virtual ~placefinder_t() {}
public:
	koord find_place(koord start, sint16 w, sint16 h, climate_bits cl, bool *r = NULL);
};

#endif
