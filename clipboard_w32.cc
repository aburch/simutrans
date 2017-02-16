/**
 * Copyright (c) 2010 Knightly
 *
 * Clipboard functions for copy and paste of text
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#define NOMINMAX 1
#include <windows.h>

#include <string.h>

#include "simsys.h"
#include "display/simgraph.h"
#include "simdebug.h"
#include "dataobj/translator.h"


#define MAX_SIZE (4096)
static char buffer[MAX_SIZE] = "";


/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 * @author Knightly
 */
void dr_copy(const char *source, size_t length)
{
	// convert utf-8 strings into utf-16
	assert( (length<<1)<MAX_SIZE );
	utf16 *utf16_buffer = (utf16 *)buffer;
	size_t utf16_count = 0;
	utf16 char_code;
	size_t offset = 0;
	while(  (char_code=utf8_to_utf16((const utf8*)(source+offset), &offset))  &&  offset<=length  ) {
		*utf16_buffer++ = char_code;
		++utf16_count;
	}
	*utf16_buffer = 0;
	source = buffer;
	length = (utf16_count << 1) + 1;

	// create a memory block and copy the text
	HGLOBAL hText;	// handle to the memory block for holding the text
	char *pText;	// pointer to the start of the memory block
	if(  (hText=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,length+1))  &&  (pText=(char*)GlobalLock(hText))  ) {
		while(  (length--)>0  ) {
			*pText++ = *source++;
		}
		*pText = 0;
		GlobalUnlock(hText);
	}
	else {
		if(  hText  ) {
			GlobalFree(hText);
		}
		return;
	}

	// open the clipboard and set the clipboard data
	if(  OpenClipboard(NULL)  ) {
		if(  EmptyClipboard()  ) {
			SetClipboardData(CF_UNICODETEXT, hText);
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
	size_t inserted_length = 0;

	HGLOBAL hText;	// handle to the memory block for holding the text
	const char *pText;	// pointer to the start of the memory block
	if(  OpenClipboard(NULL)  ) {
		if(  (hText=GetClipboardData(CF_UNICODETEXT))  &&  (pText=(const char*)GlobalLock(hText))  ) {
			// convert clipboard text from utf-16 to utf-8
			const utf16 *utf16_text = (const utf16 *)pText;
			utf8 *utf8_buffer = (utf8 *)buffer;
			size_t tmp_length = 0;
			size_t byte_count;
			while(  *utf16_text  &&  tmp_length+(byte_count=utf16_to_utf8(*utf16_text, utf8_buffer))<=max_length  ) {
				tmp_length += byte_count;
				utf8_buffer += byte_count;
				++utf16_text;
			}
			pText = buffer;
			max_length = tmp_length;

			inserted_length = max_length;

			// move the text to free up space for inserting the clipboard content
			char *target_old_end = target + strlen(target);
			char *target_new_end = target_old_end + max_length;
			while(  target_old_end>=target  ) {
				*target_new_end-- = *target_old_end--;
			}

			// insert the clipboard content
			while(  (max_length--)>0  ) {
				*target++ = *pText++;
			}

			GlobalUnlock(hText);
		}
		CloseClipboard();
	}

	// return the inserted length for cursor advancing
	return inserted_length;
}