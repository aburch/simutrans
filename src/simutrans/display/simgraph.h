/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_SIMGRAPH_H
#define DISPLAY_SIMGRAPH_H


#include "../simcolor.h"
#include "../utils/unicode.h"
#include "../simtypes.h"
#include "clip_num.h"
#include "simimg.h"
#include "scr_coord.h"


#if COLOUR_DEPTH != 0

extern int default_font_ascent;
extern int default_font_linespace;

#  define LINEASCENT (default_font_ascent)
#  define LINESPACE  (default_font_linespace)
#else
#  define LINEASCENT 0
// a font height of zero could cause division by zero errors, even though it should not be used in a server
#  define LINESPACE  1
#endif


/**
* Alignment enum to align controls against each other
* Vertical and horizontal alignment can be masked together
* Unused bits are reserved for future use, set to 0.
*/
enum control_alignments_t {

	ALIGN_NONE       = 0x00,

	ALIGN_TOP        = 0x01,
	ALIGN_CENTER_V   = 0x02,
	ALIGN_BOTTOM     = 0x03,
	ALIGN_INTERIOR_V = 0x00,
	ALIGN_EXTERIOR_V = 0x10,
	ALIGN_STRETCH_V  = 0x20,

	ALIGN_LEFT       = 0x04,
	ALIGN_CENTER_H   = 0x08,
	ALIGN_RIGHT      = 0x0C,
	ALIGN_INTERIOR_H = 0x00,
	ALIGN_EXTERIOR_H = 0x40,
	ALIGN_STRETCH_H  = 0x80,

	// These flags does not belong in here but
	// are defined here until we sorted this out.
	// They are only used in display_text_proportional_len_clip_rgb()
//	DT_DIRTY         = 0x8000,
	DT_CLIP          = 0x4000
};
typedef uint16 control_alignment_t;

struct clip_dimension {
	scr_coord_val x, xx, w, y, yy, h;
};

// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
{\
clip_dimension const p_cr = display_get_clip_wh(); \
display_set_clip_wh(x, y, w, h);

// save the current clipping and set a new one
// fit it to old clipping region
#define PUSH_CLIP_FIT(x,y,w,h) \
{\
	clip_dimension const p_cr = display_get_clip_wh(); \
	display_set_clip_wh(x, y, w, h CLIP_NUM_DEFAULT, true);

// restore a saved clipping rect
#define POP_CLIP() \
display_set_clip_wh(p_cr.x, p_cr.y, p_cr.w, p_cr.h); \
}

/**
 *
 */
PIXVAL color_idx_to_rgb(PIXVAL idx);
PIXVAL color_rgb_to_idx(PIXVAL color);

/*
 * Get 24bit RGB888 colour from an index of the old 8bit palette
 */
uint32 get_color_rgb(uint8 idx);

/*
 * Environment colours from RGB888 to system format
 */
void env_t_rgb_to_system_colors();

/**
 * Helper functions for clipping along tile borders.
 */
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi  CLIP_NUM_DEF);
void clear_all_poly_clip(CLIP_NUM_DEF0);
void activate_ribi_clip(int ribi  CLIP_NUM_DEF);

/* Do no access directly, use the get_tile_raster_width()
 * function instead.
 */
extern scr_coord_val tile_raster_width;
inline scr_coord_val get_tile_raster_width(){return tile_raster_width;}


extern scr_coord_val base_tile_raster_width;
inline scr_coord_val get_base_tile_raster_width(){return base_tile_raster_width;}

/* changes the raster width after loading */
scr_coord_val display_set_base_raster_width(scr_coord_val new_raster);


int zoom_factor_up();
int zoom_factor_down();


/**
 * Initialises the graphics module
 */
bool simgraph_init(scr_size window_size, sint16 fullscreen);
bool is_display_init();
void simgraph_exit();
void simgraph_resize(scr_size new_window_size);


/**
 * Loads the font, returns the number of characters in it
 * @param reload if true forces reload
 */
bool display_load_font(const char *fname, bool reload = false);

image_id get_image_count();
void register_image(class image_t *);

// delete all images above a certain number ...
void display_free_all_images_above( image_id above );

// unzoomed offsets
void display_get_base_image_offset( image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw );
// zoomed offsets
void display_get_image_offset( image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw );
void display_mark_img_dirty( image_id image, scr_coord_val x, scr_coord_val y );

void mark_rect_dirty_wc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2); // clips to screen only
void mark_rect_dirty_clip(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF); // clips to clip_rect
void mark_screen_dirty();

scr_coord_val display_get_width();
scr_coord_val display_get_height();
void display_set_height(scr_coord_val);
void display_set_actual_width(scr_coord_val);

// force a certain size on a image (for rescaling tool images)
void display_fit_img_to_width( const image_id n, sint16 new_w );

void display_day_night_shift(int night);

// scrolls horizontally, will ignore clipping etc.
void display_scroll_band( const scr_coord_val start_y, const scr_coord_val x_offset, const scr_coord_val h );

// set first and second company color for player
void display_set_player_color_scheme(const int player, const uint8 col1, const uint8 col2 );

// only used for GUI, display image inside a rect
void display_img_aligned( const image_id n, scr_rect area, int align, const bool dirty);

// display image with day and night change
void display_img_aux(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF);

/**
 * draws the images with alpha, either blended or as outline
 */
void display_rezoomed_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

#define ALPHA_RED 0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE 0x4

void display_rezoomed_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

// display image with color (if there) and optional day and night change
void display_color_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

// display unzoomed image
void display_base_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

typedef image_id stretch_map_t[3][3];

// this displays a 3x3 array of images to fit the scr_rect
void display_img_stretch( const stretch_map_t &imag, scr_rect area );

// this displays a 3x3 array of images to fit the scr_rect like above, but blend the color
void display_img_stretch_blend( const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color );

// display unzoomed image with alpha, either blended or as outline
void display_base_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_base_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

// pointer to image display procedures
typedef void (*display_image_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF);
typedef void (*display_blend_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
typedef void (*display_alpha_proc)(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);

// variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
extern display_alpha_proc display_alpha;
extern signed short current_tile_raster_width;

// call this instead of referring to current_tile_raster_width directly
#define get_current_tile_raster_width() (current_tile_raster_width)

// for switching between image procedure sets and setting current tile raster width
inline void display_set_image_proc( bool is_global )
{
	if(  is_global  ) {
		display_normal = display_img_aux;
		display_color = display_color_img;
		display_blend = display_rezoomed_img_blend;
		display_alpha = display_rezoomed_img_alpha;
		current_tile_raster_width = get_tile_raster_width();
	}
	else {
		display_normal = display_base_img;
		display_color = display_base_img;
		display_blend = display_base_img_blend;
		display_alpha = display_base_img_alpha;
		current_tile_raster_width = get_base_tile_raster_width();
	}
}

// Blends two colors
PIXVAL display_blend_colors(PIXVAL background, PIXVAL foreground, int percent_blend);

// blends a rectangular region
void display_blend_wh_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, int percent_blend );

void display_fillbox_wh_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty);

void display_fillbox_wh_clip_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_vline_wh_clip_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val h, PIXVAL c, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_flush_buffer();

void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, const PIXVAL *arr);

// compound painting routines
void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
void display_ddd_box_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color, bool dirty);
void display_ddd_box_clip_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color);


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos);
sint32 get_prev_char(const char* text, sint32 pos);

scr_coord_val display_get_char_width(utf32 c);

/* returns true, if this is a valid character */
bool has_character( utf16 char_code );

/**
 * Returns the width of the widest character in a string.
 * @param text  pointer to a string of characters to evaluate.
 * @param len   length of text buffer to evaluate. If set to 0,
 *              evaluate until null termination.
 */
scr_coord_val display_get_char_max_width(const char* text, size_t len=0);

/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 */
utf32 get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width);

/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 */
utf32 get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);

/*
 * returns the index of the last character that would fit within the width
 * If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val ellipsis_width=0 );

/* routines for string len (macros for compatibility with old calls) */
#define proportional_string_width(text)          display_calc_proportional_string_len_width(text, 0x7FFF)
#define proportional_string_len_width(text, len) display_calc_proportional_string_len_width(text, len)

// length of a string in pixel
int display_calc_proportional_string_len_width(const char* text, size_t len);

// box which will contain the multi (or single) line of text
void display_calc_proportional_multiline_string_len_width( int &xw, int &yh, const char *text, size_t len );

/*
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 */

// #ifdef MULTI_THREAD
int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, sint32 len  CLIP_NUM_DEF  CLIP_NUM_DEFAULT_ZERO);
/* macro are for compatibility */
#define display_proportional_rgb(               x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align,           color, dirty, -1 )
#define display_proportional_clip_rgb(          x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align | DT_CLIP, color, dirty, -1 )


/// Display a string that is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given, it just display the full string
void display_proportional_ellipsis_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed = false, PIXVAL shadow_color = 0 );

void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_farbe, FLAGGED_PIXVAL text_farbe, const char *text, int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);


int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *inbuf, PIXVAL color);

// line drawing primitives
void display_direct_line_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const PIXVAL color);
void display_direct_line_dotted_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const scr_coord_val draw, const scr_coord_val dontDraw, const PIXVAL color);
void display_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, const PIXVAL color );
void display_filled_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, const PIXVAL color );
void draw_bezier_rgb(scr_coord_val Ax, scr_coord_val Ay, scr_coord_val Bx, scr_coord_val By, scr_coord_val ADx, scr_coord_val ADy, scr_coord_val BDx, scr_coord_val BDy, const PIXVAL colore, scr_coord_val draw, scr_coord_val dontDraw);

void display_right_triangle_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val height, const PIXVAL colval, const bool dirty);
void display_signal_direction_rgb(scr_coord_val x, scr_coord_val y, uint8 way_dir, uint8 sig_dir, PIXVAL col1, PIXVAL col1_dark, bool is_diagonal=false, uint8 slope = 0 );

void display_set_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO, bool fit = false);
clip_dimension display_get_clip_wh(CLIP_NUM_DEF0 CLIP_NUM_DEFAULT_ZERO);

void display_push_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_swap_clip_wh(CLIP_NUM_DEF0);
void display_pop_clip_wh(CLIP_NUM_DEF0);


bool display_snapshot( const scr_rect &area );

#if COLOUR_DEPTH != 0
extern uint8 display_day_lights[  LIGHT_COUNT * 3];
extern uint8 display_night_lights[LIGHT_COUNT * 3];
#endif

#endif
