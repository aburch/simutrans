/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simconst.h"
#include "../sys/simsys.h"
#include "../descriptor/image.h"

#include "simgraph.h"


extern const sint32 zoom_num[1] = { 1 };
extern const sint32 zoom_den[1] = { 1 };

static PIXVAL          simgraph0_palette_lookup             (palette_index_t idx);
static palette_index_t simgraph0_palette_indexof            (PIXVAL color);
static rgb888_t        simgraph0_get_color_rgb              (palette_index_t idx);
static rgb888_t        simgraph0_get_pixval_rgb             (PIXVAL c);
static void            simgraph0_env_t_rgb_to_system_colors ();
static void            simgraph0_set_player_color_scheme    (const int player, const uint8 col1, const uint8 col2);
static void            simgraph0_set_light_color            (int light_idx, rgb888_t day_light, rgb888_t night_light);
static void            simgraph0_set_daynight_level         (int night);
static scr_coord_val   simgraph0_set_base_raster_width      (scr_coord_val new_raster);
static int             simgraph0_zoom_factor_up             ();
static int             simgraph0_zoom_factor_down           ();
static bool            simgraph0_init                       (scr_size window_size, sint16 full_screen);
static bool            simgraph0_is_display_init            ();
static void            simgraph0_exit                       ();
static void            simgraph0_on_window_resized          (scr_size new_window_size);
static bool            simgraph0_load_font                  (const char *fname, bool reload);
static image_id        simgraph0_get_image_count            ();
static image_id        simgraph0_register_image             (const image_t *image_in);
static void            simgraph0_free_all_images_above      (image_id above );
static scr_rect        simgraph0_get_base_image_offset      (image_id image);
static scr_rect        simgraph0_get_image_offset           (image_id image);
static void            simgraph0_mark_img_dirty             (image_id image, scr_coord_val xp, scr_coord_val yp);
static void            simgraph0_mark_rect_dirty_wc         (scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2);
static void            simgraph0_mark_rect_dirty_clip       (scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF);
static void            simgraph0_mark_screen_dirty          ();
static scr_size        simgraph0_get_screen_size            ();
static void            simgraph0_set_screen_actual_width    (scr_coord_val w);
static void            simgraph0_set_screen_height          (scr_coord_val const h);
static scr_size        simgraph0_get_best_matching_size     (const image_id n, sint16 zoom_percent);
static void            simgraph0_fit_img_to_width           (const image_id n, sint16 new_w);
static void            simgraph0_move_scroll_band           (scr_coord_val start_y, scr_coord_val x_offset, scr_coord_val h);
static void            simgraph0_set_image_procs            (bool is_global);
static void            simgraph0_draw_img_aligned           (const image_id n, scr_rect area, int align, const bool dirty);
static void            simgraph0_draw_img_aux               (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_rezoomed_img_blend    (const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_rezoomed_img_alpha    (const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_color_img             (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_base_img              (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_base_img_blend        (const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_base_img_alpha        (const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_stretch_map           (const stretch_map_t &, scr_rect);
static void            simgraph0_draw_stretch_map_blend     (const stretch_map_t &, scr_rect, FLAGGED_PIXVAL);
static PIXVAL          simgraph0_blend_colors               (PIXVAL, PIXVAL, int);
static void            simgraph0_tint_rect                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, int);
static void            simgraph0_draw_rect                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool);
static void            simgraph0_draw_rect_clipped          (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_rect_colors_clipped   (scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL* color, scr_coord_val num_colors, bool horizontal, bool dirty   CLIP_NUM_DEF);
static void            simgraph0_draw_rounded_rect_clipped  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool);
static void            simgraph0_draw_vline_clipped         (scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_flush_framebuffer          ();
static void            simgraph0_set_cursor_visible         (bool);
static void            simgraph0_set_default_cursor         (int);
static void            simgraph0_set_show_load_cursor       (bool);
static void            simgraph0_draw_array                 (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL *);
static scr_coord_val   simgraph0_calc_text_width_n          (const char *, size_t);
static scr_size        simgraph0_calc_multiline_text_size   (const char *);
static size_t          simgraph0_calc_text_index_for_width  (const char *, scr_coord_val);
static bool            simgraph0_font_has_character         (utf16 char_code);
static utf32           simgraph0_get_next_char_with_metrics (const char* &text, unsigned char &byte_length, unsigned char &pixel_width);
static utf32           simgraph0_get_prev_char_with_metrics (const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);
static scr_coord_val   simgraph0_get_char_width             (utf32 c);
static scr_coord_val   simgraph0_get_number_width           ();
static scr_coord_val   simgraph0_draw_text_clipped_n        (scr_coord_val, scr_coord_val, const char*, control_alignment_t , const PIXVAL, bool, sint32  CLIP_NUM_DEF_NOUSE);
static scr_coord_val   simgraph0_draw_multiline_text        (scr_coord_val, scr_coord_val, const char *, PIXVAL);
static void            simgraph0_draw_text_ellipsis_shadowed(scr_rect, const char *, int, PIXVAL, bool, bool, PIXVAL);
static void            simgraph0_draw_text_outlined         (scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int);
static void            simgraph0_draw_text_shadowed         (scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int);
static void            simgraph0_draw_box3d                 (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, bool);
static void            simgraph0_draw_box3d_clipped         (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL);
static void            simgraph0_draw_textbox3d_clipped     (scr_coord_val, scr_coord_val, FLAGGED_PIXVAL, FLAGGED_PIXVAL, const char *, int  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_draw_line                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL);
static void            simgraph0_draw_line_dotted           (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL);
static void            simgraph0_draw_empty_circle          (scr_coord_val, scr_coord_val, int, const PIXVAL);
static void            simgraph0_draw_filled_circle         (scr_coord_val, scr_coord_val, int, const PIXVAL);
static void            simgraph0_draw_bezier                (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, scr_coord_val, scr_coord_val);
static void            simgraph0_draw_right_triangle        (scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, const bool);
static bool            simgraph0_take_screenshot            (const scr_rect &);
static void            simgraph0_draw_signal_direction      (scr_coord_val, scr_coord_val, uint8, uint8, PIXVAL, PIXVAL, bool, uint8);
static void            simgraph0_set_clip_rect              (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF, bool fit);
static clip_dimension  simgraph0_get_clip_rect              (CLIP_NUM_DEF_NOUSE0);
static void            simgraph0_push_clip_rect             (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_swap_clip_rect             (CLIP_NUM_DEF_NOUSE0);
static void            simgraph0_pop_clip_rect              (CLIP_NUM_DEF_NOUSE0);

#ifdef MULTI_THREAD
static void            simgraph0_add_poly_clip              (int, int, int, int, int  CLIP_NUM_DEF_NOUSE);
static void            simgraph0_clear_all_poly_clip        (const sint8);
static void            simgraph0_activate_ribi_clip         (int  CLIP_NUM_DEF_NOUSE);
#else
static void            simgraph0_add_poly_clip              (int, int, int, int, int);
static void            simgraph0_clear_all_poly_clip        ();
static void            simgraph0_activate_ribi_clip         (int);
#endif



simgraph_t g_simgraph0 = {
	/*.type =*/ SIMGRAPH_TYPE_NULL,

	/*.tile_raster_width         =*/ 16, // zoomed
	/*.base_tile_raster_width    =*/ 16, // original
	/*.current_tile_raster_width =*/ 0,
	/*.draw_normal               =*/ NULL,
	/*.draw_color                =*/ NULL,
	/*.draw_blend                =*/ NULL,
	/*.draw_alpha                =*/ NULL,

	/*.zoom_num =*/ { 1 },
	/*.zoom_den =*/ { 0 },

	/*.palette_lookup              =*/ simgraph0_palette_lookup,
	/*.palette_indexof             =*/ simgraph0_palette_indexof,
	/*.get_color_rgb               =*/ simgraph0_get_color_rgb,
	/*.get_pixval_rgb              =*/ simgraph0_get_pixval_rgb,
	/*.env_t_rgb_to_system_colors  =*/ simgraph0_env_t_rgb_to_system_colors,
	/*.set_player_color_scheme     =*/ simgraph0_set_player_color_scheme,
	/*.set_light_color             =*/ simgraph0_set_light_color,
	/*.set_daynight_level          =*/ simgraph0_set_daynight_level,
	/*.set_base_raster_width       =*/ simgraph0_set_base_raster_width,
	/*.zoom_factor_up              =*/ simgraph0_zoom_factor_up,
	/*.zoom_factor_down            =*/ simgraph0_zoom_factor_down,
	/*.init                        =*/ simgraph0_init,
	/*.is_display_init             =*/ simgraph0_is_display_init,
	/*.exit                        =*/ simgraph0_exit,
	/*.on_window_resized           =*/ simgraph0_on_window_resized,
	/*.load_font                   =*/ simgraph0_load_font,
	/*.get_image_count             =*/ simgraph0_get_image_count,
	/*.register_image              =*/ simgraph0_register_image,
	/*.free_all_images_above       =*/ simgraph0_free_all_images_above,
	/*.get_base_image_offset       =*/ simgraph0_get_base_image_offset,
	/*.get_image_offset            =*/ simgraph0_get_image_offset,
	/*.mark_img_dirty              =*/ simgraph0_mark_img_dirty,
	/*.mark_rect_dirty_wc          =*/ simgraph0_mark_rect_dirty_wc,
	/*.mark_rect_dirty_clip        =*/ simgraph0_mark_rect_dirty_clip,
	/*.mark_screen_dirty           =*/ simgraph0_mark_screen_dirty,
	/*.get_screen_size             =*/ simgraph0_get_screen_size,
	/*.set_screen_height           =*/ simgraph0_set_screen_height,
	/*.set_screen_actual_width     =*/ simgraph0_set_screen_actual_width,
	/*.get_best_matching_size      =*/ simgraph0_get_best_matching_size,
	/*.fit_img_to_width            =*/ simgraph0_fit_img_to_width,
	/*.move_scroll_band            =*/ simgraph0_move_scroll_band,
	/*.set_image_procs             =*/ simgraph0_set_image_procs,
	/*.draw_img_aligned            =*/ simgraph0_draw_img_aligned,
	/*.draw_img_aux                =*/ simgraph0_draw_img_aux,
	/*.draw_rezoomed_img_blend     =*/ simgraph0_draw_rezoomed_img_blend,
	/*.draw_rezoomed_img_alpha     =*/ simgraph0_draw_rezoomed_img_alpha,
	/*.draw_color_img              =*/ simgraph0_draw_color_img,
	/*.draw_base_img               =*/ simgraph0_draw_base_img,
	/*.draw_base_img_blend         =*/ simgraph0_draw_base_img_blend,
	/*.draw_base_img_alpha         =*/ simgraph0_draw_base_img_alpha,
	/*.draw_stretch_map            =*/ simgraph0_draw_stretch_map,
	/*.draw_stretch_map_blend      =*/ simgraph0_draw_stretch_map_blend,
	/*.blend_colors                =*/ simgraph0_blend_colors,
	/*.tint_rect                   =*/ simgraph0_tint_rect,
	/*.draw_rect                   =*/ simgraph0_draw_rect,
	/*.draw_rect_clipped           =*/ simgraph0_draw_rect_clipped,
	/*.draw_rect_colors_clipped    =*/ simgraph0_draw_rect_colors_clipped,
	/*.draw_rounded_rect_clipped   =*/ simgraph0_draw_rounded_rect_clipped,
	/*.draw_vline_clipped          =*/ simgraph0_draw_vline_clipped,
	/*.flush_framebuffer           =*/ simgraph0_flush_framebuffer,
	/*.set_cursor_visible          =*/ simgraph0_set_cursor_visible,
	/*.set_default_cursor          =*/ simgraph0_set_default_cursor,
	/*.set_show_load_cursor        =*/ simgraph0_set_show_load_cursor,
	/*.draw_array                  =*/ simgraph0_draw_array,
	/*.font_has_character          =*/ simgraph0_font_has_character,
	/*.get_char_width              =*/ simgraph0_get_char_width,
	/*.get_number_width            =*/ simgraph0_get_number_width,
	/*.get_next_char_with_metrics  =*/ simgraph0_get_next_char_with_metrics,
	/*.get_prev_char_with_metrics  =*/ simgraph0_get_prev_char_with_metrics,
	/*.calc_text_width_n           =*/ simgraph0_calc_text_width_n,
	/*.calc_multiline_text_size    =*/ simgraph0_calc_multiline_text_size,
	/*.calc_text_index_for_width   =*/ simgraph0_calc_text_index_for_width,
	/*.draw_text_clipped_n         =*/ simgraph0_draw_text_clipped_n,
	/*.draw_multiline_text         =*/ simgraph0_draw_multiline_text,
	/*.draw_text_ellipsis_shadowed =*/ simgraph0_draw_text_ellipsis_shadowed,
	/*.draw_text_outlined          =*/ simgraph0_draw_text_outlined,
	/*.draw_text_shadowed          =*/ simgraph0_draw_text_shadowed,
	/*.draw_box3d                  =*/ simgraph0_draw_box3d,
	/*.draw_box3d_clipped          =*/ simgraph0_draw_box3d_clipped,
	/*.draw_textbox3d_clipped      =*/ simgraph0_draw_textbox3d_clipped,
	/*.draw_line                   =*/ simgraph0_draw_line,
	/*.draw_line_dotted            =*/ simgraph0_draw_line_dotted,
	/*.draw_empty_circle           =*/ simgraph0_draw_empty_circle,
	/*.draw_filled_circle          =*/ simgraph0_draw_filled_circle,
	/*.draw_bezier                 =*/ simgraph0_draw_bezier,
	/*.draw_right_triangle         =*/ simgraph0_draw_right_triangle,
	/*.draw_signal_direction       =*/ simgraph0_draw_signal_direction,
	/*.take_screenshot             =*/ simgraph0_take_screenshot,
	/*.set_clip_rect               =*/ simgraph0_set_clip_rect,
	/*.get_clip_rect               =*/ simgraph0_get_clip_rect,
	/*.push_clip_rect              =*/ simgraph0_push_clip_rect,
	/*.swap_clip_rect              =*/ simgraph0_swap_clip_rect,
	/*.pop_clip_rect               =*/ simgraph0_pop_clip_rect,
	/*.add_poly_clip               =*/ simgraph0_add_poly_clip,
	/*.clear_all_poly_clip         =*/ simgraph0_clear_all_poly_clip,
	/*.activate_ribi_clip          =*/ simgraph0_activate_ribi_clip,
};


static PIXVAL simgraph0_palette_lookup(palette_index_t idx)
{
	return idx;
}

static palette_index_t simgraph0_palette_indexof(PIXVAL color)
{
	return (palette_index_t)color;
}


static rgb888_t simgraph0_get_color_rgb(palette_index_t)
{
	return { 0,0,0 };
}

static rgb888_t simgraph0_get_pixval_rgb(palette_index_t)
{
	return { 0,0,0 };
}

static void simgraph0_env_t_rgb_to_system_colors()
{
}

static scr_coord_val simgraph0_set_base_raster_width(scr_coord_val)
{
	return 0;
}

void set_zoom_factor(int)
{
}

static int simgraph0_zoom_factor_up()
{
	return false;
}

static int simgraph0_zoom_factor_down()
{
	return false;
}

static void simgraph0_mark_rect_dirty_wc(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val)
{
}

static void simgraph0_mark_rect_dirty_clip(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_mark_screen_dirty()
{
}

static void simgraph0_mark_img_dirty(image_id, scr_coord_val, scr_coord_val)
{
}

static bool simgraph0_load_font(const char *, bool)
{
	return true;
}

static scr_size simgraph0_get_screen_size()
{
	return scr_size{ 0,0 };
}

static void simgraph0_set_screen_height(scr_coord_val)
{
}

static void simgraph0_set_screen_actual_width(scr_coord_val)
{
}

static void simgraph0_set_daynight_level(int)
{
}

static void simgraph0_set_player_color_scheme(const int, const uint8, const uint8)
{
}


static image_id simgraph0_register_image(const image_t *image)
{
	return 1;
}


static bool simgraph0_take_screenshot(const scr_rect &)
{
	return false;
}


static scr_rect simgraph0_get_image_offset(image_id)
{
	return scr_rect{ 0, 0, 0, 0 };
}


static scr_rect simgraph0_get_base_image_offset(image_id)
{
	return scr_rect{ 0, 0, 0, 0 };
}


static clip_dimension simgraph0_get_clip_rect(CLIP_NUM_DEF_NOUSE0)
{
	clip_dimension clip_rect;
	clip_rect.x  = 0;
	clip_rect.xx = 0;
	clip_rect.w  = 0;
	clip_rect.y  = 0;
	clip_rect.yy = 0;
	clip_rect.h  = 0;
	return clip_rect;
}

static void simgraph0_set_clip_rect(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE, bool)
{
}

static void simgraph0_push_clip_rect(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_swap_clip_rect(CLIP_NUM_DEF_NOUSE0)
{
}

static void simgraph0_pop_clip_rect(CLIP_NUM_DEF_NOUSE0)
{
}

static void simgraph0_move_scroll_band(scr_coord_val, scr_coord_val, scr_coord_val)
{
}

static void simgraph0_draw_img_aux(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_color_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static scr_size simgraph0_get_best_matching_size(const image_id, sint16)
{
	return scr_size(32, 32); // default size
}


static void simgraph0_draw_base_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_fit_img_to_width( const image_id, sint16)
{
}

static void simgraph0_draw_stretch_map(const stretch_map_t &, scr_rect)
{
}

static void simgraph0_draw_stretch_map_blend( const stretch_map_t &, scr_rect, FLAGGED_PIXVAL)
{
}

static void simgraph0_draw_rezoomed_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_rezoomed_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_base_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_base_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, bool  CLIP_NUM_DEF_NOUSE)
{
}

static PIXVAL simgraph0_blend_colors(PIXVAL, PIXVAL, int)
{
	return 0;
}


static void simgraph0_tint_rect(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, int)
{
}


static void simgraph0_draw_rect(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool)
{
}


static void simgraph0_draw_rect_clipped(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_rect_colors_clipped(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL* color, scr_coord_val num_colors, bool horizontal, bool dirty  CLIP_NUM_DEF)
{
}

static void simgraph0_draw_vline_clipped(scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_draw_rounded_rect_clipped(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool)
{
}

static void simgraph0_draw_array(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL *)
{
}

static scr_coord_val simgraph0_get_char_width(utf32)
{
	return 0;
}

static scr_coord_val simgraph0_get_number_width()
{
	return 0;
}

static utf32 simgraph0_get_next_char_with_metrics(const char* &, unsigned char &, unsigned char &)
{
	return 0;
}

static utf32 simgraph0_get_prev_char_with_metrics(const char* &, const char *const, unsigned char &, unsigned char &)
{
	return 0;
}

static bool simgraph0_font_has_character( utf16 )
{
	return false;
}

static size_t simgraph0_calc_text_index_for_width(const char *, scr_coord_val)
{
	return 0;
}

static scr_coord_val simgraph0_calc_text_width_n(const char*, size_t)
{
	return 0;
}


static scr_size simgraph0_calc_multiline_text_size(const char *)
{
	return { 0, 0 };
}


static scr_coord_val simgraph0_draw_text_clipped_n(scr_coord_val, scr_coord_val, const char*, control_alignment_t , const PIXVAL, bool, sint32  CLIP_NUM_DEF_NOUSE)
{
	return 0;
}

static void simgraph0_draw_text_outlined(scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int)
{
}

static void simgraph0_draw_text_shadowed(scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int)
{
}

static void simgraph0_draw_box3d(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, bool)
{
}

static void simgraph0_draw_box3d_clipped(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL)
{
}

static void simgraph0_draw_textbox3d_clipped(scr_coord_val, scr_coord_val, FLAGGED_PIXVAL, FLAGGED_PIXVAL, const char *, int  CLIP_NUM_DEF_NOUSE)
{
}

static scr_coord_val simgraph0_draw_multiline_text(scr_coord_val, scr_coord_val, const char *, PIXVAL)
{
	return 0;
}

static void simgraph0_flush_framebuffer()
{
}

static void simgraph0_set_cursor_visible(bool)
{
}

static void simgraph0_set_default_cursor(int)
{
}

static void simgraph0_set_show_load_cursor(bool)
{
}

static bool simgraph0_init(scr_size, sint16)
{
	return true;
}

static bool simgraph0_is_display_init()
{
	return false;
}

static void simgraph0_free_all_images_above(image_id)
{
}

static void simgraph0_exit()
{
	dr_os_close();
}

static void simgraph0_on_window_resized(scr_size)
{
}

void display_snapshot()
{
}

static void simgraph0_draw_line(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

static void simgraph0_draw_line_dotted(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

static void simgraph0_draw_empty_circle(scr_coord_val, scr_coord_val, int, const PIXVAL)
{
}

static void simgraph0_draw_filled_circle(scr_coord_val, scr_coord_val, int, const PIXVAL)
{
}

static void simgraph0_draw_bezier(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, scr_coord_val, scr_coord_val)
{
}

static void simgraph0_draw_right_triangle(scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, const bool)
{
}

static void simgraph0_draw_signal_direction( scr_coord_val, scr_coord_val, uint8, uint8, PIXVAL, PIXVAL, bool, uint8 )
{
}


static void simgraph0_draw_img_aligned(const image_id, scr_rect, int, bool)
{
}

static void simgraph0_draw_text_ellipsis_shadowed( scr_rect, const char *, int, PIXVAL, bool, bool, PIXVAL)
{
}

static image_id simgraph0_get_image_count()
{
	return 0;
}

#ifdef MULTI_THREAD
static void simgraph0_add_poly_clip(int, int, int, int, int  CLIP_NUM_DEF_NOUSE)
{
}

static void simgraph0_clear_all_poly_clip(const sint8)
{
}

static void simgraph0_activate_ribi_clip(int  CLIP_NUM_DEF_NOUSE)
{
}
#else
static void simgraph0_add_poly_clip(int, int, int, int, int)
{
}

static void simgraph0_clear_all_poly_clip()
{
}

static void simgraph0_activate_ribi_clip(int)
{
}
#endif


static void simgraph0_set_image_procs(bool is_global)
{
	if(  is_global  ) {
		g_simgraph0.draw_normal = simgraph0_draw_img_aux;
		g_simgraph0.draw_color  = simgraph0_draw_color_img;
		g_simgraph0.draw_blend  = simgraph0_draw_rezoomed_img_blend;
		g_simgraph0.draw_alpha  = simgraph0_draw_rezoomed_img_alpha;
		g_simgraph0.current_tile_raster_width = g_simgraph0.tile_raster_width;
	}
	else {
		g_simgraph0.draw_normal = simgraph0_draw_base_img;
		g_simgraph0.draw_color  = simgraph0_draw_base_img;
		g_simgraph0.draw_blend  = simgraph0_draw_base_img_blend;
		g_simgraph0.draw_alpha  = simgraph0_draw_base_img_alpha;
		g_simgraph0.current_tile_raster_width = gfx->get_base_tile_raster_width();
	}
}


void simgraph0_set_light_color(int, rgb888_t, rgb888_t)
{
}
