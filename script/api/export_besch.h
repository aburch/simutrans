#ifndef _EXPORT_BESCH_H__
#define _EXPORT_BESCH_H__

#include "../../squirrel/squirrel.h"

/** @file export_besch.c Helper functions to export besch-classes */

/**
 * Prepares creation of class @p name in the table on top of the stack.
 *
 * Defines constructor. Class creation will be finished by sub-sequent call to end_besch_class.
 * @param name name of new class
 * @param func function pointer to a function that retrieves pointer-to-besch by name
 */
void begin_besch_class(HSQUIRRELVM vm, const char* name, const void* (*func)(const char*));

/**
 * Ends class definition.
 * Has to be called after begin_besch_class.
 */
void end_besch_class(HSQUIRRELVM vm);

#endif
