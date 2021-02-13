/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "raw_image.h"

#include <string>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror

#include "../simmem.h"
#include "../simdebug.h"


static std::string filename_;


bool raw_image_t::read_png_data(FILE *file)
{
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (png_ptr == NULL) {
		dbg->error( "raw_image_t::read_png_data", "Could not create read struct");
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		dbg->error( "raw_image_t::read_png_data", "Could not create info struct");
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return false;
	}

#ifdef PNG_SETJMP_SUPPORTED
	if(  setjmp(png_jmpbuf(png_ptr)  )) {
		dbg->error( "raw_image_t::read_png_data", "Fatal error in %s.", filename_.c_str());
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_info**)0);
		return false;
	}
#endif

	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, file);

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED
	 */
	png_read_info(png_ptr, info_ptr);

	//png_uint_32 is 64 bit on some architectures!
	png_uint_32 new_width;
	png_uint_32 new_height;
	int bit_depth;
	int color_type;

	if (!png_get_IHDR(png_ptr, info_ptr, &new_width, &new_height, &bit_depth, &color_type, 0, 0, 0)) {
		dbg->error("raw_image_t::read_png_data", "Failed to read IHDR from '%s'", filename_.c_str());
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_info**)0);
		return false;
	}

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* Expand paletted colors into true RGB triplets */
	if(  color_type == PNG_COLOR_TYPE_PALETTE  ) {
		png_set_expand(png_ptr); // tRNS to alpha

		if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
			png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
		}

		color_type = PNG_COLOR_TYPE_RGBA;
	}
	else if (color_type == PNG_COLOR_TYPE_RGB) {
		// add opaque alpha channel
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
		color_type = PNG_COLOR_TYPE_RGBA;
	}
	else if (color_type == PNG_COLOR_TYPE_GA) {
		dbg->warning("raw_image_t::read_png_data", "Ignoring alpha channel for grayscale image '%s'", filename_.c_str());
		png_set_strip_alpha(png_ptr);
		color_type = PNG_COLOR_TYPE_GRAY;
	}

	// update info - png_get_rowbytes might return incorrect values
	png_read_update_info( png_ptr,  info_ptr);

	const size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	png_bytep *row_pointers = MALLOCN(png_bytep, new_height);

	row_pointers[0] = MALLOCN(png_byte, rowbytes * new_height);
	for (uint32 row = 1; row < new_height; row++) {
		row_pointers[row] = row_pointers[row - 1] + rowbytes;
	}

	/* Read the entire image in one go */
	png_read_image(png_ptr, row_pointers);

	// we use fixed height here because block is of limited, fixed size
	// not fixed any more

	format_t new_fmt = FMT_INVALID;

	switch (color_type) {
	case PNG_COLOR_TYPE_RGBA: new_fmt = FMT_RGBA8888; break;
	case PNG_COLOR_TYPE_RGB:  new_fmt = FMT_RGB888; break;
	case PNG_COLOR_TYPE_GRAY: new_fmt = FMT_GRAY8;  break;
	}

	if (new_fmt == FMT_INVALID) {
		dbg->error("raw_image_t::read_png_data", "Unsupported PNG format");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_info**)0);
		return false;
	}

	const uint8 new_bpp = raw_image_t::bpp_for_format(raw_image_t::format_t(new_fmt));
	const size_t old_size = width     * height     * (bpp    /CHAR_BIT);
	const size_t new_size = new_width * new_height * (new_bpp/CHAR_BIT);

	if (new_size == 0) {
		free(data);
		data = NULL;
	}
	else if (new_size > old_size) {
		data = REALLOC(data, uint8, new_size);
	}

	fmt    = new_fmt;
	width  = new_width;
	height = new_height;
	bpp    = new_bpp;

	if (new_size > 0) {
		uint8 *dst = data;
		for (uint32 y = 0; y < height; y++) {
			for (uint32 x = 0; x < width * (bpp/CHAR_BIT); x++) {
				*dst++ = row_pointers[y][x];
			}
		}
	}

	free(row_pointers[0]);
	free(row_pointers);

	/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* At this point you have read the entire image */

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	return true;
}


bool raw_image_t::read_png(const char *fname)
{
	// remember the file name for better error messages.
	filename_ = fname;

	FILE* file = fopen(fname, "rb");

	if (file) {
		const bool ok = read_png_data(file);
		fclose(file);
		return ok;
	}
	else {
		dbg->warning( "raw_image_t::read_png", "Cannot open %s: %s", fname, strerror(errno) );
		return false;
	}
}


bool raw_image_t::write_png(const char *file_name) const
{
	// remember the file name for better error messages.
	filename_ = file_name;

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	FILE *fp = fopen(file_name, "wb");
	if (!fp) {
		return false;
	}

	// init structures
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose( fp );
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
		fclose( fp );
		return false;
	}

#ifdef PNG_SETJMP_SUPPORTED
	if(  setjmp( png_jmpbuf(png_ptr) )  ) {
		dbg->error( "raw_image_t::write_png", "fatal error");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
#endif

	// assign file
	png_init_io(png_ptr, fp);

#if PNG_LIBPNG_VER_MAJOR<=1  &&  PNG_LIBPNG_VER_MINOR<5
	/* set the zlib compression level */
	png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );
#endif

	// output header
	int color_type;

	switch (fmt) {
	case FMT_RGBA8888: color_type = PNG_COLOR_TYPE_RGBA; break;
	case FMT_RGB888:   color_type = PNG_COLOR_TYPE_RGB;  break;
	case FMT_GRAY8:    color_type = PNG_COLOR_TYPE_GRAY; break;
	default:
		dbg->fatal( "raw_image_t::write_png", "Cannot write png file: Unupported source format" );
	}

	png_set_IHDR( png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
	png_write_info(png_ptr, info_ptr);

	switch (fmt) {
	case FMT_RGBA8888:
	case FMT_RGB888:
	case FMT_GRAY8:
		{
			for (uint32 y = 0; y < height; ++y) {
				// hack to compile with old libpng versions that take a png_bytep instead of a png_const_bytep
				const uint8 *row = access_pixel(0, y);
				png_write_row(png_ptr, const_cast<png_bytep>(row));
			}
		}
		break;

	default:
		dbg->error("raw_image_t::write_png", "Unsupported source format for writing png");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose( fp );
		return false;
	}

	// free all
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose( fp );
	return true;
}
