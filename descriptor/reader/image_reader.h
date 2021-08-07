/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_IMAGE_READER_H
#define DESCRIPTOR_READER_IMAGE_READER_H


#include "obj_reader.h"


class image_t;


class image_reader_t : public obj_reader_t {
	static image_reader_t the_instance;

	image_reader_t() { register_reader(); }
public:
	static image_reader_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_image; }
	char const* get_type_name() const OVERRIDE { return "image"; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

private:
	bool image_has_valid_data(image_t *img) const;
};

#endif
