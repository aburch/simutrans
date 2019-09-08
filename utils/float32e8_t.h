/*
 * float32e8_t.h
 *
 *  Created on: 22.05.2011
 *      Author: Bernd
 */

#ifndef FLOAT32E8_T_H_
#define FLOAT32E8_T_H_

#include <iostream>
using namespace std;

#include <string>
#include <math.h>

#ifndef NO_SIMUTRANS
	#include "../simtypes.h"
#else
	typedef unsigned	long long	uint64;
	typedef 			long long	sint64;
	typedef unsigned	long		uint32;
	typedef 			long		sint32;
	typedef unsigned	short		uint16;
	typedef 			short		sint16;
	typedef unsigned	char  		uint8;
#endif
	
class loadsave_t;

class float32e8_t
{
protected:
	static const float32e8_t integers[257];
	static const uint8 _ild[256];

	static inline uint8 ild(const uint32 x)
	{
		if (x & 0xffff0000L)
		{
			if (x & 0xff000000L)
			{
				return 24 + _ild[x>>24];
			}
			else
			{
				return 16 + _ild[x>>16];
			}
		}
		else
		{
			if (x & 0xffffff00L)
			{
				return 8 + _ild[x>>8];
			}
			else
			{
				return _ild[(uint8)x];
			}
		}
	}
	
protected:
	uint32 m;	// mantissa
	sint16 e;	// exponent
	bool ms:1;	// sign of mantissa

	inline void set_zero() { m = 0L; e = 0; ms = false; }
public:
	static const uint8 bpm = 32; // bits per mantissa
	static const uint8 bpe = 10; // bits per exponent
	static const sint16 min_exponent;
	static const sint16 max_exponent;
	static const uint32 max_mantissa;
	static const float32e8_t zero;
	static const float32e8_t micro;
	static const float32e8_t ten_thousandth;
	static const float32e8_t milli;
	static const float32e8_t centi;
	static const float32e8_t tenth;
	static const float32e8_t quarter;
	static const float32e8_t third;
	static const float32e8_t half;

	static const float32e8_t one;
	static const float32e8_t two;
	static const float32e8_t three;
	static const float32e8_t four;
	static const float32e8_t ten;
	static const float32e8_t hundred;
	static const float32e8_t thousand;
	static const float32e8_t tenthousand;

	inline float32e8_t() : m(0L), e(0), ms(false) {};

	inline float32e8_t(const float32e8_t &value) { m = value.m; e = value.e; ms = value.ms; }
	inline float32e8_t(const uint32 mantissa, const sint16 exponent, const bool negative_man) { m = mantissa; e = exponent; ms = negative_man; }
	inline void set_value(const float32e8_t &value) { m = value.m; e = value.e; ms = value.ms; }
	inline bool is_zero() const { return m == 0L; }

#ifdef USE_DOUBLE
	inline float32e8_t(const double value) { set_value(value); }
	void set_value(const double value);
	inline const float32e8_t & operator = (const double value)	{ set_value(value); return *this; }
#endif

	inline float32e8_t(const uint8 value) { set_value(value); }
	inline float32e8_t(const sint32 value) { set_value(value); }
	inline float32e8_t(const uint32 value) { set_value(value); }
	inline float32e8_t(const sint64 value) { set_value(value); }
	inline float32e8_t(const uint64 value) { set_value(value); }

	inline float32e8_t(const sint32 nominator, const sint32 denominator) { set_value(float32e8_t(nominator) / float32e8_t(denominator)); }
	inline float32e8_t(const uint32 nominator, const uint32 denominator) { set_value(float32e8_t(nominator) / float32e8_t(denominator)); }
	//inline float32e8_t(const sint64 nominator, const sint64 denominator) { set_value(float32e8_t(nominator) / float32e8_t(denominator)); }
	//inline float32e8_t(const uint64 nominator, const uint64 denominator) { set_value(float32e8_t(nominator) / float32e8_t(denominator)); }

	inline void set_value(const uint8 value)
	{
		set_value(integers[value]);
		if (!m && value)
		{
			// As some constants may be initialized before integers[], we must check if initialization has been done.
			// Do not check integers[0].m. This mantissa will still be 0 after initialization.
			ms = false;
			e = ild(value);
			m = value;
			m = m << (32 - e);
		}
	}

	inline void set_value(const uint32 value)
	{
		// As some constants may be initialized before integers[], we must check if initialization has been done.
		// Do not check integers[0].m. This mantissa will still be 0 after initialization.
		if (value < 256)
			set_value((uint8)value);
		else
		{
			ms = false;
			e = ild(value);
			m = (value) << (32 - e);
		}
	}

	inline void set_value(const sint32 value)
	{
		if (value < 0)
		{
			set_value((uint32)-value);
			ms = true;
		}
		else
			set_value((uint32)value);
	}

	inline void set_value(const uint64 value)
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

	inline void set_value(const sint64 value)
	{
		if (value < 0)
		{
			set_value((uint64)-value);
			ms = true;
		}
		else
			set_value((uint64)value);
	}

	inline const float32e8_t & operator = (const uint8 value)	{ set_value(value);	return *this; }
	inline const float32e8_t & operator = (const sint32 value)	{ set_value(value);	return *this; }
	inline const float32e8_t & operator = (const uint32 value)	{ set_value(value);	return *this; }
	inline const float32e8_t & operator = (const sint64 value)	{ set_value(value);	return *this; }
	inline const float32e8_t & operator = (const uint64 value)	{ set_value(value);	return *this; }

	inline bool operator < (const float32e8_t &value) const
	{
		if (ms)
			return !value.ms || e > value.e || (e == value.e && m > value.m);
		else if (value.ms)
			return false;
		else if (m == 0 || value.m == 0)
			return m < value.m;
		else
			return e < value.e || (e == value.e && m < value.m);
	}

	inline bool operator <= (const float32e8_t &value) const
	{
		if (ms)
			return !value.ms || e > value.e || (e == value.e && m >= value.m);
		else if (value.ms)
			return false;
		else if (m == 0 || value.m == 0)
			return m <= value.m;
		else
			return e < value.e || (e == value.e && m <= value.m);
	}

	inline bool operator > (const float32e8_t &value) const
	{
		if (ms)
			return value.ms && (e < value.e || (e == value.e && m < value.m));
		else if (value.ms)
			return true;
		else if (m == 0 || value.m == 0)
			return m > value.m;
		else
			return e > value.e || (e == value.e && m > value.m);
	}

	inline bool operator >= (const float32e8_t &value) const
	{
		if (ms)
			return value.ms && (e < value.e || (e == value.e && m <= value.m));
		else if (value.ms)
			return true;
		else if (m == 0 || value.m == 0)
			return m >= value.m;
		else
			return e > value.e || (e == value.e && m >= value.m);
	}

	inline bool operator == (const float32e8_t &value) const { return m == value.m && e == value.e && ms == value.ms; }
	inline bool operator != (const float32e8_t &value) const { return m != value.m || e != value.e || ms != value.ms; }

	inline bool operator <  (const sint32 value) const { return *this <  float32e8_t((sint32) value); }
	inline bool operator <= (const sint32 value) const { return *this <= float32e8_t((sint32) value); }
	inline bool operator == (const sint32 value) const { return *this == float32e8_t((sint32) value); }
	inline bool operator != (const sint32 value) const { return *this != float32e8_t((sint32) value); }
	inline bool operator >= (const sint32 value) const { return *this >= float32e8_t((sint32) value); }
	inline bool operator >  (const sint32 value) const { return *this >  float32e8_t((sint32) value); }

	inline bool operator <  (const sint64 value) const { return *this <  float32e8_t((sint64) value); }
	inline bool operator <= (const sint64 value) const { return *this <= float32e8_t((sint64) value); }
	inline bool operator == (const sint64 value) const { return *this == float32e8_t((sint64) value); }
	inline bool operator != (const sint64 value) const { return *this != float32e8_t((sint64) value); }
	inline bool operator >= (const sint64 value) const { return *this >= float32e8_t((sint64) value); }
	inline bool operator >  (const sint64 value) const { return *this >  float32e8_t((sint64) value); }

	inline const float32e8_t operator - () const { return float32e8_t(m, e, !ms); }

	const float32e8_t operator + (const float32e8_t &value) const;
	const float32e8_t operator - (const float32e8_t &value) const;
	const float32e8_t operator * (const float32e8_t &value) const;
	const float32e8_t operator / (const float32e8_t &value) const;

	inline const float32e8_t operator + (const uint8 value) const { return *this + float32e8_t(value); } 
	inline const float32e8_t operator - (const uint8 value) const { return *this - float32e8_t(value); } 
	inline const float32e8_t operator * (const uint8 value) const { return *this * float32e8_t(value); } 
	inline const float32e8_t operator / (const uint8 value) const { return *this / float32e8_t(value); } 

	inline const float32e8_t operator + (const sint32 value) const { return *this + float32e8_t(value); } 
	inline const float32e8_t operator - (const sint32 value) const { return *this - float32e8_t(value); } 
	inline const float32e8_t operator * (const sint32 value) const { return *this * float32e8_t(value); } 
	inline const float32e8_t operator / (const sint32 value) const { return *this / float32e8_t(value); } 

	inline const float32e8_t operator + (const uint32 value) const { return *this + float32e8_t(value); } 
	inline const float32e8_t operator - (const uint32 value) const { return *this - float32e8_t(value); } 
	inline const float32e8_t operator * (const uint32 value) const { return *this * float32e8_t(value); } 
	inline const float32e8_t operator / (const uint32 value) const { return *this / float32e8_t(value); } 

	inline const float32e8_t operator + (const sint64 value) const { return *this + float32e8_t(value); }
	inline const float32e8_t operator - (const sint64 value) const { return *this - float32e8_t(value); }
	inline const float32e8_t operator * (const sint64 value) const { return *this * float32e8_t(value); }
	inline const float32e8_t operator / (const sint64 value) const { return *this / float32e8_t(value); }

	inline const float32e8_t operator + (const uint64 value) const { return *this + float32e8_t(value); }
	inline const float32e8_t operator - (const uint64 value) const { return *this - float32e8_t(value); }
	inline const float32e8_t operator * (const uint64 value) const { return *this * float32e8_t(value); }
	inline const float32e8_t operator / (const uint64 value) const { return *this / float32e8_t(value); }

	inline const float32e8_t & operator += (const float32e8_t &value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const float32e8_t &value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const float32e8_t &value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const float32e8_t &value) { set_value(*this / value); return *this; }

	inline const float32e8_t & operator += (const uint8 value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const uint8 value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const uint8 value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const uint8 value) { set_value(*this / value); return *this; }

	inline const float32e8_t & operator += (const sint32 value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const sint32 value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const sint32 value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const sint32 value) { set_value(*this / value); return *this; }

	inline const float32e8_t & operator += (const uint32 value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const uint32 value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const uint32 value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const uint32 value) { set_value(*this / value); return *this; }

	inline const float32e8_t & operator += (const sint64 value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const sint64 value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const sint64 value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const sint64 value) { set_value(*this / value); return *this; }

	inline const float32e8_t & operator += (const uint64 value) { set_value(*this + value); return *this; }
	inline const float32e8_t & operator -= (const uint64 value) { set_value(*this - value); return *this; }
	inline const float32e8_t & operator *= (const uint64 value) { set_value(*this * value); return *this; }
	inline const float32e8_t & operator /= (const uint64 value) { set_value(*this / value); return *this; }

	inline const float32e8_t abs() const { return ms ? float32e8_t(m, e, false) : *this; }
	inline int sgn() const { return ms ? -1 : m ? 1 : 0; }
	inline int sgn(const float32e8_t &eps) const { return *this < -eps ? -1 : *this > eps ? 1 : 0; }
	const float32e8_t log2() const;
	const float32e8_t exp2() const;

	// For efficient use in checksums:
	inline uint32 get_mantissa() { return m; }

	void rdwr(loadsave_t *file);

private:
#ifdef USE_DOUBLE
public:
#else
	friend ostream & operator << (ostream &out, const float32e8_t &x);
#endif
public:
	double to_double() const;
	sint32 to_sint32() const;
	//const string to_string() const;

	inline operator sint32 () const { return to_sint32(); }
};

ostream & operator << (ostream &out, const float32e8_t &x);

inline const float32e8_t operator + (const uint8 x, const float32e8_t &y) {return float32e8_t(x) + y; }
inline const float32e8_t operator - (const uint8 x, const float32e8_t &y) {return float32e8_t(x) - y; }
inline const float32e8_t operator * (const uint8 x, const float32e8_t &y) {return float32e8_t(x) * y; }
#if 0
// This MUST be commented out.  Otherwise GCC gives us endless warnings which we cannot shut off.
inline const float32e8_t operator / (const uint8 x, const float32e8_t &y) {return float32e8_t(x) / y; }
#endif

inline const float32e8_t operator + (const sint32 x, const float32e8_t &y) {return float32e8_t(x) + y; }
inline const float32e8_t operator - (const sint32 x, const float32e8_t &y) {return float32e8_t(x) - y; }
inline const float32e8_t operator * (const sint32 x, const float32e8_t &y) {return float32e8_t(x) * y; }
inline const float32e8_t operator / (const sint32 x, const float32e8_t &y) {return float32e8_t(x) / y; }

inline const float32e8_t operator + (const uint32 x, const float32e8_t &y) {return float32e8_t(x) + y; }
inline const float32e8_t operator - (const uint32 x, const float32e8_t &y) {return float32e8_t(x) - y; }
inline const float32e8_t operator * (const uint32 x, const float32e8_t &y) {return float32e8_t(x) * y; }
inline const float32e8_t operator / (const uint32 x, const float32e8_t &y) {return float32e8_t(x) / y; }

inline const float32e8_t operator + (const sint64 x, const float32e8_t &y) {return float32e8_t(x) + y; }
inline const float32e8_t operator - (const sint64 x, const float32e8_t &y) {return float32e8_t(x) - y; }
inline const float32e8_t operator * (const sint64 x, const float32e8_t &y) {return float32e8_t(x) * y; }
inline const float32e8_t operator / (const sint64 x, const float32e8_t &y) {return float32e8_t(x) / y; }

inline const float32e8_t operator + (const uint64 x, const float32e8_t &y) {return float32e8_t(x) + y; }
inline const float32e8_t operator - (const uint64 x, const float32e8_t &y) {return float32e8_t(x) - y; }
inline const float32e8_t operator * (const uint64 x, const float32e8_t &y) {return float32e8_t(x) * y; }
inline const float32e8_t operator / (const uint64 x, const float32e8_t &y) {return float32e8_t(x) / y; }

inline const float32e8_t abs(const float32e8_t &x) { return x.abs(); }
inline const float32e8_t log2(const float32e8_t &x) { return x.log2(); }
inline const float32e8_t exp2(const float32e8_t &x) { return x.exp2(); }
inline const float32e8_t pow(const float32e8_t &base, const float32e8_t &expo) { return base.is_zero() ? float32e8_t::zero : exp2(expo * base.log2()); }
inline const float32e8_t sqrt(const float32e8_t &x) { return pow(x, float32e8_t::half); }
inline int sgn(const float32e8_t &x) { return x.sgn(); }
inline int sgn(const float32e8_t &x, const float32e8_t &eps) { return x.sgn(eps); }

class float32e8_exception_t {
private:
	std::string message;
public:
	float32e8_exception_t(const char* message)
	{
		this->message = message;
	}

	const char* get_message()  const { return message.c_str(); }
};

#ifdef NETTOOL
#undef min
#undef max
#endif

static inline float32e8_t fl_min(const float32e8_t a, const float32e8_t b)
{
	return a < b ? a : b;
}

static inline float32e8_t fl_max(const float32e8_t a, const float32e8_t b)
{
	return a > b ? a : b;
}


#endif /* FLOAT32E8_T_H_ */
