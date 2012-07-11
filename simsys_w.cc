/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef _WIN32
#error "Only Windows has GDI!"
#endif

#include <stdio.h>
#include <stdlib.h>

// windows Bibliotheken DirectDraw 5.x
#define UNICODE 1
// windows.h defines min and max macros which we don't want
#define NOMINMAX 1
#include <windows.h>
#include <winreg.h>
#include <wingdi.h>
#include <mmsystem.h>

#include "simgraph.h"


// needed for wheel
#ifndef WM_MOUSEWHEEL
#	define WM_MOUSEWHEEL 0x020A
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#	define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD(wparam))
#endif

// 16 Bit may be much slower than 15 unfourtunately on some hardware
#define USE_16BIT_DIB

#include "simmem.h"
#include "simsys_w32_png.h"
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"
#include "simdebug.h"
#include "macros.h"

typedef unsigned short PIXVAL;

static HWND hwnd;
static bool is_fullscreen = false;
static bool is_not_top = false;
static MSG msg;
static RECT WindowSize = { 0, 0, 0, 0 };
static RECT MaxSize;
static HINSTANCE hInstance;

static BITMAPINFO* AllDib;
static PIXVAL*     AllDibData;

volatile HDC hdc = NULL;


#ifdef MULTI_THREAD

HANDLE	hFlushThread=0;
CRITICAL_SECTION redraw_underway;

// forward decleration
DWORD WINAPI dr_flush_screen(LPVOID lpParam);
#endif


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
	res.w = GetSystemMetrics(SM_CXSCREEN);
	res.h = GetSystemMetrics(SM_CYSCREEN);
	return res;
}


static void create_window(DWORD const ex_style, DWORD const style, int const x, int const y, int const w, int const h)
{
	RECT r = { 0, 0, w, h };
	AdjustWindowRectEx(&r, style, false, ex_style);
	hwnd = CreateWindowExA(ex_style, "Simu", SIM_TITLE, style, x, y, r.right - r.left, r.bottom - r.top, 0, 0, hInstance, 0);
	ShowWindow(hwnd, SW_SHOW);
}


// open the window
int dr_os_open(int const w, int const h, int fullscreen)
{
	MaxSize.right = (w+15)&0x7FF0;
	MaxSize.bottom = h+1;

#ifdef MULTI_THREAD
	InitializeCriticalSection( &redraw_underway );
	hFlushThread = CreateThread( NULL, 0, dr_flush_screen, 0, CREATE_SUSPENDED, NULL );
#endif

	// fake fullscreen
	if (fullscreen) {
		// try to force display mode and size
		DEVMODE settings;

		MEMZERO(settings);
		settings.dmSize = sizeof(settings);
		settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		settings.dmBitsPerPel = COLOUR_DEPTH;
		settings.dmPelsWidth  = w;
		settings.dmPelsHeight = h;
		settings.dmDisplayFrequency = 0;

		if(  ChangeDisplaySettings(&settings, CDS_TEST)!=DISP_CHANGE_SUCCESSFUL  ) {
			ChangeDisplaySettings( NULL, 0 );
			printf( "dr_os_open()::Could not reduce color depth to 16 Bit in fullscreen." );
			fullscreen = false;
		}
		else {
			ChangeDisplaySettings(&settings, CDS_FULLSCREEN);
		}
		is_fullscreen = fullscreen;
	}
	if(  fullscreen  ) {
		create_window(WS_EX_TOPMOST, WS_POPUP, 0, 0, w, h);
	}
	else {
		create_window(0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, w, h);
	}

	WindowSize.right  = w;
	WindowSize.bottom = h;

	AllDib = MALLOCF(BITMAPINFO, bmiColors, 3);
	BITMAPINFOHEADER& header = AllDib->bmiHeader;
	header.biSize          = sizeof(BITMAPINFOHEADER);
	header.biWidth         = w;
	header.biHeight        = h;
	header.biPlanes        = 1;
	header.biBitCount      = COLOUR_DEPTH;
	header.biCompression   = BI_RGB;
	header.biSizeImage     = 0;
	header.biXPelsPerMeter = 0;
	header.biYPelsPerMeter = 0;
	header.biClrUsed       = 0;
	header.biClrImportant  = 0;
	DWORD* const masks = (DWORD*)AllDib->bmiColors;
#ifdef USE_16BIT_DIB
	header.biCompression   = BI_BITFIELDS;
	masks[0]               = 0x0000F800;
	masks[1]               = 0x000007E0;
	masks[2]               = 0x0000001F;
#else
	masks[0]               = 0x01;
	masks[1]               = 0x02;
	masks[2]               = 0x03;
#endif

	return MaxSize.right;
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
	if(  is_fullscreen  ) {
		ChangeDisplaySettings(NULL, 0);
	}
}


// reiszes screen
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

	if(w>MaxSize.right  ||  h>=MaxSize.bottom) {
		// since the query routines that return the desktop data do not take into account a change of resolution
		free(AllDibData);
		AllDibData = NULL;
		MaxSize.right = w;
		MaxSize.bottom = h+1;
		AllDibData = MALLOCN(PIXVAL, MaxSize.right * MaxSize.bottom);
		*textur = (unsigned short*)AllDibData;
	}

	AllDib->bmiHeader.biWidth  = w;
	AllDib->bmiHeader.biHeight = h;
	WindowSize.right           = w;
	WindowSize.bottom          = h;

#ifdef MULTI_THREAD
	LeaveCriticalSection( &redraw_underway );
#endif
	return w;
}


unsigned short *dr_textur_init()
{
	size_t const n = MaxSize.right * MaxSize.bottom;
	AllDibData = MALLOCN(PIXVAL, n);
	// start with black
	MEMZERON(AllDibData, n);
	return (unsigned short*)AllDibData;
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
#ifdef USE_16BIT_DIB
	return ((r & 0x00F8) << 8) | ((g & 0x00FC) << 3) | (b >> 3);
#else
	return ((r & 0x00F8) << 7) | ((g & 0x00F8) << 2) | (b >> 3); // 15 Bit
#endif
}


#ifdef MULTI_THREAD
// multhreaded screen copy ...
DWORD WINAPI dr_flush_screen(LPVOID lpParam)
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
	// make really sure we are not beyond screen coordinates
	h = min( yp+h, WindowSize.bottom ) - yp;
#ifdef DEBUG
	w = min( xp+w, WindowSize.right ) - xp;
	if(  h>1  &&  w>0  )
#endif
	{
		AllDib->bmiHeader.biHeight = h + 1;
		StretchDIBits(hdc, xp, yp, w, h, xp, h + 1, w, -h, AllDibData + yp * WindowSize.right, AllDib, DIB_RGB_COLORS, SRCCOPY);
	}
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
	POINT pt = { x, y };

	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
}


// set the mouse cursor (pointer/load)
void set_pointer(int loading)
{
	SetCursor(LoadCursor(NULL, loading != 0 ? IDC_WAIT : IDC_ARROW));
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
#if defined USE_16BIT_DIB
	int const bpp = COLOUR_DEPTH;
#else
	int const bpp = 15;
#endif
	if (!dr_screenshot_png(filename, display_get_width() - 1, WindowSize.bottom + 1, AllDib->bmiHeader.biWidth, (unsigned short*)AllDibData, bpp)) {
		// not successful => save as BMP
		if (FILE* const fBmp = fopen(filename, "wb")) {
			BITMAPFILEHEADER bf;

			// since the number of drawn pixel can be smaller than the actual width => only use the drawn pixel for bitmap
			LONG const old_width = AllDib->bmiHeader.biWidth;
			AllDib->bmiHeader.biWidth  = display_get_width() - 1;
			AllDib->bmiHeader.biHeight = WindowSize.bottom   + 1;

			bf.bfType = 0x4d42; //"BM"
			bf.bfReserved1 = 0;
			bf.bfReserved2 = 0;
			bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(DWORD)*3;
			bf.bfSize      = (bf.bfOffBits + AllDib->bmiHeader.biHeight * AllDib->bmiHeader.biWidth * 2L + 3L) / 4L;
			fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, fBmp);
			fwrite(AllDib, sizeof(AllDib->bmiHeader) + sizeof(*AllDib->bmiColors) * 3, 1, fBmp);

			for (LONG i = 0; i < AllDib->bmiHeader.biHeight; ++i) {
				// row must be alsway even number of pixel
				fwrite(AllDibData + (AllDib->bmiHeader.biHeight - 1 - i) * old_width, (AllDib->bmiHeader.biWidth + 1) & 0xFFFE, 2, fBmp);
			}
			AllDib->bmiHeader.biWidth = old_width;

			fclose(fBmp);
		}
		else {
			return -1;
		}
	}
	return 0;
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
	static int last_mb = 0;	// last mouse button state
	switch (msg) {

		case WM_ACTIVATE: // may check, if we have to restore color depth
			if(is_fullscreen) {
				// avoid double calls
				static bool while_handling = false;
				if(while_handling) {
					break;
				}
				while_handling = true;

				if(LOWORD(wParam)!=WA_INACTIVE  &&  is_not_top) {
					// try to force display mode and size
					DEVMODE settings;

					MEMZERO(settings);
					settings.dmSize = sizeof(settings);
					settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
#if defined USE_16BIT_DIB
					settings.dmBitsPerPel = COLOUR_DEPTH;
#else
					settings.dmBitsPerPel = 15;
#endif
					settings.dmPelsWidth  = MaxSize.right;
					settings.dmPelsHeight = MaxSize.bottom;
					settings.dmDisplayFrequency = 0;

					// should be alsway sucessful, since it worked as least once ...
					ChangeDisplaySettings(&settings, CDS_FULLSCREEN);
					is_not_top = false;

					Beep( 110, 250 );
					// must reshow window, otherwise startbar will be topmost ...
					create_window(WS_EX_TOPMOST, WS_POPUP, 0, 0, MaxSize.right, MaxSize.bottom);
					DestroyWindow( this_hwnd );
					while_handling = false;
					return true;
				}
				else if(LOWORD(wParam)==WA_INACTIVE  &&  !is_not_top) {
					// restore default
					CloseWindow( hwnd );
					ChangeDisplaySettings( NULL, 0 );
					is_not_top = true;
					Beep( 440, 250 );
				}

				while_handling = false;
			}
			break;

		case WM_GETMINMAXINFO:
			if(is_fullscreen) {
				LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam;
				lpmmi->ptMaxPosition.x = 0;
				lpmmi->ptMaxPosition.y = 0;
			}
			break;

		case WM_LBUTTONDOWN: /* originally ButtonPress */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_LEFTBUTTON;
			sys_event.key_mod = ModifierKeys();
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_LBUTTONUP: /* originally ButtonRelease */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_LEFTUP;
			sys_event.key_mod = ModifierKeys();
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_RBUTTONDOWN: /* originally ButtonPress */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTBUTTON;
			sys_event.key_mod = ModifierKeys();
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_RBUTTONUP: /* originally ButtonRelease */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTUP;
			sys_event.key_mod = ModifierKeys();
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_MOUSEMOVE:
			sys_event.type    = SIM_MOUSE_MOVE;
			sys_event.code    = SIM_MOUSE_MOVED;
			sys_event.key_mod = ModifierKeys();
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_MOUSEWHEEL:
			sys_event.type = SIM_MOUSE_BUTTONS;
			sys_event.code = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
			sys_event.key_mod = ModifierKeys();
			/* the returned coordinate in LPARAM are absolute coordinates, which will deeply confuse simutrans
			 * we just reuse the coordinates we used the last time but not chaning mx/my ...
			 */
			return 0;

		case WM_SIZE: // resize client area
			if(lParam!=0) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SIM_SYSTEM_RESIZE;

				sys_event.mx = LOWORD(lParam) + 1;
				if (sys_event.mx <= 0) {
					sys_event.mx = 4;
				}

				sys_event.my = HIWORD(lParam);
				if (sys_event.my <= 1) {
					sys_event.my = 64;
				}
			}
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdcp;

			hdcp = BeginPaint(hwnd, &ps);
			AllDib->bmiHeader.biHeight = WindowSize.bottom;
			StretchDIBits(hdcp, 0, 0, WindowSize.right, WindowSize.bottom, 0, WindowSize.bottom + 1, WindowSize.right, -WindowSize.bottom, AllDibData, AllDib, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(this_hwnd, &ps);
			break;
		}

		case WM_KEYDOWN: { /* originally KeyPress */
			// check for not numlock!
			int numlock = (GetKeyState(VK_NUMLOCK) & 1) == 0;

			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			sys_event.key_mod = ModifierKeys();

			if (numlock) {
				// do low level special stuff here
				switch (wParam) {
					case VK_NUMPAD0:   sys_event.code = '0';           break;
					case VK_NUMPAD1:   sys_event.code = '1';           break;
					case VK_NUMPAD3:   sys_event.code = '3';           break;
					case VK_NUMPAD7:   sys_event.code = '7';           break;
					case VK_NUMPAD9:   sys_event.code = '9';           break;
					case VK_NUMPAD2:   sys_event.code = SIM_KEY_DOWN;  break;
					case VK_NUMPAD4:   sys_event.code = SIM_KEY_LEFT;  break;
					case VK_NUMPAD6:   sys_event.code = SIM_KEY_RIGHT; break;
					case VK_NUMPAD8:   sys_event.code = SIM_KEY_UP;    break;
					case VK_PAUSE:     sys_event.code = 16;            break;	// Pause -> ^P
					case VK_SEPARATOR: sys_event.code = 127;           break;	// delete
				}
				// check for numlock!
				if (sys_event.code != 0) break;
			}

			// do low level special stuff here
			switch (wParam) {
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
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = wParam;
			sys_event.key_mod = ModifierKeys();
			break;

		case WM_CLOSE:
			if (AllDibData != NULL) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SIM_SYSTEM_QUIT;
			}
			break;

		case WM_DESTROY:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_QUIT;
			if (AllDibData == NULL) {
				PostQuitMessage(0);
				hwnd = NULL;
			}
			break;

		default:
			return DefWindowProc(this_hwnd, msg, wParam, lParam);
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
	} while (wait && sys_event.type == SIM_NOEVENT);
}


void GetEvents()
{
	// already even processed?
	if(sys_event.type==SIM_NOEVENT) {
		internal_GetEvents(true);
	}
}


void GetEventsNoWait()
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



unsigned long dr_time(void)
{
	return timeGetTime();
}




void dr_sleep(uint32 millisec)
{
	Sleep(millisec);
}


int CALLBACK WinMain(HINSTANCE const hInstance, HINSTANCE, LPSTR, int)
{
	WNDCLASSW wc;
	bool timer_is_set = false;

	wc.lpszClassName = L"Simu";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
	wc.lpszMenuName = NULL;

	RegisterClass(&wc);

	GetWindowRect(GetDesktopWindow(), &MaxSize);

	// maybe set timer to 1ms intervall on Win2k upwards ...
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
	if(	hFlushThread ) {
		TerminateThread( hFlushThread, 0 );
	}
#endif
	return res;
}
