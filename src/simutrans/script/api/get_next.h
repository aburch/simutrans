/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_GET_NEXT_H
#define SCRIPT_API_GET_NEXT_H


#include "../../simtypes.h"
#include "../../../squirrel/squirrel.h"

/**
 * Implements custom function to realize foreach-iterators.
 * Can be used to export _nexti method.
 * Takes argument of _nexti from top of the stack.
 * Pushes null or the new index to the stack.
 * @pre assumed to be bound to _nexti meta method.
 * @param count total number of elements in iterated list
 */
SQInteger generic_get_next(HSQUIRRELVM vm, uint32 count);

/**
 * Same as generic_get_next.
 * @param F custom function to increase index. Takes old index as parameter.
 */
SQInteger generic_get_next_f(HSQUIRRELVM vm, uint32 count, uint32 F(uint32) );

#endif
