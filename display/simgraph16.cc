/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "../macros.h"
#include "../simtypes.h"
#include "font.h"
#include "../pathes.h"
#include "../simconst.h"
#include "../sys/simsys.h"
#include "../simmem.h"
#include "../simdebug.h"
#include "../descriptor/image.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../unicode.h"
#include "../simticker.h"
#include "../utils/simstring.h"
#include "../io/raw_image.h"

#include "../gui/simwin.h"
#include "../dataobj/environment.h"

#include "simgraph.h"

#include "../obj/roadsign.h" // for signal status indicator


#ifdef _MSC_VER
#	include <io.h>
#	define W_OK 2
#else
#	include <unistd.h>
#endif

#ifdef MULTI_THREAD
#include "../utils/simthread.h"

// currently just redrawing/rezooming
static pthread_mutex_t rezoom_img_mutex[MAX_THREADS];
static pthread_mutex_t recode_img_mutex;
#endif

// to pass the extra clipnum when not needed use this
#ifdef MULTI_THREAD
#define CLIPNUM_IGNORE , 0
#else
#define CLIPNUM_IGNORE
#endif


#include "simgraph.h"

// undefine for debugging the update routines
//#define DEBUG_FLUSH_BUFFER


#ifdef USE_SOFTPOINTER
static int softpointer = -1;
#endif
static int standard_pointer = -1;


#ifdef USE_SOFTPOINTER
/*
 * Icon bar needs to redrawn on mouse hover
 */
int old_my = -1;
#endif


/*
 * struct to hold the information about visible area
 * at screen line y
 * associated to some clipline
 */
struct xrange {
	sint64 sx, sy;
	scr_coord_val y;
	bool non_convex_active;
};


class clip_line_t {
private:
	// line from (x0,y0) to (x1 y1)
	// clip (do not draw) everything right from the ray (x0,y0)->(x1,y1)
	// pixels on the ray are not drawn
	// (not_convex) if y0>=y1 then clip along the path (x0,-inf)->(x0,y0)->(x1,y1)
	// (not_convex) if y0<y1  then clip along the path (x0,y0)->(x1,y1)->(x1,+inf)
	int x0, y0;
	int dx, dy;
	sint64 sdy, sdx;
	sint64 inc;
	bool non_convex;

public:
	void clip_from_to(int x0_, int y0_, int x1, int y1, bool non_convex_) {
		x0 = x0_;
		dx = x1 - x0;
		y0 = y0_;
		dy = y1 - y0;
		non_convex = non_convex_;
		int steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
		if(  steps == 0  ) {
			return;
		}
		sdx = ((sint64)dx << 16) / steps;
		sdy = ((sint64)dy << 16) / steps;
		// to stay right from the line
		// left border: xmin <= x
		// right border: x < xmax
		if(  dy > 0  ) {
			if(  dy > dx  ) {
				inc = 1 << 16;
			}
			else {
				inc = ((sint64)dx << 16) / dy -  (1 << 16);
			}
		}
		else if(  dy < 0  ) {
			if(  dy < dx  ) {
				inc = 0; // (+1)-1 << 16;
			}
			else {
				inc = 0;
			}
		}
	}

	// clip if
	// ( x-x0) . (  y1-y0 )
	// ( y-y0) . (-(x1-x0)) < 0
	// -- initialize the clipping
	//    has to be called before image will be drawn
	//    return interval for x coordinate
	inline void get_x_range(scr_coord_val y, xrange &r, bool use_non_convex) const {
		// do everything for the previous row
		y--;
		r.y = y;
		r.non_convex_active = false;
		if(  non_convex  &&  use_non_convex  &&  y < y0  &&  y < (y0 + dy)  ) {
			r.non_convex_active = true;
			y = min(y0, y0+dy) - 1;
		}
		if(  dy != 0  ) {
			// init Bresenham algorithm
			const sint64 t = (((sint64)y - y0) << 16) / sdy;
			// sx >> 16 = x
			// sy >> 16 = y
			r.sx = t * sdx + inc + ((sint64)x0 << 16);
			r.sy = t * sdy + ((sint64)y0 << 16);
		}
	}

	// -- step one line down, return interval for x coordinate
	inline void inc_y(xrange &r, int &xmin, int &xmax) const {
		r.y++;
		// switch between clip vertical and along ray
		if(  r.non_convex_active  ) {
			if(  r.y == min( y0, y0 + dy )  ) {
				r.non_convex_active = false;
			}
			else {
				if(  dy < 0  ) {
					const int r_xmax = x0 + dx;
					if(  xmax > r_xmax  ) {
						xmax = r_xmax;
					}
				}
				else {
					const int r_xmin = x0 + 1;
					if(  xmin < r_xmin  ) {
						xmin = r_xmin;
					}
				}
				return;
			}
		}
		// go along the ray, Bresenham
		if(  dy != 0  ) {
			if(  dy > 0  ) {
				do {
					r.sx += sdx;
					r.sy += sdy;
				} while(  (r.sy >> 16) < r.y  );
				const int r_xmin = r.sx >> 16;
				if(  xmin < r_xmin  ) {
					xmin = r_xmin;
				}
			}
			else {
				do {
					r.sx -= sdx;
					r.sy -= sdy;
				} while(  (r.sy >> 16) < r.y  );
				const int r_xmax = r.sx >> 16;
				if(  xmax > r_xmax  ) {
					xmax = r_xmax;
				}
			}
		}
		// horizontal clip
		else {
			const bool clip = dx * (r.y - y0) > 0;
			if(  clip  ) {
				// invisible row
				xmin = +1;
				xmax = -1;
			}
		}
	}
};

#define MAX_POLY_CLIPS 6

MSVC_ALIGN(64) struct clipping_info_t {
	// current clipping rectangle
	clip_dimension clip_rect;
	// clipping rectangle to be swapped by display_clip_wh_toggle
	clip_dimension clip_rect_swap;
	bool swap_active;
	// poly clipping variables
	int number_of_clips;
	uint8 active_ribi;
	uint8 clip_ribi[MAX_POLY_CLIPS];
	clip_line_t poly_clips[MAX_POLY_CLIPS];
	xrange xranges[MAX_POLY_CLIPS];
} GCC_ALIGN(64); // aligned to separate cachelines

#ifdef MULTI_THREAD
clipping_info_t clips[MAX_THREADS];
#define CR0 clips[0]
#else
clipping_info_t clips;
#define CR0 clips
#endif

#define CR clips CLIP_NUM_INDEX

static font_t default_font;

// needed for resizing gui
int default_font_ascent = 0;
int default_font_linespace = 0;


#define RGBMAPSIZE (0x8000+LIGHT_COUNT+MAX_PLAYER_COUNT)


/*
 * mapping tables for RGB 555 to actual output format
 * plus the special (player, day&night) colors appended
 *
 * 0x0000 - 0x7FFF: RGB colors
 * 0x8000 - 0x800F: Player colors
 * 0x8010 - 0x001F: Day&Night special colors
 * The following transparent colors are not in the colortable
 * 0x8020 - 0xFFE1: 3 4 3 RGB transparent colors in 31 transparency levels
 */
static PIXVAL rgbmap_day_night[RGBMAPSIZE];


/*
 * same as rgbmap_day_night, but always daytime colors
 */
static PIXVAL rgbmap_all_day[RGBMAPSIZE];


/*
 * used by pixel copy functions, is one of rgbmap_day_night
 * rgbmap_all_day
 */
static PIXVAL *rgbmap_current = 0;


/*
 * mapping table for special-colors (AI player colors)
 * to actual output format - day&night mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_day_night[256];


/*
 * mapping table for special-colors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
PIXVAL specialcolormap_all_day[256];


/*
 * contains all color conversions for transparency
 * 16 player colors, 15 special colors and 1024 3 4 3 encoded colors for transparent base
 */
static PIXVAL transparent_map_day_night[MAX_PLAYER_COUNT+LIGHT_COUNT+1024];
static PIXVAL transparent_map_all_day[MAX_PLAYER_COUNT+LIGHT_COUNT+1024];
//static PIXVAL *transparent_map_current;

/*
 * contains all color conversions for transparency
 * 16 player colors, 15 special colors and 1024 3 4 3 encoded colors for transparent base
 */
static uint8 transparent_map_day_night_rgb[(MAX_PLAYER_COUNT+LIGHT_COUNT+1024)*4];
static uint8 transparent_map_all_day_rgb[(MAX_PLAYER_COUNT+LIGHT_COUNT+1024)*4];
//static uint8 *transparent_map_current_rgb;

// offsets of first and second company color
static uint8 player_offsets[MAX_PLAYER_COUNT][2];


/*
 * Image map descriptor structure
 */
struct imd {
	sint16 x; // current (zoomed) min x offset
	sint16 y; // current (zoomed) min y offset
	sint16 w; // current (zoomed) width
	sint16 h; // current (zoomed) height

	uint8 recode_flags;
	uint16 player_flags; // bit # is player number, ==1 cache image needs recoding

	PIXVAL* data[MAX_PLAYER_COUNT]; // current data - zoomed and recolored (player + daynight)

	PIXVAL* zoom_data; // zoomed original data
	uint32 len;    // current zoom image data size (or base if not zoomed) (used for allocation purposes only)

	sint16 base_x; // min x offset
	sint16 base_y; // min y offset
	sint16 base_w; // width
	sint16 base_h; // height

	PIXVAL* base_data; // original image data
};

// Flags for recoding
#define FLAG_HAS_PLAYER_COLOR (1)
#define FLAG_HAS_TRANSPARENT_COLOR (2)
#define FLAG_ZOOMABLE (4)
#define FLAG_REZOOM (8)
//#define FLAG_POSITION_CHANGED (16)

#define TRANSPARENT_RUN (0x8000u)

// different masks needed for RGB 555 and RGB 565 for blending
#define ONE_OUT_16 (0x7bef)
#define TWO_OUT_16 (0x39E7)
#define ONE_OUT_15 (0x3DEF)
#define TWO_OUT_15 (0x1CE7)

static int bitdepth = 16;

static scr_coord_val disp_width  = 640;
static scr_coord_val disp_actual_width  = 640;
static scr_coord_val disp_height = 480;


/*
 * Static buffers for rezoom_img()
 */
static uint8 *rezoom_baseimage[MAX_THREADS];
static PIXVAL *rezoom_baseimage2[MAX_THREADS];
static size_t rezoom_size[MAX_THREADS];

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
 * dirty tile management structures
 */
#define DIRTY_TILE_SIZE 16
#define DIRTY_TILE_SHIFT 4

static uint32 *tile_dirty = NULL;
static uint32 *tile_dirty_old = NULL;

static int tiles_per_line = 0;
static int tile_buffer_per_line = 0; // number of tiles that fit the allocated buffer per line - maintain alignment - x=0 is always first bit in a word for each row
static int tile_lines = 0;
static int tile_buffer_length = 0;


static int light_level = 0;
static int night_shift = -1;


/*
 * special colors during daytime
 */
uint8 display_day_lights[LIGHT_COUNT*3] = {
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
	0xE1, 0x00, 0xE1, // purple light for signals
	0x01, 0x01, 0xFF, // blue light
};


/*
 * special colors during nighttime
 */
uint8 display_night_lights[LIGHT_COUNT*3] = {
	0xD3, 0xC3, 0x80, // Dark windows, lit yellowish at night
	0x80, 0xC3, 0xD3, // Lighter windows, lit blueish at night
	0xFF, 0xFF, 0x53, // Yellow light
	0xFF, 0x21, 0x1D, // Red light
	0x01, 0xDD, 0x01, // Green light
	0x6B, 0x6B, 0x6B, // Non-darkening grey 1 (menus)
	0x9B, 0x9B, 0x9B, // Non-darkening grey 2 (menus)
	0xB3, 0xB3, 0xB3, // non-darkening grey 3 (menus)
	0xC9, 0xC9, 0xC9, // Non-darkening grey 4 (menus)
	0xDF, 0xDF, 0xDF, // Non-darkening grey 5 (menus)
	0xFF, 0xFF, 0xE3, // Nearly white light at day, yellowish light at night
	0xD3, 0xC3, 0x80, // Windows, lit yellow
	0xD3, 0xC3, 0x80, // Windows, lit yellow
	0xE1, 0x00, 0xE1, // purple light for signals
	0x01, 0x01, 0xFF, // blue light
};


// the players colors and colors for simple drawing operations
// each three values correspond to a color, each 8 colors are a player color
static const uint8 special_pal[SPECIAL_COLOR_COUNT*3] =
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
	235, 118, 0,
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
	180, 180, 0,
	193, 193, 0,
	215, 215, 0,
	235, 235, 0,
	255, 255, 0,
	255, 255, 64,
	255, 255, 128,
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
 * tile raster width
 */
scr_coord_val tile_raster_width = 16;      // zoomed
scr_coord_val base_tile_raster_width = 16; // original

// variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = NULL;
display_image_proc display_color = NULL;
display_blend_proc display_blend = NULL;
display_alpha_proc display_alpha = NULL;
signed short current_tile_raster_width = 0;


/*
 * Zoom factor
 */
#define MAX_ZOOM_FACTOR (9)
#define ZOOM_NEUTRAL (3)
static uint32 zoom_factor = ZOOM_NEUTRAL;
static sint32 zoom_num[MAX_ZOOM_FACTOR+1] = { 2, 3, 4, 1, 3, 5, 1, 3, 1, 1 };
static sint32 zoom_den[MAX_ZOOM_FACTOR+1] = { 1, 2, 3, 1, 4, 8, 2, 8, 4, 8 };

/*
 * Gets a colour index and returns RGB888
 */
uint32 get_color_rgb(uint8 idx)
{
	// special_pal has 224 rgb colors
	if (idx < SPECIAL_COLOR_COUNT) {
		return special_pal[idx*3 + 0]<<16 | special_pal[idx*3 + 1]<<8 | special_pal[idx*3 + 2];
	}

	// if it uses one of the special light colours it's under display_day_lights
	if (idx < SPECIAL_COLOR_COUNT + LIGHT_COUNT) {
		const uint8 lidx = idx - SPECIAL_COLOR_COUNT;
		return display_day_lights[lidx*3 + 0]<<16 | display_day_lights[lidx*3 + 1]<<8 | display_day_lights[lidx*3 + 2];
	}

	// Return black for anything else
	return 0;
}

/**
 * Convert indexed colors to rgb and back
 */
PIXVAL color_idx_to_rgb(PIXVAL idx)
{
	return (specialcolormap_all_day[(idx)&0x00FF]);
}

PIXVAL color_rgb_to_idx(PIXVAL color)
{
	for(PIXVAL i=0; i<=0xff; i++) {
		if (specialcolormap_all_day[i] == color) {
			return i;
		}
	}
	return 0;
}


/*
 * Convert env_t colours from RGB888 to the system format
 */
void env_t_rgb_to_system_colors()
{
	// get system colours for the default colours or settings.xml
	uint32 rgb = env_t::default_window_title_color_rgb;
	env_t::default_window_title_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::front_window_text_color_rgb;
	env_t::front_window_text_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::bottom_window_text_color_rgb;
	env_t::bottom_window_text_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::tooltip_color_rgb;
	env_t::tooltip_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::tooltip_textcolor_rgb;
	env_t::tooltip_textcolor = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::cursor_overlay_color_rgb;
	env_t::cursor_overlay_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );

	rgb = env_t::background_color_rgb;
	env_t::background_color = get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );
}


/* changes the raster width after loading */
scr_coord_val display_set_base_raster_width(scr_coord_val new_raster)
{
	scr_coord_val old = base_tile_raster_width;
	base_tile_raster_width = new_raster;
	tile_raster_width = (new_raster *  zoom_num[zoom_factor]) / zoom_den[zoom_factor];
	return old;
}


// ----------------------------------- clipping routines ------------------------------------------


scr_coord_val display_get_width()
{
	return disp_actual_width;
}


// only use, if you are really really sure!
void display_set_actual_width(scr_coord_val w)
{
	disp_actual_width = w;
}


scr_coord_val display_get_height()
{
	return disp_height;
}


void display_set_height(scr_coord_val const h)
{
	disp_height = h;
}


/**
 * Clips intervall [x,x+w] such that left <= x and x+w <= right.
 * If @p w is negative, it stays negative.
 * @returns difference between old and new value of @p x.
 */
inline int clip_intv(scr_coord_val &x, scr_coord_val &w, const scr_coord_val left, const scr_coord_val right)
{
	scr_coord_val xx = min(x+w, right);
	scr_coord_val xoff = left - x;
	if (xoff > 0) { // equivalent to x < left
		x = left;
	}
	else {
		xoff = 0;
	}
	w = xx - x;
	return xoff;
}

/// wrapper for clip_intv
static int clip_wh(scr_coord_val *x, scr_coord_val *w, const scr_coord_val left, const scr_coord_val right)
{
	return clip_intv(*x, *w, left, right);
}


/// wrapper for clip_intv, @returns whether @p w is positive
static bool clip_lr(scr_coord_val *x, scr_coord_val *w, const scr_coord_val left, const scr_coord_val right)
{
	clip_intv(*x, *w, left, right);
	return *w > 0;
}


/**
 * Get the clipping rectangle dimensions
 */
clip_dimension display_get_clip_wh(CLIP_NUM_DEF0)
{
	return CR.clip_rect;
}


/**
 * Set the clipping rectangle dimensions
 *
 * here, a pixel at coordinate xp is displayed if
 *  clip. x <= xp < clip.xx
 * the right-most pixel of an image located at xp with width w is displayed if
 *  clip.x < xp+w <= clip.xx
 * analogously for the y coordinate
 */
void display_set_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF, bool fit)
{
	if (!fit) {
		clip_wh( &x, &w, 0, disp_width);
		clip_wh( &y, &h, 0, disp_height);
	}
	else {
		clip_wh( &x, &w, CR.clip_rect.x, CR.clip_rect.xx);
		clip_wh( &y, &h, CR.clip_rect.y, CR.clip_rect.yy);
	}

	CR.clip_rect.x = x;
	CR.clip_rect.y = y;
	CR.clip_rect.w = w;
	CR.clip_rect.h = h;
	CR.clip_rect.xx = x + w; // watch out, clips to scr_coord_val max
	CR.clip_rect.yy = y + h; // watch out, clips to scr_coord_val max
}

void display_push_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF)
{
	assert(!CR.swap_active);
	// save active clipping rectangle
	CR.clip_rect_swap = CR.clip_rect;
	// active rectangle provided by parameters
	display_set_clip_wh(x, y, w, h  CLIP_NUM_PAR);
	CR.swap_active = true;
}

void display_swap_clip_wh(CLIP_NUM_DEF0)
{
	if (CR.swap_active) {
		// swap clipping rectangles
		clip_dimension save = CR.clip_rect;
		CR.clip_rect = CR.clip_rect_swap;
		CR.clip_rect_swap = save;
	}
}

void display_pop_clip_wh(CLIP_NUM_DEF0)
{
	if (CR.swap_active) {
		// swap original clipping rectangle back
		CR.clip_rect   = CR.clip_rect_swap;
		CR.swap_active = false;
	}
}

/*
 * Add clipping line through (x0,y0) and (x1,y1)
 * with associated ribi
 * if ribi & 16 then non-convex clipping.
 */
void add_poly_clip(int x0,int y0, int x1, int y1, int ribi  CLIP_NUM_DEF)
{
	if(  CR.number_of_clips < MAX_POLY_CLIPS  ) {
		CR.poly_clips[CR.number_of_clips].clip_from_to( x0, y0, x1, y1, ribi&16 );
		CR.clip_ribi[CR.number_of_clips] = ribi&15;
		CR.number_of_clips++;
	}
}


/*
 * Clears all clipping lines
 */
void clear_all_poly_clip(CLIP_NUM_DEF0)
{
	CR.number_of_clips = 0;
	CR.active_ribi = 15; // set all to active
}


/*
 * Activates clipping lines associated with ribi
 * ie if clip_ribi[i] & active_ribi
 */
void activate_ribi_clip(int ribi  CLIP_NUM_DEF)
{
	CR.active_ribi = ribi;
}

/*
 * Initialize clipping region for image starting at screen line y
 */
static inline void init_ranges(int y  CLIP_NUM_DEF)
{
	for(  uint8 i = 0;  i < CR.number_of_clips;  i++  ) {
		if(  (CR.clip_ribi[i] & CR.active_ribi)  ) {
			CR.poly_clips[i].get_x_range( y, CR.xranges[i], CR.active_ribi & 16 );
		}
	}
}


/*
 * Returns left/right border of visible area
 * Computes l/r border for the next line (ie y+1)
 * takes also clipping rectangle into account
 */
inline void get_xrange_and_step_y(int &xmin, int &xmax  CLIP_NUM_DEF)
{
	xmin = CR.clip_rect.x;
	xmax = CR.clip_rect.xx;
	for(  uint8 i = 0;  i < CR.number_of_clips;  i++  ) {
		if(  (CR.clip_ribi[i] & CR.active_ribi)  ) {
			CR.poly_clips[i].inc_y( CR.xranges[i], xmin, xmax );
		}
	}
}


// ------------------------------ dirty tile stuff --------------------------------


/*
 * Simutrans keeps a list of dirty areas, i.e. places that received new graphics
 * and must be copied to the screen after an update
 */
static inline void mark_tile_dirty(const int x, const int y)
{
	const int bit = x + y * tile_buffer_per_line;
#if 0
	assert(bit / 8 < tile_buffer_length);
#endif
	tile_dirty[bit >> 5] |= 1 << (bit & 31);
}


/**
 * Mark tile as dirty, with _NO_ clipping
 */
static void mark_rect_dirty_nc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2)
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

	for(  ;  y1 <= y2;  y1++  ) {
		int bit = y1 * tile_buffer_per_line + x1;
		const int end = bit + x2 - x1;
		do {
			tile_dirty[bit >> 5] |= 1 << (bit & 31);
		} while(  ++bit <= end  );
	}
}


/**
 * Mark tile as dirty, with clipping
 */
void mark_rect_dirty_wc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2)
{
	// inside display?
	if(  x2 >= 0  &&  y2 >= 0  &&  x1 < disp_width  &&  y1 < disp_height  ) {
		if(  x1 < 0  ) {
			x1 = 0;
		}
		if(  y1 < 0  ) {
			y1 = 0;
		}
		if(  x2 >= disp_width  ) {
			x2 = disp_width - 1;
		}
		if(  y2 >= disp_height  ) {
			y2 = disp_height - 1;
		}
		mark_rect_dirty_nc( x1, y1, x2, y2 );
	}
}


void mark_rect_dirty_clip(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF)
{
	// inside clip_rect?
	if(  x2 >= CR.clip_rect.x  &&  y2 >= CR.clip_rect.y  &&  x1 < CR.clip_rect.xx  &&  y1 < CR.clip_rect.yy  ) {
		if(  x1 < CR.clip_rect.x  ) {
			x1 = CR.clip_rect.x;
		}
		if(  y1 < CR.clip_rect.y  ) {
			y1 = CR.clip_rect.y;
		}
		if(  x2 >= CR.clip_rect.xx  ) {
			x2 = CR.clip_rect.xx-1;
		}
		if(  y2 >= CR.clip_rect.yy  ) {
			y2 = CR.clip_rect.yy-1;
		}
		mark_rect_dirty_nc( x1, y1, x2, y2 );
	}
}


/**
 * Mark the whole screen as dirty.
 *
 */
void mark_screen_dirty()
{
	memset( tile_dirty, 0xFFFFFFFF, sizeof(uint32) * tile_buffer_length );
}


/**
 * the area of this image need update
 */
void display_mark_img_dirty(image_id image, scr_coord_val xp, scr_coord_val yp)
{
	if(  image < anz_images  ) {
		mark_rect_dirty_wc(
			xp + images[image].x,
			yp + images[image].y,
			xp + images[image].x + images[image].w - 1,
			yp + images[image].y + images[image].h - 1
		);
	}
}


// ------------------------- rendering images for display --------------------------------

/*
 * Simutrans caches player colored images, to allow faster drawing of them
 * They are derived from a base image, which may need zooming too
 */

/**
 * Flag all images for rezoom on next draw
 */
static void rezoom()
{
	for(  image_id n = 0;  n < anz_images;  n++  ) {
		if(  (images[n].recode_flags & FLAG_ZOOMABLE) != 0  &&  images[n].base_h > 0  ) {
			images[n].recode_flags |= FLAG_REZOOM;
		}
	}
}


void set_zoom_factor(int z)
{
	// do not zoom beyond 4 pixels
	if(  (base_tile_raster_width * zoom_num[z]) / zoom_den[z] > 4  ) {
		zoom_factor = z;
		tile_raster_width = (base_tile_raster_width * zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		dbg->message("set_zoom_factor()", "Zoom level now %d (%i/%i)", zoom_factor, zoom_num[zoom_factor], zoom_den[zoom_factor] );
		rezoom();
	}
}


int zoom_factor_up()
{
	// zoom out, if size permits
	if(  zoom_factor > 0  ) {
		set_zoom_factor( zoom_factor-1 );
		return true;
	}
	return false;
}


int zoom_factor_down()
{
	if(  zoom_factor < MAX_ZOOM_FACTOR  ) {
		set_zoom_factor( zoom_factor+1 );
		return true;
	}
	return false;
}


static uint8 player_night=0xFF;
static uint8 player_day=0xFF;
static void activate_player_color(sint8 player_nr, bool daynight)
{
	// caches the last settings
	if(!daynight) {
		if(player_day!=player_nr) {
			int i;
			player_day = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_all_day[0x8000+i] = specialcolormap_all_day[player_offsets[player_day][0]+i];
				rgbmap_all_day[0x8008+i] = specialcolormap_all_day[player_offsets[player_day][1]+i];
#ifdef RGB555
				transparent_map_all_day[i] = (specialcolormap_all_day[player_offsets[player_day][0] + i] >> 2) & TWO_OUT_15;
				transparent_map_all_day[i + 8] = (specialcolormap_all_day[player_offsets[player_day][1] + i] >> 2) & TWO_OUT_15;
				// those save RGB components
				transparent_map_all_day_rgb[i * 4 + 0] = specialcolormap_all_day[player_offsets[player_day][0] + i] >> 10;
				transparent_map_all_day_rgb[i * 4 + 1] = (specialcolormap_all_day[player_offsets[player_day][0] + i] >> 5) & 0x31;
				transparent_map_all_day_rgb[i * 4 + 2] = specialcolormap_all_day[player_offsets[player_day][0] + i] & 0x1F;
				transparent_map_all_day_rgb[i * 4 + 0 + 32] = specialcolormap_all_day[player_offsets[player_day][1] + i] >> 10;
				transparent_map_all_day_rgb[i * 4 + 1 + 32] = (specialcolormap_all_day[player_offsets[player_day][1] + i] >> 5) & 0x1F;
				transparent_map_all_day_rgb[i * 4 + 2 + 32] = specialcolormap_all_day[player_offsets[player_day][1] + i] & 0x1F;
#else
				transparent_map_all_day[i] = (specialcolormap_all_day[player_offsets[player_day][0] + i] >> 2) & TWO_OUT_16;
				transparent_map_all_day[i + 8] = (specialcolormap_all_day[player_offsets[player_day][1] + i] >> 2) & TWO_OUT_16;
				// those save RGB components
				transparent_map_all_day_rgb[i * 4 + 0] = specialcolormap_all_day[player_offsets[player_day][0] + i] >> 11;
				transparent_map_all_day_rgb[i * 4 + 1] = (specialcolormap_all_day[player_offsets[player_day][0] + i] >> 5) & 0x3F;
				transparent_map_all_day_rgb[i * 4 + 2] = specialcolormap_all_day[player_offsets[player_day][0] + i] & 0x1F;
				transparent_map_all_day_rgb[i * 4 + 0 + 32] = specialcolormap_all_day[player_offsets[player_day][1] + i] >> 11;
				transparent_map_all_day_rgb[i * 4 + 1 + 32] = (specialcolormap_all_day[player_offsets[player_day][1] + i] >> 5) & 0x3F;
				transparent_map_all_day_rgb[i * 4 + 2 + 32] = specialcolormap_all_day[player_offsets[player_day][1] + i] & 0x1F;
#endif
			}
		}
		rgbmap_current = rgbmap_all_day;
	}
	else {
		// changing color table
		if(player_night!=player_nr) {
			int i;
			player_night = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[player_night][0]+i];
				rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[player_night][1]+i];
#ifdef RGB555
				transparent_map_day_night[i] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 2) & TWO_OUT_15;
				transparent_map_day_night[i + 8] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 2) & TWO_OUT_15;
				// those save RGB components
				transparent_map_day_night_rgb[i * 4 + 0] = specialcolormap_day_night[player_offsets[player_day][0] + i] >> 10;
				transparent_map_day_night_rgb[i * 4 + 1] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 5) & 0x31;
				transparent_map_day_night_rgb[i * 4 + 2] = specialcolormap_day_night[player_offsets[player_day][0] + i] & 0x1F;
				transparent_map_day_night_rgb[i * 4 + 0 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] >> 10;
				transparent_map_day_night_rgb[i * 4 + 1 + 32] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 5) & 0x1F;
				transparent_map_day_night_rgb[i * 4 + 2 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] & 0x1F;
#else
				transparent_map_day_night[i] = (specialcolormap_day_night[player_offsets[player_day][0]+i] >> 2) & TWO_OUT_16;
				transparent_map_day_night[i+8] = (specialcolormap_day_night[player_offsets[player_day][1]+i] >> 2) & TWO_OUT_16;
				// those save RGB components
				transparent_map_day_night_rgb[i*4+0] = specialcolormap_day_night[player_offsets[player_day][0]+i] >> 11;
				transparent_map_day_night_rgb[i*4+1] = (specialcolormap_day_night[player_offsets[player_day][0]+i] >> 5) & 0x3F;
				transparent_map_day_night_rgb[i*4+2] = specialcolormap_day_night[player_offsets[player_day][0]+i] & 0x1F;
				transparent_map_day_night_rgb[i*4+0+32] = specialcolormap_day_night[player_offsets[player_day][1]+i] >> 11;
				transparent_map_day_night_rgb[i*4+1+32] = (specialcolormap_day_night[player_offsets[player_day][1]+i] >> 5) & 0x3F;
				transparent_map_day_night_rgb[i*4+2+32] = specialcolormap_day_night[player_offsets[player_day][1]+i] & 0x1F;
#endif
			}
		}
		rgbmap_current = rgbmap_day_night;
	}
}


/**
 * Flag all images to recode colors on next draw
 */
static void recode()
{
	for(  image_id n = 0;  n < anz_images;  n++  ) {
		images[n].player_flags = 0xFFFF;  // recode all player colors
	}
}


// to switch between 15 bit and 16 bit recoding ...
typedef void (*display_recode_img_src_target_proc)(scr_coord_val h, PIXVAL *src, PIXVAL *target);
static display_recode_img_src_target_proc recode_img_src_target = NULL;


/**
 * Convert a certain image data to actual output data
 */
static void recode_img_src_target_15(scr_coord_val h, PIXVAL *src, PIXVAL *target)
{
	if(  h > 0  ) {
		do {
			uint16 runlen = *target++ = *src++;
			// decode rows
			do {
				// clear run is always ok
				runlen = *target++ = *src++;
				if(  runlen & TRANSPARENT_RUN  ) {
					runlen &= ~TRANSPARENT_RUN;
					while(  runlen--  ) {
						if(  *src < 0x8020+(31*16)  ) {
							// expand transparent player color
							PIXVAL rgb555 = rgbmap_day_night[(*src-0x8020)/31+0x8000];
							PIXVAL alpha = (*src-0x8020) % 31;
							PIXVAL pix = ((rgb555 >> 6) & 0x0380) | ((rgb555 >>  4) & 0x0038) | ((rgb555 >> 2) & 0x07);
							*target++ = 0x8020 + 31*31 + pix*31 + alpha;
							src ++;
						}
						else {
							*target++ = *src++;
						}
					}
				}
				else {
					// now just convert the color pixels
					while(  runlen--  ) {
						*target++ = rgbmap_day_night[*src++];
					}
				}
				// next clear run or zero = end
			} while(  (runlen = *target++ = *src++)  );
		} while(  --h  );
	}
}

static void recode_img_src_target_16(scr_coord_val h, PIXVAL *src, PIXVAL *target)
{
	if(  h > 0  ) {
		do {
			uint16 runlen = *target++ = *src++;
			// decode rows
			do {
				// clear run is always ok
				runlen = *target++ = *src++;
				if(  runlen & TRANSPARENT_RUN  ) {
					runlen &= ~TRANSPARENT_RUN;
					while(  runlen--  ) {
						if(  *src < 0x8020+(31*16)  ) {
							// expand transparent player color
							PIXVAL rgb565 = rgbmap_day_night[(*src-0x8020)/31+0x8000];
							PIXVAL alpha = (*src-0x8020) % 31;
							PIXVAL pix = ((rgb565 >> 6) & 0x0380) | ((rgb565 >>  3) & 0x0078) | ((rgb565 >> 2) & 0x07);
							*target++ = 0x8020 + 31*31 + pix*31 + alpha;
							src ++;
						}
						else {
							*target++ = *src++;
						}
					}
				}
				else {
					// now just convert the color pixels
					while(  runlen--  ) {
						*target++ = rgbmap_day_night[*src++];
					}
				}
				// next clear run or zero = end
			} while(  (runlen = *target++ = *src++)  );
		} while(  --h  );
	}
}


image_id get_image_count()
{
	return anz_images;
}


/**
 * Handles the conversion of an image to the output color
 */
static void recode_img(const image_id n, const sint8 player_nr)
{
	// may this image be zoomed
#ifdef MULTI_THREAD
	pthread_mutex_lock( &recode_img_mutex );
	if(  (images[n].player_flags & (1<<player_nr)) == 0  ) {
		// other thread did already the re-code...
		pthread_mutex_unlock( &recode_img_mutex );
		return;
	}
#endif
	PIXVAL *src = images[n].zoom_data != NULL ? images[n].zoom_data : images[n].base_data;

	if(  images[n].data[player_nr] == NULL  ) {
		images[n].data[player_nr] = MALLOCN( PIXVAL, images[n].len );
	}
	// contains now the player color ...
	activate_player_color( player_nr, true );
	recode_img_src_target( images[n].h, src, images[n].data[player_nr] );
	images[n].player_flags &= ~(1<<player_nr);
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &recode_img_mutex );
#endif
}


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

// zoom-in pixel taking color of above/below and transparency of diagonal neighbor into account
PIXVAL inline zoomin_pixel(uint8 *p, uint8* pab, uint8 *prl, uint8* pdia)
{
	if (p[0] == 255) {
		if ( (pab[0] | prl[0] | pdia[0])==255) {
			return 0x73FE; // pixel and one neighbor transparent -> return transparent
		}
		// pixel transparent but all three neighbors not -> interpolate
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
 */
static void rezoom_img(const image_id n)
{
	// may this image be zoomed
	if(  n < anz_images  &&  images[n].base_h > 0  ) {
#ifdef MULTI_THREAD
		pthread_mutex_lock( &rezoom_img_mutex[n % env_t::num_threads] );
		if(  (images[n].recode_flags & FLAG_REZOOM) == 0  ) {
			// other routine did already the re-zooming ...
			pthread_mutex_unlock( &rezoom_img_mutex[n % env_t::num_threads] );
			return;
		}
#endif
		// we may need night conversion afterwards
		images[n].player_flags = 0xFFFF; // recode all player colors

		//  we recalculate the len (since it may be larger than before)
		// thus we have to free the old caches
		if(  images[n].zoom_data != NULL  ) {
			free( images[n].zoom_data );
			images[n].zoom_data = NULL;
		}
		for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			if(  images[n].data[i] != NULL  ) {
				free( images[n].data[i] );
				images[n].data[i] = NULL;
			}
		}

		// just restore original size?
		if(  zoom_factor == ZOOM_NEUTRAL  ||  (images[n].recode_flags&FLAG_ZOOMABLE) == 0  ) {
			// this we can do be a simple copy ...
			images[n].x = images[n].base_x;
			images[n].w = images[n].base_w;
			images[n].y = images[n].base_y;
			images[n].h = images[n].base_h;
			// recalculate length
			sint16 h = images[n].base_h;
			PIXVAL *sp = images[n].base_data;

			while(  h-- > 0  ) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += (*sp)&(~TRANSPARENT_RUN); // MSVC crashes on (*sp)&(~TRANSPARENT_RUN) + 1 !!!
					sp ++;
				} while(  *sp  );
				sp++;
			}
			images[n].len = (uint32)(size_t)(sp - images[n].base_data);
			images[n].recode_flags &= ~FLAG_REZOOM;
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &rezoom_img_mutex[n % env_t::num_threads] );
#endif
			return;
		}

		// now we want to downsize the image
		// just divide the sizes
		images[n].x = (images[n].base_x * zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].y = (images[n].base_y * zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].w = (images[n].base_w * zoom_num[zoom_factor]) / zoom_den[zoom_factor];
		images[n].h = (images[n].base_h * zoom_num[zoom_factor]) / zoom_den[zoom_factor];

		if(  images[n].h > 0  &&  images[n].w > 0  ) {
			// just recalculate the image in the new size
			PIXVAL *src = images[n].base_data;
			PIXVAL *dest = NULL;
			// embed the baseimage in an image with margin ~ remainder
			const sint16 x_rem = (images[n].base_x * zoom_num[zoom_factor]) % zoom_den[zoom_factor];
			const sint16 y_rem = (images[n].base_y * zoom_num[zoom_factor]) % zoom_den[zoom_factor];
			const sint16 xl_margin = max( x_rem, 0);
			const sint16 xr_margin = max(-x_rem, 0);
			const sint16 yl_margin = max( y_rem, 0);
			const sint16 yr_margin = max(-y_rem, 0);
			// baseimage top-left  corner is at (xl_margin, yl_margin)
			// ...       low-right corner is at (xr_margin, yr_margin)

			sint32 orgzoomwidth = ((images[n].base_w + zoom_den[zoom_factor] - 1 ) / zoom_den[zoom_factor]) * zoom_den[zoom_factor];
			sint32 newzoomwidth = (orgzoomwidth*zoom_num[zoom_factor])/zoom_den[zoom_factor];
			sint32 orgzoomheight = ((images[n].base_h + zoom_den[zoom_factor] - 1 ) / zoom_den[zoom_factor]) * zoom_den[zoom_factor];
			sint32 newzoomheight = (orgzoomheight * zoom_num[zoom_factor]) / zoom_den[zoom_factor];

			// we will unpack, re-sample, pack it

			// thus the unpack buffer must at least fit the window => find out maximum size
			// Note: This value is certainly way bigger than the average size we'll get,
			// but it's the worst scenario possible, a succession of solid - transparent - solid - transparent
			// pattern.
			// This would encode EACH LINE as:
			// 0x0000 (0 transparent) 0x0001 PIXWORD 0x0001 (every 2 pixels, 3 words) 0x0000 (EOL)
			// The extra +1 is to make sure we cover divisions with module != 0
			// We end with an over sized buffer for the normal usage, but since it's re-used for all re-zooms,
			// it's not performance critical and we are safe from all possible inputs.

			size_t new_size = ( ( (newzoomwidth * 3) / 2 ) + 1 + 2) * newzoomheight * sizeof(PIXVAL);
			size_t unpack_size = (xl_margin + orgzoomwidth + xr_margin) * (yl_margin + orgzoomheight + yr_margin) * 4;
			if(  unpack_size > new_size  ) {
				new_size = unpack_size;
			}
			new_size = ((new_size * 128) + 127) / 128; // enlarge slightly to try and keep buffers on their own cacheline for multithreaded access. A portable aligned_alloc would be better.
			if(  rezoom_size[n % env_t::num_threads] < new_size  ) {
				free( rezoom_baseimage2[n % env_t::num_threads] );
				free( rezoom_baseimage[n % env_t::num_threads] );
				rezoom_size[n % env_t::num_threads] = new_size;
				rezoom_baseimage[n % env_t::num_threads]  = MALLOCN( uint8, new_size );
				rezoom_baseimage2[n % env_t::num_threads] = (PIXVAL *)MALLOCN( uint8, new_size );
			}
			memset( rezoom_baseimage[n % env_t::num_threads], 255, new_size ); // fill with invalid data to mark transparent regions

			// index of top-left corner
			uint32 baseoff = 4 * (yl_margin * (xl_margin + orgzoomwidth + xr_margin) + xl_margin);
			sint32 basewidth = xl_margin + orgzoomwidth + xr_margin;

			// now: unpack the image
			for(  sint32 y = 0;  y < images[n].base_h;  ++y  ) {
				uint16 runlen;
				uint8 *p = rezoom_baseimage[n % env_t::num_threads] + baseoff + y * (basewidth * 4);

				// decode line
				runlen = *src++;
				do {
					// clear run
					p += (runlen & ~TRANSPARENT_RUN) * 4;
					// color pixel
					runlen = (*src++) & ~TRANSPARENT_RUN;
					while(  runlen--  ) {
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
				} while(  runlen != 0  );
			}

			// now we have the image, we do a repack then
			dest = rezoom_baseimage2[n % env_t::num_threads];
			switch(  zoom_den[zoom_factor]  ) {
				case 1: {
					assert(zoom_num[zoom_factor]==2);

					// first half row - just copy values, do not fiddle with neighbor colors
					uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff;
					for(  sint16 x = 0;  x < orgzoomwidth;  x++  ) {
						PIXVAL c1 = compress_pixel_transparent( p1 + (x * 4) );
						// now set the pixel ...
						dest[x * 2] = c1;
						dest[x * 2 + 1] = c1;
					}
					// skip one line
					dest += newzoomwidth;

					for(  sint16 y = 0;  y < orgzoomheight - 1;  y++  ) {
						uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + y * (basewidth * 4);
						// copy leftmost pixels
						dest[0] = compress_pixel_transparent( p1 );
						dest[newzoomwidth] = compress_pixel_transparent( p1 + basewidth * 4 );
						for(  sint16 x = 0;  x < orgzoomwidth - 1;  x++  ) {
							uint8 *px1 = p1 + (x * 4);
							// pixel at 2,2 in 2x2 superpixel
							dest[x * 2 + 1] = zoomin_pixel( px1, px1 + 4, px1 + basewidth * 4, px1 + basewidth * 4 + 4 );

							// 2x2 superpixel is transparent but original pixel was not
							// preserve one pixel
							if(  dest[x * 2 + 1] == 0x73FE  &&  px1[0] != 255  &&  dest[x * 2] == 0x73FE  &&  dest[x * 2 - newzoomwidth] == 0x73FE  &&  dest[x * 2 - newzoomwidth - 1] == 0x73FE  ) {
								// preserve one pixel
								dest[x * 2 + 1] = compress_pixel( px1 );
							}

							// pixel at 2,1 in next 2x2 superpixel
							dest[x * 2 + 2] = zoomin_pixel( px1 + 4, px1, px1 + basewidth * 4 + 4, px1 + basewidth * 4 );

							// pixel at 1,2 in next row 2x2 superpixel
							dest[x * 2 + newzoomwidth + 1] = zoomin_pixel( px1 + basewidth * 4, px1 + basewidth * 4 + 4, px1, px1 + 4 );

							// pixel at 1,1 in next row next 2x2 superpixel
							dest[x * 2 + newzoomwidth + 2] = zoomin_pixel( px1 + basewidth * 4 + 4, px1 + basewidth * 4, px1 + 4, px1 );
						}
						// copy rightmost pixels
						dest[2 * orgzoomwidth - 1] = compress_pixel_transparent( p1 + 4 * (orgzoomwidth - 1) );
						dest[2 * orgzoomwidth + newzoomwidth - 1] = compress_pixel_transparent( p1 + 4 * (orgzoomwidth - 1) + basewidth * 4 );
						// skip two lines
						dest += 2 * newzoomwidth;
					}
					// last half row - just copy values, do not fiddle with neighbor colors
					p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + (orgzoomheight - 1) * (basewidth * 4);
					for(  sint16 x = 0;  x < orgzoomwidth;  x++  ) {
						PIXVAL c1 = compress_pixel_transparent( p1 + (x * 4) );
						// now set the pixel ...
						dest[x * 2]   = c1;
						dest[x * 2 + 1] = c1;
					}
					break;
				}
				case 2:
					for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
						uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 0 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p2 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 1 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
							uint8 valid = 0;
							uint8 r = 0, g = 0, b = 0;
							sint16 xreal1 = ((x * zoom_den[zoom_factor] + 0 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal2 = ((x * zoom_den[zoom_factor] + 1 - x_rem) / zoom_num[zoom_factor]) * 4;
							SumSubpixel( p1 + xreal1 );
							SumSubpixel( p1 + xreal2 );
							SumSubpixel( p2 + xreal1 );
							SumSubpixel( p2 + xreal2 );
							if(  valid == 0  ) {
								*dest++ = 0x73FE;
							}
							else if(  valid == 255  ) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) + (((uint16)(g/valid))<<5) + (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 3:
					for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
						uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 0 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p2 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 1 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p3 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 2 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
							uint8 valid = 0;
							uint16 r = 0, g = 0, b = 0;
							sint16 xreal1 = ((x * zoom_den[zoom_factor] + 0 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal2 = ((x * zoom_den[zoom_factor] + 1 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal3 = ((x * zoom_den[zoom_factor] + 2 - x_rem) / zoom_num[zoom_factor]) * 4;
							SumSubpixel( p1 + xreal1 );
							SumSubpixel( p1 + xreal2 );
							SumSubpixel( p1 + xreal3 );
							SumSubpixel( p2 + xreal1 );
							SumSubpixel( p2 + xreal2 );
							SumSubpixel( p2 + xreal3 );
							SumSubpixel( p3 + xreal1 );
							SumSubpixel( p3 + xreal2 );
							SumSubpixel( p3 + xreal3 );
							if(  valid == 0  ) {
								*dest++ = 0x73FE;
							}
							else if(  valid == 255  ) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 4:
					for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
						uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 0 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p2 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 1 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p3 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 2 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p4 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 3 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
							uint8 valid = 0;
							uint16 r = 0, g = 0, b = 0;
							sint16 xreal1 = ((x * zoom_den[zoom_factor] + 0 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal2 = ((x * zoom_den[zoom_factor] + 1 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal3 = ((x * zoom_den[zoom_factor] + 2 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal4 = ((x * zoom_den[zoom_factor] + 3 - x_rem) / zoom_num[zoom_factor]) * 4;
							SumSubpixel( p1 + xreal1 );
							SumSubpixel( p1 + xreal2 );
							SumSubpixel( p1 + xreal3 );
							SumSubpixel( p1 + xreal4 );
							SumSubpixel( p2 + xreal1 );
							SumSubpixel( p2 + xreal2 );
							SumSubpixel( p2 + xreal3 );
							SumSubpixel( p2 + xreal4 );
							SumSubpixel( p3 + xreal1 );
							SumSubpixel( p3 + xreal2 );
							SumSubpixel( p3 + xreal3 );
							SumSubpixel( p3 + xreal4 );
							SumSubpixel( p4 + xreal1 );
							SumSubpixel( p4 + xreal2 );
							SumSubpixel( p4 + xreal3 );
							SumSubpixel( p4 + xreal4 );
							if(  valid == 0  ) {
								*dest++ = 0x73FE;
							}
							else if(  valid == 255  ) {
								*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
							}
							else {
								*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
							}
						}
					}
					break;
				case 8:
					for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
						uint8 *p1 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 0 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p2 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 1 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p3 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 2 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p4 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 3 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p5 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 4 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p6 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 5 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p7 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 6 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						uint8 *p8 = rezoom_baseimage[n % env_t::num_threads] + baseoff + ((y * zoom_den[zoom_factor] + 7 - y_rem) / zoom_num[zoom_factor]) * (basewidth * 4);
						for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
							uint8 valid = 0;
							uint16 r = 0, g = 0, b = 0;
							sint16 xreal1 = ((x * zoom_den[zoom_factor] + 0 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal2 = ((x * zoom_den[zoom_factor] + 1 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal3 = ((x * zoom_den[zoom_factor] + 2 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal4 = ((x * zoom_den[zoom_factor] + 3 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal5 = ((x * zoom_den[zoom_factor] + 4 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal6 = ((x * zoom_den[zoom_factor] + 5 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal7 = ((x * zoom_den[zoom_factor] + 6 - x_rem) / zoom_num[zoom_factor]) * 4;
							sint16 xreal8 = ((x * zoom_den[zoom_factor] + 7 - x_rem) / zoom_num[zoom_factor]) * 4;
							SumSubpixel( p1 + xreal1 );
							SumSubpixel( p1 + xreal2 );
							SumSubpixel( p1 + xreal3 );
							SumSubpixel( p1 + xreal4 );
							SumSubpixel( p1 + xreal5 );
							SumSubpixel( p1 + xreal6 );
							SumSubpixel( p1 + xreal7 );
							SumSubpixel( p1 + xreal8 );
							SumSubpixel( p2 + xreal1 );
							SumSubpixel( p2 + xreal2 );
							SumSubpixel( p2 + xreal3 );
							SumSubpixel( p2 + xreal4 );
							SumSubpixel( p2 + xreal5 );
							SumSubpixel( p2 + xreal6 );
							SumSubpixel( p2 + xreal7 );
							SumSubpixel( p2 + xreal8 );
							SumSubpixel( p3 + xreal1 );
							SumSubpixel( p3 + xreal2 );
							SumSubpixel( p3 + xreal3 );
							SumSubpixel( p3 + xreal4 );
							SumSubpixel( p3 + xreal5 );
							SumSubpixel( p3 + xreal6 );
							SumSubpixel( p3 + xreal7 );
							SumSubpixel( p3 + xreal8 );
							SumSubpixel( p4 + xreal1 );
							SumSubpixel( p4 + xreal2 );
							SumSubpixel( p4 + xreal3 );
							SumSubpixel( p4 + xreal4 );
							SumSubpixel( p4 + xreal5 );
							SumSubpixel( p4 + xreal6 );
							SumSubpixel( p4 + xreal7 );
							SumSubpixel( p4 + xreal8 );
							SumSubpixel( p5 + xreal1 );
							SumSubpixel( p5 + xreal2 );
							SumSubpixel( p5 + xreal3 );
							SumSubpixel( p5 + xreal4 );
							SumSubpixel( p5 + xreal5 );
							SumSubpixel( p5 + xreal6 );
							SumSubpixel( p5 + xreal7 );
							SumSubpixel( p5 + xreal8 );
							SumSubpixel( p6 + xreal1 );
							SumSubpixel( p6 + xreal2 );
							SumSubpixel( p6 + xreal3 );
							SumSubpixel( p6 + xreal4 );
							SumSubpixel( p6 + xreal5 );
							SumSubpixel( p6 + xreal6 );
							SumSubpixel( p6 + xreal7 );
							SumSubpixel( p6 + xreal8 );
							SumSubpixel( p7 + xreal1 );
							SumSubpixel( p7 + xreal2 );
							SumSubpixel( p7 + xreal3 );
							SumSubpixel( p7 + xreal4 );
							SumSubpixel( p7 + xreal5 );
							SumSubpixel( p7 + xreal6 );
							SumSubpixel( p7 + xreal7 );
							SumSubpixel( p7 + xreal8 );
							SumSubpixel( p8 + xreal1 );
							SumSubpixel( p8 + xreal2 );
							SumSubpixel( p8 + xreal3 );
							SumSubpixel( p8 + xreal4 );
							SumSubpixel( p8 + xreal5 );
							SumSubpixel( p8 + xreal6 );
							SumSubpixel( p8 + xreal7 );
							SumSubpixel( p8 + xreal8 );
							if(  valid == 0  ) {
								*dest++ = 0x73FE;
							}
							else if(  valid == 255  ) {
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
			dest = (PIXVAL*)rezoom_baseimage[n % env_t::num_threads];
			for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
				PIXVAL *line = ((PIXVAL *)rezoom_baseimage2[n % env_t::num_threads]) + (y * newzoomwidth);
				PIXVAL count;
				sint16 x = 0;
				uint16 clear_colored_run_pair_count = 0;

				do {
					// check length of transparent pixels
					for(  count = 0;  x < newzoomwidth  &&  line[x] == 0x73FE;  count++, x++  )
						{}
					// first runlength: transparent pixels
					*dest++ = count;
					uint16 has_alpha = 0;
					// copy for non-transparent
					count = 0;
					while(  x < newzoomwidth  &&  line[x] != 0x73FE  ) {
						PIXVAL pixval = line[x++];
						if(  pixval >= 0x8020  &&  !has_alpha  ) {
							if(  count  ) {
								*dest++ = count;
								dest += count;
								count = 0;
								*dest++ = TRANSPARENT_RUN;
							}
							has_alpha = TRANSPARENT_RUN;
						}
						else if(  pixval < 0x8020  &&  has_alpha  ) {
							if(  count  ) {
								*dest++ = count+TRANSPARENT_RUN;
								dest += count;
								count = 0;
								*dest++ = TRANSPARENT_RUN;
							}
							has_alpha = 0;
						}
						count++;
						dest[count] = pixval;
					}

					/*
					 * If it is not the first clear-colored-run pair and its colored run is empty
					 * --> it is superfluous and can be removed by rolling back the pointer
					 */
					if(  clear_colored_run_pair_count > 0  &&  count == 0  ) {
						dest--;
						// this only happens at the end of a line, so no need to increment clear_colored_run_pair_count
					}
					else {
						*dest++ = count+has_alpha; // number of colored pixels
						dest += count; // skip them
						clear_colored_run_pair_count++;
					}
				} while(  x < newzoomwidth  );
				*dest++ = 0; // mark line end
			}

			// something left?
			images[n].w = newzoomwidth;
			images[n].h = newzoomheight;
			if(  newzoomheight > 0  ) {
				const size_t zoom_len = (size_t)(((uint8 *)dest) - ((uint8 *)rezoom_baseimage[n % env_t::num_threads]));
				images[n].len = (uint32)(zoom_len / sizeof(PIXVAL));
				images[n].zoom_data = MALLOCN(PIXVAL, images[n].len);
				assert( images[n].zoom_data );
				memcpy( images[n].zoom_data, rezoom_baseimage[n % env_t::num_threads], zoom_len );
			}
		}
		else {
//			if (images[n].w <= 0) {
//				// h=0 will be ignored, with w=0 there was an error!
//				printf("WARNING: image%d w=0!\n", n);
//			}
			images[n].h = 0;
		}
		images[n].recode_flags &= ~FLAG_REZOOM;
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &rezoom_img_mutex[n % env_t::num_threads] );
#endif
	}
}


// force a certain size on a image (for rescaling tool images)
void display_fit_img_to_width( const image_id n, sint16 new_w )
{
	if(  n < anz_images  &&  images[n].base_h > 0  &&  images[n].w != new_w  ) {
		int old_zoom_factor = zoom_factor;
		for(  int i=0;  i<=MAX_ZOOM_FACTOR;  i++  ) {
			int zoom_w = (images[n].base_w * zoom_num[i]) / zoom_den[i];
			if(  zoom_w <= new_w  ) {
				uint8 old_zoom_flag = images[n].recode_flags & FLAG_ZOOMABLE;
				images[n].recode_flags |= FLAG_REZOOM | FLAG_ZOOMABLE;
				zoom_factor = i;
				rezoom_img(n);
				images[n].recode_flags &= ~FLAG_ZOOMABLE;
				images[n].recode_flags |= old_zoom_flag;
				zoom_factor = old_zoom_factor;
				return;
			}
		}
	}
}


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

	// again the same but for transparent colors
	for (i = 0; i < 0x0400; i++) {
		// RGB 343 input
		int R = (i & 0x0380) >> 2;
		int G = (i & 0x0078) << 1;
		int B = (i & 0x0007) << 5;

		// lines generate all possible colors in 343RGB code - input
		// however the result is in 888RGB - 8bit per channel
		R = (int)(R * RG_night_multiplier);
		G = (int)(G * RG_night_multiplier);
		B = (int)(B * B_night_multiplier);

#ifdef RGB555
		// 15 bit colors form here!
		PIXVAL color = get_system_color(R, G, B);
		transparent_map_day_night[MAX_PLAYER_COUNT + LIGHT_COUNT + i] = (color >> 2) & TWO_OUT_15;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 0] = color >> 10;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 1] = (color >> 5) & 0x1F;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 2] = color & 0x1F;
#else
		// 16 bit colors form here!
		PIXVAL color = get_system_color(R, G, B);
		transparent_map_day_night[MAX_PLAYER_COUNT + LIGHT_COUNT + i] = (color >> 2) & TWO_OUT_16;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 0] = color >> 11;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 1] = (color >> 5) & 0x3F;
		transparent_map_day_night_rgb[(MAX_PLAYER_COUNT + LIGHT_COUNT + i) * 4 + 2] = color & 0x1F;
#endif
	}

	// player color map (and used for map display etc.)
	for (i = 0; i < SPECIAL_COLOR_COUNT; i++) {
		const int R = (int)(special_pal[i*3 + 0] * RG_night_multiplier);
		const int G = (int)(special_pal[i*3 + 1] * RG_night_multiplier);
		const int B = (int)(special_pal[i*3 + 2] * B_night_multiplier);

		specialcolormap_day_night[i] = get_system_color(R, G, B);
	}

	// special light colors (actually, only non-darkening greys should be used)
	for(i=0;  i<LIGHT_COUNT;  i++  ) {
		specialcolormap_day_night[SPECIAL_COLOR_COUNT+i] = get_system_color( display_day_lights[i*3 + 0], display_day_lights[i*3 + 1], display_day_lights[i*3 + 2] );
	}

	// init with black for forbidden colors
	for(i=SPECIAL_COLOR_COUNT+LIGHT_COUNT;  i<256;  i++  ) {
		specialcolormap_day_night[i] = 0;
	}

	// default player colors
	for(i=0;  i<8;  i++  ) {
		rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[0][0]+i];
		rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[0][1]+i];
#ifdef RGB555
		// 15 bit colors from here!
		transparent_map_day_night[i] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 2) & TWO_OUT_15;
		transparent_map_day_night[i + 8] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 2) & TWO_OUT_15;
		transparent_map_day_night_rgb[i * 4 + 0] = specialcolormap_day_night[player_offsets[player_day][0] + i] >> 10;
		transparent_map_day_night_rgb[i * 4 + 1] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 5) & 0x1F;
		transparent_map_day_night_rgb[i * 4 + 2] = specialcolormap_day_night[player_offsets[player_day][0] + i] & 0x1F;
		transparent_map_day_night_rgb[i * 4 + 0 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] >> 10;
		transparent_map_day_night_rgb[i * 4 + 1 + 32] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 5) & 0x1F;
		transparent_map_day_night_rgb[i * 4 + 2 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] & 0x1F;
#else
		// 16 bit colors from here!
		transparent_map_day_night[i] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 2) & TWO_OUT_16;
		transparent_map_day_night[i + 8] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 2) & TWO_OUT_16;
		// save RGB components
		transparent_map_day_night_rgb[i * 4 + 0] = specialcolormap_day_night[player_offsets[player_day][0] + i] >> 11;
		transparent_map_day_night_rgb[i * 4 + 1] = (specialcolormap_day_night[player_offsets[player_day][0] + i] >> 5) & 0x3F;
		transparent_map_day_night_rgb[i * 4 + 2] = specialcolormap_day_night[player_offsets[player_day][0] + i] & 0x1F;
		transparent_map_day_night_rgb[i * 4 + 0 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] >> 11;
		transparent_map_day_night_rgb[i * 4 + 1 + 32] = (specialcolormap_day_night[player_offsets[player_day][1] + i] >> 5) & 0x3F;
		transparent_map_day_night_rgb[i * 4 + 2 + 32] = specialcolormap_day_night[player_offsets[player_day][1] + i] & 0x1F;
#endif
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

		PIXVAL color = get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
		rgbmap_day_night[0x8000 + MAX_PLAYER_COUNT + i] = color;
#ifdef RGB555
		// 15 bit colors from here!
		transparent_map_day_night[i + MAX_PLAYER_COUNT] = (color >> 2) & TWO_OUT_15;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 0] = color >> 10;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 1] = (color >> 5) & 0x1F;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 2] = color & 0x1F;
#else
		// 16 bit colors from here!
		transparent_map_day_night[i + MAX_PLAYER_COUNT] = (color >> 2) & TWO_OUT_16;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 0] = color >> 11;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 1] = (color >> 5) & 0x3F;
		transparent_map_day_night_rgb[(i + MAX_PLAYER_COUNT) * 4 + 2] = color & 0x1F;
#endif
	}

	// convert to RGB xxx
	recode();
}


void display_day_night_shift(int night)
{
	if(  night != night_shift  ) {
		night_shift = night;
		calc_base_pal_from_night_shift(night);
		mark_screen_dirty();
	}
}


// set first and second company color for player
void display_set_player_color_scheme(const int player, const uint8 col1, const uint8 col2 )
{
	if(player_offsets[player][0]!=col1  ||  player_offsets[player][1]!=col2) {
		// set new player colors
		player_offsets[player][0] = col1;
		player_offsets[player][1] = col2;
		if(player==player_day  ||  player==player_night) {
			// and recalculate map (and save it)
			calc_base_pal_from_night_shift(0);
			memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
			memcpy(transparent_map_all_day, transparent_map_day_night, lengthof(transparent_map_day_night) * sizeof(PIXVAL));
			memcpy(transparent_map_all_day_rgb, transparent_map_day_night_rgb, lengthof(transparent_map_day_night_rgb) * sizeof(uint8));
			if(night_shift!=0) {
				calc_base_pal_from_night_shift(night_shift);
			}
			// calc_base_pal_from_night_shift resets player_night to 0
			player_day = player_night;
		}
		recode();
		mark_screen_dirty();
	}
}



void register_image(image_t *image_in)
{
	struct imd *image;

	/* valid image? */
	if(  image_in->len == 0  ||  image_in->h == 0  ) {
		dbg->warning("register_image()", "Ignoring image %d because of missing data", anz_images);
		image_in->imageid = IMG_EMPTY;
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
			dbg->fatal( "register_image", "*** Out of images (more than %li!) ***", anz_images );
		}
		images = REALLOC(images, imd, alloc_images);
	}

	image_in->imageid = anz_images;
	image = &images[anz_images];
	anz_images++;

	image->x = image_in->x;
	image->w = image_in->w;
	image->y = image_in->y;
	image->h = image_in->h;

	image->recode_flags = FLAG_REZOOM;
	if(  image_in->zoomable  ) {
		image->recode_flags |= FLAG_ZOOMABLE;
	}
	image->player_flags = 0xFFFF; // recode all player colors

	// find out if there are really player colors
	for(  PIXVAL *src = image_in->data, y = 0;  y < image_in->h;  ++y  ) {
		uint16 runlen;

		// decode line
		runlen = *src++;
		do {
			// clear run .. nothing to do
			runlen = *src++;
			if(  runlen & TRANSPARENT_RUN  ) {
				image->recode_flags |= FLAG_HAS_TRANSPARENT_COLOR;
				runlen &= ~TRANSPARENT_RUN;
			}
			// no this many color pixel
			while(  runlen--  ) {
				// get rgb components
				PIXVAL s = *src++;
				if(  s>=0x8000  &&  s<0x8010  ) {
					image->recode_flags |= FLAG_HAS_PLAYER_COLOR;
				}
			}
			runlen = *src++;
		} while(  runlen!=0  ); // end of row: runlen == 0
	}

	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		image->data[i] = NULL;
	}

	image->zoom_data = NULL;
	image->len = image_in->len;

	image->base_x = image_in->x;
	image->base_w = image_in->w;
	image->base_y = image_in->y;
	image->base_h = image_in->h;

	// since we do not recode them, we can work with the original data
	image->base_data = image_in->data;

	// now find out, it contains player colors

}


// delete all images above a certain number ...
// (mostly needed when changing climate zones)
void display_free_all_images_above( image_id above )
{
	while(  above < anz_images  ) {
		anz_images--;
		if(  images[anz_images].zoom_data != NULL  ) {
			free( images[anz_images].zoom_data );
		}
		for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			if(  images[anz_images].data[i] != NULL  ) {
				free( images[anz_images].data[i] );
			}
		}
	}
}


// query offsets
void display_get_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < anz_images  ) {
		*xoff = images[image].x;
		*yoff = images[image].y;
		*xw   = images[image].w;
		*yw   = images[image].h;
	}
}


// query un-zoomed offsets
void display_get_base_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < anz_images  ) {
		*xoff = images[image].base_x;
		*yoff = images[image].base_y;
		*xw   = images[image].base_w;
		*yw   = images[image].base_h;
	}
}

// ------------------ display all kind of images from here on ------------------------------


/**
 * Copy Pixel from src to dest
 */
static inline void pixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	// for gcc this seems to produce the optimal code ...
	while (src < end) {
		*dest++ = *src++;
	}
}



#ifdef RGB555
/**
 * Copy pixel, replace player color
 */
static inline void colorpixcopy(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	if (*src < 0x8020) {
		while (src < end) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while (src < end) {
			// a semi-transparent pixel
			uint16 alpha = ((*src - 0x8020) % 31) + 1;
			if ((alpha & 0x07) == 0) {
				const PIXVAL colval = transparent_map_day_night[(*src++ - 0x8020) / 31];
				alpha /= 8;
				*dest = alpha * colval + (4 - alpha) * (((*dest) >> 2) & TWO_OUT_15);
				dest++;
			}
			else {
				uint8* trans_rgb = transparent_map_day_night_rgb + ((*src++ - 0x8020) / 31) * 4;
				const PIXVAL r_src = *trans_rgb++;
				const PIXVAL g_src = *trans_rgb++;
				const PIXVAL b_src = *trans_rgb++;

				const PIXVAL r_dest = (*dest >> 10);
				const PIXVAL g_dest = (*dest >> 5) & 0x1F;
				const PIXVAL b_dest = (*dest & 0x1F);
				const PIXVAL r = r_dest + (((r_src - r_dest) * alpha) >> 5);
				const PIXVAL g = g_dest + (((g_src - g_dest) * alpha) >> 5);
				const PIXVAL b = b_dest + (((b_src - b_dest) * alpha) >> 5);
				*dest++ = (r << 10) | (g << 5) | b;
			}
		}
	}
}
#else
/**
 * Copy pixel, replace player color
 */
static inline void colorpixcopy(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	if (*src < 0x8020) {
		while (src < end) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while (src < end) {
			// a semi-transparent pixel
			uint16 alpha = ((*src - 0x8020) % 31) + 1;
			//assert( *src>=0x8020+16*31 );
			if ((alpha & 0x07) == 0) {
				const PIXVAL colval = transparent_map_day_night[(*src++ - 0x8020) / 31];
				alpha /= 8;
				*dest = alpha * colval + (4 - alpha) * (((*dest) >> 2) & TWO_OUT_16);
				dest++;
			}
			else {
				uint8* trans_rgb = transparent_map_day_night_rgb + ((*src++ - 0x8020) / 31) * 4;
				const PIXVAL r_src = *trans_rgb++;
				const PIXVAL g_src = *trans_rgb++;
				const PIXVAL b_src = *trans_rgb++;

				//				const PIXVAL colval = transparent_map_day_night[(*src++-0x8020)/31];
				//				const PIXVAL r_src = (colval >> 11);
				//				const PIXVAL g_src = (colval >> 5) & 0x3F;
				//				const PIXVAL b_src = colval & 0x1F;
								// all other alphas
				const PIXVAL r_dest = (*dest >> 11);
				const PIXVAL g_dest = (*dest >> 5) & 0x3F;
				const PIXVAL b_dest = (*dest & 0x1F);
				const PIXVAL r = r_dest + (((r_src - r_dest) * alpha) >> 5);
				const PIXVAL g = g_dest + (((g_src - g_dest) * alpha) >> 5);
				const PIXVAL b = b_dest + (((b_src - b_dest) * alpha) >> 5);
				*dest++ = (r << 11) | (g << 5) | b;
			}
		}
	}
}
#endif



#ifdef RGB555
/**
 * Copy pixel, replace player color
 */
static inline void colorpixcopydaytime(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	if (*src < 0x8020) {
		while (src < end) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while (src < end) {
			// a semi-transparent pixel
			uint16 alpha = ((*src - 0x8020) % 31) + 1;
			if ((alpha & 0x07) == 0) {
				const PIXVAL colval = transparent_map_all_day[(*src++ - 0x8020) / 31];
				alpha /= 8;
				*dest = alpha * colval + (4 - alpha) * (((*dest) >> 2) & TWO_OUT_15);
				dest++;
			}
			else {
				uint8* trans_rgb = transparent_map_all_day_rgb + ((*src++ - 0x8020) / 31) * 4;
				const PIXVAL r_src = *trans_rgb++;
				const PIXVAL g_src = *trans_rgb++;
				const PIXVAL b_src = *trans_rgb++;

				const PIXVAL r_dest = (*dest >> 10);
				const PIXVAL g_dest = (*dest >> 5) & 0x1F;
				const PIXVAL b_dest = (*dest & 0x1F);
				const PIXVAL r = r_dest + (((r_src - r_dest) * alpha) >> 5);
				const PIXVAL g = g_dest + (((g_src - g_dest) * alpha) >> 5);
				const PIXVAL b = b_dest + (((b_src - b_dest) * alpha) >> 5);
				*dest++ = (r << 10) | (g << 5) | b;
			}
		}
	}
}
#else
/**
 * Copy pixel, replace player color
 */
static inline void colorpixcopydaytime(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	if (*src < 0x8020) {
		while (src < end) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while (src < end) {
			// a semi-transparent pixel
			uint16 alpha = ((*src - 0x8020) % 31) + 1;
			//assert( *src>=0x8020+16*31 );
			if ((alpha & 0x07) == 0) {
				const PIXVAL colval = transparent_map_all_day[(*src++ - 0x8020) / 31];
				alpha /= 8;
				*dest = alpha * colval + (4 - alpha) * (((*dest) >> 2) & TWO_OUT_16);
				dest++;
			}
			else {
				uint8* trans_rgb = transparent_map_all_day_rgb + ((*src++ - 0x8020) / 31) * 4;
				const PIXVAL r_src = *trans_rgb++;
				const PIXVAL g_src = *trans_rgb++;
				const PIXVAL b_src = *trans_rgb++;

				//				const PIXVAL colval = transparent_map_day_night[(*src++-0x8020)/31];
				//				const PIXVAL r_src = (colval >> 11);
				//				const PIXVAL g_src = (colval >> 5) & 0x3F;
				//				const PIXVAL b_src = colval & 0x1F;
								// all other alphas
				const PIXVAL r_dest = (*dest >> 11);
				const PIXVAL g_dest = (*dest >> 5) & 0x3F;
				const PIXVAL b_dest = (*dest & 0x1F);
				const PIXVAL r = r_dest + (((r_src - r_dest) * alpha) >> 5);
				const PIXVAL g = g_dest + (((g_src - g_dest) * alpha) >> 5);
				const PIXVAL b = b_dest + (((b_src - b_dest) * alpha) >> 5);
				*dest++ = (r << 11) | (g << 5) | b;
			}
		}
	}
}
#endif



/**
 * templated pixel copy routines
 * to be used in display_img_pc
 */
enum pixcopy_routines {
	plain = 0,    /// simply copies the pixels
	colored = 1,  /// replaces player colors
	daytime = 2   /// use daytime color lookup for transparent pixels
};


template<pixcopy_routines copyroutine> void templated_pixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end);


template<> void templated_pixcopy<plain>(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	pixcopy(dest, src, end);
}


template<> void templated_pixcopy<colored>(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	colorpixcopy(dest, src, end);
}


template<> void templated_pixcopy<daytime>(PIXVAL* dest, const PIXVAL* src, const PIXVAL* const end)
{
	colorpixcopydaytime(dest, src, end);
}


/**
 * draws image with clipping along arbitrary lines
 */
template<pixcopy_routines copyroutine>
static void display_img_pc(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		PIXVAL *tp = textur + yp * disp_width;

		// initialize clipping
		init_ranges( yp  CLIP_NUM_PAR);
		do { // line decoder
			int xpos = xp;

			// display image
			int runlen = *sp++;

			// get left/right boundary, step
			int xmin, xmax;
			get_xrange_and_step_y( xmin, xmax  CLIP_NUM_PAR );
			do {
				// we start with a clear run (which may be 0 pixels)
				xpos += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = *sp++;
				uint16 has_alpha = runlen & TRANSPARENT_RUN;
				runlen &= ~TRANSPARENT_RUN;

				// something to display?
				if (xmin < xmax  &&  xpos + runlen > xmin && xpos < xmax) {
					const int left = (xpos >= xmin ? 0 : xmin - xpos);
					const int len  = (xmax - xpos >= runlen ? runlen : xmax - xpos);
					if(  !has_alpha  ) {
						templated_pixcopy<copyroutine>(tp + xpos + left, sp + left, sp + len);
					}
					else {
						colorpixcopy(tp + xpos + left, sp + left, sp + len);
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
 * Draw image with horizontal clipping
 */
static void display_img_wc(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // line decoder
			int xpos = xp;

			// display image
			uint16 runlen = *sp++;

			do {
				// we start with a clear run
				xpos += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = *sp++;
				uint16 has_alpha = runlen & TRANSPARENT_RUN;
				runlen &= ~TRANSPARENT_RUN;

				// something to display?
				if(  xpos + runlen > CR.clip_rect.x  &&  xpos < CR.clip_rect.xx  ) {
					const int left = (xpos >= CR.clip_rect.x ? 0 : CR.clip_rect.x - xpos);
					const int len  = (CR.clip_rect.xx - xpos >= runlen ? runlen : CR.clip_rect.xx - xpos);
					if(  !has_alpha  ) {
						pixcopy(tp + xpos + left, sp + left, sp + len);
					}
					else {
						colorpixcopy(tp + xpos + left, sp + left, sp + len);
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
 * Draw each image without clipping
 */
static void display_img_nc(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + xp + yp * disp_width;

		do { // line decoder
			uint16 runlen = *sp++;
			PIXVAL *p = tp;

			// one line decoder
			do {
				// we start with a clear run
				p += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = *sp++;
				if(  runlen & TRANSPARENT_RUN  ) {
					runlen &= ~TRANSPARENT_RUN;
					colorpixcopy( p, sp, sp+runlen );
					p += runlen;
					sp += runlen;
				}
				else {
#ifdef LOW_LEVEL
#ifdef SIM_BIG_ENDIAN
					// low level c++ without any unrolling
					while(  runlen--  ) {
						*p++ = *sp++;
					}
#else
					// trying to merge reads and writes
					if(  runlen  ) {
						// align to 4 bytes, should use uintptr_t but not available
						if(  reinterpret_cast<size_t>(p) & 0x2  ) {
							*p++ = *sp++;
							runlen--;
						}
						// aligned fast copy loop
						bool const postalign = runlen & 1;
						runlen >>= 1;
						uint32 *ld = (uint32 *)p;
						while (runlen--) {
#if defined _MSC_VER // MSVC can read unaligned
							*ld++ = *(uint32 const *const)sp;
#else
							// little endian order, assumed by default
							*ld++ = (uint32(sp[1]) << 16) | uint32(sp[0]);
#endif
							sp += 2;
						}
						p = (PIXVAL*)ld;
						// finish unaligned remainder
						if(  postalign  ) {
							*p++ = *sp++;
						}
					}
#endif
#else
					// high level c++
					const PIXVAL *const splast = sp + runlen;
					p = std::copy(sp, splast, p);
					sp = splast;
#endif
				}
				runlen = *sp++;
			} while (runlen != 0);

			tp += disp_width;
		} while (--h > 0);
	}
}


// only used for GUI
void display_img_aligned( const image_id n, scr_rect area, int align, const bool dirty)
{
	if(  n < anz_images  ) {
		scr_coord_val x,y;

		// align the image horizontally
		x = area.x;
		if(  (align & ALIGN_RIGHT) == ALIGN_CENTER_H  ) {
			x -= images[n].x;
			x += (area.w-images[n].w)/2;
		}
		else if(  (align & ALIGN_RIGHT) == ALIGN_RIGHT  ) {
			x = area.get_right() - images[n].x - images[n].w;
		}

		// align the image vertically
		y = area.y;
		if(  (align & ALIGN_BOTTOM) == ALIGN_CENTER_V  ) {
			y -= images[n].y;
			y += (area.h-images[n].h)/2;
		}
		else if(  (align & ALIGN_BOTTOM) == ALIGN_BOTTOM  ) {
			y = area.get_bottom() - images[n].y - images[n].h;
		}

		display_color_img( n, x, y, 0, false, dirty  CLIP_NUM_DEFAULT);
	}
}


/**
 * Draw image with vertical clipping (quickly) and horizontal (slowly)
 */
void display_img_aux(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr_raw, const bool /*daynight*/, const bool dirty  CLIP_NUM_DEF)
{
	if(  n < anz_images  ) {
		// only use player images if needed
		const sint8 use_player = (images[n].recode_flags & FLAG_HAS_PLAYER_COLOR) * player_nr_raw;
		// need to go to nightmode and or re-zoomed?
		PIXVAL *sp;

		if(  use_player > 0  ) {
			// player colour images are rezoomed/recoloured in display_color_img
			sp = images[n].data[use_player];
			if(  sp == NULL  ) {
				dbg->warning("display_img_aux", "CImg[%i] %u failed!", use_player, n);
				return;
			}
		}
		else {
			if(  (images[n].recode_flags & FLAG_REZOOM)  ) {
				rezoom_img( n );
				recode_img( n, 0 );
			}
			else if(  (images[n].player_flags & 1)  ) {
				recode_img( n, 0 );
			}
			sp = images[n].data[0];
			if(  sp == NULL  ) {
				dbg->warning("display_img_aux", "Img %u failed!", n);
				return;
			}
		}
		// now, since zooming may have change this image
		yp += images[n].y;
		scr_coord_val h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		scr_coord_val reduce_h = yp + h - CR.clip_rect.yy;
		if(  reduce_h > 0  ) {
			h -= reduce_h;
		}
		// still something to draw
		if(  h <= 0  ) {
			return;
		}

		// vertically lines to skip (only bottom is visible
		scr_coord_val skip_lines = CR.clip_rect.y - (int)yp;
		if(  skip_lines > 0  ) {
			if(  skip_lines >= h  ) {
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
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp ++;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const scr_coord_val w = images[n].w;
			xp += images[n].x;

			// clipping at poly lines?
			if(  CR.number_of_clips > 0  ) {
					display_img_pc<plain>( h, xp, yp, sp  CLIP_NUM_PAR );
					// since height may be reduced, start marking here
					if(  dirty  ) {
						mark_rect_dirty_clip( xp, yp, xp + w - 1, yp + h - 1  CLIP_NUM_PAR );
					}
			}
			else {
				// use horizontal clipping or skip it?
				if(  xp >= CR.clip_rect.x  &&  xp + w <= CR.clip_rect.xx  ) {
					// marking change?
					if(  dirty  ) {
						mark_rect_dirty_nc( xp, yp, xp + w - 1, yp + h - 1 );
					}
					display_img_nc( h, xp, yp, sp );
				}
				else if(  xp < CR.clip_rect.xx  &&  xp + w > CR.clip_rect.x  ) {
					display_img_wc( h, xp, yp, sp  CLIP_NUM_PAR);
					// since height may be reduced, start marking here
					if(  dirty  ) {
						mark_rect_dirty_clip( xp, yp, xp + w - 1, yp + h - 1  CLIP_NUM_PAR );
					}
				}
			}
		}
	}
}


// local helper function for tiles buttons
static void display_three_image_row( image_id i1, image_id i2, image_id i3, scr_rect row, FLAGGED_PIXVAL)
{
	if(  i1!=IMG_EMPTY  ) {
		scr_coord_val w = images[i1].w;
		display_color_img( i1, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
		row.x += w;
		row.w -= w;
	}
	// right
	if(  i3!=IMG_EMPTY  ) {
		scr_coord_val w = images[i3].w;
		display_color_img( i3, row.get_right()-w, row.y, 0, false, true  CLIP_NUM_DEFAULT);
		row.w -= w;
	}
	// middle
	if(  i2!=IMG_EMPTY  ) {
		scr_coord_val w = images[i2].w;
		// tile it wide
		while(  w <= row.w  ) {
			display_color_img( i2, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
			row.x += w;
			row.w -= w;
		}
		// for the rest we have to clip the rectangle
		if(  row.w > 0  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh( cl.x, cl.y, max(0,min(row.get_right(),cl.xx)-cl.x), cl.h );
			display_color_img( i2, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
	}
}

static scr_coord_val get_img_width(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].w : 0;
}
static scr_coord_val get_img_height(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].h : 0;
}

typedef void (*DISP_THREE_ROW_FUNC)(image_id, image_id, image_id, scr_rect, FLAGGED_PIXVAL);

/**
 * Base method to display a 3x3 array of images to fit the scr_rect.
 * Special cases:
 * - if images[*][1] are empty, display images[*][0] vertically aligned
 * - if images[1][*] are empty, display images[0][*] horizontally aligned
 */
static void display_img_stretch_intern( const stretch_map_t &imag, scr_rect area, DISP_THREE_ROW_FUNC display_three_image_rowf, FLAGGED_PIXVAL color)
{
	scr_coord_val h_top    = max(max( get_img_height(imag[0][0]), get_img_height(imag[1][0])), get_img_height(imag[2][0]));
	scr_coord_val h_middle = max(max( get_img_height(imag[0][1]), get_img_height(imag[1][1])), get_img_height(imag[2][1]));
	scr_coord_val h_bottom = max(max( get_img_height(imag[0][2]), get_img_height(imag[1][2])), get_img_height(imag[2][2]));

	// center vertically if images[*][1] are empty, display images[*][0]
	if(  imag[0][1] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[2][1] == IMG_EMPTY  ) {
		scr_coord_val h = max(h_top, get_img_height(imag[1][1]));
		// center vertically
		area.y += (area.h-h)/2;
	}

	// center horizontally if images[1][*] are empty, display images[0][*]
	if(  imag[1][0] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[1][2] == IMG_EMPTY  ) {
		scr_coord_val w_left = max(max( get_img_width(imag[0][0]), get_img_width(imag[0][1])), get_img_width(imag[0][2]));
		// center vertically
		area.x += (area.w-w_left)/2;
	}

	// top row
	display_three_image_rowf( imag[0][0], imag[1][0], imag[2][0], area, color);

	// bottom row
	if(  h_bottom > 0  ) {
		scr_rect row( area.x, area.y+area.h-h_bottom, area.w, h_bottom );
		display_three_image_rowf( imag[0][2], imag[1][2], imag[2][2], row, color);
	}

	// now stretch the middle
	if(  h_middle > 0  ) {
		scr_rect row( area.x, area.y+h_top, area.w, area.h-h_top-h_bottom);
		// tile it wide
		while(  h_middle <= row.h  ) {
			display_three_image_rowf( imag[0][1], imag[1][1], imag[2][1], row, color);
			row.y += h_middle;
			row.h -= h_middle;
		}
		// for the rest we have to clip the rectangle
		if(  row.h > 0  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh( cl.x, cl.y, cl.w, max(0,min(row.get_bottom(),cl.yy)-cl.y) );
			display_three_image_rowf( imag[0][1], imag[1][1], imag[2][1], row, color);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
	}
}

void display_img_stretch( const stretch_map_t &imag, scr_rect area)
{
	display_img_stretch_intern(imag, area, display_three_image_row, 0);
}

static void display_three_blend_row( image_id i1, image_id i2, image_id i3, scr_rect row, FLAGGED_PIXVAL color )
{
	if(  i1!=IMG_EMPTY  ) {
		scr_coord_val w = images[i1].w;
		display_rezoomed_img_blend( i1, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
		row.x += w;
		row.w -= w;
	}
	// right
	if(  i3!=IMG_EMPTY  ) {
		scr_coord_val w = images[i3].w;
		display_rezoomed_img_blend( i3, row.get_right()-w, row.y, 0, color, false, true CLIPNUM_IGNORE );
		row.w -= w;
	}
	// middle
	if(  i2!=IMG_EMPTY  ) {
		scr_coord_val w = images[i2].w;
		// tile it wide
		while(  w <= row.w  ) {
			display_rezoomed_img_blend( i2, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
			row.x += w;
			row.w -= w;
		}
		// for the rest we have to clip the rectangle
		if(  row.w > 0  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh( cl.x, cl.y, max(0,min(row.get_right(),cl.xx)-cl.x), cl.h );
			display_rezoomed_img_blend( i2, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
	}
}


// this displays a 3x3 array of images to fit the scr_rect like above, but blend the color
void display_img_stretch_blend( const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color )
{
	display_img_stretch_intern(imag, area, display_three_blend_row, color);
}


/**
 * Draw Image, replace player color,
 * assumes height is ok and valid data are calculated.
 * color replacement needs the original data => sp points to non-cached data
 */
static void display_color_img_wc(const PIXVAL* sp, scr_coord_val x, scr_coord_val y, scr_coord_val h  CLIP_NUM_DEF)
{
	PIXVAL* tp = textur + y * disp_width;

	do { // line decoder
		int xpos = x;

		// Display image

		uint16 runlen = *sp++;

		do {
			// we start with a clear run
			xpos += (runlen & ~TRANSPARENT_RUN);

			// now get colored pixels
			runlen = (*sp++) & ~TRANSPARENT_RUN; // we recode anyway, so no need to do it explicitely

			// something to display?
			if (xpos + runlen > CR.clip_rect.x && xpos < CR.clip_rect.xx) {
				const int left = (xpos >= CR.clip_rect.x ? 0 : CR.clip_rect.x - xpos);
				const int len = (CR.clip_rect.xx - xpos > runlen ? runlen : CR.clip_rect.xx - xpos);

				colorpixcopy(tp + xpos + left, sp + left, sp + len);
			}

			sp += runlen;
			xpos += runlen;
		} while ((runlen = *sp++));

		tp += disp_width;
	} while (--h);
}


/**
 * Draw Image, replace player color, as above, but uses daytime colors for transparent pixels
 */
static void display_color_img_wc_daytime(const PIXVAL* sp, scr_coord_val x, scr_coord_val y, scr_coord_val h  CLIP_NUM_DEF)
{
	PIXVAL* tp = textur + y * disp_width;

	do { // line decoder
		int xpos = x;

		// Display image

		uint16 runlen = *sp++;

		do {
			// we start with a clear run
			xpos += (runlen & ~TRANSPARENT_RUN);

			// now get colored pixels
			runlen = (*sp++) & ~TRANSPARENT_RUN; // we recode anyway, so no need to do it explicitely

			// something to display?
			if (xpos + runlen > CR.clip_rect.x && xpos < CR.clip_rect.xx) {
				const int left = (xpos >= CR.clip_rect.x ? 0 : CR.clip_rect.x - xpos);
				const int len = (CR.clip_rect.xx - xpos > runlen ? runlen : CR.clip_rect.xx - xpos);

				colorpixcopydaytime(tp + xpos + left, sp + left, sp + len);
			}

			sp += runlen;
			xpos += runlen;
		} while ((runlen = *sp++));

		tp += disp_width;
	} while (--h);
}


/**
 * Draw Image, replaced player color
 */
void display_color_img(const image_id n, scr_coord_val xp, scr_coord_val yp, sint8 player_nr_raw, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  n < anz_images  ) {
		// do we have to use a player nr?
		const sint8 player_nr = (images[n].recode_flags & FLAG_HAS_PLAYER_COLOR) * player_nr_raw;
		// first: size check
		if(  (images[n].recode_flags & FLAG_REZOOM)  ) {
			rezoom_img( n );
		}

		if(  daynight  ||  night_shift == 0  ) {
			// ok, now we could use the same faster code as for the normal images
			if(  (images[n].player_flags & (1<<player_nr))  ) {
				recode_img( n, player_nr );
			}
			display_img_aux( n, xp, yp, player_nr, true, dirty  CLIP_NUM_PAR);
			return;
		}
		else {
		// do player colour substitution but not daynight - can't use cached images. Do NOT call multithreaded.
		// now test if visible and clipping needed
			const scr_coord_val x = images[n].x + xp;
			      scr_coord_val y = images[n].y + yp;
			const scr_coord_val w = images[n].w;
			      scr_coord_val h = images[n].h;
			if(  h <= 0  ||  x >= CR.clip_rect.xx  ||  y >= CR.clip_rect.yy  ||  x + w <= CR.clip_rect.x  ||  y + h <= CR.clip_rect.y  ) {
				// not visible => we are done
				// happens quite often ...
				return;
			}

			if(  dirty  ) {
				mark_rect_dirty_wc( x, y, x + w - 1, y + h - 1 );
			}

			activate_player_color( player_nr, daynight );

			// color replacement needs the original data => sp points to non-cached data
			const PIXVAL *sp = images[n].zoom_data != NULL ? images[n].zoom_data : images[n].base_data;

			// clip top/bottom
			scr_coord_val yoff = clip_wh( &y, &h, CR.clip_rect.y, CR.clip_rect.yy );
			if(  h > 0  ) { // clipping may have reduced it
				// clip top
				while(  yoff  ) {
					yoff--;
					do {
						// clear run + colored run + next clear run
						++sp;
						sp += (*sp) & (~TRANSPARENT_RUN);
						sp++;
					} while (*sp);
					sp++;
				}

				// clipping at poly lines?
				if(  CR.number_of_clips > 0  ) {
					daynight ? display_img_pc<colored>(h, x, y, sp  CLIP_NUM_PAR) : display_img_pc<daytime>(h, x, y, sp  CLIP_NUM_PAR);
				}
				else {
					daynight ? display_color_img_wc(sp, x, y, h  CLIP_NUM_PAR) : display_color_img_wc_daytime(sp, x, y, h  CLIP_NUM_PAR);
				}
			}
		}
	} // number ok
}


/**
 * draw unscaled images, replaces base color
 */
void display_base_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  base_tile_raster_width==tile_raster_width  ) {
		// same size => use standard routine
		display_color_img( n, xp, yp, player_nr, daynight, dirty  CLIP_NUM_PAR);
	}
	else if(  n < anz_images  ) {
		// now test if visible and clipping needed
		const scr_coord_val x = images[n].base_x + xp;
		      scr_coord_val y = images[n].base_y + yp;
		const scr_coord_val w = images[n].base_w;
		      scr_coord_val h = images[n].base_h;

		if(  h <= 0  ||  x >= CR.clip_rect.xx  ||  y >= CR.clip_rect.yy  ||  x + w <= CR.clip_rect.x  ||  y + h <= CR.clip_rect.y  ) {
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
		scr_coord_val yoff = clip_wh( &y, &h, CR.clip_rect.y, CR.clip_rect.yy );
		if(  h > 0  ) { // clipping may have reduced it
			// clip top
			while(  yoff  ) {
				yoff--;
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp++;
				} while (*sp);
				sp++;
			}
			// clipping at poly lines?
			if(  CR.number_of_clips > 0  ) {
				daynight ? display_img_pc<colored>(h, x, y, sp  CLIP_NUM_PAR) : display_img_pc<daytime>(h, x, y, sp  CLIP_NUM_PAR);
			}
			else {
				daynight ? display_color_img_wc(sp, x, y, h  CLIP_NUM_PAR) : display_color_img_wc_daytime(sp, x, y, h  CLIP_NUM_PAR);
			}
		}

	} // number ok
}



// Blends two colors
PIXVAL display_blend_colors(PIXVAL background, PIXVAL foreground, int percent_blend)
{
	const PIXVAL alpha = (percent_blend*64)/100;

	switch( alpha ) {
		case 0: // nothing to do ...
			return background;
#ifdef RGB555
		case 16:
		{
			const PIXVAL two = TWO_OUT_15;
			return (3 * (((background) >> 2) & two)) + (((foreground) >> 2) & two);
		}
		case 32:
		{
			const PIXVAL one = ONE_OUT_15;
			return ((((background) >> 1) & one)) + (((foreground) >> 1) & one);
		}
		case 48:
		{
			const PIXVAL two = TWO_OUT_15;
			return ((((background) >> 2) & two)) + (3 * ((foreground) >> 2) & two);
		}

		case 64:
			return foreground;

		default:
			// any percentage blending: SLOW!
			// 555 BITMAPS
			const PIXVAL r_src = (background >> 10) & 0x1F;
			const PIXVAL g_src = (background >> 5) & 0x1F;
			const PIXVAL b_src = background & 0x1F;
			const PIXVAL r_dest = (foreground >> 10) & 0x1F;
			const PIXVAL g_dest = (foreground >> 5) & 0x1F;
			const PIXVAL b_dest = (foreground & 0x1F);

			const PIXVAL r = (r_dest * alpha + r_src * (64 - alpha) + 32) >> 6;
			const PIXVAL g = (g_dest * alpha + g_src * (64 - alpha) + 32) >> 6;
			const PIXVAL b = (b_dest * alpha + b_src * (64 - alpha) + 32) >> 6;
			return (r << 10) | (g << 5) | b;
#else
		case 16:
		{
			const PIXVAL two = TWO_OUT_16;
			return (3 * (((background) >> 2) & two)) + (((foreground) >> 2) & two);
		}
		case 32:
		{
			const PIXVAL one = ONE_OUT_16;
			return ((((background) >> 1) & one)) + (((foreground) >> 1) & one);
		}
		case 48:
		{
			const PIXVAL two = TWO_OUT_16;
			return ((((background) >> 2) & two)) + (3 * ((foreground) >> 2) & two);
		}

		case 64:
			return foreground;

		default:
			// any percentage blending: SLOW!
			// 565 colors
			const PIXVAL r_src = (background >> 11);
			const PIXVAL g_src = (background >> 5) & 0x3F;
			const PIXVAL b_src = background & 0x1F;
			const PIXVAL r_dest = (foreground >> 11);
			const PIXVAL g_dest = (foreground >> 5) & 0x3F;
			const PIXVAL b_dest = (foreground & 0x1F);
			const PIXVAL r = (r_dest * alpha + r_src * (64 - alpha) + 32) >> 6;
			const PIXVAL g = (g_dest * alpha + g_src * (64 - alpha) + 32) >> 6;
			const PIXVAL b = (b_dest * alpha + b_src * (64 - alpha) + 32) >> 6;
			return (r << 11) | (g << 5) | b;
#endif
	}
}


/* from here code for transparent images */
typedef void (*blend_proc)(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len);

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


// the following 6 functions are for display_base_img_blend()
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
 * Blends a rectangular region with a color
 */
void display_blend_wh_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL colval, int percent_blend )
{
	if(  clip_lr( &xp, &w, CR0.clip_rect.x, CR0.clip_rect.xx )  &&  clip_lr( &yp, &h, CR0.clip_rect.y, CR0.clip_rect.yy )  ) {
		const PIXVAL alpha = (percent_blend*64)/100;

		switch( alpha ) {
			case 0: // nothing to do ...
				break;

			case 16:
			case 32:
			case 48:
			{
				// fast blending with 1/4 | 1/2 | 3/4 percentage
				blend_proc blend = outline[ (alpha>>4) - 1 ];

				for(  scr_coord_val y=0;  y<h;  y++  ) {
					blend( textur + xp + (yp+y) * disp_width, NULL, colval, w );
				}
			}
			break;

			case 64:
				// opaque ...
				display_fillbox_wh_rgb( xp, yp, w, h, colval, false );
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


static void display_img_blend_wc(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp, int colour, blend_proc p  CLIP_NUM_DEF )
{
	if(  h > 0  ) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // line decoder
			int xpos = xp;

			// display image
			uint16 runlen = *sp++;

			do {
				// we start with a clear run
				xpos += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = (*sp++) & (~TRANSPARENT_RUN);

				// something to display?
				if(  xpos + runlen > CR.clip_rect.x  &&  xpos < CR.clip_rect.xx  ) {
					const int left = (xpos >= CR.clip_rect.x ? 0 : CR.clip_rect.x - xpos);
					const int len  = (CR.clip_rect.xx - xpos >= runlen ? runlen : CR.clip_rect.xx - xpos);
					p(tp + xpos + left, sp + left, colour, len - left);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/* from here code for transparent images */


typedef void (*alpha_proc)(PIXVAL *dest, const PIXVAL *src, const PIXVAL *alphamap, const unsigned alpha_flags, const PIXVAL colour, const PIXVAL len);
static alpha_proc alpha;
static alpha_proc alpha_recode;


static void pix_alpha_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL *alphamap, const unsigned alpha_flags, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;

	const uint16 rmask = alpha_flags & ALPHA_RED ? 0x7c00 : 0;
	const uint16 gmask = alpha_flags & ALPHA_GREEN ? 0x03e0 : 0;
	const uint16 bmask = alpha_flags & ALPHA_BLUE ? 0x001f : 0;

	while(  dest < end  ) {
		// read mask components - always 15bpp
		uint16 alpha_value = ((*alphamap) & bmask) + (((*alphamap) & gmask) >> 5) + (((*alphamap) & rmask) >> 10);

		if(  alpha_value > 30  ) {
			// opaque, just copy source
			*dest = *src;
		}
		else if(  alpha_value > 0  ) {
			alpha_value = alpha_value > 15 ? alpha_value + 1 : alpha_value;

			//read screen components - 15bpp
			const uint16 rbs = (*dest) & 0x7c1f;
			const uint16 gs =  (*dest) & 0x03e0;

			// read image components - 15bpp
			const uint16 rbi = (*src) & 0x7c1f;
			const uint16 gi =  (*src) & 0x03e0;

			// calculate and write destination components - 16bpp
			const uint16 rbd = ((rbi * alpha_value) + (rbs * (32 - alpha_value))) >> 5;
			const uint16 gd  = ((gi  * alpha_value) + (gs  * (32 - alpha_value))) >> 5;
			*dest = (rbd & 0x7c1f) | (gd & 0x03e0);
		}

		dest++;
		src++;
		alphamap++;
	}
}


static void pix_alpha_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL *alphamap, const unsigned alpha_flags, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;

	const uint16 rmask = alpha_flags & ALPHA_RED ? 0x7c00 : 0;
	const uint16 gmask = alpha_flags & ALPHA_GREEN ? 0x03e0 : 0;
	const uint16 bmask = alpha_flags & ALPHA_BLUE ? 0x001f : 0;

	while(  dest < end  ) {
		// read mask components - always 15bpp
		uint16 alpha_value = ((*alphamap) & bmask) + (((*alphamap) & gmask) >> 5) + (((*alphamap) & rmask) >> 10);

		if(  alpha_value > 30  ) {
			// opaque, just copy source
			*dest = *src;
		}
		else if(  alpha_value > 0  ) {
			alpha_value = alpha_value > 15 ? alpha_value + 1 : alpha_value;

			//read screen components - 16bpp
			const uint16 rbs = (*dest) & 0xf81f;
			const uint16 gs =  (*dest) & 0x07e0;

			// read image components 16bpp
			const uint16 rbi = (*src) & 0xf81f;
			const uint16 gi =  (*src) & 0x07e0;

			// calculate and write destination components - 16bpp
			const uint16 rbd = ((rbi * alpha_value) + (rbs * (32 - alpha_value))) >> 5;
			const uint16 gd  = ((gi  * alpha_value) + (gs  * (32 - alpha_value))) >> 5;
			*dest = (rbd & 0xf81f) | (gd & 0x07e0);
		}

		dest++;
		src++;
		alphamap++;
	}
}


static void pix_alpha_recode_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL *alphamap, const unsigned alpha_flags, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;

	const uint16 rmask = alpha_flags & ALPHA_RED ? 0x7c00 : 0;
	const uint16 gmask = alpha_flags & ALPHA_GREEN ? 0x03e0 : 0;
	const uint16 bmask = alpha_flags & ALPHA_BLUE ? 0x001f : 0;

	while(  dest < end  ) {
		// read mask components - always 15bpp
		uint16 alpha_value = ((*alphamap) & bmask) + (((*alphamap) & gmask) >> 5) + (((*alphamap) & rmask) >> 10);

		if(  alpha_value > 30  ) {
			// opaque, just copy source
			*dest = rgbmap_current[*src];
		}
		else if(  alpha_value > 0  ) {
			alpha_value = alpha_value > 15 ? alpha_value + 1 : alpha_value;

			//read screen components - 15bpp
			const uint16 rbs = (*dest) & 0x7c1f;
			const uint16 gs =  (*dest) & 0x03e0;

			// read image components - 15bpp
			const uint16 rbi = (rgbmap_current[*src]) & 0x7c1f;
			const uint16 gi =  (rgbmap_current[*src]) & 0x03e0;

			// calculate and write destination components - 16bpp
			const uint16 rbd = ((rbi * alpha_value) + (rbs * (32 - alpha_value))) >> 5;
			const uint16 gd  = ((gi  * alpha_value) + (gs  * (32 - alpha_value))) >> 5;
			*dest = (rbd & 0x7c1f) | (gd & 0x03e0);
		}

		dest++;
		src++;
		alphamap++;
	}
}


static void pix_alpha_recode_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL *alphamap, const unsigned alpha_flags, const PIXVAL , const PIXVAL len)
{
	const PIXVAL *const end = dest + len;

	const uint16 rmask = alpha_flags & ALPHA_RED ? 0x7c00 : 0;
	const uint16 gmask = alpha_flags & ALPHA_GREEN ? 0x03e0 : 0;
	const uint16 bmask = alpha_flags & ALPHA_BLUE ? 0x001f : 0;

	while(  dest < end  ) {
		// read mask components - always 15bpp
		uint16 alpha_value = ((*alphamap) & bmask) + (((*alphamap) & gmask) >> 5) + (((*alphamap) & rmask) >> 10);

		if(  alpha_value > 30  ) {
			// opaque, just copy source
			*dest = rgbmap_current[*src];
		}
		else if(  alpha_value > 0  ) {
			alpha_value = alpha_value> 15 ? alpha_value + 1 : alpha_value;

			//read screen components - 16bpp
			const uint16 rbs = (*dest) & 0xf81f;
			const uint16 gs =  (*dest) & 0x07e0;

			// read image components 16bpp
			const uint16 rbi = (rgbmap_current[*src]) & 0xf81f;
			const uint16 gi =  (rgbmap_current[*src]) & 0x07e0;

			// calculate and write destination components - 16bpp
			const uint16 rbd = ((rbi * alpha_value) + (rbs * (32 - alpha_value))) >> 5;
			const uint16 gd  = ((gi  * alpha_value) + (gs  * (32 - alpha_value))) >> 5;
			*dest = (rbd & 0xf81f) | (gd & 0x07e0);
		}

		dest++;
		src++;
		alphamap++;
	}
}


static void display_img_alpha_wc(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp, const PIXVAL *alphamap, const uint8 alpha_flags, int colour, alpha_proc p  CLIP_NUM_DEF )
{
	if(  h > 0  ) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // line decoder
			int xpos = xp;

			// display image
			uint16 runlen = *sp++;
			alphamap++;

			do {
				// we start with a clear run
				xpos += runlen;

				// now get colored pixels
				runlen = ((*sp++) & ~TRANSPARENT_RUN);
				alphamap++;

				// something to display?
				if(  xpos + runlen > CR.clip_rect.x  &&  xpos < CR.clip_rect.xx  ) {
					const int left = (xpos >= CR.clip_rect.x ? 0 : CR.clip_rect.x - xpos);
					const int len  = (CR.clip_rect.xx - xpos >= runlen ? runlen : CR.clip_rect.xx - xpos);
					p( tp + xpos + left, sp + left, alphamap + left, alpha_flags, colour, len - left );
				}

				sp += runlen;
				alphamap += runlen;
				xpos += runlen;
				alphamap++;
			} while(  (runlen = *sp++)  );

			tp += disp_width;
		} while(  --h  );
	}
}


/**
 * draws the transparent outline of an image
 */
void display_rezoomed_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char /*player_nr*/, const FLAGGED_PIXVAL color_index, const bool /*daynight*/, const bool dirty  CLIP_NUM_DEF)
{
	if(  n < anz_images  ) {
		// need to go to nightmode and or rezoomed?
		if(  (images[n].recode_flags & FLAG_REZOOM)  ) {
			rezoom_img( n );
			recode_img( n, 0 );
		}
		else if(  (images[n].player_flags & 1)  ) {
			recode_img( n, 0 );
		}
		PIXVAL *sp = images[n].data[0];

		// now, since zooming may have change this image
		xp += images[n].x;
		yp += images[n].y;
		scr_coord_val h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		scr_coord_val reduce_h = yp + h - CR.clip_rect.yy;
		if(  reduce_h > 0  ) {
			h -= reduce_h;
		}
		// still something to draw
		if(  h <= 0  ) return;

		// vertically lines to skip (only bottom is visible)
		scr_coord_val skip_lines = CR.clip_rect.y - (int)yp;
		if(  skip_lines > 0  ) {
			if(  skip_lines >= h  ) {
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
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp++;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const scr_coord_val w = images[n].w;
			// get the real color
			const PIXVAL color = color_index & 0xFFFF;
			// we use function pointer for the blend runs for the moment ...
			blend_proc pix_blend = (color_index&OUTLINE_FLAG) ? outline[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ] : blend[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ];

			// use horizontal clipping or skip it?
			if(  xp >= CR.clip_rect.x  &&  xp + w  <= CR.clip_rect.xx  ) {
				// marking change?
				if(  dirty  ) {
					mark_rect_dirty_nc( xp, yp, xp + w - 1, yp + h - 1 );
				}
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend  CLIP_NUM_PAR );
			}
			else if(  xp < CR.clip_rect.xx  &&  xp + w > CR.clip_rect.x  ) {
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend  CLIP_NUM_PAR );
				// since height may be reduced, start marking here
				if(  dirty  ) {
					mark_rect_dirty_wc( xp, yp, xp + w - 1, yp + h - 1 );
				}
			}
		}
	}
}


void display_rezoomed_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8 /*player_nr*/, const FLAGGED_PIXVAL color_index, const bool /*daynight*/, const bool dirty  CLIP_NUM_DEF)
{
	if(  n < anz_images  &&  alpha_n < anz_images  ) {
		// need to go to nightmode and or rezoomed?
		if(  (images[n].recode_flags & FLAG_REZOOM)  ) {
			rezoom_img( n );
			recode_img( n, 0 );
		}
		else if(  (images[n].player_flags & 1)  ) {
			recode_img( n, 0 );
		}
		if(  (images[alpha_n].recode_flags & FLAG_REZOOM)  ) {
			rezoom_img( alpha_n );
		}
		PIXVAL *sp = images[n].data[0];
		// alphamap image uses base data as we don't want to recode
		PIXVAL *alphamap = images[alpha_n].zoom_data != NULL ? images[alpha_n].zoom_data : images[alpha_n].base_data;
		// now, since zooming may have change this image
		xp += images[n].x;
		yp += images[n].y;
		scr_coord_val h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		scr_coord_val reduce_h = yp + h - CR.clip_rect.yy;
		if(  reduce_h > 0  ) {
			h -= reduce_h;
		}
		// still something to draw
		if(  h <= 0  ) {
			return;
		}

		// vertically lines to skip (only bottom is visible
		scr_coord_val skip_lines = CR.clip_rect.y - (int)yp;
		if(  skip_lines > 0  ) {
			if(  skip_lines >= h  ) {
				// not visible at all
				return;
			}
			h -= skip_lines;
			yp += skip_lines;
			// now skip them
			while(  skip_lines--  ) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp++;
					alphamap++;
					alphamap += (*alphamap) & (~TRANSPARENT_RUN);
					alphamap++;
				} while(  *sp  );
				sp++;
				alphamap++;
			}
			// now sp is the new start of an image with height h (same for alphamap)
		}

		// new block for new variables
		{
			// needed now ...
			const scr_coord_val w = images[n].w;
			// get the real color
			const PIXVAL color = color_index & 0xFFFF;

			// use horizontal clipping or skip it?
			if(  xp >= CR.clip_rect.x  &&  xp + w  <= CR.clip_rect.xx  ) {
				// marking change?
				if(  dirty  ) {
					mark_rect_dirty_nc( xp, yp, xp + w - 1, yp + h - 1 );
				}
				display_img_alpha_wc( h, xp, yp, sp, alphamap, alpha_flags, color, alpha  CLIP_NUM_PAR );
			}
			else if(  xp < CR.clip_rect.xx  &&  xp + w > CR.clip_rect.x  ) {
				display_img_alpha_wc( h, xp, yp, sp, alphamap, alpha_flags, color, alpha  CLIP_NUM_PAR );
				// since height may be reduced, start marking here
				if(  dirty  ) {
					mark_rect_dirty_wc( xp, yp, xp + w - 1, yp + h - 1 );
				}
			}
		}
	}
}


// For blending or outlining unzoomed image. Adapted from display_base_img() and display_unzoomed_img_blend()
void display_base_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  base_tile_raster_width == tile_raster_width  ) {
		// same size => use standard routine
		display_rezoomed_img_blend( n, xp, yp, player_nr, color_index, daynight, dirty  CLIP_NUM_PAR );
	}
	else if(  n < anz_images  ) {
		// now test if visible and clipping needed
		scr_coord_val x = images[n].base_x + xp;
		scr_coord_val y = images[n].base_y + yp;
		scr_coord_val w = images[n].base_w;
		scr_coord_val h = images[n].base_h;
		if(  h == 0  ||  x >= CR.clip_rect.xx  ||  y >= CR.clip_rect.yy  ||  x + w <= CR.clip_rect.x  ||  y + h <= CR.clip_rect.y  ) {
			// not visible => we are done
			// happens quite often ...
			return;
		}

		PIXVAL *sp = images[n].base_data;

		// must the height be reduced?
		scr_coord_val reduce_h = y + h - CR.clip_rect.yy;
		if(  reduce_h > 0  ) {
			h -= reduce_h;
		}

		// vertical lines to skip (only bottom is visible)
		scr_coord_val skip_lines = CR.clip_rect.y - (int)y;
		if(  skip_lines > 0  ) {
			h -= skip_lines;
			y += skip_lines;
			// now skip them
			while (skip_lines--) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp++;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			const PIXVAL color = color_index & 0xFFFF;
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
			if(  x >= CR.clip_rect.x  &&  x + w <= CR.clip_rect.xx  ) {
				if(  dirty  ) {
					mark_rect_dirty_nc( x, y, x + w - 1, y + h - 1 );
				}
				display_img_blend_wc( h, x, y, sp, color, pix_blend  CLIP_NUM_PAR );
			}
			else {
				if(  dirty  ) {
					mark_rect_dirty_wc( x, y, x + w - 1, y + h - 1 );
				}
				display_img_blend_wc( h, x, y, sp, color, pix_blend  CLIP_NUM_PAR );
			}
		}
	} // number ok
}


void display_base_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  base_tile_raster_width == tile_raster_width  ) {
		// same size => use standard routine
		display_rezoomed_img_alpha( n, alpha_n, alpha_flags, xp, yp, player_nr, color_index, daynight, dirty  CLIP_NUM_PAR );
	}
	else if(  n < anz_images  ) {
		// now test if visible and clipping needed
		scr_coord_val x = images[n].base_x + xp;
		scr_coord_val y = images[n].base_y + yp;
		scr_coord_val w = images[n].base_w;
		scr_coord_val h = images[n].base_h;
		if(  h == 0  ||  x >= CR.clip_rect.xx  ||  y >= CR.clip_rect.yy  ||  x + w <= CR.clip_rect.x  ||  y + h <= CR.clip_rect.y  ) {
			// not visible => we are done
			// happens quite often ...
			return;
		}

		PIXVAL *sp = images[n].base_data;
		PIXVAL *alphamap = images[alpha_n].base_data;

		// must the height be reduced?
		scr_coord_val reduce_h = y + h - CR.clip_rect.yy;
		if(  reduce_h > 0  ) {
			h -= reduce_h;
		}

		// vertical lines to skip (only bottom is visible)
		scr_coord_val skip_lines = CR.clip_rect.y - (int)y;
		if(  skip_lines > 0  ) {
			h -= skip_lines;
			y += skip_lines;
			// now skip them
			while(  skip_lines--  ) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += (*sp) & (~TRANSPARENT_RUN);
					sp++;
				} while(  *sp  );
				do {
					// clear run + colored run + next clear run
					alphamap++;
					alphamap += (*alphamap) & (~TRANSPARENT_RUN);
					alphamap++;
				} while(  *alphamap  );
				sp++;
				alphamap++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			const PIXVAL color = color_index & 0xFFFF;

			// recode is needed only for blending
			if(  !(color_index & OUTLINE_FLAG)  ) {
				// colors for 2nd company color
				if(  player_nr >= 0  ) {
					activate_player_color( player_nr, daynight );
				}
				else {
					// no player
					activate_player_color( 0, daynight );
				}
			}

			// use horizontal clipping or skip it?
			if(  x >= CR.clip_rect.x  &&  x + w <= CR.clip_rect.xx  ) {
				if(  dirty  ) {
					mark_rect_dirty_nc( x, y, x + w - 1, y + h - 1 );
				}
				display_img_alpha_wc( h, x, y, sp, alphamap, alpha_flags, color, alpha_recode  CLIP_NUM_PAR );
			}
			else {
				if(  dirty  ) {
					mark_rect_dirty_wc( x, y, x + w - 1, y + h - 1 );
				}
				display_img_alpha_wc( h, x, y, sp, alphamap, alpha_flags, color, alpha_recode  CLIP_NUM_PAR );
			}
		}
	} // number ok
}


// ----------------- basic painting procedures ----------------


// scrolls horizontally, will ignore clipping etc.
void display_scroll_band(scr_coord_val start_y, scr_coord_val x_offset, scr_coord_val h)
{
	start_y  = max(start_y,  0);
	x_offset = min(x_offset, disp_width);
	h        = min(h,        disp_height);

	const PIXVAL *const src = textur + start_y * disp_width + x_offset;
	PIXVAL *const dst = textur + start_y * disp_width;
	const size_t amount = sizeof(PIXVAL) * (h * disp_width - x_offset);

	memmove(dst, src, amount);
}


/**
 * Draw one Pixel
 */
#ifdef DEBUG_FLUSH_BUFFER
static void display_pixel(scr_coord_val x, scr_coord_val y, PIXVAL color, bool mark_dirty=true)
#else
static void display_pixel(scr_coord_val x, scr_coord_val y, PIXVAL color)
#endif
{
	if(  x >= CR0.clip_rect.x  &&  x < CR0.clip_rect.xx  &&  y >= CR0.clip_rect.y  &&  y < CR0.clip_rect.yy  ) {
		PIXVAL* const p = textur + x + y * disp_width;

		*p = color;
#ifdef DEBUG_FLUSH_BUFFER
		if(  mark_dirty  ) {
#endif
			mark_tile_dirty(x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT);
#ifdef DEBUG_FLUSH_BUFFER
		}
#endif
	}
}


/**
 * Draw filled rectangle
 */
static void display_fb_internal(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL colval, bool dirty, scr_coord_val cL, scr_coord_val cR, scr_coord_val cT, scr_coord_val cB)
{
	if (clip_lr(&xp, &w, cL, cR) && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const int dx = disp_width - w;

		if (dirty) {
			mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
		}
#if defined USE_ASSEMBLER && defined __GNUC__ && defined __i686__
		// GCC might not use "rep stos" so force its use
		const uint32 longcolval = (colval << 16) | colval;
		do {
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
			p += dx;
		} while (--h);
#elif defined LOW_LEVEL
		// low level c++
		const uint32 colvald = (colval << 16) | colval;
		do {
			scr_coord_val count = w;

			// align to 4 bytes, should use uintptr_t but not available
			if(  reinterpret_cast<size_t>(p) & 0x2  ) {
				*p++ = (PIXVAL)colvald;
				count--;
			}
			// aligned fast fill loop
			bool const postalign = count & 1;
			count >>= 1;
			uint32 *lp = (uint32 *)p;
			while(count--) {
				*lp++ = colvald;
			}
			p = (PIXVAL *)lp;
			// finish unaligned remainder
			if(  postalign  ) {
				*p++ = (PIXVAL)colvald;
			}
			p += dx;
		} while (--h);
#else
		// high level c++
		do {
			PIXVAL *const fillend = p + w;
			std::fill(p, fillend, colval);
			p = fillend + dx;
		} while (--h);
#endif
	}
}


void display_fillbox_wh_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_fillbox_wh_clip_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty  CLIP_NUM_DEF)
{
	display_fb_internal( xp, yp, w, h, color, dirty, CR.clip_rect.x, CR.clip_rect.xx, CR.clip_rect.y, CR.clip_rect.yy );
}


/**
 * Draw vertical line
 */
static void display_vl_internal(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL colval, int dirty, scr_coord_val cL, scr_coord_val cR, scr_coord_val cT, scr_coord_val cB)
{
	if (xp >= cL && xp < cR && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp, yp + h - 1);

		do {
			*p = colval;
			p += disp_width;
		} while (--h != 0);
	}
}


void display_vline_wh_rgb(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL color, bool dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_vline_wh_clip_rgb(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL color, bool dirty  CLIP_NUM_DEF)
{
	display_vl_internal( xp, yp, h, color, dirty, CR.clip_rect.x, CR.clip_rect.xx, CR.clip_rect.y, CR.clip_rect.yy );
}


/**
 * Draw raw Pixel data
 */
void display_array_wh(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, const PIXVAL *arr)
{
	const int arr_w = w;
	const scr_coord_val xoff = clip_wh( &xp, &w, CR0.clip_rect.x, CR0.clip_rect.xx );
	const scr_coord_val yoff = clip_wh( &yp, &h, CR0.clip_rect.y, CR0.clip_rect.yy );
	if(  w > 0  &&  h > 0  ) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const PIXVAL *arr_src = arr;

		mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);
		if(  xp == CR0.clip_rect.x  ) arr_src += xoff;
		if(  yp == CR0.clip_rect.y  ) arr_src += yoff * arr_w;
		do {
			unsigned int ww = w;

			do {
				*p++ = *arr_src++;
			} while (--ww > 0);
			arr_src += arr_w - w;
			p += disp_width - w;
		} while (--h != 0);
	}
}


// --------------------------------- text rendering stuff ------------------------------


bool display_load_font(const char *fname, bool reload)
{
	font_t loaded_fnt;

	if(  fname == NULL  ) {
		dbg->error( "display_load_font", "NULL filename" );
		return false;
	}

	// skip reloading if already in memory, if bdf font
	if(  !reload  &&  default_font.is_loaded()  &&  strcmp( default_font.get_fname(), fname ) == 0  ) {
		return true;
	}

	if(  loaded_fnt.load_from_file(fname)  ) {
		default_font = loaded_fnt;
		default_font_ascent    = default_font.get_ascent();
		default_font_linespace = default_font.get_linespace();

		env_t::fontname = fname;

		return default_font.is_loaded();
	}

	return false;
}


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos)
{
	return utf8_get_next_char((const utf8*)text, pos);
}


sint32 get_prev_char(const char* text, sint32 pos)
{
	if(  pos <= 0  ) {
		return 0;
	}
	return utf8_get_prev_char((const utf8*)text, pos);
}


scr_coord_val display_get_char_width(utf32 c)
{
	return default_font.get_glyph_advance(c);
}


/* returns the width of this character or the default (Nr 0) character size */
scr_coord_val display_get_char_max_width(const char* text, size_t len) {

	scr_coord_val max_len=0;

	for(unsigned n=0; (len && n<len) || (len==0 && *text != '\0'); n++) {
		max_len = max(max_len,display_get_char_width(*text++));
	}

	return max_len;
}


/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 */
utf32 get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width)
{
	size_t len = 0;
	utf32 const char_code = utf8_decoder_t::decode((utf8 const *)text, len);

	if(  char_code==0  ||  char_code == '\n') {
		// case : end of text reached -> do not advance text pointer
		// also stop at linebreaks
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}
	else {
		text += len;
		byte_length = (uint8)len;
		pixel_width = default_font.get_glyph_advance(char_code);
	}
	return char_code;
}


/* returns true, if this is a valid character */
bool has_character(utf16 char_code)
{
	if(  char_code >= default_font.glyphs.size()  ) {
		// or we crash when accessing the non-existing char ...
		return false;
	}
	bool b1 = default_font.is_loaded();
	font_t::glyph_t& gl = default_font.glyphs[char_code];
	uint8  ad = gl.advance;
	return b1 && ad != 0xFF;

	// this return false for some reason on CJK for valid characters ?!?
	// return default_font.is_valid_glyph(char_code);
}



/*
 * returns the index of the last character that would fit within the width
 * If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val ellipsis_width )
{
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	const char *tmp_text = text;
	while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_width > (current_offset+ellipsis_width+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	size_t ellipsis_idx = max_idx;

	// now check if the text would fit completely
	if(  ellipsis_width  &&  pixel_width > 0  ) {
		// only when while above failed because of exceeding length
		current_offset += pixel_width;
		max_idx += byte_length;
		// check the rest ...
		while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_width > (current_offset+pixel_width)  ) {
			current_offset += pixel_width;
			max_idx += byte_length;
		}
		// if this fits, return end of string
		if(  max_width > (current_offset+pixel_width)  ) {
			return max_idx+byte_length;
		}
	}
	return ellipsis_idx;
}


/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 */
utf32 get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width)
{
	if(  text<=text_start  ) {
		// case : start of text reached or passed -> do not move the pointer backwards
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}

	utf32 char_code;
	// determine the start of the previous logical character
	do {
		--text;
	} while (  text>text_start  &&  (*text & 0xC0)==0x80  );

	size_t len = 0;
	char_code = utf8_decoder_t::decode((utf8 const *)text, len);
	byte_length = (uint8)len;
	pixel_width = default_font.get_glyph_advance(char_code);

	return char_code;
}


/* proportional_string_width with a text of a given length
* extended for universal font routines with unicode support
*/
int display_calc_proportional_string_len_width(const char *text, size_t len)
{
	const font_t* const fnt = &default_font;
	unsigned int width = 0;

	// decode char
	const char *const end = text + len;
	while(  text < end  ) {
		const utf32 iUnicode = utf8_decoder_t::decode((utf8 const *&)text);
		if(  iUnicode == UNICODE_NUL ||  iUnicode == '\n') {
			return width;
		}
		width += fnt->get_glyph_advance(iUnicode);
	}

	return width;
}



/* display_calc_proportional_multiline_string_len_width
* calculates the width and hieght of a box containing the text inside
*/
void display_calc_proportional_multiline_string_len_width(int &xw, int &yh, const char *text, size_t len)
{
	const font_t* const fnt = &default_font;
	int width = 0;

	xw = yh = 0;

	// decode char
	const char *const end = text + len;
	while(  text < end  ) {
		utf32 iUnicode = utf8_decoder_t::decode((utf8 const *&)text);
		if(  iUnicode == '\n'  ) {
			// new line: record max width
			xw = max( xw, width );
			yh += LINESPACE;
			width = 0;
		}
		if(  iUnicode == UNICODE_NUL ) {
			return;
		}

		width += fnt->get_glyph_advance(iUnicode);
	}

	xw = max( xw, width );
	yh += LINESPACE;
}



/**
 * Helper: calculates 8bit mask for clipping.
 * Calculates mask to fit pixel interval [xRL,xR) to clipping interval [cL, cR).
 */
static unsigned char get_h_mask(const int xL, const int xR, const int cL, const int cR)
{
	// do not mask
	unsigned char mask = 0xff;

	// check, if there is something to display
	if (xR <= cL || xL >= cR) return 0;
	// 8bit masks only
	assert(xR - xL <= 8);

	// check for left border
	if (xL < cL) {
		// left border clipped
		mask = 0xff >> (cL - xL);
	}
	// check for right border
	if (xR > cR) {
		// right border clipped
		mask &= 0xff << (xR - cR);
	}
	return mask;
}


/**
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 */
int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, sint32 len  CLIP_NUM_DEF)
{
	scr_coord_val cL, cR, cT, cB;

	// TAKE CARE: Clipping area may be larger than actual screen size
	if(  (flags & DT_CLIP)  ) {
		cL = CR.clip_rect.x;
		cR = CR.clip_rect.xx;
		cT = CR.clip_rect.y;
		cB = CR.clip_rect.yy;
	}
	else {
		cL = 0;
		cR = disp_width;
		cT = 0;
		cB = disp_height;
	}

	if (len < 0) {
		// don't know len yet
		len = 0x7FFF;
	}

	// adapt x-coordinate for alignment
	switch (flags & ( ALIGN_LEFT | ALIGN_CENTER_H | ALIGN_RIGHT) ) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_CENTER_H:
			x -= display_calc_proportional_string_len_width(txt, len) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len);
			break;
	}

	// still something to display?
	const font_t *const fnt = &default_font;

	if (x >= cR || y >= cB || y + fnt->get_linespace() <= cT) {
		// nothing to display
		return 0;
	}

	// store the initial x (for dirty marking)
	const scr_coord_val x0 = x;

	scr_coord_val y_offset = 0; // real y for display with clipping
	scr_coord_val glyph_height = fnt->get_linespace();
	const scr_coord_val yy = y + fnt->get_linespace();

	// calculate vertical y clipping parameters
	if (y < cT) {
		y_offset = cT - y;
	}
	if (yy > cB) {
		glyph_height -= yy - cB;
	}

	// big loop, draw char by char
	utf8_decoder_t decoder((utf8 const*)txt);
	size_t iTextPos = 0; // pointer on text position

	while (iTextPos < (size_t)len  &&  decoder.has_next()) {
		// decode char
		utf32 c = decoder.next();
		iTextPos = decoder.get_position() - (utf8 const*)txt;

		if(  c == '\n'  ) {
			// stop at linebreak
			break;
		}
		// print unknown character?
		else if(  !fnt->is_valid_glyph(c)  ) {
			c = 0;
		}

		// get the data from the font
		int glyph_width = fnt->get_glyph_width(c);
		const uint8 glyph_yoffset = std::max(fnt->get_glyph_yoffset(c), (uint8)y_offset);

		// currently max character width 16 bit supported by font.h/font.cc
		for(  int i=0;  i<2;  i++  ) {
			const uint8 bits = std::min(8, glyph_width);
			uint8 mask = get_h_mask(x + i*8, x + i*8 + bits, cL, cR);
			glyph_width -= bits;

			const uint8 *p = fnt->get_glyph_bitmap(c) + glyph_yoffset + i*GLYPH_BITMAP_HEIGHT;
			if(  mask!=0  ) {
				int screen_pos = (y+glyph_yoffset) * disp_width + x + i*8;

				for (int h = glyph_yoffset; h < glyph_height; h++) {
					unsigned int dat = *p++ & mask;
					PIXVAL* dst = textur + screen_pos;

					if(  dat  !=  0  ) {
						for(  size_t dat_offset = 0 ; dat_offset < 8 ; dat_offset++  ) {
							if(  (dat & (0x80 >> dat_offset))  ) {
								dst[dat_offset] = color;
							}
						}
					}
					screen_pos += disp_width;
				}
			}
		}

		x += fnt->get_glyph_advance(c);
	}

	if(  dirty  ) {
		// here, because only now we know the length also for ALIGN_LEFT text
		mark_rect_dirty_clip( x0, y, x - 1, y + LINESPACE - 1  CLIP_NUM_PAR);
	}

	// warning: actual len might be longer, due to clipping!
	return x - x0;
}


/// Displays a string which is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given then it just displays the full string
void display_proportional_ellipsis_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed, PIXVAL shadow_color)
{
	const scr_coord_val ellipsis_width = translator::get_lang()->ellipsis_width;
	const scr_coord_val max_screen_width = r.w;
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	if(  align & ALIGN_CENTER_V  ) {
		r.y += (r.h - LINESPACE)/2;
		align &= ~ALIGN_CENTER_V;
	}

	const char *tmp_text = text;
	while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+ellipsis_width+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	size_t max_idx_before_ellipsis = max_idx;
	scr_coord_val max_offset_before_ellipsis = current_offset;

	// now check if the text would fit completely
	if(  ellipsis_width  &&  pixel_width > 0  ) {
		// only when while above failed because of exceeding length
		current_offset += pixel_width;
		max_idx += byte_length;
		// check the rest ...
		while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+pixel_width)  ) {
			current_offset += pixel_width;
			max_idx += byte_length;
		}
		// if it does not fit
		if(  max_screen_width < (current_offset+pixel_width)  ) {
			scr_coord_val w = 0;
			// since we know the length already, we try to center the text with the remaining pixels of the last character
			if(  align & ALIGN_CENTER_H  ) {
				w = (max_screen_width-max_offset_before_ellipsis-ellipsis_width)/2;
			}
			if (shadowed) {
				display_text_proportional_len_clip_rgb( r.x+w+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);
			}
			w += display_text_proportional_len_clip_rgb( r.x+w, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);

			if (shadowed) {
				display_text_proportional_len_clip_rgb( r.x+w+1, r.y+1, translator::translate("..."), ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
			}

			display_text_proportional_len_clip_rgb( r.x+w, r.y, translator::translate("..."), ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
			return;
		}
		else {
			// if this fits, end of string
			max_idx += byte_length;
			current_offset += pixel_width;
		}
	}
	switch (align & ALIGN_RIGHT) {
		case ALIGN_CENTER_H:
			r.x += (max_screen_width - current_offset)/2;
			break;
		case ALIGN_RIGHT:
			r.x += max_screen_width - current_offset;
		default: ;
	}
	if (shadowed) {
		display_text_proportional_len_clip_rgb( r.x+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	}
	display_text_proportional_len_clip_rgb( r.x, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
}


/**
 * Draw shaded rectangle using direct color values
 */
void display_ddd_box_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color, bool dirty)
{
	display_fillbox_wh_rgb(x1, y1,         w, 1, tl_color, dirty);
	display_fillbox_wh_rgb(x1, y1 + h - 1, w, 1, rd_color, dirty);

	h -= 2;

	display_vline_wh_rgb(x1,         y1 + 1, h, tl_color, dirty);
	display_vline_wh_rgb(x1 + w - 1, y1 + 1, h, rd_color, dirty);
}


void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos - 1, ypos    , text, flags, shadow_color, dirty, len  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 2, text, flags, shadow_color, dirty, len  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos, ypos + 1, text, flags, text_color, dirty, len  CLIP_NUM_DEFAULT);
}


void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 1 + (12 - LINESPACE) / 2, text, flags, shadow_color, dirty, len  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos, ypos + (12 - LINESPACE) / 2, text, flags, text_color, dirty, len  CLIP_NUM_DEFAULT);
}


/**
 * Draw shaded rectangle using direct color values
 */
void display_ddd_box_clip_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color)
{
	display_fillbox_wh_clip_rgb(x1, y1,         w, 1, tl_color, true);
	display_fillbox_wh_clip_rgb(x1, y1 + h - 1, w, 1, rd_color, true);

	h -= 2;

	display_vline_wh_clip_rgb(x1,         y1 + 1, h, tl_color, true);
	display_vline_wh_clip_rgb(x1 + w - 1, y1 + 1, h, rd_color, true);
}


/**
 * display text in 3d box with clipping
 */
void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_color, FLAGGED_PIXVAL text_color, const char *text, int dirty  CLIP_NUM_DEF)
{
	const int vpadding = LINESPACE / 7;
	const int hpadding = LINESPACE / 4;

	scr_coord_val width = proportional_string_width(text);

	PIXVAL lighter = display_blend_colors(ddd_color, color_idx_to_rgb(COL_WHITE), 25);
	PIXVAL darker  = display_blend_colors(ddd_color, color_idx_to_rgb(COL_BLACK), 25);

	display_fillbox_wh_clip_rgb( xpos+1, ypos - vpadding + 1, width+2*hpadding-2, LINESPACE+2*vpadding-2, ddd_color, dirty CLIP_NUM_PAR);

	display_fillbox_wh_clip_rgb( xpos, ypos - vpadding, width + 2*hpadding - 2, 1, lighter, dirty );
	display_fillbox_wh_clip_rgb( xpos, ypos + LINESPACE + vpadding - 1, width + 2*hpadding - 2, 1, darker,  dirty );

	display_vline_wh_clip_rgb( xpos, ypos - vpadding - 1, LINESPACE + vpadding * 2 - 1, lighter, dirty );
	display_vline_wh_clip_rgb( xpos + width + 2*hpadding, ypos - vpadding - 1, LINESPACE + vpadding * 2 - 1, darker,  dirty );

	display_text_proportional_len_clip_rgb( xpos+hpadding, ypos+1, text, ALIGN_LEFT | DT_CLIP, text_color, dirty, -1);
}


/**
 * Draw multiline text
 */
int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *buf, PIXVAL color)
{
	int max_px_len = 0;
	if (buf != NULL && *buf != '\0') {
		const char *next;

		do {
			next = strchr(buf, '\n');
			const int px_len = display_text_proportional_len_clip_rgb(
				x, y, buf,
				ALIGN_LEFT | DT_CLIP, color, true,
				next != NULL ? (int)(size_t)(next - buf) : -1
			);
			if(  px_len>max_px_len  ) {
				max_px_len = px_len;
			}
			y += LINESPACE;
		} while ((void)(buf = (next ? next+1 : NULL)), buf != NULL);
	}
	return max_px_len;
}


/**
 * draw line from x,y to xx,yy
 **/
void display_direct_line_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const PIXVAL colval)
{
	int i, steps;
	sint64 xp, yp;
	sint64 xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = ((sint64)dx << 16) / steps;
	ys = ((sint64)dy << 16) / steps;

	xp = (sint64)x << 16;
	yp = (sint64)y << 16;

	for (i = 0; i <= steps; i++) {
#ifdef DEBUG_FLUSH_BUFFER
		display_pixel(xp >> 16, yp >> 16, colval, false);
#else
		display_pixel(xp >> 16, yp >> 16, colval);
#endif
		xp += xs;
		yp += ys;
	}
}


//taken from function display_direct_line() above, to draw a dotted line: draw=pixels drawn, dontDraw=pixels skipped
void display_direct_line_dotted_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const scr_coord_val draw, const scr_coord_val dontDraw, const PIXVAL colval)
{
	int i, steps;
	sint64 xp, yp;
	sint64 xs, ys;
	int counter=0;
	bool mustDraw=true;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = ((sint64)dx << 16) / steps;
	ys = ((sint64)dy << 16) / steps;

	xp = (sint64)x << 16;
	yp = (sint64)y << 16;

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
void display_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, const PIXVAL colval )
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_pixel( x0, y0 + radius, colval );
	display_pixel( x0, y0 - radius, colval );
	display_pixel( x0 + radius, y0, colval );
	display_pixel( x0 - radius, y0, colval );

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
void display_filled_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, const PIXVAL colval )
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_fb_internal( x0-radius, y0, radius+radius+1, 1, colval, false, CR0.clip_rect.x, CR0.clip_rect.xx, CR0.clip_rect.y, CR0.clip_rect.yy );
	display_pixel( x0, y0 + radius, colval );
	display_pixel( x0, y0 - radius, colval );
	display_pixel( x0 + radius, y0, colval );
	display_pixel( x0 - radius, y0, colval );

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
		display_fb_internal( x0-x, y0+y, x+x, 1, colval, false, CR0.clip_rect.x, CR0.clip_rect.xx, CR0.clip_rect.y, CR0.clip_rect.yy );
		display_fb_internal( x0-x, y0-y, x+x, 1, colval, false, CR0.clip_rect.x, CR0.clip_rect.xx, CR0.clip_rect.y, CR0.clip_rect.yy );

		display_fb_internal( x0-y, y0+x, y+y, 1, colval, false, CR0.clip_rect.x, CR0.clip_rect.xx, CR0.clip_rect.y, CR0.clip_rect.yy );
		display_fb_internal( x0-y, y0-x, y+y, 1, colval, false, CR0.clip_rect.x, CR0.clip_rect.xx, CR0.clip_rect.y, CR0.clip_rect.yy );
	}
//	mark_rect_dirty_wc( x0-radius, y0-radius, x0+radius+1, y0+radius+1 );
}



void display_signal_direction_rgb(scr_coord_val x, scr_coord_val y, uint8 way_dir, uint8 sig_dir, PIXVAL col1, PIXVAL col1_dark, bool is_diagonal, uint8 slope )
{
	uint8 width  = is_diagonal ? current_tile_raster_width/6*0.353 :current_tile_raster_width/6;
	const uint8 height = is_diagonal ?current_tile_raster_width/6*0.353 :current_tile_raster_width/12;
	const uint8 thickness = max( current_tile_raster_width/36, 2);

	x += current_tile_raster_width/2;
	y += (current_tile_raster_width*9)/16;

	if (is_diagonal) {

		if (way_dir == ribi_t::northeast || way_dir == ribi_t::southwest) {
			// vertical
			x += (way_dir==ribi_t::northeast) ?current_tile_raster_width/4 : (-current_tile_raster_width/4);
			y += current_tile_raster_width/16;
			width = width<<2; // 4x

			// upper
			for (uint8 xoff = 0; xoff < width/2; xoff++) {
				const uint8 yoff = (uint8)((xoff+1)/2);
				// up
				if (sig_dir & ribi_t::east || sig_dir & ribi_t::south) {
					display_vline_wh_clip_rgb(x + xoff, y+yoff, width/4 - yoff, col1, true);
					display_vline_wh_clip_rgb(x-xoff-1, y+yoff, width/4 - yoff, col1, true);
				}
				// down
				if (sig_dir & ribi_t::west || sig_dir & ribi_t::north) {
					display_vline_wh_clip_rgb(x + xoff, y+current_tile_raster_width/6,              width/4-yoff, col1,      true);
					display_vline_wh_clip_rgb(x + xoff, y+current_tile_raster_width/6+width/4-yoff, thickness,    col1_dark, true);
					display_vline_wh_clip_rgb(x-xoff-1, y+current_tile_raster_width/6,              width/4-yoff, col1,      true);
					display_vline_wh_clip_rgb(x-xoff-1, y+current_tile_raster_width/6+width/4-yoff, thickness,    col1_dark, true);
				}
			}
			// up
			if (sig_dir & ribi_t::east || sig_dir & ribi_t::south) {
				display_fillbox_wh_clip_rgb(x - width/2, y + width/4, width, thickness, col1_dark, true);
			}
		}
		else {
			// horizontal
			y -= current_tile_raster_width/12;
			if (way_dir == ribi_t::southeast) {
				y += current_tile_raster_width/4;
			}

			for (uint8 xoff = 0; xoff < width*2; xoff++) {
				const uint8 h = width*2 - (scr_coord_val)(xoff + 1);
				// left
				if (sig_dir & ribi_t::north || sig_dir & ribi_t::east) {
					display_vline_wh_clip_rgb(x - xoff - width*2, y + (scr_coord_val)((xoff+1)/2),   h, col1, true);
					display_vline_wh_clip_rgb(x - xoff - width*2, y + (scr_coord_val)((xoff+1)/2)+h, thickness, col1_dark, true);
				}
				// right
				if (sig_dir & ribi_t::south || sig_dir & ribi_t::west) {
					display_vline_wh_clip_rgb(x + xoff + width*2, y + (scr_coord_val)((xoff+1)/2),   h, col1, true);
					display_vline_wh_clip_rgb(x + xoff + width*2, y + (scr_coord_val)((xoff+1)/2)+h, thickness, col1_dark, true);
				}
			}
		}
	}
	else {
		if (sig_dir & ribi_t::south) {
			// upper right
			scr_coord_val slope_offset_y = corner_se( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				display_vline_wh_clip_rgb( x + xoff, y - slope_offset_y, (scr_coord_val)(xoff/2) + 1, col1, true );
				display_vline_wh_clip_rgb( x + xoff, y - slope_offset_y + (scr_coord_val)(xoff/2) + 1, thickness, col1_dark, true );
			}
		}
		if (sig_dir & ribi_t::east) {
			scr_coord_val slope_offset_y = corner_se( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				display_vline_wh_clip_rgb(x - xoff - 1, y - slope_offset_y, (scr_coord_val)(xoff/2) + 1, col1, true);
				display_vline_wh_clip_rgb(x - xoff - 1, y - slope_offset_y + (scr_coord_val)(xoff/2) + 1, thickness, col1_dark, true);
			}
		}
		if (sig_dir & ribi_t::west) {
			scr_coord_val slope_offset_y = corner_nw( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				display_vline_wh_clip_rgb(x + xoff, y - slope_offset_y + height*2 - (scr_coord_val)(xoff/2) + 1, (scr_coord_val)(xoff/2) + 1, col1, true);
				display_vline_wh_clip_rgb(x + xoff, y - slope_offset_y + height*2 + 1, thickness, col1_dark, true);
			}
		}
		if (sig_dir & ribi_t::north) {
			scr_coord_val slope_offset_y = corner_nw( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				display_vline_wh_clip_rgb(x - xoff - 1, y - slope_offset_y + height*2 - (scr_coord_val)(xoff/2) + 1, (scr_coord_val)(xoff/2) + 1, col1, true);
				display_vline_wh_clip_rgb(x - xoff - 1, y - slope_offset_y + height*2 + 1, thickness, col1_dark, true);
			}
		}
	}
}


/**
 * Print a bezier curve between points A and B
 * @Ax,Ay=start coordinate of Bezier curve
 * @Bx,By=end coordinate of Bezier curve
 * @ADx,ADy=vector for start direction of curve
 * @BDx,BDy=vector for end direction of Bezier curve
 * @colore=color for curve to be drawn
 * @draw=for dotted lines, how many pixels to be drawn (leave 0 for solid line)
 * @dontDraw=for dotted lines, how many pixels to not be drawn (leave 0 for solid line)
 */
void draw_bezier_rgb(scr_coord_val Ax, scr_coord_val Ay, scr_coord_val Bx, scr_coord_val By, scr_coord_val ADx, scr_coord_val ADy, scr_coord_val BDx, scr_coord_val BDy, const PIXVAL colore, scr_coord_val draw, scr_coord_val dontDraw)
{
	scr_coord_val Cx,Cy,Dx,Dy;
	Cx = Ax + ADx;
	Cy = Ay + ADy;
	Dx = Bx + BDx;
	Dy = By + BDy;

	/* float a,b,rx,ry,oldx,oldy;
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
				display_direct_line_rgb(rx,ry,oldx,oldy,colore);
			else
				display_direct_line_dotted_rgb(rx,ry,oldx,oldy,draw,dontDraw,colore);
	  }
*/

	sint32 rx = Ax*32*32*32; // init with a=0, b=32
	sint32 ry = Ay*32*32*32; // init with a=0, b=32
	// fixed point: we cycle between 0 and 32, rather than 0 and 1
	for(  sint32 a=1;  a<=32;  a++  ) {
		const sint32 b = 32 - a;
		const sint32 oldx = rx;
		const sint32 oldy = ry;
		rx = Ax*b*b*b + 3*Cx*b*b*a + 3*Dx*b*a*a + Bx*a*a*a;
		ry = Ay*b*b*b + 3*Cy*b*b*a + 3*Dy*b*a*a + By*a*a*a;
		//fixed point: due to cycling between 0 and 32 (2<<5), we divide by 32^3=2>>15 because of cubic interpolation
		if(  !draw  &&  !dontDraw  ) {
			display_direct_line_rgb( rx>>15, ry>>15, oldx>>15, oldy>>15, colore );
		}
		else {
			display_direct_line_dotted_rgb( rx>>15, ry>>15, oldx>>15, oldy>>15, draw, dontDraw, colore );
		}
	}
}



// Only right facing at the moment
void display_right_triangle_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val height, const PIXVAL colval, const bool dirty)
{
	y += (height / 2);
	while(  height > 0  ) {
		display_vline_wh_rgb( x, y-(height/2), height, colval, dirty );
		x++;
		height -= 2;
	}
}



// ------------------- other support routines that actually interface with the OS -----------------


/**
 * copies only the changed areas to the screen using the "tile dirty buffer"
 * To get large changes, actually the current and the previous one is used.
 */
void display_flush_buffer()
{
	static const uint8 MultiplyDeBruijnBitPosition[32] =
	{
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

#ifdef USE_SOFTPOINTER
	ex_ord_update_mx_my();

	const scr_coord_val ticker_ypos_bottom = display_get_height() - win_get_statusbar_height() - (env_t::menupos == MENU_BOTTOM) * env_t::iconsize.h;
	const scr_coord_val ticker_ypos_top = ticker_ypos_bottom - TICKER_HEIGHT;

	// use mouse pointer image if available
	if (softpointer != -1 && standard_pointer >= 0) {
		display_color_img(standard_pointer, sys_event.mx, sys_event.my, 0, false, true  CLIP_NUM_DEFAULT);

		// if software emulated mouse pointer is over the ticker, redraw it totally at next occurs
		if (!ticker::empty() && sys_event.my+images[standard_pointer].h >= ticker_ypos_top &&
		   sys_event.my <= ticker_ypos_bottom) {
			ticker::set_redraw_all(true);
		}
	}
	// no pointer image available, draw a crosshair
	else {
		display_fb_internal(sys_event.mx - 1, sys_event.my - 3, 3, 7, color_idx_to_rgb(COL_WHITE), true, 0, disp_width, 0, disp_height);
		display_fb_internal(sys_event.mx - 3, sys_event.my - 1, 7, 3, color_idx_to_rgb(COL_WHITE), true, 0, disp_width, 0, disp_height);
		display_direct_line_rgb( sys_event.mx-2, sys_event.my, sys_event.mx+2, sys_event.my, color_idx_to_rgb(COL_BLACK) );
		display_direct_line_rgb( sys_event.mx, sys_event.my-2, sys_event.mx, sys_event.my+2, color_idx_to_rgb(COL_BLACK) );

		// if crosshair is over the ticker, redraw it totally at next occurs
		if(!ticker::empty() && sys_event.my+2 >= ticker_ypos_top && sys_event.my-2 <= ticker_ypos_bottom) {
			ticker::set_redraw_all(true);
		}
	}
	old_my = sys_event.my;
#endif

	// combine current with last dirty tiles
	for(  int i = 0;  i < tile_buffer_length;  i++  ) {
		tile_dirty_old[i] |= tile_dirty[i];
	}

	const int tile_words_per_line = tile_buffer_per_line >> 5;
	ALLOCA( uint32, masks, tile_words_per_line );

	for(  int x1 = 0;  x1 < tiles_per_line;  x1++  ) {
		const uint32 x_search_mask = 1 << (x1 & 31); // bit mask for finding bit x set
		int y1 = 0;
		do {
			const int word_max = (0 + (y1 + 1) * tile_buffer_per_line) >> 5; // first word on next line. limit search to < max
			const int word_x1 = (x1 + y1 * tile_buffer_per_line) >> 5;
			if(  (tile_dirty_old[word_x1] & x_search_mask) == x_search_mask  ) {
				// found dirty tile at x1, now find contiguous block of dirties - x2
				const uint32 testval = ~((~(0xFFFFFFFF << (x1 & 31))) | tile_dirty_old[word_x1]);
				int word_x2 = word_x1;
				int x2;
				if(  testval == 0  ) { // dirty block spans words
					masks[0] = tile_dirty_old[word_x1];
					word_x2++;
					while(  word_x2 < word_max  &&  tile_dirty_old[word_x2] == 0xFFFFFFFF  ) {
						masks[word_x2 - word_x1] = 0xFFFFFFFF; // dirty block spans this entire word
						word_x2++;
					}
					if(  word_x2 >= word_max                   // dirty tiles extend all the way to screen edge
						 ||  !(tile_dirty_old[word_x2] & 1)  ) { // dirty block actually ended on the word edge
						x2 = 32; // set to whole word
						word_x2--; // masks already set in while loop above
					}
					else { // dirty block ends in word_x2
						const uint32 tv = ~tile_dirty_old[word_x2];
						x2 = MultiplyDeBruijnBitPosition[(((tv & -tv) * 0x077CB531U)) >> 27];
						masks[word_x2-word_x1] = 0xFFFFFFFF >> (32 - x2);
					}
				}
				else { // dirty block is all within one word - word_x1
					x2 = MultiplyDeBruijnBitPosition[(((testval & -testval) * 0x077CB531U)) >> 27];
					masks[0] = (0xFFFFFFFF << (32 - x2 + (x1 & 31))) >> (32 - x2);
				}

				for(  int i = word_x1;  i <= word_x2;  i++  ) { // clear dirty
					tile_dirty_old[i] &= ~masks[i - word_x1];
				}

				// x2 from bit index to tile coords
				x2 += (x1 & ~31) + ((word_x2 - word_x1) << 5);

				// find how many rows can be combined into one rectangle
				int y2 = y1 + 1;
				bool xmatch = true;
				while(  y2 < tile_lines  &&  xmatch  ) {
					const int li = (x1 + y2 * tile_buffer_per_line) >> 5;
					const int ri = li + word_x2 - word_x1;
					for(  int i = li;  i <= ri;  i++ ) {
						if(  (tile_dirty_old[i] & masks[i - li])  !=  masks[i - li]  ) {
							xmatch = false;
							break;
						}
					}
					if(  xmatch  ) {
						for(  int i = li;  i <= ri;  i++  ) { // clear dirty
							tile_dirty_old[i] &= ~masks[i - li];
						}
						y2++;
					}
				}

#ifdef DEBUG_FLUSH_BUFFER
				display_vline_wh_rgb( (x1 << DIRTY_TILE_SHIFT) - 1, y1 << DIRTY_TILE_SHIFT, (y2 - y1) << DIRTY_TILE_SHIFT, color_idx_to_rgb(COL_YELLOW), false);
				display_vline_wh_rgb( x2 << DIRTY_TILE_SHIFT,  y1 << DIRTY_TILE_SHIFT, (y2 - y1) << DIRTY_TILE_SHIFT, color_idx_to_rgb(COL_YELLOW), false);
				display_fillbox_wh_rgb( x1 << DIRTY_TILE_SHIFT, y1 << DIRTY_TILE_SHIFT, (x2 - x1) << DIRTY_TILE_SHIFT, 1, color_idx_to_rgb(COL_YELLOW), false);
				display_fillbox_wh_rgb( x1 << DIRTY_TILE_SHIFT, (y2 << DIRTY_TILE_SHIFT) - 1, (x2 - x1) << DIRTY_TILE_SHIFT, 1, color_idx_to_rgb(COL_YELLOW), false);
				display_direct_line_rgb( x1 << DIRTY_TILE_SHIFT, y1 << DIRTY_TILE_SHIFT, x2 << DIRTY_TILE_SHIFT, (y2 << DIRTY_TILE_SHIFT) - 1, color_idx_to_rgb(COL_YELLOW) );
				display_direct_line_rgb( x1 << DIRTY_TILE_SHIFT, (y2 << DIRTY_TILE_SHIFT) - 1, x2 << DIRTY_TILE_SHIFT, y1 << DIRTY_TILE_SHIFT, color_idx_to_rgb(COL_YELLOW) );
#else
				dr_textur( x1 << DIRTY_TILE_SHIFT, y1 << DIRTY_TILE_SHIFT, (x2 - x1) << DIRTY_TILE_SHIFT, (y2 - y1) << DIRTY_TILE_SHIFT );
#endif
				y1 = y2; // continue search from bottom of found rectangle
			}
			else {
				y1++;
			}
		} while(  y1 < tile_lines  );
	}
#ifdef DEBUG_FLUSH_BUFFER
	dr_textur(0, 0, disp_actual_width, disp_height );
#endif

	// swap tile buffers
	uint32 *tmp = tile_dirty_old;
	tile_dirty_old = tile_dirty;
	tile_dirty = tmp; // _old was cleared to 0 in above loops
}


/**
 * Turn mouse pointer visible/invisible
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
 */
void display_set_pointer(int pointer)
{
	standard_pointer = pointer;
}


/**
 * mouse pointer image
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
 */
bool simgraph_init(scr_size window_size, sint16 full_screen)
{
	disp_actual_width = window_size.w;
	disp_height = window_size.h;

#ifdef MULTI_THREAD
	pthread_mutex_init( &recode_img_mutex, NULL );
#endif

	// init rezoom_img()
	for(  int i = 0;  i < MAX_THREADS;  i++  ) {
#ifdef MULTI_THREAD
		pthread_mutex_init( &rezoom_img_mutex[i], NULL );
#endif
		rezoom_baseimage[i] = NULL;
		rezoom_baseimage2[i] = NULL;
		rezoom_size[i] = 0;
	}

	// get real width from os-dependent routines
	disp_width = dr_os_open(window_size, full_screen);
	if(  disp_width<=0  ) {
		dr_fatal_notify( "Cannot open window!" );
		return false;
	}

	textur = dr_textur_init();

	// init, load, and check fonts
	if(  !display_load_font(env_t::fontname.c_str())  &&  !display_load_font(FONT_PATH_X "prop.fnt") ) {
		dr_fatal_notify("No fonts found!");
		return false;
	}

	// allocate dirty tile flags
	tiles_per_line = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_per_line = (tiles_per_line + 31) & ~31;
	tile_lines = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length = (tile_lines * tile_buffer_per_line / 32);

	tile_dirty = MALLOCN( uint32, tile_buffer_length );
	tile_dirty_old = MALLOCN( uint32, tile_buffer_length );

	mark_screen_dirty();
	MEMZERON( tile_dirty_old, tile_buffer_length );

	// init player colors
	for( int i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		player_offsets[i][0] = i*8;
		player_offsets[i][1] = i*8+24;
	}

	display_set_clip_wh(0, 0, disp_width, disp_height);

	// Calculate daylight rgbmap and save it for unshaded tile drawing
	player_day = 0;
	display_day_night_shift(0);
	memcpy(specialcolormap_all_day, specialcolormap_day_night, 256 * sizeof(PIXVAL));
	memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
	memcpy(transparent_map_all_day, transparent_map_day_night, lengthof(transparent_map_day_night) * sizeof(PIXVAL));
	memcpy(transparent_map_all_day_rgb, transparent_map_day_night_rgb, lengthof(transparent_map_day_night_rgb) * sizeof(uint8));

	// find out bit depth
	{
		uint32 c = get_system_color( 0, 255, 0 );
		while((c&1)==0) {
			c >>= 1;
		}
		if(c==31) {
			// 15 bit per pixel
			bitdepth = 15;
			blend[0] = pix_blend25_15;
			blend[1] = pix_blend50_15;
			blend[2] = pix_blend75_15;
			blend_recode[0] = pix_blend_recode25_15;
			blend_recode[1] = pix_blend_recode50_15;
			blend_recode[2] = pix_blend_recode75_15;
			outline[0] = pix_outline25_15;
			outline[1] = pix_outline50_15;
			outline[2] = pix_outline75_15;
			alpha = pix_alpha_15;
			alpha_recode = pix_alpha_recode_15;
			recode_img_src_target = recode_img_src_target_15;
#ifndef RGB555
			dr_fatal_notify( "Compiled for 16 bit color depth but using 15!" );
#endif
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
			alpha = pix_alpha_16;
			alpha_recode = pix_alpha_recode_16;
			recode_img_src_target = recode_img_src_target_16;
#ifdef RGB555
			dr_fatal_notify( "Compiled for 15 bit color depth but using 16!" );
#endif
		}
	}

	return true;
}


/**
 * Check if the graphic module already was initialized.
 */
bool is_display_init()
{
	return textur != NULL  &&  default_font.is_loaded()  &&  images!=NULL;
}


/**
 * Close the Graphic module
 */
void simgraph_exit()
{
	dr_os_close();

	free( tile_dirty_old );
	free( tile_dirty );
	display_free_all_images_above(0);
	free(images);

	tile_dirty = tile_dirty_old = NULL;
	images = NULL;
#ifdef MULTI_THREAD
	pthread_mutex_destroy( &recode_img_mutex );
	for(  int i = 0;  i < MAX_THREADS;  i++  ) {
		pthread_mutex_destroy( &rezoom_img_mutex[i] );
	}
#endif
}


/* changes display size
 */
void simgraph_resize(scr_size new_window_size)
{
	disp_actual_width = max( 16, new_window_size.w );
	if(  new_window_size.h<=0  ) {
		new_window_size.h = 64;
	}
	// only resize, if internal values are different
	if (disp_width != new_window_size.w || disp_height != new_window_size.h) {
		scr_coord_val new_pitch = dr_textur_resize(&textur, new_window_size.w, new_window_size.h);
		if(  new_pitch!=disp_width  ||  disp_height != new_window_size.h) {
			disp_width = new_pitch;
			disp_height = new_window_size.h;

			free( tile_dirty_old );
			free( tile_dirty);

			// allocate dirty tile flags
			tiles_per_line = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
			tile_buffer_per_line = (tiles_per_line + 31) & ~31;
			tile_lines = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
			tile_buffer_length = (tile_lines * tile_buffer_per_line / 32);

			tile_dirty = MALLOCN( uint32, tile_buffer_length );
			tile_dirty_old = MALLOCN( uint32, tile_buffer_length );

			display_set_clip_wh(0, 0, disp_actual_width, disp_height);
		}

		mark_screen_dirty();
		MEMZERON( tile_dirty_old, tile_buffer_length );
	}
}


/**
 * Take Screenshot
 */
bool display_snapshot( const scr_rect &area )
{
	if (access(SCREENSHOT_PATH_X, W_OK) == -1) {
		return false; // directory not accessible
	}

	static int number = 0;
	char filename[80];

	// find the first not used screenshot image
	do {
		sprintf(filename, SCREENSHOT_PATH_X "simscr%02d.png", number++);
	} while (access(filename, W_OK) != -1);

	// now save the screenshot
	scr_rect clipped_area = area;
	clipped_area.clip(scr_rect(0, 0, disp_actual_width, disp_height));

	raw_image_t img(clipped_area.w, clipped_area.h, raw_image_t::FMT_RGB888);

	for (scr_coord_val y = clipped_area.y; y < clipped_area.y + clipped_area.h; ++y) {
		uint8 *dst = img.access_pixel(0, y);
		const PIXVAL *row = textur + 0 + y*disp_width;

		for (scr_coord_val x = clipped_area.x; x < clipped_area.x + clipped_area.w; ++x) {
			const PIXVAL pixel = *row++;

#ifdef RGB555
			*dst++ = ((pixel >> 10) & 0x1F) << (8-5); // R
			*dst++ = ((pixel >>  5) & 0x1F) << (8-5); // G
			*dst++ = ((pixel >>  0) & 0x1F) << (8-5); // B
#else
			*dst++ = ((pixel >> 11) & 0x1F) << (8-5); // R
			*dst++ = ((pixel >>  5) & 0x3F) << (8-6); // G
			*dst++ = ((pixel >>  0) & 0x1F) << (8-5); // B
#endif
		}
	}

	return img.write_png(filename);
}
