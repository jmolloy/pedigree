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
#ifndef MEMORY_BACKEND_H
#define MEMORY_BACKEND_H

#include <config/ConfigurationManager.h>
#include <config/ConfigurationBackend.h>
#include <processor/types.h>


/** Memory configuration backend. Stores everything in RAM,
  * won't save to file. Good for runtime-only information.
  */
class MemoryBackend : public ConfigurationBackend
{
public:
    MemoryBackend(String configStore);
    virtual ~MemoryBackend();

    virtual size_t createTable(String table);
    /** Inserts the value 'value' into the table 'table', with its key as 'key' */
    virtual void insert(String table, String key, ConfigValue &value);
    /** Returns the value in table, with key matching 'key', or zero. */
    virtual ConfigValue &select(String table, String key);

    /** Watch a specific table entry. */
    virtual void watch(String table, String key, ConfigurationWatcher watcher);
    /** Remove a watcher from a table entry. */
    virtual void unwatch(String table, String key, ConfigurationWatcher watcher);

    String getTypeName();

private:

    // A Table
    struct Table
    {
        Table() : m_Rows() {};
        ~Table() {};

        RadixTree<ConfigValue*> m_Rows;
    };

    // Our tables
    RadixTree<Table*> m_Tables;
};

#endif
