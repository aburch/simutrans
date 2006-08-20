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
