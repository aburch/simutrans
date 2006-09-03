#include "unicode.h"

#ifdef _MSC_VER
#	define inline _inline
#endif



static inline int is_1byte_seq(utf8 c) { return c<0x80; }	// normal ASCII (equivalen to (c & 0x80) == 0x00)
static inline int is_2byte_seq(utf8 c) { return (c & 0xE0) == 0xC0; } // 2 Byte sequence, total letter value is 110xxxxx 10yyyyyy => 00000xxx xxyyyyyy
static inline int is_3byte_seq(utf8 c) { return (c & 0xF0) == 0xE0; }	// 3 Byte sequence, total letter value is 1110xxxx 10yyyyyy 10zzzzzz => xxxxyyyy yyzzzzzz
static inline int is_cont_char(utf8 c) { return (c & 0xC0) == 0x80; }	// the bytes in a sequence have always the format 10xxxxxx


int utf8_get_next_char(const utf8* text, int pos)
{
	// go right one character
	// the bytes in a sequence have always the format 10xxxxxx, thus we use is_cont_char()
	// this will always work, even if we do not start on a sequence starting character
	do {
		pos++;
	} while (is_cont_char(text[pos]));
	return pos;
}


int utf8_get_prev_char(const utf8* text, int pos)
{
/*
	if(pos==0) {
		return 0;
	}
*/
	// go left one character
	// the bytes in a sequence have always the format 10xxxxxx, thus we use is_cont_char()
	do {
		pos--;
	} while (pos>0  &&  is_cont_char(text[pos]));
	return pos;
}



// these routines must be able to handle a mix of old and new, since after switching a
// language the visible gui elements are not immediately retranslated
utf16 utf8_to_utf16(const utf8* text, int* len)
{
	if (is_1byte_seq(text[0])) {
		// ASCII
		*len += 1;
		return text[0];

	} else if (is_2byte_seq(text[0])) {
		if (!is_cont_char(text[1])) {
			// just assume a 8 Bit normal character then
			*len += 1;
			return text[0];
		}
		*len += 2;
		// 2 Byte sequence, total letter value is 110xxxxx 10yyyyyy => 00000xxx xxyyyyyy
		return ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);

	} else if (is_3byte_seq(text[0])) {
		if (!is_cont_char(text[1]) || !is_cont_char(text[2])) {
			// just assume a 8 Bit normal character then
			*len += 1;
			return text[0];
		}
		*len += 3;
		// 3 Byte sequence, total letter value is 1110xxxx 10yyyyyy 10zzzzzz => xxxxyyyy yyzzzzzz
		return ((text[0] & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
	} else {
		// 4 Byte characters are UTF32, which we do not support
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
	} else /* if (c < 0x10000) */ {
		// Assume always a 3 byte sequence, since we do not support 4 byte UTF32
		out[0] = 0xC0 | (c >> 12);
		out[1] = 0x80 | (c >>  6 & 0x3F);
		out[2] = 0x80 | (c >>  0 & 0x3F);
		return 3;
	}
}



