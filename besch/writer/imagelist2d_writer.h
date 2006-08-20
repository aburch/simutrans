#ifndef __IMAGELIST2D_WRITER_H
#define __IMAGELIST2D_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


template<class T> class slist_tpl;


class imagelist2d_writer_t : public obj_writer_t {
    static imagelist2d_writer_t the_instance;

    imagelist2d_writer_t() { register_writer(false); }
public:
    static imagelist2d_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_imagelist2d; }
    virtual const char *get_type_name() const { return "imagelist2d"; }

    void write_obj(FILE *fp, obj_node_t &parent, const slist_tpl< slist_tpl<cstring_t> > &keys);
};

#endif // __IMAGELIST2D_WRITER_H
