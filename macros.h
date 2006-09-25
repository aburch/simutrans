#ifndef MACROS_H
#define MACROS_H

#ifdef _MSC_VER
#	define inline _inline
#endif

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

#endif
