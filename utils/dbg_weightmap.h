#ifndef DBG_WEIGHTMAP_H
#define DBG_WEIGHTMAP_H
#ifdef DBG_WEIGHTMAP

#include <string>
#include "../tpl/array2d_tpl.h"
#include "../dataobj/koord.h"
#include "../tpl/vector_tpl.h"

void dbg_weightmap(array2d_tpl<double> &map, array2d_tpl< vector_tpl<koord> > &places, unsigned weight_max, const char *base_filename, unsigned n);
#else
inline void dbg_weightmap(array2d_tpl<double> &map, array2d_tpl< vector_tpl<koord> > &places, unsigned weight_max, const char *base_filename, unsigned n) {};
#endif
#endif
