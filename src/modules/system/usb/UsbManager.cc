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

#include <usb/UsbConstants.h>
#include <usb/UsbManager.h>
#include <usb/UsbDevice.h>
#include <usb/UsbKeyboard.h>
#include <usb/UsbController.h>

UsbManager UsbManager::m_Instance;

void UsbManager::enumerate()
{
    NOTICE("USB: Enumerating...");
    for(int i=1;i<127&&m_Devices[0]->ping();i++)
    {
        if(m_Devices[i])
            continue;
        m_Devices[0]->assign(i);
        m_Devices[i] = new UsbDevice(i);
        UsbInfo *info = m_Devices[i]->getUsbInfo();
        NOTICE("USB: Device: "<<info->str_prod<<", class="<<info->usb_class<<", subclass="<<info->usb_subclass<<", proto="<<info->usb_protocol);
        if(info->usb_class==9)
            for(int j=1;j<=8;j++)
                m_Devices[i]->control(0x23, 3, 4, j, 0);
        else if(info->usb_class==3&&info->usb_subclass==1&&info->usb_protocol==1)
            m_Devices[i] = new UsbKeyboard(m_Devices[i]);
    }
}

ssize_t UsbManager::setup(uint8_t addr, UsbSetup *setup)
{
    return m_Controller->setup(addr, setup);
}

ssize_t UsbManager::in(uint8_t addr, uint8_t endpoint, void *buff, size_t size)
{
    return m_Controller->in(addr, endpoint, buff, size);
}

ssize_t UsbManager::out(uint8_t addr, void *buff, size_t size)
{
    return m_Controller->out(addr, buff, size);
}
