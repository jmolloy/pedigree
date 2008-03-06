/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_MACHINE_IRQMANAGER_H
#define KERNEL_MACHINE_IRQMANAGER_H

#include <machine/types.h>
#include <machine/IrqHandler.h>

/** @ingroup kernelmachine
 * @{ */

/** This class handles IRQ (un)registration */
class IrqManager
{
  public:
    /** Get the IrqManager instance */
    static IrqManager &instance();

    /** Register an ISA irq
     *\param[in] irq the ISA irq number (from 0 to 15)
     *\param[in] handler pointer to the IrqHandler class
     *\return the irq's identifier */
    virtual irq_id_t registerIsaIrqHandler(uint8_t irq, IrqHandler *handler) = 0;
    /** Register a PCI irq
     *\todo what parameters do we need here? Perhaps a PCIDevice class or something? */
    virtual irq_id_t registerPciIrqHandler(IrqHandler *handler) = 0;
    /** Acknoledge the IRQ reception, in case you returned false in the IrqHandler::irq() function. If
     *  this is not called there won't be any following irqs.
     *\param[in] Id the irq's identifier */
    virtual void acknowledgeIrq(irq_id_t Id) = 0;
    /** Unregister a previously registered IrqHandler
     *\param[in] Id the irq's identifier */
    virtual void unregisterHandler(irq_id_t Id, IrqHandler *handler) = 0;

  protected:
    /** The default constructor */
    inline IrqManager(){}
    /** The destructor */
    inline virtual ~IrqManager(){}
  private:
    /** The copy-constructor
     *\note NOT implemented */
    IrqManager(const IrqManager &);
    /** The assignment operator
     *\note NOT implemented */
    IrqManager &operator = (const IrqManager &);
};

/** @} */

#endif
