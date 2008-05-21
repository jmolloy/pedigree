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
#ifndef MACHINE_OF_DEVICE_H
#define MACHINE_OF_DEVICE_H

#include <utilities/StaticString.h>
#include <machine/types.h>
#include <processor/types.h>
#include <machine/openfirmware/types.h>

/**
 * Provides an interface to query OpenFirmware about a device.
 */
class OFDevice
{
  /** OpenFirmware can access our m_Handle variable. */
  friend class OpenFirmware;
  public:
    OFDevice(OFHandle handle);
    virtual ~OFDevice();
  
    virtual void getProperty(const char *pProperty, NormalStaticString &buf);
    virtual OFHandle getProperty(const char *pProperty);
    virtual void setProperty(const char *pProperty, NormalStaticString &val);
    
    virtual void executeMethod(const char *method, size_t nArgs, OFParam p1=0,
                                                                 OFParam p2=0,
                                                                 OFParam p3=0,
                                                                 OFParam p4=0,
                                                                 OFParam p5=0,
                                                                 OFParam p6=0);
  protected:
    OFHandle m_Handle;
};

#endif
