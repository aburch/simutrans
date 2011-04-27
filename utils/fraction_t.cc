/*
 * fraction_t.cc
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#include "fraction_t.h"

const int64 common_factor(int64 a, int64 b)
// Returns the largest common factor in a and be or 1, if there is no common factor.
{
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;

	int64 c;

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

const int ild(int64 x)
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
	int64 msk;
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

const fraction_t log(const fraction_t &x)
{
	// requires x > 0
	fraction_t t1 = fraction_t(x.n - x.d, x.n + x.d);
	t1.shorten();
	fraction_t t2 = t1 * t1;
	fraction_t r = t1;
	fraction_t l = r;
	int64 n = 1;
	while ((n += 2) < 50)
	{
		t1 *= t2;
		r += t1 / n;
		r.shorten();
//		cout << n << ": log(" << x.n << " / " << x.d << ") = " << 2 * r.n << " / " << r.d << " = " << (double) 2 * r.n / r.d << "\n";
//		cout.flush();
		if (r == l)
			break;
		l = r;
	}
	return r * 2;
}

const fraction_t exp(const fraction_t &x)
{
	fraction_t t = x.shorten();
	fraction_t t1 = t;
	fraction_t r = (x + 1).shorten();
	fraction_t l = r;
	int64 n = 1;
	while (++n < 30)
	{
		t1 *= t / n;
		r += t1;
		r.shorten();
//		cout << n << ": exp(" << x.n << " / " << x.d << ") = " << r.n << " / " << r.d << " = " << (double) r.n / r.d << "\n";
//		cout.flush();
		if (r == l)
			break;
		l = r;
	}
	return r;
}

fraction_t::fraction_t(double value)
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
		d = (int64) exp(min(m - x, 12) * log(10));
		n = (int64) (value * d);
	}
}

const fraction_t & fraction_t::operator += (const fraction_t &f)
{
	if (d == f.d)
	{
		int iun = ild(n), ivn = ild(f.n);
		if (iun + ivn < 127)
		{
			n += f.n;
			return *this;
		}
	}

	fraction_t v(f.n, f.d);
	int iun = ild(n), ivd = ild(v.d);
	if (iun + ivd >= 64)
	{
		if (iun >= 32)
			shrink();
		if (ivd >= 32)
		{
			v.shrink();
			ivd = ild(v.d);
		}
	}
	int iud = ild(d), ivn = ild(v.n);
	if (iud + ivn >= 64)
	{
		if (iud >= 32)
		{
			shrink();
			iud = ild(d);
		}
		if (ivn >= 32)
			v.shrink();
	}
	if (iud + ivd >= 64)
	{
		if (iud >= 32)
			shrink();
		if (ivd >= 32)
			v.shrink();
	}
	n = n * v.d + d * v.n;
	d = d * v.d;
	return *this;
}

const fraction_t & fraction_t::operator *= (const fraction_t &f)
{
	fraction_t v(f.n, f.d);
	int iun = ild(n), ivn = ild(v.n);
	if (iun + ivn >= 64)
	{
		if (iun >= 32)
			shrink();
		if (ivn >= 32)
			v.shrink();
	}
	int iud = ild(d), ivd = ild(v.d);
	if (iud + ivd >= 64)
	{
		if (iud >= 32)
			shrink();
		if (ivd >= 32)
			v.shrink();
	}
	n *= v.n;
	d *= v.d;
	return *this;
}

const fraction_t fraction_t::shrink() const
{
	fraction_t r = *this;
	int64 an = n >= 0 ? n : -n;
	int64 ad = d >= 0 ? d : -d;;
	if (an & top33 || ad & top33)
	{
		int64 mask = top4;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- > 8 && sn == 0 && sd == 0; mask >>= 4)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max(sn, sd) - 7) * 4;
		r.n = n / (1LL << s);
		r.d = d / (1LL << s);
	}
	return r;
}

const fraction_t & fraction_t::shrink()
{
	int64 an = n >= 0 ? n : -n;
	int64 ad = d >= 0 ? d : -d;;
	if (an & top33 || ad & top33)
	{
		int64 mask = top4;
		int s, sn = 0, sd = 0;
		for (s = 16; s-- >= 8 && sn == 0 && sd == 0; mask >>= 4)
		{
			if (sn == 0 && an & mask)
				sn = s;
			if (sd == 0 && ad & mask)
				sd = s;
		}
		s = (max(sn, sd) - 7) * 4;
		n = n / (1LL << s);
		d = d / (1LL << s);
	}
	return *this;
}
