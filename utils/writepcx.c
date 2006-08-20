#include <stdio.h>

/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
static int iputw(int w, FILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (putc(b2,f)==b2)
      if (putc(b1,f)==b1)
         return w;

   return EOF;
}


/* write_pcx:
 *  Writes a bitmap into a PCX file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
void write_pcx(char *filename,
              unsigned char* data,
	      int width, int height,
	      unsigned char *pal)
{
    FILE *f;
    int c;
    int x, y;
    int runcount;
    int depth, planes;
    char runchar;
    char ch = 0;

    f = fopen(filename, "wb");
    if (!f) {
	fprintf(stderr, "Cant open '%s' for writing.\n", filename);
	return;
    }

    depth = 8;
    if (depth == 8)
	planes = 1;
    else
	planes = 3;

    putc(10, f);                      /* manufacturer */
    putc(5, f);                       /* version */
    putc(1, f);                       /* run length encoding  */
    putc(8, f);                       /* 8 bits per pixel */
    iputw(0, f);                      /* xmin */
    iputw(0, f);                      /* ymin */
    iputw(width-1, f);               /* xmax */
    iputw(height-1, f);               /* ymax */
    iputw(320, f);                    /* HDpi */
    iputw(200, f);                    /* VDpi */

    for (c=0; c<16; c++) {
	putc(pal[c*3+0], f);
	putc(pal[c*3+1], f);
	putc(pal[c*3+2], f);
    }

    putc(0, f);                       /* reserved */
    putc(planes, f);                  /* one or three color planes */
    iputw(width, f);                 /* number of bytes per scanline */
    iputw(1, f);                      /* color palette */
    iputw(width, f);                 /* hscreen size */
    iputw(height, f);                 /* vscreen size */
    for (c=0; c<54; c++) {            /* filler */
	putc(0, f);
    }

    for (y=0; y<height; y++) {             /* for each scanline... */
	runcount = 0;
	runchar = 0;
	for (x=0; x<width*planes; x++) {   /* for each pixel... */
	    if (depth == 8) {
		ch = data[x + y*width];
	    }
	    else {
/* 24 bit not yet supported
		if (x<width) {
		    c = getpixel(data, x, y);
		    ch = getr_depth(depth, c);
		}
		else if (x<width*2) {
		    c = getpixel(data, x-width, y);
		    ch = getg_depth(depth, c);
		}
		else {
		    c = getpixel(data, x-width*2, y);
		    ch = getb_depth(depth, c);
		}
*/
	    }
	    if (runcount==0) {
		runcount = 1;
		runchar = ch;
	    }
	    else {
		if ((ch != runchar) || (runcount >= 0x3f)) {
		    if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
			putc(0xC0 | runcount, f);
		    putc(runchar,f);
		    runcount = 1;
		    runchar = ch;
		}
		else
		    runcount++;
	    }
	}
	if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
	    putc(0xC0 | runcount, f);
	putc(runchar,f);
    }

    if (depth == 8) {                      /* 256 color palette */
	putc(12, f);

	for (c=0; c<256; c++) {
	    putc(pal[c*3+0], f);
	    putc(pal[c*3+1], f);
	    putc(pal[c*3+2], f);
	}
    }

    fclose(f);
}
