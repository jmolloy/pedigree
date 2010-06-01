// C++ entry point 

#if 0
inline void writeChar(char c)
{
#if defined( ARM_VERSATILE )
  volatile char *p = reinterpret_cast<volatile char*>(0x101f1000);
#elif defined( ARM_INTEGRATOR )
  volatile char *p = reinterpret_cast<volatile char*>(0x16000000);
#else
  #error No valid ARM board!
#endif
  *p = c;
  asm volatile("" ::: "memory");
#ifndef SERIAL_IS_FILE
  *p = 0;
  asm volatile("" ::: "memory");
#endif
}

inline void writeStr(const char *str)
{
  char c;
  while ((c = *str++))
    writeChar(c);
}

void writeHex(unsigned int n)
{
    bool noZeroes = true;

    int i;
    unsigned int tmp;
    for (i = 28; i > 0; i -= 4)
    {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && noZeroes)
        {
            continue;
        }
    
        if (tmp >= 0xA)
        {
            noZeroes = false;
            writeChar (tmp-0xA+'a');
        }
        else
        {
            noZeroes = false;
            writeChar( tmp+'0');
        }
    }
  
    tmp = n & 0xF;
    if (tmp >= 0xA)
    {
        writeChar (tmp-0xA+'a');
    }
    else
    {
        writeChar( tmp+'0');
    }

}

#endif

/// Page/section references are from the OMAP35xx Technical Reference Manual

#define DLL_REG         0x00 // R/W
#define RHR_REG         0x00 // R
#define THR_REG         0x00 // W
#define DLH_REG         0x04 // R/W
#define IER_REG         0x04 // R/W
#define IIR_REG         0x08 // R
#define FCR_REG         0x08 // W
#define EFR_REG         0x08 // RW
#define LCR_REG         0x0C // RW
#define MCR_REG         0x10 // RW
#define XON1_ADDR1_REG  0x10 // RW
#define LSR_REG         0x14 // R
#define XON2_ADDR2_REG  0x14 // RW
#define MSR_REG         0x18 // R
#define TCR_REG         0x18 // RW
#define XOFF1_REG       0x18 // RW
#define SPR_REG         0x1C // RW
#define TLR_REG         0x1C // RW
#define XOFF2_REG       0x1C // RW
#define MDR1_REG        0x20 // RW
#define MDR2_REG        0x24 // RW

#define USAR_REG        0x38 // R

#define SCR_REG         0x40 // RW
#define SSR_REG         0x44 // R

#define MVR_REG         0x50 // R
#define SYSC_REG        0x54 // RW
#define SYSS_REG        0x58 // R
#define WER_REG         0x5C // RW

extern "C" void __start()
{
    volatile unsigned int *usrleds = (volatile unsigned int*) 0x49056090;
    usrleds[0] = 0x00600000;
    
    volatile unsigned char *uart = (volatile unsigned char*) 0x49020000; // 0x4806A000;

    /** Reset the UART. Page 2677, section 17.5.1.1.1 **/

    // 1. Initiate a software reset
    uart[SYSC_REG] |= 0x2;

    // 2. Wait for the end of the reset operation
    while(!(uart[SYSS_REG] & 0x1));

    /** Configure FIFOs and DMA **/

    // 1. Switch to configuration mode B to access the EFR_REG register
    unsigned char old_lcr_reg = uart[LCR_REG];
    uart[LCR_REG] = 0xBF;

    // 2. Enable submode TCR_TLR to access TLR_REG (part 1 of 2)
    unsigned char efr_reg = uart[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 3. Switch to configuration mode A
    uart[LCR_REG] = 0x80;

    // 4. Enable submode TCL_TLR to access TLR_REG (part 2 of 2)
    unsigned char mcr_reg = uart[MCR_REG];
    unsigned char old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[MCR_REG] = mcr_reg;

    // 5. Enable FIFO, load new FIFO triggers (part 1 of 3), and the new DMA mode (part 1 of 2)
    uart[FCR_REG] = 1; // TX and RX FIFO triggers at 8 characters, no DMA mode

    // 6. Switch to configuration mode B to access EFR_REG
    uart[LCR_REG] = 0xBF;

    // 7. Load new FIFO triggers (part 2 of 3)
    uart[TLR_REG] = 0;

    // 8. Load new FIFO triggers (part 3 of 3) and the new DMA mode (part 2 of 2)
    uart[SCR_REG] = 0;

    // 9. Restore the ENHANCED_EN value saved in step 2
    if(!old_enhanced_en)
        uart[EFR_REG] = uart[EFR_REG] ^ 0x8;

    // 10. Switch to configuration mode A to access the MCR_REG register
    uart[LCR_REG] = 0x80;

    // 11. Restore the MCR_REG TCR_TLR value saved in step 4
    if(!old_tcl_tlr)
        uart[MCR_REG] = uart[MCR_REG] ^ 0x20;

    // 12. Restore the LCR_REG value stored in step 1
    uart[LCR_REG] = old_lcr_reg;

    /** Configure protocol, baud and interrupts **/

    // 1. Disable UART to access DLL_REG and DLH_REG
    uart[MDR1_REG] = (uart[MDR1_REG] & ~0x7) | 0x7;

    // 2. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 3. Enable access to IER_REG
    efr_reg = uart[EFR_REG];
    old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 4. Switch to operational mode to access the IER_REG register
    uart[LCR_REG] = 0;

    // 5. Clear IER_REG
    uart[IER_REG] = 0;

    // 6. Switch to configuration mode B to access DLL_REG and DLH_REG
    uart[LCR_REG] = 0xBF;

    // 7. Load the new divisor value (looking for 115200 baud)
    uart[0x0] = 0x1A; // divisor low byte
    uart[0x4] = 0; // divisor high byte

    // 8. Switch to operational mode to access the IER_REG register
    uart[LCR_REG] = 0;

    // 9. Load new interrupt configuration
    uart[IER_REG] = 0; // No interrupts wanted at this stage

    // 10. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 11. Restore the ENHANCED_EN value saved in step 3
    if(old_enhanced_en)
        uart[EFR_REG] = uart[EFR_REG] ^ 0x8;

    // 12. Load the new protocol formatting (parity, stop bit, character length)
    // and enter operational mode
    uart[LCR_REG] = 0x3; // 8 bit characters, no parity, one stop bit

    // 13. Load the new UART mode
    uart[MDR1_REG] = 0;

    /** Configure hardware flow control */

    // 1. Switch to configuration mode A to access the MCR_REG register
    old_lcr_reg = uart[LCR_REG];
    uart[LCR_REG] = 0x80;

    // 2. Enable submode TCR_TLR to access TCR_REG (part 1 of 2)
    mcr_reg = uart[MCR_REG];
    old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[MCR_REG] = mcr_reg;

    // 3. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 4. Enable submode TCR_TLR to access the TCR_REG register (part 2 of 2)
    efr_reg = uart[EFR_REG];
    old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 5. Load new start and halt trigger values
    uart[TCR_REG] = 0;

    // 6. Disable RX/TX hardware flow control mode, and restore the ENHANCED_EN
    // values stored in step 4
    uart[EFR_REG] = 0;

    // 7. Switch to configuration mode A to access MCR_REG
    uart[LCR_REG] = 0x80;

    // 8. Restore the MCR_REG TCR_TLR value stored in step 2
    if(!old_tcl_tlr)
        uart[MCR_REG] = uart[MCR_REG] ^ 0x20;

    // 9. Restore the LCR_REG value saved in step 1
    uart[LCR_REG] = old_lcr_reg;

    usrleds[1] = 0x00200000;

    // Write some text.
    uart[0] = 'H';
    uart[0] = 'e';
    uart[0] = 'l';
    uart[0] = 'l';
    uart[0] = 'o';
    uart[0] = '!';
    uart[0] = ' ';
    uart[0] = ':';
    uart[0] = 'D';
    
    while (1);
}

