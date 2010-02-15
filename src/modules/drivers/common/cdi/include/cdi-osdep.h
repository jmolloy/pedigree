/*
 * Copyright (c) 2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_OSDEP_H_
#define _CDI_OSDEP_H_

#include <Module.h>

struct cdi_driver;

/**
 * \german
 * Muss fuer jeden CDI-Treiber genau einmal verwendet werden, um ihn bei der
 * CDI-Bibliothek zu registrieren
 *
 * @param name Name des Treibers
 * @param drv Pointer auf eine Treiberbeschreibung (struct cdi_driver*)
 * @param deps Liste von Namen anderer Treiber, von denen der Treiber abhaengt
 * \endgerman
 *
 * \english
 * CDI_DRIVER shall be used exactly once for each CDI driver. It registers
 * the driver with the CDI library.
 *
 * @param name Name of the driver
 * @param drvobj A driver description (struct cdi_driver)
 * @param deps List of names of other drivers on which this driver depends
 * \endenglish
 */
#define CDI_DRIVER(name, drvobj, deps...) \
    void mod_entry() {(drvobj).drv.init(); cdi_pedigree_walk_dev_list_init((drvobj).drv);} \
    void mod_exit() {cdi_pedigree_walk_dev_list_destroy((drvobj).drv); (drvobj).drv.destroy();} \
    MODULE_NAME(name); \
    MODULE_ENTRY(mod_entry); \
    MODULE_EXIT(mod_exit); \
    MODULE_DEPENDS("cdi" deps);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \english
 * This function walks the list of devices stored by a driver and initialises
 * each one. It is called when the module is executed, directly after driver
 * initialisation.
 *
 * @param driver Driver struct containing the list of devices and function
 *               pointers.
 */
void cdi_pedigree_walk_dev_list_init(struct cdi_driver dev);

/**
 * \english
 * This function walks the list of devices stored by a driver and destroys
 * each one. It is called when the module is unloaded, right before driver
 * destruction.
 *
 * @param driver Driver struct containing the list of devices and function
 *               pointers.
 */
void cdi_pedigree_walk_dev_list_destroy(struct cdi_driver dev);

#ifdef __cplusplus
}; // extern "C"
#endif

/**
 * \english
 * OS-specific PCI metadata.
 * \endenglish
 */
typedef struct
{
    void *backdev;
} cdi_pci_device_osdep;

/**
 * \english
 * OS-specific DMA metadata.
 * \endenglish
 */
typedef struct
{
    void *region;
    void *realbuffer;
} cdi_dma_osdep;

/**
 * \german
 * OS-spezifische Daten fuer Speicherbereiche
 * \endgerman
 * \english
 * OS-specific data for memory areas.
 * \endenglish
 */
typedef struct {
    /// A MemoryRegion on the kernel heap
    void *pMemRegion;
} cdi_mem_osdep;

/**
 * Tyndur-specific, handling that here until it's fixed
 */
struct FILE
{
};

#endif
