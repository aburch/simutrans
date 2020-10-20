/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef _SIM_DEBUG
#define _SIM_DEBUG

#include <assert.h>
#include <stdlib.h>

#include "simdebug.h"
#include "utils/log.h"


/**
 * The log for all messages
 */
log_t *dbg = NULL;


/**
 * Inits logging facility.
 */
void init_logging(const char* logname, bool force_flush, bool log_debug, const char* greeting, const char* syslogtag )
{
	dbg = new log_t( logname, force_flush, log_debug, true, greeting, syslogtag );
}


#if (MSG_LEVEL >= 1)

#ifdef _MSC_VER
int __cdecl _purecall()
#else
extern "C" void __cxa_pure_virtual()
#endif
{
	dbg->fatal("unknown", "pure virtual function call");
}

#endif

#endif
