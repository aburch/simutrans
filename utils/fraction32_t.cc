/*
 * fraction32_t.cc
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#ifdef DEBUG_COUT
#include <iostream>
using namespace std;
#endif

#include "fraction32_t.h"

#define BITS 32

// used by shrink()
static const int32 top2 	= 0x60000000L;
static const int32 top17	= 0xffff8000L;

// used by ild()
static const int32 top8	= 0xff000000L;
static const int32 top16	= 0xffff0000L;
static const int32 top24	= 0xffffff00L;

// used by fraction32_t(double)
static const int32 maxlong = 0x7fffffffL;

const int32 common_factor(int32 a, int32 b)
// Returns the largest common factor in a and be or 1, if there is no common factor.
{
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;

	int32 c;

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

const int ild(int32 x)
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
	int32 msk;
	int r;
	if (x < 0)
		x = -x;
	if (x & top16)
	{
		if (x & top8)
		{
			msk = 0x40000000L;
			r = 31;
		}
		else
		{
			msk = 0x00800000L;
			r = 24;
		}
	}
	else
	{
		if (x & top24)
		{
			msk = 0x00008000L;
			r = 16;
		}
		else
		{
			msk = 0x00000080L;
			r = 8;
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

const fraction32_t log(const fraction32_t &x)
{
	// requires x > 0
	fraction32_t t1 = fraction32_t(x.n - x.d, x.n + x.d);
	t1.shorten();
	fraction32_t t2 = t1 * t1;
	fraction32_t r = t1;
	fraction32_t l = r;
	int32 n = 1;
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

const fraction32_t exp(const fraction32_t &x)
{
	fraction32_t t = x.shorten();
	fraction32_t t1 = t;
	fraction32_t r = (x + 1).shorten();
	fraction32_t l = r;
	int32 n = 1;
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

fraction32_t::fraction32_t(double value)
{
	double a = fabs(value);
	if (a > maxlong)
	{
		n = 1;
		d = 0;
	}
	else if (a < 1/maxlong)
	{
		n = 0;
		d = 1;
	}
	else
	{
		int m = (int)(log((float)maxlong) / log((float)10.0));
		int x = (int)(log(a) / log((float)10.0));
		d = (int32) exp(min(m - x, 8) * log((float)10.0));
		n = (int32) (value * d);
	}
}

const fraction32_t & fraction32_t::operator += (const fraction32_t &f)
{
	if (d == f.d)
	{
		int iun = ild(n), ivn = ild(f.n);
		if (iun + ivn < 2 * BITS - 1)
		{
			n += f.n;
			return *this;
		}
	}

	fraction32_t v(f.n, f.d);
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
	return *this;
}

const fraction32_t & fraction32_t::operator *= (const fraction32_t &f)
{
	fraction32_t v(f.n, f.d);
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
	return *this;
}

const fraction32_t fraction32_t::shrink() const
{
	fraction32_t r = *this;
	int32 an = n >= 0 ? n : -n;
	int32 ad = d >= 0 ? d : -d;;
	if (an & top17 || ad & top17)
	{
		int32 mask = top2;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- > 8 && sn == 0 && sd == 0; mask >>= 2)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max(sn, sd) - 7) * 2;
		r.n = n / (1LL << s);
		r.d = d / (1LL << s);
	}
	return r;
}

const fraction32_t & fraction32_t::shrink()
{
	int32 an = n >= 0 ? n : -n;
	int32 ad = d >= 0 ? d : -d;;
	if (an & top17 || ad & top17)
	{
		int32 mask = top2;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- >= 8 && sn == 0 && sd == 0; mask >>= 2)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max(sn, sd) - 7) * 2;
		n = n / (1LL << s);
		d = d / (1LL << s);
	}
	return *this;
}
