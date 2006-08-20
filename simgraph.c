/*
 * Copyright (c) 2001 Hansjörg Malthaner
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
#undef DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "simgraph.h"
#include "simsys.h"
#include "simmem.h"
#include "simdebug.h"
#include "utils/writepcx.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

// since 0.80.pre1 Simutrans can read all config files
// from a directory:
// this is set from simgraph_init()
static const char * configdir = 0;


// do we need a mouse pointer emulation ?
// assume yes by default, but the system wrapper
// is inquired during initialisation
static int use_softpointer = 1;


static int softpointer = 261;
static int old_my = -1;              // die icon leiste muss neu gezeichnet werden wenn der
                                     // Mauszeiger darueber schwebt


static unsigned char dr_fonttab[2048];  /* Unser Zeichensatz sitzt hier */
static unsigned char npr_fonttab[3072]; // Niels: npr are my initials

static unsigned char dr_4x7font[7*256];


/*
 * pixel value type, colormap index
 */
typedef unsigned char PIXVAL;


struct imd {
    signed char x;
    signed char w;
    signed char y;
    signed char h;
    int len;
    PIXVAL * data;
};


static int anz_images = 0;

static int disp_width = 640;
static int disp_height = 480;

static struct imd *images = NULL;


static PIXVAL *textur = NULL;


static struct clip_dimension clip_rect;

// dirty tile management strcutures

#define DIRTY_TILE_SIZE 16
#define DIRTY_TILE_SHIFT 4

static char *tile_dirty = NULL;
static char *tile_dirty_old = NULL;

static int tiles_per_line = 0;
static int tile_lines = 0;


// colormap management structures

static unsigned char day_pal[256*3];
static unsigned char night_pal[256*3];
static unsigned char base_pal[256*3];

static int light_level = 0;
static int color_level = 1;


// tile raster width
static int tile_raster_width = 64;


int get_tile_raster_width()
{
  return tile_raster_width;
}


void set_tile_raster_width(int rw)
{
  tile_raster_width = rw
}



// ------------ procedure like defines --------------------


#define mark_tile_dirty(x,y) tile_dirty[(x) + (y)*tiles_per_line] = 1
#define mark_tiles_dirty(x1, x2, y) {const int line = (y) * tiles_per_line; int x; for(x=(x1); x<=(x2); x++) {	tile_dirty[x + line] = 1; } };

#define is_tile_dirty(x,y) ((tile_dirty[(x) + (y)*tiles_per_line]) || (tile_dirty_old[(x) + (y)*tiles_per_line]))


// ----------------- procedures ---------------------------


/**
 * Markiert ein Tile as schmutzig, ohne Clipping
 * @author Hj. Malthaner
 */
static void mark_rect_dirty_nc(int x1, int y1, int x2, int y2)
{
    int y;

    // floor to tile size

    x1 >>= DIRTY_TILE_SHIFT;
    y1 >>= DIRTY_TILE_SHIFT;
    x2 >>= DIRTY_TILE_SHIFT;
    y2 >>= DIRTY_TILE_SHIFT;


    assert(x1 >= 0);
    assert(x1 < tiles_per_line);
    assert(y1 >= 0);
    assert(y1 < tile_lines);
    assert(x2 >= 0);
    assert(x2 < tiles_per_line);
    assert(y2 >= 0);
    assert(y2 < tile_lines);


    for(y=y1; y<=y2; y++) {
	mark_tiles_dirty(x1, x2, y);
    }
}


/**
 * Markiert ein Tile as schmutzig, mit Clipping
 * @author Hj. Malthaner
 */
void mark_rect_dirty_wc(int x1, int y1, int x2, int y2)
{
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 < 0) x2 = 0;
    if(y2 < 0) y2 = 0;

    if(x1 >= disp_width) {
	x2 = x1 = disp_width-1;
    } else if(x2 >= disp_width) {
	x2 = disp_width-1;
    }

    if(y1 >= disp_height) {
	y2 = y1 = disp_height-1;
    } else if(y2 >= disp_height) {
	y2 = disp_height-1;
    }

    mark_rect_dirty_nc(x1, y1, x2, y2);
}


/**
 * Clipped einen Wert in Intervall
 * @author Hj. Malthaner
 */
static int clip(int w, int u, int o)
{
    return w < u ? u : w > o ? o : w;
}


/**
 * Laedt den Font
 * @author Hj. Malthaner
 */
static void init_font()
{
    FILE *f = NULL;

    // suche in ./draw.fnt

    if(f==NULL ) {
	f=fopen("./draw.fnt","rb");
    }

    if(f==NULL) {
	printf("Error: Cannot open draw.fnt!\n");
	exit(1);
    } else {
	int i;
	for(i=0;i<2048;i++) dr_fonttab[i]=getc(f);
	    fclose(f);
    }


    // also, read font.dat
    {
      int i,r;
      f = fopen("prop.fnt","rb");
      if (f==NULL) {
	printf("Error: Cannot open prop.fnt!\n");
	exit(1);
      }
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

    strcpy(fname, configdir);
    strcat(fname, filename);

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
	dr_setRGB8multi(0, anzahl, palette);

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


static void load_4x7_font()
{
    FILE *file = fopen("4x7.hex", "rb");

    int fh = 7;

    if(file) {
	char buf [80];
        int  n, line;
        char *p;

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

    } else {
	fprintf(stderr, "Error: can't open file '%s' for reading\n", "4x7.hex");
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
    unsigned char palette[256*3];
    const double ll = 1.0 - light_level/20.0;
    int i;

    light_level = new_light_level;

    for(i=0; i<256; i++) {
	const int n = i*3;
	palette[n+0] = clip((int)(pow(base_pal[n+0]/255.0, ll)*255.0), 0, 255);
	palette[n+1] = clip((int)(pow(base_pal[n+1]/255.0, ll)*255.0), 0, 255);
	palette[n+2] = clip((int)(pow(base_pal[n+2]/255.0, ll)*255.0), 0, 255);
    }

    dr_setRGB8multi(0, 256, palette);
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
	load_palette("simud70.pal", day_pal);
	load_palette("simun70.pal", night_pal);
	break;
    case 1:
	load_palette("simud80.pal", day_pal);
	load_palette("simun80.pal", night_pal);
	break;
    case 2:
	load_palette("simud90.pal", day_pal);
	load_palette("simun90.pal", night_pal);
	break;
    case 3:
	load_palette("simud100.pal", day_pal);
	load_palette("simun100.pal", night_pal);
	break;
    }

    memcpy(base_pal, day_pal, 256*3);
    display_set_light(display_get_light());
}


static int night_shift = -1;

static void calc_base_pal_from_night_shift(const int night)
{
    const int day = 4 - night;
    int i;

    for(i=0; i<256; i++) {
	base_pal[i*3+0] = (day_pal[i*3+0] * day + night_pal[i*3+0] * night) >> 2;
	base_pal[i*3+1] = (day_pal[i*3+1] * day + night_pal[i*3+1] * night) >> 2;
	base_pal[i*3+2] = (day_pal[i*3+2] * day + night_pal[i*3+2] * night) >> 2;
    }
}


void display_day_night_shift(int night)
{
    if(night != night_shift) {
	night_shift = night;

	calc_base_pal_from_night_shift(night);

	display_set_light(light_level);
	mark_rect_dirty_nc(0,0, disp_width-1, disp_height-1);
    }
}



/**
 * Setzt Farbeintrag
 * @author Hj. Malthaner
 */
void display_set_player_colors(const unsigned char *day, const unsigned char *night)
{
    memcpy(day_pal, day, 12);
    memcpy(night_pal, night, 12);

    calc_base_pal_from_night_shift(night_shift);

    display_set_light(light_level);
    mark_rect_dirty_nc(0,0, disp_width-1, disp_height-1);
}


/**
 * Liest 32Bit wert Plattfromunabhängig
 * @author Hj. Malthaner
 */
static int fread_int(FILE *f)
{
    int i = 0;

    i += fgetc(f);
    i += fgetc(f) << 8;
    i += fgetc(f) << 16;
    i += fgetc(f) << 24;

    return i;
}


/**
 * Laedt daten.pak
 * @author Hj. Malthaner
 */
static void init_images(const char *filename)
{
    FILE *f = fopen(filename, "rb");

    if( f ) {
	int i;

	anz_images = fread_int(f);

        // aligned on 8 byte doesn't hurt as it seems?!
	images = (struct imd *) guarded_malloc(sizeof(struct imd)*anz_images);

	for(i=0; i<anz_images; i++) {
	    images[i].x    = fgetc(f);
            images[i].w    = fgetc(f);
            images[i].y    = fgetc(f);
            images[i].h    = fgetc(f);

            images[i].len  = fread_int(f);

	    if(images[i].h > 0) {
                images[i].data = malloc(images[i].len*sizeof(PIXVAL));
		fread(images[i].data, images[i].len*sizeof(PIXVAL), 1, f);

	    } else {
		images[i].data = NULL;
	    }
	}

	fclose(f);
    } else {
	printf("Error: can't read daten pak file '%s', aborting.\n",
	       filename);
	exit(1);
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
static int clip_lr(int *x, int *w, int left, int right)
{
  int l = *x;      // leftmost pixel
  int r = *x+*w-1; // rightmost pixel

  if (l > right) { *w = 0; return FALSE; }
  if (r < left)  { *w = 0; return FALSE; }

  // there is something to show.
  if (l < left)  { *w -= left - l;  *x = l = left; }
  if (r > right) { *w -= r - right; /* r = right */ }
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


/**
 * Kopiert Pixel von src nach dest
 * @author Hj. Malthaner
 */
#ifdef USE_C

static void pixcopy(PIXVAL *dest,
                    const PIXVAL *src,
                    int len)
{
    *dest;

    switch(len & 3) {
    case 3: *dest++ = *src++;
    case 2: *((short *)dest)++ = *((short *)src)++;
	break;
    case 1: *dest++ = *src++;
    }

    len >>= 2;

    while(len) {
	len --;
	*((long*)dest)++ = *((long*)src)++;
    }
}


/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static void colorpixcpy(PIXVAL *dest, const PIXVAL *src,
                        const PIXVAL * const end,
			const PIXVAL color)
{
    unsigned char c;

    while(src < end) {
	c = *src++;
	*dest++ = (c > (unsigned char)3) ? c : c | color;
    }
}

#else

extern void pixcopy(unsigned char *dest,
                    const unsigned char *src,
                    int len);

extern void colorpixcpy(unsigned char *dest, const unsigned char *src,
                        const unsigned char * const end, const unsigned char color);

extern void
decode_img(unsigned char *tp, const unsigned char *sp,
           int h, const int disp_width);

#endif


/**
 * Zeichnet Bild mit Clipping
 * @author Hj. Malthaner
 */
static void
display_img_wc(const int n, const int xp, const int yp)
{
    if(n >= 0 && n < anz_images) {

	int h = images[n].h;
	int y = yp + images[n].y;
        int yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	if(h > 0) {
            const PIXVAL *sp = images[n].data;
	    PIXVAL *tp = textur + y*disp_width;

	    // oben clippen

	    while(yoff) {
		yoff --;
		do {
		    if(*(++sp)) {
			sp += *sp + 1;
		    }
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

		    if(runlen) {

			if(xpos >= clip_rect.x && runlen+xpos <= clip_rect.xx) {
                            pixcopy(tp+xpos, sp, runlen);
                        } else if(xpos < clip_rect.x) {
			    if(runlen+xpos >= clip_rect.x) {
                                pixcopy(tp+clip_rect.x, sp+(clip_rect.x-xpos), runlen-(clip_rect.x-xpos));
			    }
			} else if(clip_rect.xx >= xpos) {
			    pixcopy(tp+xpos, sp, clip_rect.xx-xpos+1);
			}
			sp += runlen;
			xpos += runlen;
			runlen = *sp ++;
		    }
		} while(runlen);

		tp += disp_width;

	    } while(--h);
	}
    }
}

/**
 * Zeichnet Bild ohne Clipping
 * @author Hj. Malthaner
 */
static void
display_img_nc(const int n, const int xp, const int yp)
{
    if(n >= 0 && n < anz_images) {

	signed char h = images[n].h;

	if(h > 0) {
            const PIXVAL *sp = images[n].data;
            PIXVAL *tp = textur + xp + (yp + images[n].y)*disp_width;

#ifdef USE_C

	    // bild darstellen

	    do { // zeilen dekodieren

		unsigned char runlen = *sp++;

		// eine Zeile dekodieren
		do {

		    // wir starten mit einem clear run
		    tp += runlen;

		    // jetzt kommen farbige pixel
		    runlen = *sp++;

		    if(runlen) {

			pixcopy(tp, sp, runlen);
			sp += runlen;
			tp += runlen;
			runlen = *sp++;
		    }

		} while(runlen);

		tp += (disp_width-tile_raster_width);

	    } while(--h);
#else
	    decode_img(tp, sp, h, disp_width);

#endif

	}
    }
}

/**
 * Zeichnet Bild
 * @author Hj. Malthaner
 */
void
display_img(const int n, const int xp, const int yp, const int dirty)
{
    if(dirty && n>=0 && n<anz_images) {
	mark_rect_dirty_wc(xp+images[n].x,
                           yp+images[n].y,
                           xp+images[n].x+images[n].w-1,
                           yp+images[n].y+images[n].h-1);
    }


    if(xp>=clip_rect.x &&
       yp>=clip_rect.y &&
       xp < clip_rect.xx-tile_raster_width &&
       yp < clip_rect.yy-tile_raster_width) {

	display_img_nc(n, xp, yp);

    } else if(xp>clip_rect.x-tile_raster_width &&
	      yp>clip_rect.y-tile_raster_width &&
	      xp < clip_rect.xx &&
	      yp < clip_rect.yy) {
	display_img_wc(n, xp, yp);
    }
}



/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static void
display_color_img_aux(const int n, const int xp, const int yp, const int color)
{
    if(n >= 0 && n < anz_images) {

	int h = images[n].h;
	int y = yp + images[n].y;
        int yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	if(h > 0) {
            const PIXVAL *sp = images[n].data;
	    PIXVAL *tp = textur + y*disp_width;

	    // oben clippen

	    while(yoff) {
		yoff --;
		do {
		    if(*(++sp)) {
			sp += *sp + 1;
		    }
		} while(*sp);
	        sp ++;
	    }

	    do { // zeilen dekodieren
                int xpos = xp;

		// bild darstellen

		do {
		    // wir starten mit einem clear run

		    xpos += *sp ++;

		    // jetzt kommen farbige pixel

		    if(*sp) {
			const int runlen = *sp++;

			if(xpos >= clip_rect.x && runlen+xpos <= clip_rect.xx) {
			    colorpixcpy(tp+xpos, sp, sp+runlen, (unsigned char)color);
                        } else if(xpos < clip_rect.x) {
			    if(runlen+xpos >= clip_rect.x) {
                                colorpixcpy(tp+clip_rect.x, sp+(clip_rect.x-xpos), sp+runlen, (unsigned char)color);
			    }
			} else if(clip_rect.xx >= xpos) {
			    colorpixcpy(tp+xpos, sp, sp+clip_rect.xx-xpos+1, (unsigned char)color);
			}

			sp += runlen;
			xpos += runlen;
		    }
		} while(*sp);

		tp += disp_width;
	        sp ++;

	    } while(--h);
	}
    }
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
void
display_color_img(const int n, const int xp, const int yp, const int color, const int dirty)
{
    if(dirty && n>=0 && n<anz_images) {
	mark_rect_dirty_wc(xp+images[n].x-8,
	                   yp+images[n].y-4,
	                   xp+images[n].x+images[n].w+8-1,
	                   yp+images[n].y+images[n].h+4-1);
    }


    // since the colors for player 0 are already right,
    // only use the expensive replacement routine for colored images
    // of other players

    if(color) {
	display_color_img_aux(n, xp, yp, color);
    } else {
	display_img_wc(n, xp, yp);
    }
}

/**
 * Zeichnet ein Pixel
 * @author Hj. Malthaner
 */
void
display_pixel(int x, int y, const int color)
{
    if(x >= clip_rect.x && x<=clip_rect.xx &&
       y >= clip_rect.y && y<=clip_rect.yy) {

        PIXVAL * const p = textur + x + y*disp_width;
	*p = color;

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
			textur[screen_pos+b] = fgpen;
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
display_text(int font, int x, int y, const char *txt, const int color, int dirty)
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
static void display_fb_internal(int xp, int yp, int w, int h,
				const int color, const int dirty,
				int cL, int cR, int cT, int cB)
{
    clip_lr(&xp, &w, cL, cR);
    clip_lr(&yp, &h, cT, cB);

    if(w > 0 && h > 0) {
        PIXVAL *p = textur + xp + yp*disp_width;

	if(dirty) {
	    mark_rect_dirty_nc(xp, yp, xp+w-1, yp+h-1);
	}

	do {
	    memset(p, color, w);
            p += disp_width;
	} while(--h);
    }
}

void
display_fillbox_wh(int xp, int yp, int w, int h,
		   const int color, const int dirty)
{
  display_fb_internal(xp,yp,w,h,color,dirty,
		      0,disp_width-1,0,disp_height-1);
}

void
display_fillbox_wh_clip(int xp, int yp, int w, int h,
			const int color, const int dirty)
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
        unsigned char *p = textur + xp + yp*disp_width;

        if (dirty) {
	  mark_rect_dirty_nc(xp, yp, xp, yp+h-1);
	}

	do {
	    *p = color;
            p += disp_width;
	} while(--h);
    }
}


void
display_vline_wh(const int xp, int yp, int h, const int color, const int dirty)
{
  display_vl_internal(xp,yp,h,color,dirty,
		      0,disp_width-1,0,disp_height-1);
}


void
display_vline_wh_clip(const int xp, int yp, int h, const int color, const int dirty)
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
	    memcpy(p, arr, w);
            p += disp_width;
	    arr += arr_w;
	} while(--h);
    }
}


// displays various in-window buttons
void display_button_picture(int x, int y, unsigned char *button)
{
    int w = 10, h = 10;
    int xorig = x, yorig = y;

    clip_wh(&x, &w, clip_rect.x, clip_rect.xx);
    clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

    if(w > 0 && h > 0) {
	PIXVAL *p = textur + x + y*disp_width;
	button += (y-yorig)*10 + (x-xorig);

	do {
	    memcpy(p, button, w);
	    p += disp_width;
	    button += 10;
	} while(--h);
    }
}

// --------------- compound painting procedures ---------------


// displays resize box down right. 16x16 px.
void display_resize_box(int x, int y, unsigned char *button)
{
    int w = 16, h = 16;
    int xorig = x, yorig = y;

    clip_wh(&x, &w, 0, disp_width);
    clip_wh(&y, &h, 0, disp_height);

    if(w > 0 && h > 0) {
	PIXVAL *p = textur + x + y*disp_width;
	button += (y-yorig)*16 + (x-xorig);

	do {
	    memcpy(p, button, w);
	    p += disp_width;
	    button += 16;
	} while(--h);
    }
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

  display_vline_wh(xpos-2, ypos-6-hgt, 10, ddd_farbe+1, dirty);
  display_vline_wh(xpos+width-3, ypos-6-hgt, 10, ddd_farbe-1, dirty);

  display_proportional(xpos+2, ypos-9-hgt, text, ALIGN_LEFT, text_farbe,FALSE);
}

// clip Left/Right/Top/Bottom
void display_p_internal(int x, int y, const char *txt,
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
  const PIXVAL color = color_index;

  text_width = proportional_string_width(txt);

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
  while ((c = *((unsigned char *)txt)++)) {
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

  display_p_internal(x,y,txt,align,color,dirty,0,disp_width-1,0,disp_height-1);
}


void display_proportional_clip(int x, int y, const char *txt,
			       int align, const int color,
			       const int dirty)
{
    display_p_internal(x,y,txt,align,color,dirty,
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
 */
void
display_multiline_text(int x, int y, const char *inbuf, int color)
{
    char tmp[4096];
    char *buf = tmp;
    char *next;
    int y_off = 0;

    // be sure not to copy more than buffer size
    strncpy(buf, inbuf, 4095);

    // always close with a 0 byte
    buf[4095] = 0;

    while( (*buf != 0) && (next = strchr(buf,'\n')) ) {
	*next = 0;
//	display_text(x,y+y_off, buf, color, TRUE);
        display_proportional_clip(x,y+y_off, buf, ALIGN_LEFT, color, TRUE);
	buf = next+1;
	y_off += LINESPACE;
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
	    display_img(softpointer, sys_event.mx, sys_event.my, TRUE);
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
    memset(tile_dirty, 0, tile_lines*tiles_per_line);
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
 * unbenutzt ?
 * @author Hj. Malthaner
 */
void
my_save_exit()
{
    dr_os_close();
}


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int
simgraph_init(int width, int height,
              const char * a_configdir,
              const char * pakfilename,
	      int use_shm, int do_sync)
{
    int parameter[2];
    int ok;

    configdir = a_configdir;

    parameter[0]=use_shm;
    parameter[1]=do_sync;

    dr_os_init(2, parameter);

    ok = dr_os_open(width, height);

    if(ok) {

	disp_width = width;
	disp_height = height;

        textur = dr_textur_init();
	use_softpointer = dr_use_softpointer();

	init_font(".drawrc");
	load_4x7_font();
	display_set_color(1);
        init_images(pakfilename);

        printf("Init. done.\n");

    } else {
	puts("Error  : can't open window!");
        exit(-1);
    }


    // allocate dirty tile flags
    tiles_per_line = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
    tile_lines = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;

    tile_dirty = guarded_malloc( tile_lines*tiles_per_line );
    tile_dirty_old = guarded_malloc( tile_lines*tiles_per_line );

    memset(tile_dirty, 1, tile_lines*tiles_per_line);
    memset(tile_dirty_old, 1, tile_lines*tiles_per_line);

    display_setze_clip_wh(0, 0, disp_width, disp_height);

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

    do {
	sprintf(buf,"simscr%02d.pcx", number++);
    } while(access(buf, 6) != -1);

    write_pcx(buf, textur, disp_width, disp_height, base_pal);
}


/**
 * Laedt Einstellungen
 * @author Hj. Malthaner
 */
void display_laden(FILE* file)
{
    int i,r,g,b;

    unsigned char day[12];
    unsigned char night[12];

    fscanf(file, "%d %d %d\n", &light_level, &color_level, &night_shift);

    display_set_light(light_level);
    display_set_color(color_level);

    for(i=0; i<4; i++) {
	fscanf(file, "%d %d %d\n", &r, &g, &b);
	day[i*3+0] = r;
	day[i*3+1] = g;
	day[i*3+2] = b;

	fscanf(file, "%d %d %d\n", &r, &g, &b);
	night[i*3+0] = r;
	night[i*3+1] = g;
	night[i*3+2] = b;
    }

    display_set_player_colors(day, night);
}


/**
 * Speichert Einstellungen
 * @author Hj. Malthaner
 */
void display_speichern(FILE* file)
{
    int i;
    fprintf(file, "%d %d %d\n", light_level, color_level, night_shift);

    for(i=0; i<4; i++) {
	fprintf(file, "%d %d %d\n", day_pal[i*3+0], day_pal[i*3+1], day_pal[i*3+2]);
	fprintf(file, "%d %d %d\n", night_pal[i*3+0], night_pal[i*3+1], night_pal[i*3+2]);
    }
}
