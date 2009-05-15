#ifndef UNICODE_H
#define UNICODE_H

#include <stddef.h>

typedef unsigned char  utf8;
typedef unsigned short utf16;

size_t utf8_get_next_char(const utf8 *text, size_t pos);
long utf8_get_prev_char(const utf8 *text, long pos);

utf16 utf8_to_utf16(const utf8 *text, size_t *len);
int	utf16_to_utf8(utf16 unicode, utf8 *out);

#endif
