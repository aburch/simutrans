/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef MACROS_H
#define MACROS_H


#include "simtypes.h"

#include <cstring>


template<typename T, size_t N>
static constexpr size_t lengthof(const T (&)[N]) { return N; }

template<typename T, size_t N>
static constexpr T *endof(T (&arr)[N]) { return arr + N; }


#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

#define MEMZERON(ptr, n) memset((ptr), 0, sizeof(*(ptr)) * (n))
#define MEMZERO(obj)     MEMZERON(&(obj), 1)


// make sure, a value in within the borders
template<typename T> static inline T clamp(T v, T l, T u)
{
	return v < l ? l : (v > u ? u :v);
}


namespace sim {

template<class T> inline void swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

// XXX Workaround for GCC 2.95
template<typename T> static inline T up_cast(T x) { return x; }

}

#endif
