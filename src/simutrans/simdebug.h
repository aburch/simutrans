/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMDEBUG_H
#define SIMDEBUG_H


// do not check assertions
//#define NDEBUG 1


// check assertions
//#undef NDEBUG
//#define NDEBUG


#include <assert.h>

#include "utils/log.h"

/**
 * Logger instance, this is a globally exported object.
 */
extern log_t *dbg;


/**
 * Inits logging facility.
 */
void init_logging(const char *logname, bool force_flush, bool log_debug, const char *greeting, const char* syslogtag );


#if !defined(MSG_LEVEL)
#if defined(DEBUG)
#define MSG_LEVEL 4
#else
#define MSG_LEVEL 0
#endif
#endif


#if MSG_LEVEL >= 4
#define DBG_DEBUG4 dbg->debug
#define DBG_MESSAGE dbg->message
#define DBG_DEBUG dbg->message

#elif MSG_LEVEL >= 3
#define DBG_DEBUG4(i,...) ;
#define DBG_MESSAGE dbg->message
#define DBG_DEBUG dbg->message

#elif MSG_LEVEL >= 1
#define DBG_DEBUG4(i,...) ;
#define DBG_MESSAGE(i,...) ;
#define DBG_DEBUG dbg->message

#else
// nothing to debug -> then ignore
#define DBG_DEBUG4(i,...) ;
#define DBG_MESSAGE(i,...) ;
#define DBG_DEBUG(i,...) ;
#endif


#endif
