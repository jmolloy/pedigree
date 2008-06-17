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
#ifndef MACHINE_CONTROLLER_H
#define MACHINE_CONTROLLER_H

#include <machine/Device.h>

/**
 * A controller is a hub that controls multiple devices.
 */
class Controller : public Device
{
public:
  Controller()
  {
  }
  Controller(Controller *pDev) :
    Device(pDev)
  {
  }
  virtual ~Controller()
  {
  }

  virtual Type getType()
  {
    return Device::Controller;
  }

  virtual void getName(String &str)
  {
    str = "Generic controller";
  }

  virtual void dump(String &str)
  {
    str = "Generic controller";
  }
};

#endif
