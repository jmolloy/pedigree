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

        /// The type for a given callback (enum values can't be used for bitwise
        /// operations, so we define these as constants).
        const static int Key = 1;
        const static int Mouse = 2;
        const static int Joystick = 4;
        const static int RawKey = 8;
        const static int Unknown = 255;
        
        typedef int CallbackType;

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
                struct
                {
                    /// HID scancode for the key (most generic type of scancode,
                    /// and easy to build translation tables for)
                    uint8_t scancode;
                    
                    /// Whether this is a keyUp event or not.
                    bool keyUp;
                } rawkey;
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

        /// Shuts down the worker thread, clears queues, and removes callbacks.
        void shutdown();

        /// Singleton design
        static InputManager& instance()
        {
            return m_Instance;
        }

        /// Called whenever a key is pressed and needs to be added to the queue
        void keyPressed(uint64_t key);
        
        /// Called whenever a raw key signal comes in
        /// \param scancode a HID scancode
        void rawKeyUpdate(uint8_t scancode, bool bKeyUp);

        /// Called whenever mouse input comes in.
        void mouseUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap);

        /// Called whenever joystick input comes in
        void joystickUpdate(ssize_t relX, ssize_t relY, ssize_t relZ, uint32_t buttonBitmap);

        /// Installs a callback
        void installCallback(CallbackType filter, callback_t callback, Thread *pThread = 0, uintptr_t param = 0);

        /// Removes a callback
        void removeCallback(callback_t callback, Thread *pThread = 0);
        
        /// Removes a callback by searching for a Thread pointer. This can be
        /// used to avoid useless and broken links to a Thread in the callback
        /// list if the Thread doesn't clean up properly.
        bool removeCallbackByThread(Thread *pThread);

        /// Thread trampoline
        static int trampoline(void *ptr);

        /// Main worker thread
        void mainThread();

        /// Returns whether the instance is creating notifications.
        bool isActive() const
        {
            return m_bActive;
        }

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

            /// Parameter to put into the serialised buffer sent to userspace.
            /// Typically holds the address of a userspace callback.
            uintptr_t nParam;
            
            /// Filter for this callback
            CallbackType filter;
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

        /// Are we active?
        bool m_bActive;
};

#endif
