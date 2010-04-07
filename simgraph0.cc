/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * hansjoerg.malthaner@gmx.de
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

#include "simconst.h"
#include "simsys.h"
#include "simdebug.h"
#include "besch/bild_besch.h"

#ifdef _MSC_VER
#	include <io.h>
#	include <direct.h>
#	define W_OK 2
#else
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "simgraph.h"

typedef uint16 PIXVAL;

int large_font_height = 10;

KOORD_VAL tile_raster_width = 16; // zoomed
KOORD_VAL base_tile_raster_width = 16; // original

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

static inline void mark_tile_dirty(const int, const int)
{
}

static inline void mark_tiles_dirty(const int, const int, const int)
{
}

static inline int is_tile_dirty(const int, const int)
{
	return false;
}

void mark_rect_dirty_wc(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL)
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

sint16 display_get_width(void)
{
	return 0;
}

sint16 display_get_height(void)
{
	return 0;
}

sint16 display_set_height(KOORD_VAL)
{
	return 0;
}

int display_get_light(void)
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

void register_image(struct bild_t* bild)
{
	bild->bild_nr = 1;
}

void display_get_image_offset(unsigned, KOORD_VAL *, KOORD_VAL *, KOORD_VAL *, KOORD_VAL *)
{
}

void display_get_base_image_offset(unsigned, KOORD_VAL *, KOORD_VAL *, KOORD_VAL *, KOORD_VAL *)
{
}

void display_set_base_image_offset(unsigned, KOORD_VAL, KOORD_VAL)
{
}

int get_maus_x(void)
{
	return sys_event.mx;
}

int get_maus_y(void)
{
	return sys_event.my;
}

struct clip_dimension display_get_clip_wh(void)
{
	struct clip_dimension clip_rect;
	return clip_rect;
}

void display_set_clip_wh(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL)
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

void display_img_aux(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
{
}

void display_color_img(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
{
}

void display_base_img(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const int, const int)
{
}

void display_rezoomed_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
{
}

void display_base_img_blend(const unsigned, KOORD_VAL, KOORD_VAL, const signed char, const PLAYER_COLOR_VAL, const int, const int)
{
}

void display_mark_img_dirty(unsigned, KOORD_VAL, KOORD_VAL)
{
}

void display_fillbox_wh(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, int)
{
}

void display_fillbox_wh_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, int)
{
}

void display_vline_wh(const KOORD_VAL, KOORD_VAL, KOORD_VAL, const PLAYER_COLOR_VAL, int)
{
}

void display_vline_wh_clip(const KOORD_VAL, KOORD_VAL, KOORD_VAL, const PLAYER_COLOR_VAL, int)
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

int display_calc_proportional_string_len_width(const char*, size_t)
{
	return 0;
}

int display_text_proportional_len_clip(KOORD_VAL, KOORD_VAL, const char*, int, const PLAYER_COLOR_VAL, long)
{
	return 0;
}

void display_ddd_box(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL)
{
}

void display_ddd_box_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL)
{
}

void display_ddd_proportional(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
{
}

void display_ddd_proportional_clip(KOORD_VAL, KOORD_VAL, KOORD_VAL, KOORD_VAL, PLAYER_COLOR_VAL, PLAYER_COLOR_VAL, const char *, int)
{
}

void display_multiline_text(KOORD_VAL, KOORD_VAL, const char *, PLAYER_COLOR_VAL)
{
}

void display_flush_buffer(void)
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

int simgraph_init(KOORD_VAL, KOORD_VAL, int)
{
	return TRUE;
}

int is_display_init(void)
{
	return false;
}

void display_free_all_images_above( unsigned)
{
}

int simgraph_exit()
{
	return dr_os_close();
}

void simgraph_resize(KOORD_VAL, KOORD_VAL)
{
}

void display_snapshot()
{
}

void display_direct_line(const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const KOORD_VAL, const PLAYER_COLOR_VAL)
{
}

void display_set_progress_text(const char *)
{
}

void display_progress(int, int)
{
}
