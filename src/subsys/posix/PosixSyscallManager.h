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
#ifndef POSIX_SYSCALL_MANAGER_H
#define POSIX_SYSCALL_MANAGER_H

#include <processor/types.h>
#include <processor/SyscallHandler.h>

class PosixSyscallManager : public SyscallHandler
{
public:
  void initialise();

  /** Calls a syscall. */
  uintptr_t call(uintptr_t function, uintptr_t p1=0, uintptr_t p2=0, uintptr_t p3=0, uintptr_t p4=0, uintptr_t p5=0);

  /** Called when a syscall arrives. */
  virtual uintptr_t syscall(SyscallState &state);

  /** The constructor */
  PosixSyscallManager();
  /** The destructor */
  virtual ~PosixSyscallManager();

private:
  /** The copy-constructor
   *\note Not implemented (singleton) */
  PosixSyscallManager(const PosixSyscallManager &);
  /** The copy-constructor
   *\note Not implemented (singleton) */
  PosixSyscallManager &operator = (const PosixSyscallManager &);
};

#endif
