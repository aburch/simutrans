#include "sq_extensions.h"

#include "squirrel/sqpcheader.h" // for declarations...
#include "squirrel/sqvm.h"       // for Raise_Error_vl
#include <stdarg.h>

void sq_raise_error(HSQUIRRELVM vm, const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	vm->Raise_Error_vl(s, vl);
	va_end(vl);

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
