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
#include <processor/Processor.h>
#include <processor/hosted/ProcessorInformation.h>
#include <processor/hosted/VirtualAddressSpace.h>
#include <Log.h>

namespace __pedigree_hosted {};
using namespace __pedigree_hosted;

#include <signal.h>
#include <errno.h>

extern void *safe_stack_top;

/**
 * So, the sigaltstack implementation implements EPERM for sigaltstack by
 * checking the userspace stack pointer. While this is usually OK, as it will
 * protect most bad uses of sigaltstack, we need to outsmart this to make
 * sigaltstack work more like the TSS-based stack pointers seen in x86.
 *
 * All this requires is to temporarily run on a different stack :-)
 */
static bool trickSigaltstack(uintptr_t stack, stack_t *p)
{
    if(!stack)
    {
        stack = reinterpret_cast<uintptr_t>(&safe_stack_top);
    }

    int r = callOnStack(stack, reinterpret_cast<uintptr_t>(sigaltstack), reinterpret_cast<uintptr_t>(p));
    if (r < 0)
    {
        WARNING("sigaltstack failed to set new stack");
        return false;
    }

    return true;
}

void HostedProcessorInformation::setKernelStack(uintptr_t stack)
{
    if (stack)
    {
        void *new_sp = reinterpret_cast<void*>(stack - KERNEL_STACK_SIZE);
        stack_t s;
        sigaltstack(0, &s);
        if (s.ss_sp != new_sp)
        {
            ByteSet(&s, 0, sizeof(s));
            s.ss_sp = new_sp;
            s.ss_size = KERNEL_STACK_SIZE;
            int r = sigaltstack(&s, 0);
            if (r < 0 && errno == EPERM)
            {
                trickSigaltstack(stack, &s);
            }
        }

    }
    else if (!stack)
    {
        stack_t s;
        sigaltstack(0, &s);
        s.ss_flags |= SS_DISABLE;
        int r = sigaltstack(&s, 0);
        if (r < 0 && errno == EPERM)
        {
            trickSigaltstack(stack, &s);
        }
    }

    m_KernelStack = stack;
}
