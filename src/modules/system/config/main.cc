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
#include "sqlite3.h"
#include <utilities/utility.h>
#include <BootstrapInfo.h>
#include <panic.h>

#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/MemoryRegion.h>

#include <linker/KernelElf.h>

#include "Config.h"

extern BootstrapStruct_t *g_pBootstrapInfo;

uint8_t *g_pFile = 0;
size_t g_FileSz = 0;

extern "C" void log_(unsigned long a)
{
    NOTICE("Int: " << a);
}

extern "C" int atoi(const char *str)
{
    return strtoul(str, 0, 10);
}

#ifndef STATIC_DRIVERS // Defined in the kernel.
extern "C" int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *c1 = reinterpret_cast<const unsigned char*>(s1);
    const unsigned char *c2 = reinterpret_cast<const unsigned char*>(s2);
    for (size_t i = 0; i < n; i++)
        if (c1[i] != c2[i])
            return (c1[i]>c2[i]) ? 1 : -1;
    return 0;
}
#endif

int xClose(sqlite3_file *file)
{
    return 0;
}

int xRead(sqlite3_file *file, void *ptr, int iAmt, sqlite3_int64 iOfst)
{
    int ret = 0;
    if ((iOfst + static_cast<unsigned int>(iAmt)) >= g_FileSz)
    {
        if(static_cast<unsigned int>(iAmt) > g_FileSz)
            iAmt = g_FileSz;
        memset(ptr, 0, iAmt);
        iAmt = g_FileSz - iOfst;
        ret = SQLITE_IOERR_SHORT_READ;
    }
    memcpy(ptr, &g_pFile[iOfst], iAmt);
    return ret;
}

int xReadFail(sqlite3_file *file, void *ptr, int iAmt, sqlite3_int64 iOfst)
{
    memset(ptr, 0, iAmt);
    return SQLITE_IOERR_SHORT_READ;
}

int xWrite(sqlite3_file *file, const void *ptr, int iAmt, sqlite3_int64 iOfst)
{
    // Write past the end of the file?
    if((iOfst + static_cast<unsigned int>(iAmt)) >= g_FileSz)
    {
        // How many extra bytes do we need?
        size_t nNewSize = iOfst + iAmt + 1; // We know the read crosses the EOF, so this is correct

        // Allocate it, zero, and copy
        uint8_t *tmp = new uint8_t[nNewSize];
        memset(tmp, 0, nNewSize - 1);
        memcpy(tmp, g_pFile, g_FileSz);
        
        delete [] g_pFile;
        
        g_pFile = tmp;
        g_FileSz = nNewSize - 1;
    }

    memcpy(&g_pFile[iOfst], ptr, iAmt);
    return 0;
}

int xWriteFail(sqlite3_file *file, const void *ptr, int iAmt, sqlite3_int64 iOfst)
{
    return 0;
}

int xTruncate(sqlite3_file *file, sqlite3_int64 size)
{
    return 0;
}

int xSync(sqlite3_file *file, int flags)
{
    return 0;
}

int xFileSize(sqlite3_file *file, sqlite3_int64 *pSize)
{
    *pSize = g_FileSz;
    return 0;
}

int xLock(sqlite3_file *file, int a)
{
    return 0;
}

int xUnlock(sqlite3_file *file, int a)
{
    return 0;
}

int xCheckReservedLock(sqlite3_file *file, int *pResOut)
{
    *pResOut = 0;
    return 0;
}

int xFileControl(sqlite3_file *file, int op, void *pArg)
{
    return 0;
}

int xSectorSize(sqlite3_file *file)
{
    return 1;
}

int xDeviceCharacteristics(sqlite3_file *file)
{
    return 0;
}

static struct sqlite3_io_methods theio = 
{
    1,
    &xClose,
    &xRead,
    &xWrite,
    &xTruncate,
    &xSync,
    &xFileSize,
    &xLock,
    &xUnlock,
    &xCheckReservedLock,
    &xFileControl,
    &xSectorSize,
    &xDeviceCharacteristics
};

static struct sqlite3_io_methods theio_fail = 
{
    1,
    &xClose,
    &xReadFail,
    &xWriteFail,
    &xTruncate,
    &xSync,
    &xFileSize,
    &xLock,
    &xUnlock,
    &xCheckReservedLock,
    &xFileControl,
    &xSectorSize,
    &xDeviceCharacteristics
};


int xOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *file, int flags, int *pOutFlags)
{
    if (strcmp(zName, "root»/.pedigree-root"))
    {
        // Assume journal file, return failure functions.
        file->pMethods = &theio_fail;
        return 0;
    }

    if (!g_pBootstrapInfo->isDatabaseLoaded())
    {
        FATAL("Config database not loaded!");
    }

    file->pMethods = &theio;
    return 0;
}

int xDelete(sqlite3_vfs *vfs, const char *zName, int syncDir)
{
    return 0;
}

int xAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut)
{
    return 0;
}

int xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut)
{
    strncpy(zOut, zName, nOut);
    return 0;
}

void *xDlOpen(sqlite3_vfs *vfs, const char *zFilename)
{
    return 0;
}

void xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg)
{
}

void (*xDlSym(sqlite3_vfs *vfs, void *p, const char *zSymbol))(void)
{
    return 0;
}

void xDlClose(sqlite3_vfs *vfs, void *v)
{
}

int xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut)
{
    return 0;
}

int xSleep(sqlite3_vfs *vfs, int microseconds)
{
    return 0;
}

int xCurrentTime(sqlite3_vfs *vfs, double *)
{
    return 0;
}

int xGetLastError(sqlite3_vfs *vfs, int i, char *c)
{
    return 0;
}



static struct sqlite3_vfs thevfs =
{
    1,
    sizeof(void*),
    32,
    0,
    "no-vfs",
    0,
    &xOpen,
    &xDelete,
    &xAccess,
    &xFullPathname,
    &xDlOpen,
    &xDlError,
    &xDlSym,
    &xDlClose,
    &xRandomness,
    &xSleep,
    &xCurrentTime,
    &xGetLastError
};

int sqlite3_os_init()
{
    sqlite3_vfs_register(&thevfs, 1);
    return 0;
}

int sqlite3_os_end()
{
    return 0;
}

void xCallback0(sqlite3_context *context, int n, sqlite3_value **values)
{
    const unsigned char *text = sqlite3_value_text(values[0]);

    if (!text) return;

    uintptr_t x;
    if (text[0] == '0')
    {
        x = strtoul(reinterpret_cast<const char*>(text), 0, 16);
    }
    else
    {
        x = KernelElf::instance().lookupSymbol(reinterpret_cast<const char*>(text));
        if (!x)
        {
            ERROR("Couldn't trigger callback `" << reinterpret_cast<const char*>(text) << "': symbol not found.");
            return;
        }
    }
    
    void (*func)(void) = reinterpret_cast< void (*)(void) >(x);
    func();
    sqlite3_result_int(context, 0);
}

void xCallback1(sqlite3_context *context, int n, sqlite3_value **values)
{
    const char *text = reinterpret_cast<const char*> (sqlite3_value_text(values[0]));

    if (!text) return;

    uintptr_t x;
    if (text[0] == '0')
    {
        x = strtoul(text, 0, 16);
    }
    else
    {
        x = KernelElf::instance().lookupSymbol(text);
        if (!x)
        {
            ERROR("Couldn't trigger callback `" << text << "': symbol not found.");
            return;
        }
    }
    
    void (*func)(const char *) = reinterpret_cast< void (*)(const char*) >(x);
    func(reinterpret_cast<const char*>(sqlite3_value_text(values[1])));
    sqlite3_result_int(context, 0);
}

void xCallback2(sqlite3_context *context, int n, sqlite3_value **values)
{
    const char *text = reinterpret_cast<const char*> (sqlite3_value_text(values[0]));

    if (!text) return;

    uintptr_t x;
    if (text[0] == '0')
    {
        x = strtoul(text, 0, 16);
    }
    else
    {
        x = KernelElf::instance().lookupSymbol(text);
        if (!x)
        {
            ERROR("Couldn't trigger callback `" << text << "': symbol not found.");
            return;
        }
    }
    
    void (*func)(const char *, const char*) = reinterpret_cast< void (*)(const char*,const char*) >(x);
    func(reinterpret_cast<const char*>(sqlite3_value_text(values[1])), reinterpret_cast<const char*>(sqlite3_value_text(values[2])));
    sqlite3_result_int(context, 0);
}

sqlite3 *g_pSqlite = 0;

#ifdef STATIC_DRIVERS
#include "config_database.h"
#endif

static void init()
{
#ifndef STATIC_DRIVERS
    if (!g_pBootstrapInfo->isDatabaseLoaded())
        FATAL("Database not loaded, cannot continue.");

    uint8_t *pPhys = g_pBootstrapInfo->getDatabaseAddress();
    size_t sSize = g_pBootstrapInfo->getDatabaseSize();

    if ((reinterpret_cast<physical_uintptr_t>(pPhys) & (PhysicalMemoryManager::getPageSize() - 1)) != 0)
        panic("Config: Alignment issues");

    MemoryRegion region("Config");

    if (PhysicalMemoryManager::instance().allocateRegion(region,
                                                         (sSize + PhysicalMemoryManager::getPageSize() - 1) / PhysicalMemoryManager::getPageSize(),
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::KernelMode,
                                                         reinterpret_cast<physical_uintptr_t>(pPhys))
        == false)
    {
        ERROR("Config: allocateRegion failed.");
    }

    g_pFile = new uint8_t[sSize];
    memcpy(g_pFile, region.virtualAddress(), sSize);
    g_FileSz = sSize;
    NOTICE("region.va: " << (uintptr_t)region.virtualAddress());
#else
    g_pFile = file;
    g_FileSz = sizeof file;
#endif
    sqlite3_initialize();
    NOTICE("Initialize fin");
    int ret = sqlite3_open("root»/.pedigree-root", &g_pSqlite);
    NOTICE("Open fin");
    if (ret)
    {
        FATAL("sqlite3 error: " << sqlite3_errmsg(g_pSqlite));
    }

    sqlite3_create_function(g_pSqlite, "pedigree_callback", 1, SQLITE_ANY, 0, &xCallback0, 0, 0);
    sqlite3_create_function(g_pSqlite, "pedigree_callback", 2, SQLITE_ANY, 0, &xCallback1, 0, 0);
    sqlite3_create_function(g_pSqlite, "pedigree_callback", 3, SQLITE_ANY, 0, &xCallback2, 0, 0);
}

static void destroy()
{
}

MODULE_INFO("config", &init, &destroy);
