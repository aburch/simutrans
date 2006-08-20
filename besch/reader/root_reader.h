#include "obj_reader.h"

class root_reader_t : public obj_reader_t {
    static root_reader_t the_instance;

    root_reader_t() { register_reader(); }
public:
    static root_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_root; }
    virtual const char *get_type_name() const { return "root"; }
protected:
    virtual void register_obj(obj_besch_t *&data);
};
