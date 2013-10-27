/*
 * Bit depth independent backend using allgro (mainly for BeOS)
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#include <stdlib.h>
#include <stdio.h>

#include "macros.h"
#include "simsys.h"
#include "simevent.h"
#include "display/simgraph.h"
#include "simsys_w32_png.h"
#include "simversion.h"

#include <allegro.h>


static void simtimer_init();

static int width  = 800;
static int height = 600;


/* event-handling */

/* queue Eintraege sind wie folgt aufgebaut
 * 4 integer werte
 *  class
 *  code
 *  mx
 *  my
 */

#define queue_length 4096
#define queue_items 6


static volatile unsigned int event_top_mark = 0;
static volatile unsigned int event_bot_mark = 0;
static volatile int event_queue[queue_length * queue_items + 8];


#define INSERT_EVENT(a, b) \
	event_queue[event_top_mark++] = a; \
	event_queue[event_top_mark++] = b; \
	event_queue[event_top_mark++] = mouse_x; \
	event_queue[event_top_mark++] = mouse_y; \
	event_queue[event_top_mark++] = mouse_b; \
	event_queue[event_top_mark++] = mouse_z; \
	if (event_top_mark >= queue_length * queue_items) event_top_mark = 0;


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

	if (flags & MOUSE_FLAG_MOVE_Z) {
	    if(event_top_mark > 0) {
	        if (event_queue[event_top_mark-1] < mouse_z) {
	            INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_WHEELUP)
            }
	        else if (event_queue[event_top_mark-1] > mouse_z) {
	            INSERT_EVENT(SIM_MOUSE_BUTTONS, SIM_MOUSE_WHEELDOWN)
            }
	    }
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
		case KEY_PAUSE:     this_key =  16; break;

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



void my_close_button_callback()
{
	INSERT_EVENT(SIM_SYSTEM, SYSTEM_QUIT)
}
END_OF_FUNCTION(my_close_button_callback)



/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */


bool dr_os_init(int const* parameter)
{
	if (allegro_init() != 0) {
		dr_fatal_notify("Could not init Allegro.\n");
		return false;
	}

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

	simtimer_init();

	return true;
}


resolution dr_query_screen_resolution()
{
	resolution res;
	if (get_desktop_resolution(&res.w, &res.h) != 0) {
		res.w = width;
		res.h = height;
	}
	return res;
}


int dr_os_open(int const w, int const h, int const fullscreen)
{
	width = w;
	height = h;


	install_keyboard();

	set_color_depth(COLOUR_DEPTH);
	if (set_gfx_mode(fullscreen? GFX_AUTODETECT : GFX_AUTODETECT_WINDOWED, w, h, 0, 0) != 0) {
		fprintf(stderr, "Error: %s\n", allegro_error);
		return 0;
	}

	if (install_mouse() < 0) {
		fprintf(stderr, "Cannot init. mouse: no driver ?");
		return 0;
	}

	set_mouse_speed(1, 1);

	mouse_callback = my_mouse_callback;
	keyboard_ucallback = my_keyboard_callback;

	sys_event.mx = mouse_x;
	sys_event.my = mouse_y;

	set_window_title(SIM_TITLE);

	return w;
}


void dr_os_close()
{
	allegro_exit();
}


/*
 * Hier beginnen die eigentlichen graphischen Funktionen
 */

static BITMAP* texture_map;

unsigned short* dr_textur_init()
{
	texture_map = create_bitmap(width, height);
	if (texture_map == NULL) {
		printf("Error: can't create double buffer bitmap, aborting!");
		exit(1);
	}

//	return reinterpret_cast<unsigned short*>(texture_map->line[0]);
	return (unsigned short *)(texture_map->line[0]);
}


// resizes screen (Not allowed)
int dr_textur_resize(unsigned short**, int, int)
{
	display_set_actual_width( width );
	return width;
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


void dr_prepare_flush()
{
	return;
}



void dr_flush()
{
	display_flush_buffer();
}

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename, int x, int y, int w, int h)
{
#ifdef WIN32
	if(  dr_screenshot_png(filename, max(w,width-1), h, width, ((short unsigned int *)texture_map)+x+y*width, 16)  ) {
		return 1;
	}
#endif
	PALETTE pal;
	get_palette(pal);
	return save_bitmap(filename, texture_map, pal) == 0 ? 1 : -1;
}


void move_pointer(int x, int y)
{
	position_mouse(x, y);
}


void set_pointer(int loading)
{
	// not supported
}


static inline int recalc_keys()
{
	int mod = key_shifts;

	return
		(mod & KB_SHIFT_FLAG ? 1 : 0) |
		(mod & KB_CTRL_FLAG  ? 2 : 0);
}



static void internalGetEvents()
{
	if (event_top_mark != event_bot_mark) {
		sys_event.type    = event_queue[event_bot_mark++];
		sys_event.code    = event_queue[event_bot_mark++];
		sys_event.mx      = event_queue[event_bot_mark++];
		sys_event.my      = event_queue[event_bot_mark++];
		sys_event.mb      = event_queue[event_bot_mark++];
		event_bot_mark++;   // jump over mouse_z
		sys_event.key_mod = recalc_keys();

		if (event_bot_mark >= queue_length * queue_items) {
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


void GetEvents()
{
	while (event_top_mark == event_bot_mark) {
		// try to be nice where possible
		rest(1);
	}

	do {
		internalGetEvents();
	} while(sys_event.type == SIM_MOUSE_MOVE);
}


void GetEventsNoWait()
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


void ex_ord_update_mx_my()
{
	sys_event.mx = mouse_x;
	sys_event.my = mouse_y;
	sys_event.mb = mouse_b;
}


static volatile long milli_counter;

void timer_callback()
{
	milli_counter += 5;
}
END_OF_FUNCTION(timer_callback)

static void simtimer_init()
{
	printf("Installing timer...\n");

	LOCK_VARIABLE(milli_counter);
	LOCK_FUNCTION(timer_callback);

	printf("Preparing timer ...\n");

	install_timer();

	printf("Starting timer...\n");

	if (install_int(timer_callback, 5) == 0) {
		printf("Timer installed.\n");
	}
	else {
		dr_fatal_notify("Error: Timer not available, aborting.\n");
		exit(1);
	}
}


unsigned long dr_time()
{
	return milli_counter;
}


void dr_sleep(uint32 usec)
{
	rest(usec);
}


int main(int argc, char **argv)
{
	return sysmain(argc, argv);
}
END_OF_MAIN()
