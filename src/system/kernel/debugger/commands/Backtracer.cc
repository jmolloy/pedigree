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

#include <Backtracer.h>
#include <Backtrace.h>
#include <DebuggerIO.h>
#include <utilities/utility.h>
#include <utilities/StaticString.h>

#include <DwarfUnwinder.h>
#include <FileLoader.h>

Backtracer::Backtracer()
{
}

Backtracer::~Backtracer()
{
}

void Backtracer::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
  // TODO: add symbols.
  output = "<address> (optional)";
}

bool Backtracer::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen)
{
  Backtrace bt;
  bt.performBacktrace(state);

  bt.prettyPrint(output);
  
//   DwarfUnwinder du(elf.debugFrameTable(), elf.debugFrameTableLength());
//   ProcessorState initial(state);
//   ProcessorState next;
//   du.unwind(initial, next);
  return true;
}

const NormalStaticString Backtracer::getString()
{
  return NormalStaticString("backtrace");
}
