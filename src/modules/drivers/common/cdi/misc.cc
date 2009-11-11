/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <Log.h>
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>
#include <machine/IrqHandler.h>

#include <utilities/TimeoutGuard.h>
#include <process/Semaphore.h>
#include <process/Mutex.h>
#include <LockGuard.h>

class CdiIrqHandler : public IrqHandler {
        virtual bool irq(irq_id_t number, InterruptState &state);
};

static CdiIrqHandler cdi_irq_handler;

#include "cdi.h"
#include "cdi/misc.h"
#include "cdi/cmos.h"
#include "cdi/io.h"

/** Anzahl der verfuegbaren IRQs */
#define IRQ_COUNT           0x10

/** Array mit allen IRQ-Handlern; Als index wird die Nummer benutzt */
static void (*driver_irq_handler[IRQ_COUNT])(struct cdi_device*) = { NULL };
/** Array mit den passenden Geraeten zu den registrierten IRQs */
static struct cdi_device* driver_irq_device[IRQ_COUNT] = { NULL };
/**
 * Array, das die jeweilige Anzahl an aufgerufenen Interrupts seit dem
 * cdi_reset_wait_irq speichert.
 */
static Spinlock irqCountLock;
static Semaphore *driver_irq_count[IRQ_COUNT] = {0};
// static volatile uint8_t driver_irq_count[IRQ_COUNT] = { 0 };

/**
 * Interner IRQ-Handler, der den IRQ-Handler des Treibers aufruft
 */
bool CdiIrqHandler::irq(irq_id_t irq, InterruptState &state)
{
    if(driver_irq_count[irq])
    {
        irqCountLock.acquire();
        driver_irq_count[irq]->release();
        irqCountLock.release();
    }

    if (driver_irq_handler[irq]) {
        driver_irq_handler[irq](driver_irq_device[irq]);
    }

    return true;
}

extern "C" {

/**
 * Registiert einen neuen IRQ-Handler.
 *
 * @param irq Nummer des zu reservierenden IRQ
 * @param handler Handlerfunktion
 * @param device Geraet, das dem Handler als Parameter uebergeben werden soll
 */
void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*),
    struct cdi_device* device)
{
    if (irq >= IRQ_COUNT) {
        // FIXME: Eigentlich sollte diese Funktion etwas weniger optimistisch
        // sein, und einen Rueckgabewert haben.
        return;
    }

    // Der Interrupt wurde schon mal registriert
    if (driver_irq_handler[irq]) {
        NOTICE("cdi: Versuch IRQ " << irq << " mehrfach zu registrieren");
        return;
    }

    if(driver_irq_count[irq])
        delete driver_irq_count[irq];
    driver_irq_count[irq] = new Semaphore(0);

    driver_irq_handler[irq] = handler;
    driver_irq_device[irq] = device;

    Machine::instance().getIrqManager()->registerIsaIrqHandler(irq, static_cast<IrqHandler*>(&cdi_irq_handler));
}

/**
 * Setzt den IRQ-Zaehler fuer cdi_wait_irq zurueck.
 *
 * @param irq Nummer des IRQ
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_reset_wait_irq(uint8_t irq)
{
    if (irq > IRQ_COUNT) {
        return -1;
    }

    if(driver_irq_count[irq])
    {
        irqCountLock.acquire();
        while(driver_irq_count[irq]->tryAcquire())
            driver_irq_count[irq]->release();
        irqCountLock.release();
        return 0;
    }

    return -1;
}

// Dummy-Callback fuer den timer_register-Aufruf in cdi_wait_irq
//static void wait_irq_dummy_callback(void) { }

/**
 * Wartet bis der IRQ aufgerufen wurde. Der interne Zaehler muss zuerst mit
 * cdi_reset_wait_irq zurueckgesetzt werden, damit auch die IRQs abgefangen
 * werden koennen, die kurz vor dem Aufruf von dieser Funktion aufgerufen
 * werden.
 *
 * @param irq       Nummer des IRQ auf den gewartet werden soll
 * @param timeout   Anzahl der Millisekunden, die maximal gewartet werden sollen
 *
 * @return 0 wenn der irq aufgerufen wurde, -1 wenn eine ungueltige IRQ-Nummer
 * angegeben wurde, -2 wenn eine nicht registrierte IRQ-Nummer angegeben wurde,
 * und -3 im Falle eines Timeouts.
 */
int cdi_wait_irq(uint8_t irq, uint32_t timeout)
{
    if (irq > IRQ_COUNT) {
        return -1;
    }

    if (!driver_irq_handler[irq]) {
        return -2;
    }

    Semaphore *pSem;

    {
        LockGuard<Spinlock> lock(irqCountLock);

        pSem = driver_irq_count[irq];
        if(!pSem)
        {
            pSem = new Semaphore(0);
            driver_irq_count[irq] = pSem;
        }
    }

    if(pSem)
    {
        pSem->acquire(1, 0, timeout * 1000);
        if(Processor::information().getCurrentThread()->wasInterrupted())
            return -3;
        else
            return 0;
    }
    else
        return -2;
}
}


/**
 * Reserviert physisch zusammenhaengenden Speicher.
 *
 * @param size Groesse des benoetigten Speichers in Bytes
 * @param vaddr Pointer, in den die virtuelle Adresse des reservierten
 * Speichers geschrieben wird.
 * @param paddr Pointer, in den die physische Adresse des reservierten
 * Speichers geschrieben wird.
 *
 * @return 0 wenn der Speicher erfolgreich reserviert wurde, -1 sonst
 */
int cdi_alloc_phys_mem(size_t size, void** vaddr, void** paddr)
{
    MemoryRegion* region = new MemoryRegion("cdi");
    size_t page_size = PhysicalMemoryManager::instance().getPageSize();

    if (!PhysicalMemoryManager::instance().allocateRegion(
        *region, (size + page_size - 1) / page_size,
        PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        WARNING("cdi: Couldn't allocate physical memory!");
        return -1;
    }

    *vaddr = region->virtualAddress();
    *paddr = reinterpret_cast<void*>(region->physicalAddress());

    return 0;
}

/**
 * Reserviert physisch zusammenhaengenden Speicher an einer definierten Adresse.
 *
 * @param size Groesse des benoetigten Speichers in Bytes
 * @param paddr Physikalische Adresse des angeforderten Speicherbereichs
 *
 * @return Virtuelle Adresse, wenn Speicher reserviert wurde, sonst 0
 */
void* cdi_alloc_phys_addr(size_t size, uintptr_t paddr)
{
    MemoryRegion* region = new MemoryRegion("cdi");
    size_t page_size = PhysicalMemoryManager::instance().getPageSize();
    void* vaddr;

    if (!PhysicalMemoryManager::instance().allocateRegion(
        *region, (size + page_size - 1) / page_size,
        PhysicalMemoryManager::continuous, VirtualAddressSpace::Write,
        paddr))
    {
        WARNING("cdi: Couldn't allocate physical address!");
        return NULL;
    }

    vaddr = region->virtualAddress();

    return vaddr;
}

/**
 * Reserviert IO-Ports
 *
 * @return 0 wenn die Ports erfolgreich reserviert wurden, -1 sonst.
 */
int cdi_ioports_alloc(uint16_t start, uint16_t count)
{
    // Not required in Pedigree drivers (ring0)
    return 0;
}

/**
 * Gibt reservierte IO-Ports frei
 *
 * @return 0 wenn die Ports erfolgreich freigegeben wurden, -1 sonst.
 */
int cdi_ioports_free(uint16_t start, uint16_t count)
{
    // Not required in Pedigree drivers (ring0)
    return 0;
}

/**
 * Unterbricht die Ausfuehrung fuer mehrere Millisekunden
 */
void cdi_sleep_ms(uint32_t ms)
{
    Semaphore sem(0);
    sem.acquire(1, 0, ms * 1000);
}

uint8_t cdi_cmos_read(uint8_t index)
{
    cdi_outb(0x70, index);
    return cdi_inb(0x71);
}

void cdi_cmos_write(uint8_t index, uint8_t value)
{
    cdi_outb(0x70, index);
    cdi_outb(0x71, value);
}
