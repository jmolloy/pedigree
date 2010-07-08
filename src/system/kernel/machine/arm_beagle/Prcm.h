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
#include <processor/types.h>
#include <processor/MemoryRegion.h>

/** PRCM - Power, Reset and Clock Management - abstraction
  * \todo Power management
  * \todo Reset
  */
class Prcm
{
    public:
        Prcm();
        virtual ~Prcm();

        static Prcm &instance()
        {
            return m_Instance;
        }

        enum Clock
        {
            FCLK_32K    = 0,
            SYS_CLK     = 1,

            L3_CLK_DIV1 = 1,
            L3_CLK_DIV2 = 2,
        };

        enum ModuleBases
        {
            IVA2_CM                 = 0x0000,
            OCP_System_Reg_CM       = 0x0800,
            MPU_CM                  = 0x0900,
            CORE_CM                 = 0x0A00,
            SGX_CM                  = 0x0B00,
            WKUP_CM                 = 0x0C00,
            Clock_Control_Reg_CM    = 0x0D00,
            DSS_CM                  = 0x0E00,
            CAM_CM                  = 0x0F00,
            PER_CM                  = 0x1000,
            EMU_CM                  = 0x1100,
            Global_Reg_CM           = 0x1200,
            NEON_CM                 = 0x1300,
            USBHOST_CM              = 0x1400,
        };

        enum Registers_PER
        {
            CM_FCLKEN_PER           = 0x00, // RW
            CM_ICLKEN_PER           = 0x10, // RW
            CM_IDLEST_PER           = 0x20, // R
            CM_AUTOIDLE_PER         = 0x30, // RW
            CM_CLKSEL_PER           = 0x40, // RW
            CM_SLEEPDEP_PER         = 0x44, // RW
            CM_CLKSTCRL_PER         = 0x48, // RW
            CM_CLKSTST_PER          = 0x4C, // R
        };

        enum Register_CORE
        {
            CM_FCLKEN1_CORE         = 0x00, // RW
            CM_FCLKEN3_CORE         = 0x08, // RW
            CM_ICLKEN1_CORE         = 0x10, // RW
            CM_ICLKEN3_CORE         = 0x18, // RW
            CM_IDLEST1_CORE         = 0x20, // R
            CM_IDLEST3_CORE         = 0x28, // R
            CM_AUTOIDLE1_CORE       = 0x30, // RW
            CM_AUTOIDLE3_CORE       = 0x38, // RW
            CM_CLKSEL_CORE          = 0x40, // RW
            CM_CLKSTCTRL_CORE       = 0x48, // RW
            CM_CLKSTST_CORE         = 0x4C, // R
        };

        enum Registers_PLL
        {
            CM_CLKEN_PLL            = 0x00, // RW
            CM_CLKEN2_PLL           = 0x04, // RW
            CM_IDLEST_CKGEN         = 0x20, // R
            CM_IDLEST2_CKGEN        = 0x24, // R
            CM_AUTOIDLE_PLL         = 0x30, // RW
            CM_AUTOIDLE2_PLL        = 0x34, // RW
            CM_CLKSEL1_PLL          = 0x40, // RW
            CM_CLKSEL2_PLL          = 0x44, // RW
            CM_CLKSEL3_PLL          = 0x48, // RW
            CM_CLKSEL4_PLL          = 0x4C, // RW
            CM_CLKSEL5_PLL          = 0x50, // RW
            CM_CLKOUT_CTRL          = 0x70, // RW
        };

        /** Initialises the PRCM from a specific base */
        void initialise(uintptr_t base);

        /** Handle source clock selection for PER */
        void SelectClockPER(size_t clock, Clock which);

        /** Handle functional clock enable/disable for PER */
        void SetFuncClockPER(size_t clock, bool enabled);

        /** Handle interface clock enable/disable for PER */
        void SetIfaceClockPER(size_t clock, bool enabled);

        /** Handle functional clock enable/disable for CORE */
        void SetFuncClockCORE(size_t n, size_t clock, bool enabled);

        /** Handle interface clock enable/disable for CORE */
        void SetIfaceClockCORE(size_t n, size_t clock, bool enabled);

        /** Waits for a bit in the IDLEST CORE register to change state. */
        void WaitCoreIdleStatus(size_t n, size_t clock, bool waitForOn);

        /** Waits for a bit in the IDLEST PLL register to change state. */
        void WaitPllIdleStatus(size_t n, size_t clock, bool waitForOn);

        /** Handle source clock selection for CORE */
        void SelectClockCORE(size_t clock, Clock which);

        /** Enables or disables a PLL clock (programmer chooses value) */
        void SetClockPLL(size_t n, size_t value);

        /** Clock selection for PLL. The register contents must be set by the
          * programmer. */
        void SelectClockPLL(size_t n, size_t value);

    private:
        static Prcm m_Instance;

        MemoryRegion m_Base;
};
