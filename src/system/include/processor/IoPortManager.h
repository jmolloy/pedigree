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
#include <processor/IoPort.h>
#include <utilities/RangeList.h>

/** @addtogroup kernelprocessor
 * @{ */

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)

  /** Singleton class which manages hardware I/O port (de)allocate
   *\brief Manages hardware I/O port (de)allocations
   *\todo functions to enumerate the I/O ports */
  class IoPortManager
  {
    public:
      /** Get the instance of the I/O manager */
      inline static IoPortManager &instance(){return m_Instance;}

      /** Allocate a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] Port pointer to the I/O port range object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to allocate
       *\todo We might want to add a module identifier
       *\return true, if the I/O port has been allocated successfull, false
       *        otherwise */
      bool allocate(const IoPort *Port,
                    io_port_t ioPort,
                    size_t size);
      /** Free a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] Port pointer to the I/O port range object */
      void free(const IoPort *Port);

      /** Initialise the IoPortManager with an initial I/O range
       *\param[in] ioPortBase base I/O port for the range
       *\param[in] size number of successive I/O ports - 1 */
      void initialise(io_port_t ioPortBase, size_t size);

    private:
      /** The default constructor */
      inline IoPortManager()
        : m_List(){}
      /** The destructor */
      inline virtual ~IoPortManager(){}
      /** The copy-constructor
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager(const IoPortManager &);
      /** The assignment operator
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager &operator = (const IoPortManager &);

      /** The list of free I/O ports */
      RangeList<uint32_t> m_List;

      /** The IoPortManager instance */
      static IoPortManager m_Instance;
  };

#endif

/** @} */

#endif
