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
#ifndef KERNEL_MACHINE_X86_COMMON_SMP_H
#define KERNEL_MACHINE_X86_COMMON_SMP_H

#if defined(SMP)

#include <compiler.h>
#include <processor/types.h>
#include <utilities/Vector.h>
#include "../../core/processor/x86_common/Multiprocessor.h"

/** @addtogroup kernelmachinex86common
 * @{ */

/** Implementation of the Intel Multiprocessor Specification, see
 *  http://www.intel.com/design/pentium/datashts/242016.HTM for the
 *  Specification */
class Smp
{
  public:
    /** Get the instance of the Smp class */
    inline static Smp &instance()
      {return m_Instance;}

    /** Search for the tables and initialise internal data structures
     *\note the first MB of RAM must be identity mapped */
    void initialise() INITIALISATION_ONLY;
    /** Were valid tables found? */
    inline bool valid()
      {return m_bValid;}

    #if defined(APIC)
      /** Get the physical address of all local APICs
       *\return physical address of all local APICs */
      inline uint64_t getLocalApicAddress()
        {return m_LocalApicAddress;}
      /** Get a list of I/O APICs
       *\return list of I/O APICs */ 
      inline Vector<IoApicInformation*> &getIoApicList()
        {return m_IoApics;}
    #endif

  private:
    /** The constructor does nothing */
    Smp() INITIALISATION_ONLY;
    /** Copy-constructor
     *\note NOT implemented (singleton class) */
    Smp(const Smp &);
    /** Assignment operator
     *\note NOT implemented (singleton class) */
    Smp &operator = (const Smp &);
    /** The destructor does nothing */
    inline ~Smp(){}

    /** The floating pointer structure according to the Intel Multiprocessor
     *  Specification */
    struct FloatingPointer
    {
      uint32_t signature;
      uint32_t physicalAddress;
      uint8_t length;
      uint8_t revision;
      uint8_t checksum;
      uint8_t features[5];
    } PACKED;

    /** The configuration table header according to the Intel Multiprocessor
     *  Specification */
    struct ConfigTableHeader
    {
      uint32_t signature;
      uint16_t baseTableLength;
      uint8_t revision;
      uint8_t checksum;
      char oem[8];
      char product[12];
      uint32_t oemTable;
      uint16_t oemTableSize;
      uint16_t entryCount;
      uint32_t localApicAddress;
      uint16_t extendedTableLength;
      uint8_t extendedChecksum;
      uint8_t reserved;
    } PACKED;

    /** Entry for a processor within the configuration table */
    struct Processor
    {
      uint8_t entryType;
      uint8_t localApicId;
      uint8_t localApicVersion;
      uint8_t flags;
      uint32_t signature;
      uint32_t featureFlags;
      uint32_t res0;
      uint32_t res1;
    } PACKED;

    /** Entry for a bus within the configuration table */
    struct Bus
    {
      uint8_t entryType;
      uint8_t busId;
      char name[6];
    } PACKED;

    /** Entry for an I/O APIC within the configuration table */
    struct IoApic
    {
      uint8_t entryType;
      uint8_t id;
      uint8_t version;
      uint8_t flags;
      uint32_t address;
    } PACKED;

    struct IoInterruptAssignment
    {
      uint8_t entryType;
      uint8_t type;
      uint16_t flags;
      uint8_t busId;
      uint8_t busIrq;
      uint8_t ioApicId;
      uint8_t ioApicIntn;
    } PACKED;

    struct LocalInterruptAssignment
    {
      uint8_t entryType;
      uint8_t type;
      uint16_t flags;
      uint8_t busId;
      uint8_t busIrq;
      uint8_t localApicId;
      uint8_t localApicIntn;
    } PACKED;

    /** Find the floating pointer structure
     *\return true, if we successfully found one, false otherwise */
    bool find() INITIALISATION_ONLY;
    /** Find the floating pointer structure within a specific memory area
     *\param[in] pMemory pointer to the beginning of the area
     *\param[in] sMemory size in bytes of the area
     *\return pointer to the floating pointer structure, 0 otherwise */
    FloatingPointer *find(void *pMemory, size_t sMemory) INITIALISATION_ONLY;
    /** Check if the checksum of the floating pointer structure is valid
     *\param[in] pFloatingPointer pointer to the floating pointer structure
     *\return true, if the checksum is valid, false otherwise */
    bool checksum(const FloatingPointer *pFloatingPointer) INITIALISATION_ONLY;
    /** Check if the checksum of the configuration table is valid
     *\param[in] pConfigTable pointer to the configuration table
     *\return true, if the checksum is valid, false otherwise */
    bool checksum(const ConfigTableHeader *pConfigTable) INITIALISATION_ONLY;

    /** Were tables found and are they valid? */
    bool m_bValid;
    /** Pointer to the floating pointer structure */
    FloatingPointer *m_pFloatingPointer;
    /** Pointer to the configuration table */
    ConfigTableHeader *m_pConfigTable;

    #if defined(APIC)
      /** Is PIC Mode implemented? Otherwise Virtual-Wire Mode is implemented */
      bool m_bPICMode;
      /** Physical address of all local APICs */
      uint64_t m_LocalApicAddress;
      /** List of I/O APICs */
      Vector<IoApicInformation*> m_IoApics;

      #if defined(MULTIPROCESSOR)
        /** List of processors */
        Vector<ProcessorInformation*> m_Processors;
      #endif
    #endif

    /** Instance of the class */
    static Smp m_Instance;
};

/** @} */

#endif

#endif
