/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "image.h"
#include "../display/simgraph.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "../macros.h"

#include <string.h>
#include <stdlib.h>


/**
 * Definition of special colors
 */
const uint32 image_t::rgbtab[SPECIAL] = {
	0x244B67, // Player color 1
	0x395E7C,
	0x4C7191,
	0x6084A7,
	0x7497BD,
	0x88ABD3,
	0x9CBEE9,
	0xB0D2FF,

	0x7B5803, // Player color 2
	0x8E6F04,
	0xA18605,
	0xB49D07,
	0xC6B408,
	0xD9CB0A,
	0xECE20B,
	0xFFF90D,

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
	0xFF017F, // purple light
	0x0101FF, // blue light
};


image_t* image_t::copy_image(const image_t& other)
{
	image_t* img = new image_t(other.len);
	img->len = other.len;
	img->x   = other.x;
	img->y   = other.y;
	img->w   = other.w;
	img->h   = other.h;
	img->imageid  = IMG_EMPTY;
	img->zoomable = other.zoomable;
	memcpy(img->data, other.data, other.len * sizeof(PIXVAL));
	return img;
}

// creates a single pixel dummy picture
image_t* image_t::create_single_pixel()
{
	image_t* desc = new image_t(4);
	desc->len = 4;
	desc->x = 0;
	desc->y = 0;
	desc->w = 1;
	desc->h = 1;
	desc->zoomable = 0;
	desc->data[0] = 0;
	desc->data[1] = 0;
	desc->data[2] = 0;
	desc->data[3] = 0;
	return desc;
}


/* rotate_image_data - produces a (rotated) image
 * only rotates by 90 degrees or multiples thereof, and assumes a square image
 * Otherwise it will only succeed for angle=0;
*/
image_t *image_t::copy_rotate(const sint16 angle) const
{
#if COLOUR_DEPTH == 0
	(void)angle;
	return create_single_pixel();
#endif
	assert(angle == 0 || (w == h && x == 0 && y == 0));

	image_t* target_image = copy_image(*this);

	// the format is
	// transparent PIXELVAL number
	// PIXEL number of pixels, data*PIXVAL
	// repeated until zero transparent pixels
	// in pak64 case it is 0 64 64*PIXVAL 0 for a single line, e.g. 70 bytes per line for pak64
	// first data will have an offset of two PIXVAL
	// now you should understand below arithmetics ...

	sint16        const x_y    = w;
	PIXVAL const* const src    = get_data();
	PIXVAL*       const target = target_image->get_data();

	switch(angle) {
		case 90:
			for(int j=0; j<x_y; j++) {
				for(int i=0; i<x_y; i++) {
					target[j*(x_y+3)+i+2]=src[i*(x_y+3)+(x_y-j-1)+2];
				}
			}
		break;

		case 180:
			for(int j=0; j<x_y; j++) {
				for(int i=0; i<x_y; i++) {
					target[j*(x_y+3)+i+2]=src[(x_y-j-1)*(x_y+3)+(x_y-i-1)+2];
				}
			}
		break;
		case 270:
			for(int j=0; j<x_y; j++) {
				for(int i=0; i<x_y; i++) {
					target[j*(x_y+3)+i+2]=src[(x_y-i-1)*(x_y+3)+j+2];
				}
			}
		break;
		default: // no rotation, just converts to array
			;
	}
	return target_image;
}


image_t *image_t::copy_flipvertical() const
{
	image_t* target_image = copy_image(*this);

	// the format is
	// transparent PIXELVAL number
	// PIXEL number of pixels, data*PIXVAL
	// repeated until zero transparent pixels
	// in pak64 case it is 0 64 64*PIXVAL 0 for a single line, e.g. 70 bytes per line for pak64
	// first data will have an offset of two PIXVAL
	// now you should understand below arithmetics ...

	sint16        const x_y    = w;
	PIXVAL const* const src    = get_data();
	PIXVAL*       const target = target_image->get_data();

	for(  int j = 0;  j < x_y;  j++  ) {
		for(  int i = 0;  i < x_y;  i++  ) {
			target[i * (x_y + 3) + j + 2] = src[(x_y - i - 1) * (x_y + 3) + j + 2];
		}
	}
	return target_image;
}


image_t *image_t::copy_fliphorizontal() const
{
	image_t* target_image  = copy_image(*this);

	// the format is
	// transparent PIXELVAL number
	// PIXEL number of pixels, data*PIXVAL
	// repeated until zero transparent pixels
	// in pak64 case it is 0 64 64*PIXVAL 0 for a single line, e.g. 70 bytes per line for pak64
	// first data will have an offset of two PIXVAL
	// now you should understand below arithmetics ...

	sint16        const x_y    = w;
	PIXVAL const* const src    = get_data();
	PIXVAL*       const target = target_image->get_data();

	for(  int i = 0;  i < x_y;  i++  ) {
		for(  int j = 0;  j < x_y;  j++  ) {
			target[i * (x_y + 3) + j + 2] = src[i * (x_y + 3) + (x_y - j - 1) + 2];
		}
	}
	return target_image;
}
