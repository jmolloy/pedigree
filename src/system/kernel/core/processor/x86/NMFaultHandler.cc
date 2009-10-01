/*
 * Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ¤hler, Matthew Iselin
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

#include <Log.h>
#include <Debugger.h>
#include <processor/NMFaultHandler.h>
#include <process/Scheduler.h>
#include <processor/PhysicalMemoryManager.h>

NMFaultHandler NMFaultHandler::m_Instance;

#define NM_FAULT_EXCEPTION      0x07
/// \todo Move these to some X86 Processor header
#define CR0_NE                  (1<<5)
#define CR0_TS                  (1<<3)
#define CR0_EM                  (1<<2)
#define CR0_MP                  (1<<1)
#define CPUID_FEAT_ECX_XSAVE    (1<<26)
#define CPUID_FEAT_EDX_FPU      (1<<0)

bool NMFaultHandler::initialise()
{
    // Check for FPU and XSAVE
    uint32_t eax, ebx, ecx, edx;
    Processor::cpuid(1, 0, eax, ebx, ecx, edx);
    if(ecx & CPUID_FEAT_ECX_XSAVE && edx & CPUID_FEAT_EDX_FPU)
    {
        // Initialise the FPU and the TS bit
        uint32_t cr0;
        uint16_t cw = 0x3f7;
        asm volatile ("mov %%cr0, %0" : "=r" (cr0));
        cr0 = (cr0 | CR0_NE | CR0_MP) & ~(CR0_EM | CR0_TS);
        asm volatile ("mov %0, %%cr0" :: "r" (cr0));
        asm volatile ("finit");
        asm volatile ("fldcw %0" :: "m" (cw));
        cr0 |= CR0_TS;
        asm volatile ("mov %0, %%cr0" :: "r" (cr0));

        // Register the handler
        return InterruptManager::instance().registerInterruptHandler(NM_FAULT_EXCEPTION, this);
    }
    return false;
}

void NMFaultHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
    // Check the TS bit
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    if(cr0 & CR0_TS)
    {
        cr0 &= ~CR0_TS;
        asm volatile ("mov %0, %%cr0" :: "r" (cr0));
    }
    else
    {
        FATAL_NOLOCK("NM: TS already disabled");
    }

}

NMFaultHandler::NMFaultHandler()
{
}
