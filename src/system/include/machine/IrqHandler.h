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

#ifndef KERNEL_MACHINE_IRQHANDLER_H
#define KERNEL_MACHINE_IRQHANDLER_H

#include <machine/types.h>
#include <processor/state.h>

/** @addtogroup kernelmachine
 * @{ */

/** Abstract base class for all irq-handlers. All irq-handlers must
 * be derived from this class */
class IrqHandler
{
  public:
    /** Called when the handler is registered with the irq manager and the irq occurred
     *\note If this function returns false you have to call IrqManager::acknowledgeIrq() when
     *      you removed the interrupt reason.
     *\param[in] number the irq number
     *\return should return true, if the interrupt reason was removed, or false otherwise */
    virtual bool irq(irq_id_t number, InterruptState &state) = 0;

  protected:
    /** Virtual destructor */
    inline virtual ~IrqHandler(){}
};

/** @} */

#endif
