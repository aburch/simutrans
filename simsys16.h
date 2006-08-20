/*
 * simsys16.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simsys_h
#define simsys_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


/* Variablen zur Messageverarbeitung */

/* Klassen */

#define SIM_NOEVENT         0
#define SIM_MOUSE_BUTTONS   1
#define SIM_KEYBOARD        2
#define SIM_MOUSE_MOVE      3
#define SIM_SYSTEM          254
#define SIM_IGNORE_EVENT    255

/* Aktionen */ /* added RIGHTUP and MIDUP */
#define SIM_MOUSE_LEFTUP            1
#define SIM_MOUSE_RIGHTUP           2
#define SIM_MOUSE_MIDUP             3
#define SIM_MOUSE_LEFTBUTTON        4
#define SIM_MOUSE_RIGHTBUTTON       5
#define SIM_MOUSE_MIDBUTTON         6
#define SIM_MOUSE_MOVED             7
#define SIM_MOUSE_WHEELUP           8 //2004-11-03 hsiegeln added
#define SIM_MOUSE_WHEELDOWN         9 //2004-11-03 hsiegeln added

#define SIM_SYSTEM_QUIT             1
#define SIM_SYSTEM_RESIZE             2
#define SIM_SYSTEM_UPDATE           3
#define SIM_F1                      256

/* Globale Variablen zur Messagebearbeitung */

struct sys_event
{
    unsigned long type;
    unsigned long code;
    int mx;                  /* es sind negative Koodinaten mgl */
    int my;
	unsigned int key_mod; /* key mod, like ALT, STRG, SHIFT */
};

extern struct sys_event sys_event;


int dr_os_init(int n, int *parameter);
int dr_os_open(int w, int h, int fullscreen);
int dr_os_close();

unsigned short * dr_textur_init();
int dr_textur_resize( unsigned short **textur,int w, int h);

unsigned short *dr_start_update();
unsigned short *dr_end_update();

/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b);


/**
 * Does this system wrapper need software cursor?
 * @return true if a software cursor is needed
 * @author Hj. Malthaner
 */
int dr_use_softpointer();

void dr_textur(int xp, int yp, int w, int h);
void dr_flush();

void show_pointer(int yesno);

void move_pointer(int x, int y);


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename);


void ex_ord_update_mx_my();

void GetEvents();
void GetEventsNoWait();

unsigned long dr_time(void);
void dr_sleep(unsigned long usec);

/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename);

/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume);


/**
 * sets midi playback volume
 * @param vol volume in range 0..255
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol);


/**
 * gets midi title
 * @author Hj. Malthaner
 */
const char * dr_get_midi_title(int index);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
int dr_load_midi(const char *filename);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
void dr_play_midi(int key);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
void dr_stop_midi();


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
long dr_midi_pos();


/**
 * Midi shutdown/cleanup
 * @author Owen Rudge
 */
void dr_destroy_midi();
void set_midi_pos(int);

#ifdef __cplusplus
}
#endif

#endif
