/*
 * definitions for the system dependent part of simutrans
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef simsys_h
#define simsys_h

#include <stddef.h>
#include "simtypes.h"

// Provide chdir().
#ifdef _WIN32
#	include <direct.h>
#else
#	include <unistd.h>
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
#define SIM_MOUSE_WHEELUP           8 //2003-11-04 hsiegeln added
#define SIM_MOUSE_WHEELDOWN         9 //2003-11-04 hsiegeln added

#define SIM_SYSTEM_QUIT             1
#define SIM_SYSTEM_RESIZE             2
#define SIM_SYSTEM_UPDATE           3

/* Globale Variablen zur Messagebearbeitung */

struct sys_event
{
	unsigned long type;
	unsigned long code;
	int mx;                  /* es sind negative Koodinaten mgl */
	int my;
	int mb;
	unsigned int key_mod; /* key mod, like ALT, STRG, SHIFT */
};

extern struct sys_event sys_event;


bool dr_os_init(int const* parameter);

/* maximum size possible (if there) */
struct resolution
{
	int w;
	int h;
};
resolution dr_query_screen_resolution();

int dr_os_open(int w, int h, int fullscreen);
void dr_os_close();

// returns the locale; NULL if unknown
const char *dr_get_locale_string();

void dr_mkdir(char const* path);

/* query home directory */
char const* dr_query_homedir();

unsigned short* dr_textur_init(void);


void dr_textur(int xp, int yp, int w, int h);

/* returns the actual width (might be larger than requested! */
int dr_textur_resize(unsigned short** textur, int w, int h);

// needed for screen update
void dr_prepare_flush();	// waits, if previous update not yet finished
void dr_flush(void);	// copy to screen (eventuall multithreaded)

/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b);

void show_pointer(int yesno);

void set_pointer(int loading);

void move_pointer(int x, int y);

void ex_ord_update_mx_my(void);

void GetEvents(void);
void GetEventsNoWait(void);

unsigned long dr_time(void);
void dr_sleep(uint32 millisec);

// error message in case of fatal events
void dr_fatal_notify(char const* msg);

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename, int x, int y, int w, int h);

/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 * @author Knightly
 */
void dr_copy(const char *source, size_t length);

/**
 * Paste text from the clipboard
 * @param target : pointer to the insertion position in the target text
 * @param max_length : maximum number of character bytes which could be inserted
 * @return number of character bytes actually inserted -> for cursor advancing
 * @author Knightly
 */
size_t dr_paste(char *target, size_t max_length);

/**
 * Open a program/starts a script to download pak sets from sourceforge
 * @param path_to_program : actual simutrans pakfile directory
 * @param portabel : true if lokal files to be save in simutransdir
 * @return false, if nothing was downloaded
 */
bool dr_download_pakset( const char *path_to_program, bool portable );

int sysmain(int argc, char** argv);

#endif
