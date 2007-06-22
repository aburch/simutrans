/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
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

#ifdef __cplusplus
extern "C" {
// since our simgraph16.c ist a plain c-file, it will never see this
extern int large_font_height;
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "simcolor.h"

#define LINESPACE 11

// size of koordinates
typedef short KOORD_VAL;


struct clip_dimension {
    int x, xx, w, y, yy, h;
};


// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
{\
const struct clip_dimension p_cr = display_gib_clip_wh(); \
display_setze_clip_wh(x, y, w, h);

// restore a saved clipping rect
#define POP_CLIP() \
display_setze_clip_wh(p_cr.x, p_cr.y, p_cr.w, p_cr.h); \
}


/* Do no access directly, use the get_tile_raster_width()
 * macro instead.
 * @author Hj. Malthaner
 */
#define get_tile_raster_width()    (tile_raster_width)
extern int tile_raster_width;

/* changes the raster width after loading */
int display_set_base_raster_width(int new_raster);


int get_zoom_factor(void);
void set_zoom_factor(int rw);


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int use_shm, int do_sync, int fullscreen);
int is_display_init(void);
int simgraph_exit(void);
void simgraph_resize(KOORD_VAL w, KOORD_VAL h);

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

void display_set_image_offset( unsigned bild, int xoff, int yoff );
void display_get_image_offset( unsigned bild, int *xoff, int *yoff, int *xw, int *yw );
void display_mark_img_dirty( unsigned bild, int x, int y );

int gib_maus_x(void);
int gib_maus_y(void);

void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2);

KOORD_VAL display_get_width(void);
KOORD_VAL display_get_height(void);
KOORD_VAL display_set_height(KOORD_VAL);


int display_get_light(void);
void display_set_light(int new_light_level);

void display_day_night_shift(int night);


// scrolls horizontally, will ignore clipping etc.
void	display_scroll_band( const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h );

// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 );

// display image with day and night change
void display_img_aux(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool player);
#define display_img( n, x, y, d ) display_img_aux( (n), (x), (y), (d), 0 )

/**
 * draws the images with alpha, either blended or as outline
 * @author kierongreen
 */
void display_img_blend(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const PLAYER_COLOR_VAL color, const int daynight, const int dirty);

// display image with color (if there) and optinal day and nightchange
void display_color_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const signed char color, const int daynight, const int dirty);

void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty);
void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int d);
void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty);
void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL c, int d);
void display_clear(void);

void display_flush_buffer(void);

void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy);
void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr);

// compound painting routines

void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);


// unicode save moving in strings
int get_next_char(const char* text, int pos);
int get_prev_char(const char* text, int pos);

/* routines for string len (macros for compatibility with old calls) */
#define proportional_string_width(text)          display_calc_proportional_string_len_width(text, 0x7FFF)
#define proportional_string_len_width(text, len) display_calc_proportional_string_len_width(text, len)
// length of a string in pixel
int display_calc_proportional_string_len_width(const char* text, int len);

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

int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char* txt, int flags, PLAYER_COLOR_VAL color_index, int len);
/* macro are for compatibility */
#define display_proportional(     x,  y, txt, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align | (dirty ? DT_DIRTY : 0),           color,  -1)
#define display_proportional_clip(x,  y, txt, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align | (dirty ? DT_DIRTY : 0) | DT_CLIP, color,  -1)

void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe,const char *text, int dirty);
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty);

void display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *inbuf, PLAYER_COLOR_VAL color);

void display_direct_line(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color);

int count_char(const char *str, const char c);

void display_setze_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h);
struct clip_dimension display_gib_clip_wh(void);

void display_snapshot(void);

#ifdef __cplusplus
}
#endif

#endif
