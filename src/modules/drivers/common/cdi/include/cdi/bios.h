/*
 * Copyright (c) 2008 Mathias Gottschlag
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_BIOS_H_
#define _CDI_BIOS_H_

#include <stdint.h>

#include "cdi/lists.h"

struct cdi_bios_registers {
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t ds;
    uint16_t es;
};

/**
 * Struktur zum Zugriff auf Speicherbereiche des 16bit-Prozesses
 */
struct cdi_bios_memory {
    /**
     * Virtuelle Addresse im Speicher des 16bit-Prozesses. Muss unterhalb von
     * 0xC0000 liegen.
     */
    uintptr_t dest;
    /**
     * Pointer auf reservierten Speicher für die Daten des Speicherbereichs. Wird
     * beim Start des Tasks zum Initialisieren des Bereichs benutzt und erhaelt
     * auch wieder die Daten nach dem BIOS-Aufruf.
     */
    void *src;
    /**
     * Laenge des Speicherbereichs
     */
    uint16_t size;
};

/**
 * Ruft den BIOS-Interrupt 0x10 auf.
 * @param registers Pointer auf eine Register-Struktur. Diese wird beim Aufruf
 * in die Register des Tasks geladen und bei Beendigung des Tasks wieder mit den
 * Werten des Tasks gefuellt.
 * @param memory Speicherbereiche, die in den Bios-Task kopiert und bei Beendigung
 * des Tasks wieder zurueck kopiert werden sollen. Die Liste ist vom Typ struct
 * cdi_bios_memory.
 * @return 0, wenn der Aufruf erfolgreich war, -1 bei Fehlern
 */
int cdi_bios_int10(struct cdi_bios_registers *registers, cdi_list_t memory);

#endif

