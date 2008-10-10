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

#ifndef KERNEL_MACHINE_PPC_COMMON_HEATHROW_H
#define KERNEL_MACHINE_PPC_COMMON_HEATHROW_H

#include <processor/MemoryMappedIo.h>
#include <processor/InterruptManager.h>
#include <machine/IrqManager.h>

/** @addtogroup kernelmachineppccommon
 * @{ */

/** The PPC's programmable interrupt controller as IrqManager */
class Heathrow : public IrqManager,
                 private InterruptHandler
{
  public:
    /** Get the Pic class instance
     *\return the Pic class instance */
    inline static Heathrow &instance(){return m_Instance;}

    //
    // IrqManager interface
    //
    virtual irq_id_t registerIsaIrqHandler(uint8_t irq, IrqHandler *handler);
    virtual irq_id_t registerPciIrqHandler(IrqHandler *handler);
    virtual void acknowledgeIrq(irq_id_t Id);
    virtual void unregisterHandler(irq_id_t Id, IrqHandler *handler);

    /** Initialises the PIC hardware and registers the interrupts with the
     *  InterruptManager.
     *\return true, if successfull, false otherwise */
    bool initialise() INITIALISATION_ONLY;

  private:
    /** The default constructor */
    Heathrow() INITIALISATION_ONLY;
    /** The destructor */
    inline virtual ~Heathrow(){}
    /** The copy-constructor
     *\note NOT implemented */
    Heathrow(const Heathrow &);
    /** The assignment operator
     *\note NOT implemented */
    Heathrow &operator = (const Heathrow &);

    void searchNode(class Device *pDev);

    //
    // InterruptHandler interface
    //
    virtual void interrupt(size_t interruptNumber, InterruptState &state);

    void eoi(uint8_t irq);
    void enable(uint8_t irq, bool enable);
    void enableAll(bool enable);

    /** The PIC I/O Port range */
    IoBase *m_pPort;

    /** The IRQ handler */
    IrqHandler *m_Handler[64];

    /** The number of IRQs available */
    size_t m_nIrqs;

    /** The Pic instance */
    static Heathrow m_Instance;

  uint32_t m_LowMask;
  uint32_t m_HighMask;
};

/** @} */

#endif
