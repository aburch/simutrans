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

/*
 * characters are stored dense in a array
 * first 12 bytes are the first row
 * then come nibbles with the second part (6 bytes)
 * then the start offset for drawing
 * then a byte with the real width
 */
#define CHARACTER_LEN (20)


/**
 * Loads a font
 */
bool load_font(font_type* font, const char* fname);


#ifdef __cplusplus
}
#endif

#endif
