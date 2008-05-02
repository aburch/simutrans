/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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

#if defined DEBUG
#	include <stdlib.h>
#	define NOT_REACHED abort();
#else
#	define NOT_REACHED
#endif

/* divers enums:
 * better defined here than scattered in thousand files ...
 */
enum climate
{
	water_climate = 0,
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
	maglev_wt        =   6,
	tram_wt          =   7,
	narrowgauge_wt   =   8,
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
#ifndef __BEOS__
typedef   signed int        sint32;
typedef unsigned int        uint32;
#else
// BeOS: int!=long (even though both 32 bit)
typedef   signed long       sint32;
typedef unsigned long       uint32;
#endif
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

// endian coversion routines

inline uint8 endian_uint8(const char * data)
{
	return (uint8)*data;
}

inline uint16 endian_uint16(const uint16 *d)
{
#ifndef BIG_ENDIAN
	return  *d;
#else
	const uint8 *data = (const uint8 *)d;
	return ((uint16)data[0]) | ( ((uint16)data[1]) << 8 );
#endif
}

inline uint32 endian_uint32(const uint32 *d)
{
#ifndef BIG_ENDIAN
	return *d;
#else
	const uint8 *data = (const uint8 *)d;
	return  ((uint32)data[0])  | ( ((uint32)data[1]) << 8 )  | ( ((uint32)data[2]) << 16 )  | ( ((uint32)data[3]) << 24 );
#endif
}

inline uint64 endian_uint64(const uint64 * d)
{
#ifndef BIG_ENDIAN
	return *d;
#else
	const uint8 *data = (const uint8 *)d;
	return  ((uint64)data[0])  | ( ((uint64)data[1]) << 8 )  | ( ((uint64)data[2]) << 16 )  | ( ((uint64)data[3]) << 24 ) | (((uint64)data[4]) << 32 ) | ( ((uint64)data[5]) << 40 )  | ( ((uint64)data[6]) << 48 )  | ( ((uint64)data[7]) << 56 );
#endif
}


/**
 * Sometimes we need to pass pointers as well as integers through a
 * standardized interface (i.e. via a function pointer). This union is used as
 * a helper type to avoid cast operations.  This isn't very clean, but if used
 * with care it seems better than using "long" and casting to a pointer type.
 * In all cases it ensures that no bits are lost.
 * @author Hj. Malthaner
 */
union value_t
{
	value_t()                : p(0)   {}
	value_t(long itg)        : i(itg) {}
	value_t(const void* ptr) : p(ptr) {}

	const void* p;
	long i;
};

#else
// c definitionen
typedef enum bool { false, true } bool;
#endif

#endif
