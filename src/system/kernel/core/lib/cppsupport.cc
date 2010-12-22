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

#include <processor/types.h>
#include <Spinlock.h>
#include "cppsupport.h"
#include <panic.h>
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <Log.h>

#include <utilities/Tree.h>

Tree<void*, void*> g_FreedPointers;

#include "SlamAllocator.h"

/// If the debug allocator is enabled, this switches it into underflow detection
/// mode.
#define DEBUG_ALLOCATOR_CHECK_UNDERFLOWS

// Required for G++ to link static init/destructors.
void *__dso_handle;

// Defined in the linker.
extern uintptr_t start_ctors;
extern uintptr_t end_ctors;

#ifdef USE_DEBUG_ALLOCATOR
Spinlock allocLock;
uintptr_t heapBase = 0x60000000; // 0xC0000000;
#endif

/// Calls the constructors for all global objects.
/// Call this before using any global objects.
void initialiseConstructors()
{
  // Constructor list is defined in the linker script.
  // The .ctors section is just an array of function pointers.
  // iterate through, calling each in turn.
  uintptr_t *iterator = reinterpret_cast<uintptr_t*>(&start_ctors);
  while (iterator < reinterpret_cast<uintptr_t*>(&end_ctors))
  {
    void (*fp)(void) = reinterpret_cast<void (*)(void)>(*iterator);
    fp();
    iterator++;
  }
}

/// Required for G++ to compile code.
extern "C" void __cxa_atexit(void (*f)(void *), void *p, void *d)
{
}

/// Called by G++ if a pure virtual function is called. Bad Thing, should never happen!
extern "C" void __cxa_pure_virtual()
{
    FATAL_NOLOCK("Pure virtual function call made");
}

/// Called by G++ if function local statics are initialised for the first time
extern "C" int __cxa_guard_acquire()
{
  return 1;
}
extern "C" void __cxa_guard_release()
{
  // TODO
}

extern "C" void *malloc(size_t sz)
{
    return reinterpret_cast<void *>(new uint8_t[sz]); //SlamAllocator::instance().allocate(sz));
}

extern "C" void free(void *p)
{
    if (p == 0)
        return;
    //SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
    delete reinterpret_cast<uint8_t*>(p);
}

extern "C" void *realloc(void *p, size_t sz)
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
    memcpy(tmp, p, copySz);
    free(p);

    return tmp;
}

void *operator new (size_t size) throw()
{
#ifdef USE_DEBUG_ALLOCATOR
    
    /// \todo underflow flag

    // Grab the size of the SlamAllocator header and footer
    size_t slamHeader = SlamAllocator::instance().headerSize();
    size_t slamFooter = SlamAllocator::instance().footerSize();

    // Find the number of pages we need for this allocation. Make sure that
    // we get a full page for our allocation if we need it.
    size_t nPages = ((size + slamHeader + slamFooter) / 0x1000) + 2;
    size_t blockSize = nPages * 0x1000;

    // Allocate the space
    uintptr_t ret = SlamAllocator::instance().allocate(blockSize);

    //NOTICE_NOLOCK("ret = " << ret << " [" << (ret + blockSize) << "]");

    // Calculate the offset at which we will return a data pointer
    uintptr_t unmapAddress = (ret & ~0xFFF) + (blockSize - 0x1000);
    //NOTICE_NOLOCK("unmap address is " << unmapAddress);
    uintptr_t dataPointer = unmapAddress - (size + slamFooter);
    //NOTICE_NOLOCK("Data pointer at " << dataPointer << ", size = " << size << " [" << blockSize << "]");

    // Okay, now the header and footer are at completely incorrect locations.
    // Let's resolve that now.
    void *header = reinterpret_cast<void*>(ret - slamHeader);
    void *targetLoc = reinterpret_cast<void*>(dataPointer - slamHeader);

    //NOTICE_NOLOCK("old header at " << reinterpret_cast<uintptr_t>(header));
    //NOTICE_NOLOCK("copied to " << reinterpret_cast<uintptr_t>(targetLoc));

    memcpy(targetLoc, header, slamHeader);
    void *footer = reinterpret_cast<void*>(ret + blockSize);
    targetLoc = reinterpret_cast<void*>(unmapAddress - slamFooter);

    //NOTICE_NOLOCK("old footer at " << reinterpret_cast<uintptr_t>(footer));
    //NOTICE_NOLOCK("copied to " << reinterpret_cast<uintptr_t>(targetLoc));

    memcpy(targetLoc, footer, slamFooter);

    //FATAL_NOLOCK("k");


    /// \todo unmap a page for overflow checks

    // All done, return the address
    return reinterpret_cast<void*>(dataPointer);
    
#if 0
    
    // Make the last page non-writable (but allow it to be read)
    /// \todo Flag for this behaviour - perhaps want to look for read overflows too?
    physical_uintptr_t phys = 0; size_t flags = 0;
    void *remapStart = reinterpret_cast<void*>(remapPoint);
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    /*
    if(va.isMapped(remapStart)) // Pedantically check for mapping
    {
        va.getMapping(remapStart, phys, flags);
        va.unmap(remapStart);
        va.map(phys, remapStart, VirtualAddressSpace::KernelMode);
    }
    */

    // NOTICE_NOLOCK("sz=" << size << ", alloc=" << ret << ", ptr=" << dataPointer << ", remap=" << remapPoint);
    return reinterpret_cast<void*>(dataPointer);

#endif

#elif defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON) || defined(ARM_COMMON)
    void *ret = reinterpret_cast<void *>(SlamAllocator::instance().allocate(size));
    return ret;
#else
    return 0;
#endif
}
void *operator new[] (size_t size) throw()
{
#ifdef USE_DEBUG_ALLOCATOR
    
    /// \todo underflow flag

    // Grab the size of the SlamAllocator header and footer
    size_t slamHeader = SlamAllocator::instance().headerSize();
    size_t slamFooter = SlamAllocator::instance().footerSize();

    // Find the number of pages we need for this allocation. Make sure that
    // we get a full page for our allocation if we need it.
    size_t nPages = ((size + slamHeader + slamFooter) / 0x1000) + 2;
    size_t blockSize = nPages * 0x1000;

    // Allocate the space
    uintptr_t ret = SlamAllocator::instance().allocate(blockSize);

    //NOTICE_NOLOCK("ret = " << ret << " [" << (ret + blockSize) << "]");

    // Calculate the offset at which we will return a data pointer
    uintptr_t unmapAddress = (ret & ~0xFFF) + (blockSize - 0x1000);
    //NOTICE_NOLOCK("unmap address is " << unmapAddress);
    uintptr_t dataPointer = unmapAddress - (size + slamFooter);
    //NOTICE_NOLOCK("Data pointer at " << dataPointer << ", size = " << size << " [" << blockSize << "]");

    // Okay, now the header and footer are at completely incorrect locations.
    // Let's resolve that now.
    void *header = reinterpret_cast<void*>(ret - slamHeader);
    void *targetLoc = reinterpret_cast<void*>(dataPointer - slamHeader);

    //NOTICE_NOLOCK("old header at " << reinterpret_cast<uintptr_t>(header));
    //NOTICE_NOLOCK("copied to " << reinterpret_cast<uintptr_t>(targetLoc));

    memcpy(targetLoc, header, slamHeader);
    void *footer = reinterpret_cast<void*>(ret + blockSize);
    targetLoc = reinterpret_cast<void*>(unmapAddress - slamFooter);

    //NOTICE_NOLOCK("old footer at " << reinterpret_cast<uintptr_t>(footer));
    //NOTICE_NOLOCK("copied to " << reinterpret_cast<uintptr_t>(targetLoc));

    memcpy(targetLoc, footer, slamFooter);

    //FATAL_NOLOCK("k");


    /// \todo unmap a page for overflow checks

    // All done, return the address
    return reinterpret_cast<void*>(dataPointer);

#elif defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON) || defined(ARM_COMMON)
    void *ret = reinterpret_cast<void *>(SlamAllocator::instance().allocate(size));
    return ret;
#else
    return 0;
#endif
}
void *operator new (size_t size, void* memory) throw()
{
  return memory;
}
void *operator new[] (size_t size, void* memory) throw()
{
  return memory;
}
void operator delete (void * p)
{
#ifdef USE_DEBUG_ALLOCATOR
    if(p == 0) return;

    // The pointer will be offset into the first page, so make sure that it's
    // properly aligned against a page boundary so we can free it. Maybe.
    uintptr_t temp = reinterpret_cast<uintptr_t>(p);
    temp = (temp & ~0xFFF) + 0x8; /// \bug Hard-coded size of SlamAllocator header
    SlamAllocator::instance().free(temp);

    return;
#endif

#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON) || defined(ARM_COMMON)
    if (p == 0) return;
    if(SlamAllocator::instance().isPointerValid(reinterpret_cast<uintptr_t>(p)))
        SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
#endif
}
void operator delete[] (void * p)
{
#ifdef USE_DEBUG_ALLOCATOR
    if(p == 0) return;

    // The pointer will be offset into the first page, so make sure that it's
    // properly aligned against a page boundary so we can free it. Maybe.
    uintptr_t temp = reinterpret_cast<uintptr_t>(p);
    temp = (temp & ~0xFFF) + 0x8; /// \bug Hard-coded size of SlamAllocator header
    SlamAllocator::instance().free(temp);

    return;
#endif

#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON) || defined(ARM_COMMON)
    if (p == 0) return;
    if(SlamAllocator::instance().isPointerValid(reinterpret_cast<uintptr_t>(p)))
        SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
#endif
}
void operator delete (void *p, void *q)
{
  // TODO
  panic("Operator delete -implement");
}
void operator delete[] (void *p, void *q)
{
  // TODO
  panic("Operator delete[] -implement");
}

#ifdef ARMV7

extern "C"
{

/** Atomic compare and swap operation... or as close as we can get to it. */
bool __sync_bool_compare_and_swap_4(void *ptr, void *oldval, void *newval)
{
    unsigned int notequal = 0, notexclusive = 0;
    
    asm volatile("dmb"); // Memory barrier
    
    do
    {
        asm volatile("ldrex     %0, [%2]\r\n"     // Load current value at &ptr
                     "subs      %0, %0, %3\r\n"   // Subtract by oldval...
                     "mov       %1, %0\r\n"       // ... so notequal will be zero if equal
                     "strexeq   %0, %4, [%2]"     // Write back the result
                     : "=&r" (notexclusive), "=&r" (notequal)
                     : "r" (&ptr), "Ir" (oldval), "r" (newval)
                     : "cc");
    } while(notexclusive && !notequal);
    
    asm volatile("dmb"); // Memory barrier
    
    return !notequal;
}

}

#endif

