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
#include <process/Ipc.h>

#include <utilities/RadixTree.h>

#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>

#include <utilities/MemoryPool.h>

using namespace Ipc;

#define MEMPOOL_BUFF_SIZE       4096
#define MEMPOOL_BASE_SIZE       1024 /// \todo Tune.

MemoryPool __ipc_mempool("IPC Message Pool");

RadixTree<IpcEndpoint*> __endpoints;

IpcEndpoint *Ipc::getEndpoint(String &name)
{
    return __endpoints.lookup(name);
}

void Ipc::createEndpoint(String &name)
{
    if(__endpoints.lookup(name))
        return;
    __endpoints.insert(name, new IpcEndpoint(name));
}

void Ipc::removeEndpoint(String &name)
{
    if(!__endpoints.lookup(name))
        return;
    __endpoints.remove(name);
}

bool Ipc::send(IpcEndpoint *pEndpoint, IpcMessage *pMessage, bool bAsync)
{
    if(!(pEndpoint && pMessage))
        return false;

    pEndpoint->pushMessage(pMessage);

    return true;
}

bool Ipc::recv(IpcEndpoint *pEndpoint, IpcMessage **pMessage, bool bAsync)
{
    if(!(pEndpoint && pMessage))
        return false;

    IpcMessage *pMsg = pEndpoint->getMessage(!bAsync);
    if(pMsg)
    {
        /// > 4 KB messages only use this form of IPC as a synchronisation
        /// method, they actually communicate via reads and writes to their
        /// shared block of memory.
        *pMessage = new IpcMessage(*pMsg);

        return true;
    }

    return false;
}

Ipc::IpcMessage::IpcMessage() : nPages(1), m_vAddr(0), m_pMemRegion(0)
{
    if(!__ipc_mempool.initialised())
    {
        if(!__ipc_mempool.initialise(MEMPOOL_BASE_SIZE, MEMPOOL_BUFF_SIZE))
        {
            ERROR("IpcMessage: memory pool could not be initialised.");
            return;
        }
    }
    
    // Allocate the message.
    uintptr_t msg = __ipc_mempool.allocate();
    if(msg)
    {
        // Remap to user read/write.
        Processor::information().getVirtualAddressSpace().setFlags(reinterpret_cast<void*>(msg), VirtualAddressSpace::Write);
        
        m_vAddr = msg;
    }
    else
    {
        ERROR("IpcMessage: no memory available.");
        return;
    }
}

Ipc::IpcMessage::IpcMessage(size_t nBytes, uintptr_t regionHandle) : nPages(0), m_vAddr(0), m_pMemRegion(0)
{
    /// \todo Remap the MemoryRegion at regionHandle into THIS address space,
    /// if it's not the same one.
    /// \todo regionHandle == 0 means "create a new one".

    nPages = (nBytes / 4096) + 1;
    m_pMemRegion = reinterpret_cast<MemoryRegion*>(regionHandle);

    if(nPages == 1)
        IpcMessage();

    if(!m_pMemRegion)
        ERROR("IpcMessage: regionHandle misused.");
}

Ipc::IpcMessage::~IpcMessage()
{
    /// \todo Problem: an endpoint might still have this message in a queue.
    /// We should notify it that we're now dead.
    if(m_pMemRegion)
    {
        delete m_pMemRegion;
    }
    else if(m_vAddr)
        __ipc_mempool.free(m_vAddr);
}

void *Ipc::IpcMessage::getBuffer()
{
    if(m_vAddr)
        return reinterpret_cast<void*>(m_vAddr);
    else if(m_pMemRegion)
        return m_pMemRegion->virtualAddress();
    else
        return 0;
}

void *Ipc::IpcMessage::getHandle()
{
    if(nPages > 1)
        return m_pMemRegion;
    else
        return 0;
}
