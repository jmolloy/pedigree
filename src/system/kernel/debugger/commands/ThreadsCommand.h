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
#ifndef THREADSCOMMAND_H
#define THREADSCOMMAND_H

#include <DebuggerCommand.h>
#include <Scrollable.h>
#include <process/Thread.h>

/** @addtogroup kerneldebuggercommands
 * @{ */

/**
 * Allows step execution.
 */
class ThreadsCommand : public DebuggerCommand,
                       public Scrollable
{
public:
  /**
   * Default constructor - zero's stuff.
   */
  ThreadsCommand();

  /**
     * Default destructor - does nothing.
   */
  ~ThreadsCommand();

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
    return NormalStaticString("threads");
  }
  
  /** Sets the pointers to use to change the thread the debugger debugs. */
  void setPointers(Thread **ppThread, InterruptState *pState)
  {
    //m_ppThread = ppThread;
    //m_pState = pState;
  }

  //
  // Scrollable interface
  //
  virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual size_t getLineCount();
  
private:
  bool swapThread(InterruptState &state, DebuggerIO *pScreen);
  size_t m_SelectedLine;
  size_t m_nLines;
};

/** @} */

#endif
