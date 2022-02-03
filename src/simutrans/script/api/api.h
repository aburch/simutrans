/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_API_H
#define SCRIPT_API_API_H


/** @file api.h declarations of export functions. */

#include "../../../squirrel/squirrel.h"
#include "api_command.h"

void export_city(HSQUIRRELVM vm);
void export_control(HSQUIRRELVM vm);
void export_convoy(HSQUIRRELVM vm);
void export_factory(HSQUIRRELVM vm);
void export_goods_desc(HSQUIRRELVM vm);
void export_gui(HSQUIRRELVM vm, bool scenario);
void export_halt(HSQUIRRELVM vm);
void export_line(HSQUIRRELVM vm);
void export_map_objects(HSQUIRRELVM vm);
void export_player(HSQUIRRELVM vm, bool scenario);
void export_scenario(HSQUIRRELVM vm);
void export_settings(HSQUIRRELVM vm);
void export_schedule(HSQUIRRELVM vm);
void export_simple(HSQUIRRELVM vm);
void export_string_methods(HSQUIRRELVM vm);  // api_scenario.cc
void export_tiles(HSQUIRRELVM vm);
void export_world(HSQUIRRELVM vm, bool scenario);

void export_pathfinding(HSQUIRRELVM vm);

void export_global_constants(HSQUIRRELVM vm);


#endif
