/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#ifndef NET_SYSCALLS_H
#define NET_SYSCALLS_H

#include <vfs/File.h>

int posix_socket(int domain, int type, int protocol);
int posix_connect(int sock, struct sockaddr* address, size_t addrlen);

ssize_t posix_send(int sock, const void* buff, size_t bufflen, int flags);
ssize_t posix_sendto(int sock, const void* buff, size_t bufflen, int flags, const struct sockaddr* address, size_t addrlen);
ssize_t posix_recv(int sock, void* buff, size_t bufflen, int flags);
ssize_t posix_recvfrom(int sock, void* buff, size_t bufflen, int flags, struct sockaddr* address, size_t* addrlen);

int posix_listen(int sock, int backlog);
int posix_bind(int sock, const struct sockaddr *address, size_t addrlen);
int posix_accept(int sock, struct sockaddr* address, size_t* addrlen);

int posix_gethostbyaddr(const void* addr, size_t len, int type, void* ent);
int posix_gethostbyname(const char* name, void* hostinfo, int offset);

#endif
