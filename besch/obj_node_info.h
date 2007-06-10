/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 */
#ifndef __OBJ_NODE_INFO_H
#define __OBJ_NODE_INFO_H

#include "../simtypes.h"

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
    uint16  size;
};

#endif
