/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_SIMGRAPH_H
#define DISPLAY_SIMGRAPH_H


#include "../simcolor.h"
#include "../simtypes.h"
#include "clip_num.h"
#include "simimg.h"
#include "scr_coord.h"


class image_t;


/**
* Alignment enum to align controls against each other.
* Vertical and horizontal alignment can be masked together
* Unused bits are reserved for future use, set to 0.
*/
enum control_alignments_t {

	ALIGN_NONE       = 0x00,

	ALIGN_TOP        = 0x01,
	ALIGN_CENTER_V   = 0x02,
	ALIGN_BOTTOM     = 0x03,

	ALIGN_LEFT       = 0x04,
	ALIGN_CENTER_H   = 0x08,
	ALIGN_RIGHT      = 0x0C,

	// These flags does not belong in here but
	// are defined here until we sorted this out.
	// They are only used in display_text_proportional_len_clip_rgb()
	DT_CLIP          = 0x4000
};
typedef uint16 control_alignment_t;

struct clip_dimension {
	scr_coord_val x, xx, w, y, yy, h;
};

// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
	{\
		clip_dimension const p_cr = gfx->get_clip_rect(CLIP_NUM_DEFAULT_VALUE); \
		gfx->set_clip_rect(x, y, w, h CLIP_NUM_DEFAULT, false);

// save the current clipping and set a new one
// fit it to old clipping region
#define PUSH_CLIP_FIT(x,y,w,h) \
	{\
		clip_dimension const p_cr = gfx->get_clip_rect(CLIP_NUM_DEFAULT_VALUE); \
		gfx->set_clip_rect(x, y, w, h CLIP_NUM_DEFAULT, true);

// restore a saved clipping rect
#define POP_CLIP() \
		gfx->set_clip_rect(p_cr.x, p_cr.y, p_cr.w, p_cr.h CLIP_NUM_DEFAULT, false); \
	}


// pointer to image display procedures
typedef void (*draw_image_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF);
typedef void (*draw_blend_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
typedef void (*draw_alpha_proc)(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);


#define ALPHA_RED   0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE  0x4

/// 9-slice type
typedef image_id stretch_map_t[3][3];

#if COLOUR_DEPTH != 0
#  define MAX_ZOOM_FACTOR (9)
#  define ZOOM_NEUTRAL    (3)
#else
#  define MAX_ZOOM_FACTOR (0)
#  define ZOOM_NEUTRAL    (0)
#endif


enum simgraph_type_t
{
	SIMGRAPH_TYPE_NULL     = 0, // Dummy renderer, used for headless servers
	SIMGRAPH_TYPE_SOFTWARE = 1, // 16 bit software (CPU) renderer, default
};


/// Graphics renderer interface
struct simgraph_t
{
	simgraph_type_t type;

	/// Do no access directly, use the get_* functions below instead.
	scr_coord_val tile_raster_width;
	scr_coord_val base_tile_raster_width;
	signed short current_tile_raster_width;

	// variables for storing currently used image procedure set
	draw_image_proc draw_normal;
	draw_image_proc draw_color;
	draw_blend_proc draw_blend;
	draw_alpha_proc draw_alpha;

	inline scr_coord_val get_tile_raster_width()         const { return tile_raster_width;         }
	inline scr_coord_val get_base_tile_raster_width()    const { return base_tile_raster_width;    }
	inline scr_coord_val get_current_tile_raster_width() const { return current_tile_raster_width; }

	sint32 zoom_num[MAX_ZOOM_FACTOR+1];
	sint32 zoom_den[MAX_ZOOM_FACTOR+1];

	//
	// Colour palette stuff
	//

	/// Looks up a colour from its palette index
	PIXVAL (*palette_lookup)(palette_index_t idx);

	/// Retrieves the palette index of a color, or returns 0 if the color is not in the palette.
	palette_index_t (*palette_indexof)(PIXVAL color);

	/// Get 24bit RGB888 colour from an index of the old 8bit palette
	rgb888_t(*get_color_rgb)(palette_index_t idx);

	/// Get 24bit RGB888 colour from a PIVAL
	rgb888_t(*get_pixval_rgb)(PIXVAL c);

	/// Environment colours from RGB888 to system format
	void (*env_t_rgb_to_system_colors)();

	/// set primary and secondary company color for player
	void (*set_player_color_scheme)(int player_idx, uint8 col1_idx, uint8 col2_idx);

	/// Change the value of the day/night palette entry at index @p color_idx
	void (*set_light_color)(int color_idx, rgb888_t day_color, rgb888_t night_color);

	void (*set_daynight_level)(int night_level);


	/// changes the raster width after loading
	scr_coord_val (*set_base_raster_width)(scr_coord_val new_raster);

	int (*zoom_factor_up)();
	int (*zoom_factor_down)();

	/// Initialises the graphics module
	bool (*init)(scr_size window_size, sint16 fullscreen);

	bool (*is_display_init)();

	/// Closes the graphics module
	void (*exit)();

	void (*on_window_resized)(scr_size new_window_size);

	/// Loads the font, returns the number of characters in it
	/// @param reload if true forces reload
	bool (*load_font)(const char *fname, bool reload /*=false*/);

	/// Returns the number of currently registered images.
	image_id (*get_image_count)();

	/// Registers an image with the renderer so the image can be drawn.
	/// @returns the image id (should be stored to img->imgid)
	image_id (*register_image)(const image_t *img);

	// delete all images above a certain number ...
	void (*free_all_images_above)(image_id above);

	/// unzoomed offsets
	scr_rect (*get_base_image_offset)(image_id image);

	/// zoomed offsets
	scr_rect (*get_image_offset)(image_id image);

	/// Mark image @p image at screen position (x,y) as dirty.
	void (*mark_img_dirty)(image_id image, scr_coord_val x, scr_coord_val y);

	/// Marks a rectangular region of the screen as dirty.
	/// Clips to screen only.
	void (*mark_rect_dirty_wc)(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2);

	/// As @ref mark_rect_dirty_wc, but clips to active clip rect (@ref set_clip_rect)
	void (*mark_rect_dirty_clip)(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF); // clips to clip_rect

	/// Mark the whole screen dirty.
	void (*mark_screen_dirty)();

	/// Returns the size of the drawable area.
	/// @note The size of the underlying render target texture may be larger for alignment reasons.
	scr_size (*get_screen_size)();

	void (*set_screen_height)      (scr_coord_val new_height);
	void (*set_screen_actual_width)(scr_coord_val new_actual_width);

	// get next smallest size when scaling to percent
	scr_size (*get_best_matching_size)(image_id n, sint16 zoom_percent);

	/// force a certain size on a image (for rescaling tool images)
	void (*fit_img_to_width)(image_id n, sint16 new_w);

	/// scrolls horizontally @p x_offset pixels to the left, will ignore clipping etc.
	void (*move_scroll_band)(scr_coord_val start_y, scr_coord_val x_offset, scr_coord_val h);

	/// for switching between image procedure sets, and for setting current tile raster width
	void (*set_image_procs)(bool is_global);

	/// Only used for GUI, display image inside a rect
	void (*draw_img_aligned)(image_id n, scr_rect area, int align, bool dirty);

	/// display image with day and night change
	void (*draw_img_aux)(image_id n, scr_coord_val xp, scr_coord_val yp, signed char player_nr, bool daynight, bool dirty  CLIP_NUM_DEF);

	/// draws the images with alpha, either blended or as outline
	void (*draw_rezoomed_img_blend)(image_id n, scr_coord_val xp, scr_coord_val yp, signed char player_nr, FLAGGED_PIXVAL color_index, bool daynight, bool dirty  CLIP_NUM_DEF);
	#define draw_img_blend( n, x, y, c, dn, d ) draw_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

	void (*draw_rezoomed_img_alpha)(image_id n, image_id alphamask_n, unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, signed char player_nr, FLAGGED_PIXVAL color_index, bool daynight, bool dirty  CLIP_NUM_DEF);
	#define draw_img_alpha( n, a, f, x, y, c, dn, d ) draw_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

	/// display image with color (if there) and optional day and night change
	void (*draw_color_img)(image_id n, scr_coord_val xp, scr_coord_val yp, signed char player_nr, bool daynight, bool dirty  CLIP_NUM_DEF);

	// display unzoomed image
	void (*draw_base_img)(image_id n, scr_coord_val xp, scr_coord_val yp, signed char player_nr, bool daynight, bool dirty  CLIP_NUM_DEF);

	// display unzoomed image with alpha, either blended or as outline
	void (*draw_base_img_blend)(image_id n, scr_coord_val xp, scr_coord_val yp, signed char player_nr, FLAGGED_PIXVAL color_index, bool daynight, bool dirty  CLIP_NUM_DEF);
	void (*draw_base_img_alpha)(image_id n, image_id alpha_n, unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, sint8 player_nr, FLAGGED_PIXVAL color_index, bool daynight, bool dirty  CLIP_NUM_DEF);

	/// this displays a 3x3 array of images to fit the scr_rect
	void (*draw_stretch_map)(const stretch_map_t &imag, scr_rect area);

	/// this displays a 3x3 array of images to fit the scr_rect like above, but blend the color
	void (*draw_stretch_map_blend)(const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color);

	/// Blends two colors
	PIXVAL (*blend_colors)(PIXVAL background, PIXVAL foreground, int percent_blend);

	/// Tints a rectangular framebuffer region with @p color using linear blending, clips to active clip rect
	/// @param opacity_percent 0=do nothing, 100=replace framebuffer completely
	void (*tint_rect)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, int opacity_percent);

	/// Fills a rectangular framebuffer region with a solid color.
	/// @note Ignores active clip rect
	void (*draw_rect)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty);

	/// Same as @ref draw_rect, but clips to active clip rect.
	void (*draw_rect_clipped)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty  CLIP_NUM_DEF);

	///  banded rectangle with multicolors
	void (*draw_rect_colors_clipped)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL* color, scr_coord_val num_colors, bool horizontal, bool dirty  CLIP_NUM_DEF);

	/// Draw rect with rounded corners.
	void (*draw_rounded_rect_clipped)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty);

	/// Draws vertical line from {xp,yp} tp {xp,yp+h-1}
	void (*draw_vline_clipped)(scr_coord_val xp, scr_coord_val yp, scr_coord_val h, PIXVAL color, bool dirty  CLIP_NUM_DEF);

	/// Flushes all dirty areas to the screen.
	void (*flush_framebuffer)();

	/// Shows or hides the mouse cursor.
	void (*set_cursor_visible)(bool visible);

	void (*set_default_cursor)(int cursor_id);

	/// Changes whether the load cursor or the default cursor is shown.
	void (*set_show_load_cursor)(bool show);

	/// Copies rectangular region of pixels to the framebuffer.
	void (*draw_array)(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, const PIXVAL *arr);


	//
	// Font stuff and glyph metrics
	// TODO this should maybe be moved somewhere else
	//

	/// returns true if this is a valid character for the currently loaded font
	bool (*font_has_character)( utf16 char_code );

	scr_coord_val (*get_char_width)(utf32 c);
	scr_coord_val (*get_number_width)();

	/**
	 * For the next logical character in the text, returns the character code
	 * as well as retrieves the char byte count and the screen pixel width
	 * CAUTION : The text pointer advances to point to the next logical character
	 */
	utf32 (*get_next_char_with_metrics)(const char *&text, unsigned char &byte_length, unsigned char &pixel_width);

	/**
	 * For the previous logical character in the text, returns the character code
	 * as well as retrieves the char byte count and the screen pixel width
	 * CAUTION : The text pointer recedes to point to the previous logical character
	 */
	utf32 (*get_prev_char_with_metrics)(const char *&text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);


	//
	// Text rendering
	//

	/// Calculates the screen width of a UTF-8 encoded string in pixels.
	/// @param len number of bytes in @p text, or -1 if @p text is null-terminated.
	scr_coord_val (*calc_text_width_n)(const char *text, size_t len);

	inline scr_coord_val calc_text_width(const char *text) const { return calc_text_width_n(text, 0x7FFFu); }

	/// @returns Size of the bounding box which contains the single- or multiline @p text
	scr_size (*calc_multiline_text_size)(const char *text);

	/// Returns the index of the last character that would fit within the width
	/// If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
	/// @returns index of next character. if text[index]==0 the whole string fits
	size_t (*calc_text_index_for_width)(const char *text, scr_coord_val max_width);

	scr_coord_val (*draw_text_clipped_n)(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, PIXVAL color, bool dirty, sint32 len  CLIP_NUM_DEF);

	inline scr_coord_val draw_text_clipped(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, PIXVAL color, bool dirty) const
	{
		return draw_text_clipped_n(x, y, txt, flags | DT_CLIP, color, dirty, -1 CLIP_NUM_DEFAULT);
	}

	inline scr_coord_val draw_text(scr_coord_val x, scr_coord_val y, const char *txt, control_alignment_t flags, PIXVAL color, bool dirty) const
	{
		return draw_text_clipped_n(x, y, txt, flags, color, dirty, -1 CLIP_NUM_DEFAULT);
	}

	scr_coord_val (*draw_multiline_text)(scr_coord_val x, scr_coord_val y, const char *text, PIXVAL color);

	/// Display a string that is abbreviated by the (language specific) ellipsis character if too wide
	/// If enough space is given, it just display the full string
	void (*draw_text_ellipsis_shadowed)(scr_rect r, const char *text, int align, PIXVAL color, bool dirty, bool shadow, PIXVAL shadow_color);

	void draw_text_ellipsis(scr_rect r, const char *text, int align, PIXVAL color, bool dirty) const
	{
		draw_text_ellipsis_shadowed(r, text, align, color, dirty, false, 0);
	}

	// compound painting routines
	void (*draw_text_outlined)(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty);
	void (*draw_text_shadowed)(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty);

	void (*draw_box3d)        (scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color, bool dirty);
	void (*draw_box3d_clipped)(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color);

	void (*draw_textbox3d_clipped)(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_farbe, FLAGGED_PIXVAL text_farbe, const char *text, int dirty  CLIP_NUM_DEF);

	/// Draw a straight solid line.
	void (*draw_line)       (scr_coord_val x, scr_coord_val y, scr_coord_val xx, scr_coord_val yy, PIXVAL color);
	void (*draw_line_dotted)(scr_coord_val x, scr_coord_val y, scr_coord_val xx, scr_coord_val yy, scr_coord_val draw, scr_coord_val dontDraw, PIXVAL color);

	void (*draw_empty_circle) (scr_coord_val x0, scr_coord_val y0, int radius, PIXVAL color);
	void (*draw_filled_circle)(scr_coord_val x0, scr_coord_val y0, int radius, PIXVAL color);

	/// Draw a cubic Bezier curve between (Ax,Ay) and (Bx,By).
	void (*draw_bezier)(scr_coord_val Ax, scr_coord_val Ay, scr_coord_val Bx, scr_coord_val By, scr_coord_val ADx, scr_coord_val ADy, scr_coord_val BDx, scr_coord_val BDy, PIXVAL colore, scr_coord_val draw, scr_coord_val dontDraw);

	/// Draw a right-facing triangle (e.g. for convoi progress bars)
	void (*draw_right_triangle)(scr_coord_val x, scr_coord_val y, scr_coord_val height, PIXVAL colval, bool dirty);

	void (*draw_signal_direction)(scr_coord_val x, scr_coord_val y, uint8 way_dir, uint8 sig_dir, PIXVAL col1, PIXVAL col1_dark, bool is_diagonal, uint8 slope);

	/// Takes a screenshot of @p screen_area and saves the image as 'simscrXX.png' (with XX being a placeholder for two numeric characters).
	/// @returns true on success
	/// @note For simgraph0 this is a no-op (as there is no display)
	bool (*take_screenshot)(const scr_rect &screen_area);

	//
	// Clipping
	//

	void (*set_clip_rect)(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF, bool fit /*=false*/);

	clip_dimension (*get_clip_rect)(CLIP_NUM_DEF0);

	void (*push_clip_rect)(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF);

	void (*swap_clip_rect)(CLIP_NUM_DEF0);

	void (*pop_clip_rect)(CLIP_NUM_DEF0);

	/// Add a polygonal clip line (for clipping along tile borders)
	void (*add_poly_clip)(int x0_,int y0_, int x1, int y1, int ribi  CLIP_NUM_DEF);
	void (*clear_all_poly_clip)(CLIP_NUM_DEF0);
	void (*activate_ribi_clip)(int ribi  CLIP_NUM_DEF);
};


const simgraph_t *simgraph_select(simgraph_type_t preferred_type);

/// Assign to this the output of simgraph_select
extern const simgraph_t *gfx;


//
// Variables required for layouting
//

extern scr_coord_val default_font_ascent;
extern scr_coord_val default_font_linespace;

#define LINEASCENT (default_font_ascent)
#define LINESPACE  (default_font_linespace)


#endif
