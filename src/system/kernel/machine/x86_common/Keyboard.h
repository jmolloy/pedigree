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

#ifndef MACHINE_X86_KEYBOARD_H
#define MACHINE_X86_KEYBOARD_H

#include <machine/Keyboard.h>
#include <machine/IrqManager.h>
#include <processor/types.h>
#include <processor/IoPort.h>
#include <process/Semaphore.h>

#define BUFLEN 256

/**
 * Keyboard device abstraction.
 */
class X86Keyboard : public Keyboard,
                    private IrqHandler
{
public:
  X86Keyboard (uint32_t portBase);
  virtual ~X86Keyboard();
  
  /**
   * Initialises the device.
   */
  virtual void initialise();

  virtual void setDebugState(bool enableDebugState);
  virtual bool getDebugState();
  
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
  virtual bool shift();
  
  /**
   * \return True if ctrl is currently held.
   */
  virtual bool ctrl();
  
  /**
   * \return True if alt is currently held.
   */
  virtual bool alt();
  
  /**
   * \return True if caps lock is currently on.
   */
  virtual bool capsLock();

  //
  // IrqHandler interface
  //
  virtual bool irq(irq_id_t number, InterruptState &state);

private:
  /**
   * Converts a scancode into a "real" character.
   */
  char scancodeToChar(uint8_t scancode);

  /**
   * True if we're in debug state.
   */
  bool m_bDebugState;

  /**
   * True if shift is held.
   */
  bool m_bShift;
  
  /**
   * True if ctrl is held.
   */
  bool m_bCtrl;
  
  /**
   * True if alt is held.
   */
  bool m_bAlt;
  
  /**
   * True if caps lock is on.
   */
  bool m_bCapsLock;
  
  /**
   * The IO port through which to access the keyboard.
   */
  IoPort m_Port;

  /**
   * Circular input buffer.
   */
  char m_Buffer[BUFLEN];
  int m_BufStart, m_BufEnd;

  /**
   * Semaphore for how many items are in the buffer.
   */
  Semaphore m_BufLength;

  /**
   * IRQ id.
   */
  irq_id_t m_IrqId;
};

#endif
