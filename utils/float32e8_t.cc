/*
 * float32e8_t.cc
 *
 *  Created on: 22.05.2011
 *      Author: Bernd Gabriel
 */

#include <stdlib.h>
#include "float32e8_t.h"
#include "../simdebug.h"
#include "../dataobj/loadsave.h"

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

#define EXPONENT_BITS 10
#define MIN_EXPONENT -1023
#define MAX_EXPONENT 1023
#define MAX_MANTISSA 0xffffffff

const uint8 float32e8_t::_ild[256] =
{
	 0,  1,  2,  2,  3,  3,  3,  3,    4,  4,  4,  4,  4,  4,  4,  4,   //   0.. 15
	 5,  5,  5,  5,  5,  5,  5,  5,    5,  5,  5,  5,  5,  5,  5,  5,   //  16.. 31
	 6,  6,  6,  6,  6,  6,  6,  6,    6,  6,  6,  6,  6,  6,  6,  6,   //  32.. 
	 6,  6,  6,  6,  6,  6,  6,  6,    6,  6,  6,  6,  6,  6,  6,  6,   //    .. 63

	 7,  7,  7,  7,  7,  7,  7,  7,    7,  7,  7,  7,  7,  7,  7,  7,   //  64.. 
	 7,  7,  7,  7,  7,  7,  7,  7,    7,  7,  7,  7,  7,  7,  7,  7,   //  
	 7,  7,  7,  7,  7,  7,  7,  7,    7,  7,  7,  7,  7,  7,  7,  7,   //  
	 7,  7,  7,  7,  7,  7,  7,  7,    7,  7,  7,  7,  7,  7,  7,  7,   //    ..127

	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   // 128.. 
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   //  
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   //  
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   //    ..191

	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   // 192.. 
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   //  
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8,   //  
	 8,  8,  8,  8,  8,  8,  8,  8,    8,  8,  8,  8,  8,  8,  8,  8    //    ..255
};

//uint8 float32e8_t::ild(const uint32 x)
//// "integer logarithmus digitalis"
//// Returns the number of the highest used bit of abs(x):
//// 0: no bits in use x == 0
//// 1: x == 1
//// 2: 2 <= x < 4
//// 3: 4 <= x < 8
//// ...
//// 32: 2^31 <= x < 2^32
//{
//	if (x & 0xffff0000L)
//	{
//		if (x & 0xff000000L)
//		{
//			return 24 + _ild[x>>24];
//		}
//		else
//		{
//			return 16 + _ild[x>>16];
//		}
//	}
//	else
//	{
//		if (x & 0xffffff00L)
//		{
//			return 8 + _ild[x>>8];
//		}
//		else
//		{
//			return _ild[x];
//		}
//	}
//}

struct float32e8_pair_t {
	float32e8_t v;
	float32e8_t r;
};

// This is a list of 1+2^-i, log2(1+2^-i) for i between 1 and 31
// The logs are rounded to the nearest representable float
// They have been verified by ACarlotti using integer arithmetic in Python
#define LD_TBL_LEN 31
const float32e8_pair_t ld_tbl[LD_TBL_LEN] =
{
		{ float32e8_t(0xc0000000, 1, false), float32e8_t(0x95c01a3a,   0, false) }, // 0.584963
		{ float32e8_t(0xa0000000, 1, false), float32e8_t(0xa4d3c25e,  -1, false) }, // 0.321928
		{ float32e8_t(0x90000000, 1, false), float32e8_t(0xae00d1d0,  -2, false) }, // 0.169925
		{ float32e8_t(0x88000000, 1, false), float32e8_t(0xb31fb7d6,  -3, false) }, // 0.0874628
		{ float32e8_t(0x84000000, 1, false), float32e8_t(0xb5d69bac,  -4, false) }, // 0.0443941
		{ float32e8_t(0x82000000, 1, false), float32e8_t(0xb73cb42e,  -5, false) }, // 0.0223678
		{ float32e8_t(0x81000000, 1, false), float32e8_t(0xb7f285b7,  -6, false) }, // 0.0112273
		{ float32e8_t(0x80800000, 1, false), float32e8_t(0xb84e236c,  -7, false) }, // 0.00562455
		{ float32e8_t(0x80400000, 1, false), float32e8_t(0xb87c1ff8,  -8, false) }, // 0.00281502
		{ float32e8_t(0x80200000, 1, false), float32e8_t(0xb89329ba,  -9, false) }, // 0.00140819
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
		return zero;
	}
	float32e8_t r((sint32)(e - 1L));
	float32e8_t v(m, 1, false);
	for (int i = 0; i < LD_TBL_LEN; i++)
	{
		const float32e8_pair_t &p = get_pair(i);
		if (p.v <= v)
		{
			v /= p.v;
			r += p.r;
		}
	}
	return r;
}

const float32e8_t float32e8_t::exp2() const
{
	if (!m)	return one;

	if (e > EXPONENT_BITS)
	{
		dbg->error(" float32e8_t::exp2()", "Illegal argument of exp2(%.9G): must be between about %d and %d.", to_double(), MIN_EXPONENT, MAX_EXPONENT);
	}
	sint16 e1 = e > 0 ? m >> (32 - e) : 0;
	e1 = ms ? - 1 - e1 : e1;

	float32e8_t v(0x80000000, e1+1, false);
	float32e8_t r(m, e, ms);
	r -= e1;

	for (int i = 0; i < LD_TBL_LEN; i++)
	{
		const float32e8_pair_t &p = get_pair(i);
		if (p.r <= r)
		{
			r -= p.r;
			v *= p.v;
		}
	}
	// We do this last step to improve rounding and reduce bias:
	const float32e8_pair_t &p = get_pair(LD_TBL_LEN-1);
	if (p.r <= r * 2)
	{
		v *= p.v;
	}

	return v;
}

const sint16 float32e8_t::min_exponent = MIN_EXPONENT;
const sint16 float32e8_t::max_exponent = MAX_EXPONENT;
const uint32 float32e8_t::max_mantissa = MAX_MANTISSA;

// used to initialize integers[] used in float32e8_t::set_value.
class float32e8ini_t : public float32e8_t
{
public:
	inline float32e8ini_t(const uint32 value) 
	{ 
		ms = false; 
		e = ild(value); 
		m = (value) << (32 - e); 
	} 
};

const float32e8_t float32e8_t::integers[257] = 
{
	float32e8ini_t(  0), float32e8ini_t(  1), float32e8ini_t(  2), float32e8ini_t(  3),	float32e8ini_t(  4),	
	float32e8ini_t(  5), float32e8ini_t(  6), float32e8ini_t(  7), float32e8ini_t(  8),	float32e8ini_t(  9),	
	float32e8ini_t( 10), float32e8ini_t( 11), float32e8ini_t( 12), float32e8ini_t( 13), float32e8ini_t( 14), 
	float32e8ini_t( 15), float32e8ini_t( 16), float32e8ini_t( 17), float32e8ini_t( 18), float32e8ini_t( 19),
	float32e8ini_t( 20), float32e8ini_t( 21), float32e8ini_t( 22), float32e8ini_t( 23), float32e8ini_t( 24), 
	float32e8ini_t( 25), float32e8ini_t( 26), float32e8ini_t( 27), float32e8ini_t( 28), float32e8ini_t( 29),
	float32e8ini_t( 30), float32e8ini_t( 31), float32e8ini_t( 32), float32e8ini_t( 33), float32e8ini_t( 34), 
	float32e8ini_t( 35), float32e8ini_t( 36), float32e8ini_t( 37), float32e8ini_t( 38), float32e8ini_t( 39),
	float32e8ini_t( 40), float32e8ini_t( 41), float32e8ini_t( 42), float32e8ini_t( 43), float32e8ini_t( 44), 
	float32e8ini_t( 45), float32e8ini_t( 46), float32e8ini_t( 47), float32e8ini_t( 48), float32e8ini_t( 49),
	float32e8ini_t( 50), float32e8ini_t( 51), float32e8ini_t( 52), float32e8ini_t( 53), float32e8ini_t( 54), 
	float32e8ini_t( 55), float32e8ini_t( 56), float32e8ini_t( 57), float32e8ini_t( 58), float32e8ini_t( 59),
	float32e8ini_t( 60), float32e8ini_t( 61), float32e8ini_t( 62), float32e8ini_t( 63), float32e8ini_t( 64), 
	float32e8ini_t( 65), float32e8ini_t( 66), float32e8ini_t( 67), float32e8ini_t( 68), float32e8ini_t( 69),
	float32e8ini_t( 70), float32e8ini_t( 71), float32e8ini_t( 72), float32e8ini_t( 73), float32e8ini_t( 74), 
	float32e8ini_t( 75), float32e8ini_t( 76), float32e8ini_t( 77), float32e8ini_t( 78), float32e8ini_t( 79),
	float32e8ini_t( 80), float32e8ini_t( 81), float32e8ini_t( 82), float32e8ini_t( 83), float32e8ini_t( 84), 
	float32e8ini_t( 85), float32e8ini_t( 86), float32e8ini_t( 87), float32e8ini_t( 88), float32e8ini_t( 89),
	float32e8ini_t( 90), float32e8ini_t( 91), float32e8ini_t( 92), float32e8ini_t( 93), float32e8ini_t( 94), 
	float32e8ini_t( 95), float32e8ini_t( 96), float32e8ini_t( 97), float32e8ini_t( 98), float32e8ini_t( 99),
	float32e8ini_t(100), float32e8ini_t(101), float32e8ini_t(102), float32e8ini_t(103), float32e8ini_t(104), 
	float32e8ini_t(105), float32e8ini_t(106), float32e8ini_t(107), float32e8ini_t(108), float32e8ini_t(109),	
	float32e8ini_t(110), float32e8ini_t(111), float32e8ini_t(112), float32e8ini_t(113), float32e8ini_t(114), 
	float32e8ini_t(115), float32e8ini_t(116), float32e8ini_t(117), float32e8ini_t(118), float32e8ini_t(119),
	float32e8ini_t(120), float32e8ini_t(121), float32e8ini_t(122), float32e8ini_t(123), float32e8ini_t(124), 
	float32e8ini_t(125), float32e8ini_t(126), float32e8ini_t(127), float32e8ini_t(128), float32e8ini_t(129),
	float32e8ini_t(130), float32e8ini_t(131), float32e8ini_t(132), float32e8ini_t(133), float32e8ini_t(134),
	float32e8ini_t(135), float32e8ini_t(136), float32e8ini_t(137), float32e8ini_t(138), float32e8ini_t(139),
	float32e8ini_t(140), float32e8ini_t(141), float32e8ini_t(142), float32e8ini_t(143), float32e8ini_t(144), 
	float32e8ini_t(145), float32e8ini_t(146), float32e8ini_t(147), float32e8ini_t(148), float32e8ini_t(149),
	float32e8ini_t(150), float32e8ini_t(151), float32e8ini_t(152), float32e8ini_t(153), float32e8ini_t(154), 
	float32e8ini_t(155), float32e8ini_t(156), float32e8ini_t(157), float32e8ini_t(158), float32e8ini_t(159),
	float32e8ini_t(160), float32e8ini_t(161), float32e8ini_t(162), float32e8ini_t(163), float32e8ini_t(164), 
	float32e8ini_t(165), float32e8ini_t(166), float32e8ini_t(167), float32e8ini_t(168), float32e8ini_t(169),
	float32e8ini_t(170), float32e8ini_t(171), float32e8ini_t(172), float32e8ini_t(173), float32e8ini_t(174), 
	float32e8ini_t(175), float32e8ini_t(176), float32e8ini_t(177), float32e8ini_t(178), float32e8ini_t(179),
	float32e8ini_t(180), float32e8ini_t(181), float32e8ini_t(182), float32e8ini_t(183), float32e8ini_t(184), 
	float32e8ini_t(185), float32e8ini_t(186), float32e8ini_t(187), float32e8ini_t(188), float32e8ini_t(189),
	float32e8ini_t(190), float32e8ini_t(191), float32e8ini_t(192), float32e8ini_t(193), float32e8ini_t(194), 
	float32e8ini_t(195), float32e8ini_t(196), float32e8ini_t(197), float32e8ini_t(198), float32e8ini_t(199),
	float32e8ini_t(200), float32e8ini_t(201), float32e8ini_t(202), float32e8ini_t(203), float32e8ini_t(204), 
	float32e8ini_t(205), float32e8ini_t(206), float32e8ini_t(207), float32e8ini_t(208), float32e8ini_t(209),	
	float32e8ini_t(210), float32e8ini_t(211), float32e8ini_t(212), float32e8ini_t(213), float32e8ini_t(214), 
	float32e8ini_t(215), float32e8ini_t(216), float32e8ini_t(217), float32e8ini_t(218), float32e8ini_t(219),
	float32e8ini_t(220), float32e8ini_t(221), float32e8ini_t(222), float32e8ini_t(223), float32e8ini_t(224), 
	float32e8ini_t(225), float32e8ini_t(226), float32e8ini_t(227), float32e8ini_t(228), float32e8ini_t(229),
	float32e8ini_t(230), float32e8ini_t(231), float32e8ini_t(232), float32e8ini_t(233), float32e8ini_t(234), 
	float32e8ini_t(235), float32e8ini_t(236), float32e8ini_t(237), float32e8ini_t(238), float32e8ini_t(239),
	float32e8ini_t(240), float32e8ini_t(241), float32e8ini_t(242), float32e8ini_t(243), float32e8ini_t(244), 
	float32e8ini_t(245), float32e8ini_t(246), float32e8ini_t(247), float32e8ini_t(248), float32e8ini_t(249),
	float32e8ini_t(250), float32e8ini_t(251), float32e8ini_t(252), float32e8ini_t(253), float32e8ini_t(254), 
	float32e8ini_t(255), float32e8ini_t(256) 
};

// some "integer" constants.
const float32e8_t float32e8_t::zero((uint32) 0);
const float32e8_t float32e8_t::one((uint32) 1);
const float32e8_t float32e8_t::two((uint32) 2);
const float32e8_t float32e8_t::three((uint32) 3);
const float32e8_t float32e8_t::four((uint32) 4);
const float32e8_t float32e8_t::ten((uint32) 10);
const float32e8_t float32e8_t::hundred((uint32) 100);
const float32e8_t float32e8_t::thousand((uint32) 1000);
const float32e8_t float32e8_t::tenthousand((uint32) 10000);

// some "fractional" constants.
const float32e8_t float32e8_t::tenth((uint32) 1, (uint32) 10);
const float32e8_t float32e8_t::quarter((uint32) 1, (uint32)  4);
const float32e8_t float32e8_t::third((uint32) 1, (uint32)  3);
const float32e8_t float32e8_t::half((uint32) 1, (uint32) 2);
const float32e8_t float32e8_t::milli((uint32) 1, (uint32) 1000);
const float32e8_t float32e8_t::micro((uint32) 1, (uint32) 1000000);

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
			r.e = MAX_EXPONENT;
			r.m = 0xffffffffL;
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
				return zero;
			}
			uint8 ld = 32 - ild(r.m);
			r.e -= ld;
			r.m <<= ld;
		}

		if (r.e < MIN_EXPONENT)
		{
			return zero;
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
			r.e = MAX_EXPONENT;
			r.m = 0xffffffffL;
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
				return zero;
			}
			uint8 ld = 32 - ild(r.m);
			r.e -= ld;
			r.m <<= ld;
		}

		if (r.e < MIN_EXPONENT)
		{
			return zero;
		}
	}
	return r;
}

const float32e8_t float32e8_t::operator * (const float32e8_t & x) const
{
	if (!m || !x.m)
	{
		return zero;
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
		return zero;
	}
	if (r.e > MAX_EXPONENT)
	{
		dbg->error("float32e8_t::operator * (const float32e8_t & x) const", "Overflow in: %.9G * %.9G", this->to_double(), x.to_double());
		r.e = MAX_EXPONENT;
		r.m = 0xffffffffL;
	}
	r.ms = ms ^ x.ms;
	return r;
}

const float32e8_t float32e8_t::operator / (const float32e8_t & x) const
{
	if (x.m == 0)
	{
		dbg->error("float32e8_t::operator / (const float32e8_t & x) const", "Division by zero in: %.9G / %.9G", this->to_double(), x.to_double());
		return *this; // Catch the error
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
	if (r.e < MIN_EXPONENT)
	{
		return zero;
	}
	if (r.e > MAX_EXPONENT)
	{
		dbg->error("float32e8_t::operator / (const float32e8_t & x) const", "Overflow in: %.9G / %.9G", this->to_double(), x.to_double());
		r.e = MAX_EXPONENT;
		r.m = 0xffffffffL;
	}
	r.ms = ms ^ x.ms;
	return r;
}

double float32e8_t::to_double() const
{
	double rm = ms ? -(double)m : m;
	double re = pow((double)2.0, e - 32);
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
		return ms ? -(sint32) SINT32_MAX_VALUE : (sint32) SINT32_MAX_VALUE;
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

#ifndef MAKEOBJ
void float32e8_t::rdwr(loadsave_t *file)
{
	xml_tag_t k( file, "float32e8" );
	file->rdwr_long(m);
	file->rdwr_short(e);
	bool ms_bool = ms;
	file->rdwr_bool(ms_bool);
	ms = ms_bool;
}
#endif
