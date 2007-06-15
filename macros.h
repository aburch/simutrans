#ifndef MACROS_H
#define MACROS_H

#define lengthof(x) (sizeof(x) / sizeof(*(x)))

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


#ifdef __cplusplus
namespace sim {

template<class T> inline void swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

}
#endif

#endif
