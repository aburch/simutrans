#include "allegro.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include "simsys.h"
#include "simevent.h"
#include "simmem.h"


static void simtimer_init(void);

static int width  = 640;
static int height = 480;

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
static volatile int event_queue[queue_length * 4 + 8];


#define INSERT_EVENT(a, b, c, d) \
	event_queue[event_top_mark++] = a; \
	event_queue[event_top_mark++] = b; \
	event_queue[event_top_mark++] = c; \
	event_queue[event_top_mark++] = d; \
	if (event_top_mark >= queue_length * 4) event_top_mark = 0;


void my_mouse_callback(int flags)
{
	if (flags & MOUSE_FLAG_MOVE) {
		INSERT_EVENT(SIM_MOUSE_MOVE, SIM_MOUSE_MOVED, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_LEFT_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTBUTTON, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_MIDDLE_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDBUTTON, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_RIGHT_DOWN) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTBUTTON, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_LEFT_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTUP, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_MIDDLE_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDUP, mouse_x, mouse_y)
	}

	if (flags & MOUSE_FLAG_RIGHT_UP) {
		INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTUP, mouse_x, mouse_y)
	}
}
END_OF_FUNCTION(my_mouse_callback)


static int my_keyboard_callback(int this_key, int* scancode)
{
	if (this_key > 0) {
		// seems to be ASCII
		INSERT_EVENT(SIM_KEYBOARD, this_key, mouse_x, mouse_y);
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

	INSERT_EVENT(SIM_KEYBOARD, this_key, mouse_x, mouse_y);
	*scancode = 0;
	return 0;
}
END_OF_FUNCTION(my_keyboard_callback)


/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */


int dr_os_init(const int* parameter)
{
	int ok = allegro_init();

	LOCK_VARIABLE(event_top_mark);
	LOCK_VARIABLE(event_bot_mark);
	LOCK_VARIABLE(event_queue);
	LOCK_FUNCTION(my_mouse_callback);
	LOCK_FUNCTION(my_keyboard_callback);

	if (ok == 0) simtimer_init();

	return ok == 0;
}


int dr_os_open(int w, int h, int fullscreen)
{
	width = w;
	height = h;

	set_color_depth(16);

	install_keyboard();

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

	return texture_map->line[0];
}


// reiszes screen (Not allowed)
int dr_textur_resize(unsigned short** textur, int w, int h)
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


void dr_flush(void)
{
	// allegro doesn't use flush
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
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
		sys_event.key_mod = recalc_keys();

		if (event_bot_mark >= queue_length * 4) event_bot_mark = 0;
	} else {
		sys_event.type = SIM_NOEVENT;
		sys_event.code = SIM_NOEVENT;
		sys_event.mx   = mouse_x;
		sys_event.my   = mouse_y;
	}
}


void GetEvents(void)
{
	while (event_top_mark == event_bot_mark) {
		// try to be nice where possible
#if !defined(__MINGW32__) && !defined(__BEOS__)
		usleep(1000);
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


void dr_sleep(unsigned long usec)
{
	if (usec > 1023) {
		// schlaeft meist etwas zu kurz,
		// usec/1024 statt usec/1000
		rest(usec >> 10);
	}
}


int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
	return simu_main(argc, argv);
}
END_OF_MAIN()
