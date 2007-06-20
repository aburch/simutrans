/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef SIMTYPES_H
#define SIMTYPES_H

#if defined _MSC_VER
#	if _MSC_VER <= 1200
#		error "Simutrans cannot be compiled with Visual C++ 6.0 or earlier."
#	endif
#
#	include <malloc.h>
#	define ALLOCA(type, name, count) type* name = static_cast<type*>(alloca(sizeof(type) * (count)))
#
# define inline _inline
#else
#	define ALLOCA(type, name, count) type name[count]
#endif

/* divers enums:
 * better defined here than scattered in thousand files ...
 */
enum climate
{
	water_climate,
	desert_climate,
	tropic_climate,
	mediterran_climate,
	temperate_climate,
	tundra_climate,
	rocky_climate,
	arctic_climate,
	MAX_CLIMATES
};

enum climate_bits
{
	water_climate_bit      = 1 << water_climate,
	desert_climate_bit     = 1 << desert_climate,
	tropic_climate_bit     = 1 << tropic_climate,
	mediterran_climate_bit = 1 << mediterran_climate,
	temperate_climate_bit  = 1 << temperate_climate,
	tundra_climate_bit     = 1 << tundra_climate,
	rocky_climatebit       = 1 << rocky_climate,
	arctic_climate_bit     = 1 << arctic_climate,
	ALL_CLIMATES           = (1 << MAX_CLIMATES) - 1,
	all_but_water_climate  = ALL_CLIMATES & ~water_climate_bit,
	all_but_arctic_climate = ALL_CLIMATES & ~arctic_climate_bit
};

/**
 * Vordefinierte Wegtypen.
 * @author Hj. Malthaner
 */
enum waytype_t {
	invalid_wt       =  -1,
	ignore_wt        =   0,
	road_wt          =   1,
	track_wt         =   2,
	water_wt         =   3,
	overheadlines_wt =   4,
	monorail_wt      =   5,
	tram_wt          =   7,
	air_wt           =  16,
	powerline_wt     = 128
};


// makros are not very safe: thus use these macro like functions
// otherwise things may fail or functions are called uneccessarily twice

#define CLIP(wert,mini,maxi)  min(max((wert),(mini)),(maxi))

// Hajo: define machine independant types
typedef unsigned int        uint;
typedef   signed char       sint8;
typedef unsigned char       uint8;
typedef   signed short      sint16;
typedef unsigned short      uint16;
typedef   signed int        sint32;
typedef unsigned int        uint32;
#ifdef _MSC_VER
typedef   signed __int64	  sint64;
typedef unsigned __int64    uint64;
#	define GCC_PACKED
#	define NORETURN __declspec(noreturn)
#	pragma warning(disable: 4200 4311 4800 4996)
#else
typedef   signed long long  sint64;
typedef unsigned long long  uint64;
#	define GCC_PACKED __attribute__ ((__packed__))
#	define NORETURN   __attribute__ ((noreturn))
#endif

#ifdef __cplusplus

template<typename T> static inline int sgn(T x)
{
		if (x < 0) return -1;
		if (x > 0) return  1;
		return 0;
}

static inline int min(const int a, const int b)
{
    return a < b ? a : b;
}

static inline int max(const int a, const int b)
{
    return a > b ? a : b;
}

/**
 * Sometimes we need to pass pointers as well as integers through a
 * standardized interface (i.e. via a function pointer). This union is used as
 * a helper type to avoid cast operations.  This isn't very clean, but if used
 * with care it seems better than using "long" and casting to a pointer type.
 * In all cases it ensures that no bits are lost.
 * @author Hj. Malthaner
 */
typedef union anyvalue{

  anyvalue()                : p(0)   {}
  anyvalue(long itg)        : i(itg) {}
  anyvalue(const void* ptr) : p(ptr) {}

	const void* p;
	long i;
} value_t;

#else
	typedef enum bool { false, true } bool;
#endif

#endif
