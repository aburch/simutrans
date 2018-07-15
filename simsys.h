/*
 * definitions for the system dependent part of simutrans
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef simsys_h
#define simsys_h

#include <stddef.h>
#include "simtypes.h"
#include <zlib.h>

// Provide chdir().
#if defined(_WIN32) && !defined(__CYGWIN__)
#	include <direct.h>
#else
#	include <unistd.h>
#endif

/* Variable for message processing */

/* Classes */

#define SIM_NOEVENT         0
#define SIM_MOUSE_BUTTONS   1
#define SIM_KEYBOARD        2
#define SIM_MOUSE_MOVE      3
#define SIM_STRING          4
#define SIM_SYSTEM          254
#define SIM_IGNORE_EVENT    255

/* Actions */ /* added RIGHTUP and MIDUP */
#define SIM_MOUSE_LEFTUP            1
#define SIM_MOUSE_RIGHTUP           2
#define SIM_MOUSE_MIDUP             3
#define SIM_MOUSE_LEFTBUTTON        4
#define SIM_MOUSE_RIGHTBUTTON       5
#define SIM_MOUSE_MIDBUTTON         6
#define SIM_MOUSE_MOVED             7
#define SIM_MOUSE_WHEELUP           8 //2003-11-04 hsiegeln added
#define SIM_MOUSE_WHEELDOWN         9 //2003-11-04 hsiegeln added

/* Global Variable for message processing */

struct sys_event
{
	unsigned long type;
	union {
		unsigned long code;
		void *ptr;
	};
	int mx;                  /* es sind negative Koodinaten mgl */
	int my;
	int mb;
	/**
	 * new window size for SYSTEM_RESIZE
	 */
	int size_x, size_y;
	unsigned int key_mod; /* key mod, like ALT, STRG, SHIFT */
};

extern struct sys_event sys_event;

extern char const PATH_SEPARATOR[];

// scale according to dpi setting
bool dr_auto_scale(bool);

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

// Functions the same as normal mkdir except path must be UTF-8 encoded and a default mode of 0777 is assumed.
int dr_mkdir(char const* path);

/**
 * Moves the specified file to the system's trash bin.
 * If trash is not available on the platform, removes file.
 * @param path UTF-8 path to the file to delete.
 * @return False on success.
 */
bool dr_movetotrash(const char *path);

/**
 * Returns true if platform supports recycle bin, otherwise false.
 * Used to control which UI tooltip is shown for deletion.
 */
bool dr_cantrash();

// Functions the same as cstdio remove except path must be UTF-8 encoded.
int dr_remove(const char *path);

// rename a file and delete eventually existing file new_utf8
int dr_rename( const char *existing_utf8, const char *new_utf8 );

// Functions the same as chdir except path must be UTF-8 encoded.
int dr_chdir(const char *path);

// Functions the same as getcwd except path must be UTF-8 encoded.
char *dr_getcwd(char *buf, size_t size);

// Functions the same as fopen except filename must be UTF-8 encoded.
FILE *dr_fopen(const char *filename, const char *mode);

// Functions the same as gzopen except path must be UTF-8 encoded.
gzFile dr_gzopen(const char *path, const char *mode);

// Functions the same as stat except path must be UTF-8 encoded.
int dr_stat(const char *path, struct stat *buf);

/* query home directory */
char const* dr_query_homedir();

unsigned short* dr_textur_init();

// returns the file path to a font file (or more than one, if used with number higher than zero)
const char *dr_query_fontpath( int );

void dr_textur(int xp, int yp, int w, int h);

/* returns the actual width (might be larger than requested! */
int dr_textur_resize(unsigned short** textur, int w, int h);

// needed for screen update
void dr_prepare_flush();	// waits, if previous update not yet finished
void dr_flush();	// copy to screen (eventually multithreaded)

/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b);

void show_pointer(int yesno);

void set_pointer(int loading);

void move_pointer(int x, int y);

int get_mouse_x();
int get_mouse_y();

void ex_ord_update_mx_my();

void GetEvents();
void GetEventsNoWait();

uint32 dr_time();
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
 * @param portabel : true if local files to be save in simutransdir
 * @return false, if nothing was downloaded
 */
bool dr_download_pakset( const char *path_to_program, bool portable );

/**
 * Shows the touch keyboard when using systems without a hardware keyboard.
 * Will be ignored if there is an hardware keyboard available.
 */
void dr_start_textinput();

/**
 * Hides the touch keyboard when using systems without a hardware keyboard.
 * Will be ignored it there is no on-display keyboard shown.
 */
void dr_stop_textinput();

/**
 * Inform the IME of a ideal place to open its popup.
 */
void dr_notify_input_pos(int x, int y);

int sysmain(int argc, char** argv);

#endif
