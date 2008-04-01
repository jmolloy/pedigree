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
#ifndef MACHINE_KEYBOARD_H
#define MACHINE_KEYBOARD_H

#include <processor/types.h>

/**
 * Keyboard device abstraction.
 */
class Keyboard
{
public:
  virtual ~Keyboard() {}
  
  /**
   * Initialises the device.
   */
  virtual void initialise() =0;
  
  /**
   * Retrieves a character from the keyboard. Blocking I/O.
   * \return The character recieved or zero if it is a character
   *         without an ascii representation.
   */
  virtual char getChar() =0;
  
  /**
   * Retrieves a character from the keyboard. Non blocking I/O.
   * \return The character recieved or zero if it is a character
   *         without an ascii representation, or zero also if no
   *         character was present.
   */
  virtual char getCharNonBlock() =0;
  
  /**
   * \return True if shift is currently held.
   */
  virtual bool shift() =0;
  
  /**
   * \return True if ctrl is currently held.
   */
  virtual bool ctrl() =0;
  
  /**
   * \return True if alt is currently held.
   */
  virtual bool alt() =0;
  
  /**
   * \return True if caps lock is currently on.
   */
  virtual bool capsLock() =0;
};

#endif
