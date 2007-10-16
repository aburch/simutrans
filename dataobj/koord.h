#ifndef KOORD_H
#define KOORD_H

#include <cstdlib>
#include "ribi.h"
#include "../simtypes.h"

class loadsave_t;

/**
 * 2d Koordinaten
 */
class koord
{
public:
	sint16 x;
	sint16 y;

	koord() : x(0), y(0) {}

	koord(short xp, short yp) : x(xp), y(yp) {}
	koord(loadsave_t* file);
	koord(ribi_t::ribi ribi) { *this = from_ribi[ribi]; }
	koord(hang_t::typ hang)  { *this = from_hang[hang]; }

	void rdwr(loadsave_t *file);

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
		sint16 new_x = y_size-y;
		y = x;
		x = new_x;
	}

	static const koord invalid;
	static const koord nord;
	static const koord sued;
	static const koord ost;
	static const koord west;
	// die 4 Grundrichtungen als Array
	static const koord nsow[4];
	// 8 next neighbours
	static const koord neighbours[8];
	// 15 next nearest neightbours
	static const koord second_neighbours[16];

private:
	static const koord from_ribi[16];
	static const koord from_hang[16];
};


static inline koord operator * (const koord& k, const int m)
{
	return koord(k.x * m, k.y * m);
}


static inline unsigned abs_distance(koord a, koord b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}


static inline bool operator == (const koord& a, const koord& b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))==0;
}


static inline bool operator != (const koord & a, const koord & b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))!=0;
}


static inline koord operator + (const koord& a, const koord& b)
{
	return koord(a.x + b.x, a.y + b.y);
}


static inline koord operator - (const koord& a, const koord& b)
{
	return koord(a.x - b.x, a.y - b.y);
}


static inline koord operator - (const koord& a)
{
	return koord(-a.x, -a.y);
}

#endif
