/*
 * image_encoder.c
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>

#include "image_encoder.h"


/*
 * Hajo: transparent color
 */
// #define TRANSPARENT 0xE7FFFF
#define TRANSPARENT 0x73FE


/*
 * Hajo: number of special colors
 */
#define SPECIAL 17


/*
 * Hajo: table of special colors
 */
static const PIXRGB rgbtab[SPECIAL] =
{
  0x395E7C, // player colors
  0x6084A7,
  0x88ABD3,
  0xB0D2FF,

  0x57656F, // Dark windows, lit yellowish at night
  0x7F9BF1, // Lighter windows, lit blueish at night
  0xFFFF53, // Yellow light
  0xFF211D, // Red light
  0x01DD01, // Green light
  0x6B6B6B, // Non-darkening grey 1 (menus)
  0x9B9B9B, // Non-darkening grey 2 (menus)
  0xB3B3B3, // non-darkening grey 3 (menus)
  0xC9C9C9, // Non-darkening grey 4 (menus)
  0xDFDFDF, // Non-darkening grey 5 (menus)
  0xE3E3FF, // Nearly white light at day, yellowish light at night
  0xC1B1D1, // Windows, lit yellow
  0x4D4D4D, // Windows, lit yellow
};


/*
 * Hajo: if we gather statistics ...
 */
static int special_hist[SPECIAL];


/*
 * Converts RGB888 to RGB1555 considering special color encoding
 * @author Hj. Malthaner
 */
static PIXVAL pixrgb_to_pixval(int rgb)
{
  PIXVAL result = 0;
  int standard = 1;
  int i;

  /*
  for(i=0; i<SPECIAL_HIST; i++) {
    if(special2[i] == (PIXRGB)rgb) {
      special2_hist[i] ++;
    }
  }
  */


  for(i=0; i<SPECIAL; i++) {
    if(rgbtab[i] == (PIXRGB)rgb) {
      result = 0x8000 + i;

      standard = 0;
      break;
    }
  }

  if(standard) {
    const int r = rgb >> 16;
    const int g = (rgb >> 8) & 0xFF;
    const int b = rgb & 0xFF;

    // RGB 555
    result = ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3);
  }

  return result;
}


/**
 * Returns RGB888 from RGB555 block data
 *
 * Changed to macro by V. Meyer
 *
 * @author Hj. Malthaner
 */
#define block_getpix(block, x, y, width)  ((block)[(y) * (width) + (x)])


/*
 * Determines actual image dimensions (bounding box)
 * @author Hj. Malthaner
 */
static void init_dim(const PIXVAL *image,
		     struct dimension *dim,
		     const int img_size)
{
    int x,y;
    int found = 0;

    dim->ymin = dim->xmin = img_size;
    dim->ymax = dim->xmax = 0;

    for(y=0; y<img_size; y++) {
	for(x=0; x<img_size; x++) {
	    if(image[x+y*img_size] != TRANSPARENT) {
		if(x<dim->xmin)
		    dim->xmin = x;
		if(y<dim->ymin)
		    dim->ymin = y;
		if(x>dim->xmax)
		    dim->xmax = x;
		if(y>dim->ymax)
		    dim->ymax = y;
		found = 1;
	    }
	}
    }

    if(found == 0) {
	// Negativ values not usable, use marker xmin > xmax
	dim->xmin = 1;
	dim->ymin = 1;
    }
}


/*
 * Encodes an image. Returns a painter to encoded data.
 * Data must be free'd by caller.
 * @author Hj. Malthaner
 */
static PIXVAL* encode_image_aux(PIXVAL *block,
				int img_size,
				const struct dimension *dim,
				int *len)
{
    int y,line;
    PIXVAL *dest;
    PIXVAL *dest_base = malloc(sizeof(PIXVAL) * img_size * img_size * 2);
    PIXVAL *run_counter;

    y = dim->ymin;
    dest = dest_base;

    for(line=0; line<(dim->ymax-dim->ymin+1); line++) {
	int   row = 0;
	PIXVAL pix = block_getpix(block, row, (y+line), img_size);
	unsigned char count = 0;

	do {
	    count = 0;
	    while(pix == TRANSPARENT && row < img_size) {

		row ++;
		count ++;

		if(row < img_size) {
		    pix = block_getpix(block, row, (y+line), img_size);
		}
	    }

	    *dest++ = count;

	    run_counter = dest++;
	    count = 0;

	    while(pix != TRANSPARENT && row < img_size) {
		*dest++ = pix;

		row ++;
		count ++;

		if(row < img_size) {
		    pix = block_getpix(block, row, (y+line), img_size);
		}
	    }
	    *run_counter = count;
	} while(row < img_size);

	*dest++ = 0;
    }

    *len = (dest - dest_base);

    return dest_base;
}


/*
 * Encodes an image. Returns a painter to encoded data.
 * Data must be free'd by caller.
 * @author Hj. Malthaner
 */
PIXVAL* encode_image(PIXVAL *block,
		     int img_size,
		     struct dimension *dim,
		     int *len)
{
  init_dim(block, dim, img_size);
  return encode_image_aux(block, img_size, dim, len);
}
