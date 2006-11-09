#ifndef __GOOD_READER_H
#define __GOOD_READER_H

#include "obj_reader.h"


class good_reader_t : public obj_reader_t {
    static good_reader_t the_instance;

    good_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
    static good_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_good; }
    virtual const char *get_type_name() const { return "good"; }

    /**
     * Read a goods info node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
