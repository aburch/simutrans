#ifndef UNICODE_H
#define UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

int	display_get_unicode(void);
int	display_set_unicode(int use_unicode);
int unicode_get_previous_character( const char *text, int cursor_pos);
int unicode_get_next_character( const char *text, int cursor_pos);
unsigned short utf82unicode (unsigned char const *ptr, int *iLen );
int	unicode2utf8( unsigned unicode, unsigned char *out );

#ifdef __cplusplus
}
#endif

#endif
