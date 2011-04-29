/*
 * fraction64_t.h
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#ifndef FRACTION64_T_H_
#define FRACTION64_T_H_

#include <math.h>
#include "../simtypes.h"

const sint64 common_factor(sint64 a, sint64 b);
const int ild(sint64 x);

inline sint64 min(sint64 a, sint64 b)
{
	if (a <= b)
		return a;
	return b;
}

inline sint64 max(sint64 a, sint64 b)
{
	if (a >= b)
		return a;
	return b;
}

class fraction64_t {
public:
	sint64 n;
	sint64 d;

	// constructors

	inline fraction64_t() {}

	inline fraction64_t(sint64 nominator, sint64 denominator)
	{
		n = nominator;
		d = denominator;
	}

	inline fraction64_t(int value)
	{
		n = value;
		d = 1;
	}

	inline fraction64_t(long value)
	{
		n = value;
		d = 1;
	}

	inline fraction64_t(sint64 value)
	{
		n = value;
		d = 1;
	}

	/*fraction64_t(double value);*/

	// operators: additon and subtraction

	const fraction64_t & operator += (const fraction64_t &f);

	inline const fraction64_t & operator -= (const fraction64_t &f)
	{
		return *this += fraction64_t(-f.n, f.d);
	}

	inline const fraction64_t operator + (const fraction64_t &f) const
	{
		fraction64_t r(n, d);
		return r += f;
	}

	inline const fraction64_t operator - (const fraction64_t &f) const
	{
		fraction64_t r(n, d);
		return r += fraction64_t(-f.n, f.d);
	}

	inline const fraction64_t & operator += (sint64 value)
	{
		return *this += fraction64_t(value, 1);
	}

	inline const fraction64_t & operator -= (sint64 value)
	{
		return *this += fraction64_t(value, -1);
	}

	inline const fraction64_t operator + (sint64 value) const
	{
		fraction64_t r(n, d);
		return r += fraction64_t(value, 1);
	}

	inline const fraction64_t operator - (sint64 value) const
	{
		fraction64_t r(n, d);
		return r += fraction64_t(value, -1);
	}

	// operators: multiplication and division

	const fraction64_t & operator *= (const fraction64_t &f);

	inline const fraction64_t & operator /= (const fraction64_t &f)
	{
		return *this *= fraction64_t(f.d, f.n);
	}

	inline const fraction64_t operator * (const fraction64_t &f) const
	{
		fraction64_t r(n, d);
		return r *= f;
	}

	inline const fraction64_t operator / (const fraction64_t &f) const
	{
		fraction64_t r(n, d);
		return r *= fraction64_t(f.d, f.n);
	}

	inline const fraction64_t & operator *= (sint64 value)
	{
		return *this *= fraction64_t(value, 1);
	}

	inline const fraction64_t & operator /= (sint64 value)
	{
		return *this *= fraction64_t(1, value);
	}

	inline const fraction64_t operator * (sint64 value) const
	{
		fraction64_t r(n, d);
		return r *= fraction64_t(value, 1);
	}

	inline const fraction64_t operator / (sint64 value) const
	{
		fraction64_t r(n, d);
		return r *= fraction64_t(1, value);
	}

	// operators: comparators

	inline bool operator < (const fraction64_t &f) const
	{
		if (n < f.n && d >= f.d)
			return true;
		if (n == f.n && d > f.d)
			return true;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n < r.d;
	}

	inline bool operator <= (const fraction64_t &f) const
	{
		if (n <= f.n && d >= f.d)
			return true;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n <= r.d;
	}

	inline bool operator == (const fraction64_t &f) const
	{
		if (n == f.n && d == f.d)
			return true;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n == r.d;
	}

	inline bool operator != (const fraction64_t &f) const
	{
		if (n == f.n && d == f.d)
			return false;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n != r.d;
	}

	inline bool operator >= (const fraction64_t &f) const
	{
		if (n >= f.n && d <= f.d)
			return true;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n >= r.d;
	}

	inline bool operator > (const fraction64_t &f) const
	{
		if (n > f.n && d <= f.d)
			return true;
		if (n == f.n && d < f.d)
			return true;
		fraction64_t r = *this / f;
		r.shrink();
		return r.n > r.d;
	}

	// shorten and shrink
	//
	// shorten() without loss of the exact value, but requires a common factor in nominator and denominator.
	// shrink() with loss of the exact value. average deviation: 1 / 2^16

	inline const fraction64_t shorten() const
	{
		fraction64_t r;
		sint64 factor = common_factor(n, d);
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

	inline const fraction64_t & shorten()
	{
		sint64 factor = common_factor(n, d);
		if (factor > 1)
		{
			n /= factor;
			d /= factor;
		}
		return *this;
	}

	const fraction64_t shrink() const;

	const fraction64_t & shrink();
};

const fraction64_t log(const fraction64_t &x);
const fraction64_t exp(const fraction64_t &x);

inline const fraction64_t pow(const fraction64_t &base, const fraction64_t &expo)
{
	return exp(expo * log(base));
}


#endif /* FRACTION_T_H_ */
