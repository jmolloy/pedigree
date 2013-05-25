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
#include <processor/Processor.h>

NMFaultHandler NMFaultHandler::m_Instance;

#define NM_FAULT_EXCEPTION      0x07
/// \todo Move these to some X86 Processor header
#define CR0_NE                  (1<<5)
#define CR0_TS                  (1<<3)
#define CR0_EM                  (1<<2)
#define CR0_MP                  (1<<1)
#define CR4_OSFXSR              (1 << 9)
#define CR4_OSXMMEXCPT          (1 << 10)
#define CPUID_FEAT_EDX_FXSR     (1<<24)
#define CPUID_FEAT_EDX_FPU      (1<<0)
#define CPUID_FEAT_EDX_SSE      (1<<25)

#define MXCSR_PM            (1 << 12)
#define MXCSR_UM            (1 << 11)
#define MXCSR_OM            (1 << 10)
#define MXCSR_ZM            (1 <<  9)
#define MXCSR_DM            (1 <<  8)
#define MXCSR_IM            (1 <<  7)

#define MXCSR_RC            13

#define MXCSR_RC_NEAREST    0
#define MXCSR_RC_DOWN       1
#define MXCSR_RC_UP         2
#define MXCSR_RC_TRUNCATE   3

#define MXCSR_MASK          28

static bool FXSR_Support, FPU_Support;

static inline void _SetFPUControlWord(uint16_t cw)
{
    // FLDCW = Load FPU Control Word
    asm volatile("  fldcw %0;   "::"m"(cw)); 
}

bool NMFaultHandler::initialise()
{
    // Check for FPU and XSAVE
    uint32_t eax, ebx, ecx, edx, mxcsr = 0;
    uint64_t cr0, cr4;

    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    Processor::cpuid(1, 0, eax, ebx, ecx, edx);
    
    if(edx & CPUID_FEAT_EDX_FPU)
    {
        FPU_Support = true;
    
        cr0 = (cr0 | CR0_NE | CR0_MP) & ~(CR0_EM | CR0_TS);
        asm volatile ("mov %0, %%cr0" :: "r" (cr0));
            
        // init the FPU
        asm volatile ("finit");
        
        // set the FPU Control Word
        _SetFPUControlWord(0x33F);
 
        asm volatile ("mov %0, %%cr0" :: "r" (cr0));
    }
    else
        FPU_Support = false;
          
    if(edx & CPUID_FEAT_EDX_FXSR)
    {
        FXSR_Support = true;
        cr4 |= CR4_OSFXSR;          // set the FXSAVE/FXRSTOR support bit
    }
    else
        FXSR_Support = false;
    
    if(edx & CPUID_FEAT_EDX_SSE)
    {
        cr4 |= CR4_OSXMMEXCPT;      // set the SIMD floating-point exception handling bit
        asm volatile("mov %0, %%cr4;"::"r"(cr4));
            
        // mask all exceptions
        mxcsr |= (MXCSR_PM | MXCSR_UM | MXCSR_OM | MXCSR_ZM | MXCSR_DM | MXCSR_IM);
            
        // set the rounding method 
        mxcsr |= (MXCSR_RC_TRUNCATE << MXCSR_RC);
            
        // write the control word
        asm volatile("ldmxcsr %0;"::"m"(mxcsr));
    }
    else
        asm volatile("mov %0, %%cr4;"::"r"(cr4));
    
    // Register the handler
    InterruptManager::instance().registerInterruptHandler(NM_FAULT_EXCEPTION, this);
        
    // set the bit that causes a DeviceNotAvailable upon SSE, MMX, or FPU instruction execution
    cr0 |= CR0_TS;
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));
    return false;
}

// old/current owner
static X64SchedulerState *x87FPU_MMX_XMM_MXCSR_StateOwner = 0;
static X64SchedulerState x87FPU_MMX_XMM_MXCSR_StateBlank;
void NMFaultHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
    // Check the TS bit
    uint64_t cr0;
    
    // new owner
    Thread *pCurrentThread = Processor::information().getCurrentThread();
    X64SchedulerState *pCurrentState = &pCurrentThread->state();
    
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
        
    // bochs breakpoint
    //asm volatile("xchg %bx, %bx;");
    
    // if this task has never used SSE before, we need to init the state space
    if(FXSR_Support)
    {
        if(!(pCurrentState->flags & (1 << 1)))
        {
            if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner != 0)
                asm volatile("fxrstor (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateBlank.x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));

            asm volatile("fxsave  (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            
            pCurrentState->flags |= (1 << 1);
        }
        
        // we don't need to save/restore if this is true!
        if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner == (size_t)pCurrentState)
            return;
        
        // if this is NULL, then no thread has ever been here before! :)
        if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner != 0)
        {
            // save the old owner's state
            asm volatile("fxsave  (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            
            if((size_t)pCurrentState != 0)
            {
                // restore the new owner's state
                asm volatile("fxrstor (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            }
        }
        else
        {
            // if no owner is defined, skip the restoration process as there's no need
            asm volatile("fxsave  (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            asm volatile("fxsave  (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateBlank.x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
        }
    }
    else if(FPU_Support)
    {
        if(!(pCurrentState->flags & (1 << 1)))
        {
            if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner != 0)
                asm volatile("frstor (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateBlank.x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));

            asm volatile("fsave  (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            
            pCurrentState->flags |= (1 << 1);
        }
        
        // we don't need to save/restore if this is true!
        if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner == (size_t)pCurrentState)
            return;
        
        // if this is NULL, then no thread has ever been here before! :)
        if((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner != 0)
        {
            // save the old owner's state
            asm volatile("fsave  (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateOwner->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            
            if((size_t)pCurrentState != 0)
            {
                // restore the new owner's state
                asm volatile("frstor (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            }
        }
        else
        {
            // if no owner is defined, skip the restoration process as there's no need
            asm volatile("fsave  (%0);"::"r"(((((size_t)pCurrentState->x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
            asm volatile("fsave  (%0);"::"r"(((((size_t)x87FPU_MMX_XMM_MXCSR_StateBlank.x87FPU_MMX_XMM_MXCSR_State + 32) & ~15) + 16)));
        }
    }
    else
    {
        ERROR("FXSAVE and FSAVE are not supported");
    }
    
    // old/current = new
    x87FPU_MMX_XMM_MXCSR_StateOwner = pCurrentState;
}

NMFaultHandler::NMFaultHandler()
{
}
