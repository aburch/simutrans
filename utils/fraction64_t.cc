/*
 * fraction64_t.cc
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#ifdef DEBUG_COUT
#include <iostream>
using namespace std;
#endif

#include "fraction64_t.h"

#define BITS 64

// used by shrink()
static const sint64 top4 	= 0x7800000000000000LL;
static const sint64 top33	= 0xffffffff80000000LL;

// used by ild()
static const sint64 top16	= 0xffff000000000000LL;
static const sint64 top32	= 0xffffffff00000000LL;
static const sint64 top48	= 0xffffffffffff0000LL;

// used by fraction64_t(double)
static const sint64 maxlonglong 	= 0x7fffffffffffffffLL;

const sint64 common_factor(sint64 a, sint64 b)
// Returns the largest common factor in a and be or 1, if there is no common factor.
{
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;

	sint64 c;

	if (a < b)
	{
		c = a;
		a = b;
		b = c;
	}
	//cout << "a " << a << ", b " << b ;
	while ( (c = a % b ) != 0 )
	{
		//cout << ", c " << c;
		a = b;
		b = c;
	}
	//cout << "\n";
	return b;
}

const int ild(sint64 x)
// "integer logarithmus digitalis"
// Returns the number of the highest used bit of abs(x):
// 0: no bits in use x == 0
// 1: x == 1
// 2: 2 <= x < 4
// 3: 4 <= x < 8
// ...
// 63: 2^62 <= x < 2^63
//
// function is used to foresee integer overflow.
{
	sint64 msk;
	int r;
	if (x < 0)
		x = -x;
	if (x & top32)
	{
		if (x & top16)
		{
			msk = 0x4000000000000000LL;
			r = 63;
		}
		else
		{
			msk = 0x800000000000LL;
			r = 48;
		}
	}
	else
	{
		if (x & top48)
		{
			msk = 0x80000000LL;
			r = 32;
		}
		else
		{
			msk = 0x8000LL;
			r = 16;
		}
	}
	while (r)
	{
		if (x & msk)
			break;
		msk >>= 1;
		r--;
	}
	return r;
}

const fraction64_t log(const fraction64_t &x)
{
	// requires x > 0
	fraction64_t t1 = fraction64_t(x.n - x.d, x.n + x.d);
	t1.shorten();
	fraction64_t t2 = t1 * t1;
	fraction64_t r = t1;
	fraction64_t l = r;
	sint64 n = 1;
	while ((n += 2) < 50)
	{
		t1 *= t2;
		r += t1 / n;
		r.shorten();
#ifdef DEBUG_COUT
		cout << n << ": log(" << x.n << " / " << x.d << ") = " << 2 * r.n << " / " << r.d << " = " << (double) 2 * r.n / r.d << "\n";
		cout.flush();
#endif
		if (r == l)
			break;
		l = r;
	}
	return r * 2;
}

const fraction64_t exp(const fraction64_t &x)
{
	fraction64_t t = x.shorten();
	fraction64_t t1 = t;
	fraction64_t r = (x + 1).shorten();
	fraction64_t l = r;
	sint64 n = 1;
	while (++n < 30)
	{
		t1 *= t / n;
		r += t1;
		r.shorten();
#ifdef DEBUG_COUT
		cout << n << ": exp(" << x.n << " / " << x.d << ") = " << r.n << " / " << r.d << " = " << (double) r.n / r.d << "\n";
		cout.flush();
#endif
		if (r == l)
			break;
		l = r;
	}
	return r;
}

#ifdef USE_DOUBLE
fraction64_t::fraction64_t(double value)
{
	double a = fabs(value);
	if (a > maxlonglong)
	{
		n = 1;
		d = 0;
	}
	else if (a < 1/maxlonglong)
	{
		n = 0;
		d = 1;
	}
	else
	{
		int m = (int)(log(maxlonglong) / log(10));
		int x = (int)(log(a) / log(10));
		d = (sint64) exp(min_64(m - x, 12) * log(10));
		n = (sint64) (value * d);
	}
}
#endif

const fraction64_t & fraction64_t::operator += (const fraction64_t &f)
{
	normalize();
	if (d == f.d)
	{
		int iun = ild(n), ivn = ild(f.n);
		if (iun + ivn < 2 * BITS - 1)
		{
			n += f.n;
			return *this;
		}
	}

	fraction64_t v(f.n, f.d);
	int iun = ild(n), ivd = ild(v.d);
	if (iun + ivd >= BITS)
	{
		if (iun >= BITS/2)
			shrink();
		if (ivd >= BITS/2)
		{
			v.shrink();
			ivd = ild(v.d);
		}
	}
	int iud = ild(d), ivn = ild(v.n);
	if (iud + ivn >= BITS)
	{
		if (iud >= BITS/2)
		{
			shrink();
			iud = ild(d);
		}
		if (ivn >= BITS/2)
			v.shrink();
	}
	if (iud + ivd >= BITS)
	{
		if (iud >= BITS/2)
			shrink();
		if (ivd >= BITS/2)
			v.shrink();
	}
	n = n * v.d + d * v.n;
	d = d * v.d;
	normalize();
	return *this;
}

const fraction64_t & fraction64_t::operator *= (const fraction64_t &f)
{
	fraction64_t v(f.n, f.d);
	int iun = ild(n), ivn = ild(v.n);
	if (iun + ivn >= BITS)
	{
		if (iun >= BITS/2)
			shrink();
		if (ivn >= BITS/2)
			v.shrink();
	}
	int iud = ild(d), ivd = ild(v.d);
	if (iud + ivd >= BITS)
	{
		if (iud >= BITS/2)
			shrink();
		if (ivd >= BITS/2)
			v.shrink();
	}
	n *= v.n;
	d *= v.d;
	normalize();
	return *this;
}

const fraction64_t fraction64_t::shrink() const
{
	fraction64_t r = *this;
	sint64 an = n >= 0 ? n : -n;
	sint64 ad = d >= 0 ? d : -d;;
	if (an & top33 || ad & top33)
	{
		sint64 mask = top4;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- > 8 && sn == 0 && sd == 0; mask >>= 4)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max_64(sn, sd) - 7) * 4;
		r.n = n / (1LL << s);
		r.d = d / (1LL << s);
	}
	return r;
}

const fraction64_t & fraction64_t::shrink()
{
	sint64 an = n >= 0 ? n : -n;
	sint64 ad = d >= 0 ? d : -d;;
	if (an & top33 || ad & top33)
	{
		sint64 mask = top4;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- >= 8 && sn == 0 && sd == 0; mask >>= 4)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max_64(sn, sd) - 7) * 4;
		n = n / (1LL << s);
		d = d / (1LL << s);
	}
	return *this;
}
