#ifndef __TUNNEL_READER_H
#define __TUNNEL_READER_H

#include "obj_reader.h"


class tunnel_reader_t : public obj_reader_t {
    static tunnel_reader_t the_instance;

    tunnel_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);

    bool successfully_loaded() const;
public:
    static tunnel_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_tunnel; }
    virtual const char *get_type_name() const { return "tunnel"; }
};

#endif // __TUNNEL_READER_H
