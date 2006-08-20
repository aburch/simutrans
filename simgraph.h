/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 */

/* simgraph.h
 *
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 *
 */
#ifndef simgraph_h
#define simgraph_h

/* prissi: if uncommented, also support for a small size font will be enabled (by not used at the moment) */
//#define USE_SMALL_FONT

#ifdef __cplusplus
extern "C" {
// since our simgraph16.c ist a plain c-file, it will never see this
extern int large_font_height;
#else
// only needed for non C++
typedef enum {false=0, true=1 } bool;
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "simcolor.h"

#define DPY_WIDTH   12
#define DPY_HEIGHT  28

#define LINESPACE 11

// size of koordinates
typedef short KOORD_VAL;


typedef struct
{
	int	height;
	int	descent;
	unsigned num_chars;
	char	name[256];
	unsigned char	*screen_width;
	unsigned char	*char_data;
} font_type;


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


// function prototypes

struct bild_besch_t;


/* Do no access directly, use the get_tile_raster_width()
 * macro instead.
 * @author Hj. Malthaner
 */
extern int tile_raster_width;

#define get_tile_raster_width()    (tile_raster_width)


int get_zoom_factor();
void set_zoom_factor(int rw);


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int use_shm, int do_sync, int fullscreen);
int is_display_init();
int simgraph_exit();
void simgraph_resize(KOORD_VAL w, KOORD_VAL h);

/*
 * uncomment to enable unicode
 */
#define UNICODE_SUPPORT

/* unicode helper functions
 * @author prissi
 */
int	display_get_unicode(void);
int	display_set_unicode(int use_unicode);
int unicode_get_previous_character( const char *text, int cursor_pos);
int unicode_get_next_character( const char *text, int cursor_pos);
unsigned short utf82unicode (unsigned char const *ptr, int *iLen );
int	unicode2utf8( unsigned unicode, unsigned char *out );

/* Loads the fonts (large=true loads large font)
 * @author prissi
 */
bool display_load_font(const char *fname, bool large );

/* checks if a small and a large font exists;
 * if not the missing font will be emulated
 * @author prissi
 */
void	display_check_fonts(void);

int display_get_font_height_small();
int display_get_font_height();

void register_image(struct bild_besch_t *buffer);
void register_image_copy(struct bild_besch_t *bild);

void display_set_image_offset( unsigned bild, int xoff, int yoff );
void display_get_image_offset( unsigned bild, int *xoff, int *yoff, int *xw, int *yw );

int gib_maus_x();
int gib_maus_y();

void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2);

KOORD_VAL display_get_width();
KOORD_VAL display_get_height();
KOORD_VAL display_set_height(KOORD_VAL);


int  display_get_light();
void display_set_light(int new_light_level);

int display_get_color();
void display_set_color(int new_color_level);

void display_day_night_shift(int night);


/**
 * Sets color set for player 0
 * @param entry   number of color set, range 0..15
 * @author Hj. Malthaner
 */
void display_set_player_color(int entry);

// scrolls horizontally, will ignore clipping etc.
void	display_scroll_band( const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h );

// display image with day and night change
void display_img_aux(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool player);
#define display_img( n, x, y, d ) display_img_aux( (n), (x), (y), (d), 0 )

// dispaly image with color (if there) and optinal day and nightchange
void display_color_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const COLOR_VAL color, const int daynight, const int dirty);

void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty);
void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int d);
void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty);
void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL c, int d);
void display_clear();

void display_flush_buffer();

void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy);
void display_show_pointer(int yesno);
void display_set_pointer(int pointer);

void display_pixel(KOORD_VAL x, KOORD_VAL y, PLAYER_COLOR_VAL color);


void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr);

// compound painting routines

void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color);

#define ALIGN_LEFT 0
#define ALIGN_MIDDLE 1
#define ALIGN_RIGHT 2

/* routines for string len (macros for compatibility with old calls) */
#define small_proportional_string_width(text) display_calc_proportional_string_len_width( text, 0x7FFF, false )
#define proportional_string_width(text) display_calc_proportional_string_len_width( text, 0x7FFF, true )
#define proportional_string_len_width(text,len) display_calc_proportional_string_len_width( text, len, true )

int display_calc_proportional_string_len_width(const char *text, int len,bool use_large_font );

/*
 * len parameter added - use -1 for previous bvbehaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */
int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char *txt, int align, const PLAYER_COLOR_VAL color_index, int dirty, bool use_large_font, int len, bool use_clipping );
/* macro are for compatibility */
#define display_small_proportional( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, -1, false )
#define display_small_proportional_clip( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, -1, true )
#define display_small_proportional_len_clip( x,  y, txt,  len, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, len, true )
#define display_proportional( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, -1, false )
#define display_proportional_clip( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, -1, true )
#define display_proportional_len_clip( x,  y, txt,  len, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, len, true )

void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe,const char *text, int dirty);
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt,PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty);

void display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *inbuf, PLAYER_COLOR_VAL color);

void display_direct_line(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color);

int count_char(const char *str, const char c);

void display_setze_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h);
struct clip_dimension display_gib_clip_wh(void);

void display_snapshot(void);

/**
 * Loading/Saving can be to a zlib or normal file handle
 * @author Volker Meyer
 * @date  20.06.2003
 */
void display_laden(void* file, int zipped);
void display_speichern(void* file, int zipped);

#ifdef __cplusplus
}
#endif

#endif
