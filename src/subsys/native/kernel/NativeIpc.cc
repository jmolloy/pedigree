/*
 * Copyright (c) 2011 Matthew Iselin
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
#include <ipc/Ipc.h>

#include <utilities/Tree.h>
#include <utilities/String.h>

/// Lookup between userspace message pointers and the kernel-side heap pointer
/// that actually contains the relevant information for IPC.
Tree<PedigreeIpc::IpcMessage *, Ipc::IpcMessage *> __msg_lookup;

uintptr_t createStandardMessage(PedigreeIpc::IpcMessage *pMessage)
{
    Ipc::IpcMessage *pKernelMessage = new Ipc::IpcMessage();
    __msg_lookup.insert(pMessage, pKernelMessage);

    return reinterpret_cast<uintptr_t>(pKernelMessage->getBuffer());
}

uintptr_t createSharedMessage(PedigreeIpc::IpcMessage *pMessage, size_t nBytes, uintptr_t handle)
{
    Ipc::IpcMessage *pKernelMessage = new Ipc::IpcMessage(nBytes, handle);
    __msg_lookup.insert(pMessage, pKernelMessage);

    return reinterpret_cast<uintptr_t>(pKernelMessage->getBuffer());
}

void *getIpcSharedRegion(PedigreeIpc::IpcMessage *pMessage)
{
    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(pMessage);
    if(pKernelMessage)
        return pKernelMessage->getBuffer();

    return 0;
}

void destroyMessage(PedigreeIpc::IpcMessage *pMessage)
{
    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(pMessage);
    if(pKernelMessage)
    {
        __msg_lookup.remove(pMessage);
        delete pKernelMessage;
    }
}

bool sendIpc(PedigreeIpc::IpcEndpoint *pEndpoint, PedigreeIpc::IpcMessage *pMessage, bool bAsync)
{
    Ipc::IpcMessage *pKernelMessage = __msg_lookup.lookup(pMessage);
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
    Ipc::IpcMessage *pKernelMessage = reinterpret_cast<Ipc::IpcMessage*>(pMessage);
    __msg_lookup.insert(pUserMessage, pKernelMessage);

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
