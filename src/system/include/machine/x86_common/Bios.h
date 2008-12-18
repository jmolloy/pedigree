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
#ifndef BIOS_H
#define BIOS_H

#include <processor/types.h>

/** Provides an interface to BIOS functions. Currently, this is implemented as a wrapper around X86Emu, taken from the
    XFree86 source. */
class Bios
{
public:
  /** Returns the singleton BIOS instance. */
  static Bios &instance() {return m_Instance;}

  /** Executes a BIOS interrupt.
      \param i The interrupt number to execute. */
  void executeInterrupt (int i);

  /** Returns n bytes of sub-64K space. */
  uintptr_t malloc (int n);

  /** Sets the AX register. */
  void setAx (int n);
  /** Sets the BX register. */
  void setBx (int n);
  /** Sets the CX register. */
  void setCx (int n);
  /** Sets the DX register. */
  void setDx (int n);
  /** Sets the DI register. */
  void setDi (int n);
  /** Sets the ES register. */
  void setEs (int n);

  /** Gets the AX register. */
  int getAx ();
  /** Gets the BX register. */
  int getBx ();
  /** Gets the CX register. */
  int getCx ();
  /** Gets the DX register. */
  int getDx ();
  /** Gets the DI register. */
  int getDi ();
  /** Gets the ES register. */
  int getEs ();

private:
  /** Private constructor - Singleton class. */
  Bios ();
  ~Bios ();

  /** Malloc location. */
  uintptr_t mallocLoc;

  /** Singleton instance. */
  static Bios m_Instance;
};

#endif
