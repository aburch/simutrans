#include "unicode.h"

#ifdef _MSC_VER
#	define inline _inline
#endif

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
