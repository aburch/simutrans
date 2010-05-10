#ifndef MACROS_H
#define MACROS_H

#ifdef __cplusplus
template <typename T, unsigned N> static inline void lengthof_check(T (&)[N]) {}
#	define lengthof(x) (1 ? sizeof(x) / sizeof(*(x)) : (lengthof_check((x)), 0))
#else
#	define lengthof(x) (sizeof(x) / sizeof(*(x)))
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
