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

class InputEvent : public Event
{
    public:
        InputEvent(InputManager::InputNotification *pNote, uintptr_t handlerAddress) :
                    Event(handlerAddress, true, 0), m_Notification()
        {
            m_Notification = *pNote;
        };
        virtual ~InputEvent()
        {};

        virtual size_t serialize(uint8_t *pBuffer)
        {
            size_t *buf = reinterpret_cast<size_t*>(pBuffer);
            *buf = EventNumbers::InputEvent;
            memcpy(&buf[1], &m_Notification, sizeof(InputManager::InputNotification));
            return sizeof(InputManager::InputNotification) + sizeof(size_t);
        }

        static bool unserialize(uint8_t *pBuffer, InputEvent &event)
        {
            size_t *buf = reinterpret_cast<size_t*>(pBuffer);
            if(*buf != EventNumbers::InputEvent)
                return false;

            memcpy(&event.m_Notification, &buf[1], sizeof(InputManager::InputNotification));
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

void InputManager::mouseUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap)
{
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
#ifdef THREADS
    LockGuard<Spinlock> guard(m_QueueLock);
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

void InputManager::installCallback(CallbackType type, callback_t callback, Thread *pThread)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    if(type == Key)
    {
        CallbackItem *item = new CallbackItem;
        item->func = callback;
#ifdef THREADS
        item->pThread = pThread;
#endif
        m_Callbacks.pushBack(item);
    }
}

void InputManager::removeCallback(CallbackType type, callback_t callback, Thread *pThread)
{
    LockGuard<Spinlock> guard(m_QueueLock);
    if(type == Key)
    {
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
                Thread *pThread = (*it)->pThread;
                callback_t func = (*it)->func;
                if(!pThread)
                {
                    /// \todo Verify that the callback is in fact in the kernel
                    func(*pNote);
                    delete pNote;
                    continue;
                }

                InputEvent *pEvent = new InputEvent(pNote, reinterpret_cast<uintptr_t>(func));
                pThread->sendEvent(pEvent);
                delete pNote;
            }
        }
    }
#endif
}
