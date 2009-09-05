#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "cbuffer_t.h"
#include "../simtypes.h"


/**
 * Creates a new cbuffer with capacity cap
 * @param cap the capacity
 * @author Hj. Malthaner
 */
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


/**
 * Clears the buffer
 * @author Hj. Malthaner
 */
void cbuffer_t::clear()
{
  buf[0] = '\0';
  size = 0;
}


/**
 * Appends text. If buffer is full, exceeding text will not
 * be appended.
 * @author Hj. Malthaner
 */
void cbuffer_t::append(const char * text)
{
  while(size < capacity-2  &&  *text) {
    buf[size++] = *text++;
  }

  buf[size] = 0;
}


/**
 * Appends a number. If buffer is full, exceeding digits will not
 * be appended.
 * @author Hj. Malthaner
 */
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


void cbuffer_t::printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int count = vsnprintf( buf+size, capacity-size, fmt, ap);
	if(count==-1) {
		// truncated
		buf[capacity-1] = 0;
	}
	else if(capacity-size <= (uint)count) {
		size = capacity - 1;
	}
	else {
		size += count;
	}
	va_end(ap);
}


void cbuffer_t::extent(const unsigned int by_amount)
{
	if(  size+by_amount > capacity  ) {
		unsigned int new_capacity = capacity + by_amount;
		char *new_buf = new char [new_capacity];
		memcpy( new_buf, buf, capacity );
		delete [] buf;
		buf = new_buf;
		capacity = new_capacity;
	}
}

