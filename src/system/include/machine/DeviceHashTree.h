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
#ifndef _MACHINE_DEVICE_HASH_TREE_H
#define _MACHINE_DEVICE_HASH_TREE_H

#include <utilities/Tree.h>
#include <utilities/String.h>
class Device;

/**
 * DeviceHashTree: Name-based device access
 *
 * This class provides access to devices via a SHA1 hash generated from several
 * unique identifers of the device. These hashes are deterministic across boots
 * and are therefore ideal for use in addressing devices in userspace scripts
 * or in configuration files.
 */
class DeviceHashTree
{
    public:
        DeviceHashTree();
        virtual ~DeviceHashTree();

        static DeviceHashTree &instance()
        {
            return m_Instance;
        }

        /** Whether or not the hash tree is ready for use. */
        bool initialised()
        {
            return m_bInitialised;
        }

        /**
         * Fills the hash tree, starting at the given Device and recursively
         * traversing each device with children.
         */
        void fill(Device *root);

        /**
         * Adds a device, if it doesn't already exist
         */
        void add(Device *p);

        /** Gets a device from an integer hash */
        Device *getDevice(uint32_t hash);

        /** Gets a device from a string hash */
        Device *getDevice(String hash);

        /** Grabs the hash for a given device */
        size_t getHash(Device *p);

    private:

        static DeviceHashTree m_Instance;

        bool m_bInitialised;

        Tree<size_t, Device*> m_DeviceTree;
};

#endif