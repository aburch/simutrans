/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_EXPORT_OBJS_H
#define SCRIPT_EXPORT_OBJS_H


#include "../squirrel/squirrel.h"

/**
 * Registers the complete export interface.
 */
void register_export_function(HSQUIRRELVM vm);

#endif
