// C++ entry point

#include "Elf32.h"

// autogen.h contains the full kernel binary in a char array
#include "autogen.h"

extern "C" {
volatile unsigned char *uart1 = (volatile unsigned char*) 0x4806A000;
volatile unsigned char *uart2 = (volatile unsigned char*) 0x4806C000;
volatile unsigned char *uart3 = (volatile unsigned char*) 0x49020000;
extern int memset(void *buf, int c, size_t len);
};

// http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html
// http://www.arm.linux.org.uk/developer/booting.php

#define ATAG_NONE       0
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54410005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008
#define ATAG_CMDLINE    0x54410009

struct atag_header {
    uint32_t size;
    uint32_t tag;
};

struct atag_core {
    uint32_t flags;
    uint32_t pagesize;
    uint32_t rootdev;
};

struct atag_mem {
    uint32_t size;
    uint32_t start;
};

struct atag_videotext {
    unsigned char x;
    unsigned char y;
    unsigned short video_page;
    unsigned char video_mode;
    unsigned char video_cols;
    unsigned short video_ega_bx;
    unsigned char video_lines;
    unsigned char video_isvga;
    unsigned short video_points;
};

struct atag_ramdisk {
    uint32_t flags;
    uint32_t size;
    uint32_t start;
};

struct atag_initrd2 {
    uint32_t start;
    uint32_t size;
};

struct atag_serialnr {
    uint32_t low;
    uint32_t high;
};

struct atag_revision {
    uint32_t rev;
};

struct atag_videolfb {
    unsigned short lfb_width;
    unsigned short lfb_height;
    unsigned short lfb_depth;
    unsigned short lfb_linelength;
    uint32_t lfb_base;
    uint32_t lfb_size;
    unsigned char red_size;
    unsigned char red_pos;
    unsigned char green_size;
    unsigned char green_pos;
    unsigned char blue_size;
    unsigned char blue_pos;
    unsigned char rsvd_size;
    unsigned char rsvd_pos;
};

struct atag_cmdline {
    char cmdline[1]; // Minimum size.
};

struct atag {
    struct atag_header hdr;
    union {
        struct atag_core         core;
        struct atag_mem          mem;
        struct atag_videotext    videotext;
        struct atag_ramdisk      ramdisk;
        struct atag_initrd2      initrd2;
        struct atag_serialnr     serialnr;
        struct atag_revision     revision;
        struct atag_videolfb     videolfb;
        struct atag_cmdline      cmdline;
    } u;
};

/// Bootstrap structure passed to the kernel entry point.
struct BootstrapStruct_t
{
    uint32_t flags;

    uint32_t mem_lower;
    uint32_t mem_upper;

    uint32_t boot_device;

    uint32_t cmdline;

    uint32_t mods_count;
    uint32_t mods_addr;

    /* ELF information */
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length;
    uint32_t drives_addr;

    uint32_t config_table;

    uint32_t boot_loader_name;

    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed));

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
            continue;
    
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
        uart_write(uart, tmp-0xA+'a');
    else
        uart_write(uart, tmp+'0');
}

/** GPIO implementation for the BeagleBoard */
class BeagleGpio
{
    public:
        BeagleGpio()
        {}
        ~BeagleGpio()
        {}

        void initialise()
        {
            m_gpio1 = (volatile unsigned int *) 0x48310000;
            initspecific(1, m_gpio1);
            m_gpio2 = (volatile unsigned int *) 0x49050000;
            initspecific(2, m_gpio2);
            m_gpio3 = (volatile unsigned int *) 0x49052000;
            initspecific(3, m_gpio3);
            m_gpio4 = (volatile unsigned int *) 0x49054000;
            initspecific(4, m_gpio4);
            m_gpio5 = (volatile unsigned int *) 0x49056000;
            initspecific(5, m_gpio5);
            m_gpio6 = (volatile unsigned int *) 0x49058000;
            initspecific(6, m_gpio6);
        }

        void clearpin(int pin)
        {
            // Grab the GPIO MMIO range for the pin
            int base = 0;
            volatile unsigned int *gpio = getGpioForPin(pin, &base);
            if(!gpio)
            {
                writeStr(3, "BeagleGpio::drivepin : No GPIO found for pin ");
                writeHex(3, pin);
                writeStr(3, "!\r\n");
                return;
            }

            // Write to the CLEARDATAOUT register
            gpio[0x24] = (1 << base);
        }

        void drivepin(int pin)
        {
            // Grab the GPIO MMIO range for the pin
            int base = 0;
            volatile unsigned int *gpio = getGpioForPin(pin, &base);
            if(!gpio)
            {
                writeStr(3, "BeagleGpio::drivepin : No GPIO found for pin ");
                writeHex(3, pin);
                writeStr(3, "!\r\n");
                return;
            }

            // Write to the SETDATAOUT register. We can set a specific bit in
            // this register without needing to maintain the state of a full
            // 32-bit register (zeroes have no effect).
            gpio[0x25] = (1 << base);
        }

        bool pinstate(int pin)
        {
            // Grab the GPIO MMIO range for the pin
            int base = 0;
            volatile unsigned int *gpio = getGpioForPin(pin, &base);
            if(!gpio)
            {
                writeStr(3, "BeagleGpio::pinstate : No GPIO found for pin ");
                writeHex(3, pin);
                writeStr(3, "!\r\n");
                return false;
            }

            return (gpio[0x25] & (1 << base));
        }

        int capturepin(int pin)
        {
            // Grab the GPIO MMIO range for the pin
            int base = 0;
            volatile unsigned int *gpio = getGpioForPin(pin, &base);
            if(!gpio)
            {
                writeStr(3, "BeagleGpio::capturepin :No GPIO found for pin ");
                writeHex(3, pin);
                writeStr(3, "!\r\n");
                return 0;
            }

            // Read the data from the pin
            return (gpio[0xE] & (1 << base)) >> (base ? base - 1 : 0);
        }

        void enableoutput(int pin)
        {
            // Grab the GPIO MMIO range for the pin
            int base = 0;
            volatile unsigned int *gpio = getGpioForPin(pin, &base);
            if(!gpio)
            {
                writeStr(3, "BeagleGpio::enableoutput :No GPIO found for pin ");
                writeHex(3, pin);
                writeStr(3, "!\r\n");
                return;
            }

            // Set the pin as an output (if it's an input, the bit is set)
            if(gpio[0xD] & (1 << base))
                gpio[0xD] ^= (1 << base);
        }
        
    private:

        /// Initialises a specific GPIO to a given set of defaults
        void initspecific(int n, volatile unsigned int *gpio)
        {
            if(!gpio)
                return;

            // Write information about it
            /// \todo When implementing within Pedigree, we'll have a much nicer
            ///       interface for string manipulation and writing stuff to the
            ///       UART.
            unsigned int rev = gpio[0];
            writeStr(3, "GPIO");
            writeHex(3, n);
            writeStr(3, ": revision ");
            writeHex(3, (rev & 0xF0) >> 4);
            writeStr(3, ".");
            writeHex(3, rev & 0x0F);
            writeStr(3, " - initialising: ");

            // 1. Perform a software reset of the GPIO.
            gpio[0x4] = 2;
            while(!(gpio[0x5] & 1)); // Poll GPIO_SYSSTATUS, bit 0

            // 2. Disable all IRQs
            gpio[0x7] = 0; // GPIO_IRQENABLE1
            gpio[0xB] = 0; // GPIO_IRQENABLE2

            // 3. Enable the module
            gpio[0xC] = 0;

            // Completed the reset and initialisation.
            writeStr(3, "Done.\r\n");
        }

        /// Gets the correct GPIO MMIO range for a given GPIO pin. The base
        /// indicates which bit represents this pin in registers, where relevant
        volatile unsigned int *getGpioForPin(int pin, int *bit)
        {
            volatile unsigned int *gpio = 0;
            if(pin < 32)
            {
                *bit = pin;
                gpio = m_gpio1;
            }
            else if((pin >= 34) && (pin < 64))
            {
                *bit = pin - 34;
                gpio = m_gpio2;
            }
            else if((pin >= 64) && (pin < 96))
            {
                *bit = pin - 64;
                gpio = m_gpio3;
            }
            else if((pin >= 96) && (pin < 128))
            {
                *bit = pin - 96;
                gpio = m_gpio4;
            }
            else if((pin >= 128) && (pin < 160))
            {
                *bit = pin - 128;
                gpio = m_gpio5;
            }
            else if((pin >= 160) && (pin < 192))
            {
                *bit = pin - 160;
                gpio = m_gpio6;
            }
            else
                gpio = 0;
            return gpio;
        }
        
        volatile unsigned int *m_gpio1;
        volatile unsigned int *m_gpio2;
        volatile unsigned int *m_gpio3;
        volatile unsigned int *m_gpio4;
        volatile unsigned int *m_gpio5;
        volatile unsigned int *m_gpio6;
};

/// First level descriptor - roughly equivalent to a page directory entry
/// on x86
struct FirstLevelDescriptor
{
    /// Type field for descriptors
    /// 0 = fault
    /// 1 = page table
    /// 2 = section or supersection
    /// 3 = reserved
    
    union {
        struct {
            uint32_t type : 2;
            uint32_t ignore : 30;
        } PACKED fault;
        struct {
            uint32_t type : 2;
            uint32_t sbz1 : 1;
            uint32_t ns : 1;
            uint32_t sbz2 : 1;
            uint32_t domain : 4;
            uint32_t imp : 1;
            uint32_t baseaddr : 22;
        } PACKED pageTable;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t xn : 1;
            uint32_t domain : 4; /// extended base address for supersection
            uint32_t imp : 1;
            uint32_t ap1 : 2;
            uint32_t tex : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t sectiontype : 1; /// = 0 for section, 1 for supersection
            uint32_t ns : 1;
            uint32_t base : 12;
        } PACKED section;
        
        uint32_t entry;
    } descriptor;
} PACKED;

/// Second level descriptor - roughly equivalent to a page table entry
/// on x86
struct SecondLevelDescriptor
{
    /// Type field for descriptors
    /// 0 = fault
    /// 1 = large page
    /// >2 = small page (NX at bit 0)
    
    union
    {
        struct {
            uint32_t type : 2;
            uint32_t ignore : 30;
        } PACKED fault;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t ap1 : 2;
            uint32_t sbz : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t tex : 3;
            uint32_t xn : 1;
            uint32_t base : 16;
        } PACKED largepage;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t ap1 : 2;
            uint32_t sbz : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t base : 20;
        } PACKED smallpage;
        
        uint32_t entry;
    } descriptor;
} PACKED;

extern "C" void __start(uint32_t r0, uint32_t machineType, struct atag *tagList)
{
    BeagleGpio gpio;
    
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

    writeStr(3, "Pedigree for the BeagleBoard\r\n\r\n");

    gpio.initialise();

    gpio.enableoutput(149);
    gpio.enableoutput(150);

    gpio.drivepin(149); // Switch on the USR1 LED to show we're active and thinking
    gpio.clearpin(150);

    writeStr(3, "\r\nPlease press the USER button on the board to continue.\r\n");

    while(!gpio.capturepin(7));

    writeStr(3, "USER button pressed, continuing...\r\n\r\n");

    writeStr(3, "Press 1 to toggle the USR0 LED, and 2 to toggle the USR1 LED.\r\nPress 0 to clear both LEDs. Hit ENTER to boot the kernel.\r\n");
    while(1)
    {
        char c = uart_read(3);
        if(c == '1')
        {
            writeStr(3, "Toggling USR0 LED\r\n");
            if(gpio.pinstate(150))
                gpio.clearpin(150);
            else
                gpio.drivepin(150);
        }
        else if(c == '2')
        {
            writeStr(3, "Toggling USR1 LED\r\n");
            if(gpio.pinstate(149))
                gpio.clearpin(149);
            else
                gpio.drivepin(149);
        }
        else if(c == '0')
        {
            writeStr(3, "Clearing both USR0 and USR1 LEDs\r\n");
            gpio.clearpin(149);
            gpio.clearpin(150);
        }
        else if((c == 13) || (c == 10))
            break;
    }

#if 0 // Set to 1 to enable the MMU test instead of loading the kernel.
    writeStr(3, "\r\n\r\nVirtual memory test starting...\r\n");

    FirstLevelDescriptor *pdir = (FirstLevelDescriptor*) 0x80100000;
    memset(pdir, 0, 0x4000);

    uint32_t base1 = 0x80000000; // Currently running code
    uint32_t base2 = 0x49000000; // UART3

    // First section covers the current code, identity mapped.
    uint32_t pdir_offset = base1 >> 20;
    pdir[pdir_offset].descriptor.entry = base1;
    pdir[pdir_offset].descriptor.section.type = 2;
    pdir[pdir_offset].descriptor.section.b = 0;
    pdir[pdir_offset].descriptor.section.c = 0;
    pdir[pdir_offset].descriptor.section.xn = 0;
    pdir[pdir_offset].descriptor.section.domain = 0;
    pdir[pdir_offset].descriptor.section.imp = 0;
    pdir[pdir_offset].descriptor.section.ap1 = 3;
    pdir[pdir_offset].descriptor.section.ap2 = 0;
    pdir[pdir_offset].descriptor.section.tex = 0;
    pdir[pdir_offset].descriptor.section.s = 1;
    pdir[pdir_offset].descriptor.section.nG = 0;
    pdir[pdir_offset].descriptor.section.sectiontype = 0;
    pdir[pdir_offset].descriptor.section.ns = 0;

    // Second section covers the UART, identity mapped.
    pdir_offset = base2 >> 20;
    pdir[pdir_offset].descriptor.entry = base2;
    pdir[pdir_offset].descriptor.section.type = 2;
    pdir[pdir_offset].descriptor.section.b = 0;
    pdir[pdir_offset].descriptor.section.c = 0;
    pdir[pdir_offset].descriptor.section.xn = 0;
    pdir[pdir_offset].descriptor.section.domain = 0;
    pdir[pdir_offset].descriptor.section.imp = 0;
    pdir[pdir_offset].descriptor.section.ap1 = 3;
    pdir[pdir_offset].descriptor.section.ap2 = 0;
    pdir[pdir_offset].descriptor.section.tex = 0;
    pdir[pdir_offset].descriptor.section.s = 1;
    pdir[pdir_offset].descriptor.section.nG = 0;
    pdir[pdir_offset].descriptor.section.sectiontype = 0;
    pdir[pdir_offset].descriptor.section.ns = 0;

    writeStr(3, "Writing to TTBR0 and enabling access to domain 0...\r\n");

    asm volatile("MCR p15,0,%0,c2,c0,0" : : "r" (0x80100000));
    asm volatile("MCR p15,0,%0,c3,c0,0" : : "r" (0xFFFFFFFF)); // Manager access to all domains for now

    // Enable the MMU
    uint32_t sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    if(!(sctlr & 1))
        sctlr |= 1;
    else
        writeStr(3, "It seems the MMU is already enabled?\r\n");

    writeStr(3, "Enabling the MMU...\r\n");
    asm volatile("MCR p15,0,%0,c1,c0,0" : : "r" (sctlr));

    // If you can see the string, the identity map is complete and working.
    writeStr(3, "\r\n\r\nTest completed without errors.\r\n");

    while(1)
    {
        asm volatile("wfi");
    }

#else

    writeStr(3, "\r\n\r\nPlease wait while the kernel is loaded...\r\n");

    Elf32 elf("kernel");
    writeStr(3, "Preparing file... ");
    elf.load((uint8_t*)file, 0);
    writeStr(3, "Done!\r\n");

    writeStr(3, "Loading file into memory (please wait) ");
    elf.writeSections();
    writeStr(3, " Done!\r\n");
    int (*main)(struct BootstrapStruct_t*) = (int (*)(struct BootstrapStruct_t*)) elf.getEntryPoint();

    struct BootstrapStruct_t *bs = reinterpret_cast<struct BootstrapStruct_t *>(0x80008000);
    writeStr(3, "Creating bootstrap information structure... ");
    memset(bs, 0, sizeof(bs));
    bs->shndx = elf.m_pHeader->shstrndx;
    bs->num = elf.m_pHeader->shnum;
    bs->size = elf.m_pHeader->shentsize;
    bs->addr = (unsigned int)elf.m_pSectionHeaders;

    // Repurpose these variables a little....
    bs->mods_addr = reinterpret_cast<uint32_t>(elf.m_pBuffer);
    bs->mods_count = (sizeof file) + 0x1000;

    // For every section header, set .addr = .offset + m_pBuffer.
    for (int i = 0; i < elf.m_pHeader->shnum; i++)
    {
        elf.m_pSectionHeaders[i].addr = elf.m_pSectionHeaders[i].offset + (uint32_t)elf.m_pBuffer;
    }
    writeStr(3, "Done!\r\n");

    // Just before running the kernel proper, turn off both LEDs so we can use
    // their states for debugging the kernel.
    gpio.clearpin(149);
    gpio.clearpin(150);

    // Run the kernel, finally
    writeStr(3, "Now starting the Pedigree kernel (can take a while, please wait).\r\n\r\n");
    main(bs);
#endif
    
    while (1)
    {
        asm volatile("wfi");
    }
}

