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
#ifndef KERNEL_PROCESSOR_X86_COMMON_IOPORTMANAGER_H
#define KERNEL_PROCESSOR_X86_COMMON_IOPORTMANAGER_H

#include <utilities/RangeList.h>
#include <processor/IoPortManager.h>

/** @addtogroup kernelprocessorx86common
 * @{ */

class X86CommonIoPortManager : public IoPortManager
{
  public:
    /** Get the X86CommonIoPortManager instance
     *\return instance of the X86CommonIoPortManager */
    inline static X86CommonIoPortManager &instance(){return m_Instance;}

    //
    // IoPortManager interface
    //
    virtual bool allocate(io_port_t ioPort, size_t size);
    virtual void free(io_port_t ioPort, size_t size);

    void initialise();

  private:
    /** The default constructor */
    inline X86CommonIoPortManager()
      : m_List(){}
    /** The destructor */
    inline ~X86CommonIoPortManager(){}
    /** The copy-constructor
     *\note No implementation provided (X86CommonIoPortManager is a singleton) */
    X86CommonIoPortManager(const X86CommonIoPortManager &);
    /** The assignment operator
     *\note No implementation provided (X86CommonIoPortManager is a singleton) */
    X86CommonIoPortManager &operator = (const X86CommonIoPortManager &);

    /** The list of free I/O ports */
    RangeList<uint32_t> m_List;

    static X86CommonIoPortManager m_Instance;
};

/** @} */

#endif
