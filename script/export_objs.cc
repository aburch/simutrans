#include "export_objs.h"

#include "../simworld.h"
#include "../dataobj/scenario.h"
#include <string.h>

#include "api_function.h"
#include "api_class.h"
#include "export_besch.h"

#include "api/api.h"

static karte_t *welt;


SQInteger world_get_time(HSQUIRRELVM vm)
{
	sq_newtableex(vm, 2);
	uint32 yearmonth = welt->get_current_month();
	script_api::param<uint32>::create_slot(vm, "year",  yearmonth/12);
	script_api::param<uint32>::create_slot(vm, "month", yearmonth%12);
	return 1;
}

void register_export_function(HSQUIRRELVM vm, karte_t *welt_)
{
	welt = welt_;
	script_api::welt = welt_;

	script_api::start_squirrel_type_logging();

	sq_pushroottable(vm);

	script_api::register_function(vm, world_get_time, "get_time", 1, ".");
	sq_pushstring(vm, "gui", -1);
	if (SQ_SUCCEEDED(sq_get(vm, -2))) {
		script_api::register_method(vm, &scenario_t::open_info_win, "open_info_win");
	}

	sq_pop(vm, 1); // class

	export_city(vm);
	export_convoy(vm);
	export_factory(vm);
	export_halt(vm);
	export_player(vm);
	export_scenario(vm);
	export_settings(vm);
	export_tiles(vm);

	sq_pop(vm, 1); // root table

	register_besch_classes(vm);

	script_api::end_squirrel_type_logging();
}
