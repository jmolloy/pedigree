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

#ifndef KERNEL_CORE_PROCESSOR_NMFAULTHANDLER_H_
#define KERNEL_CORE_PROCESSOR_NMFAULTHANDLER_H_

#include <processor/InterruptManager.h>

/** @addtogroup kernelprocessor
 * @{ */

/** The x86 Device Not Present(NM) Exception handler. */
class NMFaultHandler : private InterruptHandler
{
public:
    /** Get the NMFaultHandler instance
     *  \return the NMFaultHandler instance.  */
    inline static NMFaultHandler& instance()  {return m_Instance;}

    /** Register the NMFaultHandler with the InterruptManager.
     * \return true if sucessful, false otherwise.  */
    bool initialise();

    //
    // InterruptHandler interface.
    //
    virtual void interrupt(size_t interruptNumber, InterruptState &state);
private:
    /** The default constructor.  */
    NMFaultHandler() INITIALISATION_ONLY;

    /**The copy constructor.
     * Note not implemented.  */
    NMFaultHandler(const NMFaultHandler&);

    /** The NMFaultHandler instance */
    static NMFaultHandler m_Instance;
};

/** @} */

#endif /* KERNEL_CORE_PROCESSOR_X86_NMFAULTHANDLER_H_ */
