#ifndef __BUILDING_READER_H
#define __BUILDING_READER_H

#include "obj_reader.h"


class tile_reader_t : public obj_reader_t {
    static tile_reader_t the_instance;

	tile_reader_t() { register_reader(); }
public:
    static tile_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_tile; }
    virtual const char *get_type_name() const { return "tile"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};


class building_reader_t : public obj_reader_t {
    static building_reader_t the_instance;

    building_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static building_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_building; }
    virtual const char *get_type_name() const { return "building"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

};

#endif // __BUILDING_READER_H
