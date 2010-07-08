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
#include "Gpio.h"
#include <Log.h>

Gpio Gpio::m_Instance;

void Gpio::clearpin(int pin)
{
    // Grab the GPIO MMIO range for the pin
    int base = 0;
    volatile uint32_t *gpio = getGpioForPin(pin, &base);
    if(!gpio)
    {
        NOTICE("GPIO::clearpin - no GPIO found for pin " << Dec << pin << Hex << ".");
        return;
    }

    // Write to the CLEARDATAOUT register
    gpio[0x24] = (1 << base);
}

void Gpio::drivepin(int pin)
{
    // Grab the GPIO MMIO range for the pin
    int base = 0;
    volatile uint32_t *gpio = getGpioForPin(pin, &base);
    if(!gpio)
    {
        NOTICE("GPIO::drivepin - no GPIO found for pin " << Dec << pin << Hex << ".");
        return;
    }

    // Write to the SETDATAOUT register. We can set a specific bit in
    // this register without needing to maintain the state of a full
    // 32-bit register (zeroes have no effect).
    gpio[0x25] = (1 << base);
}

bool Gpio::pinstate(int pin)
{
    // Grab the GPIO MMIO range for the pin
    int base = 0;
    volatile uint32_t *gpio = getGpioForPin(pin, &base);
    if(!gpio)
    {
        NOTICE("GPIO::pinstate - no GPIO found for pin " << Dec << pin << Hex << ".");
        return false;
    }

    return (gpio[0x25] & (1 << base));
}

int Gpio::capturepin(int pin)
{
    // Grab the GPIO MMIO range for the pin
    int base = 0;
    volatile uint32_t *gpio = getGpioForPin(pin, &base);
    if(!gpio)
    {
        NOTICE("GPIO::capturepin - no GPIO found for pin " << Dec << pin << Hex << ".");
        return 0;
    }

    // Read the data from the pin
    return (gpio[0xE] & (1 << base)) >> (base ? base - 1 : 0);
}

void Gpio::enableoutput(int pin)
{
    // Grab the GPIO MMIO range for the pin
    int base = 0;
    volatile uint32_t *gpio = getGpioForPin(pin, &base);
    if(!gpio)
    {
        NOTICE("GPIO::enableoutput - no GPIO found for pin " << Dec << pin << Hex << ".");
        return;
    }

    // Set the pin as an output (if it's an input, the bit is set)
    if(gpio[0xD] & (1 << base))
        gpio[0xD] ^= (1 << base);
}

void Gpio::initspecific(int n, volatile uint32_t *gpio)
{
    if(!gpio)
        return;

    // Write information about it
    unsigned int rev = gpio[0];
    NOTICE("GPIO" << Dec << n << Hex << " at " << reinterpret_cast<uintptr_t>(gpio) << ": revision " << Dec << ((rev & 0xF0) >> 4) << "." << (rev & 0xF) << Hex << ".");

    // 1. Perform a software reset of the GPIO.
    gpio[0x4] = 2;
    while(!(gpio[0x5] & 1)); // Poll GPIO_SYSSTATUS, bit 0

    // 2. Disable all IRQs
    gpio[0x7] = 0; // GPIO_IRQENABLE1
    gpio[0xB] = 0; // GPIO_IRQENABLE2

    // 3. Enable the module
    gpio[0xC] = 0;
}

volatile uint32_t *Gpio::getGpioForPin(int pin, int *bit)
{
    volatile uint32_t *gpio = 0;
    if(pin < 32)
    {
        *bit = pin;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio1.virtualAddress());
    }
    else if((pin >= 34) && (pin < 64))
    {
        *bit = pin - 34;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio2.virtualAddress());
    }
    else if((pin >= 64) && (pin < 96))
    {
        *bit = pin - 64;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio3.virtualAddress());
    }
    else if((pin >= 96) && (pin < 128))
    {
        *bit = pin - 96;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio4.virtualAddress());
    }
    else if((pin >= 128) && (pin < 160))
    {
        *bit = pin - 128;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio5.virtualAddress());
    }
    else if((pin >= 160) && (pin < 192))
    {
        *bit = pin - 160;
        gpio = reinterpret_cast<volatile uint32_t*>(m_Gpio6.virtualAddress());
    }
    else
        gpio = 0;
    return gpio;
}
