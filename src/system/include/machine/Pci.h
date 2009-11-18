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

#ifndef PCI_COMMON_H
#define PCI_COMMON_H

#include <processor/types.h>
#include <machine/Device.h>

/** Architecture-independent interface to a PCI bus */
class PciBus
{
    public:
        PciBus();
        virtual ~PciBus();

        static PciBus &instance()
        {
            return m_Instance;
        }

        /**
         * Initialises the object for use.
         */
        void initialise();

        /**
         * Reads from the configuration space
         * \param pDev the device to read configuration space for
         * \param offset the offset into the configuration space to read
         */
        uint32_t readConfigSpace(Device *pDev, uint8_t offset);

        /**
         * Reads from the configuration space
         * \param bus bus number for the read address
         * \param device device number for the read address
         * \param function function number for the read address
         * \param offset the offset into the configuration space to read
         */
        uint32_t readConfigSpace(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

        /**
         * Writes to the configuration space
         * \param pDev the device to write configuration space for
         * \param offset the offset into the configuration space to write
         * \param data the data to write
         */
        void writeConfigSpace(Device *pDev, uint8_t offset, uint32_t data);

        /**
         * Writes to the configuration space
         * \param bus bus number for the write address
         * \param device device number for the write address
         * \param function function number for the write address
         * \param offset the offset into the configuration space to write
         * \param data the data to write
         */
        void writeConfigSpace(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data);

    private:
        static PciBus m_Instance;
};

#endif
