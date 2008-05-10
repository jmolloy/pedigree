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

#include "DumpCommand.h"
#include <utilities/utility.h>
#include <processor/Processor.h>

DumpCommand::DumpCommand()
 : DebuggerCommand()
{
}

DumpCommand::~DumpCommand()
{
}

void DumpCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool DumpCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  if (state.kernelMode() == true)
    output += "kernel-mode\n";

  // Output all the other registers
  size_t curLength = output.length();
  for (size_t i = 0;i < state.getRegisterCount();i++)
  {
    output += state.getRegisterName(i);
    output += ": 0x";
    output.append(state.getRegister(i), 16, state.getRegisterSize(i) * 2, '0');

    if (i == (state.getRegisterCount() - 1))
      output += '\n';
    else
    {
      size_t addLength = strlen(state.getRegisterName(i + 1)) + 4 + state.getRegisterSize(i + 1) * 2;
      if ((addLength + (output.length() - curLength)) > 80)
      {
        output += '\n';
        curLength = output.length();
      }
      else
        output += ' ';
    }
  }
  return true;
}
