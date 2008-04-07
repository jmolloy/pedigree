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
#ifndef KERNEL_PROCESSOR_X86_COMMON_SMP_H
#define KERNEL_PROCESSOR_X86_COMMON_SMP_H

#include <compiler.h>
#include <processor/types.h>
#include <utilities/Vector.h>
#include "Multiprocessor.h"

/** @addtogroup kernelprocessorx86common
 * @{ */

class Smp
{
  public:
    inline Smp()
      : m_pFloatingPointer(0), m_pConfigTable(0){}
    inline ~Smp(){}

    bool getProcessorList(uint64_t &localApicsAddress,
                          Vector<ProcessorInformation*> &Processors,
                          Vector<IoApicInformation*> &IoApics,
                          bool &bHasPics,
                          bool &bPicMode);

  private:
    Smp(const Smp &);
    Smp &operator = (const Smp &);

    struct FloatingPointer
    {
      uint32_t signature;
      uint32_t physicalAddress;
      uint8_t length;
      uint8_t revision;
      uint8_t checksum;
      uint8_t features[5];
    } PACKED;

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

    struct Bus
    {
      uint8_t entryType;
      uint8_t busId;
      char name[6];
    } PACKED;

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

    bool find();
    FloatingPointer *find(void *pMemory, size_t sMemory);
    bool checksum(const FloatingPointer *pFloatingPointer);
    bool checksum(const ConfigTableHeader *pConfigTable);

    FloatingPointer *m_pFloatingPointer;
    ConfigTableHeader *m_pConfigTable;
};

/** @} */

#endif
