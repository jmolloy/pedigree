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
#include "Module.h"

Module::Module()
{
}

Module::~ Module()
{
}

Module::Symbol *Module::lookupSymbol(String name)
{
  for (Vector<Module::Symbol*>::Iterator it = m_Symbols.begin();
       it != m_Symbols.end();
       it++)
  {
    if (name == (*it)->name)
      return *it;
  }

  // Fail.
  return 0;
}

Module::Symbol *Module::lookupSymbol(uintptr_t address)
{
  for (Vector<Module::Symbol*>::Iterator it = m_Symbols.begin();
       it != m_Symbols.end();
       it++)
  {
    if ( (address >= (*it)->address) && (address < (*it)->address+(*it)->length) )
      return *it;
  }

  // Fail.
  return 0;
}



