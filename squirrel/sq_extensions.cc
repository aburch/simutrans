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


SQRESULT sq_raise_error(HSQUIRRELVM vm, const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	vm->Raise_Error_vl(s, vl);
	va_end(vl);
	return SQ_ERROR;
}

SQRESULT sq_call_restricted(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool throw_if_no_ops, SQInteger ops)
{
	if(v->_ops_remaining < 4*ops  &&  sq_getvmstate(v)==SQ_VMSTATE_IDLE) {
		v->_ops_remaining += ops;
	}

	bool save_throw_if_no_ops = v->_throw_if_no_ops;
	v->_throw_if_no_ops = throw_if_no_ops;

	SQRESULT ret = sq_call(v, params, retval, true /*raise_error*/);
	v->_throw_if_no_ops = save_throw_if_no_ops;

	return ret;
}

SQRESULT sq_resumevm(HSQUIRRELVM v, SQBool retval, SQInteger ops)
{
	if(v->_ops_remaining < 4*ops) {
		v->_ops_remaining += ops;
	}

	bool save_throw_if_no_ops = v->_throw_if_no_ops;
	v->_throw_if_no_ops = false;

	SQRESULT ret = sq_wakeupvm(v, false, retval, true /*raise_error*/, false);
	v->_throw_if_no_ops = save_throw_if_no_ops;

	return ret;
}
