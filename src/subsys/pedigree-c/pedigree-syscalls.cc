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
#include <config/Config.h>
#include <linker/KernelElf.h>
#include <vfs/File.h>
#include <vfs/MemoryMappedFile.h>
#include <vfs/Symlink.h>
#include <vfs/VFS.h>
#include <Log.h>
#include <syscallError.h>

#include <machine/InputManager.h>
#include <machine/KeymapManager.h>

#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include <users/UserManager.h>

#define PEDIGREEC_WITHIN_KERNEL
#include "pedigree-syscalls.h"

struct blitargs
{
    Graphics::Buffer *pBuffer;
    uint32_t srcx, srcy, destx, desty, width, height;
} PACKED;

struct drawargs
{
    void *a;
    uintptr_t b, c, d, e, f, g, h;
} PACKED;

struct createargs
{
    void *pFramebuffer;
    void *pReturnProvider;
    size_t x, y, w, h;
} PACKED;

struct fourargs
{
    uintptr_t a, b, c, d;
} PACKED;

struct sixargs
{
    uintptr_t a, b, c, d, e, f;
} PACKED;

#define MAX_RESULTS 32
static Config::Result *g_Results[MAX_RESULTS];

void pedigree_config_init()
{
    memset(g_Results, 0, sizeof(Config::Result*)*MAX_RESULTS);
}

void pedigree_config_getcolname(size_t resultIdx, size_t n, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getColumnName(n);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

void pedigree_config_getstr(size_t resultIdx, size_t row, size_t n, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getStr(row, n);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

void pedigree_config_getstr(size_t resultIdx, size_t row, const char *col, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getStr(row, col);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

int pedigree_config_getnum(size_t resultIdx, size_t row, size_t n)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(row, n));
}

int pedigree_config_getnum(size_t resultIdx, size_t row, const char *col)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(row, col));
}

int pedigree_config_getbool(size_t resultIdx, size_t row, size_t n)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getBool(row, n));
}

int pedigree_config_getbool(size_t resultIdx, size_t row, const char *col)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(row, col));
}

const char *pConfigPermissionError = "insufficient permissions: only root allowed";

int pedigree_config_query(const char *query)
{
    for (size_t i = 0; i < MAX_RESULTS; i++)
    {
        if (g_Results[i] == 0)
        {
            // Check for user performing the query: only root has config access
            if(Processor::information().getCurrentThread()->getParent()->getUser()->getId())
            {
                char *pError = new char [strlen(pConfigPermissionError) + 1];
                strcpy(pError, pConfigPermissionError);
                g_Results[i] = new Config::Result(0, 0, 0, pError, -1);
            }
            else
                g_Results[i] = Config::instance().query(query);

            return i;
        }
    }
    ERROR("Insufficient free resultsets.");
    return -1;
}

void pedigree_config_freeresult(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    delete g_Results[resultIdx];
    g_Results[resultIdx] = 0;
}

int pedigree_config_numcols(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;
    return g_Results[resultIdx]->cols();
}

int pedigree_config_numrows(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;
    return g_Results[resultIdx]->rows();
}

int pedigree_config_was_successful(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;
    return (g_Results[resultIdx]->succeeded()) ? 0 : -1;
}

void pedigree_config_get_error_message(size_t resultIdx, char *buf, int buflen)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    strncpy(buf, g_Results[resultIdx]->errorMessage(), buflen);
}

// Module handling functions

// Load a module
void pedigree_module_load(char *_file)
{
    // Attempt to find the file, first!
    File* file = VFS::instance().find(String(_file), Processor::information().getCurrentThread()->getParent()->getCwd());
    if (!file)
    {
        // Error - not found.
        SYSCALL_ERROR(DoesNotExist);
        return;
    }

    while (file->isSymlink())
        file = Symlink::fromFile(file)->followLink();

    if (file->isDirectory())
    {
        // Error - is directory.
        SYSCALL_ERROR(IsADirectory);
        return;
    }

    // Map the module in the memory
    uintptr_t buffer = 0;
    MemoryMappedObject *pMmFile = MemoryMapManager::instance().mapFile(file, buffer, file->getSize());
    KernelElf::instance().loadModule(reinterpret_cast<uint8_t*>(buffer), file->getSize(), true);
    MemoryMapManager::instance().unmap(pMmFile);
}

// Unload a module
void pedigree_module_unload(char *name)
{
    KernelElf::instance().unloadModule(name, true);
}

// Check if a module is loaded
int pedigree_module_is_loaded(char *name)
{
    return (KernelElf::instance().moduleIsLoaded(name)?1:0);
}

// Get the first module that depends on the specified module
int pedigree_module_get_depending(char *name, char *buf, size_t bufsz)
{
    char *dep = KernelElf::instance().getDependingModule(name);
    if(dep)
        strncpy(buf, dep, bufsz);
    else
        return 0;
    return 1;
}

void pedigree_input_install_callback(void *p, uint32_t type, uintptr_t param)
{
    // First parameter is now obsolete, gotta remove it some time...
    InputManager::instance().installCallback(
        static_cast<InputManager::CallbackType>(type),
        reinterpret_cast<InputManager::callback_t>(p),
        Processor::information().getCurrentThread(),
        param);
}

void pedigree_input_remove_callback(void *p)
{
    // First parameter is now obsolete, gotta remove it some time...
    InputManager::instance().removeCallback(
        reinterpret_cast<InputManager::callback_t>(p),
        Processor::information().getCurrentThread());
}

int pedigree_load_keymap(char *buf, size_t len)
{
    // File format:  0    Sparse tree offset
    //               4    Data tree offset
    //               ...  Sparse tree & data tree.

    uint32_t sparseTableOffset  = *reinterpret_cast<uint32_t*>(&buf[0]);
    uint32_t dataTableOffset    = *reinterpret_cast<uint32_t*>(&buf[4]);
    uint32_t sparseTableSize    = dataTableOffset - sparseTableOffset;
    uint32_t dataTableSize      = len - dataTableOffset;

    uint8_t *sparseTable = new uint8_t[sparseTableSize];
    memcpy(sparseTable, &buf[sparseTableOffset], sparseTableSize);

    uint8_t *dataTable = new uint8_t[dataTableSize];
    memcpy(dataTable, &buf[dataTableOffset], dataTableSize);

    KeymapManager::instance().useKeymap(sparseTable, dataTable);

    return 0;
}

int pedigree_gfx_get_provider(void *p)
{
    if(!p)
        return -1;

    GraphicsService::GraphicsProvider gfxProvider;

    // Grab the current graphics provider for the system, use it to display the
    // splash screen to the user.
    /// \todo Check for failure
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
    Service         *pService  = ServiceManager::instance().getService(String("graphics"));
    if(pFeatures->provides(ServiceFeatures::probe))
    {
        if(pService)
        {
            if(!pService->serve(ServiceFeatures::probe, reinterpret_cast<void*>(&gfxProvider), sizeof(gfxProvider)))
                return -1;
        }
        else
            return -1;
    }
    else
        return -1;

    memcpy(p, &gfxProvider, sizeof(gfxProvider));

    return 0;
}

int pedigree_gfx_get_curr_mode(void *p, void *sm)
{
    if(!p)
        return -1;

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    Display::ScreenMode mode;
    pProvider->pDisplay->getCurrentScreenMode(mode);

    memcpy(sm, &mode, sizeof(mode));

    return 0;
}

uintptr_t pedigree_gfx_get_raw_buffer(void *p)
{
    if(!p)
        return -1;

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    void *raw = pProvider->pFramebuffer->getRawBuffer();
    physical_uintptr_t ret = 0;

    // Return a physical address, the userspace application can mmap this
    // with MAP_PHYS_OFFSET. This allows us to avoid having to map in areas
    // of the kernel address space that the application can use.
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if(va.isMapped(raw))
    {
        NOTICE("hi");
        size_t flags = 0;
        va.getMapping(raw, ret, flags);
    }

    return ret;
}

int pedigree_gfx_create_buffer(void *p, void **b, void *args)
{
    if(!p)
        return -1;

    fourargs *pArgs = reinterpret_cast<fourargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    *b = pProvider->pFramebuffer->createBuffer(reinterpret_cast<void*>(pArgs->a), static_cast<Graphics::PixelFormat>(pArgs->b), pArgs->c, pArgs->d);

    return *b ? 0 : -1;
}

int pedigree_gfx_destroy_buffer(void *p, void *b)
{
    if(!p)
        return -1;

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->destroyBuffer(reinterpret_cast<Graphics::Buffer*>(b));

    return 0;
}

void pedigree_gfx_redraw(void *p, void *args)
{
    if(!p)
        return;

    sixargs *pArgs = reinterpret_cast<sixargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->redraw(pArgs->a, pArgs->b, pArgs->c, pArgs->d, pArgs->e);
}

void pedigree_gfx_blit(void *p, void *args)
{
    if(!p || !args)
        return;

    blitargs *pArgs = reinterpret_cast<blitargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->blit(pArgs->pBuffer, pArgs->srcx, pArgs->srcy, pArgs->destx, pArgs->desty, pArgs->width, pArgs->height);
}

void pedigree_gfx_set_pixel(void *p, uint32_t x, uint32_t y, uint32_t colour, uint32_t fmt)
{
    if(!p)
        return;

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->setPixel(x, y, colour, static_cast<Graphics::PixelFormat>(fmt));
}

void pedigree_gfx_rect(void *p, void *args)
{
    if(!p || !args)
        return;

    sixargs *pArgs = reinterpret_cast<sixargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->rect(pArgs->a, pArgs->b, pArgs->c, pArgs->d, pArgs->e, static_cast<Graphics::PixelFormat>(pArgs->f));
}

void pedigree_gfx_copy(void *p, void *args)
{
    if(!p || !args)
        return;

    sixargs *pArgs = reinterpret_cast<sixargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->copy(pArgs->a, pArgs->b, pArgs->c, pArgs->d, pArgs->e, pArgs->f);
}

void pedigree_gfx_line(void *p, void *args)
{
    if(!p || !args)
        return;

    sixargs *pArgs = reinterpret_cast<sixargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->line(pArgs->a, pArgs->b, pArgs->c, pArgs->d, pArgs->e, static_cast<Graphics::PixelFormat>(pArgs->f));
}

void pedigree_gfx_draw(void *p, void *args)
{
    if(!p || !args)
        return;

    drawargs *pArgs = reinterpret_cast<drawargs*>(args);

    /// \todo Exploit: could allow userspace code to be run at ring0
    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->draw(pArgs->a, pArgs->b, pArgs->c, pArgs->d, pArgs->e, pArgs->f, pArgs->g, static_cast<Graphics::PixelFormat>(pArgs->h));
}

int pedigree_gfx_create_fbuffer(void *p, void *args)
{
    if(!p || !args)
        return - 1;

    createargs *pArgs = reinterpret_cast<createargs*>(args);

    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);
    GraphicsService::GraphicsProvider *pReturnProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(pArgs->pReturnProvider);

    pReturnProvider->pFramebuffer = Graphics::createFramebuffer(pProvider->pFramebuffer, pArgs->x, pArgs->y, pArgs->w, pArgs->h, pArgs->pFramebuffer);
    if(!pReturnProvider->pFramebuffer)
        return -1;

    return 0;
}

void pedigree_gfx_delete_fbuffer(void *p)
{
    if(!p)
        return;

    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    Graphics::destroyFramebuffer(pProvider->pFramebuffer);
}

void pedigree_gfx_fbinfo(void *p, size_t *w, size_t *h, uint32_t *fmt, size_t *bypp)
{
    if(!p)
        return;

    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    if(w)
        *w = pProvider->pFramebuffer->getWidth();
    if(h)
        *h = pProvider->pFramebuffer->getHeight();
    if(fmt)
        *fmt = static_cast<uint32_t>(pProvider->pFramebuffer->getFormat());
    if(bypp)
        *bypp = pProvider->pFramebuffer->getBytesPerPixel();
}

void pedigree_gfx_setpalette(void* p, uint32_t *data, size_t entries)
{
    if(!p)
        return;

    GraphicsService::GraphicsProvider *pProvider = reinterpret_cast<GraphicsService::GraphicsProvider*>(p);

    pProvider->pFramebuffer->setPalette(data, entries);
}

void pedigree_event_return()
{
    // Return to the old code
    Processor::information().getScheduler().eventHandlerReturned();

    FATAL("event_return: should never get here");
}

void *pedigree_sys_request_mem(size_t len) {
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    uintptr_t mapAddress = 0;
    if(!pProcess->getSpaceAllocator().allocate(len, mapAddress)) {
        return 0;
    }

    return reinterpret_cast<void *>(mapAddress);
}
