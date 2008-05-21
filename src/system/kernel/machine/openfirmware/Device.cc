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

#include <machine/openfirmware/Device.h>
#include <machine/openfirmware/OpenFirmware.h>

OFDevice::OFDevice(OFHandle handle) :
  m_Handle(handle)
{
}

OFDevice::~OFDevice()
{
}

void OFDevice::getProperty(const char *pProperty, NormalStaticString &buf)
{
  OpenFirmware::instance().call("getprop", 4, static_cast<OFParam> (m_Handle),
                                              static_cast<OFParam> (reinterpret_cast<char*> (buf)),
                                              static_cast<OFParam> (64);
}

void OFDevice::setProperty(const char *pProperty, NormalStaticString &val)
{
  /// \todo
}

void OFDevice::executeMethod(const char *method, size_t nArgs, OFParam p1=0,
                                                               OFParam p2=0,
                                                               OFParam p3=0,
                                                               OFParam p4=0,
                                                               OFParam p5=0,
                                                               OFParam p6=0)
{
  OpenFirmware::instance().call("call-method", nArgs+2, static_cast<OFParam> (method),
                                                        static_cast<OFParam> (m_Handle),
                                                        p1,p2,p3,p4,p5,p6);
}
