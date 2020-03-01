/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef _EXP_GAME_OBJ_H_
#define _EXP_GAME_OBJ_H_

#include "../squirrel/squirrel.h"

/**
 * Registers the complete export interface.
 * @param scenario true if exporting is for scenario scripting
 */
void register_export_function(HSQUIRRELVM vm, bool scenario);

#endif
