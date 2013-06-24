#ifndef BEDROCK_SPIN_LOCK
#define BEDROCK_SPIN_LOCK

// This is OS X specific.
#if !defined(__MACH__) || !defined(__APPLE__)
# error "This platform is not OS X, do not include this file."
#endif

#include <errno.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#include <libkern/OSAtomic.h>
#include "util.h"

// OS X does not have CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0

// OS X also does not have spinlocks
// and because the core uses them, they must
// be redefined with this assembly.

typedef OSSpinLock pthread_spinlock_t;

#ifndef PTHREAD_PROCESS_SHARED
# define PTHREAD_PROCESS_SHARED 1
#endif

#ifndef PTHREAD_PROCESS_PRIVATE
# define PTHREAD_PROCESS_PRIVATE 2
#endif

static inline int pthread_spin_init(pthread_spinlock_t *lock, int bedrock_attribute_unused pshared) {
	*lock = OS_SPINLOCK_INIT;
	return 0;
}

static inline int pthread_spin_destroy(pthread_spinlock_t bedrock_attribute_unused *lock) {
	return 0;
}

static inline int pthread_spin_lock(pthread_spinlock_t *lock) {
	OSSpinLockLock(lock);
	return 0;
}

static inline int pthread_spin_trylock(pthread_spinlock_t *lock) {
	return (int)OSSpinLockTry(lock);
}

static inline int pthread_spin_unlock(pthread_spinlock_t *lock) {
	OSSpinLockUnlock(lock);
	return 0;
}


// OS X compat for clock_gettime
static inline int clock_gettime(int bedrock_attribute_unused foo, struct timespec *ts) 
{ 
    struct timeval tv; 
 
    gettimeofday(&tv, NULL); 
    ts->tv_sec = tv.tv_sec; 
    ts->tv_nsec = tv.tv_usec * 1000; 
    return (0); 
}


#endif
