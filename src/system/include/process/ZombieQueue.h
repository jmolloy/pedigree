/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef _PROCESS_ZOMBIE_QUEUE_H
#define _PROCESS_ZOMBIE_QUEUE_H

#include <processor/types.h>
#include <utilities/RequestQueue.h>
#include <Log.h>

#include "Process.h"
#include "Thread.h"

/** 
  * ZombieQueue: takes zombie processes and threads, and frees them. Solves the
  * problem where an object needs to be deleted by itself (delete this is BAD).
  */
class ZombieQueue : public RequestQueue
{
    public:
        ZombieQueue();
        virtual ~ZombieQueue();

        static ZombieQueue &instance()
        {
            return m_Instance;
        }

        void addProcess(Process *pProcess);
        void addThread(Thread *pThread);

    private:
        virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                        uint64_t p6, uint64_t p7, uint64_t p8);

        static ZombieQueue m_Instance;
};

#endif

