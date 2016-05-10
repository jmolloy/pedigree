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

#ifndef KERNEL_MACHINE_X86_COMMON_PC_H
#define KERNEL_MACHINE_X86_COMMON_PC_H

#include <machine/Machine.h>
#include "Pic.h"
#include "Rtc.h"
#include "Pit.h"
#include "Serial.h"
#include "Vga.h"
#include "Keyboard.h"
#if defined(SMBIOS)
  #include "SMBios.h"
#endif
#include "LocalApic.h"

/**
 * Concretion of the abstract Machine class for x86 and x64 machines
 */
class Pc : public Machine
{
  public:
    inline static Pc &instance(){return m_Instance;}

    virtual void initialise() INITIALISATION_ONLY;
    virtual void deinitialise();

    #if defined(MULTIPROCESSOR)
      void initialiseProcessor() INITIALISATION_ONLY;
    #endif

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

    #if defined(APIC)
      /** Get the Local APIC class instance
       *\return reference to the Local APIC class instance */
      inline LocalApic &getLocalApic()
        {return m_LocalApic;}
    #endif

    #ifdef MULTIPROCESSOR
      virtual void stopAllOtherProcessors();
    #endif


  private:
    /**
    * Default constructor, does nothing.
    */
    Pc() INITIALISATION_ONLY;
    Pc(const Pc &);
    Pc &operator = (const Pc &);
    /**
    * Virtual destructor, does nothing.
    */
    virtual ~Pc();

    X86Serial m_pSerial[2];
    X86Vga m_Vga;
    Keyboard *m_Keyboard;

    #if defined(SMBIOS)
      SMBios m_SMBios;
    #endif

    #if defined(APIC)
      LocalApic m_LocalApic;
    #endif

    static Pc m_Instance;
};

#endif
