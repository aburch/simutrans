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


void cbuffer_t::append(double n,int decimals)
{
	char tmp[32];
	number_to_string( tmp, n, decimals );
	append(tmp);
}


void cbuffer_t::printf(const char* fmt, ...)
{
	for (;;) {
		va_list ap;
		va_start(ap, fmt);
		size_t const n     = capacity - size;
		int    const count = vsnprintf(buf + size, n, fmt, ap);
		va_end(ap);

		size_t inc;
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
			// Make room for the string, but at least double the size of the buffer.
			inc = (size_t)count + 1 /* for NUL */ - n;
			if (inc < capacity)
				inc = capacity;
		}
		extend(inc);
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
