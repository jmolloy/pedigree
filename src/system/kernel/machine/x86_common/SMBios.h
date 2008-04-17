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
#ifndef KERNEL_MACHINE_X86_COMMON_SMBIOS_H
#define KERNEL_MACHINE_X86_COMMON_SMBIOS_H

#if defined(SMBIOS)

#include <compiler.h>
#include <processor/types.h>

class SMBios
{
  public:
    /** The default constructor does nothing */
    SMBios();
    /** The destructor does nothing */
    ~SMBios();

    /** Find and parse the SMBios tables */
    void initialise();

  private:
    /** Copy-constructor
     *\note NOT implemented */
    SMBios(const SMBios &);
    /** Assignment operator
     *\note NOT implemented */
    SMBios &operator = (const SMBios &);

    struct EntryPoint
    {
      uint32_t signature;
      uint8_t checksum;
      uint8_t length;
      uint8_t majorVersion;
      uint8_t minorVersion;
      uint16_t maxStructureSize;
      uint8_t revision;
      uint8_t reserved[5];
      uint8_t signature2[5];
      uint8_t checksum2;
      uint16_t tableLength;
      uint32_t tableAddress;
      uint16_t structureCount;
      uint8_t bcdRevision;
    } PACKED;

    struct Header
    {
      uint8_t type;
      uint8_t length;
      uint16_t handle;
    } PACKED;

    struct BiosInformation
    {
      uint8_t vendorIndex;
      uint8_t biosVersionIndex;
      uint16_t biosStartSegment;
      uint8_t biosReleaseDateIndex;
      uint8_t biosRomSize;
      uint32_t biosCharacteristics;
    } PACKED;

    Header *next(Header *pHeader);
    const char *getString(const Header *pHeader, size_t index);
    void find();
    bool checksum(const EntryPoint *pEntryPoint);

    EntryPoint *m_pEntryPoint;
};

#endif

#endif
