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
#include <processor/state.h>

const char *X86InterruptStateRegisterName[11] =
{
  "eax",
  "ebx",
  "ecx",
  "edx",
  "edi",
  "esi",
  "ebp",
  "esp",
  "eip",
  "eflags"
};

size_t X86InterruptState::getRegisterCount() const
{
  return 10;
}
processor_register_t X86InterruptState::getRegister(size_t index) const
{
  if (index == 0)return m_Eax;
  if (index == 1)return m_Ebx;
  if (index == 2)return m_Ecx;
  if (index == 3)return m_Edx;
  if (index == 4)return m_Edi;
  if (index == 5)return m_Esi;
  if (index == 6)return m_Ebp;
  if (index == 7)return getStackPointer();
  if (index == 8)return m_Eip;
  if (index == 9)return m_Eflags;
  return 0;
}
const char *X86InterruptState::getRegisterName(size_t index) const
{
  return X86InterruptStateRegisterName[index];
}
