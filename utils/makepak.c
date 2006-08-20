/* makepak.c
 *
 * daten.pak generator fuer Simutrans
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 *
 * ??.??.00 trennung von simgraph.c und makepak.c
 */

/*
 * 18.11.97 lineare Speicherung fuer Images -> hoehere Performance
 * 22.03.00 modifiziert für run-längen speicherung -> hoehere Performance
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dr_rdgif.h"

#define IMG_SIZE 64

#define TRANSPARENT 255


typedef unsigned char PIXEL;

struct dimension {
    int xmin,xmax,ymin,ymax;
};

struct imd {
    signed char x;
    signed char w;
    signed char y;
    signed char h;
    int len;
    PIXEL * data;
};



#define NUM_IMAGES   2000

static struct imd images[NUM_IMAGES];


static unsigned char *block;


#undef COUNT_COLORS

#ifndef COUNT_COLORS
#define block_getpix(x,y) (block[((y)*512)+x])
#else

static int histogramm[256];

static int block_getpix(int x, int y)
{
    const int c = block[y*512+x];

    histogramm[c] ++;

    return c;
}
#endif

void
getline(char *str, int max, FILE *f)
{
     fgets(str, max, f);
     str[strlen(str)-1]=0;
}

void
init_dim(PIXEL *image, struct dimension *dim)
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

static char* encode_image(int x, int y, struct dimension *dim, int *len)
{
    int line;
    char *dest;
    char *dest_base = malloc(8200);
    char *run_counter;

    y += dim->ymin;
    dest = dest_base;

    for(line=0; line<(dim->ymax-dim->ymin+1); line++) {
	int   row = 0;
	PIXEL pix = block_getpix(x, y+line);
	char count = 0;

	do {
	    count = 0;
	    while(pix == TRANSPARENT && row < 64) {
		count ++;
		row ++;
		pix = block_getpix(x+row, y+line);
	    }

//	    printf("-%d ", count);

	    *dest++ = count;

	    if(row < 64) {

		run_counter = dest++;
		count = 0;

		while(pix != TRANSPARENT && row < 64) {
		    *dest++ = (char)pix;

		    count ++;
		    row ++;
		    pix = block_getpix(x+row, y+line);
		}
		*run_counter = count;

//		printf("+%d ", count);

	    }

	} while(row < 64);

//	printf("\n");

	*dest++ = 0;
    }

    *len = dest - dest_base;

    return dest_base;
}

static void
init_image_block(char *filename, int start)
{
  struct dimension dim;
  PIXEL image[IMG_SIZE*IMG_SIZE];
  int nr = start;

  int z,n;
  int x,y;

  printf("processing '%s'\n", filename);
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
      images[nr].w = dim.xmax - dim.xmin + 1;
      images[nr].y = dim.ymin;
      images[nr].h = dim.ymax - dim.ymin + 1;

      if(images[nr].h > 0) {
	int len;
	images[nr].data = encode_image(n*IMG_SIZE, z*IMG_SIZE, &dim, &len);
	images[nr].len = len;
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
  FILE *f;

  sprintf(pbuf, "./%s/makepak.dat", path);

  f = fopen(pbuf, "rb");

  if(f) {

    while(! feof(f)) {
      int i;
      fgets(buf, 255, f);
      buf[255] = '\0';

      i = strlen(buf);

      while(buf[i] < 32) {
	buf[i--] = '\0';
      }

      if(!feof(f) &&
         *buf != '#' &&
         *buf > ' ') {
	init_image_block(buf, n);
	n += 40;
      }
    }


    fclose(f);
  } else {
    fprintf(stderr,
	    "Error: can't open images/makepak.dat for reading.\n");
    exit(1);
  }


  printf("processed %d images.\n", n);


  f = fopen("daten.pak","wb");

  if(f) {
    int i;

    i = n;
    fwrite_int(&i, f);

    for(i=0; i<n; i++) {

      fputc(images[i].x, f);
      fputc(images[i].w, f);
      fputc(images[i].y, f);
      fputc(images[i].h, f);
      fwrite_int(&(images[i].len), f);

      if(images[i].h > 0) {
	fwrite(images[i].data, images[i].len, 1, f);
    }
  }


    fclose(f);
  } else {
    printf("Can't write daten.pak.\n");
    exit(1);
  }
}

int
simgraph_init(const char *path)
{
    block = (unsigned char*) malloc(512*360);

    if(block) {

        init_images(path);

	return 1;
    } else {
	puts("Error: not enough memory!");
	return 0;
    }
}

int
simgraph_exit()
{
    free(block);
}

int
main(int argc, char *argv[])
{
    int i;

    puts("  \nMakepak v1.07 by Hansjoerg Malthaner  (c) 1998-2001\n");
    puts("  This program creates the 'daten.pak' file used");
    puts("  by Simutrans from a bundle of PNG images.");
    puts("  The images must be stored in a subfolder");
    puts("  called 'images'.\n");
    puts("  Makepak reads images/makepak.dat and processes");
    puts("  all files listed in this file.\n");
    puts("  If you have questions about Simutrans or Makepak");
    puts("  contact the author at:");
    puts("    hansjoerg.malthaner@gmx.net or");
    puts("    hansjoerg.malthaner@danet.de\n");

    simgraph_init(argc > 1 ? argv[1] : "images");


    simgraph_exit();

    printf("\n>>>> Wrote daten.pak successfully.\n");

#ifdef COUNT_COLORS
    for(i=0; i<256; i++) {
	printf("%3d %8d\n", i, histogramm[i]);
    }
#endif

    return 0;
}
