#ifndef KOORD_H
#define KOORD_H

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
	sint16 x;
	sint16 y;

	koord() : x(0), y(0) {}

	koord(short xp, short yp) : x(xp), y(yp) {}
	koord(loadsave_t* file);
	koord(ribi_t::ribi ribi) { *this = from_ribi[ribi]; }
	koord(slope_t::type hang)  { *this = from_hang[hang]; }

	// use this instead of koord(simrand(x),simrand(y)) to avoid
	// different order on different compilers
	static koord koord_random(uint16 xrange, uint16 yrange);

	void rdwr(loadsave_t *file);

	const char *get_str() const;
	const char *get_fullstr() const;	// including brackets

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
	static const koord nsew[4];
	// 8 next neighbours
	static const koord neighbours[8];
	// 15 next nearest neightbours
	static const koord second_neighbours[16];

private:
	static const koord from_ribi[16];
	static const koord from_hang[81];
};

//static inline uint32 int_sqrt(const uint32 num) 
//{
//    if (0 == num) 
//	{ 
//		// Avoid zero divide
//		return 0; 
//	}  
//    uint32 n = (num / 2) + 1;       // Initial estimate, never low
//    uint32 n1 = (n + (num / n)) / 2;
//    while (n1 < n) 
//	{
//        n = n1;
//        n1 = (n + (num / n)) / 2;
//    }
//    return n;
//}

//static inline uint32 accurate_distance(const koord &a, const koord &b)
//{
//	// Euclidian distance
//	const sint32 delta_x = (sint32)a.x - (sint32)b.x;
//	const sint32 delta_y = (sint32)a.y - (sint32)b.y;
//	return int_sqrt( delta_x * delta_x + delta_y * delta_y );
//}


// Knightly : shortest distance in cardinal (N, E, S, W) and ordinal (NE, SE, SW, NW) directions
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

static inline uint32 koord_distance(const koord &a, const koord &b)
{
	// Manhattan distance
	return abs(a.x - b.x) + abs(a.y - b.y);
}

// Knightly : multiply the value by the distance weight
static inline uint32 weight_by_distance(const uint32 value, const uint32 distance)
{
	return (uint32)( ((sint64)value << 10) / (sint64)(distance < 4u ? 4u : distance) );
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

static inline bool operator == (const koord& a, int b)
{
	// For hashtable use.
	return b == 0 && a == koord::invalid;
}


///**
// * Ordering based on relative distance to a fixed point `origin'.
// */
//class RelativeDistanceOrdering
//{
//private:
//	const koord m_origin;
//public:
//	RelativeDistanceOrdering(const koord& origin)
//		: m_origin(origin)
//	{ /* nothing */ }
//
//	/**
//	 * Returns true if `a' is closer to the origin than `b', otherwise false.
//	 */
//	bool operator()(const koord& a, const koord& b) const
//	{
//		return abs_distance(m_origin, a) < abs_distance(m_origin, b);
//	}
//};

#endif
