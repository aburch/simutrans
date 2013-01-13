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

#define LINEASCENT (large_font_ascent)
#define LINESPACE (large_font_total_height)

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
 * helper functions for clipping along tile borders
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

int	display_set_unicode(int use_unicode);

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

void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2);
void mark_screen_dirty();

KOORD_VAL display_get_width(void);
KOORD_VAL display_get_height(void);
void      display_set_height(KOORD_VAL);
void      display_set_actual_width(KOORD_VAL);


int display_get_light(void);
void display_set_light(int new_light_level);

void display_day_night_shift(int night);


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

// display image with color (if there) and optinal day and nightchange
void display_color_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);

// display unzoomed image
void display_base_img(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);

// Knightly : display unzoomed image with alpha, either blended or as outline
void display_base_img_blend(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);

// Knightly : pointer to image display procedures
typedef void (*display_image_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty);
typedef void (*display_blend_proc)(const unsigned n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty);

// Knightly : variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
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
		current_tile_raster_width = get_tile_raster_width(); \
	} \
	else { \
		display_normal = display_base_img; \
		display_color = display_base_img; \
		display_blend = display_base_img_blend; \
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
void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos);
long get_prev_char(const char* text, long pos);

KOORD_VAL display_get_char_width(utf16 c);

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
enum
{
	ALIGN_LEFT   = 0 << 0,
	ALIGN_MIDDLE = 1 << 0,
	ALIGN_RIGHT  = 2 << 0,
	ALIGN_MASK   = 3 << 0,
	DT_DIRTY     = 1 << 2,
	DT_CLIP      = 1 << 3
};

int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char* txt, int flags, PLAYER_COLOR_VAL color_index, long len);
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
