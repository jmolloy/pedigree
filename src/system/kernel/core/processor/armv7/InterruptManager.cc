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

#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <panic.h>
#ifdef DEBUGGER
  #include <Debugger.h>
#endif
#include <Log.h>

#define SYSCALL_INTERRUPT_NUMBER 8
#define BREAKPOINT_INTERRUPT_NUMBER 9

const char *g_ExceptionNames[32] = {
  "Interrupt",
  "TLB modification exception",
  "TLB exception (load or instruction fetch)",
  "TLB exception (store)",
  "Address error exception (load or instruction fetch)",
  "Address error exception (store)",
  "Bus error exception (instruction fetch)",
  "Bus error exception (data: load or store)",
  "Syscall exception",
  "Breakpoint exception",
  "Reserved instruction exception",
  "Coprocessor unusable exception",
  "Arithmetic overflow exception",
  "Trap exception",
  "LDCz/SDCz to uncached address",
  "Virtual coherency exception",
  "Machine check exception",
  "Floating point exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Watchpoint exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
};

ARMV7InterruptManager ARMV7InterruptManager::m_Instance;

MemoryRegion ARMV7InterruptManager::m_MPUINTCRegion("mpu-intc");

SyscallManager &SyscallManager::instance()
{
  return ARMV7InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return ARMV7InterruptManager::instance();
}

bool ARMV7InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  /// \todo This is very machine-specific...
  volatile uint32_t *mpuIntcRegisters = reinterpret_cast<volatile uint32_t*>(ARMV7InterruptManager::m_MPUINTCRegion.virtualAddress());
  if(!mpuIntcRegisters)
    return false;

  /// \todo Needs locking
  if (UNLIKELY(interruptNumber >= 96))
    return false;
  if (UNLIKELY(handler != 0 && m_Handler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_Handler[interruptNumber] == 0))
    return false;

  // Unmask this interrupt
  size_t n = (interruptNumber % 96) / 32;
  mpuIntcRegisters[INTCPS_MIR_CLEAR + (n * 8)] = 1 << (interruptNumber % 32);

  m_Handler[interruptNumber] = handler;
  return true;
}

#ifdef DEBUGGER

  bool ARMV7InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {  
    volatile uint32_t *mpuIntcRegisters = reinterpret_cast<volatile uint32_t*>(ARMV7InterruptManager::m_MPUINTCRegion.virtualAddress());
    if(!mpuIntcRegisters)
        return false;

    /// \todo Needs locking
    if (UNLIKELY(interruptNumber >= 96))
      return false;
    if (UNLIKELY(handler != 0 && m_DbgHandler[interruptNumber] != 0))
      return false;
    if (UNLIKELY(handler == 0 && m_DbgHandler[interruptNumber] == 0))
      return false;

    // Unmask this interrupt
    size_t n = (interruptNumber % 96) / 32;
    mpuIntcRegisters[INTCPS_MIR_CLEAR + (n * 8)] = 1 << (interruptNumber % 32);

    m_DbgHandler[interruptNumber] = handler;
    return true;
  }
  size_t ARMV7InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t ARMV7InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool ARMV7InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  //TODO: Needs locking

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(handler != 0 && m_SyscallHandler[Service] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_SyscallHandler[Service] == 0))
    return false;

  m_SyscallHandler[Service] = handler;
  return true;
}

uintptr_t ARMV7InterruptManager::syscall(Service_t service,
                                            uintptr_t function,
                                            uintptr_t p1, uintptr_t p2,
                                            uintptr_t p3, uintptr_t p4,
                                            uintptr_t p5)
{
    /// \todo Software interrupt
    return 0;
}

extern "C" void arm_swint_handler() __attribute__((interrupt("SWI")));
extern "C" void arm_instundef_handler()__attribute__((naked));
extern "C" void arm_fiq_handler() __attribute__((interrupt("FIQ")));
extern "C" void arm_irq_handler(InterruptState &state);
extern "C" void arm_reset_handler() __attribute__((naked));
extern "C" void arm_prefetch_abort_handler() __attribute__((naked));
extern "C" void arm_data_abort_handler() __attribute__((naked));
extern "C" void arm_addrexcept_handler() __attribute__((naked));

extern "C" void arm_swint_handler()
{
  NOTICE_NOLOCK("swi");
}

extern "C" void arm_instundef_handler()
{
  NOTICE_NOLOCK("undefined instruction");
  while( 1 );
}

extern "C" void arm_fiq_handler()
{
  NOTICE_NOLOCK("FIQ");
  while( 1 );
}

extern "C" void arm_irq_handler(InterruptState &state)
{
  // InterruptState state; /// \todo Do something useful with this
  ARMV7InterruptManager::interrupt(state);
}

extern "C" void arm_reset_handler()
{
  NOTICE_NOLOCK("reset");
  while( 1 );
}

extern "C" void arm_prefetch_abort_handler()
{
  NOTICE_NOLOCK("prefetch abort");
  while( 1 );
}

extern "C" void arm_data_abort_handler()
{
  NOTICE_NOLOCK("data abort");

  uintptr_t dfar = 0;
  asm volatile("MRC p15,0,%0,c6,c0,0" : "=r" (dfar));

  NOTICE_NOLOCK("Address: " << dfar);

  uint32_t dfsr = 0;
  asm volatile("MRC p15,0,%0,c5,c0,0" : "=r" (dfsr));

  NOTICE_NOLOCK("Status: " << dfsr);

  uintptr_t linkreg = 0;
  asm volatile("mov %0, lr" : "=r" (linkreg));
  NOTICE_NOLOCK("Link register: " << linkreg);

  while( 1 );
}

extern "C" void arm_addrexcept_handler()
{
  NOTICE_NOLOCK("address exception");
  while( 1 );
}

extern uint32_t __arm_vector_table;
extern uint32_t __end_arm_vector_table;
void ARMV7InterruptManager::initialiseProcessor()
{
    // Map in the MPU interrupt controller
    if(!PhysicalMemoryManager::instance().allocateRegion(ARMV7InterruptManager::m_MPUINTCRegion,
                                                         1,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         0x48200000))
        return;

    // Map in the ARM vector table to 0xFFFF0000
    if(!VirtualAddressSpace::getKernelAddressSpace().map(reinterpret_cast<physical_uintptr_t>(&__arm_vector_table),
                                                         reinterpret_cast<void*>(0xFFFF0000),
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode))
        return;

    // Switch to the high vector for the exception base
    uint32_t sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    if(!(sctlr & 0x2000))
        sctlr |= 0x2000;
    asm volatile("MCR p15,0,%0,c1,c0,0" : : "r" (sctlr));

    // Initialise the MPU INTC
    volatile uint32_t *mpuIntcRegisters = reinterpret_cast<volatile uint32_t*>(ARMV7InterruptManager::m_MPUINTCRegion.virtualAddress());

    // Perform a reset of the MPU INTC
    mpuIntcRegisters[INTCPS_SYSCONFIG] = 2;
    while((mpuIntcRegisters[INTCPS_SYSSTATUS] & 1) == 0);

    // Write the MMIO address and hardware revisio to the console
    uint32_t revision = mpuIntcRegisters[0];
    NOTICE("MPU interrupt controller at " << Hex << reinterpret_cast<uintptr_t>(ARMV7InterruptManager::m_MPUINTCRegion.virtualAddress()) << "  - revision " << Dec << ((revision >> 4) & 0xF) << "." << (revision & 0xF) << Hex);

    // Set up the functional clock auto-idle and the synchronizer clock auto-gating
    mpuIntcRegisters[INTCPS_IDLE] = 0;

    // Program the base priority and enable FIQs where necessary for all IRQs
    for(size_t m = 0; m < 96; m++)
    {
        // Priority: 0 (highest), route to IRQ (not FIQ)
        mpuIntcRegisters[INTCPS_ILR + m] = 0;
    }

    // Mask all interrupts (when an interrupt line is registered, it will be
    // unmasked)
    for(size_t n = 0; n < 3; n++)
    {
        mpuIntcRegisters[INTCPS_MIR_SET + (n * 8)] = 0xFFFFFFFF;
        mpuIntcRegisters[INTCPS_ISR_CLEAR + (n * 8)] = 0xFFFFFFFF;
    }

    // Disable the priority threshold
    mpuIntcRegisters[INTCPS_THRESHOLD] = 0xFF;

    // Reset IRQ and FIQ output in case any are pending
    mpuIntcRegisters[INTCPS_CONTROL] = 3;

}

void ARMV7InterruptManager::interrupt(InterruptState &interruptState)
{
    volatile uint32_t *mpuIntcRegisters = reinterpret_cast<volatile uint32_t*>(ARMV7InterruptManager::m_MPUINTCRegion.virtualAddress());
    if(!mpuIntcRegisters)
        return;

    // Grab the interrupt number
    size_t intNumber = mpuIntcRegisters[INTCPS_SIR_IRQ] & 0x7F;

    #ifdef DEBUGGER
        // Call the kernel debugger's handler, if any
        if (m_Instance.m_DbgHandler[intNumber] != 0)
            m_Instance.m_DbgHandler[intNumber]->interrupt(intNumber, interruptState);
    #endif

    // Call the interrupt handler if one exists
    if (m_Instance.m_Handler[intNumber] != 0)
        m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);

    // Ack the interrupt
    mpuIntcRegisters[INTCPS_CONTROL] = 1; // Reset IRQ output and enable new IRQs

#if 0 // Below is all exception/syscall stuff

  // Call the syscall handler, if it is the syscall interrupt
  if (intNumber == SYSCALL_INTERRUPT_NUMBER)
  {
    size_t serviceNumber = interruptState.getSyscallService();
    if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_SyscallHandler[serviceNumber] != 0))
      m_Instance.m_SyscallHandler[serviceNumber]->syscall(interruptState);
  }
  // Call the normal interrupt handler, if any, otherwise
  else if (m_Instance.m_Handler[intNumber] != 0)
    m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
  else
  {
    /// \todo: Check for debugger initialisation.
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #");
    e.append (intNumber, 10);
    e.append (": \"");
    e.append (g_ExceptionNames[intNumber]);
    e.append ("\"");
#ifdef DEBUGGER
    Debugger::instance().start(interruptState, e);
#else
    panic(e);
#endif
  }

#endif
}

ARMV7InterruptManager::ARMV7InterruptManager()
{
  // Initialise the pointers to the interrupt handler
  for (size_t i = 0;i < 256;i++)
  {
    m_Handler[i] = 0;
    #ifdef DEBUGGER
      m_DbgHandler[i] = 0;
    #endif
  }

  // Initialise the pointers to the syscall handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_SyscallHandler[i] = 0;
}
ARMV7InterruptManager::~ARMV7InterruptManager()
{

}
