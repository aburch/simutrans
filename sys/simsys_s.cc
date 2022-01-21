/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>

#include "simsys.h"

#include "../macros.h"
#include "../simversion.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../simdebug.h"


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

static Uint8 blank_cursor[] = {
	0x0,
	0x0,
};

static SDL_Surface *screen;
static int width = 16;
static int height = 16;
static sint16 fullscreen = WINDOWED;

// switch on is a little faster (<3%)
static int async_blit = 0;

// try to use hardware double buffering ...
static int use_hw = 0;

static SDL_Cursor* arrow;
static SDL_Cursor* hourglass;
static SDL_Cursor* blank;

#ifdef MULTI_THREAD
#include "../utils/simthread.h"

static simthread_barrier_t redraw_barrier;
static pthread_mutex_t redraw_mutex = PTHREAD_MUTEX_INITIALIZER;

// parameters passed starting a thread
typedef struct{
	pthread_t thread;
	bool ready;
} redraw_param_t;
static redraw_param_t redraw_param;


void* redraw_thread( void* ptr )
{
	while(true) {
		simthread_barrier_wait( &redraw_barrier ); // wait to start
		pthread_mutex_lock( &redraw_mutex );
		if ( ((redraw_param_t*)ptr)->ready ) {
			pthread_mutex_unlock( &redraw_mutex );
			break;
		}
		display_flush_buffer();
		if(  use_hw  ) {
			SDL_UnlockSurface( screen );
			SDL_Flip( screen );
			SDL_LockSurface( screen );
		}
		pthread_mutex_unlock( &redraw_mutex );
	}
	return ptr;
}
#else
// SDL_Rects so SDL_UpdateRects can be used instead of SDL_UpdateRect when single threaded. (Update Rects is slower when multithreaded)
#define MAX_SDL_RECTS 2048
static int num_SDL_Rects = 0;
static SDL_Rect SDL_Rects[MAX_SDL_RECTS];
#endif

// When using -autodpi, attempt to scale things on screen to this DPI value
#define TARGET_DPI (96)

#define SCALE_SHIFT_X 5
#define SCALE_SHIFT_Y 5

#define SCALE_NEUTRAL_X (1 << SCALE_SHIFT_X)
#define SCALE_NEUTRAL_Y (1 << SCALE_SHIFT_Y)

sint32 x_scale = SCALE_NEUTRAL_X;
sint32 y_scale = SCALE_NEUTRAL_Y;

#define SCREEN_RESCALE_X(x) (((x) * SCALE_NEUTRAL_X) / x_scale)
#define SCREEN_RESCALE_Y(y) (((y) * SCALE_NEUTRAL_Y) / y_scale)

// no autoscaling yet
bool dr_set_screen_scale(sint16 /*scale_percent*/)
{
#ifdef __ANDROID__
	// SDL 1.2 does not support scaling, but the libSDL Android rendering layer
	// can stretch a rendered screensize to fit the full screen.
	// Hence zoom scaling is achieved by rendering a lower resolution;
	// dr_query_screen_resolution will therefore return a smaller resolution.

	// SDL 1.2 does not provide device dpi retrieval; this number is based
	// on average dpi of medium-end smartphones on market
	const int DEVICE_DPI = 400;

	// Touch screen UX best practice recommends button size of 42px at dpi 96 (~11 mm)
	// non-large themes have button size of 32px at dpi 96
	// large-theme have button size of 48px at dpi 96
	// However, default target DPI of 96 is fine for computer screen,
	// but not for mobile devices. A target dpi of 96 with a 6 inch screen
	// does not give enough resolution to display information, hence we must
	// target a higher dpi such as 192 to have enough screen estate, and
	// reach playable resolution. At 192 dpi, buttons are best in the
	// range 64-96 px so a new theme is preferrable.
	const int MOBILE_TARGET_DPI = TARGET_DPI * 2;

	x_scale = SCALE_NEUTRAL_X * DEVICE_DPI / MOBILE_TARGET_DPI;
	y_scale = SCALE_NEUTRAL_Y * DEVICE_DPI / MOBILE_TARGET_DPI;
	return false;
#else
	x_scale = SCALE_NEUTRAL_X;
	y_scale = SCALE_NEUTRAL_Y;
	return false;
#endif
}


sint16 dr_get_screen_scale()
{
	return 100;
}


/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
bool dr_os_init(const int* parameter)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0) {
		dbg->error("dr_os_init(SDL)", "Couldn't initialize SDL: %s", SDL_GetError());
		return false;
	}

	async_blit = parameter[0];
#ifdef USE_HW
	// force if old compile directive set
	use_hw = 1;
#else
	use_hw = parameter[1];
#endif
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	atexit(SDL_Quit); // clean up on exit
	return true;
}


resolution dr_query_screen_resolution()
{
	resolution res;
#if SDL_VERSION_ATLEAST(1, 2, 10)
	SDL_VideoInfo const& vi = *SDL_GetVideoInfo();
	res.w = vi.current_w;
	res.h = vi.current_h;
#elif defined _WIN32
	res.w = GetSystemMetrics(SM_CXSCREEN);
	res.h = GetSystemMetrics(SM_CYSCREEN);
#else
	SDL_Rect** const modi = SDL_ListModes(0, SDL_FULLSCREEN);
	if (modi && modi != (SDL_Rect**)-1) {
		// return first
		res.w = modi[0]->w;
		res.h = modi[0]->h;
	} else {
		res.w = 704;
		res.h = 560;
	}
#endif
#ifdef __ANDROID__
	res.w = SCREEN_RESCALE_X(res.w);
	res.h = SCREEN_RESCALE_Y(res.h);
#endif
	return res;
}


// open the window
int dr_os_open(const scr_size window_size, sint16 fs)
{
#ifdef MULTI_THREAD
	// init barrier
	simthread_barrier_init( &redraw_barrier, NULL, 2);

	// Initialize and set thread detached attribute
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);

	redraw_param.ready = false;
	if(  pthread_create( &(redraw_param.thread), &attr, redraw_thread, (void*)&redraw_param )  ) {
		dbg->error("dr_os_open(SDL)", "pthread_create failed, cannot multithread.");
		return 0;
	}

	pthread_attr_destroy( &attr );
#endif

	Uint32 flags = async_blit ? SDL_ASYNCBLIT : 0;

	width = window_size.w;
	height = window_size.h;
	fullscreen = fs;

	flags |= (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
	if(  use_hw  ) {
		flags |= SDL_HWSURFACE | SDL_DOUBLEBUF; // bltcopy in graphic memory should be faster ...
	}

	// open the window now
	SDL_putenv("SDL_VIDEO_CENTERED=center"); // request game window centered to stop it opening off screen since SDL1.2 has no way to open at a fixed position
	screen = SDL_SetVideoMode( max(1, window_size.w), max(1, window_size.h), COLOUR_DEPTH, flags );
	SDL_putenv("SDL_VIDEO_CENTERED="); // clear flag so it doesn't continually recenter upon resizing the window
	if(  screen == NULL  ) {
		dbg->error("dr_os_open(SDL)", "Couldn't open the window: %s", SDL_GetError());
		return 0;
	}

	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	char driver_name[128];
	SDL_VideoDriverName( driver_name, lengthof(driver_name) );
	dbg->debug("dr_os_open(SDL)", "SDL_driver=%s, hw_available=%i, video_mem=%i, blit_sw=%i, bpp=%i, bytes=%i\n", driver_name, vi->hw_available, vi->video_mem, vi->blit_sw, vi->vfmt->BitsPerPixel, vi->vfmt->BytesPerPixel );
	dbg->debug("dr_os_open(SDL)", "Screen Flags: requested=%x, actual=%x\n", flags, screen->flags );
	dbg->debug("dr_os_open(SDL)", "SDL realized screen size width=%d, height=%d, pitch=%d (requested w=%d, h=%d)\n", screen->w, screen->h, screen->pitch, window_size.w, window_size.h);

	SDL_EnableUNICODE(true);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// set the caption for the window
	SDL_WM_SetCaption(SIM_TITLE, 0);
	SDL_ShowCursor(0);
	arrow = SDL_GetCursor();
	hourglass = SDL_CreateCursor(hourglass_cursor, hourglass_cursor_mask, 16, 22, 8, 11);
	blank = SDL_CreateCursor(blank_cursor, blank_cursor, 8, 2, 0, 0);

	SDL_ShowCursor(1);

	display_set_actual_width( window_size.w );

	assert(screen->pitch % sizeof(PIXVAL) == 0);
	return screen->pitch / sizeof(PIXVAL);
}


// shut down SDL
void dr_os_close()
{
#ifdef MULTI_THREAD
	// signal thread to return
	pthread_mutex_lock( &redraw_mutex );
	redraw_param.ready = true;
	pthread_mutex_unlock( &redraw_mutex );
	// .. and join
	pthread_join(redraw_param.thread, NULL);
#endif
	SDL_FreeCursor(hourglass);
	SDL_FreeCursor(blank);
	// SDL doc says, screen is freed by SDL_Quit and should not be freed by the user
	// SDL_FreeSurface(screen);
}


// resizes screen
int dr_textur_resize(unsigned short** const textur, const int w, int const h)
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &redraw_mutex );
#endif
	if(  use_hw  ) {
		SDL_UnlockSurface( screen );
	}
	Uint32 flags = screen->flags;

	display_set_actual_width( w );

	if(  w!=screen->w  ||  h!=screen->h  ) {
		width = w;
		height = h;

		screen = SDL_SetVideoMode(max(1, w), max(1, h), COLOUR_DEPTH, flags);

		if (screen) {
			DBG_MESSAGE("dr_textur_resize(SDL)", "SDL realized screen size width=%d, height=%d (requested w=%d, h=%d)", screen->w, screen->h, w, h);
		}
		else if (dbg) {
			dbg->error("dr_textur_resize(SDL)", "screen is NULL. Good luck!");
		}
		fflush(NULL);
	}

	*textur = (unsigned short*)screen->pixels;

#ifdef MULTI_THREAD
	pthread_mutex_unlock( &redraw_mutex );
#endif

	assert(screen->pitch % sizeof(PIXVAL) == 0);
	return screen->pitch / sizeof(PIXVAL);
}


unsigned short *dr_textur_init()
{
	if(  use_hw  ) {
		SDL_LockSurface( screen );
	}
	return (unsigned short*)screen->pixels;
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
	return SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
}


void dr_prepare_flush()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &redraw_mutex );
#endif
	return;
}


void dr_flush()
{
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &redraw_mutex );
	simthread_barrier_wait( &redraw_barrier ); // start thread
#else
	display_flush_buffer();
	if(  use_hw  ) {
		SDL_UnlockSurface( screen );
		SDL_Flip( screen );
		SDL_LockSurface( screen );
	}
	else {
		if(  num_SDL_Rects > 0 ) {
			if(  num_SDL_Rects < MAX_SDL_RECTS  ) {
				SDL_UpdateRects( screen, num_SDL_Rects, SDL_Rects );
			}
			else {
				// too many Rects, just update the whole screen
				SDL_UpdateRect( screen, 0, 0, 0, 0);
			}
			num_SDL_Rects = 0;
		}
	}
#endif
}


void dr_textur(int xp, int yp, int w, int h)
{
	if(  !use_hw  )  {
		// make sure the given rectangle is completely on screen
		if(  xp + w > screen->w  ) {
			w = screen->w - xp;
		}
		if(  yp + h > screen->h  ) {
			h = screen->h - yp;
		}
#ifdef DEBUG
		// make sure both are positive numbers
		if(  w*h > 0  )
#endif
		{
#ifdef MULTI_THREAD
			SDL_UpdateRect( screen, xp, yp, w, h );
#else
			if(  num_SDL_Rects < MAX_SDL_RECTS  ) {
				SDL_Rects[num_SDL_Rects].x = xp;
				SDL_Rects[num_SDL_Rects].y = yp;
				SDL_Rects[num_SDL_Rects].w = w;
				SDL_Rects[num_SDL_Rects].h = h;
				num_SDL_Rects++;
			}
#endif
		}
	}
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


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */

static inline unsigned int ModifierKeys()
{
	SDLMod mod = SDL_GetModState();

	return
		((mod & KMOD_SHIFT) ? SIM_MOD_SHIFT : SIM_MOD_NONE) |
		((mod & KMOD_CTRL)  ? SIM_MOD_CTRL  : SIM_MOD_NONE);
}


static int conv_mouse_buttons(Uint8 const state)
{
	return
		(state & SDL_BUTTON_LMASK ? MOUSE_LEFTBUTTON  : 0) |
		(state & SDL_BUTTON_MMASK ? MOUSE_MIDBUTTON   : 0) |
		(state & SDL_BUTTON_RMASK ? MOUSE_RIGHTBUTTON : 0);
}


static void internal_GetEvents()
{
	SDL_Event event;
	event.type = 1;

	{
		int n;
		bool got_one = false;

		do {
			n = SDL_PollEvent(&event);

			if (n != 0) {
				got_one = true;

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
			sys_event.code = SYSTEM_RESIZE;
			sys_event.new_window_size_w = event.resize.w;
			sys_event.new_window_size_h = event.resize.h;
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
#ifdef _WIN32
			// SDL doesn't set numlock state correctly on startup. Revert to win32 function as workaround.
			const bool numlock = (GetKeyState(VK_NUMLOCK) & 1) != 0;
#else
			const bool numlock = SDL_GetModState() & KMOD_NUM;
#endif
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
				case SDLK_PAUSE:    code = 16;                            break;

				default:
					if (event.key.keysym.unicode != 0) {
						code = event.key.keysym.unicode;
						if (event.key.keysym.unicode == 22 /* ^V */) {
#if 0 // disabled as internal buffer is used; code is retained for possible future implementation of dr_paste()
							// X11 magic ... not tested yet!
							SDL_SysWMinfo si;
							if (SDL_GetWMInfo(&si) && si.subsystem == SDL_SYSWM_X11) {
								// clipboard under X11
								XEvent         evt;
								Atom           sseln   = XA_CLIPBOARD(si.x11.display);
								Atom           target  = XA_STRING;
								unsigned char* sel_buf = 0;
								unsigned long  sel_len = 0; /* length of sel_buf */
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
			sys_event.code = SYSTEM_QUIT;
			break;

		default:
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;
	}
}


void GetEvents()
{
	internal_GetEvents();
}


void show_pointer(int yesno)
{
	SDL_SetCursor((yesno != 0) ? arrow : blank);
}


void ex_ord_update_mx_my()
{
	SDL_PumpEvents();
}

uint32 dr_time()
{
	return SDL_GetTicks();
}


void dr_sleep(uint32 usec)
{
	SDL_Delay(usec);
}

void dr_start_textinput()
{
}

void dr_stop_textinput()
{
}

void dr_notify_input_pos(int, int)
{
}

const char* dr_get_locale()
{
	return NULL;
}

bool dr_has_fullscreen()
{
	return true;
}

sint16 dr_get_fullscreen()
{
	return fullscreen;
}

sint16 dr_toggle_borderless()
{
	return fullscreen;
}

#ifdef _MSC_VER
// Needed for MS Visual C++ with /SUBSYSTEM:CONSOLE to work , if /SUBSYSTEM:WINDOWS this function is compiled but unreachable
#undef main
int main()
{
	return WinMain(NULL,NULL,NULL,NULL);
}
#endif


#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
	int    const argc = __argc;
	char** const argv = __argv;
#endif
	return sysmain(argc, argv);
}
