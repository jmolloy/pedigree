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
#ifndef MIPS_COMMON_ASIDMANAGER_H
#define MIPS_COMMON_ASIDMANAGER_H

#include <processor/types.h>
#include <process/Mutex.h>

/** @addtogroup kernelprocessorMIPSCommon
 * @{ */

#define NUM_ASID 256

/** MIPS TLB entries are marked with an ASID - an address space identifier.
 *  There are only 256 of these available (on the R4000) so they have to be
 *  managed in some way.
 *
 *  We have the policy that all processes in the "Ready" or "Running" state have
 *  ASIDs assigned. When a thread goes to sleep it must relinquish its ASID.
 *
 *  There is a use case where we have >256 running processes - This is handled
 *  by every process calling our function 'bulldoze' just before it is scheduled.
 *  Normally 'bulldoze' will do nothing, but in the case where there is contention
 *  for its ASID the entire TLB will be flushed of any entry containing that ASID.
 */
class AsidManager
{
public:
  /** ASID typedef */
  typedef uint8_t Asid;

  /** Gets the instance of the AsidManager */
  static AsidManager &instance();

  /** Returns an ASID. The TLB is flushed of all references to that ASID. */
  Asid obtainAsid();
  /** Takes an ASID. The given ASID is returned to the pool of usable ASIDs. */
  void returnAsid(Asid asid);

  /** If the ASID given is contended, flush the TLB of all references to it. */
  void bulldoze(Asid asid);

private:
  /** Default constructor */
  AsidManager();
  /** Destructor - not implemented. */
  ~AsidManager();

  /** The array of ASID to instance count. */
  uint32_t m_Asids[NUM_ASID];

  /** Our lock, to protect the integrity of our data. */
  Mutex m_Mutex;

  /** The static AsidManager instance - singleton class. */
  static AsidManager m_Instance;
};
/** @} */

#endif
