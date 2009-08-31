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
#ifndef i945G_H
#define i945G_H

#include <processor/types.h>
#include <machine/Device.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <machine/IrqHandler.h>
#include <process/Thread.h>
#include <process/Semaphore.h>

#define i945G_VENDOR_ID 0x8086
#define i945G_DEVICE_ID 0x2772

/** Device driver for the 945G class device */
class i945G : public Device, public IrqHandler
{
    public:
        i945G(Device* pDev);
        ~i945G();

        virtual void getName(String &str)
        {
            str = "945G";
        }

        // IRQ handler callback.
        virtual bool irq(irq_id_t number, InterruptState &state);

        void Blank(bool state);
        void ReadState();
        void DumpState();
        void SetMode(uint32_t width, uint32_t height, uint32_t depth);

        IoBase *m_pBase;

        struct RegState {
            uint32_t vga0_divisor;
            uint32_t vga1_divisor;
            uint32_t vga_pd;
            uint32_t dpll_a;
            uint32_t dpll_b;
            uint32_t fpa0;
            uint32_t fpa1;
            uint32_t fpb0;
            uint32_t fpb1;
            uint32_t palette_a[256];
            uint32_t palette_b[256];
            uint32_t htotal_a;
            uint32_t hblank_a;
            uint32_t hsync_a;
            uint32_t vtotal_a;
            uint32_t vblank_a;
            uint32_t vsync_a;
            uint32_t src_size_a;
            uint32_t bclrpat_a;
            uint32_t htotal_b;
            uint32_t hblank_b;
            uint32_t hsync_b;
            uint32_t vtotal_b;
            uint32_t vblank_b;
            uint32_t vsync_b;
            uint32_t src_size_b;
            uint32_t bclrpat_b;
            uint32_t adpa;
            uint32_t dvoa;
            uint32_t dvob;
            uint32_t dvoc;
            uint32_t dvoa_srcdim;
            uint32_t dvob_srcdim;
            uint32_t dvoc_srcdim;
            uint32_t lvds;
            uint32_t pipe_a_conf;
            uint32_t pipe_b_conf;
            uint32_t disp_arb;
            uint32_t cursor_a_control;
            uint32_t cursor_b_control;
            uint32_t cursor_a_base;
            uint32_t cursor_b_base;
            uint32_t cursor_size;
            uint32_t disp_a_ctrl;
            uint32_t disp_b_ctrl;
            uint32_t disp_a_base;
            uint32_t disp_b_base;
            uint32_t cursor_a_palette[4];
            uint32_t cursor_b_palette[4];
            uint32_t disp_a_stride;
            uint32_t disp_b_stride;
            uint32_t vgacntrl;
            uint32_t add_id;
            uint32_t swf0x[7];
            uint32_t swf1x[7];
            uint32_t swf3x[3];
            uint32_t fence[8];
            uint32_t instpm;
            uint32_t mem_mode;
            uint32_t fw_blc_0;
            uint32_t fw_blc_1;
            uint16_t hwstam;
            uint16_t ier;
            uint16_t iir;
            uint16_t imr;
        };

    private:
        RegState *m_State;

        i945G(const i945G&);
        void operator =(const i945G&);
};

#endif
