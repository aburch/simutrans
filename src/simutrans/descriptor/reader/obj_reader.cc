/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "obj_reader.h"

size_t node_body_t::buf_size = 0;
uint8* node_body_t::buf = 0;
#if DEBUG
uint8 node_body_t::usage = 0;
#endif

