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
#include "BootstrapInfo.h"
#include <utility.h>

BootstrapInfo::BootstrapInfo(BootstrapStruct_t *info) :
    m_BootstrapInfo(*info)
{
}

BootstrapInfo::~BootstrapInfo()
{
}

int BootstrapInfo::getSectionHeaderCount()
{
  return m_BootstrapInfo.num;
}

uint8_t *BootstrapInfo::getSectionHeader(int n)
{
  return reinterpret_cast<uint8_t *>(m_BootstrapInfo.addr + n*m_BootstrapInfo.size);
}

int BootstrapInfo::getStringTable()
{
  return m_BootstrapInfo.shndx;
}
