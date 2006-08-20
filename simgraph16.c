/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * hansjoerg.malthaner@gmx.de
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

/* simgraph.c
 *
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 *
 *
 *
 * 18.11.97 lineare Speicherung fuer Images -> hoehere Performance
 * 22.03.00 run längen Speicherung fuer Images -> hoehere Performance
 * 15.08.00 dirty tile verwaltung fuer effizientere updates
 */

//#define DEBUG 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <zlib.h>

#include "pathes.h"
#include "simgraph.h"
#include "simsys16.h"
#include "simmem.h"
#include "simdebug.h"
#include "besch/bild_besch.h"

#include "utils/writepcx.h"
#include "utils/image_encoder.h"


#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define W_OK	2
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


/*
 * Use C implementation of image drawing routines
 */
// #define USE_C


// --------------      static data    --------------


/*
 * do we need a mouse pointer emulation ?
 * assume yes by default, but the system wrapper
 * is inquired during initialisation
 */
static int use_softpointer = 1;


static int softpointer = 261;


/*
 * die icon leiste muss neu gezeichnet werden wenn der
 * Mauszeiger darueber schwebt
 */
static int old_my = -1;

static unsigned char dr_fonttab[2048];  /* Unser Zeichensatz sitzt hier */
static unsigned char npr_fonttab[3072]; // Niels: npr are my initials

static unsigned char dr_4x7font[7*256];


/*
 * Hajo: pixel value type, RGB 555
 */
// typedef unsigned short PIXVAL;


static unsigned char day_pal[256*3];


/*
 * Hajo: player colors daytime RGB values
 * 16 sets of 16 colors each. At the moment only 7 of the
 * 16 colors of a set are used.
 * Last 16 colors are recolored player 0 color set
 */
static unsigned char special_pal[256*3 + 16*3];


#define RGBMAPSIZE (0x8000 + 29)


/*
 * Hajo: mapping tables for RGB 555 to actual output format
 * plus the special (player, day&night) colors appended
 *
 * 0x0000 - 0x7FFF: RGB colors
 * 0x8000 - 0x800F: Player colors
 * 0x8010 -       : Day&Night special colors
 */
static PIXVAL rgbmap_day_night[RGBMAPSIZE];


/*
 * Hajo: same as rgbmap_day_night, but allways daytime colors
 */
static PIXVAL rgbmap_all_day[RGBMAPSIZE];


/**
 * Hajo:used by pixel copy functions, is one of rgbmap_day_night
 * rgbmap_all_day
 */
static PIXVAL *rgbmap_current = 0;


/*
 * Hajo: mapping tables for Simutrans colormap to actual output format
 */
static PIXVAL rgbcolormap[256];


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - day&night mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_day_night[256];


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_all_day[256];


/**
 * Hajo:used by pixel copy functions, is one of specialcolormap_day_night
 * specialcolormap_all_day
 */
static PIXVAL * specialcolormap_current = 0;


/*
 * Hajo: Currently selected color set for player 0 (0..15)
 */
static int selected_player_color_set = 0;


/*
 * Hajo: Image map descriptor struture
 */
struct imd {
  unsigned char x;    // current (zoomed) min x offset
  unsigned char w;    // current (zoomed) width
  unsigned char y;    // current (zoomed) min y offset
  unsigned char h;    // current (zoomed) width

  unsigned char base_x;    // current min x offset
  unsigned char base_w;    // current width
  unsigned char base_y;    // current min y offset
  unsigned char base_h;    // current width

  int len;            // base image data size

  int zoom_len;


  PIXVAL * data;      // current data, zoomed and adapted to
                      // output format RGB 555 or RGB 565

  PIXVAL * zoom_data;  // zoomed original data

  PIXVAL * base_data;  // original image data

  PIXVAL * recoded_base_data; // original image data regoded to RGB 555 or RGB 565

  unsigned char zoomable;  // Flag if this image can be zoomed
};


int disp_width = 640;
int disp_height = 480;


/*
 * Image table
 */
struct imd *images = NULL;


/*
 * Number of loaded images
 */
static int anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static int alloc_images = 0;


/*
 * Output framebuffer
 */
PIXVAL *textur = NULL;


/*
 * Hajo: Current clipping rectangle
 */
static struct clip_dimension clip_rect;


/*
 * Hajo: dirty tile management strcutures
 */
#define DIRTY_TILE_SIZE 16
#define DIRTY_TILE_SHIFT 4

static unsigned char *tile_dirty = NULL;
static unsigned char *tile_dirty_old = NULL;

static int tiles_per_line = 0;
static int tile_lines = 0;
static int tile_buffer_length = 0;


static int light_level = 0;
static int color_level = 1;

static int night_shift = -1;


#define LIGHT_COUNT 13

/*
 * Hajo: speical colors during daytime
 */
static const int day_lights[LIGHT_COUNT] =
{
  0x57656F, // Dark windows, lit yellowish at night
  0x7F9BF1, // Lighter windows, lit blueish at night
  0xFFFF53, // Yellow light
  0xFF211D, // Red light
  0x01DD01, // Green light
  0x6B6B6B, // Non-darkening grey 1 (menus)
  0x9B9B9B, // Non-darkening grey 2 (menus)
  0xB3B3B3, // non-darkening grey 3 (menus)
  0xC9C9C9, // Non-darkening grey 4 (menus)
  0xDFDFDF, // Non-darkening grey 5 (menus)
  0xE3E3FF, // Nearly white light at day, yellowish light at night
  0xC1B1D1, // Windows, lit yellow
  0x4D4D4D, // Windows, lit yellow
};


/*
 * Hajo: speical colors during nighttime
 */
static const int night_lights[LIGHT_COUNT] =
{
  0xD3C380, // Dark windows, lit yellowish at night
  0x80C3D3, // Lighter windows, lit blueish at night
  0xFFFF53, // Yellow light
  0xFF211D, // Red light
  0x01DD01, // Green light
  0x6B6B6B, // Non-darkening grey 1 (menus)
  0x9B9B9B, // Non-darkening grey 2 (menus)
  0xB3B3B3, // non-darkening grey 3 (menus)
  0xC9C9C9, // Non-darkening grey 4 (menus)
  0xDFDFDF, // Non-darkening grey 5 (menus)
  0xFFFFE3, // Nearly white light at day, yellowish light at night
  0xD3C380, // Windows, lit yellow
  0xD3C380, // Windows, lit yellow
};


/*
 * Hajo: tile raster width
 */
int tile_raster_width = 16;
static int base_tile_raster_width = 16;


/*
 * Hajo: Zoom factor
 */
static int zoom_factor = 1;


// -------------- Function prototypes --------------

static void display_img_nc(const int n, const int xp, const int yp, const PIXVAL *);
static void rezoom();
static void recode();
static void calc_base_pal_from_night_shift(const int night);




// --------------      Functions      --------------



int get_zoom_factor()
{
  return zoom_factor;
}


void set_zoom_factor(int z)
{
  zoom_factor = z;

  fprintf(stderr, "set_zoom_factor() : factor=%d\n", zoom_factor);

  rezoom();
  recode();
}


#ifdef _MSC_VER
#define mark_tile_dirty(x,y) \
    do { \
	const int bit = (int)(x)+(int)(y)*tiles_per_line; \
	\
        /* assert(bit/8 < tile_buffer_length); */ \
	\
        tile_dirty[bit >> 3] |= 1 << (bit & 7); \
    } while(FALSE)
#else
static inline void mark_tile_dirty(const int x, const int y)
{
    const int bit = x+y*tiles_per_line;

    // assert(bit/8 < tile_buffer_length);

    tile_dirty[bit >> 3] |= 1 << (bit & 7);
}
#endif

#ifdef _MSC_VER
#define mark_tiles_dirty(x1, x2, y) \
    do { \
	int bit = y * tiles_per_line + x1; \
        const int end = bit + ((int)(x2)-(int)(x1)); \
	\
	do {  \
	    /* assert(bit/8 < tile_buffer_length);*/ \
	    \
	    tile_dirty[bit >> 3] |= 1 << (bit & 7); \
	} while(++bit <= end); \
    } while(FALSE)
#else
static inline void mark_tiles_dirty(const int x1, const int x2, const int y)
{
    int bit = y * tiles_per_line + x1;
    const int end = bit + (x2-x1);

    do {
	// assert(bit/8 < tile_buffer_length);

	tile_dirty[bit >> 3] |= 1 << (bit & 7);
    } while(++bit <= end);
}
#endif

#ifdef _MSC_VER
static int is_tile_dirty(const int x, const int y) {
     const int bit = x+y*tiles_per_line;
     const int bita = bit >> 3;
     const int bitb = 1 << (bit & 7);

     return (tile_dirty[bita] & (bitb)) |
            (tile_dirty_old[bita] & (bitb));

}
#else
static inline int is_tile_dirty(const int x, const int y) {
     const int bit = x+y*tiles_per_line;
     const int bita = bit >> 3;
     const int bitb = 1 << (bit & 7);

     return (tile_dirty[bita] & (bitb)) |
            (tile_dirty_old[bita] & (bitb));

}
#endif


/**
 * Markiert ein Tile as schmutzig, ohne Clipping
 * @author Hj. Malthaner
 */
static void mark_rect_dirty_nc(int x1, int y1, int x2, int y2)
{
    // floor to tile size

    x1 >>= DIRTY_TILE_SHIFT;
    y1 >>= DIRTY_TILE_SHIFT;
    x2 >>= DIRTY_TILE_SHIFT;
    y2 >>= DIRTY_TILE_SHIFT;

/*
    assert(x1 >= 0);
    assert(x1 < tiles_per_line);
    assert(y1 >= 0);
    assert(y1 < tile_lines);
    assert(x2 >= 0);
    assert(x2 < tiles_per_line);
    assert(y2 >= 0);
    assert(y2 < tile_lines);
*/

    for(; y1<=y2; y1++) {
	mark_tiles_dirty(x1, x2, y1);
    }
}


/**
 * Markiert ein Tile as schmutzig, mit Clipping
 * @author Hj. Malthaner
 */
void mark_rect_dirty_wc(int x1, int y1, int x2, int y2)
{
    // Hajo: inside display ?
    if(x2 >= 0 &&
       y2 >= 0 &&
       x1 < disp_width &&
       y1 < disp_height) {

	if(x1 < 0)
	    x1 = 0;

	if(y1 < 0)
	    y1 = 0;

	if(x2 >= disp_width) {
	    x2 = disp_width-1;
	}

	if(y2 >= disp_height) {
	    y2 = disp_height-1;
	}

	mark_rect_dirty_nc(x1, y1, x2, y2);
    }
}



/**
 * Convert image data to actual output data
 * @author Hj. Malthaner
 */
static void recode()
{
    int n;

    for(n=0; n<anz_images; n++) {

	int h = images[n].h;

	// printf("height = %d\n", h);

	if(h > 0) {
            PIXVAL *sp = images[n].data;

	    memcpy(images[n].data,
		   images[n].zoom_data,
		   images[n].zoom_len*sizeof(PIXVAL));

	    // convert image

	    do { // zeilen dekodieren

		unsigned int runlen = *sp++;

		// eine Zeile dekodieren
		do {

		    // jetzt kommen farbige pixel

		    runlen = *sp++;

		    // printf("runlen = %d\n", runlen);

		    while(runlen--) {
			*sp = rgbmap_day_night[*sp];
			sp++;
		    }

		} while( (runlen = *sp++) );

	    } while(--h);


	    // Hajo: if we recode the first time, we save the recoded
	    // original data XXX HACK XXX

	    if(images[n].recoded_base_data == 0) {
	      images[n].recoded_base_data =
		guarded_malloc(sizeof(PIXVAL) * images[n].zoom_len);

	      memcpy(images[n].recoded_base_data,
		     images[n].data,
		     images[n].zoom_len*sizeof(PIXVAL));
	    }
	}
    }
}


/**
 * Convert base image data to actual image size
 * @author Hj. Malthaner
 */
static void rezoom()
{
  int n;
  PIXVAL * buf = guarded_malloc(sizeof(PIXVAL) *
				base_tile_raster_width *
				base_tile_raster_width);


  for(n=0; n<anz_images; n++) {

      // Hajo: may this image be zoomed

    if(images[n].zoomable) {

	// Step 1): Decode image

	// Hajo: gruesome dirty trick: fake output onto display
	// but in fact write into buffer

	{
	  const int save_disp_width = disp_width;
	  PIXVAL *  save_textur = textur;
	  int i;

	  disp_width = base_tile_raster_width;
	  textur = buf;

	  tile_raster_width = base_tile_raster_width;
	  images[n].x = images[n].base_x;
	  images[n].w = images[n].base_w;
	  images[n].y = images[n].base_y;
	  images[n].h = images[n].base_h;

	  // Fill buffer with 'transparent' color

	  for(i=0; i<base_tile_raster_width*base_tile_raster_width; i++) {
	    buf[i] = 0x73FE;

	    // buf[i] = 0x0FFF;
	  }

	  // Render image into buffer

	  guarded_free(images[n].data);
	  images[n].data = images[n].base_data;

	  display_img_nc(n, 0, 0, images[n].data);

	  images[n].data = 0;

	  disp_width = save_disp_width;
	  textur = save_textur;
	}



	// Step 2): Scale  image

	tile_raster_width = base_tile_raster_width >> (zoom_factor-1);

	if(zoom_factor != 1)
	{
	  const int w = tile_raster_width;
	  const int h = tile_raster_width;

	  const int wstep = base_tile_raster_width*0xFFFF/w;
	  const int hstep = base_tile_raster_width*0xFFFF/h;

	  int source_y = hstep-1;
	  int x,y;

	  for(y=0; y<h; y++) {
	    int source_x = 0;
	    const PIXVAL * const src = &buf[(source_y >> 16) * base_tile_raster_width];

	    for(x=0; x<w; x++) {

	      buf[x + y*w] = src[source_x >> 16];

	      source_x += wstep;
	    }
	    source_y += hstep;
	  }
	}


	// Step 3): Encode image
	{
	  int len;
	  struct dimension dim;
	  PIXVAL *result = encode_image(buf, tile_raster_width, &dim, &len);


	  images[n].x = dim.xmin;
	  images[n].w = dim.xmax - dim.xmin+1;
	  images[n].y = dim.ymin;
	  images[n].h = dim.ymax - dim.ymin+1;

	  guarded_free(images[n].zoom_data);

	  images[n].zoom_data = result;
	  images[n].zoom_len = len;

	  images[n].data = malloc(sizeof(PIXVAL) * len);

	  /*
	  printf("%4d  x=%02d y=%02d w=%02d h=%02d len=%d\n",
		 n,
		 images[n].x,
		 images[n].y,
		 images[n].w,
		 images[n].h,
		 len);
	  */
	}

    } // if(images[n].zoomable) {

  } // for


  guarded_free(buf);
}



/**
 * Loads the fonts
 * @author Hj. Malthaner
 */
void init_font(const char *fixed_font, const char *prop_font)
{
    FILE *f = NULL;

    // suche in ./draw.fnt

    if(f==NULL ) {
	f=fopen(fixed_font,"rb");
    }

    if(f==NULL) {
	printf("Error: Cannot open '%s'\n", fixed_font);
	exit(1);
    } else {
	int i;

	printf("Loading font '%s'\n", fixed_font);

	for(i=0;i<2048;i++) {
	  dr_fonttab[i]=getc(f);
	}
	fclose(f);
    }


    // also, read font.dat
    {
      int i,r;
      f = fopen(prop_font, "rb");
      if (f==NULL) {
	printf("Error: Cannot open '%s'\n", prop_font);
	exit(1);
      }

      printf("Loading font '%s'\n", prop_font);

      for (i=0;i<3072;i++) {
	npr_fonttab[i] = r = getc(f);
      }
      if (r==EOF) {
	printf("Error: prop.fnt too short\n");
	exit(1);
      }
      if (getc(f)!=EOF) {
	printf("Error: prop.fnt too long\n");
	exit(1);
      }
      fclose(f);
    }
}


/**
 * Laedt die Palette
 * @author Hj. Malthaner
 */
static int
load_palette(const char *filename, unsigned char *palette)
{
    char fname[256];
    FILE *file;

    strcpy(fname, filename);

    file = fopen(fname,"rb");

    if(file) {

	int    x;
        int    anzahl=256;
	int    r,g,b;

        fscanf(file, "%d", &anzahl);

        for(x=0; x<anzahl; x++) {
            fscanf(file,"%d %d %d", &r, &g, &b);

	    palette[x*3+0] = r;
	    palette[x*3+1] = g;
	    palette[x*3+2] = b;
	}

	fclose(file);

    } else {
	fprintf(stderr, "Error: can't open file '%s' for reading\n", fname);
    }

    return file != NULL;
}


/**
 * Loads the special colors map
 * @author Hj. Malthaner
 */
static int
load_special_palette()
{
  const char *fname = PALETTE_PATH_X "special.pal";

  FILE *file = fopen(fname,"rt");

  if(file) {
    int i, R,G,B;
    unsigned char * p = special_pal;

    // Hajo: skip number of colors entry
    fscanf(file, "%d\n", &i);

    // Hajo: read colors
    for(i=0; i<256; i++) {
      fscanf(file, "%d %d %d\n", &R, &G, &B);

      *p++ = R;
      *p++ = G;
      *p++ = B;
    }

    fclose(file);
  } else {
    fprintf(stderr, "Error: can't open file '%s' for reading\n", fname);
  }

  return file != NULL;
}


static int nibble(char c)
{
    if(c > '9') {
	return 10 + (c - 'A');
    } else {
	return c - '0';
    }
}


void load_hex_font(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    int fh = 7;

    if(file) {
	char buf [80];
        int  n, line;
        char *p;

	printf("Loading hex font '%s'\n", filename);

	while(fgets(buf, 79, file) != NULL) {

	    sscanf(buf, "%4x", &n);

	    p = buf+5;

	    for(line=0; line<fh; line ++) {
		int val =  nibble(p[0])*16 + nibble(p[1]);
		dr_4x7font[n*fh + line] = val;
		p += 2;
		// printf("%02X", val);
	    }
	    // printf("\n");

	}
	fclose(file);
    } else {
	fprintf(stderr, "Error: can't open file '%s' for reading\n", filename);
    }
}


int display_get_width()
{
    return disp_width;
}


int display_get_height()
{
    return disp_height;
}


/**
 * Holt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
int display_get_light()
{
    return light_level;
}


/**
 * Setzt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
void display_set_light(int new_light_level)
{
  light_level = new_light_level;

  calc_base_pal_from_night_shift(night_shift);
  recode();
}


/**
 * Holt Farbeinstellungen
 * @author Hj. Malthaner
 */
int display_get_color()
{
    return color_level;
}


/**
 * Setzt Farbeinstellungen
 * @author Hj. Malthaner
 */
void display_set_color(int new_color_level)
{
    color_level = new_color_level;

    if(color_level < 0) {
	color_level = 0;
    }

    if(color_level > 3) {
	color_level = 3;
    }

    switch(color_level) {
    case 0:
        load_palette(PALETTE_PATH_X "simud70.pal", day_pal);
        break;
    case 1:
        load_palette(PALETTE_PATH_X "simud80.pal", day_pal);
        break;
    case 2:
        load_palette(PALETTE_PATH_X "simud90.pal", day_pal);
        break;
    case 3:
        load_palette(PALETTE_PATH_X "simud100.pal", day_pal);
        break;
    }
}


/* Hajos variant

static void calc_base_pal_from_night_shift(const int night)
{
    const int day = 4 - night;
    int i;

    for(i=0; i<(1<<15); i++) {

	int R;
	int G;
	int B;

	// RGB 555 input
	R = ((i & 0x7C00) >> 7) - night*24;
	G = ((i & 0x03E0) >> 2) - night*24;
	B = ((i & 0x001F) << 3) - night*16;

	rgbmap_day_night[i] = get_system_color(R > 0 ? R : 0,
        	                     G > 0 ? G : 0,
				     B > 0 ? B : 0);
    }

    // player 0 colors, unshaded
    for(i=0; i<4; i++) {
        const int R = day_pal[i*3];
	const int G = day_pal[i*3+1];
	const int B = day_pal[i*3+2];

	rgbcolormap[i] =
	  get_system_color(R > 0 ? R : 0,
			   G > 0 ? G : 0,
			   B > 0 ? B : 0);

    }

    // player 0 colors, shaded
    for(i=0; i<4; i++) {
        const int R = day_pal[i*3] - night*24;
	const int G = day_pal[i*3+1] - night*24;
	const int B = day_pal[i*3+2] - night*16;

	specialcolormap_day_night[i] =
	  rgbmap_day_night[0x8000+i] =
	  get_system_color(R > 0 ? R : 0,
			   G > 0 ? G : 0,
			   B > 0 ? B : 0);
    }

    // Lights
    for(i=0; i<LIGHT_COUNT; i++) {
      const int day_R =  day_lights[i] >> 16;
      const int day_G = (day_lights[i] >> 8) & 0xFF;
      const int day_B =  day_lights[i] & 0xFF;

      const int night_R =  night_lights[i] >> 16;
      const int night_G = (night_lights[i] >> 8) & 0xFF;
      const int night_B =  night_lights[i] & 0xFF;


      const int R = (day_R * day + night_R * night) >> 2;
      const int G = (day_G * day + night_G * night) >> 2;
      const int B = (day_B * day + night_B * night) >> 2;

      rgbmap_day_night[0x8004+i] =
	get_system_color(R > 0 ? R : 0,
			 G > 0 ? G : 0,
			 B > 0 ? B : 0);
    }


    // transform special colors
    for(i=4; i<32; i++) {
        int R = special_pal[i*3] - night*24;
	int G = special_pal[i*3+1] - night*24;
	int B = special_pal[i*3+2] - night*16;

	specialcolormap_day_night[i] = get_system_color(R > 0 ? R : 0,
					      G > 0 ? G : 0,
					      B > 0 ? B : 0);
    }


    // convert to RGB xxx
    recode();
}

*/


/* Tomas variant */

static void calc_base_pal_from_night_shift(const int night)
{
  const int day = 4 - night;
  unsigned int i;

  // constant multiplier 0,66 - dark night  255 will drop to 49, 55 to 10
  //                     0,7  - dark, but all is visible     61        13
  //                     0,73                                72        15
  //                     0,75 - quite bright                 80        17
  //                     0,8    bright                      104        22
  /*
  const double RG_nihgt_multiplier = pow(0.73,night);
  const double B_nihgt_multiplier = pow(0.82,night);
  */

  const double RG_nihgt_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
  const double B_nihgt_multiplier = pow(0.83, night) * ((light_level + 8.0) / 8.0);


  for (i=0; i<0x8000; i++) {
    // (1<<15) this is total no of all possible colors in RGB555)

    int R;
    int G;
    int B;

    // RGB 555 input
    R = ((i & 0x7C00) >> 7);
    G = ((i & 0x03E0) >> 2);
    B = ((i & 0x001F) << 3);
    // lines generate all possible colors in 555RGB code - input
    // however the result is in 888RGB - 8bit per channel


    R=(int)(R*RG_nihgt_multiplier);
    G=(int)(G*RG_nihgt_multiplier);
    B=(int)(B*B_nihgt_multiplier);

    rgbmap_day_night[i] = get_system_color(R,G,B);
  }


  // player 0 colors, unshaded
  for(i=0; i<4; i++) {
    const int R = day_pal[i*3];
    const int G = day_pal[i*3+1];
    const int B = day_pal[i*3+2];

    rgbcolormap[i] =
      get_system_color(R > 0 ? R : 0,
		       G > 0 ? G : 0,
		       B > 0 ? B : 0);
  }

  // player 0 colors, shaded
  for(i=0; i<16; i++) {
    const int R = (int)(special_pal[(256+i)*3]   * RG_nihgt_multiplier);
    const int G = (int)(special_pal[(256+i)*3+1] * RG_nihgt_multiplier);
    const int B = (int)(special_pal[(256+i)*3+2] * B_nihgt_multiplier);

    specialcolormap_day_night[i] = rgbmap_day_night[0x8000+i] =
      get_system_color(R > 0 ? R : 0,
		       G > 0 ? G : 0,
		       B > 0 ? B : 0);

    specialcolormap_all_day[i] =
      get_system_color(special_pal[(256+i)*3],
		       special_pal[(256+i)*3+1],
		       special_pal[(256+i)*3+2]);
  }

  // Lights
  for(i=0; i<LIGHT_COUNT; i++) {
    const int day_R =  day_lights[i] >> 16;
    const int day_G = (day_lights[i] >> 8) & 0xFF;
    const int day_B =  day_lights[i] & 0xFF;

    const int night_R =  night_lights[i] >> 16;
    const int night_G = (night_lights[i] >> 8) & 0xFF;
    const int night_B =  night_lights[i] & 0xFF;

    const int R = (day_R * day + night_R * night) >> 2;
    const int G = (day_G * day + night_G * night) >> 2;
    const int B = (day_B * day + night_B * night) >> 2;

    rgbmap_day_night[0x8010+i] =
      get_system_color(R > 0 ? R : 0,
		       G > 0 ? G : 0,
		       B > 0 ? B : 0);
  }


  // transform special colors
  for(i=16; i<256; i++) {
    int R = (int)(special_pal[i*3]*RG_nihgt_multiplier);
    int G = (int)(special_pal[i*3+1]*RG_nihgt_multiplier);
    int B = (int)(special_pal[i*3+2]*B_nihgt_multiplier);

    specialcolormap_day_night[i] = get_system_color(R > 0 ? R : 0,
					  G > 0 ? G : 0,
					  B > 0 ? B : 0);
  }


  // convert to RGB xxx
  recode();
}


void display_day_night_shift(int night)
{
    if(night != night_shift) {
	night_shift = night;

	calc_base_pal_from_night_shift(night);

	mark_rect_dirty_nc(0,0, disp_width-1, disp_height-1);
    }
}


/**
 * Sets color set for player 0
 * @param entry   number of color set, range 0..15
 * @author Hj. Malthaner
 */
void display_set_player_color(int entry)
{
  int i;

  selected_player_color_set = entry;

  entry *= 16*3;

  // Legacy set of 4 player colors, unshaded
  for(i=0; i<4; i++) {
    day_pal[i*3]   = special_pal[entry + i*3*2];
    day_pal[i*3+1] = special_pal[entry + i*3*2+1];
    day_pal[i*3+2] = special_pal[entry + i*3*2+2];
  }

  // New set of 16 player colors, unshaded
  for(i=0; i<16; i++) {
    special_pal[(256+i)*3]   = special_pal[entry + i*3];
    special_pal[(256+i)*3+1] = special_pal[entry + i*3+1];
    special_pal[(256+i)*3+2] = special_pal[entry + i*3+2];
  }

  calc_base_pal_from_night_shift(night_shift);

  mark_rect_dirty_nc(0,0, disp_width-1, disp_height-1);
}


/**
 * Fügt ein Images aus andere Quelle hinzu)
 * @return die neue Bildnummer
 *
 * @author V. Meyer
 */
void register_image(struct bild_besch_t *bild)
{
    if(anz_images == alloc_images) {
	alloc_images += 128;
	images = (struct imd *)guarded_realloc(images, sizeof(struct imd)*alloc_images);
    }
    images[anz_images].x = bild->x;
    images[anz_images].w = bild->w;
    images[anz_images].y = bild->y;
    images[anz_images].h = bild->h;

    images[anz_images].base_x = bild->x;
    images[anz_images].base_w = bild->w;
    images[anz_images].base_y = bild->y;
    images[anz_images].base_h = bild->h;

    images[anz_images].len = bild->len;


    images[anz_images].base_data = (PIXVAL *)(bild + 1);


    images[anz_images].zoom_data = guarded_malloc(images[anz_images].len
						  *sizeof(PIXVAL));

    images[anz_images].data = guarded_malloc(images[anz_images].len
    					     *sizeof(PIXVAL));

    images[anz_images].recoded_base_data = 0;

    images[anz_images].zoomable = bild->zoomable;


    memcpy(images[anz_images].zoom_data,
	   images[anz_images].base_data,
	   images[anz_images].len*sizeof(PIXVAL));

    images[anz_images].zoom_len = bild->len;

    bild->bild_nr = anz_images++;

    if(base_tile_raster_width < bild->w) {
      base_tile_raster_width = bild->w;
      tile_raster_width = base_tile_raster_width;
    }

}


/**
 * Holt Maus X-Position
 * @author Hj. Malthaner
 */
int gib_maus_x()
{
    return sys_event.mx;
}


/**
 * Holt Maus y-Position
 * @author Hj. Malthaner
 */
int gib_maus_y()
{
    return sys_event.my;
}


/**
 * this sets width < 0 if the range is out of bounds
 * so check the value after calling and before displaying
 * @author Hj. Malthaner
 */
static int clip_wh(int *x, int *width, const int min_width, const int max_width)
{
    // doesnt check "wider than image" case

    if(*x < min_width) {
	const int xoff = min_width - (*x);

	*width += *x-min_width;
	*x = min_width;

	if(*x + *width >= max_width) {
	    *width = max_width - *x;
	}

	return xoff;
    } else if(*x + *width >= max_width) {
	*width = max_width - *x;
    }
    return 0;
}


/**
 * places x and w within bounds left and right
 * if nothing to show, returns FALSE
 * @author Niels Roest
 **/
static int clip_lr(int *x, int *w, const int left, const int right)
{
  const int l = *x;      // leftmost pixel
  const int r = *x+*w-1; // rightmost pixel

  if (l > right || r < left) {
    *w = 0; return FALSE;
  }

  // there is something to show.
  if (l < left)  { *w -= left - l;  *x = left; }
  if (r > right) { *w -= r - right;}
  return TRUE;
}


/**
 * Ermittelt Clipping Rechteck
 * @author Hj. Malthaner
 */
struct clip_dimension display_gib_clip_wh(void)
{
    return clip_rect;
}


/**
 * Setzt Clipping Rechteck
 * @author Hj. Malthaner
 */
void display_setze_clip_wh(int x, int y, int w, int h)
{
    clip_wh(&x, &w, 0, disp_width);
    clip_wh(&y, &h, 0, disp_height);

    clip_rect.x = x;
    clip_rect.y = y;
    clip_rect.w = w;
    clip_rect.h = h;

    clip_rect.xx = x+w-1;
    clip_rect.yy = y+h-1;
}

// ----------------- basic painting procedures ----------------

#ifndef _MSC_VER
static void pixcopy(PIXVAL *dest,
                    const PIXVAL *src,
                    unsigned int len) __attribute__ ((regparm(3)));


static void colorpixcopy(PIXVAL *dest, const PIXVAL *src,
                         const PIXVAL * const end,
                         const int color) __attribute__ ((regparm(3)));

#endif




/**
 * Kopiert Pixel von src nach dest
 * @author Hj. Malthaner
 */
#ifdef _MSC_VER
#define pixcopy(_dest, _src,  len)  \
    do {  \
        PIXVAL *dest = _dest; \
        const PIXVAL *src = _src; \
	\
	/* for gcc this seems to produce the optimal code ...*/ \
	const PIXVAL *const end = dest+len; \
	while(dest < end) { \
    	    *dest ++ = *src++; \
	}  \
    } while(FALSE)
#else
static inline void pixcopy(PIXVAL *dest,
                    const PIXVAL *src,
                    const unsigned int len)
{
    // for gcc this seems to produce the optimal code ...
    const PIXVAL *const end = dest+len;
    while(dest < end) {
	*dest ++ = *src++;
    }
}
#endif

/**
 * If arguments are supposed to be changed during the call
 * use this makro instead of pixcopy()
 */
//#define PCPY(d, s, l)  while(l--) *d++ = rgbmap[*s++];
//#define PCPY(d, s, l)  while(l--) *d++ = *s++;
// #define PCPY(d, s, l)  if(runlen) do {*d++ = *s++;} while(--l)

// Hajo: unrolled loop is about 5 percent faster
#define PCPY(d, s, l) if(l & 1) {*d++ = *s++; l--;} {unsigned long *ld =(unsigned long *)d; const unsigned long *ls =(const unsigned long *)s; d+=l; s+=l; l >>= 1; while(l--) {*ld++ = *ls++;}}



/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
#ifdef _MSC_VER
#define colorpixcopy(_dest, _src, _end, color) \
    do { \
	PIXVAL *dest = _dest; \
	const PIXVAL *src = _src; \
	const PIXVAL * const end = _end; \
	\
	while(src < end) { \
            PIXVAL pix = *src++; \
            if(pix >= 0x8000 && pix <= 0x800F) { \
	        *dest++ = specialcolormap_current[(pix & 0x7FFF) + color]; \
	    } else { \
	        *dest++ = rgbmap_current[pix]; \
	    } \
	} \
    } while(FALSE)
#else
static inline void colorpixcopy(PIXVAL *dest,
				const PIXVAL *src,
				const PIXVAL * const end,
				const int color)
{
    while(src < end) {
        PIXVAL pix = *src++;

	if(pix >= 0x8000 && pix <= 0x800F) {
	    *dest++ = specialcolormap_current[(pix & 0x7FFF) + color];
	} else {
	    *dest++ = rgbmap_current[pix];
	}
    }
}
#endif


extern void
decode_img16(unsigned short *tp, const unsigned short *sp,
             int h, const int disp_width);


/**
 * Zeichnet Bild mit Clipping
 * @author Hj. Malthaner
 */
static void
display_img_wc(const int n, const int xp, const int yp, const PIXVAL *sp)
{
	int h = images[n].h;
	int y = yp + images[n].y;
        int yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	if(h > 0) {
	    PIXVAL *tp = textur + y*disp_width;

	    // oben clippen


	    while(yoff) {
		yoff --;
		do {
		    // clear run + colored run + next clear run
		    ++sp;
		    sp += *sp + 1;
		} while(*sp);
		sp ++;
	    }


	    do { // zeilen dekodieren
                int xpos = xp;

		// bild darstellen

		int runlen = *sp++;

		do {
		    // wir starten mit einem clear run

		    xpos += runlen;

		    // jetzt kommen farbige pixel

		    runlen = *sp++;

		    // Hajo: something to display?

		    if(xpos+runlen >= clip_rect.x &&
                       xpos <= clip_rect.xx) {
			const int left = xpos >= clip_rect.x ? 0 : clip_rect.x-xpos;
			const int len  = clip_rect.xx-xpos+1 >= runlen ? runlen : clip_rect.xx-xpos+1;

			pixcopy(tp+xpos+left, sp+left, len-left);
		    }

		    sp += runlen;
		    xpos += runlen;

		} while( (runlen = *sp ++) );

		tp += disp_width;

	    } while(--h);
	}
}

#ifdef USE_C

/**
 * Zeichnet Bild ohne Clipping
 * @author Hj. Malthaner
 */
static void
display_img_nc(const int n, const int xp, const int yp, const PIXVAL *sp)
{
	unsigned char h = images[n].h;

	if(h > 0) {
	    PIXVAL *tp = textur + xp + (yp + images[n].y)*disp_width;


	    // bild darstellen

	    do { // zeilen dekodieren

		unsigned int runlen = *sp++;
		PIXVAL *p = tp;
		// eine Zeile dekodieren
		do {

		    // wir starten mit einem clear run
		    p += runlen;

		    // jetzt kommen farbige pixel

		    runlen = *sp++;
		    PCPY(p, sp, runlen);

		} while( (runlen = *sp++) );

		tp += disp_width;

	    } while(--h);

	}
}

#endif


/**
 * Zeichnet Bild
 * @author Hj. Malthaner
 */
void display_img(const int n, const int xp, const int yp,
		 const char daynight, const char dirty)
{
    if(n >= 0 && n < anz_images) {
	const int x = images[n].x;
	const int y = images[n].y;
	const int w = images[n].w;
	const int h = images[n].h;

	const PIXVAL *sp = daynight ? images[n].data : images[n].recoded_base_data;

	if(dirty) {
	    mark_rect_dirty_wc(xp+x,
                               yp+y,
			       xp+x+w-1,
			       yp+y+h-1);
        }


	if(xp+x>=clip_rect.x && yp+y>=clip_rect.y &&
	   xp+x+w-1 <= clip_rect.xx && yp+y+h-1 <= clip_rect.yy) {
	  display_img_nc(n, xp, yp, sp);
	}
	else if( xp < clip_rect.xx && yp < clip_rect.yy &&
                 xp+x+w>clip_rect.x && yp+y+h>clip_rect.y ) {

	  display_img_wc(n, xp, yp, sp);
	}
    }
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static void
display_color_img_aux(const int n, const int xp, const int yp,
		      const int color)
{
    int h = images[n].h;
    int y = yp + images[n].y;
    int yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

    if(h > 0) {
        // color replacement needs the original data
	const PIXVAL *sp = images[n].zoom_data;
	PIXVAL *tp = textur + y*disp_width;

	// oben clippen

	while(yoff) {
	    yoff --;
	    do {
		// clear run + colored run + next clear run
	        ++sp;
		sp += *sp + 1;
	    } while(*sp);
	    sp ++;
        }

	do { // zeilen dekodieren
	    int xpos = xp;

	    // bild darstellen

	    int runlen = *sp++;

	    do {
	        // wir starten mit einem clear run

	        xpos += runlen;

		// jetzt kommen farbige pixel

		runlen = *sp++;

		// Hajo: something to display?

		if(xpos+runlen >= clip_rect.x &&
		   xpos <= clip_rect.xx) {
		    const int left = xpos >= clip_rect.x ? 0 : clip_rect.x-xpos;
		    const int len  = clip_rect.xx-xpos+1 >= runlen ? runlen : clip_rect.xx-xpos+1;

		    colorpixcopy(tp+xpos+left, sp+left, sp+len, color);
		}

		sp += runlen;
		xpos += runlen;

	    } while( (runlen = *sp ++) );

	    tp += disp_width;

	} while(--h);
    }
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
void
display_color_img(const int n, const int xp, const int yp, const int color,
		  const char daynight, const char dirty)
{
    if(n>=0 && n<anz_images) {
	if(dirty) {
	    mark_rect_dirty_wc(xp+images[n].x-8,
	                       yp+images[n].y-4,
			       xp+images[n].x+images[n].w+8-1,
			       yp+images[n].y+images[n].h+4-1);
        }

	// Hajo: since the colors for player 0 are already right,
	// only use the expensive replacement routine for colored images
	// of other players

	// Hajo: player 1 does not need any recoloring, too
	// thus only colors >= 8 must be recolored

	if(color >= 8 || daynight == FALSE) {
	  // Hajo: choose mapping table
	  rgbmap_current = daynight ? rgbmap_day_night : rgbmap_all_day;
	  specialcolormap_current = daynight ? specialcolormap_day_night : specialcolormap_all_day;

	  // Hajo: Simutrans used to use only 4 shades per color
	  // but now we use 16 player color, so we need to multiply
	  // the color value here
	  display_color_img_aux(n, xp, yp, color << 2);

	} else {
	  const PIXVAL *sp = images[n].data;

	  display_img_wc(n, xp, yp, sp);
	}

	/*
	xp += (images[n].x + images[n].w)/2-4;
	yp += images[n].y - 4;

	display_fillbox_wh(xp, yp,   8, 1, color+2, dirty);
	display_fillbox_wh(xp, yp+1, 8, 1, color+3, dirty);
	display_fillbox_wh(xp, yp+2, 8, 1, color+1, dirty);
	*/
    }
}

/**
 * Zeichnet ein Pixel
 * @author Hj. Malthaner
 */
void
display_pixel(int x, int y, int color)
{
    if(x >= clip_rect.x && x<=clip_rect.xx &&
       y >= clip_rect.y && y<=clip_rect.yy) {

        PIXVAL * const p = textur + x + y*disp_width;
	*p = rgbcolormap[color];

	mark_tile_dirty(x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT);
    }
}


/**
 * Zeichnet einen Text, lowlevel Funktion
 * @author Hj. Malthaner
 */
static void
dr_textur_text(PIXVAL *textur,
               const unsigned char *fontdata,
	       int fw, int fh,
               int x,int y,
               const char *txt,
               const int chars,
               const int fgpen,
	       const int dirty)
{
    if(y >= clip_rect.y && y+fh <= clip_rect.yy) {
	const PIXVAL colval = rgbcolormap[fgpen];
	int screen_pos = y*disp_width + x;
	int p;

	if(dirty) {
	    mark_rect_dirty_wc(x, y, x+chars*fw-1, y+fh-1);
	}


	for(p=0; p<chars; p++) {      /* Zeichen fuer Zeichen ausgeben */
	    int base=((unsigned char *)txt)[p] * fh;                 /* 8 Byte je Zeichen */
	    const int end = base+fh;

	    do {
		const int c=fontdata[base++];       /* Eine Zeile des Zeichens */
		int b;

		for(b=0; b<fw; b++) {
		    if(c & (128 >> b) &&
                       b+x >= clip_rect.x &&
                       b+x <= clip_rect.xx) {
			textur[screen_pos+b] = colval;
		    }
		}
		screen_pos += disp_width;
	    } while(base < end);

            x += fw;
	    screen_pos += fw - disp_width*fh;
	}
    }
}


/**
 * Zeichnet Text, highlevel Funktion
 * @author Hj. Malthaner
 */
void
display_text(int font, int x, int y, const char *txt, int color, int dirty)
{
    const int chars = strlen(txt);

    if(font == 1) {
	dr_textur_text(textur, dr_fonttab, 8, 8, x, y, txt, chars, color, dirty);
    } else {
	dr_textur_text(textur, dr_4x7font, 4, 7, x, y, txt, chars, color, dirty);
    }
}



/**
 * Zeichnet gefuelltes Rechteck
 * @author Hj. Malthaner
 */
static
#ifndef _MSC_VER
inline
#endif
void display_fb_internal(int xp, int yp, int w, int h,
			 const int color, const int dirty,
			 const int cL, const int cR, const int cT, const int cB)
{
    clip_lr(&xp, &w, cL, cR);
    clip_lr(&yp, &h, cT, cB);

    if(w > 0 && h > 0) {
        PIXVAL *p = textur + xp + yp*disp_width;
	const PIXVAL colval = color >= 0x8000 ? specialcolormap_all_day[(color & 0x7FFF)]: rgbcolormap[color];
	const unsigned long longcolval = (colval << 16) | colval;

	if(dirty) {
	    mark_rect_dirty_nc(xp, yp, xp+w-1, yp+h-1);
	}
	do {

	  int count = w >> 1;
	  while(count--) {
	    *((unsigned long*)p)++ = longcolval;
	  }
	  if(w & 1) {
	    *p++ = colval;
	  }

	  p += disp_width-w;

	} while(--h);
    }
}

void
display_fillbox_wh(int xp, int yp, int w, int h,
		   int color, int dirty)
{
  display_fb_internal(xp,yp,w,h,color,dirty,
		      0,disp_width-1,0,disp_height-1);
}

void
display_fillbox_wh_clip(int xp, int yp, int w, int h,
			int color, int dirty)
{
  display_fb_internal(xp,yp,w,h,color,dirty,
		      clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet vertikale Linie
 * @author Hj. Malthaner
 */
void
display_vl_internal(const int xp, int yp, int h, const int color, int dirty,
		    int cL, int cR, int cT, int cB)
{
    clip_lr(&yp, &h, cT, cB);

    if(xp >= cL && xp <= cR && h > 0) {
        PIXVAL *p = textur + xp + yp*disp_width;
	const PIXVAL colval = rgbcolormap[color];

        if (dirty) {
	  mark_rect_dirty_nc(xp, yp, xp, yp+h-1);
	}

	do {
	    *p = colval;
            p += disp_width;
	} while(--h);
    }
}


void
display_vline_wh(const int xp, int yp, int h, const int color, int dirty)
{
  display_vl_internal(xp,yp,h,color,dirty,
		      0,disp_width-1,0,disp_height-1);
}


void
display_vline_wh_clip(const int xp, int yp, int h, const int color, int dirty)
{
  display_vl_internal(xp,yp,h,color,dirty,
		      clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet rohe Pixeldaten
 * @author Hj. Malthaner
 */

void
display_array_wh(int xp, int yp, int w, int h, const unsigned char *arr)
{
    const int arr_w = w;
    const int xoff = clip_wh(&xp, &w, clip_rect.x, clip_rect.xx);
    const int yoff = clip_wh(&yp, &h, clip_rect.y, clip_rect.yy);

    if(w > 0 && h > 0) {
        PIXVAL *p = textur + xp + yp*disp_width;

	mark_rect_dirty_nc(xp, yp, xp+w-1, yp+h-1);

	if(xp == clip_rect.x) {
  	    arr += xoff;
	}
	if(yp == clip_rect.y) {
  	    arr += yoff*arr_w;
	}

	do {
	    const PIXVAL *const pe = p + w;
	    do {
		*p++ = rgbcolormap[*arr++];
	    } while(p < pe);

	    arr += arr_w - w;
            p += disp_width - w;
	} while(--h);
    }
}


// --------------- compound painting procedures ---------------

/**
 * proportional_string_width with a text of a given length
 *
 * @author Volker Meyer
 * @date  15.06.2003
 */
int proportional_string_len_width(const char *text, int len)
{
  int c, width = 0;
  char *char_next = npr_fonttab+256; // points to pixel-to-next-char

  while((c = *text++) && len--) { width += char_next[c]; }
  return width;
}

int proportional_string_width(const char *text)
{
  int c, width = 0;
  char *char_next = npr_fonttab+256; // points to pixel-to-next-char

  while((c = *text++)) { width += char_next[c]; }
  return width;
}

// if width equals zero, take default value
void display_ddd_proportional(int xpos, int ypos, int width, int hgt,
			      int ddd_farbe, int text_farbe,
			      const char *text, int dirty)
{
  display_fillbox_wh(xpos-2, ypos-6-hgt, width,  1, ddd_farbe+1, dirty);
  display_fillbox_wh(xpos-2, ypos-5-hgt, width, 10, ddd_farbe,   dirty);
  display_fillbox_wh(xpos-2, ypos+5-hgt, width,  1, ddd_farbe-1, dirty);

  display_vline_wh(xpos-2, ypos-6-hgt, 11, ddd_farbe+1, dirty);
  display_vline_wh(xpos+width-3, ypos-6-hgt, 11, ddd_farbe-1, dirty);

  display_proportional(xpos+2, ypos-9-hgt, text, ALIGN_LEFT, text_farbe,FALSE);
}

/**
 * display text in 3d box with clipping
 * @author: hsiegeln
 */
void display_ddd_proportional_clip(int xpos, int ypos, int width, int hgt,
			      int ddd_farbe, int text_farbe,
			      const char *text, int dirty)
{
  display_fillbox_wh_clip(xpos-2, ypos-6-hgt, width,  1, ddd_farbe+1, dirty);
  display_fillbox_wh_clip(xpos-2, ypos-5-hgt, width, 10, ddd_farbe,   dirty);
  display_fillbox_wh_clip(xpos-2, ypos+5-hgt, width,  1, ddd_farbe-1, dirty);

  display_vline_wh_clip(xpos-2, ypos-6-hgt, 11, ddd_farbe+1, dirty);
  display_vline_wh_clip(xpos+width-3, ypos-6-hgt, 11, ddd_farbe-1, dirty);

  display_proportional_clip(xpos+2, ypos-4-hgt, text, ALIGN_LEFT, text_farbe,FALSE);
}

// clip Left/Right/Top/Bottom
/**
 * len parameter added - use -1 for previous bvbehaviour.
 * @author Volker Meyer
 * @date  15.06.2003
 */
void display_p_internal(int x, int y, const char *txt, int len,
			int align,
			const int color_index,
			int dirty,
			int cL, int cR, int cT, int cB)
{
  int c;
  int text_width;
  int char_width_1, char_width_2; // 1 is char only, 2 includes room
  int screen_pos;
  unsigned char *char_next = npr_fonttab+256; // points to pixel-to-next-char
  unsigned char *char_data = npr_fonttab+512; // points to font data
  unsigned char *p;
  int yy = y+9;
  const PIXVAL color = rgbcolormap[color_index];

  text_width = proportional_string_len_width(txt, len);

  // adapt x-coordinate for alignment
  switch(align) {
   case ALIGN_LEFT: break;
   case ALIGN_MIDDLE: x -= (text_width>>1); break;
   case ALIGN_RIGHT: x -= text_width; break;
  }

  if (dirty) {
    mark_rect_dirty_wc(x, y, x+text_width-1, y+10-1);
  }

  // big loop, char by char
  while ((c = *((unsigned char *)txt)++) && len--) {
    char_width_1 = npr_fonttab[c];
    char_width_2 = char_next[c];
    screen_pos = y*disp_width + x;

    if (x>=cL && x+char_width_1<=cR) { // no horizontal blocking
      if (y>=cT && yy<=cB) { // no vertical blocking, letter fully visible
	int h,dat;

	p = char_data+c*10;
	for (h=0; h<10; h++) {
	  dat = *p++;
	  if (dat) { // a lot of times just empty
	    if (dat & 128) textur[screen_pos]   = color;
	    if (dat &  64) textur[screen_pos+1] = color;
	    if (dat &  32) textur[screen_pos+2] = color;
	    if (dat &  16) textur[screen_pos+3] = color;
	    if (dat &   8) textur[screen_pos+4] = color;
	    if (dat &   4) textur[screen_pos+5] = color;
	    if (dat &   2) textur[screen_pos+6] = color;
	    if (dat &   1) textur[screen_pos+7] = color;
	  }
	  screen_pos += disp_width;
	}
      } else if (y>=cT || yy<=cB) { // top or bottom half visible
	int h,yh,dat;

	p = char_data+c*10;
	for (h=0, yh=y; h<10; yh++, h++) {
	  dat = *p++;
	  if (dat) { // a lot of times just empty
	    if (yh>=cT && yh<=cB) {
	      if (dat & 128) textur[screen_pos]   = color;
	      if (dat &  64) textur[screen_pos+1] = color;
	      if (dat &  32) textur[screen_pos+2] = color;
	      if (dat &  16) textur[screen_pos+3] = color;
	      if (dat &   8) textur[screen_pos+4] = color;
	      if (dat &   4) textur[screen_pos+5] = color;
	      if (dat &   2) textur[screen_pos+6] = color;
	      if (dat &   1) textur[screen_pos+7] = color;
	    }
	  }
	  screen_pos += disp_width;
	}
      }
    } else if (x>=cL || x+char_width_1<=cR) { // left or right part visible
      if (y>=cT && yy<=cB) { // no vertical blocking
	int h,b,b2,dat;

	p = char_data+c*10;
	for (h=0; h<10; h++) {
	  dat = *p++;
	  if (dat) { // a lot of times just empty
	    for (b=0, b2=128; b<8; b++, b2>>=1) {
	      if (x+b < cL) continue;
	      if (x+b > cR) break;
	      if (dat & b2) textur[screen_pos+b] = color;
	    }
	  }
	  screen_pos += disp_width;
	}
      }
    } else { // something else
    }

    x += char_width_2;
  }

}
// proportional text routine
// align: ALIGN_LEFT, ALIGN_MIDDLE, ALIGN_RIGHT
void display_proportional(int x, int y, const char *txt,
			  int align, const int color, int dirty) {
  // clip vertically
  if(y < 8 || y >= disp_height-13) { return; }
  // amiga <-> pc correction
  y+=4;

  display_p_internal(x,y,txt,-1,align,color,dirty,0,disp_width-1,0,disp_height-1);
}


void display_proportional_clip(int x, int y, const char *txt,
			       int align, const int color,
			       int dirty)
{
    display_p_internal(x,y,txt,-1,align,color,dirty,
		       clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}

/**
 * This version of display_proportional_clip displays text of fixed length.
 * @author Volker Meyer
 * @date  15.06.2003
 */
void display_proportional_len_clip(int x, int y, const char *txt, int len,
			       int align, const int color,
			       int dirty)
{
    display_p_internal(x,y,txt,len,align,color,dirty,
		       clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}

/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void
display_ddd_box(int x1, int y1, int w, int h, int tl_color, int rd_color)
{
    display_fillbox_wh(x1, y1, w, 1, tl_color, TRUE);
    display_fillbox_wh(x1, y1+h-1, w, 1, rd_color, TRUE);

    h-=2;

    display_vline_wh(x1, y1+1, h, tl_color, TRUE);
    display_vline_wh(x1+w-1, y1+1, h, rd_color, TRUE);
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void
display_ddd_box_clip(int x1, int y1, int w, int h, int tl_color, int rd_color)
{
    display_fillbox_wh_clip(x1, y1, w, 1, tl_color, TRUE);
    display_fillbox_wh_clip(x1, y1+h-1, w, 1, rd_color, TRUE);

    h-=2;

    display_vline_wh_clip(x1, y1+1, h, tl_color, TRUE);
    display_vline_wh_clip(x1+w-1, y1+1, h, rd_color, TRUE);
}


/**
 * Zeichnet schattierten  Text
 * @author Hj. Malthaner
 */
void
display_ddd_text(int xpos, int ypos, int hgt,
                 int ddd_farbe, int text_farbe,
                 const char *text, int dirty)
{
    const int len = strlen(text)*4;

    display_fillbox_wh(xpos-2-len,
                       ypos-hgt-6,
                       4 + len*2, 1,
                       ddd_farbe+1,
		       dirty);
    display_fillbox_wh(xpos-2-len,
                       ypos-hgt-5,
                       4 + len*2, 8,
                       ddd_farbe,
		       dirty);
    display_fillbox_wh(xpos-2-len,
                       ypos-hgt+3,
                       4 + len*2, 1,
                       ddd_farbe-1,
		       dirty);

    display_text(1, xpos - len,
                 ypos-hgt-9,
                 text,
		 text_farbe,
		 dirty);
}


/**
 * Zaehlt Vorkommen ein Buchstabens in einem String
 * @author Hj. Malthaner
 */
int
count_char(const char *str, const char c)
{
    int count = 0;

    while(*str) {
	count += (*str++ == c);
    }
    return count;
}
/**
 * Zeichnet einen mehrzeiligen Text
 * @author Hj. Malthaner
 *
 * Better performance without copying text!
 * @author Volker Meyer
 * @date  15.06.2003
 */
void
display_multiline_text(int x, int y, const char *buf, int color)
{
    if(buf && *buf) {
	char *next;

	do {
	    next = strchr(buf,'\n');
	    display_proportional_len_clip(
		x, y,
		buf, next ? next - buf : -1,
		ALIGN_LEFT, color, TRUE);
	    buf = next + 1;
	    y += LINESPACE;
	} while(next);
    }
}


/**
 * Loescht den Bildschirm
 * @author Hj. Malthaner
 */
void
display_clear()
{
    memset(textur+32*disp_width, 32, disp_width*(disp_height-17-32));

    mark_rect_dirty_nc(0, 0, disp_width-1, disp_height-1);
}



void display_flush_buffer()
{
    int x, y;
    char * tmp;
#ifdef DEBUG
    // just for debugging
    int tile_count = 0;
#endif

    if(use_softpointer) {
	if(softpointer != 52) {
	    ex_ord_update_mx_my();
	    display_img(softpointer, sys_event.mx, sys_event.my, FALSE, TRUE);
	}
	old_my = sys_event.my;
    }

    for(y=0; y<tile_lines; y++) {
#ifdef DEBUG

	for(x=0; x<tiles_per_line; x++) {
	    if(is_tile_dirty(x, y)) {
		display_fillbox_wh(x << DIRTY_TILE_SHIFT,
                                   y << DIRTY_TILE_SHIFT,
				   DIRTY_TILE_SIZE/4,
				   DIRTY_TILE_SIZE/4,
				   3,
				   FALSE);



		dr_textur(x << DIRTY_TILE_SHIFT,
                          y << DIRTY_TILE_SHIFT,
			  DIRTY_TILE_SIZE,
			  DIRTY_TILE_SIZE);

		tile_count ++;
	    } else {
		display_fillbox_wh(x << DIRTY_TILE_SHIFT,
                                   y << DIRTY_TILE_SHIFT,
				   DIRTY_TILE_SIZE/4,
				   DIRTY_TILE_SIZE/4,
				   0,
				   FALSE);



		dr_textur(x << DIRTY_TILE_SHIFT,
                          y << DIRTY_TILE_SHIFT,
			  DIRTY_TILE_SIZE,
			  DIRTY_TILE_SIZE);

	    }
	}
#else
	x = 0;

	do {
	    if(is_tile_dirty(x, y)) {
		const int xl = x;
                do {
		    x++;
		} while(x < tiles_per_line && is_tile_dirty(x, y));

		dr_textur(xl << DIRTY_TILE_SHIFT,
                          y << DIRTY_TILE_SHIFT,
			  (x-xl)<<DIRTY_TILE_SHIFT,
			  DIRTY_TILE_SIZE);

	    }
	    x++;
	} while(x < tiles_per_line);
#endif
    }

#ifdef DEBUG
//    printf("%d von %d tiles wurden gezeichnet\n", tile_count, tile_lines*tiles_per_line);
#endif

    dr_flush();

    // swap tile buffers
    tmp = tile_dirty_old;
    tile_dirty_old = tile_dirty;

    tile_dirty = tmp;
    memset(tile_dirty, 0, tile_buffer_length);
}


/**
 * Bewegt Mauszeiger
 * @author Hj. Malthaner
 */
void display_move_pointer(int dx, int dy)
{
    move_pointer(dx, dy);
}


/**
 * Schaltet Mauszeiger sichtbar/unsichtbar
 * @author Hj. Malthaner
 */
void display_show_pointer(int yesno)
{
    if(use_softpointer) {
	if(yesno) {
	    softpointer = 261;
	} else {
	    softpointer = 52;
	}
    } else {
	show_pointer(yesno);
    }
}


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int
simgraph_init(int width, int height, int use_shm, int do_sync)
{
    int parameter[2];
    int ok;

    parameter[0]=use_shm;
    parameter[1]=do_sync;

    dr_os_init(2, parameter);

    ok = dr_os_open(width, height);

    if(ok) {
	unsigned int i;

	disp_width = width;
	disp_height = height;

        textur = dr_textur_init();
	use_softpointer = dr_use_softpointer();

	init_font(FONT_PATH_X "draw.fnt", FONT_PATH_X "prop.fnt");
	load_hex_font(FONT_PATH_X "4x7.hex");


	load_special_palette();

	display_set_color(1);


	for(i=0; i<256; i++) {
	    rgbcolormap[i] = get_system_color(day_pal[i*3], day_pal[i*3+1], day_pal[i*3+2]);
	}

    } else {
	puts("Error  : can't open window!");
        exit(-1);
    }


    // allocate dirty tile flags
    tiles_per_line = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
    tile_lines = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
    tile_buffer_length = (tile_lines*tiles_per_line+7) / 8;

    tile_dirty = guarded_malloc( tile_buffer_length );
    tile_dirty_old = guarded_malloc( tile_buffer_length );

    memset(tile_dirty, 255, tile_buffer_length);
    memset(tile_dirty_old, 255, tile_buffer_length);

    display_setze_clip_wh(0, 0, disp_width, disp_height);

    display_day_night_shift(0);
    display_set_player_color(0);


    // Hajo: Calculate daylight rgbmap and save it for unshaded
    // tile drawing
    calc_base_pal_from_night_shift(0);
    memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE*sizeof(PIXVAL));
    memcpy(specialcolormap_all_day, specialcolormap_day_night, 256*sizeof(PIXVAL));

    printf("Init. done.\n");

    return TRUE;
}


/**
 * Prueft ob das Grafikmodul schon init. wurde
 * @author Hj. Malthaner
 */
int is_display_init()
{
    return textur != NULL;
}


/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
int
simgraph_exit()
{
  // printf("Old images:\n");

    guarded_free(tile_dirty);
    guarded_free(tile_dirty_old);
    guarded_free(images);

    tile_dirty = tile_dirty_old = NULL;
    images = NULL;

    return dr_os_close();
}


/**
 * Speichert Screenshot
 * @author Hj. Malthaner
 */
void display_snapshot()
{
    static int number = 0;

    char buf[80];

#ifdef __MINGW32__
    mkdir(SCRENSHOT_PATH);
#else
    mkdir(SCRENSHOT_PATH, 0700);
#endif

    do {
	sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.bmp", number++);
    } while(access(buf, W_OK) != -1);

    dr_screenshot (buf);
}


/**
 * Laedt Einstellungen
 * @author Hj. Malthaner
 */
void display_laden(void * file, int zipped)
{
  int i;

    if(zipped) {
        char line[80];
	char *ptr = line;

	gzgets(file, line, sizeof(line));

	light_level = atoi(ptr);
	while(*ptr && *ptr++ != ' ');
	color_level = atoi(ptr);
	while(*ptr && *ptr++ != ' ');
	night_shift = atoi(ptr);

	gzgets(file, line, sizeof(line));
	i = atoi(line);
    } else {
	fscanf(file, "%d %d %d\n", &light_level, &color_level, &night_shift);

	fscanf(file, "%d\n", &i);
    }
    if(i < 0 || i > 15) {
	i = 0;
    }
    display_set_light(light_level);
    display_set_color(color_level);
    display_set_player_color(i);
}



/**
 * Speichert Einstellungen
 * @author Hj. Malthaner
 */
void display_speichern(void* file, int zipped)
{
    if(zipped) {
        gzprintf(file, "%d %d %d\n", light_level, color_level, night_shift);

	gzprintf(file, "%d\n", selected_player_color_set);
    } else {
        fprintf(file, "%d %d %d\n", light_level, color_level, night_shift);

	fprintf(file, "%d\n", selected_player_color_set);
    }
}

/**
* zeichnet linie von x,y nach xx,yy
* von Hajo
**/

void
display_direct_line(const int x,const int y,const int xx,const int yy, const int color)
{
   int i, steps;
   int xp,yp;
   int xs,ys;

   const int dx=xx-x;
   const int dy=yy-y;

   if(abs(dx)>abs(dy)) steps=abs(dx); else steps=abs(dy);

   if(steps == 0) steps = 1;

   xs=(dx << 16) / steps;
   ys=(dy << 16) / steps;

   xp=(x << 16);
   yp=(y << 16);

   for(i = 0; i <= steps; i++) {
	 display_pixel(xp >> 16,yp >> 16, color);
	 xp+=xs;
	 yp+=ys;
   }
}
