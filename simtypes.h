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

/* divers enums:
 * better defined here than scattered in thousand files ...
 */
enum climate { water_climate=0, desert_climate, tropic_climate, mediterran_climate, temperate_climate, tundra_climate, rocky_climate, arctic_climate, MAX_CLIMATES };
enum climate_bits { water_climate_bit=(1<<water_climate),  desert_climate_bit=(1<<desert_climate), tropic_climate_bit=(1<<tropic_climate),
							mediterran_climate_bit=(1<<mediterran_climate), temperate_climate_bit=(1<<temperate_climate),
							tundra_climate_bit=(1<<tundra_climate), rocky_climatebit=(1<<rocky_climate), arctic_climate_bit=(1<<arctic_climate), all_but_arctic_climate=(arctic_climate_bit-2),  all_but_water_climate=((2<<arctic_climate)-2), ALL_CLIMATES=((2<<arctic_climate)-1) };

/**
 * Vordefinierte Wegtypen.
 * @author Hj. Malthaner
 */
enum waytype_t {
	invalid_wt=-1, ignore_wt=0, road_wt=1, track_wt=2, water_wt=3,
	overheadlines_wt=4,
	monorail_wt=5,
	tram_wt=7, // Dario: Tramway
	powerline_wt=128,
	air_wt=16
};





// makros are not very safe: thus use these macro like functions
// otherwise things may fail or functions are called uneccessarily twice

#define CLIP(wert,mini,maxi)  min(max((wert),(mini)),(maxi))

// Hajo: define machine independant types

// inline funktionen



typedef unsigned int         uint;
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
#define NORETURN __declspec(noreturn)
#pragma warning(disable: 4200 4311 4800 4996 )
#else
typedef signed long long    sint64;
typedef unsigned long long  uint64;
#define GCC_PACKED __attribute__((__packed__))
#define NORETURN __attribute__ ((noreturn))
#endif

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

inline static int sgn(sint64 x)
{
    return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

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
  anyvalue() { p = 0; }
  anyvalue(long itg) { i = itg; }
  anyvalue(const void* ptr) { p = ptr; }
#endif
} value_t;



typedef struct {
	uint8	x, y;
} offset_koord;

#endif
