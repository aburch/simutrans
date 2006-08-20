/*
 * koord3d.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef koord3d_h
#define koord3d_h

#include <stdlib.h>

#ifndef koord_h
#include "koord.h"
#endif

class mempool_t;

/**
 * 3d koordinaten
 *
 * @author Hj. Malthaner
 */
class koord3d
{
private:
    static mempool_t *mempool;

public:
    // Hajo: 6-byte variant, bad for alignment :(
    // short x, y, z;

    signed int x : 11;
    signed int y : 11;
    signed int z : 10;

    // 6 byte, clear code
    // koord3d() {x=y=z=0;};

    // Hajo: performance hack
    koord3d() {/*x=y=z=0;*/  *((int *)this) = 0;};

    koord3d(short xp, short yp, short zp) {x=xp; y=yp; z=zp;};
    koord3d(koord xyp, short zp) {x=xyp.x; y=xyp.y; z=zp;};
    koord3d(loadsave_t *file);

    void rdwr(loadsave_t *file);

    void * operator new(size_t s);
    void operator delete(void *p);

    static const koord3d invalid;

    koord gib_2d() const { return koord(x, y); }
};

/**
 * operatoren für 3d Koordinaten:
 * @author V. Meyer
 */

inline koord3d operator+ (const koord3d & a, const koord3d & b)
{
    return koord3d(a.x+b.x, a.y+b.y, a.z+b.z);
}

inline koord3d operator- (const koord3d & a, const koord3d & b)
{
    return koord3d(a.x-b.x, a.y-b.y, a.z-b.z);
}

inline const koord3d& operator+= (koord3d & a, const koord3d & b)
{
    a.x+=b.x;
    a.y+=b.y;
    a.z+=b.z;
    return a;
}

inline const koord3d& operator-= (koord3d & a, const koord3d & b)
{
    a.x-=b.x;
    a.y-=b.y;
    a.z-=b.z;
    return a;
}

inline bool operator== (const koord3d & a, const koord3d &b)
{
  // return (a.x == b.x && a.y == b.y && a.z == b.z);

  // Hajo: dirty trick to speed things up
  return *((const int*)&a) == *((const int*)&b);
}

inline bool operator!= (const koord3d & a, const koord3d & b)
{
  // return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);

  // Hajo: dirty trick to speed things up
  return *((const int*)&a) != *((const int*)&b);
}

inline koord3d operator- (koord3d & a)
{
    return koord3d(-a.x, -a.y, -a.z);
}

/**
 * Operatoren, die 3d mit 2d Koordinaten verheiraten:
 * die Operatoren ermöglichen die Arithmetik 3d = 3d op 2d;
 *
 * @author V. Meyer
 */
inline koord3d operator+ (const koord3d & a, const koord & b)
{
    return koord3d(a.x+b.x, a.y+b.y, (int)a.z);
}

inline koord3d operator- (const koord3d & a, const koord & b)
{
    return koord3d(a.x-b.x, a.y-b.y, (int)a.z);
}

inline const koord3d& operator+= (koord3d & a, const koord & b)
{
    a.x+=b.x;
    a.y+=b.y;
    return a;
}

inline const koord3d& operator-= (koord3d & a, const koord & b)
{
    a.x-=b.x;
    a.y-=b.y;
    return a;
}


/* distance functions
 * @author prissi
 */
inline int koord_distance(koord a,koord b) {
	return abs(a.x-b.x)+abs(a.y-b.y);
}

inline int koord_distance(koord3d a,koord b) {
	return abs(a.x-b.x)+abs(a.y-b.y);
}

inline int koord_distance(koord a,koord3d b) {
	return abs(a.x-b.x)+abs(a.y-b.y);
}

inline int koord_distance(koord3d a,koord3d b) {
	return abs(a.x-b.x)+abs(a.y-b.y);
}


#endif
