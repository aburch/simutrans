#ifndef __OBJ_WRITER
#define __OBJ_WRITER

#include <stdio.h>
#include "../intro_dates.h"
#include "../objversion.h"
#include "../../utils/cstring_t.h"


class obj_node_t;
struct obj_node_info_t;
class tabfileobj_t;
template<class X> class stringhashtable_tpl;
template<class key, class X> class inthashtable_tpl;


class obj_writer_t {
	static stringhashtable_tpl<obj_writer_t *> *writer_by_name;
	static inthashtable_tpl<obj_type, obj_writer_t *> *writer_by_type;

protected:
	obj_writer_t() { /* Beware: Cannot register here! */}

	void register_writer(bool main_obj);
	void dump_nodes(FILE *infp, int level);
	void list_nodes(FILE *infp);
	void skip_nodes(FILE *fp);
	void show_capabilites();

	cstring_t name_from_next_node(FILE *fp) const;
	const char *obj_writer_t::node_writer_name(FILE *infp) const;

	virtual cstring_t get_node_name(FILE * /*fp*/) const { return ""; }

	virtual void dump_node(FILE *infp, const obj_node_info_t &node);
	virtual void write_obj(FILE * /*fp*/, obj_node_t &/*parent*/, tabfileobj_t &/*obj*/) {}
	void write_head(FILE *fp, obj_node_t &node, tabfileobj_t &obj);

public:
	virtual obj_type get_type() const = 0;
	virtual const char *get_type_name() const = 0;

	static void write(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);
};

#endif
