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

#include <machine/openfirmware/OpenFirmware.h>

OpenFirmware OpenFirmware::m_Instance;

OpenFirmware::OpenFirmware() :
  m_Interface(0)
{
}

OpenFirmware::~OpenFirmware()
{
}

void OpenFirmware::initialise(OFInterface interface)
{
  m_Interface = interface;
}

OFHandle OpenFirmware::findDevice(const char *pName)
{
  return static_cast<OFHandle> (call("finddevice", 1, static_cast<OFParam> (pName)));
}

OFParam OpenFirmware::call(const char *pService, int nArgs, OFParam p1=0,
                                                            OFParam p2=0,
                                                            OFParam p3=0,
                                                            OFParam p4=0,
                                                            OFParam p5=0,
                                                            OFParam p6=0,
                                                            OFParam p7=0,
                                                            OFParam p8=0)
{
  PromArgs pa;
  pa.service = pService;
  
  pa.args[0] = p1;
  pa.args[1] = p2;
  pa.args[2] = p3;
  pa.args[3] = p4;
  pa.args[4] = p5;
  pa.args[5] = p6;
  pa.args[6] = p7;
  pa.args[7] = p8;
  pa.args[8] = 0;
  pa.args[9] = 0;
  pa.nargs = nArgs;
  pa.nret = 1;
  
  m_Interface(&pa);
  
  return pa.args[nargs];
}
