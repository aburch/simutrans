#ifndef utils_writepcx_h
#define utils_writepcx_h

/* write_pcx:
 *  Writes a bitmap into a PCX file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
void write_pcx(char *filename,
              unsigned char* data,
	      int width, int height,
	      unsigned char *pal);

#endif
