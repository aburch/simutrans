#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cbuffer_t.h"
#include "simstring.h"
#include "../simtypes.h"


cbuffer_t::cbuffer_t(unsigned int cap)
{
	capacity = (cap == 0 ? 1 : cap);
	size = 0;

	buf = new char[capacity];
	buf[0] = '\0';
}


cbuffer_t::~cbuffer_t()
{
  delete [] buf;
  buf = 0;
  capacity = 0;
  size = 0;
}


void cbuffer_t::clear()
{
  buf[0] = '\0';
  size = 0;
}


void cbuffer_t::append(const char * text)
{
	while(  *text  ) {
		if(  size>=capacity-1  ) {
			// Knightly : double the capacity if full
			extend(capacity);
		}
		buf[size++] = *text++;
	}
	buf[size] = 0;
}


void cbuffer_t::append(long n)
{
	char tmp[32];
	char * p = tmp+31;
	bool neg = false;
	*p = '\0';

	if(n < 0) {
		neg = true;
		n = -n;
	}

	do {
		*--p  = '0' + (n % 10);
	} while((n/=10) > 0);

	if(neg) {
		*--p = '-';
	}

	append(p);
}


void cbuffer_t::append(double n,int decimals)
{
	char tmp[32];
	number_to_string( tmp, n, decimals );
	append(tmp);
}


void cbuffer_t::printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int count = vsnprintf( buf+size, capacity-size, fmt, ap);
	va_end(ap);
	// buffer too small?
	// we do not increase it beyond 1MB, as this is usually only needed when %s of a random memory location
#ifdef _MSC_VER
	while(count == -1  &&  errno == ERANGE  &&  capacity < 1048576) {
#else
	while(0 <= count  &&  capacity-size <= (uint)count  &&  capacity < 1048576) {
#endif
		// enlarge buffer
		extend( capacity);
		// and try again
		va_start(ap, fmt);
		count = vsnprintf( buf+size, capacity-size, fmt, ap);
		va_end(ap);
		// .. until everything fit into buffer
	}
	// error
	if(count<0) {
		// truncate
		buf[capacity-1] = 0;
	}
	else {
		size += count;
	}
}


void cbuffer_t::extend(const unsigned int by_amount)
{
	if(  size+by_amount>=capacity  ) {
		unsigned int new_capacity = capacity + by_amount;
		char *new_buf = new char [new_capacity];
		memcpy( new_buf, buf, capacity );
		delete [] buf;
		buf = new_buf;
		capacity = new_capacity;
	}
}
