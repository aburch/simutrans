/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef MACROS_H
#define MACROS_H


#include "simtypes.h"

// Ensures that the argument has array type.
template <typename T, unsigned N> static inline void lengthof_check(T (&)[N]) {}
#define lengthof(x) (1 ? sizeof(x) / sizeof(*(x)) : (lengthof_check((x)), 0))
#define endof(x) ((x) + lengthof(x))

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

#define MEMZERON(ptr, n) memset((ptr), 0, sizeof(*(ptr)) * (n))
#define MEMZERO(obj)     MEMZERON(&(obj), 1)

// make sure, a value in within the borders
static inline int clamp(int x, int min, int max)
{
	if (x <= min) {
		return min;
	}
	if (x >= max) {
		return max;
	}
	return x;
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

