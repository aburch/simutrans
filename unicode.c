#include "unicode.h"

#ifdef _MSC_VER
#	define inline _inline
#endif


#if 0
// sorry these routines do not work at all

static inline int is_1byte_seq(utf8 c) { return (c & 0x80) == 0x00; }
static inline int is_2byte_seq(utf8 c) { return (c & 0xE0) == 0xC0; }
static inline int is_3byte_seq(utf8 c) { return (c & 0xF0) == 0xE0; }
static inline int is_cont_char(utf8 c) { return (c & 0xC0) == 0x80; }


int utf8_get_next_char(const utf8* text, int pos)
{
	do {
		pos++;
	} while (is_cont_char(text[pos]));
	return pos;
}


int utf8_get_prev_char(const utf8* text, int pos)
{
	do {
		pos--;
	} while (is_cont_char(text[pos]));
	return pos;
}


utf16 utf8_to_utf16(const utf8* text, int* len)
{
	if (is_1byte_seq(text[0])) {
		*len += 1;
		return text[0];
	} else if (is_2byte_seq(text[0])) {
		// TODO improve error handling
		if (!is_cont_char(text[1])) {
			*len += 1;
			return text[0];
		}
		*len += 2;
		return ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);
	} else if (is_3byte_seq(text[0])) {
		// TODO improve error handling
		if (!is_cont_char(text[1]) || !is_cont_char(text[2])) {
			*len += 1;
			return text[0];
		}
		*len += 3;
		return ((text[0] & 0x0F) << 12) | ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);
	} else {
		// Neither a 1, 2 or 3 byte sequence
		// TODO improve error handling
		*len += 1;
		return text[0];
	}
}


int utf16_to_utf8(utf16 c, utf8* out)
{
	if (c < 0x80) {
		out[0] = c;
		return 1;
	} else if (c < 0x800) {
		out[0] = 0xC0 | (c >> 6);
		out[1] = 0x80 | (c >> 0 & 0x3F);
		return 2;
	} else /* if (c < 0x10000) */ { // Assume a 3 byte sequence
		out[0] = 0xC0 | (c >> 12);
		out[1] = 0x80 | (c >>  6 & 0x3F);
		out[2] = 0x80 | (c >>  0 & 0x3F);
		return 3;
	}
}



#else
// these are the old ones which I know will work

// --------------- compound painting procedures ---------------
/* Unicode/UTF stuff
 * taken from PalmDict (see sourceforge)
 * @date 2.1.2005
 * @author prissi
*/
// UTF-8 encouding values
#define UTF8_VALUE1     0x00        // Value for set bits for single byte UTF-8 Code.
#define UTF8_MASK1      0x80        // Mask (i.e. bits not set by the standard) 0xxxxxxx
#define UTF8_WRITE1     0xff80      // Mask of bits we cannot allow if we are going to write one byte code
#define UTF8_VALUE2     0xc0        // Two byte codes
#define UTF8_MASK2      0xe0        // 110xxxxx 10yyyyyy
#define UTF8_WRITE2     0xf800      // Mask of mits we cannot allow if we are going to write two byte code
#define UTF8_VALUE3     0xe0        // Three byte codes
#define UTF8_WRITE3     0xffff0000      // Mask of mits we cannot allow if we are going to write three byte code (16 Bit)
#define UTF8_MASK3      0xf0        // 1110xxxx 10yyyyyy 10zzzzzz
#define UTF8_VALUE4     0xf0        // Four byte values
#define UTF8_MASK4      0xf8        // 11110xxx ----    (These values are not supported).
#define UTF8_VALUEC     0x80        // Continueation byte (10xxxxxx).
#define UTF8_MASKC      0xc0



// find next unicode character
int utf8_get_next_char( const utf8 *ptr, int cursor_pos)
{
	ptr += cursor_pos;

	if (UTF8_VALUE1 == (*ptr & UTF8_MASK1)  ||  *ptr<0x80  ) {
		return cursor_pos + 1;
	}
	else {
		if (UTF8_VALUE2 == (*ptr & UTF8_MASK2)) 	{
			return cursor_pos + 2;
		}
		else {
			if (UTF8_VALUE3 == (*ptr & UTF8_MASK3)) {
				return cursor_pos + 3;
			}
		}
	}
	// should never ever get here!
	return cursor_pos + 1;
}



// find previous character border
int utf8_get_prev_char( const utf8 *text, int cursor_pos)
{
	int pos=0, last_pos=0;
	// find previous character border
	while(  pos<cursor_pos  &&  text[pos]!=0  ) {
		last_pos = pos;
		pos = utf8_get_next_char( text, pos );
	}
	return last_pos;
}



/* converts UTF8 to Unicode with save recovery
 * @author prissi
 * @date 29.11.04
 */
utf16 utf8_to_utf16(const utf8* ptr, int* iLen)
{
	unsigned short iUnicode=0;

	if (UTF8_VALUE1 == (*ptr & UTF8_MASK1)) {
		iUnicode = ptr[0];
		(*iLen) += 1;
	}
	else {
		if (UTF8_VALUE2 == (*ptr & UTF8_MASK2)) 	{
			// error handling ...
			if(  ptr[1]<=127  ) {
				(*iLen) += 1;
				return ptr[0];
			}
			//    iUnicode = (((*ptr)[1] & 0x1f) << 6) | ((*ptr)[2] & 0x3f);
			iUnicode = ptr[0] & 0x1f;
			iUnicode <<= 6;
			iUnicode |= ptr[1] & 0x3f;
			(*iLen) += 2;
		}
		else {
			if (UTF8_VALUE3 == (*ptr & UTF8_MASK3)) {
				// error handling ...
				if(  (ptr[1]<=127)    ||   (ptr[2]<=127)  ) {
					(*iLen) += 1;
					return ptr[0];
				}
				iUnicode = ptr[0] & 0x0f;
				iUnicode <<= 6;
				iUnicode |= ptr[1] & 0x3f;
				iUnicode <<= 6;
				iUnicode |= ptr[2] & 0x3f;
				(*iLen) += 3;
			}
			else {
				// 4 Byte unicode not supported
				(*iLen) += 1;
				return ptr[0];
			}
		}
	}
	return iUnicode;
}


// Converts Unicode to UTF8 sequence
int	unicode2utf8( unsigned unicode, unsigned char *out )
{
	if(  unicode<0x0080u  )
	{
		*out++ = unicode;
		return 1;
	}
	else if(  unicode<0x0800u  )
	{
		*out++ = 0xC0|(unicode>>6);
		*out++ = 0x80|(unicode&0x3F);
		return 2;
	}
	else // if(  lUnicode<0x10000l  ) immer TRUE!
	{
		*out++ = 0xE0|(unicode>>12);
		*out++ = 0x80|((unicode>>6)&0x3F);
		*out++ = 0x80|(unicode&0x3F);
		return 3;
	}
}
#endif
