/*
 * Copyright (c) 2013 Matthew Iselin
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
#include <Log.h>
#include <Module.h>
#include <vfs/VFS.h>
#include <vfs/Filesystem.h>
#include <process/Semaphore.h>
#include <process/Scheduler.h>
#include <vfs/MemoryMappedFile.h>

const char *g_FilesToPreload[] = {
    "root»/applications/winman",
    "root»/applications/tui",
    "root»/applications/TUI",
    "root»/applications/login",
    "root»/libraries/libc.so",
    "root»/libraries/libm.so",
    "root»/libraries/libcairo.so",
    "root»/libraries/libpixman-1.so",
    "root»/libraries/libz.so.1",
    "root»/libraries/libfontconfig.so",
    "root»/libraries/libfreetype.so",
    "root»/libraries/libexpat.so",
    "root»/libraries/libpng15.so",
    0
};

Semaphore g_Preloads(0);

int preloadThread(void *p)
{
    const char *s = (const char *) p;

    NOTICE("PRELOAD: " << s);

    File* pFile = VFS::instance().find(String(s));
    if(pFile)
    {
        uintptr_t buffer = 0;
        MemoryMappedFile *pMmFile = MemoryMappedFileManager::instance().map(pFile, buffer);

        NOTICE("PRELOAD: preloading " << s << "...");
        size_t sz = 0;
        while(sz < pFile->getSize())
        {
            char buf[4096];
            char test = *reinterpret_cast<char*>(buffer + sz);
            sz += 0x1000;

            Scheduler::instance().yield();

        }

        MemoryMappedFileManager::instance().unmap(pMmFile);
    }

    NOTICE("PRELOAD: preload " << s << " has completed.");
    g_Preloads.release();

    return 0;
}

static void init()
{
    size_t n = 0;
    const char *s = g_FilesToPreload[n++];
    do
    {
        NOTICE("PRELOAD: Queue " << s);
        new Thread(Processor::information().getCurrentThread()->getParent(), preloadThread, const_cast<char*>(s));
        s = g_FilesToPreload[n++];
    } while(s);

    g_Preloads.acquire(n - 1);
    NOTICE("PRELOAD: preloaded " << n << " files.");
}

static void destroy()
{
}

MODULE_INFO("File Cache Preload", &init, &destroy, "vfs", "init");
