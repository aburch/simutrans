/*
 * Copyright 2010 Simutrans contributors
 * Available under the Artistic License (see license.txt)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "macros.h"
#include "simtypes.h"
#include "font.h"
#include "pathes.h"
#include "simconst.h"
#include "simsys.h"
#include "simmem.h"
#include "simdebug.h"
#include "besch/bild_besch.h"
#include "unicode.h"
#include "simticker.h"


#ifdef _MSC_VER
#	include <io.h>
#	define W_OK 2
#else
#	include <unistd.h>
#endif

// first: find out, which copy routines we may use!
#ifndef  __GNUC__
# undef USE_C
# define USE_C
# ifndef  _M_IX86
#  define ALIGN_COPY
# endif
#else
# if defined(USE_C)  ||  !defined(__i386__)
#  undef USE_C
#  define USE_C
#  if GCC_ATLEAST(4, 2) || !defined(__i386__)
#   define ALIGN_COPY
#   warning "Needs to use slower copy with GCC > 4.2.x"
#  endif
# endif
#endif

#if MULTI_THREAD>1
#include <pthread.h>

// currently just redrawing/rezooming
static pthread_mutex_t rezoom_recode_img_mutex;
#endif

#include "simgraph.h"

// undefine for debugging the update routines
//#define DEBUG_FLUSH_BUFFER

/*
 * Hajo: RGB 1555
 */
typedef uint16 PIXVAL;


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


/*
 * Hajo: Current clipping rectangle
 */
static clip_dimension clip_rect;

// and the variables for polygon clipping

/*
 * struct to hold the information about visible area
 * at screen line y
 * associated to some clipline
 */
struct xrange {
	int sx,sy;
	KOORD_VAL y;
	bool non_convex_active;
};

#define MAX_POLY_CLIPS 6
static xrange      xranges[MAX_POLY_CLIPS];
static uint8       clip_ribi[MAX_POLY_CLIPS];
static int number_of_clips =0;
static uint8 active_ribi = 15; // set all to active


/* Flag, if we have Unicode font => do unicode (UTF8) support! *
 * @author prissi
 * @date 29.11.04
 */
static bool has_unicode = false;

static font_type large_font = { 0, 0, 0, NULL, NULL };

// needed for resizing gui
int large_font_ascent = 9;
int large_font_total_height = 11;

#define MAX_PLAYER_COUNT (16)

#define RGBMAPSIZE (0x8000+LIGHT_COUNT+16)

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
 * Hajo: same as rgbmap_day_night, but always daytime colors
 */
static PIXVAL rgbmap_all_day[RGBMAPSIZE];


/**
 * Hajo:used by pixel copy functions, is one of rgbmap_day_night
 * rgbmap_all_day
 */
static PIXVAL *rgbmap_current = 0;


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


// offsets of first and second comany color
static uint8 player_offsets[MAX_PLAYER_COUNT][2];


/*
 * Hajo: Image map descriptor struture
 */
struct imd {
	sint16 base_x; // min x offset
	sint16 base_y; // min y offset
	sint16 base_w; //  width
	sint16 base_h; // height

	sint16 x; // current (zoomed) min x offset
	sint16 y; // current (zoomed) min y offset
	sint16 w; // current (zoomed) width
	sint16 h; // current (zoomed) height

	uint32 len; // base image data size (used for allocation purposes only)

	uint8 recode_flags; // divers flags for recoding
	uint8 player_flags; // 128 = free/needs recode, otherwise coded to this color in player_data

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


static KOORD_VAL disp_width  = 640;
static KOORD_VAL disp_actual_width  = 640;
static KOORD_VAL disp_height = 480;


/*
 * Image table
 */
static struct imd* images = NULL;

/*
 * Number of loaded images
 */
static image_id anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static image_id alloc_images = 0;


/*
 * Output framebuffer
 */
static PIXVAL* textur = NULL;


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


/*
 * Hajo: speical colors during daytime
 */
COLOR_VAL display_day_lights[LIGHT_COUNT*3] = {
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
 * Hajo: speical colors during nighttime
 */
COLOR_VAL display_night_lights[LIGHT_COUNT*3] = {
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


// the players colors and colors for simple drawing operations
// each eight colors are corresponding to a player color
static const COLOR_VAL special_pal[224*3]=
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


/*
 * Hajo: tile raster width
 */
KOORD_VAL tile_raster_width = 16;	// zoomed
KOORD_VAL base_tile_raster_width = 16;	// original

// Knightly : variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = NULL;
display_image_proc display_color = NULL;
display_blend_proc display_blend = NULL;
signed short current_tile_raster_width = 0;



/*
 * Hajo: Zoom factor
 */
#define MAX_ZOOM_FACTOR (9)
static uint32 zoom_factor = 3;
static sint32 zoom_num[MAX_ZOOM_FACTOR+1] = { 2, 3, 4, 1, 3, 5, 1, 3, 1, 1 };
static sint32 zoom_den[MAX_ZOOM_FACTOR+1] = { 1, 2, 3, 1, 4, 8, 2, 8, 4, 8 };



/* changes the raster width after loading */
KOORD_VAL display_set_base_raster_width(KOORD_VAL new_raster)
{
	KOORD_VAL old = base_tile_raster_width;
	base_tile_raster_width = new_raster;
	tile_raster_width = (new_raster *  zoom_num[zoom_factor]) / zoom_den[zoom_factor];
	return old;
}


// ----------------------------------- clipping routines ------------------------------------------


sint16 display_get_width(void)
{
	return disp_actual_width;
}


// only use, if you are really really sure!
void display_set_actual_width(KOORD_VAL w)
{
	disp_actual_width = w;
}


sint16 display_get_height(void)
{
	return disp_height;
}


void display_set_height(KOORD_VAL const h)
{
	disp_height = h;
}


/**
 * this sets width < 0 if the range is out of bounds
 * so check the value after calling and before displaying
 * @author Hj. Malthaner
 */
static int clip_wh(KOORD_VAL *x, KOORD_VAL *width, const KOORD_VAL min_width, const KOORD_VAL max_width)
{
	if (*width <= 0) {
		*width = 0;
		return 0;
	}
	if (*x + *width > max_width) {
		*width = max_width - *x;
	}
	if (*x < min_width) {
		const KOORD_VAL xoff = min_width - *x;

		*width += *x-min_width;
		*x = min_width;

		return xoff;
	}
	return 0;
}


/**
 * places x and w within bounds left and right
 * if nothing to show, returns false
 * @author Niels Roest
 */
static bool clip_lr(KOORD_VAL *x, KOORD_VAL *w, const KOORD_VAL left, const KOORD_VAL right)
{
	const KOORD_VAL l = *x;      // leftmost pixel
	const sint32 r = (sint32)*x + (sint32)*w; // rightmost pixel

	if (*w <= 0 || l >= right || r <= left) {
		*w = 0;
		return false;
	}

	// there is something to show.
	if (l < left) {
		*w -= left - l;
		*x = left;
	}
	if (r > right) {
		*w -= (KOORD_VAL)(r - right);
	}
	return *w > 0;
}


/**
 * Ermittelt Clipping Rechteck
 * @author Hj. Malthaner
 */
clip_dimension display_get_clip_wh()
{
	return clip_rect;
}


/**
 * Setzt Clipping Rechteck
 * @author Hj. Malthaner
 *
 * here, a pixel at coordinate xp is displayed if
 *  clip. x <= xp < clip.xx
 * the right-most pixel of an image located at xp with width w is displayed if
 *  clip.x < xp+w <= clip.xx
 * analogously for the y coordinate
 */
void display_set_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h)
{
	clip_wh(&x, &w, 0, disp_width);
	clip_wh(&y, &h, 0, disp_height);

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.w = w;
	clip_rect.h = h;

	clip_rect.xx = x + w; // watch out, clips to KOORD_VAL max
	clip_rect.yy = y + h; // watch out, clips to KOORD_VAL max
}



class clip_line_t {
private:
	// line from (x0,y0) to (x1 y1)
	// clip (do not draw) everything right from the ray (x0,y0)->(x1,y1)
	// pixels on the ray are not drawn
	// (not_convex) if y0>=y1 then clip along the path (x0,-inf)->(x0,y0)->(x1,y1)
	// (not_convex) if y0<y1  then clip along the path (x0,y0)->(x1,y1)->(x1,+inf)
	int x0,y0;
	int dx,dy,sdy,sdx;
	int inc;
	bool non_convex;

public:
	void clip_from_to(int x0_,int y0_, int x1, int y1, bool non_convex_) {
		x0 = x0_;
		dx = x1-x0;
		y0 = y0_;
		dy = y1-y0;
		non_convex = non_convex_;
		int steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
		if (steps == 0) {
			return;
		}
		sdx = (dx << 16) / steps;
		sdy = (dy << 16) / steps;
		// to stay right from the line
		// left border: xmin <= x
		// right border: x < xmax
		if (dy>0) {
			if (dy > dx) {
				inc = 1 << 16;
			}
			else {
				inc = (dx << 16)/dy;
			}
		}
		else if (dy<0) {
			if (dy < dx) {
				inc = 0; // (+1)-1 << 16;
			}
			else {
				inc = (1 << 16) - (dx << 16) / dy;
			}
		}
	}

	// clip if
	// ( x-x0) . (  y1-y0 )
	// ( y-y0) . (-(x1-x0)) < 0
	// -- initialize the clipping
	//    has to be called before image will be drawn
	//    return interval for x coordinate
	inline void get_x_range(KOORD_VAL y, xrange &r, bool use_non_convex) const {
		// do everything for the previous row
		y--;
		r.y = y;
		r.non_convex_active = false;
		if (non_convex  &&  use_non_convex  &&  y<y0  &&  y<(y0+dy)) {
			r.non_convex_active = true;
		}
		else if (dy != 0) {
			// init Bresenham algorithm
			const int t = ((y-y0) << 16) / sdy;
			// sx >> 16 = x
			// sy >> 16 = y
			r.sx = t * sdx + inc + (x0 << 16);
			r.sy = t * sdy + (y0 << 16);
		}
	}

	// -- step one line down, return interval for x coordinate
	inline void inc_y(xrange &r, int &xmin, int &xmax) const {
		r.y ++;
		// switch between clip vertical and along ray
		if (r.non_convex_active) {
			if (r.y==min(y0,y0+dy)) {
				r.non_convex_active = false;
				if (dy != 0) {
					// init Bresenham algorithm
					const int t = ((r.y-y0) << 16) / sdy;
					// sx >> 16 = x
					// sy >> 16 = y
					r.sx = t * sdx + inc + (x0 << 16);
					r.sy = t * sdy + (y0 << 16);
					if (dy > 0) {
						const int r_xmin = r.sx >> 16;
						if (xmin < r_xmin) xmin = r_xmin;
					}
					else {
						const int r_xmax = r.sx >> 16;
						if (xmax > r_xmax) xmax = r_xmax;
					}
				}
			}
			else {
				if (dy<0) {
					const int r_xmax = x0+dx;
					if (xmax > r_xmax) xmax = r_xmax;
				}
				else {
					const int r_xmin = x0+1;
					if (xmin < r_xmin) xmin = r_xmin;
				}
			}
		}
		// go along the ray, Bresenham
		else if (dy!=0) {
			if (dy > 0) {
				do {
					r.sx += sdx;
					r.sy += sdy;
				} while ((r.sy >> 16) < r.y);
				const int r_xmin = r.sx >> 16;
				if (xmin < r_xmin) xmin = r_xmin;
			}
			else {
				do {
					r.sx -= sdx;
					r.sy -= sdy;
				} while ((r.sy >> 16) < r.y);
				const int r_xmax = r.sx >> 16;
				if (xmax > r_xmax) xmax = r_xmax;
			}
		}
		// horicontal clip
		else {
			const bool clip = dx*(r.y-y0)>0;
			if (clip) {
				// invisible row
				xmin = +1;
				xmax = -1;
			}
		}
	}
};


static clip_line_t poly_clips[MAX_POLY_CLIPS];


/*
 * Add clipping line through (x0,y0) and (x1,y1)
 * with associated ribi
 * if ribi & 16 then non-convex clipping.
 */
void add_poly_clip(int x0,int y0, int x1, int y1, int ribi)
{
	if (number_of_clips < MAX_POLY_CLIPS) {
		poly_clips[number_of_clips].clip_from_to(x0,y0,x1,y1,ribi&16);
		clip_ribi[number_of_clips] = ribi&15;
		number_of_clips++;
	}
}


/*
 * Clears all clipping lines
 */
void clear_all_poly_clip()
{
	number_of_clips = 0;
	active_ribi = 15; // set all to active
}


/*
 * Activates clipping lines associated with ribi
 * ie if clip_ribi[i] & active_ribi
 */
void activate_ribi_clip(int ribi)
{
	active_ribi = ribi;
}


/*
 * Initialize clipping region for image starting at screen line y
 */
static inline void init_ranges(int y)
{
	for (uint8 i=0; i<number_of_clips; i++) {
		if (clip_ribi[i] & active_ribi) {
			poly_clips[i].get_x_range(y,xranges[i], active_ribi & 16);
		}
	}
}


/*
 * Returns left/right border of visible area
 * Computes l/r border for the next line (ie y+1)
 * takes also clipping rectangle into account
 */
inline void get_xrange_and_step_y(int &xmin, int &xmax)
{
	xmin = clip_rect.x;
	xmax = clip_rect.xx;
	for (uint8 i=0; i<number_of_clips; i++) {
		if (clip_ribi[i] & active_ribi) {
			poly_clips[i].inc_y(xranges[i], xmin, xmax);
		}
	}
}


// ------------------------------ dirty tile stuff --------------------------------


/*
 * simutrans keeps a list of dirty areas, i.e. places that received new graphics
 * and must be copied to the screen after an update
 */
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

	for (; y1 <= y2; y1++) {
		mark_tiles_dirty(x1, x2, y1);
	}
}


/**
 * Markiert ein Tile as schmutzig, mit Clipping
 * @author Hj. Malthaner
 */
void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2)
{
	// inside display?
	if (x2 >= 0 && y2 >= 0 && x1 < disp_width && y1 < disp_height) {
		if (x1 < 0) {
			x1 = 0;
		}
		if (y1 < 0) {
			y1 = 0;
		}
		if (x2 >= disp_width) {
			x2 = disp_width  - 1;
		}
		if (y2 >= disp_height) {
			y2 = disp_height - 1;
		}
		mark_rect_dirty_nc(x1, y1, x2, y2);
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


// ------------------------- rendering images for display --------------------------------

/*
 * Simutrans caches two player colored images, to allow faster drawing of them
 * They are derived from a base image, which may need zooming too
 */

/**
 * Rezooms all images
 * @author Hj. Malthaner
 */
static void rezoom(void)
{
	uint16 n;

	for (n = 0; n < anz_images; n++) {
		if((images[n].recode_flags & FLAG_ZOOMABLE) != 0 && images[n].base_h > 0) {
			images[n].recode_flags |= FLAG_REZOOM;
		}
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;	// color will be set next time
	}
}


void set_zoom_factor(int z)
{
	zoom_factor = z;
	tile_raster_width = (base_tile_raster_width * zoom_num[zoom_factor]) / zoom_den[zoom_factor];
	fprintf(stderr, "set_zoom_factor() : set %d (%i/%i)\n", zoom_factor, zoom_num[zoom_factor], zoom_den[zoom_factor] );
	rezoom();
}



int zoom_factor_up()
{
	// zoom out, if size permits
	if(  zoom_factor>0  ) {
		set_zoom_factor( zoom_factor-1 );
		return true;
	}
	return false;
}


int zoom_factor_down()
{
	if (zoom_factor<MAX_ZOOM_FACTOR) {
		set_zoom_factor( zoom_factor+1 );
		return true;
	}
	return false;
}


static uint8 player_night=0xFF;
static uint8 player_day=0xFF;
static void activate_player_color(sint8 player_nr, bool daynight)
{
	// cahces the last settings
	if(!daynight) {
		if(player_day!=player_nr) {
			int i;
			player_day = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_all_day[0x8000+i] = specialcolormap_all_day[player_offsets[player_day][0]+i];
				rgbmap_all_day[0x8008+i] = specialcolormap_all_day[player_offsets[player_day][1]+i];
			}
		}
		rgbmap_current = rgbmap_all_day;
	}
	else {
		// changing colortable
		if(player_night!=player_nr) {
			int i;
			player_night = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[player_night][0]+i];
				rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[player_night][1]+i];
			}
		}
		rgbmap_current = rgbmap_day_night;
	}
}


/**
 * Convert image data to actual output data
 * @author Hj. Malthaner
 */
static void recode(void)
{
	unsigned n;

	for (n = 0; n < anz_images; n++) {
		// recode images only if the recoded image is needed
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;
	}
}


/**
 * Convert a certain image data to actual output data
 * @author Hj. Malthaner
 */
static void recode_img_src_target(KOORD_VAL h, PIXVAL *src, PIXVAL *target)
{
	if (h > 0) {
		do {
			uint16 runlen = *target++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				while (runlen--) {
					// now just convert the color pixels
					*target++ = rgbmap_day_night[*src++];
				}
			} while ((runlen = *target++ = *src++));
		} while (--h);
	}
}



/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_normal_img(const unsigned int n)
{
#if MULTI_THREAD>1
	pthread_mutex_lock( &rezoom_recode_img_mutex );
#endif
	PIXVAL *src = images[n].zoom_data != NULL ? images[n].zoom_data : images[n].base_data;

	if (images[n].data == NULL) {
		images[n].data = MALLOCN(PIXVAL, images[n].len);
	}
	// now do normal recode
	activate_player_color( 0, true );
	recode_img_src_target(images[n].h, src, images[n].data);
	images[n].recode_flags &= ~FLAG_NORMAL_RECODE;
#if MULTI_THREAD>1
	pthread_mutex_unlock( &rezoom_recode_img_mutex );
#endif
}


#ifdef NEED_PLAYER_RECODE

/**
 * Convert a certain image data to actual output data for a certain player
 * @author prissi
 */
static void recode_img_src_target_color(KOORD_VAL h, PIXVAL *src, PIXVAL *target)
{
	if (h > 0) {
		do {
			uint16 runlen = *target++ = *src++;
			// eine Zeile dekodieren

			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				// now just convert the color pixels
				while (runlen--) {
					*target++ = rgbmap_day_night[*src++];
				}
				// next clea run or zero = end
			} while ((runlen = *target++ = *src++));
		} while (--h);
	}
}


/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_color_img(const unsigned int n, const unsigned char player_nr)
{
	// Hajo: may this image be zoomed
#if MULTI_THREAD>1
	pthread_mutex_lock( &rezoom_recode_img_mutex );
#endif
	PIXVAL *src = images[n].zoom_data != NULL ? images[n].zoom_data : images[n].base_data;

	images[n].player_flags = player_nr;
	if(  images[n].player_data == NULL  ) {
		images[n].player_data = MALLOCN(PIXVAL, images[n].len);
	}
	// contains now the player color ...
	activate_player_color( player_nr, true );
	recode_img_src_target_color(images[n].h, src, images[n].player_data );
#if MULTI_THREAD>1
	pthread_mutex_unlock( &rezoom_recode_img_mutex );
#endif
}

#endif



// for zoom out
#define SumSubpixel(p) \
	if(*(p)<255  &&  valid<255) { \
	if(*(p)==1) { valid = 255; r = g = b = 0; } else { valid ++; } /* mark special colors */\
		r += (p)[1]; \
		g += (p)[2]; \
		b += (p)[3]; \
	}


// recode 4*bytes into PIXVAL
PIXVAL inline compress_pixel(uint8* p)
{
	return (((p[0]==1 ? 0x8000 : 0 ) | p[1]) + (((uint16)p[2])<<5) + (((uint16)p[3])<<10));
}

// recode 4*bytes into PIXVAL, respect transparency
PIXVAL inline compress_pixel_transparent(uint8 *p)
{
	return p[0]==255 ? 0x73FE : compress_pixel(p);
}

// zoomin pixel taking color of above/below and transparency of diagonal neighbor into account
PIXVAL inline zoomin_pixel(uint8 *p, uint8* pab, uint8 *prl, uint8* pdia)
{
	if (p[0] == 255) {
		if ( (pab[0] | prl[0] | pdia[0])==255) {
			return 0x73FE; // pixel and one neighbor transparent -> return transparent
		}
		// pixel transparent but all three neightbors not -> interpolate
		uint8 valid=0;
		uint8 r=0, g=0, b=0;
		SumSubpixel(pab);
		SumSubpixel(prl);
		if(valid==0) {
			return 0x73FE;
		}
		else if(valid==255) {
			return (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
		}
		else {
			return (r/valid) + (((uint16)(g/valid))<<5) + (((uint16)(b/valid))<<10);
		}
	}
	else {
		if ( (pab[0] & prl[0] & pdia[0])!=255) {
			// pixel and one neighbor not transparent
			return compress_pixel(p);
		}
		return 0x73FE;
	}
}


/**
 * Convert base image data to actual image size
 * Uses averages of all sampled points to get the "real" value
 * Blurs a bit
 * @author prissi
 */
static void rezoom_img(const image_id n)
{
	// Hajo: may this image be zoomed
	if (n < anz_images && images[n].base_h > 0) {
#if MULTI_THREAD>1
		pthread_mutex_lock( &rezoom_recode_img_mutex );
		if(  (images[n].recode_flags & FLAG_REZOOM) == 0  ) {
			// other routine did already the rezooming ...
			pthread_mutex_unlock( &rezoom_recode_img_mutex );
			return;
		}
#endif
		// we may need night conversion afterwards
		images[n].recode_flags &= ~FLAG_REZOOM;
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;

		//  we recalculate the len (since it may be larger than before)
		// thus we have to free the old caches
		if (images[n].zoom_data != NULL) {
			guarded_free(images[n].zoom_data);
			images[n].zoom_data = NULL;
		}
		if (images[n].data != NULL) {
			guarded_free(images[n].data);
			images[n].data = NULL;
		}
		if (images[n].player_data != NULL) {
			guarded_free(images[n].player_data);
			images[n].player_data = NULL;
		}

		// just restore original size?
		if (tile_raster_width == base_tile_raster_width  ||  (images[n].recode_flags&FLAG_ZOOMABLE)==0) {

			// this we can do be a simple copy ...
			images[n].x = images[n].base_x;
			images[n].w = images[n].base_w;
			images[n].y = images[n].base_y;
			images[n].h = images[n].base_h;
			{
				// recalculate length
				sint16 h = images[n].base_h;
				PIXVAL *sp = images[n].base_data;

				while(h-->0) {
					do {
						// clear run + colored run + next clear run
						sp++;
						sp += *sp + 1;
					} while (*sp);
					sp++;
				}
				images[n].len = (uint32)(size_t)(sp-images[n].base_data);
			}
#if MULTI_THREAD>1
			pthread_mutex_unlock( &rezoom_recode_img_mutex );
#endif
			return;
		}

		// now we want to downsize the image
		// just divede the sizes
		images[n].x = (images[n].base_x*zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].y = (images[n].base_y*zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].w = (images[n].base_w*zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].h = (images[n].base_h*zoom_num[zoom_factor]) / zoom_den[zoom_factor];

		if (images[n].h > 0  &&  images[n].w > 0) {
			// just recalculate the image in the new size
			static uint8 *baseimage = NULL;
			static uint32 size = 0;
			static PIXVAL *baseimage2 = NULL;
			PIXVAL *src = images[n].base_data;
			PIXVAL *dest = NULL;
			// embed the baseimage in an image with margin ~ remainder
			const sint16 x_rem = (images[n].base_x*zoom_num[zoom_factor]) % zoom_den[zoom_factor];
			const sint16 y_rem = (images[n].base_y*zoom_num[zoom_factor]) % zoom_den[zoom_factor];
			const sint16 xl_margin = max( x_rem, 0);
			const sint16 xr_margin = max(-x_rem, 0);
			const sint16 yl_margin = max( y_rem, 0);
			const sint16 yr_margin = max(-y_rem, 0);
			// baseimage top-left  corner is at (xl_margin, yl_margin)
			// ...       low-right corner is at (xr_margin, yr_margin)


			sint32 orgzoomwidth = ((images[n].base_w + zoom_den[zoom_factor] - 1 ) / zoom_den[zoom_factor]) * zoom_den[zoom_factor];
			sint32 newzoomwidth = (orgzoomwidth*zoom_num[zoom_factor])/zoom_den[zoom_factor];
			sint32 orgzoomheight = ((images[n].base_h + zoom_den[zoom_factor] - 1 ) / zoom_den[zoom_factor]) * zoom_den[zoom_factor];
			sint32 newzoomheight = (orgzoomheight*zoom_num[zoom_factor])/zoom_den[zoom_factor];

			// we will upack, resample, pack it

			// thus the unpack buffer must at least fit the window => find out maximum size
			size_t new_size = newzoomwidth*(newzoomheight+6)*sizeof(PIXVAL);
			size_t unpack_size = (xl_margin+orgzoomwidth+xr_margin)*(yl_margin+orgzoomheight+yr_margin)*4;
			if( unpack_size > new_size ) {
				new_size = unpack_size;
			}
			if(size < new_size) {
				free( baseimage );
				free( baseimage2 );
				size = new_size;
				baseimage  = MALLOCN( uint8, size );
				baseimage2 = (PIXVAL *)MALLOCN( uint8, size );
			}
			memset( baseimage, 255, size ); // fill with invalid data to mark transparent regions

			// index of top-left corner
			uint32 baseoff = 4*(yl_margin*(xl_margin+orgzoomwidth+xr_margin)+xl_margin);
			sint32 basewidth = xl_margin+orgzoomwidth+xr_margin;

			// now: unpack the image
			for (sint32 y = 0; y < images[n].base_h; ++y) {
				uint16 runlen;
				uint8 *p = baseimage + baseoff + y*(basewidth*4);

				// decode line
				runlen = *src++;
				do {
					// clear run
					p += runlen*4;
					// color pixel
					runlen = *src++;
					while (runlen--) {
						// get rgb components
						PIXVAL s = *src++;
						*p++ = (s>>15);
						*p++ = (s & 31);
						s >>= 5;
						*p++ = (s & 31);
						s >>= 5;
						*p++ = (s & 31);
					}
					runlen = *src++;
				} while (runlen!=0);
			}


			// now we have the image, we do a repack then
			dest = baseimage2;
			switch(zoom_den[zoom_factor]) {
				case 1: {
					assert(zoom_num[zoom_factor]==2);

					// first half row - just copy values, do not fiddle with neighbor colors
					uint8 *p1 = baseimage + baseoff;
					for(  sint16 x=0;  x<orgzoomwidth;  x++  ) {
						PIXVAL c1 = compress_pixel_transparent(p1 +(x*4) );
						// now set the pixel ...
						dest[x*2] = c1;
						dest[x*2+1] = c1;
					}
					// skip one line
					dest += newzoomwidth;

					for(  sint16 y=0;  y<orgzoomheight-1;  y++  ) {
						uint8 *p1 = baseimage + baseoff + y*(basewidth*4);
						// copy leftmost pixels
						dest[0] = compress_pixel_transparent(p1);
						dest[newzoomwidth] = compress_pixel_transparent(p1+basewidth*4);
						for(  sint16 x=0;  x<orgzoomwidth-1;  x++  ) {
							uint8 *px1=p1+(x*4);
							// pixel at 2,2 in 2x2 superpixel
							dest[x*2+1] = zoomin_pixel(px1, px1+4, px1+basewidth*4, px1+basewidth*4+4);

							// 2x2 superpixel is transparent but original pixel was not
							// preserve one pixel
							if( dest[x*2+1]==0x73FE  &&  px1[0]!=255  &&  dest[x*2]==0x73FE
								&&  dest[x*2-newzoomwidth]==0x73FE  &&  dest[x*2-newzoomwidth-1]==0x73FE) {
								// preserve one pixel
								dest[x*2+1] = compress_pixel(px1);
							}

							// pixel at 2,1 in next 2x2 superpixel
							dest[x*2+2] = zoomin_pixel(px1+4, px1, px1+basewidth*4+4, px1+basewidth*4);

							// pixel at 1,2 in next row 2x2 superpixel
							dest[x*2+newzoomwidth+1] = zoomin_pixel(px1+basewidth*4, px1+basewidth*4+4, px1, px1+4);

							// pixel at 1,1 in next row next 2x2 superpixel
							dest[x*2+newzoomwidth+2] = zoomin_pixel(px1+basewidth*4+4, px1+basewidth*4, px1+4, px1);
						}
						// copy rightmost pixels
						dest[2*orgzoomwidth-1] = compress_pixel_transparent(p1+4*(orgzoomwidth-1));
						dest[2*orgzoomwidth+newzoomwidth-1] = compress_pixel_transparent(p1+4*(orgzoomwidth-1)+basewidth*4);
						// skip two lines
						dest += 2*newzoomwidth;
					}
					// last half row - just copy values, do not fiddle with neighbor colors
					p1 = baseimage + baseoff + (orgzoomheight-1)*(basewidth*4);
					for(  sint16 x=0;  x<orgzoomwidth;  x++  ) {
						PIXVAL c1 = compress_pixel_transparent(p1 +(x*4) );
						// now set the pixel ...
						dest[x*2]   = c1;
						dest[x*2+1] = c1;
					}
					break;
				}
				case 2:
					for(  sint16 y=0;  y<newzoomheight;  y++  ) {
						uint8 *p1 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+0-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p2 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+1-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						for(  sint16 x=0;  x<newzoomwidth;  x++  ) {
							uint8 valid=0;
							uint8 r=0,g=0,b=0;
							sint16 xreal1 = ((x*zoom_den[zoom_factor]+0-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal2 = ((x*zoom_den[zoom_factor]+1-x_rem)/zoom_num[zoom_factor])*4;
							SumSubpixel(p1+xreal1);
							SumSubpixel(p1+xreal2);
							SumSubpixel(p2+xreal1);
							SumSubpixel(p2+xreal2);
							if(valid==0) {
								*dest++ = 0x73FE;
							}
							else if(valid==255) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) + (((uint16)(g/valid))<<5) + (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 3:
					for(  sint16 y=0;  y<newzoomheight;  y++  ) {
						uint8 *p1 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+0-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p2 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+1-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p3 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+2-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						for(  sint16 x=0;  x<newzoomwidth;  x++  ) {
							uint8 valid=0;
							uint16 r=0,g=0,b=0;
							sint16 xreal1 = ((x*zoom_den[zoom_factor]+0-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal2 = ((x*zoom_den[zoom_factor]+1-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal3 = ((x*zoom_den[zoom_factor]+2-x_rem)/zoom_num[zoom_factor])*4;
							SumSubpixel(p1+xreal1);
							SumSubpixel(p1+xreal2);
							SumSubpixel(p1+xreal3);
							SumSubpixel(p2+xreal1);
							SumSubpixel(p2+xreal2);
							SumSubpixel(p2+xreal3);
							SumSubpixel(p3+xreal1);
							SumSubpixel(p3+xreal2);
							SumSubpixel(p3+xreal3);
							if(valid==0) {
								*dest++ = 0x73FE;
							}
							else if(valid==255) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 4:
					for(  sint16 y=0;  y<newzoomheight;  y++  ) {
						uint8 *p1 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+0-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p2 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+1-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p3 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+2-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p4 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+3-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						for(  sint16 x=0;  x<newzoomwidth;  x++  ) {
							uint8 valid=0;
							uint16 r=0,g=0,b=0;
							sint16 xreal1 = ((x*zoom_den[zoom_factor]+0-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal2 = ((x*zoom_den[zoom_factor]+1-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal3 = ((x*zoom_den[zoom_factor]+2-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal4 = ((x*zoom_den[zoom_factor]+3-x_rem)/zoom_num[zoom_factor])*4;
							SumSubpixel(p1+xreal1);
							SumSubpixel(p1+xreal2);
							SumSubpixel(p1+xreal3);
							SumSubpixel(p1+xreal4);
							SumSubpixel(p2+xreal1);
							SumSubpixel(p2+xreal2);
							SumSubpixel(p2+xreal3);
							SumSubpixel(p2+xreal4);
							SumSubpixel(p3+xreal1);
							SumSubpixel(p3+xreal2);
							SumSubpixel(p3+xreal3);
							SumSubpixel(p3+xreal4);
							SumSubpixel(p4+xreal1);
							SumSubpixel(p4+xreal2);
							SumSubpixel(p4+xreal3);
							SumSubpixel(p4+xreal4);
							if(valid==0) {
								*dest++ = 0x73FE;
							}
							else if(valid==255) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 8:
					for(  sint16 y=0;  y<newzoomheight;  y++  ) {
						uint8 *p1 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+0-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p2 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+1-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p3 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+2-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p4 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+3-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p5 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+4-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p6 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+5-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p7 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+6-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						uint8 *p8 = baseimage + baseoff + ((y*zoom_den[zoom_factor]+7-y_rem)/zoom_num[zoom_factor])*(basewidth*4);
						for(  sint16 x=0;  x<newzoomwidth;  x++  ) {
							uint8 valid=0;
							uint16 r=0,g=0,b=0;
							sint16 xreal1 = ((x*zoom_den[zoom_factor]+0-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal2 = ((x*zoom_den[zoom_factor]+1-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal3 = ((x*zoom_den[zoom_factor]+2-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal4 = ((x*zoom_den[zoom_factor]+3-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal5 = ((x*zoom_den[zoom_factor]+4-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal6 = ((x*zoom_den[zoom_factor]+5-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal7 = ((x*zoom_den[zoom_factor]+6-x_rem)/zoom_num[zoom_factor])*4;
							sint16 xreal8 = ((x*zoom_den[zoom_factor]+7-x_rem)/zoom_num[zoom_factor])*4;
							SumSubpixel(p1+xreal1);
							SumSubpixel(p1+xreal2);
							SumSubpixel(p1+xreal3);
							SumSubpixel(p1+xreal4);
							SumSubpixel(p1+xreal5);
							SumSubpixel(p1+xreal6);
							SumSubpixel(p1+xreal7);
							SumSubpixel(p1+xreal8);
							SumSubpixel(p2+xreal1);
							SumSubpixel(p2+xreal2);
							SumSubpixel(p2+xreal3);
							SumSubpixel(p2+xreal4);
							SumSubpixel(p2+xreal5);
							SumSubpixel(p2+xreal6);
							SumSubpixel(p2+xreal7);
							SumSubpixel(p2+xreal8);
							SumSubpixel(p3+xreal1);
							SumSubpixel(p3+xreal2);
							SumSubpixel(p3+xreal3);
							SumSubpixel(p3+xreal4);
							SumSubpixel(p3+xreal5);
							SumSubpixel(p3+xreal6);
							SumSubpixel(p3+xreal7);
							SumSubpixel(p3+xreal8);
							SumSubpixel(p4+xreal1);
							SumSubpixel(p4+xreal2);
							SumSubpixel(p4+xreal3);
							SumSubpixel(p4+xreal4);
							SumSubpixel(p4+xreal5);
							SumSubpixel(p4+xreal6);
							SumSubpixel(p4+xreal7);
							SumSubpixel(p4+xreal8);
							SumSubpixel(p5+xreal1);
							SumSubpixel(p5+xreal2);
							SumSubpixel(p5+xreal3);
							SumSubpixel(p5+xreal4);
							SumSubpixel(p5+xreal5);
							SumSubpixel(p5+xreal6);
							SumSubpixel(p5+xreal7);
							SumSubpixel(p5+xreal8);
							SumSubpixel(p6+xreal1);
							SumSubpixel(p6+xreal2);
							SumSubpixel(p6+xreal3);
							SumSubpixel(p6+xreal4);
							SumSubpixel(p6+xreal5);
							SumSubpixel(p6+xreal6);
							SumSubpixel(p6+xreal7);
							SumSubpixel(p6+xreal8);
							SumSubpixel(p7+xreal1);
							SumSubpixel(p7+xreal2);
							SumSubpixel(p7+xreal3);
							SumSubpixel(p7+xreal4);
							SumSubpixel(p7+xreal5);
							SumSubpixel(p7+xreal6);
							SumSubpixel(p7+xreal7);
							SumSubpixel(p7+xreal8);
							SumSubpixel(p8+xreal1);
							SumSubpixel(p8+xreal2);
							SumSubpixel(p8+xreal3);
							SumSubpixel(p8+xreal4);
							SumSubpixel(p8+xreal5);
							SumSubpixel(p8+xreal6);
							SumSubpixel(p8+xreal7);
							SumSubpixel(p8+xreal8);
							if(valid==0) {
								*dest++ = 0x73FE;
							}
							else if(valid==255) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				default: assert(0);
			}

			// now encode the image again
			dest = (PIXVAL*)baseimage;
			for(  sint16 y=0;  y<newzoomheight;  y++  ) {
				PIXVAL *line = ((PIXVAL *)baseimage2) + (y*newzoomwidth);
				PIXVAL i;
				sint16 x = 0;
				uint16 clear_colored_run_pair_count = 0;

				do {
					// check length of transparent pixels
					for (i = 0;  line[x] == 0x73FE  &&  x < newzoomwidth;  i++, x++)
						{}
					// first runlength: transparent pixels
					*dest++ = i;
					// copy for non-transparent
					for (i = 0;  line[x] != 0x73FE  &&  x < newzoomwidth;  i++, x++) {
						dest[i + 1] = line[x];
					}

					/* Knightly:
					 *		If it is not the first clear-colored-run pair and its colored run is empty
					 *		--> it is superfluous and can be removed by rolling back the pointer
					 */
					if(  clear_colored_run_pair_count>0  &&  i==0  ) {
						dest--;
						// this only happens at the end of a line, so no need to increment clear_colored_run_pair_count
					}
					else {
						*dest++ = i;	// number of colored pixel
						dest += i;	// skip them
						clear_colored_run_pair_count++;
					}
				} while(x<newzoomwidth);
				*dest++ = 0; // mark line end
			}

			// something left?
			images[n].w = newzoomwidth;
			images[n].h = newzoomheight;
			if(newzoomheight>0) {
				const size_t zoom_len = (size_t)(((uint8 *)dest) - ((uint8 *)baseimage));
				images[n].len = (uint32)(zoom_len/sizeof(PIXVAL));
				images[n].zoom_data = MALLOCN(PIXVAL, images[n].len);
				assert( images[n].zoom_data  );
				memcpy( images[n].zoom_data, baseimage, zoom_len );
			}
		}
		else {
			if (images[n].w <= 0) {
				// h=0 will be ignored, with w=0 there was an error!
				printf("WARNING: image%d w=0!\n", n);
			}
			images[n].h = 0;
		}
#if MULTI_THREAD>1
		pthread_mutex_unlock( &rezoom_recode_img_mutex );
#endif
	}
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

	const double RG_night_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_night_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);

	for (i = 0; i < 0x8000; i++) {
		// (1<<15) this is total no of all possible colors in RGB555)
		// RGB 555 input
		int R = (i & 0x7C00) >> 7;
		int G = (i & 0x03E0) >> 2;
		int B = (i & 0x001F) << 3;
		// lines generate all possible colors in 555RGB code - input
		// however the result is in 888RGB - 8bit per channel

		R = (int)(R * RG_night_multiplier);
		G = (int)(G * RG_night_multiplier);
		B = (int)(B * B_night_multiplier);

		rgbmap_day_night[i] = get_system_color(R, G, B);
	}

	// player color map (and used for map display etc.)
	for (i = 0; i < 224; i++) {
		const int R = (int)(special_pal[i*3 + 0] * RG_night_multiplier);
		const int G = (int)(special_pal[i*3 + 1] * RG_night_multiplier);
		const int B = (int)(special_pal[i*3 + 2] * B_night_multiplier);

		specialcolormap_day_night[i] = get_system_color(R, G, B);
	}
	// special light colors (actually, only non-darkening greys should be used
	for(i=0;  i<LIGHT_COUNT;  i++  ) {
		specialcolormap_day_night[i+224] = get_system_color( display_day_lights[i*3 + 0], display_day_lights[i*3 + 1], 	display_day_lights[i*3 + 2] );
	}
	// init with black for forbidden colors
	for(i=224+LIGHT_COUNT;  i<256;  i++  ) {
		specialcolormap_day_night[i] = 0;
	}
	// default player colors
	for(i=0;  i<8;  i++  ) {
		rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[0][0]+i];
		rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[0][1]+i];
	}
	player_night = 0;

	// Lights
	for (i = 0; i < LIGHT_COUNT; i++) {
		const int day_R = display_day_lights[i*3+0];
		const int day_G = display_day_lights[i*3+1];
		const int day_B = display_day_lights[i*3+2];

		const int night_R = display_night_lights[i*3+0];
		const int night_G = display_night_lights[i*3+1];
		const int night_B = display_night_lights[i*3+2];

		const int R = (day_R * day + night_R * night2) >> 2;
		const int G = (day_G * day + night_G * night2) >> 2;
		const int B = (day_B * day + night_B * night2) >> 2;

		rgbmap_day_night[0x8010 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// convert to RGB xxx
	recode();
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


void display_day_night_shift(int night)
{
	if (night != night_shift) {
		night_shift = night;
		calc_base_pal_from_night_shift(night);
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
	}
}



// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 )
{
	if(player_offsets[player][0]!=col1  ||  player_offsets[player][1]!=col2) {
		// set new player colors
		player_offsets[player][0] = col1;
		player_offsets[player][1] = col2;
		if(player==player_day  ||  player==player_night) {
			// and recalculate map (and save it)
			calc_base_pal_from_night_shift(0);
			memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
			if(night_shift!=0) {
				calc_base_pal_from_night_shift(night_shift);
			}
			// calc_base_pal_from_night_shift resets player_night to 0
			player_day = player_night;
		}
		recode();
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
	}
}


void register_image(struct bild_t* bild)
{
	struct imd* image;

	/* valid image? */
	if(  bild->len == 0  ||  bild->h == 0  ) {
		fprintf(stderr, "Warning: ignoring image %d because of missing data\n", anz_images);
		bild->bild_nr = IMG_LEER;
		return;
	}

	if(  anz_images == alloc_images  ) {
		if(  images==NULL  ) {
			alloc_images = 510;
		}
		else {
			alloc_images += 512;
		}
		if(  anz_images > alloc_images  ) {
			// overflow
			dbg->fatal( "register_image", "*** Out of images (more than 65534!) ***" );
		}
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

	// allocate and copy if needed
	image->recode_flags = FLAG_NORMAL_RECODE | FLAG_REZOOM | (bild->zoomable & FLAG_ZOOMABLE);
	image->player_flags = NEED_PLAYER_RECODE;

	image->base_data = NULL;
	image->zoom_data = NULL;
	image->data = NULL;
	image->player_data = NULL;	// chaches data for one AI

	// since we do not recode them, we can work with the original data
	image->base_data = bild->data;

	// does this image have color?
	if(  bild->h > 0  ) {
		int h = bild->h;
		const PIXVAL *src = image->base_data;

		do {
			src++; // offset of first start
			do {
				uint16 runlen;

				for (runlen = *src++; runlen != 0; runlen--) {
					PIXVAL pix = *src++;

					if (pix>=0x8000  &&  pix<=0x800F) {
						image->recode_flags |= FLAG_PLAYERCOLOR;
						return;
					}
				}
				// next clear run or zero = end
			} while(  *src++ != 0  );
		} while(  --h != 0  );
	}
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
		if(images[anz_images].player_data!=NULL) {
			guarded_free( images[anz_images].player_data );
		}
		if(images[anz_images].data!=NULL) {
			guarded_free( images[anz_images].data );
		}
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


// prissi: query unzoomed offsets
void display_get_base_image_offset(unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw)
{
	if (bild < anz_images) {
		*xoff = images[bild].base_x;
		*yoff = images[bild].base_y;
		*xw   = images[bild].base_w;
		*yw   = images[bild].base_h;
	}
}


// prissi: changes the offset of an image
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


// ------------------ display all kind of images from here on ------------------------------


/**
 * Kopiert Pixel von src nach dest
 * @author Hj. Malthaner
 */
static inline void pixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	// for gcc this seems to produce the optimal code ...
	while (src < end) {
		*dest++ = *src++;
	}
}


/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static inline void colorpixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	while (src < end) {
		*dest++ = rgbmap_current[*src++];
	}
}

/**
 * templated pixel copy routines
 * to be used in display_img_pc
 */
enum pixcopy_routines {
	plain = 0,	/// simply copies the pixels
	colored = 1	/// replaces player colors
};

template<pixcopy_routines copyroutine> void templated_pixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end);
template<> void templated_pixcopy<plain>(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	pixcopy(dest, src, end);
}
template<> void templated_pixcopy<colored>(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	colorpixcopy(dest, src, end);
}

/**
 * draws image with clipping along arbitrary lines
 * @author Dwachs
 */
template<pixcopy_routines copyroutine>
static void display_img_pc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		// initialize clipping
		init_ranges(yp);

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			int runlen = *sp++;

			// get left/right boundary, step
			int xmin, xmax;
			get_xrange_and_step_y(xmin, xmax);

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xmin < xmax  &&  xpos + runlen > xmin && xpos < xmax) {
					const int left = (xpos >= xmin ? 0 : xmin - xpos);
					const int len  = (xmax - xpos >= runlen ? runlen : xmax - xpos);

					templated_pixcopy<copyroutine>(tp + xpos + left, sp + left, sp + len);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild mit horizontalem Clipping
 * @author Hj. Malthaner
 */
static void display_img_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			uint16 runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen > clip_rect.x && xpos < clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);

					pixcopy(tp + xpos + left, sp + left, sp + len);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild ohne jedes Clipping
 */
static void display_img_nc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + xp + yp * disp_width;

		do { // zeilen dekodieren
#ifdef USE_C
			uint16 runlen = *sp++;
#else
			// assembler needs this size
			uint32 runlen = *sp++;
#endif
			PIXVAL *p = tp;

			// eine Zeile dekodieren
			do {
				// wir starten mit einem clear run
				p += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;
#ifdef USE_C
#ifndef ALIGN_COPY
				{
					// "classic" C code (why is it faster!?!)
					const uint32 *ls;
					uint32 *ld;

					if (runlen & 1) {
						*p++ = *sp++;
					}

					ls = (const uint32 *)sp;
					ld = (uint32 *)p;
					runlen >>= 1;
					while (runlen--) {
						*ld++ = *ls++;
					}
					p = (PIXVAL*)ld;
					sp = (const PIXVAL*)ls;
				}
#else
				// some architectures: faster with inline of memory functions!
				memcpy( p, sp, runlen*sizeof(PIXVAL) );
				sp += runlen;
				p += runlen;
#endif
#else
				// this code is sometimes slower, mostly 5% faster, not really clear why and when (cache alignment?)
				asm volatile (
					// rep movsw and we would be finished, but we unroll
					// uneven number of words to copy
					"shrl %2\n\t"
					"jnc 0f\n\t"
					// Copy first word
					// *p++ = *sp++;
					"movsw\n\t"
					"0:\n\t"
					"negl %2\n\t"
					"addl $1f, %2\n\t"
					"jmp * %2\n\t"
					"ud2\n\t"
					".p2align 4\n\t"
#define MOVSD1   "movsd\n\t"
#define MOVSD2   MOVSD1   MOVSD1
#define MOVSD4   MOVSD2   MOVSD2
#define MOVSD8   MOVSD4   MOVSD4
#define MOVSD16  MOVSD8   MOVSD8
#define MOVSD32  MOVSD16  MOVSD16
#define MOVSD64  MOVSD32  MOVSD32
#define MOVSD128 MOVSD64  MOVSD64
#define MOVSD256 MOVSD128 MOVSD128
					MOVSD256
#undef MOVSD256
#undef MOVSD128
#undef MOVSD64
#undef MOVSD32
#undef MOVSD16
#undef MOVSD8
#undef MOVSD4
#undef MOVSD2
#undef MOVSD1
					"1:\n\t"
					: "+D" (p), "+S" (sp), "+r" (runlen)
					:
					: "cc", "memory"
				);
#endif
				runlen = *sp++;
			} while (runlen != 0);

			tp += disp_width;
		} while (--h > 0);
	}
}


/**
 * Zeichnet Bild mit verticalem clipping (schnell) und horizontalem (langsam)
 * @author prissi
 */
void display_img_aux(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const sint8 use_player, const int /*daynight*/, const int dirty)
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
			if (images[n].recode_flags&FLAG_REZOOM) {
				rezoom_img(n);
				recode_normal_img(n);
			} else if (images[n].recode_flags&FLAG_NORMAL_RECODE) {
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

			// clipping at poly lines?
			if (number_of_clips>0) {
					display_img_pc<plain>(h, xp, yp, sp);
					// since height may be reduced, start marking here
					if (dirty) {
						mark_rect_dirty_wc(xp, yp, xp + w - 1, yp + h - 1);
					}
			}
			else {
				// use horizontal clipping or skip it?
				if (xp >= clip_rect.x  &&  xp + w <= clip_rect.xx) {
					// marking change?
					if (dirty) {
						mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
					}
					display_img_nc(h, xp, yp, sp);
				} else if (xp < clip_rect.xx  &&  xp + w > clip_rect.x) {
					display_img_wc(h, xp, yp, sp);
					// since height may be reduced, start marking here
					if (dirty) {
						mark_rect_dirty_wc(xp, yp, xp + w - 1, yp + h - 1);
					}
				}
			}
		}
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * assumes height is ok and valid data are caluclated
 * color replacement needs the original data => sp points to non-cached data
 * @author hajo/prissi
 */
static void display_color_img_wc(const PIXVAL *sp, KOORD_VAL x, KOORD_VAL y, KOORD_VAL h )
{
	PIXVAL *tp = textur + y * disp_width;

	do { // zeilen dekodieren
		int xpos = x;

		// bild darstellen

		uint16 runlen = *sp++;

		do {
			// wir starten mit einem clear run
			xpos += runlen;

			// jetzt kommen farbige pixel
			runlen = *sp++;

			// Hajo: something to display?
			if (xpos + runlen > clip_rect.x && xpos < clip_rect.xx) {
				const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
				const int len  = (clip_rect.xx-xpos > runlen ? runlen : clip_rect.xx - xpos);

				colorpixcopy(tp + xpos + left, sp + left, sp + len);
			}

			sp += runlen;
			xpos += runlen;
		} while ((runlen = *sp++));

		tp += disp_width;
	} while (--h);
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
void display_color_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const sint8 player_nr, const int daynight, const int dirty)
{
	if (n < anz_images) {

		// first: size check
		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
		}

		if(  daynight  ||  night_shift==0  ) {
			// Hajo: since the colors for player 0 are already right,
			// only use the expensive replacement routine for colored images
			// of other players
			if(  player_nr==0  ||  (images[n].recode_flags & FLAG_PLAYERCOLOR)==0  ) {
				display_img_aux(n, xp, yp, false, true, dirty);
				return;
			}

			// first test, if we can/need to build a cached version
			if(  (images[n].player_flags&(~NEED_PLAYER_RECODE)) == 0  ) {
				// we can still recoler if needed
				recode_color_img(n, player_nr);
			}
			// ok, there is a cached version
			if(  (images[n].player_flags&(~NEED_PLAYER_RECODE)) == player_nr  ) {
				// ok, now we could use the same faster code as for the normal images
				display_img_aux(n, xp, yp, true, true, dirty);
				return;
			}
		}

		// prissi: now test if visible and clipping needed
		{
			const KOORD_VAL x = images[n].x + xp;
			      KOORD_VAL y = images[n].y + yp;
			const KOORD_VAL w = images[n].w;
			      KOORD_VAL h = images[n].h;

			if (h <= 0 || x >= clip_rect.xx || y >= clip_rect.yy || x + w <= clip_rect.x || y + h <= clip_rect.y) {
				// not visible => we are done
				// happens quite often ...
				return;
			}

			if (dirty) {
				mark_rect_dirty_wc(x, y, x + w - 1, y + h - 1);
			}

			// colors for 2nd company color
			if(player_nr>=0) {
				activate_player_color( player_nr, daynight );
			}
			else {
				// no player
				activate_player_color( 0, daynight );
			}
			// color replacement needs the original data => sp points to non-cached data
			const PIXVAL *sp = (tile_raster_width != base_tile_raster_width  &&  images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;

			// clip top/bottom
			KOORD_VAL yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);
			if (h > 0) { // clipping may have reduced it

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

				// clipping at poly lines?
				if (number_of_clips>0) {
					display_img_pc<colored>(h, x, y, sp);
				}
				else {
					display_color_img_wc(sp, x, y, h );
				}
			}
		}
	} // number ok
}



/**
 * draw unscaled images, replaces base color
 * @author prissi
 */
void display_base_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const sint8 player_nr, const int daynight, const int dirty)
{
	if(  base_tile_raster_width==tile_raster_width  ) {
		// same size => use standard routine
		display_color_img( n, xp, yp, player_nr, daynight, dirty );
	}
	else if (n < anz_images) {

		// prissi: now test if visible and clipping needed
		const KOORD_VAL x = images[n].base_x + xp;
		      KOORD_VAL y = images[n].base_y + yp;
		const KOORD_VAL w = images[n].base_w;
		      KOORD_VAL h = images[n].base_h;

		if (h <= 0 || x >= clip_rect.xx || y >= clip_rect.yy || x + w <= clip_rect.x || y + h <= clip_rect.y) {
			// not visible => we are done
			// happens quite often ...
			return;
		}

		if (dirty) {
			mark_rect_dirty_wc(x, y, x + w - 1, y + h - 1);
		}

		// colors for 2nd company color
		if(player_nr>=0) {
			activate_player_color( player_nr, daynight );
		}
		else {
			// no player
			activate_player_color( 0, daynight );
		}

		// color replacement needs the original data => sp points to non-cached data
		const PIXVAL *sp = images[n].base_data;

		// clip top/bottom
		KOORD_VAL yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);
		if (h > 0) { // clipping may have reduced it

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
			display_color_img_wc( sp, x, y, h );
		}

	} // number ok
}



/* from here code for transparent images */
typedef void (*blend_proc)(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len);

// different masks needed for RGB 555 and RGB 565
#define ONE_OUT_16 (0x7bef)
#define TWO_OUT_16 (0x39E7)
#define ONE_OUT_15 (0x3DEF)
#define TWO_OUT_15 (0x1CE7)

static void pix_blend75_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((*src)>>2) & TWO_OUT_15)) + (((*dest)>>2) & TWO_OUT_15);
		dest++;
		src++;
	}
}

static void pix_blend75_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((*src)>>2) & TWO_OUT_16)) + (((*dest)>>2) & TWO_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend50_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>1) & ONE_OUT_15) + (((*dest)>>1) & ONE_OUT_15);
		dest++;
		src++;
	}
}

static void pix_blend50_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>1) & ONE_OUT_16) + (((*dest)>>1) & ONE_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend25_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>2) & TWO_OUT_15) + (3*(((*dest)>>2) & TWO_OUT_15));
		dest++;
		src++;
	}
}

static void pix_blend25_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>2) & TWO_OUT_16) + (3*(((*dest)>>2) & TWO_OUT_16));
		dest++;
		src++;
	}
}

// Knightly : the following 6 functions are for display_base_img_blend()
static void pix_blend_recode75_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((rgbmap_current[*src])>>2) & TWO_OUT_15)) + (((*dest)>>2) & TWO_OUT_15);
		dest++;
		src++;
	}
}

static void pix_blend_recode75_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((rgbmap_current[*src])>>2) & TWO_OUT_16)) + (((*dest)>>2) & TWO_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend_recode50_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((rgbmap_current[*src])>>1) & ONE_OUT_15) + (((*dest)>>1) & ONE_OUT_15);
		dest++;
		src++;
	}
}

static void pix_blend_recode50_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((rgbmap_current[*src])>>1) & ONE_OUT_16) + (((*dest)>>1) & ONE_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend_recode25_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((rgbmap_current[*src])>>2) & TWO_OUT_15) + (3*(((*dest)>>2) & TWO_OUT_15));
		dest++;
		src++;
	}
}

static void pix_blend_recode25_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((rgbmap_current[*src])>>2) & TWO_OUT_16) + (3*(((*dest)>>2) & TWO_OUT_16));
		dest++;
		src++;
	}
}

static void pix_outline75_15(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*((colour>>2) & TWO_OUT_15)) + (((*dest)>>2) & TWO_OUT_15);
		dest++;
	}
}

static void pix_outline75_16(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*((colour>>2) & TWO_OUT_16)) + (((*dest)>>2) & TWO_OUT_16);
		dest++;
	}
}

static void pix_outline50_15(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>1) & ONE_OUT_15) + (((*dest)>>1) & ONE_OUT_15);
		dest++;
	}
}

static void pix_outline50_16(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>1) & ONE_OUT_16) + (((*dest)>>1) & ONE_OUT_16);
		dest++;
	}
}

static void pix_outline25_15(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>2) & TWO_OUT_15) + (3*(((*dest)>>2) & TWO_OUT_15));
		dest++;
	}
}

static void pix_outline25_16(PIXVAL *dest, const PIXVAL *, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>2) & TWO_OUT_16) + (3*(((*dest)>>2) & TWO_OUT_16));
		dest++;
	}
}


// will kept the actual values
static blend_proc blend[3];
static blend_proc blend_recode[3];
static blend_proc outline[3];


/**
 * blends a rectangular region with a color
 */
void display_blend_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, int color, int percent_blend )
{
	if(  clip_lr(&xp, &w, clip_rect.x, clip_rect.xx)  &&  clip_lr(&yp, &h, clip_rect.y, clip_rect.yy)  ) {

		const PIXVAL colval = specialcolormap_all_day[color & 0xFF];
		const PIXVAL alpha = (percent_blend*64)/100;

		switch( alpha ) {
			case 0:	// nothing to do ...
				break;

			case 16:
			case 32:
			case 48:
			{
				// fast blending with 1/4 | 1/2 | 3/4 percentage
				blend_proc blend = outline[ (alpha>>4) - 1 ];

				for(  KOORD_VAL y=0;  y<h;  y++  ) {
					blend( textur + xp + (yp+y) * disp_width, NULL, colval, w );
				}
			}
			break;

			case 64:
				// opaque ...
				display_fillbox_wh( xp, yp, w, h, color, false );
				break;

			default:
				// any percentage blending: SLOW!
				if(  blend[0] == pix_blend25_15  ) {
					// 555 BITMAPS
					const PIXVAL r_src = (colval >> 10) & 0x1F;
					const PIXVAL g_src = (colval >> 5) & 0x1F;
					const PIXVAL b_src = colval & 0x1F;
					for(  ;  h>0;  yp++, h--  ) {
						PIXVAL *dest = textur + yp*disp_width + xp;
						const PIXVAL *const end = dest + w;
						while (dest < end) {
							const PIXVAL r_dest = (*dest >> 10) & 0x1F;
							const PIXVAL g_dest = (*dest >> 5) & 0x1F;
							const PIXVAL b_dest = (*dest & 0x1F);
							const PIXVAL r = r_dest + ( ( (r_src - r_dest) * alpha ) >> 6 );
							const PIXVAL g = g_dest + ( ( (g_src - g_dest) * alpha ) >> 6 );
							const PIXVAL b = b_dest + ( ( (b_src - b_dest) * alpha ) >> 6 );
							*dest++ = (r << 10) | (g << 5) | b;
						}
					}
				}
				else {
					// 565 BITMAPS
					const PIXVAL r_src = (colval >> 11);
					const PIXVAL g_src = (colval >> 5) & 0x3F;
					const PIXVAL b_src = colval & 0x1F;
					for(  ;  h>0;  yp++, h--  ) {
						PIXVAL *dest = textur + yp*disp_width + xp;
						const PIXVAL *const end = dest + w;
						while (dest < end) {
							const PIXVAL r_dest = (*dest >> 11);
							const PIXVAL g_dest = (*dest >> 5) & 0x3F;
							const PIXVAL b_dest = (*dest & 0x1F);
							const PIXVAL r = r_dest + ( ( (r_src - r_dest) * alpha ) >> 6 );
							const PIXVAL g = g_dest + ( ( (g_src - g_dest) * alpha ) >> 6 );
							const PIXVAL b = b_dest + ( ( (b_src - b_dest) * alpha ) >> 6 );
							*dest++ = (r << 11) | (g << 5) | b;
						}
					}
				}
				break;
		}
	}
}


static void display_img_blend_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp, int colour, blend_proc p )
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			uint16 runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen > clip_rect.x && xpos < clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);
					p(tp + xpos + left, sp + left, colour, len - left);
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
void display_rezoomed_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char /*player_nr*/, const PLAYER_COLOR_VAL color_index, const int /*daynight*/, const int dirty)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		PIXVAL *sp;
		KOORD_VAL h, reduce_h, skip_lines;

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
			recode_normal_img(n);
		} else if (images[n].recode_flags&FLAG_NORMAL_RECODE) {
			recode_normal_img(n);
		}
		sp = images[n].data;

		// now, since zooming may have change this image
		xp += images[n].x;
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
			// get the real color
			const PIXVAL color = specialcolormap_all_day[color_index & 0xFF];
			// we use function pointer for the blend runs for the moment ...
			blend_proc pix_blend = (color_index&OUTLINE_FLAG) ? outline[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ] : blend[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ];

			// use horzontal clipping or skip it?
			if (xp >= clip_rect.x && xp + w  <= clip_rect.xx) {
				// marking change?
				if (dirty) {
					mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
				}
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend );
			} else if (xp < clip_rect.xx && xp + w > clip_rect.x) {
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend );
				// since height may be reduced, start marking here
				if (dirty) {
					mark_rect_dirty_wc(xp, yp, xp + w - 1, yp + h - 1);
				}
			}
		}
	}
}



// Knightly : For blending or outlining unzoomed image. Adapted from display_base_img() and display_unzoomed_img_blend()
void display_base_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty)
{
	if(  base_tile_raster_width==tile_raster_width  ) {
		// same size => use standard routine
		display_rezoomed_img_blend( n, xp, yp, player_nr, color_index, daynight, dirty );
	}
	else if (n < anz_images) {

		// prissi: now test if visible and clipping needed
		KOORD_VAL x = images[n].base_x + xp;
		KOORD_VAL y = images[n].base_y + yp;
		KOORD_VAL w = images[n].base_w;
		KOORD_VAL h = images[n].base_h;

		if (h == 0 || x >= clip_rect.xx || y >= clip_rect.yy || x + w <= clip_rect.x || y + h <= clip_rect.y) {
			// not visible => we are done
			// happens quite often ...
			return;
		}

		PIXVAL *sp = images[n].base_data;

		// must the height be reduced?
		KOORD_VAL reduce_h = y + h - clip_rect.yy;
		if (reduce_h > 0) {
			h -= reduce_h;
		}

		// vertical lines to skip (only bottom is visible)
		KOORD_VAL skip_lines = clip_rect.y - (int)y;
		if (skip_lines > 0) {
			h -= skip_lines;
			y += skip_lines;
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
			const PIXVAL color = specialcolormap_all_day[color_index & 0xFF];
			blend_proc pix_blend = (color_index&OUTLINE_FLAG) ? outline[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ] : blend_recode[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ];

			// recode is needed only for blending
			if(  !(color_index&OUTLINE_FLAG)  ) {
				// colors for 2nd company color
				if(player_nr>=0) {
					activate_player_color( player_nr, daynight );
				}
				else {
					// no player
					activate_player_color( 0, daynight );
				}
			}

			// use horizontal clipping or skip it?
			if(  x>=clip_rect.x  &&  x+w<=clip_rect.xx  ) {
				if (dirty) {
					mark_rect_dirty_nc(x, y, x + w - 1, y + h - 1);
				}
				display_img_blend_wc( h, x, y, sp, color, pix_blend );
			}
			else {
				if (dirty) {
					mark_rect_dirty_wc(x, y, x + w - 1, y + h - 1);
				}
				display_img_blend_wc( h, x, y, sp, color, pix_blend );
			}
		}
	} // number ok
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


/**
 * Zeichnet ein Pixel
 * @author Hj. Malthaner
 */
static void display_pixel(KOORD_VAL x, KOORD_VAL y, PIXVAL color)
{
	if (x >= clip_rect.x && x < clip_rect.xx && y >= clip_rect.y && y < clip_rect.yy) {
		PIXVAL* const p = textur + x + y * disp_width;

		*p = color;
		mark_tile_dirty(x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT);
	}
}


/**
 * Zeichnet gefuelltes Rechteck
 */
static void display_fb_internal(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, int color, bool dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	if (clip_lr(&xp, &w, cL, cR) && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		int dx = disp_width - w;
		const PIXVAL colval = specialcolormap_all_day[color & 0xFF];
#if !defined( USE_C )  ||  !defined( ALIGN_COPY )
		const uint32 longcolval = (colval << 16) | colval;
#endif

		if (dirty) {
			mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
		}

		do {
#ifdef USE_C
			KOORD_VAL count = w;
#ifdef ALIGN_COPY
			// unfourtunately the GCC > 4.1.x has a bug in the optimizer
			while(  count-- != 0  ) {
				*p++ = colval;
			}
#else
			uint32 *lp;

			if(  count & 1  ) {
				*p++ = colval;
			}
			count >>= 1;
			lp = (uint32 *)p;
			while(  count-- != 0  ) {
				*lp++ = longcolval;
			}
			p = (PIXVAL *)lp;
#endif
#else
			unsigned int count = w;
			asm volatile (
				// uneven words to copy?
				"shrl %1\n\t"
				"jnc 0f\n\t"
				// set first word
				"stosw\n\t"
				"0:\n\t"
				// now we set long words ...
				"rep\n\t"
				"stosl"
				: "+D" (p), "+c" (count)
				: "a" (longcolval)
				: "cc", "memory"
			);
#endif
			p += dx;
		} while (--h != 0);
	}
}


void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, bool dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, bool dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet vertikale Linie
 * @author Hj. Malthaner
 */
static void display_vl_internal(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	if (xp >= cL && xp < cR && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const PIXVAL colval =specialcolormap_all_day[color & 0xFF];

		if (dirty) mark_rect_dirty_nc(xp, yp, xp, yp + h - 1);

		do {
			*p = colval;
			p += disp_width;
		} while (--h != 0);
	}
}


void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, bool dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, bool dirty)
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
		const COLOR_VAL *arr_src = arr;

		mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		if (xp == clip_rect.x) arr_src += xoff;
		if (yp == clip_rect.y) arr_src += yoff * arr_w;

		do {
			unsigned int ww = w;

			do {
				*p++ = specialcolormap_all_day[*arr_src++];
			} while (--ww > 0);
			arr_src += arr_w - w;
			p += disp_width - w;
		} while (--h != 0);
	}
}


// --------------------------------- text rendering stuff ------------------------------


int	display_set_unicode(int use_unicode)
{
	return has_unicode = (use_unicode != 0);
}


bool display_load_font(const char* fname)
{
	font_type fnt;
	if (load_font(&fnt, fname)) {
		free(large_font.screen_width);
		free(large_font.char_data);
		large_font = fnt;
		large_font_ascent = large_font.height + large_font.descent;
		large_font_total_height = large_font.height;
		return true;
	}
	else {
		return false;
	}
}


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos)
{
	if (has_unicode) {
		return utf8_get_next_char((const utf8*)text, pos);
	}
	else {
		return pos + 1;
	}
}


long get_prev_char(const char* text, long pos)
{
	if (pos <= 0) {
		return 0;
	}
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
	size_t len;
	unsigned short char_code;
	if(  has_unicode  ) {
		len = 0;
		char_code = utf8_to_utf16((const utf8 *)text, &len);
	}
	else {
		len = 1;
		char_code = (unsigned char)(*text);
	}
	if(  char_code==0  ) {
		// case : end of text reached -> do not advance text pointer
		byte_length = 0;
		pixel_width = 0;
	}
	else {
		text += len;
		byte_length = len;
		if(  char_code>=large_font.num_chars  ||  (pixel_width=large_font.screen_width[char_code])==0  ) {
			// default width for missing characters
			pixel_width = large_font.screen_width[0];
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
	}
	else {
		--text;
		char_code = (unsigned char)(*text);
		byte_length = 1;
	}
	if(  char_code>=large_font.num_chars  ||  (pixel_width=large_font.screen_width[char_code])==0  ) {
		// default width for missing characters
		pixel_width = large_font.screen_width[0];
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
	unsigned int width = 0;
	int w;

#ifdef UNICODE_SUPPORT
	if (has_unicode) {
		unsigned short iUnicode;
		size_t	iLen = 0;

		// decode char; Unicode is always 8 pixel (so far)
		while (iLen < len) {
			iUnicode = utf8_to_utf16((utf8 const*)text + iLen, &iLen);
			if (iUnicode == 0) {
				return width;
			}
			else if(iUnicode>=fnt->num_chars  ||  (w = fnt->screen_width[iUnicode])==0  ) {
				// default width for missing characters
				w = fnt->screen_width[0];
			}
			width += w;
		}
	} else {
#endif
		uint8 char_width;
		unsigned int c;
		while(  *text != 0  &&  len > 0  ) {
			c = (unsigned char)*text;
			if(  c>=fnt->num_chars  ||  (char_width=fnt->screen_width[c])==0  ) {
				// default width for missing characters
				char_width = fnt->screen_width[0];
			}
			width += char_width;
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
	if (xR <= cL || xL >= cR) return 0;
	mask = 0xFF;
	// check for left border
	if (xL < cL && xR > cL) {
		// Left border clipped
		mask = byte_to_mask_array[cL - xL];
	}
	// check for right border
	if (xL < cR && xR > cR) {
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
	const font_type* const fnt = &large_font;
	KOORD_VAL cL, cR, cT, cB;
	uint32 c;
	size_t iTextPos = 0; // pointer on text position: prissi
	int char_width_1, char_width_2; // 1 is char only, 2 includes room
	int screen_pos;
	const uint8 *char_data;
	const uint8 *p;
	KOORD_VAL yy = y + fnt->height;
	KOORD_VAL x0;	// store the inital x (for dirty marking)
	KOORD_VAL y_offset, char_height;	// real y for display with clipping
	unsigned char mask1, mask2;	// for horizontal clipping
	const PIXVAL color = specialcolormap_all_day[color_index & 0xFF];
#ifndef USE_C
	// faster drawing with assembler
	const uint32 color2 = (color << 16) | color;
#endif

	// TAKE CARE: Clipping area may be larger than actual screen size ...
	if (flags & DT_CLIP) {
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
	if (x >= cR || y >= cB || y + fnt->height <= cT) {
		// nothing to display
		return 0;
	}

	// x0 contains the startin x
	x0 = x;
	y_offset = 0;
	char_height = fnt->height;
	// calculate vertical y clipping parameters
	if (y < cT) {
		y_offset = cT - y;
	}
	if (yy > cB) {
		char_height -= yy - cB;
	}

	// big loop, char by char
	while (iTextPos < (size_t)len  &&  txt[iTextPos] != 0) {
		int h;
		uint8 char_yoffset;

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
		char_yoffset = (sint8)char_data[CHARACTER_LEN-2];
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
			char_yoffset = (uint8)y_offset;
		}
		screen_pos = (y+char_yoffset) * disp_width + x;

		p = char_data + char_yoffset;
		for (h = char_yoffset; h < char_height; h++) {
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

		// extra four bits for overwidth characters (up to 12 pixel supported for unicode)
		if (char_width_1 > 8 && mask2 != 0) {
			p = char_data + char_yoffset/2+12;
			screen_pos = (y+char_yoffset) * disp_width + x + 8;
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
	display_fillbox_wh(x1, y1,         w, 1, tl_color, true);
	display_fillbox_wh(x1, y1 + h - 1, w, 1, rd_color, true);

	h -= 2;

	display_vline_wh(x1,         y1 + 1, h, tl_color, true);
	display_vline_wh(x1 + w - 1, y1 + 1, h, rd_color, true);
}


void display_outline_proportional(KOORD_VAL xpos, KOORD_VAL ypos, PLAYER_COLOR_VAL text_color, PLAYER_COLOR_VAL shadow_color, const char *text, int dirty)
{
	const int flags = ALIGN_LEFT | DT_CLIP | (dirty ? DT_DIRTY : 0);
	display_text_proportional_len_clip(xpos - 1, ypos - 1 + (12 - large_font_total_height) / 2, text, flags, shadow_color, -1);
	display_text_proportional_len_clip(xpos + 1, ypos + 1 + (12 - large_font_total_height) / 2, text, flags, shadow_color, -1);
	display_text_proportional_len_clip(xpos, ypos + (12 - large_font_total_height) / 2, text, flags, text_color, -1);
}


void display_shadow_proportional(KOORD_VAL xpos, KOORD_VAL ypos, PLAYER_COLOR_VAL text_color, PLAYER_COLOR_VAL shadow_color, const char *text, int dirty)
{
	const int flags = ALIGN_LEFT | DT_CLIP | (dirty ? DT_DIRTY : 0);
	display_text_proportional_len_clip(xpos + 1, ypos + 1 + (12 - large_font_total_height) / 2, text, flags, shadow_color, -1);
	display_text_proportional_len_clip(xpos, ypos + (12 - large_font_total_height) / 2, text, flags, text_color, -1);
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color)
{
	display_fillbox_wh_clip(x1, y1,         w, 1, tl_color, true);
	display_fillbox_wh_clip(x1, y1 + h - 1, w, 1, rd_color, true);

	h -= 2;

	display_vline_wh_clip(x1,         y1 + 1, h, tl_color, true);
	display_vline_wh_clip(x1 + w - 1, y1 + 1, h, rd_color, true);
}


// if width equals zero, take default value
void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_total_height / 2 + 1;

	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt - 1, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh(xpos - 2,         ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh(xpos + width - 3, ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos - halfheight + 1, text, ALIGN_LEFT, text_farbe, -1);
}


/**
 * display text in 3d box with clipping
 * @author: hsiegeln
 */
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_total_height / 2 + 1;

	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - 1 - hgt, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh_clip(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh_clip(xpos - 2,         ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh_clip(xpos + width - 3, ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos - 5 + (12 - large_font_total_height) / 2, text, ALIGN_LEFT | DT_CLIP, text_farbe, -1);
}


/**
 * Zeichnet einen mehrzeiligen Text
 * @author Hj. Malthaner
 *
 * Better performance without copying text!
 * @author Volker Meyer
 * @date  15.06.2003
 */
int display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *buf, PLAYER_COLOR_VAL color)
{
	int max_px_len = 0;
	if (buf != NULL && *buf != '\0') {
		const char *next;

		do {
			next = strchr(buf, '\n');
			const int px_len = display_text_proportional_len_clip(
				x, y, buf,
				ALIGN_LEFT | DT_DIRTY | DT_CLIP, color,
				next != NULL ? (int)(size_t)(next - buf) : -1
			);
			if(  px_len>max_px_len  ) {
				max_px_len = px_len;
			}
			buf = next + 1;
			y += LINESPACE;
		} while (next != NULL);
	}
	return max_px_len;
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
	const PIXVAL colval = specialcolormap_all_day[color & 0xFF];

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	for (i = 0; i <= steps; i++) {
		display_pixel(xp >> 16, yp >> 16, colval);
		xp += xs;
		yp += ys;
	}
}


//taken from function display_direct_line() above, to draw a dotted line: draw=pixels drawn, dontDraw=pixels skipped
void display_direct_line_dotted(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const KOORD_VAL draw, const KOORD_VAL dontDraw, const PLAYER_COLOR_VAL color)
{
	int i, steps;
	int xp, yp;
	int xs, ys;
	int counter=0;
	bool mustDraw=true;

	const int dx = xx - x;
	const int dy = yy - y;
	const PIXVAL colval = specialcolormap_all_day[color & 0xFF];

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	for(  i = 0;  i <= steps;  i++  ) {
		counter ++;
		if(  mustDraw  ) {
			if(  counter == draw  ) {
				mustDraw = !mustDraw;
				counter = 0;
			}
		}
		if(  !mustDraw  ) {
			if(  counter == dontDraw  ) {
				mustDraw=!mustDraw;
				counter=0;
			}
		}

		if(  mustDraw  ) {
			display_pixel( xp >> 16, yp >> 16, colval );
		}
		xp += xs;
		yp += ys;
	}
}


// bresenham circle (from wikipedia ...)
void display_circle( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PLAYER_COLOR_VAL color )
{
	const PIXVAL colval = specialcolormap_all_day[color & 0xFF];
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_pixel( x0, y0 + radius, colval );
	display_pixel( x0, y0 - radius, colval );
	display_pixel( x0 + radius, y0, colval );
	display_pixel( x0 - radius, y0, colval);

	while(x < y) {
		// ddF_x == 2 * x + 1;
		// ddF_y == -2 * y;
		// f == x*x + y*y - radius*radius + 2*x - y + 1;
		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		display_pixel( x0 + x, y0 + y, colval );
		display_pixel( x0 - x, y0 + y, colval );
		display_pixel( x0 + x, y0 - y, colval );
		display_pixel( x0 - x, y0 - y, colval );
		display_pixel( x0 + y, y0 + x, colval );
		display_pixel( x0 - y, y0 + x, colval );
		display_pixel( x0 + y, y0 - x, colval );
		display_pixel( x0 - y, y0 - x, colval );
	}
}


// bresenham circle (from wikipedia ...)
void display_filled_circle( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PLAYER_COLOR_VAL color )
{
	const PIXVAL colval = specialcolormap_all_day[color & 0xFF];
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_fb_internal( x0-radius, y0, radius+radius+1, 1, color, false, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
	display_pixel( x0, y0 + radius, colval );
	display_pixel( x0, y0 - radius, colval );
	display_pixel( x0 + radius, y0, colval );
	display_pixel( x0 - radius, y0, colval);

	while(x < y) {
		// ddF_x == 2 * x + 1;
		// ddF_y == -2 * y;
		// f == x*x + y*y - radius*radius + 2*x - y + 1;
		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		display_fb_internal( x0-x, y0+y, x+x, 1, color, false, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
		display_fb_internal( x0-x, y0-y, x+x, 1, color, false, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);

		display_fb_internal( x0-y, y0+x, y+y, 1, color, false, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
		display_fb_internal( x0-y, y0-x, y+y, 1, color, false, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
	}
	mark_rect_dirty_wc( x0-radius, y0-radius, x0+radius+1, y0+radius+1 );
}


/**
 * Print a bezier curve between points A and B
 * @author yorkeiser
 * @date  08.04.2012
 * @Ax,Ay=start coordinate of Bezier curve
 * @Bx,By=end coordinate of Bezier curve
 * @ADx,ADy=vector for start direction of curve
 * @BDx,BDy=vector for end direction of Bezier curve
 * @colore=color for curve to be drawn
 * @draw=for dotted lines, how many pixels to be drawn (leave 0 for solid line)
 * @dontDraw=for dotted lines, how many pixels to not be drawn (leave 0 for solid line)
 */
void draw_bezier(KOORD_VAL Ax, KOORD_VAL Ay, KOORD_VAL Bx, KOORD_VAL By, KOORD_VAL ADx, KOORD_VAL ADy, KOORD_VAL BDx, KOORD_VAL BDy, const PLAYER_COLOR_VAL colore, KOORD_VAL draw, KOORD_VAL dontDraw)
{
	KOORD_VAL Cx,Cy,Dx,Dy;
	Cx = Ax + ADx;
	Cy = Ay + ADy;
	Dx = Bx + BDx;
	Dy = By + BDy;

	/*	float a,b,rx,ry,oldx,oldy;
	for (float t=0.0;t<=1;t+=0.05)
	{
		a = t;
		b = 1.0 - t;
		if (t>0.0)
		{
			oldx=rx;
			oldy=ry;
		}
		rx = Ax*b*b*b + 3*Cx*b*b*a + 3*Dx*b*a*a + Bx*a*a*a;
		ry = Ay*b*b*b + 3*Cy*b*b*a + 3*Dy*b*a*a + By*a*a*a;
		if (t>0.0)
			if (!draw && !dontDraw)
				display_direct_line(rx,ry,oldx,oldy,colore);
			else
				display_direct_line_dotted(rx,ry,oldx,oldy,draw,dontDraw,colore);
	  }
*/

	sint32 a, b, rx, ry, oldx, oldy;
	// fixed point: we cycle between 0 and 32, rather than 0 and 1
	for(  sint32 t=0;  t<=32;  t++  ) {
		a = t;
		b = 32 - t;
		if(  t > 0  ) {
			oldx = rx;
			oldy = ry;
		}
		rx = Ax*b*b*b + 3*Cx*b*b*a + 3*Dx*b*a*a + Bx*a*a*a;
		ry = Ay*b*b*b + 3*Cy*b*b*a + 3*Dy*b*a*a + By*a*a*a;
		//fixed point: due to cycling between 0 and 32 (2<<5), we divide by 32^3=2>>15 because of cubic interpolation
		if( t > 0  ) {
			if(  !draw  &&  !dontDraw  ) {
				display_direct_line( rx>>15, ry>>15, oldx>>15, oldy>>15, colore );
			}
			else {
				display_direct_line_dotted( rx>>15, ry>>15, oldx>>15, oldy>>15, draw, dontDraw, colore );
			}
		}
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


// draws a progress bar and flushes the display
void display_progress(int part, int total)
{
	const int width=disp_actual_width/2;
	part = (part*width)/total;

	dr_prepare_flush();

	// outline
	display_ddd_box(width/2-2, disp_height/2-9, width+4, 20, COL_GREY6, COL_GREY4);
	display_ddd_box(width/2-1, disp_height/2-8, width+2, 18, COL_GREY4, COL_GREY6);

	// inner
	display_fillbox_wh(width / 2, disp_height / 2 - 7, width, 16, COL_GREY5, true);

	// progress
	display_fillbox_wh(width / 2, disp_height / 2 - 5, part,  12, COL_BLUE,  true);

	if(progress_text) {
		display_proportional(width,disp_height/2-4,progress_text,ALIGN_MIDDLE,COL_WHITE,0);
	}
	dr_flush();
}


// ------------------- other support routines that actually interface with the OS -----------------


/**
 * copies only the changed areas to the screen using the "tile dirty buffer"
 * To get large changes, actually the current and the previous one is used.
 */
void display_flush_buffer(void)
{
	int x, y;
	unsigned char* tmp;

#ifdef USE_SOFTPOINTER
	ex_ord_update_mx_my();

	// use mouse pointer image if available
	if (softpointer != -1 && standard_pointer >= 0) {
		display_color_img(standard_pointer, sys_event.mx, sys_event.my, 0, false, true);

		// if software emulated mouse pointer is over the ticker, redraw it totally at next occurs
		if (!ticker::empty() && sys_event.my+images[standard_pointer].h >= disp_height-TICKER_YPOS_BOTTOM &&
		   sys_event.my <= disp_height-TICKER_YPOS_BOTTOM+TICKER_HEIGHT) {
			ticker::set_redraw_all(true);
		}
	}
	// no pointer image available, draw a crosshair
	else {
		display_fb_internal(sys_event.mx - 1, sys_event.my - 3, 3, 7, COL_WHITE, true, 0, disp_width, 0, disp_height);
		display_fb_internal(sys_event.mx - 3, sys_event.my - 1, 7, 3, COL_WHITE, true, 0, disp_width, 0, disp_height);
		display_direct_line( sys_event.mx-2, sys_event.my, sys_event.mx+2, sys_event.my, COL_BLACK );
		display_direct_line( sys_event.mx, sys_event.my-2, sys_event.mx, sys_event.my+2, COL_BLACK );

		// if crosshair is over the ticker, redraw it totally at next occurs
		if(!ticker::empty() && sys_event.my+2 >= disp_height-TICKER_YPOS_BOTTOM &&
		   sys_event.my-2 <= disp_height-TICKER_YPOS_BOTTOM+TICKER_HEIGHT) {
			ticker::set_redraw_all(true);
		}
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

				display_vline_wh((xl << DIRTY_TILE_SHIFT) - 1, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, false);
				display_vline_wh( x  << DIRTY_TILE_SHIFT,      y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, false);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT,  y << DIRTY_TILE_SHIFT,                        (x - xl) << DIRTY_TILE_SHIFT, 1, 80, false);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT, (y << DIRTY_TILE_SHIFT) + DIRTY_TILE_SIZE - 1, (x - xl) << DIRTY_TILE_SHIFT, 1, 80, false);
			}
			x++;
		} while (x < tiles_per_line);
	}
	dr_textur(0, 0, disp_actual_width, disp_height);
#else
	for (y = 0; y < tile_lines; y++) {
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
	}
#endif

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
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
void simgraph_init(KOORD_VAL width, KOORD_VAL height, int full_screen)
{
	int i;

	disp_actual_width = width;
	disp_height = height;

#if MULTI_THREAD>1
	pthread_mutex_init( &rezoom_recode_img_mutex, NULL );
#endif
	// get real width from os-dependent routines
	disp_width = dr_os_open(width, height, full_screen);
	if(  disp_width>0  ) {

		textur = dr_textur_init();

		// init, load, and check fonts
		large_font.screen_width = NULL;
		large_font.char_data = NULL;
		display_load_font(FONT_PATH_X "prop.fnt");
	}
	else {
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
	for(i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		player_offsets[i][0] = i*8;
		player_offsets[i][1] = i*8+24;
	}

	display_set_clip_wh(0, 0, disp_width, disp_height);

	// Hajo: Calculate daylight rgbmap and save it for unshaded tile drawing
	display_day_night_shift(0);
	memcpy(specialcolormap_all_day, specialcolormap_day_night, 256 * sizeof(PIXVAL));
	memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));

	// find out bit depth
	{
		uint32 c = get_system_color( 0, 255, 0 );
		while((c&1)==0) {
			c >>= 1;
		}
		if(c==31) {
			// 15 bit per pixel
			blend[0] = pix_blend25_15;
			blend[1] = pix_blend50_15;
			blend[2] = pix_blend75_15;
			blend_recode[0] = pix_blend_recode25_15;
			blend_recode[1] = pix_blend_recode50_15;
			blend_recode[2] = pix_blend_recode75_15;
			outline[0] = pix_outline25_15;
			outline[1] = pix_outline50_15;
			outline[2] = pix_outline75_15;
		}
		else {
			blend[0] = pix_blend25_16;
			blend[1] = pix_blend50_16;
			blend[2] = pix_blend75_16;
			blend_recode[0] = pix_blend_recode25_16;
			blend_recode[1] = pix_blend_recode50_16;
			blend_recode[2] = pix_blend_recode75_16;
			outline[0] = pix_outline25_16;
			outline[1] = pix_outline50_16;
			outline[2] = pix_outline75_16;
		}
	}

	printf("Init done.\n");
	fflush(NULL);
}


/**
 * Prueft ob das Grafikmodul schon init. wurde
 * @author Hj. Malthaner
 */
int is_display_init(void)
{
	return textur != NULL  &&  large_font.num_chars>0;
}


/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
void simgraph_exit()
{
	dr_os_close();

	guarded_free(tile_dirty);
	guarded_free(tile_dirty_old);
	display_free_all_images_above(0);
	guarded_free(images);

	tile_dirty = tile_dirty_old = NULL;
	images = NULL;
#if MULTI_THREAD>1
	pthread_mutex_destroy( &rezoom_recode_img_mutex );
#endif
}


/* changes display size
 * @author prissi
 */
void simgraph_resize(KOORD_VAL w, KOORD_VAL h)
{
	disp_actual_width = max( 16, w );
	if(  h<=0  ) {
		h = 64;
	}
	// only resize, if internal values are different
	if (disp_width != w || disp_height != h) {
		KOORD_VAL new_width = dr_textur_resize(&textur, w, h);
		if(  new_width!=disp_width  ||  disp_height != h) {

			disp_width = new_width;
			disp_height = h;

			guarded_free(tile_dirty);
			guarded_free(tile_dirty_old);

			tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
			tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
			tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

			tile_dirty     = MALLOCN(unsigned char, tile_buffer_length);
			tile_dirty_old = MALLOCN(unsigned char, tile_buffer_length);

			memset(tile_dirty,     255, tile_buffer_length);
			memset(tile_dirty_old, 255, tile_buffer_length);

			display_set_clip_wh(0, 0, disp_actual_width, disp_height);
		}
		memset(tile_dirty,     255, tile_buffer_length);
		memset(tile_dirty_old, 255, tile_buffer_length);
	}
}


/**
 * Sets a new value for "textur"
 */
void reset_textur(void *new_textur)
{
	textur=(PIXVAL *)new_textur;
}


/**
 * Speichert Screenshot
 * @author Hj. Malthaner
 */
void display_snapshot( int x, int y, int w, int h )
{
	static int number = 0;

	char buf[80];

	dr_mkdir(SCRENSHOT_PATH);

	// find the first not used screenshot image
	do {
		sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.png", number);
		if(access(buf, W_OK) == -1) {
			sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.bmp", number);
		}
		number ++;
	} while (access(buf, W_OK) != -1);
	sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.bmp", number-1);

	dr_screenshot(buf, x, y, w, h);
}
