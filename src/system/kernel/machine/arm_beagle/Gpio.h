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
#ifndef _GPIO_H
#define _GPIO_H

#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/MemoryRegion.h>
#include <Log.h>

/** GPIO implementation for the BeagleBoard */
class Gpio
{
    public:
        Gpio() : m_Gpio1("GPIO1"), m_Gpio2("GPIO2"), m_Gpio3("GPIO3"),
                 m_Gpio4("GPIO4"), m_Gpio5("GPIO5"), m_Gpio6("GPIO6")
        {}
        ~Gpio()
        {}

        static Gpio &instance()
        {
            return m_Instance;
        }

        void initialise(uintptr_t gpio1, uintptr_t gpio2, uintptr_t gpio3, uintptr_t gpio4, uintptr_t gpio5, uintptr_t gpio6)
        {
            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio1,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio1))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(1, reinterpret_cast<volatile uint32_t*>(m_Gpio1.virtualAddress()));

            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio2,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio2))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(2, reinterpret_cast<volatile uint32_t*>(m_Gpio2.virtualAddress()));

            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio3,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio3))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(3, reinterpret_cast<volatile uint32_t*>(m_Gpio3.virtualAddress()));

            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio4,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio4))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(4, reinterpret_cast<volatile uint32_t*>(m_Gpio4.virtualAddress()));

            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio5,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio5))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(5, reinterpret_cast<volatile uint32_t*>(m_Gpio5.virtualAddress()));

            if(!PhysicalMemoryManager::instance().allocateRegion(
                        m_Gpio6,
                        1,
                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                        gpio6))
            {
                ERROR("GPIO: Couldn't get a memory region!");
                return;
            }
            initspecific(6, reinterpret_cast<volatile uint32_t*>(m_Gpio6.virtualAddress()));
        }

        void clearpin(int pin);

        void drivepin(int pin);

        bool pinstate(int pin);

        int capturepin(int pin);

        void enableoutput(int pin);
        
    private:

        /// Initialises a specific GPIO to a given set of defaults
        void initspecific(int n, volatile uint32_t *gpio);

        /// Gets the correct GPIO MMIO range for a given GPIO pin. The base
        /// indicates which bit represents this pin in registers, where relevant
        volatile uint32_t *getGpioForPin(int pin, int *bit);

        MemoryRegion m_Gpio1;
        MemoryRegion m_Gpio2;
        MemoryRegion m_Gpio3;
        MemoryRegion m_Gpio4;
        MemoryRegion m_Gpio5;
        MemoryRegion m_Gpio6;

        static Gpio m_Instance;
};


#endif
