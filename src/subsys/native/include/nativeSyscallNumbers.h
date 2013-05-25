/*
 * Copyright (c) 2011 James Molloy, Jörg Pfähler, Matthew Iselin
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

#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

#define NATIVE_USERSPACE_TO_KERNEL      1

#define IPC_CREATE_STANDARD_MESSAGE             2
#define IPC_CREATE_SHARED_MESSAGE               3
#define IPC_GET_SHARED_REGION                   4
#define IPC_DESTROY_MESSAGE                     5
#define IPC_SEND_IPC                            6
#define IPC_RECV_PHASE1                         7
#define IPC_RECV_PHASE2                         8
#define IPC_CREATE_ENDPOINT                     9
#define IPC_REMOVE_ENDPOINT                     10
#define IPC_GET_ENDPOINT                        11

#endif
