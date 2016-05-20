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
  m_LocalSymbols(), m_GlobalSymbols(), m_WeakSymbols(), m_pOriginatingElf(pElf)
{
}

SymbolTable::~SymbolTable()
{
}

void SymbolTable::copyTable(Elf *pNewElf, const SymbolTable &newSymtab)
{
    // Safe to do this, all members are SharedPointers and will be copy
    // constructed by these operations.
    m_LocalSymbols = newSymtab.m_LocalSymbols;
    m_GlobalSymbols = newSymtab.m_GlobalSymbols;
    m_WeakSymbols = newSymtab.m_WeakSymbols;
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
    Symbol *pSymbol = new Symbol(pParent, binding, value);
    SharedPointer<Symbol> newSymbol(pSymbol);

    insertShared(name, newSymbol);
    return newSymbol;
}

void SymbolTable::insertShared(String name, SharedPointer<SymbolTable::Symbol> symbol)
{
    SharedPointer<symbolTree_t> tree = getOrInsertTree(symbol->getParent());
    tree->insert(name, symbol);

    // Insert global/weak as well - if the lookup fails in the ELF's table,
    // it'll fall back to these.
    if (symbol->getBinding() == Global)
    {
      m_GlobalSymbols.insert(name, symbol);
    }
    else if (symbol->getBinding() == Weak)
    {
      m_WeakSymbols.insert(name, symbol);
    }
    return;
}

void SymbolTable::eraseByElf(Elf *pParent)
{
    // Will wipe out recursively by destroying the SharedPointers within.
    m_LocalSymbols.remove(pParent);

    /// \todo wipe out global/weak symbols.

    for (auto it = m_GlobalSymbols.begin(); it != m_GlobalSymbols.end();)
    {
        if (it->getParent() == pParent)
        {
            it = m_GlobalSymbols.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = m_WeakSymbols.begin(); it != m_WeakSymbols.end();)
    {
        if (it->getParent() == pParent)
        {
            it = m_WeakSymbols.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

uintptr_t SymbolTable::lookup(String name, Elf *pElf, Policy policy, Binding *pBinding)
{
  // Local.
  SharedPointer<Symbol> sym;
  SharedPointer<symbolTree_t> symbolTree = m_LocalSymbols.lookup(pElf);
  if (symbolTree)
  {
    sym = symbolTree->lookup(name);
    if (sym)
    {
      return sym->getValue();
    }
  }

  // Global.
  sym = m_GlobalSymbols.lookup(name);
  if (sym)
  {
    return sym->getValue();
  }

  // Weak.
  sym = m_WeakSymbols.lookup(name);
  if (sym)
  {
    return sym->getValue();
  }

  return 0;
}

SharedPointer<SymbolTable::symbolTree_t> SymbolTable::getOrInsertTree(Elf *p)
{
    SharedPointer<symbolTree_t> tree = m_LocalSymbols.lookup(p);
    if (tree)
        return tree;

    tree = SharedPointer<symbolTree_t>::allocate();
    m_LocalSymbols.insert(p, tree);
    return tree;
}
