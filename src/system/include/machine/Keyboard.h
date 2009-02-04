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

#ifndef MACHINE_KEYBOARD_H
#define MACHINE_KEYBOARD_H

#include <processor/types.h>

#define KB_ARROWLEFT 24
#define KB_ARROWUP 1
#define KB_ARROWDOWN 2
#define KB_ARROWRIGHT 3
#define KB_INSERT 4
#define KB_HOME 5
#define KB_PAGEUP 6
#define KB_DELETE 7
#define KB_END 8
#define KB_PAGEDOWN 9
#define KB_NUMLOCK 10
#define KB_F1 11
#define KB_F2 12
#define KB_F3 13
#define KB_F4 14
#define KB_F5 15
#define KB_F6 16
#define KB_F7 17
#define KB_F8 18
#define KB_F9 19
#define KB_F10 20
#define KB_F11 21
#define KB_F12 22
#define KB_SCROLLOCK 23

/**
 * Keyboard device abstraction.
 */
class Keyboard
{
public:

  /** Character representation including any modifier bits. */
  typedef struct
  {
    uint16_t ctrl : 1;
    uint16_t alt : 1;
    uint16_t shift : 1;
    uint16_t caps_lock : 1;
    uint16_t num_lock : 1;
    uint16_t is_special : 1;
    uint16_t valid : 1;
    uint16_t reserved : 1;
    uint16_t value : 8;
  } Character;

#define CHARACTER_VALID(x) (x.valid==1)

  virtual ~Keyboard() {}
  
  /**
   * Initialises the device.
   */
  virtual void initialise() =0;

  /**
   * Sets the state of the device. When debugging, it is unwise to rely on interrupt-
   * driven I/O, however in normal use polling is extremely slow and CPU-intensive.
   *
   * The debugger therefore will set the device to "debug state" by calling this function with
   * the argument "true". In "debug state", any buffered input will be discarded, the device's interrupt
   * masked, and the device will rely on polling only. This will be the default state.
   *
   * When the device is set to "normal state" by calling this function with the argument
   * "false", interrupts may be used, along with buffered input, and it is recommended that
   * during blocking I/O a Semaphore is used to signal incoming interrupts, so that the blocked thread
   * may go to sleep.
   */
  virtual void setDebugState(bool enableDebugState) =0;
  virtual bool getDebugState() =0;
  
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
   * Retrieves a character from the keyboard in special descriptor format.
   * Blocking I/O.
   * \return The character received along with any modifier keys used.
   */
  virtual Character getCharacter() =0;

  /**
   * Retrieves a character from the keyboard in special descriptor format.
   * Nonblocking I/O.
   * \return The character recieved along with any modifier keys used. The character
   *         can be tested for validity with the CHARACTER_VALID macro.
   */
  virtual Character getCharacterNonBlock() =0;
  
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
