/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_INT_MATH_H
#define UTILS_INT_MATH_H


#include "../simtypes.h"


inline int log2(uint64 n)
{
#define S(k) do { if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; } } while(false)

	int i = -(n == 0);
	S(32);
	S(16);
	S(8);
	S(4);
	S(2);
	S(1);
	return i;

#undef S
}

inline int log2(uint32 n)
{
#define S(k) do { if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; } } while (false)

	int i = -(n == 0);
	S(16);
	S(8);
	S(4);
	S(2);
	S(1);
	return i;

#undef S
}

inline int log2(uint16 n)
{
#define S(k) do { if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; } } while(false)

	int i = -(n == 0);
	S(8);
	S(4);
	S(2);
	S(1);
	return i;

#undef S
}

inline int log10(uint64 n)
{
#define S(k, m) do { if (n >= UINT64_C(m)) { i += k; n /= UINT64_C(m); } } while(false)

	int i = -(n == 0);
	S(16,10000000000000000);
	S(8,100000000);
	S(4,10000);
	S(2,100);
	S(1,10);
	return i;

#undef S
}

#endif
