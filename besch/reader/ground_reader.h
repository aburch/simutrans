#ifndef __GROUND_READER_H
#define __GROUND_READER_H

#include "obj_reader.h"


class ground_reader_t : public obj_reader_t {
    static ground_reader_t the_instance;

    ground_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
    static ground_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_ground; }
    virtual const char *get_type_name() const { return "ground"; }
};

#endif // __GROUND_READER_H
