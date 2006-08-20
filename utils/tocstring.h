/*
 * tocstring.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/*
 * This module contains functions to convert primitive datatypes
 * to cstring_t representations, i.e. to display their values
 *
 * Please note that these functions are not very efficient. Don't
 * use them in performance critical routines, not even in loops!!!
 */


#ifndef TO_CSTRING_H
#define TO_CSTRING_H

#ifndef CSTRING_T_H
#include "cstring_t.h"
#endif

#ifndef koord_h
#include "../dataobj/koord.h"
#endif

#ifndef koord3d_h
#include "../dataobj/koord3d.h"
#endif


/**
 * Gives a string representation of the coordinate
 * %d, %d
 * @author Hj. Malthaner
 * @date 30-Aug-03
 */
cstring_t k2_to_cstr(koord k);


/**
 * Gives a string representation of the 3d coordinate
 * %d, %d, %d
 * @author Hj. Malthaner
 * @date 30-Aug-03
 */
cstring_t k3_to_cstr(koord3d k);


#endif
