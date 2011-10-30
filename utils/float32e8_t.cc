/*
 * float32e8_t.cc
 *
 *  Created on: 22.05.2011
 *      Author: Bernd
 */

#include <stdlib.h>
#include "float32e8_t.h"
#include "../simdebug.h"

ostream & operator << (ostream &out, const float32e8_t &x)
{
	out << x.to_double();
	return out;
}

//remember: most significant bit is always set except for 0.
//after any operation the mantissa is shifted and the exponent is in-/decremented accordingly.
//        {           m,   e,    ms };
//  0.00 = { 0x00000000L,  0, false };
//  0.25 = { 0x80000000L, -1, false };
//  0.50 = { 0x80000000L,  0, false };
//  1.00 = { 0x80000000L,  1, false };
//  2.00 = { 0x80000000L,  2, false };
// -2.00 = { 0x80000000L,  2,  true };
// -5.00 = { 0xA0000000L,  3,  true };

static const double log_of_2 = log(2.0);

#define EXPONENT_BITS 10
#define MIN_EXPONENT -1023
#define MAX_EXPONENT 1023
#define MAX_MANTISSA 0xffffffff

uint8 ild(const uint32 x)
// "integer logarithmus digitalis"
// Returns the number of the highest used bit of abs(x):
// 0: no bits in use x == 0
// 1: x == 1
// 2: 2 <= x < 4
// 3: 4 <= x < 8
// ...
// 32: 2^31 <= x < 2^32
{
	uint32 msk;
	uint8 r;
	if (x & 0xffff0000L)
	{
		if (x & 0xff000000L)
		{
			msk = 0x80000000L;
			r = 32;
		}
		else
		{
			msk = 0x00800000L;
			r = 24;
		}
	}
	else
	{
		if (x & 0xffffff00L)
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

struct float32e8_pair_t {
	float32e8_t v;
	float32e8_t r;
};

#define LD_TBL_LEN 31
const float32e8_pair_t ld_tbl[LD_TBL_LEN] =
{
		{ float32e8_t(0xc0000000, 1, false), float32e8_t(0x95c01a3a, 0, false) }, // 0.584963
		{ float32e8_t(0xa0000000, 1, false), float32e8_t(0xa4d3c25e, -1, false) }, // 0.321928
		{ float32e8_t(0x90000000, 1, false), float32e8_t(0xae00d1d0, -2, false) }, // 0.169925
		{ float32e8_t(0x88000000, 1, false), float32e8_t(0xb31fb7d6, -3, false) }, // 0.0874628
		{ float32e8_t(0x84000000, 1, false), float32e8_t(0xb5d69bac, -4, false) }, // 0.0443941
		{ float32e8_t(0x82000000, 1, false), float32e8_t(0xb73cb42e, -5, false) }, // 0.0223678
		{ float32e8_t(0x81000000, 1, false), float32e8_t(0xb7f285b7, -6, false) }, // 0.0112273
		{ float32e8_t(0x80800000, 1, false), float32e8_t(0xb84e236c, -7, false) }, // 0.00562455
		{ float32e8_t(0x80400000, 1, false), float32e8_t(0xb87c1ff8, -8, false) }, // 0.00281502
		{ float32e8_t(0x80200000, 1, false), float32e8_t(0xb89329ba, -9, false) }, // 0.00140819
		{ float32e8_t(0x80100000, 1, false), float32e8_t(0xb89eb17c, -10, false) }, // 0.000704269
		{ float32e8_t(0x80080000, 1, false), float32e8_t(0xb8a47615, -11, false) }, // 0.000352177
		{ float32e8_t(0x80040000, 1, false), float32e8_t(0xb8a75890, -12, false) }, // 0.000176099
		{ float32e8_t(0x80020000, 1, false), float32e8_t(0xb8a8c9d9, -13, false) }, // 8.80524e-05
		{ float32e8_t(0x80010000, 1, false), float32e8_t(0xb8a98280, -14, false) }, // 4.40269e-05
		{ float32e8_t(0x80008000, 1, false), float32e8_t(0xb8a9ded4, -15, false) }, // 2.20136e-05
		{ float32e8_t(0x80004000, 1, false), float32e8_t(0xb8aa0cff, -16, false) }, // 1.10068e-05
		{ float32e8_t(0x80002000, 1, false), float32e8_t(0xb8aa2414, -17, false) }, // 5.50343e-06
		{ float32e8_t(0x80001000, 1, false), float32e8_t(0xb8aa2f9f, -18, false) }, // 2.75172e-06
		{ float32e8_t(0x80000800, 1, false), float32e8_t(0xb8aa3564, -19, false) }, // 1.37586e-06
		{ float32e8_t(0x80000400, 1, false), float32e8_t(0xb8aa3847, -20, false) }, // 6.8793e-07
		{ float32e8_t(0x80000200, 1, false), float32e8_t(0xb8aa39b8, -21, false) }, // 3.43965e-07
		{ float32e8_t(0x80000100, 1, false), float32e8_t(0xb8aa3a71, -22, false) }, // 1.71983e-07
		{ float32e8_t(0x80000080, 1, false), float32e8_t(0xb8aa3acd, -23, false) }, // 8.59913e-08
		{ float32e8_t(0x80000040, 1, false), float32e8_t(0xb8aa3afb, -24, false) }, // 4.29957e-08
		{ float32e8_t(0x80000020, 1, false), float32e8_t(0xb8aa3b12, -25, false) }, // 2.14978e-08
		{ float32e8_t(0x80000010, 1, false), float32e8_t(0xb8aa3b1e, -26, false) }, // 1.07489e-08
		{ float32e8_t(0x80000008, 1, false), float32e8_t(0xb8aa3b24, -27, false) }, // 5.37446e-09
		{ float32e8_t(0x80000004, 1, false), float32e8_t(0xb8aa3b26, -28, false) }, // 2.68723e-09
		{ float32e8_t(0x80000002, 1, false), float32e8_t(0xb8aa3b28, -29, false) }, // 1.34361e-09
		{ float32e8_t(0x80000001, 1, false), float32e8_t(0xb8aa3b29, -30, false) }, // 6.71807e-10
};

const float32e8_pair_t & get_pair(int i) { return ld_tbl[i]; }

const float32e8_t float32e8_t::log2() const
{
	if (ms || m == 0L)
	{
		dbg->error("float32e8_t float32e8_t::log2()", "Illegal argument of log2(%.9G): must be > 0.", to_double());
	}
	float32e8_t r((sint32)(e - 1L));
	float32e8_t v(m, 1, false);
	int i = 0;
	while (i < LD_TBL_LEN && v != one)
	{
#ifdef DEBUG_COUT
		cout << "\t" << i << ") v.e = " << v.e << ", v = " << v << ", r = " << r << "\n"; cout.flush();
#endif
		const float32e8_pair_t &p = get_pair(i);
		if (v.e <= 0)
		{
			v *= p.v;
			r -= p.r;
			if (v.e > 0) i++;
		}
		else
		{
			v /= p.v;
			r += p.r;
			if (v.e <= 0) i++;
		}
	}
	return r;
}

const float32e8_t float32e8_t::exp2() const
{
	if (!m)	return one;

	sint16 e1 = e < 0 ? -e : e;
	if (e > EXPONENT_BITS)
	{
		dbg->error(" float32e8_t::exp2()", "Illegal argument of exp2(%.9G): must be between about %d and %d.", to_double(), MIN_EXPONENT, MAX_EXPONENT);
	}
	uint32 m1 = e > 0 ? m >> (32 - e) : 0;
	float32e8_t v(0x80000000L, (sint16)m1 + 1, false);
	if (e >= -EXPONENT_BITS)
	{
		// if e < -EXPONENT_BITS, the result is a little bit more than 1.0, but beyond the precision of a float32e8_t.
		uint32 m2 = m << e1;
		if (!m2) return v;
		uint8 ld = 32 - ild(m2);
		float32e8_t r(m2 << ld, -ld, false);

		int i = 0;
		while (i < LD_TBL_LEN && r.m != 0)
		{
#ifdef DEBUG_COUT
//			cout << "\t" << i << ") v.e = " << v.e << ", v = " << v << ", r = " << r << "\n"; cout.flush();
#endif
			const float32e8_pair_t &p = get_pair(i);
			if (!r.ms)
			{
				v *= p.v;
				r -= p.r;
				if (r.ms) i++;
			}
			else
			{
				v /= p.v;
				r += p.r;
				if (!r.ms) i++;
			}
		}

		if (ms)	return one / v;
	}
	return v;
}

const sint16 float32e8_t::min_exponent = MIN_EXPONENT;
const sint16 float32e8_t::max_exponent = MAX_EXPONENT;
const uint32 float32e8_t::max_mantissa = MAX_MANTISSA;

// some "integer" constants.
const float32e8_t float32e8_t::zero((uint32) 0);
const float32e8_t float32e8_t::one((uint32) 1);
const float32e8_t float32e8_t::two((uint32) 2);
const float32e8_t float32e8_t::three((uint32) 3);

// some "fractional" constants.
const float32e8_t float32e8_t::tenth((uint32) 1, (uint32) 10);
const float32e8_t float32e8_t::third((uint32) 1, (uint32)  3);
const float32e8_t float32e8_t::half((uint32) 1, (uint32) 2);

#ifdef USE_DOUBLE
void float32e8_t::set_value(const double value)
{
	ms = value < 0;
	double v = ms ? -value : value;
	e = (sint16)ceil(::log2(v));
	if (e < MIN_EXPONENT)
	{
		set_zero();
		return;
	}
	if (e > MAX_EXPONENT)
	{
		char val_str[200];
		dbg->error("float32e8_t::set_value(const double value)", "Overflow in converting %G to float32e8_t: exponent %d > %d", value, e, MIN_EXPONENT);
	}
	v = v * pow(2, (32 - e)) + 0.5;
	m = (uint32) v;
}
#endif

void float32e8_t::set_value(const sint32 value)
{
	if (value < 0)
	{
		set_value((uint32)-value);
		ms = true;
	}
	else
		set_value((uint32)value);
}

void float32e8_t::set_value(const uint32 value)
{
	switch (value)
	{
	case 0:
		set_zero();
		return;

	case 1:
		if (one.m) // after initializing one.m is no longer 0
		{
			set_value(one);
			return;
		}

	case 2:
		if (two.m) // after initializing two.m is no longer 0
		{
			set_value(two);
			return;
		}

	case 3:
		if (three.m) // after initializing three.m is no longer 0
		{
			set_value(three);
			return;
		}
	}
	ms = false;
	e = ild(value);
	m = (value) << (32 - e);
}

void float32e8_t::set_value(const sint64 value)
{
	if (value < 0)
	{
		set_value((uint64)-value);
		ms = true;
	}
	else
		set_value((uint64)value);
}

void float32e8_t::set_value(const uint64 value)
{
	m = value >> 32;
	if (m)
	{
		ms = false;
		e = ild(m) + 32;
		m = (uint32)(value >> (64 - e));
	}
	else
	{
		set_value((uint32) value);
	}
}

const float32e8_t float32e8_t::operator + (const float32e8_t & x) const
{
	if (!m) return x;
	if (!x.m) return *this;

	sint16 msx = x.e;
	sint16 msy = e;
	sint16 dms = msx - msy;
	bool x_op_y = dms > 0 || (dms == 0 && x.m > m);

	uint32 op1;
	uint32 op2;
	float32e8_t r;
	if (x_op_y)
	{
		r.ms = x.ms;
		r.e = msx;
		if (dms >= 32)
		{
			r.m = x.m;
			return r;
		}
		op1 = x.m;
		op2 = m >> dms;
	}
	else
	{
		r.ms = ms;
		r.e = msy;
		dms = -dms;
		if (dms >= 32)
		{
			r.m = m;
			return r;
		}
		op1 = m;
		op2 = x.m >> dms;
	}

	if (ms == x.ms)
	{
		// add
		r.m = op1 + op2;
		if (r.m < op1)
		{
			// overflown
			r.e++;
			r.m = 0x80000000 | r.m >> 1;
		}

		if (r.e > MAX_EXPONENT)
		{
			dbg->error("float32e8_t::operator + (const float32e8_t & x) const", "Overflow in: %.9G + %.9G", this->to_double(), x.to_double());
		}
	}
	else
	{
		// sub
		r.m = op1 - op2;
		if (!(r.m & 0x80000000))
		{
			if (!r.m)
			{
				r.set_zero();
				return r;
			}
			uint8 ld = 32 - ild(r.m);
			r.e -= ld;
			r.m <<= ld;
		}

		if (r.e < MIN_EXPONENT)
		{
			r.set_zero();
			return r;
		}
	}
	return r;
}

const float32e8_t float32e8_t::operator - (const float32e8_t & x) const
{
	if (!m) return -x;
	if (!x.m) return *this;

	sint16 msx = x.e;
	sint16 msy = e;
	sint16 dms = msx - msy;
	bool x_op_y = dms > 0 || (dms == 0 && x.m > m);

	uint32 op1;
	uint32 op2;
	float32e8_t r;
	if (x_op_y)
	{
		r.ms = !x.ms;
		r.e = msx;
		if (dms >= 32)
		{
			r.m = x.m;
			return r;
		}
		op1 = x.m;
		op2 = m >> dms;
	}
	else
	{
		r.ms = ms;
		r.e = msy;
		dms = -dms;
		if (dms >= 32)
		{
			r.m = m;
			return r;
		}
		op1 = m;
		op2 = x.m >> dms;
	}

	if (ms != x.ms)
	{
		// add
		r.m = op1 + op2;
		if (r.m < op1)
		{
			// overflown
			r.e++;
			r.m = 0x80000000 | r.m >> 1;
		}

		if (r.e > MAX_EXPONENT)
		{
			dbg->error("float32e8_t::operator - (const float32e8_t & x) const", "Overflow in: %.9G - %.9G", this->to_double(), x.to_double());
		}
	}
	else
	{
		// sub
		r.m = op1 - op2;
		if (!(r.m & 0x80000000))
		{
			if (!r.m)
			{
				r.set_zero();
				return r;
			}
			uint8 ld = 32 - ild(r.m);
			r.e -= ld;
			r.m <<= ld;
		}

		if (r.e < MIN_EXPONENT)
		{
			r.set_zero();
			return r;
		}
	}
	return r;
}

const float32e8_t float32e8_t::operator * (const float32e8_t & x) const
{
	if (!m || !x.m)
	{
		float32e8_t r;
		r.set_zero();
		return r;
	}
	if (m == 0x80000000L && e == 0x01)
	{
		if (ms)
			return -x;
		return x;
	}
	if (x.m == 0x80000000L && x.e == 0x01)
	{
		if (x.ms)
			return -*this;
		return *this;
	}

	uint64 rm = (uint64) m * (uint64) x.m;
	float32e8_t r;
	r.e = e + x.e;
	r.m = (uint32) (rm >> 32);
	if (!(r.m & 0x80000000L))
	{
		r.m = (uint32) (rm >> 31);
		r.e--;
	}
	if (r.e < MIN_EXPONENT)
	{
		float32e8_t r;
		r.set_zero();
		return r;
	}
	if (r.e > MAX_EXPONENT)
	{
		dbg->error("float32e8_t::operator * (const float32e8_t & x) const", "Overflow in: %.9G * %.9G", this->to_double(), x.to_double());
	}
	r.ms = ms ^ x.ms;
	return r;
}

const float32e8_t float32e8_t::operator / (const float32e8_t & x) const
{
	if (x.m == 0)
	{
		dbg->error("float32e8_t::operator / (const float32e8_t & x) const", "Division by zero in: %.9G / %.9G", this->to_double(), x.to_double());
	}

	uint64 rm = ((uint64)m << 32) / x.m;
	float32e8_t r;
	r.e = e - x.e;
	if (m >= x.m)
	{
		r.m = (uint32) (rm >> 1);
		r.e++;
	}
	else
	{
		r.m = (uint32) rm;
	}
	if (r.e > MAX_EXPONENT)
	{
		dbg->error("float32e8_t::operator / (const float32e8_t & x) const", "Overflow in: %.9G / %.9G", this->to_double(), x.to_double());
	}
	if (r.e < MIN_EXPONENT)
	{
		float32e8_t r;
		r.set_zero();
		return r;
	}
	r.ms = ms ^ x.ms;
	return r;
}

double float32e8_t::to_double() const
{
	double rm = ms ? -(double)m : m;
	double re = exp((e - 32) * log_of_2);
	return rm * re;
}

sint32 float32e8_t::to_sint32() const
{
	// return trunc(*this):
	if (e <= 0)
		return 0; // abs(*this) < 1
	if (e > 32)
	{
		dbg->error("float32e8_t::to_sint32() const", "Cannot convert float32e8_t value %G to sint32: exponent %d >= 32 exceed sint32 range", to_double(), e);
	}
	uint32 rm = m >> (32 - e);
	return ms ? -(sint32) rm : (sint32) rm;
}

//const string float32e8_t::to_string() const
//{
//	char buf[256];
//	sprintf(buf, "float32e8_t(0x%08lx, %d, %s)", m, e, ms ? "true" : "false");
//	string result(buf);
//	return result;
//}
