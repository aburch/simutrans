#include "simsys.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#include <windows.h>
#	define PATH_MAX 1024
#else
#	include <limits.h>
#endif


static void dr_mkdir(char const* const path)
{
#ifdef _WIN32
	CreateDirectoryA(path, 0);
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
