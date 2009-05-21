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

#include <config/ConfigurationManager.h>
#include <config/ConfigurationBackend.h>

#include <processor/types.h>

ConfigurationManager ConfigurationManager::m_Instance;

ConfigurationManager::ConfigurationManager() :
    m_Backends()
{
}

ConfigurationManager::~ConfigurationManager()
{
}

ConfigurationManager &ConfigurationManager::instance()
{
    return m_Instance;
}

size_t ConfigurationManager::write(
            String configStore,
            String Table,
            String KeyName,
            String KeyValue,
            String ValueName,
            uintptr_t buffer,
            size_t nBytes)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return 0;

    // Perform the write
    return backend->write(Table, KeyName, KeyValue, ValueName, buffer, nBytes);
}

size_t ConfigurationManager::read(
            String configStore,
            String Table,
            String KeyName,
            String KeyValue,
            String ValueName,
            uintptr_t buffer,
            size_t maxBytes)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return 0;

    // Perform the read
    return backend->read(Table, KeyName, KeyValue, ValueName, buffer, maxBytes);
}

bool ConfigurationManager::installBackend(ConfigurationBackend *pBackend, String configStore)
{
    // Check for sanity
    if(!pBackend)
        return false;

    // Get the real config store to use
    String realConfigStore = configStore;
    if(configStore.length() == 0)
        realConfigStore = pBackend->getConfigStore();

    // Install into the list
    if(!backendExists(realConfigStore))
    {
        m_Backends.insert(realConfigStore, pBackend);
        return true;
    }
    else
        return false;
}

void ConfigurationManager::removeBackend(String configStore)
{
    m_Backends.remove(configStore);
}

bool ConfigurationManager::backendExists(String configStore)
{
    return (m_Backends.lookup(configStore) != 0);
}
