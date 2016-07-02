/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_API_H
#define SCRIPT_API_API_H


/** @file api.h declarations of export functions. */

#include "../../squirrel/squirrel.h"

void export_city(HSQUIRRELVM vm);
void export_control(HSQUIRRELVM vm);
void export_convoy(HSQUIRRELVM vm);
void export_factory(HSQUIRRELVM vm);
void export_goods_desc(HSQUIRRELVM vm);
void export_gui(HSQUIRRELVM vm);
void export_halt(HSQUIRRELVM vm);
void export_line(HSQUIRRELVM vm);
void export_map_objects(HSQUIRRELVM vm);
void export_player(HSQUIRRELVM vm);
void export_scenario(HSQUIRRELVM vm);
void export_settings(HSQUIRRELVM vm);
void export_schedule(HSQUIRRELVM vm);
void export_simple(HSQUIRRELVM vm);
void export_tiles(HSQUIRRELVM vm);
void export_world(HSQUIRRELVM vm);

void export_global_constants(HSQUIRRELVM vm);


#endif
