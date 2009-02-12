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

#ifndef TUI_H
#define TUI_H

#include <console/Console.h>
#include <utilities/RequestQueue.h>
#include <machine/Display.h>

/** A text user interface class. This class inherits from RequestQueue so that it
    can expose Console functionality. */
class TUI : public RequestQueue
{
public:
  TUI();
  ~TUI();

  // Called by video drivers to inform us that a mode change has taken place.
  void modeChanged(Display::ScreenMode mode, void *pFramebuffer);

protected:
  //
  // RequestQueue interface.
  //
  virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                  uint64_t p6, uint64_t p7, uint64_t p8);


  bool m_Echo;
  bool m_EchoNewlines;
  bool m_EchoBackspace;
  bool m_MapCrToNl;
  bool m_MapNlToCr;

  bool m_bIsTextMode;
  Display::ScreenMode m_Mode;

  char m_pQueue[32];
  size_t m_QueueLength;
};

#endif
