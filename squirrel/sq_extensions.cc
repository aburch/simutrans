#include "sq_extensions.h"

#include "squirrel/sqpcheader.h" // for declarations...
#include "squirrel/sqvm.h"       // for Raise_Error_vl
#include <stdarg.h>


void* get_instanceup(HSQUIRRELVM vm, SQInteger index, void* tag, const char* type)
{
	void* ptr = NULL;
	if(!SQ_SUCCEEDED(sq_getinstanceup(vm, index, &ptr, tag))) {
		sq_raise_error(vm, "Expected instance of class %s.", type ? type : "<unknown>");
	}
	return ptr;
}


void sq_raise_error(HSQUIRRELVM vm, const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	vm->Raise_Error_vl(s, vl);
	va_end(vl);

	vm->_error_handler_called = false;
	vm->CallErrorHandler(vm->_lasterror);
}

SQRESULT sq_call_restricted(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror, SQInteger ops)
{
	if(v->_ops_remaining < 4*ops) {
		v->_ops_remaining += ops;
	}
	bool n = v->_throw_if_no_ops;
	v->_throw_if_no_ops = true;

	SQRESULT ret = sq_call(v, params, retval, raiseerror);
	v->_throw_if_no_ops = n;
	return ret;
}
