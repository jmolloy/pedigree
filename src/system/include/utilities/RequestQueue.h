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

#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <processor/types.h>
#include <process/Semaphore.h>
#include <process/Mutex.h>

class Thread;

#define REQUEST_QUEUE_NUM_PRIORITIES 4

/** Implements a request queue, with one worker thread performing
 * all requests. All requests appear synchronous to the calling thread -
 * calling threads are blocked on mutexes (so they can be put to sleep) until
 * their request is complete. */
class RequestQueue
{
    friend class Thread;
public:
    /** Creates a new RequestQueue. */
    RequestQueue();
    virtual ~RequestQueue();

    /** Initialises the queue, spawning the worker thread. */
    virtual void initialise();

    /** Destroys the queue, killing the worker thread (safely) */
    virtual void destroy();

    /** Adds a request to the queue. Blocks until it finishes and returns the result.
        \param priority The priority to attach to this request. Lower number is higher priority. */
    uint64_t addRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                        uint64_t p6=0, uint64_t p7=0, uint64_t p8=0);

    /** Adds an asynchronous request to the queue. Will not block. */
    uint64_t addAsyncRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                             uint64_t p6=0, uint64_t p7=0, uint64_t p8=0);

    /**
     * Halt RequestQueue operations, but do not terminate the worker thread.
     */
    void halt();

    /**
     * Resume RequestQueue operations.
     */
    void resume();

protected:
    /** Callback - classes are expected to inherit and override this function. It's called when a
        request needs to be executed (by the worker thread). */
    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                    uint64_t p6, uint64_t p7, uint64_t p8) = 0;

    RequestQueue(const RequestQueue&);
    void operator =(const RequestQueue&);

    /** Request structure */
    class Request
    {
    public:
        Request() : p1(0),p2(0),p3(0),p4(0),p5(0),p6(0),p7(0),p8(0),ret(0),
#ifdef THREADS
                    mutex(true),pThread(0),
#endif
                    bReject(false),bCompleted(false),next(0),refcnt(0),
                    owner(0),priority(0) {}
        ~Request() {}
        uint64_t p1,p2,p3,p4,p5,p6,p7,p8;
        uint64_t ret;
#ifdef THREADS
        Mutex mutex;
        Thread *pThread;
#endif
        bool bReject;
        bool bCompleted;
        Request *next;
        size_t refcnt;
        RequestQueue *owner;
        size_t priority;
    private:
        Request(const Request&);
        void operator =(const Request&);
    };

    /**
     * Defaults to never comparing as equal. Used to determine duplicates
     * when adding async requests.
     */
    virtual bool compareRequests(const Request &a, const Request &b)
    {
        return false;
    }

    /**
     * Check whether the given request is still valid in terms of this RequestQueue.
     */
    bool isRequestValid(const Request *r);

    /** Thread trampoline */
    static int trampoline(void *p);

    /** Asynchronous thread trampoline */
    static int doAsync(void *p);

    /** Thread worker function */
    int work();

    /** The request queue */
    Request *m_pRequestQueue[REQUEST_QUEUE_NUM_PRIORITIES];

    /** True if the worker thread should cleanup and stop. */
    volatile bool m_Stop;

#ifdef THREADS
    /** Mutex to be held when the request queue is being changed. */
    Mutex m_RequestQueueMutex;

    /** The semaphore giving the number of items in the queue. */
    Semaphore m_RequestQueueSize;
    
    Thread *m_pThread;

    bool m_Halted;
    Mutex m_HaltAcknowledged;
#endif
};

#endif
