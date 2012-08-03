#include "api_class.h"

/**
 * pushes class
 */
SQInteger script_api::push_class(HSQUIRRELVM vm, const char* classname)
{
	sq_pushroottable(vm);
	sq_pushstring(vm, classname, -1);
	if(!SQ_SUCCEEDED(sq_get(vm, -2))) {
		sq_pop(vm, 1);
		sq_raise_error(vm, "class %s not found");
		return SQ_ERROR;
	}
	sq_remove(vm, -2); // remove root table
	return SQ_OK;
}

/**
 * pushes constructor of class: closure (the constructor) and environment (the class)
 */
SQInteger script_api::push_constructor(HSQUIRRELVM vm, const char* classname)
{
	if (!SQ_SUCCEEDED(push_class(vm, classname))) {
		return SQ_ERROR;
	}
	sq_pushstring(vm, "constructor", -1);
	if(!SQ_SUCCEEDED(sq_get(vm, -2))) {
		sq_pop(vm, 1);
		sq_raise_error(vm, "no constructor of class %s not found");
		return SQ_ERROR;
	}
	return SQ_OK;
}
