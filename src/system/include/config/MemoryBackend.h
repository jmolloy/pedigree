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

    size_t write(String TableName, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t nBytes);
    size_t read(String TableName, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t maxBytes);

    String getTypeName();

private:

    // Value information
    struct ValueInfo
    {
        ValueInfo() : buffer(0), nBytes(0) {};
        ~ValueInfo() {};

        uintptr_t buffer;
        size_t nBytes;
    };

    // A Value name
    struct Value
    {
        Value() : m_ValueInfo() {};
        ~Value() {};

        RadixTree<ValueInfo*> m_ValueInfo;
    };

    // A Key
    struct Key
    {
        Key() : m_Values() {};
        ~Key() {};

        RadixTree<Value*> m_Values;
    };

    // A Table
    struct Table
    {
        Table() : m_Keys() {};
        ~Table() {};

        RadixTree<Key*> m_Keys;
    };

    // Our tables
    RadixTree<Table*> m_Tables;
};

#endif
