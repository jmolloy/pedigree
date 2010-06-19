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

#include <processor/state.h>

const char *ARMV7InterruptStateRegisterName[17] =
{
  "spsr",
  "r0",
  "r1",
  "r2",
  "r3",
  "r12",
  "lr",
  "pc"
};

ARMV7InterruptState::ARMV7InterruptState() :
    m_spsr(), m_r0(), m_r1(), m_r2(),
    m_r3(), m_r12(), m_lr(), m_pc()
{
}

ARMV7InterruptState::ARMV7InterruptState(const ARMV7InterruptState &is) :
    m_spsr(is.m_spsr), m_r0(is.m_r0), m_r1(is.m_r1), m_r2(is.m_r2),
    m_r3(is.m_r3), m_r12(is.m_r12), m_lr(is.m_lr), m_pc(is.m_pc)
{
}

ARMV7InterruptState & ARMV7InterruptState::operator=(const ARMV7InterruptState &is)
{
    m_spsr = is.m_spsr; m_r0 = is.m_r0; m_r1 = is.m_r1; m_r2 = is.m_r2; m_r3 = is.m_r3;
    m_r12 = is.m_r12; m_lr = is.m_lr; m_pc = is.m_pc;
    return *this;
}

size_t ARMV7InterruptState::getRegisterCount() const
{
  return 8;
}
processor_register_t ARMV7InterruptState::getRegister(size_t index) const
{
  switch (index)
  {
    case 0: return m_spsr;
    case 1: return m_r0;
    case 2: return m_r1;
    case 3: return m_r2;
    case 4: return m_r3;
    case 5: return m_r12;
    case 6: return m_lr;
    case 7: return m_pc;
    default: return 0;
  }
}
const char *ARMV7InterruptState::getRegisterName(size_t index) const
{
  return ARMV7InterruptStateRegisterName[index];
}
