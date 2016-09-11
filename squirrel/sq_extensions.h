#ifndef _SQ_EXTENSIONS_
#define _SQ_EXTENSIONS_

#include "squirrel.h"

/**
 * Extensions to the squirrel engine
 * for simutrans
 */

/**
 * Register vm to store internal information.
 */
void register_vm(HSQUIRRELVM v);
/**
 * Should be called if vm closes.
 */
void unregister_vm(HSQUIRRELVM v);

/**
 * Returns instance userpointer of instance, checks type tag.
 * @param index of instance in stack
 * @param tag type tag of class
 * @param type squirrel-side class name - used for error message
 * @returns userpointer or NULL in case of error (tag does not match)
 */
void* get_instanceup(HSQUIRRELVM vm, SQInteger index, void* tag, const char* type);

/**
 * Raises error.
 * @param s is format string analogue to printf-interface
 * @returns SQ_ERROR (-1)
 */
SQRESULT sq_raise_error(HSQUIRRELVM vm, const SQChar *s, ...);

/**
 * calls a function with limited number of opcodes.
 * returns and suspends vm if opcode limit is exceeded
 */
SQRESULT sq_call_restricted(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool throw_if_no_ops, SQInteger ops = 1000);

/**
 * if @r is not null, then no suspendable functions should be called in this vm
 */
void sq_block_suspend(HSQUIRRELVM v, const char* f);

/**
 * @returns the name of the suspend-blocking function
 */
const char* sq_get_suspend_blocker(HSQUIRRELVM v);

/**
 * resumes suspended vm.
 * returns and suspends (again) vm if opcode limit is exceeded.
 */
SQRESULT sq_resumevm(HSQUIRRELVM v, SQBool retval, SQInteger ops = 1000);

/**
 * pops return value from stack and sets it for the suspended script.
 */
void sq_setwakeupretvalue(HSQUIRRELVM v);

/**
 * checks whether vm can be resumed. in particular if script waits for return value on wakeup.
 */
bool sq_canresumevm(HSQUIRRELVM v);

/// @returns total amount of opcodes executed by vm
SQRESULT sq_get_ops_total(HSQUIRRELVM v);

/// @returns amount of remaining opcodes until vm will be suspended
SQRESULT sq_get_ops_remaing(HSQUIRRELVM v);

#endif
