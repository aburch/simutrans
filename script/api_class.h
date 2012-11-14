#ifndef _API_CLASS_H_
#define _API_CLASS_H_

#include "api_param.h"

#include "../squirrel/squirrel.h"
#include "../squirrel/sq_extensions.h"  // sq_call_restricted

#include <string>

/** @file api_class.h  handling classes and instances */

namespace script_api {

	/**
	 * Pushes the squirrel class onto the stack.
	 * @param classname name of squirrel class
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger push_class(HSQUIRRELVM vm, const char* classname);

	/**
	 * Pushes the constructor of squirrel class onto the stack.
	 * That is: closure (the constructor) and environment (the class)
	 * @param classname name of squirrel class
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger push_constructor(HSQUIRRELVM vm, const char* classname);

	/**
	 * Function to create & push instances of squirrel classes.
	 * @param classname name of squirrel class
	 */
	inline SQInteger push_instance(HSQUIRRELVM vm, const char* classname)
	{
		if (!SQ_SUCCEEDED(push_constructor(vm, classname)) ) {
			return -1;
		}
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 1, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

	template<class A1>
	SQInteger push_instance(HSQUIRRELVM vm, const char* classname, const A1 & a1)
	{
		if (!SQ_SUCCEEDED(push_constructor(vm, classname)) ) {
			return -1;
		}
		param<A1>::push(vm, a1);
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 2, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

	template<class A1, class A2>
	SQInteger push_instance(HSQUIRRELVM vm, const char* classname, const A1 & a1, const A2 & a2)
	{
		if (!SQ_SUCCEEDED(push_constructor(vm, classname)) ) {
			return -1;
		}
		param<A1>::push(vm, a1);
		param<A2>::push(vm, a2);
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 3, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

	template<class A1, class A2, class A3>
	SQInteger push_instance(HSQUIRRELVM vm, const char* classname, const A1 & a1, const A2 & a2, const A3 & a3)
	{
		if (!SQ_SUCCEEDED(push_constructor(vm, classname)) ) {
			return -1;
		}
		param<A1>::push(vm, a1);
		param<A2>::push(vm, a2);
		param<A3>::push(vm, a3);
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 4, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

	template<class A1, class A2, class A3, class A4>
	SQInteger push_instance(HSQUIRRELVM vm, const char* classname, const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4)
	{
		if (!SQ_SUCCEEDED(push_constructor(vm, classname)) ) {
			return -1;
		}
		param<A1>::push(vm, a1);
		param<A2>::push(vm, a2);
		param<A3>::push(vm, a3);
		param<A4>::push(vm, a4);
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 5, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

}; // end of namespace
#endif
