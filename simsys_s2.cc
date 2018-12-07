/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 *
 * It contains the code to use the SDL2 backend for simutrans displayu
 *
 */

#include <SDL2/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>

#ifdef __CYGWIN__
extern int __argc;
extern char **__argv;
#endif

#include "macros.h"
#include "simsys_w32_png.h"
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"
#include "display/simgraph.h"
#include "simdebug.h"
#include "dataobj/environment.h"
#include "gui/simwin.h"
#include "gui/components/gui_komponente.h"
#include "gui/components/gui_textinput.h"

// Maybe Linux is not fine too, had critical bugs...
#if !defined(__linux__)
#define USE_SDL_TEXTEDITING
#else
#endif

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

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *screen_tx;
static SDL_Surface *screen;

static int sync_blit = 0;
static int use_dirty_tiles = 1;

static SDL_Cursor *arrow;
static SDL_Cursor *hourglass;
static SDL_Cursor *blank;


int x_scale = 32;
int y_scale = 32;


// no autoscaling yet
bool dr_auto_scale(bool on_off )
{
#if SDL_VERSION_ATLEAST(2,0,4)
	if(  on_off  ) {
		float hdpi, vdpi;
		SDL_Init( SDL_INIT_VIDEO );
		if(  SDL_GetDisplayDPI( 0, NULL, &hdpi, &vdpi )==0  ) {
			x_scale = ((long)hdpi*32+1)/96;
			y_scale = ((long)vdpi*32+1)/96;
			return true;
		}
		return false;
	}
	else
#else
#pragma message "SDL version must be at least 2.0.4 to support autoscaling."
#endif
	{
		x_scale = 48;
		y_scale = 48;
		(void)on_off;
		return false;
	}
}

/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
bool dr_os_init(const int* parameter)
{
	if(  SDL_Init( SDL_INIT_VIDEO ) != 0  ) {
		fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
		return false;
	}
	printf("SDL Driver: %s\n", SDL_GetCurrentVideoDriver() );

	// disable event types not interested in
#ifndef USE_SDL_TEXTEDITING
	SDL_EventState( SDL_TEXTEDITING, SDL_DISABLE );
#endif
	SDL_EventState( SDL_FINGERDOWN, SDL_DISABLE );
	SDL_EventState( SDL_FINGERUP, SDL_DISABLE );
	SDL_EventState( SDL_FINGERMOTION, SDL_DISABLE );
	SDL_EventState( SDL_DOLLARGESTURE, SDL_DISABLE );
	SDL_EventState( SDL_DOLLARRECORD, SDL_DISABLE );
	SDL_EventState( SDL_MULTIGESTURE, SDL_DISABLE );
	SDL_EventState( SDL_CLIPBOARDUPDATE, SDL_DISABLE );
	SDL_EventState( SDL_DROPFILE, SDL_DISABLE );

	sync_blit = parameter[0];  // hijack SDL1 -async flag for SDL2 vsync
	use_dirty_tiles = !parameter[1]; // hijack SDL1 -use_hw flag to turn off dirty tile updates (force fullscreen updates)

	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	SDL_StartTextInput();

	atexit( SDL_Quit ); // clean up on exit
	return true;
}


resolution dr_query_screen_resolution()
{
	resolution res;
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode( 0, &mode );
	DBG_MESSAGE("dr_query_screen_resolution(SDL)", "screen resolution width=%d, height=%d", mode.w, mode.h );
	res.w = (mode.w*32)/x_scale;
	res.h = (mode.h*32)/y_scale;
	return res;
}


bool internal_create_surfaces(const bool, int w, int h )
{
	// The pixel format needs to match the graphics code within simgraph16.cc.
	// Note that alpha is handled by simgraph16, not by SDL.
	const Uint32 pixel_format = SDL_PIXELFORMAT_RGB565;

	SDL_RendererInfo ri;
	const int num_rend = SDL_GetNumRenderDrivers();
	for(  int i = 0;  i < num_rend;  i++  ) {
		SDL_GetRenderDriverInfo( i, &ri );
		char str[4096];
		str[0] = 0;
		for(  Uint32 j = 0;  j < ri.num_texture_formats;  j++  ) {
			strcat( str, ", " );
			strcat( str, SDL_GetPixelFormatName( ri.texture_formats[j] ) );
		}
		DBG_DEBUG( "internal_create_surfaces()", "Renderer: %s, Max_w: %d, Max_h: %d, Flags: %d, Formats: %d%s", ri.name, ri.max_texture_width, ri.max_texture_height, ri.flags, ri.num_texture_formats, str );
	}

	Uint32 flags = SDL_RENDERER_ACCELERATED;
	if(  sync_blit  ) {
		flags |= SDL_RENDERER_PRESENTVSYNC;
	}
	renderer = SDL_CreateRenderer( window, -1, flags );
	if(  renderer == NULL  ) {
		dbg->warning( "internal_create_surfaces()", "Couldn't create accelerated renderer: %s", SDL_GetError() );

		flags &= ~SDL_RENDERER_ACCELERATED;
		flags |= SDL_RENDERER_SOFTWARE;
		renderer = SDL_CreateRenderer( window, -1, flags );
		if(  renderer == NULL  ) {
			dbg->fatal( "internal_create_surfaces()", "No SDL2 renderer found!" );
		}
		SDL_GetRendererInfo( renderer, &ri );
		dbg->warning( "internal_create_surfaces()", "Using fallback software renderer %s instead of accelerated: Performance may be low!", ri.name );
	}

	SDL_GetRendererInfo( renderer, &ri );
	DBG_DEBUG( "internal_create_surfaces()", "Using: Renderer: %s, Max_w: %d, Max_h: %d, Flags: %d, Formats: %d, %s", ri.name, ri.max_texture_width, ri.max_texture_height, ri.flags, ri.num_texture_formats, SDL_GetPixelFormatName(SDL_PIXELFORMAT_RGB565) );

	screen_tx = SDL_CreateTexture( renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, w, h );
	if(  screen_tx == NULL  ) {
		dbg->error( "internal_create_surfaces()", "Couldn't create texture: %s", SDL_GetError() );
		return false;
	}

	// Color component bitmasks for the RGB565 pixel format used by simgraph16.cc
	int bpp;
	Uint32 rmask, gmask, bmask, amask;
	SDL_PixelFormatEnumToMasks( pixel_format, &bpp, &rmask, &gmask, &bmask, &amask );
	if(  bpp != COLOUR_DEPTH  ||  amask != 0  ) {
		dbg->error( "internal_create_surfaces()", "Pixel format error. %d != %d, %d != 0", bpp, COLOUR_DEPTH, amask );
		return false;
	}

	screen = SDL_CreateRGBSurface( 0, w, h, bpp, rmask, gmask, bmask, amask );
	if(  screen == NULL  ) {
		dbg->error( "internal_create_surfaces()", "Couldn't get the window surface: %s", SDL_GetError() );
		return false;
 	}

	return true;
}


// open the window
int dr_os_open(int width, int height, int const fullscreen)
{
	// scale up
	int w = (width*32l)/x_scale;
	int h = (height*32l)/y_scale;

	// some cards need those alignments
	// especially 64bit want a border of 8bytes
	w = (w + 15) & 0x7FF0;
	if(  w <= 0  ) {
		w = 16;
	}
	width = (w*x_scale)/32l;

	Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;
	flags |= SDL_WINDOW_ALLOW_HIGHDPI; // apparently needed for Apple retina displays
	window = SDL_CreateWindow( SIM_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags );
	if(  window == NULL  ) {
		fprintf( stderr, "Couldn't open the window: %s\n", SDL_GetError() );
		return 0;
	}

	if(  !internal_create_surfaces( true, w, h )  ) {
		return 0;
	}
	DBG_MESSAGE("dr_os_open(SDL)", "SDL realized screen size width=%d, height=%d (internal w=%d, h=%d)", width, height, w, h );
	SDL_SetWindowSize( window, width, height );

	SDL_ShowCursor(0);
	arrow = SDL_GetCursor();
	hourglass = SDL_CreateCursor( hourglass_cursor, hourglass_cursor_mask, 16, 22, 8, 11 );
	blank = SDL_CreateCursor( blank_cursor, blank_cursor, 8, 2, 0, 0 );
	SDL_ShowCursor(1);

	if(  !env_t::hide_keyboard  ) {
		// endable keyboard input at all times unless requested otherwise
	    SDL_StartTextInput();
	}

	display_set_actual_width( w );
	display_set_height( h );
	return w;
}


// shut down SDL
void dr_os_close()
{
	SDL_FreeCursor( blank );
	SDL_FreeCursor( hourglass );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_StopTextInput();
}


// resizes screen
int dr_textur_resize(unsigned short** const textur, int w, int const h )
{
	// enforce multiple of 16 pixels, or there are likely mismatches
//	w = (w + 15 ) & 0x7FF0;

	// w, h are the width in pixel, we calculate now the scree size
	int width = (w*x_scale)/32l;
	int height = (h*y_scale)/32l;

	SDL_UnlockTexture( screen_tx );
	if(  width != screen->w  ||  height != screen->h  ) {
		// Recreate the SDL surfaces at the new resolution.
		// First free surface and then renderer.
		SDL_FreeSurface( screen );
		screen = NULL;
		// This destroys texture as well.
		SDL_DestroyRenderer( renderer );
		renderer = NULL;
		screen_tx = NULL;

		internal_create_surfaces( false, w, h );
		if(  screen  ) {
			DBG_MESSAGE("dr_textur_resize(SDL)", "SDL realized screen size width=%d, height=%d (requested w=%d, h=%d)", screen->w, screen->h, w, h );
		}
		else {
			dbg->warning("dr_textur_resize(SDL)", "screen is NULL. Good luck!");
		}
		fflush( NULL );
	}
	display_set_actual_width( screen->w );
	SDL_SetWindowSize( window, width, height );
	*textur = dr_textur_init();
	return screen->w;
}


unsigned short *dr_textur_init()
{
	// SDL_LockTexture modifies pixels, so copy it first
	void *pixels = screen->pixels;
	int pitch = screen->pitch;

	SDL_LockTexture( screen_tx, NULL, &pixels, &pitch );
	return (unsigned short*)screen->pixels;
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
	SDL_PixelFormat *fmt = SDL_AllocFormat( SDL_PIXELFORMAT_RGB565 );
	unsigned int ret = SDL_MapRGB( fmt, (Uint8)r, (Uint8)g, (Uint8)b );
	SDL_FreeFormat( fmt );
	return ret;
}


void dr_prepare_flush()
{
	return;
}


void dr_flush()
{
	display_flush_buffer();
	if(  !use_dirty_tiles  ) {
		SDL_UpdateTexture( screen_tx, NULL, screen->pixels, screen->pitch );
	}
	if(  x_scale != 32  &&  y_scale != 32  ) {
		SDL_Rect rDest = { 0, 0, (screen->w*x_scale)/32, (screen->h*y_scale)/32 };
		SDL_Rect rSrc = { 0, 0, screen->w, screen->h };
		SDL_RenderCopy( renderer, screen_tx, &rSrc, &rDest );
	}
	else {
		SDL_RenderCopy( renderer, screen_tx, NULL, NULL );
	}
	SDL_RenderPresent( renderer );
}


void dr_textur(int xp, int yp, int w, int h)
{
	if(  use_dirty_tiles  ) {
		SDL_Rect r;
		r.x = xp;
		r.y = yp;
		r.w = xp + w > screen->w ? screen->w - xp : w;
		r.h = yp + h > screen->h ? screen->h - yp : h;
		SDL_UpdateTexture( screen_tx, &r, (uint8*)screen->pixels + yp * screen->pitch + xp * 2, screen->pitch );
	}
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
	SDL_WarpMouseInWindow( window, (x*x_scale)/32, (y*y_scale)/32 );
}


// set the mouse cursor (pointer/load)
void set_pointer(int loading)
{
	SDL_SetCursor( loading ? hourglass : arrow );
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
	if(  dr_screenshot_png( filename, w, h, screen->w, ((unsigned short *)(screen->pixels)) + x + y * screen->w, screen->format->BitsPerPixel )  ) {
		return 1;
	}
#endif
	(void)x; (void)y; (void)w; (void)h;
	return SDL_SaveBMP( screen, filename ) == 0 ? 1 : -1;
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */


static inline unsigned int ModifierKeys()
{
	SDL_Keymod mod = SDL_GetModState();

	return
		(mod & KMOD_SHIFT ? 1 : 0)
		| (mod & KMOD_CTRL ? 2 : 0)
#ifdef __APPLE__
		// Treat the Command key as a control key.
		| (mod & KMOD_GUI ? 2 : 0)
#endif
		;
}


static int conv_mouse_buttons(Uint8 const state)
{
	return
		(state & SDL_BUTTON_LMASK ? MOUSE_LEFTBUTTON  : 0) |
		(state & SDL_BUTTON_MMASK ? MOUSE_MIDBUTTON   : 0) |
		(state & SDL_BUTTON_RMASK ? MOUSE_RIGHTBUTTON : 0);
}


static void internal_GetEvents(bool const wait)
{
	// Apparently Cocoa SDL posts key events that meant to be used by IM...
	// Ignoring SDL_KEYDOWN during preedit seems to work fine.
	static bool composition_is_underway = false;

	SDL_Event event;
	event.type = 1;
	if(  wait  ) {
		int n;
		do {
			SDL_WaitEvent( &event );
			n = SDL_PollEvent( NULL );
		} while(  n != 0  &&  event.type == SDL_MOUSEMOTION  );
	}
	else {
		int n;
		bool got_one = false;
		do {
			n = SDL_PollEvent( &event );
			if(  n != 0  ) {
				got_one = true;
				if(  event.type == SDL_MOUSEMOTION  ) {
					sys_event.type = SIM_MOUSE_MOVE;
					sys_event.code = SIM_MOUSE_MOVED;
					sys_event.mx   = (event.motion.x*32)/x_scale;
					sys_event.my   = (event.motion.y*32)/y_scale;
					sys_event.mb   = conv_mouse_buttons( event.motion.state );
				}
			}
		} while(  n != 0  &&  event.type == SDL_MOUSEMOTION  );
		if(  !got_one  ) {
			return;
		}
	}

	static char textinput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
	switch(  event.type  ) {
		case SDL_WINDOWEVENT: {
			if(  event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED  ) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SYSTEM_RESIZE;
				sys_event.size_x = (((event.window.data1+7)&0xFFFFFFF8)*32l)/x_scale;
				sys_event.size_y = (event.window.data2*32l)/y_scale;
			}
			// Ignore other window events.
			break;
		}
		case SDL_MOUSEBUTTONDOWN: {
			sys_event.type    = SIM_MOUSE_BUTTONS;
			switch(  event.button.button  ) {
				case SDL_BUTTON_LEFT:   sys_event.code = SIM_MOUSE_LEFTBUTTON;  break;
				case SDL_BUTTON_MIDDLE: sys_event.code = SIM_MOUSE_MIDBUTTON;   break;
				case SDL_BUTTON_RIGHT:  sys_event.code = SIM_MOUSE_RIGHTBUTTON; break;
				case SDL_BUTTON_X1:     sys_event.code = SIM_MOUSE_WHEELUP;     break;
				case SDL_BUTTON_X2:     sys_event.code = SIM_MOUSE_WHEELDOWN;   break;
			}
			sys_event.mx      = (event.button.x*32)/x_scale;
			sys_event.my      = (event.button.y*32)/y_scale;
			sys_event.mb      = conv_mouse_buttons( SDL_GetMouseState(0, 0) );
			sys_event.key_mod = ModifierKeys();
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			sys_event.type    = SIM_MOUSE_BUTTONS;
			switch(  event.button.button  ) {
				case SDL_BUTTON_LEFT:   sys_event.code = SIM_MOUSE_LEFTUP;  break;
				case SDL_BUTTON_MIDDLE: sys_event.code = SIM_MOUSE_MIDUP;   break;
				case SDL_BUTTON_RIGHT:  sys_event.code = SIM_MOUSE_RIGHTUP; break;
			}
			sys_event.mx      = (event.button.x*32)/x_scale;
			sys_event.my      = (event.button.y*32)/y_scale;
			sys_event.mb      = conv_mouse_buttons( SDL_GetMouseState(0, 0) );
			sys_event.key_mod = ModifierKeys();
			break;
		}
		case SDL_MOUSEWHEEL: {
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = event.wheel.y > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
			sys_event.key_mod = ModifierKeys();
			break;
		}
		case SDL_KEYDOWN: {
			// Hack: when 2 byte character composition is under way, we have to leave the key processing with the IME
			// BUT: if not, we have to do it ourselves, or the cursor or return will not be recognised
			if(  composition_is_underway  ) {
				if(  gui_component_t *c = win_get_focus()  ) {
					if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
						if(  tinp->get_composition()[0]  ) {
							// pending string, handled by IME
							break;
						}
					}
				}
			}

			unsigned long code;
#ifdef _WIN32
			// SDL doesn't set numlock state correctly on startup. Revert to win32 function as workaround.
			const bool numlock = (GetKeyState(VK_NUMLOCK) & 1) != 0;
#else
			const bool numlock = SDL_GetModState() & KMOD_NUM;
#endif
			sys_event.key_mod = ModifierKeys();
			SDL_Keycode sym = event.key.keysym.sym;
			switch(  sym  ) {
				case SDLK_BACKSPACE:  code = SIM_KEY_BACKSPACE;             break;
				case SDLK_TAB:        code = SIM_KEY_TAB;                   break;
				case SDLK_RETURN:     code = SIM_KEY_ENTER;                 break;
				case SDLK_ESCAPE:     code = SIM_KEY_ESCAPE;                break;
				case SDLK_DELETE:     code = SIM_KEY_DELETE;                break;
				case SDLK_DOWN:       code = SIM_KEY_DOWN;                  break;
				case SDLK_END:        code = SIM_KEY_END;                   break;
				case SDLK_HOME:       code = SIM_KEY_HOME;                  break;
				case SDLK_F1:         code = SIM_KEY_F1;                    break;
				case SDLK_F2:         code = SIM_KEY_F2;                    break;
				case SDLK_F3:         code = SIM_KEY_F3;                    break;
				case SDLK_F4:         code = SIM_KEY_F4;                    break;
				case SDLK_F5:         code = SIM_KEY_F5;                    break;
				case SDLK_F6:         code = SIM_KEY_F6;                    break;
				case SDLK_F7:         code = SIM_KEY_F7;                    break;
				case SDLK_F8:         code = SIM_KEY_F8;                    break;
				case SDLK_F9:         code = SIM_KEY_F9;                    break;
				case SDLK_F10:        code = SIM_KEY_F10;                   break;
				case SDLK_F11:        code = SIM_KEY_F11;                   break;
				case SDLK_F12:        code = SIM_KEY_F12;                   break;
				case SDLK_F13:        code = SIM_KEY_F13;                   break;
				case SDLK_F14:        code = SIM_KEY_F14;                   break;
				case SDLK_F15:        code = SIM_KEY_F15;                   break;
				case SDLK_KP_0:       code = numlock ? 0 : 0;               break;
				case SDLK_KP_1:       code = numlock ? 0 : SIM_KEY_END;     break;
				case SDLK_KP_2:       code = numlock ? 0 : SIM_KEY_DOWN;    break;
				case SDLK_KP_3:       code = numlock ? 0 : '<';             break;
				case SDLK_KP_4:       code = numlock ? 0 : SIM_KEY_LEFT;    break;
				case SDLK_KP_5:       code = numlock ? 0 : 0;               break;
				case SDLK_KP_6:       code = numlock ? 0 : SIM_KEY_RIGHT;   break;
				case SDLK_KP_7:       code = numlock ? 0 : SIM_KEY_HOME;    break;
				case SDLK_KP_8:       code = numlock ? 0 : SIM_KEY_UP;      break;
				case SDLK_KP_9:       code = numlock ? 0 : '>';             break;
				case SDLK_KP_DECIMAL: code = numlock ? 0 : SIM_KEY_DELETE;  break;
				case SDLK_KP_ENTER:   code = SIM_KEY_ENTER;                 break;
				case SDLK_LEFT:       code = SIM_KEY_LEFT;                  break;
				case SDLK_PAGEDOWN:   code = '<';                           break;
				case SDLK_PAGEUP:     code = '>';                           break;
				case SDLK_RIGHT:      code = SIM_KEY_RIGHT;                 break;
				case SDLK_UP:         code = SIM_KEY_UP;                    break;
				case SDLK_PAUSE:      code = 16;                            break;
				default: {
					// Handle CTRL-keys. SDL_TEXTINPUT event handles regular input
					if(  (sys_event.key_mod & 2)  &&  SDLK_a <= sym  &&  sym <= SDLK_z  ) {
						code = event.key.keysym.sym & 31;
					}
					else {
						code = 0;
					}
					break;
				}
			}
			sys_event.type    = SIM_KEYBOARD;
			sys_event.code    = code;
			break;
		}
		case SDL_TEXTINPUT: {
			size_t in_pos = 0;
			utf32 uc = utf8_decoder_t::decode((utf8 const*)event.text.text, in_pos);
			if(  event.text.text[in_pos]==0  ) {
				// single character
				sys_event.type    = SIM_KEYBOARD;
				sys_event.code    = (unsigned long)uc;
			}
			else {
				// string
				strcpy( textinput, event.text.text );
				sys_event.type    = SIM_STRING;
				sys_event.ptr     = (void*)textinput;
			}
			sys_event.key_mod = ModifierKeys();
			composition_is_underway = false;
			break;
		}
#ifdef USE_SDL_TEXTEDITING
		case SDL_TEXTEDITING: {
			//printf( "SDL_TEXTEDITING {timestamp=%d, \"%s\", start=%d, length=%d}\n", event.edit.timestamp, event.edit.text, event.edit.start, event.edit.length );
			strcpy( textinput, event.edit.text );
			if(  !textinput[0]  ) {
				composition_is_underway = false;
			}
			int i = 0;
			int start = 0;
			for(  ; i<event.edit.start; ++i  ) {
				start = utf8_get_next_char( (utf8 *)event.edit.text, start );
			}
			int end = start;
			for(  ; i<event.edit.start+event.edit.length; ++i  ) {
				end = utf8_get_next_char( (utf8*)event.edit.text, end );
			}

			if(  gui_component_t *c = win_get_focus()  ) {
				if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
					tinp->set_composition_status( textinput, start, end-start );
				}
			}
			composition_is_underway = true;
			break;
		}
#endif
		case SDL_MOUSEMOTION: {
			sys_event.type    = SIM_MOUSE_MOVE;
			sys_event.code    = SIM_MOUSE_MOVED;
			sys_event.mx      = (event.motion.x*32)/x_scale;
			sys_event.my      = (event.motion.y*32)/y_scale;
			sys_event.mb      = conv_mouse_buttons( event.motion.state );
			sys_event.key_mod = ModifierKeys();
			break;
		}
		case SDL_KEYUP: {
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			break;
		}
		case SDL_QUIT: {
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SYSTEM_QUIT;
			break;
		}
		default: {
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;
		}
	}
}


void GetEvents()
{
	internal_GetEvents( true );
}


void GetEventsNoWait()
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	internal_GetEvents( false );
}


void show_pointer(int yesno)
{
	SDL_SetCursor( (yesno != 0) ? arrow : blank );
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
	SDL_Delay( usec );
}


void dr_start_textinput()
{
	if(  env_t::hide_keyboard  ) {
	    SDL_StartTextInput();
	}
}


void dr_stop_textinput()
{
	if(  env_t::hide_keyboard  ) {
	    SDL_StopTextInput();
	}
}

void dr_notify_input_pos(int x, int y)
{
	SDL_Rect rect = { (x*x_scale)/32, ((y + LINESPACE)*y_scale)/32, 1, 1};
	SDL_SetTextInputRect( &rect );
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
