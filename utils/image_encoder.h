/*
 * image_encoder.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef image_encoder_h
#define image_encoder_h


/*
 * Hajo: RGB 888 padded to 32 bit
 */
typedef unsigned int   PIXRGB;


/*
 * Hajo: RGB 1555
 */
typedef unsigned short PIXVAL;


/*
 * Hajo: image bounding box
 */
struct dimension  {
    int xmin,xmax,ymin,ymax;
};


/*
 * Encodes an image. Returns a painter to encoded data.
 * Data must be free'd by caller.
 * @author Hj. Malthaner
 */
PIXVAL* encode_image(PIXVAL *block,
		     int img_size,
		     struct dimension *dim,
		     int *len);

#endif // image_encoder_h
