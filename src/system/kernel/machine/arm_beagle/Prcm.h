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
            FCLK_32K,
            SYS_CLK
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

        /** Initialises the PRCM from a specific base */
        void initialise(uintptr_t base);

        /** Handle source clock selection for PER */
        void SelectClockPER(size_t clock, Clock which);

        /** Handle functional clock enable/disable for PER */
        void SetFuncClockPER(size_t clock, bool enabled);

        /** Handle interface clock enable/disable for PER */
        void SetIfaceClockPER(size_t clock, bool enabled);

    private:
        static Prcm m_Instance;

        MemoryRegion m_Base;
};
