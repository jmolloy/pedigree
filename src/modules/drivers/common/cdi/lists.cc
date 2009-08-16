/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

typedef struct {
        struct list_node* anchor;
            unsigned long size;
} list_t;

extern "C"
{
list_t* list_create(void);
void list_destroy(list_t* list);
list_t* list_push(list_t* list, void* value);
void* list_pop(list_t* list);
int list_is_empty(list_t* list);
void* list_get_element_at(list_t* list, int index);
list_t* list_insert(list_t* list, int index, void* value);
void* list_remove(list_t* list, int index);
unsigned long list_size(list_t* list);
};


#include "cdi/lists.h"


struct cdi_list_implementation {
    struct list_node* anchor;
    size_t size;
};

/** 
 * Erzeugt eine neue Liste 
 */
cdi_list_t cdi_list_create()
{
    return reinterpret_cast<cdi_list_t>(list_create());
}

/** 
 * Gibt eine Liste frei (Werte der Listenglieder müssen bereits 
 * freigegeben sein) 
 */
void cdi_list_destroy(cdi_list_t list)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    list_destroy(l);
}

/**
 * Fuegt ein neues Element am Anfang der Liste ein
 */
cdi_list_t cdi_list_push(cdi_list_t list, void* value)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return reinterpret_cast<cdi_list_t>(list_push(l, value));
}

/**
 * Entfernt ein Element am Anfang der Liste und gibt seinen Wert zurueck
 */
void* cdi_list_pop(cdi_list_t list)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return list_pop(l); 
}

/**
 * Prueft, ob die Liste leer ist. Gibt 1 zurueck, wenn sie leer ist;
 * 0, wenn sie Elemente enthaelt
 */
size_t cdi_list_empty(cdi_list_t list)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return list_is_empty(l);
}

/**
 * Gibt ein Listenelement zurueck
 */
void* cdi_list_get(cdi_list_t list, size_t index)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return list_get_element_at(l, index);
}

/**
 * Fuegt ein neues Listenelement ein
 *
 * @param index Zukuenftiger Index des neu einzufuegenden Elements
 */
cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void* value)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return reinterpret_cast<cdi_list_t>(list_insert(l, index, value));
}

/**
 * Loescht ein Listenelement
 */
void* cdi_list_remove(cdi_list_t list, size_t index)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return list_remove(l, index);
}

/**
 * Gibt die Laenge der Liste zurueck
 */
size_t cdi_list_size(cdi_list_t list)
{
    list_t* l = reinterpret_cast<list_t*>(list);
    return list_size(l);
}
