/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
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

#ifndef USBMANAGER_H
#define USBMANAGER_H

#include <vfs/VFS.h>
#include <vfs/Filesystem.h>
#include <utilities/RequestQueue.h>
#include <utilities/Vector.h>
#include <process/Scheduler.h>
#include <usb/UsbConstants.h>
#include <usb/UsbDevice.h>
#include <usb/UsbController.h>

/** Provides an interface to Endpoints for applications */
class UsbManager
{
    public:
        UsbManager() : m_Controller(0) {m_Devices[0] = new UsbDevice();};
        virtual ~UsbManager() {};

        static UsbManager &instance()
        {
            return m_Instance;
        };

        // UsbManager interface.

        ssize_t setup(uint8_t addr, UsbSetup *setup);
        ssize_t in(uint8_t addr, uint8_t endpoint, void *buff, size_t size);
        ssize_t out(uint8_t addr, void *buff, size_t size);

        void enumerate();

        UsbController *getController()
        {
            return m_Controller;
        }
        void setController(UsbController *controller)
        {
            m_Controller = controller;
        }

    private:
        UsbController *m_Controller;
        UsbDevice *m_Devices[128];

        static UsbManager m_Instance;
        UsbManager(const UsbManager &e);
        const UsbManager& operator = (const UsbManager& e);
};

#endif
