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
#ifndef BreakpointCommand_H
#define BreakpointCommand_H

#include <DebuggerCommand.h>

/**
 * Debugger command that allows viewing of the kernel log.
 */
class BreakpointCommand : public DebuggerCommand
{
public:
  /**
   * Default constructor - zero's stuff.
   */
  BreakpointCommand();

  /**
   * Default destructor - does nothing.
   */
  ~BreakpointCommand();

  /**
   * Return an autocomplete string, given an input string.
   */
  void autocomplete(char *input, char *output, int len);

  /**
   * Execute the command with the given screen.
   */
  bool execute(char *input, char *output, int len, InterruptState &state, DebuggerIO *screen);
  
  /**
   * Returns the string representation of this command.
   */
  const char *getString()
  {
    return "breakpoint";
  }
  
};

#endif

