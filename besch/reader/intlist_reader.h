#ifndef __INTLIST_READER_H
#define __INTLIST_READER_H

#include "obj_reader.h"


class intlist_reader_t : public obj_reader_t {
    static intlist_reader_t the_instance;

    intlist_reader_t() { register_reader(); }
public:
    static intlist_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_intlist; }
    virtual const char *get_type_name() const { return "intlist"; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __INTLIST_READER_H
