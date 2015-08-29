/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_MACHINE_HOSTED_PC_H
#define KERNEL_MACHINE_HOSTED_PC_H

#include <machine/Machine.h>
#include "Serial.h"
#include "Vga.h"

/**
 * Concretion of the abstract Machine class for hosted systems
 */
class HostedMachine : public Machine
{
  public:
    inline static HostedMachine &instance(){return m_Instance;}

    virtual void initialise() INITIALISATION_ONLY;

    virtual void initialiseDeviceTree();

    virtual Serial *getSerial(size_t n);
    virtual size_t getNumSerial();
    virtual Vga *getVga(size_t n);
    virtual size_t getNumVga();
    virtual IrqManager *getIrqManager();
    virtual SchedulerTimer *getSchedulerTimer();
    virtual Timer *getTimer();
    virtual Keyboard *getKeyboard();
    virtual void setKeyboard(Keyboard *kb);

  private:
    /**
    * Default constructor, does nothing.
    */
    HostedMachine() INITIALISATION_ONLY;
    HostedMachine(const HostedMachine &);
    HostedMachine &operator = (const HostedMachine &);
    /**
    * Virtual destructor, does nothing.
    */
    virtual ~HostedMachine();

    HostedSerial m_Serial[1];
    HostedVga m_Vga;

    static HostedMachine m_Instance;
};

#endif
