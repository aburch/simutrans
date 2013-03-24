/*
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef SIMTYPES_H
#define SIMTYPES_H

#include "utils/for.h"

#if defined _MSC_VER
#	if _MSC_VER <= 1200
#		error "Simutrans cannot be compiled with Visual C++ 6.0 or earlier."
#	endif
#
#	include <malloc.h>
#	define ALLOCA(type, name, count) type* name = static_cast<type*>(alloca(sizeof(type) * (count)))
#
# define inline _inline
#
#elif defined __clang__
#	include <stdlib.h>
#	define ALLOCA(type, name, count) type* name = static_cast<type*>(alloca(sizeof(type) * (count)))
#else
#	define ALLOCA(type, name, count) type name[count]
#endif

#if defined DEBUG
#	include <stdlib.h>
#	define NOT_REACHED abort();
#else
#	define NOT_REACHED
#endif

#define GCC_ATLEAST(major, minor) (defined __GNUC__ && (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor))))

#define CXX11(gcc_major, gcc_minor, msc_ver) ( \
	__cplusplus >= 201103L || \
	(defined __GXX_EXPERIMENTAL_CXX0X__ && GCC_ATLEAST((gcc_major), (gcc_minor))) || \
	(defined _MSC_VER && (msc_ver) != 0 && _MSC_VER >= (msc_ver)) \
)

#if CXX11(4, 4, 0)
#	define DELETED = delete
#else
#	define DELETED
#endif

#if CXX11(4, 7, 1400)
#	define OVERRIDE override
#else
#	define OVERRIDE
#endif

#ifdef __cplusplus
#	define ENUM_BITSET(T) \
		static inline T operator ~  (T  a)      { return     (T)~(unsigned)a;                } \
		static inline T operator &  (T  a, T b) { return     (T)((unsigned)a & (unsigned)b); } \
		static inline T operator &= (T& a, T b) { return a = (T)((unsigned)a & (unsigned)b); } \
		static inline T operator |  (T  a, T b) { return     (T)((unsigned)a | (unsigned)b); } \
		static inline T operator |= (T& a, T b) { return a = (T)((unsigned)a | (unsigned)b); }
#else
#	define ENUM_BITSET(T)
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
#ifndef NO_UINT32_TYPES
typedef unsigned int        uint32;
#endif
#else
// BeOS: int!=long (even though both 32 bit)
typedef   signed long       sint32;
#ifndef NO_UINT32_TYPES
typedef unsigned long       uint32;
#endif
#endif
typedef   signed long long  sint64;
typedef unsigned long long  uint64;
#ifdef _MSC_VER
#	define GCC_PACKED
#	define NORETURN __declspec(noreturn)
#	pragma warning(disable: 4200 4311 4800 4996)
#else
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

static inline uint16 endian(uint16 v)
{
#ifdef SIM_BIG_ENDIAN
	v = v << 8 | v >> 8; // 0x0011
#endif
	return v;
}

static inline uint32 endian(uint32 v)
{
#ifdef SIM_BIG_ENDIAN
	v = v << 16              | v >> 16;              // 0x22330011
	v = v <<  8 & 0xFF00FF00 | v >>  8 & 0x00FF00FF; // 0x33221100
#endif
	return v;
}

static inline uint64 endian(uint64 v)
{
#ifdef SIM_BIG_ENDIAN
	v = v << 32                         | v >> 32;                         // 0x4455667700112233
	v = v << 16 & 0xFFFF0000FFFF0000ULL | v >> 16 & 0x0000FFFF0000FFFFULL; // 0x6677445522330011
	v = v <<  8 & 0xFF00FF00FF00FF00ULL | v >>  8 & 0x00FF00FF00FF00FFULL; // 0x7766554433221100
#endif
	return v;
}

static inline sint16 endian(sint16 const v) { return sint16(endian(uint16(v))); }
static inline sint32 endian(sint32 const v) { return sint32(endian(uint32(v))); }
static inline sint64 endian(sint64 const v) { return sint64(endian(uint64(v))); }


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
