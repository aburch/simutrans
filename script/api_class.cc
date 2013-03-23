#include "api_class.h"


/**
 * creates class, leaves it at the top of the stack
 */
SQInteger script_api::create_class(HSQUIRRELVM vm, const char* classname, const char* baseclass)
{
	sq_pushroottable(vm);
	if (baseclass) {
		sq_pushstring(vm, baseclass, -1);
		if(!SQ_SUCCEEDED(sq_get(vm, -2))) {
			sq_pop(vm, 1);
			sq_raise_error(vm, "base class %s to derive %s from it not found", baseclass, classname);
			return SQ_ERROR;
		}
	}
	sq_newclass(vm, baseclass!=NULL);
	sq_pushstring(vm, classname, -1);
	sq_push(vm, -2);
	// stack: root, class, classname, class
	sq_newslot(vm, -4, false);
	sq_remove(vm, -2); // remove root table
	return SQ_OK;
}


SQInteger script_api::begin_class(HSQUIRRELVM vm, const char* classname, const char* /* baseclasses - dummy */)
{
	return push_class(vm, classname);
}


void script_api::end_class(HSQUIRRELVM vm)
{
	sq_pop(vm,1);
}


/**
 * pushes class
 */
SQInteger script_api::push_class(HSQUIRRELVM vm, const char* classname)
{
	sq_pushroottable(vm);
	sq_pushstring(vm, classname, -1);
	if(!SQ_SUCCEEDED(sq_get(vm, -2))) {
		sq_pop(vm, 1);
		sq_raise_error(vm, "class %s not found", classname);
		return SQ_ERROR;
	}
	sq_remove(vm, -2); // remove root table
	return SQ_OK;
}

/**
 * pushes constructor of class: closure (the constructor) and environment (the class)
 */
SQInteger script_api::prepare_constructor(HSQUIRRELVM vm, const char* classname)
{
	if (!SQ_SUCCEEDED(push_class(vm, classname))) {
		return SQ_ERROR;
	}
	sq_pushnull(vm);
	return SQ_OK;
}
