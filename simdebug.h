/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simdebug_h
#define simdebug_h

// do not check assertions
//#define NDEBUG 1


// check assertions
//#undef NDEBUG
//#define NDEBUG


#include <assert.h>



#ifdef __cplusplus

#include "utils/log.h"

/**
 * Never access this directly!
 * @author Hj. Malthaner
 */
extern log_t *dbg;


/**
 * Inits logging facility.
 * @author Hj. Malthaner
 */
void init_logging(const char *logname, bool force_flush, bool log_debug, const char *greeting );

#ifndef DEBUG

// nothing to debug -> then ignore
#define DBG_MESSAGE(i,...) ;
#define DBG_DEBUG(i,...) ;
#define DBG_DEBUG4(i,...) ;

#else

//#define DBG_MESSAGE(i,...) dbg->message(i,__VA_ARGS__)
//#define DBG_DEBUG(i,...) dbg->message(i,__VA_ARGS__)
#define DBG_MESSAGE dbg->message
#define DBG_DEBUG dbg->message
#define DBG_DEBUG4 dbg->debug

#endif

#endif

#endif
