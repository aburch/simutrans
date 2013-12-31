#ifndef _EXPORT_BESCH_H__
#define _EXPORT_BESCH_H__

#include "../../squirrel/squirrel.h"

/** @file export_besch.h Helper functions to export besch-classes */

/**
 * Creates class @p name in top of the stack.
 *
 * Defines constructor. Call has to be complemented by call to end_class.
 * @param name name of new class
 * @param base name of base class (or NULL)
 * @param func function pointer to a function that retrieves pointer-to-besch by name
 */
void begin_besch_class(HSQUIRRELVM vm, const char* name, const char* parent, const void* (*func)(const char*));

/// Function signature to retrieve besch-pointer from name
typedef const void* (*GETBESCHFUNC)(const char*);

#endif
