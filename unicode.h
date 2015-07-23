#ifndef UNICODE_H
#define UNICODE_H

#include <stddef.h>
#include "simtypes.h"

typedef unsigned char  utf8;
typedef unsigned short utf16;

size_t utf8_get_next_char(const utf8 *text, size_t pos);
sint32 utf8_get_prev_char(const utf8 *text, sint32 pos);

utf16 utf8_to_utf16(const utf8 *text, size_t *len);
int	utf16_to_utf8(utf16 unicode, utf8 *out);

// returns latin2 or 0 for error
uint8 unicode_to_latin2( utf16 chr );
utf16 latin2_to_unicode( uint8 chr );


#endif
