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
	sint16 z;

	koord3d() : x(0), y(0), z(0) {}

	koord3d(short xp, short yp, short zp) : x(xp), y(yp), z(zp) {}
	koord3d(koord xyp, short zp) : x(xyp.x), y(xyp.y), z(zp) {}
	koord3d(loadsave_t* file);

	void rdwr(loadsave_t* file);

	void* operator new(size_t s);
	void operator delete(void* p);

	static const koord3d invalid;

	koord gib_2d() const { return koord(x, y); }

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
	return a.x == b.x && a.y == b.y && a.z == b.z;
}


static inline bool operator != (const koord3d& a, const koord3d& b)
{
	return a.x != b.x || a.y != b.y || a.z != b.z;
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
