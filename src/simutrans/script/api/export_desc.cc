/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "export_desc.h"

#include "../script.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../tpl/vector_tpl.h"
#include "../../utils/plainstring.h"

using namespace script_api;


static vector_tpl<GETDESCFUNC> registered_desc_functions;


SQInteger get_desc_pointer(HSQUIRRELVM vm)
{
	void *tag = NULL;
	// get type tag of class
	sq_gettypetag(vm, 1, &tag);
	plainstring name = param<plainstring>::get(vm, 2);
	if (tag  &&  registered_desc_functions.is_contained((GETDESCFUNC)tag) ) {
		// call method
		const void *desc = ((GETDESCFUNC)tag)(name.c_str());
		// set userpointer of class instance
		sq_setinstanceup(vm, 1, const_cast<void*>(desc) );
		return 0;
	}
	return -1;
}


void begin_desc_class(HSQUIRRELVM vm, const char* name, const char* base, GETDESCFUNC func)
{
	SQInteger res = create_class(vm, name, base);
	assert( SQ_SUCCEEDED(res) ); (void)res;
	// store method to retrieve desc in typetag pointer
	sq_settypetag(vm, -1, (void*)func);
	registered_desc_functions.append_unique( func );
	// register constructor
	register_function(vm, get_desc_pointer, "constructor", 2, "xs");
	// now functions can be registered
}
