/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "simsys.h"

#include "../display/simgraph.h"
#include "../simdebug.h"
#include "../dataobj/translator.h"

#ifndef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <string.h>


/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 */
void dr_copy(const char *source, size_t length)
{
	char *text = (char *)malloc( length+1 );
	strncpy( text, source, length );
	text[length] = 0;
	SDL_SetClipboardText( text );
	free( text );
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

	if(  SDL_HasClipboardText()  ) {
		char *str = SDL_GetClipboardText();

		// Insert string.
		size_t const str_len = strlen(str);
		insert_len = str_len <= max_length ? str_len : (size_t)utf8_get_prev_char((utf8 const *)str, max_length);
		memmove(target + insert_len, target, strlen(target) + 1);
		memcpy(target, str, insert_len);

		SDL_free( str );
	}

	return insert_len;
}
