#include <string.h>
#include <stdlib.h>
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

struct dimension
{
	int xmin;
	int xmax;
	int ymin;
	int ymax;
};

// number of special colors

#define SPECIAL 31


/*
 * Definition of special colors
 * @author Hj. Malthaner
 */
static const PIXRGB rgbtab[SPECIAL] = {
	0x244B67, // Player color 1
	0x395E7C,
	0x4C7191,
	0x6084A7,
	0x7497BD,
	0x88ABD3,
	0x9CBEE9,
	0xB0D2FF,

	0x7B5803, // Player color 2
	0x8E6F04,
	0xA18605,
	0xB49D07,
	0xC6B408,
	0xD9CB0A,
	0xECE20B,
	0xFFF90D,

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
	0xFF017F, // purple light
	0x0101FF, // blue light
};


static int special_hist[SPECIAL];


cstring_t image_writer_t::last_img_file("");

unsigned image_writer_t::width;
unsigned image_writer_t::height;
unsigned char* image_writer_t::block = NULL;
int image_writer_t::img_size = 64;


void image_writer_t::dump_special_histogramm()
{
	for(int i = 0; i < SPECIAL; i++) {
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
	PIXVAL pix;

	for (int i = 0; i < SPECIAL; i++) {
		if (rgbtab[i] == (PIXRGB)rgb) {
			pix = 0x8000 + i;
			return endian_uint16(&pix);
		}
	}

	const int r = (rgb >> 16);
	const int g = (rgb >>  8) & 0xFF;
	const int b = (rgb >>  0) & 0xFF;

	// RGB 555
	pix = ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3);
	return endian_uint16(&pix);
}



static void init_dim(PIXRGB* image, dimension* dim, int img_size)
{
	int x,y;
	bool found = false;

	dim->ymin = dim->xmin = img_size;
	dim->ymax = dim->xmax = 0;

	for (y = 0; y < img_size; y++) {
		for (x = 0; x < img_size; x++) {
			if (image[x + y * img_size] != TRANSPARENT) {
				if (x < dim->xmin) dim->xmin = x;
				if (y < dim->ymin) dim->ymin = y;
				if (x > dim->xmax) dim->xmax = x;
				if (y > dim->ymax) dim->ymax = y;
				found = true;
			}
		}
	}
	if (!found) {
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
PIXVAL* image_writer_t::encode_image(int x, int y, dimension* dim, int* len)
{
	int line;
	PIXVAL* dest;
	PIXVAL* dest_base = new PIXVAL[img_size * img_size * 2];
	PIXVAL* colored_run_counter;

	dest = dest_base;

	x += dim->xmin;
	y += dim->ymin;
	const int width = dim->xmax - dim->xmin + 1;
	const int height = dim->ymax - dim->ymin + 1;

	for (line = 0; line < height; line++) {
		int row_px_count = 0;
		PIXRGB pix = block_getpix(x, y + line);
		uint16 count = 0;
		uint16 clear_colored_run_pair_count = 0;

		do {
			count = 0;
			while (pix == TRANSPARENT && row_px_count < width) {
				count++;
				row_px_count++;
				pix = block_getpix(x + row_px_count, y + line);
			}

			*dest++ = endian_uint16(&count);

			colored_run_counter = dest++;
			count = 0;

			while (pix != TRANSPARENT && row_px_count < width) {
				*dest++ = pixrgb_to_pixval(pix);
				count++;
				row_px_count++;
				pix = block_getpix(x + row_px_count, y + line);
			}

			/* Knightly:
			 *		If it is not the first clear-colored-run pair and its colored run is empty
			 *		--> it is superfluous and can be removed by rolling back the pointer
			 */
			if(  clear_colored_run_pair_count>0  &&  count==0  ) {
				dest -= 2;
				// this only happens at the end of a line, so no need to increment clear_colored_run_pair_count
			}
			else {
				*colored_run_counter = endian_uint16(&count);
				clear_colored_run_pair_count++;
			}
		} while (row_px_count < width);

		*dest++ = 0;
	}

	*len = dest - dest_base;

	return dest_base;
}



bool image_writer_t::block_laden(const char* fname)
{
	// The last png-file is cached
	if (last_img_file == fname || load_block(&block, &width, &height, fname, img_size)) {
		last_img_file = fname;
		return true;
	} else {
		last_img_file = "";
		return false;
	}
}



/* the syntax for image the string is
 *   "-" empty image
 * [> ]imagefilename_without_extension[[[[.row].col],xoffset],yoffset]
 *  leading "> " maen an unzoomable image
 *  after the dots also spaces are allowed
 */
void image_writer_t::write_obj(FILE* outfp, obj_node_t& parent, cstring_t an_imagekey)
{
	bild_t bild;
	dimension dim;
	PIXVAL* pixdata = NULL;
	cstring_t imagekey;

	memset(&bild, 0, sizeof(bild));

	// Hajo: if first char is a '>' then this image is not zoomeable
	if (an_imagekey.len() > 2 && an_imagekey[0] == '>') {
		imagekey = an_imagekey.substr(2, an_imagekey.len());
		bild.zoomable = false;
	} else {
		imagekey = an_imagekey;
		bild.zoomable = true;
	}

	if (imagekey != "-" && imagekey != "") {
		// divide key in filename and image number
		int row = -1, col = -1;
		cstring_t numkey;

		int j = imagekey.find_back('/');
		if (j == -1) {
			numkey = imagekey;
		} else {
			numkey = imagekey.substr(j + 1, imagekey.len());
		}

		int i = numkey.find('.');
		if (i == -1) {
			char reason[1024];
			sprintf(reason, "no image number in %s", (const char*)imagekey );
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		numkey = numkey.substr(i + 1, numkey.len());

		imagekey = root_writer_t::get_inpath() + imagekey.substr(0, imagekey.len() - numkey.len() - 1) +  ".png";


		i = numkey.find('.');
		if (i == -1) {
			row = atoi(numkey);
		} else {
			row = atoi(numkey.substr(0, i));
			col = atoi(numkey.substr(i + 1, numkey.len()));

			// add image offsets
			numkey = numkey.substr(i + 1, numkey.len());
			i = numkey.find(',');
			if (i != -1) {
				bild.x = atoi(numkey.substr(i+1, numkey.len()));
				numkey = numkey.substr(i + 1, numkey.len());
				i = numkey.find(',');
				if(i!=-1) {
					bild.y = atoi(numkey.substr(i + 1, numkey.len()));
				}
			}
		}

		// Load complete file
		if (!block_laden(imagekey)) {
			char reason[1024];
			sprintf(reason, "cannot open %s", (const char*)imagekey );
			throw obj_pak_exception_t("image_writer_t", reason);
		}

		if (col == -1) {
			col = row % (width / img_size);
			row = row / (width / img_size);
		}
		if (col >= (int)(width / img_size) || row >= (int)(height / img_size)) {
			char reason[1024];
			sprintf(reason, "invalid image number in %s.%s", (const char*)imagekey, (const char*)numkey);
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		row *= img_size;
		col *= img_size;

		// Temp. read image and determine drawing area.
		PIXRGB* image = new PIXRGB[img_size * img_size];
		for (int x = 0; x < img_size; x++) {
			for (int y = 0; y < img_size; y++) {
				image[x + y * img_size] = block_getpix(x + col, y + row);
			}
		}
		init_dim(image, &dim, img_size);
		delete image;

		bild.x += dim.xmin;
		bild.y += dim.ymin;
		bild.w = dim.xmax - dim.xmin + 1;
		bild.h = dim.ymax - dim.ymin + 1;
		bild.len = 0;

		if (bild.h > 0) {
			int len;
			pixdata = encode_image(col, row, &dim, &len);
			if (len>65535) {
				printf("ERROR: packed image size (%i) exceeded 65535 bytes!\n",len);
				abort();
			}
			bild.len = len;
		}
	}

#ifdef IMG_VERSION0
	// version 0
	obj_node_t node(this, 12 + (bild.len * sizeof(uint16)), &parent);

	// to avoid any problems due to structure changes, we write manually the data
	node.write_uint8 (outfp, bild.x,         0);
	node.write_uint8 (outfp, bild.w,         1);
	node.write_uint8 (outfp, bild.y,         2);
	node.write_uint8 (outfp, bild.h,         3);
	node.write_uint32(outfp, bild.len,       4);
	node.write_uint16(outfp, 0,              8);
	node.write_uint8 (outfp, bild.zoomable, 10);
	node.write_uint8 (outfp, 0,             11);

	if (bild.len) {
		// only called, if there is something to store
		node.write_data_at(outfp, pixdata, 12, bild.len * sizeof(PIXVAL));
		free(pixdata);
	}
#elif IMG_VERSION2
	// version 1 or 2
	obj_node_t node(this, 10 + (bild.len * sizeof(uint16)), &parent);

	// to avoid any problems due to structure changes, we write manually the data
	node.write_uint16(outfp, bild.x,        0);
	node.write_uint16(outfp, bild.y,        2);
	node.write_uint8 (outfp, bild.w,        4);
	node.write_uint8 (outfp, bild.h,        5);
	node.write_uint8 (outfp, 2,             6); // version
	node.write_uint16(outfp, bild.len,      7);
	node.write_uint8 (outfp, bild.zoomable, 9);

	if (bild.len) {
		// only called, if there is something to store
		node.write_data_at(outfp, pixdata, 10, bild.len * sizeof(PIXVAL));
		free(pixdata);
	}
#else
	// version 3
	obj_node_t node(this, 10 + (bild.len * sizeof(uint16)), &parent);

	// to avoid any problems due to structure changes, we write manually the data
	node.write_uint16(outfp, bild.x,        0);
	node.write_uint16(outfp, bild.y,        2);
	node.write_uint16 (outfp, bild.w,        4);
	node.write_uint8 (outfp, 3,             6); // version, always at position 6!
	node.write_uint16 (outfp, bild.h,        7);
	// len is now automatically calculated
	node.write_uint8 (outfp, bild.zoomable, 9);

	if (bild.len) {
		// only called, if there is something to store
		node.write_data_at(outfp, pixdata, 10, bild.len * sizeof(PIXVAL));
		free(pixdata);
	}
#endif

	node.write(outfp);
}
