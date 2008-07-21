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

void probePciDevices()
{
  // Here we probe OpenFirmware and add its devices into the device tree.
  OFDevice root( OpenFirmware::instance().findDevice("/"));
  probeDev(&root, &Device::root());
}

static void probeDev(OFDevice *pDev, Device *device)
{
  OFHandle hChild = OpenFirmware::instance().getFirstChild(pDev);
  
  while (hChild != 0)
  {
    OFDevice dChild (hChild);
    hChild = OpenFirmware::instance().getSibling(&dChild);

    NormalStaticString name, type, compatible;
    dChild.getProperty("name", name);
    dChild.getProperty("device_type", type);
    dChild.getProperty("compatible", compatible);

    if (type.length() == 0)
      continue;

    // Recognise common device types.
    Device *deviceChild = 0;
    if (type == "pci" || type == "mac-io" || type == "pci-ide" || type == "usb")
      deviceChild = new Bridge();
    else if (type == "display" || compatible == "display")
      deviceChild = new Display();
    else
      deviceChild = new Controller();

    deviceChild->setSpecificType(String(static_cast<const char*> (type)));
    deviceChild->setParent(device);
    device->addChild(deviceChild);
    /// \todo Interrupt number.

    probeDev(&dChild, deviceChild);
  }
}