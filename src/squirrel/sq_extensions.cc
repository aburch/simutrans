#include "sq_extensions.h"

#include "squirrel/sqpcheader.h" // for declarations...
#include "squirrel/sqvm.h"       // for Raise_Error_vl
#include <stdarg.h>
#include "../simutrans/tpl/ptrhashtable_tpl.h"

// store data associate to vm's here
struct my_vm_info_t {
	const char* suspend_blocker; /// if not null then no suspendable functions should be called
};

static ptrhashtable_tpl<HSQUIRRELVM, my_vm_info_t> vm_info;

void register_vm(HSQUIRRELVM v)
{
	vm_info.put(v);
}

void unregister_vm(HSQUIRRELVM v)
{
	vm_info.remove(v);
}

void* get_instanceup(HSQUIRRELVM vm, SQInteger index, void* tag, const char* type)
{
	void* ptr = NULL;
	if(SQ_SUCCEEDED(sq_getinstanceup(vm, index, &ptr, tag))) {
		return ptr;
	}
	else {
		sq_raise_error(vm, "Expected instance of class %s.", type ? type : "<unknown>");
		return NULL;
	}
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

// see sq_wakeupvm
void sq_setwakeupretvalue(HSQUIRRELVM v)
{
	if(!v->_suspended) {
		return;
	}
	SQInteger target = v->_suspended_target;
	if(target != -1) {
		v->GetAt(v->_stackbase+v->_suspended_target) = v->GetUp(-1); //retval
	}
	v->Pop();
	// make target invalid
	v->_suspended_target = -1;
}

bool sq_canresumevm(HSQUIRRELVM v)
{
	// true if not suspended or not waiting for any return value
	return (!v->_suspended  ||  v->_suspended_target==-1);
}

SQRESULT sq_get_ops_total(HSQUIRRELVM v)
{
	sq_pushinteger(v, v->_ops_total);
	return 1;
}

SQRESULT sq_get_ops_remaing(HSQUIRRELVM v)
{
	sq_pushinteger(v, v->_ops_remaining);
	return 1;
}

void sq_block_suspend(HSQUIRRELVM v, const char* f)
{
	if (my_vm_info_t* i = vm_info.access(v)) {
		i->suspend_blocker = f;
	}
}

const char* sq_get_suspend_blocker(HSQUIRRELVM v)
{
	return vm_info.get(v).suspend_blocker;
}
