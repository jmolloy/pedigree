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

#ifndef DEBUGGER_COMMAND_H
#define DEBUGGER_COMMAND_H

#include <processor/state.h>
#include <utilities/StaticString.h>

/** @addtogroup kerneldebugger
 * @{ */

class DebuggerIO;

/**
 * Abstract class defining the interface for a DebuggerCommand. Class Debugger
 * has a set of these DebuggerCommands, and calls the getString and autocomplete
 * functions to augment the command line interface with visual prompts.
 */
class DebuggerCommand
{
public:
  DebuggerCommand() {};
  virtual ~DebuggerCommand() {};
  
  /**
   * Return an autocomplete string, given an input string. The string put into *output must not
   * exceed len in length.
   */
  virtual void autocomplete(const HugeStaticString &input, HugeStaticString &output) = 0;

  /**
   * Execute the command with the given screen. The command can take over the screen while
   * executing, but must return it to CLI mode (via enableCLI) before returning.
   * \return True if the debugger should continue accepting commands, false if it should return
   *         control to the kernel.
   */
  virtual bool execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen) = 0;
  
  /**
   * Returns the string representation of this command.
   */
  virtual const NormalStaticString getString() = 0;
};

/** @} */

#endif
