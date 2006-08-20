#include "obj_reader.h"

class text_reader_t : public obj_reader_t {
    static text_reader_t the_instance;

    text_reader_t() { register_reader(); }
public:
    static text_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_text; }
    virtual const char *get_type_name() const { return "text"; }
};
