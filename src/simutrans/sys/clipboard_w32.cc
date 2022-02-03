/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "simsys.h"

#include "../display/simgraph.h"
#include "../simdebug.h"
#include "../dataobj/translator.h"

#include <windows.h>

#include <string.h>


/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 */
void dr_copy(const char *source, size_t length)
{
	if(  OpenClipboard(NULL)  ) {
		if(  EmptyClipboard()  ) {
			// Allocate clipboard space.
			int const len = (int)length;
			int const wsize = MultiByteToWideChar(CP_UTF8, 0, source, len, NULL, 0) + 1;
			LPWSTR const wstr = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wsize * sizeof(WCHAR));

			if(  wstr != NULL  ) {
				// Convert string. NUL appended implicitly by 0ed memory.
				MultiByteToWideChar(CP_UTF8, 0, source, len, wstr, wsize);
				SetClipboardData(CF_UNICODETEXT, wstr);
			}
		}
		CloseClipboard();
	}
}


/**
 * Paste text from the clipboard
 * @param target : pointer to the insertion position in the target text
 * @param max_length : maximum number of character bytes which could be inserted
 * @return number of character bytes actually inserted -> for cursor advancing
 */
size_t dr_paste(char *target, size_t max_length)
{
	size_t insert_len = 0;

	if(  OpenClipboard(NULL)  ) {
		LPWSTR const wstr = (LPWSTR)GetClipboardData(CF_UNICODETEXT);
		if(  wstr != NULL  ) {
			// Convert entire clipboard to UTF-8.
			int const size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
			char *const str = new char[size];
			WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, size, NULL, NULL);

			// Insert string.
			size_t const str_len = strlen(str);
			insert_len = str_len <= max_length ? str_len : (size_t)utf8_get_prev_char((utf8 const *)str, max_length);
			memmove(target + insert_len, target, strlen(target) + 1);
			memcpy(target, str, insert_len);

			delete[] str;
		}
		CloseClipboard();
	}

	return insert_len;
}
