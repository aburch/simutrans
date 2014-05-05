#ifndef simthread_h
#define simthread_h

#ifdef MULTI_THREAD

#if _XOPEN_SOURCE < 600
// enable barriers by this
#define _XOPEN_SOURCE 600
#endif

#ifndef _MSC_VER
#include <unistd.h>   // _POSIX_BARRIERS macro
// Visual C++ does not supply unistd.h
#endif

#include <pthread.h>

// Mac OS X defines this initializers without _NP.
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

// use our implementation if no posix barriers are available
#if defined(_POSIX_BARRIERS)  &&  (_POSIX_BARRIERS > 0)
#define _USE_POSIX_BARRIERS
#endif

#ifdef _USE_POSIX_BARRIERS
// redirect simthread functions to use supported pthread barriers
typedef pthread_barrierattr_t simthread_barrierattr_t;
typedef pthread_barrier_t simthread_barrier_t;
#define simthread_barrier_init pthread_barrier_init
#define simthread_barrier_destroy pthread_barrier_destroy
#define simthread_barrier_wait pthread_barrier_wait

#else
// add barrier support using other pthread primitives
typedef int simthread_barrierattr_t;
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int tripCount;
} simthread_barrier_t;

// needed because our signature doesn't match the one from simthread_barrier_init(3)
int simthread_barrier_init(simthread_barrier_t *barrier, const simthread_barrierattr_t *attr, unsigned int count);
int simthread_barrier_destroy(simthread_barrier_t *barrier);
int simthread_barrier_wait(simthread_barrier_t *barrier);

#endif

#endif

#endif
