#ifndef KOORD_H
#define KOORD_H

#include <cstdlib>
#include <assert.h>

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

	const char *get_str() const;

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


static inline uint32 abs_distance(const koord &a, const koord &b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
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

static inline bool operator <= (const koord & a, const koord & b)
{
	return (a.x == b.x) ? (a.y <= b.y) : (a.x < b.x);
}

static inline koord operator - (const koord &a, const koord &b)
{
	return koord(a.x - b.x, a.y - b.y);
}


static inline koord operator - (const koord &a)
{
	return koord(-a.x, -a.y);
}

/**
 * Ordering based on relative distance to a fixed point `origin'.
 */
class RelativeDistanceOrdering
{
private:
	const koord m_origin;
public:
	RelativeDistanceOrdering(const koord& origin)
		: m_origin(origin)
	{ /* nothing */ }

	/**
	 * Returns true if `a' is closer to the origin than `b', otherwise false.
	 */
	bool operator()(const koord& a, const koord& b) const
	{
		return abs_distance(m_origin, a) < abs_distance(m_origin, b);
	}
};

#endif
