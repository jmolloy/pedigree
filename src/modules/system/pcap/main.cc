/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <Log.h>
#include <Module.h>
#include <machine/Machine.h>
#include <compiler.h>
#include <time/Time.h>
#include <network-stack/Filter.h>

struct PcapHeader
{
    uint32_t magic;
    uint16_t major;
    uint16_t minor;
    uint32_t tz;
    uint32_t sigfig;
    uint32_t caplen;
    uint32_t network;
};

struct PcapRecord
{
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t stored_length;
    uint32_t orig_length;
};

#define PCAP_MAGIC 0xa1b2c3d4

#define PCAP_MAJOR 2
#define PCAP_MINOR 4

#define PCAP_NETWORK 1

static size_t g_FilterEntry = 0;

static Serial *getSerial() PURE;
static Serial *getSerial()
{
    // Ignore if we don't have a machine abstraction yet.
    if (!Machine::instance().isInitialised())
    {
        return 0;
    }

    // Serial port to write PCAP data to.
    if (Machine::instance().getNumSerial() < 3)
    {
        return 0;
    }

    return Machine::instance().getSerial(2);
}

static bool filterCallback(uintptr_t packet, size_t size)
{
    Serial *pSerial = getSerial();
    if (!pSerial)
    {
        return true;
    }

    static uint64_t time = 0;

    // Time::Timestamp time = Time::getTimeNanoseconds();

    // Don't care about timing in Wireshark but do care about ordering, and
    // so we want timestamps to always increase.
    time += Time::Multiplier::MILLISECOND;

    PcapRecord header;
    header.ts_sec = time / Time::Multiplier::SECOND;
    header.ts_usec = (time % Time::Multiplier::SECOND) / Time::Multiplier::MICROSECOND;
    header.stored_length = size;
    header.orig_length = size;

    // Write the pcap record header.
    const uint8_t *headerData = reinterpret_cast<const uint8_t *>(&header);
    for (size_t i = 0; i < sizeof(header); ++i)
    {
        pSerial->write(headerData[i]);
    }

    // Write the packet data now.
    const uint8_t *data = reinterpret_cast<const uint8_t *>(packet);
    for (size_t i = 0; i < size; ++i)
    {
        pSerial->write(data[i]);
    }

    // Always let the packet through.
    return true;
}

static bool entry()
{
    Serial *pSerial = getSerial();
    if (!pSerial)
    {
        NOTICE("pcap: could not find a useful serial port");
        return false;
    }

    g_FilterEntry = NetworkFilter::instance().installCallback(1, filterCallback);
    if (g_FilterEntry == static_cast<size_t>(-1))
    {
        NOTICE("pcap: could not install callback");
        return false;
    }

    PcapHeader header;
    header.magic = PCAP_MAGIC;
    header.major = PCAP_MAJOR;
    header.minor = PCAP_MINOR;
    header.tz = 0;
    header.sigfig = 0;
    header.caplen = 0xFFFF;
    header.network = PCAP_NETWORK;

    const uint8_t *data = reinterpret_cast<const uint8_t *>(&header);
    for (size_t i = 0; i < sizeof(header); ++i)
    {
        pSerial->write(data[i]);
    }

    return true;
}

static void exit()
{
    NetworkFilter::instance().removeCallback(1, g_FilterEntry);
}

MODULE_INFO("pcap", &entry, &exit, "network-stack");
