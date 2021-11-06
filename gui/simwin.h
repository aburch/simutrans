/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SIMWIN_H
#define GUI_SIMWIN_H


#include <stddef.h> // for ptrdiff_t

#include "../simtypes.h"
#include "../simconst.h"
#include "../display/scr_coord.h"


/*
 * The function implements a WindowManager 'Object'
 * There's only one WindowManager
 */


class karte_t;
class scr_coord;
class loadsave_t;
class gui_frame_t;
class gui_component_t;
struct event_t;

/* Types for the window */
enum wintype {
	w_info          = 1 << 0, // A info window
	w_do_not_delete = 1 << 1, // A window whose GUI object should not be deleted on close
	w_no_overlap    = 1 << 2, // try to place it below a previous window with the same flag
	w_time_delete   = 1 << 3  // deletion after MESG_WAIT has elapsed
};
ENUM_BITSET(wintype)


enum magic_numbers {
	magic_none     = -1,
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
	magic_UNUSED_railtools,
	magic_UNUSED_monorailtools,
	magic_UNUSED_tramtools, // Dario: Tramway
	magic_UNUSED_roadtools,
	magic_UNUSED_shiptools,
	magic_UNUSED_airtools,
	magic_UNUSED_specialtools,
	magic_UNUSED_listtools,
	magic_UNUSED_edittools,
	magic_UNUSED_slopetools,
	magic_UNUSED_halt_list_t,
	magic_UNUSED_label_frame,
	magic_city_info_t,
	magic_citylist_frame_t,
	magic_mainhelp,

	// player dependent stuff => 16 times present
	magic_finances_t,
	magic_convoi_list        = magic_finances_t         + MAX_PLAYER_COUNT,
	magic_convoi_list_filter = magic_convoi_list        + MAX_PLAYER_COUNT,
	magic_line_list          = magic_convoi_list_filter + MAX_PLAYER_COUNT,
	magic_halt_list          = magic_line_list          + MAX_PLAYER_COUNT,
	magic_line_management_t  = magic_halt_list          + MAX_PLAYER_COUNT,
	magic_ai_options_t       = magic_line_management_t  + MAX_PLAYER_COUNT,
	magic_ai_selector        = magic_ai_options_t       + MAX_PLAYER_COUNT,
	magic_pwd_t              = magic_ai_selector        + MAX_PLAYER_COUNT,
	magic_jump               = magic_pwd_t              + MAX_PLAYER_COUNT,
	magic_headquarter        = magic_jump               + MAX_PLAYER_COUNT,

	// normal stuff
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
	magic_line_schedule_rdwr_dummy,  // only used to save/load line schedules
	magic_motd,
	magic_factory_info, // only used to load/save
	magic_font,
	magic_soundfont, // only with USE_FLUIDSYNTH_MIDI
	magic_edit_groundobj,

	// magic numbers with big jumps between them
	magic_convoi_info,
	magic_UNUSED_convoi_detail = magic_convoi_info          + 0x10000, // unused range
	magic_halt_info            = magic_UNUSED_convoi_detail + 0x10000,
	magic_UNUSED_halt_detail   = magic_halt_info            + 0x10000, // unused range
	magic_toolbar              = magic_UNUSED_halt_detail   + 0x10000,
	magic_script_error         = magic_toolbar              + 0x100,
	magic_haltlist_filter,
	magic_depot, // only used to load/save
	magic_depotlist   = magic_depot + MAX_PLAYER_COUNT,
	magic_vehiclelist = magic_depotlist   + MAX_PLAYER_COUNT,
	magic_pakinstall,
	magic_max
};

// Holding time for auto-closing windows
#define MESG_WAIT 80

// windows with a valid id can be saved and restored
void rdwr_all_win(loadsave_t *file);

// save windowsizes in settings
void rdwr_win_settings(loadsave_t *file);

void win_clamp_xywh_position(scr_coord_val &x, scr_coord_val &y, scr_size wh, bool move_to_full_view);

int create_win(gui_frame_t*, wintype, ptrdiff_t magic);
int create_win(scr_coord_val x, scr_coord_val y, gui_frame_t*, wintype, ptrdiff_t magic, bool move_to_show_full=false);

// call to avoid the main menu getting mouse events while dragging
void catch_dragging();

bool check_pos_win(event_t*);

bool win_is_open(gui_frame_t *ig );


scr_coord const& win_get_pos(gui_frame_t const*);
void win_set_pos(gui_frame_t *ig, int x, int y);

gui_frame_t *win_get_top();

// returns the focused component of the top window
gui_component_t *win_get_focus();

// true, if the focus is currently in a text field
bool win_is_textinput();

uint32 win_get_open_count();

gui_frame_t* win_get_index(uint32 i);

// returns the window (if open) otherwise zero
gui_frame_t *win_get_magic(ptrdiff_t magic);

// sets the magic of a gui_frame_t (needed during reload of windows)
bool win_set_magic( gui_frame_t *gui, ptrdiff_t magic );

/**
 * Checks if a window is a top level window
 */
bool win_is_top(const gui_frame_t *ig);


// return true if actually window was destroyed (or marked for destruction)
bool destroy_win(const gui_frame_t *ig);
bool destroy_win(const ptrdiff_t magic);

void destroy_all_win(bool destroy_sticky);

void rollup_all_win();
void rolldown_all_win();

bool top_win(const gui_frame_t *ig, bool keep_rollup=false  );
void display_all_win();
void win_rotate90( sint16 new_size );
void move_win(int win);

void win_display_flush(double konto); // draw the frame and all windows

uint16 win_get_statusbar_height();

void win_poll_event(event_t*);

bool win_change_zoom_factor(bool magnify);

/**
 * Sets the world this window manager is attached to.
 */
void win_set_world(karte_t *world);

/**
 * Forces the redraw of the world on next frame.
 */
void win_redraw_world();

/**
 * Loads new font. Notifies gui's, world.
 */
void win_load_font(const char *fname, uint8 fontsize);

/**
 * Sets the tooltip to display.
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 */
void win_set_tooltip(scr_coord_val xpos, scr_coord_val ypos, const char *text, const void *const owner = 0, const void *const group = 0);

/**
 * Sets a static tooltip that follows the mouse
 * *MUST* be explicitly unset!
 */
void win_set_static_tooltip(const char *text);

// shows a modal dialoge (blocks other interaction)
void modal_dialogue(gui_frame_t* gui, ptrdiff_t magic, karte_t* welt, bool (*quit)());

#endif
