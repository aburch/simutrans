#ifndef _SQ_EXTENSIONS_
#define _SQ_EXTENSIONS_

#include "squirrel.h"

/**
 * Extensions to the squirrel engine
 * for simutrans
 */

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
 */
void sq_raise_error(HSQUIRRELVM vm, const SQChar *s, ...);

/**
 * call a function with limited number of opcodes
 * returns and suspends vm if opcode limit is exceeded
 */
SQRESULT sq_call_restricted(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror, SQInteger ops = 100000);

#endif
