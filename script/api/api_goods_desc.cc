#include "api.h"

/** @file api_goods_desc.cc exports goods descriptors - ware_besch_t. */

#include "export_besch.h"
#include "../api_function.h"
#include "../../besch/ware_besch.h"
#include "../../bauer/warenbauer.h"

using namespace script_api;

#define begin_class begin_besch_class
#define end_class end_besch_class

void export_goods_desc(HSQUIRRELVM vm)
{
	/**
	 * Descriptor of goods and freight types.
	 */
	const ware_besch_t* (*F)(const char*) = &warenbauer_t::get_info;
	begin_class(vm, "good_desc_x", ( const void* (*)(const char*))F);
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
