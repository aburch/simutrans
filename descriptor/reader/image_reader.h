/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_IMAGE_READER_H
#define DESCRIPTOR_READER_IMAGE_READER_H


#include "obj_reader.h"


class image_t;


class image_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(image_reader_t, obj_image, "image");

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

private:
	bool image_has_valid_data(image_t *img) const;
};


#endif
