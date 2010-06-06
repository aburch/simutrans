/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef WIN32
#define SDL
#endif

#ifdef SDL
#include <SDL.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <SDL_syswm.h>
#include <windows.h>
#else
#include <sys/stat.h>
#ifndef __HAIKU__
#include <sys/errno.h>
#else
#include <posix/errno.h>
#endif
#endif

#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

#undef min
#undef max

#include "macros.h"
#include "simmain.h"
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"
#include "simgraph.h"
#include "./dataobj/umgebung.h"

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

	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	atexit(SDL_Quit); // clean up on exit

	// Added by : Knightly
	// Note		: SDL will call timeBeginPeriod(1) even if we don't call SDL_GetTicks()
	//			  Thus, there is no benefit of using performance counter with SDL version
	umgebung_t::default_einstellungen.set_system_time_option(0); // reset to using multimedia timer

	return TRUE;
}


/* maximum size possible (if there) */
int dr_query_screen_width()
{
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL)>=1210
	const SDL_VideoInfo *vi=SDL_GetVideoInfo();
	return vi->current_w;
#else
#ifdef _WIN32
	return GetSystemMetrics( SM_CXSCREEN );
#else
	SDL_Rect **modi;
	modi = SDL_ListModes (NULL, SDL_FULLSCREEN );
	if (modi == NULL  ||  modi == (SDL_Rect**)-1  ) {
		return 704;
	}
	else {
		// return first
		return modi[0]->w;
	}
#endif
#endif
}



int dr_query_screen_height()
{
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL)>=1210
	const SDL_VideoInfo *vi=SDL_GetVideoInfo();
	return vi->current_h;
#else
#ifdef _WIN32
	return GetSystemMetrics( SM_CYSCREEN );
#else
	SDL_Rect **modi;
	modi = SDL_ListModes (NULL, SDL_FULLSCREEN );
	if (modi == NULL  ||  modi == (SDL_Rect**)-1  ) {
		return 704;
	}
	else {
		// return first
		return modi[0]->h;
	}
#endif
#endif
}




// open the window
int dr_os_open(int w, int h, int bpp, int fullscreen)
{
	Uint32 flags = sync_blit ? 0 : SDL_ASYNCBLIT;

	width = w;
	height = h;

	flags |= (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
#ifdef USE_HW
	{
		const SDL_VideoInfo *vi=SDL_GetVideoInfo();
		printf( "hw_available=%i, video_mem=%i, blit_sw=%i\n", vi->hw_available, vi->video_mem, vi->blit_sw );
		printf( "bpp=%i, bytes=%i\n", vi->vfmt->BitsPerPixel, vi->vfmt->BytesPerPixel );
	}
	flags |= SDL_HWSURFACE | SDL_DOUBLEBUF; // bltcopy in graphic memory should be faster ...
#endif

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
	SDL_WM_SetCaption("Simutrans " VERSION_NUMBER NARROW_EXPERIMENTAL_VERSION,NULL);
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
	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hHomeDir)==ERROR_SUCCESS) {
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
	ALLOCA(SDL_Color, rgb, count);
	int n;

	for (n = 0; n < count; n++) {
		rgb[n].r = *data++;
		rgb[n].g = *data++;
		rgb[n].b = *data++;
	}

	SDL_SetColors(screen, rgb, first, count);
}


void dr_prepare_flush()
{
	return;
}



void dr_flush(void)
{
	display_flush_buffer();
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




// try saving png using gdiplus.dll
extern "C" int dr_screenshot_png(const char *filename,  int w, int h, unsigned short *data, int bitdepth );

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
#ifdef WIN32
	if(dr_screenshot_png(filename, width, height, ( unsigned short *)(screen->pixels), screen->format->BitsPerPixel)) {
		return 1;
	}
#endif
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


static int conv_mouse_buttons(Uint8 const state)
{
	return
		(state & SDL_BUTTON_LMASK ? MOUSE_LEFTBUTTON  : 0) |
		(state & SDL_BUTTON_MMASK ? MOUSE_MIDBUTTON   : 0) |
		(state & SDL_BUTTON_RMASK ? MOUSE_RIGHTBUTTON : 0);
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
	}
	else {
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
					sys_event.mb   = conv_mouse_buttons(event.motion.state);
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
			sys_event.mb      = conv_mouse_buttons(SDL_GetMouseState(0, 0));
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
			sys_event.mb      = conv_mouse_buttons(SDL_GetMouseState(0, 0));
			switch (event.button.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTUP;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDUP;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTUP; break;
			}
			break;

		case SDL_KEYDOWN:
		{
			unsigned long code;
			bool   const  numlock = SDL_GetModState() & KMOD_NUM;
			SDLKey const  sym     = event.key.keysym.sym;
			switch (sym) {
				case SDLK_DELETE:   code = 127;                           break;
				case SDLK_DOWN:     code = SIM_KEY_DOWN;                  break;
				case SDLK_END:      code = SIM_KEY_END;                   break;
				case SDLK_HOME:     code = SIM_KEY_HOME;                  break;
				case SDLK_F1:       code = SIM_KEY_F1;                    break;
				case SDLK_F2:       code = SIM_KEY_F2;                    break;
				case SDLK_F3:       code = SIM_KEY_F3;                    break;
				case SDLK_F4:       code = SIM_KEY_F4;                    break;
				case SDLK_F5:       code = SIM_KEY_F5;                    break;
				case SDLK_F6:       code = SIM_KEY_F6;                    break;
				case SDLK_F7:       code = SIM_KEY_F7;                    break;
				case SDLK_F8:       code = SIM_KEY_F8;                    break;
				case SDLK_F9:       code = SIM_KEY_F9;                    break;
				case SDLK_F10:      code = SIM_KEY_F10;                   break;
				case SDLK_F11:      code = SIM_KEY_F11;                   break;
				case SDLK_F12:      code = SIM_KEY_F12;                   break;
				case SDLK_F13:      code = SIM_KEY_F13;                   break;
				case SDLK_F14:      code = SIM_KEY_F14;                   break;
				case SDLK_F15:      code = SIM_KEY_F15;                   break;
				case SDLK_KP0:      code = numlock ? '0' : 0;             break;
				case SDLK_KP1:      code = numlock ? '1' : SIM_KEY_END;   break;
				case SDLK_KP2:      code = numlock ? '2' : SIM_KEY_DOWN;  break;
				case SDLK_KP3:      code = numlock ? '3' : '<';           break;
				case SDLK_KP4:      code = numlock ? '4' : SIM_KEY_LEFT;  break;
				case SDLK_KP5:      code = numlock ? '5' : 0;             break;
				case SDLK_KP6:      code = numlock ? '6' : SIM_KEY_RIGHT; break;
				case SDLK_KP7:      code = numlock ? '7' : SIM_KEY_HOME;  break;
				case SDLK_KP8:      code = numlock ? '8' : SIM_KEY_UP;    break;
				case SDLK_KP9:      code = numlock ? '9' : '>';           break;
				case SDLK_LEFT:     code = SIM_KEY_LEFT;                  break;
				case SDLK_PAGEDOWN: code = '<';                           break;
				case SDLK_PAGEUP:   code = '>';                           break;
				case SDLK_RIGHT:    code = SIM_KEY_RIGHT;                 break;
				case SDLK_UP:       code = SIM_KEY_UP;                    break;

				default:
					if (event.key.keysym.unicode != 0) {
						code = event.key.keysym.unicode;
						if (event.key.keysym.unicode == 22 /* ^V */) {
#ifdef _WIN32
							// paste
							if (OpenClipboard(NULL)) {
								if (HANDLE const hText = GetClipboardData(CF_UNICODETEXT)) {
									if (WCHAR const* chr = static_cast<WCHAR const*>(GlobalLock(hText))) {
										SDL_Event new_event;
										new_event.type           = SDL_KEYDOWN;
										new_event.key.keysym.sym = SDLK_UNKNOWN;
										for (; *chr != '\0'; ++chr) {
											if (*chr == '\n') continue;
											new_event.key.keysym.unicode = *chr;
											SDL_PushEvent(&new_event);
										}
										GlobalUnlock(hText);
									}
								}
								CloseClipboard();
							}
#elif 0
							// X11 magic ... not tested yet!
							SDL_SysWMinfo si;
							if (SDL_GetWMInfo(&si) && si.subsystem == SDL_SYSWM_X11) {
								// clipboard under X11
								XEvent         evt;
								Atom           sseln   = XA_CLIPBOARD(si.x11.display);
								Atom           target  = XA_STRING;
								unsigned char* sel_buf = 0;
								unsigned long  sel_len = 0;	/* length of sel_buf */
								unsigned int   context = XCLIB_XCOUT_NONE;
								xcout(si.x11.display, si.x11.window, evt, sseln, target, &sel_buf, &sel_len, &context);
								/* fallback is needed. set XA_STRING to target and restart the loop. */
								if (context != XCLIB_XCOUT_FALLBACK) {
									// something in clipboard
									SDL_Event new_event;
									new_event.type           = SDL_KEYDOWN;
									new_event.key.keysym.sym = SDLK_UNKNOWN;
									for (unsigned char const* chr = sel_buf; sel_len-- >= 0; ++chr) {
										new_event.key.keysym.sym = *chr == '\n' ? '\r' : *chr;
										SDL_PushEvent(&new_event);
									}
									free(sel_buf);
								}
							}
#endif
						}
					} else if (0 < sym && sym < 127) {
						code = event.key.keysym.sym; // try with the ASCII code
					} else {
						code = 0;
					}
			}
			sys_event.type    = SIM_KEYBOARD;
			sys_event.code    = code;
			sys_event.key_mod = ModifierKeys();
			break;
		}

		case SDL_MOUSEMOTION:
			sys_event.type = SIM_MOUSE_MOVE;
			sys_event.code = SIM_MOUSE_MOVED;
			sys_event.mx   = event.motion.x;
			sys_event.my   = event.motion.y;
			sys_event.mb   = conv_mouse_buttons(event.motion.state);
			sys_event.key_mod = ModifierKeys();
			break;

		case SDL_ACTIVEEVENT:
		case SDL_KEYUP:
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			break;

		case SDL_QUIT:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_QUIT;
			break;

		default:
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


bool dr_fatal_notify(const char* msg, int choices)
{
#ifdef _WIN32
	if(choices==0) {
		MessageBoxA( NULL, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_OK );
		return 0;
	}
	else {
		return MessageBoxA( NULL, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_RETRYCANCEL	)==IDRETRY;
	}
#else
	fputs(msg, stderr);
	return choices;
#endif
}


#ifdef _WIN32
BOOL APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nShowCmd*/)
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
	char *argv[32], *p;
	int argc;
	char pathname[PATH_MAX];

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
#elif !defined __BEOS__
#	if defined __GLIBC__
	/* glibc has a non-standard extension */
	char* buffer2 = NULL;
#	else
	char buffer2[PATH_MAX];
#	endif
	char buffer[PATH_MAX];
	int length = readlink("/proc/self/exe", buffer, lengthof(buffer) - 1);
	if (length != -1) {
		buffer[length] = '\0'; /* readlink() does not NUL-terminate */
		argv[0] = buffer;
	}
	// no process file system => need to parse argv[0]
	/* should work on most unix or gnu systems */
	argv[0] = realpath(argv[0], buffer2);
#endif
	return simu_main(argc, argv);
}
#endif
