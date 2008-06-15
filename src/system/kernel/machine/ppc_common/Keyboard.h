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

#ifndef MACHINE_MALTA_KEYBOARD_H
#define MACHINE_MALTA_KEYBOARD_H

#include <machine/Keyboard.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>

class PPCKeyboard : public Keyboard
{
public:
  PPCKeyboard ();
  virtual ~PPCKeyboard();
  
  /**
   * Initialises the device.
   */
  virtual void initialise();
  
  /**
   * Retrieves a character from the keyboard. Blocking I/O.
   * \return The character recieved or zero if it is a character
   *         without an ascii representation.
   */
  virtual char getChar();
  
  /**
   * Retrieves a character from the keyboard. Non blocking I/O.
   * \return The character recieved or zero if it is a character
   *         without an ascii representation, or zero also if no
   *         character was present.
   */
  virtual char getCharNonBlock();
  
  /**
   * \return True if shift is currently held.
   */
  virtual bool shift() {return false;}
  
  /**
   * \return True if ctrl is currently held.
   */
  virtual bool ctrl() {return false;}
  
  /**
   * \return True if alt is currently held.
   */
  virtual bool alt() {return false;}
  
  /**
   * \return True if caps lock is currently on.
   */
  virtual bool capsLock() {return false;}

private:
  PPCKeyboard(const PPCKeyboard &);
  PPCKeyboard &operator = (const PPCKeyboard &);

  /** Our keyboard device. */
  OFHandle m_Dev;
};

#endif
