
/*
 * read defines
 */

#include <png.h>
#include <stdlib.h>


#include "dr_rdpng.h"



static png_uint_32 width, height;
static int bit_depth, color_type, interlace_type;

static int load_as_block = 0;


static void
read_png(unsigned char *block, FILE *file)
{
    png_structp png_ptr;
    png_infop   info_ptr;
    png_bytep row_pointers[1024];
    unsigned row, x, y;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if (png_ptr == NULL) {
	printf("read_png: Could not create read struct.\n");
	exit(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
	printf("read_png: Could not create info struct.\n");
	png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
	exit(1);
    }


    if (setjmp(png_ptr->jmpbuf)) {

	printf("read_png: fatal error.\n");
        png_read_destroy(png_ptr, info_ptr, (png_info *)0);
        /* free pointers before returning, if necessary */
        free(png_ptr);
        free(info_ptr);

	exit(1);
    }

    /* Set up the input control if you are using standard C streams */
    png_init_io(png_ptr, file);


    /* The call to png_read_info() gives us all of the information from the
     * PNG file before the first IDAT (image data chunk).  REQUIRED
     */
    png_read_info(png_ptr, info_ptr);



    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height, &bit_depth, &color_type,
		 &interlace_type, NULL, NULL);


    // printf("read_png: width=%d, height=%d, bit_depth=%d\n", width, height, bit_depth);
    // printf("read_png: color_type=%d, interlace_type=%d\n", color_type, interlace_type);


    /* tell libpng to strip 16 bit/color files down to 8 bits/color */
    png_set_strip_16(png_ptr);


    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images).
     */
    png_set_packing(png_ptr);



    /* dither rgb files down to 8 bit palette & reduce palettes
     * to the number of colors available on your screen
     */
    if (color_type & PNG_COLOR_MASK_COLOR)
    {
	if (info_ptr->valid & PNG_INFO_PLTE) {
	    png_set_dither(png_ptr,
		           info_ptr->palette,
			   info_ptr->num_palette,
			   256,
			   info_ptr->hist,
			   1);
         } else {
	    printf("Fatal error: Cant convert image to colormap.\n");
	    exit(1);
	}
    }


    png_start_read_image(png_ptr);

    /* The easiest way to read the image: */

    for(row = 0; row < height; row++) {
	row_pointers[row] = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    /* Read the entire image in one go */
    png_read_image(png_ptr, row_pointers);


    for(y=0; y<height; y++) {
	const int off = y*width;
	for(x=0; x<width; x++) {
	    block[off + x] = row_pointers[y][x];
	}
    }


    for(row = 0; row < height; row++) {
	free(row_pointers[row]);
    }


    /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
    png_read_end(png_ptr, info_ptr);


    /* At this point you have read the entire image */

    /* clean up after the read, and free any memory allocated - REQUIRED */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
}


int load_block(unsigned char *block, char *fname)
{
    FILE *file;

    file = fopen(fname, "rb");

    if(file) {
	read_png(block, file);
	fclose(file);
    } else {
	perror("Error:");
    }

    return file != 0;
}
