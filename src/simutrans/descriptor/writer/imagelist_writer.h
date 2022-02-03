/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_IMAGELIST_WRITER_H
#define DESCRIPTOR_WRITER_IMAGELIST_WRITER_H


#include <string>
#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"


template<class T> class slist_tpl;
class obj_node_t;


class imagelist_writer_t : public obj_writer_t
{
private:
	static imagelist_writer_t the_instance;

	imagelist_writer_t() { register_writer(false); }

public:
	static imagelist_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_imagelist; }
	const char* get_type_name() const OVERRIDE { return "imagelist"; }

	void write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<std::string>& keys);
};


#endif
