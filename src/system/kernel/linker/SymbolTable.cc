/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
  if(!m_Tree.count())
    return;
  for (SymbolTrie::Iterator it = m_Tree.begin();
        it != m_Tree.end();
        it++)
  {
    if (*it)
    {
      if(!((*it)->count()))
        continue;

      // SharedPointers will be destructed as part of the list terminating.
      delete *it;
    }
  }
}

void SymbolTable::copyTable(Elf *pNewElf, const SymbolTable &newSymtab)
{
    // Copy the initial tree (which will copy mere pointers to the lists)
    m_Tree = newSymtab.m_Tree;

    // Iterate over the tree and copy the lists pointer-by-pointer
    if(!m_Tree.count())
        return;
    for (SymbolTrie::Iterator it = newSymtab.m_Tree.begin();
        it != newSymtab.m_Tree.end();
        it++)
    {
        if (*it)
        {
            if(!((*it)->count()))
                continue;

            // We now have a list, but we're not going to iterate...
            SymbolList *newList = new SymbolList;
            for(SymbolList::Iterator it2 = (*it)->begin();
                it2 != (*it)->end();
                it2++)
            {
                SharedPointer<Symbol> &pSymbol = *it2;
                newList->pushBack(pSymbol);
            }

            *it = newList;
        }
    }
}

void SymbolTable::insert(String name, Binding binding, Elf *pParent, uintptr_t value)
{
  doInsert(name, binding, pParent, value);
}

void SymbolTable::insertMultiple(SymbolTable *pOther, String name, Binding binding, Elf *pParent, uintptr_t value)
{
  SharedPointer<Symbol> ptr = doInsert(name, binding, pParent, value);
  if (pOther)
    pOther->insertShared(name, ptr);
}

SharedPointer<SymbolTable::Symbol> SymbolTable::doInsert(String name, Binding binding, Elf *pParent, uintptr_t value)
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
    SharedPointer<Symbol> pSym = (*it);
    if (pSym->getBinding() == binding && pSym->getParent() == pParent)
      // This item is already in the list. Don't add it again.
      return pSym;
  }

  // Not already in list, so create a new SharedPointer for it.
  SharedPointer<Symbol> newSymbol;
  Symbol *pSymbol = new Symbol(pParent, binding, value);
  newSymbol.reset(pSymbol);
  pList->pushBack (newSymbol);

  return newSymbol;
}

void SymbolTable::insertShared(String name, SharedPointer<SymbolTable::Symbol> symbol)
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
    SharedPointer<Symbol> pSym = (*it);
    if (pSym->getBinding() == symbol->getBinding() && pSym->getParent() == symbol->getParent())
      // This item is already in the list. Don't add it again.
      return;
  }

  // Just add the shared pointer instead of creating a new one.
  pList->pushBack(symbol);
}

void SymbolTable::eraseByElf(Elf *pParent)
{
    for (SymbolTrie::Iterator tit = m_Tree.begin();
            tit != m_Tree.end();
            tit++)
    {
        // Verify that there's actually a valid list
        if(!*tit)
            continue;

        SymbolList *pList = *tit;
        for (SymbolList::Iterator it = pList->begin();
            it != pList->end();
            it++)
        {
            // Verify the symbol pointer
            if(!*it)
                continue;

            SharedPointer<Symbol> pSym = *it;
            if (pSym->getParent() == pParent)
            {
                it = pList->erase(it);

                /// \todo Epic quick hack, rewrite
                if(it == pList->end())
                    break;
            }
        }
    }
}

uintptr_t SymbolTable::lookup(String name, Elf *pElf, Policy policy, Binding *pBinding)
{
  SymbolList *pList = m_Tree.lookup(name);
  if (!pList)
  {
    return 0;
  }

  // If we're on a local-first policy, check for local symbols of pElf.
  if (policy == LocalFirst)
  {
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      SharedPointer<Symbol> pSym = *it;
      if (pSym->getBinding() == Local && pSym->getParent() == pElf)
        return pSym->getValue();
    }

    // Scan for global variables.
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      SharedPointer<Symbol> pSym = *it;
      if (pSym->getBinding() == Global && pSym->getParent() == m_pOriginatingElf)
        return pSym->getValue();
    }

    // Scan for global variables.
    for (SymbolList::Iterator it = pList->begin();
          it != pList->end();
          it++)
    {
      SharedPointer<Symbol> pSym = *it;
      if (pSym->getBinding() == Global && pSym->getParent() == pElf)
        return pSym->getValue();
    }
  }

  // Scan for global variables.
  for (SymbolList::Iterator it = pList->begin();
        it != pList->end();
        it++)
  {
    SharedPointer<Symbol> pSym = *it;
    if (pSym->getBinding() == Global &&
        (policy != NotOriginatingElf || pSym->getParent() != m_pOriginatingElf))
    {
      return pSym->getValue();
    }
  }

  // Lastly, scan for weak variables.
  for (SymbolList::Iterator it = pList->begin();
        it != pList->end();
        it++)
  {
    SharedPointer<Symbol> pSym = *it;
    if (pSym->getBinding() == Weak &&
        (policy != NotOriginatingElf || pSym->getParent() != m_pOriginatingElf))
    {
      return pSym->getValue();
    }
  }

  // Nothing found.
  return 0;
}

