/*
 * fraction_t.h
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#ifndef FRACTION_T_H_
#define FRACTION_T_H_

#include <math.h>

typedef long long int64;

// used by shrink()
static const int64 top4 	= 0x7800000000000000LL;
static const int64 top33	= 0xffffffff80000000LL;

// used by ild()
static const int64 top16	= 0xffff000000000000LL;
static const int64 top32	= 0xffffffff00000000LL;
static const int64 top48	= 0xffffffffffff0000LL;

// used by fraction_t(double)
static const int64 maxlonglong 	= 0x7fffffffffffffffLL;

const int64 common_factor(int64 a, int64 b);
const int ild(int64 x);

inline int64 min(int64 a, int64 b)
{
	if (a <= b)
		return a;
	return b;
}

inline int64 max(int64 a, int64 b)
{
	if (a >= b)
		return a;
	return b;
}

class fraction_t {
public:
	int64 n;
	int64 d;

	// constructors

	inline fraction_t() {}

	inline fraction_t(int64 nominator, int64 denominator)
	{
		n = nominator;
		d = denominator;
	}

	inline fraction_t(int value)
	{
		n = value;
		d = 1;
	}

	inline fraction_t(long value)
	{
		n = value;
		d = 1;
	}

	inline fraction_t(int64 value)
	{
		n = value;
		d = 1;
	}

	fraction_t(double value);

	// operators: additon and subtraction

	const fraction_t & operator += (const fraction_t &f);

	inline const fraction_t & operator -= (const fraction_t &f)
	{
		return *this += fraction_t(-f.n, f.d);
	}

	inline const fraction_t operator + (const fraction_t &f) const
	{
		fraction_t r(n, d);
		return r += f;
	}

	inline const fraction_t operator - (const fraction_t &f) const
	{
		fraction_t r(n, d);
		return r += fraction_t(-f.n, f.d);
	}

	inline const fraction_t & operator += (int64 value)
	{
		return *this += fraction_t(value, 1);
	}

	inline const fraction_t & operator -= (int64 value)
	{
		return *this += fraction_t(value, -1);
	}

	inline const fraction_t operator + (int64 value) const
	{
		fraction_t r(n, d);
		return r += fraction_t(value, 1);
	}

	inline const fraction_t operator - (int64 value) const
	{
		fraction_t r(n, d);
		return r += fraction_t(value, -1);
	}

	// operators: multiplication and division

	const fraction_t & operator *= (const fraction_t &f);

	inline const fraction_t & operator /= (const fraction_t &f)
	{
		return *this *= fraction_t(f.d, f.n);
	}

	inline const fraction_t operator * (const fraction_t &f) const
	{
		fraction_t r(n, d);
		return r *= f;
	}

	inline const fraction_t operator / (const fraction_t &f) const
	{
		fraction_t r(n, d);
		return r *= fraction_t(f.d, f.n);
	}

	inline const fraction_t & operator *= (int64 value)
	{
		return *this *= fraction_t(value, 1);
	}

	inline const fraction_t & operator /= (int64 value)
	{
		return *this *= fraction_t(1, value);
	}

	inline const fraction_t operator * (int64 value) const
	{
		fraction_t r(n, d);
		return r *= fraction_t(value, 1);
	}

	inline const fraction_t operator / (int64 value) const
	{
		fraction_t r(n, d);
		return r *= fraction_t(1, value);
	}

	// operators: comparators

	inline bool operator < (const fraction_t &f) const
	{
		if (n < f.n && d >= f.d)
			return true;
		if (n == f.n && d > f.d)
			return true;
		fraction_t r = *this / f;
		r.shrink();
		return r.n < r.d;
	}

	inline bool operator <= (const fraction_t &f) const
	{
		if (n <= f.n && d >= f.d)
			return true;
		fraction_t r = *this / f;
		r.shrink();
		return r.n <= r.d;
	}

	inline bool operator == (const fraction_t &f) const
	{
		if (n == f.n && d == f.d)
			return true;
		fraction_t r = *this / f;
		r.shrink();
		return r.n == r.d;
	}

	inline bool operator != (const fraction_t &f) const
	{
		if (n == f.n && d == f.d)
			return false;
		fraction_t r = *this / f;
		r.shrink();
		return r.n != r.d;
	}

	inline bool operator >= (const fraction_t &f) const
	{
		if (n >= f.n && d <= f.d)
			return true;
		fraction_t r = *this / f;
		r.shrink();
		return r.n >= r.d;
	}

	inline bool operator > (const fraction_t &f) const
	{
		if (n > f.n && d <= f.d)
			return true;
		if (n == f.n && d < f.d)
			return true;
		fraction_t r = *this / f;
		r.shrink();
		return r.n > r.d;
	}

	// shorten and shrink
	//
	// shorten() without loss of the exact value, but requires a common factor in nominator and denominator.
	// shrink() with loss of the exact value. average deviation: 1 / 2^16

	inline const fraction_t shorten() const
	{
		fraction_t r;
		int64 factor = common_factor(n, d);
		if (factor > 1)
		{
			r.n = n / factor;
			r.d = d / factor;
		}
		else
		{
			r.n = n;
			r.d = d;
		}
		return r;
	}

	inline const fraction_t & shorten()
	{
		int64 factor = common_factor(n, d);
		if (factor > 1)
		{
			n /= factor;
			d /= factor;
		}
		return *this;
	}

	const fraction_t shrink() const;

	const fraction_t & shrink();
};

const fraction_t log(const fraction_t &x);
const fraction_t exp(const fraction_t &x);

inline const fraction_t pow(const fraction_t &base, const fraction_t &expo)
{
	return exp(expo * log(base));
}


#endif /* FRACTION_T_H_ */
