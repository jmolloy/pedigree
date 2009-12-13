/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

//#include <stdlib.h>

#include <Module.h>

#include "cdi.h"
#include "cdi-osdep.h"
#include "cdi/lists.h"

static cdi_list_t drivers = NULL;

static void cdi_destroy(void);

/**
 * Muss vor dem ersten Aufruf einer anderen CDI-Funktion aufgerufen werden.
 * Initialisiert interne Datenstruktur der Implementierung fuer das jeweilige
 * Betriebssystem.
 */
void cdi_init(void)
{
    drivers = cdi_list_create();
}

void cdi_pedigree_walk_dev_list_init(struct cdi_driver dev)
{
    struct cdi_driver* driver = &dev;
    struct cdi_device* device;
    int i;
    for (i = 0; (device = reinterpret_cast<struct cdi_device*>(cdi_list_get(dev.devices, i))); i++) {
        device->driver = driver;

        if (driver->init_device) {
            driver->init_device(device);
        }
    }
}

void cdi_pedigree_walk_dev_list_destroy(struct cdi_driver dev)
{
    struct cdi_driver* driver = &dev;
    struct cdi_device* device;
    int i;
    for (i = 0; (device = reinterpret_cast<struct cdi_device*>(cdi_list_get(dev.devices, i))); i++) {
        if (driver->remove_device) {
            driver->remove_device(device);
        }
    }
}

/**
 * Wird bei der Deinitialisierung aufgerufen
 */
static void cdi_destroy(void) 
{
    // Drivers are already destroyed by the module exit function
}

/**
 * Fuehrt alle registrierten Treiber aus. Nach dem Aufruf dieser Funktion 
 * duerfen keine weiteren Befehle ausgefuehrt werden, da nicht definiert ist,
 * ob und wann die Funktion zurueckkehrt.
 */
void cdi_run_drivers(void)
{
    // Geraete initialisieren
    struct cdi_driver* driver;
    int i;
    for (i = 0; (driver = reinterpret_cast<struct cdi_driver*>(cdi_list_get(drivers, i))); i++) {
        cdi_pedigree_walk_dev_list_init(*driver);
    }
}

/**
 * Initialisiert die Datenstrukturen fuer einen Treiber
 */
void cdi_driver_init(struct cdi_driver* driver)
{
    driver->devices = cdi_list_create();
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Treiber
 */
void cdi_driver_destroy(struct cdi_driver* driver)
{
    cdi_list_destroy(driver->devices);
}

/**
 * Registriert den Treiber fuer ein neues Geraet
 *
 * @param driver Zu registierender Treiber
 */
void cdi_driver_register(struct cdi_driver* driver)
{
    cdi_list_push(drivers, driver);
}

MODULE_NAME("cdi");
MODULE_ENTRY(&cdi_init);
MODULE_EXIT(&cdi_destroy);
MODULE_DEPENDS("network-stack", "VFS");
