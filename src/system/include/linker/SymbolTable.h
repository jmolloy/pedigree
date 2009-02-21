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

#ifndef KERNEL_LINKER_SYMBOLTABLE_H
#define KERNEL_LINKER_SYMBOLTABLE_H

#include <utilities/RadixTree.h>
#include <utilities/List.h>

class Elf;

/** This class allows quick access to symbol information held
 *  within ELF files. The lookup operation allows multiple
 *  policies to retrieve the wanted symbol.
 *
 *  \note Deletion is not implemented - the normal use case
 *        for this class is insertion and lookup. Deletion
 *        would almost never occur, and so the class is
 *        optimised solely for the first two operations. */
class SymbolTable
{
public:
  /** Binding types, to define how symbols interact. */
  enum Binding
  {
    Local,
    Global,
    Weak
  };

  /** Lookup policies - given multiple definitions of a symbol,
   *  how do we determine the best response? */
  enum Policy
  {
    LocalFirst,          ///< Default policy - searches for local definitions of a symbol first.
    NotOriginatingElf    ///< Does not search the ELF given as pElf. This is used during lookups for
                         ///  R_COPY relocations, where one symbol must be linked to another.
  };

  /** Class constructor - creates an empty table. */
  SymbolTable(Elf *pElf);
  /** Destructor - destroys all information. */
  ~SymbolTable();

  /** Copy constructor. */
  SymbolTable(const SymbolTable &symtab);

  /** Insert a symbol into the table. */
  void insert(String name, Binding binding, Elf *pParent, uintptr_t value);

  /** Looks up a symbol in the table, optionally outputting the
   *  binding value.
   *
   *  If the policy is set as "LocalFirst" (the default), then
   *  Local and Global definitions from pElf are given
   *  priority.
   *
   *  If the policy is set as "NotOriginatingElf", no symbols
   *  in pElf will ever be matched, preferring those from other
   *  ELFs. This is used for R_COPY relocations.
   *
   *  \return The value of the found symbol. */
  uintptr_t lookup(String name, Elf *pElf, Policy policy=LocalFirst, Binding *pBinding=0);

private:
  class Symbol
  {
  public:
    Symbol() : m_pParent(0), m_Binding(Global), m_Value(0) {}
    Symbol(Elf *pP, Binding b, uintptr_t v) : m_pParent(pP), m_Binding(b), m_Value(v) {}
    Elf *getParent() {return m_pParent;}
    Binding getBinding() {return m_Binding;}
    uintptr_t getValue() {return m_Value;}
  private:
    Elf *m_pParent;
    Binding m_Binding;
    uintptr_t m_Value;
  };

  typedef List<Symbol*> SymbolList;
  RadixTree<SymbolList*> m_Tree;
  Elf *m_pOriginatingElf;
};

#endif

