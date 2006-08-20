/*
 * makepak.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/* makepak.c
 *
 * daten.pak generator fuer Simugraph
 * Hj. Malthaner, Aug. 1997
 *
 *
 * ??.??.00 trennung von simgraph.c und makepak.c
 */

/*
 * 18.11.97 lineare Speicherung fuer Images -> hoehere Performance
 * 22.03.00 modifiziert für run-längen speicherung -> hoehere Performance
 * 21-Apr-01 modified for usage of 16/24 bit PPM images
 * 16-Jan-02 modified for usage of 8/16/24 bit PNG images
 * 19-May-02 modified to keep track of the 'special colors'
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#include "dr_rdppm.h"
#include "dr_rdpng.h"

#define IMG_SIZE 64
#define IMG_PER_PIC 40


//#define TRANSPARENT 0x808088
#define TRANSPARENT 0xE7FFFF


typedef unsigned char  PIXEL;
typedef unsigned short PIXVAL;
typedef unsigned int   PIXRGB;



struct dimension {
    int xmin,xmax,ymin,ymax;
};


struct imd {
    signed char x;
    signed char w;
    signed char y;
    signed char h;
    int len;
    PIXVAL * data;
};

static struct imd images[IMG_PER_PIC];


static unsigned char *block;

// number of special colors
#define SPECIAL 4

static PIXRGB rgbtab[SPECIAL] =
{
  0x395E7C,
  0x6084A7,
  0x88ABD3,
  0xB0D2FF
};



static inline PIXRGB block_getpix(int x, int y)
{
  return ((block[((y)*512*3)+(x)*3] << 16) + (block[((y)*512*3)+(x)*3+1] << 8) + (block[((y)*512*3)+(x)*3+2]));
}


static PIXVAL pixrgb_to_pixval(int rgb)
{
  PIXVAL result = 0;
  int standard = 1;
  int i;

  for(i=0; i<SPECIAL; i++) {
    if(rgbtab[i] == rgb) {
      result = 0x8000 + i;

      standard = 0;
      break;
    }
  }

  if(standard) {
    const int r = rgb >> 16;
    const int g = (rgb >> 8) & 0xFF;
    const int b = rgb & 0xFF;

    // RGB 555
    result = ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3);
  }

  return result;
}



void
getline(char *str, int max, FILE *f)
{
     fgets(str, max, f);
     str[strlen(str)-1]=0;
}


void
init_dim(PIXRGB *image, struct dimension *dim)
{
    int x,y;

    dim->ymin = dim->xmin = IMG_SIZE;
    dim->ymax = dim->xmax = 0;

    for(y=0; y<IMG_SIZE; y++) {
	for(x=0; x<IMG_SIZE; x++) {
	    if(image[x+y*IMG_SIZE] != TRANSPARENT) {
		if(x<dim->xmin)
		    dim->xmin = x;
		if(y<dim->ymin)
		    dim->ymin = y;
		if(x>dim->xmax)
		    dim->xmax = x;
		if(y>dim->ymax)
		    dim->ymax = y;
	    }
	}
    }


/*    dim[n].ymin = dim[n].xmin = 32;
    dim[n].ymax = dim[n].xmax = 63;
*/
/*    printf("Dim[%d] %d %d %d %d\n",n,dim[n].xmin,dim[n].ymin,dim[n].xmax,dim[n].ymax);
*/
}

static PIXVAL* encode_image(int x, int y, struct dimension *dim, int *len)
{
    int line;
    PIXVAL *dest;
    PIXVAL *dest_base = malloc(64*64*2*sizeof(PIXVAL));
    PIXVAL *run_counter;

    y += dim->ymin;
    dest = dest_base;

    for(line=0; line<(dim->ymax-dim->ymin+1); line++) {
	int   row = 0;
	PIXRGB pix = block_getpix(x, (y+line));
	char count = 0;

	do {
	    count = 0;
	    while(pix == TRANSPARENT && row < 64) {
		count ++;
		row ++;
		pix = block_getpix((x+row), (y+line));
	    }

//	    printf("-%d ", count);

	    *dest++ = count;

	    // if(row < 64) {

		run_counter = dest++;
		count = 0;

		while(pix != TRANSPARENT && row < 64) {
		    *dest++ = pixrgb_to_pixval(pix);

		    count ++;
		    row ++;
		    pix = block_getpix((x+row), (y+line));
		}
		*run_counter = count;

//		printf("+%d ", count);

	    // }

	} while(row < 64);

//	printf("\n");

	*dest++ = 0;
    }

    *len = (dest - dest_base);

    return dest_base;
}


static void
init_image_block(char *filename, int start)
{
    struct dimension dim;
    PIXRGB image[IMG_SIZE*IMG_SIZE];
    int nr = 0;

    int z,n;
    int x,y;

/* no longer needed since we switched to png images
    char cmd [1024];

    sprintf(cmd, "gunzip -c %s > /tmp/dummy.ppm", filename);
    system( cmd );

    printf("processing %s ... ", filename);


    if(load_block(block, "/tmp/dummy.ppm") == 0) {
	exit(1);
    }
*/

    printf("processing '%s' - image numbers %d - %d\n", filename, start, start+39);
    fflush(stdout);

    if(load_block(block, filename) == 0) {
	exit(1);
    }


    for(z=0; z<5; z++) {
	for(n=0; n<8; n++) {
	    for(x=0; x<IMG_SIZE; x++) {
		for(y=0; y<IMG_SIZE; y++) {
		    image[x+y*IMG_SIZE] = block_getpix(x+n*IMG_SIZE, y+z*IMG_SIZE);
		}
	    }
	    init_dim(image, &dim);

	    images[nr].x = dim.xmin;
	    images[nr].y = dim.ymin;

	    images[nr].w = dim.xmax - dim.xmin + 1;
	    images[nr].h = dim.ymax - dim.ymin + 1;

	    if(images[nr].h > 0) {
		int len;
		images[nr].data = encode_image(n*IMG_SIZE, z*IMG_SIZE, &dim, &len);
		images[nr].len = len;
//		printf("Image %d length %d\n", nr, len);
	    } else {
		images[nr].data = NULL;
		images[nr].len = 0;
	    }

	    nr ++;
	}
    }
}

void
fwrite_int(int *p, FILE *f)
{
    const int i = *p;
    fputc(i & 255,f);
    fputc((i >> 8) & 255,f);
    fputc((i >> 16) & 255,f);
    fputc((i >> 24) & 255,f);
}

void
init_images(const char *path)
{
    char pbuf[256];
    char buf[256];
    int n = 0;
    FILE *fin;
    FILE *fout;

    sprintf(pbuf, "./%s/makepak.dat", path);

    fin = fopen(pbuf, "rb");
    fout = fopen("daten.pak","wb");

    if(fin && fout) {
	fwrite_int(&n, fout);	// write dummy

	while(! feof(fin)) {
	    int i;
	    fgets(buf, 255, fin);
	    buf[255] = '\0';

	    i = strlen(buf);

	    while(buf[i] < 32) {
		buf[i--] = '\0';
	    }

	    if(!feof(fin) &&
		*buf != '#' &&
		*buf > ' ') {
		init_image_block(buf, n);
		n += IMG_PER_PIC;

		// write:
		for(i=0; i<IMG_PER_PIC; i++) {
	    	    fputc(images[i].x, fout);
		    fputc(images[i].w, fout);
	    	    fputc(images[i].y, fout);
	    	    fputc(images[i].h, fout);
	            fwrite_int(&(images[i].len), fout);

	            if(images[i].h > 0) {
		        fwrite(images[i].data, images[i].len*sizeof(PIXVAL), 1, fout);
		        free(images[i].data);
			images[i].data = NULL;
	    	    }
		}
	    }
	}
	fseek(fout, 0, SEEK_SET);
	fwrite_int(&n, fout);	// write correct value
	fclose(fin);
	fclose(fout);
    } else {
    	if(!fin) {
	    fprintf(stderr,
		"Error: can't open %s/makepak.dat for reading.\n", path);
    	} else {
	    fclose(fin);
	    printf("Can't write daten.pak.\n");
	}
	exit(1);
    }
    printf("processed %d images.\n", n);
}


int
simgraph_init(const char *path)
{
    block = (unsigned char*) malloc(512*360*3);

    if(block) {

        init_images(path);

	return 1;
    } else {
	puts("Error: not enough memory!");
	return 0;
    }
}


void
simgraph_exit()
{
    free(block);
}


int
main(int argc, char *argv[])
{
    puts("  \nMakepak16 v1.04 by Hj. Malthaner  (c) 1998-2002\n");
    puts("  This program creates the 'daten.pak' file used");
    puts("  by Simutrans from a bundle of PNG images.");
    puts("  The images must be stored in a subfolder");
    puts("  called 'images'.\n");
    puts("  Makepak16 reads images/makepak.dat and processes");
    puts("  all files listed in this file.\n");
    puts("  If you have questions about Simutrans or Makepak");
    puts("  contact the author at:");
    puts("    hansjoerg.malthaner@gmx.net or");
    puts("    hansjoerg.malthaner@danet.de\n");
    puts("  (This version is internally changed by V.Meyer to");
    puts("  allow any number of images).\n");

    simgraph_init(argc > 1 ? argv[1] : "images");

    simgraph_exit();

    puts("  >>>> Wrote daten.pak successfully.\n");

#ifdef COUNT_COLORS
    for(i=0; i<256; i++) {
	printf("%3d %8d\n", i, histogramm[i]);
    }
#endif

    return 0;
}
