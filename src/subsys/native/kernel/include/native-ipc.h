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

#define _NATIVE_IPC_API_H
#define _NATIVE_IPC_API_H

#include <native/types.h>
#include <native/ipc/Ipc.h>

/**
 * This file defines the system calls for IPC for the Native subsystem's
 * SyscallManager.
 */

/**
 * Creates a standard, < 4 KB IPC message and returns the address of its buffer.
 * \param pMessage Userspace message pointer to link to the created kernel message.
 */
extern uintptr_t createStandardMessage(PedigreeIpc::IpcMessage *pMessage);

/**
 * Creates a shared region of memory for IPC, and returns the address of its
 * buffer. Can either create a new region or copy an existing one.
 * \param pMessage Userspace message pointer to link to the created kernel one.
 * \param nBytes Size of the region to allocate.
 * \param handle A handle to an existing region to map into the current address space.
 */
extern uintptr_t createSharedMessage(PedigreeIpc::IpcMessage *pMessage, size_t nBytes, uintptr_t handle);

/**
 * Obtains the region handle for the given message, if one exists. Returns null
 * if no shared region exists. Must be called after createSharedMessage.
 */
extern void *getIpcSharedRegion(PedigreeIpc::IpcMessage *pMessage);

/**
 * Destroys the given message on the kernel side, freeing the memory related to
 * it for other messages to use.
 */
extern void destroyMessage(PedigreeIpc::IpcMessage *pMessage);

/**
 * Sends the given message to the given endpoint, optionally asynchronously.
 */
extern bool sendIpc(PedigreeIpc::IpcEndpoint *pEndpoint, PedigreeIpc::IpcMessage *pMessage, bool bAsync);

/**
 * First phase of receiving a message. Will return 0 if an error occurred or no
 * message is queued, otherwise will return a pointer to be passed to the
 * PedigreeIpc::SharedIpcMessage::SharedIpcMessage(void*) constructor.
 */
extern void *recvIpcPhase1(PedigreeIpc::IpcEndpoint *pEndpoint, bool bAsync);

/**
 * Second phase of receiving a message. To be called within the
 * PedigreeIpc::SharedIpcMessage::SharedIpcMessage(void*) constructor.
 * Takes a userspace pointer and a kernel message pointer (returned by recvPhase1)
 * and links them, then returns the buffer address for the message.
 */
extern uintptr_t recvIpcPhase2(PedigreeIpc::IpcMessage *pUserMessage, void *pMessage);

/**
 * Creates an endpoint with the given name, if one doesn't exist already.
 */
extern void createEndpoint(const char *name);

/**
 * Removes an endpoint with the given name.
 */
extern void removeEndpoint(const char *name);

/**
 * Gets a pointer to the endpoint with the given name.
 */
extern PedigreeIpc::IpcEndpoint *getEndpoint(const char *name);
