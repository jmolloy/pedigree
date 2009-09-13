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
#include <Log.h>

#include "SlamAllocator.h"

// Required for G++ to link static init/destructors.
void *__dso_handle;

// Defined in the linker.
uintptr_t start_ctors;
uintptr_t end_ctors;

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
    return reinterpret_cast<void *>(SlamAllocator::instance().allocate(sz));
}

extern "C" void free(void *p)
{
    if (p == 0)
        return;
    SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
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
    
    void *tmp = malloc(sz);
    memcpy(tmp, p, sz);
    free(p);

    return tmp;
}

void *operator new (size_t size) throw()
{
#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON)
  void *ret = reinterpret_cast<void *>(SlamAllocator::instance().allocate(size));
  return ret;
#else
  return 0;
#endif
}
void *operator new[] (size_t size) throw()
{
#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON)
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
#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON)
    if (p == 0) return;
  uintptr_t up = reinterpret_cast<uintptr_t>(p);
  if (up < 0xc0000000 || up > 0xd0000000)
  {
      ERROR_NOLOCK("Bad free " << Hex << up);
      Processor::breakpoint();
  }
  SlamAllocator::instance().free(reinterpret_cast<uintptr_t>(p));
#endif
}
void operator delete[] (void * p)
{
#if defined(X86_COMMON) || defined(MIPS_COMMON) || defined(PPC_COMMON)
    if (p == 0) return;

  uintptr_t up = reinterpret_cast<uintptr_t>(p);
  if (up < 0xc0000000 || up > 0xd0000000)
  {
      ERROR_NOLOCK("Bad free " << Hex << up);
      Processor::breakpoint();
  }

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
