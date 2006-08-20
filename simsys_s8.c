/*
 * simsys_s.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <SDL.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "simsys.h"
#include "simmem.h"


static SDL_Surface *screen;
static int width = 640;
static int height = 480;

struct sys_event sys_event;


/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */

// open libraries, allocate memory...
int dr_os_init(int n, int *parameter)
{
        // initialize SDL
        int ok = SDL_Init(SDL_INIT_VIDEO||);
        if(ok != 0) {
                printf("Couldn't initialize SDL: %s\n", SDL_GetError());
                return FALSE;
        }

        // if SDL gets initialized normally, return zero
        atexit(SDL_Quit); // clean up on exit
        return ok == 0;
}

// open the window
int dr_os_open(int w, int h)
{

        width = w;
        height = h;

        // open the window now
        screen = SDL_SetVideoMode(w, h, 8, SDL_HWPALETTE);
        if (screen == NULL) {
                printf("Couldn't open the window: %s\n", SDL_GetError());
                return FALSE;
        }

        if (screen->pitch != w) {
            printf("!!!!!!!!!!!!!!!!!warnign, pitch != width\n");
        }

	SDL_EnableUNICODE(TRUE);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

        // set the caption for the window
        SDL_WM_SetCaption("Simutrans",NULL);
        SDL_ShowCursor(0);

        return TRUE;
}

// shut down SDL
int dr_os_close(void)
{
        SDL_FreeSurface(screen);
        return TRUE;
}


unsigned char *dr_textur_init()
{
        return (unsigned char *) (screen->pixels);
}


void dr_flush()
{
//    SDL_UpdateRect(screen, 0, 0, width, height);
}


void
dr_textur(int xp, int yp, int w, int h)
{
    // make sure the given rectangle is completely on screen
    if ((xp + w) > screen->w) w = screen->w-xp;
    if ((yp + h) > screen->h) h = screen->h-yp;
    SDL_UpdateRect(screen, xp, yp, w, h);
}


void dr_setRGB8(int n, int r, int g, int b)
{
        SDL_Color rgb;
        rgb.r = r;
        rgb.g = g;
        rgb.b = b;

        SDL_SetColors(screen, &rgb, n, 1);
}


void dr_setRGB8multi(int first, int count, unsigned char * data)
{
#ifdef _MSC_VER
    SDL_Color *rgb = guarded_malloc(sizeof(SDL_Color) * count);
#else
    SDL_Color rgb[count];
#endif
    int n;

    for(n=0; n<count; n++) {
	rgb[n].r = *data++;
	rgb[n].g = *data++;
	rgb[n].b = *data++;
    }

    SDL_SetColors(screen, rgb, first, count);
#ifdef _MSC_VER
    guarded_free(rgb);
#endif
}


/*
 * move cursor to the specified location
 */
void move_pointer(int x, int y)
{
        SDL_WarpMouse(x,y);
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */
static void internal_GetEvents(int wait)
{
    SDL_Event event;
    event.type = 1;

    if(wait) {
        int n = 0;

        do {
            SDL_WaitEvent(&event);
            n = SDL_PollEvent(NULL);
        }  while (n != 0 && event.type==SDL_MOUSEMOTION);

    } else {
        int n = 0;
        int got_one = FALSE;

        do {
            n = SDL_PollEvent(&event);

	    if(n != 0) {
		got_one = TRUE;

	    	if(event.type == SDL_MOUSEMOTION) {
        		sys_event.type=SIM_MOUSE_MOVE;
        		sys_event.code= SIM_MOUSE_MOVED;
        		sys_event.mx=event.motion.x;
        		sys_event.my=event.motion.y;
	    	}
	    }

	} while (n != 0 && event.type==SDL_MOUSEMOTION);

        if(!got_one) {
            return;
        }
    }


    switch(event.type) {
    case SDL_MOUSEBUTTONDOWN:     /* originally ButtonPress */
        sys_event.type=SIM_MOUSE_BUTTONS;
        sys_event.mx=event.button.x;
        sys_event.my=event.button.y;
        switch(event.button.button) {
        case 1:
            sys_event.code=SIM_MOUSE_LEFTBUTTON;
            break;
        case 2:
            sys_event.code=SIM_MOUSE_MIDBUTTON;
            break;
        case 3:
            sys_event.code=SIM_MOUSE_RIGHTBUTTON;
            break;
        }
        break;
   case SDL_MOUSEBUTTONUP:     /* originally ButtonRelease */
        sys_event.type=SIM_MOUSE_BUTTONS;
        sys_event.mx=event.button.x;
        sys_event.my=event.button.y;
        switch(event.button.button) {
        case 1:
            sys_event.code=SIM_MOUSE_LEFTUP;
            break;
        case 2:
            sys_event.code=SIM_MOUSE_MIDUP;
            break;
        case 3:
            sys_event.code=SIM_MOUSE_RIGHTUP;
            break;
        }
        break;
    case SDL_KEYDOWN:   /* originally KeyPress */
        sys_event.type=SIM_KEYBOARD;

	if(event.key.keysym.sym >= 32) {
	    sys_event.code=event.key.keysym.unicode;
	} else {
	    sys_event.code=event.key.keysym.sym;
	}

        printf("Key '%c' (%d) was pressed\n", sys_event.code, sys_event.code);

        break;
    case SDL_MOUSEMOTION:    /* originally MotionNotify */
        sys_event.type=SIM_MOUSE_MOVE;
        sys_event.code= SIM_MOUSE_MOVED;
        sys_event.mx=event.motion.x;
        sys_event.my=event.motion.y;
        break;
    case 15:
    case 14:
    case SDL_KEYUP: /* originally KeyRelease */
        sys_event.type=SIM_IGNORE_EVENT;
        sys_event.code=0;
        break;
    default:
        printf("Unbekanntes Ereignis # %d!\n",event.type);
        sys_event.type=SIM_IGNORE_EVENT;
        sys_event.code=0;
    }
}

void GetEvents()
{
    internal_GetEvents(TRUE);
}

void GetEventsNoWait()
{
    sys_event.type = SIM_NOEVENT;
    sys_event.code = 0;

    internal_GetEvents(FALSE);
}

// I added this because it's needed by simgraph.c
void show_pointer(int yesno)
{
    if (yesno)
	SDL_ShowCursor(1);
    else
	SDL_ShowCursor(0);
}


void ex_ord_update_mx_my()
{
        // this procedure is left empty for the SDL version
}


unsigned long dr_time(void)
{
  return SDL_GetTicks();
}


void dr_sleep(unsigned long usec)
{
    if(usec >= 1024) {
	// schlaeft meist etwas zu kurz,
        // usec/1024 statt usec/1000
        SDL_Delay(usec >> 10);
    }
}


int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return simu_main(argc, argv);
}
