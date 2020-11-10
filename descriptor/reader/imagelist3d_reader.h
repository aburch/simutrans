/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_IMAGELIST3D_READER_H
#define DESCRIPTOR_READER_IMAGELIST3D_READER_H


#include "obj_reader.h"


class imagelist3d_reader_t : public obj_reader_t
{
    static imagelist3d_reader_t the_instance;

    imagelist3d_reader_t() { register_reader(); }

public:
    static imagelist3d_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_imagelist3d; }
    virtual const char *get_type_name() const { return "imagelist3d"; }

    virtual obj_desc_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif
