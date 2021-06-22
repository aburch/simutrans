/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_IMAGELIST2D_WRITER_H
#define DESCRIPTOR_WRITER_IMAGELIST2D_WRITER_H


#include <string>
#include "obj_writer.h"
#include "../objversion.h"


template<class T> class slist_tpl;


class imagelist2d_writer_t : public obj_writer_t
{
private:
	static imagelist2d_writer_t the_instance;

	imagelist2d_writer_t() { register_writer(false); }

public:
	static imagelist2d_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_imagelist2d; }
	const char* get_type_name() const OVERRIDE { return "imagelist2d"; }

	void write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<std::string> >& keys);
};


#endif
