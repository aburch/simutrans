/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_EXPORT_DESC_H
#define SCRIPT_API_EXPORT_DESC_H


#include "../../../squirrel/squirrel.h"

/** @file export_desc.h Helper functions to export desc-classes */

/**
 * Creates class @p name in top of the stack.
 *
 * Defines constructor. Call has to be complemented by call to end_class.
 * @param name name of new class
 * @param parent name of base class (or NULL)
 * @param func function pointer to a function that retrieves pointer-to-desc by name
 */
void begin_desc_class(HSQUIRRELVM vm, const char* name, const char* parent, const void* (*func)(const char*));

/// Function signature to retrieve desc-pointer from name
typedef const void* (*GETDESCFUNC)(const char*);

#endif
