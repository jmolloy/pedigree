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
#ifndef USBHUB_H
#define USBHUB_H

#include <machine/Device.h>
#include <process/Mutex.h>
#include <process/Semaphore.h>
#include <processor/types.h>
#include <usb/Usb.h>
#include <utilities/ExtensibleBitmap.h>

class UsbHub : public virtual Device
{
    public:

        inline UsbHub() : m_SyncSemaphore(0) {}
        inline virtual ~UsbHub() {}

        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes){}
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0){return 0;}
        virtual void doAsync(uintptr_t pTransaction){}
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0){}

        void deviceConnected(uint8_t nPort, UsbSpeed speed);
        void deviceDisconnected(uint8_t nPort);

        void getUsedAddresses(ExtensibleBitmap *pBitmap);

        //ssize_t doSync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, size_t nBytes, uint32_t timeout=5000);
        //ssize_t doSync(uintptr_t queueHead);

        virtual UsbSpeed getHubSpeed()
        {
            return LowSpeed;
        }

        /// Creates a qTD with link fields as necessary that handles a single transaction
        /// \param pNext Null = end of queue, else pointer to next qTD
        //virtual uintptr_t createTD(uintptr_t pNext, bool bToggle, bool bDirection, bool bIsSetup, void *pData, size_t nBytes)
        //{
        //    return 0;
        //}

        /// Creates a QH with link fields as necessary that handles an endpoint
        //virtual uintptr_t createQH(uintptr_t pNext, uintptr_t pFirstQTD, size_t qTDCount, bool head, UsbEndpoint &endpointInfo, QHMetaData *pMetaData)
        //{
        //   return 0;
        //}

    private:

        Mutex m_SyncMutex;
        Semaphore m_SyncSemaphore;
        ssize_t m_SyncRet;

        static void syncCallback(uintptr_t pParam, ssize_t ret);
};

#endif
