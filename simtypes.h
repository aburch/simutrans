/*
 * simtypes.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simtypes_h
#define simtypes_h

// makros are not very safe: thus use these macro like functions
// otherwise things may fail or functions are called uneccessarily twice

#define CLIP(wert,mini,maxi)  min(max((wert),(mini)),(maxi))

// Hajo: define machine independant types

// inline funktionen


#ifdef __cplusplus

#if defined(_MSC_VER)  &&  _MSC_VER<=1200
// old Microsoft compability stuff
#error "Simutrans cannot compile with braindead combilers! Get rid of your zombie!"
#if!defined(max)
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#define strdup _strdup

#else

inline static int sgn(int x)
{
    return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

inline static int min(const int a, const int b)
{
    return (a < b) ? a : b;
}

inline static int max(const int a, const int b)
{
    return (a > b) ? a : b;
}
#endif
#else
	typedef enum bool { false, true } bool;
#endif

typedef signed char          sint8;
typedef unsigned char        uint8;
typedef signed short        sint16;
typedef unsigned short      uint16;
typedef signed int          sint32;
typedef unsigned int        uint32;
#ifdef _MSC_VER
typedef signed __int64	    sint64;
typedef unsigned __int64    uint64;
#define GCC_PACKED
#else
typedef signed long long    sint64;
typedef unsigned long long  uint64;
#define GCC_PACKED __attribute__((__packed__))
#endif

/**
 * Sometimes we need to pass pointers as well as integers through
 * a standardized interface (i.e. via a function pointer). This
 * union is used as a helper type to avoid cast operations.
 * This isn't very clean, but if used with care it seems better
 * than using "long" and casting to a pointer type. In all cases
 * it ensures that no bits are lost.
 * @author Hj. Malthaner
 */
typedef union anyvalue{
  const void * p;
  long i;

#ifdef __cplusplus
  anyvalue() {p = 0;};
  anyvalue(long itg) {i = itg;};
  anyvalue(const void *ptr) {p = ptr;};
#endif
} value_t;



typedef struct {
	uint8	x, y;
} offset_koord;

#endif
