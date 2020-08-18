/*
 * Ein-/Ausgaberoutinen fuer Simutrans
 * Hj. Malthaner, 2000
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
