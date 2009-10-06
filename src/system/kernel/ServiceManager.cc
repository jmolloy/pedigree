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
#include "ServiceManager.h"

ServiceManager ServiceManager::m_Instance;

ServiceManager::ServiceManager() :
    m_Services()
{
}

ServiceManager::~ServiceManager()
{
    /// \todo Delete all the pointers!
}

ServiceFeatures *ServiceManager::enumerateOperations(String serviceName)
{
    InternalService *p = m_Services.lookup(serviceName);
    if(p)
        return p->pFeatures;
    else
        return 0;
}

void ServiceManager::addService(String serviceName, Service *s, ServiceFeatures *feats)
{
    InternalService *p = new InternalService;
    p->pService = s;
    p->pFeatures = feats;
    m_Services.insert(serviceName, p);
}

void ServiceManager::removeService(String serviceName)
{
    m_Services.remove(serviceName);
}

Service *ServiceManager::getService(String serviceName)
{
    InternalService *p = m_Services.lookup(serviceName);
    if(p)
        return p->pService;
    else
        return 0;
}
