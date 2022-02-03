/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMIO_H
#define SIMIO_H


#include <cstdio>


/**
 * Reads a line from a file. Skips lines starting with #
 * @see fgets
 */
char *read_line(char *s, int size, FILE *stream);

#endif
