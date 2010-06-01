// C++ entry point 

extern "C" {
volatile unsigned char *uart1 = (volatile unsigned char*) 0x4806A000;
volatile unsigned char *uart2 = (volatile unsigned char*) 0x4806C000;
volatile unsigned char *uart3 = (volatile unsigned char*) 0x49020000;

volatile unsigned char *leds = (volatile unsigned char*) 0x49056000; // GPIO5
}

/// \note Page/section references are from the OMAP35xx Technical Reference Manual

/** UART/IrDA/CIR Registers */
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

/// Gets a uart MMIO block given a number
extern "C" volatile unsigned char *uart_get(int n)
{
    if(n == 1)
        return uart1;
    else if(n == 2)
        return uart2;
    else if(n == 3)
        return uart3;
    else
        return 0;
}

/// Perform a software reset of a given UART
extern "C" bool uart_softreset(int n)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return false;

    /** Reset the UART. Page 2677, section 17.5.1.1.1 **/

    // 1. Initiate a software reset
    uart[SYSC_REG] |= 0x2;

    // 2. Wait for the end of the reset operation
    while(!(uart[SYSS_REG] & 0x1));

    return true;
}

/// Configure FIFOs and DMA to default values
extern "C" bool uart_fifodefaults(int n)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return false;

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

    return true;
}

/// Configure the UART protocol (to defaults - 115200 baud, 8 character bits,
/// no paruart_protoconfigity, 1 stop bit). Will also enable the UART for output as a side
/// effect.
extern "C" bool uart_protoconfig(int n)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return false;
    
    /** Configure protocol, baud and interrupts **/

    // 1. Disable UART to access DLL_REG and DLH_REG
    uart[MDR1_REG] = (uart[MDR1_REG] & ~0x7) | 0x7;

    // 2. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 3. Enable access to IER_REG
    unsigned char efr_reg = uart[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
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

    return true;
}

/// Completely disable flow control on the UART
extern "C" bool uart_disableflowctl(int n)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return false;

    /** Configure hardware flow control */

    // 1. Switch to configuration mode A to access the MCR_REG register
    unsigned char old_lcr_reg = uart[LCR_REG];
    uart[LCR_REG] = 0x80;

    // 2. Enable submode TCR_TLR to access TCR_REG (part 1 of 2)
    unsigned char mcr_reg = uart[MCR_REG];
    unsigned char old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[MCR_REG] = mcr_reg;

    // 3. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 4. Enable submode TCR_TLR to access the TCR_REG register (part 2 of 2)
    unsigned char efr_reg = uart[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
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

    return true;
}

extern "C" void uart_write(int n, char c)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return;

    // Wait until the hold register is empty
    while(!(uart[LSR_REG] & 0x20));
    uart[THR_REG] = c;
}

extern "C" char uart_read(int n)
{
    volatile unsigned char *uart = uart_get(n);
    if(!uart)
        return 0;

    // Wait for data in the receive FIFO
    while(!(uart[LSR_REG] & 0x1));
    return uart[RHR_REG];
}

extern "C" inline void writeStr(int n, const char *str)
{
  char c;
  while ((c = *str++))
    uart_write(n, c);
}

extern "C" void writeHex(int uart, unsigned int n)
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
            uart_write(uart, tmp-0xA+'a');
        }
        else
        {
            noZeroes = false;
            uart_write(uart, tmp+'0');
        }
    }
  
    tmp = n & 0xF;
    if (tmp >= 0xA)
    {
        uart_write(uart, tmp-0xA+'a');
    }
    else
    {
        uart_write(uart, tmp+'0');
    }

}

bool led_state(int n)
{
    char state = leds[0x96];
    if((n == 0) && (state & 0x40))
        return true;
    else if((n == 1) && (state & 0x20))
        return true;
    else
        return false;
}

void led_on(int n)
{
    if(led_state(n))
        return; // Already on!

    char states = leds[0x96];
    if(n == 0)
        states |= 0x40;
    else if(n == 1)
        states |= 0x20;
    else
        return;
    leds[0x96] = states;
}

void led_off(int n)
{
    if(!led_state(n))
        return; // Already off!

    char states = 0;
    if(n == 0)
        states = 0x40;
    else if(n == 1)
        states = 0x20;
    else
        return;
    leds[0x92] = states;
}

void led_toggle(int n)
{
    if(led_state(n))
        led_off(n);
    else
        led_on(n);
}

void led_clear()
{
    leds[0x92] = 0x60;
}

extern "C" void __start()
{
    bool b = uart_softreset(3);
    if(!b)
        while(1);
    b = uart_fifodefaults(3);
    if(!b)
        while(1);
    b = uart_protoconfig(3);
    if(!b)
        while(1);
    b = uart_disableflowctl(3);
    if(!b)
        while(1);
    
    led_clear();
    led_on(0); // Switch on the USR0 LED to show we're active and thinking

    writeStr(3, "Pedigree for the BeagleBoard\r\n\r\n");

    writeStr(3, "Press 1 to toggle the USR0 LED, and 2 to toggler the USR1 LED.\r\nPress 0 to clear both LEDs.\r\n");
    while(1)
    {
        char c = uart_read(3);
        if(c == '1')
        {
            writeStr(3, "Toggling USR0 LED\r\n");
            led_toggle(0);
        }
        else if(c == '2')
        {
            writeStr(3, "Toggling USR1 LED\r\n");
            led_toggle(1);
        }
        else if(c == '0')
        {
            writeStr(3, "Clearing both USR0 and USR1 LEDs\r\n");
            led_clear();
        }
    }
    
    while (1);
}

