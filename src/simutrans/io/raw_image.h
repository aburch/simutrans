/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RAW_IMAGE_H
#define IO_RAW_IMAGE_H


#include "../simtypes.h"

#include <cassert>
#include <stdio.h>


/**
 * Class to store, read or write raw image data in various formats.
 * The data are stored without gaps, i.e.
 * @code
 *   access_pixel(x, y+1) == access_pixel(x, y) + get_width() * (get_bpp()/CHAR_BIT)
 * @endcode
 * for 0 <= x < width and 0 <= y < height-1
 */
class raw_image_t
{
public:
	enum format_t
	{
		FMT_INVALID = 0xFF,
		FMT_GRAY8   = 0, ///< 8 bpp grayscale
		FMT_RGBA8888,    ///< 32 bpp RGBA; alpha == 0xFF is fully opaque
		FMT_RGB888       ///< 24 bpp RGB
	};

public:
	/// Constructs an empty invalid image.
	raw_image_t();

	/// Allocates a writable image with the specified dimensions and format.
	raw_image_t(uint32 width, uint32 height, format_t format);

	~raw_image_t();

	friend void swap(raw_image_t &lhs, raw_image_t &rhs);

private:
	/// Copy ctor; use copy_from for explicit copying
	raw_image_t(const raw_image_t &rhs);

	raw_image_t &operator=(raw_image_t rhs);

public:
	format_t get_format() const { return (format_t)fmt; }
	uint8 get_bpp() const { return bpp; }

	uint32 get_width() const { return width; }
	uint32 get_height() const { return height; }

	/// Access the first byte of a pixel. The actual number of bytes per pixel and the order of color channels depends on the format.
	const uint8 *access_pixel(uint32 x, uint32 y) const { assert(fmt != FMT_INVALID); return data + (x + y*width) * (bpp/CHAR_BIT); }
	      uint8 *access_pixel(uint32 x, uint32 y)       { assert(fmt != FMT_INVALID); return data + (x + y*width) * (bpp/CHAR_BIT); }

	/// @returns the bits per pixel for @p format
	static uint8 bpp_for_format(format_t format);

	void copy_from(const raw_image_t &rhs);

public:
	/// Read image from file. File format is detected automatically.
	/// @returns true on success
	bool read_from_file(const char *filename);

	/// @returns true on success
	bool write_png(const char *filename) const;
	bool write_bmp(const char *filename) const;
	bool write_ppm(const char *filename) const;

private:
	bool read_bmp(const char *filename);
	bool read_ppm(const char *filename);
	bool read_png(const char *filename);

	bool read_png_data(FILE *file);

private:
	uint8 *data;
	uint32 width;
	uint32 height;
	uint8 fmt;
	uint8 bpp;
};


void swap(raw_image_t &lhs, raw_image_t &rhs);


#endif
