#include "api.h"

/** @file api_simple.cc exports simple data types. */

#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/ribi.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"

using namespace script_api;

#define STATIC


SQInteger script_api::push_ribi(HSQUIRRELVM vm, ribi_t::ribi ribi)
{
	welt->get_scenario()->ribi_w2sq(ribi);
	return param<uint8>::push(vm, ribi);
}


ribi_t::ribi script_api::get_ribi(HSQUIRRELVM vm, SQInteger index)
{
	ribi_t::ribi ribi = param<uint8>::get(vm, index) & ribi_t::alle;
	welt->get_scenario()->ribi_sq2w(ribi);
	return ribi;
}

#define map_ribi_ribi(f) \
	SQInteger export_## f(HSQUIRRELVM vm) \
	{ \
		ribi_t::ribi ribi = get_ribi(vm, -1); \
		ribi = ribi_t::f(ribi); \
		return push_ribi(vm, ribi); \
	}

#define map_ribi_any(f,type) \
	SQInteger export_## f(HSQUIRRELVM vm) \
	{ \
		ribi_t::ribi ribi = get_ribi(vm, -1); \
		type ret = ribi_t::f(ribi); \
		return script_api::param<type>::push(vm, ret); \
	}

// export the ribi functions
map_ribi_any(ist_einfach, bool);
map_ribi_any(is_twoway, bool);
map_ribi_any(is_threeway, bool);
map_ribi_any(ist_kurve, bool);
map_ribi_any(ist_gerade, bool);
map_ribi_ribi(doppelt);
map_ribi_ribi(rueckwaerts);

void export_simple(HSQUIRRELVM vm)
{
	/**
	 * Class holding static methods to work with directions.
	 * Directions are just bit-encoded integers.
	 */
	begin_class(vm, "dir");
	/**
	 * @param d direction to test
	 * @return whether direction is single direction, i.e. just one of n/s/e/w
	 * @typemask bool(dir)
	 */
	STATIC register_function(vm, &export_ist_einfach,  "is_single",  2, "yi");
	/**
	 * @param d direction to test
	 * @return whether direction is double direction, e.g. n+e, n+s.
	 * @typemask bool(dir)
	 */
	STATIC register_function(vm, &export_is_twoway,  "is_twoway",  2, "yi");
	/**
	 * @param d direction to test
	 * @return whether direction is triple direction, e.g. n+s+e.
	 * @typemask bool(dir)
	 */
	STATIC register_function(vm, &export_is_threeway,  "is_threeway",  2, "yi");
	/**
	 * @param d direction to test
	 * @return whether direction is curve, e.g. n+e, s+w.
	 * @typemask bool(dir)
	 */
	STATIC register_function(vm, &export_ist_kurve,  "is_curve",  2, "yi");
	/**
	 * @param d direction to test
	 * @return whether direction is straight and has no curves in it, e.g. n+w, w.
	 * @typemask bool(dir)
	 */
	STATIC register_function(vm, &export_ist_gerade,  "is_straight",  2, "yi");
	/**
	 * @param d direction
	 * @return complements direction to complete straight, i.e. w -> w+e, but n+w -> 0.
	 * @typemask dir(dir)
	 */
	STATIC register_function(vm, &export_doppelt,  "double",  2, "yi");
	/**
	 * @param d direction to test
	 * @return backward direction, e.g. w -> e, n+w -> s+e, n+w+s -> e.
	 * @typemask dir(dir)
	 */
	STATIC register_function(vm, &export_rueckwaerts,  "backward",  2, "yi");

	end_class(vm);
}
