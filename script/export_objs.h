#ifndef _EXP_GAME_OBJ_H_
#define _EXP_GAME_OBJ_H_

#include "../squirrel/squirrel.h"

class karte_t;
/**
 * Registers the complete export interface.
 */
void register_export_function(HSQUIRRELVM vm, karte_t *w);

#endif
