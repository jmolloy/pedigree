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

#include <Module.h>
#include <Log.h>
#include <processor/Processor.h>
#include <process/Process.h>
#include <process/Scheduler.h>
#include <vfs/VFS.h>
#include "PosixSyscallManager.h"
#include "signal-syscalls.h"
#include "system-syscalls.h"
#include "UnixFilesystem.h"
#include "DevFs.h"

PosixSyscallManager g_PosixSyscallManager;

UnixFilesystem *g_pUnixFilesystem;

static void init()
{
  g_PosixSyscallManager.initialise();

  DevFs::instance().initialise(0);

  g_pUnixFilesystem = new UnixFilesystem();

  VFS::instance().addAlias(g_pUnixFilesystem, g_pUnixFilesystem->getVolumeLabel());
  VFS::instance().addAlias(&DevFs::instance(), DevFs::instance().getVolumeLabel());
}

static void destroy()
{
}

#ifdef ARM_COMMON
MODULE_INFO("posix", &init, &destroy, "console");
#else
MODULE_INFO("posix", &init, &destroy, "console", "network-stack", "TUI");
#endif
