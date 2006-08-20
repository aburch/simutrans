#ifndef _SIM_DEBUG
#define _SIM_DEBUG

#include <assert.h>

#include "simdebug.h"
#include "simtools.h"
#include "utils/log.h"


/**
 * The log for all messages
 * @author Hj. Malthaner
 */
log_t *dbg = NULL;


int make_this_a_division_by_zero=0;

/**
 * Inits logging facility.
 * @author Hj. Malthaner
 */
void init_logging(const char *logname, bool force_flush, bool log_debug)
{
    dbg = new log_t(logname, force_flush, log_debug);
}


// the next two functions are for helping with debugging
#ifdef DEBUG

#ifdef _MSC_VER
// this make a pure virtual function call an division by zero error for MSVC
int __cdecl _purecall()
{
	int b = simrand(12);
	dbg->error("Pure virtual function call","-> DEBUGGER will be activated!");
	b /= make_this_a_division_by_zero;
	printf("%i",b);
}

#else
// this make a pure virtual function call an division by zero error for GCC
extern "C" void __cxa_pure_virtual()
{
	int b = simrand(12);
	dbg->error("Pure virtual function call","-> DEBUGGER will be activated!");
	b /= make_this_a_division_by_zero;
	printf("%i",b);
}
#endif

#endif

#endif
