/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

#ifndef simsys_h
#define simsys_h

#include "simtypes.h"

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
#define SIM_MOUSE_WHEELUP           8 //2003-11-04 hsiegeln added
#define SIM_MOUSE_WHEELDOWN         9 //2003-11-04 hsiegeln added

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


int dr_os_init(const int* parameter);
int dr_os_open(int w, int h, int bpp, int fullscreen);
int dr_os_close(void);

/* query home directory */
char *dr_query_homedir(void);

unsigned short* dr_textur_init(void);


void dr_textur(int xp, int yp, int w, int h);
int dr_textur_resize(unsigned short** textur,int w, int h, int bpp);
void dr_flush(void);

void dr_setRGB8multi(int first, int count, unsigned char * data);

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

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename);


#ifdef __cplusplus
}
#endif

#endif
