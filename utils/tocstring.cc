/*
 * tocstring.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */



#include "tocstring.h"


/**
 * Gives a string representation of the coordinate
 * %d, %d
 * @author Hj. Malthaner
 * @date 30-Aug-03
 */
cstring_t k2_to_cstr(koord k)
{
  return csprintf("%d, %d", k.x, k.y);
}


/**
 * Gives a string representation of the 3d coordinate
 * %d, %d, %d
 * @author Hj. Malthaner
 * @date 30-Aug-03
 */
cstring_t k3_to_cstr(koord3d k)
{
  return csprintf("%d, %d, %d", k.x, k.y, k.z);
}
