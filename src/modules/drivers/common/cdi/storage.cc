/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <processor/Processor.h>
#include <processor/types.h>
#include <Log.h>

#include <stdio.h>
#include <stdlib.h>

#include "cdi/storage.h"

/**
 * External prototype for this function so that we don't pollute the CDI header
 */
extern "C" {
void cdi_cpp_disk_register(void* void_pdev, struct cdi_storage_device* device);

int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos, size_t size, void* dest);
int cdi_storage_write(struct cdi_storage_device* device, uint64_t pos, size_t size, void* src);
};

/**
 * Initialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 */
void cdi_storage_driver_init(struct cdi_storage_driver* driver)
{
    cdi_driver_init(reinterpret_cast<struct cdi_driver*>(driver));
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 */
void cdi_storage_driver_destroy(struct cdi_storage_driver* driver)
{
    cdi_driver_destroy(reinterpret_cast<struct cdi_driver*>(driver));
}

/**
 * Registiert einen Massenspeichertreiber
 */
void cdi_storage_driver_register(struct cdi_storage_driver* driver)
{
    static int initialized = 0;

    cdi_driver_register(reinterpret_cast<struct cdi_driver*>(driver));
}

/**
 * Initialisiert einen Massenspeicher
 */
void cdi_storage_device_init(struct cdi_storage_device* device)
{
    cdi_cpp_disk_register(device->dev.backdev, device);
}

/**
 * Liest Daten von einem Massenspeichergeraet
 *
 * @param pos Startposition in Bytes
 * @param size Anzahl der zu lesenden Bytes
 * @param dest Buffer
 */
int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos, size_t size, void* dest)
{
    struct cdi_storage_driver* driver = reinterpret_cast<struct cdi_storage_driver*>(device->dev.driver);
    size_t block_size = device->block_size;
    // Start- und Endblock
    uint64_t block_read_start = pos / block_size;
    uint64_t block_read_end = (pos + size) / block_size;
    // Anzahl der Blocks die gelesen werden sollen
    uint64_t block_read_count = block_read_end - block_read_start;
    
    // Wenn nur ganze Bloecke gelesen werden sollen, geht das etwas effizienter
    if (((pos % block_size) == 0) && (((pos + size) %  block_size) == 0)) {
        // Nur ganze Bloecke
        return driver->read_blocks(device, block_read_start, block_read_count, dest);
    } else {
        // FIXME: Das laesst sich garantiert etwas effizienter loesen
        block_read_count++;
        uint8_t buffer[block_read_count * block_size];
        
        // In Zwischenspeicher einlesen
        if (driver->read_blocks(device, block_read_start, block_read_count, buffer) != 0)
        {
            return -1;
        }
        
        // Bereich aus dem Zwischenspeicher kopieren
        memcpy(dest, buffer + (pos % block_size), size);
    }
    return 0;
}

/**
 * Schreibt Daten auf ein Massenspeichergeraet
 *
 * @param pos Startposition in Bytes
 * @param size Anzahl der zu schreibendes Bytes
 * @param src Buffer
 */
int cdi_storage_write(struct cdi_storage_device* device, uint64_t pos,
    size_t size, void* src)
{
    struct cdi_storage_driver* driver = reinterpret_cast<struct cdi_storage_driver*>(device->dev.driver);
    
    size_t block_size = device->block_size;
    uint64_t block_write_start = pos / block_size;
    uint8_t *source = reinterpret_cast<uint8_t*>(src);
    uint8_t buffer[block_size];
    size_t offset;
    size_t tmp_size;

    // Wenn die Startposition nicht auf einer Blockgrenze liegt, muessen wir
    // hier zuerst den ersten Block laden, die gewuenschten Aenderungen machen,
    // und den Block wieder Speichern.
    offset = (pos % block_size);
    if (offset != 0) {
        tmp_size = block_size - offset;
        tmp_size = (tmp_size > size ? size : tmp_size);

        if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
        memcpy(buffer + offset, source, tmp_size);

        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }

        size -= tmp_size;
        source += tmp_size;
        block_write_start++;
    }
    
    // Jetzt wird die Menge der ganzen Blocks auf einmal geschrieben, falls
    // welche existieren
    tmp_size = size / block_size;
    if (tmp_size != 0) {
        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, tmp_size, source)
            != 0) 
        {
            return -1;
        }
        size -= tmp_size * block_size;
        source += tmp_size * block_size;
        block_write_start += block_size;        
    }

    // Wenn der letzte Block nur teilweise beschrieben wird, geschieht das hier
    if (size != 0) {
        // Hier geschieht fast das Selbe wie oben beim ersten Block
        if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
        memcpy(buffer, source, size);

        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
    }
    return 0;
}
