/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

/*
 * betriebssystemspez. routinen fuer Simutrans
 * aus dr_sys_x.c abgeleitet
 *
 * April 2000
 *
 * Hj. Malthaner
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <sys/shm.h>
#include <sys/time.h>
#include <X11/extensions/XShm.h>

#include "simsys.h"

// Variablen zur Messageverarbeitung

struct sys_event sys_event;

// interne vars.

static int using_shm = FALSE;
static int doing_sync = TRUE;

static int display_depth = 8;
static int is_truecolor = FALSE;

static Cursor standard_cursor;
static Cursor invisible_cursor;

static unsigned long planetab[32];
static unsigned long coltab[256];
static unsigned long colortrans[256];

static Display *md;              /* global fuer die Zeichenroutinen */
static Window  mw;
static GC      mgc;
static int     ms;
static unsigned long mbg,mfg;    /* Vorder/Hintergrund */
static XSizeHints mh;
static Colormap cmap;

static int width = 640;
static int height = 480;


// Time at program start
static struct timeval start;


// Hilfsprozeduren


static void
init_cursors()
{
    static char bits[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    Pixmap pix;
    XColor cfg;

    // create invisible cursor

    cfg.red = cfg.green = cfg.blue = 0;
    pix = XCreateBitmapFromData(md, mw, bits, 8, 8);
    invisible_cursor = XCreatePixmapCursor(md, pix, pix, &cfg, &cfg, 0, 0);
    XFreePixmap(md, pix);

    // create standard cursor

    standard_cursor = XCreateFontCursor(md,130);
}

/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */

int dr_os_init(int n, int *parameter)
{
    using_shm = parameter[0];
    doing_sync = parameter[1];

    // init time count
    gettimeofday(&start, NULL);

    return TRUE;
}

int dr_os_open(int w, int h)
{
    XSetWindowAttributes attr;


    width = w;
    height = h;

    md=XOpenDisplay(NULL);

    if(md==NULL) {
	fprintf(stderr, "Kann Display nicht oeffnen !");
	exit(1);
    }

    ms=DefaultScreen(md);


    display_depth = DefaultDepth(md, ms);

    fprintf(stderr,"Using %d bpp\n", display_depth);


    if(display_depth == 8) {
	is_truecolor = FALSE;

    } else if(display_depth >= 15) {
	is_truecolor = TRUE;

	fprintf(stderr,"Warning: using experimental HiColor/TrueColor mode\n");
    } else {
	fprintf(stderr,"Problem: Simutrans braucht ein 8 Bit pseudo color visual (256 Farben mit Palette)\n");
	fprintf(stderr,"         oder ein direct color/true color visual (HiColor)\n\n");
        exit(2);
    }

    mbg = WhitePixel(md,ms);
    mfg = BlackPixel(md,ms);

    mh.x = 0;                               /* Fenster sollte im sichtbaren */
    mh.y = 0;                                /* bereich liegen */
    mh.width = width;
    mh.height = height;

    // set window size hints
    // min_size = size = max_size  -> no resizing

    mh.width = width;
    mh.height = height;

    mh.min_width = mh.max_width = width;
    mh.min_height = mh.max_height = height;

    mh.flags=PSize|PMinSize|PMaxSize;

    fprintf(stderr,"Opening program window ...\n");

    mw=XCreateSimpleWindow(md,DefaultRootWindow(md),
                           mh.x,mh.y,mh.width,mh.height, 5,
			   mfg,mbg);

    XSetStandardProperties(md,mw,"Simutrans","Simu",None,NULL,0,NULL);

    mgc=XCreateGC(md,mw,0,0);
    XSetBackground(md,mgc,mbg);
    XSetForeground(md,mgc,mfg);

    XSelectInput(md,mw,  VisibilityChangeMask|
			 ButtonPressMask|ButtonReleaseMask|
			 KeyPressMask|KeyReleaseMask|
			 PointerMotionMask);

    attr.backing_store=Always;
    XChangeWindowAttributes(md,mw,CWBackingStore,&attr);

    if(!is_truecolor) {
	cmap=XCreateColormap(md,mw,DefaultVisual(md,ms),AllocNone);
	XAllocColorCells(md,cmap,0,planetab,0,coltab,256);
	XSetWindowColormap(md,mw,cmap);
    }

    XMapRaised(md,mw);

    init_cursors();

    XDefineCursor(md, mw, standard_cursor);

    sys_event.mx = width/2;
    sys_event.my = height/2;


    XSetForeground(md,mgc,1);

    return TRUE;
}

int dr_os_close()
{
    XCloseDisplay(md);                    /* raeumt alles auf */
    return TRUE;
}

/*
 * Hier beginnen die eigentlichen graphischen Funktionen
 */

static XImage *texturimg;
static XShmSegmentInfo xshminfo;

// this is used if we need to fake an 8 bit array
static unsigned char * data8;

unsigned char *
dr_textur_init()
{
    if(!using_shm) {
        int depth = display_depth;
	if(depth == 24) {
	    depth = 32;
        }

	if(depth == 15) {
	    depth = 16;
	}

	texturimg=XCreateImage(md, DefaultVisual(md, ms),
                               display_depth, ZPixmap, 0,
                               malloc(width*height*(depth/8)),
			       width, height, depth, width*(depth/8));

	printf("Texturimage : %p\n, ",texturimg);

	if(is_truecolor) {
	    // fake an 8 bit array and convert data before displaying
	    return (data8 = malloc(width*height));
	} else {
	    return texturimg->data;
	}

    } else {
	int depth = display_depth;

	if(depth == 24) {
	    depth = 32;
        }

	xshminfo.shmid = shmget(IPC_PRIVATE,
			        width * height*(depth/8),
				IPC_CREAT | 0777);

	texturimg = XShmCreateImage(md, DefaultVisual(md, ms),
                             display_depth, ZPixmap, NULL, &xshminfo,
			     width, height);

	printf("Texturimage : %p, ",texturimg);
	printf("shmid : %d\n",xshminfo.shmid);

	texturimg->data = (char *) shmat(xshminfo.shmid, 0, 0);
	printf("Data : %p\n",texturimg->data);

	xshminfo.shmaddr = texturimg->data;
	xshminfo.readOnly = False;

	printf("Attach : %d\n",XShmAttach(md, &xshminfo));

	if(is_truecolor) {
	    // fake an 8 bit array and convert data before displaying
	    return (data8 = malloc(width*height));
	} else {
	    return texturimg->data;
	}
    }
}


void
dr_textur(int xp, int yp, int w, int h)
{
    // clipping unten
    if(yp+h > height) {
	h = height-yp;
    }


    // if we are in truecolor we need to convert our image data

    if(is_truecolor) {
	const unsigned char * src = data8 + xp + yp*width;
	const int skip = width-w;

	if(display_depth <=16) {

	    unsigned short * dest =
                ((unsigned short *) (texturimg->data)) + xp + yp*width;
            int hc = h;
	    do {
		const unsigned char * const end = src + w;
		do {
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		} while(src < end);

		dest += skip;
	        src += skip;

            } while(--hc > 0);

	} else {
	    unsigned int * dest = (unsigned int *)texturimg->data + xp + yp*width;
            int hc = h;
	    do {
		const unsigned char * const end = src + w;
		do {
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		    *dest++ = colortrans[*src++];
		} while(src < end);

		dest += skip;
	        src += skip;

            } while(--hc > 0);
	}
    }

    if(using_shm) {
	// printf("%d %d %d %d\n", xp, yp, w, h);

	XShmPutImage(md,mw,mgc,texturimg, xp, yp, xp, yp,
                     w, h, 0);
    } else {
	XPutImage(md,mw,mgc,texturimg, xp, yp, xp, yp,
                  w, h);
    }
}

void dr_flush()
{
    if(doing_sync) {
	XSync(md,FALSE);
    }
}


void dr_setRGB8(int n, int r, int g, int b)
{
    XColor xc;

    xc.flags=DoRed|DoGreen|DoBlue;
    xc.pixel=n;
    xc.red=r<<8;
    xc.green=g<<8;
    xc.blue=b<<8;

    if(!is_truecolor) {
	XStoreColor(md,cmap,&xc);
	XFlush(md);
    }

    if(is_truecolor) {
	XAllocColor(md,
                    DefaultColormap(md, ms),
		    &xc);

	// printf("Storing color %d with value %lx\n", n, xc.pixel);
        colortrans[n] = xc.pixel;

    }
}

void dr_setRGB8multi(int first, int count, unsigned char * data)
{
    XColor xc;
    int n;

    for(n=0; n<count; n++) {
	xc.pixel = n+first;
	xc.flags = DoRed|DoGreen|DoBlue;
	xc.red = data[n*3+0]<<8;
	xc.green = data[n*3+1]<<8;
	xc.blue = data[n*3+2]<<8;


	if(!is_truecolor) {
	    XStoreColor(md,cmap,&xc);
	} else {
	    XAllocColor(md,
                        DefaultColormap(md, ms),
		        &xc);

	    // printf("Storing color %d with value %lx\n", n, xc.pixel);
	    colortrans[n] = xc.pixel;
	}
    }
    XFlush(md);
}


void show_pointer(int yesno)
{
    if(yesno) {
	XDefineCursor(md, mw, standard_cursor);
    } else {
	XDefineCursor(md, mw, invisible_cursor);
    }
}


void move_pointer(int x, int y)
{
    XWarpPointer(md, None, mw, 0, 0, width, height, x, y);
}

/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */

static void
internal_GetEvents(int wait)
{
    XEvent event;
    char   text[4];
    KeySym mykey;

    if(wait) {
	do {
	    XNextEvent(md,&event);
	}while(event.type==GraphicsExpose ||
	       event.type==NoExpose ||
	       (XEventsQueued(md,QueuedAlready) > 0 && event.type==MotionNotify)
	       );
    } else {
	do {
	    XNextEvent(md,&event);
	}while(event.type==GraphicsExpose ||
	       event.type==NoExpose ||
	       (XEventsQueued(md,QueuedAfterFlush) > 0 && event.type==MotionNotify)
	       );
    }

    switch(event.type) {
    case ButtonPress:
	sys_event.type=SIM_MOUSE_BUTTONS;
	sys_event.mx=event.xbutton.x;
	sys_event.my=event.xbutton.y;
	switch(event.xbutton.button) {
	case 1:
	    sys_event.code=SIM_MOUSE_LEFTBUTTON;
	    break;
	case 2:
	    sys_event.code=SIM_MOUSE_MIDBUTTON;
	    break;
	case 3:
	    sys_event.code=SIM_MOUSE_RIGHTBUTTON;
	    break;
	}
	break;
    case ButtonRelease:
	sys_event.type=SIM_MOUSE_BUTTONS;
	sys_event.mx=event.xbutton.x;
	sys_event.my=event.xbutton.y;
	switch(event.xbutton.button) {
	case 1:
	    sys_event.code=SIM_MOUSE_LEFTUP;
	    break;
	case 2:
	    sys_event.code=SIM_MOUSE_MIDUP;
	    break;
	case 3:
	    sys_event.code=SIM_MOUSE_RIGHTUP;
	    break;
	}
	break;
    case KeyPress:
	sys_event.type=SIM_KEYBOARD;

	if(XLookupString(&event.xkey,text,1,&mykey,0)) {
	    sys_event.code=text[0];
	} else {
	    sys_event.code=0;
	}

	break;
    case MotionNotify:
	sys_event.type=SIM_MOUSE_MOVE;
	sys_event.code= SIM_MOUSE_MOVED;
	sys_event.mx=event.xmotion.x;
	sys_event.my=event.xmotion.y;
	break;
    case 15:
	// exposure oder so
	dr_textur(0, 0, width, height);
	sys_event.type=SIM_IGNORE_EVENT;
	sys_event.code=0;
	break;
    case 14:
	printf("Unbekanntes Ereignis # %d!\n",event.type);
    case KeyRelease:
	sys_event.type=SIM_IGNORE_EVENT;
	sys_event.code=0;
	break;
    default:
	printf("Unbekanntes Ereignis # %d!\n",event.type);
	sys_event.type=SIM_IGNORE_EVENT;
	sys_event.code=0;
    }
}


void GetEvents()
{
    internal_GetEvents(TRUE);
}

void GetEventsNoWait()
{
    int n=XEventsQueued(md,QueuedAfterFlush);

    sys_event.type = SIM_NOEVENT;
    sys_event.code = 0;

    if(n > 0) {
	internal_GetEvents(FALSE);
    }
}

void ex_ord_update_mx_my()
{
    // wird fuer X-Windows nicht benoetigt
}


unsigned long dr_time(void)
{
    struct timeval now;
    unsigned long time;

    gettimeofday(&now, NULL);
    time=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
    return time;
}


void dr_sleep(unsigned long usec)
{
    if(usec > 1000) {
	usleep( usec );
    }
}


int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return simu_main(argc, argv);
}
