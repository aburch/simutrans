/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_IMAGE_ARRAY_H
#define DESCRIPTOR_IMAGE_ARRAY_H


#include "image_list.h"

/**
 * Two-dimensional array of images
 *
 * Child nodes:
 *  0   1st Image-list
 *  1   2nd Image-list
 * ... ...
 */
class image_array_t : public obj_desc_t {
	friend class imagelist2d_reader_t;

	uint16  count;

public:
	image_array_t() : count(0) {}

	uint16 get_count() const { return count; }

	image_list_t const* get_list(uint16 i)            const { return i < count ? get_child<image_list_t>(i)               : 0; }
	image_t      const* get_image(uint16 i, uint16 j) const { return i < count ? get_child<image_list_t>(i)->get_image(j) : 0; }
};

#endif
