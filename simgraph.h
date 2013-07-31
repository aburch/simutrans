/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

/*
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 */
#ifndef simgraph_h
#define simgraph_h

extern int large_font_ascent;
extern int large_font_total_height;

#include "simcolor.h"
#include "unicode.h"
#include "simtypes.h"
#include "scr_coord.h"

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

	ALIGN_LEFT       = 0x04,
	ALIGN_CENTER_H   = 0x08,
	ALIGN_RIGHT      = 0x0C,
	ALIGN_INTERIOR_H = 0x00,
	ALIGN_EXTERIOR_H = 0x20,

	// These flags does not belong in here but
	// are defined here until we sorted this out.
	// They are inly used in display_text_proportional_len_clip()
	DT_DIRTY         = 0x8000,
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

/**
 * Helper functions for clipping along tile borders.
 * @note NOT multi-thread safe
 * @author Dwachs
 */
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi=15);
void clear_all_poly_clip();
void activate_ribi_clip(int ribi=15);

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


int zoom_factor_up(void);
int zoom_factor_down(void);


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
void simgraph_init(KOORD_VAL width, KOORD_VAL height, int fullscreen);
int is_display_init(void);
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

void register_image(struct bild_t*);

// delete all images above a certain number ...
void display_free_all_images_above( unsigned above );

// unzoomed offsets
void display_set_base_image_offset( unsigned bild, KOORD_VAL xoff, KOORD_VAL yoff );
void display_get_base_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
// zoomed offsets
void display_get_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
void display_get_base_image_offset( unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
void display_mark_img_dirty( unsigned bild, KOORD_VAL x, KOORD_VAL y );

int get_maus_x(void);
int get_maus_y(void);


void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2); // clips to screen only
void mark_rect_dirty_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2); // clips to clip_rect
void mark_screen_dirty();

KOORD_VAL display_get_width(void);
KOORD_VAL display_get_height(void);
void      display_set_height(KOORD_VAL);
void      display_set_actual_width(KOORD_VAL);


/*
 * pixels stored as RGB 1555
 * @author Hajo
 */
typedef uint16 PIXVAL;


int display_get_light(void);
void display_set_light(int new_light_level);

void display_day_night_shift(int night);

// returns next matching color to an rgb
COLOR_VAL display_get_index_from_rgb( uint8 r, uint8 g, uint8 b );

// scrolls horizontally, will ignore clipping etc.
void display_scroll_band( const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h );

// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 );

// display image with day and night change
void display_img_aux(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
#define display_img( n, x, y, d ) display_img_aux( (n), (x), (y), 0, true, (d) )

/**
 * draws the images with alpha, either blended or as outline
 * @author kierongreen
 */
void display_rezoomed_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d) )

#define ALPHA_RED 0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE 0x4

void display_rezoomed_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d) )

// display image with color (if there) and optinal day and nightchange
void display_color_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);

// display unzoomed image
void display_base_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);

// Knightly : display unzoomed image with alpha, either blended or as outline
void display_base_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
void display_base_img_alpha(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);

// Knightly : pointer to image display procedures
typedef void (*display_image_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
typedef void (*display_blend_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);
typedef void (*display_alpha_proc)(const unsigned n, const unsigned alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);

// Knightly : variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
extern display_alpha_proc display_alpha;
extern signed short current_tile_raster_width;

// Knightly : call this instead of referring to current_tile_raster_width directly
#define get_current_tile_raster_width() (current_tile_raster_width)

// Knightly : for switching between image procedure sets and setting current tile raster width
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

// blends a rectangular region
void display_blend_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, int color, int percent_blend );

void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, bool dirty);
void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, bool dirty);
void display_vline_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PLAYER_COLOR_VAL color, bool dirty);
void display_vline_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PLAYER_COLOR_VAL c, bool dirty);
void display_clear(void);

void display_flush_buffer(void);

void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy);
void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr);

// compound painting routines
void display_outline_proportional(KOORD_VAL xpos, KOORD_VAL ypos, PLAYER_COLOR_VAL text_color, PLAYER_COLOR_VAL shadow_color, const char *text, int dirty);
void display_shadow_proportional(KOORD_VAL xpos, KOORD_VAL ypos, PLAYER_COLOR_VAL text_color, PLAYER_COLOR_VAL shadow_color, const char *text, int dirty);
void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color, bool dirty);
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);


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
 * If an eclipse len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next chracter. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char* text, scr_coord_val max_width, scr_coord_val eclipse_width=0 );


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

int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char* txt, control_alignment_t flags, const PLAYER_COLOR_VAL color_index, long len);
/* macro are for compatibility */
#define display_proportional(     x,  y, txt, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align | (dirty ? DT_DIRTY : 0),           color,  -1)
#define display_proportional_clip(x,  y, txt, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align | (dirty ? DT_DIRTY : 0) | DT_CLIP, color,  -1)

void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe,const char *text, int dirty);
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty);

int display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *inbuf, PLAYER_COLOR_VAL color);

void display_direct_line(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color);
void display_direct_line_dotted(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const KOORD_VAL draw, const KOORD_VAL dontDraw, const PLAYER_COLOR_VAL color);
void display_circle( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PLAYER_COLOR_VAL color );
void display_filled_circle( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PLAYER_COLOR_VAL color );
void draw_bezier(KOORD_VAL Ax, KOORD_VAL Ay, KOORD_VAL Bx, KOORD_VAL By, KOORD_VAL ADx, KOORD_VAL ADy, KOORD_VAL BDx, KOORD_VAL BDy, const PLAYER_COLOR_VAL colore, KOORD_VAL draw, KOORD_VAL dontDraw);

void display_set_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h);
clip_dimension display_get_clip_wh();

void display_snapshot( int x, int y, int w, int h );

#if COLOUR_DEPTH != 0
extern COLOR_VAL display_day_lights[  LIGHT_COUNT * 3];
extern COLOR_VAL display_night_lights[LIGHT_COUNT * 3];
#endif

#endif
