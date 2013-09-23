#ifndef _API_CLASS_H_
#define _API_CLASS_H_

#include "api_param.h"

#include "../squirrel/squirrel.h"
#include "../squirrel/sq_extensions.h"  // sq_call_restricted

#include <string>

/** @file api_class.h  handling classes and instances */

namespace script_api {


	/**
	 * Creates squirrel class on the stack. Inherits from @p baseclass.
	 * Has to be complemented by call to end_class.
	 * @param classname name of squirrel class
	 * @param baseclass name of base class (or NULL)
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger create_class(HSQUIRRELVM vm, const char* classname, const char* baseclass = NULL);

	/**
	 * Creates squirrel class on the stack. Inherits from @p baseclass.
	 * Has to be complemented by call to end_class.
	 * Assigns the tag from param<C>::tag() to the class.
	 * @tparam C assigns tag from C::tag() to the new class
	 * @return SQ_OK or SQ_ERROR
	 */
	template<class C>
	SQInteger create_class(HSQUIRRELVM vm, const char* classname, const char* baseclass = NULL)
	{
		SQInteger res = create_class(vm, classname, baseclass);
		if (SQ_SUCCEEDED(res)) {
			sq_settypetag(vm, -1, param<C>::tag());
		}
		return res;
	}

	/**
	 * Pushes class on stack.
	 * Has to be complemented by call to end_class.
	 * @param classname name of squirrel class, must exist prior to calling this function
	 * @param baseclasses dummy string containing base classes - to create nice doxygen output
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger begin_class(HSQUIRRELVM vm, const char* classname, const char* baseclasses = NULL);

	/**
	 * Pops class from stack.
	 */
	void end_class(HSQUIRRELVM vm);

	/**
	 * Pushes the squirrel class onto the stack.
	 * Has to be complemented by call to end_class.
	 * @param classname name of squirrel class
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger push_class(HSQUIRRELVM vm, const char* classname);


	/**
	 * Prepares call to class to instantiate.
	 * Pushes class and dummy object.
	 * @param classname name of squirrel class
	 * @return SQ_OK or SQ_ERROR
	 */
	SQInteger prepare_constructor(HSQUIRRELVM vm, const char* classname);

	/**
	 * Function to create & push instances of squirrel classes.
	 * @param classname name of squirrel class
	 */
	inline SQInteger push_instance(HSQUIRRELVM vm, const char* classname)
	{
		if (!SQ_SUCCEEDED(prepare_constructor(vm, classname)) ) {
			return -1;
		}
		bool ok = SQ_SUCCEEDED(sq_call_restricted(vm, 1, true, false));
		sq_remove(vm, ok ? -2 : -1); // remove closure
		return ok ? 1 : -1;
	}

	template<class A1>
	SQInteger push_instance(HSQUIRRELVM vm, const char* classname, const A1 & a1)
	{
		if (!SQ_SUCCEEDED(prepare_constructor(vm, classname)) ) {
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
		if (!SQ_SUCCEEDED(prepare_constructor(vm, classname)) ) {
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
		if (!SQ_SUCCEEDED(prepare_constructor(vm, classname)) ) {
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
		if (!SQ_SUCCEEDED(prepare_constructor(vm, classname)) ) {
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
