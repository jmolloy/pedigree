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

#ifndef KERNEL_MACHINE_X86_COMMON_LOCAL_APIC_H
#define KERNEL_MACHINE_X86_COMMON_LOCAL_APIC_H

#if defined(APIC)

#include <processor/types.h>
#include <processor/MemoryMappedIo.h>
#include <machine/IrqManager.h>
#include <machine/SchedulerTimer.h>
#include <processor/state.h>
#include <processor/InterruptHandler.h>

#define IPI_HALT_VECTOR                                 0xFB
#define ERROR_VECTOR                                    0xFC
#define SPURIOUS_VECTOR                                 0xFD
#define TIMER_VECTOR                                    0xFE

/** @addtogroup kernelmachinex86common
 * @{ */

/** The x86/x64 local APIC
 *\todo Initialise the Local APIC Timer */
class LocalApic : public SchedulerTimer,
                  private InterruptHandler
{
  public:
    /** The default constructor */
    inline LocalApic()
      : m_IoSpace("Local APIC"), m_Handler(0) {}
    /** The destructor */
    inline virtual ~LocalApic(){}

    /** Initialise the local APIC class. This includes allocating the I/O space.
     *\param[in] physicalAddress the physical address of the Local APIC (taken from the SMP/ACPI tables)
     *\return true, if the Local APIC class and this processor's Local APIC have been initialised
     *        successfully, false otherwise
     *\todo Needs information about BSPs LINTs */
    bool initialise(uint64_t physicalAddress) INITIALISATION_ONLY;
    /** Initialise the local APIC on the current processor
     *\return true, if this processor's Local APIC has been initialised successfully
     *\todo Needs information about the LINTs of this processor's local APIC */
    bool initialiseProcessor() INITIALISATION_ONLY;

    /** Local APIC delivery modes */
    enum
    {
      deliveryModeFixed           = 0,
      deliveryModeLowestPriority  = 1,
      deliveryModeSmi             = 2,
      deliveryModeNmi             = 4,
      deliveryModeInit            = 5,
      deliveryModeStartup         = 6,
      deliveryModeExtInt          = 7
    };

    /** Issue an IPI (= Interprocessor Interrupt)
     *\param[in] destinationApicId Identifier of the Local APIC of the destination processor
     *\param[in] vector the IPI vector
     *\param[in] deliveryMode the delivery mode
     *\param[in] bAssert Assert?
     *\param[in] bLevelTriggered Level-triggered? */
    void interProcessorInterrupt(uint8_t destinationApicId,
                                 uint8_t vector,
                                 size_t deliveryMode,
                                 bool bAssert,
                                 bool bLevelTriggered);

    /** Issue an IPI (= Interprocessor Interrupt) to all logical processors except this one.
     * (i.e. to all other cores).
     * \param[in] vector The IPI vector
     * \param[in] deliveryMode The delivery mode */
    void interProcessorInterruptAllExcludingThis(uint8_t vector,
                                                 size_t deliveryMode);

    /** Get the Local APIC Id for this processor
     *\return the Local APIC Id of this processor */
    uint8_t getId();

    //
    // SchedulerTimer interface
    //
    virtual bool registerHandler(TimerHandler *handler)
      {m_Handler = handler; return false;}

    void ack();

  private:
    /** The copy-constructor
     *\note NOT implemented */
    LocalApic(const LocalApic &);
    /** The assignment operator
     *\note NOT implemented */
    LocalApic &operator = (const LocalApic &);

    /** Check whether the local APIC is enabled and at the desired address
     *\param[in] physicalAddress the desired physical address
     *\return true, if the local APIC is enabled and at physicalAddress, false otherwise */
    bool check(uint64_t physicalAddress) INITIALISATION_ONLY;

    //
    // InterruptHandler interface
    //
    virtual void interrupt(size_t nInterruptNumber, InterruptState &state);


    /** The local APIC memory-mapped I/O space */
    MemoryMappedIo m_IoSpace;

    /** The scheduler. */
    TimerHandler *m_Handler;
};

/** @} */

#endif

#endif
