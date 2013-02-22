#include "export_objs.h"

#include <string.h>

#include "api_function.h"
#include "api_class.h"

#include "api/api.h"

static karte_t *welt;

void register_export_function(HSQUIRRELVM vm, karte_t *welt_)
{
	welt = welt_;
	script_api::welt = welt_;

	script_api::start_squirrel_type_logging();

	sq_pushroottable(vm);

	export_city(vm);
	export_convoy(vm);
	export_factory(vm);
	export_goods_desc(vm);
	export_gui(vm);
	export_halt(vm);
	export_player(vm);
	export_scenario(vm);
	export_schedule(vm);
	export_settings(vm);
	export_tiles(vm);
	export_world(vm);

	sq_pop(vm, 1); // root table

	script_api::end_squirrel_type_logging();
}
