/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *  2006 completely changed by prissi and Kieron Green
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Modulbeschreibung:
 *      Calculates the ground for different levels using the textures here
 *
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simgraph.h"
#include "../simconst.h"
#include "spezial_obj_tpl.h"
#include "grund_besch.h"
#include "../dataobj/umgebung.h"

const int totalslopes_single = 16;
const int totalslopes = 81;


/****************************************************************************************************
* some functions for manipulations/blending images
* maybe they should be put in their own module, even though they are only used here ...
*/

typedef uint16 PIXVAL;

#define red_comp(pix)			(((pix)>>10)&0x001f)
#define green_comp(pix)		(((pix)>>5)&0x001f)
#define blue_comp(pix)			((pix)&0x001f)


/*  mixed_color
	this uses pixel map to produce an pixel from pixels src1, src2 and src3
	src1 uses the red component of map
	src2 uses the green component of map
	src3 uses the blue component of map
*/
/*static PIXVAL mixed_color(PIXVAL map, PIXVAL src1, PIXVAL src2, PIXVAL src3)
{
	if(map!=0) {
		uint16 rc = red_comp(map);
		uint16 gc = green_comp(map);
		uint16 bc = blue_comp(map);

		// overflow safe method ...
		uint16 rcf = (rc*red_comp(src1) + gc*red_comp(src2) + bc*red_comp(src3) )/(rc+gc+bc);
		uint16 gcf = (rc*green_comp(src1) + gc*green_comp(src2) + bc*green_comp(src3) )/(rc+bc+gc);
		uint16 bcf = (rc*blue_comp(src1) + gc*blue_comp(src2) + bc*blue_comp(src3) )/(rc+gc+bc);

		return (bcf)|(gcf<<5)|(rcf<<10);
	}
	return 0;
} */


/* combines a texture and a lightmap
 * just weights all pixels by the lightmap
 */
static bild_besch_t* create_textured_tile(const bild_besch_t* bild_lightmap, const bild_besch_t* bild_texture)
{
	if(  bild_lightmap == NULL  ) {
		bild_besch_t *bild_dest = bild_besch_t::create_single_pixel();
		bild_dest->register_image();
		return bild_dest;
	}

	bild_besch_t *bild_dest = bild_lightmap->copy_rotate(0);
#if COLOUR_DEPTH != 0
	PIXVAL* dest = bild_dest->get_daten();

	PIXVAL const* const texture = bild_texture->get_daten();
	sint16        const x_y     = bild_texture->get_pic()->w;
	// now mix the images
	for (int j = 0; j < bild_dest->get_pic()->h; j++) {
		sint16 x = *dest++;
		const sint16 offset = (bild_dest->get_pic()->y + j - bild_texture->get_pic()->y) * (x_y + 3) + 2; // position of the pixel in a rectangular map
		do
		{
			sint16 runlen = *dest++;
			for(int i=0; i<runlen; i++) {
				uint16 mix = texture[offset+x];
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
				x ++;
			}
			x += *dest;
		} while(  (*dest++)!=0 );
	}
	assert(dest - bild_dest->get_daten() == (ptrdiff_t)bild_dest->get_pic()->len);
#endif
	bild_dest->register_image();
	return bild_dest;
}


/* combines a texture and a lightmap
 * does a very simple stretching of the texture and the mix images
 * BEWARE: Assumes all images but bild_lightmap are square!
 * BEWARE: no special colors or your will see literally blue!
 */
/*static bild_besch_t* create_textured_tile_mix(const bild_besch_t* bild_lightmap, hang_t::typ slope, const bild_besch_t* bild_mixmap,  const bild_besch_t* bild_src1, const bild_besch_t* bild_src2, const bild_besch_t* bild_src3)
{
	bild_besch_t *bild_dest = bild_lightmap->copy_rotate(0);
#if COLOUR_DEPTH != 0
	PIXVAL const* const mixmap  = bild_mixmap->get_daten();
	PIXVAL const* const src1    = bild_src1->get_daten() - bild_src1->get_pic()->y * (bild_src1->get_pic()->w + 3L);
	PIXVAL const* const src2    = bild_src2->get_daten() - bild_src2->get_pic()->y * (bild_src2->get_pic()->w + 3L);
	PIXVAL const* const src3    = bild_src3->get_daten() - bild_src3->get_pic()->y * (bild_src3->get_pic()->w + 3L);
	sint32        const x_y     = bild_src1->get_pic()->w;
	sint32        const mix_x_y = bild_mixmap->get_pic()->w;
	sint16 tile_x, tile_y;

*/	/*
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

/*	// we will need them very often ...
	const sint16 corner1_y = (3*x_y)/4 - corner1(slope)*tile_raster_scale_y(TILE_HEIGHT_STEP,x_y);
	const sint16 corner2_y = x_y - corner2(slope)*tile_raster_scale_y(TILE_HEIGHT_STEP,x_y);
	const sint16 corner3_y = (3*x_y)/4 - corner3(slope)*tile_raster_scale_y(TILE_HEIGHT_STEP,x_y);
	const sint16 corner4_y = (x_y/2) - corner4(slope)*tile_raster_scale_y(TILE_HEIGHT_STEP,x_y);
	const sint16 middle_y = (corner2_y+corner4_y)/2;

	// now mix the images
	PIXVAL* dest = bild_dest->get_daten();
	for (int j = 0; j < bild_dest->get_pic()->h; j++) {
		tile_y = bild_dest->get_pic()->y + j;
		tile_x = *dest++;
		// offset is the pixel position in the mixmap bitmaps;
		// so we can avoid stretching the mixmaps
		const sint32 offset = (bild_dest->get_pic()->y + j) * (x_y + 3) + 2;
		do
		{
			sint16 runlen = *dest++;
			for(int i=0; i<runlen; i++) {
				// now we must calculate the target pixel
				// after the upper explanation, you will understand this is longish:
				sint16 tile_y_corrected;

				// first; check, if we are front or back half
				// back half means, we are above a line from the left_y (corner1), middle_y, right_y (corner2)
				const sint16 back_y = (tile_x<x_y/2) ? corner1_y + ((middle_y-corner1_y)*tile_x)/(x_y/2) : middle_y + ((corner3_y-middle_y)*(tile_x-(x_y/2)))/(x_y/2);
				// in the middle? the it is just the diagonal in the mixmap
				if(back_y==tile_y) {
					tile_y_corrected = 0;
				}
				else if(back_y>tile_y) {
					// left quadrant calulation: mirror of right quadrat
					sint16 x = tile_x;
					if(x>=x_y/2) {
						x = x_y-tile_x;
					}
					// we are in the back tile => calculate border y
					sint16 backborder_y;
					if(tile_x>x_y/2) {
						backborder_y = corner4_y + ((corner3_y-corner4_y)*(x_y/2-x))/(x_y/2);
					}
					else {
						backborder_y = corner1_y + ((corner4_y-corner1_y)*x)/(x_y/2);
					}
					// ok, now we have to calculate the y coordinate ...
					if(backborder_y<tile_y) {
						tile_y_corrected = -((back_y-tile_y)*x)/(back_y - backborder_y);
					}
					else {
						tile_y_corrected = -x;
					}
				}
				else {
					// left quadrant calulation: mirror of right quadrat
					sint16 x = tile_x;
					if(x>=x_y/2) {
						x = x_y-tile_x;
					}
					// we are in the front tile => calculate border y
					sint16 frontborder_y;
					if(tile_x>x_y/2) {
						frontborder_y = corner2_y + ((corner3_y-corner2_y)*(x_y/2-x))/(x_y/2);
					}
					else {
						frontborder_y = corner1_y + ((corner2_y-corner1_y)*x)/(x_y/2);
					}
					// ok, now we have to calculate the y coordinate ...
					if(frontborder_y>tile_y) {
						tile_y_corrected = -((back_y-tile_y)*x)/(frontborder_y-back_y);
					}
					else {
						tile_y_corrected = x;
					}
				}

				// now we have calulated the y_t of square tile that is rotated by 45 degree
				// so we just have to do a 45 deg backtransform ...
				// (and do not forget: tile_y_corrected middle = 0!
				sint16 x_t = tile_x - tile_y_corrected;
				sint16 y_t = tile_y_corrected+tile_x;
				// due to some inexactness of interger arithmethics, we have to take care of overflow
				if(y_t>=x_y) {
					y_t = x_y-1;
				}
				if(x_t>=x_y) {
					x_t = x_y-1;
				}
				sint32 mixmap_offset = ( (y_t*mix_x_y) / x_y) *(mix_x_y+3) + 2 + (x_t * mix_x_y)/x_y;

				PIXVAL mix = mixed_color(mixmap[mixmap_offset],src1[offset+tile_x],src2[offset+tile_x],src3[offset+tile_x]);
				// to see onyl the mixmap for mixing, uncomment next line
//				PIXVAL mix = mixmap[mixmap_offset];
				PIXVAL grey = *dest;
				PIXVAL rc = (red_comp(grey)*red_comp(mix))/16;
				// just avoid overflow
				if(rc>=32) {
					rc = 31;
				}
				PIXVAL gc = (green_comp(grey)*green_comp(mix))/16;
				if(gc>=32) {
					gc = 31;
				}
				PIXVAL bc = (blue_comp(grey)*blue_comp(mix))/16;
				if(bc>=32) {
					bc = 31;
				}
				*dest++ = (rc<<10) | (gc<<5) | bc;
				tile_x ++;
			}
			tile_x += *dest;
		} while(  *dest++!=0 );
	}
#endif
	bild_dest->register_image();
	return bild_dest;
} */


/* combines a texture and a lightmap
 * does a very simple stretching of the texture and the mix images
 * BEWARE: Assumes all images but bild_lightmap are square!
 * BEWARE: no special colors or your will see literally blue!
 */
static bild_besch_t* create_alpha_tile(const bild_besch_t* bild_lightmap, hang_t::typ slope, const bild_besch_t* bild_alphamap)
{
	if(  bild_lightmap == NULL  ||  bild_alphamap == NULL  ||  bild_alphamap->get_pic()->w < 2  ) {
		bild_besch_t *bild_dest = bild_besch_t::create_single_pixel();
		bild_dest->register_image();
		return bild_dest;
	}
	assert( bild_alphamap->get_pic()->w == bild_alphamap->get_pic()->h);

	bild_besch_t *bild_dest = bild_lightmap->copy_rotate(0);

	PIXVAL const* const alphamap  = bild_alphamap->get_daten();
	const sint32 x_y     = bild_dest->get_pic()->w;
	const sint32 mix_x_y = bild_alphamap->get_pic()->w;
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
	const sint16 corner1_y = (3 * x_y) / 4 - corner1(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner2_y = x_y - corner2(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner3_y = (3 * x_y) / 4 - corner3(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 corner4_y = (x_y / 2) - corner4(slope) * tile_raster_scale_y( TILE_HEIGHT_STEP, x_y );
	const sint16 middle_y = (corner2_y + corner4_y) / 2;
	// now mix the images
	PIXVAL* dest = bild_dest->get_daten();
	for(  int j = 0;  j < bild_dest->get_pic()->h;  j++  ) {
		tile_y = bild_dest->get_pic()->y + j;
		tile_x = *dest++;
		do {
			sint16 runlen = *dest++;
			for(  int i = 0;  i < runlen;  i++  ) {
				// now we must calculate the target pixel
				// after the upper explanation, you will understand this is longish:
				sint16 tile_y_corrected;

				// first; check, if we are front or back half
				// back half means, we are above a line from the left_y (corner1), middle_y, right_y (corner2)
				const sint16 back_y = (tile_x < x_y / 2) ? corner1_y + ((middle_y - corner1_y) * tile_x) / (x_y / 2) : middle_y + ((corner3_y - middle_y) * (tile_x - (x_y / 2))) / (x_y / 2);
				// in the middle? the it is just the diagonal in the mixmap
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
						backborder_y = corner4_y + ((corner3_y - corner4_y) * (x_y / 2 - x)) / (x_y / 2);
					}
					else {
						backborder_y = corner1_y + ((corner4_y - corner1_y) * x) / (x_y / 2);
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
					if(  x >= x_y / 2  ) {
						x = x_y - tile_x;
					}
					// we are in the front tile => calculate border y
					sint16 frontborder_y;
					if(  tile_x > x_y / 2  ) {
						frontborder_y = corner2_y + ((corner3_y - corner2_y) * (x_y / 2 - x)) / (x_y / 2);
					}
					else {
						frontborder_y = corner1_y + ((corner2_y - corner1_y) * x) / (x_y / 2);
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
				sint16 x_t = tile_x - tile_y_corrected;
				sint16 y_t = tile_y_corrected + tile_x;
				// due to some inexactness of interger arithmethics, we have to take care of overflow
				if(  y_t >= x_y  ) {
					y_t = x_y - 1;
				}
				if(  x_t >= x_y  ) {
					x_t = x_y - 1;
				}
				sint32 alphamap_offset = ((y_t * mix_x_y) / x_y) * (mix_x_y + 3) + 2 + (x_t * mix_x_y) / x_y;

				// see only the mixmap for mixing
				*dest++ = alphamap[alphamap_offset];
				tile_x++;
			}
			tile_x += *dest;
		} while(  *dest++ != 0  );
	}

	bild_dest->register_image();
	return bild_dest;
}



/****************************************************************************************************
* the real textures are registered/calculated below
*/

karte_t *grund_besch_t::welt = NULL;


/* convert double to single slopes
 */
const uint8 grund_besch_t::slopetable[80] =
{
	0, 1 , 0xFF,
	2 , 3 , 0xFF, 0xFF, 0xFF, 0xFF,
	4 , 5, 0xFF, 6, 7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	8, 9, 0xFF, 10, 11, 0xFF, 0xFF, 0xFF, 0xFF, 12, 13, 0xFF,  14, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// how many animation stages we got for waves
uint16 grund_besch_t::water_animation_stages = 1;
sint16 grund_besch_t::water_depth_levels = 0;

// are double_grounds available in this pakset ?
bool grund_besch_t::double_grounds = true;

static const grund_besch_t* boden_texture            = NULL;
static const grund_besch_t* light_map                = NULL;
static const grund_besch_t* transition_water_texture = NULL;
static const grund_besch_t* transition_slope_texture = NULL;
const grund_besch_t *grund_besch_t::fundament = NULL;
const grund_besch_t *grund_besch_t::slopes = NULL;
const grund_besch_t *grund_besch_t::fences = NULL;
const grund_besch_t *grund_besch_t::marker = NULL;
const grund_besch_t *grund_besch_t::borders = NULL;
const grund_besch_t *grund_besch_t::sea = NULL;
const grund_besch_t *grund_besch_t::ausserhalb = NULL;

static spezial_obj_tpl<grund_besch_t> grounds[] = {
    { &boden_texture,	    "ClimateTexture" },
    { &light_map,	    "LightTexture" },
    { &transition_water_texture,    "ShoreTrans" },
    { &transition_slope_texture,    "SlopeTrans" },
    { &grund_besch_t::fundament,    "Basement" },
    { &grund_besch_t::slopes,    "Slopes" },
    { &grund_besch_t::fences,   "Fence" },
    { &grund_besch_t::marker,   "Marker" },
    { &grund_besch_t::borders,   "Borders" },
    { &grund_besch_t::sea,   "Water" },
    { &grund_besch_t::ausserhalb,   "Outside" },
    { NULL, NULL }
};

// the water and seven climates
static const char* const climate_names[MAX_CLIMATES] =
{
    "Water", "desert", "tropic", "mediterran", "temperate", "tundra", "rocky", "arctic"
};

// from this number on there will be all ground images
// i.e. 15 times slopes + 7
image_id grund_besch_t::image_offset = IMG_LEER;
static const uint8 number_of_climates = 7;
static slist_tpl<bild_besch_t *> ground_bild_list;
static uint16 climate_bild[32];
image_id alpha_bild[totalslopes];
image_id alpha_corners_bild[totalslopes * 15];
image_id alpha_water_bild[totalslopes * 15];

/*
 *      called every time an object is read
 *      the object will be assigned according to its name
 */
bool grund_besch_t::register_besch(const grund_besch_t *besch)
{
	if(strcmp("Outside", besch->get_name())==0) {
		bild_besch_t const* const bild = besch->get_child<bildliste2d_besch_t>(2)->get_bild(0,0);
		dbg->message("grund_besch_t::register_besch()", "setting raster width to %i", bild->get_pic()->w);
		display_set_base_raster_width(bild->get_pic()->w);
	}
	// find out water animation stages
	if(strcmp("Water", besch->get_name())==0) {
		water_animation_stages = 0;
		while(  besch->get_bild(0, water_animation_stages)!=IMG_LEER  ) {
			DBG_MESSAGE( "water", "bild(0,%i)=%u", water_animation_stages, besch->get_bild(0, water_animation_stages) );
			water_animation_stages ++;
		}
		// then ignore all ms settings
		if(water_animation_stages==1) {
			umgebung_t::water_animation = 0;
		}
		water_depth_levels = besch->get_child<bildliste2d_besch_t>(2)->get_anzahl()-2;
		if(water_depth_levels<=0) {
			water_depth_levels = 0;
		}
	}
	return 	::register_besch(grounds, besch);
}


/*
 * called after loading; usually checks for completeness
 * however, in our case hang_t::we have to calculate all textures
 * and put them into images
 */
bool grund_besch_t::alles_geladen()
{
	DBG_MESSAGE("grund_besch_t::alles_geladen()","boden");
	return ::alles_geladen(grounds);
}


/* returns the untranslated name of the matching climate
 */
char const* grund_besch_t::get_climate_name_from_bit(climate n)
{
	return n<MAX_CLIMATES ? climate_names[n] : NULL;
}


void grund_besch_t::init_ground_textures(karte_t *w)
{
	grund_besch_t::welt = w;

	printf("Calculating textures ...");

	// free old ones
	if(image_offset!=IMG_LEER) {
		display_free_all_images_above( image_offset );
	}
#if COLOUR_DEPTH != 0
	while (!ground_bild_list.empty()) {
		delete ground_bild_list.remove_first();
	}
#endif

	// not the wrong tile size?
	assert(boden_texture->get_bild_ptr(0)->get_pic()->w == grund_besch_t::ausserhalb->get_bild_ptr(0)->get_pic()->w);

	// create rotations of the mixer
	bild_besch_t *all_rotations_beach[totalslopes]; // water->sand->texture
	bild_besch_t *all_rotations_slope[totalslopes]; // texture1->texture2

	bild_besch_t *final_tile;

	// check if there are double slopes available
	for(  int imgindex = 4;  imgindex < 15;  imgindex++  ) {
		if(  transition_slope_texture->get_bild_ptr(imgindex) == NULL  ) {
			double_grounds = false;
			break;
		}
	}

	// calculate the matching slopes ...
	for(  int slope = 1;  slope < totalslopes;  slope++  ) {
		all_rotations_beach[slope] = NULL;
		all_rotations_slope[slope] = NULL;
		bild_besch_t *tmp_pic = NULL;
		switch(  slope  ) {
			case hang_t::nord: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(0)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(0)->copy_rotate(180);
				break;
			}
			case hang_t::ost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(0)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(0)->copy_rotate(90);
				break;
			}
			case hang_t::sued: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(0)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(0)->copy_rotate(0);
				break;
			}
			case hang_t::west: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(0)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(0)->copy_rotate(270);
				break;
			}
			case hang_t::nordwest + hang_t::nordost + hang_t::suedost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(1)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(1)->copy_rotate(0);
				break;
			}
			case hang_t::nordost + hang_t::suedost + hang_t::suedwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(1)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(1)->copy_rotate(270);
				break;
			}
			case hang_t::suedost + hang_t::suedwest + hang_t::nordwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(1)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(1)->copy_rotate(180);
				break;
			}
			case hang_t::suedwest + hang_t::nordwest + hang_t::nordost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(1)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(1)->copy_rotate(90);
				break;
			}
			case hang_t::nordwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(2)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(2)->copy_rotate(90);
				break;
			}
			case hang_t::nordost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(2)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(2)->copy_rotate(0);
				break;
			}
			case hang_t::suedost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(2)->copy_rotate(270);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(2)->copy_rotate(270);
				break;
			}
			case hang_t::suedwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(2)->copy_rotate(180);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(2)->copy_rotate(180);
				break;
			}
			case hang_t::suedwest + hang_t::nordost: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(3)->copy_rotate(90);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(3)->copy_rotate(90);
				break;
			}
			case hang_t::suedost + hang_t::nordwest: {
				all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(3)->copy_rotate(0);
				all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(3)->copy_rotate(0);
				break;
			}
			case hang_t::suedwest + hang_t::nordost + hang_t::suedost + hang_t::nordwest: {
				if(  double_grounds  ) {
					all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(14)->copy_rotate(0);
					all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(11)->copy_rotate(0);
				}
				else {
					all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(0)->copy_rotate(0);
					all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(0)->copy_rotate(0);
				}
				break;
			}
			default: {
				if(  double_grounds  ) {
					switch(  slope  ) {
						case hang_t::nord * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(4)->copy_rotate(180);
							break;
						}
						case hang_t::ost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(4)->copy_rotate(90);
							break;
						}
						case hang_t::sued * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(4)->copy_rotate(0);
							break;
						}
						case hang_t::west * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(4)->copy_rotate(270);
							break;
						}
						case hang_t::nordwest * 2 + hang_t::nordost * 2 + hang_t::suedost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(5)->copy_rotate(0);
							break;
						}
						case hang_t::nordost * 2 + hang_t::suedost * 2 + hang_t::suedwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(5)->copy_rotate(270);
							break;
						}
						case hang_t::suedost * 2 + hang_t::suedwest * 2 + hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(5)->copy_rotate(180);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordwest * 2 + hang_t::nordost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(5)->copy_rotate(90);
							break;
						}
						case hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(6)->copy_rotate(90);
							break;
						}
						case hang_t::nordost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(6)->copy_rotate(0);
							break;
						}
						case hang_t::suedost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(6)->copy_rotate(270);
							break;
						}
						case hang_t::suedwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(6)->copy_rotate(180);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(7)->copy_rotate(90);
							break;
						}
						case hang_t::suedost * 2 + hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(7)->copy_rotate(0);
							break;
						}
						case hang_t::nordwest + hang_t::nordost * 2 + hang_t::suedost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(13)->copy_rotate(0);
							break;
						}
						case hang_t::nordost + hang_t::suedost * 2+hang_t::suedwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(13)->copy_rotate(270);
							break;
						}
						case hang_t::suedost + hang_t::suedwest * 2 + hang_t::nordwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(13)->copy_rotate(180);
							break;
						}
						case hang_t::suedwest + hang_t::nordwest * 2 + hang_t::nordost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(13)->copy_rotate(90);
							break;
						}
						case hang_t::nordost * 2 + hang_t::suedost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(8)->copy_rotate(270);
							break;
						}
						case hang_t::suedost * 2 + hang_t::suedwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(8)->copy_rotate(180);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(8)->copy_rotate(90);
							break;
						}
						case hang_t::nordwest * 2 + hang_t::nordost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(8)->copy_rotate(0);
							break;
						}
						case hang_t::nordwest + hang_t::nordost * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(8)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::nordost + hang_t::suedost * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(8)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::suedost + hang_t::suedwest * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(8)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest * 2 + hang_t::suedwest: {
							tmp_pic = transition_slope_texture->get_bild_ptr(8)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest * 2 + hang_t::suedost + hang_t::suedwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(12)->copy_rotate(270);
							break;
						}
						case hang_t::nordwest * 2 + hang_t::nordost * 2 + hang_t::suedwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(12)->copy_rotate(180);
							break;
						}
						case hang_t::nordwest + hang_t::nordost * 2 + hang_t::suedost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(12)->copy_rotate(90);
							break;
						}
						case hang_t::nordost + hang_t::suedost * 2 + hang_t::suedwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(12)->copy_rotate(0);
							break;
						}
						case hang_t::nordost * 2 + hang_t::suedost * 2 + hang_t::suedwest: {
							tmp_pic = transition_slope_texture->get_bild_ptr(12)->copy_rotate(270);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest + hang_t::suedost * 2 + hang_t::suedwest * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(12)->copy_rotate(180);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest * 2 + hang_t::nordost + hang_t::suedwest * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(12)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest * 2 + hang_t::nordost * 2 + hang_t::suedost: {
							tmp_pic = transition_slope_texture->get_bild_ptr(12)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::suedwest + hang_t::suedost + hang_t::nordost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(10)->copy_rotate(270);
							break;
						}
						case hang_t::suedwest + hang_t::nordwest + hang_t::suedost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(10)->copy_rotate(180);
							break;
						}
						case hang_t::nordwest + hang_t::nordost + hang_t::suedwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(10)->copy_rotate(90);
							break;
						}
						case hang_t::suedost + hang_t::nordost + hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(10)->copy_rotate(0);
							break;
						}
						case hang_t::suedwest + hang_t::suedost + hang_t::nordwest * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(10)->copy_rotate(270);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::suedwest + hang_t::nordwest + hang_t::nordost * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(10)->copy_rotate(180);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::nordwest + hang_t::nordost + hang_t::suedost * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(10)->copy_rotate(90);
							all_rotations_slope[slope] = tmp_pic->copy_flipvertical();
							delete tmp_pic;
							break;
						}
						case hang_t::suedost + hang_t::nordost + hang_t::suedwest * 2: {
							tmp_pic = transition_slope_texture->get_bild_ptr(10)->copy_rotate(0);
							all_rotations_slope[slope] = tmp_pic->copy_fliphorizontal();
							delete tmp_pic;
							break;
						}
						case hang_t::suedost + hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(9)->copy_rotate(270);
							break;
						}
						case hang_t::suedwest + hang_t::nordost * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(9)->copy_rotate(180);
							break;
						}
						case hang_t::suedost * 2 + hang_t::nordwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(9)->copy_rotate(90);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(9)->copy_rotate(0);
							break;
						}
						case hang_t::suedost * 2 + hang_t::nordwest * 2 + hang_t::suedwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(11)->copy_rotate(270);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordost * 2 + hang_t::nordwest: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(11)->copy_rotate(180);
							break;
						}
						case hang_t::suedost * 2 + hang_t::nordwest * 2 + hang_t::nordost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(11)->copy_rotate(90);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordost * 2 + hang_t::suedost: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(11)->copy_rotate(0);
							break;
						}
						case hang_t::suedwest * 2 + hang_t::nordost * 2 + hang_t::suedost * 2 + hang_t::nordwest * 2: {
							all_rotations_slope[slope] = transition_slope_texture->get_bild_ptr(14)->copy_rotate(0);
							all_rotations_beach[slope] = transition_water_texture->get_bild_ptr(11)->copy_rotate(0);
							break;
						}
					}
				}
				else {
					all_rotations_slope[slope] = NULL;
				}
				break;
			}
		}
	}

	// water images for water and overlay
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		int slope = double_grounds ? dslope : slopetable[dslope];
		final_tile = create_textured_tile( light_map->get_bild_ptr( slope ), boden_texture->get_bild_ptr( water_climate ) );
		ground_bild_list.append( final_tile );
	}
	image_offset = final_tile->get_nummer() - totalslopes + 2;
	DBG_MESSAGE("grund_besch_t::calc_water_level()","image_offset=%d", image_offset );

	// now the other transitions
	for(  int i=0;  i < number_of_climates;  i++  ) {
		// normal tile (no transition, not snow)
		climate_bild[i] = final_tile->get_nummer() + 1;
		for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
			int slope = double_grounds ? dslope : slopetable[dslope];
			final_tile = create_textured_tile( light_map->get_bild_ptr( slope ), boden_texture->get_bild_ptr( i+1 ) );
			ground_bild_list.append( final_tile );
		}
	}
	// finally full snow
	climate_bild[number_of_climates] = final_tile->get_nummer() + 1;
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		int slope = double_grounds ? dslope : slopetable[dslope];
		final_tile = create_textured_tile( light_map->get_bild_ptr( slope ), boden_texture->get_bild_ptr( arctic_climate ) );
		ground_bild_list.append( final_tile );
	}

	// alpha slopes for snowline
	for(  int dslope = 1;  dslope < totalslopes - 1;  dslope++  ) {
		int slope = double_grounds ? dslope : slopetable[dslope];
		final_tile = create_alpha_tile( light_map->get_bild_ptr( slope ), dslope, all_rotations_slope[dslope] );
		alpha_bild[dslope] = final_tile->get_nummer();
	}

	// alpha transitions for climates
	for(  int dslope = 0;  dslope < totalslopes - 1;  dslope++  ) {
		for(  int corners = 1;  corners < 16;  corners++  ) {
			int slope = double_grounds ? dslope : slopetable[dslope];
			uint8 double_corners = corners == 15 ? 80 : scorner1(corners) + 3 * scorner2(corners) + 9 * scorner3(corners) + 27 * scorner4(corners);
			final_tile = create_alpha_tile( light_map->get_bild_ptr( slope ), dslope, all_rotations_slope[double_corners] );
			alpha_corners_bild[dslope * 15 + corners - 1] = final_tile->get_nummer();

			double_corners = corners == 15 ? 80 : (1 - scorner1(corners)) + 3 * (1 - scorner2(corners)) + 9 * (1 - scorner3(corners)) + 27 * (1 - scorner4(corners));
			if(  all_rotations_beach[double_corners]  ) {
				final_tile = create_alpha_tile( light_map->get_bild_ptr( slope ), dslope, all_rotations_beach[double_corners] );
				alpha_water_bild[dslope * 15 + corners - 1] = final_tile->get_nummer();
			}
		}
	}

#if COLOUR_DEPTH != 0
	// free the helper bitmap
	for(  int slope = 1;  slope < totalslopes;  slope++  ) {
		delete all_rotations_slope[slope];
		if(  all_rotations_beach[slope]  ) {
			delete all_rotations_beach[slope];
		}
	}
#endif
	//dbg->message("grund_besch_t::calc_water_level()", "Last image nr %u", final_tile->get_pic()->bild_nr);
	printf("done\n");
}


/* returns a ground image for all ground tiles
 * current is the current climate
 * transition is true, if the next level is another climate
 * snow is true, if above slowline
 *
 * Since not all of the climates are used in their numerical order, we use a
 * private (static table "height_to_texture_climate" for lookup)
 */
image_id grund_besch_t::get_ground_tile(grund_t *gr)
{
	hang_t::typ slope = gr->get_disp_slope();
	sint16 height = gr->get_disp_height();
	koord k = gr->get_pos().get_2d();
	const sint16 tile_h = height - welt->get_water_hgt(k);
	if(  tile_h < 0  ||  (tile_h == 0  &&  slope == hang_t::flach)  ) {
		// deep water
		bildliste2d_besch_t const* const liste = sea->get_child<bildliste2d_besch_t>(2);
		int nr = min( -tile_h, liste->get_anzahl() - 2 );
		return liste->get_bild( nr, 0 )->get_nummer();
	}
	else {
		const bool snow = height >= welt->get_snowline();
		const sint16 climate_nr = snow ? number_of_climates : (welt->get_climate(k) > 1 ? welt->get_climate(k) - 1 : 0);
		// returns base climate for tile, transitions will be overlayed later
		return climate_bild[climate_nr] + (uint16)slope;
	}
	return IMG_LEER;
}


image_id grund_besch_t::get_water_tile(hang_t::typ slope)
{
	return climate_bild[0] - totalslopes + 1 + (uint16)slope;
}


image_id grund_besch_t::get_climate_tile(climate cl, hang_t::typ slope)
{
	return climate_bild[cl <= 0 ? 0 : cl - 1] + (uint16)slope;
}


image_id grund_besch_t::get_snow_tile(hang_t::typ slope)
{
	return climate_bild[number_of_climates] + (uint16)slope;
}


image_id grund_besch_t::get_beach_tile(hang_t::typ slope, uint8 corners)
{
	return alpha_water_bild[slope * 15 + corners - 1];
}


image_id grund_besch_t::get_alpha_tile(hang_t::typ slope, uint8 corners)
{
	return alpha_corners_bild[slope * 15 + corners - 1];
}


image_id grund_besch_t::get_alpha_tile(hang_t::typ slope)
{
	return alpha_bild[slope];
}
