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

#include "pathes.h"
#include "simtypes.h"
#include "simconst.h"
#include "simsys.h"
#include "simmem.h"
#include "simdebug.h"
#include "besch/bild_besch.h"
#include "unicode.h"

//#include "utils/writepcx.h"
//#include "utils/image_encoder.h"


#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define W_OK	2
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "simgraph.h"


/*
 * Hajo: RGB 888 padded to 32 bit
 */
typedef unsigned int   PIXRGB;


/*
 * Hajo: RGB 1555
 */
typedef uint16 PIXVAL;


// --------------      static data    --------------

#ifdef USE_SOFTPOINTER
static int softpointer = -1;
#endif
static int standard_pointer = -1;


/*
 * die icon leiste muss neu gezeichnet werden wenn der
 * Mauszeiger darueber schwebt
 */
int old_my = -1;


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


/*
 * Hajo: pixel value type, RGB 555
 */
// typedef unsigned short PIXVAL;


static unsigned char day_pal[256 * 3];


/*
 * Hajo: player colors daytime RGB values
 * 16 sets of 16 colors each. At the moment only 7 of the
 * 16 colors of a set are used.
 * Last 16 colors are recolored player 0 color set
 */
static unsigned char special_pal[256 * 3 + 16 * 3];


#define RGBMAPSIZE (0x8000 + 29)


/*
 * Hajo: mapping tables for RGB 555 to actual output format
 * plus the special (player, day&night) colors appended
 *
 * 0x0000 - 0x7FFF: RGB colors
 * 0x8000 - 0x800F: Player colors
 * 0x8010 -       : Day&Night special colors
 */
static PIXVAL rgbmap_day_night[RGBMAPSIZE];


/*
 * Hajo: same as rgbmap_day_night, but allways daytime colors
 */
static PIXVAL rgbmap_all_day[RGBMAPSIZE];


/**
 * Hajo:used by pixel copy functions, is one of rgbmap_day_night
 * rgbmap_all_day
 */
static PIXVAL *rgbmap_current = 0;


/*
 * Hajo: mapping tables for Simutrans colormap to actual output format
 */
static PIXVAL rgbcolormap[256];


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - day&night mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_day_night[256];


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_all_day[256];


/**
 * Hajo:used by pixel copy functions, is one of specialcolormap_day_night
 * specialcolormap_all_day
 */
static PIXVAL* specialcolormap_current = 0;


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

	unsigned len; // base image data size (used for allocation purposes only)
	unsigned char recode_flags[4]; // first byte: needs recode, second byte: code normal, second byte: code for player1, third byte: flags

	PIXVAL* data; // current data, zoomed and adapted to output format RGB 555 or RGB 565

	PIXVAL* zoom_data; // zoomed original data

	PIXVAL* base_data; // original image data

	PIXVAL* player_data; // current data coded for player1 (since many building belong to him)
};

// offsets in the recode array
#define NEED_REZOOM (0)
#define NEED_NORMAL_RECODE (1)
#define NEED_PLAYER_RECODE (2)
#define FLAGS (3)

// flags for recoding
#define FLAG_ZOOMABLE (1)
#define FLAG_PLAYERCOLOR (2)


static sint16 disp_width  = 640;
static sint16 disp_height = 480;


/*
 * Image table
 */
static struct imd* images = NULL;

/*
 * Number of loaded images
 */
static unsigned anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static unsigned alloc_images = 0;


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


/*
 * Hajo: tile raster width
 */
int tile_raster_width = 16;
static int base_tile_raster_width = 16;

/*
 * Hajo: Zoom factor
 */
static int zoom_factor = 1;


/* changes the raster width after loading */
int display_set_base_raster_width(int new_raster)
{
	int old = base_tile_raster_width;
	base_tile_raster_width = new_raster;
	tile_raster_width = new_raster / zoom_factor;
	return old;
}


// -------------- Function prototypes --------------
void display_img_nc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL*);

static void rezoom();
static void recode();

static void calc_base_pal_from_night_shift(const int night);

/**
 * Convert a certain image data to actual output data
 * @author Hj. Malthaner
 */
static void recode_img_src_target(KOORD_VAL h, PIXVAL *src, PIXVAL *target);

#ifdef NEED_PLAYER_RECODE
/**
 * Convert a certain image data to actual output data
 * @author prissi
 */
static void recode_img_src_target_color(const KOORD_VAL h, PIXVAL *src, PIXVAL *target, const unsigned char color);
#endif


/**
 * If arguments are supposed to be changed during the call
 * use this makro instead of pixcopy()
 */
//#define PCPY(d, s, l) while (l--) *d++ = rgbmap[*s++];
//#define PCPY(d, s, l) while (l--) *d++ = *s++;
//#define PCPY(d, s, l) if (runlen) do {*d++ = *s++; } while (--l)

// Hajo: unrolled loop is about 5 percent faster
#define PCPY(d, s, l) \
	if (l & 1) { \
		*d++ = *s++; \
		l--; \
	} \
	{ \
		unsigned long *ld = (unsigned long*)d; \
		const unsigned long *ls = (const unsigned long*)s; \
		d += l; \
		s += l; \
		l >>= 1; \
		while (l--) { \
			*ld++ = *ls++; \
		} \
	}


// --------------      Functions      --------------



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
#if 0
	recode(); //already handled by rezoom
#endif
}


#ifdef _MSC_VER
#	define inline _inline
#endif


static inline void mark_tile_dirty(const int x, const int y)
{
	const int bit = x+y*tiles_per_line;

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
	// Hajo: inside display ?
	if (x2 >= 0 && y2 >= 0 && x1 < disp_width && y1 < disp_height) {
		if (x1 < 0) x1 = 0;
		if (y1 < 0) y1 = 0;
		if (x2 >= disp_width)  x2 = disp_width  - 1;
		if (y2 >= disp_height) y2 = disp_height - 1;

		mark_rect_dirty_nc(x1, y1, x2, y2);
	}
}


/**
 * Convert image data to actual output data
 * @author Hj. Malthaner
 */
static void recode()
{
	unsigned n;

	for (n = 0; n < anz_images; n++) {
		// tut jetzt on demand recode_img() für jedes Bild einzeln
		images[n].recode_flags[NEED_NORMAL_RECODE] = 128;
		images[n].recode_flags[NEED_PLAYER_RECODE] = 128;
	}
}


/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_normal_img(const unsigned int n)
{
	PIXVAL *src = images[n].base_data;

	// then use the right data
	if (zoom_factor > 1) {
		if (images[n].zoom_data != NULL) src = images[n].zoom_data;
	}
	if (images[n].data == NULL) {
		images[n].data = (PIXVAL*)guarded_malloc(sizeof(PIXVAL) * images[n].len);
	}
	// now do normal recode
	recode_img_src_target(images[n].h, src, images[n].data);
	images[n].recode_flags[NEED_NORMAL_RECODE] = 0;
}


/**
 * Convert a certain image data to actual output data
 * @author Hj. Malthaner
 */
static void recode_img_src_target(KOORD_VAL h, PIXVAL *src, PIXVAL *target)
{
	if (h > 0) {
		do {
			unsigned char runlen = *target ++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				while (runlen--) {
					// now just convert the color pixels
					*target++ = rgbmap_day_night[*src++];
				}
			} while ((runlen = *target ++ = *src++));
		} while (--h);
	}
}


#ifdef NEED_PLAYER_RECODE

/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_color_img(const unsigned int n, const unsigned char color)
{
	PIXVAL *src = images[n].base_data;

	images[n].recode_flags[NEED_PLAYER_RECODE] = color;
	// then use the right data
	if (zoom_factor > 1) {
		if (images[n].zoom_data != NULL) src = images[n].zoom_data;
	}
	// second recode modi, for player one (city and factory AI)
	if (images[n].player_data == NULL) {
		images[n].player_data = (PIXVAL*)guarded_malloc(sizeof(PIXVAL) * images[n].len);
	}
	// contains now the player color ...
	recode_img_src_target_color(images[n].h, src, images[n].player_data, color << 2);
}


/**
 * Convert a certain image data to actual output data for a certain player
 * @author prissi
 */
static void recode_img_src_target_color(KOORD_VAL h, PIXVAL *src, PIXVAL *target, const unsigned char color)
{
	if (h > 0) {
		do {
			unsigned char runlen = *target ++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				// now just convert the color pixels
				while (runlen--) {
					PIXVAL pix = *src++;

					if ((0x8000 ^ pix) <= 0x000F) {
						*target++ = specialcolormap_day_night[((unsigned char)pix & 0x0F) + color];
					} else {
						*target++ = rgbmap_day_night[pix];
					}
				}
				// next clea run or zero = end
			} while ((runlen = *target ++ = *src++));
		} while (--h);
	}
}

#endif


/**
 * Rezooms all images
 * @author Hj. Malthaner
 */
static void rezoom()
{
	unsigned n;
	for (n = 0; n < anz_images; n++) {
		images[n].recode_flags[NEED_REZOOM] = (images[n].recode_flags[FLAGS] & FLAG_ZOOMABLE) != 0 && images[n].base_h > 0;
		images[n].recode_flags[NEED_NORMAL_RECODE] = 128;
		images[n].recode_flags[NEED_PLAYER_RECODE] = 128;	// color will be set next time
	} // for
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
		images[n].h = images[n].base_h/zoom_factor;
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
				while (runlen--) *p++ = 0x73FE;

				// decode line
				runlen = *src++;
				color -= runlen;
				do {
					// clear run
					while (runlen--) *p++ = 0x73FE;
					// color pixel
					runlen = *src++;
					color += runlen;
					while (runlen--) *p++ = *src++;
				} while ((runlen = *src++));

				if (y_left == 0 || last_color < color) {
					// required; but if the following are longer, take them instead (aviods empty pixels)
					// so we have to set/save the beginning
					unsigned char i, step=0;

					if (y_left == 0) {
						last_dest = dest;
					} else {
						dest = last_dest;
					}

					// encode this line
					do {
						// check length of transparent pixels
						for (i = 0; line[step] == 0x73FE && step < imgw; i++, step += zoom_factor) {}
						*dest++ = i;
						// chopy for non-transparent
						for (i = 0; line[step] != 0x73FE && step < imgw; i++, step += zoom_factor) {
							dest[i + 1] = line[step];
						}
						*dest++ = i;
						dest += i;
					} while (step < imgw);
					// mark line end
					*dest++ = 0;
					if (y_left == 0) {
						// ok now reset countdown counter
						y_left = zoom_factor;
					}
					last_color = color;
				}

				// countdown line skip counter
				y_left --;
			} while (--h);

			// something left?
			{
				const unsigned zoom_len = dest - images[n].zoom_data;
				if (zoom_len > images[n].len) {
					printf("*** FATAL ***\nzoom_len (%i) > image_len (%i)", zoom_len, images[n].len);
					fflush(NULL);
					exit(0);
				}
				if (zoom_len == 0) images[n].h = 0;
			}
		} else {
			if (images[n].w <= 0) {
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


static int nibble(char c)
{
	if (c > '9') {
		return 10 + c - 'A';
	} else {
		return c - '0';
	}
}


/* decodes a single line line of a character
 * @author prissi
 */
static void dsp_decode_bdf_data_row(unsigned char *data, int y, int g_width, char *str)
{
	unsigned char data2;
	char	buf[3];

	buf[0] = str[0];
	buf[1] = str[1];
	buf[2] = 0;
	data[y] = (unsigned char)strtol(buf, NULL ,16);
	// read second byte but use only first two nibbles
	if (g_width > 8) {
		buf[0] = str[2];
		buf[1] = str[3];
		buf[2] = 0;
		data2 = (unsigned char)(strtol(buf, NULL, 16) & 0xC0);
		data[12 + (y / 4)] |= data2 >> (2 * (y & 0x03));
	}
}


/**
 * reads a single character
 * @author prissi
 */
static int dsp_read_bdf_glyph(FILE *fin, unsigned char *data, unsigned char *screen_w, bool allow_256up, int f_height, int f_desc)
{
	char str[256];
	int	char_nr = 0;
	int g_width, h, c, g_desc;
	int	d_width = 0;
	int y;

	str[255] = 0;
	while (!feof(fin)) {
		fgets(str, 255, fin);

		// endcoding (char number) in decimal
		if (strncmp(str, "ENCODING", 8) == 0) {
			char_nr = atoi(str + 8);
			if ((!allow_256up && char_nr > 255) || char_nr <= 0 || char_nr >= 65536) {
				printf("Unexpected character (%i) for %i character font!\n", char_nr, allow_256up ? 65536 : 256);
				char_nr = 0;
				continue;
			}
			continue;
		}

		// information over size and coding
		if (strncmp(str, "BBX", 3) == 0) {
			sscanf(str + 3, "%d %d %d %d", &g_width, &h, &c, &g_desc);
			continue;
		}

		// information over size and coding
		if (strncmp(str, "DWIDTH", 6) == 0) {
			d_width = atoi(str + 6);
			continue;
		}

		// start if bitmap data
		if (strncmp(str, "BITMAP", 6) == 0) {
			const int top = f_height + f_desc - h - g_desc;

			// set place for high nibbles to zero
			data[16 * char_nr + 12] = 0;
			data[16 * char_nr + 13] = 0;
			data[16 * char_nr + 14] = 0;

			// maximum size 10 pixels
			h += top;
			if (h > 12) h = 12;

			// read for height times
			for (y = top; y < h; y++) {
				fgets(str, 255, fin);
				dsp_decode_bdf_data_row(data + char_nr * 16, y, g_width, str);
			}
			continue;
		}

		// finally add width information (width = 0: not there!)
		if (strncmp(str, "ENDCHAR", 7) == 0) {
			data[16 * char_nr + 15] = g_width;
			if (d_width == 0) {
				// no screen width: should not happen, but we can recover
				printf("BDF warning: %i has no screen width assigned!\n", char_nr);
				d_width = g_width + 1;
			}
			screen_w[char_nr] = g_width;
			return char_nr;
			continue;
		}
	}
	return 0;
}


/**
 * reads a single character
 * @author prissi
 */
static bool dsp_read_bdf_font(FILE *fin, font_type *font)
{
	char str[256];
	unsigned char *data = NULL, *screen_widths = NULL;
	int	f_width, f_height, f_lead, f_desc;
	int f_chars = 0;
	int h;

	str[255] = 0;
	while (!feof(fin)) {
		fgets(str, 255, fin);

		if (strncmp(str, "FONTBOUNDINGBOX", 15 ) == 0) {
			sscanf(str + 15, "%d %d %d %d", &f_width, &f_height, &f_lead, &f_desc);
			continue;
		}

		if (strncmp(str, "CHARS", 5) == 0) {
			f_chars = atoi(str + 5);
			if (f_chars > 255) {
				f_chars = 65536;
			} else {
				// normal 256 chars
				f_chars = 256;
			}
			data = (unsigned char*)calloc(f_chars, 16);
			if (data == NULL) {
				printf( "No enough memory for font allocation!\n" );
				fflush(NULL);
				return false;
			}
			screen_widths = (unsigned char*)calloc(f_chars, 1);
			if (screen_widths == NULL) {
				free(data);
				printf("No enough memory for font allocation!\n");
				fflush(NULL);
				return false;
			}
			data[32 * 16] = 0; // screen width of space
			if (f_height >= 6) {
				if (f_height < 24) {
					screen_widths[32] = f_height / 2;
				} else {
					screen_widths[32] = 12;
				}
			} else {
				screen_widths[32] = 3;
			}
			continue;
		}

		if (strncmp(str, "STARTCHAR", 9) == 0 && f_chars > 0) {
			dsp_read_bdf_glyph(fin, data, screen_widths, f_chars > 256, f_height, f_desc);
			continue;
		}
	}

	// ok, was successful?
	if (f_chars > 0) {
		// init default char for missing characters (just a box)
		screen_widths[0] = 8;

		data[0] = 0x7E;
		for (h = 1; h < f_height - 2; h++) data[h] = 0x42;
		data[h++] = 0x7E;
		for (;  h < 15; h++) data[h] = 0;
		data[15] = 7;

		// now free old and assign new
		if (font->screen_width != NULL) free(font->screen_width);
		if (font->char_data    != NULL) free(font->char_data);
		font->screen_width = screen_widths;
		font->char_data    = data;
		font->height       = f_height;
		font->descent      = f_height + f_desc;
		font->num_chars    = f_chars;
		return true;
	}
	return false;
}


/**
 * Loads the fonts (true for large font)
 * @author prissi
 */
bool display_load_font(const char *fname, bool large)
{
	FILE *f = NULL;
	unsigned char c;
	font_type *fnt = large ? &large_font : &small_font;

	f = fopen(fname, "rb");
	if (f == NULL) {
		printf("Error: Cannot open '%s'\n", fname);
		fflush(NULL);
		return false;
	}
	c = getc(f);

	// binary => the assume dumpe prop file
	if (c < 32) {
		// read classical prop font
		unsigned char npr_fonttab[3074];
		int i;

		rewind(f);
		printf("Loading font '%s'\n", fname);
		fflush(NULL);

		if (fread(npr_fonttab, 1, 3072, f) != 3072) {
			printf("Error: %s wrong size for old format prop font!\n",fname);
			fflush(NULL);
			fclose(f);
			return false;
		}
		fclose(f);
		// convert to new standard font
		if (fnt->screen_width != NULL) free(fnt->screen_width);
		if (fnt->char_data    != NULL) free(fnt->char_data);
		strcpy(fnt->name, fname);
		fnt->screen_width = (unsigned char*)malloc(256);
		fnt->char_data    = (unsigned char*)malloc(16 * 256);
		fnt->num_chars    = 256;
		fnt->height       = 10;
		fnt->descent      = -1;

		for (i = 0; i < 256; i++) {
			int j;

			fnt->screen_width[i] = npr_fonttab[256 + i];
			for (j = 0; j < 10; j++) {
				fnt->char_data[i * 16 + j] = npr_fonttab[512 + i * 10 + j];
			}
			for (; j < 15; j++) {
				fnt->char_data[i * 16 + j] = 0;
			}
			fnt->char_data[16 * i + 15] = npr_fonttab[i];
		}
		fnt->screen_width[32] = 4;
		fnt->char_data[16 * 32 + 15] = 0;
		printf("%s sucessful loaded as old format prop font!\n",fname);
		fflush(NULL);
		return true;
	}

	// load old hex font format
	if (c == '0') {
		unsigned char dr_4x7font[7 * 256];
		int fh = 7;
		char buf [80];
		int i;
		int  n, line;
		char *p;

		rewind(f);

		while (fgets(buf, 79, f) != NULL) {
			sscanf(buf, "%4x", &n);
			p = buf + 5;

			for (line = 0; line < fh; line++) {
				int val = nibble(p[0]) * 16 + nibble(p[1]);

				dr_4x7font[n * fh + line] = val;
				p += 2;
			}
		}
		fclose(f);
		// convert to new standard font
		if (fnt->screen_width != NULL) free(fnt->screen_width);
		if (fnt->char_data    != NULL) free(fnt->char_data);
		strcpy(fnt->name, fname);
		fnt->screen_width = (unsigned char*)malloc(256);
		fnt->char_data    = (unsigned char*)malloc(16 * 256);
		fnt->num_chars    = 256;
		fnt->height       = 7;
		fnt->descent      = -1;

		for (i = 0; i < 256; i++) {
			int j;

			fnt->screen_width[i] = 4;
			for (j = 0; j < 7; j++) {
				fnt->char_data[i * 16 + j] = dr_4x7font[i * 7 + j];
			}
			for (; j < 15; j++) {
				fnt->char_data[i * 16 + j] = 0xFF;
			}
			fnt->char_data[i * 16 + 15] = 3;
		}
		printf("%s sucessful loaded as old format hex font!\n",fname);
		fflush(NULL);
		return true;
	}

	/* also, read unicode font.dat
	 * @author prissi
	 */
	printf("Loading BDF font '%s'\n", fname);
	fflush(NULL);
	if (dsp_read_bdf_font(f, fnt)) {
		printf("Loading BDF font %s ok\n", fname);
		fflush(NULL);
		fclose(f);
		if (large) large_font_height = large_font.height;
		return true;
	}
	return false;
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
void	display_check_fonts(void)
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
	char fname[256];
	FILE *file;

	strcpy(fname, filename);

	file = fopen(fname,"rb");

	if (file) {
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
		fprintf(stderr, "Error: can't open file '%s' for reading\n", fname);
	}

	return file != NULL;
}


/**
 * Loads the special colors map
 * @author Hj. Malthaner
 */
static int load_special_palette()
{
	const char *fname = PALETTE_PATH_X "special.pal";

	FILE *file = fopen(fname, "rt");

	if (file) {
		int i, R,G,B;
		unsigned char* p = special_pal;

		// Hajo: skip number of colors entry
		fscanf(file, "%d\n", &i);

		// Hajo: read colors
		for (i = 0; i < 256; i++) {
			fscanf(file, "%d %d %d\n", &R, &G, &B);

			*p++ = R;
			*p++ = G;
			*p++ = B;
		}

		fclose(file);
	} else {
		fprintf(stderr, "Error: can't open file '%s' for reading\n", fname);
	}

	return file != NULL;
}


sint16 display_get_width()
{
	return disp_width;
}


sint16 display_get_height()
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
int display_get_light()
{
	return light_level;
}


/**
 * Setzt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
void display_set_light(int new_light_level)
{
	light_level = new_light_level;
	calc_base_pal_from_night_shift(night_shift);
	recode();
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


#if 0
/* Hajos variant */
static void calc_base_pal_from_night_shift(const int night)
{
	const int day = 4 - night;
	int i;

	for (i = 0; i < (1 << 15); i++) {
		int R;
		int G;
		int B;

		// RGB 555 input
		R = ((i & 0x7C00) >> 7) - night * 24;
		G = ((i & 0x03E0) >> 2) - night * 24;
		B = ((i & 0x001F) << 3) - night * 16;

		rgbmap_day_night[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// player 0 colors, unshaded
	for (i = 0; i < 4; i++) {
		const int R = day_pal[i * 3];
		const int G = day_pal[i * 3 + 1];
		const int B = day_pal[i * 3 + 2];

		rgbcolormap[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// player 0 colors, shaded
	for (i = 0; i < 4; i++) {
		const int R = day_pal[i * 3]     - night * 24;
		const int G = day_pal[i * 3 + 1] - night * 24;
		const int B = day_pal[i * 3 + 2] - night * 16;

		specialcolormap_day_night[i] = rgbmap_day_night[0x8000 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
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

		rgbmap_day_night[0x8004 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}


	// transform special colors
	for (i = 4; i < 32; i++) {
		int R = special_pal[i * 3]     - night * 24;
		int G = special_pal[i * 3 + 1] - night * 24;
		int B = special_pal[i * 3 + 2] - night * 16;

		specialcolormap_day_night[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// convert to RGB xxx
	recode();
}
#endif


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
	const double RG_nihgt_multiplier = pow(0.73,night);
	const double B_nihgt_multiplier = pow(0.82,night);
#endif

	const double RG_nihgt_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_nihgt_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);


	for (i = 0; i < 0x8000; i++) {
		// (1<<15) this is total no of all possible colors in RGB555)

		int R;
		int G;
		int B;

		// RGB 555 input
		R = ((i & 0x7C00) >> 7);
		G = ((i & 0x03E0) >> 2);
		B = ((i & 0x001F) << 3);
		// lines generate all possible colors in 555RGB code - input
		// however the result is in 888RGB - 8bit per channel

		R = (int)(R * RG_nihgt_multiplier);
		G = (int)(G * RG_nihgt_multiplier);
		B = (int)(B * B_nihgt_multiplier);

		rgbmap_day_night[i] = get_system_color(R, G, B);
	}


	// player 0 colors, unshaded
	for (i = 0; i < 4; i++) {
		const int R = day_pal[i * 3];
		const int G = day_pal[i * 3 + 1];
		const int B = day_pal[i * 3 + 2];

		rgbcolormap[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// player 0 colors, shaded
	for (i = 0; i < 16; i++) {
		const int R = (int)(special_pal[(256 + i) * 3]     * RG_nihgt_multiplier);
		const int G = (int)(special_pal[(256 + i) * 3 + 1] * RG_nihgt_multiplier);
		const int B = (int)(special_pal[(256 + i) * 3 + 2] * B_nihgt_multiplier);

		specialcolormap_day_night[i] = rgbmap_day_night[0x8000 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);

		specialcolormap_all_day[i] = get_system_color(
			special_pal[(256 + i) * 3],
			special_pal[(256 + i) * 3 + 1],
			special_pal[(256 + i) * 3 + 2]
		);
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

		rgbmap_day_night[0x8010 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}


	// transform special colors
	for (i = 16; i < 256; i++) {
		int R = (int)(special_pal[i * 3]     * RG_nihgt_multiplier);
		int G = (int)(special_pal[i * 3 + 1] * RG_nihgt_multiplier);
		int B = (int)(special_pal[i * 3 + 2] * B_nihgt_multiplier);

		specialcolormap_day_night[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// convert to RGB xxx
	recode();
}


void display_day_night_shift(int night)
{
	if (night != night_shift) {
		night_shift = night;
		calc_base_pal_from_night_shift(night);
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
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
		day_pal[i * 3]     = special_pal[entry + i * 3 * 2];
		day_pal[i * 3 + 1] = special_pal[entry + i * 3 * 2 + 1];
		day_pal[i * 3 + 2] = special_pal[entry + i * 3 * 2 + 2];
	}

	// New set of 16 player colors, unshaded
	for (i = 0; i < 16; i++) {
		special_pal[(256 + i) * 3]     = special_pal[entry + i * 3];
		special_pal[(256 + i) * 3 + 1] = special_pal[entry + i * 3 + 1];
		special_pal[(256 + i) * 3 + 2] = special_pal[entry + i * 3 + 2];
	}

	calc_base_pal_from_night_shift(night_shift);

	mark_rect_dirty_nc(0,0, disp_width-1, disp_height-1);
}


/**
 * Fügt ein Image aus anderer Quelle hinzu
 */
void register_image(struct bild_besch_t *bild)
{
	struct imd* image;

	/* valid image? */
	if (bild->len == 0 || bild->h == 0) {
		fprintf(stderr, "Warning: ignoring image %d because of missing data\n", anz_images);
		bild->bild_nr = -1;
		return;
	}

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

	// since we do not recode them, we can work with the original data
	image->base_data = (PIXVAL*)(bild + 1);

	image->recode_flags[FLAGS] = bild->zoomable & FLAG_ZOOMABLE;

	// does this image have color?
	if (bild->h > 0) {
		int h = bild->h;
		const PIXVAL *src = image->base_data;

		do {
			*src++; // offset of first start
			do {
				PIXVAL runlen;

				for (runlen = *src++; runlen != 0; runlen--) {
					PIXVAL pix = *src++;

					if (0x8000 <= pix && pix <= 0x800F) {
						image->recode_flags[FLAGS] |= FLAG_PLAYERCOLOR;
						return;
					}
				}
				// next clear run or zero = end
			} while (*src++ != 0);
		} while (--h != 0);
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
int gib_maus_x()
{
	return sys_event.mx;
}


/**
 * Holt Maus y-Position
 * @author Hj. Malthaner
 */
int gib_maus_y()
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
static short clip_lr(KOORD_VAL *x, KOORD_VAL *w, const KOORD_VAL left, const KOORD_VAL right)
{
	const KOORD_VAL l = *x;      // leftmost pixel
	const KOORD_VAL r = *x + *w; // rightmost pixel

	if (l > right || r < left) {
		*w = 0;
		return FALSE;
	}

	// there is something to show.
	if (l < left)  {
		*w -= left - l;
		*x = left;
	}
	if (r > right) {
		*w -= r - right;
	}
	return *w > 0;
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

	clip_rect.xx = x + w;
	clip_rect.yy = y + h;
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

	while (dest < end) {
		*dest ++ = *src++;
	}
}


/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static inline void colorpixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end, const COLOR_VAL color)
{
	while (src < end) {
		PIXVAL pix = *src++;

		if ((0x8000 ^ pix) <= 0x000F) {
			*dest++ = specialcolormap_current[((unsigned char)pix & 0x0F) + color];
		} else {
			*dest++ = rgbmap_current[pix];
		}
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
					const int len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);

					pixcopy(tp+xpos+left, sp+left, len-left);
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
#ifdef USE_C
				PCPY(p, sp, runlen);
#else
				asm volatile (
					"cld\n\t"
					// rep movsw and we would be finished, but we unroll
					// uneven number of words to copy
					"testb $1, %%cl\n\t"
					"je .Lrlev\n\t"
					// Copy first word
					// *p++ = *sp++;
					"movsw\n\t"
					".Lrlev:\n\t"
					// now we copy long words ...
					"shrl %2\n\t"
					"rep\n\t"
					"movsd\n\t"
					: "+D" (p), "+S" (sp), "+c" (runlen)
					:
					: "cc"
				);
#endif
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
void display_img_aux(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool use_player)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		PIXVAL *sp;
		KOORD_VAL h, reduce_h, skip_lines;

		if (use_player) {
			sp = images[n].player_data;
			if (sp == NULL) {
				printf("CImg %i failed!\n", n);
				return;
			}
		} else {
			if (images[n].recode_flags[NEED_REZOOM]) {
				rezoom_img(n);
				recode_normal_img(n);
			} else if (images[n].recode_flags[NEED_NORMAL_RECODE]) {
				recode_normal_img(n);
			}
			sp = images[n].data;
			if (sp == NULL) {
				printf("Img %i failed!\n", n);
				return;
			}
		}
		// now, since zooming may have change this image
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
				sp ++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL w = images[n].w;

			// use horzontal clipping or skip it?
			if (xp + x >= clip_rect.x && xp + x + w - 1 <= clip_rect.xx) {
				// marking change?
				if (dirty) mark_rect_dirty_nc(xp + x, yp, xp + x + w - 1, yp + h - 1);
				display_img_nc(h, xp, yp, sp);
			} else if (xp <= clip_rect.xx && xp + x + w > clip_rect.x) {
				display_img_wc(h, xp, yp, sp);
				// since height may be reduced, start marking here
				if (dirty) mark_rect_dirty_wc(xp + x, yp, xp + x + w - 1, yp + h - 1);
			}
		}
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * assumes height is ok and valid data are caluclated
 * @author hajo/prissi
 */
static void display_color_img_aux(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const COLOR_VAL color)
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
			} while(*sp);
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
					const int left = xpos >= clip_rect.x ? 0 : clip_rect.x - xpos;
					const int len  = clip_rect.xx-xpos > runlen ? runlen : clip_rect.xx - xpos;

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
		// Hajo: since the colors for player 0 are already right,
		// only use the expensive replacement routine for colored images
		// of other players
		// Hajo: player 1 does not need any recoloring, too
		if (daynight && (color < 4 || (images[n].recode_flags[FLAGS] & FLAG_PLAYERCOLOR) == 0)) {
			display_img_aux(n, xp, yp, dirty, false);
			return;
		}

		if (images[n].recode_flags[NEED_REZOOM]) {
			rezoom_img(n);
		}

		{
			// first test, if there is a cached version (or we can built one ... )
			const unsigned char player_flag = images[n].recode_flags[NEED_PLAYER_RECODE]&0x7F;
			if (daynight && (player_flag == 0 || player_flag == color)) {
				if (images[n].recode_flags[NEED_PLAYER_RECODE] == 128 || player_flag == 0) {
					recode_color_img(n, color);
				}
				// ok, now we could use the same faster code as for the normal images
				display_img_aux(n, xp, yp, dirty, true);
				return;
			}
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

			if (dirty) mark_rect_dirty_wc(xp + x, yp + y, xp + x + w - 1, yp + y + h - 1);
		}

		// Hajo: choose mapping table
		rgbmap_current = daynight ? rgbmap_day_night : rgbmap_all_day;
		specialcolormap_current = daynight ? specialcolormap_day_night : specialcolormap_all_day;
		// Hajo: Simutrans used to use only 4 shades per color
		// but now we use 16 player color, so we need to multiply
		// the color value here
		display_color_img_aux(n, xp, yp, color << 2);
	} // number ok
}


/**
 * the area of this image need update
 * @author Hj. Malthaner
 */
void display_mark_img_dirty(unsigned bild, int xp, int yp)
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
static void display_pixel(KOORD_VAL x, KOORD_VAL y, PIXVAL color)
{
	if (x >= clip_rect.x && x<=clip_rect.xx && y >= clip_rect.y && y<=clip_rect.yy) {
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
	if (clip_lr(&xp, &w, cL, cR) && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		int dx = disp_width - w;
		const PIXVAL colval = (color >= PLAYER_FLAG ? specialcolormap_all_day[color & 0xFF] : rgbcolormap[color]);
		const unsigned int longcolval = (colval << 16) | colval;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		do {
			unsigned int count = w;
#ifdef USE_C
			unsigned int* lp;

			if (count & 1) *p++ = longcolval;
			count >>= 1;
			lp = p;
			while (count-- != 0) *lp++ = longcolval;
			p = lp;
#else
			asm volatile (
				"cld\n\t"
				// uneven words to copy?
				// if(w&1)
				"testb $1,%%cl\n\t"
				"je .LrSev\n\t"
				// set first word
				"stosw\n\t"
				".LrSev:\n\t"
				// now we set long words ...
				"shrl %%ecx\n\t"
				"rep\n\t"
				"stosl"
				: "+D" (p), "+c" (count)
				: "a" (longcolval)
				: "cc"
			);
#endif

			p += dx;
		} while (--h != 0);
	}
}


void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty)
{
  display_fb_internal(xp, yp, w, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h,PLAYER_COLOR_VAL color, int dirty)
{
  display_fb_internal(xp, yp, w, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet vertikale Linie
 * @author Hj. Malthaner
 */
static void display_vl_internal(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	if (xp >= cL && xp <= cR &&  clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const PIXVAL colval = (color >= PLAYER_FLAG ? specialcolormap_all_day[color & 0xFF] : rgbcolormap[color]);

		if (dirty) mark_rect_dirty_nc(xp, yp, xp, yp+h-1);

		do {
			*p = colval;
			p += disp_width;
		} while (--h);
	}
}


void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp,yp,h,color,dirty,0,disp_width,0,disp_height);
}


void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp,yp,h,color,dirty,clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
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
		const COLOR_VAL *arr_src = arr;

		mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		if (xp == clip_rect.x) arr_src += xoff;
		if (yp == clip_rect.y) arr_src += yoff * arr_w;

		do {
			unsigned int ww = w;
#ifdef USE_C
			do {
				*p++ = rgbcolormap[*arr_src++];
				ww--;
			} while (ww > 0);
#else
			asm volatile (
				"cld\n"
				".Ldaw:\n\t"
				"lodsb\n\t"
				"movzbl %%al, %%eax\n\t"
#if defined(__MINGW32__) || defined(__CYGWIN__)
				"movw _rgbcolormap(%%eax,%%eax),%%ax\n\t"
#else
				"movw rgbcolormap(%%eax,%%eax),%%ax\n\t"
#endif
				"stosw\n\t"
				"loopl .Ldaw\n\t"
				: "+D" (p), "+S" (arr_src), "+c" (ww)
				:
				: "eax"
			);
#endif
			arr_src += arr_w - w;
			p += disp_width - w;
		} while (--h);
	}
}


// unicode save moving in strings
int get_next_char(const char* text, int pos)
{
	if (has_unicode) {
		return utf8_get_next_char((const utf8*)text, pos);
	}
	return pos + 1;
}


int get_prev_char(const char* text, int pos)
{
	if (pos <= 0) return 0;
	if (has_unicode) {
		return utf8_get_prev_char((const utf8 *)text, pos);
	}
	return pos - 1;
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
static const unsigned char left_byte_to_v_mask_array[5]  = { 0xFF, 0x3F, 0x0F, 0x03, 0x00};
static const unsigned char right_byte_to_v_mask_array[5] = { 0xFF, 0xFC, 0xF0, 0xC0, 0x00};

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
	const PIXVAL color = (color_index >= PLAYER_FLAG ? specialcolormap_all_day[color_index & 0xFF]: rgbcolormap[color_index]);
#ifndef USE_C
	// faster drawing with assembler
	const unsigned long color2 = (color << 16) | color;
#endif

	// TAKE CARE: Clipping area may be larger than actual screen size ...
	if (use_clipping) {
		cL = clip_rect.x;
		cR = clip_rect.xx;
		cT = clip_rect.y;
		cB = clip_rect.yy;
	} else {
		cL = 0;
		cR = disp_width;
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
#if 0
		char_height -= y_offset;
#endif
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
		// print unknow character?
		if (c >= fnt->num_chars || fnt->screen_width[c] == 0) {
			c = 0;
		}
		// get the data from the font
		char_width_1 = fnt->char_data[16*c+15];
		char_width_2 = fnt->screen_width[c];
		char_data = fnt->char_data+(16l*c);
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
		screen_pos = y0*disp_width + x;

		p = char_data + y_offset;
		for (h = y_offset; h < char_height; h++) {
			unsigned int dat = *p++ & mask1;
			PIXVAL* dst = textur + screen_pos;

#ifdef USE_C
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
#else
			// assemble variant of the above, using table and string instructions:
			// optimized for long pipelines ...
#			include "text_pixel.c"
#endif
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
				if (dat & 128) textur[screen_pos + 0] = color;
				if (dat &  64) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &  32) textur[screen_pos + 0] = color;
				if (dat &  16) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   8) textur[screen_pos + 0] = color;
				if (dat &   4) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   2) textur[screen_pos + 0] = color;
				if (dat &   1) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
			} else {
				screen_pos += disp_width * 4;
			}

			dat = *p++ & mask2;
			// vertical clipping
			if (v_clip) dat &= get_v_mask(y_offset, char_height, 4, 8);
			if (dat != 0) {
				if (dat & 128) textur[screen_pos + 0] = color;
				if (dat &  64) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &  32) textur[screen_pos + 0] = color;
				if (dat &  16) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   8) textur[screen_pos + 0] = color;
				if (dat &   4) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   2) textur[screen_pos + 0] = color;
				if (dat &   1) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
			} else {
				screen_pos += disp_width*4;
			}

			dat = *p++ & mask2;
			// vertical clipping
			if (v_clip) dat &= get_v_mask(y_offset, char_height, 8, 12);
			if (dat != 0) {
				if (dat & 128) textur[screen_pos + 0] = color;
				if (dat &  64) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &  32) textur[screen_pos + 0] = color;
				if (dat &  16) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   8) textur[screen_pos + 0] = color;
				if (dat &   4) textur[screen_pos + 1] = color;
				screen_pos += disp_width;
				if (dat &   2) textur[screen_pos + 0] = color;
				if (dat &   1) textur[screen_pos + 1] = color;
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
	int halfheight = (large_font_height / 2) + 1;

	display_fillbox_wh(xpos - 2, ypos - halfheight - 1 - hgt, width, 1,            ddd_farbe + 1, dirty);
	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt,     width, halfheight*2, ddd_farbe,     dirty);
	display_fillbox_wh(xpos - 2, ypos + halfheight - hgt,     width, 1,            ddd_farbe - 1, dirty);

	display_vline_wh(xpos - 2,         ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh(xpos + width - 3, ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos-halfheight + 1, text, ALIGN_LEFT, text_farbe, FALSE, true, -1, false);
}


/**
 * display text in 3d box with clipping
 * @author: hsiegeln
 */
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = (large_font_height / 2) + 1;

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
	display_text_proportional_len_clip(xpos + 2, ypos - 5 + (12 - large_font_height) / 2, text, ALIGN_LEFT, text_farbe,FALSE, true, -1, true);
}


/**
 * Zaehlt Vorkommen ein Buchstabens in einem String
 * @author Hj. Malthaner
 */
int count_char(const char *str, const char c)
{
	int count = 0;

	while (*str) {
		count += (*str++ == c);
	}
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
	if (buf && *buf) {
		char *next;

		do {
			next = strchr(buf,'\n');
			display_text_proportional_len_clip(
				x, y, buf,
				ALIGN_LEFT, color, TRUE,
				true, next ? next - buf : -1, true
			);
			buf = next + 1;
			y += LINESPACE;
		} while (next);
	}
}


//#define DEBUG_FLUSH_BUFFER

/**
 * copies only the changed areas to the screen using the "tile dirty buffer"
 * To get large changes, actually the current and the previous one is used.
 */
void display_flush_buffer()
{
	int x, y;
	unsigned char * tmp;

#ifdef USE_SOFTPOINTER
	if (softpointer != -1) {
		ex_ord_update_mx_my();
		display_color_img(standard_pointer, sys_event.mx, sys_event.my, 0, false, true);
	}
	old_my = sys_event.my;
#endif

#ifdef DEBUG_FLUSH_BUFFER
	for (y = 0; y < tile_lines - 1; y++) {
		x = 0;

		do {
			if (is_tile_dirty(x, y)) {
				const int xl = x;
				do {
					x++;
				} while(x < tiles_per_line && is_tile_dirty(x, y));

				display_vline_wh((xl << DIRTY_TILE_SHIFT) - 1, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, FALSE);
				display_vline_wh(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, FALSE);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, (x - xl) << DIRTY_TILE_SHIFT, 1, 80, FALSE);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT, (y << DIRTY_TILE_SHIFT) + DIRTY_TILE_SIZE - 1, (x - xl) << DIRTY_TILE_SHIFT, 1, 80, FALSE);
			}
			x++;
		} while (x < tiles_per_line);
	}
	dr_textur(0, 0, disp_width, disp_height);
#else
	for (y = 0; y < tile_lines; y++) {
		x = 0;

		do {
			if(is_tile_dirty(x, y)) {
				const int xl = x;
				do {
					x++;
				} while(x < tiles_per_line && is_tile_dirty(x, y));

				dr_textur(xl << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, (x - xl) << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE);
			}
			x++;
		} while (x < tiles_per_line);
	}
#endif

	dr_flush();

	// swap tile buffers
	tmp = tile_dirty_old;
	tile_dirty_old = tile_dirty;
	tile_dirty = tmp;
	// and clear it
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
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int use_shm, int do_sync, int full_screen)
{
	int parameter[2];
	int ok;

	parameter[0] = use_shm;
	parameter[1] = do_sync;

	dr_os_init(2, parameter);

	// make sure it something of 16 (also better for caching ... )
	width = (width + 15) & 0x7FF0;
	ok = dr_os_open(width, height, full_screen);

	if (ok) {
		unsigned int i;

		disp_width = width;
		disp_height = height;

		textur = dr_textur_init();

		// init, load, and check fonts
		large_font.screen_width = NULL;
		large_font.char_data = NULL;
		display_load_font( FONT_PATH_X "prop.fnt", true );
#ifdef USE_SMALL_FONT
		small_font.screen_width = NULL;
		small_font.char_data = NULL;
		display_load_font( FONT_PATH_X "4x7.hex", false );
		display_check_fonts();
#endif

		load_special_palette();
		display_set_color(1);

		for (i = 0; i < 256; i++) {
			rgbcolormap[i] = get_system_color(day_pal[i*3], day_pal[i*3+1], day_pal[i*3+2]);
		}
	} else {
		puts("Error  : can't open window!");
		exit(-1);
	}

	// allocate dirty tile flags
	tiles_per_line = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_lines = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length = (tile_lines*tiles_per_line+7) / 8;

	tile_dirty     = (unsigned char*)guarded_malloc(tile_buffer_length);
	tile_dirty_old = (unsigned char*)guarded_malloc(tile_buffer_length);

	memset(tile_dirty, 255, tile_buffer_length);
	memset(tile_dirty_old, 255, tile_buffer_length);

	display_setze_clip_wh(0, 0, disp_width, disp_height);

	display_day_night_shift(0);
	display_set_player_color(0);

	// Hajo: Calculate daylight rgbmap and save it for unshaded tile drawing
	calc_base_pal_from_night_shift(0);
	memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
	memcpy(specialcolormap_all_day, specialcolormap_day_night, 256 * sizeof(PIXVAL));

	printf("Init done.\n");
	fflush(NULL);

	return TRUE;
}


/**
 * Prueft ob das Grafikmodul schon init. wurde
 * @author Hj. Malthaner
 */
int is_display_init()
{
	return textur != NULL;
}


/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
int simgraph_exit()
{
	unsigned n;

	guarded_free(tile_dirty);
	guarded_free(tile_dirty_old);
	/* free images */
	for (n = 0;  n < anz_images; n++) {
		if (images[n].zoom_data != NULL) guarded_free(images[n].zoom_data);
		if (images[n].data      != NULL) guarded_free(images[n].data);
	}
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

		dr_textur_resize(&textur, disp_width, disp_height);

		// allocate dirty tile flags
		tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

		tile_dirty     = (unsigned char *)guarded_malloc(tile_buffer_length);
		tile_dirty_old = (unsigned char *)guarded_malloc(tile_buffer_length);

		memset(tile_dirty, 255, tile_buffer_length);
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
void display_laden(void * file, int zipped)
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
	if(i < 0 || i > 15) i = 0;
	display_set_light(light_level);
	display_set_color(color_level);
	display_set_player_color(i);
}


/**
 * Speichert Einstellungen
 * @author Hj. Malthaner
 */
void display_speichern(void * file, int zipped)
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
void display_direct_line(const KOORD_VAL x,const KOORD_VAL y,const KOORD_VAL xx,const KOORD_VAL yy, const PLAYER_COLOR_VAL color)
{
	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;
	const PIXVAL colval = (color >= PLAYER_FLAG ? specialcolormap_all_day[color & 0xFF] : rgbcolormap[color]);

	if (abs(dx) > abs(dy)) {
		steps = abs(dx);
	} else {
		steps = abs(dy);
	}

	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	for (i = 0; i <= steps; i++) {
		display_pixel(xp >> 16,yp >> 16, colval);
		xp += xs;
		yp += ys;
	}
}
