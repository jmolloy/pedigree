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
#ifndef KERNEL_PROCESSOR_IOPORTMANAGER_H
#define KERNEL_PROCESSOR_IOPORTMANAGER_H

#include <processor/types.h>

/** @addtogroup kernelprocessor
 * @{ */

#ifndef KERNEL_PROCESSOR_NO_PORT_IO

  /** Singleton class which manages hardware I/O port (de)allocate
   *\brief Manages hardware I/O port (de)allocations
   *\todo functions to enumerate the I/O ports */
  class IoPortManager
  {
    public:
      /** Get the instance of the I/O manager */
      static IoPortManager &instance();

      /** Allocate a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to allocate
       *\todo We might want to add the module name, or some equivalent here
       *\return true, if the I/O port has been allocated successfull, false
       *        otherwise */
      virtual bool allocate(io_port_t ioPort, size_t size) = 0;
      /** Free a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to free
       *\todo do we really need the second parameter? */
      virtual void free(io_port_t ioPort, size_t size) = 0;

    protected:
      /** The default constructor */
      inline IoPortManager(){}
      /** The destructor */
      inline virtual ~IoPortManager(){}

    private:
      /** The copy-constructor
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager(const IoPortManager &);
      /** The assignment operator
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager &operator = (const IoPortManager &);
  };

#endif

/** @} */

#endif
