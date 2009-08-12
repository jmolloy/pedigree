/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_LISTS_
#define _CDI_LISTS_
#include <stddef.h>
#include <stdint.h>

/**
 * Repraesentiert eine Liste.
 *
 * Der Felder der Struktur sind implementierungsabhaengig. Zum Zugriff auf
 * Listen muessen immer die spezifizierten Funktionen benutzt werden.
 */
typedef struct cdi_list_implementation* cdi_list_t;

/** 
 * Erzeugt eine neue Liste 
 *
 * @return Neu erzeugte Liste oder NULL, falls kein Speicher reserviert werden
 * konnte
 */
cdi_list_t cdi_list_create(void);

/** 
 * Gibt eine Liste frei (Werte der Listenglieder müssen bereits 
 * freigegeben sein) 
 */
void cdi_list_destroy(cdi_list_t list);

/**
 * Fuegt ein neues Element am Anfang (Index 0) der Liste ein
 *
 * @param list Liste, in die eingefuegt werden soll
 * @param value Einzufuegendes Element
 *
 * @return Die Liste, in die eingefuegt wurde, oder NULL, wenn das Element
 * nicht eingefuegt werden konnte (z.B. kein Speicher frei).
 */
cdi_list_t cdi_list_push(cdi_list_t list, void* value);

/**
 * Entfernt ein Element am Anfang (Index 0) der Liste und gibt seinen Wert 
 * zurueck.
 *
 * @param list Liste, aus der das Element entnommen werden soll
 * @return Das entfernte Element oder NULL, wenn die Liste leer war
 */
void* cdi_list_pop(cdi_list_t list);

/**
 * Prueft, ob die Liste leer ist. 
 *
 * @param list Liste, die ueberprueft werden soll
 * @return 1, wenn die Liste leer ist; 0, wenn sie Elemente enthaelt
 */
size_t cdi_list_empty(cdi_list_t list);

/**
 * Gibt ein Listenelement zurueck
 *
 * @param list Liste, aus der das Element gelesen werden soll
 * @param index Index des zurueckzugebenden Elements
 *
 * @return Das angeforderte Element oder NULL, wenn kein Element mit dem
 * angegebenen Index existiert.
 */
void* cdi_list_get(cdi_list_t list, size_t index);

/**
 * Fuegt ein neues Listenelement ein. Der Index aller Elemente, die bisher
 * einen groesseeren oder gleich grossen Index haben, verschieben sich
 * um eine Position nach hinten.
 *
 * @param list Liste, in die eingefuegt werden soll
 * @param index Zukuenftiger Index des neu einzufuegenden Elements
 * @param value Neu einzufuegendes Element
 *
 * @return Die Liste, in die eingefuegt wurde, oder NULL, wenn nicht eingefuegt
 * werden konnte (z.B. weil der Index zu gross ist)
 */
cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void* value);

/**
 * Loescht ein Listenelement
 *
 * @param list Liste, aus der entfernt werden soll
 * @param index Index des zu entfernenden Elements
 *
 * @return Das entfernte Element oder NULL, wenn kein Element mit dem
 * angegebenen Index existiert.
 */
void* cdi_list_remove(cdi_list_t list, size_t index);

/**
 * Gibt die Laenge der Liste zurueck
 *
 * @param list Liste, deren Laenge zurueckgegeben werden soll
 */
size_t cdi_list_size(cdi_list_t list);

#endif
