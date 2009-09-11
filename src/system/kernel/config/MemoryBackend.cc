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

size_t MemoryBackend::createTable(String table)
{
    m_Tables.insert(table, new Table());
    return 0;
}

void MemoryBackend::insert(String table, String key, ConfigValue &value)
{
    Table *pT = m_Tables.lookup(table);
    if (!pT) return;

    ConfigValue *pConfigValue = new ConfigValue(value);
    pT->m_Rows.insert(key, pConfigValue);
}

ConfigValue &MemoryBackend::select(String table, String key)
{
    static ConfigValue v;
    v.type = Invalid;

    Table *pT = m_Tables.lookup(table);
    if (!pT) return v;

    ConfigValue *pV = pT->m_Rows.lookup(key);
    if (pV)
        return *pV;
    else
        return v;
}

void MemoryBackend::watch(String table, String key, ConfigurationWatcher watcher)
{
    Table *pT = m_Tables.lookup(table);
    if (!pT) return;

    ConfigValue *pV = pT->m_Rows.lookup(key);
    if (pV)
    {
        for (int i = 0; i < MAX_WATCHERS; i++)
        {
            if (pV->watchers[i] == 0)
            {
                pV->watchers[i] = watcher;
                break;
            }
        }
    }
}

void MemoryBackend::unwatch(String table, String key, ConfigurationWatcher watcher)
{
    Table *pT = m_Tables.lookup(table);
    if (!pT) return;

    ConfigValue *pV = pT->m_Rows.lookup(key);
    if (pV)
    {
        for (int i = 0; i < MAX_WATCHERS; i++)
        {
            if (pV->watchers[i] == watcher)
            {
                pV->watchers[i] = 0;
                break;
            }
        }
    }
}

String MemoryBackend::getTypeName()
{
    return String("MemoryBackend");
}
