/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

char *read_line(char *s, int size, FILE *file)
{
	char *r;

	do {
		r = fgets(s, size, file);
	} while(r != NULL && *s == '#');

	return r;
}
