#ifndef FONT_H
#define FONT_H

#include "../simtypes.h"

struct font_type
{
	sint16	height;
	sint16	descent;
	uint16 num_chars;
	uint8 *screen_width;
	uint8 *char_data;
};

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

#endif
