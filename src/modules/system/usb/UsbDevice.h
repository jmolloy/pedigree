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
#include <utilities/assert.h>
#include <usb/Usb.h>
#include <usb/UsbDescriptors.h>

class UsbDevice : public Device
{
    public:

        struct Setup
        {
            inline Setup(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length) :
                nRequestType(requestType), nRequest(request), nValue(value), nIndex(index), nLength(length) {}

            uint8_t nRequestType;
            uint8_t nRequest;
            uint16_t nValue;
            uint16_t nIndex;
            uint16_t nLength;
        } PACKED;

        struct UnknownDescriptor
        {
            inline UnknownDescriptor(uint8_t *pBuffer, uint8_t type, size_t length) : nType(type), nLength(length)
            {
                pDescriptor = new uint8_t[nLength];
                memcpy(pDescriptor, pBuffer, nLength);
            }

            void *pDescriptor;
            uint8_t nType;
            size_t nLength;
        };

        struct Endpoint
        {
            Endpoint(UsbEndpointDescriptor *pDescriptor, UsbSpeed speed);

            enum TransferTypes
            {
                Control = 0,
                Isochronus = 1,
                Bulk = 2,
                Interrupt = 3
            };

            uint8_t nEndpoint;
            bool bIn;
            bool bOut;
            uint8_t nTransferType;
            uint16_t nMaxPacketSize;

            bool bDataToggle;
        };

        struct Interface
        {
            Interface(UsbInterfaceDescriptor *pDescriptor);
            ~Interface();

            uint8_t nInterface;
            uint8_t nAlternateSetting;
            uint8_t nClass;
            uint8_t nSubclass;
            uint8_t nProtocol;
            uint8_t nString;

            Vector<Endpoint*> endpointList;
            Vector<UnknownDescriptor*> otherDescriptorList;
            String sString;
        };

        struct ConfigDescriptor
        {
            ConfigDescriptor(void *pConfigBuffer, size_t nConfigLength, UsbSpeed speed);
            ~ConfigDescriptor();

            uint8_t nConfig;
            uint8_t nString;

            Vector<Interface*> interfaceList;
            Vector<UnknownDescriptor*> otherDescriptorList;
            String sString;
        };

        struct DeviceDescriptor
        {
            DeviceDescriptor(UsbDeviceDescriptor *pDescriptor);
            ~DeviceDescriptor();

            uint16_t nBcdUsbRelease;
            uint8_t nClass;
            uint8_t nSubclass;
            uint8_t nProtocol;
            uint8_t nMaxControlPacketSize;
            uint16_t nVendorId;
            uint16_t nProductId;
            uint16_t nBcdDeviceRelease;
            uint8_t nVendorString;
            uint8_t nProductString;
            uint8_t nSerialString;
            uint8_t nConfigurations;

            Vector<ConfigDescriptor*> configList;
            String sVendor;
            String sProduct;
            String sSerial;
        };
        
        struct DeviceQualifier
        {
            uint16_t nVersion;
            uint8_t nClass;
            uint8_t nSubclass;
            uint8_t nProtocol;
            uint8_t nMaxControlPacketSize;
            uint8_t nConfigurations;
            uint8_t nRsvd;
        } PACKED;

        /// Possible states for an USB device
        enum UsbState
        {
            Connected = 0,
            Addressed,
            HasDescriptors,
            Configured,
            HasInterface,
            HasDriver
        };

        /// Default constructor
        UsbDevice(uint8_t nPort, UsbSpeed speed);

        /// Copy constructor
        UsbDevice(UsbDevice *pDev);

        /// Destructor
        virtual ~UsbDevice();

        /// Initialises the device at the given address
        void initialise(uint8_t nAddress);

        /// Implemented by the driver class, initialises driver-specific stuff
        virtual void initialiseDriver() {}

        /// Access to internal information
        virtual void getName(String &str)
        {
            str = "USB Device";
        }

        virtual Type getType()
        {
            return UsbGeneric;
        }

        /// Returns the current address of the device
        inline uint8_t getAddress()
        {
            return m_nAddress;
        }

        /// Returns the number of the port on which the device is connected
        inline uint8_t getPort()
        {
            return m_nPort;
        }

        /// Returns the speed at which the device operates
        inline UsbSpeed getSpeed()
        {
            return m_Speed;
        }

        /// Returns the current state of the device
        inline UsbState getUsbState()
        {
            return m_UsbState;
        }

        /// Returns the device descriptor of the device
        inline DeviceDescriptor *getDescriptor()
        {
            return m_pDescriptor;
        }

        /// Returns the configuration in use
        inline ConfigDescriptor *getConfiguration()
        {
            return m_pConfiguration;
        }

        /// Returns the interface in use
        inline Interface *getInterface()
        {
            return m_pInterface;
        }

        /// Switches to the given configuration
        void useConfiguration(uint8_t nConfig);

        /// Switches to the given interface
        void useInterface(uint8_t nInterface);

    protected:

        // Sync transfer methods
        ssize_t doSync(Endpoint *pEndpoint, UsbPid pid, uintptr_t pBuffer, size_t nBytes, size_t timeout);
        ssize_t syncIn(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes, size_t timeout = 5000);
        ssize_t syncOut(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes, size_t timeout = 5000);

        void addInterruptInHandler(Endpoint *pEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        /// Performs an USB control request
        bool controlRequest(uint8_t nRequestType, uint8_t nRequest, uint16_t nValue, uint16_t nIndex, uint16_t nLength=0, uintptr_t pBuffer=0);

        /// Gets device's current status
        uint16_t getStatus();

        /// Clears a halt on the given endpoint
        bool clearEndpointHalt(Endpoint *pEndpoint);

        /// Gets a descriptor from the device
        void *getDescriptor(uint8_t nDescriptorType, uint8_t nDescriptorIndex, uint16_t nBytes, uint8_t requestType=0);

        /// Gets a descriptor's length from the device
        uint8_t getDescriptorLength(uint8_t nDescriptorType, uint8_t nDescriptorIndex, uint8_t requestType=0);

        /// Gets a string
        String getString(uint8_t nString);

        /// The current address of the device
        uint8_t m_nAddress;

        /// The number of the port on which the device is connected
        uint8_t m_nPort;

        /// The speed at which the device operates
        UsbSpeed m_Speed;

        /// The current state of the device
        UsbState m_UsbState;

        /// Device descriptor for this device
        DeviceDescriptor *m_pDescriptor;

        /// Configuration in use
        ConfigDescriptor *m_pConfiguration;

        /// Interface in use
        Interface *m_pInterface;

    private:

        UsbDevice(const UsbDevice &d);
        const UsbDevice& operator = (const UsbDevice& d);
};

#endif
