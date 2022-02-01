/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef _WIN32
#error "Only Windows has GDI!"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <winreg.h>
#include <wingdi.h>
#include <mmsystem.h>
#include <imm.h>
#include <commdlg.h>

#include <shellapi.h>
#include <shlobj.h>

#include <string>

#ifdef __CYGWIN__
extern int __argc;
extern char **__argv;
#endif

#include "simsys.h"

#include "../macros.h"
#include "../simconst.h"
#include "../simdebug.h"
#include "../simmem.h"
#include "../simversion.h"
#include "../simevent.h"
#include "../dataobj/environment.h"
#include "../display/simgraph.h"
#include "../gui/simwin.h"
#include "../gui/gui_frame.h"
#include "../gui/components/gui_component.h"
#include "../gui/components/gui_textinput.h"

// needed for wheel
#ifndef WM_MOUSEWHEEL
#	define WM_MOUSEWHEEL 0x020A
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#	define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD(wparam))
#endif



/*
 * The class name used to configure the main window.
 */
static const LPCWSTR WINDOW_CLASS_NAME = L"Simu";

static volatile HWND hwnd;
static sint16 fullscreen = WINDOWED;
static bool is_not_top = false;
static MSG msg;
static RECT WindowSize = { 0, 0, 0, 0 };
static RECT MaxSize;
static HINSTANCE hInstance;

static BITMAPINFO* AllDib = NULL;
static PIXVAL*     AllDibData;

volatile HDC hdc = NULL;


#ifdef MULTI_THREAD

HANDLE hFlushThread=0;
CRITICAL_SECTION redraw_underway;

// forward deceleration
DWORD WINAPI dr_flush_screen(LPVOID lpParam);
#endif

static long x_scale = 32;
static long y_scale = 32;



// scale automatically
bool dr_set_screen_scale(sint16 percent)
{
	bool scale_ok = false;
	long old_scale_x = x_scale, old_scale_y= y_scale;

	HDC hdc = GetDC(NULL);
	if(  percent == -1  &&  hdc  ) {
		x_scale = (GetDeviceCaps(hdc, LOGPIXELSX)*32)/96;
		y_scale = (GetDeviceCaps(hdc, LOGPIXELSY)*32)/96;
		scale_ok = true;
	}
	else if (percent == 0) {
		x_scale = x_scale = 32;
		scale_ok = true;
	}
	else if (percent < 0) {
		dbg->error("dr_set_screen_scale(Win32)", "Invalid screen scaling %d (Must be >= -1)", percent);
		return false;
	}
	else {
		x_scale = (percent * 32) / 100;
		y_scale = (percent * 32) / 100;
	}
	ReleaseDC(NULL, hdc);

	if(  (x_scale!=old_scale_x  ||  y_scale!=old_scale_y)  &&  hwnd) {
		// force window size update
		RECT TempRect;
		GetWindowRect(hwnd, &TempRect);
		const LRESULT res = PostMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(TempRect.right - TempRect.left, TempRect.bottom - TempRect.top));
	}
	return true;
}


sint16 dr_get_screen_scale()
{
	return (x_scale * 100) / 32;
}



/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
bool dr_os_init(int const* /*parameter*/)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	return true;
}


resolution dr_query_screen_resolution()
{
	resolution res;
	res.w = (GetSystemMetrics(SM_CXSCREEN)*32)/x_scale;
	res.h = (GetSystemMetrics(SM_CYSCREEN)*32)/y_scale;
	return res;
}


static void create_window(DWORD const ex_style, DWORD const style, int const x, int const y, int const w, int const h)
{
	RECT r = { 0, 0, w, h };
	AdjustWindowRectEx(&r, style, false, ex_style);

	// Convert UTF-8 to UTF-16.
	int const size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, SIM_TITLE, -1, NULL, 0);
	LPWSTR const wSIM_TITLE = new WCHAR[size];
	MultiByteToWideChar(CP_UTF8, 0, SIM_TITLE, -1, wSIM_TITLE, size);

	hwnd = CreateWindowExW(ex_style, WINDOW_CLASS_NAME, wSIM_TITLE, style, x, y, r.right - r.left, r.bottom - r.top, 0, 0, hInstance, 0);

	delete[] wSIM_TITLE;

	ShowWindow(hwnd, SW_SHOW);
	SetTimer( hwnd, 0, 1111, NULL ); // HACK: so windows thinks we are not dead when processing a timer every 1111 ms ...
}

static DEVMODE settings;


// open the window
int dr_os_open(const scr_size window_size, sint16 fs)
{
	MaxSize.right = ((window_size.w*x_scale)/32+15) & 0x7FF0;
	MaxSize.bottom = (window_size.h*y_scale)/32;
	fullscreen = fs;

#ifdef MULTI_THREAD
	InitializeCriticalSection( &redraw_underway );
	hFlushThread = CreateThread( NULL, 0, dr_flush_screen, 0, CREATE_SUSPENDED, NULL );
#endif
	MEMZERO(settings);

	// fake fullscreen
	if (fullscreen) {
		// try to force display mode and size
		settings.dmSize = sizeof(settings);
		settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		settings.dmBitsPerPel = COLOUR_DEPTH;
		settings.dmPelsWidth  = MaxSize.right;
		settings.dmPelsHeight = MaxSize.bottom;
		settings.dmDisplayFrequency = 0;

		if(  ChangeDisplaySettings(&settings, CDS_TEST)!=DISP_CHANGE_SUCCESSFUL  ) {
			// evt. try again in 32 bit
			if(  COLOUR_DEPTH<32  ) {
				settings.dmBitsPerPel = 32;
			}
			dbg->warning("dr_os_open(w32)", "Could not reduce color depth to 16 Bit in fullscreen." );
		}
		if(  ChangeDisplaySettings(&settings, CDS_TEST)!=DISP_CHANGE_SUCCESSFUL  ) {
			ChangeDisplaySettings( NULL, 0 );
			fullscreen = WINDOWED;
		}
		else {
			ChangeDisplaySettings(&settings, CDS_FULLSCREEN);
		}
	}
	if(  fullscreen  ) {
		create_window(WS_EX_TOPMOST, WS_POPUP, 0, 0, MaxSize.right, MaxSize.bottom);
	}
	else {
		create_window(0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, MaxSize.right, MaxSize.bottom);
	}

	WindowSize.right  = MaxSize.right;
	WindowSize.bottom = MaxSize.bottom;

	AllDib = MALLOCF(BITMAPINFO, bmiColors, 3);
	BITMAPINFOHEADER& header = AllDib->bmiHeader;
	header.biSize          = sizeof(BITMAPINFOHEADER);
	header.biWidth         = (window_size.w+15) & 0x7FF0;
	header.biHeight        = window_size.h;
	header.biPlanes        = 1;
	header.biBitCount      = COLOUR_DEPTH;
	header.biCompression   = BI_RGB;
	header.biSizeImage     = 0;
	header.biXPelsPerMeter = 0;
	header.biYPelsPerMeter = 0;
	header.biClrUsed       = 0;
	header.biClrImportant  = 0;
	DWORD* const masks = (DWORD*)AllDib->bmiColors;
#ifdef RGB555
	masks[0]               = 0x01;
	masks[1]               = 0x02;
	masks[2]               = 0x03;
#else
	header.biCompression   = BI_BITFIELDS;
	masks[0]               = 0x0000F800;
	masks[1]               = 0x000007E0;
	masks[2]               = 0x0000001F;
#endif

	MaxSize.right = header.biWidth;
	MaxSize.bottom = header.biHeight;

	return header.biWidth;
}


void dr_os_close()
{
#ifdef MULTI_THREAD
	if(  hFlushThread  ) {
		TerminateThread( hFlushThread, 0 );
		LeaveCriticalSection( &redraw_underway );
		DeleteCriticalSection( &redraw_underway );
	}
#endif
	if (hwnd != NULL) {
		DestroyWindow(hwnd);
	}
	free(AllDibData);
	AllDibData = NULL;
	free(AllDib);
	AllDib = NULL;
	if(  fullscreen == FULLSCREEN ) {
		ChangeDisplaySettings(NULL, 0);
	}
}


// resizes screen
int dr_textur_resize(unsigned short** const textur, int w, int const h)
{
#ifdef MULTI_THREAD
	EnterCriticalSection( &redraw_underway );
#endif

	// some cards need those alignments
	w = (w + 15) & 0x7FF0;
	if(  w<=0  ) {
		w = 16;
	}

	int img_w = w;
	int img_h = h;

	if(  w > MaxSize.right  ||  h >= MaxSize.bottom  ) {
		// since the query routines that return the desktop data do not take into account a change of resolution
		free(AllDibData);
		AllDibData = NULL;
		MaxSize.right = w;
		MaxSize.bottom = h;
		AllDibData = MALLOCN(PIXVAL, img_w * img_h);
		*textur = (unsigned short*)AllDibData;
	}

	AllDib->bmiHeader.biWidth  = img_w;
	AllDib->bmiHeader.biHeight = img_h;
	WindowSize.right           = w;
	WindowSize.bottom          = h;

#ifdef MULTI_THREAD
	LeaveCriticalSection( &redraw_underway );
#endif
	return w;
}


unsigned short *dr_textur_init()
{
	size_t const n = AllDib->bmiHeader.biWidth * AllDib->bmiHeader.biHeight;
	AllDibData = MALLOCN(PIXVAL, n);
	// start with black
	MEMZERON(AllDibData, n);
	return (unsigned short*)AllDibData;
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
#ifdef RGB555
	return ((r & 0x00F8) << 7) | ((g & 0x00F8) << 2) | (b >> 3); // 15 Bit
#else
	return ((r & 0x00F8) << 8) | ((g & 0x00FC) << 3) | (b >> 3);
#endif
}


#ifdef MULTI_THREAD
// multithreaded screen copy ...
DWORD WINAPI dr_flush_screen(LPVOID /*lpParam*/)
{
	while(1) {
		// wait for finish of thread
		EnterCriticalSection( &redraw_underway );
		hdc = GetDC(hwnd);
		display_flush_buffer();
		ReleaseDC(hwnd, hdc);
		hdc = NULL;
		LeaveCriticalSection( &redraw_underway );
		// suspend myself after one update
		SuspendThread( hFlushThread );
	}
	return 0;
}
#endif

void dr_prepare_flush()
{
#ifdef MULTI_THREAD
	// now the thread is finished ...
	EnterCriticalSection( &redraw_underway );
#endif
}

void dr_flush()
{
#ifdef MULTI_THREAD
	// just let the thread do its work
	LeaveCriticalSection( &redraw_underway );
	ResumeThread( hFlushThread );
#else
	assert(hdc==NULL);
	hdc = GetDC(hwnd);
	display_flush_buffer();
	ReleaseDC(hwnd, hdc);
	hdc = NULL;
#endif
}



void dr_textur(int xp, int yp, int w, int h)
{
	if(   x_scale==32  &&  y_scale==32  ) {
		// make really sure we are not beyond screen coordinates
		w = min( xp+w, AllDib->bmiHeader.biWidth ) - xp;
		h = min( yp+h, AllDib->bmiHeader.biHeight ) - yp;
		if(  h>1  &&  w>0  ) {
			StretchDIBits(hdc, xp, yp, w, h, xp, yp+h+1, w, -h, AllDibData, AllDib, DIB_RGB_COLORS, SRCCOPY);
		}
	}
	else {
		// align on 32 border to avoid rounding errors
		w += (xp % 32);
		h += (yp % 32);
		w = (w+31) & 0xFFE0;
		h = (h+31) & 0xFFE0;
		xp &= 0xFFE0;
		yp &= 0xFFE0;
		int xscr = (xp/32)*x_scale;
		int yscr = (yp/32)*y_scale;
		// make really sure we are not beyond screen coordinates
		w = min( xp+w, AllDib->bmiHeader.biWidth ) - xp;
		h = min( yp+h, AllDib->bmiHeader.biHeight ) - yp;
		if(  h>1  &&  w>0  ) {
			SetStretchBltMode( hdc, HALFTONE );
			SetBrushOrgEx( hdc, 0, 0, NULL );
			StretchDIBits(hdc, xscr, yscr, (w*x_scale)/32, (h*y_scale)/32, xp, yp+h+1, w, -h, AllDibData, AllDib, DIB_RGB_COLORS, SRCCOPY);
		}
	}
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
	if (fullscreen == WINDOWED) {
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
		fullscreen = BORDERLESS;
	}
	else if (fullscreen == BORDERLESS) {
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 10, 10, GetSystemMetrics(SM_CXSCREEN)-20, GetSystemMetrics(SM_CYSCREEN)-20, SWP_SHOWWINDOW);
		fullscreen = WINDOWED;
	}
	return fullscreen;
}

sint16 dr_suspend_fullscreen()
{
	int was_fullscreen = 0;
	if(  hwnd  &&  fullscreen  ) {
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
		was_fullscreen = fullscreen;
		fullscreen = WINDOWED;
	}
	else {
		ShowWindow(hwnd, SW_SHOWMINNOACTIVE);
	}
	return was_fullscreen;
}

void dr_restore_fullscreen(sint16 was_fullscreen)
{
	if(  hwnd  &&  was_fullscreen  ) {
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_EX_TOPMOST);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
		was_fullscreen = fullscreen;
	}
	else {
		ShowWindow(hwnd, SW_RESTORE);
	}
}

// move cursor to the specified location
bool move_pointer(int x, int y)
{
	POINT pt = { ((long)x*x_scale+16)/32, ((long)y*y_scale+16)/32 };

	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
	return true;
}


// set the mouse cursor (pointer/load)
void set_pointer(int loading)
{
	SetCursor(LoadCursor(NULL, loading != 0 ? IDC_WAIT : IDC_ARROW));
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */

static inline unsigned int ModifierKeys()
{
	return
		(GetKeyState(VK_SHIFT)   < 0  ? 1 : 0) |
		(GetKeyState(VK_CONTROL) < 0  ? 2 : 0); // highest bit set or return value<0 -> key is pressed
}


/* Windows eventhandler: does most of the work */
LRESULT WINAPI WindowProc(HWND this_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool update_mouse = false;
	// Used for IME handling.
	static utf8 *u8buf = NULL;
	static size_t u8bufsize;

	static int last_mb = 0; // last mouse button state
	switch (msg) {
		case WM_TIMER: // dummy timer even to keep windows thinking we are still active
			return 0;

		case WM_ACTIVATE: // may check, if we have to restore color depth
			if(fullscreen) {
				// avoid double calls
				static bool while_handling = false;
				if(while_handling) {
					break;
				}
				while_handling = true;

				if(LOWORD(wParam)!=WA_INACTIVE  &&  is_not_top) {
#ifdef MULTI_THREAD
					// no updating while deleting a window please ...
					EnterCriticalSection( &redraw_underway );
#endif
					// try to force display mode and size
					// should be always successful, since it worked as least once ...
					ChangeDisplaySettings(&settings, CDS_FULLSCREEN);
					is_not_top = false;

					// must reshow window, otherwise startbar will be topmost ...
					create_window(WS_EX_TOPMOST, WS_POPUP, 0, 0, MaxSize.right, MaxSize.bottom);
					DestroyWindow( this_hwnd );
					while_handling = false;
#ifdef MULTI_THREAD
					LeaveCriticalSection( &redraw_underway );
#endif
					return true;
				}
				else if(LOWORD(wParam)==WA_INACTIVE  &&  !is_not_top) {
					// restore default
					CloseWindow( hwnd );
					ChangeDisplaySettings( NULL, 0 );
					is_not_top = true;
				}

				while_handling = false;
			}
			break;

		case WM_GETMINMAXINFO:
			if(fullscreen) {
				LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam;
				lpmmi->ptMaxPosition.x = 0;
				lpmmi->ptMaxPosition.y = 0;
			}
			break;

		case WM_LBUTTONDOWN: /* originally ButtonPress */
			SetCapture(this_hwnd);
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_LEFTBUTTON;
			update_mouse = true;
			break;

		case WM_LBUTTONUP: /* originally ButtonRelease */
			ReleaseCapture();
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_LEFTUP;
			update_mouse = true;
			break;

		case WM_MBUTTONDOWN: /* because capture or release may not start with the expected button */
		case WM_XBUTTONDOWN:
			SetCapture(this_hwnd);
			break;

		case WM_MBUTTONUP: /* because capture or release may not start with the expected button */
		case WM_XBUTTONUP:
			ReleaseCapture();
			break;

		case WM_RBUTTONDOWN: /* originally ButtonPress */
			SetCapture(this_hwnd);
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTBUTTON;
			update_mouse = true;
			break;

		case WM_RBUTTONUP: /* originally ButtonRelease */
			ReleaseCapture();
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTUP;
			update_mouse = true;
			break;

		case WM_MOUSEMOVE:
			sys_event.type    = SIM_MOUSE_MOVE;
			sys_event.code    = SIM_MOUSE_MOVED;
			update_mouse = true;
			break;

		case WM_MOUSEWHEEL:
			sys_event.type = SIM_MOUSE_BUTTONS;
			sys_event.code = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
			sys_event.key_mod = ModifierKeys();
			/* the returned coordinate in LPARAM are absolute coordinates, which will deeply confuse simutrans
			 * we just reuse the coordinates we used the last time by not changing mx/my ...
			 */
			return 0;

#ifdef WM_DPICHANGED
		case WM_DPICHANGED:
			{
				RECT *r = (LPRECT)lParam;
				x_scale = (LOWORD(wParam)*32)/96;
				y_scale = (HIWORD(wParam)*32)/96;
				SetWindowPos( hwnd, NULL, r->left, r->top, r->right-r->left, r->bottom-r->top, SWP_NOZORDER );
			}
			break;
#endif

		case WM_SIZE: // resize client area
			if(lParam!=0) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SYSTEM_RESIZE;

				sys_event.new_window_size_w = (LOWORD(lParam)*32)/x_scale;
				if (sys_event.new_window_size_w <= 0) {
					sys_event.new_window_size_w = 4;
				}

				sys_event.new_window_size_h = (HIWORD(lParam)*32)/y_scale;
				if (sys_event.new_window_size_h <= 1) {
					sys_event.new_window_size_h = 64;
				}
			}
			break;

		case WM_PAINT: {
			if (AllDib != NULL) {
				PAINTSTRUCT ps;
				HDC hdcp;

				hdcp = BeginPaint(hwnd, &ps);
				AllDib->bmiHeader.biHeight = (WindowSize.bottom*32)/y_scale;
				StretchDIBits(hdcp, 0, 0, WindowSize.right, WindowSize.bottom, 0, AllDib->bmiHeader.biHeight + 1, AllDib->bmiHeader.biWidth, -AllDib->bmiHeader.biHeight, AllDibData, AllDib, DIB_RGB_COLORS, SRCCOPY);
				EndPaint(this_hwnd, &ps);
			}
			break;
		}

		case WM_KEYDOWN: { /* originally KeyPress */
			// check for not numlock!
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			sys_event.key_mod = ModifierKeys();

			sint16 code = lParam >> 16;
			if(  code >= 0x47  &&  code <= 0x52  &&  code != 0x4A  &&  code != 0x4e  ) {
				if(  (GetKeyState( VK_NUMLOCK ) & 1) == 0  ||  (env_t::numpad_always_moves_map  &&  !win_is_textinput())  ) { // numlock off?
					switch( code ) {
						case 0x47: code = SIM_KEY_UPLEFT; break;
						case 0x48: code = SIM_KEY_UP; break;
						case 0x49: code = SIM_KEY_UPRIGHT; break;
						case 0x4B: code = SIM_KEY_LEFT; break;
						case 0x4C: code = SIM_KEY_CENTER; break;
						case 0x4D: code = SIM_KEY_RIGHT; break;
						case 0x4F: code = SIM_KEY_DOWNLEFT; break;
						case 0x50: code = SIM_KEY_DOWN; break;
						case 0x51: code = SIM_KEY_DOWNRIGHT; break;
						case 0x52: code = SIM_KEY_NUMPAD_BASE; break;
					}
					if(  code>=SIM_KEY_NUMPAD_BASE  ) {
						// ok found something
						sys_event.code = code;
						break;
					}
				}
			}

			// do low level special stuff here
			switch (wParam) {
				case VK_SCROLL: sys_event.code = SIM_KEY_SCROLLLOCK; break;
				case VK_PAUSE:  sys_event.code = SIM_KEY_PAUSE; break;
				case VK_LEFT:   sys_event.code = SIM_KEY_LEFT;  break;
				case VK_RIGHT:  sys_event.code = SIM_KEY_RIGHT; break;
				case VK_UP:     sys_event.code = SIM_KEY_UP;    break;
				case VK_DOWN:   sys_event.code = SIM_KEY_DOWN;  break;
				case VK_PRIOR:  sys_event.code = '>';           break;
				case VK_NEXT:   sys_event.code = '<';           break;
				case VK_DELETE: sys_event.code = 127;           break;
				case VK_HOME:   sys_event.code = SIM_KEY_HOME;  break;
				case VK_END:    sys_event.code = SIM_KEY_END;   break;
			}
			// check for F-Keys!
			if (sys_event.code == 0 && wParam >= VK_F1 && wParam <= VK_F15) {
				sys_event.code = wParam - VK_F1 + SIM_KEY_F1;
				//printf("WindowsEvent: Key %i, (state %i)\n", sys_event.code, sys_event.key_mod);
			}
			// some result?
			if (sys_event.code != 0) return 0;
			sys_event.type = SIM_NOEVENT;
			sys_event.code = 0;
			break;
		}

		case WM_CHAR: /* originally KeyPress */
		{
			sint16 code = lParam >> 16;
			if(  code >= 0x47  &&  code <= 0x52  &&  code != 0x4A  &&  code != 0x4e  ) {
				if(  (GetKeyState( VK_NUMLOCK ) & 1) == 0  ||  (env_t::numpad_always_moves_map  &&  !win_is_textinput())  ) { // numlock off?
					// we handled this numpad keys already above ...
					sys_event.type = SIM_NOEVENT;
					sys_event.code = 0;
					break;
				}
			}
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = wParam;
			sys_event.key_mod = ModifierKeys();
			break;
		}

		case WM_IME_SETCONTEXT:
			// attempt to avoid crash at windows 1809> just not call DefWinodwsProc seems to work for SDL2 ...
//			DefWindowProc( this_hwnd, msg, wParam, lParam&~ISC_SHOWUICOMPOSITIONWINDOW );
			lParam = 0;
			return 0;

		case WM_IME_STARTCOMPOSITION:
			break;

		case WM_IME_REQUEST:
			if(  wParam == IMR_QUERYCHARPOSITION  ) {
				if(  gui_component_t *c = win_get_focus()  ) {
					if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
						IMECHARPOSITION *icp = (IMECHARPOSITION *)lParam;
						icp->cLineHeight = LINESPACE;

						scr_coord pos = tinp->get_pos();
						scr_coord gui_xy = win_get_pos( win_get_top() );
						LONG x = pos.x + gui_xy.x + tinp->get_current_cursor_x();
						const char *composition = tinp->get_composition();
						for(  DWORD i=0; i<icp->dwCharPos; ++i  ) {
							if(  !*composition  ) {
								break;
							}
							unsigned char pixel_width;
							unsigned char unused;
							get_next_char_with_metrics( composition, unused, pixel_width );
							x += pixel_width;
						}
						icp->pt.x = x;
						icp->pt.y = pos.y + gui_xy.y + D_TITLEBAR_HEIGHT;
						ClientToScreen( this_hwnd, &icp->pt );
						icp->rcDocument.left = 0;
						icp->rcDocument.right = 0;
						icp->rcDocument.top = 0;
						icp->rcDocument.bottom = 0;
						//printf("IMECHARPOSITION {dwCharPos=%d, pt {x=%d, y=%d} }\n", icp->dwCharPos, icp->pt.x, icp->pt.y);
						return TRUE;
					}
				}
				break;
			}
			return DefWindowProcW( this_hwnd, msg, wParam, lParam );

		case WM_IME_COMPOSITION: {
			HIMC immcx = 0;
			if(  lParam & (GCS_RESULTSTR|GCS_COMPSTR)  ) {
				immcx = ImmGetContext( this_hwnd );
			}
			if(  lParam & GCS_RESULTSTR  ) {
				// Retrieve the composition result.
				size_t u16size = ImmGetCompositionStringW(immcx, GCS_RESULTSTR, NULL, 0);
				utf16 *u16buf = (utf16*)malloc(u16size + 2);
				size_t copied = ImmGetCompositionStringW(immcx, GCS_RESULTSTR, u16buf, u16size + 2);

				// clear old composition
				if(  gui_component_t *c = win_get_focus()  ) {
					if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
						tinp->set_composition_status( NULL, 0, 0 );
					}
				}
				// add result
				if(  u16size>2  ) {
					u16buf[copied/2] = 0;

					// Grow the buffer as needed.
					size_t u8size = u16size + u16size/2;
					if(  !u8buf  ) {
						u8bufsize = u8size + 1;
						u8buf = (utf8 *)malloc( u8bufsize );
					}
					else if(  u8size >= u8bufsize  ) {
						u8bufsize = max( u8bufsize*2, u8size+1 );
						free( u8buf );
						u8buf = (utf8 *)malloc( u8bufsize );
					}

					// Convert UTF-16 to UTF-8.
					utf16 *s = u16buf;
					int i = 0;
					while(  *s  ) {
						i += utf16_to_utf8( *s++, u8buf+i );
					}
					u8buf[i] = 0;
					free( u16buf );

					sys_event.type = SIM_STRING;
					sys_event.ptr = (void*)u8buf;
				}
				else {
					// single key
					sys_event.type = SIM_KEYBOARD;
					sys_event.code = u16buf[0];
				}
				sys_event.key_mod = ModifierKeys();
			}
			else if(  lParam & (GCS_COMPSTR|GCS_COMPATTR)  ) {
				if(  gui_component_t *c = win_get_focus()  ) {
					if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
						// Query current conversion status
						int num_attr = ImmGetCompositionStringW( immcx, GCS_COMPATTR, NULL, 0 );
						if(  num_attr <= 0  ) {
							// This shouldn't happen... just in case.
							break;
						}
						char *attrs = (char *)malloc( num_attr );
						ImmGetCompositionStringW( immcx, GCS_COMPATTR, attrs, num_attr );
						int start = 0;
						for(  ; start < num_attr; ++start  ) {
							if(  attrs[start] == ATTR_TARGET_CONVERTED || attrs[start] == ATTR_TARGET_NOTCONVERTED  ) {
								break;
							}
						}
						int end = start;
						for(  ; end < num_attr; ++end  ) {
							if(  attrs[end] != ATTR_TARGET_CONVERTED && attrs[end] != ATTR_TARGET_NOTCONVERTED  ) {
								break;
							}
						}
						free( attrs );

						// Then retrieve the composition text
						size_t u16size = ImmGetCompositionStringW( immcx, GCS_COMPSTR, NULL, 0 );
						utf16 *u16buf = (utf16 *)malloc( u16size+2 );
						size_t copied = ImmGetCompositionStringW( immcx, GCS_COMPSTR, u16buf, u16size+2 );
						u16buf[copied/2] = 0;

						// Grow the buffer as needed.
						size_t u8size = u16size + u16size/2;
						if(  !u8buf  ) {
							u8bufsize = u8size + 1;
							u8buf = (utf8 *)malloc( u8bufsize );
						}
						else if(  u8size >= u8bufsize  ) {
							u8bufsize = max( u8bufsize*2, u8size+1 );
							free( u8buf );
							u8buf = (utf8 *)malloc( u8bufsize );
						}

						// Convert UTF-16 to UTF-8.
						utf16 *s = u16buf;
						int offset = 0;
						int i = 0;
						int u8start = 0, u8end = 0;
						while(  true  ) {
							if(  i == start  ) u8start = offset;
							if(  i == end  ) u8end = offset;
							if(  !*s  ) {
								break;
							}
							offset += utf16_to_utf8( *s, u8buf + offset );
							++s;
							++i;
						}
						u8buf[offset] = 0;
						free( u16buf );

						tinp->set_composition_status( (char *)u8buf, u8start, u8end-u8start );
					}
				}
			}
			if(  immcx  ) {
				ImmReleaseContext( this_hwnd, immcx );
			}
			break;
		}

		case WM_IME_ENDCOMPOSITION:
			if(  gui_component_t *c = win_get_focus()  ) {
				if(  gui_textinput_t *tinp = dynamic_cast<gui_textinput_t *>(c)  ) {
					tinp->set_composition_status( NULL, 0, 0 );
				}
			}
			break;

		case WM_CLOSE:
			if (AllDibData != NULL) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SYSTEM_QUIT;
			}
			break;

		case WM_DESTROY:
			free( u8buf );
			if(  hwnd==this_hwnd  ||  AllDibData == NULL  ) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SYSTEM_QUIT;
				if(  AllDibData == NULL  ) {
					PostQuitMessage(0);
					hwnd = NULL;
				}
			}
			break;

		default:
			return DefWindowProcW(this_hwnd, msg, wParam, lParam);
	}

	if(  update_mouse  ) {
		sys_event.key_mod = ModifierKeys();
		sys_event.mb = last_mb = (wParam&3);
		sint16 x = LOWORD(lParam);
		sys_event.mx      = (x * 32l)/x_scale;
		sint16 y = HIWORD(lParam);
		sys_event.my      = (y * 32l)/y_scale;
	}


	return 0;
}


static void internal_GetEvents(bool const wait)
{
	do {
		// wait for keybord/mouse event
		GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	} while(wait && sys_event.type == SIM_NOEVENT);

}


void GetEvents()
{
	if (sys_event.type==SIM_NOEVENT  &&  PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
		internal_GetEvents(false);
	}
}


void show_pointer(int yesno)
{
	static int state=true;
	if(state^yesno) {
		ShowCursor(yesno);
		state = yesno;
	}
}



void ex_ord_update_mx_my()
{
	// evt. called before update
}



uint32 dr_time()
{
	return timeGetTime();
}




void dr_sleep(uint32 millisec)
{
	Sleep(millisec);
}

void dr_start_textinput()
{
}

void dr_stop_textinput()
{
	HIMC immcx = ImmGetContext( hwnd );
	ImmNotifyIME( immcx, NI_COMPOSITIONSTR, CPS_CANCEL, 0 );
	ImmReleaseContext( hwnd, immcx );
}

void dr_notify_input_pos(int x, int y)
{
	x = (x*x_scale)/32;
	y = (y*y_scale)/32;

	COMPOSITIONFORM co;
	co.dwStyle = CFS_POINT;
	co.ptCurrentPos.x = (LONG)x;
	co.ptCurrentPos.y = (LONG)y;

	CANDIDATEFORM ca;
	ca.dwIndex = 0;
	ca.dwStyle = CFS_CANDIDATEPOS;
	ca.ptCurrentPos.x = (LONG)x;
	ca.ptCurrentPos.y = (LONG)y + D_TITLEBAR_HEIGHT;

	HIMC immcx = ImmGetContext( hwnd );
	ImmSetCompositionWindow( immcx, &co );
	ImmSetCandidateWindow( immcx, &ca );
	ImmReleaseContext( hwnd, immcx );
}

#ifdef _MSC_VER
// Needed for MS Visual C++ with /SUBSYSTEM:CONSOLE to work , if /SUBSYSTEM:WINDOWS this function is compiled but unreachable
int main()
{
	HINSTANCE const hInstance = (HINSTANCE)GetModuleHandle(NULL);
	return WinMain(hInstance,NULL,NULL,NULL);
}
#endif


const char* dr_get_locale()
{
	static char LanguageCode[5]="";
	GetLocaleInfoA( GetUserDefaultUILanguage(), LOCALE_SISO639LANGNAME, LanguageCode, lengthof( LanguageCode ) );
	return LanguageCode;
}

int CALLBACK WinMain(HINSTANCE const hInstance, HINSTANCE, LPSTR, int)
{
	WNDCLASSW wc;
	bool timer_is_set = false;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassW(&wc);

	GetWindowRect(GetDesktopWindow(), &MaxSize);

	// maybe set timer to 1ms interval on Win2k upwards ...
	{
		OSVERSIONINFO osinfo;
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (GetVersionEx(&osinfo)  &&  osinfo.dwPlatformId==VER_PLATFORM_WIN32_NT) {
			timeBeginPeriod(1);
			timer_is_set = true;
		}
	}

	int const res = sysmain(__argc, __argv);
	if(  timer_is_set  ) {
		timeEndPeriod(1);
	}

#ifdef MULTI_THREAD
	if(  hFlushThread ) {
		TerminateThread( hFlushThread, 0 );
	}
#endif
	return res;
}
