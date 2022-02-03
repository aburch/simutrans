/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_IMAGE_LIST_H
#define DESCRIPTOR_IMAGE_LIST_H


#include "image.h"

/**
 * One-dimensional image list.
 *
 * Child nodes:
 *  0   1st Image
 *  1   2nd Image
 * ... ...
 */
class image_list_t : public obj_desc_t {
	friend class imagelist_reader_t;

	uint16  count;

public:
	image_list_t() : count(0) {}

	uint16 get_count() const { return count; }

	image_t const* get_image(uint16 i) const { return i < count ? get_child<image_t>(i) : 0; }

	image_id get_image_id(uint16 i) const {
		const image_t *image = get_image(i);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}
};

#endif
