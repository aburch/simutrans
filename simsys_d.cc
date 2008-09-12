/*
 * Bit depth independent backend using allgro (mainly for BeOS)
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <math.h>

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

#include "macros.h"
#include "simmain.h"
#include "simsys.h"
#include "simevent.h"
#include "simgraph.h"

#ifdef _WIN32
#define BITMAP winBITMAP
#define WinMain winWinMain
#include <windows.h>
#undef BITMAP
#undef WinMain
#endif

#include <allegro.h>



static void simtimer_init(void);

static int width  = 800;
static int height = 600;

struct sys_event sys_event;


/* event-handling */

/* queue Eintraege sind wie folgt aufgebaut
 * 4 integer werte
 *  class
 *  code
 *  mx
 *  my
 */

#define queue_length 4096


static volatile unsigned int event_top_mark = 0;
static volatile unsigned int event_bot_mark = 0;
static volatile int event_queue[queue_length * 5 + 8];


#define INSERT_EVENT(a, b) \
	event_queue[event_top_mark++] = a; \
	event_queue[event_top_mark++] = b; \
	event_queue[event_top_mark++] = mouse_x; \
	event_queue[event_top_mark++] = mouse_y; \
	event_queue[event_top_mark++] = mouse_b; \
	if (event_top_mark >= queue_length * 5) event_top_mark = 0;


void my_mouse_callback(int flags)
{
	if (flags & MOUSE_FLAG_MOVE) {
		INSERT_EVENT(SIM_MOUSE_MOVE, SIM_MOUSE_MOVED)
	}

	if (flags & MOUSE_FLAG_LEFT_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTBUTTON)
	}

	if (flags & MOUSE_FLAG_MIDDLE_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDBUTTON)
	}

	if (flags & MOUSE_FLAG_RIGHT_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTBUTTON)
	}

	if (flags & MOUSE_FLAG_LEFT_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTUP)
	}

	if (flags & MOUSE_FLAG_MIDDLE_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDUP)
	}

	if (flags & MOUSE_FLAG_RIGHT_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTUP)
	}
}
END_OF_FUNCTION(my_mouse_callback)


static int my_keyboard_callback(int this_key, int* scancode)
{
	if (this_key > 0) {
		// seems to be ASCII
		INSERT_EVENT(SIM_KEYBOARD, this_key);
		*scancode = 0;
		return 0;
	}

	int numlock = (key_shifts & KB_NUMLOCK_FLAG) != 0;

	switch (*scancode) {
		case KEY_PGUP: this_key = '>'; break;
		case KEY_PGDN: this_key = '<'; break;

		case KEY_HOME:  this_key = SIM_KEY_HOME;  break;
		case KEY_END:   this_key = SIM_KEY_END;   break;
		case KEY_DOWN:  this_key = SIM_KEY_DOWN;  break;
		case KEY_LEFT:  this_key = SIM_KEY_LEFT;  break;
		case KEY_RIGHT: this_key = SIM_KEY_RIGHT; break;
		case KEY_UP:    this_key = SIM_KEY_UP;    break;

		case KEY_0_PAD: this_key = '0';                           break;
		case KEY_1_PAD: this_key = '1';                           break;
		case KEY_2_PAD: this_key = numlock ? '2' : SIM_KEY_DOWN;  break;
		case KEY_3_PAD: this_key = '3';                           break;
		case KEY_4_PAD: this_key = numlock ? '4' : SIM_KEY_LEFT;  break;
		case KEY_5_PAD: this_key = '5';                           break;
		case KEY_6_PAD: this_key = numlock ? '6' : SIM_KEY_RIGHT; break;
		case KEY_7_PAD: this_key = '7';                           break;
		case KEY_8_PAD: this_key = numlock ? '8' : SIM_KEY_UP;    break;
		case KEY_9_PAD: this_key = '9';                           break;

		case KEY_F1:  this_key = SIM_KEY_F1;  break;
		case KEY_F2:  this_key = SIM_KEY_F2;  break;
		case KEY_F3:  this_key = SIM_KEY_F3;  break;
		case KEY_F4:  this_key = SIM_KEY_F4;  break;
		case KEY_F5:  this_key = SIM_KEY_F5;  break;
		case KEY_F6:  this_key = SIM_KEY_F6;  break;
		case KEY_F7:  this_key = SIM_KEY_F7;  break;
		case KEY_F8:  this_key = SIM_KEY_F8;  break;
		case KEY_F9:  this_key = SIM_KEY_F9;  break;
		case KEY_F10: this_key = SIM_KEY_F10; break;
		case KEY_F11: this_key = SIM_KEY_F11; break;
		case KEY_F12: this_key = SIM_KEY_F12; break;

		case KEY_ENTER_PAD: this_key =  13; break;
		case KEY_DEL:       this_key = 127; break;

		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_LCONTROL:
		case KEY_RCONTROL:
		case KEY_NUMLOCK:
			return this_key;

		default:
			*scancode = 0;
			return 0;
	}

	INSERT_EVENT(SIM_KEYBOARD, this_key);
	*scancode = 0;
	return 0;
}
END_OF_FUNCTION(my_keyboard_callback)



void my_close_button_callback(void)
{
	INSERT_EVENT(SIM_SYSTEM, SIM_SYSTEM_QUIT)
}
END_OF_FUNCTION(my_close_button_callback)



/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */


int dr_os_init(const int* parameter)
{
	int ok = allegro_init();

	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	LOCK_VARIABLE(event_top_mark);
	LOCK_VARIABLE(event_bot_mark);
	LOCK_VARIABLE(event_queue);
	LOCK_FUNCTION(my_mouse_callback);
	LOCK_FUNCTION(my_keyboard_callback);
	LOCK_FUNCTION(my_close_button_callback);
	set_close_button_callback(my_close_button_callback);

	if (ok == 0) {
		simtimer_init();
	}

	return ok == 0;
}


/* maximum size possible (if there) */
int dr_query_screen_width()
{
#ifdef _WIN32
	return GetSystemMetrics( SM_CXSCREEN );
#else
	int w, h;
	if(get_desktop_resolution(&w, &h) == 0) {
		return w;
	}
	return width;
#endif
}



int dr_query_screen_height()
{
#ifdef _WIN32
	return GetSystemMetrics( SM_CYSCREEN );
#else
	int w, h;
	if(get_desktop_resolution(&w, &h) == 0) {
		return h;
	}
	return height;
#endif
}



int dr_os_open(int w, int h, int bpp, int fullscreen)
{
	width = w;
	height = h;


	install_keyboard();

	set_color_depth(bpp);
	if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
		fprintf(stderr, "Error: %s\n", allegro_error);
		return FALSE;
	}

	if (install_mouse() < 0) {
		fprintf(stderr, "Cannot init. mouse: no driver ?");
		return FALSE;
	}

	set_mouse_speed(1, 1);

	mouse_callback = my_mouse_callback;
	keyboard_ucallback = my_keyboard_callback;

	sys_event.mx = mouse_x;
	sys_event.my = mouse_y;

	set_window_title("Simutrans");

	return TRUE;
}


int dr_os_close(void)
{
	allegro_exit();
	return TRUE;
}


// query home directory
char *dr_query_homedir(void)
{
	static char buffer[1024];
	char b2[1060];
#ifdef _WIN32
	DWORD len=960;
	HKEY hHomeDir;
	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ,	&hHomeDir)==ERROR_SUCCESS) {
		RegQueryValueExA(hHomeDir,"Personal",NULL,NULL,(BYTE *)buffer,&len);
		strcat(buffer,"\\Simutrans");
		CreateDirectoryA( buffer, NULL );
		strcat(buffer, "\\");

		// create other subdirectories
		sprintf(b2, "%ssave", buffer );
		CreateDirectoryA( b2, NULL );
		sprintf(b2, "%sscreenshot", buffer );
		CreateDirectoryA( b2, NULL );
		sprintf(b2, "%smaps", buffer );
		CreateDirectoryA( b2, NULL );

		return buffer;
	}
	return NULL;
#else
#ifndef __MACOS__
	sprintf( buffer, "%s/simutrans", getenv("HOME") );
#else
	sprintf( buffer, "%s/Documents/simutrans", getenv("HOME") );
#endif
	int err = mkdir( buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
	if(err  &&  err!=EEXIST) {
		// could not create directory
		// we assume success anyway
	}
	strcat( buffer, "/" );
	sprintf( b2, "%smaps", buffer );
	mkdir( b2, 0700 );
	sprintf( b2, "%sscreenshot", buffer );
	mkdir( b2, 0700 );
	sprintf( b2, "%ssave", buffer );
	mkdir( b2, 0700 );
	return buffer;
#endif
}



/*
 * Hier beginnen die eigentlichen graphischen Funktionen
 */

static BITMAP* texture_map;

unsigned short* dr_textur_init(void)
{
	texture_map = create_bitmap(width, height);
	if (texture_map == NULL) {
		printf("Error: can't create double buffer bitmap, aborting!");
		exit(1);
	}

//	return reinterpret_cast<unsigned short*>(texture_map->line[0]);
	return (unsigned short *)(texture_map->line[0]);
}


// reiszes screen (Not allowed)
int dr_textur_resize(unsigned short** textur, int w, int h, int bpp)
{
	return FALSE;
}


void dr_textur(int xp, int yp, int w, int h)
{
	blit(texture_map, screen, xp, yp, xp, yp, w, h);
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
#if 1// USE_16BIT_DIB
	return ((r & 0x00F8) << 8) | ((g & 0x00FC) << 3) | (b >> 3);
#else
	return ((r & 0x00F8) << 7) | ((g & 0x00F8) << 2) | (b >> 3); // 15 Bit
#endif
}


void dr_setRGB8multi(int first, int count, unsigned char* data)
{
	PALETTE p;
	int n;

	for (n = 0; n < count; n++) {
		p[n + first].r = data[n * 3 + 0] >> 2;
		p[n + first].g = data[n * 3 + 1] >> 2;
		p[n + first].b = data[n * 3 + 2] >> 2;
	}

	set_palette_range(p, first, first + count - 1, TRUE);
}


void dr_prepare_flush()
{
	return;
}



void dr_flush(void)
{
	display_flush_buffer();
}


#ifdef WIN32
// try saving png using gdiplus.dll
extern "C" int dr_screenshot_png(const char *filename,  int w, int h, unsigned short *data, int bitdepth );
#endif

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
#ifdef WIN32
	if(dr_screenshot_png(filename, width, height, (short unsigned int *)texture_map, 16)) {
		return 1;
	}
#endif
	return 0;
}


void move_pointer(int x, int y)
{
	position_mouse(x, y);
}


void set_pointer(int loading)
{
	// not supported
}


static inline int recalc_keys(void)
{
	int mod = key_shifts;

	return
		(mod & KB_SHIFT_FLAG ? 1 : 0) |
		(mod & KB_CTRL_FLAG  ? 2 : 0);
}



static void internalGetEvents(void)
{
	if (event_top_mark != event_bot_mark) {
		sys_event.type    = event_queue[event_bot_mark++];
		sys_event.code    = event_queue[event_bot_mark++];
		sys_event.mx      = event_queue[event_bot_mark++];
		sys_event.my      = event_queue[event_bot_mark++];
		sys_event.mb      = event_queue[event_bot_mark++];
		sys_event.key_mod = recalc_keys();

		if (event_bot_mark >= queue_length * 5) {
			event_bot_mark = 0;
		}
	} else {
		sys_event.type = SIM_NOEVENT;
		sys_event.code = SIM_NOEVENT;
		sys_event.mx   = mouse_x;
		sys_event.my   = mouse_y;
		sys_event.mb   = mouse_b;
	}
}


void GetEvents(void)
{
	while (event_top_mark == event_bot_mark) {
		// try to be nice where possible
#if !defined(__MINGW32__)
#if!defined(__BEOS__)
		usleep(1000);
#endif
#else
		Sleep(5);
#endif
	}

	do {
		internalGetEvents();
	} while(sys_event.type == SIM_MOUSE_MOVE);
}


void GetEventsNoWait(void)
{
	if (event_top_mark != event_bot_mark) {
		do {
			internalGetEvents();
		} while (event_top_mark != event_bot_mark && sys_event.type == SIM_MOUSE_MOVE);
	} else {
		sys_event.type    = SIM_NOEVENT;
		sys_event.code    = SIM_NOEVENT;
		sys_event.key_mod = recalc_keys();
		sys_event.mx      = mouse_x;
		sys_event.my      = mouse_y;
	}
}


void show_pointer(int yesno)
{
}


void ex_ord_update_mx_my(void)
{
	sys_event.mx = mouse_x;
	sys_event.my = mouse_y;
	sys_event.mb = mouse_b;
}


static volatile long milli_counter;

void timer_callback(void)
{
	milli_counter += 5;
}
END_OF_FUNCTION(timer_callback)


static void simtimer_init(void)
{
	printf("Installing timer...\n");

	LOCK_VARIABLE(milli_counter);
	LOCK_FUNCTION(timer_callback);

	printf("Preparing timer ...\n");

	install_timer();

	printf("Starting timer...\n");

	if (install_int(timer_callback, 5) == 0) {
		printf("Timer installed.\n");
	} else {
		printf("Error: Timer not available, aborting.\n");
		exit(1);
	}
}


unsigned long dr_time(void)
{
	return milli_counter;
}


void dr_sleep(uint32 usec)
{
	rest(usec);
}


bool dr_fatal_notify(const char* msg, int choices)
{
#ifdef _WIN32
	if(choices==0) {
		MessageBox( NULL, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_OK );
		return 0;
	}
	else {
		return MessageBox( NULL, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_RETRYCANCEL	)==IDRETRY;
	}
#else
//	beep();
	return choices;
#endif
}


int main(int argc, char **argv)
{
#ifdef _WIN32
	char pathname[1024];

	// prepare commandline
	GetModuleFileNameA( GetModuleHandle(NULL), pathname, 1024 );
	argv[0] = pathname;
#else
#ifndef __BEOS__
#	if defined __GLIBC__
	/* glibc has a non-standard extension */
	char* buffer2 = NULL;
#else
	char buffer2[PATH_MAX];
#endif
	char buffer[PATH_MAX];
	int length = readlink("/proc/self/exe", buffer, lengthof(buffer) - 1);
	if (length != -1) {
		buffer[length] = '\0'; /* readlink() does not NUL-terminate */
		argv[0] = buffer;
	}
	// no process file system => need to parse argv[0]
	/* should work on most unix or gnu systems */
	argv[0] = realpath (argv[0], buffer2);
#endif
#endif
	return simu_main(argc, argv);
}
END_OF_MAIN()
