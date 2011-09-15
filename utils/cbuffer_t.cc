#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cbuffer_t.h"
#include "simstring.h"
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
  delete [] buf;
}


void cbuffer_t::clear()
{
  buf[0] = '\0';
  size = 0;
}


void cbuffer_t::append(const char * text)
{
	size_t const n = strlen(text);
	extend(n);
	memcpy(buf + size, text, n + 1);
	size += n;
}


void cbuffer_t::append(double n,int decimals)
{
	char tmp[32];
	number_to_string( tmp, n, decimals );
	append(tmp);
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
		char pos[6];
		static char format_string[256];
		char *cfmt = format_string;
		static char buffer[16000];	// the longest possible buffer ...
		int count = 0;
		for(  ;  c  &&  count<9;  count++  ) {
			sprintf( pos, "%%%i$", count+1 );
			c = strstr( fmt, pos );
			if(  c  ) {
				// extend format string, using 1 as marke between strings
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
	for (;;) {
		size_t const n     = capacity - size;
		size_t inc;

		va_list ap;
		va_start(ap, fmt);
		int    const count = my_vsnprintf(buf + size, n, fmt, ap );
		va_end(ap);
		if (count < 0) {
#ifdef _WIN32
			inc = capacity;
#else
			// error
			buf[size] = '\0';
			break;
#endif
		} else if ((size_t)count < n) {
			size += count;
			break;
		} else {
			// Make room for the string.
			inc = (size_t)count;
		}
		extend(inc);
	}
}


void cbuffer_t::extend(unsigned int min_free_space)
{
	if (min_free_space >= capacity - size) {
		unsigned int by_amount = min_free_space + 1 - (capacity - size);
		if (by_amount < capacity) {
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
