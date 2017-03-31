/*
 * Ein-/Ausgaberoutinen fuer Simutrans
 * Hj. Malthaner, 2000
 */

/**
 * Reads a line from a file. Skips lines starting with #
 *
 * @see fgets
 * @author Hj. Malthaner
 */
char *read_line(char *s, int size, FILE *stream);
