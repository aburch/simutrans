/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * hansjoerg.malthaner@gmx.de
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

/* simgraph.c
 *
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 *
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
#include <zlib.h>

#include "simtypes.h"
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

#ifdef USE_SMALL_FONT
static font_type	large_font, small_font;
#else
static font_type	large_font;
#define small_font (large_font)
#endif

// needed for gui
int large_font_height = 10;


static const PIXVAL colortable_8bit[3 * 256] = {
	 65,  96, 122,
	105, 135, 165,
	146, 176, 210,
	187, 216, 255,
	110, 110, 110,
	148, 148, 148,
	186, 186, 186,
	225, 225, 225,
	 37,  74, 142,
	 60, 106, 177,
	 83, 138, 212,
	107, 170, 247,
	137, 110,  19,
	175, 155,  28,
	212, 200,  36,
	250, 245,  45,
	102,  43,  22,
	147,  61,  30,
	191,  79,  37,
	237,  98,  44,
	 47,  77,  21,
	 68, 118,  33,
	 90, 159,  45,
	112, 200,  58,
	 14, 105,  97,
	 19, 149, 137,
	 25, 192, 176,
	 31, 236, 217,
	105,  41, 148,
	146,  67, 180,
	186,  92, 211,
	227, 118, 244,
  0x57, 0x65, 0x6F, // Dark windows, lit yellowish at night
  0x7F, 0x9B, 0xF1, // Lighter windows, lit blueish at night
  0xFF, 0xFF, 0x53, // Yellow light
  0xFF, 0x21, 0x1D, // Red light
  0x01, 0xDD, 0x01, // Green light
  0x6B, 0x6B, 0x6B, // Non-darkening grey 1 (menus)
  0x9B, 0x9B, 0x9B, // Non-darkening grey 2 (menus)
  0xB3, 0xB3, 0xB3, // non-darkening grey 3 (menus)
  0xC9, 0xC9, 0xC9, // Non-darkening grey 4 (menus)
  0xDF, 0xDF, 0xDF, // Non-darkening grey 5 (menus)
  0xE3, 0xE3, 0xFF, // Nearly white light at day, yellowish light at night
  0xC1, 0xB1, 0xD1, // Windows, lit yellow
  0x4D, 0x4D, 0x4D, // Windows, lit yellow
	 32,  64, 255, // blue for non-zomables (non darkening)
	 80, 150,  30, // grass green
	255,  32,   0, // very red
	222,  52,  52, // light red
	225, 192,   0, // red yellow
	160,  80,   0, // orange
	180, 128,  96, // earth color
	136, 171, 211, // light player color
	 57,  94, 124, // player color
	128, 255, 128, // blueish green
	225, 255, 128, // very light green
	161, 110, 120, // normal darkening colors again
	 84, 125, 118,
	 38,  21,  21,
	165, 186, 145,
	 91,  60,  88,
	255, 255, 251,
	 66,  97,  93,
	176, 163, 197,
	124,  90, 138,
	238, 242, 232,
	132, 156, 176,
	185, 164, 144,
	 75,  96,  65,
	184, 205, 208,
	 97,  77, 118,
	174, 130, 140,
	 26,  40,  36,
	143, 181, 143,
	 90,  66,  63,
	232, 219, 219,
	 93, 137, 134,
	191, 188, 160,
	197, 183, 210,
	 49,  42,  63,
	150,  99, 133,
	 82,  68, 102,
	113, 164, 137,
	212, 229, 222,
	 40,  36,  26,
	 72,  52,  79,
	 22,  39,  22,
	 21,  21,  38,
	  0,   0,   0,
	 22,  22,  22,
	 46,  46,  46,
	 61,  61,  61,
	 78,  78,  78,
	 95,  95,  95,
	113, 113, 113,
	132, 132, 132,
	152, 152, 152,
	172, 172, 172,
	191, 191, 191,
	211, 211, 211,
	231, 231, 231,
	247, 247, 247,
	255, 255, 255,
	 96,  22,  13,
	135,  46,  22,
	170, 122, 104,
	220, 195, 186,
	198, 125, 107,
	248, 199, 190,
	128, 101, 86,
	142, 118, 100,
	150, 150, 107,
	178, 153, 110,
	206, 156, 114,
	212, 188, 121,
	254, 231, 197,
	247, 209, 162,
	129,  78,  56,
	183, 129,  91,
	238, 180, 126,
	247, 222, 131,
	254, 254, 138,
	173,  69,  31,
	212,  90,  39,
	219,  97, 104,
	100,  75,  24,
	128,  79,  28,
	156, 115,  37,
	156,  82,  31,
	191, 117,  42,
	202, 145,  49,
	219, 121,  45,
	225, 152,  52,
	107, 107,  31,
	135, 110,  34,
	141, 141,  42,
	169, 145,  45,
	204, 179,  55,
	212, 212,  62,
	232, 184,  59,
	239, 215,  65,
	114, 114,  97,
	100, 122,  79,
	121, 145, 104,
	184, 184, 117,
	156, 181, 114,
	226, 226, 193,
	204, 226, 161,
	255, 255, 204,
	 49,  70,  39,
	 72,  72,  21,
	176, 176,  52,
	191, 216, 125,
	163, 212, 120,
	198, 247, 131,
	170, 244, 128,
	 79, 103,  28,
	113, 138,  38,
	148, 173,  49,
	120, 169,  45,
	184, 207,  59,
	128, 201,  52,
	 44,  69,  18,
	 52,  92,  23,
	 64, 129,  31,
	 85, 135,  34,
	 92, 166,  41,
	 99, 197,  48,
	134, 232,  59,
	106, 229,  55,
	 87, 111,  94,
	164, 188, 180,
	136, 185, 176,
	199, 223, 190,
	171, 220, 186,
	206, 254, 197,
	 48,  90,  63,
	 93, 142, 100,
	128, 178, 110,
	100, 173, 107,
	146, 200, 169,
	143, 216, 183,
	135, 209, 117,
	  8,  41,   8,
	 19,  84,  18,
	 19,  64,  51,
	 31, 104,  86,
	 37, 135,  93,
	 72, 170, 104,
	 45, 143, 159,
	 66, 214, 238,
	 42, 150, 133,
	 53, 189, 170,
	 43, 190,  41,
	114, 139, 162,
	157, 157, 173,
	122, 122, 162,
	157, 181, 208,
	235, 235, 255,
	207, 231, 255,
	200, 200, 248,
	179, 228, 252,
	 51,  76,  83,
	 73,  98, 119,
	101, 150, 165,
	 94, 119, 159,
	 88,  88, 152,
	172, 197, 245,
	165, 165, 238,
	144, 192, 242,
	 66, 116, 155,
	 83, 114, 185,
	100, 144, 206,
	137, 161, 235,
	130, 130, 228,
	116, 189, 238,
	109, 158, 232,
	102, 126, 224,
	 25,  31,  94,
	 41,  79, 122,
	 43,  55, 135,
	 53,  85, 147,
	 38, 111, 152,
	 81, 154, 228,
	 74, 123, 221,
	 67,  92, 214,
	 74,  28,  22,
	107,  83,  89,
	114,  96, 116,
	168, 144, 169,
	185, 160, 176,
	223, 192, 223,
	255, 238, 255,
	228, 203, 253,
	 68,  47,  48,
	135,  87,  94,
	150, 126, 166,
	178, 129, 169,
	213, 164, 180,
	193, 169, 242,
	221, 172, 245,
	241, 168, 183,
	116,  91, 156,
	144,  94, 159,
	172,  98, 162,
	163,  90,  97,
	190, 104, 159,
	191,  94, 100,
	214, 141, 238,
	 94,  26,  65,
	 97,  25, 131,
	142,  64, 112,
	140,  54, 165,
	184,  83, 200,
	207, 109, 232,
	241, 255, 255
};


// the palette for the display ...
static PIXVAL textur_palette[256 * 3];

// the palette actually used for the display ...
static PIXVAL day_pal[256 * 3];

// the palette with player colors
static PIXVAL special_pal[256 * 3];

/*
 * Hajo: Currently selected color set for player 0 (0..15)
 */
static int selected_player_color_set = 0;


/*
 * Hajo: Image map descriptor struture
 */
struct imd {
	unsigned char base_x; // current min x offset
	unsigned char base_w; // current width
	unsigned char base_y; // current min y offset
	unsigned char base_h; // current width

	unsigned char x; // current (zoomed) min x offset
	unsigned char w; // current (zoomed) width
	unsigned char y; // current (zoomed) min y offset
	unsigned char h; // current (zoomed) width

	unsigned int len; // base image data size (used for allocation purposes only)
	unsigned char recode_flags[4]; // first byte: needs recode, second byte: code normal, second byte: code for player1

	PIXVAL* data; // current data, zoomed and adapted to output format RGB 555 or RGB 565

	PIXVAL* zoom_data; // zoomed original data

	PIXVAL* base_data; // original image data

	PIXVAL* player_data; // current data coded for player1 (since many building belong to him)
};

// offsets in the recode array
#define NEED_REZOOM (0)
#define NEED_NORMAL_RECODE (1)
#define NEED_PLAYER_RECODE (2)
#define ZOOMABLE (3)



static int disp_width  = 640;
static int disp_height = 480;


/*
 * Image table
 */
static struct imd* images = NULL;

/*
 * Number of loaded images
 */
static unsigned int anz_images = 0;

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
static int color_level = 1;

static int night_shift = -1;


#define LIGHT_COUNT 13

/*
 * Hajo: speical colors during daytime
 */
static const int day_lights[LIGHT_COUNT] = {
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
 * Hajo: speical colors during nighttime
 */
static const int night_lights[LIGHT_COUNT] = {
	0xD3C380, // Dark windows, lit yellowish at night
	0x80C3D3, // Lighter windows, lit blueish at night
	0xFFFF53, // Yellow light
	0xFF211D, // Red light
	0x01DD01, // Green light
	0x6B6B6B, // Non-darkening grey 1 (menus)
	0x9B9B9B, // Non-darkening grey 2 (menus)
	0xB3B3B3, // non-darkening grey 3 (menus)
	0xC9C9C9, // Non-darkening grey 4 (menus)
	0xDFDFDF, // Non-darkening grey 5 (menus)
	0xFFFFE3, // Nearly white light at day, yellowish light at night
	0xD3C380, // Windows, lit yellow
	0xD3C380, // Windows, lit yellow
};


static PIXVAL* conversion_table = NULL;

/*
 * Hajo: tile raster width
 */
int tile_raster_width = 16;
static int base_tile_raster_width = 16;

/*
 * Hajo: Zoom factor
 */
static int zoom_factor = 1;


/**
 * Rezooms all images
 * @author Hj. Malthaner
 */
static void rezoom(void)
{
	unsigned int n;

	for (n = 0; n < anz_images; n++) {
		images[n].recode_flags[NEED_REZOOM] = (images[n].recode_flags[ZOOMABLE] && images[n].base_h > 0);
		images[n].recode_flags[NEED_NORMAL_RECODE] = 128;
		images[n].recode_flags[NEED_PLAYER_RECODE] = 128;	// color will be set next time
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


#ifdef _MSC_VER
#	define inline _inline
#endif


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
		memcpy(day_pal, colortable_8bit, sizeof(day_pal));
	}

	conversion_table = guarded_malloc(32768 + 256);
	for (red = 0; red < 256; red += 8) {
		for (green = 0; green < 256; green += 8) {
			for (blue = 0; blue < 256; blue += 8) {
				const int index = red << 7 | green << 2 | blue >> 3;

				PIXVAL best_match = 88;
				long distance = 0x00FFFFFF;
				int i;
				for (i = 4; i < 256; i++) {
					if (i == 32) i += 24;

					long new_dist =
						abs(red   - day_pal[i * 3 + 0]) +
						abs(green - day_pal[i * 3 + 1]) +
						abs(blue  - day_pal[i * 3 + 2]);
					if (new_dist < distance) {
						distance = new_dist;
						best_match = i;
					}
				}
				conversion_table[index] = best_match;
			}
		}
	}
	// new the special colors
	int index;
	for (index =  0; index <  7; index++) conversion_table[index + 32768] = index / 2;
	for (index =  8; index < 16; index++) conversion_table[index + 32768] = 88;
	for (index = 16; index < 40; index++) conversion_table[index + 32768] = 16 + index;

	printf("ok.\n");
}


/**
 * Convert a certain image data to actual output data for a certain player
 * @author prissi
 */
static void recode_img_src16_target8(int h, PIXVAL *src16, PIXVAL *target, bool non_dark)
{
	if (conversion_table == NULL) init_16_to_8_conversion();

	unsigned short *src=(unsigned short *)src16;
	if (h > 0) {
		do {
			unsigned char runlen = *target ++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok
				runlen = *target++ = *src++;
				// now just convert the color pixels
				if (non_dark) {
					while (runlen--) {
						unsigned short pix = *src++;
						*target++ = conversion_table[pix];
					}
				} else {
						// find best match by foot (slowly, but not often used)
					while (runlen-- != 0) {
						unsigned short pix = *src++;
						long red   = (pix >> 7) & 0x00F8;
						long green = (pix >> 2) & 0x00F8;
						long blue  = (pix << 3) & 0x00F8;
						if (pix > 0x8000) {
							if (pix < 0x8008) {
								*target++ = pix > 0x8004 ? 52 : 53;
								continue;
							}
							if (pix == 0x8010 || pix == 0x8011 || (pix >= 0x801A && pix < 0x801C)) {
								pix   = conversion_table[pix];
								red   = day_pal[pix * 3 + 0];
								green = day_pal[pix * 3 + 1];
								blue  = day_pal[pix * 3 + 2];
							} else {
								*target++ = (pix & 0x00FF) + 16;
								continue;
							}
						}

						PIXVAL best_match = 88;
						long distance = red * red + green * green + blue * blue;
						int i;
						for (i = 32 + 2; i < 56; i++) {
							if (i == 42) i = 45;

							long new_dist =
								(red   - colortable_8bit[i * 3 + 0]) * (red   - colortable_8bit[i * 3 + 0]) +
								(green - colortable_8bit[i * 3 + 1]) * (green - colortable_8bit[i * 3 + 1]) +
								(blue  - colortable_8bit[i * 3 + 2]) * (blue  - colortable_8bit[i * 3 + 2]);

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
		images[n].recode_flags[NEED_REZOOM] = FALSE;
		images[n].recode_flags[NEED_NORMAL_RECODE] = 128;
		images[n].recode_flags[NEED_PLAYER_RECODE] = 128;

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
			unsigned char y_left = (images[n].base_y + zoom_factor - 1) % zoom_factor;
			unsigned char h = images[n].base_h;

			static PIXVAL line[512];
			PIXVAL *src = images[n].base_data;
			PIXVAL *dest, *last_dest;

			// decode/recode linewise
			unsigned int last_color = 255; // ==255 to keep compiler happy

			if (images[n].zoom_data == NULL) {
				// normal len is ok, since we are only skipping parts ...
				images[n].zoom_data = guarded_malloc(sizeof(PIXVAL) * images[n].len);
			}
			last_dest = dest = images[n].zoom_data;

			do { // decode/recode line
				unsigned int runlen;
				unsigned int color = 0;
				PIXVAL *p = line;
				const int imgw = images[n].base_x + images[n].base_w;

				// left offset, which was left by division
				runlen = images[n].base_x%zoom_factor;
				while (runlen--) *p++ = 54;

				// decode line
				runlen = *src++;
				color -= runlen;
				do {
					// clear run
					while (runlen--) *p++ = 54;
					// color pixel
					runlen = *src++;
					color += runlen;
					while (runlen--) *p++ = *src++;
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
						for (i = 0; line[step] == 54 && step < imgw; i++, step += zoom_factor) {}
						*dest++ = i;
						// chopy for non-transparent
						for (i = 0; line[step] != 54 && step < imgw; i++, step += zoom_factor) {
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


int	display_set_unicode(int use_unicode)
{
	return has_unicode = (use_unicode != 0);
}


/**
 * Loads the fonts (true for large font)
 * @author prissi
 */
bool display_load_font(const char* fname, bool large)
{
	if (load_font(large ? &large_font : &small_font, fname)) {
		if (large) large_font_height = large_font.height;
		return true;
	} else {
		return false;
	}
}


#ifdef USE_SMALL_FONT

/* copy a font
 * @author prissi
 */
static void copy_font(font_type *dest_font, font_type *src_font)
{
	memcpy(dest_font, src_font, sizeof(font_type));
	dest_font->char_data = malloc(dest_font->num_chars * 16);
	memcpy(dest_font->char_data, src_font->char_data, 16 * dest_font->num_chars);
	dest_font->screen_width = malloc(dest_font->num_chars);
	memcpy(dest_font->screen_width, src_font->screen_width, dest_font->num_chars);
}


/* checks if a small and a large font exists;
 * if not the missing font will be emulated
 * @author prissi
 */
void display_check_fonts(void)
{
	// replace missing font, if none there (better than crashing ... )
	if (small_font.screen_width == NULL && large_font.screen_width != NULL) {
		printf("Warning: replacing small  with large font!\n");
		copy_font(&small_font, &large_font);
	}
	if (large_font.screen_width == NULL && small_font.screen_width != NULL) {
		printf("Warning: replacing large  with small font!\n");
		copy_font(&large_font, &small_font);
	}
}

#endif


/**
 * Laedt die Palette
 * @author Hj. Malthaner
 */
static int load_palette(const char *filename, unsigned char *palette)
{
	FILE* file = fopen(filename,"rb");

	if (file != NULL) {
		int x;
		int anzahl = 256;
		int r, g, b;

		fscanf(file, "%d", &anzahl);

		for (x = 0; x < anzahl; x++) {
			fscanf(file,"%d %d %d", &r, &g, &b);

			palette[x * 3 + 0] = r;
			palette[x * 3 + 1] = g;
			palette[x * 3 + 2] = b;
		}

		fclose(file);
	} else {
		fprintf(stderr, "Error: can't open file '%s' for reading\n", filename);
	}

	return file != NULL;
}


KOORD_VAL display_get_width(void)
{
	return disp_width;
}


KOORD_VAL display_get_height(void)
{
	return disp_height;
}


/**
 * Holt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
int display_get_light(void)
{
	return light_level;
}


/**
 * Setzt Farbeinstellungen
 * @author Hj. Malthaner
 */
static void display_set_color(int new_color_level)
{
	color_level = new_color_level;

	if (color_level < 0) color_level = 0;
	if (color_level > 3) color_level = 3;

	switch (color_level) {
		case 0: load_palette(PALETTE_PATH_X "simud70.pal",  day_pal); break;
		case 1: load_palette(PALETTE_PATH_X "simud80.pal",  day_pal); break;
		case 2: load_palette(PALETTE_PATH_X "simud90.pal",  day_pal); break;
		case 3: load_palette(PALETTE_PATH_X "simud100.pal", day_pal); break;
	}
}



/* Tomas variant */
static void calc_base_pal_from_night_shift(const int night)
{
	const int day = 4 - night;
	unsigned int i;

	// constant multiplier 0,66 - dark night  255 will drop to 49, 55 to 10
	//                     0,7  - dark, but all is visible     61        13
	//                     0,73                                72        15
	//                     0,75 - quite bright                 80        17
	//                     0,8    bright                      104        22
#if 0
	const double RG_nihgt_multiplier = pow(0.73, night);
	const double B_nihgt_multiplier  = pow(0.82, night);
#endif

	const double RG_nihgt_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_nihgt_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);

	for (i = 0; i<256; i++) {
		// (1<<15) this is total no of all possible colors in RGB555)
		if (i == 32) i = 56; // skip special colors

		// RGB 555 input
		int R = day_pal[i * 3 + 0];
		int G = day_pal[i * 3 + 1];
		int B = day_pal[i * 3 + 2];

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
		const int day_R =  day_lights[i] >> 16;
		const int day_G = (day_lights[i] >> 8) & 0xFF;
		const int day_B =  day_lights[i] & 0xFF;

		const int night_R =  night_lights[i] >> 16;
		const int night_G = (night_lights[i] >> 8) & 0xFF;
		const int night_B =  night_lights[i] & 0xFF;

		const int R = (day_R * day + night_R * night) >> 2;
		const int G = (day_G * day + night_G * night) >> 2;
		const int B = (day_B * day + night_B * night) >> 2;

		textur_palette[i * 3 + 32 * 3 + 0] = R>0 ? R : 0;
		textur_palette[i * 3 + 32 * 3 + 1] = G>0 ? R : 0;
		textur_palette[i * 3 + 32 * 3 + 2] = B>0 ? R : 0;
	}
	// non darkening
	for (i = LIGHT_COUNT + 32; i < 56; i++) {
		textur_palette[i * 3 + 0] = colortable_8bit[i * 3 + 0];
		textur_palette[i * 3 + 1] = colortable_8bit[i * 3 + 1];
		textur_palette[i * 3 + 2] = colortable_8bit[i * 3 + 2];
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
	}
}


/**
 * Sets color set for player 0
 * @param entry   number of color set, range 0..15
 * @author Hj. Malthaner
 */
static void display_set_player_color(int entry)
{
	int i;

	selected_player_color_set = entry;

	entry *= 16 * 3;

	// Legacy set of 4 player colors, unshaded
	for (i = 0; i < 4; i++) {
		day_pal[i * 3 + 0] = special_pal[entry + i * 3 * 2 + 0];
		day_pal[i * 3 + 1] = special_pal[entry + i * 3 * 2 + 1];
		day_pal[i * 3 + 2] = special_pal[entry + i * 3 * 2 + 2];
	}

	// light
	for (i = 32; i < 56; i++) {
		day_pal[i * 3 + 0] = colortable_8bit[i * 3 * 2 + 0];
		day_pal[i * 3 + 1] = colortable_8bit[i * 3 * 2 + 1];
		day_pal[i * 3 + 2] = colortable_8bit[i * 3 * 2 + 2];
	}

	calc_base_pal_from_night_shift(night_shift);

	mark_rect_dirty_nc(0, 0, disp_width - 1, disp_height - 1);
}


/**
 * Fügt ein Image aus anderer Quelle hinzu
 */
void register_image(struct bild_besch_t *bild)
{
	struct imd* image;

	if (anz_images >= 65535) {
		fprintf(stderr, "FATAL:\n*** Out of images (more than 65534!) ***\n");
		abort();
	}

	if (anz_images == alloc_images) {
		alloc_images += 128;
		images = guarded_realloc(images, sizeof(*images) * alloc_images);
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
	image->recode_flags[NEED_NORMAL_RECODE] = 128;
#ifdef NEED_PLAYER_RECODE
	image->recode_flags[NEED_PLAYER_RECODE] = 128;
#endif
	image->recode_flags[NEED_REZOOM] = true;

	image->base_data = NULL;
	image->zoom_data = NULL;
	image->data = NULL;
	image->player_data = NULL;	// chaches data for one AI
	image->recode_flags[ZOOMABLE] = (bild->zoomable&1);

	image->base_data = (PIXVAL *)guarded_malloc(image->len*sizeof(PIXVAL));
	if ((bild->zoomable & 0xFE) == 0) {
		// this is an 16 Bit image => we need to resize it to 8 Bit ...
		recode_img_src16_target8(image->base_h, (PIXVAL*)(bild + 1), image->base_data, bild->zoomable & 1);
	} else {
		memcpy(image->base_data, (PIXVAL*)(bild + 1), image->len * sizeof(PIXVAL));
	}

	if (base_tile_raster_width < bild->w) {
		base_tile_raster_width = bild->w;
		tile_raster_width = base_tile_raster_width;
	}
}


// prissi: query offsets
void display_get_image_offset(unsigned bild, int *xoff, int *yoff, int *xw, int *yw)
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
void display_set_image_offset(unsigned bild, int xoff, int yoff)
{
	if (bild < anz_images && images[bild].base_h > 0 && images[bild].base_w > 0) {
		int h = images[bild].base_h;
		PIXVAL *sp = images[bild].base_data;

		// avoid overflow
		if (images[bild].base_x + xoff < 0) xoff = -images[bild].base_x;
		images[bild].base_x += xoff;
		images[bild].base_y += yoff;
		// the offfset is hardcoded into the image
		while (h-- > 0) {
			// one line
			*sp += xoff;	// reduce number of transparent pixels (always first)
			do {
				// clear run + colored run + next clear run
				++sp;
				sp += *sp + 1;
			} while (*sp);
			sp++;
		}
		// need recoding
		images[anz_images].recode_flags[NEED_NORMAL_RECODE] = 128;
#ifdef NEED_PLAYER_RECODE
		images[anz_images].recode_flags[NEED_PLAYER_RECODE] = 128;
#endif
		images[anz_images].recode_flags[NEED_REZOOM] = true;
	}
}


/**
 * Holt Maus X-Position
 * @author Hj. Malthaner
 */
int gib_maus_x(void)
{
	return sys_event.mx;
}


/**
 * Holt Maus y-Position
 * @author Hj. Malthaner
 */
int gib_maus_y(void)
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
struct clip_dimension display_gib_clip_wh(void)
{
	return clip_rect;
}


/**
 * Setzt Clipping Rechteck
 * @author Hj. Malthaner
 */
void display_setze_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h)
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
void display_img_nc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
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
					const unsigned short* ls;
					unsigned short* ld;

					if (runlen & 1) *p++ = *sp++;

					ls = (const unsigned short*)sp;
					ld = (unsigned short*)p;
					runlen >>= 1;
					while (runlen--) *ld++ = *ls++;
					p = (PIXVAL*)ld;
					sp = (const PIXVAL*)ls;
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
void display_img_aux(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool player)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		int h, reduce_h, skip_lines;

		if (images[n].recode_flags[NEED_REZOOM]) rezoom_img(n);
		const PIXVAL* sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;

		// now, since zooming may have change this image
		yp += images[n].y;
		h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		reduce_h = yp + h - 1 - clip_rect.yy;
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
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL w = images[n].w;

			// since height may be reduced, start marking here
			if (dirty) mark_rect_dirty_wc(xp + x, yp, xp + x + w - 1, yp + h - 1);

			// use horzontal clipping or skip it?
			if (xp + x >= clip_rect.x && xp + x + w - 1 <= clip_rect.xx) {
				display_img_nc(h, xp, yp, sp);
			} else if (xp <= clip_rect.xx && xp + x + w > clip_rect.x) {
				display_img_wc(h, xp, yp, sp);
			}
		}
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * assumes height is ok and valid data are caluclated
 * @author hajo/prissi
 */
static void display_color_img_aux(const int n, const KOORD_VAL xp, const KOORD_VAL yp, const int color)
{
	KOORD_VAL h = images[n].h;
	KOORD_VAL y = yp + images[n].y;
	KOORD_VAL yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	if (h > 0) { // clipping may have reduced it
		// color replacement needs the original data
		const PIXVAL *sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;
		PIXVAL *tp = textur + y * disp_width;

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

					colorpixcopy(tp + xpos + left, sp + left, sp + len, color);
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
void display_color_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const COLOR_VAL color, const int daynight, const int dirty)
{
	if (n < anz_images) {
		// since vehicle need larger dirty rect!
		if (dirty) {
			mark_rect_dirty_wc(
				xp + images[n].x - 8,
				yp + images[n].y - 4,
				xp + images[n].x + images[n].w + 8 - 1,
				yp + images[n].y + images[n].h + 4 - 1
			);
		}

		// Hajo: since the colors for player 0 are already right,
		// only use the expensive replacement routine for colored images
		// of other players
		// Hajo: player 1 does not need any recoloring, too
		if (color < 8 && daynight) {
			display_img_aux(n, xp, yp, false, false);
			return;
		}

		if (images[n].recode_flags[NEED_REZOOM]) rezoom_img(n);

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
		}

		display_color_img_aux(n, xp, yp, color); // color is multiple of 4
	} // number ok
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
		const PIXVAL colval = color;
		PIXVAL *p = textur + xp + yp*disp_width;
		const unsigned long longcolval = (colval << 8) | colval;
		const int dx = disp_width - w;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		do {
			unsigned int count = w;
			unsigned short* lp;

			if (count & 1) *p++ = longcolval;
			count >>= 1;
			lp = p;
			while (count-- != 0) *lp++ = longcolval;
			p = lp;

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
static void display_vl_internal(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const int color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	clip_lr(&yp, &h, cT, cB - 1);

	if (xp >= cL && xp <= cR && h > 0) {
		PIXVAL *p = textur + xp + yp*disp_width;
		const PIXVAL colval = color;

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


/* proportional_string_width with a text of a given length
 * extended for universal font routines with unicode support
 * @author Volker Meyer
 * @date  15.06.2003
 * @author prissi
 * @date 29.11.04
 */
int display_calc_proportional_string_len_width(const char *text, int len, bool use_large_font)
{
	font_type *fnt = use_large_font ? &large_font : &small_font;
	unsigned int c, width = 0;
	int w;

#ifdef UNICODE_SUPPORT
	if (has_unicode) {
		unsigned short iUnicode;
		int	iLen = 0;

		// decode char; Unicode is always 8 pixel (so far)
		while (iLen < len) {
			iUnicode = utf8_to_utf16(text + iLen, &iLen);
			if (iUnicode == 0) return width;
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


/* @ see get_v_mask() */
static const unsigned char left_byte_to_v_mask_array[5]  = { 0xFF, 0x3F, 0x0F, 0x03, 0x00 };
static const unsigned char right_byte_to_v_mask_array[5] = { 0xFF, 0xFC, 0xF0, 0xC0, 0x00 };

/* Helper: calculates mask for clipping of 2Bit extension *
 * Attention: xL-xR must be <=4 !!!
 * @author priss
 * @date  29.11.04
 */
static unsigned char get_v_mask(const int yT, const int yB, const int cT, const int cB)
{
	// do not mask
	unsigned char mask;

	// check, if there is something to display
	if (yB <= cT || yT > cB) return 0;

	mask = 0xFF;

	// top bits masked
	if (yT >= cT && yT < cT + 4) {
		mask = left_byte_to_v_mask_array[yT - cT];
	}
	// mask height start bits
	if (mask && yB > cT && yB <= cT + 4) {
		// Left border clipped
		mask &= right_byte_to_v_mask_array[cT + 4 - yB];
	}
	return mask;
}


/*
 * len parameter added - use -1 for previous bvbehaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */
int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char *txt, int align, const PLAYER_COLOR_VAL color_index, int dirty, bool use_large_font, int len, bool use_clipping)
{
	font_type *fnt = use_large_font ? &large_font : &small_font;
	KOORD_VAL cL, cR, cT, cB;
	unsigned c;
	int	iTextPos = 0; // pointer on text position: prissi
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
	if (use_clipping) {
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
	switch (align) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_MIDDLE:
			x -= display_calc_proportional_string_len_width(txt, len, use_large_font) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len, use_large_font);
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
			c = utf8_to_utf16(txt + iTextPos, &iTextPos);
		} else {
#endif
			c = (unsigned char)txt[iTextPos++];
#ifdef UNICODE_SUPPORT
		}
#endif
		// print unknown character?
		if (c >= fnt->num_chars || fnt->screen_width[c] == 0) c = 0;

		// get the data from the font
		char_width_1 = fnt->char_data[16 * c + 15];
		char_width_2 = fnt->screen_width[c];
		char_data = fnt->char_data + 16l * c;
		if (char_width_1 > 8) {
			mask1 = get_h_mask(x, x + 8, cL, cR);
			// we need to double mask 2, since only 2 Bits are used
			mask2 = get_h_mask(x + 8, x + char_width_1, cL, cR);
			// since only two pixels are used
			mask2 &= 0xC0;
			if (mask2 & 0x80) mask2 |= 0xAA;
			if (mask2 & 0x40) mask2 |= 0x55;
		} else {
			// char_width_1<= 8: call directly
			mask1 = get_h_mask(x, x + char_width_1, cL, cR);
			mask2 = 0;
		}
		// do the display
		screen_pos = y0 * disp_width + x;

		p = char_data + y_offset;
		for (h = y_offset; h < char_height; h++) {
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

		// extra two bit for overwidth characters (up to 10 pixel supported for unicode)
		// if the character height is smaller than 10, not all is needed; but we do this anyway!
		if (char_width_1 > 8 && mask2 != 0) {
			int dat = 0;

			p = char_data + 12;
			screen_pos = y * disp_width + x + 8;

			dat = *p++ & mask2;
			// vertical clipping
			if (v_clip) dat &= get_v_mask(y_offset, char_height, 0, 4);
			if (dat != 0) {
				if (dat & 0x80) textur[screen_pos + 0] = color;
				if (dat & 0x40) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x20) textur[screen_pos + 0] = color;
				if (dat & 0x10) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x08) textur[screen_pos + 0] = color;
				if (dat & 0x04) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x02) textur[screen_pos + 0] = color;
				if (dat & 0x01) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
			} else {
				screen_pos += disp_width * 4;
			}

			dat = *p++ & mask2;
			// vertical clipping
			if (v_clip) dat &= get_v_mask(y_offset, char_height, 4, 8);
			if (dat != 0) {
				if (dat & 0x80) textur[screen_pos + 0] = color;
				if (dat & 0x40) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x20) textur[screen_pos + 0] = color;
				if (dat & 0x10) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x08) textur[screen_pos + 0] = color;
				if (dat & 0x04) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x02) textur[screen_pos + 0] = color;
				if (dat & 0x01) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
			} else {
				screen_pos += disp_width * 4;
			}

			dat = *p++ & mask2;
			// vertical clipping
			if (v_clip) dat &= get_v_mask(y_offset, char_height, 8, 12);
			if (dat != 0) {
				if (dat & 0x80) textur[screen_pos + 0] = color;
				if (dat & 0x40) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x20) textur[screen_pos + 0] = color;
				if (dat & 0x10) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x08) textur[screen_pos + 0] = color;
				if (dat & 0x04) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat & 0x02) textur[screen_pos + 0] = color;
				if (dat & 0x01) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
			}
		}
		// next char: screen width
		x += char_width_2;
	}

	if (dirty) {
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

	display_text_proportional_len_clip(xpos + 2, ypos - halfheight + 1, text, ALIGN_LEFT, text_farbe, FALSE, true, -1, false);
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
	display_text_proportional_len_clip(xpos + 2, ypos - 5 + (12 - large_font_height) / 2, text, ALIGN_LEFT, text_farbe, FALSE, true, -1, true);
}


/**
 * Zaehlt Vorkommen ein Buchstabens in einem String
 * @author Hj. Malthaner
 */
int count_char(const char *str, const char c)
{
	int count = 0;

	while (*str != '\0') count += (*str++ == c);
	return count;
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
		char *next;

		do {
			next = strchr(buf, '\n');
			display_text_proportional_len_clip(
				x, y, buf,
				ALIGN_LEFT, color, TRUE,
				true, next != NULL ? next - buf : -1, true
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

	dr_flush();

	// swap tile buffers
	tmp = tile_dirty_old;
	tile_dirty_old = tile_dirty;
	tile_dirty = tmp;
	memset(tile_dirty, 0, tile_buffer_length);
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
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int use_shm, int do_sync, int full_screen)
{
	int parameter[2];

	parameter[0] = use_shm;
	parameter[1] = do_sync;

	dr_os_init(2, parameter);

	// make sure it something of 16 (also better for caching ... )
	width = (width + 15) & 0x7FF0;

	if (dr_os_open(width, height, full_screen)) {
		disp_width = width;
		disp_height = height;

		textur = dr_textur_init();

		// init, load, and check fonts
		large_font.screen_width = NULL;
		large_font.char_data = NULL;
		display_load_font(FONT_PATH_X "prop.fnt", true);
#ifdef USE_SMALL_FONT
		small_font.screen_width = NULL;
		small_font.char_data = NULL;
		display_load_font(FONT_PATH_X "4x7.hex", false);
		display_check_fonts();
#endif

		load_palette(PALETTE_PATH_X "special.pal", special_pal);
		display_set_color(1);
	} else {
		puts("Error  : can't open window!");
		exit(-1);
	}

	// allocate dirty tile flags
	tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

	tile_dirty     = guarded_malloc(tile_buffer_length);
	tile_dirty_old = guarded_malloc(tile_buffer_length);

	memset(tile_dirty,     255, tile_buffer_length);
	memset(tile_dirty_old, 255, tile_buffer_length);

	display_setze_clip_wh(0, 0, disp_width, disp_height);

	display_day_night_shift(0);
	display_set_player_color(0);

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


/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
int simgraph_exit()
{
	guarded_free(tile_dirty);
	guarded_free(tile_dirty_old);
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

		dr_textur_resize((unsigned short**)&textur, disp_width, disp_height);

		tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

		tile_dirty     = guarded_malloc(tile_buffer_length);
		tile_dirty_old = guarded_malloc(tile_buffer_length);

		memset(tile_dirty,     255, tile_buffer_length);
		memset(tile_dirty_old, 255, tile_buffer_length);

		display_setze_clip_wh(0, 0, disp_width, disp_height);
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
 * Laedt Einstellungen
 * @author Hj. Malthaner
 */
void display_laden(void* file, int zipped)
{
	int i;

	if (zipped) {
		char line[80];
		char *ptr = line;

		gzgets(file, line, sizeof(line));

		light_level = atoi(ptr);
		while (*ptr && *ptr++ != ' ') {}
		color_level = atoi(ptr);
		while (*ptr && *ptr++ != ' ') {}
		night_shift = atoi(ptr);

		gzgets(file, line, sizeof(line));
		i = atoi(line);
	} else {
		fscanf(file, "%d %d %d\n", &light_level, &color_level, &night_shift);
		fscanf(file, "%d\n", &i);
	}
	if (i < 0 || i > 15) i = 0;
	display_set_light(light_level);
	display_set_color(color_level);
	display_set_player_color(0);
}


/**
 * Speichert Einstellungen
 * @author Hj. Malthaner
 */
void display_speichern(void* file, int zipped)
{
	if (zipped) {
		gzprintf(file, "%d %d %d\n", light_level, color_level, night_shift);
		gzprintf(file, "%d\n", selected_player_color_set);
	} else {
		fprintf(file, "%d %d %d\n", light_level, color_level, night_shift);
		fprintf(file, "%d\n", selected_player_color_set);
	}
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
