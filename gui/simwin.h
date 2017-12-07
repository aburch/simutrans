/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * The function implements a WindowManager 'Object'
 */

#ifndef simwin_h
#define simwin_h

#include <stddef.h> // for ptrdiff_t

#include "../simtypes.h"
#include "../simconst.h"

class world_t;
class scr_coord;
class loadsave_t;
class gui_frame_t;
class gui_component_t;
struct event_t;

/* Types for the window */
enum wintype {
	w_info         = 1, // A info window
	w_do_not_delete= 2, // A window whose GUI object should not be deleted on close
	w_no_overlap   = 4, // try to place it below a previous window with the same flag
	w_time_delete  = 8  // deletion after MESG_WAIT has elapsed
};
ENUM_BITSET(wintype)


enum magic_numbers {
	magic_none = -1,
	magic_reserved = 0,

	// from here on, delete second 'new'-ed object in create_win
	magic_settings_frame_t,
	magic_sprachengui_t,
	magic_themes,
	magic_welt_gui_t,
	magic_climate,
	magic_reliefmap,
	magic_farbengui_t,
	magic_color_gui_t,
	magic_ki_kontroll_t,
	magic_optionen_gui_t,
	magic_sound_kontroll_t,
	magic_load_t,
	magic_save_t,
	magic_railtools,
	magic_monorailtools,
	magic_tramtools, // Dario: Tramway
	magic_roadtools,
	magic_shiptools,
	magic_airtools,
	magic_specialtools,
	magic_listtools,
	magic_edittools,
	magic_slopetools,
	magic_halt_list_t,
	magic_label_frame,
	magic_city_info_t,
	magic_citylist_frame_t,
	magic_mainhelp,
	// player dependent stuff => 16 times present
	magic_finances_t,
	magic_convoi_list=magic_finances_t+MAX_PLAYER_COUNT,
	magic_convoi_list_filter=magic_convoi_list+MAX_PLAYER_COUNT,
	magic_line_list=magic_convoi_list_filter+MAX_PLAYER_COUNT,
	magic_halt_list=magic_line_list+MAX_PLAYER_COUNT,
	magic_line_management_t=magic_halt_list+MAX_PLAYER_COUNT,
	magic_ai_options_t=magic_line_management_t+MAX_PLAYER_COUNT,
	magic_ai_selector=magic_ai_options_t+MAX_PLAYER_COUNT,
	magic_pwd_t=magic_ai_selector+MAX_PLAYER_COUNT,
	// normal stuff
	magic_jump=magic_pwd_t+MAX_PLAYER_COUNT,
	magic_curiositylist,
	magic_factorylist,
	magic_goodslist,
	magic_messageframe,
	magic_message_options,
	magic_scenario_frame,
	magic_scenario_info,
	magic_edit_factory,
	magic_edit_attraction,
	magic_edit_house,
	magic_edit_tree,
	magic_bigger_map,
	magic_labellist,
	magic_station_building_select,
	magic_server_frame_t,
	magic_pakset_info_t,
	magic_schedule_rdwr_dummy,	// only used to save/load schedules
	magic_line_schedule_rdwr_dummy,	// only used to save/load line schedules
	magic_motd,
	magic_convoi_info,
	magic_factory_info,
	magic_convoi_detail=magic_convoi_info+65536,
	magic_halt_info=magic_convoi_detail+65536,
	magic_halt_detail=magic_halt_info+65536,
	magic_toolbar=magic_halt_detail+65536,
	magic_info_pointer=magic_toolbar+256,
	magic_max = magic_info_pointer+843
};

// Holding time for auto-closing windows
#define MESG_WAIT 80

void init_map_win();

// windows with a valid id can be saved and restored
void rdwr_all_win(loadsave_t *file);

int create_win(gui_frame_t*, wintype, ptrdiff_t magic);
int create_win(int x, int y, gui_frame_t*, wintype, ptrdiff_t magic);

bool check_pos_win(event_t*);

bool win_is_open(gui_frame_t *ig );

// returns the window on this positions
gui_frame_t *win_get_oncoord( const scr_coord pt );

scr_coord const& win_get_pos(gui_frame_t const*);
void win_set_pos(gui_frame_t *ig, int x, int y);

gui_frame_t *win_get_top();

// Knightly : returns the focused component of the top window
gui_component_t *win_get_focus();

int win_get_open_count();

// returns the window (if open) otherwise zero
gui_frame_t *win_get_magic(ptrdiff_t magic);

// sets the magic of a gui_frame_t (needed during reload of windows)
bool win_set_magic( gui_frame_t *gui, ptrdiff_t magic );

/**
 * Checks if a window is a top level window
 *
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_frame_t *ig);


// return true if actually window was destroyed (or marked for destruction)
bool destroy_win(const gui_frame_t *ig);
bool destroy_win(const ptrdiff_t magic);

void destroy_all_win(bool destroy_sticky);

bool top_win(const gui_frame_t *ig, bool keep_rollup=false  );
void display_all_win();
void win_rotate90( sint16 new_size );
void move_win(int win);

void win_display_flush(double konto); // draw the frame and all windows
void win_get_event(event_t*);
void win_poll_event(event_t*);

bool win_change_zoom_factor(bool magnify);

/**
 * Sets the world this window manager is attached to.
 */
void win_set_world(world_t *world);

/**
 * Forces the redraw of the world on next frame.
 */
void win_redraw_world();

/**
 * Sets the tooltip to display.
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 * @author Hj. Malthaner, Knightly
 */
void win_set_tooltip(int xpos, int ypos, const char *text, const void *const owner = 0, const void *const group = 0);

/**
 * Sets a static tooltip that follows the mouse
 * *MUST* be explicitly unset!
 * @author Hj. Malthaner
 */
void win_set_static_tooltip(const char *text);

#endif
