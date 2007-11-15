/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef WIN32
#error "Only Windows has GDI!"
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>

// windows Bibliotheken DirectDraw 5.x
#define UNICODE 1
#include <windows.h>
#include <winreg.h>
#include <wingdi.h>
#include <mmsystem.h>

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
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"
#include "simdebug.h"

static HWND hwnd;
static bool is_fullscreen = false;
static bool is_not_top = false;
static MSG msg;
static RECT WindowSize = { 0, 0, 0, 0 };
static RECT MaxSize;
static HINSTANCE hInstance;

static BITMAPINFOHEADER *AllDib = NULL;
static unsigned short *AllDibData = NULL;

static HDC hdc = NULL;

const wchar_t* const title =
#ifdef _MSC_VER
			L"Simutrans " WIDE_VERSION_NUMBER;
#else
			L"" SAVEGAME_PREFIX " " VERSION_NUMBER " - " VERSION_DATE;
#endif



/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
int dr_os_init(const int* parameter)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	return TRUE;
}



// query home directory
char *dr_query_homedir()
{
	static char buffer[1024];
	char b2[1060];
	DWORD len=960;
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
}



// open the window
int dr_os_open(int w, int h, int bpp, int fullscreen)
{
	// fake fullscreen
	if (fullscreen) {
		// try to force display mode and size
		DEVMODE settings;

		memset(&settings, 0, sizeof(settings));
		settings.dmSize = sizeof(settings);
		settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
#ifdef USE_16BIT_DIB
		settings.dmBitsPerPel = 16;
#else
		settings.dmBitsPerPel = 16;
#endif
		settings.dmPelsWidth  = w;
		settings.dmPelsHeight = h;
		settings.dmDisplayFrequency = 0;

		if(  ChangeDisplaySettings(&settings, CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL  &&  w!=MaxSize.right  &&  h!=MaxSize.bottom) {
			ChangeDisplaySettings( NULL, 0 );
			printf( "dr_os_open()::Could not reduce color depth to 16 Bit in fullscreen." );
			fullscreen = false;
		}
		else {
			MaxSize.right = w;
			MaxSize.bottom = h;
		}
		is_fullscreen = fullscreen;
	}
	if(  fullscreen  ) {
		hwnd = CreateWindowEx(
			WS_EX_TOPMOST,
			L"Simu", title,
			WS_POPUP,
			0, 0,
			w, h,
			NULL, NULL, hInstance, NULL
		);
	} else {
		hwnd = CreateWindow(
			L"Simu", title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			w + GetSystemMetrics(SM_CXFRAME),
			h - 1 + 2 * GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION),
			NULL, NULL, hInstance, NULL
		);
	}
	ShowWindow(hwnd, SW_SHOW);

	WindowSize.right  = w;
	WindowSize.bottom = h;

	AllDib = GlobalAlloc(GMEM_FIXED, sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3);
	AllDib->biSize = sizeof(BITMAPINFOHEADER);
	AllDib->biCompression = BI_BITFIELDS;
	*((DWORD*)(AllDib + 1) + 0) = 0x01;
	*((DWORD*)(AllDib + 1) + 1) = 0x02;
	*((DWORD*)(AllDib + 1) + 2) = 0x03;

	AllDib->biSize         = sizeof(BITMAPINFOHEADER);
	AllDib->biWidth        = w;
	AllDib->biHeight       = h;
	AllDib->biPlanes       = 1;
	AllDib->biBitCount     = 16;
	AllDib->biCompression  = BI_RGB;
	AllDib->biClrUsed      = 0;
	AllDib->biClrImportant = 0;
#ifdef USE_16BIT_DIB
	AllDib->biCompression = BI_BITFIELDS;
	*((DWORD*)(AllDib + 1) + 0) = 0x0000F800;
	*((DWORD*)(AllDib + 1) + 1) = 0x000007E0;
	*((DWORD*)(AllDib + 1) + 2) = 0x0000001F;
#endif
	return TRUE;
}


int dr_os_close(void)
{
	if (hdc != NULL) {
		ReleaseDC(hwnd, hdc);
		hdc = NULL;
	}
	if (hwnd != NULL) {
		DestroyWindow(hwnd);
	}
	if (AllDibData != NULL) {
		GlobalFree(GlobalHandle(AllDibData));
		AllDibData = NULL;
	}
	if (AllDib != NULL) {
		GlobalFree(AllDib);
		AllDib = NULL;
	}
	ChangeDisplaySettings(NULL, 0);
	return TRUE;
}


// reiszes screen
int dr_textur_resize(unsigned short **textur, int w, int h, int bpp)
{
	if(w>MaxSize.right+15  ||  h>MaxSize.bottom+2) {
		// since the query routines that return the desktop data do not take into account a change of resolution
		GlobalFree( GlobalHandle( AllDibData ) );
		AllDibData = NULL;
		MaxSize.right = (w+16) & 0xFFF0;
		MaxSize.bottom = h;
		AllDibData = (unsigned short*)GlobalLock(GlobalAlloc(GMEM_MOVEABLE, (MaxSize.right + 15) * 2 * (MaxSize.bottom + 2)));
		*textur = AllDibData;
	}
	AllDib->biWidth   = w;
	WindowSize.right  = w;
	WindowSize.bottom = h;
	return TRUE;
}


unsigned short *dr_textur_init()
{
	AllDibData = (unsigned short*)GlobalLock( GlobalAlloc(GMEM_MOVEABLE, (MaxSize.right + 15) * 2 * (MaxSize.bottom + 2)));
	return AllDibData;
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



void dr_flush()
{
	if (hdc != NULL) {
		ReleaseDC(hwnd, hdc);
		hdc = NULL;
	}
}



void dr_textur(int xp, int yp, int w, int h)
{
	if (hdc == NULL) hdc = GetDC(hwnd);
	AllDib->biHeight = h+1;
	StretchDIBits(
		hdc,
		xp, yp, w, h,
		xp, h + 1, w, -h,
		(LPSTR)(AllDibData + yp * WindowSize.right), (LPBITMAPINFO)AllDib,
		DIB_RGB_COLORS, SRCCOPY
	);
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


// try using GDIplus to save an screenshot
extern bool dr_screenshot_png(const char *filename, int w, int h, unsigned short *AllDibData );

/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
#ifdef USE_16BIT_DIB
	if(!dr_screenshot_png(filename,AllDib->biWidth,WindowSize.bottom + 1,AllDibData,16)) {
#else
	if(!dr_screenshot_png(filename,AllDib->biWidth,WindowSize.bottom + 1,AllDibData,15)) {
#endif
		// not successful => save as BMP
		FILE *fBmp = fopen(filename, "wb");
		if (fBmp) {
			BITMAPFILEHEADER bf;
			unsigned i;

			AllDib->biHeight = WindowSize.bottom + 1;

			bf.bfType = 0x4d42; //"BM"
			bf.bfReserved1 = 0;
			bf.bfReserved2 = 0;
			bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(DWORD)*3;
			bf.bfSize = (bf.bfOffBits + AllDib->biHeight * AllDib->biWidth * 2l + 3l) / 4l;
			fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, fBmp);
			fwrite(AllDib, sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3, 1, fBmp);

			for (i = 0; i < AllDib->biHeight; i++) {
				fwrite(AllDibData + (AllDib->biHeight - 1 - i) * AllDib->biWidth, AllDib->biWidth, 2, fBmp);
			}

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

struct sys_event sys_event;

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

					memset(&settings, 0, sizeof(settings));
					settings.dmSize = sizeof(settings);
					settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
#ifdef USE_16BIT_DIB
					settings.dmBitsPerPel = 16;
#else
					settings.dmBitsPerPel = 16;
#endif
					settings.dmPelsWidth  = MaxSize.right;
					settings.dmPelsHeight = MaxSize.bottom;
					settings.dmDisplayFrequency = 0;

					// should be alsway sucessful, since it worked as least once ...
					ChangeDisplaySettings(&settings, CDS_FULLSCREEN);
					is_not_top = false;

					Beep( 110, 250 );
					// must reshow window, otherwise startbar will be topmost ...
					hwnd = CreateWindowEx(
						WS_EX_TOPMOST,
						L"Simu", title,
						WS_POPUP,
						0, 0,
						MaxSize.right, MaxSize.bottom,
						NULL, NULL, hInstance, NULL
					);
					ShowWindow( hwnd, SW_SHOW );
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
			sys_event.key_mod = wParam>>2;
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_LBUTTONUP: /* originally ButtonRelease */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_LEFTUP;
			sys_event.key_mod = wParam>>2;
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_RBUTTONDOWN: /* originally ButtonPress */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTBUTTON;
			sys_event.key_mod = wParam>>2;
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_RBUTTONUP: /* originally ButtonRelease */
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.code    = SIM_MOUSE_RIGHTUP;
			sys_event.key_mod = wParam>>2;
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_MOUSEMOVE:
			sys_event.type    = SIM_MOUSE_MOVE;
			sys_event.code    = SIM_MOUSE_MOVED;
			sys_event.key_mod = wParam>>2;
			sys_event.mb = last_mb = (wParam&3);
			sys_event.mx      = LOWORD(lParam);
			sys_event.my      = HIWORD(lParam);
			break;

		case WM_MOUSEWHEEL:
			sys_event.type = SIM_MOUSE_BUTTONS;
			sys_event.code = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
			return 0;

		case WM_SIZE: // resize client area
			if(lParam!=0) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SIM_SYSTEM_RESIZE;

				sys_event.mx = LOWORD(lParam) + 1;
				if (sys_event.mx <= 0) sys_event.mx = 4;

				sys_event.my = HIWORD(lParam);
				if (sys_event.my <= 1) sys_event.my = 64;
			}
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdcp;

			if (hdc != NULL) {
				ReleaseDC(this_hwnd, hdc);
				hdc = NULL;
			}

			hdcp = BeginPaint(hwnd, &ps);
			AllDib->biHeight = WindowSize.bottom;
			StretchDIBits(
				hdcp,
				0, 0, WindowSize.right, WindowSize.bottom,
				0, WindowSize.bottom + 1, WindowSize.right, -WindowSize.bottom,
				(LPSTR)AllDibData, (LPBITMAPINFO)AllDib,
				DIB_RGB_COLORS, SRCCOPY
			);
			EndPaint(this_hwnd, &ps);
			break;
		}

		case WM_KEYDOWN: { /* originally KeyPress */
			// check for not numlock!
			int numlock = (GetKeyState(VK_NUMLOCK) & 1) == 0;

			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;

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
					case VK_SEPARATOR: sys_event.code = 127; //delete
					break;
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
				sys_event.key_mod = (GetKeyState(VK_CONTROL) != 0) * 2; // control state
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
			break;

		case WM_CLOSE:
			if (AllDibData != NULL) {
				sys_event.type = SIM_SYSTEM;
				sys_event.code = SIM_SYSTEM_QUIT;
				return FALSE;
			}
			break;

		case WM_DESTROY:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SIM_SYSTEM_QUIT;
			if (AllDibData == NULL) {
				PostQuitMessage(0);
				hwnd = NULL;
				return TRUE;
			}
			return FALSE;

		default:
			return DefWindowProc(this_hwnd, msg, wParam, lParam);
	}
	return FALSE;
}


static void internal_GetEvents(int wait)
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
		internal_GetEvents(TRUE);
	}
}


void GetEventsNoWait()
{
	if (sys_event.type==SIM_NOEVENT  &&  PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
		internal_GetEvents(FALSE);
	}
}


void show_pointer(int yesno)
{
	ShowCursor(yesno);
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



bool dr_fatal_notify(const char* msg, int choices)
{
	if(choices==0) {
		MessageBoxA( hwnd, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_OK );
		return 0;
	}
	else {
		return MessageBoxA( hwnd, msg, "Fatal Error", MB_ICONEXCLAMATION|MB_RETRYCANCEL	)==IDRETRY;
	}
}




/************************** Winmain ***********************************/

int simu_main(int argc, char **argv);


BOOL APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSW wc;
	char pathname[1024];
	char *argv[32], *p;
	int argc;

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

	GetWindowRect(GetDesktopWindow(), &MaxSize);

	simu_main(argc, argv);

	return 0;
}
