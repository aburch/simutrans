/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../world/simworld.h"
#include "../display/simgraph.h"
#include "../simconst.h"
#include "spezial_obj_tpl.h"
#include "ground_desc.h"
#include "../dataobj/environment.h"

//const int totalslopes_single = 16;
const int totalslopes = 81;


/****************************************************************************************************
* some functions for manipulations/blending images
* maybe they should be put in their own module, even though they are only used here ...
*/

#if COLOUR_DEPTH != 0
#define red_comp(pix)    (((pix)>>10)&0x001f)
#define green_comp(pix)   (((pix)>>5)&0x001f)
#define blue_comp(pix)         ((pix)&0x001f)
#endif


/* combines a texture and a lightmap
 * just weights all pixels by the lightmap
 * @param binary if true, then a binary decision is made: if lightmap is grey then take original pixel, if not set to black
 */
static image_t* create_textured_tile(const image_t* image_lightmap, const image_t* image_texture, bool binary = false)
{
	if(  image_lightmap == NULL  ) {
		image_t *image_dest = image_t::create_single_pixel();
		image_dest->register_image();
		return image_dest;
	}

	image_t *image_dest = image_lightmap->copy_rotate(0);
#if COLOUR_DEPTH != 0
	PIXVAL* dest = image_dest->get_data();

	PIXVAL const* const texture = image_texture->get_data();
	sint16        const x_y     = image_texture->get_pic()->w;
	// now mix the images
	for (int j = 0; j < image_dest->get_pic()->h; j++) {
		sint32 x = *dest++;
		assert(x >= 0);
		const sint32 offset = (image_dest->get_pic()->y + j - image_texture->get_pic()->y) * (x_y + 3) + 2; // position of the pixel in a rectangular map
		do
		{
			sint16 runlen = *dest++;
			assert(runlen >= 0);
			for(int i=0; i<runlen; i++) {
				assert(offset+x>0);
				uint16 mix = texture[offset+x];

				if (!binary) {
					uint16 grey = *dest;
					uint16 rc = (red_comp(grey)*red_comp(mix))/16;
					if(rc>=32) {
						rc = 31;
					}
					uint16 gc = (green_comp(grey)*green_comp(mix))/16;
					if(gc>=32) {
						gc = 31;
					}
					uint16 bc = (blue_comp(grey)*blue_comp(mix))/16;
					if(bc>=32) {
						bc = 31;
					}
					*dest++ = (rc<<10) | (gc<<5) | bc;
				}
				else {
					if (*dest) { *dest = mix;}
					dest++;
				}
				x ++;
			}
			x += *dest;
		} while(  (*dest++)!=0 );
	}
	assert(dest - image_dest->get_data() == (ptrdiff_t)image_dest->get_pic()->len);
#else
	(void)image_texture;
	(void)binary;
#endif
	image_dest->register_image();
	return image_dest;
}


/* combines a texture and a lightmap
 * does a very simple stretching of the texture and the mix images
 * BEWARE: Assumes all images but image_lightmap are square!
 * BEWARE: no special colors or your will see literally blue!
 */
static image_t* create_alpha_tile(const image_t* image_lightmap, slope_t::type slope, const image_t* image_alphamap)
{
	if(  image_lightmap == NULL  ||  image_alphamap == NULL  ||  image_alphamap->get_pic()->w < 2  ) {
		image_t *image_dest = image_t::create_single_pixel();
		image_dest->register_image();
		return image_dest;
	}
	assert( image_alphamap->get_pic()->w == image_alphamap->get_pic()->h);

	image_t *image_dest = image_lightmap->copy_rotate(0);

	PIXVAL const* const alphamap  = image_alphamap->get_data();
	const sint32 x_y     = image_dest->get_pic()->w;
	const sint32 mix_x_y = image_alphamap->get_pic()->w;
	sint16 tile_x, tile_y;

	/*
	* to go from mixmap xy to tile xy is simple:
	* (x,y)_tile = (mixmap_x+mixmap_y)/2 , (mixmap_y-mixmap_x)/4+(3/4)*tilesize
	* This is easily inverted to
	* (x,y)mixmap = x_tile-2*y_tile+(3/2)*tilesize, x_tile+2*y_tile-(3/2)*tilesize
	* tricky are slopes. There we have to add an extra distortion
	* /4\
	* 1+3
	* \2/
	* Luckily this distortion is only for the y direction.
	* for corner 1: max(0,(tilesize-(x+y))*HEIGHT_STEP)/tilesize )
	* for corner 2: max(0,((y-x)*HEIGHT_STEP)/tilesize )
	* for corner 3: max(0,((x+y)-tilesize)*HEIGHT_STEP)/tilesize )
	* for corner 4: max(0,((x-y)*HEIGHT_STEP)/tilesize )
	* the maximum operators make the inversion of the above equation nearly impossible.
	*/

	// we will need them very often ...
	const sint16 corner_sw_y = (3 * x_y) / 4 - corner_sw(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner_se_y = x_y - corner_se(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner_ne_y = (3 * x_y) / 4 - corner_ne(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner_nw_y = (x_y / 2) - corner_nw(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 middle_y = (corner_se_y + corner_nw_y) / 2;
	// now mix the images
	PIXVAL* dest = image_dest->get_data();
	for(  int j = 0;  j < image_dest->get_pic()->h;  j++  ) {
		tile_y = image_dest->get_pic()->y + j;
		tile_x = *dest++;
		do {
			sint16 runlen = *dest++;
			for(  int i = 0;  i < runlen;  i++  ) {
				// now we must calculate the target pixel
				// after the upper explanation, you will understand this is longish:
				sint16 tile_y_corrected;

				// first; check, if we are front or back half
				// back half means, we are above a line from the left_y (corner_sw), middle_y, right_y (corner_se)
				const sint16 back_y = (x_y < 2) ? 0 : ( (tile_x < x_y / 2) ? corner_sw_y + ((middle_y - corner_sw_y) * tile_x) / (x_y / 2) : middle_y + ((corner_ne_y - middle_y) * (tile_x - (x_y / 2))) / (x_y / 2) );
				// in the middle? then it is just the diagonal in the mixmap
				if(  back_y == tile_y  ) {
					tile_y_corrected = 0;
				}
				else if(  back_y > tile_y  ) {
					// left quadrant calulation: mirror of right quadrat
					sint16 x = tile_x;
					if(  x >= x_y / 2  ) {
						x = x_y - tile_x;
					}
					// we are in the back tile => calculate border y
					sint16 backborder_y;
					if(  tile_x > x_y / 2  ) {
						backborder_y = corner_nw_y + ((corner_ne_y - corner_nw_y) * (x_y / 2 - x)) / (x_y / 2);
					}
					else {
						backborder_y = corner_sw_y + ((corner_nw_y - corner_sw_y) * x) / (x_y / 2);
					}
					// ok, now we have to calculate the y coordinate ...
					if(  backborder_y < tile_y  ) {
						tile_y_corrected = -((back_y - tile_y) * x) / (back_y - backborder_y);
					}
					else {
						tile_y_corrected = -x;
					}
				}
				else {
					// left quadrant calulation: mirror of right quadrat
					sint16 x = tile_x;
					// put condition this way, testing (x >= x_y) breaks if x_y == 1.
					if(  2*x >= x_y ) {
						x = x_y - tile_x;
					}
					// we are in the front tile => calculate border y
					sint16 frontborder_y = 0;
					if(  tile_x > x_y / 2  ) {
						frontborder_y = corner_se_y + ((corner_ne_y - corner_se_y) * (x_y / 2 - x)) / (x_y / 2);
					}
					else if(  x_y >=2  ) {
						frontborder_y = corner_sw_y + ((corner_se_y - corner_sw_y) * x) / (x_y / 2);
					}
					// ok, now we have to calculate the y coordinate ...
					if(  frontborder_y > tile_y  ) {
						tile_y_corrected = -((back_y - tile_y) * x) / (frontborder_y - back_y);
					}
					else {
						tile_y_corrected = x;
					}
				}

				// now we have calulated the y_t of square tile that is rotated by 45 degree
				// so we just have to do a 45 deg backtransform ...
				// (and do not forget: tile_y_corrected middle = 0!
				sint32 x_t = tile_x - tile_y_corrected;
				sint32 y_t = tile_y_corrected + tile_x;
				// due to some inexactness of integer arithmethics, we have to take care of overflow and underflow
				x_t = clamp(x_t, 0, x_y-1);
				y_t = clamp(y_t, 0, x_y-1);
				sint32 alphamap_offset = ((y_t * mix_x_y) / x_y) * (mix_x_y + 3) + 2 + (x_t * mix_x_y) / x_y;

				// see only the mixmap for mixing
				//
				// clear 0x8000 bit as it has special meaning,
				// confuses rezoom_img() and crashes later
				*dest++ = alphamap[alphamap_offset] & 0x7fff;
				tile_x++;
			}
			tile_x += *dest;
		} while(  *dest++ != 0  );
	}

	image_dest->register_image();
	return image_dest;
}


// copy ref texture, copy pixels from image into new texture
static image_t* create_texture_from_tile(const image_t* image, const image_t* ref)
{
	if(  image == NULL  ||  image->get_pic()->w < 2  ) {
		image_t *image_dest = image_t::create_single_pixel();
		return image_dest;
	}
	// assumes ref is texture image with no clear runs, full rows.
	image_t *image_dest = image_t::copy_image(*ref);
	PIXVAL *const sp2 = image_dest->get_data();

	assert(ref->w == ref->y + ref->h  &&  ref->x == 0);

	const sint32 ref_w = ref->w;
	const sint32 height= image->get_pic()->h;

	// decode image and put it into dest
	const PIXVAL* sp = image->get_data();

	for(int y = 0;  y < height;  y++  ) {

		int x = image->x;
		uint16 runlen = *sp++;

		do {
			// we start with a clear run
			x += runlen;

			// now get colored pixels
			runlen = (*sp++);

			for(uint16 i=0; i<runlen; i++) {
				PIXVAL p = *sp++;

				// macro to copy pixels into rle-encoded image, with range check
#				define copypixel(xx, yy) \
				if (ref->y <= (yy)  &&  (yy) < ref->h  &&  0 <= (xx)  &&  (xx) < ref_w) { \
					size_t const index = (ref_w + 3) * (yy - ref->y) + xx + 2; \
					assert(index < image_dest->len); \
					sp2[index] = p; \
				}
				/* Put multiple copies into dest image
				 *
				 * image is assumed to be tile shaped,
				 * and is copied four times to cover tiles of neighboring tiles.
				 *
				 * copy +   copy
				 * | /     \  |
				 * +  image   +
				 * | \     /  |
				 * copy +   copy
				 *
				 * ref image is assumed to be rectangular,
				 * it is used to fill holes due to missing pixels in image.
				 */
				copypixel(x, y + image->y);
				copypixel(x + ref_w/2, y + image->y + ref_w/4);
				copypixel(x - ref_w/2, y + image->y + ref_w/4);
				copypixel(x + ref_w/2, y + image->y - ref_w/4);
				copypixel(x - ref_w/2, y + image->y - ref_w/4);

				x++;
			}
		} while(  (runlen = *sp++)  );
	}
	// image_dest not registered
	return image_dest;
#undef copypixel
}

/****************************************************************************************************
* the real textures are registered/calculated below
*/

karte_t *ground_desc_t::world = NULL;


/* convert double to single slopes
 */
const uint8 ground_desc_t::slopetable[80] =
{
	0,    1,    0xFF, 2,    3,    0xFF, 0xFF, 0xFF, 0xFF, 4,
	5,    0xFF, 6,    7,    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 8,    9,    0xFF,
	10,   11,   0xFF, 0xFF, 0xFF, 0xFF, 12,   13,   0xFF, 14,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


// since we only use valid slope (to gain some more image slots) we use this lookup table
// 255 slopes are invalid
/* for double slope it should look like this, and for single slope like above
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 255, 255, 40, 255, 255, 41, 42, 43,
	44, 255, 255, 45, 255, 255, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 255, 255, 59, 255, 255, 60, 61, 62, 63, 255, 255, 64, 255, 255
*/
uint16 doubleslope_to_imgnr[81];


// how many animation stages we got for waves
uint16 ground_desc_t::water_animation_stages = 1;
sint16 ground_desc_t::water_depth_levels = 0;

// are double_grounds available in this pakset ?
bool ground_desc_t::double_grounds = true;

static const ground_desc_t* boden_texture            = NULL;
static const ground_desc_t* light_map                = NULL;
static const ground_desc_t* transition_water_texture = NULL;
static const ground_desc_t* transition_slope_texture = NULL;
const ground_desc_t *ground_desc_t::shore = NULL;
const ground_desc_t *ground_desc_t::fundament = NULL;
const ground_desc_t *ground_desc_t::slopes = NULL;
const ground_desc_t *ground_desc_t::fences = NULL;
const ground_desc_t *ground_desc_t::marker = NULL;
const ground_desc_t *ground_desc_t::borders = NULL;
const ground_desc_t *ground_desc_t::sea = NULL;
const ground_desc_t *ground_desc_t::outside = NULL;

static special_obj_tpl<ground_desc_t> const grounds[] = {
	{ &ground_desc_t::shore,     "Shore"          },
	{ &boden_texture,            "ClimateTexture" },
	{ &light_map,                "LightTexture"   },
	{ &transition_water_texture, "ShoreTrans"     },
	{ &transition_slope_texture, "SlopeTrans"     },
	{ &ground_desc_t::fundament, "Basement"       },
	{ &ground_desc_t::slopes,    "Slopes"         },
	{ &ground_desc_t::fences,    "Fence"          },
	{ &ground_desc_t::marker,    "Marker"         },
	{ &ground_desc_t::borders,   "Borders"        },
	{ &ground_desc_t::sea,       "Water"          },
	{ &ground_desc_t::outside,   "Outside"        },
	{ NULL, NULL }
};

// the water and seven climates
static const char* const climate_names[MAX_CLIMATES] =
{
	"sea", "desert", "tropic", "mediterran", "temperate", "tundra", "rocky", "arctic"
};

// from this number on there will be all ground images
// i.e. 15 times slopes + 7
image_id ground_desc_t::image_offset = IMG_EMPTY;
static const uint8 number_of_climates = 7;
static slist_tpl<image_t *> ground_image_list;
static image_id climate_image[32], water_image;
image_id alpha_image[totalslopes];
image_id alpha_corners_image[totalslopes * 15];
image_id alpha_water_image[totalslopes * 15];

/*
 *      called every time an object is read
 *      the object will be assigned according to its name
 */
bool ground_desc_t::register_desc(const ground_desc_t *desc)
{
	if(strcmp("Outside", desc->get_name())==0) {
		image_t const* const image = desc->get_child<image_array_t>(2)->get_image(0,0);
		dbg->message("ground_desc_t::register_desc()", "setting raster width to %i", image->get_pic()->w);
		display_set_base_raster_width(image->get_pic()->w);
	}
	// find out water animation stages
	if(strcmp("Water", desc->get_name())==0) {
		water_animation_stages = 0;
		while(  desc->get_image(0, water_animation_stages)!=IMG_EMPTY  ) {
			DBG_MESSAGE( "water", "image(0,%i)=%u", water_animation_stages, desc->get_image(0, water_animation_stages) );
			water_animation_stages ++;
		}
		// then ignore all ms settings
		if(water_animation_stages==1) {
			env_t::water_animation = 0;
		}
		water_depth_levels = desc->get_child<image_array_t>(2)->get_count()-2;
		if(water_depth_levels<=0) {
			water_depth_levels = 0;
		}
	}
	return ::register_desc(grounds, desc);
}


/*
 * called after loading; usually checks for completeness
 * however, we have to calculate all textures
 * and put them into images
 */
bool ground_desc_t::successfully_loaded()
{
	DBG_MESSAGE("ground_desc_t::successfully_loaded()","boden");
	return ::successfully_loaded(grounds+1);
}


/* returns the untranslated name of the matching climate
 */
char const* ground_desc_t::get_climate_name_from_bit(climate n)
{
	return n<MAX_CLIMATES ? climate_names[n] : NULL;
}


void ground_desc_t::init_ground_textures(karte_t *world)
{
	ground_desc_t::world = world;

	DBG_DEBUG("ground_desc_t::init_ground_textures()", "Calculating ground textures ...");

	// free old ones
	if(image_offset!=IMG_EMPTY) {
		display_free_all_images_above( image_offset );
	}
#if COLOUR_DEPTH != 0
	while (!ground_image_list.empty()) {
		delete ground_image_list.remove_first();
	}
#endif

	// not the wrong tile size?
	assert(boden_texture->get_image_ptr(0)->get_pic()->w == ground_desc_t::outside->get_image_ptr(0)->get_pic()->w);

	// create rotations of the mixer
	image_t *all_rotations_beach[totalslopes]; // water->sand->texture
	image_t *all_rotations_slope[totalslopes]; // texture1->texture2

	image_t *final_tile = NULL;

	bool full_climate = true;
	// check if there are double slopes available
	for(  int imgindex = 16;  imgindex < totalslopes;  imgindex++  ) {
		if(  light_map->get_image_ptr(imgindex) == NULL  ) {
			double_grounds = false;
			break;
		}
	}
	for(  int imgindex = 4;  imgindex < 15;  imgindex++  ) {
		if(  transition_slope_texture->get_image_ptr(imgindex) == NULL   ||
			(imgindex<=11  &&  transition_water_texture->get_image_ptr(imgindex) == NULL) ) {
			full_climate = false;
			break;
		}
	}
	// double slope needs full climates
	assert(!double_grounds  ||  full_climate);

	// calculate the matching slopes ...
	doubleslope_to_imgnr[0] = 0;
	for(  int slope = 1, slopeimgnr=1;  slope < totalslopes;  slope++  ) {
		all_rotations_beach[slope] = NULL;
		all_rotations_slope[slope] = NULL;
		doubleslope_to_imgnr[slope] = 255;

		if(  slope != 80  &&  (slope_t::is_all_up(slope)  ||  (!double_grounds  &&  slope_t::max_diff(slope)>1)  )  ) {
			// no need to initialize unneeded slopes
			// slope 80 is needed below
			continue;
		}

		// now add this image
		doubleslope_to_imgnr[slope] = slopeimgnr++;

		image_t *tmp_pic = NULL;
		switch(  slope  ) {
			case slope_t::north: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(180);
				break;
			}
			case slope_t::east: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(90);
				break;
			}
			case slope_t::south: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(0);
				break;
			}
			case slope_t::west: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(270);
				break;
			}
			case slope_t::northwest + slope_t::northeast + slope_t::southeast: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(1)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(1)->copy_rotate(0);
				break;
			}
			case slope_t::northeast + slope_t::southeast + slope_t::southwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(1)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(1)->copy_rotate(270);
				break;
			}
			case slope_t::southeast + slope_t::southwest + slope_t::northwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(1)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(1)->copy_rotate(180);
				break;
			}
			case slope_t::southwest + slope_t::northwest + slope_t::northeast: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(1)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(1)->copy_rotate(90);
				break;
			}
			case slope_t::northwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(2)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(2)->copy_rotate(90);
				break;
			}
			case slope_t::northeast: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(2)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(2)->copy_rotate(0);
				break;
			}
			case slope_t::southeast: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(2)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(2)->copy_rotate(270);
				break;
			}
			case slope_t::southwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(2)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(2)->copy_rotate(180);
				break;
			}
			case slope_t::southwest + slope_t::northeast: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(3)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(3)->copy_rotate(90);
				break;
			}
			case slope_t::southeast + slope_t::northwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(3)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_image_ptr(3)->copy_rotate(0);
				break;
			}
			case slope_t::southwest + slope_t::northeast + slope_t::southeast + slope_t::northwest: {
				if(  double_grounds  ) {
					all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(14)->copy_rotate(0);
					all_rotations_beach[slope] = transition_water_texture->get_image_ptr(11)->copy_rotate(0);
				}
				else {
					all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(0);
					all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(0);
				}
				break;
			}
			default: {
				if(  full_climate  ) {
					switch(  slope  ) {
						case slope_t::north * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(4)->copy_rotate(180);
							break;
						}
						case slope_t::east * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(4)->copy_rotate(90);
							break;
						}
						case slope_t::south * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(4)->copy_rotate(0);
							break;
						}
						case slope_t::west * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(4)->copy_rotate(270);
							break;
						}
						case slope_t::northwest * 2 + slope_t::northeast * 2 + slope_t::southeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(5)->copy_rotate(0);
							break;
						}
						case slope_t::northeast * 2 + slope_t::southeast * 2 + slope_t::southwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(5)->copy_rotate(270);
							break;
						}
						case slope_t::southeast * 2 + slope_t::southwest * 2 + slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(5)->copy_rotate(180);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northwest * 2 + slope_t::northeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(5)->copy_rotate(90);
							break;
						}
						case slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(6)->copy_rotate(90);
							break;
						}
						case slope_t::northeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(6)->copy_rotate(0);
							break;
						}
						case slope_t::southeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(6)->copy_rotate(270);
							break;
						}
						case slope_t::southwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(6)->copy_rotate(180);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(7)->copy_rotate(90);
							break;
						}
						case slope_t::southeast * 2 + slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(7)->copy_rotate(0);
							break;
						}
						case slope_t::northwest + slope_t::northeast * 2 + slope_t::southeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(13)->copy_rotate(0);
							break;
						}
						case slope_t::northeast + slope_t::southeast * 2+slope_t::southwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(13)->copy_rotate(270);
							break;
						}
						case slope_t::southeast + slope_t::southwest * 2 + slope_t::northwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(13)->copy_rotate(180);
							break;
						}
						case slope_t::southwest + slope_t::northwest * 2 + slope_t::northeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(13)->copy_rotate(90);
							break;
						}
						case slope_t::northeast * 2 + slope_t::southeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(8)->copy_rotate(270);
							break;
						}
						case slope_t::southeast * 2 + slope_t::southwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(8)->copy_rotate(180);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(8)->copy_rotate(90);
							break;
						}
						case slope_t::northwest * 2 + slope_t::northeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(8)->copy_rotate(0);
							break;
						}
						case slope_t::northwest + slope_t::northeast * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(8)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::northeast + slope_t::southeast * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(8)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::southeast + slope_t::southwest * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(8)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest * 2 + slope_t::southwest: {
							tmp_pic = transition_slope_texture->get_image_ptr(8)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest * 2 + slope_t::southeast + slope_t::southwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(12)->copy_rotate(270);
							break;
						}
						case slope_t::northwest * 2 + slope_t::northeast * 2 + slope_t::southwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(12)->copy_rotate(180);
							break;
						}
						case slope_t::northwest + slope_t::northeast * 2 + slope_t::southeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(12)->copy_rotate(90);
							break;
						}
						case slope_t::northeast + slope_t::southeast * 2 + slope_t::southwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(12)->copy_rotate(0);
							break;
						}
						case slope_t::northeast * 2 + slope_t::southeast * 2 + slope_t::southwest: {
							tmp_pic = transition_slope_texture->get_image_ptr(12)->copy_rotate(270);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest + slope_t::southeast * 2 + slope_t::southwest * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(12)->copy_rotate(180);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest * 2 + slope_t::northeast + slope_t::southwest * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(12)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest * 2 + slope_t::northeast * 2 + slope_t::southeast: {
							tmp_pic = transition_slope_texture->get_image_ptr(12)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::southwest + slope_t::southeast + slope_t::northeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(10)->copy_rotate(270);
							break;
						}
						case slope_t::southwest + slope_t::northwest + slope_t::southeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(10)->copy_rotate(180);
							break;
						}
						case slope_t::northwest + slope_t::northeast + slope_t::southwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(10)->copy_rotate(90);
							break;
						}
						case slope_t::southeast + slope_t::northeast + slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(10)->copy_rotate(0);
							break;
						}
						case slope_t::southwest + slope_t::southeast + slope_t::northwest * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(10)->copy_rotate(270);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::southwest + slope_t::northwest + slope_t::northeast * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(10)->copy_rotate(180);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::northwest + slope_t::northeast + slope_t::southeast * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(10)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case slope_t::southeast + slope_t::northeast + slope_t::southwest * 2: {
							tmp_pic = transition_slope_texture->get_image_ptr(10)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case slope_t::southeast + slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(9)->copy_rotate(270);
							break;
						}
						case slope_t::southwest + slope_t::northeast * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(9)->copy_rotate(180);
							break;
						}
						case slope_t::southeast * 2 + slope_t::northwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(9)->copy_rotate(90);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(9)->copy_rotate(0);
							break;
						}
						case slope_t::southeast * 2 + slope_t::northwest * 2 + slope_t::southwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(11)->copy_rotate(270);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northeast * 2 + slope_t::northwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(11)->copy_rotate(180);
							break;
						}
						case slope_t::southeast * 2 + slope_t::northwest * 2 + slope_t::northeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(11)->copy_rotate(90);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northeast * 2 + slope_t::southeast: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(11)->copy_rotate(0);
							break;
						}
						case slope_t::southwest * 2 + slope_t::northeast * 2 + slope_t::southeast * 2 + slope_t::northwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(14)->copy_rotate(0);
							all_rotations_beach[slope] = transition_water_texture->get_image_ptr(11)->copy_rotate(0);
							break;
						}
					}
				}
				else {
					all_rotations_slope[slope] = NULL;
					if (slope == slope_t::southwest * 2 + slope_t::northeast * 2 + slope_t::southeast * 2 + slope_t::northwest * 2) {
						all_rotations_slope[slope] = transition_slope_texture->get_image_ptr(0)->copy_rotate(0);
						all_rotations_beach[slope] = transition_water_texture->get_image_ptr(0)->copy_rotate(0);
					}
				}
				break;
			}
		}
	}

	// from here on the images are generated by us => deletion also by us then
	image_offset = get_image_count();
	DBG_MESSAGE("ground_desc_t::init_ground_textures()","image_offset=%d", image_offset );

	// water images for water and overlay
	water_image = image_offset;

	image_t **water_stage_texture = new image_t*[water_animation_stages];
	for(uint16 stage = 0; stage < water_animation_stages; stage++) {
		water_stage_texture[stage] = create_texture_from_tile(sea->get_image_ptr(0 /*depth*/, stage), boden_texture->get_image_ptr(water_climate));
	}
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		for(uint16 stage = 0; stage < water_animation_stages; stage++) {
			if(  doubleslope_to_imgnr[dslope] != 255  ) {
				int slope = double_grounds ? dslope : slopetable[dslope];
				final_tile = create_textured_tile( light_map->get_image_ptr( slope ), water_stage_texture[stage], true);
				ground_image_list.append( final_tile );
			}
		}
	}
	for(uint16 stage = 0; stage < water_animation_stages; stage++) {
		delete water_stage_texture[stage];
	}
	delete [] water_stage_texture;

	// now the other transitions
	for(  int i=0;  i < number_of_climates;  i++  ) {
		// normal tile (no transition, not snow)
		climate_image[i] = get_image_count();
		for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
			if(  doubleslope_to_imgnr[dslope] != 255  ) {
				int slope = double_grounds ? dslope : slopetable[dslope];
				final_tile = create_textured_tile( light_map->get_image_ptr( slope ), boden_texture->get_image_ptr( i+1 ) );
				ground_image_list.append( final_tile );
			}
		}
	}
	// finally full snow
	climate_image[number_of_climates] = final_tile->get_id() + 1;
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		if(  doubleslope_to_imgnr[dslope] != 255  ) {
			int slope = double_grounds ? dslope : slopetable[dslope];
			final_tile = create_textured_tile( light_map->get_image_ptr( slope ), boden_texture->get_image_ptr( arctic_climate ) );
			ground_image_list.append( final_tile );
		}
	}

	// alpha slopes for snowline
	for(  int dslope = 1;  dslope < totalslopes - 1;  dslope++  ) {
		if(  doubleslope_to_imgnr[dslope] != 255  ) {
			int slope = double_grounds ? dslope : slopetable[dslope];
			final_tile = create_alpha_tile( light_map->get_image_ptr( slope ), dslope, all_rotations_slope[dslope] );
			alpha_image[dslope] = final_tile->get_id();
		}
		else {
			alpha_image[dslope] = IMG_EMPTY;
		}
	}

	// alpha transitions for climates
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		for(  int corners = 1;  corners < 16;  corners++  ) {
			if(  doubleslope_to_imgnr[dslope] != 255  ) {
				// slope of tile
				int slope = double_grounds ? dslope : slopetable[dslope];
				// corners with transition
				uint8 double_corners = corners == 15 ? 80 : scorner_sw(corners) + 3 * scorner_se(corners) + 9 * scorner_ne(corners) + 27 * scorner_nw(corners);

				// create alpha image
				final_tile = create_alpha_tile( light_map->get_image_ptr( slope ), dslope, all_rotations_slope[double_corners] );
				alpha_corners_image[dslope * 15 + corners - 1] = final_tile->get_id();

				double_corners = corners == 15 ? 80 : (1 - scorner_sw(corners)) + 3 * (1 - scorner_se(corners)) + 9 * (1 - scorner_ne(corners)) + 27 * (1 - scorner_nw(corners));
				if(  all_rotations_beach[double_corners]  ) {
					final_tile = create_alpha_tile( light_map->get_image_ptr( slope ), dslope, all_rotations_beach[double_corners] );
					alpha_water_image[dslope * 15 + corners - 1] = final_tile->get_id();
				}
			}
			else {
				alpha_corners_image[dslope * 15 + corners - 1] = IMG_EMPTY;
				alpha_water_image[dslope * 15 + corners - 1] = IMG_EMPTY;
			}
		}
	}

#if COLOUR_DEPTH != 0
	// free the helper bitmap
	for(  int slope = 1;  slope < totalslopes;  slope++  ) {
		delete all_rotations_slope[slope];
		delete all_rotations_beach[slope];
	}
#endif
	//dbg->message("ground_desc_t::calc_water_level()", "Last image nr %u", final_tile->get_pic()->imageid);
	DBG_DEBUG("ground_desc_t::init_ground_textures()", "Init ground textures successful");
}


/* returns a ground image for all ground tiles
 * current is the current climate
 * transition is true, if the next level is another climate
 * snow is true, if above slowline
 *
 * Since not all of the climates are used in their numerical order, we use a
 * private (static table "height_to_texture_climate" for lookup)
 */
image_id ground_desc_t::get_ground_tile(grund_t *gr)
{
	slope_t::type slope = gr->get_grund_hang();
	sint16 height = gr->get_hoehe();
	koord k = gr->get_pos().get_2d();
	const sint16 tile_h = height - world->get_water_hgt(k);
	if(  tile_h < 0  ||  (tile_h == 0  &&  slope == slope_t::flat)  ) {
		// deep water
		image_array_t const* const list = sea->get_child<image_array_t>(2);
		int nr = min( -tile_h, list->get_count() - 2 );
		return list->get_image( nr, 0 )->get_id();
	}
	else {
		const bool snow = height >= world->get_snowline();
		const sint16 climate_nr = snow ? number_of_climates : (world->get_climate(k) > 1 ? world->get_climate(k) - 1 : 0);
		// returns base climate for tile, transitions will be overlayed later
		return climate_image[climate_nr] + doubleslope_to_imgnr[slope];
	}
	return IMG_EMPTY;
}


image_id ground_desc_t::get_water_tile(slope_t::type slope, int stage)
{
	return water_image + stage + water_animation_stages*doubleslope_to_imgnr[slope];
}


image_id ground_desc_t::get_climate_tile(climate cl, slope_t::type slope)
{
	return climate_image[cl <= 0 ? 0 : cl - 1] + doubleslope_to_imgnr[slope];
}


image_id ground_desc_t::get_snow_tile(slope_t::type slope)
{
	return climate_image[number_of_climates] + doubleslope_to_imgnr[slope];
}


image_id ground_desc_t::get_beach_tile(slope_t::type slope, uint8 corners)
{
	return alpha_water_image[slope * 15 + corners - 1];
}


image_id ground_desc_t::get_alpha_tile(slope_t::type slope, uint8 corners)
{
	return alpha_corners_image[slope * 15 + corners - 1];
}


image_id ground_desc_t::get_alpha_tile(slope_t::type slope)
{
	return alpha_image[slope];
}
