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

#include <machine/InputManager.h>
#include <LockGuard.h>
#include <Log.h>

InputManager InputManager::m_Instance;

InputManager::InputManager() :
    m_KeyQueue(), m_KeyQueueSize(0), m_QueueLock(), m_KeyCallbacks(), m_pThread(0)
{
}

InputManager::~InputManager()
{
}

void InputManager::initialise()
{
    // Start the worker thread.
#ifdef THREADS
    m_pThread = new Thread(Processor::information().getCurrentThread()->getParent(),
                           reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
                           reinterpret_cast<void*> (this));
#endif
}

void InputManager::keyPressed(uint64_t key)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    m_KeyQueue.pushBack(key);
    m_KeyQueueSize.release();
}

void InputManager::installCallback(CallbackType type, callback_t callback)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    if(type == Key)
    {
        m_KeyCallbacks.pushBack(
                    reinterpret_cast<void*>(
                    reinterpret_cast<uint32_t>(callback)
                    )
        );
    }
}

int InputManager::trampoline(void *ptr)
{
    InputManager *p = reinterpret_cast<InputManager *>(ptr);
    p->mainThread();
    return 0;
}

void InputManager::mainThread()
{
    while(true)
    {
        m_KeyQueueSize.acquire();
        if(!m_KeyQueue.count())
            continue; /// \todo Handle exit condition

        m_QueueLock.acquire();
        uint64_t key = m_KeyQueue.popFront();
        m_QueueLock.release();

        for(List<void*>::Iterator it = m_KeyCallbacks.begin();
            it != m_KeyCallbacks.end();
            it++)
        {
            /// \todo I'd love to see a direct call to userspace at some point,
            ///       which would totally revolutionise the way the TUI works.
            if(*it)
            {
                callback_t f = reinterpret_cast<callback_t>(*it);
                f(key);
            }
        }
    }
}
