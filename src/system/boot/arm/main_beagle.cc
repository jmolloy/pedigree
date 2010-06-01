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

    // 1. Switch to configuration mode B to access the UARTi.EFR_REG register
    unsigned char old_lcr_reg = uart[0xC];
    uart[0xC] = 0xBF;

    // Enable TCL_TLR to access the TLR register (part 1)
    // Set the EHANCED_EN bit
    unsigned char efr_reg = uart[0x8];
    unsigned char old_efr_reg = efr_reg;
    if(!(efr_reg & 0x8))
        efr_reg |= 0x8;
    uart[0x8] = efr_reg;

    // Switch to configuration mode A
    uart[0xC] = 0x80;

    // Enable TCL_TLR to access the TLR register (part 2)
    // Set the TCL_TLR bit
    unsigned char mcr_reg = uart[0x10];
    unsigned char old_mcr_reg = mcr_reg;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[0x10] = mcr_reg;

    // Configure the FIFO and DMA modes (disable both)
    uart[0x8] = 0;

    // Restore the ENHANCED_EN and TCL_TLR bit values
    uart[0x10] = old_mcr_reg;
    uart[0xC] = 0xBF;
    uart[0x8] = old_efr_reg;
    uart[0xC] = old_lcr_reg;

    /** Configure protocol, baud and interrupts **/

    // Disable the UART
    unsigned char mdr1_reg = uart[0x20];
    mdr1_reg = (mdr1_reg & ~0x7) | 0x7;
    uart[0x20] = mdr1_reg;

    // Switch to configuration mode B
    uart[0xC] = 0xBF;

    // Enable access to the IER register (save the old ENHANCED_EN value)
    efr_reg = old_efr_reg = uart[0x8];
    if(!(efr_reg & 0x8))
        efr_reg |= 0x8;
    uart[0x8] = efr_reg;

    // Switch to operational mode to access the IER register
    uart[0xC] = 0;

    // Clear all interrupt enable bits
    uart[0x4] = 0;

    // Switch back to configuration mode B
    uart[0xC] = 0xBF;

    // Load the divisor for the baud rate
    uart[0x0] = 0x1A; // divisor low byte
    uart[0x4] = 0; // divisor high byte

    // Restore the ENHANCED_EN value
    uart[0x8] = old_efr_reg;

    // Load parity, stop bit and character length information, then switch to
    // operational mode
    unsigned char lcr_reg = 0x3; // 8 bit characters, no parity, one stop bit
    uart[0xC] = lcr_reg;

    usrleds[1] = 0x00200000;

    // Write some text.
    uart[0] = 'H';
    uart[0] = 'e';
    uart[0] = 'l';
    uart[0] = 'l';
    uart[0] = 'o';
    uart[0] = '!';
    
    while (1);
}

