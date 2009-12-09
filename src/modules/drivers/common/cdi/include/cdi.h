/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

/** \addtogroup core */
/*\@{*/

#ifndef _CDI_H_
#define _CDI_H_

#include <stdint.h>

#include "cdi-osdep.h"
#include "cdi/lists.h"

typedef enum {
    CDI_UNKNOWN         = 0,
    CDI_NETWORK         = 1,
    CDI_STORAGE         = 2,
    CDI_SCSI            = 3,
    CDI_VIDEO           = 4,
    CDI_AUDIO           = 5,
    CDI_AUDIO_MIXER     = 6,
    CDI_USB_HCD         = 7,
    CDI_USB             = 8,
    CDI_FILESYSTEM      = 9,
} cdi_device_type_t;

struct cdi_driver;
struct cdi_device {
    cdi_device_type_t   type;
    const char*         name;
    struct cdi_driver*  driver;

    void*               backdev;
};

struct cdi_driver {
    cdi_device_type_t   type;
    const char*         name;
    cdi_list_t          devices;

    void (*init_device)(struct cdi_device* device);
    void (*remove_device)(struct cdi_device* device);

    int (*init)(void);
    int (*destroy)(void);
};

/**
 * Muss vor dem ersten Aufruf einer anderen CDI-Funktion aufgerufen werden.
 * Initialisiert interne Datenstruktur der Implementierung fuer das jeweilige
 * Betriebssystem und startet anschliessend alle Treiber.
 *
 * Ein wiederholter Aufruf bleibt ohne Effekt.
 */
void cdi_init(void);

/**
 * Diese Funktion wird von Treibern aufgerufen, nachdem ein neuer Treiber
 * hinzugefuegt worden ist.
 *
 * Sie registriert typischerweise die neu hinzugefuegten Treiber und/oder
 * Geraete beim Betriebssystem und startet damit ihre Ausfuehrung.
 *
 * Nach dem Aufruf dieser Funktion duerfen vom Treiber keine weiteren Befehle
 * ausgefuehrt werden, da nicht definiert ist, ob und wann die Funktion
 * zurueckkehrt.
 */
void cdi_run_drivers(void);

/**
 * Initialisiert die Datenstrukturen fuer einen Treiber
 * (erzeugt die devices-Liste)
 */
void cdi_driver_init(struct cdi_driver* driver);

/**
 * Deinitialisiert die Datenstrukturen fuer einen Treiber
 * (gibt die devices-Liste frei)
 */
void cdi_driver_destroy(struct cdi_driver* driver);

/**
 * Registriert den Treiber fuer ein neues Geraet
 *
 * @param driver Zu registierender Treiber
 */
void cdi_driver_register(struct cdi_driver* driver);

#endif

/*\@}*/

