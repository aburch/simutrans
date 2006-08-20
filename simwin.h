/*
 * simwin.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simwin_h
#define simwin_h

#ifndef simevent_h
#include "simevent.h"
#endif

#ifndef simtypes_h
#include "simtypes.h"
#endif


struct simwin;

class ding_t;
class karte_t;
class gui_fenster_t;


/* Typen fuer die Fenster */

enum wintype {
  w_info,	 	  // Ein Info-Fenster
  w_autodelete, 	  // Ein Info-Fenster dessen GUI-Objekt beimschliessen gelöscht werden soll
  w_frameless             // Ein Fenster ohne Rahmen und Titelzeile
};


enum magic_numbers {
    magic_none = -1,
    magic_reserved = 0,

    // from here on, delete second 'new'-ed object in create_win
    magic_sprachengui_t,
    magic_reliefmap,
    magic_farbengui_t,
    magic_color_gui_t,
    magic_ki_kontroll_t,
    magic_welt_gui_t,
    magic_optionen_gui_t,
    magic_sound_kontroll_t,
    magic_load_t,
    magic_save_t,
    magic_finances_t,
    magic_bridgetools,
    magic_railtools,
    magic_roadtools,
    magic_shiptools,
    magic_slopetools,
		magic_tramtools, // Dario: Tramway
    magic_convoi_t,
    magic_halt_list_t,
    magic_label_frame,
    magic_city_info_t,
    magic_autosave_t,	    // comes later
    magic_specialtools,
    magic_listtools,
    magic_edittools
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
bool request_focus();


/**
 * redirect keyboard input into game engine
 *
 * @author Hj. Malthaner
 */
void release_focus();


int create_win(gui_fenster_t *ig, enum wintype wt, int magic);
int create_win(int x, int y, gui_fenster_t *ig, enum wintype wt);
int create_win(int x, int y, int dauer, gui_fenster_t *ig, enum wintype wt, int magic = -1);

bool check_pos_win(struct event_t *ev);

int win_get_posx(gui_fenster_t *ig);
int win_get_posy(gui_fenster_t *ig);
void win_set_pos(gui_fenster_t *ig, int x, int y);


/**
 * Checks ifa window is a top level window
 *
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_fenster_t *ig);


void destroy_win(const gui_fenster_t *ig);
void destroy_all_win();

int top_win(int win);
void display_win(int win);
void display_all_win();
void move_win(int win);

void win_display_menu(); // draw the menu
void win_display_flush(int tage, int color, double konto); // draw the frame and all windows
void swallow_next_release_event(bool *set_to_false); // see simwin.cc
void win_get_event(struct event_t *ev);
void win_poll_event(struct event_t *ev);
void win_blick_aendern(karte_t * welt, struct event_t *ev);

void win_setze_welt(karte_t *welt);

void win_set_zoom_factor(int rw);


/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_tooltip(int xpos, int ypos, const char *text);

#endif
