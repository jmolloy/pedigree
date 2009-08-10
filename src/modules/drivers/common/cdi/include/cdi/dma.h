/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_DMA_H_
#define _CDI_DMA_H_

#include <stdint.h>
#include <stdio.h>
#include "cdi.h"

struct cdi_dma_handle {
    uint8_t     channel;
    size_t      length;
    uint8_t     mode;
    void*       buffer;

    // LOST-Implementation...
    FILE*       file;
};

// Geraet => Speicher
#define CDI_DMA_MODE_READ           (1 << 2)
// Speicher => Geraet
#define CDI_DMA_MODE_WRITE          (2 << 2)
#define CDI_DMA_MODE_ON_DEMAND      (0 << 6)
#define CDI_DMA_MODE_SINGLE         (1 << 6)
#define CDI_DMA_MODE_BLOCK          (2 << 6)


/**
 * Initialisiert einen Transport per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_open(struct cdi_dma_handle* handle, uint8_t channel, uint8_t mode,
    size_t length, void* buffer);

/**
 * Liest Daten per DMA ein
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_read(struct cdi_dma_handle* handle);

/**
 * Schreibt Daten per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_write(struct cdi_dma_handle* handle);

/**
 * Schliesst das DMA-Handle wieder
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_close(struct cdi_dma_handle* handle);

#endif

