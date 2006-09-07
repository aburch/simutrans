#ifndef FONT_H
#define FONT_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct font_type {
	int	height;
	int	descent;
	unsigned int num_chars;
	unsigned char* screen_width;
	unsigned char* char_data;
} font_type;


/**
 * Loads a font
 */
bool load_font(font_type* font, const char* fname);


#ifdef __cplusplus
}
#endif

#endif
