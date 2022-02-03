/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "simworld.h"
#include "../simintr.h"
#include "../simtypes.h"
#include "placefinder.h"

pos_list_t::pos_list_t(sint16 max_radius)
{
	this->max_radius = max_radius + 1;
	columns = new sint16[this->max_radius];

	restart();
}

pos_list_t::~pos_list_t()
{
	delete [] columns;
}


void pos_list_t::restart()
{
	radius = 1;
	columns[0] = 0;
	row = 0;
	quadrant = 0;
}


sint16 pos_list_t::find_best_row()
{
	sint16 best_dist = -1;
	sint16 best_row = -1;

	for(sint16 i = 0; i < radius; i++) {
		if(columns[i] < radius) {
			int dist = (int)i * i + columns[i] * columns[i];

			if(best_dist == -1 || dist < best_dist) {
				best_dist = dist;
				best_row = i;
			}
		}
	}
	if(radius < max_radius && best_dist > radius * radius) {
		return -1;
	} else {
		return best_row;
	}
}


bool pos_list_t::get_next_pos(koord &k)
{
	if(row != -1) {
		if(quadrant++ == 4) {
			quadrant = 1;

			columns[row]++;

			row = find_best_row();
			if(row == -1) {
				if(radius < max_radius) {
					columns[radius++] = 0;
				}
				row = find_best_row();
			}
		}
	}
	if(row != -1) {
		if(quadrant == 1 && !row) {
			quadrant += 2; // skip second 0, +/-y
		}
		if(quadrant % 2 == 1 && !columns[row]) {
			quadrant ++; // skip second +/-x, 0
		}
		return get_pos(k);
	}
	return false;
}


bool pos_list_t::get_pos(koord &k)
{
	if(row != -1) {
		switch(quadrant) {
			case 1:
				k = koord(row, columns[row]);
				return true;
			case 2:
				k = koord(row, (short)-columns[row]);
				return true;
			case 3:
				k = koord((short)-row, columns[row]);
				return true;
			case 4:
				k = koord(-row, -columns[row]);
				return true;
		}
	}
	return false;
}


pos_list_wh_t::pos_list_wh_t(sint16 max_radius, sint16 w, sint16 h) :
	pos_list_t(max_radius)
{
	restart(w, h);
}


void pos_list_wh_t::restart(sint16 w, sint16 h)
{
	this->w = w;
	this->h = h;
	dx = dy = 0;
	pos_list_t::restart();
}


bool pos_list_wh_t::get_next_pos(koord &k)
{
	get_pos(k);

	if(k.x == 0 && k.y == 0 && (dx > 0 || dy > 0)) {
		if(dx > 0) {
			dx--;
		}
		else if(dy > 0) {
			dy--;
			dx = w - 1;
		}
		k.x -= dx;
		k.y -= dy;
	}
	else if(dx > 0) {
		k.x -= --dx;
		if(k.y <= 0) {
			k.y -= h - 1;
		}
	}
	else if(dy > 0) {
		k.y -= --dy;
		if(k.x <= 0) {
			k.x -= w - 1;
		}
	}
	else {
		if(pos_list_t::get_next_pos(k)) {
			if(k.y == 0) {
				dy = h - 1;
			}
			if(k.x == 0) {
				dx = w - 1;
			}
			if(k.y <= 0) {
				k.y -= h - 1;
			}
			if(k.x <= 0) {
				k.x -= w - 1;
			}
		}
		else {
			return false;
		}
	}
	return true;
}

/*
 * Checks if all tiles are suitable for the building to be built
 * Includes boundary tiles around the building
 */
bool placefinder_t::is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const
{
	if(!welt->is_within_limits(pos)) {
		return false;
	}
	koord k(w, h);

	while(k.y-- > 0) {
		k.x = w;
		while(k.x-- > 0) {
			if(!is_tile_ok(pos, k, cl)) {
				return false;
			}
		}
	}
	return true;
}

/*
 * Checks if the tile is part of the building or if it's a boundary tile
 * @return false, when tile is part of the building
 * @return true, when tile is the boundary tile of the building
 */
bool placefinder_t::is_boundary_tile(koord d) const
{
	return d.x == 0 || d.x == w - 1 || d.y == 0 || d.y == h - 1;
}

/*
 * Checks if the tile is suitable for building, used by is_area_ok()
 * This function is replaced for building types that require rules
 */
bool placefinder_t::is_tile_ok(koord /*pos*/, koord /*d*/, climate_bits /*cl*/) const
{
	return true;
}

koord placefinder_t::find_place(koord start, sint16 w, sint16 h, climate_bits cl, bool *r)
{
	sint16 radius = max_radius > 0 ? max_radius : welt->get_size_max();
	pos_list_wh_t results_straight(radius, w, h);

	this->w = w;
	this->h = h;

	koord rel1, rel2;

	if((r && *r) && w != h) {
		//
		// Here we also search for the rotated positions.
		//
		pos_list_wh_t results_rotated(radius, h, w);

		if(is_area_ok(start, w, h, cl)) {
			*r = false;
			return start;
		}
		if(is_area_ok(start, h, w, cl)) {
			*r = true;
			return start;
		}
		while(results_straight.get_next_pos(rel1) && results_rotated.get_next_pos(rel2)) {
			if(is_area_ok(start + rel1, w, h, cl)) {
				*r = false;
				return start + rel1;
			}
			if(is_area_ok(start + rel2, h, w, cl)) {
				*r = true;
				return start + rel2;
			}
			INT_CHECK("simcity 1313");
		}
	}
	else {
		//
		// Search without rotated positions.
		//
		if(is_area_ok(start, w, h, cl)) {
			return start;
		}
		while(results_straight.get_next_pos(rel1)) {
			if(is_area_ok(start + rel1, w, h, cl)) {
				if(r) {
					*r = false;
				}
				return start + rel1;
			}
			INT_CHECK("simcity 1314");
		}
	}
	return koord::invalid;
}
