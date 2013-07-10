#include "get_next.h"

SQInteger generic_get_next(HSQUIRRELVM vm, uint32 count)
{
	SQInteger index;
	if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &index))) {
		// now increase index
		if (0<=index  &&  (uint32)index+1<count) {
			index ++;
			sq_pushinteger(vm, index);
		}
		else {
			sq_pushnull(vm);
		}
	}
	else {
		if ( count>0 ) {
			sq_pushinteger(vm, 0);
		}
		else {
			sq_pushnull(vm);
		}
	}
	return 1;
}
