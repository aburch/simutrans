/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "raw_image.h"

#include "classify_file.h"
#include "../simmem.h"

#include <cassert>
#include <cstring>
#include <stdlib.h>
#include <algorithm>


raw_image_t::raw_image_t()
	: data(NULL)
	, width(0)
	, height(0)
	, fmt(FMT_INVALID)
	, bpp(0)
{
}


raw_image_t::raw_image_t(uint32 width, uint32 height, raw_image_t::format_t format)
	: data(NULL)
	, width(width)
	, height(height)
	, fmt(format)
	, bpp(bpp_for_format(format))
{
	data = MALLOCN(uint8, width * height * (bpp/CHAR_BIT));
}


raw_image_t::raw_image_t(const raw_image_t& rhs) :
	data(NULL),
	width(rhs.width),
	height(rhs.height),
	fmt(rhs.fmt),
	bpp(rhs.bpp)
{
	const size_t data_size = width * height * (bpp/CHAR_BIT);

	data = MALLOCN(uint8, data_size);
	memcpy(data, rhs.data, data_size);
}


raw_image_t::~raw_image_t()
{
	if (data) {
		free(data);
	}
}


raw_image_t &raw_image_t::operator=(raw_image_t rhs)
{
	swap(*this, rhs);
	return *this;
}


void raw_image_t::copy_from(const raw_image_t &rhs)
{
	*this = rhs;
}


bool raw_image_t::read_from_file(const char *filename)
{
	file_info_t finfo;
	const file_classify_status_t status = classify_image_file(filename, &finfo);

	if (status != FILE_CLASSIFY_OK) {
		return false;
	}

	switch (finfo.file_type) {
	case file_info_t::TYPE_PNG: return read_png(filename);
	case file_info_t::TYPE_BMP: return read_bmp(filename);
	case file_info_t::TYPE_PPM: return read_ppm(filename);
	default: return false;
	}
}


uint8 raw_image_t::bpp_for_format(raw_image_t::format_t format)
{
	switch (format) {
	case FMT_INVALID:  return 0;
	case FMT_GRAY8:    return 8;
	case FMT_RGB888:   return 24;
	case FMT_RGBA8888: return 32;
	default: assert(false); return 0;
	}
}


void swap(raw_image_t &lhs, raw_image_t &rhs)
{
	using std::swap;

	swap(lhs.data, rhs.data);
	swap(lhs.width, rhs.width);
	swap(lhs.height, rhs.height);
	swap(lhs.fmt, rhs.fmt);
	swap(lhs.bpp, rhs.bpp);
}
