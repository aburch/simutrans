/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UNICODE_H
#define UNICODE_H


#include <stddef.h>
#include "../simtypes.h"

// Unicode type large enough to hold every single possible Unicode code point.
typedef uint32 utf32;

typedef unsigned char  utf8;
typedef unsigned short utf16;

extern utf32 const UNICODE_NUL;

// the bytes in a sequence have always the format 10xxxxxx
inline int is_cont_char(utf8 c) { return (c & 0xC0) == 0x80; }

/**
 * UTF-8 string decoder that can be used to iterate through all code points.
 */
class utf8_decoder_t
{
private:
	// Pointer to UTF-8 formated C string.
	utf8 const *utf8str;
public:
	// Constructs a UTF-8 decoder for the given C string.
	utf8_decoder_t(utf8 const *str);

	/**
	 * Decodes a Unicode code point from the byte sequence pointed to by buff.
	 * On return buff has been advanced to point at the beginning of the next Unicode code point.
	 * Does not respect NUL terminator character, care should be taken to detect the emmited UNICODE_NUL when decoding C strings to avoid buffer over run errors.
	 * Invalid Unicode sequences are intepreted using ISO-8859-1 and advance buff 1 byte.
	 */
	static utf32 decode(utf8 const *&buff);

	/**
	 * Decodes a Unicode code point from the byte sequence pointed to by buff.
	 * On return len contains the length of the Unicode character in bytes.
	 * Does not respect NUL terminator character, care should be taken to detect the emmited UNICODE_NUL when decoding C strings to avoid buffer over run errors.
	 * Invalid Unicode sequences are intepreted using ISO-8859-1 with a len of 1.
	 */
	static utf32 decode(utf8 const *const buff, size_t &len);

	/**
	 * Returns true if there are more code points left to decode.
	 * Returns false if at end of string.
	 */
	bool has_next() const;

	/**
	 * Returns the next Unicode code point value in the string.
	 * Returns UNICODE_NUL if has_next returns false.
	 */
	utf32 next();

	/**
	 * Returns the current position of the decoder.
	 * This is a pointer to the next character.
	 */
	utf8 const *get_position();
};

size_t utf8_get_next_char(const utf8 *text, size_t pos);
sint32 utf8_get_prev_char(const utf8 *text, sint32 pos);

int utf16_to_utf8(utf16 unicode, utf8 *out);

// returns latin2 or 0 for error
uint8 unicode_to_latin2( utf16 chr );
utf16 latin2_to_unicode( uint8 chr );

// caseless strstr with utf8
const utf8 *utf8caseutf8( const utf8 *haystack_start, const utf8 *needle_start );
const char *utf8caseutf8( const char *haystack, const char *needle );


#endif
