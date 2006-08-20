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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


#define DPY_WIDTH   12
#define DPY_HEIGHT  28

#define LINESPACE 11


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



#define tile_raster_scale(v, rw)   (((v)*(rw)) >> 6)


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
int simgraph_init(int width, int height, int use_shm, int do_sync);

/**
 * Loads the fonts
 * @author Hj. Malthaner
 */
void init_font(const char *fixed_font, const char *prop_font);

void load_hex_font(const char *filename);

void init_images(const char *filename);

void register_image(struct bild_besch_t *buffer);


int is_display_init();
int simgraph_exit();

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


void display_img(const int n, const int xp, const int yp,
		 const char daynight, const char dirty);

void display_color_img(const int n, const int xp, const int yp,
		       const int color, const char daynight, const char dirty);

void display_fillbox_wh(int xp, int yp, int w, int h, int color, int dirty);
void display_fillbox_wh_clip(int xp, int yp, int w, int h, int color, int d);
void display_vline_wh(const int xp, int yp, int h, const int color, int dirty);
void display_vline_wh_clip(const int xp, int yp, int h, const int c, int d);
void display_clear();

void display_flush_buffer();

void display_move_pointer(int dx, int dy);
void display_show_pointer(int yesno);


void display_pixel(int x, int y, int color);

void display_ddd_text(int xpos, int ypos, int hgt,
                      int ddd_farbe, int text_farbe,
                      const char *text, int dirty);

void display_text(int font, int x, int y, const char *txt, int color, int dirty);
void display_array_wh(int xp, int yp, int w, int h, const unsigned char *arr);

// compound painting routines

void display_ddd_box(int x1, int y1, int w, int h, int tl_color, int rd_color);
void display_ddd_box_clip(int x1, int y1, int w, int h, int tl_color, int rd_color);

#define ALIGN_LEFT 0
#define ALIGN_MIDDLE 1
#define ALIGN_RIGHT 2
int proportional_string_width(const char *text);
int proportional_string_len_width(const char *text, int len);

void display_ddd_proportional(int xpos, int ypos, int width, int hgt,
			      int ddd_farbe, int text_farbe,
			      const char *text, int dirty);
void display_ddd_proportional_clip(int xpos, int ypos, int width, int hgt,
			      int ddd_farbe, int text_farbe,
			      const char *text, int dirty);
void display_proportional(int x, int y, const char *txt,
			  int align, const int color, int dirty);
void display_proportional_clip(int x, int y, const char *txt,
			       int align, const int color, int dirty);
void display_direct_line(const int x, const int y, const int xx, const int yy, const int color);

int count_char(const char *str, const char c);
void display_multiline_text(int x, int y, const char *inbuf, int color);

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
