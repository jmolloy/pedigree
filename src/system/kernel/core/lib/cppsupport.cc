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

#include <compiler.h>
#include <processor/types.h>
#include <Spinlock.h>
#include "cppsupport.h"
#include <panic.h>
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>
#include <Log.h>

#include <utilities/MemoryTracing.h>

#include "SlamAllocator.h"

/// If the debug allocator is enabled, this switches it into underflow detection
/// mode.
#define DEBUG_ALLOCATOR_CHECK_UNDERFLOWS

// Required for G++ to link static init/destructors.
#ifndef HOSTED
void *__dso_handle;
#endif

// Defined in the linker.
extern uintptr_t start_ctors;
extern uintptr_t end_ctors;
extern uintptr_t start_dtors;
extern uintptr_t end_dtors;

/// Calls the constructors for all global objects.
/// Call this before using any global objects.
void initialiseConstructors()
{
  // Constructor list is defined in the linker script.
  // The .ctors section is just an array of function pointers.
  // iterate through, calling each in turn.
  uintptr_t *iterator = &start_ctors;
  while (iterator < &end_ctors)
  {
    void (*fp)(void) = reinterpret_cast<void (*)(void)>(*iterator);
    fp();
    iterator++;
  }
}

void runKernelDestructors()
{
  uintptr_t *iterator = &start_dtors;
  while (iterator < &end_dtors)
  {
    void (*fp)(void) = reinterpret_cast<void (*)(void)>(*iterator);
    fp();
    iterator++;
  }
}

#ifdef MEMORY_TRACING
static bool traceAllocations = true;
void startTracingAllocations()
{
    traceAllocations = true;
}

void stopTracingAllocations()
{
    traceAllocations = false;
}

static volatile int g_TraceLock = 0;

void traceAllocation(void *ptr, MemoryTracing::AllocationTrace type, size_t size)
{
    // Don't trace if we're not allowed to.
    if (!traceAllocations)
        return;

    MemoryTracing::AllocationTraceEntry entry;
    entry.data.type = type;
    entry.data.sz = size & 0xFFFFFFFFU;
    entry.data.ptr = reinterpret_cast<uintptr_t>(ptr) & 0xFFFFFFFFU;

#define BT_FRAME(N) do { \
    if (!__builtin_frame_address(N)) \
    { \
        entry.data.bt[N] = 0; \
        break; \
    } \
    entry.data.bt[N] = reinterpret_cast<uintptr_t>(__builtin_return_address(N)) & 0xFFFFFFFFU; \
} while(0)

    BT_FRAME(0);
    BT_FRAME(1);
    BT_FRAME(2);
    BT_FRAME(3);
    BT_FRAME(4);

    __asm__ __volatile__("pushfq; cli" ::: "memory");

    for(size_t i = 0; i < sizeof entry.buf; ++i)
    {
        __asm__ __volatile__("outb %%al, %%dx" :: "Nd" (0x2F8), "a" (entry.buf[i]));
    }

    __asm__ __volatile__("popf" ::: "memory");
}

/**
 * Adds a metadata field to the memory trace.
 *
 * This is typically used to define the region in which a module has been
 * loaded, so the correct debug symbols can be loaded and used.
 */
void traceMetadata(NormalStaticString str, void *p1, void *p2)
{
    // this can be provided by scripts/addr2line.py these days
#if 0
    LockGuard<Spinlock> guard(traceLock);

    // Yes, this means we'll lose early init mallocs. Oh well...
    if(!Machine::instance().isInitialised())
        return;

    Serial *pSerial = Machine::instance().getSerial(1);
    if(!pSerial)
        return;

    char buf[128];
    ByteSet(buf, 0, 128);

    size_t off = 0;

    const MemoryTracing::AllocationTrace type = MemoryTracing::Metadata;

    MemoryCopy(&buf[off], &type, 1);
    ++off;
    MemoryCopy(&buf[off], static_cast<const char *>(str), str.length());
    off += 64; // Statically sized segment.
    MemoryCopy(&buf[off], &p1, sizeof(void*));
    off += sizeof(void*);
    MemoryCopy(&buf[off], &p2, sizeof(void*));
    off += sizeof(void*);

    for(size_t i = 0; i < off; ++i)
    {
        pSerial->write(buf[i]);
    }
#endif
}
#endif

#ifdef ARM_COMMON
#define ATEXIT __aeabi_atexit
#else
#define ATEXIT __cxa_atexit
#endif

/// Required for G++ to compile code.
extern "C" void ATEXIT(void (*f)(void *), void *p, void *d)
{
}

/// Called by G++ if a pure virtual function is called. Bad Thing, should never happen!
extern "C" void __cxa_pure_virtual() NORETURN;
void __cxa_pure_virtual()
{
    FATAL_NOLOCK("Pure virtual function call made");
}

/// Called by G++ if function local statics are initialised for the first time
#ifndef HAS_THREAD_SANITIZER
extern "C" int __cxa_guard_acquire()
{
  return 1;
}
extern "C" void __cxa_guard_release()
{
  // TODO
}
#endif

#if !(defined(HAS_ADDRESS_SANITIZER) || defined(HAS_THREAD_SANITIZER))
#ifdef HOSTED
extern "C" void *_malloc(size_t sz)
#else
extern "C" void *malloc(size_t sz)
#endif
{
    return reinterpret_cast<void *>(new uint8_t[sz]); //SlamAllocator::instance().allocate(sz));
}

#ifdef HOSTED
extern "C" void _free(void *p)
#else
extern "C" void free(void *p)
#endif
{
    if (p == 0)
        return;
    //SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
    delete [] reinterpret_cast<uint8_t*>(p);
}

#ifdef HOSTED
extern "C" void *_realloc(void *p, size_t sz)
#else
extern "C" void *realloc(void *p, size_t sz)
#endif
{
    if (p == 0)
        return malloc(sz);
    if (sz == 0)
    {
        free(p);
        return 0;
    }

    // Don't attempt to read past the end of the source buffer if we can help it
    size_t copySz = SlamAllocator::instance().allocSize(reinterpret_cast<uintptr_t>(p)) - sizeof(SlamAllocator::AllocFooter);
    if(copySz > sz)
        copySz = sz;
    
    /// \note If sz > p's original size, this may fail.
    void *tmp = malloc(sz);
    MemoryCopy(tmp, p, copySz);
    free(p);

    return tmp;
}

void *operator new (size_t size) noexcept
{
    void *ret = reinterpret_cast<void *>(SlamAllocator::instance().allocate(size));
    return ret;
}
void *operator new[] (size_t size) noexcept
{
    void *ret = reinterpret_cast<void *>(SlamAllocator::instance().allocate(size));
    return ret;
}
void *operator new (size_t size, void* memory) noexcept
{
  return memory;
}
void *operator new[] (size_t size, void* memory) noexcept
{
  return memory;
}
void operator delete (void * p) noexcept
{
    if (p == 0) return;
    if(SlamAllocator::instance().isPointerValid(reinterpret_cast<uintptr_t>(p)))
        SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
}
void operator delete[] (void * p) noexcept
{
    if (p == 0) return;
    if(SlamAllocator::instance().isPointerValid(reinterpret_cast<uintptr_t>(p)))
        SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
}
void operator delete (void *p, void *q) noexcept
{
  // TODO
  panic("Operator delete (placement) -implement");
}
void operator delete[] (void *p, void *q) noexcept
{
  // TODO
  panic("Operator delete[] (placement) -implement");
}
#endif

#if defined(HOSTED) && (!(defined(HAS_ADDRESS_SANITIZER) || defined(HAS_THREAD_SANITIZER)))
extern "C"
{

void *__wrap_malloc(size_t sz)
{
  return _malloc(sz);
}

void *__wrap_realloc(void *p, size_t sz)
{
  return _realloc(p, sz);
}

void __wrap_free(void *p)
{
  return _free(p);
}

}
#endif
