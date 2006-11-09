#ifndef __VEHICLE_WRITER_H
#define __VEHICLE_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


class vehicle_writer_t : public obj_writer_t {
    static vehicle_writer_t the_instance;

    vehicle_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }
public:

    /**
     * Writes vehicle node data to file
     * @author Hj. Malthaner
     */
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_vehicle; }
    virtual const char *get_type_name() const { return "vehicle"; }
};

#endif
