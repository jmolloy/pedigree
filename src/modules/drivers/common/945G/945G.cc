/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITRTLSS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, RTLGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONRTLCTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include "945G.h"
#include "945GConstants.h"
#include <Log.h>
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <machine/IrqManager.h>

#define ROUND_UP_TO(x, y)   (((x) + (y) - 1) / (y) * (y))
#define ROUND_DOWN_TO(x, y) ((x) / (y) * (y))

struct pll_min_max {
    uint32_t min_m, max_m, min_m1, max_m1;
    uint32_t min_m2, max_m2, min_n, max_n;
    uint32_t min_p, max_p, min_p1, max_p1;
    uint32_t min_vco, max_vco, p_transition_clk, ref_clk;
    uint32_t p_inc_lo, p_inc_hi;
};

static void get_p1p2(uint32_t dpll, uint32_t *o_p1, uint32_t *o_p2)
{
    uint32_t p1, p2;
    p1 = (dpll&DPLL_P1_FORCE_DIV2?1:(dpll >> DPLL_P1_SHIFT)&0xff);
    p2 = (dpll >> DPLL_I9XX_P2_SHIFT) & DPLL_P2_MASK;
    *o_p1 = p1;
    *o_p2 = p2;
}

static uint32_t calc_vclock3(uint32_t m, uint32_t n, uint32_t p)
{
    if(p == 0 || n == 0)
        return 0;
    return 96000 * m / n / p;
}

static uint32_t calc_vclock(uint32_t m1, uint32_t m2, uint32_t n, uint32_t p1, uint32_t p2)
{
    uint32_t m, vco, p;
    m = (5 * (m1 + 2)) + (m2 + 2);
    n += 2;
    vco = 96000 * m / n;
    p = ((p1) * (p2 ? 5 : 10));
    return vco / p;
}


/* Split the M parameter into M1 and M2. */
static uint32_t splitm(uint32_t m, uint32_t *retm1, uint32_t *retm2)
{
    uint32_t m1, m2;
    uint32_t testm;
    struct pll_min_max pll = {75, 120, 10, 20, 5, 9, 4, 7, 5, 80, 1, 8, 1400000, 2800000, 200000, 96000, 10, 5};
    for(m1 = pll.min_m1; m1 < pll.max_m1 + 1; m1++) {
        for(m2 = pll.min_m2; m2 < pll.max_m2 + 1; m2++) {
            testm = (5 * (m1 + 2)) + (m2 + 2);
            if(testm == m) {
                *retm1 = static_cast<uint32_t>(m1);
                *retm2 = static_cast<uint32_t>(m2);
                return 0;
            }
        }
    }
    return 1;
}

/* Split the P parameter into P1 and P2. */
static uint32_t splitp(uint32_t p, uint32_t *retp1, uint32_t *retp2)
{
    uint32_t p1, p2;
    p2 = (p % 10) ? 1 : 0;

    p1 = p / (p2 ? 5 : 10);

    *retp1 = static_cast<uint32_t>(p1);
    *retp2 = static_cast<uint32_t>(p2);
    return 0;
}

static uint32_t calc_pll_params(uint32_t clock, uint32_t *retm1, uint32_t *retm2, uint32_t *retn, uint32_t *retp1, uint32_t *retp2, uint32_t *retclock)
{
    uint32_t m1=0, m2=0, n, p1, p2, n1, testm;
    uint32_t f_vco, p, p_best = 0, m, f_out = 0;
    uint32_t err_max, err_target, err_best = 10000000;
    uint32_t n_best = 0, m_best = 0, f_best, f_err;
    uint32_t p_min, p_max, p_inc, div_max;
    struct pll_min_max pll = {75, 120, 10, 20, 5, 9, 4, 7, 5, 80, 1, 8, 1400000, 2800000, 200000, 96000, 10, 5};

    /* Accept 0.5% difference, but aim for 0.1% */
    err_max = 5 * clock / 1000;
    err_target = clock / 1000;

    div_max = pll.max_vco / clock;

    p_inc = (clock <= pll.p_transition_clk) ? pll.p_inc_lo : pll.p_inc_hi;
    p_min = p_inc;
    p_max = ROUND_DOWN_TO(div_max, p_inc);
    if(p_min < pll.min_p)
        p_min = pll.min_p;
    if(p_max > pll.max_p)
        p_max = pll.max_p;

    p = p_min;
    do {
        if(splitp(p, &p1, &p2)) {
            p += p_inc;
            continue;
        }
        n = pll.min_n;
        f_vco = clock * p;

        do {
            m = ROUND_UP_TO(f_vco * n, pll.ref_clk) / pll.ref_clk;
            if(m < pll.min_m)
                m = pll.min_m + 1;
            if(m > pll.max_m)
                m = pll.max_m - 1;
            for(testm = m - 1; testm <= m; testm++) {
                f_out = calc_vclock3(testm, n, p);
                if(splitm(testm, &m1, &m2))
                    continue;
                if(clock > f_out)
                    f_err = clock - f_out;
                else/* slightly bias the error for bigger clocks */
                    f_err = f_out - clock + 1;

                if(f_err < err_best) {
                    m_best = testm;
                    n_best = n;
                    p_best = p;
                    f_best = f_out;
                    err_best = f_err;
                }
            }
            n++;
        } while((n <= pll.max_n) && (f_out >= clock));
        p += p_inc;
    } while((p <= p_max));

    if(!m_best)
        return 1;
    m = m_best;
    n = n_best;
    p = p_best;
    splitm(m, &m1, &m2);
    splitp(p, &p1, &p2);
    n1 = n - 2;
    *retm1 = m1;
    *retm2 = m2;
    *retn = n1;
    *retp1 = p1;
    *retp2 = p2;
    *retclock = calc_vclock(m1, m2, n1, p1, p2);

    return 0;
}

i945G::i945G(Device* pDev) : Device(pDev), m_pBase(0), m_State()
{
    setSpecificType(String("945G-card"));

    // grab the ports
    m_pBase = m_Addresses[0]->m_Io;

    //Blank(false);
    //ReadState();
    SetMode(1024, 768, 16);
    //Blank(true);
    NOTICE("945G: refreshed");
    for(uint32_t i=0;i<1024*768;i++)
    {
        m_Addresses[2]->m_Io->write16(i&0xffff, i*2);
        asm volatile("wbinvd");
    }
    NOTICE("945G: tested");

    //ReadState();
    //DumpState();

    //Processor::breakpoint();

    // install the IRQ and register the NIC in the stack
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler *>(this));
}

i945G::~i945G()
{
}

void i945G::ReadState()
{
    uint32_t i;
    /* Read in as much of the HW state as possible. */
    m_State->vga0_divisor = m_pBase->read32(VGA0_DIVISOR);
    m_State->vga1_divisor = m_pBase->read32(VGA1_DIVISOR);
    m_State->vga_pd = m_pBase->read32(VGAPD);
    m_State->dpll_a = m_pBase->read32(DPLL_A);
    m_State->dpll_b = m_pBase->read32(DPLL_B);
    m_State->fpa0 = m_pBase->read32(FPA0);
    m_State->fpa1 = m_pBase->read32(FPA1);
    m_State->fpb0 = m_pBase->read32(FPB0);
    m_State->fpb1 = m_pBase->read32(FPB1);
    m_State->htotal_a = m_pBase->read32(HTOTAL_A);
    m_State->hblank_a = m_pBase->read32(HBLANK_A);
    m_State->hsync_a = m_pBase->read32(HSYNC_A);
    m_State->vtotal_a = m_pBase->read32(VTOTAL_A);
    m_State->vblank_a = m_pBase->read32(VBLANK_A);
    m_State->vsync_a = m_pBase->read32(VSYNC_A);
    m_State->src_size_a = m_pBase->read32(SRC_SIZE_A);
    m_State->bclrpat_a = m_pBase->read32(BCLRPAT_A);
    m_State->htotal_b = m_pBase->read32(HTOTAL_B);
    m_State->hblank_b = m_pBase->read32(HBLANK_B);
    m_State->hsync_b = m_pBase->read32(HSYNC_B);
    m_State->vtotal_b = m_pBase->read32(VTOTAL_B);
    m_State->vblank_b = m_pBase->read32(VBLANK_B);
    m_State->vsync_b = m_pBase->read32(VSYNC_B);
    m_State->src_size_b = m_pBase->read32(SRC_SIZE_B);
    m_State->bclrpat_b = m_pBase->read32(BCLRPAT_B);
    m_State->adpa = m_pBase->read32(ADPA);
    m_State->dvoa = m_pBase->read32(DVOA);
    m_State->dvob = m_pBase->read32(DVOB);
    m_State->dvoc = m_pBase->read32(DVOC);
    m_State->dvoa_srcdim = m_pBase->read32(DVOA_SRCDIM);
    m_State->dvob_srcdim = m_pBase->read32(DVOB_SRCDIM);
    m_State->dvoc_srcdim = m_pBase->read32(DVOC_SRCDIM);
    m_State->lvds = m_pBase->read32(LVDS);
    m_State->pipe_a_conf = m_pBase->read32(PIPEACONF);
    m_State->pipe_b_conf = m_pBase->read32(PIPEBCONF);
    m_State->disp_arb = m_pBase->read32(DISPARB);
    m_State->cursor_a_control = m_pBase->read32(CURSOR_A_CONTROL);
    m_State->cursor_b_control = m_pBase->read32(CURSOR_B_CONTROL);
    m_State->cursor_a_base = m_pBase->read32(CURSOR_A_BASEADDR);
    m_State->cursor_b_base = m_pBase->read32(CURSOR_B_BASEADDR);
    for(i = 0; i < 4; i++)
    {
        m_State->cursor_a_palette[i] = m_pBase->read32(CURSOR_A_PALETTE0 + (i << 2));
        m_State->cursor_b_palette[i] = m_pBase->read32(CURSOR_B_PALETTE0 + (i << 2));
    }
    m_State->cursor_size = m_pBase->read32(CURSOR_SIZE);
    m_State->disp_a_ctrl = m_pBase->read32(DSPACNTR);
    m_State->disp_b_ctrl = m_pBase->read32(DSPBCNTR);
    m_State->disp_a_base = m_pBase->read32(DSPABASE);
    m_State->disp_b_base = m_pBase->read32(DSPBBASE);
    m_State->disp_a_stride = m_pBase->read32(DSPASTRIDE);
    m_State->disp_b_stride = m_pBase->read32(DSPBSTRIDE);
    m_State->vgacntrl = m_pBase->read32(VGACNTRL);
    m_State->add_id = m_pBase->read32(ADD_ID);
    for(i = 0; i < 7; i++)
    {
        m_State->swf0x[i] = m_pBase->read32(SWF00 + (i << 2));
        m_State->swf1x[i] = m_pBase->read32(SWF10 + (i << 2));
        if(i < 3)
            m_State->swf3x[i] = m_pBase->read32(SWF30 + (i << 2));
    }
    for(i = 0; i < 8; i++)
        m_State->fence[i] = m_pBase->read32(FENCE + (i << 2));
    m_State->instpm = m_pBase->read32(INSTPM);
    m_State->mem_mode = m_pBase->read32(MEM_MODE);
    m_State->fw_blc_0 = m_pBase->read32(FW_BLC_0);
    m_State->fw_blc_1 = m_pBase->read32(FW_BLC_1);
    m_State->hwstam = m_pBase->read16(HWSTAM);
    m_State->ier = m_pBase->read16(IER);
    m_State->iir = m_pBase->read16(IIR);
    m_State->imr = m_pBase->read16(IMR);
}

void i945G::DumpState()
{
    uint32_t i, m1, m2, n, p1, p2;
    /* Read in as much of the HW state as possible. */
    NOTICE("hw state dump start");
    NOTICE("    VGA0_DIVISOR:       " << m_State->vga0_divisor);
    NOTICE("    VGA1_DIVISOR:       " << m_State->vga1_divisor);
    NOTICE("    VGAPD:          " << m_State->vga_pd);
    n = (m_State->vga0_divisor >> FP_N_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m1 = (m_State->vga0_divisor >> FP_M1_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m2 = (m_State->vga0_divisor >> FP_M2_DIVISOR_SHIFT) & FP_DIVISOR_MASK;

    get_p1p2(m_State->vga_pd, &p1, &p2);

    NOTICE("    VGA0: (m1, m2, n, p1, p2) = (" << m1 << ", " << m2 << ", " << n << ", " << p1 << ", " << p2 << ")");
    NOTICE("    VGA0: clock is " << calc_vclock(m1, m2, n, p1, p2));

    n = (m_State->vga1_divisor >> FP_N_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m1 = (m_State->vga1_divisor >> FP_M1_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m2 = (m_State->vga1_divisor >> FP_M2_DIVISOR_SHIFT) & FP_DIVISOR_MASK;

    get_p1p2(m_State->vga_pd, &p1, &p2);

    NOTICE("    VGA1: (m1, m2, n, p1, p2) = (" << m1 << ", " << m2 << ", " << n << ", " << p1 << ", " << p2 << ")");
    NOTICE("    VGA1: clock is " << calc_vclock(m1, m2, n, p1, p2));

    NOTICE("    DPLL_A:         " << m_State->dpll_a);
    NOTICE("    DPLL_B:         " << m_State->dpll_b);
    NOTICE("    FPA0:           " << m_State->fpa0);
    NOTICE("    FPA1:           " << m_State->fpa1);
    NOTICE("    FPB0:           " << m_State->fpb0);
    NOTICE("    FPB1:           " << m_State->fpb1);

    n = (m_State->fpa0 >> FP_N_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m1 = (m_State->fpa0 >> FP_M1_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m2 = (m_State->fpa0 >> FP_M2_DIVISOR_SHIFT) & FP_DIVISOR_MASK;

    get_p1p2(m_State->dpll_a, &p1, &p2);

    NOTICE("    PLLA0: (m1, m2, n, p1, p2) = (" << m1 << ", " << m2 << ", " << n << ", " << p1 << ", " << p2 << ")");
    NOTICE("    PLLA0: clock is " << calc_vclock(m1, m2, n, p1, p2));

    n = (m_State->fpa1 >> FP_N_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m1 = (m_State->fpa1 >> FP_M1_DIVISOR_SHIFT) & FP_DIVISOR_MASK;
    m2 = (m_State->fpa1 >> FP_M2_DIVISOR_SHIFT) & FP_DIVISOR_MASK;

    get_p1p2(m_State->dpll_a, &p1, &p2);

    NOTICE("    PLLA1: (m1, m2, n, p1, p2) = (" << m1 << ", " << m2 << ", " << n << ", " << p1 << ", " << p2 << ")");
    NOTICE("    PLLA1: clock is " << calc_vclock(m1, m2, n, p1, p2));

    NOTICE("    HTOTAL_A:       " << m_State->htotal_a);
    NOTICE("    HBLANK_A:       " << m_State->hblank_a);
    NOTICE("    HSYNC_A:        " << m_State->hsync_a);
    NOTICE("    VTOTAL_A:       " << m_State->vtotal_a);
    NOTICE("    VBLANK_A:       " << m_State->vblank_a);
    NOTICE("    VSYNC_A:        " << m_State->vsync_a);
    NOTICE("    SRC_SIZE_A:     " << m_State->src_size_a);
    NOTICE("    BCLRPAT_A:      " << m_State->bclrpat_a);
    NOTICE("    HTOTAL_B:       " << m_State->htotal_b);
    NOTICE("    HBLANK_B:       " << m_State->hblank_b);
    NOTICE("    HSYNC_B:        " << m_State->hsync_b);
    NOTICE("    VTOTAL_B:       " << m_State->vtotal_b);
    NOTICE("    VBLANK_B:       " << m_State->vblank_b);
    NOTICE("    VSYNC_B:        " << m_State->vsync_b);
    NOTICE("    SRC_SIZE_B:     " << m_State->src_size_b);
    NOTICE("    BCLRPAT_B:      " << m_State->bclrpat_b);

    NOTICE("    ADPA:           " << m_State->adpa);
    NOTICE("    DVOA:           " << m_State->dvoa);
    NOTICE("    DVOB:           " << m_State->dvob);
    NOTICE("    DVOC:           " << m_State->dvoc);
    NOTICE("    DVOA_SRCDIM:        " << m_State->dvoa_srcdim);
    NOTICE("    DVOB_SRCDIM:        " << m_State->dvob_srcdim);
    NOTICE("    DVOC_SRCDIM:        " << m_State->dvoc_srcdim);
    NOTICE("    LVDS:           " << m_State->lvds);

    NOTICE("    PIPEACONF:      " << m_State->pipe_a_conf);
    NOTICE("    PIPEBCONF:      " << m_State->pipe_b_conf);
    NOTICE("    DISPARB:        " << m_State->disp_arb);

    NOTICE("    CURSOR_A_CONTROL:   " << m_State->cursor_a_control);
    NOTICE("    CURSOR_B_CONTROL:   " << m_State->cursor_b_control);
    NOTICE("    CURSOR_A_BASEADDR:  " << m_State->cursor_a_base);
    NOTICE("    CURSOR_B_BASEADDR:  " << m_State->cursor_b_base);

    NOTICE("    CURSOR_SIZE:        " << m_State->cursor_size);

    NOTICE("    DSPACNTR:       " << m_State->disp_a_ctrl);
    NOTICE("    DSPBCNTR:       " << m_State->disp_b_ctrl);
    NOTICE("    DSPABASE:       " << m_State->disp_a_base);
    NOTICE("    DSPBBASE:       " << m_State->disp_b_base);
    NOTICE("    DSPASTRIDE:     " << m_State->disp_a_stride);
    NOTICE("    DSPBSTRIDE:     " << m_State->disp_b_stride);

    NOTICE("    VGACNTRL:       " << m_State->vgacntrl);
    NOTICE("    ADD_ID:         " << m_State->add_id);

    NOTICE("    INSTPM          " << m_State->instpm);
    NOTICE("    MEM_MODE        " << m_State->mem_mode);
    NOTICE("    FW_BLC_0        " << m_State->fw_blc_0);
    NOTICE("    FW_BLC_1        " << m_State->fw_blc_1);

    NOTICE("    HWSTAM          " << m_State->hwstam);
    NOTICE("    IER         " << m_State->ier);
    NOTICE("    IIR         " << m_State->iir);
    NOTICE("    IMR         " << m_State->imr);
    NOTICE("hw state dump end");
}

/* It is assumed that hw is filled in with the initial state information. */
void i945G::SetMode(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t pipe = PIPE_A;
    uint32_t *dpll, *fp0, *fp1, *pipe_conf;
    uint32_t m1, m2, n, p1, p2, clock;
    uint32_t hsync_start, hsync_end, hblank_start, hblank_end, htotal, hactive;
    uint32_t vsync_start, vsync_end, vblank_start, vblank_end, vtotal, vactive;
    uint32_t vsync_pol, hsync_pol;
    uint32_t dpll_reg, fp0_reg, fp1_reg, pipe_conf_reg, pipe_stat_reg;
    uint32_t hsync_reg, htotal_reg, hblank_reg;
    uint32_t vsync_reg, vtotal_reg, vblank_reg;
    uint32_t src_size_reg;
    uint32_t count, tmp_val[3];
    uint32_t hblank, _htotal, hperiod, pixclock, left_margin, right_margin, lower_margin, upper_margin, hsync, vsync, sync;
    uint64_t tm;

    width &= ~7;
    if(width == (height * 4)/3 && !((height * 4) % 3)) vsync = 4;
    else if(width == (height * 16)/9 && !((height * 16) % 9)) vsync = 5;
    else if(width == (height * 16)/10 && !((height * 16) % 10)) vsync = 6;
    else if(width == (height * 5)/4 && !((height * 5) % 4)) vsync = 7;
    else if(width == (height * 15)/9 && !((height * 15) % 9)) vsync = 7;
    else vsync = 10;
    hperiod = 2*(16116666/(2*(height+3)));
    uint32_t idc = (7680000-((76800 * hperiod)/1000))/256;
    hblank = (idc<20000?width/4:(width*idc)/(100000-idc))&~0xf;
    _htotal = width + hblank;
    pixclock = 1000000000/(1000000000UL/(((_htotal*1000000)/hperiod/250)*250));
    hsync = ((8*_htotal)/100)&~7;
    left_margin = hblank/2;
    right_margin = hblank - hsync - left_margin;
    upper_margin = 0;
    uint32_t vbil = 550000/hperiod + 4, mvbil = vsync+9;
    lower_margin = (vbil<mvbil?mvbil:vbil)-vsync;

    /* Check whether pipe A or pipe B is enabled.
    if(m_State->pipe_a_conf & PIPECONF_ENABLE)
        pipe = PIPE_A;
    else if(m_State->pipe_b_conf & PIPECONF_ENABLE)
        pipe = PIPE_B;*/

    /* Set which pipe's registers will be set. */
    if(pipe == PIPE_B) {
        dpll = &m_State->dpll_b;
        fp0 = &m_State->fpb0;
        fp1 = &m_State->fpb1;
        pipe_conf = &m_State->pipe_b_conf;
        dpll_reg = DPLL_B;
        fp0_reg = FPB0;
        fp1_reg = FPB1;
        pipe_conf_reg = PIPEBCONF;
        pipe_stat_reg = PIPEBSTAT;
        hsync_reg = HSYNC_B;
        htotal_reg = HTOTAL_B;
        hblank_reg = HBLANK_B;
        vsync_reg = VSYNC_B;
        vtotal_reg = VTOTAL_B;
        vblank_reg = VBLANK_B;
        src_size_reg = SRC_SIZE_B;
    } else {
        dpll = &m_State->dpll_a;
        fp0 = &m_State->fpa0;
        fp1 = &m_State->fpa1;
        pipe_conf = &m_State->pipe_a_conf;
        dpll_reg = DPLL_A;
        fp0_reg = FPA0;
        fp1_reg = FPA1;
        pipe_conf_reg = PIPEACONF;
        pipe_stat_reg = PIPEASTAT;
        hsync_reg = HSYNC_A;
        htotal_reg = HTOTAL_A;
        hblank_reg = HBLANK_A;
        vsync_reg = VSYNC_A;
        vtotal_reg = VTOTAL_A;
        vblank_reg = VBLANK_A;
        src_size_reg = SRC_SIZE_A;
    }

    /* Use ADPA register for sync control. */
    m_State->adpa &= ~ADPA_USE_VGA_HVPOLARITY;

    /* sync polarity */
    hsync_pol = ADPA_SYNC_ACTIVE_LOW;
    vsync_pol = ADPA_SYNC_ACTIVE_HIGH;
    m_State->adpa &= ~((ADPA_SYNC_ACTIVE_MASK << ADPA_VSYNC_ACTIVE_SHIFT) | (ADPA_SYNC_ACTIVE_MASK << ADPA_HSYNC_ACTIVE_SHIFT));
    m_State->adpa |= (hsync_pol << ADPA_HSYNC_ACTIVE_SHIFT) | (vsync_pol << ADPA_VSYNC_ACTIVE_SHIFT);

    /* Connect correct pipe to the analog port DAC */
    m_State->adpa &= ~(PIPE_MASK << ADPA_PIPE_SELECT_SHIFT);
    m_State->adpa |= (pipe << ADPA_PIPE_SELECT_SHIFT);

    /* Set DPMS state to D0 (on) */
    m_State->adpa &= ~ADPA_DPMS_CONTROL_MASK;
    m_State->adpa |= ADPA_DPMS_D0;

    m_State->adpa |= ADPA_DAC_ENABLE;

    *dpll |= (DPLL_VCO_ENABLE | DPLL_VGA_MODE_DISABLE);
    *dpll &= ~(DPLL_RATE_SELECT_MASK | DPLL_REFERENCE_SELECT_MASK);
    *dpll |= (DPLL_REFERENCE_DEFAULT | DPLL_RATE_SELECT_FP0);

    calc_pll_params(pixclock, &m1, &m2, &n, &p1, &p2, &clock);

    *dpll &= ~DPLL_P1_FORCE_DIV2;
    *dpll &= ~((DPLL_P2_MASK << DPLL_P2_SHIFT) | (DPLL_P1_MASK << DPLL_P1_SHIFT));

    *dpll |= (p2 << DPLL_I9XX_P2_SHIFT);
    *dpll |= (1 << (p1 - 1)) << DPLL_P1_SHIFT;

    *fp0 = (n << FP_N_DIVISOR_SHIFT) | (m1 << FP_M1_DIVISOR_SHIFT) | (m2 << FP_M2_DIVISOR_SHIFT);
    *fp1 = *fp0;

    m_State->dvob &= ~PORT_ENABLE;
    m_State->dvoc &= ~PORT_ENABLE;

    /* Use display plane A. */
    m_State->disp_a_ctrl |= DISPPLANE_PLANE_ENABLE;
    m_State->disp_a_ctrl &= ~DISPPLANE_GAMMA_ENABLE;
    m_State->disp_a_ctrl &= ~DISPPLANE_PIXFORMAT_MASK;
    switch (depth) {
    case 8:
        m_State->disp_a_ctrl |= DISPPLANE_8BPP | DISPPLANE_GAMMA_ENABLE;
        break;
    case 15:
        m_State->disp_a_ctrl |= DISPPLANE_15_16BPP;
        break;
    case 16:
        m_State->disp_a_ctrl |= DISPPLANE_16BPP;
        break;
    case 24:
        m_State->disp_a_ctrl |= DISPPLANE_32BPP_NO_ALPHA;
        break;
    }
    m_State->disp_a_ctrl &= ~(PIPE_MASK << DISPPLANE_SEL_PIPE_SHIFT);
    m_State->disp_a_ctrl |= (pipe << DISPPLANE_SEL_PIPE_SHIFT);

    /* Set CRTC registers. */
    hactive = width;
    hsync_start = hactive + right_margin;
    hsync_end = hsync_start + hsync;
    htotal = hsync_end + left_margin;
    hblank_start = hactive;
    hblank_end = htotal;

    vactive = height;
    vsync_start = vactive + lower_margin;
    vsync_end = vsync_start + vsync;
    vtotal = vsync_end + upper_margin;
    vblank_start = vactive;
    vblank_end = vtotal;
    vblank_end = vsync_end + 1;

    /* Adjust for register values, and check for overflow. */
    hactive--;
    hsync_start--;
    hsync_end--;
    htotal--;
    hblank_start--;
    hblank_end--;
    vactive--;
    vsync_start--;
    vsync_end--;
    vtotal--;
    vblank_start--;
    vblank_end--;

    m_State->disp_a_base = 0;

    /* Set the palette to 8-bit mode. */
    *pipe_conf &= ~PIPECONF_GAMMA;
    *pipe_conf &= ~PIPECONF_INTERLACE_MASK;

    ///Start of writing mode
    m_pBase->write32(m_pBase->read32(VGACNTRL)|VGA_DISABLE, VGACNTRL);
    m_pBase->write32(m_pBase->read32(pipe_conf_reg)&~PIPECONF_ENABLE, pipe_conf_reg);

    count = 0;
    do
    {
        tmp_val[count % 3] = m_pBase->read32(PIPEA_DSL);
        if((tmp_val[0] == tmp_val[1]) && (tmp_val[1] == tmp_val[2]))
            break;
        count++;
        tm = Machine::instance().getTimer()->getTickCount();
        while(Machine::instance().getTimer()->getTickCount()<tm+1);
        if(count % 200 == 0)
            m_pBase->write32(m_pBase->read32(pipe_conf_reg)&~PIPECONF_ENABLE, pipe_conf_reg);
    } while(count < 2000);

    m_pBase->write32(m_pBase->read32(ADPA)&~ADPA_DAC_ENABLE, ADPA);

    /* Disable planes A and B. */
    m_pBase->write32(m_pBase->read32(DSPACNTR)&~DISPPLANE_PLANE_ENABLE, DSPACNTR);
    m_pBase->write32(m_pBase->read32(DSPBCNTR)&~DISPPLANE_PLANE_ENABLE, DSPBCNTR);

    /* Wait for vblank. For now, just wait for a 50Hz cycle (20ms)) */

    tm = Machine::instance().getTimer()->getTickCount();
    while(Machine::instance().getTimer()->getTickCount()<tm+20);

    m_pBase->write32(m_pBase->read32(DVOB) & ~PORT_ENABLE, DVOB);
    m_pBase->write32(m_pBase->read32(DVOC) & ~PORT_ENABLE, DVOC);
    m_pBase->write32(m_pBase->read32(ADPA) & ~ADPA_DAC_ENABLE, ADPA);

    /* Disable Sync */
    m_pBase->write32((m_pBase->read32(ADPA)&~ADPA_DPMS_CONTROL_MASK)|ADPA_DPMS_D3, ADPA);

    /* do some funky magic - xyzzy */
    m_pBase->write32(0xabcd0000, 0x61204);

    /* turn off PLL */
    m_pBase->write32(m_pBase->read32(dpll_reg)&~DPLL_VCO_ENABLE, dpll_reg);

    /* Set PLL parameters */
    m_pBase->write32(*fp0, fp0_reg);
    m_pBase->write32(*fp1, fp1_reg);

    /* Enable PLL */
    m_pBase->write32(*dpll, dpll_reg);

    tm = Machine::instance().getTimer()->getTickCount();
    while(Machine::instance().getTimer()->getTickCount()<tm+500);

    /* Set DVOs B/C */
    m_pBase->write32(m_State->dvob, DVOB);
    m_pBase->write32(m_State->dvoc, DVOC);

    /* Set pipe parameters */
    m_pBase->write32((hsync_start << HSYNCSTART_SHIFT) | (hsync_end << HSYNCEND_SHIFT), hsync_reg);
    m_pBase->write32((hblank_start << HBLANKSTART_SHIFT) | (hblank_end << HSYNCEND_SHIFT), hblank_reg);
    m_pBase->write32((htotal << HTOTAL_SHIFT) | (hactive << HACTIVE_SHIFT), htotal_reg);
    m_pBase->write32((vsync_start << VSYNCSTART_SHIFT) | (vsync_end << VSYNCEND_SHIFT), vsync_reg);
    m_pBase->write32((vblank_start << VBLANKSTART_SHIFT) | (vblank_end << VSYNCEND_SHIFT), vblank_reg);
    m_pBase->write32((vtotal << VTOTAL_SHIFT) | (vactive << VACTIVE_SHIFT), vtotal_reg);

    /* undo funky magic */
    m_pBase->write32(0x00000000, 0x61204);

    /* Set ADPA */
    m_pBase->write32(m_pBase->read32(ADPA) | ADPA_DAC_ENABLE, ADPA);
    m_pBase->write32((m_State->adpa & ~(ADPA_DPMS_CONTROL_MASK)) | ADPA_DPMS_D3 | ADPA_DAC_ENABLE, ADPA);
    m_pBase->write32((hactive << SRC_SIZE_HORIZ_SHIFT) | (vactive << SRC_SIZE_VERT_SHIFT), src_size_reg);
    m_pBase->write32(0xFFFF, pipe_stat_reg); /* clear all status bits only */

    /* Enable pipe */
    m_pBase->write32(*pipe_conf | PIPECONF_ENABLE, pipe_conf_reg);
    m_pBase->write32((m_pBase->read32(ADPA)&~ADPA_DPMS_CONTROL_MASK)|ADPA_DPMS_D0, ADPA);

    m_pBase->write32(m_State->disp_a_ctrl & ~DISPPLANE_PLANE_ENABLE, DSPACNTR);
    m_pBase->write32(m_State->disp_a_stride, DSPASTRIDE);
    m_pBase->write32(m_State->disp_a_base, DSPABASE);
    m_pBase->write32(m_pBase->read32(DSPACNTR)|DISPPLANE_PLANE_ENABLE, DSPACNTR);
    m_pBase->write32(m_State->disp_a_base, DSPABASE);
}

void i945G::Blank(bool state)
{
    m_pBase->write32((state?m_pBase->read32(DSPACNTR)|DISPPLANE_PLANE_ENABLE:m_pBase->read32(DSPACNTR)&~DISPPLANE_PLANE_ENABLE), DSPACNTR);
    m_pBase->write32(m_pBase->read32(DSPABASE), DSPABASE);
    m_pBase->write32((m_pBase->read32(ADPA)&~ADPA_DPMS_CONTROL_MASK)|(state?ADPA_DPMS_D0:ADPA_DPMS_D3), ADPA);
    NOTICE("945G: Blank: State is "<<(state?"on ":"off ")<<m_pBase->read32(ADPA));
}

bool i945G::irq(irq_id_t number, InterruptState &state)
{
    return true;
}
