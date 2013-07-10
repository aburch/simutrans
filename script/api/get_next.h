#ifndef _GET_NEXT_H_
#define _GET_NEXT_H_

#include "../../simtypes.h"
#include "../../squirrel/squirrel.h"

/**
 * Implements custom function to realize foreach-iterators.
 * Can be used to export _nexti method.
 *
 * @param count total number of elements in iterated list
 */
SQInteger generic_get_next(HSQUIRRELVM vm, uint32 count);

#endif
