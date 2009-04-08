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

#if defined(SMBIOS)

#include <Log.h>
#include <utilities/utility.h>
#include "SMBios.h"

void SMBios::initialise()
{
  // Search for the SMBIOS tables
  find();
  if (m_pEntryPoint == 0)
  {
    NOTICE("SMBios: not compliant to the SMBIOS Specification");
    return;
  }

  NOTICE("SMBIOS Specification " << Dec << m_pEntryPoint->majorVersion << "." << m_pEntryPoint->minorVersion);
  NOTICE(" entry-point at " << Hex << reinterpret_cast<uintptr_t>(m_pEntryPoint));
  NOTICE(" tables at " << Hex << m_pEntryPoint->tableAddress << " - " << (m_pEntryPoint->tableAddress + m_pEntryPoint->tableLength));

  // Go through the tables
  Header *pCurHeader = reinterpret_cast<Header*>(m_pEntryPoint->tableAddress);
  for (size_t i = 0;i < m_pEntryPoint->structureCount;i++)
  {
    switch (pCurHeader->type)
    {
      case 0:
      {
        BiosInformation *pBiosInfo = reinterpret_cast<BiosInformation*>(adjust_pointer(pCurHeader, sizeof(Header)));
        NOTICE("  BIOS Information:");
        NOTICE("   vendor \"" << getString(pCurHeader, pBiosInfo->vendorIndex) << "\"");
        NOTICE("   version \"" << getString(pCurHeader, pBiosInfo->biosVersionIndex) << "\"");
        NOTICE("   release data \"" << getString(pCurHeader, pBiosInfo->biosReleaseDateIndex) << "\"");
        NOTICE("   memory: " << Hex << (static_cast<size_t>(pBiosInfo->biosStartSegment) * 16) << " - " << (static_cast<size_t>(pBiosInfo->biosStartSegment) * 16 + 64 * 1024 * (static_cast<size_t>(pBiosInfo->biosRomSize) + 1)));
        NOTICE("   characteristics: " << Hex << pBiosInfo->biosCharacteristics);
        // TODO: extended characteristics
      }break;
      default:
      {
        NOTICE("  unknown structure (" << Hex << pCurHeader->type << ")");
      }
    }

    pCurHeader = next(pCurHeader);
  }
}

SMBios::Header *SMBios::next(Header *pHeader)
{
  uint16_t *stringTable = reinterpret_cast<uint16_t*>(adjust_pointer(pHeader, pHeader->length));
  while (*stringTable != 0)
    stringTable = adjust_pointer(stringTable, 1);
  return reinterpret_cast<Header*>(++stringTable);
}

const char *SMBios::getString(const Header *pHeader, size_t index)
{
  const char *pCur = reinterpret_cast<const char*>(adjust_pointer(pHeader, pHeader->length));
  for (size_t i = 1;i < index;i++)
    while (*pCur++ != '\0')
      ;
  return pCur;
}

void SMBios::find()
{
  m_pEntryPoint = reinterpret_cast<EntryPoint*>(0xF0000);
  while (reinterpret_cast<uintptr_t>(m_pEntryPoint) < 0x100000)
  {
    if (m_pEntryPoint->signature == 0x5F4D535F &&
        m_pEntryPoint->signature2[0] == 0x5F &&
        m_pEntryPoint->signature2[1] == 0x44 &&
        m_pEntryPoint->signature2[2] == 0x4D &&
        m_pEntryPoint->signature2[3] == 0x49 &&
        m_pEntryPoint->signature2[4] == 0x5F &&
        checksum(m_pEntryPoint) == true)
      return;
    m_pEntryPoint = adjust_pointer(m_pEntryPoint, 16);
  }
  m_pEntryPoint = 0;
}

bool SMBios::checksum(const EntryPoint *pEntryPoint)
{
  if (::checksum(reinterpret_cast<const uint8_t*>(pEntryPoint), pEntryPoint->length) == false)
    return false;

  return ::checksum(reinterpret_cast<const uint8_t*>(&pEntryPoint->signature2[0]), 0x0F);
}

SMBios::SMBios()
  : m_pEntryPoint(0)
{
}
SMBios::~SMBios()
{
}

#endif