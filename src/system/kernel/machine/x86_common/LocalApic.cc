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

#include <Log.h>
#include "LocalApic.h"
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/Machine.h>
#include <processor/InterruptManager.h>

#define LAPIC_REG_ID                                    0x0020
#define LAPIC_REG_VERSION                               0x0030
#define LAPIC_REG_TASK_PRIORITY                         0x0080
#define LAPIC_REG_PROCESSOR_PRIORITY                    0x00A0
#define LAPIC_REG_EOI                                   0x00B0
#define LAPIC_REG_LOGICAL_DESTINATION                   0x00D0
#define LAPIC_REG_DESTINATION_FORMAT                    0x00E0
#define LAPIC_REG_SPURIOUS_INT                          0x00F0
// NOTE ISR
// NOTE TMR
// NOTE IRR
#define LAPIC_REG_ERR_STATUS                            0x0280
#define LAPIC_REG_INT_CMD_LOW                           0x0300
#define LAPIC_REG_INT_CMD_HIGH                          0x0310
#define LAPIC_REG_LVT_TIMER                             0x0320
#define LAPIC_REG_LVT_THERMAL                           0x0330
#define LAPIC_REG_LVT_PERFORMANCE                       0x0340
#define LAPIC_REG_LVT_LINT0                             0x0350
#define LAPIC_REG_LVT_LINT1                             0x0360
#define LAPIC_REG_LVT_ERROR                             0x0370
#define LAPIC_REG_INITIAL_COUNT                         0x0380
#define LAPIC_REG_CURRENT_COUNT                         0x0390
#define LAPIC_REG_DIVIDE_CONFIG                         0x03E0

#define LAPIC_TIMER_PERIODIC                            0x00020000
#define LAPIC_MASKED                                    0x00010000

/** Assume 1GHz bus speed, 128 divisor, gives 7812500 for one second delay.
    For 10ms delay, divide that by 100... */
#define INITIAL_COUNT_VALUE (78125*40)

bool LocalApic::initialise(uint64_t physicalAddress)
{
  // Detect local APIC presence
  uint32_t eax, ebx, ecx, edx;
  Processor::cpuid(1, 0, eax, ebx, ecx, edx);
  if (((edx >> 9) & 0x01) != 0x01)
  {
    ERROR("Local APIC: No local APIC present");
    return false;
  }

  // Some checks
  if (check(physicalAddress) == false)
    return false;

  // Allocate the local APIC memory-mapped I/O space
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  if (physicalMemoryManager.allocateRegion(m_IoSpace,
                                           1,
                                           PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory | PhysicalMemoryManager::force,
                                           VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write | VirtualAddressSpace::CacheDisable,
                                           physicalAddress)
      == false)
  {
    ERROR("Local APIC: Could not allocate the memory region");
    return false;
  }

  // Register the timer vector.
  if (!InterruptManager::instance().registerInterruptHandler(TIMER_VECTOR, this))
    return false;

  // Register the IPI halt vector.
  if (!InterruptManager::instance().registerInterruptHandler(IPI_HALT_VECTOR, this))
    return false;

  return initialiseProcessor();
}

bool LocalApic::initialiseProcessor()
{
  // Some checks
  if (check(m_IoSpace.physicalAddress()) == false)
    return false;

  // Enable the Local APIC and set the spurious interrupt vector
  uint32_t tmp = m_IoSpace.read32(LAPIC_REG_SPURIOUS_INT);
  m_IoSpace.write32((tmp & 0xFFFFFE00) | 0x100 | SPURIOUS_VECTOR, LAPIC_REG_SPURIOUS_INT);

  // Set the task priority to 0
  tmp = m_IoSpace.read32(LAPIC_REG_TASK_PRIORITY);
  m_IoSpace.write32(tmp & 0xFFFFFF00, LAPIC_REG_TASK_PRIORITY);

  // Set the LVT error register
  tmp = m_IoSpace.read32(LAPIC_REG_LVT_ERROR);
  m_IoSpace.write32((tmp & 0xFFFEEF00) | ERROR_VECTOR, LAPIC_REG_LVT_ERROR);

  // Set the LVT timer register.
  m_IoSpace.write32(LAPIC_TIMER_PERIODIC | TIMER_VECTOR, LAPIC_REG_LVT_TIMER);

  // Initialise the intial-count register
  m_IoSpace.write32(INITIAL_COUNT_VALUE, LAPIC_REG_INITIAL_COUNT);

  // Initialise the divisor register. (Divide by 128)
  m_IoSpace.write32(0xA, LAPIC_REG_DIVIDE_CONFIG);

  // TODO

  return true;
}

void LocalApic::interProcessorInterrupt(uint8_t destinationApicId,
                                        uint8_t vector,
                                        size_t deliveryMode,
                                        bool bAssert,
                                        bool bLevelTriggered)
{
  while ((m_IoSpace.read32(LAPIC_REG_INT_CMD_LOW) & 0x1000) != 0);

  m_IoSpace.write32(destinationApicId << 24, LAPIC_REG_INT_CMD_HIGH);
  m_IoSpace.write32(vector | (deliveryMode << 8) | (bAssert ? (1 << 14) : 0) | (bLevelTriggered ? (1 << 15) : 0), LAPIC_REG_INT_CMD_LOW);
}

void LocalApic::interProcessorInterruptAllExcludingThis(uint8_t vector,
                                                        size_t deliveryMode)
{
  while ((m_IoSpace.read32(LAPIC_REG_INT_CMD_LOW) & 0x1000) != 0);

  m_IoSpace.write32(vector | (deliveryMode << 8) | (1 << 14) | (0x3 << 18), LAPIC_REG_INT_CMD_LOW);
}


uint8_t LocalApic::getId()
{
  return ((m_IoSpace.read32(LAPIC_REG_ID) >> 24) & 0xFF);
}

bool LocalApic::check(uint64_t physicalAddress)
{
  // Check whether the Local APIC is enabled or not
  if ((Processor::readMachineSpecificRegister(0x1B) & 0x800) == 0)
  {
    ERROR("Local APIC: Disabled");
    return false;
  }

  // Check Local APIC base address
  if ((Processor::readMachineSpecificRegister(0x1B) & 0xFFFFFF000ULL) != physicalAddress)
  {
    ERROR("Local APIC: Wrong physical address");
    return false;
  }

  return true;
}

void LocalApic::interrupt(size_t nInterruptNumber, InterruptState &state)
{
  if (nInterruptNumber == TIMER_VECTOR)
  {
    // TODO: Delta is wrong.
    if (LIKELY(m_Handler != 0))
    {
      NOTICE("Timer " << Processor::id());
      m_Handler->timer (0, state);
    }
    ack();
  }

  // The halt IPI is used in the debugger to stop all other cores.
  if (nInterruptNumber == IPI_HALT_VECTOR)
  {
    NOTICE("Halting processor #" << Dec << Processor::id());
    Processor::halt();
  }
}

void LocalApic::ack()
{
  // Send EOI.
  m_IoSpace.write32(0x00000000, LAPIC_REG_EOI);
}
