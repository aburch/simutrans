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

#ifdef _WIN32
#include "SDL_syswm.h"
#include <windows.h>
#define PATH_MAX (1024)
#else
#include <sys/stat.h>
#include <sys/errno.h>
#endif

#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "simmem.h"
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"

// try to use hardware double buffering ...
// this is equivalent on 16 bpp and much slower on 32 bpp
//#define USE_HW

static Uint8 hourglass_cursor[] = {
	0x3F, 0xFE, //   *************
	0x30, 0x06, //   **         **
	0x3F, 0xFE, //   *************
	0x10, 0x04, //    *         *
	0x10, 0x04, //    *         *
	0x12, 0xA4, //    *  * * *  *
	0x11, 0x44, //    *  * * *  *
	0x18, 0x8C, //    **   *   **
	0x0C, 0x18, //     **     **
	0x06, 0xB0, //      ** * **
	0x03, 0x60, //       ** **
	0x03, 0x60, //       **H**
	0x06, 0x30, //      ** * **
	0x0C, 0x98, //     **     **
	0x18, 0x0C, //    **   *   **
	0x10, 0x84, //    *    *    *
	0x11, 0x44, //    *   * *   *
	0x12, 0xA4, //    *  * * *  *
	0x15, 0x54, //    * * * * * *
	0x3F, 0xFE, //   *************
	0x30, 0x06, //   **         **
	0x3F, 0xFE  //   *************
};

static Uint8 hourglass_cursor_mask[] = {
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x0F, 0xF8, //     *********
	0x07, 0xF0, //      *******
	0x03, 0xE0, //       *****
	0x03, 0xE0, //       **H**
	0x07, 0xF0, //      *******
	0x0F, 0xF8, //     *********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x3F, 0xFE  //   *************
};


static SDL_Surface *screen;
static int width = 640;
static int height = 480;

// switch off is a little faster (<3%)
static int sync_blit = 0;

struct sys_event sys_event;

static SDL_Cursor* arrow;
static SDL_Cursor* hourglass;


/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
int dr_os_init(const int* parameter)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return FALSE;
	}

	sync_blit = parameter[1];

	atexit(SDL_Quit); // clean up on exit
	return TRUE;
}


// open the window
int dr_os_open(int w, int h, int bpp, int fullscreen)
{
  Uint32 flags = sync_blit ? 0 : SDL_ASYNCBLIT;

#ifdef USE_HW
	{
		SDL_VideoInfo *vi=SDL_GetVideoInfo();
		printf( "hw_available=%i, video_mem=%i, blit_sw=%i\n", vi->hw_available, vi->video_mem, vi->blit_sw );
		printf( "bpp=%i, bytes=%i\n", vi->vfmt->BitsPerPixel, vi->vfmt->BytesPerPixel );
	}
	flags |= SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE; // bltcopy in graphic memory should be faster ...
#endif

  width = w;
  height = h;

	flags |= (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);

  // open the window now
  screen = SDL_SetVideoMode(w, h, bpp, flags);
  if (screen == NULL) {
    fprintf(stderr, "Couldn't open the window: %s\n", SDL_GetError());
    return FALSE;
  } else {
    fprintf(stderr, "Screen Flags: requested=%x, actual=%x\n", flags, screen->flags);
  }

  SDL_EnableUNICODE(TRUE);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  // set the caption for the window
  SDL_WM_SetCaption("Simutrans " VERSION_NUMBER,NULL);
  SDL_ShowCursor(0);
  arrow = SDL_GetCursor();
  hourglass = SDL_CreateCursor(hourglass_cursor, hourglass_cursor_mask, 16, 22, 8, 11);

  return TRUE;
}


// shut down SDL
int dr_os_close(void)
{
	SDL_FreeCursor(hourglass);
	// Hajo: SDL doc says, screen is free'd by SDL_Quit and should not be
	// free'd by the user
	// SDL_FreeSurface(screen);
	return TRUE;
}


// reiszes screen
int dr_textur_resize(unsigned short** textur, int w, int h, int bpp)
{
#ifdef USE_HW
	SDL_UnlockSurface(screen);
#endif
	int flags = screen->flags;
	width = w;
	height = h;

	screen = SDL_SetVideoMode(width, height, bpp, flags);
	printf("textur_resize()::screen=%p\n", screen);
	fflush(NULL);
	*textur = (unsigned short*)screen->pixels;
	return 1;
}



// query home directory
char *dr_query_homedir(void)
{
	static char buffer[PATH_MAX];
	char b2[PATH_MAX];
#ifdef _WIN32
	DWORD len=PATH_MAX-24;
	HKEY hHomeDir;
	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ,	&hHomeDir)==ERROR_SUCCESS) {
		RegQueryValueExA(hHomeDir,"Personal",NULL,NULL,(LPCSTR)buffer,&len);
		strcat(buffer,"\\Simutrans");
		CreateDirectoryA( buffer, NULL );
		strcat(buffer, "\\");

		// create other subdirectories
		sprintf(b2, "%ssave", buffer );
		CreateDirectoryA( b2, NULL );
		sprintf(b2, "%sscreenshot", buffer );
		CreateDirectoryA( b2, NULL );

		return buffer;
	}
	return NULL;
#else
	sprintf( buffer, "%s/simutrans", getenv("HOME") );
	int err = mkdir( buffer, 0700 );
	if(err  &&  err!=EEXIST) {
		// could not create directory
		// we assume success anyway
	}
	strcat( buffer, "/" );
	sprintf( b2, "%sscreenshot", buffer );
	mkdir( b2, 0700 );
	sprintf( b2, "%ssave", buffer );
	mkdir( b2, 0700 );
	return buffer;
#endif
}



unsigned short *dr_textur_init()
{
#ifdef USE_HW
	SDL_LockSurface(screen);
#endif
	return (unsigned short*)screen->pixels;
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
	return SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
}


void dr_setRGB8multi(int first, int count, unsigned char* data)
{
#ifdef _MSC_VER
	SDL_Color *rgb = guarded_malloc(sizeof(*rgb) * count);
#else
	SDL_Color rgb[count];
#endif
	int n;

	for (n = 0; n < count; n++) {
		rgb[n].r = *data++;
		rgb[n].g = *data++;
		rgb[n].b = *data++;
	}

	SDL_SetColors(screen, rgb, first, count);
#ifdef _MSC_VER
	guarded_free(rgb);
#endif
}


void dr_flush(void)
{
#if 0
	SDL_UpdateRect(screen, 0, 0, width, height);
#endif
#ifdef USE_HW
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
	SDL_LockSurface(screen);
#endif
}


void dr_textur(int xp, int yp, int w, int h)
{
#ifndef USE_HW
	// make sure the given rectangle is completely on screen
	if (xp + w > screen->w) w = screen->w - xp;
	if (yp + h > screen->h) h = screen->h - yp;
	SDL_UpdateRect(screen, xp, yp, w, h);
#endif
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
	SDL_WarpMouse((Uint16)x, (Uint16)y);
}


// set the mouse cursor (pointer/load)
void set_pointer(int loading)
{
	SDL_SetCursor(loading ? hourglass : arrow);
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
	return SDL_SaveBMP(SDL_GetVideoSurface(), filename) == 0 ? 1 : -1;
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */


static inline unsigned int ModifierKeys(void)
{
	SDLMod mod = SDL_GetModState();

	return
		(mod & KMOD_SHIFT ? 1 : 0) |
		(mod & KMOD_CTRL  ? 2 : 0);
}


static void internal_GetEvents(int wait)
{
	SDL_Event event;
	event.type = 1;

	if (wait) {
		int n;

		do {
			SDL_WaitEvent(&event);
			n = SDL_PollEvent(NULL);
		} while (n != 0 && event.type == SDL_MOUSEMOTION);
	} else {
		int n;
		int got_one = FALSE;

		do {
			n = SDL_PollEvent(&event);

			if (n != 0) {
				got_one = TRUE;

				if (event.type == SDL_MOUSEMOTION) {
					sys_event.type = SIM_MOUSE_MOVE;
					sys_event.code = SIM_MOUSE_MOVED;
					sys_event.mx   = event.motion.x;
					sys_event.my   = event.motion.y;
				}
			}
		} while (n != 0 && event.type == SDL_MOUSEMOTION);

		if (!got_one) return;
	}

	switch (event.type) {
		case SDL_VIDEORESIZE:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_RESIZE;
			sys_event.mx   = event.resize.w;
			sys_event.my   = event.resize.h;
			printf("expose: x=%i, y=%i\n", sys_event.mx, sys_event.my);
			break;

		case SDL_VIDEOEXPOSE:
			// will be ignored ...
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_UPDATE;
			break;

		case SDL_MOUSEBUTTONDOWN:
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.key_mod = ModifierKeys();
			sys_event.mx      = event.button.x;
			sys_event.my      = event.button.y;
			switch (event.button.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTBUTTON;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDBUTTON;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTBUTTON; break;
				case 4: sys_event.code = SIM_MOUSE_WHEELUP;     break;
				case 5: sys_event.code = SIM_MOUSE_WHEELDOWN;   break;
			}
			break;

		case SDL_MOUSEBUTTONUP:
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.key_mod = ModifierKeys();
			sys_event.mx      = event.button.x;
			sys_event.my      = event.button.y;
			switch (event.button.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTUP;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDUP;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTUP; break;
			}
			break;

		case SDL_KEYDOWN:
			sys_event.type    = SIM_KEYBOARD;
			sys_event.key_mod = ModifierKeys();

			if (event.key.keysym.sym >= SDLK_KP0 && event.key.keysym.sym <= SDLK_KP9) {
				const int numlock = (SDL_GetModState() & KMOD_NUM) != 0;

				switch (event.key.keysym.sym) {
					case SDLK_KP0: sys_event.code = '0';                           break;
					case SDLK_KP1: sys_event.code = '1';                           break;
					case SDLK_KP2: sys_event.code = numlock ? '2' : SIM_KEY_DOWN;  break;
					case SDLK_KP3: sys_event.code = '3';                           break;
					case SDLK_KP4: sys_event.code = numlock ? '4' : SIM_KEY_LEFT;  break;
					case SDLK_KP5: sys_event.code = '5';                           break;
					case SDLK_KP6: sys_event.code = numlock ? '6' : SIM_KEY_RIGHT; break;
					case SDLK_KP7: sys_event.code = '7';                           break;
					case SDLK_KP8: sys_event.code = numlock ? '8' : SIM_KEY_UP;    break;
					case SDLK_KP9: sys_event.code = '9';                           break;
					default: break;
				}
			} else if (event.key.keysym.sym == SDLK_DELETE) {
				sys_event.code = 127;
			} else if (event.key.keysym.unicode > 0) {
				printf("Unicode ");
				sys_event.code = event.key.keysym.unicode;
			} else if (event.key.keysym.sym >= SDLK_F1 && event.key.keysym.sym <= SDLK_F15) {
				printf("Function ");
				sys_event.code = event.key.keysym.sym - SDLK_F1 + SIM_KEY_F1;
			} else if (event.key.keysym.sym > 0 && event.key.keysym.sym < 127) {
				printf("ASCII ");
				sys_event.code = event.key.keysym.sym;	// try with the ASCII code ...
			} else {
				switch (event.key.keysym.sym) {
					case SDLK_PAGEUP:   sys_event.code = '>';           break;
					case SDLK_PAGEDOWN: sys_event.code = '<';           break;
					case SDLK_HOME:     sys_event.code = SIM_KEY_HOME;  break;
					case SDLK_END:      sys_event.code = SIM_KEY_END;   break;
					case SDLK_DOWN:     sys_event.code = SIM_KEY_DOWN;  break;
					case SDLK_LEFT:     sys_event.code = SIM_KEY_LEFT;  break;
					case SDLK_RIGHT:    sys_event.code = SIM_KEY_RIGHT; break;
					case SDLK_UP:       sys_event.code = SIM_KEY_UP;    break;
					default:            sys_event.code = 0;             break;
				}
			}
			printf(
				"Event Key '%c' (%i) was generated\n",
				(int)sys_event.code, (int)sys_event.code
			);
			break;

		case SDL_MOUSEMOTION:
			sys_event.type = SIM_MOUSE_MOVE;
			sys_event.code = SIM_MOUSE_MOVED;
			sys_event.mx   = event.motion.x;
			sys_event.my   = event.motion.y;
			break;

		case 1:
		case SDL_KEYUP:
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			break;

		case SDL_QUIT:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_QUIT;
			break;

		default:
			printf("Unbekanntes Ereignis # %d!\n", event.type);
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;
	}
}


void GetEvents(void)
{
	internal_GetEvents(TRUE);
}


void GetEventsNoWait(void)
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	internal_GetEvents(FALSE);
}


void show_pointer(int yesno)
{
	SDL_ShowCursor(yesno != 0);
}


void ex_ord_update_mx_my()
{
	SDL_PumpEvents();
}


unsigned long dr_time(void)
{
	return SDL_GetTicks();
}


void dr_sleep(uint32 usec)
{
	SDL_Delay(usec);
}


int simu_main(int argc, char **argv);

#ifdef _WIN32
BOOL APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	char *argv[32], *p;
	int argc;
	char pathname[1024];

	// prepare commandline
	argc = 0;
	GetModuleFileNameA( hInstance, pathname, 1024 );
	argv[argc++] = pathname;
	p = strtok(lpCmdLine, " ");
	while (p != NULL) {
		argv[argc++] = p;
		p = strtok(NULL, " ");
	}
	argv[argc] = NULL;
#else
int main(int argc, char **argv)
{
	char buffer[1024];
	/* Read the target of /proc/self/exe. */
	if (readlink ("/proc/self/exe", buffer, PATH_MAX)>0) {
		argv[0] = buffer;
	}
	// no process file system => need to parse argv[0]
	/* should work on most unix or gnu systems */
	argv[0] = realpath (argv[0], buffer);
#endif
	return simu_main(argc, argv);
}
