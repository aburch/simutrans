#include <stdio.h>
#include <string.h>

#include "cbuffer_t.h"


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


/**
 * Creates a new cbuffer with capacity of the string rounded to next 256 bytes
 * @param cap the capacity
 * @author Hj. Malthaner
 */
cbuffer_t::cbuffer_t(const char *str)
{
	if(str) {
		size = strlen(str);
		capacity = (size+256)&0x7FF0;
		buf = new char[capacity];
		strcpy( buf, str );
	}
	else {
		capacity = 256;
		size = 0;
		buf = new char[capacity];
		buf[0] = '\0';
	}
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
  while(size < capacity-2 && *text) {
    buf[size++] = *text++;
  }

  buf[size] = 0;
}


/**
 * Appends a number. If buffer is full, exceeding digits will not
 * be appended.
 * @author Hj. Malthaner
 */
void cbuffer_t::append(int n)
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

  } while ((n /= 10) > 0);

  if(neg) {
    *--p = '-';
  }

  append(p);
}
