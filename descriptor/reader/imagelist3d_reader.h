/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_IMAGELIST3D_READER_H
#define DESCRIPTOR_READER_IMAGELIST3D_READER_H


#include "obj_reader.h"


class imagelist3d_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(imagelist3d_reader_t, obj_imagelist3d, "imagelist3d");

public:
	/// @copydoc obj_reader::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
