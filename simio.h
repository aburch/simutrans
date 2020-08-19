/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMIO_H
#define SIMIO_H


/**
 * Reads a line from a file. Skips lines starting with #
 *
 * @see fgets
 * @author Hj. Malthaner
 */
char *read_line(char *s, int size, FILE *stream);

#endif
