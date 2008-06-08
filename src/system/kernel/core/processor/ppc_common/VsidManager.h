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

#ifndef PPC_COMMON_VSIDMANAGER_H
#define PPC_COMMON_VSIDMANAGER_H

#include <processor/types.h>
#include <process/Mutex.h>

/** @addtogroup kernelprocessorPPCCommon
 * @{ */

/**
 * PPC has too many possible VSID's to do the same thing as MIPS' ASIDs,
 * so we have to box a little more cleverly.
 *
 * We have a pointer marking the 'high water mark' of the VSID's already allocated.
 * We also have a stack of VSIDs - when a VSID is released it is pushed onto
 * this stack to be reallocated. If the VSID released == the high water mark,
 * we don't push it and just decrement the high water mark instead.
 * 
 * When a VSID is required, we try and pop one off the stack first. If the stack is
 * empty, we increment the high water mark and return that.
 */
class VsidManager
{
public:
  /** VSID typedef */
  typedef uint32_t Vsid;

  /** Gets the instance of the VsidManager */
  static VsidManager &instance();

  /** Returns an VSID. The TLB is flushed of all references to that ASID. */
  Vsid obtainVsid();
  /** Takes an VSID. The given VSID is returned to the pool of usable VSIDs. */
  void returnVsid(Vsid vsid);

private:
  /** Stack entry type */
  struct VsidStack
  {
    Vsid vsid;
    struct VsidStack *next;
  };
  /** Default constructor */
  VsidManager();
  /** Destructor - not implemented. */
  ~VsidManager();

  /** Our 'high water mark' */
  Vsid m_HighWaterMark;
  
  /** Our stack of useable VSIDs */
  VsidStack *m_pStack;

  /** Our lock, to protect the integrity of our data. */
  Mutex m_Mutex;

  /** The static VsidManager instance - singleton class. */
  static VsidManager m_Instance;
};
/** @} */

#endif
