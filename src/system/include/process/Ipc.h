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
#ifndef _PROCESS_IPC_H
#define _PROCESS_IPC_H

#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>

#include <process/Semaphore.h>
#include <process/Mutex.h>

#include <utilities/String.h>
#include <utilities/List.h>

#include <Log.h>

namespace Ipc
{
    class IpcMessage
    {
        public:
            IpcMessage();

            /**
             * \brief Constructor for regions > 4 KB in size.
             *
             * If you pass an existing region handle, this will link the instance
             * being created to an existing message that may well be in another
             * address space. That handle is obtained via IpcMessage::getHandle,
             * and should be passed via conventional < 4 KB IPC messages.
             *
             * \param nBytes Number of bytes to allocate for this region.
             * \param regionHandle Handle to a region. Each message allocated with
             *        this method when this parameter is 0 is given a unique handle
             *        to pass to other processes in order to share memory.
             */
            IpcMessage(size_t nBytes, uintptr_t regionHandle = 0);

            virtual ~IpcMessage();

            /// Get the memory buffer for this IPC message. This is typically a
            /// shared memory region to avoid copies where possible.
            void *getBuffer();

            /// Get a handle for the region this message is in. Returns NULL for
            /// messages that do not have a size greater than 4 KB, as these are
            /// shared by default.
            void *getHandle();

            /// Copy constructor. Used to create an IpcMessage on the other side
            /// of a send (ie, in the receiving process) when the size is < 4 KB.
            /// Creating an IpcMessage in another process when the size is over 4 KB
            /// should be done with IpcMessage(size_t, uintptr_t).
            IpcMessage(const IpcMessage &src)
            {
                if(src.nPages > 1)
                    FATAL("IpcMessage: copy constructor misused.");
                nPages = src.nPages;
                m_vAddr = src.m_vAddr;
                m_pMemRegion = src.m_pMemRegion;
            }

        private:
            size_t nPages;

            /// Virtual address of a message when m_pMemRegion is invalid.
            uintptr_t m_vAddr;

            /// This is the memory region we can pass around as a handle IFF we are
            /// working with a region > 4 KB in size. Allocated as a result of
            /// the @IpcMessage constructor with regionHandle == 0 and nBytes > 4096
            MemoryRegion *m_pMemRegion;
    };

    /// Wraps a message queue and links that to a name. This handles blocking and
    /// such, reducing the complexity of the Ipc namespace (and reducing the amount
    /// of IPC-related code in ring3 APIs).
    class IpcEndpoint
    {
        public:
            IpcEndpoint(String name) :
                m_Name(name), m_Queue(), m_QueueSize(0), m_QueueLock(false)
            {
                NOTICE("Creating endpoint with name " << name);
                NOTICE("Endpoint is at " << ((uintptr_t) this));
            }

            ~IpcEndpoint()
            {
                FATAL("IpcEndpoint " << m_Name << " is being destroyed.");
            }

            Mutex *pushMessage(IpcMessage* pMessage, bool bAsync);

            IpcMessage *getMessage(bool bBlock = false);

            const String &getName() const
            {
                return m_Name;
            }

        private:

            /// A queued message ready for retrieval.
            struct QueuedMessage
            {
                Mutex *pMutex;
                IpcMessage *pMessage;
                bool bAsync;
            };

            String m_Name;

            List<QueuedMessage*> m_Queue;
            Semaphore m_QueueSize;

            Mutex m_QueueLock;
    };

    bool send(IpcEndpoint *pEndpoint, IpcMessage *pMessage, bool bAsync = false);
    bool recv(IpcEndpoint *pEndpoint, IpcMessage **pMessage, bool bAsync = false);

    IpcEndpoint *getEndpoint(String &name);

    void createEndpoint(String &name);
    void removeEndpoint(String &name);
};

#endif

