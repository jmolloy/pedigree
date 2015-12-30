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

#ifndef _POSIX_KERNEL_LOGGING_H
#define _POSIX_KERNEL_LOGGING_H

// Logs to the kernel log with the current PID.
#define POSIX_VERBOSE_LOG(f, x) do { \
        auto id = Processor::information().getCurrentThread()->getParent()->getId(); \
        NOTICE("[" << f << ":\t" << Dec << id << Hex << "]\t" << x); \
    } while(0)

// POSIX_LOG_FACILITIES is an integer for which each bit indicates a particular
// category to enable verbose logging for.
#ifdef POSIX_LOG_FACILITIES

#if POSIX_LOG_FACILITIES & 1
#define POSIX_VERBOSE_SYSTEM_SYSCALLS
#endif

#if POSIX_LOG_FACILITIES & 2
#define POSIX_VERBOSE_FILE_SYSCALLS
#endif

#if POSIX_LOG_FACILITIES & 4
#define POSIX_VERBOSE_PTHREAD_SYSCALLS
#endif

#if POSIX_LOG_FACILITIES & 8
#define POSIX_VERBOSE_NET_SYSCALLS
#endif

#if POSIX_LOG_FACILITIES & 16
#define POSIX_VERBOSE_SIGNAL_SYSCALLS
#endif

#if POSIX_LOG_FACILITIES & 32
#define POSIX_ULTRA_VERBOSE_SIGNAL_SYSCALLS
#endif

#endif

#ifdef POSIX_VERBOSE_SYSTEM_SYSCALLS
#define SC_NOTICE(x) POSIX_VERBOSE_LOG("sys", x)
#else
#define SC_NOTICE(x)
#endif

#ifdef POSIX_VERBOSE_FILE_SYSCALLS
#define F_NOTICE(x) POSIX_VERBOSE_LOG("file", x)
#else
#define F_NOTICE(x)
#endif

#ifdef POSIX_VERBOSE_PTHREAD_SYSCALLS
#define PT_NOTICE(x) POSIX_VERBOSE_LOG("thr", x)
#else
#define PT_NOTICE(x)
#endif

#ifdef POSIX_VERBOSE_NET_SYSCALLS
#define N_NOTICE(x) POSIX_VERBOSE_LOG("net", x)
#else
#define N_NOTICE(x)
#endif

#ifdef POSIX_VERBOSE_SIGNAL_SYSCALLS
#define SG_NOTICE(x) POSIX_VERBOSE_LOG("sig", x)
#else
#define SG_NOTICE(x)
#endif

#ifdef POSIX_ULTRA_VERBOSE_SIGNAL_SYSCALLS
#define SG_VERBOSE_NOTICE(x) SG_NOTICE(x)
#else
#define SG_VERBOSE_NOTICE(x)
#endif

#endif  // _POSIX_KERNEL_LOGGING_H
