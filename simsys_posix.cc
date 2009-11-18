/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <windows.h>
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

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

#undef min
#undef max

#include "macros.h"
#include "simmain.h"
#include "simsys.h"

#include <time.h>

struct sys_event sys_event;

int dr_os_init(const int*)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;
	return TRUE;
}

int dr_query_screen_width()
{
	return 0;
}

int dr_query_screen_height()
{
	return 0;
}

// open the window
int dr_os_open(int, int, int, int)
{
	return TRUE;
}

// shut down SDL
int dr_os_close(void)
{
	return TRUE;
}

// reiszes screen
int dr_textur_resize(unsigned short** textur, int, int, int)
{
	*textur = NULL;
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
		CreateDirectoryA( buffer, NULL);
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
	return NULL;
}

unsigned int get_system_color(unsigned int, unsigned int, unsigned int)
{
	return 1;
}

void dr_setRGB8multi(int, int, unsigned char*)
{
}

void dr_prepare_flush()
{
}

void dr_flush(void)
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

int dr_screenshot(const char *)
{
	return -1;
}

static inline unsigned int ModifierKeys(void)
{
	return 0;
}

void GetEvents(void)
{
}

void GetEventsNoWait(void)
{
}

void show_pointer(int)
{
}

void ex_ord_update_mx_my()
{
}

unsigned long dr_time(void)
{
	return time(NULL);
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

bool dr_fatal_notify(const char* msg, int choices)
{
	fputs(msg, stderr);
	return choices;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
	char pathname[1024];

	// prepare commandline
	GetModuleFileNameA( GetModuleHandle(NULL), pathname, 1024 );
	argv[0] = pathname;
#else
#ifndef __BEOS__
#if defined __GLIBC__
	/* glibc has a non-standard extension */
	char* buffer2 = NULL;
#else
	char buffer2[PATH_MAX];
#endif
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
#endif
	return simu_main(argc, argv);
}
