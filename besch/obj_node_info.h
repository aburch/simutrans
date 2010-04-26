/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __OBJ_NODE_INFO_H
#define __OBJ_NODE_INFO_H

#include "../simtypes.h"

// these are the sizes of the divers heades on the disk
// if the uint16 size is LARGE_RECORD_SIZE the actual length is in a following uint32
#define OBJ_NODE_INFO_SIZE (8)
#define EXT_OBJ_NODE_INFO_SIZE (12)
#define LARGE_RECORD_SIZE (0xFFFFu)

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      stored structure of a pak node inside the file.
 */
struct obj_node_info_t {
    uint32  type;
    uint16  children;
    uint32  size;
};

#endif
