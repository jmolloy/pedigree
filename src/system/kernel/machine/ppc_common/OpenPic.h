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

#ifndef KERNEL_MACHINE_PPC_COMMON_PIC_H
#define KERNEL_MACHINE_PPC_COMMON_PIC_H

#include <processor/MemoryMappedIo.h>
#include <processor/InterruptManager.h>
#include <machine/IrqManager.h>

/** @addtogroup kernelmachineppccommon
 * @{ */


// CPU registers - PPC implementation of OpenPIC does not need to implement the "this processor"
//                 registers at 0x0-0x1000, so we use the global ones at 0x20000.
#define OPENPIC_REG_TASK_PRIORITY 0x20080
#define OPENPIC_REG_ACK      0x200A0
#define OPENPIC_REG_EOI      0x200B0

// Global registers.
#define OPENPIC_REG_FEATURE  0x01000
#define OPENPIC_REG_CONF0    0x01020
#define OPENPIC_FLAG_CONF0_P 0x20000000 // 8259 passthrough DISABLE - 8259 passthrough is enabled by default, which basically turns off half of the PIC. It doesn't apply to us.
#define OPENPIC_REG_SPURIOUS 0x010E0

// IRQ sources.
#define OPENPIC_SOURCE_START 0x10000
#define OPENPIC_SOURCE_END   0x20000

#define OPENPIC_SOURCE_MASK  0x80000000
#define OPENPIC_SOURCE_ACT   0x40000000
#define OPENPIC_SOURCE_PRIORITY 0x00080000 // Default priority = 15 (highest)

/** The PPC's programmable interrupt controller as IrqManager */
class OpenPic : public IrqManager,
                private InterruptHandler
{
  public:
    /** Get the Pic class instance
     *\return the Pic class instance */
    inline static OpenPic &instance(){return m_Instance;}

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
    OpenPic() INITIALISATION_ONLY;
    /** The destructor */
    inline virtual ~OpenPic(){}
    /** The copy-constructor
     *\note NOT implemented */
    OpenPic(const OpenPic &);
    /** The assignment operator
     *\note NOT implemented */
    OpenPic &operator = (const OpenPic &);

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
    static OpenPic m_Instance;

    struct Feature
    {
      uint32_t reserved0 : 5;
      uint32_t num_irq : 11;
      uint32_t reserved1 : 3;
      uint32_t num_cpu : 5;
      uint32_t version : 8;
    };
};

/** @} */

#endif
