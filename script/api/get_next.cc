/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "get_next.h"

SQInteger generic_get_next_f(HSQUIRRELVM vm, uint32 count, uint32 F(uint32) )
{
	SQInteger index;
	if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &index))) {
		// now increase index
		if ( index >= 0 ) {
			// call the custom function
			uint32 new_index = F(index);
			if (new_index < count) {
				sq_pushinteger(vm, new_index);
				return 1;
			}
		}
	}
	else {
		if ( count>0 ) {
			sq_pushinteger(vm, 0);
			return 1;
		}
	}
	sq_pushnull(vm);
	return 1;
}


static uint32 inc(uint32 i)
{
	return i+1;
}


SQInteger generic_get_next(HSQUIRRELVM vm, uint32 count)
{
	return generic_get_next_f(vm, count, inc);
}
