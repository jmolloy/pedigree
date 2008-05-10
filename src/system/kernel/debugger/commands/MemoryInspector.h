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

#ifndef MEMORYINSPECTOR_H
#define MEMORYINSPECTOR_H

#include <DebuggerCommand.h>
#include <Scrollable.h>

/** @addtogroup kerneldebuggercommands
 * @{ */

/**
 * A debugger command to inspect memory at a location.
 */
class MemoryInspector : public DebuggerCommand,
                        public Scrollable
{
public:
  /**
   * Default constructor - zero's stuff.
   */
  MemoryInspector();

  /**
   * Default destructor - does nothing.
   */
  ~MemoryInspector();

  /**
   * Return an autocomplete string, given an input string.
   */
  void autocomplete(const HugeStaticString &input, HugeStaticString &output);

  /**
   * Execute the command with the given screen.
   */
  bool execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen);
  
  /**
   * Returns the string representation of this command.
   */
  const NormalStaticString getString()
  {
    return NormalStaticString("memory");
  }

  //
  // Scrollable interface
  //
  virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual size_t getLineCount();

private:
  void resetStatusLine(DebuggerIO *pScreen);
  void doGoto(DebuggerIO *pScreen, InterruptState &state);
  void doSearch(bool bForward, DebuggerIO *pScreen, InterruptState &state);
  bool tryGoto(LargeStaticString &str, uintptr_t &result, InterruptState &state);
  size_t m_nCharsPerLine;
};

/** @} */

#endif
