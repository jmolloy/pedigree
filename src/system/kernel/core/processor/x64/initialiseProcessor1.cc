/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include <processor/initialise.h>
#include "InterruptManager.h"
#include "SyscallManager.h"
#include "../x86_common/PhysicalMemoryManager.h"
#include "../x86_common/IoPortManager.h"

void initialiseProcessor1(const BootstrapStruct_t &Info)
{
  // Initialise this processor's interrupt handling
  X64InterruptManager::initialiseProcessor();

  // Initialise this processor's syscall handling
  X64SyscallManager::initialiseProcessor();

  // Initialise the physical memory-management
  X86CommonPhysicalMemoryManager &physicalMemoryManager = X86CommonPhysicalMemoryManager::instance();
  physicalMemoryManager.initialise(Info);

  // TODO: Initialise the Virtual memory-management (if necessary)

  // Initialise the I/O Manager
  X86CommonIoPortManager &ioPortManager = X86CommonIoPortManager::instance();
  ioPortManager.initialise();

  // TODO
}
