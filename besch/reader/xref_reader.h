#include "obj_reader.h"

class xref_reader_t : public obj_reader_t {
    static xref_reader_t the_instance;

    xref_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:
    static xref_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_xref; }
    virtual const char *get_type_name() const { return "reference"; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};
