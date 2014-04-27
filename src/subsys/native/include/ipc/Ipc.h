/*
 * 
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

#ifndef _NATIVE_IPC_H
#define _NATIVE_IPC_H

#include <types.h>

namespace PedigreeIpc
{
    /// A standard IPC message that is less than 4 KB in size.
    class StandardIpcMessage
    {
        public:
            StandardIpcMessage();
            virtual ~StandardIpcMessage();

            virtual bool initialise();

            /// Internal use only.
            StandardIpcMessage(void *);

            virtual void *getBuffer() const
            {
                return static_cast<void*>(m_vAddr);
            }

        protected:
            void *m_vAddr;
    };

    /// A message that is over 4 KB in size, and is really a shared memory area
    /// rather than an IPC message, to be precise.
    class SharedIpcMessage : public StandardIpcMessage
    {
        public:
            SharedIpcMessage(size_t nBytes, void *existingHandle);
            virtual ~SharedIpcMessage();

            virtual bool initialise();

            void *getHandle() const
            {
                return m_pHandle;
            }

            size_t getSize() const
            {
                return m_nBytes;
            }

        private:
            size_t m_nBytes;
            void *m_pHandle;
    };

    /// Wrapper for an IPC endpoint. Utterly meaningless to everyone except the
    /// kernel, where it's actually useful.
    typedef void *IpcEndpoint;

    /// System calls. Cast SharedIpcMessage pointers to StandardIpcMessage.
    bool send(IpcEndpoint *pEndpoint, StandardIpcMessage *pMessage, bool bAsync = false);
    bool recv(IpcEndpoint *pEndpoint, StandardIpcMessage **pMessage, bool bAsync = false);

    IpcEndpoint *getEndpoint(const char *name);

    void createEndpoint(const char *name);
    void removeEndpoint(const char *name);

    /// Shorthand for the < 4 KB message type.
    typedef StandardIpcMessage IpcMessage;
};

#endif

