/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "image.h"
#include "../display/simgraph.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "../macros.h"

#include <string.h>
#include <stdlib.h>

/*
 * Definition of special colors
 * @author Hj. Malthaner
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
	img->x = other.x;
	img->y = other.y;
	img->w = other.w;
	img->h = other.h;
	img->imageid = IMG_EMPTY;
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

	sint16        const x_y = w;
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

	sint16        const x_y = w;
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
	image_t* target_image = copy_image(*this);

	// the format is
	// transparent PIXELVAL number
	// PIXEL number of pixels, data*PIXVAL
	// repeated until zero transparent pixels
	// in pak64 case it is 0 64 64*PIXVAL 0 for a single line, e.g. 70 bytes per line for pak64
	// first data will have an offset of two PIXVAL
	// now you should understand below arithmetics ...

	sint16        const x_y = w;
	PIXVAL const* const src    = get_data();
	PIXVAL*       const target = target_image->get_data();

	for(  int i = 0;  i < x_y;  i++  ) {
		for(  int j = 0;  j < x_y;  j++  ) {
			target[i * (x_y + 3) + j + 2] = src[i * (x_y + 3) + (x_y - j - 1) + 2];
		}
	}
	return target_image;
}


/**
 * make any pixel in image which does not have colour components specified in opaquemask transparent
 * benefits:
 *  probable increase in plotting speed as can ignore transparent pixels
 *  if quality is not important then can increase speed further by just plotting as standard transparency not alpha
 *  decrease in memory consumption for individual images
 *
 * drawbacks:
 *  increase in number of images required as need separate images for each overlay combination
 *  this will probably also increase overall memory consumption
 *
 * NOT IMPLEMENTED CURRENTLY
 */
/*image_t *image_t::copy_maketransparent(uint8 opaquemask) const
{
	// step 1 is finding out how much memory picture will need
	// any pixel with no components specified in opaquemask will be made transparent (so needs no space)
	PIXVAL const* player = get_data();
	uint16 h = pic.h;
	uint16 len = 0;
	if(  h > 0  ) {
		do {
			uint16 transparent_runlen = *player++; // transparent
			len++;
			do {
				runlen = *player++; // opaque
				len++;
				bool run_transparent = true;
				bool first_pixel = true;
				while(  runlen--  ) {
					PIXVAL p = *player++;

					bool pixel_transparent = false; // here we will add code to test components...
					if(  pixel_transparent  ) {
						if(  !run_transparent  ) {
							len += 1; // need to store length of this transparent run
							run_transparent = true;
						}
						else {
							// don't need to increase length as will just increment runlen
						}
					}
					else {
						if(  run_transparent  ) {
							len += 2; // need to store length of this opaque run, as well as pixel
							run_transparent = false;
							first_pixel = false;
						}
						else {
							len++; // just another pixel
						}
					}
				}
				runlen = *player++; // transparent
				if(  !is_transparent  ) {
					len++; // only if last pixel was opaque
				}
				if(  first_pixel  ) {
					len += 2; // line is only transparent pixels, need to an additional opaque and transparent run
				}
			} while(  runlen != 0  );
		} while(  --h > 0  );
	}

	// now allocate memory
	image_t* target_image = new(len * sizeof(PIXVAL));

	// step 2 actually construct image
}*/


/**
 * decodes an image into a 32 bit bitmap
 */
void image_t::decode_img(sint16 xoff, sint16 yoff, uint32 *target, uint32 target_width, uint32 target_height ) const
{
	// Hajo: may this image be zoomed
	if (h > 0 && w > 0) {

		// since offset is implicit within image, do not "xoff += x;"!
		yoff += y;

		// now: unpack the image
		uint16 const *src = data;
		for (sint32 y = yoff; y < h + yoff; y++) {
			uint16 runlen;
			uint8 *p = (uint8 *)(target + max(0,xoff) + y*target_width);
			sint16 max_w = xoff;

			// decode line
			runlen = *src++;
			do {
				// clear run
				p += runlen*4;
				// color pixel
				runlen = *src++;
				while (runlen--) {
					// get rgb components
					uint16 s = *src++;
					if(  max_w>=0  &&  (uint32)max_w<target_width  &&  y>=0  ) {
						if(  s>=0x8000  ) {
							// special color
							*p++ = 0;
							*p++ = rgbtab[s&0x001F]>>16;
							*p++ = rgbtab[s&0x001F]>>8;
							*p++ = rgbtab[s&0x001F];
						}
						else {
							*p++ = 0;
							*p++ = ((s>>10)&31)<<3;
							*p++ = ((s>>5)&31)<<3;
							*p++ = ((s)&31)<<3;
						}
					}
					max_w ++;
				}
				runlen = *src++;
			} while(  runlen!=0  &&  (y<0  ||  (uint32)y<target_height)  );
		}
	}
}
