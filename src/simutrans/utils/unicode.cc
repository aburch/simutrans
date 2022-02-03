/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../utils/unicode.h"
#include "../simtypes.h"
#include <cwctype>

utf32 const UNICODE_NUL = 0;

utf8_decoder_t::utf8_decoder_t(utf8 const *const str)
{
	utf8str = str;
}

utf32 utf8_decoder_t::decode(utf8 const *&buff)
{
	size_t len = 0;
	utf32 const code_point = decode(buff, len);
	buff += len;
	return code_point;
}

utf32 utf8_decoder_t::decode(utf8 const *const buff, size_t &len) {
	// Implementation derived from RFC 3629.

	// Process character byte.
	size_t i = 0;
	len = 0;
	utf8 const character = buff[i++];
	utf32 cp = 0;
	if(  character <= 0x7F  ) {
		// ASCII character.
		cp = character;
		len = 1;
	} else if(  character < 0xC2  ) {
		// Invalid character.
	} else if(  character <= 0xDF  ) {
		// 2 byte character.
		cp = character & 0x1F;
		len = 2;
	} else if(  character <= 0xEF  ) {
		// 3 byte character.
		if(  !((character == 0xE0 && buff[i] < 0xA0) ||
			(character == 0xED && buff[i] > 0x9F))  ) {
			cp = character & 0xF;
			len = 3;
		}
	} else if(  character <= 0xF4  ) {
		// 4 byte character.
		if(  !((character == 0xF0 && buff[i] < 0x90) ||
			(character == 0xF4 && buff[i] > 0x8F))  ) {
			cp = character & 0x7;
			len = 4;
		}
	} else {
		// Invalid character.
	}

	// Process tail bytes.
	for(  ; i < len ; i++  ) {
		utf8 const tail = buff[i];
		if(  0x80 <= tail && tail <= 0xBF  ) {
			cp <<= 6;
			cp |= tail & 0x3F;
		} else {
			// Invalid tail.
			len = 0;
		}
	}

	if(  len == 0  ) {
		// Replace invalid sequences with code point of the single decoded character (ISO-8859-1).
		len = 1;
		cp = character;
	}

	return cp;
}

bool utf8_decoder_t::has_next() const
{
	return utf8str[0] != UNICODE_NUL;
}

utf32 utf8_decoder_t::next()
{
	return has_next() ? decode(utf8str) : UNICODE_NUL;
}

utf8 const *utf8_decoder_t::get_position()
{
	return utf8str;
}


size_t utf8_get_next_char(const utf8* text, size_t pos)
{
	// go right one character
	// the bytes in a sequence have always the format 10xxxxxx, thus we use is_cont_char()
	// this will always work, even if we do not start on a sequence starting character
	do {
		pos++;
	} while (is_cont_char(text[pos]));
	return pos;
}


sint32 utf8_get_prev_char(const utf8* text, sint32 pos)
{
/* not needed, since the only position calling it, checks it too
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


int utf16_to_utf8(utf16 c, utf8* out)
{
	if (c < 0x80) {
		out[0] = (utf8)c;
		return 1;
	} else if (c < 0x800) {
		out[0] = 0xC0 | (c >> 6);
		out[1] = 0x80 | (c >> 0 & 0x3F);
		return 2;
	} else /* if (c < 0x10000) */ {
		// Assume always a 3 byte sequence, since we do not support 4 byte UTF32
		out[0] = 0xE0 | (c >> 12);
		out[1] = 0x80 | (c >>  6 & 0x3F);
		out[2] = 0x80 | (c >>  0 & 0x3F);
		return 3;
	}
}



// helper function to convert unicode into latin2
static utf16 latin2_to_unicode_lookup[96] =
{
	0x00A0, // char 0xA0
	0x0104, // char 0xA1
	0x02D8, // char 0xA2
	0x0141, // char 0xA3
	0x00A4, // char 0xA4
	0x013D, // char 0xA5
	0x015A, // char 0xA6
	0x00A7, // char 0xA7
	0x00A8, // char 0xA8
	0x0160, // char 0xA9
	0x015E, // char 0xAA
	0x0164, // char 0xAB
	0x0179, // char 0xAC
	0x00AD, // char 0xAD
	0x017D, // char 0xAE
	0x017B, // char 0xAF
	0x00B0, // char 0xB0
	0x0105, // char 0xB1
	0x02DB, // char 0xB2
	0x0142, // char 0xB3
	0x00B4, // char 0xB4
	0x013E, // char 0xB5
	0x015B, // char 0xB6
	0x02C7, // char 0xB7
	0x00B8, // char 0xB8
	0x0161, // char 0xB9
	0x015F, // char 0xBA
	0x0165, // char 0xBB
	0x017A, // char 0xBC
	0x02DD, // char 0xBD
	0x017E, // char 0xBE
	0x017C, // char 0xBF
	0x0154, // char 0xC0
	0x00C1, // char 0xC1
	0x00C2, // char 0xC2
	0x0102, // char 0xC3
	0x00C4, // char 0xC4
	0x0139, // char 0xC5
	0x0106, // char 0xC6
	0x00C7, // char 0xC7
	0x010C, // char 0xC8
	0x00C9, // char 0xC9
	0x0118, // char 0xCA
	0x00CB, // char 0xCB
	0x011A, // char 0xCC
	0x00CD, // char 0xCD
	0x00CE, // char 0xCE
	0x010E, // char 0xCF
	0x0110, // char 0xD0
	0x0143, // char 0xD1
	0x0147, // char 0xD2
	0x00D3, // char 0xD3
	0x00D4, // char 0xD4
	0x0150, // char 0xD5
	0x00D6, // char 0xD6
	0x00D7, // char 0xD7
	0x0158, // char 0xD8
	0x016E, // char 0xD9
	0x00DA, // char 0xDA
	0x0170, // char 0xDB
	0x00DC, // char 0xDC
	0x00DD, // char 0xDD
	0x0162, // char 0xDE
	0x00DF, // char 0xDF
	0x0155, // char 0xE0
	0x00E1, // char 0xE1
	0x00E2, // char 0xE2
	0x0103, // char 0xE3
	0x00E4, // char 0xE4
	0x013A, // char 0xE5
	0x0107, // char 0xE6
	0x00E7, // char 0xE7
	0x010D, // char 0xE8
	0x00E9, // char 0xE9
	0x0119, // char 0xEA
	0x00EB, // char 0xEB
	0x011B, // char 0xEC
	0x00ED, // char 0xED
	0x00EE, // char 0xEE
	0x010F, // char 0xEF
	0x0111, // char 0xF0
	0x0144, // char 0xF1
	0x0148, // char 0xF2
	0x00F3, // char 0xF3
	0x00F4, // char 0xF4
	0x0151, // char 0xF5
	0x00F6, // char 0xF6
	0x00F7, // char 0xF7
	0x0159, // char 0xF8
	0x016F, // char 0xF9
	0x00FA, // char 0xFA
	0x0171, // char 0xFB
	0x00FC, // char 0xFC
	0x00FD, // char 0xFD
	0x0163, // char 0xFE
	0x02D9 // char 0xFF
};



uint8 unicode_to_latin2( utf16 chr )
{
	for(  utf8 i=0;  i<96;  i++  ) {
		if(  latin2_to_unicode_lookup[i]==chr  ) {
			return (i+0xA0);
		}
	}
	return 0;
}



utf16 latin2_to_unicode( uint8 chr )
{
	return chr >= 0xA0 ? latin2_to_unicode_lookup[chr-0xA0] : chr;
}



// inspired from Stackoverflow https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
// Handles somewhat umlaute as well (may depend a little on the locale though)
const utf8 *utf8caseutf8(const utf8 *haystack_start, const utf8 *needle_start)
{
	const utf8 *needle_p = needle_start;
	const utf8 *haystack_p = haystack_start;
	utf32 c = towlower( utf8_decoder_t::decode(needle_p ));
	needle_start = needle_p;
	if(  c=='\0'  ) {
		return haystack_start;
	}
	const utf8 *haystack_current = haystack_p;
	utf32 hs = utf8_decoder_t::decode( haystack_p );
	while(  hs != 0  ) {
		if(   towlower( hs ) == c   ) {
			const utf8 *haystack_next = haystack_p;

			while(true) {
				sint32 nc = utf8_decoder_t::decode( needle_p );
				if(  nc == 0  ) {
					return haystack_current;
				}
				hs = utf8_decoder_t::decode( haystack_p );
				if(  towlower( hs ) != towlower( nc )  ) {
					needle_p = needle_start;
					break;
				}
			}

			// advance one character
			haystack_p = haystack_next;
		}
		haystack_current = haystack_p;
		hs = utf8_decoder_t::decode( haystack_p );
	}
	return NULL;
}


// defining it in the include did not work for whatever reason
const char *utf8caseutf8( const char *haystack, const char *needle )
{
	return (const char *)utf8caseutf8( (const utf8 *)haystack, (const utf8 *)needle );
}
