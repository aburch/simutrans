/*
 * simsys_s.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef __WIN32__
#error "Only Windows has GDI!"
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// windows Bibliotheken DirectDraw 5.x
#include <windows.h>
#include <wingdi.h>
#include <mmsystem.h>

// needed for wheel
#if !defined(WM_MOUSEWHEEL)
# define WM_MOUSEWHEEL                   0x020A
#endif  //WM_MOUSEWHEEL
#if !defined(GET_WHEEL_DELTA_WPARAM)
# define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD(wparam))
#endif  //GET_WHEEL_DELTA_WPARAM


#include "simmem.h"
#include "simversion.h"
#include "simsys16.h"
#include "simevent.h"

static HWND hwnd;
static MSG msg;
static RECT WindowSize;
static RECT MaxSize;
HINSTANCE hInstance;

static BITMAPINFOHEADER AllDib;
static unsigned short *AllDibData = NULL;

static HDC hdc = NULL;

/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
int dr_os_init(int n, int *parameter)
{
  return TRUE;
}




// open the window
int dr_os_open(int w, int h,int fullscreen)
{
	if(fullscreen  &&  w==MaxSize.right  &&  h==MaxSize.bottom) {
		hwnd = CreateWindowEx(WS_EX_TOPMOST	,
				SAVEGAME_PREFIX, SAVEGAME_PREFIX " " VERSION_NUMBER " - " VERSION_DATE, WS_POPUP,
				0, 0,
				w, h,
				NULL, NULL, hInstance, NULL);
		GetClientRect( hwnd, &WindowSize );
		ShowWindow (hwnd, SW_SHOW);
	}
	else {
		hwnd = CreateWindow(
					SAVEGAME_PREFIX, SAVEGAME_PREFIX " " VERSION_NUMBER " - " VERSION_DATE, WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT,
					w+6, h+25,
					NULL, NULL, hInstance, NULL);
		SetRect( &WindowSize, 0, 0, w, h-1 );
		ShowWindow (hwnd, SW_SHOW);
	}

	AllDib.biSize = sizeof(BITMAPINFOHEADER);
	AllDib.biWidth = w;
	AllDib.biHeight = h;
	AllDib.biPlanes = 1;
	AllDib.biBitCount = 16;
	AllDib.biCompression = BI_RGB;
	AllDib.biClrUsed =
	AllDib.biClrImportant = 0;

	return TRUE;
}


// shut down SDL
int dr_os_close(void)
{
	DestroyWindow( hwnd );
	return TRUE;
}



/**
 * Sound initialisation routine
 */
void dr_init_sound()
{

}



// reiszes screen
int dr_textur_resize(unsigned short **textur,int w, int h)
{
	AllDib.biWidth = w;
	AllDib.biHeight = h;
//	SetWindowPos( hwnd, NULL, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER	);
	return TRUE;
}



unsigned short *dr_textur_init()
{
	return AllDibData = (unsigned short *)malloc( MaxSize.right*2*(MaxSize.bottom+1) );
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
//	return ((r>>3)<<10) | ((g>>3)<<5) | (b>>3);
	return ((r&0x00F8)<<7) | ((g&0x00F8)<<2) | (b>>3);
}


/**
 * Does this system wrapper need software cursor?
 * @return true if a software cursor is needed
 * @author Hj. Malthaner
 */
int dr_use_softpointer()
{
    return 0;
}




void dr_flush()
{
	/*
	HDC hdc = GetDC(hwnd);
	StretchDIBits( hdc, 0, 0, WindowSize.right, WindowSize.bottom,
		0, WindowSize.bottom+1, WindowSize.right, -WindowSize.bottom,
		(LPSTR)AllDibData, (LPBITMAPINFO)&AllDib,
		DIB_RGB_COLORS, SRCCOPY	 );
	*/
	if(hdc!=NULL) {
		ReleaseDC( hwnd, hdc );
		hdc = NULL;
	}
}



void
dr_textur(int xp, int yp, int w, int h)
{
	if(hdc==NULL) {
		hdc = GetDC(hwnd);
	}
	StretchDIBits( hdc, xp, yp, w, h,
		xp, WindowSize.bottom+1-yp, w, -h,
		(LPSTR)AllDibData, (LPBITMAPINFO)&AllDib,
		DIB_RGB_COLORS, SRCCOPY	 );
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
	POINT pt={x,y};
	ClientToScreen( hwnd, &pt );
	SetCursorPos( pt.x, pt.y );
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
	return 0;
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */

struct sys_event sys_event;

/* Windows eventhandler: does most of the work */
LRESULT WINAPI WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
		case WM_LBUTTONDOWN:	/* originally ButtonPress */
			sys_event.key_mod = wParam>>2;
			sys_event.type=SIM_MOUSE_BUTTONS;
			sys_event.mx=LOWORD(lParam);
			sys_event.my=HIWORD(lParam);
			sys_event.code=SIM_MOUSE_LEFTBUTTON;
		break;
		case WM_LBUTTONUP:	/* originally ButtonRelease */
			sys_event.key_mod = wParam>>2;
			sys_event.type=SIM_MOUSE_BUTTONS;
			sys_event.mx=LOWORD(lParam);
			sys_event.my=HIWORD(lParam);
			sys_event.code=SIM_MOUSE_LEFTUP;
		break;

		case WM_RBUTTONDOWN:	/* originally ButtonPress */
			sys_event.key_mod = wParam>>2;
			sys_event.type=SIM_MOUSE_BUTTONS;
			sys_event.mx=LOWORD(lParam);
			sys_event.my=HIWORD(lParam);
			sys_event.code=SIM_MOUSE_RIGHTBUTTON;
		break;
		case WM_RBUTTONUP:	/* originally ButtonRelease */
			sys_event.key_mod = wParam>>2;
			sys_event.type=SIM_MOUSE_BUTTONS;
			sys_event.mx=LOWORD(lParam);
			sys_event.my=HIWORD(lParam);
			sys_event.code=SIM_MOUSE_RIGHTUP;
		break;

		case WM_MOUSEMOVE:
			sys_event.type=SIM_MOUSE_MOVE;
			sys_event.code= SIM_MOUSE_MOVED;
			sys_event.key_mod = wParam>>2;
			sys_event.mx=LOWORD(lParam);
			sys_event.my=HIWORD(lParam);
		break;

		case WM_MOUSEWHEEL:
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = GET_WHEEL_DELTA_WPARAM(wParam)>0 ? '<' : '>';
			return 0;

		// resize client area
		case WM_SIZE:
		{
			GetClientRect( hwnd, &WindowSize );

			sys_event.type=SIM_SYSTEM;
			sys_event.code=SIM_SYSTEM_RESIZE;

			sys_event.mx = WindowSize.right;
			if(sys_event.mx<=0) {
				sys_event.mx = 4;
			}
			WindowSize.right = sys_event.mx;

			sys_event.my = WindowSize.bottom+1;
			if(sys_event.my<=1) {
				sys_event.my = 64;
			}
			WindowSize.bottom = sys_event.my-1;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			StretchDIBits( hdc, 0, 0, WindowSize.right, WindowSize.bottom,
								0, WindowSize.bottom+1, WindowSize.right, -WindowSize.bottom,
								(LPSTR)AllDibData, (LPBITMAPINFO)&AllDib,
								DIB_RGB_COLORS, SRCCOPY	 );
			EndPaint(hwnd, &ps);
			break;
		}

		case WM_KEYDOWN:   /* originally KeyPress */
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			// read mod key state from SDL layer

			// check for not numlock!
			if((GetKeyState(VK_NUMLOCK)&1)==0) {
				// do low level special stuff here
				switch(wParam) {
					case VK_NUMPAD1:
						sys_event.code = '1';
						break;
					case VK_NUMPAD3:
						sys_event.code = '3';
						break;
					case VK_NUMPAD7:
						sys_event.code = '7';
						break;
					case VK_NUMPAD9:
						sys_event.code = '9';
						break;
					case VK_NUMPAD4:
						sys_event.code = KEY_LEFT;
						break;
					case VK_NUMPAD6:
						sys_event.code = KEY_RIGHT;
						break;
					case VK_NUMPAD8:
						sys_event.code = KEY_UP;
						break;
					case VK_SEPARATOR	:
						sys_event.code = 127;	//delete
						break;
				}
				// check for numlock!
				if(sys_event.code!=0) {
					return 0;
				}
			}

			// do low level special stuff here
			switch(wParam) {
				case VK_LEFT:
					sys_event.code = KEY_LEFT;
					break;
				case VK_RIGHT:
					sys_event.code = KEY_RIGHT;
					break;
				case VK_UP:
					sys_event.code = KEY_UP;
					break;
				case VK_DOWN:
					sys_event.code = KEY_DOWN;
					break;
				case VK_PRIOR:
					sys_event.code = '>';
					break;
				case VK_NEXT:
					sys_event.code = '<';
					break;
				case VK_F1:
					sys_event.code = SIM_F1;
					break;
				case VK_DELETE:
					sys_event.code = 127;
					break;
				case VK_HOME:
					sys_event.code = KEY_HOME;
					break;
				case VK_END:
					sys_event.code = KEY_END;
					break;
			}
			// check for numlock!
			if(sys_event.code!=0) {
				return 0;
			}
			sys_event.type = SIM_NOEVENT;
			sys_event.code = 0;
		break;

		case WM_CHAR:   /* originally KeyPress */
			sys_event.type=SIM_KEYBOARD;
			sys_event.code = wParam;
			break;

		case WM_DESTROY:
			sys_event.type=SIM_SYSTEM;
			sys_event.code=SIM_SYSTEM_QUIT;
			PostQuitMessage(0);
			return TRUE;

        default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return FALSE;
}


static void internal_GetEvents(int wait)
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	do
	{

		// wait for keybord/mouse event
		GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if(sys_event.type==SIM_NOEVENT) {
			switch(msg.message) {

				default:
					//printf("Unbekanntes Ereignis # %d!\n",msg.message);
					sys_event.type=SIM_IGNORE_EVENT;
					sys_event.code=0;
			}
		}
	} while(  wait  &&  sys_event.type==SIM_IGNORE_EVENT  );
}


void GetEvents()
{
	internal_GetEvents(TRUE);
}


void GetEventsNoWait()
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;
	if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
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


void dr_sleep(unsigned long usec)
{
	Sleep(usec>>10);
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{
	return -1;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int sample_number, int volume)
{
}



#include "system/no_midi.c"



/************************** Winmain ***********************************/



int simu_main(int argc, char **argv);


BOOL APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASS wc;
	char *argv[32], *p;
	int argc;

	wc.lpszClassName = SAVEGAME_PREFIX;
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
	argv[argc++] = SAVEGAME_PREFIX;
	p = strtok( lpCmdLine, " " );
	while (p) {
		argv[argc++] = p;
		p = strtok( NULL, " " );
	}
	argv[argc] = NULL;

	GetWindowRect(GetDesktopWindow(), &MaxSize);

	simu_main( argc, argv );

	return 0;
}
