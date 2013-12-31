/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

/*
 * Attempt of graphics for the Simulation game
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometric representation
 */
#ifndef simgraph_h
#define simgraph_h

#include "../simcolor.h"
#include "../unicode.h"
#include "../simtypes.h"
#include "simimg.h"
#include "scr_coord.h"


extern int large_font_ascent;
extern int large_font_total_height;

#define LINEASCENT (large_font_ascent)
#define LINESPACE (large_font_total_height)

/**
* Alignment enum to align controls against each other
* Vertical and horizontal alignment can be masked together
* Unused bits are reserved for future use, set to 0.
*
* @author Max Kielland
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
	// They are inly used in display_text_proportional_len_clip()
//	DT_DIRTY         = 0x8000,
	DT_CLIP          = 0x4000
};
typedef uint16 control_alignment_t;

// size of koordinates
typedef short KOORD_VAL;


struct clip_dimension {
	KOORD_VAL x, xx, w, y, yy, h;
};

// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
{\
clip_dimension const p_cr = display_get_clip_wh(); \
display_set_clip_wh(x, y, w, h);

// restore a saved clipping rect
#define POP_CLIP() \
display_set_clip_wh(p_cr.x, p_cr.y, p_cr.w, p_cr.h); \
}


/*
 * pixels stored as RGB 1555
 * @author Hajo
 */
typedef uint16 PIXVAL;

/*
 * Hajo: mapping table for special-colors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
extern PIXVAL specialcolormap_all_day[256];

#define color_idx_to_rgb(idx) (specialcolormap_all_day[(idx)&0x00FF])
#define color_rbg_to_idx(rgb) display_get_index_from_rgb(rgb)

/**
 * Helper functions for clipping along tile borders.
 * @author Dwachs
 */
#ifdef MULTI_THREAD
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi, const sint8 clip_num);
void clear_all_poly_clip(const sint8 clip_num);
void activate_ribi_clip(int ribi, const sint8 clip_num);
#else
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi=15);
void clear_all_poly_clip();
void activate_ribi_clip(int ribi=15);
#endif

/* Do no access directly, use the get_tile_raster_width()
 * macro instead.
 * @author Hj. Malthaner
 */
#define get_tile_raster_width()    (tile_raster_width)
extern KOORD_VAL tile_raster_width;

#define get_base_tile_raster_width() (base_tile_raster_width)
extern KOORD_VAL base_tile_raster_width;

/* changes the raster width after loading */
KOORD_VAL display_set_base_raster_width(KOORD_VAL new_raster);


int zoom_factor_up();
int zoom_factor_down();


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
void simgraph_init(KOORD_VAL width, KOORD_VAL height, int fullscreen);
int is_display_init();
void simgraph_exit();
void simgraph_resize(KOORD_VAL w, KOORD_VAL h);
void reset_textur(void *new_textur);

/*
 * uncomment to enable unicode
 */
#define UNICODE_SUPPORT

int display_set_unicode(int use_unicode);

/* Loads the font
 * @author prissi
 */
bool display_load_font(const char* fname);

uint16 get_image_count();
void register_image(struct bild_t*);

// delete all images above a certain number ...
void display_free_all_images_above( unsigned above );

// unzoomed offsets
//void display_set_base_image_offset( unsigned bild, KOORD_VAL xoff, KOORD_VAL yoff );
void display_get_base_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
// zoomed offsets
void display_get_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
void display_get_base_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
void display_mark_img_dirty( unsigned bild, KOORD_VAL x, KOORD_VAL y );

int get_maus_x();
int get_maus_y();


void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2); // clips to screen only
#ifdef MULTI_THREAD
void mark_rect_dirty_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2, const sint8 clip_num); // clips to clip_rect
#else
void mark_rect_dirty_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2); // clips to clip_rect
#endif
void mark_screen_dirty();

KOORD_VAL display_get_width();
KOORD_VAL display_get_height();
void      display_set_height(KOORD_VAL);
void      display_set_actual_width(KOORD_VAL);

// force a certain size on a image (for rescaling tool images)
void display_fit_img_to_width( const image_id n, sint16 new_w );


int display_get_light();
void display_set_light(int new_light_level);

void display_day_night_shift(int night);

// returns next matching color to an rgb
COLOR_VAL display_get_index_from_rgb( uint8 r, uint8 g, uint8 b );

// scrolls horizontally, will ignore clipping etc.
void display_scroll_band( const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h );

// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 );

// only used for GUI, display image inside a rect
void display_img_aligned( const unsigned n, scr_rect area, int align, const int dirty);

// display image with day and night change
#ifdef MULTI_THREAD
void display_img_aux(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty, const sint8 clip_num);
#define display_img( n, x, y, d, c ) display_img_aux( (n), (x), (y), 0, true, (d), (c) )
#else
void display_img_aux(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
#define display_img( n, x, y, d ) display_img_aux( (n), (x), (y), 0, true, (d) )
#endif

/**
 * draws the images with alpha, either blended or as outline
 * @author kierongreen
 */
#ifdef MULTI_THREAD
void display_rezoomed_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d), 0 )
#else
void display_rezoomed_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d) )
#endif

#define ALPHA_RED 0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE 0x4

#ifdef MULTI_THREAD
void display_rezoomed_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d), 0 )
#else
void display_rezoomed_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d) )
#endif

// display image with color (if there) and optional day and night change
#ifdef MULTI_THREAD
void display_color_img_cl(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty, const sint8 clip_num);
#define display_color_img( n, x, y, p, dn, d ) display_color_img_cl( (n), (x), (y), (p), (dn), (d), 0 )
#else
void display_color_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
#endif

// display unzoomed image
#ifdef MULTI_THREAD
void display_base_img_cl(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty, const sint8 clip_num);
#define display_base_img( n, x, y, p, dn, d ) display_base_img_cl( (n), (x), (y), (p), (dn), (d), 0 )
#else
void display_base_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
#endif

typedef image_id stretch_map_t[3][3];

// this displays a 3x3 array of images to fit the scr_rect
void display_img_stretch( const stretch_map_t &imag, scr_rect area );

// this displays a 3x3 array of images to fit the scr_rect like above, but blend the color
void display_img_stretch_blend( const stretch_map_t &imag, scr_rect area, PLAYER_COLOR_VAL color );

// Knightly : display unzoomed image with alpha, either blended or as outline
#ifdef MULTI_THREAD
void display_base_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
void display_base_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
#else
void display_base_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
void display_base_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#endif

// Knightly : pointer to image display procedures
#ifdef MULTI_THREAD
typedef void (*display_image_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty, const sint8 clip_num);
typedef void (*display_blend_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
typedef void (*display_alpha_proc)(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty, const sint8 clip_num);
#else
typedef void (*display_image_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
typedef void (*display_blend_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
typedef void (*display_alpha_proc)(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#endif

// Knightly : variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
extern display_alpha_proc display_alpha;
extern signed short current_tile_raster_width;

// Knightly : call this instead of referring to current_tile_raster_width directly
#define get_current_tile_raster_width() (current_tile_raster_width)

// Knightly : for switching between image procedure sets and setting current tile raster width
#ifdef MULTI_THREAD
#define display_set_image_proc( is_global ) \
{ \
	if(  is_global  ) { \
		display_normal = display_img_aux; \
		display_color = display_color_img_cl; \
		display_blend = display_rezoomed_img_blend; \
		display_alpha = display_rezoomed_img_alpha; \
		current_tile_raster_width = get_tile_raster_width(); \
	} \
	else { \
		display_normal = display_base_img_cl; \
		display_color = display_base_img_cl; \
		display_blend = display_base_img_blend; \
		display_alpha = display_base_img_alpha; \
		current_tile_raster_width = get_base_tile_raster_width(); \
	} \
}
#else
#define display_set_image_proc( is_global ) \
{ \
	if(  is_global  ) { \
		display_normal = display_img_aux; \
		display_color = display_color_img; \
		display_blend = display_rezoomed_img_blend; \
		display_alpha = display_rezoomed_img_alpha; \
		current_tile_raster_width = get_tile_raster_width(); \
	} \
	else { \
		display_normal = display_base_img; \
		display_color = display_base_img; \
		display_blend = display_base_img_blend; \
		display_alpha = display_base_img_alpha; \
		current_tile_raster_width = get_base_tile_raster_width(); \
	} \
}
#endif

// blends a rectangular region
void display_blend_wh_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, int percent_blend );
#define display_blend_wh(xp,yp,w,h,color,percent_blend) display_blend_wh_rgb( xp,yp,w,h,specialcolormap_all_day[(color)&0xFF],percent_blend )

void display_fillbox_wh_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, bool dirty);
#define display_fillbox_wh(xp,yp,w,h,color,dirty) display_fillbox_wh_rgb( xp,yp,w,h,specialcolormap_all_day[(color)&0xFF],dirty)
#ifdef MULTI_THREAD
void display_fillbox_wh_clip_cl_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, bool dirty, const sint8 clip_num);
#define display_fillbox_wh_clip_rgb( x, y, w, h, c, d ) display_fillbox_wh_clip_cl_rgb( (x), (y), (w), (h), (c), (d), 0 )
#define display_fillbox_wh_clip_cl( x, y, w, h, c, d, num ) display_fillbox_wh_clip_cl( (x), (y), (w), (h), specialcolormap_all_day[(color)&0xFF], (d), num )
#define display_fillbox_wh_clip( x, y, w, h, c, d ) display_fillbox_wh_clip_cl_rgb( (x), (y), (w), (h), specialcolormap_all_day[(c)&0xFF], (d), 0 )
#else
void display_fillbox_wh_clip_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, bool dirty);
#define display_fillbox_wh_clip(xp,yp,w,h,color,dirty) display_fillbox_wh_clip_rgb( xp,yp,w,h,specialcolormap_all_day[(color)&0xFF],dirty)
#endif

void display_vline_wh_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PIXVAL color, bool dirty);
#define display_vline_wh(xp,yp,h,color,dirty) display_vline_wh_rgb( xp,yp,h,specialcolormap_all_day[(color)&0xFF],dirty)
#ifdef MULTI_THREAD
void display_vline_wh_clip_cl_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PIXVAL c, bool dirty, const sint8 clip_num);
#define display_vline_wh_clip_rgb( x, y, h, c, d ) display_vline_wh_clip_cl_rgb( (x), (y), (h), (c), (d), 0 )
#define display_vline_wh_clip_cl( x, y, h, c, d, num ) display_vline_wh_clip_cl_rgb( (x), (y), (h), specialcolormap_all_day[(color)&0xFF], (d), num )
#define display_vline_wh_clip( x, y, h, c, d ) display_vline_wh_clip_cl_rgb( (x), (y), (h), specialcolormap_all_day[(c)&0xFF], (d), 0 )
#else
void display_vline_wh_clip_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PIXVAL c, bool dirty);
#define display_vline_wh_clip( x, y, h, c, d ) display_vline_wh_clip_rgb( (x), (y), (h), specialcolormap_all_day[(c)&0xFF], (d) )
#endif

void display_clear();

void display_flush_buffer();

void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy);
void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr);

// compound painting routines
void display_outline_proportional_rgb(KOORD_VAL xpos, KOORD_VAL ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty);
void display_shadow_proportional_rgb(KOORD_VAL xpos, KOORD_VAL ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty);
void display_ddd_box_rgb(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PIXVAL tl_color, PIXVAL rd_color, bool dirty);
void display_ddd_box_clip_rgb(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PIXVAL tl_color, PIXVAL rd_color);

#define display_outline_proportional( x, y, text_col, shadow_col, text, dirty ) display_outline_proportional_rgb( x, y, specialcolormap_all_day[(text_col)&0xFF], specialcolormap_all_day[(shadow_col)&0xFF], text, dirty )
#define display_shadow_proportional( x, y, text_col, shadow_col, text, dirty ) display_shadow_proportional_rgb( x, y, specialcolormap_all_day[(text_col)&0xFF], specialcolormap_all_day[(shadow_col)&0xFF], text, dirty )
#define display_ddd_box( x, y, w, h, box_col, shadow_col, dirty ) display_ddd_box_rgb( x, y, w, h, specialcolormap_all_day[(box_col)&0xFF], specialcolormap_all_day[(shadow_col)&0xFF], dirty )
#define display_ddd_box_clip( x, y, w, h, box_col, shadow_col ) display_ddd_box_clip_rgb( x, y, w, h, specialcolormap_all_day[(box_col)&0xFF], specialcolormap_all_day[(shadow_col)&0xFF] )


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos);
long get_prev_char(const char* text, long pos);

KOORD_VAL display_get_char_width(utf16 c);

/**
 * Returns the width of the widest character in a string.
 * @param text  pointer to a string of characters to evaluate.
 * @param len   length of text buffer to evaluate. If set to 0,
 *              evaluate until nul termination.
 * @author      Max Kielland
 */
KOORD_VAL display_get_char_max_width(const char* text, size_t len=0);

/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 * @author Knightly
 */
unsigned short get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width);

/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 * @author Knightly
 */
unsigned short get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);

/*
 * returns the index of the last character that would fit within the width
 * If an eclipse len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val eclipse_width=0 );

/* routines for string len (macros for compatibility with old calls) */
#define proportional_string_width(text)          display_calc_proportional_string_len_width(text, 0x7FFF)
#define proportional_string_len_width(text, len) display_calc_proportional_string_len_width(text, len)
// length of a string in pixel
int display_calc_proportional_string_len_width(const char* text, size_t len);

/*
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */

#ifdef MULTI_THREAD
int display_text_proportional_len_clip_cl_rgb(KOORD_VAL x, KOORD_VAL y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, long len, const sint8 clip_num);
/* macro are for compatibility */
#define display_proportional(     x,  y, txt, align, c, dirty) display_text_proportional_len_clip_cl_rgb(x, y, txt, align,           specialcolormap_all_day[(c)&0xFF], dirty, -1, 0)
#define display_proportional_clip(x,  y, txt, align, c, dirty) display_text_proportional_len_clip_cl_rgb(x, y, txt, align | DT_CLIP, specialcolormap_all_day[(c)&0xFF], dirty, -1, 0)
#define display_text_proportional_len_clip( x, y, txt, align, c, dirty, len ) display_text_proportional_len_clip_cl_rgb( (x), (y), (txt), (align), specialcolormap_all_day[(c)&0xFF], (dirty), (len), 0 )
#define display_text_proportional_len_clip_cl( x, y, txt, align, c, dirty, len, num ) display_text_proportional_len_clip_cl_rgb( (x), (y), (txt), (align), specialcolormap_all_day[(c)&0xFF], (dirty), (len), (num) )
/* macro are for compatibility */
#define display_proportional_rgb(     x,  y, txt, align, color, dirty) display_text_proportional_len_clip_cl_rgb(x, y, txt, align,           color, dirty, -1, 0)
#define display_proportional_clip_rgb(x,  y, txt, align, color, dirty) display_text_proportional_len_clip_cl_rgb(x, y, txt, align | DT_CLIP, color, dirty, -1, 0)
#define display_text_proportional_len_clip_rgb( x, y, txt, align, color, dirty, len ) display_text_proportional_len_clip_cl_rgb( (x), (y), (txt), (align), (color), (dirty), (len), 0 )
#else
int display_text_proportional_len_clip_rgb(KOORD_VAL x, KOORD_VAL y, const char* txt, control_alignment_t flags, const PIXVAL color_index, bool dirty, long len );
/* macro are for compatibility */
#define display_proportional(     x,  y, txt, align, c, dirty) display_text_proportional_len_clip_rgb(x, y, txt, align,           specialcolormap_all_day[(c)&0xFF], dirty,  -1)
#define display_proportional_clip(x,  y, txt, align, c, dirty) display_text_proportional_len_clip_rgb(x, y, txt, align | DT_CLIP, specialcolormap_all_day[(c)&0xFF], dirty,  -1)
#define display_text_proportional_len_clip(x,  y, txt, align, c, dirty, len ) display_text_proportional_len_clip_rgb(x, y, txt, align | DT_CLIP, specialcolormap_all_day[(c)&0xFF], dirty, (len) )
/* macro are for compatibility */
#define display_proportional_rgb(     x,  y, txt, align, color, dirty) display_text_proportional_len_clip_rgb(x, y, txt, align,           color, dirty, -1)
#define display_proportional_clip_rgb(x,  y, txt, align, color, dirty) display_text_proportional_len_clip_rgb(x, y, txt, align | DT_CLIP, color, dirty, -1)
#endif

/*
 * Display a string that if abreviated by the (language specific) ellipse character if too wide
 * If enough space is given, it just display the full string
 * @returns screen_width
 */
KOORD_VAL display_proportional_ellipse_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty );
#define display_proportional_ellipse( r, txt, align, c, dirty) display_proportional_ellipse_rgb( r, txt, align, specialcolormap_all_day[(c)&0xFF], dirty )

void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe,const char *text, int dirty);
#ifdef MULTI_THREAD
void display_ddd_proportional_clip_cl(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty, const sint8 clip_num);
#define display_ddd_proportional_clip( x, y, w, h, ddd, t, txt, d ) display_ddd_proportional_clip_cl( (x), (y), (w), (h), (ddd), (t), (txt), (d), 0 )
#else
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty);
#endif

int display_multiline_text_rgb(KOORD_VAL x, KOORD_VAL y, const char *inbuf, PIXVAL color);
#define display_multiline_text( x, y, buf, c ) display_multiline_text_rgb( (x), (y), (buf), specialcolormap_all_day[(c)&0xFF] )

// line drawing primitives
void display_direct_line_rgb(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PIXVAL color);
#define display_direct_line(xp,yp,xd,yd,color) display_direct_line_rgb( xp,yp,xd,yd,specialcolormap_all_day[(color)&0xFF] )
void display_direct_line_dotted_rgb(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const KOORD_VAL draw, const KOORD_VAL dontDraw, const PIXVAL color);
#define display_direct_line_dotted(xp,yp,xd,yd,draw,dont_draw,color) display_direct_line_dotted_rgb( xp,yp,xd,yd,draw,dont_draw,specialcolormap_all_day[(color)&0xFF] )
void display_circle_rgb( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PIXVAL color );
#define display_circle(xp,yp,radius,color) display_circle_rgb( xp,yp,radius,specialcolormap_all_day[(color)&0xFF] )
void display_filled_circle_rgb( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PIXVAL color );
#define display_filled_circle(xp,yp,radius,color) display_filled_circle_rgb( xp,yp,radius,specialcolormap_all_day[(color)&0xFF] )
void draw_bezier_rgb(KOORD_VAL Ax, KOORD_VAL Ay, KOORD_VAL Bx, KOORD_VAL By, KOORD_VAL ADx, KOORD_VAL ADy, KOORD_VAL BDx, KOORD_VAL BDy, const PIXVAL colore, KOORD_VAL draw, KOORD_VAL dontDraw);
#define draw_bezier(xp,yp,xd,yd,ADx,ADy,BDx,BDy,color,draw,dont_draw) draw_bezier_rgb( xp,yp,xd,yd,ADx,ADy,BDx,BDy,specialcolormap_all_day[(color)&0xFF],draw,dont_draw )

#ifdef MULTI_THREAD
void display_set_clip_wh_cl(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h, const sint8 clip_num);
#define display_set_clip_wh( x, y, w, h ) display_set_clip_wh_cl( (x), (y), (w), (h), 0 )
clip_dimension display_get_clip_wh_cl(const sint8 clip_num);
#define display_get_clip_wh() display_get_clip_wh_cl( 0 )
#else
void display_set_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h);
clip_dimension display_get_clip_wh();
#endif

void display_snapshot( int x, int y, int w, int h );

#if COLOUR_DEPTH != 0
extern COLOR_VAL display_day_lights[  LIGHT_COUNT * 3];
extern COLOR_VAL display_night_lights[LIGHT_COUNT * 3];
#endif

#endif
