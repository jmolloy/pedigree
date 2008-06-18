/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#include <process/Thread.h>
#include <process/Semaphore.h>
#include <process/Mutex.h>

/** Implements a request queue, with one worker thread performing
 * all requests. All requests appear synchronous to the calling thread - 
 * calling threads are blocked on mutexes (so they can be put to sleep) until
 * their request is complete. */
class RequestQueue
{
public:
  /** Creates a new RequestQueue. */
  RequestQueue();
  ~RequestQueue();
  
  /** Initialises the queue, spawning the worker thread. */
  void initialise();
  
  /** Destroys the queue, killing the worker thread (safely) */
  void destroy();
  
  /** Adds a request to the queue. Blocks until it finishes and returns the result. */
  uintptr_t addRequest(uintptr_t p1=0, uintptr_t p2=0, uintptr_t p3=0, uintptr_t p4=0, uintptr_t p5=0,
                       uintptr_t p6=0, uintptr_t p7=0, uintptr_t p8=0);

protected:
  /** Callback */
  virtual int executeRequest(uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5,
                             uintptr_t p6, uintptr_t p7, uintptr_t p8) = 0;
private:
  /** Request structure */
  class Request
  {
  public:
    Request() : p1(0),p2(0),p3(0),p4(0),p5(0),p6(0),p7(0),p8(0),ret(0),mutex(true);
    ~Request();
    uintptr_t p1,p2,p3,p4,p5,p6,p7,p8;
    uintptr_t ret;
    Mutex mutex;
  };
  
  /** Thread trampoline */
  static int trampoline(void *p);
  
  /** Thread worker function */
  int work();
  
  /** The request queue */
  Request *m_pRequestQueue;
  
  /** The semaphore giving the number of items in the queue. */
  Semaphore m_RequestQueueSize;
  
  /** True if the worker thread should cleanup and stop. */
  bool m_Stop;
}

#endif
