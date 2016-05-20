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

#ifndef SYSCALL_STUBS_H

/**
 * \module syscall_stubs Syscall Stubs
 * This provides the main entry point for all syscall definitions. The header
 * exports the syscallN() functions for the target architecture automatically.
 *
 * You will need to define SERVICE, SERVICE_ERROR, and SERVICE_INIT to make use
 * of this header.
 *
 * SERVICE is the syscall service identifier, as per those in the Syscall_t
 * enum.
 *
 * SERVICE_INIT creates any locals that are needed for e.g. error handling.
 *
 * SERVICE_ERROR is the variable into which the syscall's error status will be
 * stored; it must work as a single register on the target system.
 *
 * You cannot mix syscall services in the same compilation unit.
 */

#if defined(HOSTED) && !defined(SYSCALL_TARGET_FOUND)
#include <processor/hosted/syscall-stubs.h>
#define SYSCALL_TARGET_FOUND
#endif

#if defined(X64) && !defined(SYSCALL_TARGET_FOUND)
#include <processor/x64/syscall-stubs.h>
#define SYSCALL_TARGET_FOUND
#endif

#if defined(ARM_COMMON) && !defined(SYSCALL_TARGET_FOUND)
#include <processor/armv7/syscall-stubs.h>
#define SYSCALL_TARGET_FOUND
#endif

#ifndef SYSCALL_TARGET_FOUND
#error Syscall target not found!
#endif

#endif
