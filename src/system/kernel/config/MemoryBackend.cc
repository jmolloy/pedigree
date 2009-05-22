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

#include <config/MemoryBackend.h>

MemoryBackend::MemoryBackend(String configStore) :
    ConfigurationBackend(configStore), m_Tables()
{
}

MemoryBackend::~MemoryBackend()
{
    ConfigurationManager::instance().removeBackend(m_ConfigStore);
}

size_t MemoryBackend::write(String TableName, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t nBytes)
{
    // Sanity check to avoid wasted time
    if(!buffer || !nBytes)
        return 0;

    // Look up what we can, and allocate what we can't
    Table *table = 0;
    Key *key = 0;
    Value *value = 0;
    ValueInfo *info = 0;

    table = m_Tables.lookup(TableName);
    if(table)
    {
        key = table->m_Keys.lookup(KeyName);
        if(key)
        {
            value = key->m_Values.lookup(KeyValue);
            if(value)
            {
                info = value->m_ValueInfo.lookup(ValueName);
            }
        }
    }

    // Dependence goes backwards
    if(!table)
    {
        table = new Table;
        m_Tables.insert(TableName, table);
    }

    if(!key)
    {
        key = new Key;
        table->m_Keys.insert(KeyName, key);
    }

    if(!value)
    {
        value = new Value;
        key->m_Values.insert(KeyValue, value);
    }

    if(!info)
    {
        info = new ValueInfo;
        info->buffer = reinterpret_cast<uintptr_t>(new uint8_t[nBytes]);
        info->nBytes = nBytes;
        value->m_ValueInfo.insert(ValueName, info);
    }

    // Do we need to reallocate the info?
    if(info->nBytes != nBytes)
    {
        delete reinterpret_cast<uint8_t*>(info->buffer);
        info->buffer = reinterpret_cast<uintptr_t>(new uint8_t[nBytes]);
        info->nBytes = nBytes;
    }

    // Copy!
    memcpy(reinterpret_cast<void*>(info->buffer), reinterpret_cast<void*>(buffer), nBytes);
    return nBytes;
}

size_t MemoryBackend::read(String TableName, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t maxBytes)
{
    // Sanity check to avoid wasted time
    if(!buffer || !maxBytes)
        return 0;

    // Look up the table
    Table *table = m_Tables.lookup(TableName);
    if(!table)
        return 0;

    // Look up the key name
    Key *key = table->m_Keys.lookup(KeyName);
    if(!key)
        return 0;

    // Look up the key value
    Value *value = key->m_Values.lookup(KeyValue);
    if(!value)
        return 0;

    // Finally, look up the value itself
    ValueInfo *info = value->m_ValueInfo.lookup(ValueName);
    if(!info)
        return 0;

    // Copy
    size_t bytesToCopy = (info->nBytes >= maxBytes ? maxBytes : info->nBytes);
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(info->buffer), bytesToCopy);
    return bytesToCopy;
}

String MemoryBackend::getTypeName()
{
    return String("MemoryBackend");
}
