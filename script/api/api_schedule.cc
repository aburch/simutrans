#include "api.h"

/** @file api_schedule.cc exports schedule structure and related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/fahrplan.h"
#include "../../simhalt.h"

using namespace script_api;

#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);

//static halthandle_t get_halt(const karte_t *welt, const koord3d pos, const spieler_t *sp );

halthandle_t get_halt_from_koord3d(koord3d pos, const spieler_t *sp )
{
	if (sp == NULL) {
		return halthandle_t();
	}
	return haltestelle_t::get_halt(welt, pos, sp);
}


void export_schedule(HSQUIRRELVM vm)
{
	/**
	 * Schedule entries
	 */
	begin_class("schedule_entry_x", "coord3d");

#ifdef SQAPI_DOC // document members
	/**
	 * Minimum loading percentage.
	 */
	integer load;
	/**
	 * Waiting time setting.
	 */
	integer wait;
#endif

	/**
	 * Returns halt at this entry position.
	 * @param pl player that wants to use halt here
	 * @return halt instance
	 * @typemask halt_x(player_x)
	 */
	register_method(vm, &get_halt_from_koord3d, "get_halt", true);

	end_class();

	/**
	 * Class holding the schedule
	 */
	begin_class("schedule_x", "");

#ifdef SQAPI_DOC // document members
	/**
	 * The list of schedule targets.
	 */
	array< schedule_entry_x > entries;

	/**
	 * Waytype of schedule.
	 */
	way_types waytype;
#endif

	end_class();
}
