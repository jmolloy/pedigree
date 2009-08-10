/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

/**
 * Treiber fuer Massenspeichergeraete
 * \defgroup storage
 */
/*\@{*/

#ifndef _CDI_STORAGE_H_
#define _CDI_STORAGE_H_

#include <stdint.h>

#include "cdi.h"

struct cdi_storage_device {
    struct cdi_device   dev;
    size_t              block_size;
    uint64_t            block_count;
};

struct cdi_storage_driver {
    struct cdi_driver   drv;
    
    /**
     * Liest Blocks ein
     *
     * @param start Blocknummer des ersten zu lesenden Blockes (angefangen
     *              bei 0).
     * @param count Anzahl der zu lesenden Blocks
     * @param buffer Puffer in dem die Daten abgelegt werden sollen
     *
     * @return 0 bei Erfolg, -1 im Fehlerfall.
     */
    int (*read_blocks)(struct cdi_storage_device* device, uint64_t start,
        uint64_t count, void* buffer);
    
    /**
     * Schreibt Blocks
     *
     * @param start Blocknummer des ersten zu schreibenden Blockes (angefangen
     *              bei 0).
     * @param count Anzahl der zu schreibenden Blocks
     * @param buffer Puffer aus dem die Daten gelesen werden sollen
     *
     * @return 0 bei Erfolg, -1 im Fehlerfall
     */
    int (*write_blocks)(struct cdi_storage_device* device, uint64_t start,
        uint64_t count, void* buffer);
};


/**
 * Initialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 * (erzeugt die devices-Liste)
 */
void cdi_storage_driver_init(struct cdi_storage_driver* driver);

/**
 * Deinitialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 * (gibt die devices-Liste frei)
 */
void cdi_storage_driver_destroy(struct cdi_storage_driver* driver);

/**
 * Registiert einen Massenspeichertreiber
 */
void cdi_storage_driver_register(struct cdi_storage_driver* driver);

/**
 * Initialisiert einen Massenspeicher
 */
void cdi_storage_device_init(struct cdi_storage_device* device);


#endif

/*\@}*/

