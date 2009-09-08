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

#include "HelpCommand.h"
#include <utilities/utility.h>
#include <processor/Processor.h>

HelpCommand::HelpCommand()
 : DebuggerCommand()
{
}

HelpCommand::~HelpCommand()
{
}

void HelpCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool HelpCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    output += "page-allocations - Inspect page allocations.\n";
    output += "backtrace        - Obtain a backtrace.\n";
    output += "cpuinfo          - Obtain CPUINFO details (stubbed)\n";
    output += "devices          - Inspect detected devices.\n";
    output += "disassemble      - Disassemble contents at given address.\n";
    output += "dump             - Dump machine state (registers etc).\n";
    output += "help             - Display this text.\n";
    output += "io               - List allocated IO ports and memory regions.\n";
    output += "log              - View the kernel log.\n";
    output += "lookup           - Lookup the symbol corresponding to an address.\n";
    output += "memory           - Inspect the contents of (virtual) memory.\n";
    output += "panic            - Cause a system panic.\n";
    output += "quit             - Leave and continue execution.\n";
    output += "step             - Single step and reenter the debugger.\n";
    output += "syscall          - Trace syscall execution times (stubbed).\n";
    output += "threads          - Inspect what each thread is doing.\n";
    output += "trace            - Graphical execution tracer.\n";
    output += "locks            - Show spinlock information.\n";
    output += "mapping          - Show V->P information for an effective addr.\n";
    return true;
}
