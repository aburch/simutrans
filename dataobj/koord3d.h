#ifndef KOORD3D_H
#define KOORD3D_H

#include <stdlib.h>
#include "../simtypes.h"
#include "koord.h"


/**
 * 3d Koordinaten
 */
class koord3d
{
public:
	sint16 x;
	sint16 y;
	sint8 z;

	koord3d() : x(0), y(0), z(0) {}

	const char *get_str() const;

	koord3d(sint16 xp, sint16 yp, sint8 zp) : x(xp), y(yp), z(zp) {}
	koord3d(koord xyp, sint8 zp) : x(xyp.x), y(xyp.y), z(zp) {}
	koord3d(loadsave_t* file);

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


static inline int koord_distance(koord a, koord b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline int koord_distance(koord3d a, koord b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline int koord_distance(koord a, koord3d b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline int koord_distance(koord3d a, koord3d b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}

#endif
