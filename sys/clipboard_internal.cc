/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "simsys.h"
#include "../display/simgraph.h"
#include "../simdebug.h"


#define MAX_SIZE (4096)
static char content[MAX_SIZE] = "";
static size_t content_length = 0;


/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 */
void dr_copy(const char *source, size_t length)
{
	assert( length<MAX_SIZE );
	content_length = length;
	char *content_ptr = content;
	while(  (length--)>0  ) {
		*content_ptr++ = *source++;
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
	// determine the number of bytes to be pasted
	if(  content_length<=max_length  ) {
		max_length = content_length;
	}
	else {
		// ensure that max_length aligns with logical character boundaries of clipboard content
		size_t tmp_length = 0;
		size_t byte_count;
		while(  tmp_length+(byte_count=get_next_char(content+tmp_length,0u))<=max_length  ) {
			tmp_length += byte_count;
		}
		max_length = tmp_length;
	}
	const size_t inserted_length = max_length;

	// move the text to free up space for inserting the clipboard content
	char *target_old_end = target + strlen(target);
	char *target_new_end = target_old_end + max_length;
	while(  target_old_end>=target  ) {
		*target_new_end-- = *target_old_end--;
	}

	// insert the clipboard content
	const char *content_ptr = content;
	while(  (max_length--)>0  ) {
		*target++ = *content_ptr++;
	}

	// return the inserted length for cursor advancing
	return inserted_length;
}
