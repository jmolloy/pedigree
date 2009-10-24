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
#include <usb/UsbDevice.h>
#include <usb/UsbManager.h>

ssize_t UsbDevice::setup(UsbSetup *setup)
{
    return UsbManager::instance().setup(m_Address, setup);
}

ssize_t UsbDevice::in(void *buff, size_t size)
{
    return UsbManager::instance().in(m_Address, 0, buff, size);
}

ssize_t UsbDevice::in(uint8_t endpoint, void *buff, size_t size)
{
    return UsbManager::instance().in(m_Address, endpoint, buff, size);
}

ssize_t UsbDevice::out(void *buff, size_t size)
{
    return UsbManager::instance().out(m_Address, buff, size);
}

ssize_t UsbDevice::control(uint8_t req_type, uint8_t req, uint16_t val, uint16_t index, uint16_t len)
{
    UsbSetup *stp = new UsbSetup;
    stp->req_type=req_type;
    stp->req=req;
    stp->val=val;
    stp->index=index;
    stp->len=len;
    return setup(stp);
}

int16_t UsbDevice::status()
{
    ssize_t r = control(0x80, 0, 0, 0, 2);
    if(r<0)
        return r;
    uint16_t s;
    in(&s, 2);
    return s;
}

bool UsbDevice::assign(uint8_t addr)
{
    return control(0, 5, addr, 0, 0)>0;
}

bool UsbDevice::ping()
{
    return status()>=0;
}

void *UsbDevice::getDescriptor(uint8_t des, uint8_t subdes, uint16_t size)
{
    uint8_t *buff = new uint8_t[size];
    if(control(0x80, 6, des<<8|subdes, 0, size)>0)
    {
        in(buff, size);
        return buff;
    }
    delete [] buff;
    return 0;
}

UsbInfo *UsbDevice::getUsbInfo()
{
    UsbInfo *info = new UsbInfo;
    info->dev = static_cast<UsbDeviceDescriptor *>(getDescriptor(1, 0, sizeof(UsbDeviceDescriptor)));
    uint8_t *config = static_cast<uint8_t *>(getDescriptor(2, 0, 0xff));
    info->conf = reinterpret_cast<UsbConfig *>(config);
    uint8_t ln = info->conf->total_len-info->conf->len, off=info->conf->len;
    while(ln)
    {
        if(config[off+1]==4&&config[off+2]==0)
        {
            info->iface = reinterpret_cast<UsbInterface *>(&config[off]);
            break;
        }
        ln-=config[off];
        off+=config[off];
    }
    info->str_prod = getString(info->dev->str_prod);
    info->str_ven = getString(info->dev->str_ven);
    info->str_conf = getString(info->conf->str);
    info->str_iface = getString(info->iface->str);
    if(info->dev->dev_class)
    {
        info->usb_class = info->dev->dev_class;
        info->usb_subclass = info->dev->dev_subclass;
        info->usb_protocol = info->dev->dev_proto;
    }
    else
    {
        info->usb_class = info->iface->iface_class;
        info->usb_subclass = info->iface->iface_subclass;
        info->usb_protocol = info->iface->iface_proto;
    }
    return info;
}

char *UsbDevice::getString(uint8_t str)
{
    char *buff2 = static_cast<char *>(getDescriptor(3, str, 0xff));
    if(buff2)
    {
        int sl=(buff2[0]-2)/2;
        char *buff = new char[sl+1];
        for(int i=0;i<sl;i++)
            buff[i]=buff2[2+i*2];
        buff[sl]=0;
        delete buff2;
        return buff;
    }
    return 0;
}
