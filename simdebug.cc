#ifndef _SIM_DEBUG
#define _SIM_DEBUG

#include <assert.h>
#include <stdlib.h>

#include "simdebug.h"
#include "utils/log.h"


/**
 * The log for all messages
 * @author Hj. Malthaner
 */
log_t *dbg = NULL;


/**
 * Inits logging facility.
 * @author Hj. Malthaner
 */
void init_logging(const char *logname, bool force_flush, bool log_debug)
{
    dbg = new log_t(logname, force_flush, log_debug);
}


#ifdef DEBUG

#ifdef _MSC_VER
int __cdecl _purecall()
{
	dbg->fatal("unknown","pure virtual function call");
	abort();
	return 0;	// to keep compiler happy
}
#else
extern "C" void __cxa_pure_virtual()
{
	dbg->fatal("unknown","pure virtual function call");
	abort();
}
#endif

#endif

#endif
