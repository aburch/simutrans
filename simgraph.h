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

/* prissi: the next line will make a version using only 8px height (like TTDX) */
//#define OTTD_LIKE


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


#ifdef OTTD_LIKE
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 7)
#define height_scaling(i) ((i)>>1)
#define height_unscaling(i) ((i)>>1)
#else
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)
#define height_scaling(i) (i)
#define height_unscaling(i) (i)
#endif

#define DPY_WIDTH   12
#define DPY_HEIGHT  28

#define LINESPACE 11


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
int simgraph_init(int width, int height, int use_shm, int do_sync, int fullscreen);
int is_display_init();
int simgraph_exit();
void simgraph_resize(int w, int h);

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
void display_set_image_offset( int bild, int xoff, int yoff );
void display_get_image_offset( int bild, int *xoff, int *yoff, int *xw, int *yw );

int gib_maus_x();
int gib_maus_y();

void mark_rect_dirty_wc(int x1, int y1, int x2, int y2);

int display_get_width();
int display_get_height();


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
void	display_scroll_band( const int start_y, const int x_offset, const int h );

// display image with day and night change
void display_img_aux(const int n, const int xp, int yp, const int dirty, bool player);
#define display_img( n, x, y, d ) display_img_aux( (n), (x), (y), (d), 0 )

// dispaly image with color (if there) and optinal day and nightchange
void display_color_img(const int n, const int xp, const int yp, const int color, const int daynight, const int dirty);

void display_fillbox_wh(int xp, int yp, int w, int h, int color, int dirty);
void display_fillbox_wh_clip(int xp, int yp, int w, int h, int color, int d);
void display_vline_wh(const int xp, int yp, int h, const int color, int dirty);
void display_vline_wh_clip(const int xp, int yp, int h, const int c, int d);
void display_clear();

void display_flush_buffer();

void display_move_pointer(int dx, int dy);
void display_show_pointer(int yesno);
void display_set_pointer(int pointer);

void display_pixel(int x, int y, int color);


void display_array_wh(int xp, int yp, int w, int h, const unsigned char *arr);

// compound painting routines

void display_ddd_box(int x1, int y1, int w, int h, int tl_color, int rd_color);
void display_ddd_box_clip(int x1, int y1, int w, int h, int tl_color, int rd_color);

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
void display_text_proportional_len_clip(int x, int y, const char *txt, int align, const int color_index, int dirty, bool use_large_font, int len, bool use_clipping );
/* macro are for compatibility */
#define display_small_proportional( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, -1, false )
#define display_small_proportional_clip( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, -1, true )
#define display_small_proportional_len_clip( x,  y, txt,  len, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, false, len, true )
#define display_proportional( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, -1, false )
#define display_proportional_clip( x,  y, txt,  align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, -1, true )
#define display_proportional_len_clip( x,  y, txt,  len, align, color, dirty) display_text_proportional_len_clip(x, y, txt, align, color, dirty, true, len, true )

void display_ddd_proportional(int xpos, int ypos, int width, int hgt,int ddd_farbe, int text_farbe,const char *text, int dirty);
void display_ddd_proportional_clip(int xpos, int ypos, int width, int hgt,int ddd_farbe, int text_farbe, const char *text, int dirty);

void display_multiline_text(int x, int y, const char *inbuf, int color);

void display_direct_line(const int x, const int y, const int xx, const int yy, const int color);

int count_char(const char *str, const char c);

void display_setze_clip_wh(int x, int y, int w, int h);
struct clip_dimension display_gib_clip_wh(void);

void display_snapshot(void);

/**
 * Loading/Saving can be to a zlib or normal file handle
 * @author Volker Meyer
 * @date  20.06.2003
 */
void display_laden(void* file, int zipped);
void display_speichern(void* file, int zipped);

void line(int x1s, int y1s, int x2s, int y2s, int col);

#ifdef __cplusplus
}
#endif

#endif
