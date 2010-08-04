/*
 * Copyright (c) 2010 Matthew Iselin
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
#include "Dm9601.h"
#include <Log.h>

bool Dm9601::send(size_t nBytes, uintptr_t buffer)
{
    return false;
}

bool Dm9601::setStationInfo(StationInfo info)
{
    // Free the old DNS server list, if there is one
    if(m_StationInfo.dnsServers)
        delete [] m_StationInfo.dnsServers;

    // MAC isn't changeable, so set it all manually
    m_StationInfo.ipv4 = info.ipv4;
    NOTICE("DM9601: Setting ipv4, " << info.ipv4.toString() << ", " << m_StationInfo.ipv4.toString() << "...");
    m_StationInfo.ipv6 = info.ipv6;

    m_StationInfo.subnetMask = info.subnetMask;
    NOTICE("DM9601: Setting subnet mask, " << info.subnetMask.toString() << ", " << m_StationInfo.subnetMask.toString() << "...");
    m_StationInfo.gateway = info.gateway;
    NOTICE("DM9601: Setting gateway, " << info.gateway.toString() << ", " << m_StationInfo.gateway.toString() << "...");

    // Callers do not free their dnsServers memory
    m_StationInfo.dnsServers = info.dnsServers;
    m_StationInfo.nDnsServers = info.nDnsServers;
    NOTICE("DM9601: Setting DNS servers [" << Dec << m_StationInfo.nDnsServers << Hex << " servers being set]...");

    return true;
}

StationInfo Dm9601::getStationInfo()
{
    return m_StationInfo;
}

/// Reads data from a register into a buffer
ssize_t Dm9601::readRegister(uint8_t reg, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(RequestType::Vendor | RequestDirection::In, ReadRegister, 0, reg, nBytes, buffer);
}

/// Writes data from a buffer to a register
ssize_t Dm9601::writeRegister(uint8_t reg, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(RequestType::Vendor | RequestDirection::Out, WriteRegister, 0, reg, nBytes, buffer);
}

/// Writes a single 8-bit value to a register
ssize_t Dm9601::writeRegister(uint8_t reg, uint8_t data)
{
    return controlRequest(RequestType::Vendor | RequestDirection::Out, WriteRegister1, data, reg);
}

/// Reads data from device memory into a buffer
ssize_t Dm9601::readMemory(uint16_t offset, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(RequestType::Vendor | RequestDirection::In, ReadMemory, 0, offset, nBytes, buffer);
}

/// Writes data from a buffer into device memory
ssize_t Dm9601::writeMemory(uint16_t offset, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(RequestType::Vendor | RequestDirection::Out, WriteMemory, 0, offset, nBytes, buffer);
}

/// Writes a single 8-bit value into device memory
ssize_t Dm9601::writeMemory(uint16_t offset, uint8_t data)
{
    return controlRequest(RequestType::Vendor | RequestDirection::Out, WriteMemory1, data, offset);
}

