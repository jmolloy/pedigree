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
#include "StackFrame.h"
#include <Log.h>

StackFrame::StackFrame(unsigned int nBasePointer, const char *pMangledSymbol) :
  m_nBasePointer(nBasePointer), m_Symbol()
{
  // Demangle the given symbol, storing in m_Symbol for future use.
  demangle(pMangledSymbol, &m_Symbol);
}


StackFrame::~StackFrame()
{
}

void StackFrame::prettyPrint(char *pBuf, unsigned int nBufLen)
{
  bool bIsMember = isClassMember();
  sprintf(pBuf, "%s(", m_Symbol.name);
  if (bIsMember)
  {
    char pStr[32];
    sprintf(pStr, "this=0x%x, ", getParameter(0));
    strcat(pBuf, pStr);
  }
  
  for (int i = 0; i < m_Symbol.nParams; i++)
  {
    if (i != 0)
      strcat(pBuf, ", ");
    
    char pStr[96];
    char pStr2[64];
    format(getParameter((bIsMember)?i+1:i), m_Symbol.params[i], pStr2);
    sprintf(pStr, "%s=%s", m_Symbol.params[i], pStr2);
    strcat(pBuf, pStr);
    
  }
  strcat(pBuf, ")\n");
}

unsigned int StackFrame::getParameter(unsigned int n)
{
  unsigned int *pPtr = reinterpret_cast<unsigned int *>(m_nBasePointer+(n+2)*sizeof(unsigned int));
  return *pPtr;
}
  
void StackFrame::format(unsigned int n, const char *pType, char *pDest)
{
  // Is the type a char * or const char *?
  if (!strcmp(pType, "char*") || !strcmp(pType, "const char*"))
  {
    char pStr[32];
    char *pOrigStr = reinterpret_cast<char*>(n);
    strncpy(pStr, pOrigStr, 31);
    sprintf(pDest, "\"%s\"", pStr);
  }
  // char? or const char?
  else if (!strcmp(pType, "char") || !(strcmp(pType, "const char")))
  {
    sprintf(pDest, "'%c'", static_cast<char>(n));
  }
  // bool?
  else if (!strcmp(pType, "bool") || !(strcmp(pType, "const bool")))
  {
    bool b = static_cast<bool>(n);
    if (b)
      strcpy(pDest, "true");
    else
      strcpy(pDest, "false");
  }
  // Else just use a hex integer.
  else
  {
    sprintf(pDest, "0x%x", n);
  }
}

bool StackFrame::isClassMember()
{
  char *nScopeIdx = strrchr(m_Symbol.name, ':');
  if (nScopeIdx == 0)
    return false;
  
  char *i;
  for (i = nScopeIdx-2;
       *i != 0 && *i != ':';
       i--)
    ;
  i++;
  
  if (*i >= 'A' && *i <= 'Z')
    return true;
  else
    return false;
}
