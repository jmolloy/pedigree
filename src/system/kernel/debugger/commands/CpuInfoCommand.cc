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

#include <CpuInfoCommand.h>
#include <DebuggerIO.h>
#include <udis86.h>
#include <Log.h>
#include <utilities/demangle.h>
#include <processor/Processor.h>


CpuInfoCommand::CpuInfoCommand()
{
}

CpuInfoCommand::~CpuInfoCommand()
{
}

void CpuInfoCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool CpuInfoCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen)
{
  Processor::identify (output);
  output += '\n';
  return true;
}

const NormalStaticString CpuInfoCommand::getString()
{
  return NormalStaticString("cpuinfo");
}
