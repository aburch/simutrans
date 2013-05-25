#include "api.h"

/** @file api_goods_desc.cc exports goods descriptors - ware_besch_t. */

#include "export_besch.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../besch/ware_besch.h"
#include "../../bauer/warenbauer.h"

using namespace script_api;


SQInteger get_next_ware_besch(HSQUIRRELVM vm)
{
	return generic_get_next(vm, warenbauer_t::get_waren_anzahl());
}


SQInteger get_ware_besch_index(HSQUIRRELVM vm)
{
	uint32 index = param<uint32>::get(vm, -1);

	const char* name = "None"; // fall-back
	if (index < warenbauer_t::get_waren_anzahl()) {
		name = warenbauer_t::get_info(index)->get_name();
	}
	return push_instance(vm, "good_desc_x",  name);
}


void export_goods_desc(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through the list of all good types.
	 *
	 * Usage:
	 * @code
	 * local list = good_desc_list_x()
	 * foreach(good_desc in list) {
	 *     ... // good_desc is an instance of the good_desc_x class
	 * }
	 * @endcode
	 */
	create_class(vm, "good_desc_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, get_next_ware_besch,  "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, get_ware_besch_index, "_get",    2, "xi");

	end_class(vm);


	/**
	 * Descriptor of goods and freight types.
	 */
	const ware_besch_t* (*F)(const char*) = &warenbauer_t::get_info;
	begin_besch_class(vm, "good_desc_x", ( const void* (*)(const char*))F);
	/**
	 * Constructor.
	 * @param name raw name of the freight type.
	 * @typemask (string)
	 */
	// actually created in above call to begin_besch_class
	// register_function( .., "constructor", .. )
	/**
	 * @return freight category. 0=Passengers, 1=Mail, 2=None, >=3 anything else
	 */
	register_method(vm, &ware_besch_t::get_catg_index, "get_catg_index");

	/**
	 * @return raw (untranslated) name
	 * @typemask string()
	 */
	register_method(vm, &ware_besch_t::get_name, "get_name");

	end_class(vm);
}
