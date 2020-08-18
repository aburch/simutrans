#include "simthread.h"

#if defined(_USE_POSIX_BARRIERS)  ||  !defined(MULTI_THREAD)
// use native pthread barriers
// (and do not try to compile this file for non-multithread builds)
#else
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
