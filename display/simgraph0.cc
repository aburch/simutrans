/*
 * Copyright 2010 Simutrans contributors
 * Available under the Artistic License (see license.txt)
 */

#include "../simconst.h"
#include "../simsys.h"
#include "../besch/bild_besch.h"

#include "simgraph.h"

int large_font_height = 10;
int large_font_total_height = 11;
int large_font_ascent = 9;

KOORD_VAL tile_raster_width = 16; // zoomed
KOORD_VAL base_tile_raster_width = 16; // original


/*
 * Hajo: mapping table for special-colors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
PIXVAL specialcolormap_all_day[256];


KOORD_VAL display_set_base_raster_width(KOORD_VAL)
{
	return 0;
}

void set_zoom_factor(int)
{
}

int zoom_factor_up()
{
	return false;
}

int zoom_factor_down()
{
	return false;
}

void mark_rect_dirty_wc(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL)
{
}

#ifdef MULTI_THREAD
void mark_rect_dirty_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, const sint8)
#else
void mark_rect_dirty_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL)
#endif
{
}

void mark_screen_dirty()
{
}

void display_mark_img_dirty(unsigned, KOORD_VAL, KOORD_VAL)
{
}

int display_set_unicode(int)
{
	return false;
}

bool display_load_font(const char*)
{
	return true;
}

sint16 display_get_width()
{
	return 0;
}

sint16 display_get_height()
{
	return 0;
}

void display_set_height(KOORD_VAL)
{
}

void display_set_actual_width(KOORD_VAL)
{
}

int display_get_light()
{
	return 0;
}

void display_set_light(int)
{
}

void display_day_night_shift(int)
{
}

void display_set_player_color_scheme(const int, const COLOR_VAL, const COLOR_VAL)
{
}

COLOR_VAL display_get_index_from_rgb(uint8, uint8, uint8)
{
	return 0;
}

void register_image(struct bild_t* bild)
{
	bild->bild_nr = 1;
}

void display_snapshot(int, int, int, int)
{
}

void display_get_image_offset(unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw)
{
	if (bild < 2) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

void display_get_base_image_offset(unsigned bild, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw)
{
	if (bild < 2) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

void display_set_base_image_offset(unsigned, KOORD_VAL, KOORD_VAL)
{
}

int get_maus_x()
{
	return sys_event.mx;
}

int get_maus_y()
{
	return sys_event.my;
}

#ifdef MULTI_THREAD
clip_dimension display_get_clip_wh_cl(const sint8)
#else
clip_dimension display_get_clip_wh()
#endif
{
	clip_dimension clip_rect;
	return clip_rect;
}

#ifdef MULTI_THREAD
void display_set_clip_wh_cl(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, const sint8)
#else
void display_set_clip_wh(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL)
#endif
{
}

void display_scroll_band(const KOORD_VAL, const KOORD_VAL, const KOORD_VAL)
{
}

static inline void pixcopy(PIXVAL *, const PIXVAL *, const unsigned int)
{
}

static inline void colorpixcopy(PIXVAL *, const PIXVAL *, const PIXVAL * const)
{
}

#ifdef MULTI_THREAD
void display_img_aux(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int, const sint8)
#else
void display_img_aux(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
#endif
{
}

#ifdef MULTI_THREAD
void display_color_img_cl(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int, const sint8)
#else
void display_color_img(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
#endif
{
}

#ifdef MULTI_THREAD
void display_base_img_cl(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int, const sint8)
#else
void display_base_img(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
#endif
{
}

void display_fit_img_to_width( const image_id n, sint16 new_w )
{
}

void display_img_stretch( const stretch_map_t &, scr_rect)
{
}

void display_img_stretch_blend( const stretch_map_t &, scr_rect, PLAYER_COLOR_VAL)
{
}

#ifdef MULTI_THREAD
void display_rezoomed_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int, const sint8)
#else
void display_rezoomed_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
#endif
{
}

#ifdef MULTI_THREAD
void display_rezoomed_img_alpha(const unsigned, const unsigned, const uint8, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int, const sint8)
#else
void display_rezoomed_img_alpha(const unsigned, const unsigned, const uint8, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
#endif
{
}

#ifdef MULTI_THREAD
void display_base_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int, const sint8)
#else
void display_base_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
#endif
{
}

#ifdef MULTI_THREAD
void display_base_img_alpha(const unsigned, const unsigned, const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int, const sint8)
#else
void display_base_img_alpha(const unsigned, const unsigned, const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
#endif
{
}

// Knightly : variables for storing currently used image procedure set and tile raster width
#ifdef MULTI_THREAD
display_image_proc display_normal = display_base_img_cl;
display_image_proc display_color = display_base_img_cl;
#else
display_image_proc display_normal = display_base_img;
display_image_proc display_color = display_base_img;
#endif
display_blend_proc display_blend = display_base_img_blend;
display_alpha_proc display_alpha = display_base_img_alpha;

signed short current_tile_raster_width = 0;

void display_blend_wh_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PIXVAL, int )
{
}


void display_fillbox_wh_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, bool )
{
}


#ifdef MULTI_THREAD
void display_fillbox_wh_clip_cl_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PIXVAL, bool, const sint8 )
#else
void display_fillbox_wh_clip_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, bool )
#endif
{
}

void display_vline_wh_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, bool)
{
}

#ifdef MULTI_THREAD
void display_vline_wh_clip_cl_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, bool, const sint8)
#else
void display_vline_wh_clip_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, bool)
#endif
{
}

void display_array_wh(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, const COLOR_VAL *)
{
}

size_t get_next_char(const char*, size_t pos)
{
	return pos + 1;
}

long get_prev_char(const char*, long pos)
{
	if (pos <= 0) {
		return 0;
	}
	return pos - 1;
}

KOORD_VAL display_get_char_width(utf16)
{
	return 0;
}

KOORD_VAL display_get_char_max_width(const char*, size_t)
{
	return 0;
}

unsigned short get_next_char_with_metrics(const char* &, unsigned char &, unsigned char &)
{
	return 0;
}

unsigned short get_prev_char_with_metrics(const char* &, const char *const, unsigned char &, unsigned char &)
{
	return 0;
}

size_t display_fit_proportional(const char *, scr_coord_val, scr_coord_val)
{
	return 0;
}

int display_calc_proportional_string_len_width(const char*, size_t)
{
	return 0;
}

#ifdef MULTI_THREAD
int display_text_proportional_len_clip_cl_rgb(KOORD_VAL, KOORD_VAL, const char*, control_alignment_t , const PIXVAL, bool, long, const sint8)
#else
int display_text_proportional_len_clip_rgb(KOORD_VAL, KOORD_VAL, const char*, control_alignment_t , const PIXVAL, bool, long )
#endif
{
	return 0;
}

void display_outline_proportional_rgb(KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
{
}

void display_shadow_proportional_rgb(KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
{
}

void display_ddd_box_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, bool)
{
}

void display_ddd_box_clip_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL)
{
}

void display_ddd_proportional(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
{
}

#ifdef MULTI_THREAD
void display_ddd_proportional_clip_cl(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int, const sint8)
#else
void display_ddd_proportional_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
#endif
{
}

int display_multiline_text_rgb(KOORD_VAL, KOORD_VAL, const char *, PLAYER_COLOR_VAL)
{
	return 0;
}

void display_flush_buffer()
{
}

void display_move_pointer(KOORD_VAL, KOORD_VAL)
{
}

void display_show_pointer(int)
{
}

void display_set_pointer(int)
{
}

void display_show_load_pointer(int)
{
}

void simgraph_init(KOORD_VAL, KOORD_VAL, int)
{
}

int is_display_init()
{
	return false;
}

void display_free_all_images_above( unsigned)
{
}

void simgraph_exit()
{
	dr_os_close();
}

void simgraph_resize(KOORD_VAL, KOORD_VAL)
{
}

void reset_textur(void *)
{
}

void display_snapshot()
{
}

void display_direct_line_rgb(const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const PLAYER_COLOR_VAL)
{
}

void display_direct_line_rgb(const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const PLAYER_COLOR_VAL)
{
}

void display_direct_line_dotted_rgb(const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const PLAYER_COLOR_VAL)
{
}

void display_circle_rgb( KOORD_VAL, KOORD_VAL, int, const PLAYER_COLOR_VAL )
{
}

void display_filled_circle_rgb( KOORD_VAL, KOORD_VAL, int, const PLAYER_COLOR_VAL )
{
}

void draw_bezier_rgb(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, const PLAYER_COLOR_VAL, KOORD_VAL, KOORD_VAL )
{
}

void display_set_progress_text(const char *)
{
}

void display_progress(int, int)
{
}

void display_img_aligned( const unsigned, scr_rect, int, int )
{
}

KOORD_VAL display_proportional_ellipse_rgb( scr_rect, const char *, int, PIXVAL, bool)
{
	return 0;
}

uint16 get_image_count()
{
	return 0;
}

#ifdef MULTI_THREAD
void add_poly_clip(int, int, int, int, int, const sint8)
{
}

void clear_all_poly_clip(const sint8)
{
}

void activate_ribi_clip(int, const sint8)
{
}
#else
void add_poly_clip(int, int, int, int, int)
{
}

void clear_all_poly_clip()
{
}

void activate_ribi_clip(int)
{
}
#endif
