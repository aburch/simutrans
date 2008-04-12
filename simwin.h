/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simwin_h
#define simwin_h

#include "simevent.h"
#include "simtypes.h"
#include "simconst.h"

#include "gui/werkzeug_waehler.h"	// for main menu

class karte_t;
class gui_fenster_t;
class gui_komponente_t;


/* Typen fuer die Fenster */
enum wintype {
	w_info         = 1,	// Ein Info-Fenster
	w_do_not_delete= 2, // Ein Info-Fenster dessen GUI-Objekt beim schliessen nicht gelöscht werden soll
	w_no_overlap   = 4, // try to place it below a previous window with the same flag
	w_time_delete  = 8	// deletion after MESG_WAIT has elapsed
};


enum magic_numbers {
	magic_none = -1,
	magic_reserved = 0,

	// from here on, delete second 'new'-ed object in create_win
	magic_sprachengui_t,
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
	magic_finances_t,
	magic_help,
	magic_convoi_t,
	magic_jump,
	magic_curiositylist,
	magic_factorylist,
	magic_goodslist,
	magic_messageframe,
	magic_edit_factory,
	magic_edit_attraction,
	magic_edit_house,
	magic_edit_tree,
	magic_info_pointer,	// mark end of the list
	magic_keyhelp=magic_info_pointer+842,
};

// Haltezeit für Nachrichtenfenster
#define MESG_WAIT 80


void init_map_win();


/**
 * redirect keyboard input into UI windows
 *
 * @return true if focus granted
 * @author Hj. Malthaner
 */
bool request_focus(gui_komponente_t *);


/**
 * current focus?
 *
 * @return true if focus granted
 * @author Hj. Malthaner
 */
bool has_focus(const gui_komponente_t *);


/**
 * redirect keyboard input into game engine
 *
 * @author Hj. Malthaner
 */
void release_focus(gui_komponente_t *);


int create_win(gui_fenster_t *ig, uint8 wt, long magic);
int create_win(int x, int y, gui_fenster_t *ig, uint8 wt, long magic);

bool check_pos_win(struct event_t *ev);

int win_get_posx(gui_fenster_t *ig);
int win_get_posy(gui_fenster_t *ig);
void win_set_pos(gui_fenster_t *ig, int x, int y);

const gui_fenster_t *win_get_top();

int win_get_open_count();

// returns the window (if open) otherwise zero
gui_fenster_t *win_get_magic(long magic);

/**
 * Checks ifa window is a top level window
 *
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_fenster_t *ig);


void destroy_win(const gui_fenster_t *ig);
void destroy_win(const long magic);
void destroy_all_win();

bool top_win(const gui_fenster_t *ig);
int top_win(int win);
void display_win(int win);
void display_all_win();
void move_win(int win);

void win_display_flush(double konto); // draw the frame and all windows
void win_get_event(struct event_t *ev);
void win_poll_event(struct event_t *ev);

void win_setze_welt(karte_t *welt);

bool win_change_zoom_factor(bool magnify);


/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_tooltip(int xpos, int ypos, const char *text);

/**
 * Sets a static tooltip that follows the mouse
 * *MUST* be explicitely unset!
 * @author Hj. Malthaner
 */
void win_set_static_tooltip(const char *text);

#endif
