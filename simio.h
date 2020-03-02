/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * Reads a line from a file. Skips lines starting with #
 *
 * @see fgets
 * @author Hj. Malthaner
 */
char *read_line(char *s, int size, FILE *stream);
