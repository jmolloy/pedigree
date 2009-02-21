/*
 * Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ€hler, Matthew Iselin
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

#include <linker/SymbolTable.h>
#include <linker/Elf.h>

SymbolTable::SymbolTable(Elf *pElf) :
  m_Tree(), m_pOriginatingElf(pElf)
{
}

SymbolTable::~SymbolTable()
{
  for (RadixTree<SymbolList*>::Iterator it = m_Tree.begin();
        it != m_Tree.end();
        it++)
  {
    if (*it)
    {
      for (SymbolList::Iterator it2 = (*it)->begin();
            it2 != (*it)->end();
            it2++)
      {
        delete *it2;
      }
      delete *it;
    }
  }
}

void SymbolTable::insert(String name, Binding binding, Elf *pParent, uintptr_t value)
{
  SymbolList *pList = m_Tree.lookup(name);
  if (!pList)
  {
    pList = new SymbolList();
    m_Tree.insert (name, pList);
  }

  // We need to scan the list to check we're not adding a duplicate.
  for (SymbolList::Iterator it = pList->begin();
        it != pList->end();
        it++)
  {
    Symbol *pSym;
    if (pSym->getBinding() == binding && pSym->getParent() == pParent)
      // This item is already in the list. Don't add it again.
      return;
  }

  pList->pushBack (new Symbol(pParent, binding, value));
}

uintptr_t SymbolTable::lookup(String name, Elf *pElf, Policy policy, Binding *pBinding)
{
  SymbolList *pList = m_Tree.lookup(name);
  if (!pList)
  {
    return 0;
  }
  if (policy == NotOriginatingElf)
  {
    NOTICE("not orig elf: " << name << ", pElf: " << (uintptr_t)pElf << ", origElf: " << (uintptr_t)m_pOriginatingElf);
  }
  // If we're on a local-first policy, check for local symbols of pElf.
  if (policy == LocalFirst)
  {
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      Symbol *pSym = *it;
      if (pSym->getBinding() == Local && pSym->getParent() == pElf)
        return pSym->getValue();
    }

    // Scan for global variables.
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      Symbol *pSym = *it;
      if (pSym->getBinding() == Global && pSym->getParent() == m_pOriginatingElf)
        return pSym->getValue();
    }

    // Scan for global variables.
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      Symbol *pSym = *it;
      if (pSym->getBinding() == Global && pSym->getParent() == pElf)
        return pSym->getValue();
    }
  }

  // Scan for global variables.
  for (SymbolList::Iterator it = pList->begin();
        it != pList->end();
        it++)
  {
    Symbol *pSym = *it;
    if (pSym->getBinding() == Global &&
        (policy != NotOriginatingElf || pSym->getParent() != m_pOriginatingElf))
    {
      NOTICE("here: " << pSym->getValue());
          return pSym->getValue();
    }
  }

  // Lastly, scan for weak variables.
  for (SymbolList::Iterator it = pList->begin();
        it != pList->end();
        it++)
  {
    Symbol *pSym = *it;
    if (pSym->getBinding() == Weak &&
        (policy != NotOriginatingElf || pSym->getParent() != m_pOriginatingElf))
    {
      return pSym->getValue();
    }
  }
NOTICE("nowt");
  // Nothing found.
  return 0;
}

