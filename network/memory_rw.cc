/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "memory_rw.h"
#include <string.h>
#include <stdlib.h>
#include "../simdebug.h"
#include "../simmem.h"
#include "../utils/plainstring.h"


memory_rw_t::memory_rw_t( void *ptr, uint32 max, bool saving )
{
	this->saving = saving;
	this->ptr = (char *)ptr;
	index = 0;
	max_size = max;
	overflow = false;
}


void memory_rw_t::rdwr(void *data, uint32 len)
{
	assert(data);
	if(  len+index>max_size  ) {
		len = max_size-index;
		overflow = true;
	}

	if(is_saving()) {
		memmove(ptr+index, data, len);
	}
	else {
		memmove(data, ptr+index, len);
	}
	index += len;
}


void memory_rw_t::rdwr_byte(sint8 &c)
{
	rdwr(&c, sizeof(sint8));
}


void memory_rw_t::rdwr_byte(uint8 &c)
{
	rdwr(&c, sizeof(uint8));
}


void memory_rw_t::rdwr_bool(bool &i)
{
	uint8 b = i;
	rdwr_byte(b);
	i = b!=0;
}


void memory_rw_t::rdwr_short(sint16 &i)
{
#ifdef SIM_BIG_ENDIAN
	sint16 ii;
	if(is_saving()) {
		ii = endian(i);
	}
	rdwr(&ii, sizeof(sint16));
	if(is_loading()) {
		i = endian(ii);
	}
#else
	rdwr(&i, sizeof(sint16));
#endif
}


void memory_rw_t::rdwr_short(uint16 &i)
{
	sint16 ii=i;
	rdwr_short(ii);
	i = (uint16)ii;
}


void memory_rw_t::rdwr_long(sint32 &l)
{
#ifdef SIM_BIG_ENDIAN
	sint32 ii;
	if(is_saving()) {
		ii = endian(l);
	}
	rdwr(&ii, sizeof(sint32));
	if(is_loading()) {
		l = endian(ii);
	}
#else
	rdwr(&l, sizeof(sint32));
#endif
}


void memory_rw_t::rdwr_long(uint32 &l)
{
	sint32 ll=l;
	rdwr_long(ll);
	l = (uint32)ll;
}


void memory_rw_t::rdwr_longlong(sint64 &ll)
{
#ifdef SIM_BIG_ENDIAN
	sint64  ii;
	if(is_saving()) {
		ii = endian(ii);
	}
	rdwr(&ii, sizeof(sint64));
	if(is_loading()) {
		ll = endian(ii);
	}
#else
	rdwr(&ll, sizeof(sint64));
#endif
}


void memory_rw_t::rdwr_str(char *&s)
{
	// string length
	uint16 len = 0;
	if(is_saving()) {
		len = s ? strlen(s) : 0;
	}
	rdwr_short(len);


	// now the string
	if (is_saving()) {
		if(len>0  &&  !overflow) {
			rdwr( s, len);
		}
	}
	else {
		free( (void *)s );
		s = 0;
		if(len>0  &&  !overflow) {
			s = MALLOCN(char, len + 1);
			rdwr(s, len);
			s[len] = '\0';
		}
	}
}


void memory_rw_t::rdwr_str(plainstring &s)
{
	if(is_loading()) {
		char *t = NULL;
		rdwr_str(t);
		s = t;
		free(t);
	}
	else {
		char *t = s;
		rdwr_str(t);
	}
}


void memory_rw_t::append(const memory_rw_t &mem)
{
	assert(saving  &&  mem.saving);
	rdwr(mem.ptr, mem.get_current_index());
}


void memory_rw_t::append_tail(const memory_rw_t &mem)
{
	assert(saving  &&  !mem.saving);
	rdwr(mem.ptr + mem.get_current_index(), mem.max_size - mem.get_current_index());
}
