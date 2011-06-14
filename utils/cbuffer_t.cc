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
			// Make room for the string.
			inc = (size_t)count - n;
		}
		extend(inc);
	}
}


void cbuffer_t::extend(unsigned int by_amount)
{
	if (by_amount >= capacity - size) {
		by_amount = by_amount + 1 - (capacity - size);
		if (by_amount < capacity) // At least double the size of the buffer.
			by_amount = capacity;
		unsigned int new_capacity = capacity + by_amount;
		char *new_buf = new char [new_capacity];
		memcpy( new_buf, buf, capacity );
		delete [] buf;
		buf = new_buf;
		capacity = new_capacity;
	}
}
