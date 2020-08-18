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

// the players' colors and colors for simple drawing operations
// each eight colors are corresponding to a player color
const uint8 image_t::special_pal[224*3]=
{
	36, 75, 103,
	57, 94, 124,
	76, 113, 145,
	96, 132, 167,
	116, 151, 189,
	136, 171, 211,
	156, 190, 233,
	176, 210, 255,
	88, 88, 88,
	107, 107, 107,
	125, 125, 125,
	144, 144, 144,
	162, 162, 162,
	181, 181, 181,
	200, 200, 200,
	219, 219, 219,
	17, 55, 133,
	27, 71, 150,
	37, 86, 167,
	48, 102, 185,
	58, 117, 202,
	69, 133, 220,
	79, 149, 237,
	90, 165, 255,
	123, 88, 3,
	142, 111, 4,
	161, 134, 5,
	180, 157, 7,
	198, 180, 8,
	217, 203, 10,
	236, 226, 11,
	255, 249, 13,
	86, 32, 14,
	110, 40, 16,
	134, 48, 18,
	158, 57, 20,
	182, 65, 22,
	206, 74, 24,
	230, 82, 26,
	255, 91, 28,
	34, 59, 10,
	44, 80, 14,
	53, 101, 18,
	63, 122, 22,
	77, 143, 29,
	92, 164, 37,
	106, 185, 44,
	121, 207, 52,
	0, 86, 78,
	0, 108, 98,
	0, 130, 118,
	0, 152, 138,
	0, 174, 158,
	0, 196, 178,
	0, 218, 198,
	0, 241, 219,
	74, 7, 122,
	95, 21, 139,
	116, 37, 156,
	138, 53, 173,
	160, 69, 191,
	181, 85, 208,
	203, 101, 225,
	225, 117, 243,
	59, 41, 0,
	83, 55, 0,
	107, 69, 0,
	131, 84, 0,
	155, 98, 0,
	179, 113, 0,
	203, 128, 0,
	227, 143, 0,
	87, 0, 43,
	111, 11, 69,
	135, 28, 92,
	159, 45, 115,
	183, 62, 138,
	230, 74, 174,
	245, 121, 194,
	255, 156, 209,
	20, 48, 10,
	44, 74, 28,
	68, 99, 45,
	93, 124, 62,
	118, 149, 79,
	143, 174, 96,
	168, 199, 113,
	193, 225, 130,
	54, 19, 29,
	82, 44, 44,
	110, 69, 58,
	139, 95, 72,
	168, 121, 86,
	197, 147, 101,
	226, 173, 115,
	255, 199, 130,
	8, 11, 100,
	14, 22, 116,
	20, 33, 139,
	26, 44, 162,
	41, 74, 185,
	57, 104, 208,
	76, 132, 231,
	96, 160, 255,
	43, 30, 46,
	68, 50, 85,
	93, 70, 110,
	118, 91, 130,
	143, 111, 170,
	168, 132, 190,
	193, 153, 210,
	219, 174, 230,
	63, 18, 12,
	90, 38, 30,
	117, 58, 42,
	145, 78, 55,
	172, 98, 67,
	200, 118, 80,
	227, 138, 92,
	255, 159, 105,
	11, 68, 30,
	33, 94, 56,
	54, 120, 81,
	76, 147, 106,
	98, 174, 131,
	120, 201, 156,
	142, 228, 181,
	164, 255, 207,
	64, 0, 0,
	96, 0, 0,
	128, 0, 0,
	192, 0, 0,
	255, 0, 0,
	255, 64, 64,
	255, 96, 96,
	255, 128, 128,
	0, 128, 0,
	0, 196, 0,
	0, 225, 0,
	0, 240, 0,
	0, 255, 0,
	64, 255, 64,
	94, 255, 94,
	128, 255, 128,
	0, 0, 128,
	0, 0, 192,
	0, 0, 224,
	0, 0, 255,
	0, 64, 255,
	0, 94, 255,
	0, 106, 255,
	0, 128, 255,
	128, 64, 0,
	193, 97, 0,
	215, 107, 0,
	255, 128, 0,
	255, 128, 0,
	255, 149, 43,
	255, 170, 85,
	255, 193, 132,
	8, 52, 0,
	16, 64, 0,
	32, 80, 4,
	48, 96, 4,
	64, 112, 12,
	84, 132, 20,
	104, 148, 28,
	128, 168, 44,
	164, 164, 0,
	193, 193, 0,
	215, 215, 0,
	255, 255, 0,
	255, 255, 32,
	255, 255, 64,
	255, 255, 128,
	255, 255, 172,
	32, 4, 0,
	64, 20, 8,
	84, 28, 16,
	108, 44, 28,
	128, 56, 40,
	148, 72, 56,
	168, 92, 76,
	184, 108, 88,
	64, 0, 0,
	96, 8, 0,
	112, 16, 0,
	120, 32, 8,
	138, 64, 16,
	156, 72, 32,
	174, 96, 48,
	192, 128, 64,
	32, 32, 0,
	64, 64, 0,
	96, 96, 0,
	128, 128, 0,
	144, 144, 0,
	172, 172, 0,
	192, 192, 0,
	224, 224, 0,
	64, 96, 8,
	80, 108, 32,
	96, 120, 48,
	112, 144, 56,
	128, 172, 64,
	150, 210, 68,
	172, 238, 80,
	192, 255, 96,
	32, 32, 32,
	48, 48, 48,
	64, 64, 64,
	80, 80, 80,
	96, 96, 96,
	172, 172, 172,
	236, 236, 236,
	255, 255, 255,
	41, 41, 54,
	60, 45, 70,
	75, 62, 108,
	95, 77, 136,
	113, 105, 150,
	135, 120, 176,
	165, 145, 218,
	198, 191, 232,
};


// returns next matching color to an rgb
COLOR_VAL image_t::get_index_from_rgb( uint8 r, uint8 g, uint8 b )
{
	COLOR_VAL result = 0;
	// special night/player color found?
	uint32 rgb = (r<<16) + (g<<8) + b;
	for(  uint i=0;  i<lengthof(rgbtab);  i++  ) {
		if(  rgb == rgbtab[i]  ) {
			return 224+i;
		}
	}
	// best matching
	unsigned diff = 256*3;
	for(  uint i=0;  i<lengthof(special_pal);  i+=3  ) {
		unsigned cur_diff = abs(r-special_pal[i+0]) + abs(g-special_pal[i+1]) + abs(b-special_pal[i+2]);
		if(  cur_diff < diff  ) {
			result = i/3;
			diff = cur_diff;
		}
	}
	return result;
}


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
