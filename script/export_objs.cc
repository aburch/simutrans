#include "export_objs.h"

#include <string.h>

#include "api_function.h"
#include "api_class.h"

#include "api/api.h"


void register_export_function(HSQUIRRELVM vm)
{
	script_api::start_squirrel_type_logging();

	sq_pushroottable(vm);

	export_city(vm);
	export_convoy(vm);
	export_factory(vm);
	export_goods_desc(vm);
	export_gui(vm);
	export_halt(vm);
	export_map_objects(vm);
	export_player(vm);
	export_scenario(vm);
	export_schedule(vm);
	export_settings(vm);
	export_simple(vm);
	export_tiles(vm);
	export_world(vm);

	sq_pop(vm, 1); // root table

	script_api::end_squirrel_type_logging();
}
