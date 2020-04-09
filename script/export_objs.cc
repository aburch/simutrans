/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "export_objs.h"

#include <string.h>

#include "api_function.h"
#include "api_class.h"

#include "api/api.h"


void register_export_function(HSQUIRRELVM vm, bool scenario)
{
	script_api::start_squirrel_type_logging(scenario ? "scenario" : "ai");

	sq_pushroottable(vm);

	export_city(vm);
	export_control(vm);
	export_convoy(vm);
	export_factory(vm);
	export_goods_desc(vm);
	export_gui(vm, scenario);
	export_halt(vm);
	export_line(vm);
	export_map_objects(vm);
	export_player(vm, scenario);
	if (scenario) {
		export_scenario(vm);
	}
	export_schedule(vm);
	export_settings(vm);
	export_simple(vm);
	export_string_methods(vm);
	export_tiles(vm);
	export_world(vm, scenario);
	export_pathfinding(vm);

	export_commands(vm);

	sq_pop(vm, 1); // root table

	script_api::end_squirrel_type_logging();
}
