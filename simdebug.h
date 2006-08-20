/*
 * simdebug.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simdebug_h
#define simdebug_h

// do not check assertions
//#define NDEBUG 1


// check assertions
#undef NDEBUG
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
void init_logging(const char *logname, bool force_flush, bool log_debug);


/**
 * Console output, realised as a macro
 * @author Hj. Malthaner
 */

#define print printf


#endif

#endif
