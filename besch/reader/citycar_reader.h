/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __CITYCAR_READER_H
#define __CITYCAR_READER_H

#include "obj_reader.h"


/*
 *  Autor:
 *      Volker Meyer
 */
class citycar_reader_t : public obj_reader_t {
    static citycar_reader_t the_instance;

    citycar_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static citycar_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_citycar; }
    virtual const char *get_type_name() const { return "citycar"; }
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
