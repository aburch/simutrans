
#include <string>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror

#include "../simmem.h"
#include "../simdebug.h"
#include "dr_rdpng.h"

static std::string filename_;

static void read_png(unsigned char** block, unsigned* width, unsigned* height, FILE* file, const int base_img_size)
{
	png_structp png_ptr;
	png_infop   info_ptr;
	png_bytep* row_pointers;
	unsigned row, x, y;
	int rowbytes;
	unsigned char* dst;
	int color_type;

	//png_uint_32 is 64 bit on some architectures!
	png_uint_32 widthpu32,heightpu32;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (png_ptr == NULL) {
		dbg->error( "while loading PNG", "Could not create read struct in %s.", filename_.c_str() );
		exit(1);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		dbg->error( "while loading PNG", "Could not create info struct in %s.", filename_.c_str() );
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		exit(1);
	}

#ifdef PNG_SETJMP_SUPPORTED
	if(  setjmp(png_jmpbuf(png_ptr)  )) {
		dbg->error( "while loading PNG", "Fatal error in %s.", filename_.c_str() );
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_info**)0);
		exit(1);
	}
#endif

	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, file);

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED
	 */
	png_read_info(png_ptr, info_ptr);

	int bit_depth;
	png_get_IHDR(png_ptr, info_ptr, &widthpu32, &heightpu32, &bit_depth, &color_type, 0, 0, 0);
	*width = widthpu32;
	*height = heightpu32;

	if (*height % base_img_size != 0 || *width % base_img_size != 0) {
		dbg->fatal( "while loading PNG", "Invalid image size in %s.", filename_.c_str() );
		exit(1);
	}

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* Expand paletted colors into true RGB triplets */
	png_set_expand(png_ptr);

	/* Don't output alpha channel */
	png_set_strip_alpha(png_ptr);

	if(  (color_type & PNG_COLOR_MASK_ALPHA) == PNG_COLOR_MASK_ALPHA  ) {
		dbg->warning( "while loading PNG", "ignoring alpha channel for %s", filename_.c_str() );
		// author note: It might be that this won't catch files with format
		// palette + transparency, which is a really rare but possible combination.
	}

	// update info - png_get_rowbytes might return incorrect values
	png_read_update_info( png_ptr,  info_ptr);

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	row_pointers = MALLOCN(png_byte*, *height);

	row_pointers[0] = MALLOCN(png_byte, rowbytes * *height);
	for (row = 1; row < *height; row++) {
		row_pointers[row] = row_pointers[row - 1] + rowbytes;
	}

	/* Read the entire image in one go */
	png_read_image(png_ptr, row_pointers);

	// we use fixed height here because block is of limited, fixed size
	// not fixed any more

	*block = REALLOC(*block, unsigned char, *height * *width * 3);

	dst = *block;
	for (y = 0; y < *height; y++) {
		for (x = 0; x < *width * 3; x++) {
			*dst++ = row_pointers[y][x];
		}
	}

	free(row_pointers[0]);
	free(row_pointers);

	/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* At this point you have read the entire image */

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
}


bool load_block(unsigned char** block, unsigned* width, unsigned* height, const char* fname, const int base_img_size)
{
	// remember the file name for better error messages.
	filename_ = fname;

	if (FILE* const file = fopen(fname, "rb")) {
		read_png(block, width, height, file, base_img_size);
		fclose(file);
		return true;
	}
	else {
		dbg->warning( "while loading PNG", "%s: %s", fname, strerror(errno) );
		return false;
	}
}


// output either a 32 or 16 or 15 bitmap
int write_png( const char *file_name, unsigned char *data, int width, int height, int bit_depth )
{
	// remember the file name for better error messages.
	filename_ = file_name;

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	FILE *fp = fopen(file_name, "wb");
	if (!fp) {
		return 0;
	}

	// init structures
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose( fp );
		return 0;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
		fclose( fp );
		return 0;
	}

#ifdef PNG_SETJMP_SUPPORTED
	if(  setjmp( png_jmpbuf(png_ptr) )  ) {
		dbg->error( "write_png", "fatal error");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		exit(1);
	}
#endif

	// assign file
	png_init_io(png_ptr, fp);

#if PNG_LIBPNG_VER_MAJOR<=1  &&  PNG_LIBPNG_VER_MINOR<5
	/* set the zlib compression level */
	png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );
#endif

	// output header
	png_set_IHDR( png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_INTERLACE_NONE, PNG_FILTER_TYPE_DEFAULT );
	png_write_info(png_ptr, info_ptr);

	if(  bit_depth==32  ) {
		// write image data
		int i;
		png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);
		for(  i=0;  i<height;  i++ ) {
			png_bytep row_pointer = data+(i*width*4);
			png_write_row( png_ptr, row_pointer );
		}
	}
	else {
		dbg->fatal( "write_png", "32 bit not supported!" );
		exit(0);
	}

	// free all
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose( fp );
	return 1;
}
