/************************************************
 *    Schnittstelle zum Betriebssystem          *
 *    von Hj. Malthaner Aug. 1994               *
 ************************************************/

/* Angepasst an Simu/DOS
 * Juli 98
 * Hj. Malthaner
 */

/*
 * Angepasst an Allegro
 * April 2000
 * Hj. Malthaner
 */

#ifdef _MSC_VER
#   define AL_CONST    const
#   ifndef _WINDOWS
#      define USE_CONSOLE 1
#   endif
#endif

#include "allegro.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include "simsys.h"
#include "simmem.h"


#if defined(_MSC_VER) || defined(__MINGW32__)

#include <allegro/aintwin.h>

#else

#include <sys/time.h>
// Time at program start
static struct timeval start;

#endif





#define DIGMID_SUPPORT   // for Simutrans to work with DIGMID, define this

static void simtimer_init();
static int  simsound_init();

static int width = 640;
static int height = 480;

struct sys_event sys_event;


int sound_ok = 0;


/* event-handling */

/* queue Eintraege sind wie folgt aufgebaut
 * 4 integer werte
 *  type
 *  code
 *  mx
 *  my
 */

#define queue_length 4096


static volatile unsigned int event_top_mark = 0;
static volatile unsigned int event_bot_mark = 0;
static volatile int event_queue[queue_length*4 + 8];
#if defined(_MSC_VER) || defined(__MINGW32__)
CRITICAL_SECTION    EventQueue;
#endif


#if defined(_MSC_VER) || defined(__MINGW32__)
#define INSERT_EVENT(a, b, c, d) \
	EnterCriticalSection(&EventQueue);	\
	event_queue[event_top_mark++]=a; \
	event_queue[event_top_mark++]=b; \
	event_queue[event_top_mark++]=c; \
	event_queue[event_top_mark++]=d; \
	if(event_top_mark >= queue_length*4) event_top_mark = 0; \
	LeaveCriticalSection(&EventQueue);

#define	GET_AND_REMOVE_EVENT(event) \
	EnterCriticalSection(&EventQueue); \
	event.type = event_queue[event_bot_mark++]; \
	event.code = event_queue[event_bot_mark++];\
	event.mx   = event_queue[event_bot_mark++];	\
	event.my   = event_queue[event_bot_mark++];\
	if(event_bot_mark >= queue_length*4) event_bot_mark = 0; \
	LeaveCriticalSection(&EventQueue);

#else

#define INSERT_EVENT(a, b, c, d) \
	event_queue[event_top_mark++]=a; \
	event_queue[event_top_mark++]=b; \
	event_queue[event_top_mark++]=c; \
	event_queue[event_top_mark++]=d; \
	if(event_top_mark >= queue_length*4) event_top_mark = 0;

#define	GET_AND_REMOVE_EVENT(event) \
	event.type = event_queue[event_bot_mark++]; \
	event.code = event_queue[event_bot_mark++];\
	event.mx   = event_queue[event_bot_mark++];	\
	event.my   = event_queue[event_bot_mark++];\
	if(event_bot_mark >= queue_length*4) event_bot_mark = 0;
#endif


void my_mouse_callback(int flags)
{

//	printf("%x\n", flags);

    if(flags & MOUSE_FLAG_MOVE) {
        INSERT_EVENT( SIM_MOUSE_MOVE, SIM_MOUSE_MOVED, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_LEFT_DOWN) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTBUTTON, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_MIDDLE_DOWN) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDBUTTON, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_RIGHT_DOWN) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTBUTTON, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_LEFT_UP) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_LEFTUP, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_MIDDLE_UP) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_MIDUP, mouse_x, mouse_y)
    }

    if(flags & MOUSE_FLAG_RIGHT_UP) {
        INSERT_EVENT( SIM_MOUSE_BUTTONS, SIM_MOUSE_RIGHTUP, mouse_x, mouse_y)
    }
}

END_OF_FUNCTION(my_mouse_callback);


int my_keyboard_callback(int key)
{
    // printf("%x %x\n", key >> 8, KEY_F1);

    // test keycode
    switch(key >> 8) {
    case KEY_F1:
	key = SIM_F1;
	break;
    default:
	// use ASCII code
	key &= 255;
    }


    INSERT_EVENT(SIM_KEYBOARD, key, mouse_x, mouse_y);

    return 0;
}

END_OF_FUNCTION(my_keyboard_callback);

/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */

int dr_os_init(int n, int *parameter)
{

        int ok = allegro_init();

        LOCK_VARIABLE(event_top_mark);
        LOCK_VARIABLE(event_bot_mark);
        LOCK_VARIABLE(event_queue);
        LOCK_FUNCTION(my_mouse_callback);
        LOCK_FUNCTION(my_keyboard_callback);
#if defined(_MSC_VER) || defined(__MINGW32__)
	InitializeCriticalSection(&EventQueue);
#endif

        if(ok == 0) {
                simtimer_init();
                simsound_init();
        }


        return ok == 0;
}

int dr_os_open(int w, int h)
{
        int ok;

	width = w;
	height = h;

        set_color_depth( 8 );

        install_keyboard();

        ok = set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0);
        if(ok != 0) {
                fprintf(stderr, "Error: %s\n", allegro_error);
                return FALSE;
        }

        ok = install_mouse();
        if(ok < 0) {
                fprintf(stderr, "Cannot init. mouse: no driver ?");
                return FALSE;
        }

        set_mouse_speed(1,1);

        mouse_callback = my_mouse_callback;
        keyboard_callback = my_keyboard_callback;

        sys_event.mx = mouse_x;
        sys_event.my = mouse_y;


	set_window_title("Simutrans");

        return TRUE;
}


int dr_os_close(void)
{
        allegro_exit();
#if defined(_MSC_VER) || defined(__MINGW32__)
	DeleteCriticalSection(&EventQueue);
#endif
        return TRUE;
}

/*
 * Hier beginnen die eigentlichen graphischen Funktionen
 */

static unsigned char *tex;

static BITMAP *texture_map;

unsigned char *
dr_textur_init()
{
        int i;
	tex = (unsigned char *) guarded_malloc(width*height);

        texture_map = create_bitmap(width, height);

        for(i=0; i<height; i++) {
                texture_map->line[i] = tex + width*i;
        }


        return tex;
}


/**
 * Does this system wrapper need software cursor?
 * @return true if a software cursor is needed
 * @author Hj. Malthaner
 */
int dr_use_softpointer()
{
    return 1;
}


void
dr_textur(int xp, int yp, int w, int h)
{
        blit(texture_map, screen, xp, yp, xp, yp, w, h);
}

void dr_flush()
{
        // aalegro doesn't use flush
}


void dr_setRGB8(int n, int r, int g, int b)
{
        struct RGB rgb;
        rgb.r = r >> 2;
        rgb.g = g >> 2;
        rgb.b = b >> 2;

        set_color(n, &rgb);
}

void dr_setRGB8multi(int first, int count, unsigned char * data)
{
    PALETTE p;
    int n;

    for(n=0; n<count; n++) {
	p[n+first].r = data[n*3+0] >> 2;
	p[n+first].g = data[n*3+1] >> 2;
	p[n+first].b = data[n*3+2] >> 2;
    }

    set_palette_range(p, first, first+count-1, TRUE);
}


void move_pointer(int x, int y)
{
        position_mouse(x,y);
}

/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */


void internalGetEvents()
{
	if(event_top_mark != event_bot_mark) {
		GET_AND_REMOVE_EVENT(sys_event);
	} else {
                sys_event.type = SIM_NOEVENT;
                sys_event.code = SIM_NOEVENT;

                sys_event.mx = mouse_x;
                sys_event.my = mouse_y;
        }
}


void GetEvents()
{
        while(event_top_mark == event_bot_mark) {
// try to be nice where possible
// MingW32 lacks usleep
#ifndef __MINGW32__
#   ifndef _MSC_VER
		usleep(1000);
#   else
		Sleep(1000);
#   endif
#endif
        }

        do {
                 internalGetEvents();
        } while(sys_event.type == SIM_MOUSE_MOVE);
}

void GetEventsNoWait()
{
        if(event_top_mark != event_bot_mark) {
                do {
                        internalGetEvents();
                } while(event_top_mark != event_bot_mark && sys_event.type == SIM_MOUSE_MOVE);
        } else {
                sys_event.type = SIM_NOEVENT;
                sys_event.code  = SIM_NOEVENT;

                sys_event.mx = mouse_x;
                sys_event.my = mouse_y;
        }
}

void show_pointer(int yesno)
{
}

void ex_ord_update_mx_my()
{
    sys_event.mx = mouse_x;
    sys_event.my = mouse_y;

    /*
        while(event_top_mark != event_bot_mark &&
              event_queue[event_bot_mark] == SIM_MOUSE_MOVE)
        {
                internalGetEvents();
        }

    */
}


// ----------- timer funktionen -------------



static volatile long milli_counter;

void timer_callback(void)
{
        milli_counter += 5;
}

#ifdef END_OF_FUNCTION
END_OF_FUNCTION(timer_callback);
#endif

static void
simtimer_init()
{
#if defined(_MSC_VER) || defined(__MINGW32__)

    int ok;

    printf("Installing timer...\n");

    LOCK_VARIABLE( milli_counter );
    LOCK_FUNCTION(timer_callback);

    printf("Preparing timer ...\n");

    install_timer();

    printf("Starting timer...\n");

    ok = install_int(timer_callback, 5);

    if(ok == 0) {
	printf("Timer installed.\n");
    } else {
	printf("Error: Timer not available, aborting.\n");
	exit(1);
    }

#else

    // init time count
    gettimeofday(&start, NULL);

#endif
}


unsigned long dr_time()
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    const long tmp = milli_counter;
    return tmp;
#else

  struct timeval now;
  unsigned long time;

  gettimeofday(&now, NULL);
  time=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
  return time;

#endif
}


void dr_sleep(unsigned long usec)
{
//#if defined(_MSC_VER) || defined(__MINGW32__)
    // benutze Allegro
    if(usec > 1023) {
	// schlaeft meist etwas zu kurz,
        // usec/1024 statt usec/1000
	rest(usec >> 10);
    }
//#else
//    usleep( usec );
//#endif
}


// brutal busy wait -> very precise!
/*
void dr_sleep(unsigned long usec)
{
    const unsigned long end = dr_time()+(usec>>10);

    do {
    } while(dr_time()<end);
}
*/


static int sample_number = 0;
static SAMPLE * sound_samples[1024];

static int voice[4];
static int vc;



/* MIDI: Owen Rudge */

static int midi_number = -1;

#define MAX_MIDI 30

MIDI *midi_samples[MAX_MIDI];

/**
 * initializes sound device
 * @author Hj. Malthaner
 */
int simsound_init()
{
        int voices = detect_digi_driver(DIGI_AUTODETECT);

        if(voices > 0) {
                int ok;

                // assume: all ok, override on error
                sound_ok = 1;

                fprintf(stderr, "Message: Sound driver successfully detected.\n");

#ifndef DIGMID_SUPPORT
                reserve_voices(MIN(voices,4), 0);
#endif

                ok = install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL); /***** OCR *****/

                if(ok == -1) {
                        fprintf(stderr, "Warning: %s\n", allegro_error);
                        fprintf(stderr, "Warning: shutting down sound support\n");
                        sound_ok = 0;
                } else {

               	}

               	voice[0] = -1;
               	voice[1] = -1;
               	voice[2] = -1;
               	voice[3] = -1;

                set_volume(255, 128);
        } else {
                fprintf(stderr, "Warning: No sound available!\n");
                sound_ok = 0;
        }

        return TRUE;
}

/*
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{
        if(sound_ok) {
                sound_samples[sample_number] = load_sample(filename);

                lock_sample(sound_samples[sample_number]);
        }
        return sample_number++;
}

/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume)
{
//        fprintf(stderr, "playing sound %d\n", key);



        if(sound_ok && key < 1024 && sound_samples[key] != NULL) {
/*
                play_sample(sound_samples[key],
                            volume,
                            128,
                            1000,
                            0);
*/

		int v = voice[vc & 3];

		if(v == -1) {
			// noch nie benutzt

			voice[vc & 3] = v = allocate_voice(sound_samples[key]);
		} else {
			voice_stop( v );
		}

		reallocate_voice(v, sound_samples[key]);
      		voice_set_volume(v, volume);
      		voice_set_pan(v, 128);
      		voice_set_frequency(v, sound_samples[key]->freq);
      		voice_set_playmode(v, PLAYMODE_PLAY);
      		voice_start(v);

		vc ++;
	}
}


/**
 * sets midi playback volume
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
    set_volume(-1, vol);
}


/**
 * Loads a MIDI file
 * @author Owen Rudge, changes by Hj. Malthaner
 */

int dr_load_midi(const char *filename)
{
    if(sound_ok && midi_number < MAX_MIDI-1)
    {
	const int i = midi_number + 1;

	if(i >= 0 && i < MAX_MIDI) {

	    midi_samples[i] = load_midi(filename);

	    if(midi_samples[i]) {
		midi_number = i;
	    }
	}
    }

    return midi_number;
}



/**
 * Plays a MIDI file
 * Key: The index of the MIDI file to be played
 * By Owen Rudge
 */

void dr_play_midi(int key)
{
   if (midi_number > 0)
   {
      if(sound_ok && key >= 0 && key <= midi_number && midi_samples[key] != NULL)
         play_midi(midi_samples[key], FALSE); // loop
      else
	printf("\nMessage: MIDI: Unable to play MIDI %d\n", key);
   }
}



/**
 * Stops playing MIDI file
 * By Owen Rudge
 */
void dr_stop_midi()
{
   stop_midi();
}

/**
 * Returns the midi_pos variable
 * By Owen Rudge
 */
long dr_midi_pos()
{
    if(sound_ok) {
	return (midi_pos);
    } else {
	return 0;
    }
}

/**
 * Midi shutdown/cleanup
 * By Owen Rudge
 */
void dr_destroy_midi()
{
   int i;

   for (i = 0; i <= midi_number; i++) {
      if (midi_samples[i] != NULL) {
	destroy_midi(midi_samples[i]);
      }
   }
   midi_number = -1;
}


void set_midi_pos(int pos)
{
   midi_pos = pos;
}


int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return simu_main(argc, argv);
}

END_OF_MAIN();
