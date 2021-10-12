/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simconst.h"
#include "../sys/simsys.h"
#include "../descriptor/image.h"

#include "simgraph.h"


scr_coord_val tile_raster_width = 16; // zoomed
scr_coord_val base_tile_raster_width = 16; // original


PIXVAL color_idx_to_rgb(PIXVAL idx)
{
	return idx;
}

PIXVAL color_rgb_to_idx(PIXVAL color)
{
	return color;
}


uint32 get_color_rgb(uint8)
{
	return 0;
}

void env_t_rgb_to_system_colors()
{
}

scr_coord_val display_set_base_raster_width(scr_coord_val)
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

void mark_rect_dirty_wc(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val)
{
}

void mark_rect_dirty_clip(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

void mark_screen_dirty()
{
}

void display_mark_img_dirty(image_id, scr_coord_val, scr_coord_val)
{
}

bool display_load_font(const char*, bool)
{
	return true;
}

scr_coord_val display_get_width()
{
	return 0;
}

scr_coord_val display_get_height()
{
	return 0;
}

void display_set_height(scr_coord_val)
{
}

void display_set_actual_width(scr_coord_val)
{
}

void display_day_night_shift(int)
{
}

void display_set_player_color_scheme(const int, const uint8, const uint8)
{
}

void register_image(image_t* image)
{
	image->imageid = 1;
}

bool display_snapshot(const scr_rect &)
{
	return false;
}

void display_get_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < 2  ) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

void display_get_base_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < 2  ) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

clip_dimension display_get_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
	clip_dimension clip_rect;
	clip_rect.x = 0;
	clip_rect.xx = 0;
	clip_rect.w = 0;
	clip_rect.y = 0;
	clip_rect.yy = 0;
	clip_rect.h = 0;
	return clip_rect;
}

void display_set_clip_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE, bool)
{
}

void display_push_clip_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

void display_swap_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
}

void display_pop_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
}

void display_scroll_band(const scr_coord_val, const scr_coord_val, const scr_coord_val)
{
}

void display_img_aux(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_color_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_fit_img_to_width( const image_id, sint16)
{
}

void display_img_stretch( const stretch_map_t &, scr_rect)
{
}

void display_img_stretch_blend( const stretch_map_t &, scr_rect, FLAGGED_PIXVAL)
{
}

void display_rezoomed_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_rezoomed_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, bool  CLIP_NUM_DEF_NOUSE)
{
}

// variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = display_base_img;
display_image_proc display_color = display_base_img;
display_blend_proc display_blend = display_base_img_blend;
display_alpha_proc display_alpha = display_base_img_alpha;

signed short current_tile_raster_width = 0;

PIXVAL display_blend_colors(PIXVAL, PIXVAL, int)
{
	return 0;
}


void display_blend_wh_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, int )
{
}


void display_fillbox_wh_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool )
{
}


void display_fillbox_wh_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_vline_wh_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_array_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL *)
{
}

size_t get_next_char(const char*, size_t pos)
{
	return pos + 1;
}

sint32 get_prev_char(const char*, sint32 pos)
{
	if (pos <= 0) {
		return 0;
	}
	return pos - 1;
}

scr_coord_val display_get_char_width(utf32)
{
	return 0;
}

scr_coord_val display_get_char_max_width(const char*, size_t)
{
	return 0;
}

utf32 get_next_char_with_metrics(const char* &, unsigned char &, unsigned char &)
{
	return 0;
}

utf32 get_prev_char_with_metrics(const char* &, const char *const, unsigned char &, unsigned char &)
{
	return 0;
}

bool has_character( utf16 )
{
	return false;
}

size_t display_fit_proportional(const char *, scr_coord_val, scr_coord_val)
{
	return 0;
}

int display_calc_proportional_string_len_width(const char*, size_t)
{
	return 0;
}


void display_calc_proportional_multiline_string_len_width( int &xw, int &yh, const char *, size_t )
{
	xw = yh = 0;;
}


int display_text_proportional_len_clip_rgb(scr_coord_val, scr_coord_val, const char*, control_alignment_t , const PIXVAL, bool, sint32  CLIP_NUM_DEF_NOUSE)
{
	return 0;
}

void display_outline_proportional_rgb(scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int, sint32)
{
}

void display_shadow_proportional_rgb(scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int, sint32)
{
}

void display_ddd_box_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, bool)
{
}

void display_ddd_box_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL)
{
}

void display_ddd_proportional_clip(scr_coord_val, scr_coord_val, FLAGGED_PIXVAL, FLAGGED_PIXVAL, const char *, int  CLIP_NUM_DEF_NOUSE)
{
}

int display_multiline_text_rgb(scr_coord_val, scr_coord_val, const char *, PIXVAL)
{
	return 0;
}

void display_flush_buffer()
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

bool simgraph_init(scr_size, sint16)
{
	return true;
}

bool is_display_init()
{
	return false;
}

void display_free_all_images_above(image_id)
{
}

void simgraph_exit()
{
	dr_os_close();
}

void simgraph_resize(scr_size)
{
}

void display_snapshot()
{
}

void display_direct_line_rgb(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

void display_direct_line_dotted_rgb(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

void display_circle_rgb( scr_coord_val, scr_coord_val, int, const PIXVAL )
{
}

void display_filled_circle_rgb( scr_coord_val, scr_coord_val, int, const PIXVAL )
{
}

void draw_bezier_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, scr_coord_val, scr_coord_val )
{
}

void display_right_triangle_rgb(scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, const bool)
{
}

void display_signal_direction_rgb( scr_coord_val, scr_coord_val, uint8, uint8, PIXVAL, PIXVAL, bool, uint8 )
{
}

void display_set_progress_text(const char *)
{
}

void display_progress(int, int)
{
}

void display_img_aligned( const image_id, scr_rect, int, bool )
{
}

void display_proportional_ellipsis_rgb( scr_rect, const char *, int, PIXVAL, bool, bool, PIXVAL)
{
}

image_id get_image_count()
{
	return 0;
}

#ifdef MULTI_THREAD
void add_poly_clip(int, int, int, int, int  CLIP_NUM_DEF_NOUSE)
{
}

void clear_all_poly_clip(const sint8)
{
}

void activate_ribi_clip(int  CLIP_NUM_DEF_NOUSE)
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
