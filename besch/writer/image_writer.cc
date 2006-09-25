#include <stdlib.h>
#include <string.h>

#include "../../utils/cstring_t.h"
#include "../../utils/dr_rdpng.h"

#include "../bild_besch.h"
#include "obj_node.h"
#include "root_writer.h"
#include "obj_pak_exception.h"

#include "image_writer.h"

//#define TRANSPARENT 0x808088
#define TRANSPARENT 0xE7FFFF

// Made IMG_SIZE changeable - set in static member image_writer_t::img_size
//#define IMG_SIZE 64

struct dimension  {
    int xmin,xmax,ymin,ymax;
};

// number of special colors

// #define SPECIAL 4
// #define SPECIAL 17
#define SPECIAL 29


/*
 * Definition of special colors
 * @author Hj. Malthaner
 */
static const PIXRGB rgbtab[SPECIAL] =
{
  0x395E7C, // Player colors
  0x4C7191,
  0x6084A7,
  0x7497BD,
  0x88ABD3,
  0x9CBEE9,
  0xB0D2FF,
  0xFF00FF, // Dummy entry

  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry

  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry
  0xFF00FF, // Dummy entry

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


static int special_hist[SPECIAL];



cstring_t image_writer_t::last_img_file("");

unsigned image_writer_t::width;
unsigned image_writer_t::height;
unsigned char *image_writer_t::block = NULL;
int image_writer_t::img_size = 64;


void image_writer_t::dump_special_histogramm()
{
  for(int i=0; i<SPECIAL; i++) {
    printf("%2d) 0x%06x : %d\n", i, rgbtab[i], special_hist[i]);
  }
}



/**
 * Encodes image data into the internal representation,
 * considers special colors.
 *
 * @author Hj. Malthaner
 */
static PIXVAL pixrgb_to_pixval(int rgb)
{
  PIXVAL result = 0;
  int standard = 1;
  int i;

  /*
  for(i=0; i<SPECIAL_HIST; i++) {
    if(special2[i] == (PIXRGB)rgb) {
      special2_hist[i] ++;
    }
  }
  */


  for(i=0; i<SPECIAL; i++) {
    if(rgbtab[i] == (PIXRGB)rgb) {
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


static void init_dim(PIXRGB *image, dimension *dim, int img_size)
{
    int x,y;
    bool found = false;

    dim->ymin = dim->xmin = img_size;
    dim->ymax = dim->xmax = 0;

    for(y=0; y<img_size; y++) {
	for(x=0; x<img_size; x++) {
	    if(image[x+y*img_size] != TRANSPARENT) {
		if(x<dim->xmin)
		    dim->xmin = x;
		if(y<dim->ymin)
		    dim->ymin = y;
		if(x>dim->xmax)
		    dim->xmax = x;
		if(y>dim->ymax)
		    dim->ymax = y;
		found = true;
	    }
	}
    }
    if(!found) {
	// Negativ values not usable
	dim->xmin = 1;
	dim->ymin = 1;
    }
}


/**
 * Encodes an image into a sprite data structure, considers
 * special colors.
 *
 * @author Hj. Malthaner
 */
PIXVAL* image_writer_t::encode_image(int x, int y, dimension *dim, int *len)
{
    int line;
    PIXVAL *dest;
    PIXVAL *dest_base = new PIXVAL[img_size * img_size * 2];
    PIXVAL *run_counter;

    y += dim->ymin;
    dest = dest_base;

    for(line=0; line<(dim->ymax-dim->ymin+1); line++) {
	int   row = 0;
	PIXRGB pix = block_getpix(x, (y+line));
	unsigned char count = 0;

	do {
	    count = 0;
	    while(pix == TRANSPARENT && row < img_size) {
		count ++;
		row ++;
		pix = block_getpix((x+row), (y+line));
	    }

	    *dest++ = count;

	    run_counter = dest++;
	    count = 0;

	    while(pix != TRANSPARENT && row < img_size) {
		*dest++ = pixrgb_to_pixval(pix);

		count ++;
		row ++;
		pix = block_getpix((x+row), (y+line));
	    }
	    *run_counter = count;
	} while(row < img_size);

	*dest++ = 0;
    }

    *len = (dest - dest_base);

    return dest_base;
}


bool image_writer_t::block_laden(const char *fname)
{
	// The last png-file is cached
    if(last_img_file == fname || load_block(&block, &width, &height, fname,img_size)) {
	last_img_file = fname;
	return true;
    } else {
	last_img_file = "";
	return false;
    }
}


void image_writer_t::write_obj(FILE *outfp, obj_node_t &parent, cstring_t an_imagekey)
{
    bild_besch_t bild;
    dimension dim;
    PIXVAL *pixdata = NULL;;
    cstring_t imagekey;

    memset(&bild, 0, sizeof(bild)) ;

    // Hajo: if first char is a '>' then this image is not zoomeable
    if(an_imagekey.len() > 2
       && an_imagekey.chars()[0] == '>') {

      imagekey = an_imagekey.substr(2, an_imagekey.len());
      bild.zoomable = false;

      // printf("'%s'\n", imagekey.chars());
    } else {

      imagekey = an_imagekey;
      bild.zoomable = true;
    }


    if(imagekey != "-" && imagekey != "") {
	//
	// divide key in filename and image number
	//
	int row = -1, col = -1;
	cstring_t numkey;
	cstring_t part;

	int j = imagekey.find_back('/');
	if(j == -1)
	    numkey = imagekey;
	else
	    numkey = imagekey.substr(j + 1, imagekey.len());

	int i = numkey.find('.');
	if(i == -1) {
	    cstring_t reason;

	    reason.printf("no image number in %s", imagekey.chars());
	    throw new obj_pak_exception_t("image_writer_t", reason);
	}
	numkey = numkey.substr(i + 1, numkey.len());

	imagekey = root_writer_t::get_inpath() + imagekey.substr(0, imagekey.len() - numkey.len() - 1) +  ".png";

	i = numkey.find('.');
	if(i == -1) {
	    row = atoi(numkey.chars());
	}
	else {
	    part = numkey.substr(0, i);
	    row = atoi(part.chars());
	    part = numkey.substr(i + 1, numkey.len());
	    col = atoi(part.chars());
	}
	//
	// Load complete file
	//
	if(!block_laden(imagekey.chars())) {
	    cstring_t reason;

	    reason.printf("cannot open %s", imagekey.chars());
	    throw new obj_pak_exception_t("image_writer_t", reason);
	}

	if(col == -1) {
	    col = row % (width / img_size);
	    row = row / (width / img_size);
	}
	if(col >= (int)(width / img_size) || row >= (int)(height / img_size)) {
	    cstring_t reason;

	    reason.printf("invalid image number in %s.%s", imagekey.chars(), numkey.chars());
	    throw new obj_pak_exception_t("image_writer_t", reason);
	}
	row *= img_size;
	col *= img_size;

	//
	// Temp. read image and determine drawing area.
	//
	PIXRGB *image = new PIXRGB[img_size*img_size];

    	for(int x = 0; x < img_size; x++) {
	    for(int y = 0; y < img_size; y++) {
		image[x + y * img_size] = block_getpix(x + col, y + row);
	    }
	}
	init_dim(image, &dim, img_size);

	delete image;


	bild.x = dim.xmin;
	bild.y = dim.ymin;
	bild.w = dim.xmax - dim.xmin + 1;
	bild.h = dim.ymax - dim.ymin + 1;
	bild.len = 0;

	if(bild.h > 0) {
	    int len;
	    pixdata = encode_image(col, row, &dim, &len);
	    if(len>65535) {
	    	printf("ERROR: packed image size (%i) exceeded 65535 bytes!\n",len);
	    	abort();
	    }
	    bild.len = len;
	}

	/*
	printf("x=%02d y=%02d w=%02d h=%02d len=%d\n",
	       bild.x,
	       bild.y,
	       bild.w,
	       bild.h,
	       bild.len);
	*/

    }
    obj_node_t node(this, sizeof(bild) + bild.len * sizeof(PIXVAL), &parent, true);

    node.write_data_at(outfp, &bild, 0, sizeof(bild));
    if(pixdata) {
	node.write_data_at(outfp, pixdata, sizeof(bild), bild.len * sizeof(PIXVAL));
	free(pixdata);
    }
    node.write(outfp);
}
