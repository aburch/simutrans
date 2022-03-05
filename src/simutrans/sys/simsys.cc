/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __HAIKU__
#include <Message.h>
#include <FindDirectory.h>
#include <Path.h>
#include <LocaleRoster.h>
#include <SupportDefs.h>
#define NO_UINT32_TYPES
#define NO_UINT64_TYPES
#endif

#include "../macros.h"
#include "../simmain.h"
#include "simsys.h"
#include "../pathes.h"
#include "../simevent.h"
#include "../utils/simstring.h"
#include "../simdebug.h"
#include "../simevent.h"

#ifdef _WIN32
#	include <locale>
#	include <windows.h>
#	include <winbase.h>
#	include <shellapi.h>
#	include <shlobj.h>
#	if !defined(__CYGWIN__)
#		include <direct.h>
#	else
#		include <sys\unistd.h>
#	endif
#	include "../simdebug.h"
#else
#	include <limits.h>
#	include <dirent.h>
#	if !defined __AMIGA__ && !defined __BEOS__
#		include <unistd.h>
#	endif
#	ifdef __ANDROID__
#		include <SDL2/SDL.h>
#	endif
#endif

#ifdef MULTI_THREAD
#include <pthread.h>
#endif

sys_event_t sys_event;


#ifdef _WIN32
/**
 * Utility class to handle conversion of UTF-8 to UTF-16 for use by Windows APIs.
 * Constructs a UTF-16 view of the current contents of a UTF-8 C string.
 */
class U16View {
private:
	LPCWSTR cu16str;
public:
	/**
	 * Constructs a UTF-16 view of a UTF-8 C string.
	 */
	U16View(char const *const u8str)
	{
		// Convert UTF-8 to UTF-16.
		int const size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8str, -1, NULL, 0);
		if(size == 0) {
			// It should never fail, since all file IO UTF8 comes either via keyborad input or filenames, i.e. a win32 api.
			// log may not even initialising at this point, so we just fail.
			dr_fatal_notify( "Unknown unicode character encountered!" );
			exit(0);
		}
		LPWSTR const u16str = new WCHAR[size];
		MultiByteToWideChar(CP_UTF8, 0, u8str, -1, u16str, size);

		cu16str = u16str;
	}

	/**
	 * Copy constructor to allow safe movement.
	 * Should also have move constructor to allow efficient movement but that requires C++11.
	 */
	U16View(U16View const& from)
	{
		LPWSTR const u16str = new WCHAR[wcslen(from.cu16str) + 1];
		wcscpy(u16str, from.cu16str);
		cu16str = u16str;
	}

	~U16View()
	{
		delete[] cu16str;
	}

	/**
	 * Typecast operator to allow implicit use as a LPCWSTR.
	 */
	operator LPCWSTR() const
	{
		return cu16str;
	}
};
#endif

// Platform specific path separators.
#ifdef _WIN32
char const PATH_SEPARATOR[] = "\\";
#else
char const PATH_SEPARATOR[] = "/";
#endif



/**
 * Get Mouse X-Position
 */
int get_mouse_x()
{
	return sys_event.mx;
}


/**
 * Get Mouse y-Position
 */
int get_mouse_y()
{
	return sys_event.my;
}



uint8 dr_get_max_threads()
{
	uint8 max_threads = 0;
#ifdef MULTI_THREAD
#if 0
	// does not work when crosscompile with mingw!
	max_threads = std::thread::hardware_concurrency();
#endif
	if(max_threads==0) {
#ifdef _WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		max_threads = (uint8)sysinfo.dwNumberOfProcessors;
#else
		max_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	}
#endif
	return max(max_threads,1);
}


int dr_mkdir(char const* const path)
{
#ifdef _WIN32
	// Perform operation.
	int const result = CreateDirectoryW(U16View(path), NULL) ? 0 : -1;

	// Translate error.
	if(result != 0) {
		DWORD const error = GetLastError();
		if(error == ERROR_ALREADY_EXISTS) {
			errno = EEXIST;
		} else if(error == ERROR_PATH_NOT_FOUND) {
			errno = ENOENT;
		}
	}

	return result;
#else
	return mkdir(path, 0777);
#endif
}



bool dr_movetotrash(const char *path)
{
#ifdef _WIN32
	// Convert to full path name to allow use of recycle bin.
	// Must be double null terminated for SHFILESTRUCT
	U16View const wpath(path);
	int const full_size = GetFullPathNameW(wpath, 0, NULL, NULL);
	wchar_t *const full_wpath = new wchar_t[full_size + 1];
	GetFullPathNameW(wpath, full_size, full_wpath, NULL);
	full_wpath[full_size] = L'\0';

	// Initalize file operation structure.
	SHFILEOPSTRUCTW  FileOp;
	FileOp.hwnd = GetActiveWindow();
	FileOp.wFunc = FO_DELETE;
	FileOp.pFrom = full_wpath;
	FileOp.pTo = NULL;
	FileOp.fFlags = FOF_ALLOWUNDO|FOF_NOCONFIRMATION;

	// Perform operation.
	int success = SHFileOperationW(&FileOp);

	delete[] full_wpath;

	BringWindowToTop(FileOp.hwnd);

	return success;
#else
	return remove(path);
#endif
}



bool dr_cantrash()
{
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}



int dr_remove(const char *path)
{
#ifdef _WIN32
	// Perform operation.
	bool success = DeleteFileW(U16View(path));

	// Translate error.
	if(!success) {
		DWORD error = GetLastError();
		if(error == ERROR_FILE_NOT_FOUND) {
			errno = ENOENT;
		} else if(error == ERROR_ACCESS_DENIED) {
			errno = EACCES;
		}
	}

	return success ? 0 : -1;
#else
	return remove(path);
#endif
}



int dr_rename(const char *existing_utf8, const char *new_utf8)
{
#ifdef _WIN32
	// Perform operation.
	bool success = MoveFileExW(U16View(existing_utf8), U16View(new_utf8), MOVEFILE_REPLACE_EXISTING);

	// Translate error.
	if(!success) {
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND) {
			errno = ENOENT;
		} else if(error == ERROR_ACCESS_DENIED) {
			errno = EACCES;
		}
	}

	return success ? 0 : -1;
#else
	remove( new_utf8 );
	return rename( existing_utf8, new_utf8 );
#endif
}

int dr_chdir(const char *path)
{
#ifdef _WIN32
	// Perform operation.
	bool success = SetCurrentDirectoryW(U16View(path));

	// Translate error.
	if(!success) {
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND) {
			errno = ENOENT;
		} else if(error == ERROR_ACCESS_DENIED) {
			errno = EACCES;
		}
	}

	return success ? 0 : -1;
#else
	return chdir(path);
#endif
}

char *dr_getcwd(char *buf, size_t size)
{
#ifdef _WIN32
	DWORD wsize = GetCurrentDirectoryW(0, NULL);
	WCHAR *const wpath = new WCHAR[wsize];
	DWORD success = GetCurrentDirectoryW(wsize, wpath);

	// Translate error.
	if(!success) {
		delete[] wpath;
		return NULL;
	}

	// Convert UTF-16 to UTF-8.
	int const convert_size = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, buf, (int)size, NULL, NULL);
	delete[] wpath;
	if(convert_size == 0) {
		return NULL;
	}

	return buf;
#else
	return getcwd(buf, size);
#endif
}

FILE *dr_fopen (const char *filename, const char *mode)
{
#ifdef _WIN32
	return _wfopen(U16View(filename), U16View(mode));
#else
	return fopen(filename, mode);
#endif
}

gzFile dr_gzopen(const char *path, const char *mode)
{
#ifdef _WIN32
	return gzopen_w(U16View(path), mode);
#else
	return gzopen(path, mode);
#endif
}

int dr_stat(const char *path, struct stat *buf)
{
#ifdef _WIN32
	return _wstat(U16View(path), (struct _stat*)buf);
#else
	return stat(path, buf);
#endif
}

char const *dr_query_homedir()
{
	static char buffer[PATH_MAX + 24];

#if defined _WIN32
	WCHAR whomedir[MAX_PATH];
	if(FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, whomedir))) {
		DWORD len = sizeof(whomedir);
		HKEY hHomeDir;
		if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hHomeDir) != ERROR_SUCCESS) {
			return NULL;
		}
		LONG const status = RegQueryValueExW(hHomeDir, L"Personal", NULL, NULL, (LPBYTE)whomedir, &len);
		RegCloseKey(hHomeDir);
		if(status != ERROR_SUCCESS) {
			return NULL;
		}
	}

	// Convert UTF-16 to UTF-8.
	int const convert_size = WideCharToMultiByte(CP_UTF8, 0, whomedir, -1, buffer, sizeof(buffer), NULL, NULL);
	if(convert_size == 0) {
		return NULL;
	}

	// Append Simutrans folder.
	char const foldername[] = "Simutrans";
	if(lengthof(buffer) < strlen(buffer) + strlen(foldername) + 2 * strlen(PATH_SEPARATOR) + 1){
		return NULL;
	}
	strcat(buffer, PATH_SEPARATOR);
	strcat(buffer, foldername);
#elif defined __APPLE__
	sprintf(buffer, "%s/Library/Simutrans", getenv("HOME"));
#elif defined __HAIKU__
	BPath userDir;
	find_directory(B_USER_DIRECTORY, &userDir);
	sprintf(buffer, "%s/simutrans", userDir.Path());
#elif defined __ANDROID__
	tstrncpy(buffer,SDL_GetPrefPath("Simutrans Team","simutrans"),lengthof(buffer));
#else
	if( getenv("XDG_DATA_HOME") == NULL ) {
		sprintf(buffer, "%s/simutrans", getenv("HOME"));
	} else {
		sprintf(buffer, "%s/simutrans", getenv("XDG_DATA_HOME"));
	}
#endif

	// create directory and subdirectories
	dr_mkdir(buffer);
#ifndef __ANDROID__
	strcat(buffer, PATH_SEPARATOR);
#endif
	return buffer;
}


char const *dr_query_installdir()
{
	static char buffer[PATH_MAX + 24];

#if defined _WIN32
	WCHAR whomedir[MAX_PATH];
	if(FAILED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, whomedir))) {
		return NULL;
	}

	// Convert UTF-16 to UTF-8.
	int const convert_size = WideCharToMultiByte(CP_UTF8, 0, whomedir, -1, buffer, sizeof(buffer), NULL, NULL);
	if(convert_size == 0) {
		return NULL;
	}

	// Append Simutrans folder.
	char const foldername[] = "Simutrans";
	if(lengthof(buffer) < strlen(buffer) + strlen(foldername) + 2 * strlen(PATH_SEPARATOR) + 1){
		return NULL;
	}
	strcat(buffer, PATH_SEPARATOR);
	strcat(buffer, foldername);
#elif defined __APPLE__
	sprintf(buffer, "/Library/Simutrans");
#elif defined __HAIKU__
	BPath userDir;
	find_directory(B_USER_DIRECTORY, &userDir);
	sprintf(buffer, "%s/simutrans", userDir.Path());
#elif defined __ANDROID__
	tstrncpy(buffer,SDL_GetPrefPath("Simutrans Team","simutrans"),lengthof(buffer));
#else
	if( getenv("XDG_DATA_HOME") == NULL ) {
		sprintf(buffer, "%s/simutrans/simutrans", getenv("HOME"));
	}
	else {
		sprintf(buffer, "%s/simutrans/simutrans", getenv("XDG_DATA_HOME"));
	}
#endif

	// create directory and subdirectories
	dr_mkdir(buffer);
#ifndef __ANDROID__
	strcat(buffer, PATH_SEPARATOR);
#endif
	return buffer;
}


const char *dr_query_fontpath(int which)
{
	static char buffer[PATH_MAX];
#ifdef _WIN32
	if(  which>0  ) {
		return NULL;
	}

	WCHAR fontdir[MAX_PATH];
	if(FAILED(SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, fontdir))) {
		wcscpy(fontdir, L"C:\\Windows\\Fonts");
	}

	// Convert UTF-16 to UTF-8.
	int const convert_size = WideCharToMultiByte(CP_UTF8, 0, fontdir, -1, buffer, sizeof(buffer), NULL, NULL);
	if(convert_size == 0) {
		return 0;
	}

	strcat(buffer, PATH_SEPARATOR);
	return buffer;
#else
	// linux has more than one path
	// sometimes there is the file "/etc/fonts/fonts.conf" and we can read it
	// however, since we cannot rely on it, we just try a few candiates

	// Apple only uses three locals, and Haiku has no standard ...
	const char *trypaths[] = {
#ifdef __APPLE__
		"~/Library/",
		"/Library/Fonts/",
		"/System/Library/Fonts/",
#elif defined __HAIKU__
		"/boot/system/non-packaged/data/fonts/",
		"/boot/home/config/non-packaged/data/fonts/",
		"~/config/non-packaged/data/fonts/",
		"/boot/system/data/fonts/ttfonts/",
#else
		"~/.fonts/",
		"~/.local/share/fonts/",
		"/usr/share/fonts/",
		"/usr/X11R6/lib/X11/fonts/",
		"/usr/local/share/fonts/",
#endif
		NULL
	};

	// since we include subdirectories (one level!) too
	static int which_offset, subdir_offset;
	if( which == 0 ) {
		which_offset = 0;
		subdir_offset = 0;
	}

	for( int i = which - which_offset; trypaths[ i ]; i++ ) {
		static char fontpath[PATH_MAX];
		if( trypaths[i][0] == '~' ) {
			// prepace with homedirectory
			snprintf( fontpath, PATH_MAX, "%s/%s", getenv("HOME"), trypaths[i]+2 );
		}
		else {
			tstrncpy( fontpath, trypaths[i], PATH_MAX );
		}
		DIR *dir = opendir(fontpath);
		if(  dir  ) {
			int j = 0;
			// look for subdirectories
			struct dirent *entry;
			while( (entry = readdir( dir )) ) {
				if( entry->d_type == DT_DIR ) {
					if( ((strcmp( entry->d_name, "." )) != 0) && ((strcmp( entry->d_name, ".." )) != 0) ) {
						j++;
						if( subdir_offset < j ) {
							strcpy( buffer, fontpath );
							strcat( buffer, entry->d_name );
							strcat( buffer, PATH_SEPARATOR );
							closedir( dir );
							subdir_offset++;
							which_offset++;
							return buffer;
						}
					}
				}
			}
			// last return parent folder
			closedir( dir );
			subdir_offset = 0; // we do not increase which_offset, so next the the next folder will be searched
			return fontpath;
		}
		which_offset--;
	}
	return NULL;
#endif
}


/* this retrieves the 2 byte string for the default language
 * I hope AmigaOS uses something like Linux for this ...
 */
#ifdef _WIN32

struct id2str_t { uint16 id; const char *name; };

const char *dr_get_locale_string()
{
	static id2str_t id2str[] =
	{
		{ 0x0001, "ar" },
		{ 0x0002, "bg" },
		{ 0x0003, "ca" },
		{ 0x0004, "zh\0Hans" },
		{ 0x0005, "cs" },
		{ 0x0006, "da" },
		{ 0x0007, "de" },
		{ 0x0008, "el" },
		{ 0x0009, "en" },
		{ 0x000a, "es" },
		{ 0x000b, "fi" },
		{ 0x000c, "fr" },
		{ 0x000d, "he" },
		{ 0x000e, "hu" },
		{ 0x000f, "is" },
		{ 0x0010, "it" },
		{ 0x0011, "ja" },
		{ 0x0012, "ko" },
		{ 0x0013, "nl" },
		{ 0x0014, "no" },
		{ 0x0015, "pl" },
		{ 0x0016, "pt" },
		{ 0x0017, "rm" },
		{ 0x0018, "ro" },
		{ 0x0019, "ru" },
		{ 0x001a, "hr" },
		{ 0x001b, "sk" },
		{ 0x001c, "sq" },
		{ 0x001d, "sv" },
		{ 0x001e, "th" },
		{ 0x001f, "tr" },
		{ 0x0020, "ur" },
		{ 0x0021, "id" },
		{ 0x0022, "uk" },
		{ 0x0023, "be" },
		{ 0x0024, "sl" },
		{ 0x0025, "et" },
		{ 0x0026, "lv" },
		{ 0x0027, "lt" },
		{ 0x0028, "tg" },
		{ 0x0029, "fa" },
		{ 0x002a, "vi" },
		{ 0x002b, "hy" },
		{ 0x002c, "az" },
		{ 0x002d, "eu" },
		{ 0x002e, "hsb" },
		{ 0x002f, "mk" },
		{ 0x0030, "st" },
		{ 0x0031, "ts" },
		{ 0x0032, "tn" },
		{ 0x0033, "ve" },
		{ 0x0034, "xh" },
		{ 0x0035, "zu" },
		{ 0x0036, "af" },
		{ 0x0037, "ka" },
		{ 0x0038, "fo" },
		{ 0x0039, "hi" },
		{ 0x003a, "mt" },
		{ 0x003b, "se" },
		{ 0x003c, "ga" },
		{ 0x003d, "yi" },
		{ 0x003e, "ms" },
		{ 0x003f, "kk" },
		{ 0x0040, "ky" },
		{ 0x0041, "sw" },
		{ 0x0042, "tk" },
		{ 0x0043, "uz" },
		{ 0x0044, "tt" },
		{ 0x0045, "bn" },
		{ 0x0046, "pa" },
		{ 0x0047, "gu" },
		{ 0x0048, "or" },
		{ 0x0049, "ta" },
		{ 0x004a, "te" },
		{ 0x004b, "kn" },
		{ 0x004c, "ml" },
		{ 0x004d, "as" },
		{ 0x004e, "mr" },
		{ 0x004f, "sa" },
		{ 0x0050, "mn" },
		{ 0x0051, "bo" },
		{ 0x0052, "cy" },
		{ 0x0053, "km" },
		{ 0x0054, "lo" },
		{ 0x0055, "my" },
		{ 0x0056, "gl" },
		{ 0x0057, "kok" },
		{ 0x0058, "mni" },
		{ 0x0059, "sd" },
		{ 0x005a, "syr" },
		{ 0x005b, "si" },
		{ 0x005c, "chr" },
		{ 0x005d, "iu" },
		{ 0x005e, "am" },
		{ 0x005f, "tzm" },
		{ 0x0060, "ks" },
		{ 0x0061, "ne" },
		{ 0x0062, "fy" },
		{ 0x0063, "ps" },
		{ 0x0064, "fil" },
		{ 0x0065, "dv" },
		{ 0x0066, "bin" },
		{ 0x0067, "ff" },
		{ 0x0068, "ha" },
		{ 0x0069, "ibb" },
		{ 0x006a, "yo" },
		{ 0x006b, "quz" },
		{ 0x006c, "nso" },
		{ 0x006d, "ba" },
		{ 0x006e, "lb" },
		{ 0x006f, "kl" },
		{ 0x0070, "ig" },
		{ 0x0071, "kr" },
		{ 0x0072, "om" },
		{ 0x0073, "ti" },
		{ 0x0074, "gn" },
		{ 0x0075, "haw" },
		{ 0x0076, "la" },
		{ 0x0077, "so" },
		{ 0x0078, "ii" },
		{ 0x0079, "pap" },
		{ 0x007a, "arn" },
		{ 0x007c, "moh" },
		{ 0x007e, "br" },
		{ 0x0080, "ug" },
		{ 0x0081, "mi" },
		{ 0x0082, "oc" },
		{ 0x0083, "co" },
		{ 0x0084, "gsw" },
		{ 0x0085, "sah" },
		{ 0x0086, "qut" },
		{ 0x0087, "rw" },
		{ 0x0088, "wo" },
		{ 0x008c, "prs" },
		{ 0x0091, "gd" },
		{ 0x0092, "ku" },
		{ 0x0093, "quc" },
		{ 0x0401, "ar\0SA" },
		{ 0x0402, "bg\0BG" },
		{ 0x0403, "ca\0ES" },
		{ 0x0404, "zh\0TW" },
		{ 0x0405, "cs\0CZ" },
		{ 0x0406, "da\0DK" },
		{ 0x0407, "de\0DE" },
		{ 0x0408, "el\0GR" },
		{ 0x0409, "en\0US" },
		{ 0x040a, "es\0ES_tradnl" },
		{ 0x040b, "fi\0FI" },
		{ 0x040c, "fr\0FR" },
		{ 0x040d, "he\0IL" },
		{ 0x040e, "hu\0HU" },
		{ 0x040f, "is\0IS" },
		{ 0x0410, "it\0IT" },
		{ 0x0411, "ja\0JP" },
		{ 0x0412, "ko\0KR" },
		{ 0x0413, "nl\0NL" },
		{ 0x0414, "nb\0NO" },
		{ 0x0415, "pl\0PL" },
		{ 0x0416, "pt\0BR" },
		{ 0x0417, "rm\0CH" },
		{ 0x0418, "ro\0RO" },
		{ 0x0419, "ru\0RU" },
		{ 0x041a, "hr\0HR" },
		{ 0x041b, "sk\0SK" },
		{ 0x041c, "sq\0AL" },
		{ 0x041d, "sv\0SE" },
		{ 0x041e, "th\0TH" },
		{ 0x041f, "tr\0TR" },
		{ 0x0420, "ur\0PK" },
		{ 0x0421, "id\0ID" },
		{ 0x0422, "uk\0UA" },
		{ 0x0423, "be\0BY" },
		{ 0x0424, "sl\0SI" },
		{ 0x0425, "et\0EE" },
		{ 0x0426, "lv\0LV" },
		{ 0x0427, "lt\0LT" },
		{ 0x0428, "tg\0Cyrl\0TJ" },
		{ 0x0429, "fa\0IR" },
		{ 0x042a, "vi\0VN" },
		{ 0x042b, "hy\0AM" },
		{ 0x042c, "az\0Latn\0AZ" },
		{ 0x042d, "eu\0ES" },
		{ 0x042e, "hsb\0DE" },
		{ 0x042f, "mk\0MK" },
		{ 0x0430, "st\0ZA" },
		{ 0x0431, "ts\0ZA" },
		{ 0x0432, "tn\0ZA" },
		{ 0x0433, "ve\0ZA" },
		{ 0x0434, "xh\0ZA" },
		{ 0x0435, "zu\0ZA" },
		{ 0x0436, "af\0ZA" },
		{ 0x0437, "ka\0GE" },
		{ 0x0438, "fo\0FO" },
		{ 0x0439, "hi\0IN" },
		{ 0x043a, "mt\0MT" },
		{ 0x043b, "se\0NO" },
		{ 0x043d, "yi\0Hebr" },
		{ 0x043e, "ms\0MY" },
		{ 0x043f, "kk\0KZ" },
		{ 0x0440, "ky\0KG" },
		{ 0x0441, "sw\0KE" },
		{ 0x0442, "tk\0TM" },
		{ 0x0443, "uz\0Latn\0UZ" },
		{ 0x0444, "tt\0RU" },
		{ 0x0445, "bn\0IN" },
		{ 0x0446, "pa\0IN" },
		{ 0x0447, "gu\0IN" },
		{ 0x0448, "or\0IN" },
		{ 0x0449, "ta\0IN" },
		{ 0x044a, "te\0IN" },
		{ 0x044b, "kn\0IN" },
		{ 0x044c, "ml\0IN" },
		{ 0x044d, "as\0IN" },
		{ 0x044e, "mr\0IN" },
		{ 0x044f, "sa\0IN" },
		{ 0x0450, "mn\0MN" },
		{ 0x0451, "bo\0CN" },
		{ 0x0452, "cy\0GB" },
		{ 0x0453, "km\0KH" },
		{ 0x0454, "lo\0LA" },
		{ 0x0455, "my\0MM" },
		{ 0x0456, "gl\0ES" },
		{ 0x0457, "kok\0IN" },
		{ 0x0458, "mni\0IN" },
		{ 0x0459, "sd\0Deva\0IN" },
		{ 0x045a, "syr\0SY" },
		{ 0x045b, "si\0LK" },
		{ 0x045c, "chr\0Cher\0US" },
		{ 0x045d, "iu\0Cans\0CA" },
		{ 0x045e, "am\0ET" },
		{ 0x045f, "tzm\0Arab\0MA" },
		{ 0x0460, "ks\0Arab" },
		{ 0x0461, "ne\0NP" },
		{ 0x0462, "fy\0NL" },
		{ 0x0463, "ps\0AF" },
		{ 0x0464, "fil\0PH" },
		{ 0x0465, "dv\0MV" },
		{ 0x0466, "bin\0NG" },
		{ 0x0467, "fuv\0NG" },
		{ 0x0468, "ha\0Latn\0NG" },
		{ 0x0469, "ibb\0NG" },
		{ 0x046a, "yo\0NG" },
		{ 0x046b, "quz\0BO" },
		{ 0x046c, "nso\0ZA" },
		{ 0x046d, "ba\0RU" },
		{ 0x046e, "lb\0LU" },
		{ 0x046f, "kl\0GL" },
		{ 0x0470, "ig\0NG" },
		{ 0x0471, "kr\0NG" },
		{ 0x0472, "om\0Ethi\0ET" },
		{ 0x0473, "ti\0ET" },
		{ 0x0474, "gn\0PY" },
		{ 0x0475, "haw\0US" },
		{ 0x0476, "la\0Latn" },
		{ 0x0477, "so\0SO" },
		{ 0x0478, "ii\0CN" },
		{ 0x0479, "pap\0029" },
		{ 0x047a, "arn\0CL" },
		{ 0x047c, "moh\0CA" },
		{ 0x047e, "br\0FR" },
		{ 0x0480, "ug\0CN" },
		{ 0x0481, "mi\0NZ" },
		{ 0x0482, "oc\0FR" },
		{ 0x0483, "co\0FR" },
		{ 0x0484, "gsw\0FR" },
		{ 0x0485, "sah\0RU" },
		{ 0x0486, "qut\0GT" },
		{ 0x0487, "rw\0RW" },
		{ 0x0488, "wo\0SN" },
		{ 0x048c, "prs\0AF" },
		{ 0x048d, "plt\0MG" },
		{ 0x048e, "zh\0yue\0HK" },
		{ 0x048f, "tdd\0Tale\0CN" },
		{ 0x0490, "khb\0Talu\0CN" },
		{ 0x0491, "gd\0GB" },
		{ 0x0492, "ku\0Arab\0IQ" },
		{ 0x0493, "quc\0CO" },
		{ 0x0501, "qps\0ploc" },
		{ 0x05fe, "qps\0ploca" },
		{ 0x0801, "ar\0IQ" },
		{ 0x0803, "ca\0ES\0valencia" },
		{ 0x0804, "zh\0CN" },
		{ 0x0807, "de\0CH" },
		{ 0x0809, "en\0GB" },
		{ 0x080a, "es\0MX" },
		{ 0x080c, "fr\0BE" },
		{ 0x0810, "it\0CH" },
		{ 0x0811, "ja\0Ploc\0JP" },
		{ 0x0813, "nl\0BE" },
		{ 0x0814, "nn\0NO" },
		{ 0x0816, "pt\0PT" },
		{ 0x0818, "ro\0MO" },
		{ 0x0819, "ru\0MO" },
		{ 0x081a, "sr\0Latn\0CS" },
		{ 0x081d, "sv\0FI" },
		{ 0x0820, "ur\0IN" },
		{ 0x082c, "az\0Cyrl\0AZ" },
		{ 0x082e, "dsb\0DE" },
		{ 0x0832, "tn\0BW" },
		{ 0x083b, "se\0SE" },
		{ 0x083c, "ga\0IE" },
		{ 0x083e, "ms\0BN" },
		{ 0x0843, "uz\0Cyrl\0UZ" },
		{ 0x0845, "bn\0BD" },
		{ 0x0846, "pa\0Arab\0PK" },
		{ 0x0849, "ta\0LK" },
		{ 0x0850, "mn\0Mong\0CN" },
		{ 0x0851, "bo\0BT" },
		{ 0x0859, "sd\0Arab\0PK" },
		{ 0x085d, "iu\0Latn\0CA" },
		{ 0x085f, "tzm\0Latn\0DZ" },
		{ 0x0860, "ks\0Deva" },
		{ 0x0861, "ne\0IN" },
		{ 0x0867, "ff\0Latn\0SN" },
		{ 0x086b, "quz\0EC" },
		{ 0x0873, "ti\0ER" },
		{ 0x09ff, "qps\0plocm" },
		{ 0x0c01, "ar\0EG" },
		{ 0x0c04, "zh\0HK" },
		{ 0x0c07, "de\0AT" },
		{ 0x0c09, "en\0AU" },
		{ 0x0c0a, "es\0ES" },
		{ 0x0c0c, "fr\0CA" },
		{ 0x0c1a, "sr\0Cyrl\0CS" },
		{ 0x0c3b, "se\0FI" },
		{ 0x0c5f, "tmz\0MA" },
		{ 0x0c6b, "quz\0PE" },
		{ 0x1001, "ar\0LY" },
		{ 0x1004, "zh\0SG" },
		{ 0x1007, "de\0LU" },
		{ 0x1009, "en\0CA" },
		{ 0x100a, "es\0GT" },
		{ 0x100c, "fr\0CH" },
		{ 0x101a, "hr\0BA" },
		{ 0x103b, "smj\0NO" },
		{ 0x1401, "ar\0DZ" },
		{ 0x1404, "zh\0MO" },
		{ 0x1407, "de\0LI" },
		{ 0x1409, "en\0NZ" },
		{ 0x140a, "es\0CR" },
		{ 0x140c, "fr\0LU" },
		{ 0x141a, "bs\0Latn\0BA" },
		{ 0x143b, "smj\0SE" },
		{ 0x1801, "ar\0MA" },
		{ 0x1809, "en\0IE" },
		{ 0x180a, "es\0PA" },
		{ 0x180c, "fr\0MC" },
		{ 0x181a, "sr\0Latn\0BA" },
		{ 0x183b, "sma\0NO" },
		{ 0x1c01, "ar\0TN" },
		{ 0x1c09, "en\0ZA" },
		{ 0x1c0a, "es\0DO" },
		{ 0x1c1a, "sr\0Cyrl\0BA" },
		{ 0x1c3b, "sma\0SE" },
		{ 0x2001, "ar\0OM" },
		{ 0x2009, "en\0JM" },
		{ 0x200a, "es\0VE" },
		{ 0x200c, "fr\0RE" },
		{ 0x201a, "bs\0Cyrl\0BA" },
		{ 0x203b, "sms\0FI" },
		{ 0x2401, "ar\0YE" },
		{ 0x2409, "en\0029" },
		{ 0x240a, "es\0CO" },
		{ 0x240c, "fr\0CG" },
		{ 0x241a, "sr\0Latn\0RS" },
		{ 0x243b, "smn\0FI" },
		{ 0x2801, "ar\0SY" },
		{ 0x2809, "en\0BZ" },
		{ 0x280a, "es\0PE" },
		{ 0x280c, "fr\0SN" },
		{ 0x281a, "sr\0Cyrl\0RS" },
		{ 0x2c01, "ar\0JO" },
		{ 0x2c09, "en\0TT" },
		{ 0x2c0a, "es\0AR" },
		{ 0x2c0c, "fr\0CM" },
		{ 0x2c1a, "sr\0Latn\0ME" },
		{ 0x3001, "ar\0LB" },
		{ 0x3009, "en\0ZW" },
		{ 0x300a, "es\0EC" },
		{ 0x300c, "fr\0CI" },
		{ 0x301a, "sr\0Cyrl\0ME" },
		{ 0x3401, "ar\0KW" },
		{ 0x3409, "en\0PH" },
		{ 0x340a, "es\0CL" },
		{ 0x340c, "fr\0ML" },
		{ 0x3801, "ar\0AE" },
		{ 0x3809, "en\0ID" },
		{ 0x380a, "es\0UY" },
		{ 0x380c, "fr\0MA" },
		{ 0x3c01, "ar\0BH" },
		{ 0x3c09, "en\0HK" },
		{ 0x3c0a, "es\0PY" },
		{ 0x3c0c, "fr\0HT" },
		{ 0x4001, "ar\0QA" },
		{ 0x4009, "en\0IN" },
		{ 0x400a, "es\0BO" },
		{ 0x4401, "ar\0Ploc\0SA" },
		{ 0x4409, "en\0MY" },
		{ 0x440a, "es\0SV" },
		{ 0x4801, "ar\0145" },
		{ 0x4809, "en\0SG" },
		{ 0x480a, "es\0HN" },
		{ 0x4c09, "en\0AE" },
		{ 0x4c0a, "es\0NI" },
		{ 0x5009, "en\0BH" },
		{ 0x500a, "es\0PR" },
		{ 0x5409, "en\0EG" },
		{ 0x540a, "es\0US" },
		{ 0x5809, "en\0JO" },
		{ 0x5c09, "en\0KW" },
		{ 0x6009, "en\0TR" },
		{ 0x6409, "en\0YE" },
		{ 0x641a, "bs\0Cyrl" },
		{ 0x681a, "bs\0Latn" },
		{ 0x6c1a, "sr\0Cyrl" },
		{ 0x701a, "sr\0Latn" },
		{ 0x703b, "smn" },
		{ 0x742c, "az\0Cyrl" },
		{ 0x743b, "sms" },
		{ 0x7804, "zh" },
		{ 0x7814, "nn" },
		{ 0x781a, "bs" },
		{ 0x782c, "az\0Latn" },
		{ 0x783b, "sma" },
		{ 0x7843, "uz\0Cyrl" },
		{ 0x7850, "mn\0Cyrl" },
		{ 0x785d, "iu\0Cans" },
		{ 0x7c04, "zh\0Hant" },
		{ 0x7c14, "nb" },
		{ 0x7c1a, "sr" },
		{ 0x7c28, "tg\0Cyrl" },
		{ 0x7c2e, "dsb" },
		{ 0x7c3b, "smj" },
		{ 0x7c43, "uz\0Latn" },
		{ 0x7c46, "pa\0Arab" },
		{ 0x7c50, "mn\0Mong" },
		{ 0x7c59, "sd\0Arab" },
		{ 0x7c5c, "chr\0Cher" },
		{ 0x7c5d, "iu\0Latn" },
		{ 0x7c5f, "tzm\0Latn" },
		{ 0x7c67, "ff\0Latn" },
		{ 0x7c68, "ha\0Latn" },
		{ 0x7c92, "ku\0Arab" }
	};
	const uint16 current_id = (0x000000FFFFul & GetThreadLocale());

	for(  size_t i=0;  i<lengthof(id2str)  &&  id2str[i].id<=current_id;  i++  ) {
		if(  id2str[i].id == current_id  ) {
			return id2str[i].name;
		}
	}
	return NULL;
}
#elif defined(__HAIKU__) && __HAIKU__
#include <Message.h>
#include <LocaleRoster.h>

const char *dr_get_locale_string()
{
	static char code[4];
	BMessage result;
	const char *str;
	code[0] = 0;
	if(  B_OK == BLocaleRoster::Default()->GetPreferredLanguages( &result )  ) {
		result.FindString( (const char *)"language", &str );
		for(  int i=0;  i<lengthof(code)-1  &&  isalpha(str[i]);  i++  ) {
			code[i] = tolower(str[i]);
			code[i+1] = 0;
		}
	}
	return code[0] ? code : NULL;
}
#else
#include <locale.h>

const char *dr_get_locale_string()
{
	static char code[4];
	char *ptr;
	setlocale( LC_ALL, "" );
	ptr = setlocale( LC_ALL, NULL );
	code[0] = 0;
	for(  int i=0;  i<(int)lengthof(code)-1  &&  isalpha(ptr[i]);  i++  ) {
		code[i] = tolower(ptr[i]);
		code[i+1] = 0;
	}
	setlocale( LC_ALL, "C" ); // or the number output may be broken
	return code[0] ? code : NULL;
}
#endif


void dr_fatal_notify(char const* const msg)
{
	fprintf(stderr, "dr_fatal_notify: ERROR: %s\n", msg);

#ifdef _WIN32
	MessageBoxA(0, msg, "Fatal Error", MB_ICONEXCLAMATION);
#endif

#ifdef __ANDROID__
	dbg->error(__FUNCTION__, msg);
#endif
}

/**
 * Open a program/starts a script to download pak sets from sourceforge
 * @param data_dir : actual simutrans pakfile directory
 * @return false, if nothing was downloaded
 */
bool dr_download_pakset( const char *data_dir, bool portable )
{
#ifdef _WIN32
	int old_fullscreen = dr_suspend_fullscreen();

	U16View const wpath_to_program(data_dir);
	WCHAR wparam[1024];
	swprintf(wparam, portable ? L"/P /D=%ls" : L"/D=%ls", wpath_to_program);


	SHELLEXECUTEINFOW shExInfo;
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	/* since the installer will run again a child process
	 * using "runas" will make sure it is already privileged.
	 * Otherwise the waiting for installation will fail!
	 * (no idea how this works on XP though)
	 */
	shExInfo.lpVerb = L"runas";
	shExInfo.lpFile = L"download-paksets.exe";
	shExInfo.lpParameters = wparam;
	shExInfo.lpDirectory = wpath_to_program;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if(  ShellExecuteExW(&shExInfo)  ) {
		WaitForSingleObject( shExInfo.hProcess, INFINITE );
		CloseHandle( shExInfo.hProcess );
	}
	dr_restore_fullscreen(old_fullscreen);
	return true;
#else
	(void)portable;

	char command[2048];
	dr_chdir( data_dir );
	sprintf(command, "%s/get_pak.sh", data_dir);
	const int retval = system( command );
	dbg->debug("dr_download_pakset", "Command '%s' returned %d", command, retval);
	return true;
#endif
}


int sysmain(int const argc, char** const argv)
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

#ifdef _WIN32
	// Guess required buffer length, if too small then dynamically expand up to around 65536 characters.
	// It is unlikely this loop will ever run more than once in practice but is required to cover flaws with the API.
	size_t buffersize = MAX_PATH;
	WCHAR *wpathname;
	while(true) {
		wpathname = new WCHAR[buffersize];
		DWORD result = GetModuleFileNameW(GetModuleHandleW(0), wpathname, buffersize);
		if(result < buffersize  ||  buffersize >= 1 << 16) {
			break;
		}
		delete[] wpathname;
		buffersize<<= 1;
	}

	// Convert UTF-16 to UTF-8
	int const pathnamesize = WideCharToMultiByte(CP_UTF8, 0, wpathname, -1, NULL, 0, NULL, NULL);
	char *const pathname = new char[pathnamesize];
	WideCharToMultiByte(CP_UTF8, 0, wpathname, -1, pathname, pathnamesize, NULL, NULL);
	delete[] wpathname;

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
	ssize_t const length = readlink("/proc/self/exe", buffer, lengthof(buffer) - 1);
	if (length != -1) {
		buffer[length] = '\0'; /* readlink() does not NUL-terminate */
		argv[0] = buffer;
	} else if (strchr(argv[0], '/') == NULL) {
		// no /proc, no '/' in argv[0] => search PATH
		const char* path = getenv("PATH");
		if (path != NULL) {
			size_t flen = strlen(argv[0]);
			for (;;) { /* for each directory in PATH */
				size_t dlen = strcspn(path, ":");
				if (dlen > 0 && dlen + flen + 2 < lengthof(buffer)) {
					// buffer = dir '/' argv[0] '\0'
					memcpy(buffer, path, dlen);
					buffer[dlen] = '/';
					memcpy(buffer + dlen + 1, argv[0], flen + 1);
					if (access(buffer, X_OK) == 0) {
						argv[0] = buffer;
						break; /* found it! */
					}
				}
				if (path[dlen] == '\0')
					break;
				path += dlen + 1; /* skip ':' */
			}
		}
	}
#	endif
	// no process file system => need to parse argv[0]
	/* should work on most unix or gnu systems */
	argv[0] = realpath(argv[0], buffer2);
#endif

	setlocale( LC_CTYPE, "" );
	setlocale( LC_NUMERIC, "en_US.UTF-8" );
	return simu_main(argc, argv);

#ifdef _WIN32
	// Cleanup for dynamic allocation, probably unnescescary.
	delete[] pathname;
#endif
}
