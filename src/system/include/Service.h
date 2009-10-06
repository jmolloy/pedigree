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

#ifndef SERVICE_H
#define SERVICE_H

#include <processor/types.h>
#include <ServiceFeatures.h>

/// \todo Integrate with the Event system somehow

/** Service
 *
 *  A Service is a method through which a certain component of the system
 *  can communicate and perform actions within other components without
 *  requiring any knowledge of the internals of the component.
 *
 *  An example of the use of a Service is the partitioner, which can be
 *  notified of Disk changes (LoDisk, plug & play, hotplug drives) and
 *  update the partition tree accordingly.
 */
class Service
{
    public:
        Service() {};
        virtual ~Service() {};

        /** serve: Interface through which clients interact with the Service */
        virtual bool serve(ServiceFeatures::Type type, void *pData, size_t dataLen) = 0;
};

#endif
