
#include "memory_rw.h"
#include <string.h>
#include <stdlib.h>
#include "../simdebug.h"

#undef BIG_ENDIAN

void memory_rw_t::init( void *start, uint32 max, bool rw )
{
	saving = rw;
	ptr = (char *)start;
	index = 0;
	max_size = max;
	overflow = false;
}


void memory_rw_t::rdwr(void *data, uint32 len)
{
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
#ifdef BIG_ENDIAN
	sint16 ii;
	if(is_saving()) {
		ii = (sint16)endian_uint16((uint16 *)&i);
	}
	rdwr(&ii, sizeof(sint16));
	if(is_loading()) {
		i = (sint16)endian_uint16(&ii);
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
#ifdef BIG_ENDIAN
	uint32 ii;
	if(is_saving()) {
		ii = endian_uint32((uint32 *)&l);
	}
	rdwr(&ii, sizeof(uint32));
	if(is_loading()) {
		l = (sint32)endian_uint32(&ii);
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
#ifdef BIG_ENDIAN
	sint64  ii;
	if(is_saving()) {
		ii = (sint64)endian_uint64((uint64 *)&ll);
	}
	rdwr(&ii, sizeof(sint64));
	if(is_loading()) {
		ll = (sint64)endian_uint64(&ii);
	}
#else
	rdwr(&ll, sizeof(sint64));
#endif
}


void memory_rw_t::rdwr_str(char *&s)
{
	// string length
	uint16 len;
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
			s = (char *)malloc( len+1 );
			rdwr(s, len);
			s[len] = '\0';
		}
	}
}
