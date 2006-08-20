#ifndef __CROSSING_READER_H
#define __CROSSING_READER_H

#include "obj_reader.h"


class crossing_reader_t : public obj_reader_t {
    static crossing_reader_t the_instance;

    crossing_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
    static crossing_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_crossing; }
    virtual const char *get_type_name() const { return "crossing"; }
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __CROSSING_READER_H
