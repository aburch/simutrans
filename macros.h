#ifndef MACROS_H
#define MACROS_H

// XXX Workaround: Old GCCs choke on type check.
#if defined __cplusplus && (!defined __GNUC__ || __GNUC__ >= 3)
// Ensures that the argument has array type.
template <typename T, unsigned N> static inline void lengthof_check(T (&)[N]) {}
#	define lengthof(x) (1 ? sizeof(x) / sizeof(*(x)) : (lengthof_check((x)), 0))
#else
#	define lengthof(x) (sizeof(x) / sizeof(*(x)))
#endif

#define endof(x) ((x) + lengthof(x))

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

// XXX Workaround for GCC 2.95
template<typename T> static inline T up_cast(T x) { return x; }

}
#endif

#endif
