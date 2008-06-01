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

#include <processor/Processor.h>
#include <processor/TlbManager.h>
#include <machine/openfirmware/OpenFirmware.h>
#include "InterruptManager.h"

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  // Initialise openfirmware.
  OpenFirmware::instance().initialise(reinterpret_cast<OpenFirmware::OFInterface> (Info.prom));

  // TODO: Initialise the physical memory-management
//   m_Initialised = 1;
}

void Processor::initialise2()
{
  // TODO
  // Initialise this processor's interrupt handling
  PPC32InterruptManager::initialiseProcessor();

//   m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
}
