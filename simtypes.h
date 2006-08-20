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

#define MAX(a,b)            ((a) > (b) ? (a) : (b))
#define MIN(a,b)            ((a) < (b) ? (a) : (b))
#define ABS(a)              ((a) < 0 ? -(a) : (a))

#define CLIP(wert,min,max)  MIN(MAX((wert),(min)),(max))

// Hajo: define machine independant types

typedef signed char          sint8;
typedef unsigned char        uint8;
typedef signed short        sint16;
typedef unsigned short      uint16;
typedef signed int          sint32;
typedef unsigned int        uint32;
#ifdef _MSC_VER
typedef signed __int64	    sint64;
typedef unsigned __int64    uint64;
#else
typedef signed long long    sint64;
typedef unsigned long long  uint64;
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


#endif
