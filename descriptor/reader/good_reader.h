/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_GOOD_READER_H
#define DESCRIPTOR_READER_GOOD_READER_H


#include "obj_reader.h"


class goods_reader_t : public obj_reader_t {
	static goods_reader_t the_instance;

	goods_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;
public:
	static goods_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_good; }
	char const* get_type_name() const OVERRIDE { return "good"; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};

#endif
