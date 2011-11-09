#include "macros.h"
#include "simmain.h"
#include "simsys.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <direct.h>
#	include <windows.h>
#	define PATH_MAX MAX_PATH
#else
#	include <limits.h>
#	if !defined __AMIGA__ && !defined __BEOS__
#		include <unistd.h>
#	endif
#endif


struct sys_event sys_event;


void dr_mkdir(char const* const path)
{
#ifdef _WIN32
	mkdir(path);
#else
	mkdir(path, 0777);
#endif
}


char const* dr_query_homedir()
{
	static char buffer[PATH_MAX];

#if defined _WIN32
	DWORD len = PATH_MAX - 24;
	HKEY hHomeDir;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hHomeDir) != ERROR_SUCCESS)
		return 0;
	RegQueryValueExA(hHomeDir, "Personal", 0, 0, (BYTE*)buffer, &len);
	strcat(buffer,"\\Simutrans");
#elif defined __APPLE__
	sprintf(buffer, "%s/Library/Simutrans", getenv("HOME"));
#else
	sprintf(buffer, "%s/simutrans", getenv("HOME"));
#endif

	dr_mkdir(buffer);

	// create other subdirectories
#ifdef _WIN32
	strcat(buffer, "\\");
#else
	strcat(buffer, "/");
#endif
	char b2[PATH_MAX];
	sprintf(b2, "%smaps", buffer);
	dr_mkdir(b2);
	sprintf(b2, "%ssave", buffer);
	dr_mkdir(b2);
	sprintf(b2, "%sscreenshot", buffer);
	dr_mkdir(b2);

	return buffer;
}


void dr_fatal_notify(char const* const msg)
{
#ifdef _WIN32
	MessageBoxA(0, msg, "Fatal Error", MB_ICONEXCLAMATION);
#else
	fputs(msg, stderr);
#endif
}


int sysmain(int const argc, char** const argv)
{
#ifdef _WIN32
	char pathname[1024];
	GetModuleFileNameA(GetModuleHandle(0), pathname, lengthof(pathname));
	argv[0] = pathname;
#elif !defined __BEOS__
#	if defined __GLIBC__ && !defined __AMIGA__
	/* glibc has a non-standard extension */
	char* buffer2 = 0;
#	else
	char buffer2[PATH_MAX];
#	endif
#	ifndef __AMIGA__
	char buffer[PATH_MAX];
	int length = readlink("/proc/self/exe", buffer, lengthof(buffer) - 1);
	if (length != -1) {
		buffer[length] = '\0'; /* readlink() does not NUL-terminate */
		argv[0] = buffer;
	}
#	endif
	// no process file system => need to parse argv[0]
	/* should work on most unix or gnu systems */
	argv[0] = realpath(argv[0], buffer2);
#endif

	return simu_main(argc, argv);
}
