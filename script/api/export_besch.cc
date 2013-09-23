#include "export_besch.h"

#include "../script.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../tpl/vector_tpl.h"
#include "../../utils/plainstring.h"

using namespace script_api;


static vector_tpl<GETBESCHFUNC> registered_besch_functions;


SQInteger get_besch_pointer(HSQUIRRELVM vm)
{
	void *tag = NULL;
	// get type tag of class
	sq_gettypetag(vm, 1, &tag);
	plainstring name = param<plainstring>::get(vm, 2);
	if (tag  &&  registered_besch_functions.is_contained((GETBESCHFUNC)tag) ) {
		// call method
		const void *besch = ((GETBESCHFUNC)tag)(name.c_str());
		// set userpointer of class instance
		sq_setinstanceup(vm, 1, const_cast<void*>(besch) );
		return 0;
	}
	return -1;
}


void begin_besch_class(HSQUIRRELVM vm, const char* name, const char* base, GETBESCHFUNC func)
{
	SQInteger res = create_class(vm, name, base);
	assert( SQ_SUCCEEDED(res) );
	// store method to retrieve besch in typetag pointer
	sq_settypetag(vm, -1, (void*)func);
	registered_besch_functions.append_unique( func );
	// register constructor
	register_function(vm, get_besch_pointer, "constructor", 2, "xs");
	// now functions can be registered
}
