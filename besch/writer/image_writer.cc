#include <string>
#include <string.h>
#include <stdlib.h>
#include "../../utils/dr_rdpng.h"
#include "../bild_besch.h"
#include "obj_node.h"
#include "root_writer.h"
#include "obj_pak_exception.h"
#include "image_writer.h"
#include "../../macros.h"

#include <stdio.h>

using std::string;

struct dimension
{
	int xmin;
	int xmax;
	int ymin;
	int ymax;
};

static int special_hist[SPECIAL];


string image_writer_t::last_img_file;

unsigned image_writer_t::width;
unsigned image_writer_t::height;
unsigned char* image_writer_t::block = NULL;
int image_writer_t::img_size = 64;



void image_writer_t::dump_special_histogramm()
{
	for(int i = 0; i < SPECIAL; i++) {
		printf("%2d) 0x%06x : %d\n", i, bild_besch_t::rgbtab[i], special_hist[i]);
	}
}


/**
 * Encodes image data into the internal representation,
 * considers special colors.
 *
 * @author Hj. Malthaner
 */
static uint16 pixrgb_to_pixval(uint32 rgb)
{
	uint16 pix;

	for (int i = 0; i < SPECIAL; i++) {
		if (bild_besch_t::rgbtab[i] == (uint32)rgb) {
			pix = 0x8000 + i;
			return endian(pix);
		}
	}

	const int r = (rgb >> 16);
	const int g = (rgb >>  8) & 0xFF;
	const int b = (rgb >>  0) & 0xFF;

	// RGB 555
	pix = ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3);
	return endian(pix);
}



static void init_dim(uint32 *image, dimension *dim, int img_size)
{
	int x,y;
	bool found = false;

	dim->ymin = dim->xmin = img_size;
	dim->ymax = dim->xmax = 0;

	for (y = 0; y < img_size; y++) {
		for (x = 0; x < img_size; x++) {
			if (image[x + y * img_size] != SPECIAL_TRANSPARENT) {
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
uint16 *image_writer_t::encode_image(int x, int y, dimension* dim, int* len)
{
	int line;
	uint16 *dest;
	uint16 *dest_base = new uint16[img_size * img_size * 2];
	uint16 *colored_run_counter;

	dest = dest_base;

	x += dim->xmin;
	y += dim->ymin;
	const int width = dim->xmax - dim->xmin + 1;
	const int height = dim->ymax - dim->ymin + 1;

	for (line = 0; line < height; line++) {
		int row_px_count = 0;
		uint32 pix = block_getpix(x, y + line);
		uint16 count = 0;
		uint16 clear_colored_run_pair_count = 0;

		do {
			count = 0;
			while (pix == SPECIAL_TRANSPARENT && row_px_count < width) {
				count++;
				row_px_count++;
				pix = block_getpix(x + row_px_count, y + line);
			}

			*dest++ = endian(count);

			colored_run_counter = dest++;
			count = 0;

			while (pix != SPECIAL_TRANSPARENT && row_px_count < width) {
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
				*colored_run_counter = endian(count);
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
	if(  last_img_file == fname  ||  load_block(&block, &width, &height, fname, img_size)  ) {
		last_img_file = fname;
		return true;
	}
	else {
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
void image_writer_t::write_obj(FILE* outfp, obj_node_t& parent, string an_imagekey)
{
	bild_t bild;
	dimension dim;
	uint16 *pixdata = NULL;
	string imagekey;

	MEMZERO(bild);

	// Hajo: if first char is a '>' then this image is not zoomeable
	if(  an_imagekey.size() > 2  &&  an_imagekey[0] == '>'  ) {
		imagekey = an_imagekey.substr(2, std::string::npos);
		bild.zoomable = false;
	}
	else {
		imagekey = an_imagekey;
		bild.zoomable = true;
	}

	if(  imagekey != "-"  &&  imagekey != ""  ) {
		// divide key in filename and image number
		int row = -1, col = -1;
		string numkey;

		int j = imagekey.rfind('/');
		if(  j == -1  ) {
			numkey = imagekey;
		}
		else {
			numkey = imagekey.substr(j + 1, std::string::npos);
		}

		int i = numkey.find('.');
		if(  i == -1  ) {
			char reason[1024];
			sprintf(reason, "no image number in %s", imagekey.c_str() );
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		numkey = numkey.substr( i+1, std::string::npos );

		imagekey = root_writer_t::get_inpath() + imagekey.substr( 0, imagekey.size()-numkey.size() - 1 ) +  ".png";

		row = atoi(numkey.c_str());

		i = numkey.find('.');
		if(i != -1) {
			col = atoi( numkey.c_str()+i+1 );

			// add image offsets
			int comma_pos = numkey.find(',');
			if(comma_pos != -1) {
				numkey = numkey.substr( comma_pos+1, std::string::npos);
				bild.x = atoi( numkey.c_str() );
				comma_pos = numkey.find(',');
				if(comma_pos != -1) {
					bild.y = atoi(numkey.substr( comma_pos + 1, std::string::npos).c_str());
				}
			}
		}

		// Load complete file
		if (!block_laden(imagekey.c_str())) {
			char reason[1024];
			sprintf(reason, "cannot open %s", imagekey .c_str());
			throw obj_pak_exception_t("image_writer_t", reason);
		}

		if (col == -1) {
			col = row % (width / img_size);
			row = row / (width / img_size);
		}
		if (col >= (int)(width / img_size) || row >= (int)(height / img_size)) {
			char reason[1024];
			sprintf(reason, "invalid image number in %s.%s", imagekey.c_str(), numkey.c_str());
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		row *= img_size;
		col *= img_size;

		// Temp. read image and determine drawing area.
		uint32 *image = new uint32[img_size * img_size];
		for (int x = 0; x < img_size; x++) {
			for (int y = 0; y < img_size; y++) {
				image[x + y * img_size] = block_getpix(x + col, y + row);
			}
		}
		init_dim(image, &dim, img_size);
		delete [] image;

		bild.x += dim.xmin;
		bild.y += dim.ymin;
		bild.w = dim.xmax - dim.xmin + 1;
		bild.h = dim.ymax - dim.ymin + 1;
		bild.len = 0;

		if (bild.h > 0) {
			int len;
			pixdata = encode_image(col, row, &dim, &len);
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
		node.write_data_at(outfp, pixdata, 10, bild.len * sizeof(uint16));
		free(pixdata);
	}
#endif

	node.write(outfp);
}
