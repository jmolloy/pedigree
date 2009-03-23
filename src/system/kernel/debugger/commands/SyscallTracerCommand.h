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
#include <processor/Processor.h>
#include <DebuggerCommand.h>
#include <Scrollable.h>
#include <Log.h>

class SyscallTracerCommand :  public DebuggerCommand,
                              public Scrollable
{
public:
  /**
   * Creates a new SyscallTracer object.
   */
  SyscallTracerCommand();
  ~SyscallTracerCommand();

  /**
   * Return an autocomplete string, given an input string. The string put into *output must not
   * exceed len in length.
   */
  void autocomplete(const HugeStaticString &input, HugeStaticString &output);

  /**
   * Execute the command with the given screen. The command can take over the screen while
   * executing, but must return it to CLI mode (via enableCLI) before returning.
   * \return True if the debugger should continue accepting commands, false if it should return
   *         control to the kernel.
   */
  bool execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen);

  /**
   * Returns the string representation of this command.
   */
  const NormalStaticString getString()
  {
    NOTICE("Got Syscall Command Name");    //  Never even gets logged.
    return NormalStaticString("syscall");
  }

  //
  // Scrollable interface
  //
  virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
  virtual size_t getLineCount();
};
