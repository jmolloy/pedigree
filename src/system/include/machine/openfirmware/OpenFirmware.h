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
#ifndef MACHINE_OF_OF_H
#define MACHINE_OF_OF_H

#include <machine/openfirmware/types.h>

/**
 * Interface to OpenFirmware. 
 */
class OpenFirmware
{
private:
  /** Argument structure for PROM call. */
  struct PromArgs
  {
    const char *service;
    int nargs;
    int nret;
    void *args[10];
  };
public:
  typedef int (*OFInterface)(PromArgs *);
  
  /** Retrieves the singleton instance of the OpenFirmware class. */
  static OpenFirmware &instance() {return m_Instance;}
  
  /** Initialises the OF interface. */
  void initialise(OFInterface interface);
  
  /** Finds a device in the tree by name. */
  OFHandle findDevice(const char *pName);
  
  /** Calls OpenFirmware with the given arguments, returning one argument. */
  OFParam call(const char *pService, int nArgs, OFParam p1=0,
                                                OFParam p2=0,
                                                OFParam p3=0,
                                                OFParam p4=0,
                                                OFParam p5=0,
                                                OFParam p6=0,
                                                OFParam p7=0,
                                                OFParam p8=0);
private:
  /** Default constructor is private. */
  OpenFirmware();
  ~OpenFirmware();
  
  /** The PROM callback. */
  OFInterface m_Interface;
  
  /** The singleton instance */
  static OpenFirmware m_Instance;
};

#endif
