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
#ifndef MODULE_H
#define MODULE_H

#include <utilities/Vector.h>
#include <utilities/String.h>

/** A class to describe a loaded kernel module or driver. */
class Module
{
public:
  friend class KernelElf;

  struct Symbol
  {
    String name;
    uintptr_t address;
    uintptr_t length;
  };
  
  Module();

  ~Module();

  Symbol *lookupSymbol(String name);

  Symbol *lookupSymbol(uintptr_t address);

private:
  Vector<Symbol*> m_Symbols;
};

#endif
