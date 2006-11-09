#ifndef __VEHICLE_READER_H
#define __VEHICLE_READER_H

#include "obj_reader.h"


class vehicle_reader_t : public obj_reader_t {
    static vehicle_reader_t the_instance;

    vehicle_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static vehicle_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_vehicle; }
    virtual const char *get_type_name() const { return "vehicle"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
