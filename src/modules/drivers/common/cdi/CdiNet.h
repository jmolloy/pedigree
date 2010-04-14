/*
* Copyright (c) 2009 Kevin Wolf <kevin@tyndur.org>
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
#ifndef CDI_CPP_NET_H
#define CDI_CPP_NET_H

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Network.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/IrqHandler.h>
#include <process/Thread.h>
#include <process/Semaphore.h>

#include "cdi/net.h"

/** CDI NIC Device */
class CdiNet : public Network
{
    public:
        CdiNet(Network* pDev, struct cdi_net_device* device);
        ~CdiNet();

        virtual void getName(String &str)
        {
            // TODO Get the name from the CDI driver
            if(!m_Device)
                str = "cdi-net";
            else
            {
                str = String(m_Device->dev.name);
            }
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);
        virtual StationInfo getStationInfo();

    private:
        CdiNet(const CdiNet&);
        const CdiNet & operator = (const CdiNet&);
                
        /// Called when a packet is picked up by the system
        void gotPacket()
        {
            m_StationInfo.nPackets++;
        }
          
        /// Called when a packet is dropped by the system
        void droppedPacket()
        {
            m_StationInfo.nDropped++;
        }
          
        /// Called when a packet is determined to be "bad" by the system (ie, invalid
        /// checksum).
        void badPacket()
        {
            m_StationInfo.nBad++;
        }

        struct cdi_net_device* m_Device;
        StationInfo m_StationInfo;
};

#endif
