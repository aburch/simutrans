#ifndef __TREE_READER_H
#define __TREE_READER_H

#include "obj_reader.h"


class tree_reader_t : public obj_reader_t {
    static tree_reader_t the_instance;

    tree_reader_t() { register_reader(); }
protected:
    virtual bool successfully_loaded() const;
    virtual void register_obj(obj_besch_t *&data);
public:
    static tree_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_tree; }
    virtual const char *get_type_name() const { return "tree"; }
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
