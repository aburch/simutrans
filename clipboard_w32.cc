/**
 * Copyright (c) 2010 Knightly
 *
 * Clipboard functions for copy and paste of text
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <windows.h>

#include <string.h>

#include "simsys.h"
#include "display/simgraph.h"
#include "simdebug.h"
#include "dataobj/translator.h"

/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 * @author Knightly
 */
void dr_copy(const char *source, size_t length)
{
	if(  OpenClipboard(NULL)  ) {
		if(  EmptyClipboard()  ) {
			// Allocate required global space.
			int const len = (int)length;
			int const wsize = MultiByteToWideChar(CP_UTF8, 0, source, len, NULL, 0) + 1;
			HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, wsize * sizeof(WCHAR));

			if(  hText != NULL  ) {
				// Convert and give to clipboard.
				LPWSTR const wstr = (LPWSTR)GlobalLock(hText);
				MultiByteToWideChar(CP_UTF8, 0, source, len, wstr, wsize);
				wstr[wsize - 1] = 0;
				GlobalUnlock(hText);
				SetClipboardData(CF_UNICODETEXT, hText);
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
 * @author Knightly
 */
size_t dr_paste(char *target, size_t max_length)
{
	size_t insert_len = 0;

	if(  OpenClipboard(NULL)  ) {
		HGLOBAL hText = GetClipboardData(CF_UNICODETEXT);
		if(  hText != NULL  ) {
			// Convert entire string to UTF8.
			LPWSTR const wstr = (LPWSTR)GlobalLock(hText);
			int const size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
			char *const str = new char[size];
			WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, size, NULL, NULL);

			// Insert string.
			size_t const str_len = strlen(str);
			insert_len = str_len <= max_length ? str_len : (size_t)utf8_get_prev_char((utf8 const *)str, max_length);
			memmove(target + insert_len, target, strlen(target) + 1);
			memcpy(target, str, insert_len);

			delete[] str;
			GlobalUnlock(hText);
		}
		CloseClipboard();
	}

	return insert_len;
}
