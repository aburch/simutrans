/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_KOORD_H
#define DATAOBJ_KOORD_H


#include "ribi.h"
#include "../simtypes.h"

#include <stdlib.h>

class loadsave_t;

/**
 * 2D Coordinates
 */
class koord
{
public:
	// this is set by einstelugen_t
	static uint32 locality_factor;

	sint16 x;
	sint16 y;

	koord() : x(0), y(0) {}

	koord(sint16 xp, sint16 yp) : x(xp), y(yp) {}
	koord(ribi_t::ribi ribi) { *this = from_ribi[ribi]; }
	koord(slope_t::type slope) { *this = from_hang[slope]; }

	// use this instead of koord(simrand(x),simrand(y)) to avoid
	// different order on different compilers
	static koord koord_random(uint16 xrange, uint16 yrange);

	void rdwr(loadsave_t *file);

	const char *get_str() const;
	const char *get_fullstr() const; // including brackets

	const koord& operator += (const koord & k)
	{
		x += k.x;
		y += k.y;
		return *this;
	}

	const koord& operator -= (const koord & k)
	{
		x -= k.x;
		y -= k.y;
		return *this;
	}

	void rotate90( sint16 y_size )
	{
		if(  (x&y)<0  ) {
			// do not rotate illegal coordinates
			return;
		}
		sint16 new_x = y_size-y;
		y = x;
		x = new_x;
	}

	inline void clip_min( koord k_min )
	{
		if (x < k_min.x) {
			x = k_min.x;
		}
		if (y < k_min.y) {
			y = k_min.y;
		}
	}

	inline void clip_max( koord k_max )
	{
		if (x > k_max.x) {
			x = k_max.x;
		}
		if (y > k_max.y) {
			y = k_max.y;
		}
	}

	static const koord invalid;
	static const koord north;
	static const koord south;
	static const koord east;
	static const koord west;
	// the 4 basic directions as an Array
	static const koord nesw[4];
	// 8 next neighbours
	static const koord neighbours[8];

private:
	static const koord from_ribi[16];
	static const koord from_hang[81];
};


static inline uint32 koord_distance(const koord &a, const koord &b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}

// shortest distance in cardinal (N, E, S, W) and ordinal (NE, SE, SW, NW) directions
static inline uint32 shortest_distance(const koord &a, const koord &b)
{
	const uint32 x_offset = abs(a.x - b.x);
	const uint32 y_offset = abs(a.y - b.y);
	// square root of 2 is estimated by 181/128; 64 is for rounding
	if(  x_offset>=y_offset  ) {
		return (x_offset - y_offset) + ( ((y_offset * 181u) + 64u) >> 7 );
	}
	else {
		return (y_offset - x_offset) + ( ((x_offset * 181u) + 64u) >> 7 );
	}
}

// multiply the value by the distance weight
static inline uint32 weight_by_distance(const sint32 value, const uint32 distance)
{
	return value<=0 ? 0 : 1+(uint32)( ( ((sint64)value<<8) * koord::locality_factor ) / ( (sint64)koord::locality_factor + (sint64)(distance < 4u ? 4u : distance) ) );
}

static inline koord operator * (const koord &k, const sint16 m)
{
	return koord(k.x * m, k.y * m);
}


static inline koord operator / (const koord &k, const sint16 m)
{
	return koord(k.x / m, k.y / m);
}


static inline bool operator == (const koord &a, const koord &b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))==0;
}


static inline bool operator != (const koord &a, const koord &b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))!=0;
}


static inline koord operator + (const koord &a, const koord &b)
{
	return koord(a.x + b.x, a.y + b.y);
}


static inline koord operator - (const koord &a, const koord &b)
{
	return koord(a.x - b.x, a.y - b.y);
}


static inline koord operator - (const koord &a)
{
	return koord(-a.x, -a.y);
}
#endif
