#ifndef __IMAGE_READER_H
#define __IMAGE_READER_H

#include "obj_reader.h"


class image_reader_t : public obj_reader_t {
    static image_reader_t the_instance;

    image_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:
    static image_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_image; }
    virtual const char *get_type_name() const { return "image"; }
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __IMAGE_READER_H
