#ifndef __IMAGELIST_READER_H
#define __IMAGELIST_READER_H

#include "obj_reader.h"


class imagelist_reader_t : public obj_reader_t {
    static imagelist_reader_t the_instance;

    imagelist_reader_t() { register_reader(); }
public:
    static imagelist_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_imagelist; }
    virtual const char *get_type_name() const { return "imagelist"; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
