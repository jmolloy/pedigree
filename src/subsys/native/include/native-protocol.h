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

#ifndef _PEDIGREE_NATIVE_PROTOCOL_H
#define _PEDIGREE_NATIVE_PROTOCOL_H

#include <native/Object.h>

// Values for the 'meta' field in ReturnState.
#define META_ERROR_MASK         0xFFFF
#define META_ERROR_BADOBJECT    0x0001

struct ReturnState
{
    /** Success: whether or not the syscall was successful. */
    bool success;
    /**
     * Value: 64-bit integral value returned from kernel (relevance determined
     * by caller)
     */
    uint64_t value;
    /**
     * Meta: 64-bit extra metadata, typically used to report exception
     * information from the kernel for the purpose of bubbling exceptions.
     */
    uint64_t meta;
};

/** Performs a native API system call.
 *
 * @param pObject Object which is creating the call
 * @param nOp integer defining the operation to perform
 *
 * \todo A better solution than nOp would be nice.
 * \todo Event system integration
 */
int _syscall(Object *pObject, size_t nOp);

/**
 * Register the given native API object with the kernel.
 *
 * Throws an exception if the registration fails.
 */
void register_object(Object *pObject);

/**
 * Unregister the given native API object with the kernel.
 *
 * \return true if successful, false otherwise.
 */
void unregister_object(Object *pObject);

/**
 * Perform a native system call.
 *
 * \param pObject registered object on which to perform the call
 * \param subid method call ID
 * \param params pointer to parameter block for the system call
 * \return ReturnState object containing the result of the system call
 */
ReturnState native_call(Object *pObject, uint64_t subid, void *params, size_t params_size);

#endif
