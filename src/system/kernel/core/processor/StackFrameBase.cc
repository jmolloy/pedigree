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

#if defined(DEBUGGER)

#include <Log.h>
#include <utilities/utility.h>
#include <processor/StackFrameBase.h>

StackFrameBase::StackFrameBase(const ProcessorState &State, uintptr_t basePointer,
                               LargeStaticString mangledSymbol)
  : m_Symbol(), m_State(State), m_BasePointer(basePointer)
{
  // Demangle the given symbol, storing in m_Symbol for future use.
  demangle(mangledSymbol, &m_Symbol);
}

void StackFrameBase::prettyPrint(HugeStaticString &buf)
{
  bool bIsMember = isClassMember();
  
  buf += static_cast<const char*>(m_Symbol.name);
  buf += '(';
  
#ifndef MIPS_COMMON
  if (bIsMember)
  {
    buf += "this=0x";
    buf.append(getParameter(0), 16);
    buf += ", ";
  }
#endif

  for (size_t i = 0; i < m_Symbol.nParams; i++)
  {
    if (i != 0)
      buf += ", ";
    
    format(getParameter((bIsMember)?i+1:i), m_Symbol.params[i], buf);
  }
  
  buf += ")\n";
}

void StackFrameBase::format(uintptr_t n, const LargeStaticString &type, HugeStaticString &dest)
{
#if defined(MIPS_COMMON)
  dest += type;
#else
  // Is the type a char * or const char *?
  if (type == "char*" || type == "const char*")
  {
    //LargeStaticString tmp(reinterpret_cast<const char*>(n));
    //dest += '"';
    //dest += tmp.left(22); // Only 22 characters max.
    //dest += '"';
    dest.append("(");
    dest.append(type);
    dest.append(") 0x");
    #ifdef BITS_32
      dest.append(n, 16, 8, '0');
    #endif
    #ifdef BITS_64
      dest.append(n, 16, 16, '0');
    #endif
  }
  // char? or const char?
  else if (type == "char" || type == "const char")
  {
    dest += '\'';
    dest += static_cast<char>(n);
    dest += '\'';
  }
  // bool?
  else if (type == "bool" || type == "const bool")
  {
    bool b = static_cast<bool>(n);
    if (b)
      dest += "true";
    else
      dest += "false";
  }
  // 16-bit value?
  else if (type == "short")
  {
    dest += "0x";
    dest.append(static_cast<short> (n), 16);
  }
  else if (type == "unsigned short")
  {
    dest += "0x";
    dest.append(static_cast<unsigned short> (n), 16);
  }
  // 32-bit value, in 64-bit mode?
  else if (type == "int")
  {
    dest += "0x";
    dest.append(static_cast<int> (n), 16);
  }
  else if (type == "unsigned int")
  {
    dest += "0x";
    dest.append(static_cast<unsigned int> (n), 16);
  }
  // Else just use a hex integer, represented as a cast.
  else
  {
    dest += "(";
    dest += static_cast<const char*> (type);
    dest += ")0x";
    dest.append(n, 16);
  }
#endif
}

bool StackFrameBase::isClassMember()
{
  int nScopeIdx = m_Symbol.name.last(':');
  if (nScopeIdx == -1)
    return false;

  int i;
  for (i = nScopeIdx-2;
       i >= 0 && m_Symbol.name[i] != ':';
       i--)
    ;
  i++;
  
  if (m_Symbol.name[i] >= 'A' && m_Symbol.name[i] <= 'Z')
    return true;
  else
    return false;
}

#endif
