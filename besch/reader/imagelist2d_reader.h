#ifndef __IMAGELIST2D_READER_H
#define __IMAGELIST2D_READER_H

#include "obj_reader.h"


class imagelist2d_reader_t : public obj_reader_t {
    static imagelist2d_reader_t the_instance;

    imagelist2d_reader_t() { register_reader(); }
public:
    static imagelist2d_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_imagelist2d; }
    virtual const char *get_type_name() const { return "imagelist2d"; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
