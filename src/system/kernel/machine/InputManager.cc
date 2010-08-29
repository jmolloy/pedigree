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

#include <process/Event.h>

// Incoming relative mouse movements are divided by this
#define MOUSE_REDUCE_FACTOR     1

class InputEvent : public Event
{
    public:
        InputEvent(InputManager::InputNotification *pNote, uintptr_t param, uintptr_t handlerAddress) :
                    Event(handlerAddress, true, 0), m_Notification(), m_nParam(param)
        {
            m_Notification = *pNote;
        };
        virtual ~InputEvent()
        {};

        virtual size_t serialize(uint8_t *pBuffer)
        {
            uintptr_t *buf = reinterpret_cast<uintptr_t*>(pBuffer);
            buf[0] = EventNumbers::InputEvent;
            buf[1] = m_nParam;
            memcpy(&buf[2], &m_Notification, sizeof(InputManager::InputNotification));
            return sizeof(InputManager::InputNotification) + (sizeof(uintptr_t) * 2);
        }

        static bool unserialize(uint8_t *pBuffer, InputEvent &event)
        {
            uintptr_t *buf = reinterpret_cast<uintptr_t*>(pBuffer);
            if(*buf != EventNumbers::InputEvent)
                return false;

            memcpy(&event.m_Notification, &buf[2], sizeof(InputManager::InputNotification));
            return true;
        }

        virtual inline size_t getNumber()
        {
            return EventNumbers::InputEvent;
        }

        inline InputManager::CallbackType getType()
        {
            return m_Notification.type;
        }

        inline uint64_t getKey()
        {
            return m_Notification.data.key.key;
        }

        inline ssize_t getRelX()
        {
            return m_Notification.data.pointy.relx;
        }

        inline ssize_t getRelY()
        {
            return m_Notification.data.pointy.rely;
        }

        inline ssize_t getRelZ()
        {
            return m_Notification.data.pointy.relz;
        }

        inline void getButtonStates(bool states[64], size_t maxDesired = 64)
        {
            for(size_t i = 0; i < maxDesired; i++)
                states[i] = m_Notification.data.pointy.buttons[i];
        }
    private:

        InputManager::InputNotification m_Notification;

        uintptr_t m_nParam;
};

InputManager InputManager::m_Instance;

InputManager::InputManager() :
    m_InputQueue(), m_QueueLock(), m_Callbacks()
#ifdef THREADS
    , m_InputQueueSize(0), m_pThread(0)
#endif
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
#else
    WARNING("InputManager: No thread support, no worker thread will be active");
#endif
}

void InputManager::keyPressed(uint64_t key)
{
    InputNotification *note = new InputNotification;
    note->type = Key;
    note->data.key.key = key;

    putNotification(note);
}

void InputManager::rawKeyUpdate(uint8_t scancode, bool bKeyUp)
{
    InputNotification *note = new InputNotification;
    note->type = RawKey;
    note->data.rawkey.scancode = scancode;
    note->data.rawkey.keyUp = bKeyUp;

    putNotification(note);
}

void InputManager::mouseUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap)
{
    // Smooth input out
    relX /= MOUSE_REDUCE_FACTOR;
    relY /= MOUSE_REDUCE_FACTOR;
    relZ /= MOUSE_REDUCE_FACTOR;

    InputNotification *note = new InputNotification;
    note->type = Mouse;
    note->data.pointy.relx = relX;
    note->data.pointy.rely = relY;
    note->data.pointy.relz = relZ;
    for(size_t i = 0; i < 64; i++)
        note->data.pointy.buttons[i] = buttonBitmap & (1 << i);

    putNotification(note);
}

void InputManager::joystickUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap)
{
    InputNotification *note = new InputNotification;
    note->type = Joystick;
    note->data.pointy.relx = relX;
    note->data.pointy.rely = relY;
    note->data.pointy.relz = relZ;
    for(size_t i = 0; i < 64; i++)
        note->data.pointy.buttons[i] = buttonBitmap & (1 << i);

    putNotification(note);
}

void InputManager::putNotification(InputNotification *note)
{
    LockGuard<Spinlock> guard(m_QueueLock);

    // Can we mitigate this notification?
    if(note->type == Mouse)
    {
        for(List<InputNotification*>::Iterator it = m_InputQueue.begin();
            it != m_InputQueue.end();
            it++)
        {
            if((*it)->type == Mouse)
            {
                (*it)->data.pointy.relx += note->data.pointy.relx;
                (*it)->data.pointy.rely += note->data.pointy.rely;
                (*it)->data.pointy.relz += note->data.pointy.relz;

                for(int i = 0; i < 64; i++)
                {
                    if(note->data.pointy.buttons[i])
                        (*it)->data.pointy.buttons[i] = true;
                }

                // Merged, this precise logic means only one mouse event is ever
                // in the queue, so it's safe to just return here.
                return;
            }
        }
    }

#ifdef THREADS
    m_InputQueue.pushBack(note);
    m_InputQueueSize.release();
#else
    // No need for locking, as no threads exist
    for(List<CallbackItem*>::Iterator it = m_Callbacks.begin();
        it != m_Callbacks.end();
        it++)
    {
        if(*it)
        {
            callback_t func = (*it)->func;
            func(*note);
        }
    }

    delete note;
#endif
}

void InputManager::installCallback(CallbackType filter, callback_t callback, Thread *pThread, uintptr_t param)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    CallbackItem *item = new CallbackItem;
    item->func = callback;
#ifdef THREADS
    item->pThread = pThread;
#endif
    item->nParam = param;
    item->filter = filter;
    m_Callbacks.pushBack(item);
}

void InputManager::removeCallback(callback_t callback, Thread *pThread)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    for(List<CallbackItem*>::Iterator it = m_Callbacks.begin();
        it != m_Callbacks.end();
        it++)
    {
        if(*it)
        {
            if(
#ifdef THREADS
                (pThread == (*it)->pThread) &&
#endif
                (callback == (*it)->func))
            {
                m_Callbacks.erase(it);
                return;
            }
        }
    }
}

bool InputManager::removeCallbackByThread(Thread *pThread)
{
#ifdef THREADS
    LockGuard<Spinlock> guard(m_QueueLock);
    for(List<CallbackItem*>::Iterator it = m_Callbacks.begin();
        it != m_Callbacks.end();
        it++)
    {
        if(*it)
        {
            if(pThread == (*it)->pThread)
            {
                m_Callbacks.erase(it);
                return true;
            }
        }
    }

    return false;
#endif
}

int InputManager::trampoline(void *ptr)
{
    InputManager *p = reinterpret_cast<InputManager *>(ptr);
    p->mainThread();
    return 0;
}

void InputManager::mainThread()
{
#ifdef THREADS
    while(true)
    {
        m_InputQueueSize.acquire();
        if(!m_InputQueue.count())
            continue; /// \todo Handle exit condition

        m_QueueLock.acquire();
        InputNotification *pNote = m_InputQueue.popFront();
        m_QueueLock.release();

        // Don't send the key to applications if it was zero
        if(!pNote)
            continue;

        for(List<CallbackItem*>::Iterator it = m_Callbacks.begin();
            it != m_Callbacks.end();
            it++)
        {
            if(*it)
            {
                if((*it)->filter & pNote->type)
                {
                    Thread *pThread = (*it)->pThread;
                    callback_t func = (*it)->func;
                    if(!pThread)
                    {
                        /// \todo Verify that the callback is in fact in the kernel
                        func(*pNote);
                        delete pNote;
                        continue;
                    }

                    InputEvent *pEvent = new InputEvent(pNote, (*it)->nParam, reinterpret_cast<uintptr_t>(func));
                    pThread->sendEvent(pEvent);
                    delete pNote;
                }
            }
        }
    }
#endif
}
