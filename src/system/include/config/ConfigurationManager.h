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
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <processor/types.h>
#include <utilities/RadixTree.h>

class ConfigurationBackend;

/** The central manager for the Pedigree configuration system.
  *
  * This class provides a thin layer between the kernel and multiple
  * configuration backends.
  */
class ConfigurationManager
{
public:
    ConfigurationManager();
    virtual ~ConfigurationManager();

    static ConfigurationManager &instance();

    size_t write(String configStore, String Table, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t nBytes);
    size_t read(String configStore, String Table, String KeyName, String KeyValue, String ValueName, uintptr_t buffer, size_t maxBytes);

    bool installBackend(ConfigurationBackend *pBackend, String configStore = String(""));
    void removeBackend(String configStore);

    bool backendExists(String configStore);

private:

    static ConfigurationManager m_Instance;

    RadixTree<ConfigurationBackend*> m_Backends;
};

#endif
