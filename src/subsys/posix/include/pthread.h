#ifndef __PTHREAD_H
#define __PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

typedef int pthread_t;
typedef int pthread_attr_t;

int _EXFUN(pthread_create, (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg));
int _EXFUN(pthread_join, (pthread_t thread, void **value_ptr));

#ifdef __cplusplus
}
#endif

#endif
