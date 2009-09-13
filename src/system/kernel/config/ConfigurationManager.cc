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

size_t ConfigurationManager::createTable(String configStore, String table)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return 0;
    return backend->createTable(table);
}

void ConfigurationManager::insert(String configStore, String table, String key, ConfigValue &value)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return;
    return backend->insert(table, key, value);
}

ConfigValue &ConfigurationManager::select(String configStore, String table, String key)
{
    static ConfigValue v;
    
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return v;
    return backend->select(table, key);
}

void ConfigurationManager::watch(String configStore, String table, String key, ConfigurationWatcher watcher)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return;
    return backend->watch(table, key, watcher);
}

void ConfigurationManager::unwatch(String configStore, String table, String key, ConfigurationWatcher watcher)
{
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return;
    return backend->unwatch(table, key, watcher);
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
    // Lookup the backend
    ConfigurationBackend *backend = m_Backends.lookup(configStore);
    if(!backend)
        return;

    // Remove it from the list and free used memory
    m_Backends.remove(configStore);
    delete backend;
}

bool ConfigurationManager::backendExists(String configStore)
{
    return (m_Backends.lookup(configStore) != 0);
}
