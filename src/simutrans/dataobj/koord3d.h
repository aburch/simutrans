/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_KOORD3D_H
#define DATAOBJ_KOORD3D_H


#include "koord.h"
#include "ribi.h"
#include "../simtypes.h"
#include "../tpl/vector_tpl.h"


/**
 * 3D Coordinates
 */
class koord3d
{
public:
	sint16 x;
	sint16 y;
	sint8 z;

	koord3d() : x(0), y(0), z(0) {}

	koord3d(sint16 xp, sint16 yp, sint8 zp) : x(xp), y(yp), z(zp) {}
	koord3d(koord xyp, sint8 zp) : x(xyp.x), y(xyp.y), z(zp) {}

	const char *get_str() const;
	const char *get_fullstr() const; // including brackets

	void rotate90( sint16 y_diff );

	void rdwr(loadsave_t* file);

	static const koord3d invalid;

	koord get_2d() const { return koord(x, y); }

	const koord3d& operator += (const koord3d& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}

	const koord3d& operator -= (koord3d& a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
		return *this;
	}

	const koord3d& operator += (const koord& a)
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	const koord3d& operator -= (const koord& a)
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}
} GCC_PACKED;


static inline koord3d operator + (const koord3d& a, const koord3d& b)
{
	return koord3d(a.x + b.x, a.y + b.y, a.z + b.z);
}


static inline koord3d operator - (const koord3d& a, const koord3d& b)
{
	return koord3d(a.x - b.x, a.y - b.y, a.z - b.z);
}


static inline bool operator == (const koord3d& a, const koord3d& b)
{
//	return a.x == b.x && a.y == b.y && a.z == b.z;
	return ((a.x-b.x)|(a.y-b.y)|(a.z-b.z))==0;
}


static inline bool operator != (const koord3d& a, const koord3d& b)
{
//	return a.x != b.x || a.y != b.y || a.z != b.z;
	return ((a.x-b.x)|(a.y-b.y)|(a.z-b.z))!=0;
}


static inline koord3d operator - (const koord3d& a)
{
	return koord3d(-a.x, -a.y, -a.z);
}


static inline koord3d operator + (const koord3d& a, const koord& b)
{
	return koord3d(a.x + b.x, a.y + b.y, a.z);
}


static inline koord3d operator - (const koord3d& a, const koord& b)
{
	return koord3d(a.x - b.x, a.y - b.y, a.z);
}


static inline uint32 koord_distance(koord3d a, koord b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline uint32 koord_distance(koord a, koord3d b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline uint32 koord_distance(koord3d a, koord3d b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}

/**
 * This class defines a vector_tpl<koord3d> with some
 * helper functions
 */
class koord3d_vector_t : public vector_tpl< koord3d > {
public:
	// computes ribi at position i
	ribi_t::ribi get_ribi( uint32 index ) const;
	// computes ribi at position i only if distance to previous/next is not larger than 1
	ribi_t::ribi get_short_ribi( uint32 index ) const;
	void rotate90( sint16 );
};

#endif
