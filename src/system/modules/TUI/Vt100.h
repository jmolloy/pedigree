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

#include <processor/types.h>
#include <process/Semaphore.h>

#define MAXBUFLEN 64

/** A VT100 compatible terminal emulator. It uses BootIO as its output medium, and Keyboard as its
    input. */
class Vt100
{
public:
  Vt100();
  ~Vt100();
  
  void write(char c);
  char read();

private:
  //
  // 'Buffer' here refers to the input buffer.
  //
  void bufferChar(char c);
  char unbufferChar();

  Semaphore bufLen;
  char buffer[MAXBUFLEN];
  int bufStart, bufEnd;

  //
  // Colour properties.
  //
  bool bold;
  int fg;
  int bg;

  //
  // Cursor position.
  //
  
};
