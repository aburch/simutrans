/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#else
#include <WinSock2.h> // For timeval.

// Simple system time adapter. Timezone not supported as never used.
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// File time object used for standard time reference.
	FILETIME time;

	// Converter used to convert time into comparable units.
	ULARGE_INTEGER converter;

	// The comparable time for output.
	ULONGLONG gtodtime = 0;

	// Start of epoch (January 1, 1970), defined by opengroup and after FILETIME epoch (January 1, 1601).
	// Should be constant if allowed.
	SYSTEMTIME EPOCH;
	EPOCH.wDay = 1;
	EPOCH.wDayOfWeek = 4;
	EPOCH.wHour = 0;
	EPOCH.wMilliseconds = 0;
	EPOCH.wMinute = 0;
	EPOCH.wMonth = 1;
	EPOCH.wSecond = 0;
	EPOCH.wYear = 1970;

	// Convert epoch to file time then to comparable units.
	SystemTimeToFileTime(&EPOCH, &time);
	converter.LowPart = time.dwLowDateTime;
	converter.HighPart = time.dwHighDateTime;
	gtodtime-= converter.QuadPart; // Underflow does not mater as overflow corrects.

	// Get the current time. The accuracy is OS dependant with newer OS versions being more accurate.
#if WINVER >= _WIN32_WINNT_WIN8
	GetSystemTimePreciseAsFileTime(&time) // Microsecond accurate, required accuracy.
#else
	GetSystemTimeAsFileTime(&time); // Sub-optimal accuracy but has to do.
#endif
	// Convert current time to comparable units.
	converter.LowPart = time.dwLowDateTime;
	converter.HighPart = time.dwHighDateTime;
	gtodtime+= converter.QuadPart;

	// Drop precision from "100-nanosecond intervals" to 1us intervals.
	gtodtime/= 10;

	// Fill timeval structure to meet specification.
	tv->tv_usec = (long)(gtodtime % 1000000lu);
	tv->tv_sec = (long)(gtodtime / 1000000lu);

	// Always "success" as Windows does not give any errors doing this.
	return 0;
}
#endif

#include <signal.h>

#include "macros.h"
#include "simdebug.h"
#include "simevent.h"
#include "simsys.h"


static bool sigterm_received = false;

bool dr_os_init(const int*)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;
	return true;
}

resolution dr_query_screen_resolution()
{
	resolution const res = { 0, 0 };
	return res;
}

// open the window
int dr_os_open(int, int, int)
{
	return 1;
}


void dr_os_close()
{
}

// reiszes screen
int dr_textur_resize(unsigned short** const textur, int, int)
{
	*textur = NULL;
	return 1;
}


unsigned short *dr_textur_init()
{
	return NULL;
}

unsigned int get_system_color(unsigned int, unsigned int, unsigned int)
{
	return 1;
}

void dr_prepare_flush()
{
}

void dr_flush()
{
}

void dr_textur(int, int, int, int)
{
}

void move_pointer(int, int)
{
}

void set_pointer(int)
{
}

int dr_screenshot(const char *,int,int,int,int)
{
	return -1;
}

static inline unsigned int ModifierKeys()
{
	return 0;
}

void GetEvents()
 {
	if(  sigterm_received  ) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}
 }


void GetEventsNoWait()
{
	if(  sigterm_received  ) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}
}

void show_pointer(int)
{
}

void ex_ord_update_mx_my()
{
}

static timeval first;

uint32 dr_time()
{
	timeval second;
	gettimeofday(&second,NULL);
	if (first.tv_usec > second.tv_usec) {
		// since those are often unsigned
		second.tv_usec += 1000000;
		second.tv_sec--;
	}

	return (second.tv_sec - first.tv_sec)*1000ul + (second.tv_usec - first.tv_usec)/1000ul;
}

void dr_sleep(uint32 msec)
{
/*
	// this would be 100% POSIX but is usually not very accurate ...
	if(  msec>0  ) {
		struct timeval tv;
		tv.sec = 0;
		tv.usec = msec*1000;
		select(0, 0, 0, 0, &tv);
	}
*/
#ifdef _WIN32
	Sleep( msec );
#else
	sleep( msec );
#endif
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

static void posix_sigterm(int)
{
	dbg->important("Received SIGTERM, exiting...");
	sigterm_received = 1;
}


int main(int argc, char **argv)
 {
	signal( SIGTERM, posix_sigterm );
 	gettimeofday(&first,NULL);
 	return sysmain(argc, argv);
 }
