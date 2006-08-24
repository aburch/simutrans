#ifndef UNICODE_H
#define UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  utf8;
typedef unsigned short utf16;

int utf8_get_next_char(const utf8 *text, int pos);
int utf8_get_prev_char(const utf8 *text, int pos);

utf16 utf8_to_utf16(const utf8 *text, int *len);
int	utf16_to_utf8(utf16 unicode, utf8 *out);

#ifdef __cplusplus
}
#endif

#endif
