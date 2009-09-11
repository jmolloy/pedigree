/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef CONFIG_BACKEND_H
#define CONFIG_BACKEND_H

#include <processor/types.h>

#include <config/ConfigurationManager.h>

/** A configuration backend for the Pedigree configuration system.
  *
  * By subclassing this class, it is possible to handle different
  * methods for configuration (for instance, a backend for SQL,
  * flat files, and pure memory access).
  */
class ConfigurationBackend
{
public:
    ConfigurationBackend(String configStore);
    virtual ~ConfigurationBackend();

    virtual size_t createTable(String table) =0;
    /** Inserts the value 'value' into the table 'table', with its key as 'key' */
    virtual void insert(String table, String key, ConfigValue &value) =0;
    /** Returns the value in table, with key matching 'key', or zero. */
    virtual ConfigValue &select(String table, String key) =0;

    /** Watch a specific table entry. */
    virtual void watch(String table, String key, ConfigurationWatcher watcher) =0;
    /** Remove a watcher from a table entry. */
    virtual void unwatch(String table, String key, ConfigurationWatcher watcher) =0;

    virtual String getConfigStore();

    virtual String getTypeName() =0;

protected:

    String m_ConfigStore;
};

#endif
