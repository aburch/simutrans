#ifndef _API_H_
#define _API_H_

/** @file api.h declarations of export functions. */

#include "../../squirrel/squirrel.h"

void export_city(HSQUIRRELVM vm);
void export_convoy(HSQUIRRELVM vm);
void export_factory(HSQUIRRELVM vm);
void export_goods_desc(HSQUIRRELVM vm);
void export_gui(HSQUIRRELVM vm);
void export_halt(HSQUIRRELVM vm);
void export_player(HSQUIRRELVM vm);
void export_scenario(HSQUIRRELVM vm);
void export_settings(HSQUIRRELVM vm);
void export_tiles(HSQUIRRELVM vm);
void export_world(HSQUIRRELVM vm);

void export_global_constants(HSQUIRRELVM vm);


#endif
