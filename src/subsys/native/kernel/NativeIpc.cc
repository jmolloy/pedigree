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

#include <process/Ipc.h>
#include <native/ipc/Ipc.h>

#include <utilities/Tree.h>
#include <utilities/Pair.h>
#include <utilities/String.h>

typedef Pair<uint64_t, PedigreeIpc::IpcMessage *> PerProcessUserPointerPair;

/// Lookup between userspace message pointers and the kernel-side heap pointer
/// that actually contains the relevant information for IPC.
Tree<Pair<uint64_t, PedigreeIpc::IpcMessage *>, Ipc::IpcMessage *> __msg_lookup;

inline uint64_t getPid()
{
    return Processor::information().getCurrentThread()->getParent()->getId();
}

uintptr_t createStandardMessage(PedigreeIpc::IpcMessage *pMessage)
{
    PerProcessUserPointerPair p = makePair(getPid(), pMessage);

    Ipc::IpcMessage *pKernelMessage = new Ipc::IpcMessage();
    Ipc::IpcMessage *pCheck = __msg_lookup.lookup(p);
    if(pCheck)
    {
        FATAL("Inserting an already allocated IPC message [createStandardMessage].");
    }
    __msg_lookup.insert(p, pKernelMessage);

    return reinterpret_cast<uintptr_t>(pKernelMessage->getBuffer());
}

uintptr_t createSharedMessage(PedigreeIpc::IpcMessage *pMessage, size_t nBytes, uintptr_t handle)
{
    PerProcessUserPointerPair p = makePair(getPid(), pMessage);

    Ipc::IpcMessage *pKernelMessage = new Ipc::IpcMessage(nBytes, handle);
    __msg_lookup.insert(p, pKernelMessage);

    return reinterpret_cast<uintptr_t>(pKernelMessage->getBuffer());
}

void *getIpcSharedRegion(PedigreeIpc::IpcMessage *pMessage)
{
    PerProcessUserPointerPair p = makePair(getPid(), pMessage);

    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(p);
    if(pKernelMessage)
        return pKernelMessage->getHandle();

    return 0;
}

void destroyMessage(PedigreeIpc::IpcMessage *pMessage)
{
    PerProcessUserPointerPair p = makePair(getPid(), pMessage);

    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(p);
    if(pKernelMessage)
    {
        __msg_lookup.remove(p);
        delete pKernelMessage;
    }
}

bool sendIpc(PedigreeIpc::IpcEndpoint *pEndpoint, PedigreeIpc::IpcMessage *pMessage, bool bAsync)
{
    PerProcessUserPointerPair p = makePair(getPid(), pMessage);

    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(p);
    if(pKernelMessage)
    {
        return Ipc::send(reinterpret_cast<Ipc::IpcEndpoint*>(pEndpoint), pKernelMessage, bAsync);
    }

    return false;
}

void *recvIpcPhase1(PedigreeIpc::IpcEndpoint *pEndpoint, bool bAsync)
{
    Ipc::IpcMessage *pMessage = 0;
    bool ret = Ipc::recv(reinterpret_cast<Ipc::IpcEndpoint*>(pEndpoint), &pMessage, bAsync);
    if(!ret)
        return 0;

    return reinterpret_cast<void*>(pMessage);
}

uintptr_t recvIpcPhase2(PedigreeIpc::IpcMessage *pUserMessage, void *pMessage)
{
    PerProcessUserPointerPair p = makePair(getPid(), pUserMessage);

    Ipc::IpcMessage *pKernelMessage = reinterpret_cast<Ipc::IpcMessage*>(pMessage);
    Ipc::IpcMessage *pCheck = __msg_lookup.lookup(p);
    if(pCheck)
    {
        FATAL("Inserting an already allocated IPC message [recvIpcPhase2].");
    }
    __msg_lookup.insert(p, pKernelMessage);

    return reinterpret_cast<uintptr_t>(pKernelMessage->getBuffer());
}

void createEndpoint(const char *name)
{
    String temp(name);
    Ipc::createEndpoint(temp);
}

void removeEndpoint(const char *name)
{
    String temp(name);
    Ipc::removeEndpoint(temp);
}

PedigreeIpc::IpcEndpoint *getEndpoint(const char *name)
{
    String temp(name);
    return reinterpret_cast<PedigreeIpc::IpcEndpoint *>(Ipc::getEndpoint(temp));
}
