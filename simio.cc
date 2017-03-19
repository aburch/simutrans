/*
 * Ein-/Ausgaberoutinen fuer Simutrans
 * Hj. Malthaner, 2000
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
