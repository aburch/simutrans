/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_DBG_WEIGHTMAP_H
#define UTILS_DBG_WEIGHTMAP_H


#include <string>
#include "../tpl/array2d_tpl.h"
#include "../dataobj/koord.h"
#include "../tpl/vector_tpl.h"

#ifdef DEBUG_WEIGHTMAPS
void dbg_weightmap(array2d_tpl<double> &map, array2d_tpl< vector_tpl<koord> > &places, unsigned weight_max, const char *base_filename, unsigned n);
#else
inline void dbg_weightmap(array2d_tpl<double> &map, array2d_tpl< vector_tpl<koord> > &places, unsigned weight_max, const char *base_filename, unsigned n) {};
#endif

#endif
