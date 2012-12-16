#ifndef BEDROCK_WIN_PTHREAD_H
#define BEDROCK_WIN_PTHREAD_H

#include "util/util.h"

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef HANDLE pthread_cond_t;
typedef int pthread_attr_t;
typedef void pthread_mutexattr_t;
typedef void pthread_condattr_t;

#define PTHREAD_CREATE_JOINABLE 0

extern int pthread_attr_init(pthread_attr_t *);
extern int pthread_attr_setdetachstate(pthread_attr_t *, int);
extern int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
extern int pthread_join(pthread_t, void **);
extern void pthread_exit(int);

extern int pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
extern int pthread_mutex_destroy(pthread_mutex_t *);
extern int pthread_mutex_lock(pthread_mutex_t *);
extern int pthread_mutex_trylock(pthread_mutex_t *);
extern int pthread_mutex_unlock(pthread_mutex_t *);

extern int pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
extern int pthread_cond_destroy(pthread_cond_t *);
extern int pthread_cond_signal(pthread_cond_t *);
extern int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);

#endif