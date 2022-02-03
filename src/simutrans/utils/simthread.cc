/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simthread.h"

#ifdef MULTI_THREAD
// do not try to compile this file for non-multithreaded builds

#ifndef _USE_POSIX_BARRIERS
// implement barriers using other pthread primitives
#include <errno.h>


int simthread_barrier_init(simthread_barrier_t *barrier, const simthread_barrierattr_t */*attr*/, unsigned int count)
{
	if(  count == 0  ) {
		errno = EINVAL;
		return -1;
	}
	if(  pthread_mutex_init( &barrier->mutex, 0 ) < 0  ) {
		return -1;
	}
	if(  pthread_cond_init( &barrier->cond, 0 ) < 0  ) {
		pthread_mutex_destroy( &barrier->mutex );
		return -1;
	}
	barrier->tripCount = count;
	barrier->count = 0;

	return 0;
}


int simthread_barrier_destroy(simthread_barrier_t *barrier)
{
	pthread_cond_destroy( &barrier->cond );
	pthread_mutex_destroy( &barrier->mutex );
	return 0;
}


int simthread_barrier_wait(simthread_barrier_t *barrier)
{
	pthread_mutex_lock( &barrier->mutex );
	barrier->count++;
	if(  barrier->count >= barrier->tripCount  ) {
		barrier->count = 0;
		pthread_cond_broadcast( &barrier->cond );
		pthread_mutex_unlock( &barrier->mutex );
		return 1;
	}
	else {
		pthread_cond_wait( &barrier->cond, &barrier->mutex );
		pthread_mutex_unlock( &barrier->mutex );
		return 0;
	}
}

#endif

#ifndef _SIMTHREAD_R_MUTEX_I
#include "../simdebug.h"
// initialize a recursive mutex by calling pthread_mutex_init()
recursive_mutex_maker_t::recursive_mutex_maker_t(pthread_mutex_t &mutex)
{
	pthread_mutexattr_t attr;

	if (pthread_mutexattr_init(&attr) != 0 ||
	    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||
	    pthread_mutex_init(&mutex, &attr) != 0 ||
	    pthread_mutexattr_destroy(&attr) != 0) {
		// Can't call dbg->error(), because this constructor
		// may run before simu_main() calls init_logging().
		dbg->fatal("recursive_mutex_maker_t","Can't make a recursive mutex!");
	}
}
#endif

#endif
