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
#ifndef MACHINE_INPUT_MANAGER_H
#define MACHINE_INPUT_MANAGER_H

#include <processor/Processor.h>
#include <processor/types.h>
#include <utilities/List.h>
#include <process/Semaphore.h>
#include <Spinlock.h>

/**
 * Global manager for all input from HID devices.
 */
class InputManager
{
    public:

        /// The type for a given callback. In the future callbacks may have
        /// other types such as "Mouse" which need to be handled.
        enum CallbackType
        {
            Key = 0,
            Mouse = 1,
            Joystick = 2,
            Unknown = 255
        };

        /// Structure containing notification to the remote application
        /// of input. Used to generalise input handling across the system
        /// for all types of devices.
        struct InputNotification
        {
            CallbackType type;

            union
            {
                struct
                {
                    uint64_t key;
                } key;
                struct
                {
                    ssize_t relx;
                    ssize_t rely;
                    ssize_t relz;

                    bool buttons[64];
                } pointy;
            } data;
        };

        /// Callback function type
        typedef void (*callback_t)(InputNotification &);

        /// Default constructor
        InputManager();

        /// Default destructor
        virtual ~InputManager();

        /// Begins the worker thread
        void initialise();

        /// Singleton design
        static InputManager& instance()
        {
            return m_Instance;
        }

        /// Called whenever a key is pressed and needs to be added to the queue
        void keyPressed(uint64_t key);

        /// Called whenever mouse input comes in.
        void mouseUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap);

        /// Called whenever joystick input comes in
        void joystickUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap);

        /// Installs a callback for a specific item
        void installCallback(CallbackType type, callback_t callback, Thread *pThread = 0);

        /// Removes a callback for a specific item
        void removeCallback(CallbackType type, callback_t callback, Thread *pThread = 0);

        /// Thread trampoline
        static int trampoline(void *ptr);

        /// Main worker thread
        void mainThread();

    private:
        /// Static instance
        static InputManager m_Instance;

        /// Puts a notification into the queue (doer for all main functions)
        /// \note Deletes \p note if THREADS is not defined
        void putNotification(InputNotification *note);

        /// Item in the callback list. This stores information that may be needed
        /// to create and send an Event for a userspace callback.
        struct CallbackItem
        {
            /// The handler function
            callback_t func;

#ifdef THREADS
            /// Thread to send an Event to. If null, the Event will be sent to the
            /// current thread, which is only valid for kernel callbacks (as there
            /// will be no address space switch for a call to a kernel function).
            Thread *pThread;
#endif
        };

        /// Input queue (for distribution to applications)
        List<InputNotification*> m_InputQueue;

        /// Spinlock for work on queues.
        /// \note Using a Spinlock here because a lot of our work will happen
        ///       in the middle of an IRQ where it's potentially dangerous to
        ///       reschedule (which may happen with a Mutex or Semaphore).
        Spinlock m_QueueLock;

        /// Callback list
        List<CallbackItem*> m_Callbacks;

#ifdef THREADS
        /// Key press queue Semaphore
        Semaphore m_InputQueueSize;

        /// Thread object for our worker thread
        Thread *m_pThread;
#endif
};

#endif
