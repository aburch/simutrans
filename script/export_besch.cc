#include "export_besch.h"

#include "script.h"
#include "api_function.h"
#include "../utils/plainstring.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

using namespace script_api;

static vector_tpl<const void*> registered_besch_functions;

SQInteger get_besch_pointer(HSQUIRRELVM vm)
{
	void *tag = NULL;
	// get type tag of class
	sq_gettypetag(vm, 1, &tag);
	plainstring name = param<plainstring>::get(vm, 2);
	if (tag  &&  registered_besch_functions.is_contained(tag) ) {
		// call method
		const void *besch = ((const void* (*)(const char*))tag)(name.c_str());
		// set userpointer of class instance
		sq_setinstanceup(vm, 1, const_cast<void*>(besch) );
		return 0;
	}
	return -1;
}

void begin_besch_class(HSQUIRRELVM vm, const char* name, const void* (*func)(const char*))
{
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	// get extend_get class
	sq_pushstring(vm, "extend_get", -1);
	assert( SQ_SUCCEEDED(sq_get(vm, -3)) );
	// create class
	sq_newclass(vm, -1);
	// store method to retrieve besch in typetag pointer
	sq_settypetag(vm, -1, (void*)func);
	registered_besch_functions.append_unique( (void*)func );
	// register constructor
	register_function(vm, get_besch_pointer, "constructor", 2, "xs");
	// now functions can be registered
}

void end_besch_class(HSQUIRRELVM vm)
{
	sq_newslot(vm, -3, false);
	sq_pop(vm, 1); // root table
}


void register_besch_classes(HSQUIRRELVM vm)
{
	const ware_besch_t* (*F)(const char*) = warenbauer_t::get_info;
	begin_besch_class(vm, "good_desc_x", ( const void* (*)(const char*))F);
	register_method(vm, &ware_besch_t::get_catg_index, "get_catg_index");
	end_besch_class(vm);
}
