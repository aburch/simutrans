/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "cbuffer_t.h"
#include "simstring.h"
#include "../macros.h"
#include "../simdebug.h"
#include "../simtypes.h"


cbuffer_t::cbuffer_t() :
	capacity(256),
	size(0),
	buf(new char[capacity])
{
	buf[0] = '\0';
}


cbuffer_t::~cbuffer_t()
{
	free();
}


void cbuffer_t::clear()
{
	buf[0] = '\0';
	size = 0;
}


cbuffer_t::cbuffer_t (const cbuffer_t& cbx)
{
	copy( cbx );
}


cbuffer_t& cbuffer_t::operator= (const cbuffer_t& cbx)
{
	if (  this != &cbx  )
	{
		free();
		copy( cbx );
	}

	return *this;
}


void cbuffer_t::copy (const cbuffer_t& cbx)
{
	capacity = cbx.capacity;
	size = cbx.size;
	buf = new char[capacity];
	memcpy( buf, cbx.buf, size + 1 );
}


void cbuffer_t::free ()
{
	delete [] buf;
}


void cbuffer_t::append(const char * text)
{
	size_t const n = strlen( text );
	extend( n );
	memcpy( buf + size, text, n + 1);
	size += n;
}


void cbuffer_t::append(const char* text, size_t maxchars)
{
	size_t const n = min( strlen( text ), maxchars );
	extend( n );
	memcpy( buf + size, text, n );
	size += n;
	buf[size] = '\0';  // Ensure buffer is null terminated
}


void cbuffer_t::append(double n,int decimals)
{
	char tmp[128];
	number_to_string( tmp, n, decimals );
	append(tmp);
}

void cbuffer_t::append_money(double money)
{
	char tmp[128];
	money_to_string(tmp, money, true);
	append(tmp);
}

const char* cbuffer_t::get_str() const
{
	return buf;
}


/**
 * Parses string @p format, and puts type identifiers into @p typemask.
 * If an error occurs, an error message is printed into @p error.
 * Checks for positional parameters: either all or no parameter have to be positional as eg %1$d.
 * If positional parameter %[n]$ is specified then all up to n have to be present in the string as well.
 * Treats all integer parameters %i %u %d etc the same.
 * Ignores positional width parameters as *[n].
 *
 * @param format format string
 * @param typemask pointer to array of length @p max_params
 * @param max_params length of typemask
 * @param error receives error message
 */
static void get_format_mask(const char* format, char *typemask, int max_params, cbuffer_t &error)
{
	MEMZERON(typemask, max_params);
	uint16 found = 0;
	bool positional = false;
	while(format  &&  *format) {
		uint16 pos = found;
		// skip until percent sign
		while(*format  &&  *format!='%') format++;
		if (*format == 0) {
			break;
		}
		format++;
		// read out position
		const int i = atoi(format);
		// skip numbers
		while(*format  &&  ('0'<=*format  &&  *format<='9') ) format++;

		// check for correct positional argument
		if (i>0 && i<=max_params) {
			if (format  &&  *format=='$')  {
				format ++;
				if (found > 0  &&  !positional) {
					goto err_mix_pos_nopos;
				}
				positional = true;
				pos = i-1;
			}
			else {
				// width specified, eg %2i
			}
		}
		else {
			if (found>0  &&  positional) {
				goto err_mix_pos_nopos;
			}
		}
		// now skip until format specifier
		static const char* all_types = "cCdDeEfFgGiIoOsSuUxXpPnN \t\n";
		static const char* all_masks = "cciiffffffiiiissiiiippnn   ";
		while(format  &&  *format) {
			if (const char* type = strchr(all_types, *format)) {
				char mask = *(all_masks + (type-all_types));
				if (mask == ' ') {
					// broken format string
				}
				else if (pos < max_params) {
					// found valid format
					typemask[pos] = mask;
					found++;
				}
				format++;
				break;
			}
			format++;
		}
	}
	// check whether positional parameters are continuous
	if (positional) {
		for(uint16 i=0; i<found; i++) {
			if (typemask[i]==0) {
				// unspecified
				error.printf("Positional parameter %d not specified.", i);
				return;
			}
		}
	}
	return;
err_mix_pos_nopos:
	error.append("Either all or no parameters have to be positional.");
}


/**
 * Check whether the format specifiers in @p translated match those in @p master.
 * Checks number of parameters as well as types as provided by format specifiers in @p master.
 * If @p master does not contain any format specifier then function returns true!
 * This is due to strings like 1extern that can have variable number of parameters.
 *
 * @param master the master string to compare against
 * @param translated the translated string taken from some tab file
 * @returns whether format in translated is ok
 */
bool cbuffer_t::check_format_strings(const char* master, const char* translated)
{
	if (master == NULL  ||  translated == NULL) {
		return false;
	}
	static cbuffer_t buf;
	buf.clear();
	char master_tm[10], translated_tm[10];
	// read out master string
	get_format_mask(master, master_tm, lengthof(master_tm), buf);
	if (buf.len() > 0) {
		// broken master string ?!
		dbg->warning("cbuffer_t::check_format_strings", "Broken master string '%s': %s", master, (const char*) buf);
		return false;
	}
	// read out translated string
	get_format_mask(translated, translated_tm, lengthof(translated_tm), buf);
	if (buf.len() > 0) {
		// broken translated string
		dbg->warning("cbuffer_t::check_format_strings", "Broken translation string '%s': %s", translated, (const char*) buf);
		return false;
	}
	// check for consistency
	for(uint i=0; (translated_tm[i])  &&  (i<lengthof(translated_tm)); i++) {
		if (master_tm[i]==0) {
			// too much parameters requested...
			// but some master strings like 1extern have no format specifiers - ignore these
			if (master_tm[0]) {
				dbg->warning("cbuffer_t::check_format_strings", "Translation string '%s' has more parameters than master string '%s'", translated, master);
				return false;
			}
			return true;
		}
		else if (master_tm[i]!=translated_tm[i]) {
			// wrong type
			dbg->warning("cbuffer_t::check_format_strings", "Parameter %d in translation string '%s' of '%s' has to be of type '%%%c' instead of '%%%c', Typemasks: Master = %s vs Translated = %s",
			               i+1, translated, master, master_tm[i], translated_tm[i], master_tm,translated_tm);
			return false;
		}
	}
	return true;
}


/* this is a vsnprintf which can always process positional parameters
 * like "%2$i: %1$s"
 * WARNING: posix specification as well as this function always assumes that
 * ALL parameter are either positional or not!!!
 *
 * When numbered argument specifications are used, specifying the Nth argument requires that all the
 * leading arguments, from the first to the (N-1)th, are specified in the format string.
 *
 * ATTENTION: no support for positional precision (which are not used in simutrans anyway!
 */
static int my_vsnprintf(char *buf, size_t n, const char* fmt, va_list ap )
{
#if defined _MSC_FULL_VER  &&  _MSC_FULL_VER >= 140050727  &&  !defined __WXWINCE__
	// this MSC function can handle positional parameters since 2008
	return _vsprintf_p(buf, n, fmt, ap);
#else
#if !defined(HAVE_UNIX98_PRINTF)
	// this function cannot handle positional parameters
	if(  const char *c=strstr( fmt, "%1$" )  ) {
		// but they are requested here ...
		// our routine can only handle max. 9 parameters
		char pos[14];
		static char format_string[256];
		char *cfmt = format_string;
		static char buffer[16000]; // the longest possible buffer ...
		int count = 0;
		for(  ;  c  &&  count<9;  count++  ) {
			sprintf( pos, "%%%i$", count+1 );
			c = strstr( fmt, pos );
			if(  c  ) {
				// extend format string, using 1 as mark between strings
				if(  count  ) {
					*cfmt++ = '\01';
				}
				*cfmt++ = '%';
				// now find the end
				c += 3;
				int len = strspn( c, "+-0123456789 #.hlI" )+1;
				while(  len-->0  ) {
					*cfmt++ = *c++;
				}
			}
		}
		*cfmt = 0;
		// now printf into buffer
		int result = vsnprintf( buffer, 16000, format_string, ap );
		if(  result<0  ||  result>=16000  ) {
			*buf = 0;
			return 0;
		}
		// check the length
		result += strlen(fmt);
		if(   (size_t)result > n  ) {
			// increase the size please ...
			return result;
		}
		// we have enough size: copy everything together
		*buf = 0;
		char *cbuf = buf;
		cfmt = const_cast<char *>(fmt); // cast is save, as the string is not modified
		while(  *cfmt!=0  ) {
			while(  *cfmt!='%'  &&  *cfmt  ) {
				*cbuf++ = *cfmt++;
			}
			if(  *cfmt==0  ) {
				break;
			}
			// get the nth argument
			char *carg = buffer;
			int current = cfmt[1]-'1';
			for(  int j=0;  j<current;  j++  ) {
				while(  *carg  &&  *carg!='\01'  ) {
					carg ++;
				}
				assert( *carg );
				carg ++;
			}
			while(  *carg  &&  *carg!='\01'  ) {
				*cbuf++ = *carg++;
			}
			// jump rest
			cfmt += 3;
			cfmt += strspn( cfmt, "+-0123456789 #.hlI" )+1;
		}
		*cbuf = 0;
		return cbuf-buf;
	}
	// no positional parameters: use standard vsnprintf
#endif
	// normal posix system can handle positional parameters
	return vsnprintf(buf, n, fmt, ap);
#endif
}


void cbuffer_t::printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt,  ap );
	va_end(ap);
}


void cbuffer_t::vprintf(const char *fmt, va_list ap )
{
	for (;;) {
		size_t const n = capacity - size;
		size_t inc;

		va_list args;
		va_copy(args, ap);

		const int count = my_vsnprintf( buf+size, n, fmt, args );
		if(  count < 0  ) {
#ifdef _WIN32
			inc = capacity;
#else
			// error
			buf[size] = '\0';
			break;
#endif
		}
		else if(  (size_t)count < n  ) {
			size += count;
			break;
		}
		else {
			// Make room for the string.
			inc = (size_t)count;
		}
		extend(inc);

		va_end(args);
	}
}


void cbuffer_t::extend(unsigned int min_free_space)
{
	if(  min_free_space >= capacity - size  ) {

		unsigned int by_amount = min_free_space + 1 - (capacity - size);
		if(  by_amount < capacity  ) {
			// At least double the size of the buffer.
			by_amount = capacity;
		}

		unsigned int new_capacity = capacity + by_amount;
		char *new_buf = new char [new_capacity];
		memcpy( new_buf, buf, capacity );
		delete [] buf;
		buf = new_buf;
		capacity = new_capacity;
	}
}


// remove whitespace and unprinatable characters
void cbuffer_t::trim()
{
	while (size > 0) {
		const unsigned char c = buf[size - 1];
		if (c >= 33) {
			break;
		}
		size--;
	}
}
