/*
 * Copyright (c) 2010 Matthew Iselin
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

#ifndef _PS2_MOUSE_H
#define _PS2_MOUSE_H

#include <machine/Device.h>
#include <machine/IrqHandler.h>
#include <Spinlock.h>
#include <process/Semaphore.h>

class Ps2Mouse : public Device, public IrqHandler
{
    public:
        Ps2Mouse(Device *pDev);
        virtual ~Ps2Mouse();

        virtual bool initialise(IoBase *pBase);

        virtual void getName(String &str)
        {
            str = "mouse";
        }

        virtual bool irq(irq_id_t number, InterruptState &state);

    private:
        IoBase *m_pBase;

        enum WaitType
        {
            Data,
            Signal
        };

        enum Ps2Ports
        {
            KbdStat         = 0x64,
            KbdCommand      = 0x60
        };

        enum Ps2Commands
        {
            EnablePS2       = 0xA8,
            DisableKbd      = 0xAD,
            EnableKbd       = 0xAE,
            Mouse           = 0xD4,
            MouseStream     = 0xF4,
            MouseDisable    = 0xF5,
            SetDefaults     = 0xF6,
            MouseAck        = 0xFA
        };

        void mouseWait(WaitType type);

        void mouseWrite(uint8_t data);
        uint8_t mouseRead();

        /// Enables the auxillary PS2 device (generally the mouse)
        void enableAuxDevice();

        void enableIrq();

        bool setDefaults();
        bool enableMouse();

        /// Mouse data buffer
        uint8_t m_Buffer[3];

        /// Index into the data buffer
        size_t m_BufferIndex;

        /// Lock for the mouse data buffer
        Spinlock m_BufferLock;

        /// IRQ wait semaphore
        Semaphore m_IrqWait;

        Ps2Mouse(const Ps2Mouse&);
        void operator = (const Ps2Mouse&);
};

#endif