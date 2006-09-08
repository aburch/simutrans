#ifndef MACROS_H
#define MACROS_H

static inline int clamp(int x, int min, int max)
{
	if (x <= min) return min;
	if (x >= max) return max;
	return x;
}

#endif
