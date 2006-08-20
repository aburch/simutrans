/*
 * koord.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>

#ifndef koord_h
#define koord_h

#include <new>

#ifndef dataobj_ribi_t_h
#include "ribi.h"
#endif

class loadsave_t;

/**
 * 2d koordinaten
 *
 * @author Hj. Malthaner
 */
class koord
{
private:
    static const koord from_ribi[16];
    static const koord from_hang[16];

public:
    short x, y;

    // Hajo: performance hack
    koord() {/*x=y=0;*/  *((int *)this) = 0;};

    koord(short xp, short yp) {x=xp; y=yp;};
    koord(loadsave_t *file);
    koord(ribi_t::ribi ribi) { *this = from_ribi[ribi]; }
    koord(hang_t::typ hang) { *this = from_hang[hang]; }


    void rdwr(loadsave_t *file);

    void * operator new(size_t s);
    void operator delete(void *p);

    static const koord invalid;
    static const koord nord;
    static const koord sued;
    static const koord ost;
    static const koord west;
    // die 4 Grundrichtungen als Array
    static const koord nsow[4];

    inline koord operator* (const int m) const {
	return koord(x*m, y*m);
    }

    inline koord operator/ (const int m) const {
      return koord(x/m, y/m);
    }
};



inline unsigned abs_distance(koord a,koord b)
{
	return abs(a.x-b.x)+abs(a.y-b.y);
}




inline bool operator== (const koord & a, const koord & b)
{
  //    return (a.x == b.x && a.y == b.y);

  // Hajo: dirty trick to speed things up
  return *((const int*)&a) == *((const int*)&b);
}


inline bool operator!= (const koord & a, const koord & b)
{
  // return (a.x != b.x) || (a.y != b.y);

  // Hajo: dirty trick to speed things up
  return *((const int*)&a) != *((const int*)&b);
}


inline koord operator+ (const koord & a, const koord & b)
{
    return koord(a.x+b.x, a.y+b.y);
}

inline koord operator- (const koord & a, const koord & b)
{
    return koord(a.x-b.x, a.y-b.y);
}

inline const koord& operator+= (koord & a, const koord & b)
{
    a.x+=b.x;
    a.y+=b.y;
    return a;
}

inline const koord& operator-= (koord & a, const koord & b)
{
    a.x-=b.x;
    a.y-=b.y;
    return a;
}

inline koord operator- (koord & a)
{
    return koord(-a.x, -a.y);
}

struct matrix
{
    int a,b,c,d;

    matrix() {a=b=c=d=0;};
    matrix(int aa, int bb, int cc, int dd) {
	a = aa;
	b = bb;
	c = cc;
	d = dd;
    }
};

inline koord operator* (const matrix & m, const koord & a)
{
    return koord(m.a*a.x+m.b*a.y, m.c*a.x+m.d*a.y);
}

#endif
