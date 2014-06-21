#ifndef FONT_H
#define FONT_H

#include "../simtypes.h"

struct font_type
{
	char fname[1024];
	sint16	height;
	sint16	descent;
	uint16 num_chars;
	uint8 *screen_width;
	uint8 *char_data;
};

/*
 * characters are stored dense in a array
 * 23 rows of bytes, second row for widths between 8 and 16
 * then the start offset for drawing
 * then a byte with the real width
 */
#define CHARACTER_LEN (48)
#define CHARACTER_HEIGHT (23)


/**
 * Loads a font
 */
bool load_font(font_type* font, const char* fname);

#endif
