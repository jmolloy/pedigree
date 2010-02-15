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
#include "cdi/mem.h"

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
 * \german
 * Reserviert einen Speicherbereich.
 *
 * @param size Größe des Speicherbereichs in Bytes
 * @param flags Flags, die zusätzliche Anforderungen beschreiben
 *
 * @return Eine cdi_mem_area bei Erfolg, NULL im Fehlerfall
 * \endgerman
 * \english
 * Allocates a memory area.
 *
 * @param size Size of the memory area in bytes
 * @param flags Flags that describe additional requirements
 *
 * @return A cdi_mem_area on success, NULL on failure
 * \endenglish
 */
struct cdi_mem_area* cdi_mem_alloc(size_t size, cdi_mem_flags_t flags)
{
    /// \todo Incorrectly assumes many flags won't be set

    MemoryRegion* region = new MemoryRegion("cdi");
    size_t pageSize = PhysicalMemoryManager::getPageSize();

    if(!PhysicalMemoryManager::instance().allocateRegion(
        *region, (size + (pageSize - 1)) / pageSize,
        PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        WARNING("cdi: cdi_mem_alloc: couldn't allocate memory");
        delete region;
        return 0;
    }

    struct cdi_mem_area *ret = new struct cdi_mem_area;
    ret->size = size;
    ret->vaddr = region->virtualAddress();
    ret->paddr.num = 1;
    ret->paddr.items = new struct cdi_mem_sg_item;
    ret->paddr.items->start = region->physicalAddress();
    ret->paddr.items->size = size;
    ret->osdep.pMemRegion = reinterpret_cast<void*>(region);
    return ret;
}

/**
 * \german
 * Reserviert physisch zusammenhägenden Speicher an einer definierten Adresse
 *
 * @param paddr Physische Adresse des angeforderten Speicherbereichs
 * @param size Größe des benötigten Speichers in Bytes
 *
 * @return Eine cdi_mem_area bei Erfolg, NULL im Fehlerfall
 * \endgerman
 * \english
 * Reserves physically contiguous memory at a defined address
 *
 * @param paddr Physical address of the requested memory aread
 * @param size Size of the requested memory area in bytes
 *
 * @return A cdi_mem_area on success, NULL on failure
 * \endenglish
 */
struct cdi_mem_area* cdi_mem_map(uintptr_t paddr, size_t size)
{
    MemoryRegion* region = new MemoryRegion("cdi");
    size_t pageSize = PhysicalMemoryManager::getPageSize();

    if(!PhysicalMemoryManager::instance().allocateRegion(
        *region, (size + (pageSize - 1)) / pageSize,
        PhysicalMemoryManager::continuous, VirtualAddressSpace::Write,
        paddr))
    {
        WARNING("cdi: cdi_mem_map: couldn't allocate memory");
        delete region;
        return 0;
    }

    struct cdi_mem_area *ret = new struct cdi_mem_area;
    ret->size = size;
    ret->vaddr = region->virtualAddress();
    ret->paddr.num = 1;
    ret->paddr.items = new struct cdi_mem_sg_item;
    ret->paddr.items->start = region->physicalAddress();
    ret->paddr.items->size = size;
    ret->osdep.pMemRegion = reinterpret_cast<void*>(region);
    return ret;
}

/**
 * \german
 * Gibt einen durch cdi_mem_alloc oder cdi_mem_map reservierten Speicherbereich
 * frei
 * \endgerman
 * \english
 * Frees a memory area that was previously allocated by cdi_mem_alloc or
 * cdi_mem_map
 * \endenglish
 */
void cdi_mem_free(struct cdi_mem_area* p)
{
    if(p)
    {
        MemoryRegion *mem = reinterpret_cast<MemoryRegion *>(p->osdep.pMemRegion);
        delete mem;
        delete p;
    }
}

/**
 * \german
 * Gibt einen Speicherbereich zurück, der dieselben Daten wie @a p beschreibt,
 * aber mindestens die gegebenen Flags gesetzt hat.
 *
 * Diese Funktion kann denselben virtuellen und physischen Speicherbereich wie
 * @p benutzen oder sogar @p selbst zurückzugeben, solange der gemeinsam
 * benutzte Speicher erst dann freigegeben wird, wenn sowohl @a p als auch der
 * Rückgabewert durch cdi_mem_free freigegeben worden sind.
 *
 * Ansonsten wird ein neuer Speicherbereich reserviert und (außer wenn das
 * Flag CDI_MEM_NOINIT gesetzt ist) die Daten werden aus @a p in den neu
 * reservierten Speicher kopiert.
 * \endgerman
 * \english
 * Returns a memory area that describes the same data as @a p does, but for
 * which at least the given flags are set.
 *
 * This function may use the same virtual and physical memory areas as used in
 * @a p, or it may even return @a p itself. In this case it must ensure that
 * the commonly used memory is only freed when both @a p and the return value
 * of this function have been freed by cdi_mem_free.
 *
 * Otherwise, a new memory area is allocated and data is copied from @a p into
 * the newly allocated memory (unless CDI_MEM_NOINIT is set).
 * \endenglish
 */
struct cdi_mem_area* cdi_mem_require_flags(struct cdi_mem_area* p,
    cdi_mem_flags_t flags)
{
    // Pretend the memory area matches the given flags
    /// \todo Fundamentally wrong
    return p;
}

/**
 * \german
 * Kopiert die Daten von @a src nach @a dest. Beide Speicherbereiche müssen
 * gleich groß sein.
 *
 * Das bedeutet nicht unbedingt eine physische Kopie: Wenn beide
 * Speicherbereiche auf denselben physischen Speicher zeigen, macht diese
 * Funktion nichts. Sie kann auch andere Methoden verwenden, um den Speicher
 * effektiv zu kopieren, z.B. durch geeignetes Ummappen von Pages.
 *
 * @return 0 bei Erfolg, -1 sonst
 * \endgerman
 * \english
 * Copies data from @a src to @a dest. Both memory areas must be of the same
 * size.
 *
 * This does not necessarily involve a physical copy: If both memory areas
 * point to the same physical memory, this function does nothing. It can also
 * use other methods to achieve the same visible effect, e.g. by remapping
 * pages.
 *
 * @return 0 on success, -1 otherwise
 * \endenglish
 */
int cdi_mem_copy(struct cdi_mem_area* dest, struct cdi_mem_area* src)
{
    if(dest && src && dest->size == src->size)
    {
        if(dest->vaddr != src->vaddr)
            memcpy(dest->vaddr, src->vaddr, src->size);
        return 0;
    }
    else
        return -1;
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
