/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * hansjoerg.malthaner@gmx.de
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

/*
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 * 3D, isometrische Darstellung
 *
 *
 * 18.11.97 lineare Speicherung fuer Images -> hoehere Performance
 * 22.03.00 run längen Speicherung fuer Images -> hoehere Performance
 * 15.08.00 dirty tile verwaltung fuer effizientere updates
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "simconst.h"
#include "simtypes.h"
#include "macros.h"
#include "font.h"
#include "pathes.h"
#include "simsys.h"
#include "simmem.h"
#include "simdebug.h"
#include "besch/bild_besch.h"
#include "unicode.h"


#ifdef _MSC_VER
#	include <io.h>
#	include <direct.h>
#	define W_OK 2
#else
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "simgraph.h"

// if you want to dither (ugly) define this
//#dfine DITHER


/*
 * Hajo: paletted
 */
typedef unsigned char PIXVAL;


#ifdef USE_SOFTPOINTER
static int softpointer = -1;
#endif
static int standard_pointer = -1;


#ifdef USE_SOFTPOINTER
/*
 * Die Icon Leiste muss neu gezeichnet werden wenn der Mauszeiger darueber
 * schwebt
 */
int old_my = -1;
#endif


/* Flag, if we have Unicode font => do unicode (UTF8) support! *
 * @author prissi
 * @date 29.11.04
 */
static bool has_unicode = false;

static font_type large_font;

// needed for gui
int large_font_height = 10;

#define CLEAR_COLOR (239)
#define DEFAULT_COLOR (239)

#define LIGHT_COUNT 15

// the players colors and colors for simple drawing operations
// each eight colors are corresponding to a player color
static const uint8 colortable_simutrans[224*3]=
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
	0, 0, 0,
	32, 32, 32,
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

static const PIXVAL colortable_system[3 * 16] = {
	0, 0, 0,
	128, 0, 0,
	0, 128, 0,
	128, 128, 0,
	0, 0, 128,
	128, 0, 128,
	0, 128, 128,
	192, 192, 192,
	128, 128, 128,
	255, 0, 0,
	0, 255, 0,
	255, 255, 0,
	0, 0, 255,
	255, 0, 255,
	0, 255, 255,
	255, 255, 255,
};

/*
 * Hajo: speical colors during daytime
 */
static const uint8 day_lights[LIGHT_COUNT*3] = {
	0x57,	0x65,	0x6F, // Dark windows, lit yellowish at night
	0x7F,	0x9B,	0xF1, // Lighter windows, lit blueish at night
	0xFF,	0xFF,	0x53, // Yellow light
	0xFF,	0x21,	0x1D, // Red light
	0x01,	0xDD,	0x01, // Green light
	0x6B,	0x6B,	0x6B, // Non-darkening grey 1 (menus)
	0x9B,	0x9B,	0x9B, // Non-darkening grey 2 (menus)
	0xB3,	0xB3,	0xB3, // non-darkening grey 3 (menus)
	0xC9,	0xC9,	0xC9, // Non-darkening grey 4 (menus)
	0xDF,	0xDF,	0xDF, // Non-darkening grey 5 (menus)
	0xE3,	0xE3,	0xFF, // Nearly white light at day, yellowish light at night
	0xC1,	0xB1,	0xD1, // Windows, lit yellow
	0x4D,	0x4D,	0x4D, // Windows, lit yellow
	0xE1,	0x00,	0xE1, // purple light for signals
	0x01, 0x01, 0xFF, // blue light
};


/*
 * Hajo: special colors during nighttime
 */
static const uint8 night_lights[LIGHT_COUNT*3] = {
	0xD3,	0xC3,	0x80, // Dark windows, lit yellowish at night
	0x80,	0xC3,	0xD3, // Lighter windows, lit blueish at night
	0xFF,	0xFF,	0x53, // Yellow light
	0xFF,	0x21,	0x1D, // Red light
	0x01,	0xDD,	0x01, // Green light
	0x6B,	0x6B,	0x6B, // Non-darkening grey 1 (menus)
	0x9B,	0x9B,	0x9B, // Non-darkening grey 2 (menus)
	0xB3,	0xB3,	0xB3, // non-darkening grey 3 (menus)
	0xC9,	0xC9,	0xC9, // Non-darkening grey 4 (menus)
	0xDF,	0xDF,	0xDF, // Non-darkening grey 5 (menus)
	0xFF,	0xFF,	0xE3, // Nearly white light at day, yellowish light at night
	0xD3,	0xC3,	0x80, // Windows, lit yellow
	0xD3,	0xC3,	0x80, // Windows, lit yellow
	0xE1,	0x00,	0xE1, // purple light for signals
	0x01, 0x01, 0xFF, // blue light
};


// offsets of first and second comany color
#define MAX_PLAYER_COUNT 16
static uint8 player_offsets[MAX_PLAYER_COUNT][2];

// the palette for the display ...
static PIXVAL textur_palette[256 * 3];

// the palette actually used for the display ...
static PIXVAL day_pal[256 * 3];

/*
 * Hajo: Image map descriptor struture
 */
struct imd {
	sint16 base_x; // min x offset
	sint16 base_y; // min y offset
	uint8 base_w; //  width
	uint8 base_h; // height

	sint16 x; // current (zoomed) min x offset
	sint16 y; // current (zoomed) min y offset
	uint8 w; // current (zoomed) width
	uint8 h; // current (zoomed) height

	uint16 len; // base image data size (used for allocation purposes only)

	uint8 recode_flags; // divers flags for recoding
	uint8 player_flags; // 128 = free/needs recode, otherwise coded to this color in player_data

	uint16 *src;	// original data

	PIXVAL* data; // current data, zoomed and adapted to output format RGB 555 or RGB 565

	PIXVAL* zoom_data; // zoomed original data

	PIXVAL* base_data; // original image data

	PIXVAL* player_data; // current data coded for player1 (since many building belong to him)
};

// flags for recoding
#define FLAG_ZOOMABLE (1)
#define FLAG_PLAYERCOLOR (2)
#define FLAG_REZOOM (4)
#define FLAG_NORMAL_RECODE (8)
#define FLAG_POSITION_CHANGED (16)

#define NEED_PLAYER_RECODE (128)



static int disp_width  = 640;
static int disp_height = 480;


/*
 * Image table
 */
static struct imd* images = NULL;

/*
 * Number of loaded images
 */
static uint16 anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static unsigned int alloc_images = 0;


/*
 * Output framebuffer
 */
static PIXVAL* textur = NULL;


/*
 * Hajo: Current clipping rectangle
 */
static struct clip_dimension clip_rect;


/*
 * Hajo: dirty tile management strcutures
 */
#define DIRTY_TILE_SIZE 16
#define DIRTY_TILE_SHIFT 4

static unsigned char *tile_dirty = NULL;
static unsigned char *tile_dirty_old = NULL;

static int tiles_per_line = 0;
static int tile_lines = 0;
static int tile_buffer_length = 0;

static int light_level = 0;

static int night_shift = -1;


static PIXVAL *conversion_table = NULL;
static PIXVAL darken_table[768];


/*
 * Hajo: tile raster width
 */
KOORD_VAL tile_raster_width = 16;
KOORD_VAL base_tile_raster_width = 16;

/*
 * Hajo: Zoom factor
 */
static int zoom_factor = 1;


/* changes the raster width after loading */
KOORD_VAL display_set_base_raster_width(KOORD_VAL new_raster)
{
	KOORD_VAL old = base_tile_raster_width;
	base_tile_raster_width = new_raster;
//	tile_raster_width = (new_raster *  zoom_num[zoom_factor]) / zoom_den[zoom_factor];
	tile_raster_width = new_raster / zoom_factor;
	return old;
}


/**
 * Rezooms all images
 * @author Hj. Malthaner
 */
static void rezoom(void)
{
	for(  uint16 n = 0;  n < anz_images;  n++  ) {
		if((images[n].recode_flags & FLAG_ZOOMABLE) != 0 && images[n].base_h > 0) {
			images[n].recode_flags |= FLAG_REZOOM;
		}
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;	// color will be set next time
	}
}


int get_zoom_factor()
{
	return zoom_factor;
}


void set_zoom_factor(int z)
{
	if (z > 0) {
		zoom_factor = z;
		tile_raster_width = base_tile_raster_width / z;
	}
	fprintf(stderr, "set_zoom_factor() : factor=%d\n", zoom_factor);

	rezoom();
}


int zoom_factor_up()
{
	// zoom out, if size permits
	if(  zoom_factor>1  ) {
		set_zoom_factor( zoom_factor-1 );
		return true;
	}
	return false;
}


int zoom_factor_down()
{
	if (zoom_factor<4) {
		set_zoom_factor( zoom_factor+1 );
		return true;
	}
	return false;
}


static inline void mark_tile_dirty(const int x, const int y)
{
	const int bit = x + y * tiles_per_line;

#if 0
	assert(bit / 8 < tile_buffer_length);
#endif

	tile_dirty[bit >> 3] |= 1 << (bit & 7);
}


static inline void mark_tiles_dirty(const int x1, const int x2, const int y)
{
	int bit = y * tiles_per_line + x1;
	const int end = bit + x2 - x1;

	do {
#if 0
		assert(bit / 8 < tile_buffer_length);
#endif

		tile_dirty[bit >> 3] |= 1 << (bit & 7);
	} while (++bit <= end);
}


static inline int is_tile_dirty(const int x, const int y)
{
	const int bit = x + y * tiles_per_line;
	const int bita = bit >> 3;
	const int bitb = 1 << (bit & 7);

	return (tile_dirty[bita] | tile_dirty_old[bita]) & bitb;
}


/**
 * Markiert ein Tile as schmutzig, ohne Clipping
 * @author Hj. Malthaner
 */
static void mark_rect_dirty_nc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2)
{
	// floor to tile size
	x1 >>= DIRTY_TILE_SHIFT;
	y1 >>= DIRTY_TILE_SHIFT;
	x2 >>= DIRTY_TILE_SHIFT;
	y2 >>= DIRTY_TILE_SHIFT;

#if 0
	assert(x1 >= 0);
	assert(x1 < tiles_per_line);
	assert(y1 >= 0);
	assert(y1 < tile_lines);
	assert(x2 >= 0);
	assert(x2 < tiles_per_line);
	assert(y2 >= 0);
	assert(y2 < tile_lines);
#endif

	for (; y1 <= y2; y1++) mark_tiles_dirty(x1, x2, y1);
}


/**
 * Markiert ein Tile as schmutzig, mit Clipping
 * @author Hj. Malthaner
 */
void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2)
{
	// inside display?
	if (x2 >= 0 && y2 >= 0 && x1 < disp_width && y1 < disp_height) {
		if (x1 < 0) x1 = 0;
		if (y1 < 0) y1 = 0;
		if (x2 >= disp_width)  x2 = disp_width  - 1;
		if (y2 >= disp_height) y2 = disp_height - 1;
		mark_rect_dirty_nc(x1, y1, x2, y2);
	}
}


static void init_16_to_8_conversion(void)
{
	int red;
	int green;
	int blue;

	printf("Building 16Bit to 8Bit conversion table ... ");

	if (day_pal[0] == 0) {
		// not yet init?
		memcpy(day_pal, colortable_simutrans, sizeof(colortable_simutrans));
	}

	conversion_table = MALLOCN(PIXVAL, 32768 + 16 + LIGHT_COUNT);
	for (red = 0; red < 256; red += 8) {
		for (green = 0; green < 256; green += 8) {
			for (blue = 0; blue < 256; blue += 8) {
				const int index = red << 7 | green << 2 | blue >> 3;

				PIXVAL best_match = DEFAULT_COLOR;
				long distance = 0x00FFFFFF;
				int i;
				for (i = 0; i < 224; i++) {
					long new_dist =
						abs(red   - colortable_simutrans[i * 3 + 0]) +
						abs(green - colortable_simutrans[i * 3 + 1]) +
						abs(blue  - colortable_simutrans[i * 3 + 2]);
					if (new_dist < distance) {
						distance = new_dist;
						best_match = i;
					}
				}
				conversion_table[index] = best_match;
			}
		}
	}
	// now the first and second player color
	int index;
	for (index =  0; index <  16; index++) {
		// player color
		conversion_table[index + 32768] = 240+index;
	}
	for (index = 0; index < LIGHT_COUNT; index++) {
		// non-darkening color
		conversion_table[index + 32768 + 16] = 224+index;
	}

	// init table for blending
	static uint8 overflow_index[28]=
	{
		16, 210,
		97, 194,
		112, 160,
		120, 105,
		176, 88,
		192, 176,
		88, 192,
		128, 160,
		176, 41,
		96, 66,
		176, 26,
		208, 176,
		208, 41,
		208, 209
	};
	// now darken table
	for(uint8 index = 0; index < 224; index++) {
		uint8 new_index = (index&7)==0 ? overflow_index[index/8] : index -1;	// 25% dark
		darken_table[index] = new_index;
		new_index = (new_index&7)==0 ? overflow_index[new_index/8] : new_index -1;
		darken_table[256+index] = new_index; // 50% dark
		new_index = (new_index&7)==0 ? overflow_index[new_index/8] : new_index -1;
		darken_table[512+index] = new_index; // 75% dark
	}
	for(uint16 index = 224; index < 256; index++) {
		darken_table[index] = darken_table[256+index] = darken_table[512+index] = index; // keep unchanged
	}

	printf("colortables ok.\n");
}


/**
 * Convert a certain image data to actual output data for a certain player
 * @author prissi
 */
static bool recode_img_src16_target8(int h, PIXVAL *src16, PIXVAL *target, bool darkens)
{
	bool has_player_color = false;

	if (conversion_table == NULL) {
		init_16_to_8_conversion();
	}

	unsigned short *src=(unsigned short *)src16;
#ifdef DITHER
	sint16 last_errors[3] = {0, 0, 0};
#endif
	if (h > 0) {
		do {
			unsigned char runlen = *target ++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok
				runlen = *target++ = *src++;
				// now just convert the color pixels
				if (darkens) {
					while (runlen--) {
						uint16 pix = *src++;
						if(pix>=0x8000) {
							has_player_color = pix<0x8010;
							*target++ = conversion_table[pix];
						}
						else {
#ifdef DITHER
							sint16 r = (pix>>10)&0x1F;
							sint16 g = (pix>>5)&0x1F;
							sint16 b = pix&0x1F;
							last_errors[0] = clamp( last_errors[0] + r, 0, 31 );
							last_errors[1] = clamp( last_errors[1] + g, 0, 31 );
							last_errors[2] = clamp( last_errors[2] + b, 0, 31 );
							pix = (last_errors[0]<<10) | (last_errors[1]<<5) | last_errors[2];
							*target = conversion_table[pix];
							last_errors[0] = r-(sint16)(colortable_simutrans[*target*3+0]>>3);
							last_errors[1] = g-(sint16)(colortable_simutrans[*target*3+1]>>3);
							last_errors[2] = b-(sint16)(colortable_simutrans[*target*3+2]>>3);
							target ++;
#else
							*target++ = conversion_table[pix];
#endif
						}
					}
				}
				else {
					// find best match by foot (slowly, but not often used)
					// this avoids darkening
					while (runlen-- != 0) {
						unsigned short pix = *src++;
						long red   = (pix >> 7) & 0x00F8;
						long green = (pix >> 2) & 0x00F8;
						long blue  = (pix << 3) & 0x00F8;
						if (pix > 0x8000) {
							if (pix < 0x8010) {
								*target++ = pix-0x8000;
								has_player_color = true;
								continue;
							}
							if (pix == 0x8010 || pix == 0x8011 || (pix >= 0x801A && pix < 0x801C)) {
								pix   = conversion_table[pix];
								red   = day_pal[pix * 3 + 0];
								green = day_pal[pix * 3 + 1];
								blue  = day_pal[pix * 3 + 2];
							}
							else {
								*target++ = (pix & 0x00FF) + 16;
								continue;
							}
						}

						PIXVAL best_match = DEFAULT_COLOR;
						long distance = red * red + green * green + blue * blue;
						int i;
						for(i = 0; i < 239; i++) {
							long new_dist =
								(red   - textur_palette[i * 3 + 0]) * (red   - textur_palette[i * 3 + 0]) +
								(green - textur_palette[i * 3 + 1]) * (green - textur_palette[i * 3 + 1]) +
								(blue  - textur_palette[i * 3 + 2]) * (blue  - textur_palette[i * 3 + 2]);

							if (new_dist < distance) {
								distance = new_dist;
								best_match = i;
							}
						}
						*target++ = best_match;
					}
				}
				// next clea run or zero = end
			} while ((runlen = *target ++ = *src++));
		} while (--h != 0);
	}
	return has_player_color;
}



/**
 * Convert base image data to actual image size
 * @author prissi (to make this much faster) ...
 */
static void rezoom_img(const unsigned int n)
{
	// Hajo: may this image be zoomed
	if (n < anz_images && images[n].base_h > 0) {
		// we may need night conversion afterwards
		images[n].recode_flags &= ~FLAG_REZOOM;
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;

		// just restore original size?
		if (zoom_factor <= 1) {
			// this we can do be a simple copy ...
			images[n].x = images[n].base_x;
			images[n].w = images[n].base_w;
			images[n].y = images[n].base_y;
			images[n].h = images[n].base_h;
			if (images[n].zoom_data != NULL) {
				guarded_free(images[n].zoom_data);
				images[n].zoom_data = NULL;
			}
			return;
		}

		// now we want to downsize the image
		// just divede the sizes
		images[n].x = images[n].base_x / zoom_factor;
		images[n].y = images[n].base_y / zoom_factor;
		images[n].w = (images[n].base_x + images[n].base_w) / zoom_factor - images[n].x;
		images[n].h = images[n].base_h / zoom_factor;
#if 0
		images[n].h = (images[n].base_y % zoom_factor + images[n].base_h) / zoom_factor;
#endif

		if (images[n].h > 0 && images[n].w > 0) {
			// just recalculate the image in the new size
			sint16 y_left = (images[n].base_y + zoom_factor - 1) % zoom_factor;
			uint8 h = images[n].base_h;

			static PIXVAL line[512];
			PIXVAL *src = images[n].base_data;
			PIXVAL *dest, *last_dest;

			// decode/recode linewise
			int last_color = 255; // ==255 to keep compiler happy

			if (images[n].zoom_data == NULL) {
				// normal len is ok, since we are only skipping parts ...
				images[n].zoom_data = MALLOCN(PIXVAL, images[n].len);
			}
			last_dest = dest = images[n].zoom_data;

			do { // decode/recode line
				unsigned int runlen;
				int color = 0;
				PIXVAL *p = line;
				const int imgw = images[n].base_w;

				// left offset, which was left by division
				runlen = images[n].base_x%zoom_factor;
				while (runlen--) {
					*p++ = CLEAR_COLOR;
				}

				// decode line
				runlen = *src++;
				color -= runlen;
				do {
					// clear run
					while (runlen--) {
						*p++ = CLEAR_COLOR;
					}
					// color pixel
					runlen = *src++;
					color += runlen;
					while (runlen--) {
						*p++ = *src++;
					}
				} while ((runlen = *src++));

				if (y_left == 0 || last_color < color) {
					// required; but if the following are longer, take them instead (aviods empty pixels)
					// so we have to set/save the beginning
					unsigned char step = 0;
					unsigned char i;

					if (y_left == 0) {
						last_dest = dest;
					} else {
						dest = last_dest;
					}

					// encode this line
					do {
						// check length of transparent pixels
						for (i = 0;  line[step] == CLEAR_COLOR  &&  step < imgw;  i++, step += zoom_factor  ) {}
						*dest++ = i;
						// chopy for non-transparent
						for (i = 0; line[step] != CLEAR_COLOR  &&  step < imgw;  i++, step += zoom_factor) {
							dest[i + 1] = line[step];
						}
						*dest++ = i;
						dest += i;
					} while (step < imgw);
					*dest++ = 0; // mark line end
					if (y_left == 0) {
						// ok now reset countdown counter
						y_left = zoom_factor;
					}
					last_color = color;
				}

				// countdown line skip counter
				y_left--;
			} while (--h != 0);

			// something left?
			{
				const unsigned int zoom_len = dest - images[n].zoom_data;
				if (zoom_len > images[n].len) {
					printf("*** FATAL ***\nzoom_len (%i) > image_len (%i)", zoom_len, images[n].len);
					fflush(NULL);
					exit(0);
				}
				if (zoom_len == 0) images[n].h = 0;
			}
		} else {
			if (images[n].w == 0) {
				// h=0 will be ignored, with w=0 there was an error!
				printf("WARNING: image%d w=0!\n", n);
			}
			images[n].h = 0;
		}
	}
}


int display_set_unicode(int use_unicode)
{
	return has_unicode = (use_unicode != 0);
}


bool display_load_font(const char* fname)
{
	if (load_font(&large_font, fname)) {
		large_font_height = large_font.height;
		return true;
	} else {
		return false;
	}
}


KOORD_VAL display_get_width(void)
{
	return disp_width;
}


KOORD_VAL display_get_height(void)
{
	return disp_height;
}


sint16 display_set_height(KOORD_VAL h)
{
	sint16 old = disp_height;

	disp_height = h;
	return old;
}


/**
 * Holt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
int display_get_light(void)
{
	return light_level;
}


/* Tomas variant */
static void calc_base_pal_from_night_shift(const int night)
{
	const int night2 = min(night, 4);
	const int day = 4 - night2;
	unsigned int i;

	// constant multiplier 0,66 - dark night  255 will drop to 49, 55 to 10
	//                     0,7  - dark, but all is visible     61        13
	//                     0,73                                72        15
	//                     0,75 - quite bright                 80        17
	//                     0,8    bright                      104        22

	const double RG_nihgt_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_nihgt_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);

	for (i = 0; i<224; i++) {
		// (1<<15) this is total no of all possible colors in RGB555)

		// RGB 555 input
		int R = colortable_simutrans[i * 3 + 0];
		int G = colortable_simutrans[i * 3 + 1];
		int B = colortable_simutrans[i * 3 + 2];

		// lines generate all possible colors in 555RGB code - input
		// however the result is in 888RGB - 8bit per channel
		R = (int)(R * RG_nihgt_multiplier);
		G = (int)(G * RG_nihgt_multiplier);
		B = (int)(B * B_nihgt_multiplier);

		textur_palette[i * 3 + 0] = R;
		textur_palette[i * 3 + 1] = G;
		textur_palette[i * 3 + 2] = B;
	}

	// Lights
	for (i = 0; i < LIGHT_COUNT; i++) {
		const int day_R =  day_lights[i*3];
		const int day_B =  day_lights[i*3+1];
		const int day_G = day_lights[i*3+2];

		const int night_R =  night_lights[i*3];
		const int night_G =  night_lights[i*3+1];
		const int night_B =  night_lights[i*3+2];

		const int R = (day_R * day + night_R * night2) >> 2;
		const int G = (day_G * day + night_G * night2) >> 2;
		const int B = (day_B * day + night_B * night2) >> 2;

		textur_palette[i * 3 + 224 * 3 + 0] = R>0 ? R : 0;
		textur_palette[i * 3 + 224 * 3 + 1] = G>0 ? R : 0;
		textur_palette[i * 3 + 224 * 3 + 2] = B>0 ? R : 0;
	}
	// empty
	for (i = 224+LIGHT_COUNT; i < 240; i++) {
		textur_palette[i * 3 + 0] = 0;
		textur_palette[i * 3 + 1] = 255;
		textur_palette[i * 3 + 2] = 255;
	}
	// system color
	for (i = 0; i < 16; i++) {
		textur_palette[i * 3 + 240*3 + 0] = colortable_system[i * 3 + 0];
		textur_palette[i * 3 + 240*3 + 1] = colortable_system[i * 3 + 1];
		textur_palette[i * 3 + 240*3 + 2] = colortable_system[i * 3 + 2];
	}

	dr_setRGB8multi(0, 256, textur_palette);
}


/**
 * Setzt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
void display_set_light(int new_light_level)
{
	light_level = new_light_level;
	calc_base_pal_from_night_shift(night_shift);
}


void display_day_night_shift(int night)
{
	if (night != night_shift) {
		night_shift = night;
		calc_base_pal_from_night_shift(night);
		mark_rect_dirty_nc(0, 0, disp_width - 1, disp_height - 1);
		for(  uint16 n = 0;  n < anz_images;  n++  ) {
			images[n].recode_flags |= FLAG_NORMAL_RECODE;
		}

	}
}


// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 )
{
	if(player_offsets[player][0]!=col1  ||  player_offsets[player][1]!=col2) {
		// set new player colors
		player_offsets[player][0] = col1;
		player_offsets[player][1] = col2;
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
	}
}



/**
 * Fügt ein Image aus anderer Quelle hinzu
 */
void register_image(struct bild_t* bild)
{
	struct imd* image;

	if (anz_images >= 65535) {
		fprintf(stderr, "FATAL:\n*** Out of images (more than 65534!) ***\n");
		abort();
	}

	if (anz_images == alloc_images) {
		alloc_images += 128;
		images = REALLOC(images, imd, alloc_images);
	}

	bild->bild_nr = anz_images;
	image = &images[anz_images];
	anz_images++;

	image->x = bild->x;
	image->w = bild->w;
	image->y = bild->y;
	image->h = bild->h;

	image->base_x = bild->x;
	image->base_w = bild->w;
	image->base_y = bild->y;
	image->base_h = bild->h;

	image->len = bild->len;
	if (image->len == 0) {
		image->len = 1;
		image->base_h = 0;
		image->h = 0;
	}

	// allocate and copy if needed
	image->recode_flags = FLAG_NORMAL_RECODE | FLAG_REZOOM | (bild->zoomable & FLAG_ZOOMABLE);
	image->player_flags = NEED_PLAYER_RECODE;

	image->base_data = NULL;
	image->zoom_data = NULL;
	image->data = NULL;
	image->player_data = NULL;	// chaches data for one AI

	image->base_data = MALLOCN(PIXVAL, image->len);
	// currently, makeobj has not yet an option to pak 8 Bit images ....
	if ((bild->zoomable & 0xFE) == 0) {
		// this is an 16 Bit image => we need to resize it to 8 Bit ...
		image->recode_flags |= FLAG_PLAYERCOLOR*recode_img_src16_target8(image->base_h, (PIXVAL*)(bild + 1), image->base_data, bild->zoomable & 1);
		image->src = (uint16*)(bild + 1);
	}
	else {
		memcpy(image->base_data, (PIXVAL*)(bild + 1), image->len * sizeof(PIXVAL));
		image->src = NULL;
	}

}


// prissi: query offsets
void display_get_image_offset(unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw)
{
	if (bild < anz_images) {
		*xoff = images[bild].x;
		*yoff = images[bild].y;
		*xw   = images[bild].w;
		*yw   = images[bild].h;
	}
}


// prissi: query unzoiomed offsets
void display_get_base_image_offset(unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw)
{
	if (bild < anz_images) {
		*xoff = images[bild].base_x;
		*yoff = images[bild].base_y;
		*xw   = images[bild].base_w;
		*yw   = images[bild].base_h;
	}
}


// prissi: canges the offset of an image
// we need it this complex, because the actual x-offset is coded into the image
void display_set_base_image_offset(unsigned bild, KOORD_VAL xoff, KOORD_VAL yoff)
{
	if(bild >= anz_images) {
		fprintf(stderr, "Warning: display_set_base_image_offset(): illegal image=%d\n", bild);
		return;
	}

	// only move images once
	if(  images[bild].recode_flags & FLAG_POSITION_CHANGED  ) {
		fprintf(stderr, "Warning: display_set_base_image_offset(): image=%d was already moved!\n", bild);
		return;
	}
	images[bild].recode_flags |= FLAG_POSITION_CHANGED;

	assert(images[bild].base_h > 0);
	assert(images[bild].base_w > 0);

	// avoid overflow
	images[bild].base_x += xoff;
	images[bild].base_y += yoff;
}


/**
 * Holt Maus X-Position
 * @author Hj. Malthaner
 */
int get_maus_x(void)
{
	return sys_event.mx;
}


/**
 * Holt Maus y-Position
 * @author Hj. Malthaner
 */
int get_maus_y(void)
{
	return sys_event.my;
}


/**
 * this sets width < 0 if the range is out of bounds
 * so check the value after calling and before displaying
 * @author Hj. Malthaner
 */
static int clip_wh(KOORD_VAL *x, KOORD_VAL *width, const KOORD_VAL min_width, const KOORD_VAL max_width)
{
	if (*x < min_width) {
		const KOORD_VAL xoff = min_width - *x;

		*width += *x-min_width;
		*x = min_width;

		if (*x + *width >= max_width) *width = max_width - *x;

		return xoff;
	} else if (*x + *width >= max_width) {
		*width = max_width - *x;
	}
	return 0;
}


/**
 * places x and w within bounds left and right
 * if nothing to show, returns FALSE
 * @author Niels Roest
 */
static int clip_lr(KOORD_VAL *x, KOORD_VAL *w, const KOORD_VAL left, const KOORD_VAL right)
{
	const KOORD_VAL l = *x;          // leftmost pixel
	const KOORD_VAL r = *x + *w - 1; // rightmost pixel

	if (l > right || r < left) {
		*w = 0;
		return FALSE;
	}

	// there is something to show.
	if (l < left) {
		*w -= left - l;
		*x = left;
	}
	if (r > right) {
		*w -= r - right;
	}
	return TRUE;
}


/**
 * Ermittelt Clipping Rechteck
 * @author Hj. Malthaner
 */
struct clip_dimension display_get_clip_wh(void)
{
	return clip_rect;
}


/**
 * Setzt Clipping Rechteck
 * @author Hj. Malthaner
 */
void display_set_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h)
{
#if 0
	clip_wh(&x, &w, 0, disp_width);
#endif
	clip_wh(&x, &w, 0, disp_width);
	clip_wh(&y, &h, 0, disp_height);

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.w = w;
	clip_rect.h = h;

	clip_rect.xx = x + w - 1;
	clip_rect.yy = y + h - 1;
}



// ----------------- basic painting procedures ----------------


// scrolls horizontally, will ignore clipping etc.
void display_scroll_band(const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h)
{
	const PIXVAL* src = textur + start_y * disp_width + x_offset;
	PIXVAL* dst = textur + start_y * disp_width;
	size_t amount = sizeof(PIXVAL) * (h * disp_width - x_offset);

#ifdef USE_C
	memmove(dst, src, amount);
#else
	amount /= 4;
	asm volatile (
		"rep\n\t"
		"movsl\n\t"
		: "+D" (dst), "+S" (src), "+c" (amount)
		: "memory"
	);
#endif
}


/************ display all kind of images from here on ********/


/**
 * Kopiert Pixel von src nach dest
 * @author Hj. Malthaner
 */
static inline void pixcopy(PIXVAL *dest, const PIXVAL *src, const unsigned int len)
{
	// for gcc this seems to produce the optimal code ...
	const PIXVAL *const end = dest + len;

	while (dest < end) *dest++ = *src++;
}


/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static inline void colorpixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end, const int color)
{
	while (src < end) {
		PIXVAL pix = *src++;

		*dest++ = (pix < 4) ? pix + color : pix;
	}
}


/**
 * Zeichnet Bild mit Clipping
 * @author Hj. Malthaner
 */
static void display_img_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			int runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos + 1 >= runlen ? runlen : clip_rect.xx - xpos + 1);

					pixcopy(tp + xpos + left, sp + left, len - left);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild ohne Clipping
 */
static void display_img_nc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + xp + yp * disp_width;

		do { // zeilen dekodieren
			unsigned int runlen = *sp++;
			PIXVAL *p = tp;

			// eine Zeile dekodieren
			do {
				// wir starten mit einem clear run
				p += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;
				{
					unsigned short* ls;
					unsigned short* ld;

					if (runlen & 1) *p++ = *sp++;

					ls = (unsigned short*)sp;
					ld = (unsigned short*)p;
					runlen >>= 1;
					while (runlen--) *ld++ = *ls++;
					p = (PIXVAL*)ld;
					sp = (PIXVAL*)ls;
				}
				runlen = *sp++;
			} while (runlen != 0);

			tp += disp_width;
		} while (--h != 0);
	}
}


/**
 * Zeichnet Bild mit verticalem clipping (schnell) und horizontalem (langsam)
 * @author prissi
 */
void display_img_aux(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool player)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		KOORD_VAL h, reduce_h, skip_lines;

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
		}

		const PIXVAL* sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;

		// now, since zooming may have change this image
		yp += images[n].y;
		h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		reduce_h = yp + h - clip_rect.yy;
		if (reduce_h > 0) {
			h -= reduce_h;
		}
		// still something to draw
		if (h <= 0) {
			return;
		}

		// vertically lines to skip (only bottom is visible
		skip_lines = clip_rect.y - (int)yp;
		if (skip_lines > 0) {
			if (skip_lines >= h) {
				// not visible at all
				return;
			}
			h -= skip_lines;
			yp += skip_lines;
			// now skip them
			while (skip_lines--) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += *sp + 1;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const KOORD_VAL w = images[n].w;
			xp += images[n].x;

			// use horzontal clipping or skip it?
			if (xp >= clip_rect.x  &&  xp-images[n].x + w - 1 <= clip_rect.xx) {
				// marking change?
				if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
				display_img_nc(h, xp, yp, sp);
			} else if (xp <= clip_rect.xx  &&  xp + w > clip_rect.x) {
				display_img_wc(h, xp, yp, sp);
				// since height may be reduced, start marking here
				if (dirty) mark_rect_dirty_wc(xp, yp, xp + w - 1, yp + h - 1);
			}
		}
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * assumes height is ok and valid data are caluclated
 * @author hajo/prissi
 */
static void display_color_img_aux(const int n, KOORD_VAL xp, const KOORD_VAL yp, const uint8 player)
{
	KOORD_VAL h = images[n].h;
	KOORD_VAL y = yp + images[n].y;
	KOORD_VAL yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	static uint8 conversion[256] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
	};
	static uint8 last_color1 = 255, last_color2 = 255;

	if (h > 0) { // clipping may have reduced it
		// color replacement needs the original data
		const PIXVAL *sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;
		PIXVAL *tp = textur + y * disp_width;

		if(  last_color1!=player_offsets[player][0]  ||  last_color2!=player_offsets[player][1]  ) {
			// need to activate player color
			for( int i=0;  i<8;  i++  ) {
				conversion[240+i] = player_offsets[player][0]+i; // player color 1
				conversion[248+i] = player_offsets[player][1]+i; // player color 2
			}
			last_color1 = player_offsets[player][0]; // player color 1
			last_color2 = player_offsets[player][1]; // player color 1
		}

		// oben clippen
		while (yoff) {
			yoff--;
			do {
				// clear run + colored run + next clear run
				++sp;
				sp += *sp + 1;
			} while (*sp);
			sp++;
		}

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen

			int runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos + 1 >= runlen ? runlen : clip_rect.xx - xpos + 1);

					{
						PIXVAL *dest = tp+xpos+left;
						const PIXVAL *src = sp+left;
						const PIXVAL * const end = sp + len;
						while (src < end) {
							*dest++ = conversion[*src++];
						}
					}
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
void display_color_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const sint8 color, const int daynight, const int dirty)
{
	if (n < anz_images) {
		// Hajo: since the colors for player 0 are already right,
		// only use the expensive replacement routine for colored images
		// of other players
		// Hajo: player 1 does not need any recoloring, too

		if(  images[n].recode_flags&FLAG_NORMAL_RECODE  ) {
			recode_img_src16_target8(images[n].base_h, (PIXVAL*)images[n].src, images[n].base_data, (images[n].recode_flags&FLAG_ZOOMABLE)!=0 );
			images[n].recode_flags &= ~FLAG_NORMAL_RECODE;
		}

		if (daynight  &&  (images[n].recode_flags & FLAG_PLAYERCOLOR) == 0) {
			display_img_aux(n, xp, yp, dirty, false);
			return;
		}

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
		}

		// prissi: now test if visible and clipping needed
		{
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL y = images[n].y;
			const KOORD_VAL w = images[n].w;
			const KOORD_VAL h = images[n].h;

			if (h == 0 || xp > clip_rect.xx || yp + y > clip_rect.yy || xp + x + w <= clip_rect.x || yp + y + h <= clip_rect.y) {
				// not visible => we are done
				// happens quite often ...
				return;
			}

			if (dirty) {
				mark_rect_dirty_wc(xp + x, yp + y, xp + x + w - 1, yp + y + h - 1);
			}

			display_color_img_aux( n, xp+x, yp, color );
		}
	} // number ok
}


/**
 * draw unscaled images, replaces base color
 * @author prissi
 */
void display_base_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const sint8 player_nr, const int daynight, const int dirty)
{
	display_color_img( n, xp, yp, player_nr, daynight, dirty );
/*
	if(  base_tile_raster_width==tile_raster_width  ) {
		// same size => use standard routine
		display_color_img( n, xp, yp, player_nr, daynight, dirty );
	}
	else if (n < anz_images) {

		// prissi: now test if visible and clipping needed
		const KOORD_VAL x = images[n].base_x;
		const KOORD_VAL y = images[n].base_y;
		const KOORD_VAL w = images[n].base_w;
		const KOORD_VAL h = images[n].base_h;

		if (h == 0 || xp > clip_rect.xx || yp + y > clip_rect.yy || xp + x + w <= clip_rect.x || yp + y + h <= clip_rect.y) {
			// not visible => we are done
			// happens quite often ...
			return;
		}

		if (dirty) {
			mark_rect_dirty_wc(xp + x, yp + y, xp + x + w - 1, yp + y + h - 1);
		}

		// colors for 2nd company color
		if(player_nr>=0) {
			activate_player_color( player_nr, daynight );
		}
		else {
			// no player
			activate_player_color( 0, daynight );
		}
		display_color_img_aux( images[n].base_data, xp+x, yp+y, h );
	} // number ok
*/
}

static void display_img_blend_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp, uint8 factor )
{
	const uint16 offset = 256*factor;
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			PIXVAL runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const KOORD_VAL left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const KOORD_VAL len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);
					{
						PIXVAL *dest = tp + xpos + left;
						const PIXVAL *const end = dest + len;
						while (dest < end) {
							*dest = darken_table[offset+*dest];
							dest++;
						}
					}
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}

/**
 * draws the transparent outline of an image
 * @author kierongreen
 */
void display_rezoomed_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, int, const PLAYER_COLOR_VAL color_index, const int /*daynight*/, const int dirty)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		KOORD_VAL h, reduce_h, skip_lines;

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
		}
		PIXVAL *sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;

		// now, since zooming may have change this image
		xp += images[n].x;
		yp += images[n].y;
		h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		reduce_h = yp + h - clip_rect.yy;
		if (reduce_h > 0) h -= reduce_h;
		// still something to draw
		if (h <= 0) return;

		// vertically lines to skip (only bottom is visible
		skip_lines = clip_rect.y - (int)yp;
		if (skip_lines > 0) {
			if (skip_lines >= h) {
				// not visible at all
				return;
			}
			h -= skip_lines;
			yp += skip_lines;
			// now skip them
			while (skip_lines--) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += *sp + 1;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const KOORD_VAL w = images[n].w;
			// we use function pointer for the blend runs for the moment ...
			uint8 blend_factor = (color_index&OUTLINE_FLAG) ? (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 : (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1;

			// use horzontal clipping or skip it?
			if (xp >= clip_rect.x && xp + w - 1 <= clip_rect.xx) {
				// marking change?
				if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
				display_img_blend_wc( h, xp, yp, sp, blend_factor );
			} else if (xp <= clip_rect.xx && xp + w > clip_rect.x) {
				display_img_blend_wc( h, xp, yp, sp, blend_factor );
				// since height may be reduced, start marking here
				if (dirty) mark_rect_dirty_wc(xp, yp, xp + w - 1, yp + h - 1);
			}
		}
	}
}



/**
 * the area of this image need update
 * @author Hj. Malthaner
 */
void display_mark_img_dirty(unsigned bild, KOORD_VAL xp, KOORD_VAL yp)
{
	if (bild < anz_images) {
		mark_rect_dirty_wc(
			xp + images[bild].x,
			yp + images[bild].y,
			xp + images[bild].x + images[bild].w - 1,
			yp + images[bild].y + images[bild].h - 1
		);
	}
}


/**
 * Zeichnet ein Pixel
 * @author Hj. Malthaner
 */
static void display_pixel(KOORD_VAL x, KOORD_VAL y, int color)
{
	if (x >= clip_rect.x && x <= clip_rect.xx && y >= clip_rect.y && y <= clip_rect.yy) {
		PIXVAL* const p = textur + x + y * disp_width;

		*p = color;
		mark_tile_dirty(x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT);
	}
}


/**
 * Zeichnet gefuelltes Rechteck
 */
static void display_fb_internal(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, int color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	clip_lr(&xp, &w, cL, cR);
	clip_lr(&yp, &h, cT, cB - 1);

	if (w > 0 && h > 0) {
		const PIXVAL colval = color&0xFF;
		PIXVAL *p = textur + xp + yp*disp_width;
		const unsigned long longcolval = (colval << 8) | colval;
		const int dx = disp_width - w;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		do {
			unsigned int count = w;
			unsigned short* lp;

			if (count & 1) {
				*p++ = longcolval;
			}
			count >>= 1;
			lp = (unsigned short *)p;
			while (count-- != 0) {
				*lp++ = longcolval;
			}
			p = (PIXVAL *)lp;

			p += dx;
		} while (--h != 0);
	}
}


void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, 0, disp_width - 1, 0, disp_height);
}


void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet vertikale Linie
 * @author Hj. Malthaner
 */
static void display_vl_internal(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	clip_lr(&yp, &h, cT, cB - 1);

	if (xp >= cL && xp <= cR && h > 0) {
		PIXVAL *p = textur + xp + yp*disp_width;
		const PIXVAL colval = color&0xFF;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp, yp + h - 1);

		do {
			*p = colval;
			p += disp_width;
		} while (--h != 0);
	}
}


void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, 0, disp_width - 1, 0, disp_height - 1);
}


void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet rohe Pixeldaten
 */
void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr)
{
	const int arr_w = w;
	const KOORD_VAL xoff = clip_wh(&xp, &w, clip_rect.x, clip_rect.xx);
	const KOORD_VAL yoff = clip_wh(&yp, &h, clip_rect.y, clip_rect.yy);

	if (w > 0 && h > 0) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const unsigned char *arr_src = arr;

		mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		if (xp == clip_rect.x) arr_src += xoff;
		if (yp == clip_rect.y) arr_src += yoff * arr_w;

		do {
			unsigned int ww = w;

			do {
				*p++ = *arr_src++;
				ww--;
			} while (ww > 0);
			arr_src += arr_w - w;
			p += disp_width - w;
		} while (--h != 0);
	}
}


// unicode save moving in strings
long get_prev_char(const char* text, long pos)
{
	if (has_unicode) {
		return utf8_get_next_char((const utf8*)text, pos);
	} else {
		return pos + 1;
	}
}


size_t get_next_char(const char* text, size_t pos)
{
	if (pos <= 0) return 0;
	if (has_unicode) {
		return utf8_get_prev_char((const utf8*)text, pos);
	} else {
		return pos - 1;
	}
}


KOORD_VAL display_get_char_width(utf16 c)
{
	KOORD_VAL w = large_font.screen_width[c];
	if (w == 0) w = large_font.screen_width[0];
	return w;
}


/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 * @author Knightly
 */
unsigned short get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width)
{
	unsigned short char_code;
	if(  has_unicode  ) {
		size_t len = 0;
		char_code = utf8_to_utf16((const utf8 *)text, &len);
		if(  char_code==0  ) {
			// case : end of text reached -> do not advance text pointer
			byte_length = 0;
			pixel_width = 0;
		}
		else {
			text += len;
			byte_length = len;
			if(  char_code>=large_font.num_chars  ||  (pixel_width = large_font.screen_width[char_code])==0  ) {
				// default width for missing characters
				pixel_width = large_font.screen_width[0];
			}
		}
	}
	else {
		char_code = *text;
		if(  char_code==0  ) {
			// case : end of text reached -> do not advance text pointer
			byte_length = 0;
			pixel_width = 0;
		}
		else {
			++text;
			byte_length = 1;
			pixel_width = large_font.screen_width[char_code];
		}
	}
	return char_code;
}


/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 * @author Knightly
 */
unsigned short get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width)
{
	if(  text<=text_start  ) {
		// case : start of text reached or passed -> do not move the pointer backwards
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}

	unsigned short char_code;
	if(  has_unicode  ) {
		// determine the start of the previous logical character
		do {
			--text;
		} while (  text>text_start  &&  (*text & 0xC0)==0x80  );

		size_t len = 0;
		char_code = utf8_to_utf16((const utf8 *)text, &len);
		byte_length = len;
		if(  char_code>=large_font.num_chars  ||  (pixel_width = large_font.screen_width[char_code])==0  ) {
			// default width for missing characters
			pixel_width = large_font.screen_width[0];
		}
	}
	else {
		--text;
		char_code = *text;
		byte_length = 1;
		pixel_width = large_font.screen_width[char_code];
	}
	return char_code;
}


/* proportional_string_width with a text of a given length
 * extended for universal font routines with unicode support
 * @author Volker Meyer
 * @date  15.06.2003
 * @author prissi
 * @date 29.11.04
 */
int display_calc_proportional_string_len_width(const char* text, size_t len)
{
	const font_type* const fnt = &large_font;
	unsigned int c, width = 0;
	int w;

#ifdef UNICODE_SUPPORT
	if (has_unicode) {
		utf16 iUnicode;
		size_t	iLen = 0;

		// decode char; Unicode is always 8 pixel (so far)
		while (iLen < len) {
			iUnicode = utf8_to_utf16((utf8 const*)text + iLen, &iLen);
			if (iUnicode == 0) {
				return width;
			}
			w = fnt->screen_width[iUnicode];
			if (w == 0) {
				// default width for missing characters
				w = fnt->screen_width[0];
			}
			width += w;
		}
	} else {
#endif
		while (*text != 0 && len > 0) {
			c = (unsigned char)*text;
			width += fnt->screen_width[c];
			text++;
			len--;
		}
#ifdef UNICODE_SUPPORT
	}
#endif
	return width;
}


/* @ see get_mask() */
static const unsigned char byte_to_mask_array[9] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00 };

/* Helper: calculates mask for clipping *
 * Attention: xL-xR must be <=8 !!!
 * @author priss
 * @date  29.11.04
 */
static unsigned char get_h_mask(const int xL, const int xR, const int cL, const int cR)
{
	// do not mask
	unsigned char mask;

	// check, if there is something to display
	if (xR < cL || xL >= cR) return 0;
	mask = 0xFF;
	// check for left border
	if (xL < cL && xR >= cL) {
		// Left border clipped
		mask = byte_to_mask_array[cL - xL];
	}
	// check for right border
	if (xL < cR && xR >= cR) {
		// right border clipped
		mask &= ~byte_to_mask_array[cR - xL];
	}
	return mask;
}


/*
 * len parameter added - use -1 for previous bvbehaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */
int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char* txt, int flags, const PLAYER_COLOR_VAL color_index, long len)
{
	const font_type* fnt = &large_font;
	KOORD_VAL cL, cR, cT, cB;
	unsigned c;
	size_t	iTextPos = 0; // pointer on text position: prissi
	int char_width_1, char_width_2; // 1 is char only, 2 includes room
	int screen_pos;
	unsigned char *char_data;
	unsigned char *p;
	KOORD_VAL yy = y + fnt->height;
	KOORD_VAL x0;	// store the inital x (for dirty marking)
	KOORD_VAL y0, y_offset, char_height;	// real y for display with clipping
	bool v_clip;
	unsigned char mask1, mask2;	// for horizontal clipping
	const PIXVAL color = color_index;

	// TAKE CARE: Clipping area may be larger than actual screen size ...
	if (flags & DT_CLIP) {
		cL = clip_rect.x;
		if (cL >= disp_width) cL = disp_width;
		cR = clip_rect.xx;
		if (cR >= disp_width) cR = disp_width;
		cT = clip_rect.y;
		if (cT > disp_height) cT = disp_height;
		cB = clip_rect.yy;
		if (cB > disp_height) cB = disp_height + 1;
	} else {
		cL = 0;
		cR = disp_width + 1;
		cT = 0;
		cB = disp_height;
	}
	// don't know len yet ...
	if (len < 0) len = 0x7FFF;

	// adapt x-coordinate for alignment
	switch (flags & ALIGN_MASK) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_MIDDLE:
			x -= display_calc_proportional_string_len_width(txt, len) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len);
			break;
	}

	// still something to display?
	if (x > cR || y > cB || y + fnt->height <= cT) {
		// nothing to display
		return 0;
	}

	// x0 contains the startin x
	x0 = x;
	y0 = y;
	y_offset = 0;
	char_height = fnt->height;
	v_clip = false;
	// calculate vertical y clipping parameters
	if (y < cT) {
		y0 = cT;
		y_offset = cT - y;
		char_height -= y_offset;
		v_clip = TRUE;
	}
	if (yy > cB) {
		char_height -= yy - cB;
		v_clip = TRUE;
	}

	// big loop, char by char
	while (iTextPos < len && txt[iTextPos] != 0) {
		int h;

#ifdef UNICODE_SUPPORT
		// decode char
		if (has_unicode) {
			c = utf8_to_utf16((utf8 const*)txt + iTextPos, &iTextPos);
		} else {
#endif
			c = (unsigned char)txt[iTextPos++];
#ifdef UNICODE_SUPPORT
		}
#endif
		// print unknown character?
		if (c >= fnt->num_chars || fnt->screen_width[c] == 0) {
			c = 0;
		}

		// get the data from the font
		char_data = fnt->char_data + CHARACTER_LEN * c;
		char_width_1 = char_data[CHARACTER_LEN-1];
		uint8 char_yoffset = (sint8)char_data[CHARACTER_LEN-2];
		char_width_2 = fnt->screen_width[c];
		if (char_width_1>8) {
			mask1 = get_h_mask(x, x + 8, cL, cR);
			// we need to double mask 2, since only 2 Bits are used
			mask2 = get_h_mask(x + 8, x + char_width_1, cL, cR);
			mask2 &= 0xF0;
		} else {
			// char_width_1<= 8: call directly
			mask1 = get_h_mask(x, x + char_width_1, cL, cR);
			mask2 = 0;
		}
		// do the display

		if(  y_offset>char_yoffset  ) {
			char_yoffset = y_offset;
		}
		screen_pos = (y0+char_yoffset) * disp_width + x;

		p = char_data + char_yoffset;
		for (h = char_yoffset; h < char_height; h++) {
			unsigned int dat = *p++ & mask1;
			PIXVAL* dst = textur + screen_pos;

			if (dat != 0) {
				if (dat & 0x80) dst[0] = color;
				if (dat & 0x40) dst[1] = color;
				if (dat & 0x20) dst[2] = color;
				if (dat & 0x10) dst[3] = color;
				if (dat & 0x08) dst[4] = color;
				if (dat & 0x04) dst[5] = color;
				if (dat & 0x02) dst[6] = color;
				if (dat & 0x01) dst[7] = color;
			}
			screen_pos += disp_width;
		}

		// extra four bits for overwidth characters (up to 12 pixel supported for unicode)
		if (char_width_1 > 8 && mask2 != 0) {
			p = char_data + char_yoffset/2+12;
			screen_pos = y0 * disp_width + x + 8;
			for (h = char_yoffset; h < char_height; h++) {
				unsigned int char_dat = *p;
				PIXVAL* dst = textur + screen_pos;
				if(h&1) {
					uint8 dat = (char_dat<<4) & mask2;
					if (dat != 0) {
						if (dat & 0x80) dst[0] = color;
						if (dat & 0x40) dst[1] = color;
						if (dat & 0x20) dst[2] = color;
						if (dat & 0x10) dst[3] = color;
					}
					p++;
				}
				else {
					uint8 dat = char_dat & mask2;
					if (dat != 0) {
						if (dat & 0x80) dst[0] = color;
						if (dat & 0x40) dst[1] = color;
						if (dat & 0x20) dst[2] = color;
						if (dat & 0x10) dst[3] = color;
					}
				}
				screen_pos += disp_width;
			}
		}
		// next char: screen width
		x += char_width_2;
	}

	if (flags & DT_DIRTY) {
		// here, because only now we know the lenght also for ALIGN_LEFT text
		mark_rect_dirty_wc(x0, y, x - 1, y + 10 - 1);
	}
	// warning: aktual len might be longer, due to clipping!
	return x - x0;
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color)
{
	display_fillbox_wh(x1, y1,         w, 1, tl_color, TRUE);
	display_fillbox_wh(x1, y1 + h - 1, w, 1, rd_color, TRUE);

	h -= 2;

	display_vline_wh(x1,         y1 + 1, h, tl_color, TRUE);
	display_vline_wh(x1 + w - 1, y1 + 1, h, rd_color, TRUE);
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color)
{
	display_fillbox_wh_clip(x1, y1,         w, 1, tl_color, TRUE);
	display_fillbox_wh_clip(x1, y1 + h - 1, w, 1, rd_color, TRUE);

	h -= 2;

	display_vline_wh_clip(x1,         y1 + 1, h, tl_color, TRUE);
	display_vline_wh_clip(x1 + w - 1, y1 + 1, h, rd_color, TRUE);
}


// if width equals zero, take default value
void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_height / 2 + 1;

	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt - 1, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh(xpos - 2,         ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh(xpos + width - 3, ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos - halfheight + 1, text, ALIGN_LEFT | DT_CLIP, text_farbe, -1);
}


/**
 * display text in 3d box with clipping
 * @author: hsiegeln
 */
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_height / 2 + 1;

	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - 1 - hgt, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh_clip(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh_clip(xpos - 2,         ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh_clip(xpos + width - 3, ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe - 1, dirty);
#if 0
	display_fillbox_wh_clip(xpos - 2, ypos - 6 - hgt, width,  1, ddd_farbe + 1, dirty);
	display_fillbox_wh_clip(xpos - 2, ypos - 5 - hgt, width, 10, ddd_farbe,     dirty);
	display_fillbox_wh_clip(xpos - 2, ypos + 5 - hgt, width,  1, ddd_farbe - 1, dirty);

	display_vline_wh_clip(xpos - 2,         ypos - 6 - hgt, 11, ddd_farbe + 1, dirty);
	display_vline_wh_clip(xpos + width - 3, ypos - 6 - hgt, 11, ddd_farbe - 1, dirty);
#endif
	display_text_proportional_len_clip(xpos + 2, ypos - 5 + (12 - large_font_height) / 2, text, ALIGN_LEFT | DT_CLIP, text_farbe, -1);
}


/**
 * Zeichnet einen mehrzeiligen Text
 * @author Hj. Malthaner
 *
 * Better performance without copying text!
 * @author Volker Meyer
 * @date  15.06.2003
 */
void display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *buf, PLAYER_COLOR_VAL color)
{
	if (buf != NULL && *buf != '\0') {
		const char *next;

		do {
			next = strchr(buf, '\n');
			display_text_proportional_len_clip(
				x, y, buf,
				ALIGN_LEFT | DT_DIRTY | DT_CLIP, color,
				next != NULL ? next - buf : -1
			);
			buf = next + 1;
			y += LINESPACE;
		} while (next != NULL);
	}
}


void display_flush_buffer(void)
{
	int x, y;
	unsigned char* tmp;

	dr_setRGB8multi(0, 239, textur_palette);

#ifdef USE_SOFTPOINTER
	if (softpointer != -1) {
		ex_ord_update_mx_my();
		display_color_img(standard_pointer, sys_event.mx, sys_event.my, 0, false, true);
	}
	old_my = sys_event.my;
#endif

	for (y = 0; y < tile_lines; y++) {
#ifdef DEBUG_FLUSH_BUFFER
		for (x = 0; x < tiles_per_line; x++) {
			if (is_tile_dirty(x, y)) {
				display_fillbox_wh(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE/4, DIRTY_TILE_SIZE/4, 3, FALSE);
				dr_textur(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, DIRTY_TILE_SIZE);
			} else {
				display_fillbox_wh(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE/4, DIRTY_TILE_SIZE/4, 0, FALSE);
				dr_textur(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, DIRTY_TILE_SIZE);
			}
		}
#else
		x = 0;

		do {
			if (is_tile_dirty(x, y)) {
				const int xl = x;
				do {
					x++;
				} while (x < tiles_per_line && is_tile_dirty(x, y));

				dr_textur(xl << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, (x - xl) << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE);
			}
			x++;
		} while (x < tiles_per_line);
#endif
	}

	// swap tile buffers
	tmp = tile_dirty_old;
	tile_dirty_old = tile_dirty;
	tile_dirty = tmp;
	MEMZERON(tile_dirty, tile_buffer_length);
}


/**
 * Bewegt Mauszeiger
 * @author Hj. Malthaner
 */
void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy)
{
	move_pointer(dx, dy);
}


/**
 * Schaltet Mauszeiger sichtbar/unsichtbar
 * @author Hj. Malthaner
 */
void display_show_pointer(int yesno)
{
#ifdef USE_SOFTPOINTER
	softpointer = yesno;
#else
	show_pointer(yesno);
#endif
}


/**
 * mouse pointer image
 * @author prissi
 */
void display_set_pointer(int pointer)
{
	standard_pointer = pointer;
}


/**
 * mouse pointer image
 * @author prissi
 */
void display_show_load_pointer(int loading)
{
#ifdef USE_SOFTPOINTER
	softpointer = !loading;
#else
	set_pointer(loading);
#endif
}


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int full_screen)
{
	int parameter[2];

	parameter[0] = 0;
	parameter[1] = 1;

	dr_os_init(parameter);

	// make sure it something of 16 (also better for caching ... )
	width = (width + 15) & 0x7FF0;

	if (dr_os_open(width, height, 8, full_screen)) {
		disp_width = width;
		disp_height = height;

		textur = (PIXVAL *)dr_textur_init();

		// init, load, and check fonts
		large_font.screen_width = NULL;
		large_font.char_data = NULL;
		display_load_font(FONT_PATH_X "prop.fnt");
	} else {
		puts("Error  : can't open window!");
		exit(-1);
	}

	// allocate dirty tile flags
	tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

	tile_dirty     = MALLOCN(unsigned char, tile_buffer_length);
	tile_dirty_old = MALLOCN(unsigned char, tile_buffer_length);

	memset(tile_dirty,     255, tile_buffer_length);
	memset(tile_dirty_old, 255, tile_buffer_length);

	// init player colors
	for(int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		player_offsets[i][0] = i*8;
		player_offsets[i][1] = i*8+24;
	}

	display_set_clip_wh(0, 0, disp_width, disp_height);

	display_day_night_shift(0);

	calc_base_pal_from_night_shift(0);

	printf("Init done.\n");
	fflush(NULL);

	return TRUE;
}


/**
 * Prueft ob das Grafikmodul schon init. wurde
 * @author Hj. Malthaner
 */
int is_display_init(void)
{
	return textur != NULL;
}



// delete all images above a certain number ...
// (mostly needed when changing climate zones)
void display_free_all_images_above( unsigned above )
{
	while( above<anz_images) {
		anz_images--;
		if(images[anz_images].zoom_data!=NULL) {
			guarded_free( images[anz_images].zoom_data );
		}
		if(images[anz_images].data!=NULL) {
			guarded_free( images[anz_images].data );
		}
		if(images[anz_images].base_data!=NULL) {
			guarded_free( images[anz_images].base_data );
		}
	}
}



/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
int simgraph_exit()
{
	guarded_free(tile_dirty);
	guarded_free(tile_dirty_old);
	display_free_all_images_above(0);
	guarded_free(images);

	tile_dirty = tile_dirty_old = NULL;
	images = NULL;

	return dr_os_close();
}


/* changes display size
 * @author prissi
 */
void simgraph_resize(KOORD_VAL w, KOORD_VAL h)
{
	if (disp_width != w || disp_height != h) {
		disp_width = (w + 15) & 0x7FF0;
		disp_height = h;

		guarded_free(tile_dirty);
		guarded_free(tile_dirty_old);

		dr_textur_resize((unsigned short**)&textur, disp_width, disp_height, 8);

		tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

		tile_dirty     = MALLOCN(unsigned char, tile_buffer_length);
		tile_dirty_old = MALLOCN(unsigned char, tile_buffer_length);

		memset(tile_dirty,     255, tile_buffer_length);
		memset(tile_dirty_old, 255, tile_buffer_length);

		display_set_clip_wh(0, 0, disp_width, disp_height);
	}
}


/**
 * Speichert Screenshot
 * @author Hj. Malthaner
 */
void display_snapshot()
{
	static int number = 0;

	char buf[80];

#ifdef WIN32
	mkdir(SCRENSHOT_PATH);
#else
	mkdir(SCRENSHOT_PATH, 0700);
#endif

	do {
		sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.bmp", number++);
	} while (access(buf, W_OK) != -1);

	dr_screenshot(buf);
}


/**
 * zeichnet linie von x,y nach xx,yy
 * von Hajo
 **/
void display_direct_line(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color)
{
	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	for (i = 0; i <= steps; i++) {
		display_pixel(xp >> 16, yp >> 16, color);
		xp += xs;
		yp += ys;
	}
}


/**
 * Zeichnet eine Fortschrittsanzeige
 * @author Hj. Malthaner
 */
static const char *progress_text=NULL;

void display_set_progress_text(const char *t)
{
	progress_text = t;
}

void display_progress(int part, int total)
{
	const int disp_width=display_get_width();
	const int disp_height=display_get_height();

	const int width=disp_width/2;
	part = (part*width)/total;

	dr_prepare_flush();

	// outline
	display_ddd_box(width/2-2, disp_height/2-9, width+4, 20, COL_GREY6, COL_GREY4);
	display_ddd_box(width/2-1, disp_height/2-8, width+2, 18, COL_GREY4, COL_GREY6);

	// inner
	display_fillbox_wh(width/2, disp_height/2-7, width, 16, COL_GREY5, TRUE);

	// progress
	display_fillbox_wh(width/2, disp_height/2-5, part, 12, COL_BLUE, TRUE);

	if(progress_text) {
		display_proportional(width,display_get_height()/2-4,progress_text,ALIGN_MIDDLE,COL_WHITE,0);
	}
	dr_flush();
}



// Knightly : variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = NULL;
display_image_proc display_color = NULL;
display_blend_proc display_blend = NULL;
signed short current_tile_raster_width = 0;

// clipping along tile borders not implemented for 8bit graphics
void add_poly_clip(int, int, int, int, int)
{
}

void clear_all_poly_clip()
{
}

void activate_ribi_clip(int)
{
}
