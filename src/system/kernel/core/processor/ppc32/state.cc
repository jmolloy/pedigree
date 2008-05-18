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

PPC32InterruptState::PPC32InterruptState()
{
}

PPC32InterruptState::PPC32InterruptState(const PPC32InterruptState &is)
{
}

PPC32InterruptState & PPC32InterruptState::operator=(const PPC32InterruptState &is)
{
  return *this;
}

size_t PPC32InterruptState::getRegisterCount() const
{
  return 34;
}
processor_register_t PPC32InterruptState::getRegister(size_t index) const
{
  switch (index)
  {
    default: return 0;
  }
}
const char *PPC32InterruptState::getRegisterName(size_t index) const
{
  return 0;
}
