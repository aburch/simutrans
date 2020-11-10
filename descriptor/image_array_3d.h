/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_IMAGE_ARRAY_3D_H
#define DESCRIPTOR_IMAGE_ARRAY_3D_H


#include "image_list.h"
#include "image_array.h"

/*
 *  Autors:
 *      Volker Meyer & jamespetts
 *
 *  Description:
 *      3-dimentional image array
 *
 *  Child nodes:
 *	0   1. Image list
 *	1   2. Image list
 *  2   3. Image list
 *	... ...
 */
class image_array_3d_t : public obj_desc_t {
	friend class imagelist3d_reader_t;
	friend class imagelist3d_writer_t;

	uint16  count;

public:
	image_array_3d_t() : count(0) {}

	uint16 get_count() const { return count; }

	image_array_t const* get_list_2d(uint16 i)				  const { return i < count ? get_child<image_array_t>(i)					: 0; }
	image_list_t   const* get_list(uint16 i, uint16 j)		  const { return i < count ? get_child<image_array_t>(i)->get_list(j)	: 0; }
	image_t        const* get_image(uint16 i, uint16 j, uint16 k) const { return i < count ? get_child<image_array_t>(i)->get_image(j, k)	: 0; }
};

#endif
