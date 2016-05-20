/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>

#include <list>

#define LOOPS 1024 // 10000000

// Modified from http://www.alexonlinux.com/do-you-need-mutex-to-protect-int
// Uses mutexes or spinlocks

using namespace std;

list<int> the_list;

// #define USE_SPINLOCK

#ifdef USE_SPINLOCK
pthread_spinlock_t spinlock;
#else
pthread_mutex_t mutex;
#endif

void *consumer(void *ptr)
{
    int i;

    printf("Consumer TID %lu\n", (unsigned long) ptr);

    while (1)
    {
#ifdef USE_SPINLOCK
        pthread_spin_lock(&spinlock);
#else
        pthread_mutex_lock(&mutex);
#endif

        if (the_list.empty())
        {
#ifdef USE_SPINLOCK
            pthread_spin_unlock(&spinlock);
#else
            pthread_mutex_unlock(&mutex);
#endif
            break;
        }

        i = the_list.front();
        the_list.pop_front();

#ifdef USE_SPINLOCK
        pthread_spin_unlock(&spinlock);
#else
        pthread_mutex_unlock(&mutex);
#endif
    }

    return NULL;
}

int main()
{
    int i;
    pthread_t thr1, thr2;
    struct timeval tv1, tv2;

#ifdef USE_SPINLOCK
    pthread_spin_init(&spinlock, 0);
#else
    pthread_mutex_init(&mutex, NULL);
#endif

    // syslog used to split up debug logs.
    syslog(LOG_INFO, "TEST 0");
    printf("Locking with deadlock\n");
    pthread_mutex_t deadlock_mutex;
    errno = 0;
    pthread_mutex_init(&deadlock_mutex, 0);
    i = pthread_mutex_lock(&deadlock_mutex);
    printf("First lock: %d (%s)\n", i, strerror(errno));
    i = pthread_mutex_lock(&deadlock_mutex);
    if (errno != EDEADLK) printf("Didn't get EDEADLK!\n");
    printf("Second lock: %d (%s)\n", i, strerror(errno));
    pthread_mutex_unlock(&deadlock_mutex);

    syslog(LOG_INFO, "TEST 1");

    printf("Locking without any contention...\n");
    pthread_mutex_t contention_mutex;
    pthread_mutex_init(&contention_mutex, 0);
    pthread_mutex_lock(&contention_mutex);
    printf("Acquired\n");
    pthread_mutex_unlock(&contention_mutex);
    printf("Released!\n");

    syslog(LOG_INFO, "TEST 2");

    printf("Creating a recursive lock!\n");
    pthread_mutex_t recursive;
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&recursive, &attr);
    printf("Testing recursion...\n");
    pthread_mutex_lock(&recursive);
    pthread_mutex_lock(&recursive);
    pthread_mutex_lock(&recursive);
    printf("Locked OK, testing unlock...\n");
    pthread_mutex_unlock(&recursive);
    pthread_mutex_unlock(&recursive);
    pthread_mutex_unlock(&recursive);
    int r = pthread_mutex_unlock(&recursive);
    if (r >= 0) printf("Final unlock was not an error, not OK.\n");
    printf("Testing re-acquire...\n");
    pthread_mutex_lock(&recursive);
    printf("OK!\n");

    // Creating the list content...
    for (i = 0; i < LOOPS; i++)
        the_list.push_back(i);

    // Measuring time before starting the threads...
    gettimeofday(&tv1, NULL);

    syslog(LOG_INFO, "TEST 3");
    pthread_create(&thr1, NULL, consumer, (void *) 1);
    pthread_create(&thr2, NULL, consumer, (void *) 2);

    pthread_join(thr1, NULL);
    pthread_join(thr2, NULL);
    syslog(LOG_INFO, "TEST 4");

    // Measuring time after threads finished...
    gettimeofday(&tv2, NULL);

    if (tv1.tv_usec > tv2.tv_usec)
    {
        tv2.tv_sec--;
        tv2.tv_usec += 1000000;
    }

    printf("Result - %ld.%ld\n", tv2.tv_sec - tv1.tv_sec,
        tv2.tv_usec - tv1.tv_usec);

#ifdef USE_SPINLOCK
    pthread_spin_destroy(&spinlock);
#else
    pthread_mutex_destroy(&mutex);
#endif

    return 0;
}
