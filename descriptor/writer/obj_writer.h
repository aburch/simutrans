/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_OBJ_WRITER_H
#define DESCRIPTOR_WRITER_OBJ_WRITER_H


#include <stdio.h>
#include <string>
#include "../intro_dates.h"
#include "../objversion.h"


class obj_node_t;
struct obj_node_info_t;
class tabfileobj_t;
template<class X> class stringhashtable_tpl;
template<class key, class X> class inthashtable_tpl;


class obj_writer_t
{
private:
	static stringhashtable_tpl<obj_writer_t*>* writer_by_name;
	static inthashtable_tpl<obj_type, obj_writer_t*>* writer_by_type;

	static int default_image_size;

protected:
	obj_writer_t() { /* Beware: Cannot register here! */ }

	void register_writer(bool main_obj);
	void dump_nodes(FILE* infp, int level, uint16 index = 0);
	void list_nodes(FILE* infp);
	size_t skip_nodes(FILE* fp);
	void show_capabilites();

	std::string name_from_next_node(FILE* fp) const;
	const char* node_writer_name(FILE* infp) const;

	virtual std::string get_node_name(FILE* /*fp*/) const { return ""; }

	virtual void dump_node(FILE* infp, const obj_node_info_t& node);
	virtual void write_obj(FILE* /*fp*/, obj_node_t& /*parent*/, tabfileobj_t& /*obj*/) {}
	void write_head(FILE* fp, obj_node_t& node, tabfileobj_t& obj);

public:
	// contains the name of the last obj/name header
	static const char *last_name;

	virtual ~obj_writer_t() {}

	virtual obj_type get_type() const = 0;
	virtual const char* get_type_name() const = 0;

	static void write(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);

	static void set_img_size(int img_size) { obj_writer_t::default_image_size = img_size; }
};


#endif
