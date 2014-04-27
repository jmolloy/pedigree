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

#include <ipc/Ipc.h>

#include <native-syscall.h>
#include <nativeSyscallNumbers.h>

using namespace PedigreeIpc;

PedigreeIpc::StandardIpcMessage::StandardIpcMessage() : m_vAddr(0) {}

PedigreeIpc::StandardIpcMessage::~StandardIpcMessage()
{
    syscall1(IPC_DESTROY_MESSAGE, reinterpret_cast<uintptr_t>(this));
}

PedigreeIpc::SharedIpcMessage::SharedIpcMessage(size_t nBytes, void *handle) : StandardIpcMessage(), m_nBytes(nBytes), m_pHandle(handle) {}

PedigreeIpc::SharedIpcMessage::~SharedIpcMessage()
{
    syscall1(IPC_DESTROY_MESSAGE, reinterpret_cast<uintptr_t>(this));
}

PedigreeIpc::StandardIpcMessage::StandardIpcMessage(void *pKernelMessage)
{
    m_vAddr = reinterpret_cast<void*>(syscall2(IPC_RECV_PHASE2, reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(pKernelMessage)));
}

bool PedigreeIpc::StandardIpcMessage::initialise()
{
    m_vAddr = reinterpret_cast<void*>(syscall1(IPC_CREATE_STANDARD_MESSAGE, reinterpret_cast<uintptr_t>(this)));

    return m_vAddr != 0;
}

bool PedigreeIpc::SharedIpcMessage::initialise()
{
    m_vAddr = reinterpret_cast<void*>(syscall3(IPC_CREATE_SHARED_MESSAGE, reinterpret_cast<uintptr_t>(this), m_nBytes, reinterpret_cast<uintptr_t>(m_pHandle)));
    m_pHandle = reinterpret_cast<void*>(syscall1(IPC_GET_SHARED_REGION, reinterpret_cast<uintptr_t>(this)));

    return (m_vAddr != 0) && (m_pHandle != 0);
}

bool PedigreeIpc::send(IpcEndpoint *pEndpoint, IpcMessage *pMessage, bool bAsync)
{
    return static_cast<bool>(syscall3(IPC_SEND_IPC, reinterpret_cast<uintptr_t>(pEndpoint), reinterpret_cast<uintptr_t>(pMessage), static_cast<uintptr_t>(bAsync)));
}

bool PedigreeIpc::recv(IpcEndpoint *pEndpoint, IpcMessage **pMessage, bool bAsync)
{
    void *kernelPointer = reinterpret_cast<void*>(syscall2(IPC_RECV_PHASE1, reinterpret_cast<uintptr_t>(pEndpoint), static_cast<uintptr_t>(bAsync)));
    if(!kernelPointer)
        return false;

    *pMessage = new StandardIpcMessage(kernelPointer);

    return true;
}

void PedigreeIpc::createEndpoint(const char *name)
{
    syscall1(IPC_CREATE_ENDPOINT, reinterpret_cast<uintptr_t>(name));
}

void PedigreeIpc::removeEndpoint(const char *name)
{
    syscall1(IPC_REMOVE_ENDPOINT, reinterpret_cast<uintptr_t>(name));
}

IpcEndpoint *PedigreeIpc::getEndpoint(const char *name)
{
    return reinterpret_cast<IpcEndpoint*>(syscall1(IPC_GET_ENDPOINT, reinterpret_cast<uintptr_t>(name)));
}
