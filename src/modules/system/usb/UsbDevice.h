/*
 * Copyright (c) 2010 Eduard Burtescu
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

#ifndef USBDEVICE_H
#define USBDEVICE_H

#include <machine/Device.h>
#include <processor/types.h>
#include <usb/Usb.h>
#include <utilities/assert.h>

namespace RequestType
{
    enum RequestType
    {
        Standard    = 0x00,
        Class       = 0x20,
        Vendor      = 0x40
    };
};

namespace RequestRecipient
{
    enum RequestRecipient
    {
        Device      = 0x00,
        Interface   = 0x01,
        Endpoint    = 0x02,
        Other       = 0x03
    };
};

namespace RequestDirection
{
    enum RequestDirection
    {
        Out = 0x00,
        In  = 0x80
    };
};

namespace Request
{
    enum Request
    {
        GetStatus       = 0,
        ClearFeature    = 1,
        SetFeature      = 3,
        SetAddress      = 5,
        GetDescriptor   = 6,
        SetDescriptor   = 7,
        GetConfiguration= 8,
        SetConfiguration= 9,
        GetInterface    = 10,
        SetInterface    = 11,
        SynchFrame      = 12,
    };
};

class UsbDevice : public virtual Device
{
    public:

        typedef struct Setup
        {
            inline Setup(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length) :
                nRequestType(requestType), nRequest(request), nValue(value), nIndex(index), nLength(length) {}

            uint8_t nRequestType;
            uint8_t nRequest;
            uint16_t nValue;
            uint16_t nIndex;
            uint16_t nLength;
        } PACKED Setup;

        typedef struct UnknownDescriptor
        {
            inline UnknownDescriptor(uint8_t *pBuffer, uint8_t type, size_t length) : pDescriptor(pBuffer), nType(type), nLength(length) {}

            void *pDescriptor;
            uint8_t nType;
            size_t nLength;
        } UnknownDescriptor;

        typedef struct Endpoint
        {
            inline Endpoint(void *pBuffer) : pDescriptor(static_cast<struct Descriptor*>(pBuffer)),
                nEndpoint(pDescriptor->nEndpoint), bIn(false), bOut(false),
                nTransferType(pDescriptor->nTransferType), nMaxPacketSize(pDescriptor->nMaxPacketSize),
                bDataToggle(false)
            {
                bIn = pDescriptor->bDirection;
                bOut = !bIn;
            }

            enum TransferTypes
            {
                Control = 0,
                Isochronus = 1,
                Bulk = 2,
                Interrupt = 3
            };

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint8_t nEndpoint : 4;
                uint8_t res0 : 3;
                uint8_t bDirection : 1;
                uint8_t nTransferType : 2;
                uint8_t res1 : 6;
                uint16_t nMaxPacketSize : 11;
                uint8_t res2 : 5;
                uint8_t nInterval;
            } PACKED *pDescriptor;

            uint8_t nEndpoint;
            bool bIn;
            bool bOut;
            uint8_t nTransferType;
            uint16_t nMaxPacketSize;

            bool bDataToggle;
        } Endpoint;

        typedef struct Interface
        {
            inline Interface(void *pBuffer) : pDescriptor(static_cast<struct Descriptor*>(pBuffer)) {}

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint8_t nInterface;
                uint8_t nAlternateSetting;
                uint8_t nEndpoints;
                uint8_t nClass;
                uint8_t nSubclass;
                uint8_t nProtocol;
                uint8_t nString;
            } PACKED *pDescriptor;

            Vector<Endpoint*> pEndpoints;
            Vector<UnknownDescriptor*> pOtherDescriptors;
            String sString;
        } Interface;

        typedef struct ConfigDescriptor
        {
            inline ConfigDescriptor(void *pConfigBuffer, size_t length) : pDescriptor(static_cast<struct Descriptor*>(pConfigBuffer))
            {
                uint8_t *pBuffer = static_cast<uint8_t*>(pConfigBuffer);
                size_t nOffset = pBuffer[0];
                Interface *pCurrentInterface = 0;

                while(nOffset < length)
                {
                    size_t nLength = pBuffer[nOffset];
                    uint8_t nType = pBuffer[nOffset + 1];
                    if(nType == 4)
                    {
                        if(pCurrentInterface)
                            assert(pCurrentInterface->pEndpoints.count() == pCurrentInterface->pDescriptor->nEndpoints);
                        Interface *pNewInterface = new Interface(&pBuffer[nOffset]);
                        if(!pNewInterface->pDescriptor->nAlternateSetting)
                        {
                            assert(pInterfaces.count() < pDescriptor->nInterfaces);
                            pCurrentInterface = pNewInterface;
                            pInterfaces.pushBack(pCurrentInterface);
                        }
                    }
                    else if(pCurrentInterface)
                    {
                        if((nType == 5) && (pCurrentInterface->pDescriptor->nEndpoints))
                        {
                            assert(pCurrentInterface->pEndpoints.count() < pCurrentInterface->pDescriptor->nEndpoints);
                            pCurrentInterface->pEndpoints.pushBack(new Endpoint(&pBuffer[nOffset]));
                        }
                        else
                            pCurrentInterface->pOtherDescriptors.pushBack(new UnknownDescriptor(&pBuffer[nOffset], nType, nLength));
                    }
                    else
                        pOtherDescriptors.pushBack(new UnknownDescriptor(&pBuffer[nOffset], nType, nLength));
                    nOffset += nLength;
                }
                if(pCurrentInterface)
                    assert(pCurrentInterface->pEndpoints.count() == pCurrentInterface->pDescriptor->nEndpoints);
                assert((pInterfaces.count() == pDescriptor->nInterfaces) && pInterfaces.count());
            }

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint16_t nTotalLength;
                uint8_t nInterfaces;
                uint8_t nConfig;
                uint8_t nString;
                uint8_t nAttributes;
                uint8_t nMaxPower;
            } PACKED *pDescriptor;

            Vector<Interface*> pInterfaces;
            Vector<UnknownDescriptor*> pOtherDescriptors;
            String sString;
        } ConfigDescriptor;

        typedef struct DeviceDescriptor
        {
            inline DeviceDescriptor(void *pBuffer) : pDescriptor(static_cast<struct Descriptor*>(pBuffer)) {}

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint16_t nBcdUsbRelease;
                uint8_t nClass;
                uint8_t nSubclass;
                uint8_t nProtocol;
                uint8_t nMaxPacketSize;
                uint16_t nVendorId;
                uint16_t nProductId;
                uint16_t nBcdDeviceRelease;
                uint8_t nVendorString;
                uint8_t nProductString;
                uint8_t nSerialString;
                uint8_t nConfigurations;
            } PACKED *pDescriptor;

            Vector<ConfigDescriptor*> pConfigurations;
            String sVendor;
            String sProduct;
            String sSerial;
        } DeviceDescriptor;

        /** Constructors and destructors */
        inline UsbDevice() : m_nAddress(0), m_nPort(0) {}

        inline UsbDevice(UsbDevice *pDev) : m_nAddress(pDev->m_nAddress), m_nPort(pDev->m_nPort), m_Speed(pDev->m_Speed),
            m_pInterface(pDev->m_pInterface), m_pDescriptor(pDev->m_pDescriptor), m_pConfiguration(pDev->m_pConfiguration) {}

        virtual inline ~UsbDevice() {};

        /** Access to internal information */
        virtual void getName(String &str)
        {
            str = "USB Device";
        }

        uint8_t getAddress()
        {
            return m_nAddress;
        }
        void setAddress(uint8_t nAddress)
        {
            m_nAddress = nAddress;
        }

        uint8_t getPort()
        {
            return m_nPort;
        }
        void setPort(uint8_t nPort)
        {
            m_nPort = nPort;
        }

        UsbSpeed getSpeed()
        {
            return m_Speed;
        }
        void setSpeed(UsbSpeed speed)
        {
            m_Speed = speed;
        }

        DeviceDescriptor *getDescriptor()
        {
            return m_pDescriptor;
        }
        ConfigDescriptor *getConfiguration()
        {
            return m_pConfiguration;
        }

        // Sync transfer methods
        ssize_t doSync(Endpoint *pEndpoint, UsbPid pid, uintptr_t pBuffer, size_t nBytes);
        ssize_t syncIn(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes);
        ssize_t syncOut(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes);

        // Method to perform an USB control request
        bool controlRequest(uint8_t nRequestType, uint8_t nRequest, uint16_t nValue, uint16_t nIndex, uint16_t nLength=0, uintptr_t pBuffer=0);

        // Various USB standard control request methods
        uint16_t getStatus();

        bool clearEndpointHalt(Endpoint *pEndpoint);

        bool assignAddress(uint8_t nAddress);

        bool useConfiguration(uint8_t nConfig);

        bool useInterface(uint8_t nInterface);

        void *getDescriptor(uint8_t nDescriptor, uint8_t nSubDescriptor, uint16_t nBytes, uint8_t requestType=0);

        uint8_t getDescriptorLength(uint8_t nDescriptor, uint8_t nSubDescriptor, uint8_t requestType=0);

        char *getString(uint8_t nString);

        void populateDescriptors();

    protected:

        uint8_t m_nAddress;
        uint8_t m_nPort;
        UsbSpeed m_Speed;
        Interface *m_pInterface;

    private:

        DeviceDescriptor *m_pDescriptor;
        ConfigDescriptor *m_pConfiguration;

        UsbDevice(const UsbDevice &d);
        const UsbDevice& operator = (const UsbDevice& d);
};

#endif
