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
#ifndef CACHE_H
#define CACHE_H

#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>
#include <utilities/MemoryAllocator.h>
#include <utilities/List.h>
#include <utilities/Tree.h>
#include <process/Mutex.h>
#include <LockGuard.h>

#include <processor/PhysicalMemoryManager.h>

template <class T, size_t N=sizeof(T)>
class Cache;

/** A cache page - Page given to the cache with allocator and
 *  usage count. */
class CachePage
{
public:
  /** Constructor */
  CachePage (physical_uintptr_t page) :
    m_Allocator(), m_Page(page), m_nAccessed(0)
  {
    m_Allocator.free(0, getSize());
  }
  /** Destructor */
  ~CachePage ()
  {
  }

  /** Returns the size of the page. */
  static size_t getSize()
  {
    return PhysicalMemoryManager::instance().getPageSize();
  }

  /** Allocates memory within the page. */
  MemoryAllocator m_Allocator;
  /** Physical page location. */
  physical_uintptr_t m_Page;
  /** Access count. */
  size_t m_nAccessed;
};

/** Provides an abstraction of a data cache. */
template <class T, size_t N>
class Cache<T*,N>
{
public:

  Cache() :
    m_KeyItemMap(), m_KeyPageMap(), m_ReadyPages(), m_FullPages(), m_Lock(false)
  {
    if (N >= PhysicalMemoryManager::instance().getPageSize())
      FATAL("CACHE: Cannot instantiate with sizeof template parameter > page size!");
  }
  virtual ~Cache()
  {
    for (List<CachePage*>::Iterator it = m_ReadyPages.begin();
         it != m_ReadyPages.end();
         it++)
    {
      PhysicalMemoryManager::instance().freePage ((*it)->m_Page);
      delete *it;
    }
    for (List<CachePage*>::Iterator it = m_FullPages.begin();
         it != m_FullPages.end();
         it++)
    {
      PhysicalMemoryManager::instance().freePage ((*it)->m_Page);
      delete *it;
    }
    
    m_KeyItemMap.clear();
    m_KeyPageMap.clear();
    m_ReadyPages.clear();
    m_FullPages.clear();
  }
  
  /** Attempts to look up an item from the cache. Puts the item in
   *  pItem if found and returns true, or false otherwise. */
  bool lookup (uintptr_t key, T *pItem)
  {
    LockGuard<Mutex> guard(m_Lock);

    if (!pItem)
    {
      WARNING("CACHE: lookup with NULL out parameter.");
      return false;
    }
  
    T *pT = m_KeyItemMap.lookup(key);
    if (pT)
    {
      CachePage *pCp = m_KeyPageMap.lookup(key);
      pCp->m_nAccessed ++;
  
      MemoryRegion mr("Temporary caching region");
      if (!PhysicalMemoryManager::instance().allocateRegion(mr,
                                                            1,
                                                            PhysicalMemoryManager::continuous,
                                                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                                                            reinterpret_cast<physical_uintptr_t>(pT) & ~(PhysicalMemoryManager::instance().getPageSize()-1)))
      {
        FATAL("CACHE: Failed to create specific temporary memory region");
      }

      memcpy(reinterpret_cast<void*>(pItem),
             mr.convertPhysicalPointer<void*>(reinterpret_cast<physical_uintptr_t>(pT)),
             N);
      return true;
    }

    return false;
  }

  /** Inserts an item into the cache, with the given key. The
   *  item is copied. */
  void insert (uintptr_t key, T *pItem)
  {
    // Grab the lock.
    m_Lock.acquire();

    T *pT = m_KeyItemMap.lookup(key);
    if (pT)
    {
      // Item already exists, so copy the new value into where the old one was.
      MemoryRegion mr("Temporary caching region");
      if (!PhysicalMemoryManager::instance().allocateRegion(mr,
                                                            1,
                                                            PhysicalMemoryManager::continuous,
                                                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                                                            reinterpret_cast<physical_uintptr_t>(pT) & ~(PhysicalMemoryManager::instance().getPageSize()-1)))
      {
        FATAL("CACHE: Failed to create specific temporary memory region");
      }
      memcpy(mr.convertPhysicalPointer<void*>(reinterpret_cast<physical_uintptr_t>(pT)),
             reinterpret_cast<void*>(pItem),
             N);
  
      // Don't need to unregister the memory region - will destroy when it goes out of scope.
      m_Lock.release();
      return;
    }
  
    physical_uintptr_t address = 0;
    // Item doesn't exist, so attempt to find some free space in the ready list.
    for (List<CachePage*>::Iterator it = m_ReadyPages.begin();
         it != m_ReadyPages.end();
         it++)
    {
      CachePage *pCp = *it;
      // Try and allocate some space in this page.
      if (pCp->m_Allocator.allocate (N, address))
      {
        // This page contained enough memory, so add it to the KeyPageMap.
        m_KeyPageMap.insert(key, pCp);
        // ... and also update address to reflect the true physical address.
        address += pCp->m_Page;
        break;
      }
      else
      {
        // This page should be thrown into the full pages bin.
        m_ReadyPages.erase (it);
        m_FullPages.pushBack (pCp);
        // Restart the iterator so that it is still valid. 
        it = m_ReadyPages.begin ();

        // As soon as we loop again, 'it' will be incremented. So check here if
        // it is invalid.
        if (it == m_ReadyPages.end())
          break;
      }
    }
  
    if (address == 0)
    {
      // No memory available, grab some more.
      
      // Relinquish the lock - if we're nearing full physical memory usage then the PMM may
      // call back to us to compact - in which case we'll need to grab the lock. Also, at this point
      // anything can happen to the contents of this cache without affecting us.
      m_Lock.release();
      physical_uintptr_t page = PhysicalMemoryManager::instance().allocatePage();
      m_Lock.acquire();
  
      // Locked again, now we can perform an update.
      if (page == 0)
      {
        // No physical memory available to us, print a warning and fail.
        WARNING("CACHE: No physical memory available, insertion failing.");
        m_Lock.release();
        return;
      }
      
      // Valid page, create a CachePage object.
      CachePage *pCp = new CachePage(page);
  
      // Add it to the free list.
      m_ReadyPages.pushBack(pCp);
      // And to the key-page map.
      m_KeyPageMap.insert(key, pCp);
  
      // Set address.
      if (!pCp->m_Allocator.allocate(N, address))
      {
        // Wow, we just allocated something but it can't allocate anything. Something went very wrong.
        // It's appropriate to panic here.
        FATAL("CACHE: Allocator created but unable to allocate!");
        m_Lock.release();
        return;
      }
      // Adjust address to be a true physical address.
      address += page;
    }

    // At this point address is valid, so we can map it in and copy over.
    MemoryRegion mr("Temporary caching region (2)");
    if (!PhysicalMemoryManager::instance().allocateRegion(mr,
                                                          1,
                                                          PhysicalMemoryManager::continuous,
                                                          VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                                                          address & ~(PhysicalMemoryManager::instance().getPageSize()-1)))
    {
      FATAL("CACHE: Failed to create specific temporary memory region (2)");
    }
    memcpy(mr.convertPhysicalPointer<void*>(address),
           reinterpret_cast<void*>(pItem),
           N);
  
    // Don't need to unregister the memory region - will destroy when it goes out of scope.
  
    // All done, just need to add to the tree now!
    m_KeyItemMap.insert(key, reinterpret_cast<T*> (address));

    m_Lock.release();
  }
  
  /** Attempts to "compact" the cache - (hopefully) reduces
   *  resource usage by throwing away items in a
   *  least-recently-used fashion. This is called in an
   *  emergency "physical memory getting full" situation by the
   *  PMM. */
  void compact ()
  {
    /// \todo Do this.
  }

private:

  /** Key-item pairs. */
  Tree<uintptr_t, T*> m_KeyItemMap;
  
  /** Key-page pairs. */
  Tree<uintptr_t, CachePage*> m_KeyPageMap;

  /** Physical pages allocated to the cache that have yet to be
   *  filled fully. */
  List<CachePage*> m_ReadyPages;

  /** Physical pages allocated to the cache that are full. */
  List<CachePage*> m_FullPages;

  /** Lock for this cache.
   *  \todo Change this to a readers/writers solution. */
  Mutex m_Lock;
};

#endif
