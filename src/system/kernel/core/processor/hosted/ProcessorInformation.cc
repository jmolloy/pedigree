/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <processor/types.h>
#include <processor/hosted/ProcessorInformation.h>
#include <processor/hosted/VirtualAddressSpace.h>
#include <Log.h>

namespace __pedigree_hosted {};
using namespace __pedigree_hosted;

#include <signal.h>

void HostedProcessorInformation::setKernelStack(uintptr_t stack)
{
    if (stack)
    {
        stack_t s;
        memset(&s, 0, sizeof(s));
        s.ss_sp = reinterpret_cast<void*>(stack - KERNEL_STACK_SIZE);
        s.ss_size = KERNEL_STACK_SIZE;
        sigaltstack(&s, 0);
        m_KernelStack = stack;
    }
}
